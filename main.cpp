/*
 * Realsense D400 infrared stream to H.264 with VAAPI encoding
 *
 * Copyright 2019 (C) Bartosz Meglicki <meglickib@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

/* This program is example how to use:
 * - VAAPI to hardware encode
 * - Realsense D400 greyscale infrared stream
 * - to H.264 raw video
 * - stored to disk as example
 *
 * See README.md for the details!
 *
 * Dependencies:
 * - librealsense2
 * - ffmpeg (libavcodec, libavutil)
 *
 * Building:
 * g++ main.cpp -std=c++11 -lrealsense2 -lavcodec -lavutil -o realsense-ir-to-vaapi-h264
 *
 * Running:
 * realsense-ir-to-vaapi-h264 width height framerate nr_of_seconds optional-device
 *
 * e.g.
 * realsense-ir-to-vaapi-h264 640 360 30 5
 * realsense-ir-to-vaapi-h264 640 360 30 5 /dev/dri/renderD128
 *
 * width and height have to be supported by D400 camera and H.264, framerate by D400 camera
 *
 * Testing:
 * ffplay output.h264
 */

// Realsense API
#include <librealsense2/rs.hpp>

// FFmpeg
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext.h>
#include <libavutil/pixdesc.h>
}

#include <fstream>
#include <iostream>
using namespace std;

// user input (from command line)
struct input_args
{
    int width;
    int height;
    int framerate;
    int seconds;
    const char *device; //NULL or device, e.g. "/dev/dri/renderD128"
};

// all FFmpeg data that has to be passed around
struct av_args
{
    AVBufferRef* hw_device_ctx;
    AVCodecContext* avctx;
};

bool main_loop(const input_args& input, rs2::pipeline& pipe, AVCodecContext* avctx, ofstream& out_file);
bool encode_and_write_frame(AVCodecContext* avctx, AVFrame* frame, ofstream& out_file);
void init_realsense(rs2::pipeline& pipe, const input_args& input);
bool init_av(struct av_args* av, const input_args& input);
int init_hwframes_context(av_args* av, const input_args& input);
void close_av(av_args* av);
int process_user_input(int argc, char* argv[], input_args* input);

int main(int argc, char* argv[])
{
    ofstream out_file("output.h264", ofstream::binary);
    av_args av = { NULL, NULL };
    input_args input;
    rs2::pipeline pipe;

    if(process_user_input(argc, argv, &input) < 0)
        return 1;

    if(!out_file)
        return 2;

    init_realsense(pipe, input);

    if(!init_av(&av, input))
    {
        close_av(&av);
        return 3;
    }

    bool status=main_loop(input, pipe, av.avctx, out_file);

    close_av(&av);

    out_file.close();

    if(status)
    {
        cout << "Finished successfully." << endl;
        cout << "Test with: " << endl << endl << "ffplay output.h264" << endl;
    }

    return 0;
}

//true on success, false on failure
bool main_loop(const input_args& input, rs2::pipeline& pipe, AVCodecContext* avctx, ofstream& out_file)
{
    const int frames = input.seconds * input.framerate;
    int err = 0, f;
    AVFrame *sw_frame = NULL, *hw_frame = NULL;

    if(!(sw_frame = av_frame_alloc()))
    {
        cerr << "av_frame_alloc not enough memory" << endl;
        return false;
    }

    sw_frame->width = input.width;
    sw_frame->height = input.height;
    sw_frame->format = AV_PIX_FMT_NV12;

    uint8_t *color_data = NULL; //data of dummy color plane for NV12

    for(f = 0; f < frames; ++f)
    {
        rs2::frameset frameset = pipe.wait_for_frames();
        rs2::video_frame ir_frame = frameset.get_infrared_frame(1);

        if(!color_data)
        {   //prepare dummy color plane for NV12 format, half the size of Y
            //we can't alloc it in advance, this is the first time we know realsense stride
            int size = ir_frame.get_stride_in_bytes()*ir_frame.get_height()/2;
            color_data = new uint8_t[size];
            memset(color_data, 128, size);
        }
        //supply realsense frame data as ffmpeg frame data
        sw_frame->linesize[0] = sw_frame->linesize[1] =  ir_frame.get_stride_in_bytes();
        sw_frame->data[0] = (uint8_t*) ir_frame.get_data();
        sw_frame->data[1] = color_data;

        if(!(hw_frame = av_frame_alloc()))
        {
            cerr << "av_frame_alloc not enough memory" << endl;
            break;
        }
        if((err = av_hwframe_get_buffer(avctx->hw_frames_ctx, hw_frame, 0)) < 0)
        {
            cerr << "av_hwframe_get_buffer error" << endl;
            break;
        }
        if(!hw_frame->hw_frames_ctx)
        {
            cerr << "hw_frame->hw_frames_ctx not enough memory" << endl;
            break;
        }

        if((err = av_hwframe_transfer_data(hw_frame, sw_frame, 0)) < 0)
        {
            cerr << "error while transferring frame data to surface" << endl;
            break;
        }

        cout << (f + 1) << ": width " << ir_frame.get_width() << " height " << ir_frame.get_height()
             << " stride=" << ir_frame.get_stride_in_bytes() << " bytes "
             << ir_frame.get_stride_in_bytes() * ir_frame.get_height();

        if( !encode_and_write_frame(avctx, hw_frame, out_file) )
        {
            cerr << "failed to encode/write frame" << endl;
            break;
        }
        av_frame_free(&hw_frame);
    }

    av_frame_free(&sw_frame);
    av_frame_free(&hw_frame);
    delete [] color_data;

    encode_and_write_frame(avctx, NULL, out_file);

    //all the requested frames processed?
    return f==frames;
}

// true on success, false on error
bool encode_and_write_frame(AVCodecContext* avctx, AVFrame* frame, ofstream& out_file)
{
    int ret = 0;
    AVPacket enc_pkt;

    av_init_packet(&enc_pkt);
    enc_pkt.data = NULL;
    enc_pkt.size = 0;

    if((ret = avcodec_send_frame(avctx, frame)) < 0)
    {
        cerr << "send_frame error" << endl;
        return false;
    }

    while( (ret = avcodec_receive_packet(avctx, &enc_pkt)) == 0)
    {   //finally we have H.264 data in enc_pkt
        cout << " encoded in: " << enc_pkt.size << endl;
		//do something with the data - here just dump to raw H.264 file
        out_file.write((const char*)enc_pkt.data, enc_pkt.size);
    }

    //EAGAIN means that we need to supply more data, otherwise we got and error
    return ret == AVERROR(EAGAIN);
}

void init_realsense(rs2::pipeline& pipe, const input_args& input)
{
    rs2::config cfg;
    // depth stream seems to be required for infrared to work
    cfg.enable_stream(RS2_STREAM_DEPTH, input.width, input.height, RS2_FORMAT_Z16, input.framerate);
    cfg.enable_stream(RS2_STREAM_INFRARED, 1, input.width, input.height, RS2_FORMAT_Y8, input.framerate);

    rs2::pipeline_profile profile = pipe.start(cfg);
}

// false on failure, true on success
bool init_av(struct av_args* av, const input_args& input)
{
    int err;

    AVCodec* codec = NULL;

    avcodec_register_all();
    av_log_set_level(AV_LOG_VERBOSE);

    if((err = av_hwdevice_ctx_create(&av->hw_device_ctx, AV_HWDEVICE_TYPE_VAAPI, input.device, NULL, 0) < 0))
    {
        cerr << "failed to create a VAAPI device" << endl;
        return false;
    }

    if(!(codec = avcodec_find_encoder_by_name("h264_vaapi")))
    {
        cerr << "could not find encoder" << endl;
        return false;
    }

    if(!(av->avctx = avcodec_alloc_context3(codec)))
    {
        cerr << "unable to alloc codec context" << endl;
        return false;
    }

    av->avctx->width = input.width;
    av->avctx->height = input.height;
    av->avctx->time_base = (AVRational){ 1, input.framerate };
    av->avctx->framerate = (AVRational){ input.framerate, 1 };
    av->avctx->sample_aspect_ratio = (AVRational){ 1, 1 };
    av->avctx->pix_fmt = AV_PIX_FMT_VAAPI;

    if((err = init_hwframes_context(av, input)) < 0)
    {
        cerr << "failed to set hwframe context" << endl;
        return false;
    }

    if((err = avcodec_open2(av->avctx, codec, NULL)) < 0)
    {
        cerr << "cannot open video encoder codec" << endl;
        return false;
    }
    return true;
}
int init_hwframes_context(av_args* av, const input_args& input)
{
    AVBufferRef* hw_frames_ref;
    AVHWFramesContext* frames_ctx = NULL;
    int err = 0;

    if(!(hw_frames_ref = av_hwframe_ctx_alloc(av->hw_device_ctx)))
    {
        cerr << "failed to create VAAPI frame context" << endl;
        return -1;
    }
    frames_ctx = (AVHWFramesContext*)(hw_frames_ref->data);
    frames_ctx->format = AV_PIX_FMT_VAAPI;
    frames_ctx->sw_format = AV_PIX_FMT_NV12;
    frames_ctx->width = input.width;
    frames_ctx->height = input.height;
    frames_ctx->initial_pool_size = 20;
    if((err = av_hwframe_ctx_init(hw_frames_ref)) < 0)
    {
        cerr << "failed to initialize VAAPI frame context" << endl;
        av_buffer_unref(&hw_frames_ref);
        return err;
    }
    av->avctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
    if(!av->avctx->hw_frames_ctx)
        err = AVERROR(ENOMEM);

    av_buffer_unref(&hw_frames_ref);
    return err;
}

void close_av(av_args* av)
{
    avcodec_free_context(&av->avctx);
    av_buffer_unref(&av->hw_device_ctx);
}

int process_user_input(int argc, char* argv[], input_args* input)
{
    if(argc < 5)
    {
        cerr << "Usage: " << argv[0] << " <width> <height> <framerate> <seconds> [device]" << endl;
        cerr << endl << "examples: " << endl;
        cerr << argv[0] << " 640 360 30 5" << endl;
        cerr << argv[0] << " 640 360 30 5 /dev/dri/renderD128" << endl;

        return -1;
    }

    input->width = atoi(argv[1]);
    input->height = atoi(argv[2]);
    input->framerate = atoi(argv[3]);
    input->seconds = atoi(argv[4]);
    input->device = argv[5]; //NULL as last argv argument, or device path
    return 0;
}

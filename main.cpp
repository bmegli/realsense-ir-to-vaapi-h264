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

// Hardware Video Encoder
#include "hve.h"

// Realsense API
#include <librealsense2/rs.hpp>

#include <fstream>
#include <iostream>
using namespace std;

//user supplied input
struct input_args
{
	int width;
	int height;
	int framerate;
	int seconds;
};

bool main_loop(const input_args& input, rs2::pipeline& realsense, hve *avctx, ofstream& out_file);
void dump_frame_info(rs2::video_frame &frame);
void init_realsense(rs2::pipeline& pipe, const input_args& input);
int process_user_input(int argc, char* argv[], input_args* input, hve_config *config);

int main(int argc, char* argv[])
{
	struct hve *hardware_encoder;
	struct hve_config hardware_config = {0};
	struct input_args user_input = {0};

	ofstream out_file("output.h264", ofstream::binary);
	rs2::pipeline realsense;

	if(process_user_input(argc, argv, &user_input, &hardware_config) < 0)
		return 1;

	if(!out_file)
		return 2;

	init_realsense(realsense, user_input);

	if( (hardware_encoder = hve_init(&hardware_config)) == NULL)
		return 3;
	
	bool status=main_loop(user_input, realsense, hardware_encoder, out_file);

	hve_close(hardware_encoder);

	out_file.close();

	if(status)
	{
		cout << "Finished successfully." << endl;
		cout << "Test with: " << endl << endl << "ffplay output.h264" << endl;
	}

	return 0;
}

//true on success, false on failure
bool main_loop(const input_args& input, rs2::pipeline& realsense, hve *he, ofstream& out_file)
{
	const int frames = input.seconds * input.framerate;
	int f, failed;
	hve_frame frame = {0};
	uint8_t *color_data = NULL; //data of dummy color plane for NV12
	AVPacket *packet;
	
	for(f = 0; f < frames; ++f)
	{
		rs2::frameset frameset = realsense.wait_for_frames();
		rs2::video_frame ir_frame = frameset.get_infrared_frame(1);

		if(!color_data)
		{   //prepare dummy color plane for NV12 format, half the size of Y
			//we can't alloc it in advance, this is the first time we know realsense stride
			int size = ir_frame.get_stride_in_bytes()*ir_frame.get_height()/2;
			color_data = new uint8_t[size];
			memset(color_data, 128, size);
		}
		
		//supply realsense frame data as ffmpeg frame data
		frame.linesize[0] = frame.linesize[1] =  ir_frame.get_stride_in_bytes();
		frame.data[0] = (uint8_t*) ir_frame.get_data();
		frame.data[1] = color_data;

		dump_frame_info(ir_frame);

		if(hve_send_frame(he, &frame) != HVE_OK)
		{
			cerr << "failed to send frame to hardware" << endl;
			break;
		}
		
		while( (packet=hve_receive_packet(he, &failed)) )
		{ //do something with the data - here just dump to raw H.264 file
			cout << " encoded in: " << packet->size;
			out_file.write((const char*)packet->data, packet->size);
		}
		
		if(failed != HVE_OK)
		{
			cerr << "failed to encode frame" << endl;
			break;
		}	
	}
	
	//flush the encoder by sending NULL frame
	hve_send_frame(he, NULL);
	//drain the encoder from buffered frames
	while( (packet=hve_receive_packet(he, &failed)) )
	{ 
		cout << endl << "encoded in: " << packet->size;
		out_file.write((const char*)packet->data, packet->size);
	}
	cout << endl;
		
	//all the requested frames processed?
	return f==frames;
}
void dump_frame_info(rs2::video_frame &f)
{
	cout << endl << f.get_frame_number ()
		<< ": width " << f.get_width() << " height " << f.get_height()
		<< " stride=" << f.get_stride_in_bytes() << " bytes "
		<< f.get_stride_in_bytes() * f.get_height();
}

void init_realsense(rs2::pipeline& pipe, const input_args& input)
{
	rs2::config cfg;
	// depth stream seems to be required for infrared to work
	cfg.enable_stream(RS2_STREAM_DEPTH, input.width, input.height, RS2_FORMAT_Z16, input.framerate);
	cfg.enable_stream(RS2_STREAM_INFRARED, 1, input.width, input.height, RS2_FORMAT_Y8, input.framerate);

	rs2::pipeline_profile profile = pipe.start(cfg);
}

int process_user_input(int argc, char* argv[], input_args* input, hve_config *config)
{
	if(argc < 5)
	{
		cerr << "Usage: " << argv[0] << " <width> <height> <framerate> <seconds> [device]" << endl;
		cerr << endl << "examples: " << endl;
		cerr << argv[0] << " 640 360 30 5" << endl;
		cerr << argv[0] << " 640 360 30 5 /dev/dri/renderD128" << endl;

		return -1;
	}

	config->width = input->width = atoi(argv[1]);
	config->height = input->height = atoi(argv[2]);
	config->framerate = input->framerate = atoi(argv[3]);
	
	input->seconds = atoi(argv[4]);
	
	config->device = argv[5]; //NULL as last argv argument, or device path
	
	return 0;
}

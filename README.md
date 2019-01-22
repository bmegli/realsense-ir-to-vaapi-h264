# realsense-ir-to-vaapi-h264

 This program is example how to use:
 - VAAPI through FFmpeg to hardware encode
 - Realsense D400 greyscale infrared stream 
 - to H.264 raw video
 - stored to disk as example

## CPU usage

As reported by `htop` (percentage used, 100% would mean core fully utilzed).

| Platform               | CPU       |  640x480 | 1280x720 |
|------------------------|-----------|----------|----------|
| Latte Panda Alpha      | M3-7Y30   |  15%     |   25%    |
| High end laptop (2017) | i7-7820HK |  10%     |   12%    |

## Platforms 

Unix-like operating systems (e.g. Linux).
Tested on Ubuntu 18.04.

## Hardware

- D400 series camera
- Intel VAAPI compatible hardware encoder ([Quick Sync Video](https://ark.intel.com/Search/FeatureFilter?productType=processors&QuickSyncVideo=true))

Tested with D435 camera. There is possibility that it will also work with Amd/Nvidia hardware.

## What it does

- process user input (width, height, framerate, time to capture)
- init file for raw H.264 output
- init Realsense D400 device
- init VAAPI encoder with FFmpeg
- read greyscale IR data from the camera
- encode to H.264
- write to raw H.264 file
- cleanup

Currently VAAPI NV12 Y is filled with infrared greyscale and color plane is filled with constant value.

## Dependencies

Library depends on:
- [librealsense2](https://github.com/IntelRealSense/librealsense) 
- FFmpeg avcodec and avutil (tested with 3.4 version)

Install RealSense™ SDK 2.0 as described on [github](https://github.com/IntelRealSense/librealsense) 

Works with system FFmpeg on Ubuntu 18.04.


## Building Instructions

Tested on Ubuntu 18.04.

``` bash
# update package repositories
sudo apt-get update 
# get avcodec and avutil (and ffmpeg for testing)
sudo apt-get install ffmpeg libavcodec-dev libavutil-dev
# get compilers and make
sudo apt-get install build-essential
# get git
sudo apt-get install git
# clone the repository
git clone https://github.com/bmegli/realsense-ir-to-vaapi-h264.git

# finally build the program
cd realsense-ir-to-vaapi-h264
g++ main.cpp -std=c++11 -lrealsense2 -lavcodec -lavutil -o realsense-ir-to-vaapi-h264
```

## Running 

``` bash
# realsense-ir-to-vaapi-h264 width height framerate nr_of_seconds [device]
# e.g
./realsense-ir-to-vaapi-h264 640 360 30 5
```

Details:
- width and height have to be supported by D400 camera and H.264
- framerate has to be supported by D400 camera

### Troubleshooting

If you have multiple VAAPI devices you may have to specify Intel directly.

Check with 
```bash
sudo apt-get install vainfo
# try the devices you have in /dev/dri/ path
vainfo --display drm --device /dev/dri/renderD128
```

Once you identify your Intel device run the program, e.g.

```bash
./realsense-ir-to-vaapi-h264 640 360 30 5 /dev/dri/renderD128
```

## Testing

Play result raw H.264 file with FFmpeg:

``` bash
# output goes to output.h264 file 
ffplay output.h264
```

## License

Library is licensed under Mozilla Public License, v. 2.0

This is similiar to LGPL but more permissive:
- you can use it as LGPL in prioprietrary software
- unlike LGPL you may compile it statically with your code

Like in LGPL, if you modify this library, you have to make your changes available.
Making a github fork of the library with your changes satisfies those requirements perfectly.

## Additional information

High H.264 profile supports Monochrome Video Format (4:0:0) so there may be room for improvement (I am not sure VAAPI supports it).



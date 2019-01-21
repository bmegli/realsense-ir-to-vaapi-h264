# realsense-ir-to-vaapi-h264

 This program is example how to use:
 - VAAPI through FFmpeg to hardware encode
 - Realsense D400 infrared stream 
 - to H.264 raw video
 - stored to disk as example

On my laptop it uses around 10% CPU at 1280x720.

## Platforms 

Instructions focus on Linux. Tested on Ubuntu 18.04.

The code is platform independent apart from device passed to hardware context (hardcoded string).

## Hardware

- D400 series camera
- VAAPI compatible hardware encoder

Tested with D435 camera and Intel Quick Sync VAAPI device.

## What it does

- process user input (width, height, framerate, time to capture)
- init file for raw H.264 output
- init Realsense D400 device
- init VAAPI encoder with FFmpeg
- read, encode & write
- cleanup

Currently NV12 Y is filled with infrared greyscale and color plane is filled with constant value.

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
# realsense-ir-to-vaapi-h264 width height framerate nr_of_seconds
# e.g
./realsense-ir-to-vaapi-h264 640 360 30 5
```

Details:
- width and height have to be supported by D400 camera and H.264
- framerate has to be supported by D400 camera


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



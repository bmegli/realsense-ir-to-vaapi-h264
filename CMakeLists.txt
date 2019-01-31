cmake_minimum_required(VERSION 3.0)

project(
    realsense-ir-to-vaapi-h264
)

add_library(hve SHARED hardware-video-encoder/hve.c)

add_executable(realsense-ir-to-vaapi-h264 main.cpp)
target_include_directories(realsense-ir-to-vaapi-h264 PRIVATE hardware-video-encoder)
target_link_libraries(realsense-ir-to-vaapi-h264 hve avcodec avutil realsense2)

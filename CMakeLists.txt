cmake_minimum_required(VERSION 3.10)
project(rtsp_crop_streamer)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/decoder
    ${CMAKE_SOURCE_DIR}/encoder
    ${CMAKE_SOURCE_DIR}/crop
    ${CMAKE_SOURCE_DIR}/rtsp_input
    ${CMAKE_SOURCE_DIR}/rtsp_output
    /usr/include/rga 
)

# Source files
set(SOURCES
    main.cpp
    decoder/MppDecoder.cpp
    encoder/MppEncoder.cpp
    crop/Im2dCrop.cpp
    rtsp_input/LiveRtspInput.cpp
    rtsp_output/LiveRtspOutput.cpp
)

# Libraries
find_library(MPP_LIB rockchip_mpp REQUIRED)
find_library(IM2D_LIB rga REQUIRED) 
#find_library(IM2D_LIB im2d REQUIRED)

# live555 components
find_path(LIVE555_INCLUDE_DIR liveMedia.hh PATH_SUFFIXES liveMedia)
find_library(LIVEMEDIA_LIB liveMedia)
find_library(GROUPSOCK_LIB groupsock)
find_library(USAGEENV_LIB UsageEnvironment)
find_library(BASICTASK_LIB BasicUsageEnvironment)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${LIVE555_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    ${MPP_LIB}
    ${IM2D_LIB}
    ${LIVEMEDIA_LIB}
    ${GROUPSOCK_LIB}
    ${USAGEENV_LIB}
    ${BASICTASK_LIB}
    pthread
)

cmake_minimum_required(VERSION 3.10)
project(rtsp_crop_streamer)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/decoder
    ${CMAKE_SOURCE_DIR}/encoder
    ${CMAKE_SOURCE_DIR}/crop
    ${CMAKE_SOURCE_DIR}/rtsp_client
    ${CMAKE_SOURCE_DIR}/rtsp_server
    /usr/include/rga 
    /usr/local/include
    /usr/local/include/rockchip
    /usr/local/include/liveMedia
    /usr/local/include/groupsock
    /usr/local/include/UsageEnvironment
    /usr/local/include/BasicUsageEnvironment
)

link_directories(/usr/local/lib)

# Source files
set(SOURCES
    main.cpp
    decoder/MppDecoder.cpp
    encoder/MppEncoder.cpp
    crop/RgaCrop.cpp # <== RgaCrop ishlatyapmiz  
    rtsp_client/LiveRtspInput.cpp
    rtsp_server/LiveRtspOutput.cpp
)

# Libraries
find_library(MPP_LIB rockchip_mpp REQUIRED)
find_library(RGA_LIB NAMES rga Rga REQUIRED PATHS /usr/lib /usr/local/lib)
# find_library(IM2D_LIB rga REQUIRED) 
#find_library(IM2D_LIB im2d REQUIRED)

# live555 components
#find_path(LIVE555_INCLUDE_DIR liveMedia.hh PATH_SUFFIXES liveMedia)
find_library(LIVEMEDIA_LIB liveMedia REQUIRED)
find_library(GROUPSOCK_LIB groupsock REQUIRED)
find_library(USAGEENV_LIB UsageEnvironment REQUIRED)
find_library(BASICENV_LIB BasicUsageEnvironment REQUIRED)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${LIVE555_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    ${MPP_LIB}
    ${RGA_LIB} # <--- RGA ishlatyapmiz
    ${LIVEMEDIA_LIB}
    ${GROUPSOCK_LIB}
    ${USAGEENV_LIB}
    ${BASICENV_LIB}
    pthread
)

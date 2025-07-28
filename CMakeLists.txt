
cmake_minimum_required(VERSION 3.10)
project(rtsp_crop_streamer)

set(CMAKE_CXX_STANDARD 14)





include_directories(
    ${CMAKE_SOURCE_DIR}/decoder
    ${CMAKE_SOURCE_DIR}/encoder
    ${CMAKE_SOURCE_DIR}/crop
    ${CMAKE_SOURCE_DIR}/rtsp_client
    ${CMAKE_SOURCE_DIR}/rtsp_server
    /workspace/live/liveMedia/include
    /workspace/live/groupsock/include
    /workspace/live/BasicUsageEnvironment/include
    /workspace/live/UsageEnvironment/include
    /workspace/live/include
    /usr/local/include         # for rga/rga.h
    /usr/local/include/rockchip # if rk_mpi.h is here
    /usr/include/rockchip        # fallback
    /opt/openssl-aarch64/include
)

link_directories(
    /workspace/live/liveMedia
    /workspace/live/groupsock
    /workspace/live/BasicUsageEnvironment
    /workspace/live/UsageEnvironment
)

set(SOURCES
    main.cpp
    decoder/MppDecoder.cpp
    encoder/MppEncoder.cpp
    crop/RgaCrop.cpp
    rtsp_client/LiveRtspInput.cpp
    rtsp_server/LiveRtspOutput.cpp
)

find_library(MPP_LIB rockchip_mpp
    PATHS /workspace/mpp/build/linux/aarch64/mpp
    REQUIRED
)

find_library(RGA_LIB NAMES rga Rga
    PATHS /usr/lib /usr/local/lib
    REQUIRED
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    liveMedia
    groupsock
    BasicUsageEnvironment
    UsageEnvironment
    ${MPP_LIB}
    ${RGA_LIB}
    /workspace/openssl/build/opt/openssl/lib/libssl.so
    /workspace/openssl/build/opt/openssl/lib/libcrypto.so
    pthread
)

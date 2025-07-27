// #pragma once

// #include <string>
// #include <vector>
// #include <mutex>
// #include <deque>

// class LiveRtspOutput {
// public:
//     LiveRtspOutput(const std::string& path, int port, const std::string& streamName);
//     ~LiveRtspOutput();

//     void run();

//     // External call from encoder output
//     static void write_frame(const uint8_t* data, size_t size);

// public:
//     std::string path;
//     int port;
//     std::string streamName;

//     static std::mutex queue_mutex;
//     static std::deque<std::vector<uint8_t>> frame_queue;
// };


// #pragma once

// #include <string>
// #include <vector>
// #include <mutex>
// #include <deque>

// class LiveRtspOutput {
// public:
//     LiveRtspOutput(const std::string& path, int port, const std::string& streamName);
//     ~LiveRtspOutput();

//     void run();

//     // External call from encoder output
//     static void write_frame(const uint8_t* data, size_t size);

// private:
//     std::string path;
//     int port;
//     std::string streamName;

//     // RTSP server components
//     static std::mutex queue_mutex;
//     static std::deque<std::vector<uint8_t>> frame_queue;

//     // Declare FrameSource as a friend so it can access private members
//     friend class FrameSource;
// };


#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <deque>
#include <cstdint>

class LiveRtspOutput {
public:
    LiveRtspOutput(const std::string& path, int port, const std::string& streamName);
    ~LiveRtspOutput();

    void run();
    static void write_frame(const uint8_t* data, size_t size);

public:
    std::string path;
    int port;
    std::string streamName;

    static std::mutex queue_mutex;
    static std::deque<std::vector<uint8_t>> frame_queue;
};

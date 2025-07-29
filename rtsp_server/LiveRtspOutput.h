
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

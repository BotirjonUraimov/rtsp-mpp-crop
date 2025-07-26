#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <deque>

class LiveRtspOutput {
public:
    LiveRtspOutput(const std::string& path, int port, const std::string& streamName);
    ~LiveRtspOutput();

    void run();

    // External call from encoder output
    static void write_frame(const uint8_t* data, size_t size);

private:
    std::string path;
    int port;
    std::string streamName;

    static std::mutex queue_mutex;
    static std::deque<std::vector<uint8_t>> frame_queue;
};

#pragma once

#include <functional>
#include <thread>
#include <string>

class LiveRtspInput {
public:
    using FrameCallback = std::function<void(uint8_t*, size_t)>;

    LiveRtspInput(const std::string& rtsp_url);
    ~LiveRtspInput();

    void set_on_frame(FrameCallback cb);
    void start();

private:
    std::string m_url;
    FrameCallback m_on_frame;
    std::thread m_thread;
    bool m_running;

    void run();
};

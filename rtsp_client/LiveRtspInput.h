#pragma once

#include <functional>
#include <thread>
#include <string>
#include <atomic>

class LiveRtspInput {
public:
    using FrameCallback = std::function<void(uint8_t*, size_t)>;

    LiveRtspInput(const std::string& rtsp_url);
    ~LiveRtspInput();

    void set_on_frame(FrameCallback cb);
    void start();
    void stop();
    
    

    FrameCallback m_on_frame;
private:
    std::string m_url;
    std::thread m_thread;
    std::atomic<bool> m_running;   // <== bool emas, atomic
    char m_watchVar = 0; 
 // Live555 event loop uchun

    void run();

};

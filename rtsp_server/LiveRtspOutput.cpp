#include "LiveRtspOutput.h"
#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <thread>
#include <chrono>
#include <iostream>

std::mutex LiveRtspOutput::queue_mutex;
std::deque<std::vector<uint8_t>> LiveRtspOutput::frame_queue;

class FrameSource : public FramedSource {
public:
    static FrameSource* createNew(UsageEnvironment& env) {
        return new FrameSource(env);
    }

protected:
    FrameSource(UsageEnvironment& env) : FramedSource(env) {}

    virtual void doGetNextFrame() override {
        if (LiveRtspOutput::frame_queue.empty()) {
            nextTask() = envir().taskScheduler().scheduleDelayedTask(10000,
                (TaskFunc*)FrameSource::deliverFrameStub, this);
            return;
        }

        std::vector<uint8_t> frame;
        {
            std::lock_guard<std::mutex> lock(LiveRtspOutput::queue_mutex);
            if (!LiveRtspOutput::frame_queue.empty()) {
                frame = std::move(LiveRtspOutput::frame_queue.front());
                LiveRtspOutput::frame_queue.pop_front();
            }
        }

        if (!frame.empty()) {
            size_t toCopy = std::min(frame.size(), static_cast<size_t>(fMaxSize));
            memcpy(fTo, frame.data(), toCopy);
            fFrameSize = toCopy;
            fDurationInMicroseconds = 0;
            gettimeofday(&fPresentationTime, NULL);
            afterGetting(this);
        }
    }

    static void deliverFrameStub(void* clientData) {
        ((FrameSource*)clientData)->doGetNextFrame();
    }
};

LiveRtspOutput::LiveRtspOutput(const std::string& p, int prt, const std::string& name)
    : path(p), port(prt), streamName(name) {}

LiveRtspOutput::~LiveRtspOutput() {}

void LiveRtspOutput::run() {
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

    RTSPServer* server = RTSPServer::createNew(*env, port);
    if (!server) {
        std::cerr << "[RTSP] Failed to create server on port " << port << std::endl;
        return;
    }

    ServerMediaSession* sms = ServerMediaSession::createNew(*env, streamName.c_str(), NULL, NULL);
    sms->addSubsession(PassiveServerMediaSubsession::createNew(
        H264VideoStreamFramer::createNew(*env, FrameSource::createNew(*env)), True));
    server->addServerMediaSession(sms);

    std::cout << "[RTSP] Stream available at rtsp://<device-ip>:" << port << "/" << streamName << std::endl;

    env->taskScheduler().doEventLoop();
}

void LiveRtspOutput::write_frame(const uint8_t* data, size_t size) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    frame_queue.emplace_back(data, data + size);
    if (frame_queue.size() > 100) {
        frame_queue.pop_front(); // drop oldest if full
    }
}

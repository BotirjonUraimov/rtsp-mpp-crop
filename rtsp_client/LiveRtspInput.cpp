#include "LiveRtspInput.h"
#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>

class DummySink : public MediaSink {
public:
    DummySink(UsageEnvironment& env, LiveRtspInput::FrameCallback cb)
        : MediaSink(env), callback(cb) {
        fBuffer = new uint8_t[200000]; // 200 KB buffer
    }

    virtual ~DummySink() {
        delete[] fBuffer;
    }

    static DummySink* createNew(UsageEnvironment& env, LiveRtspInput::FrameCallback cb) {
        return new DummySink(env, cb);
    }

private:
    uint8_t* fBuffer;
    LiveRtspInput::FrameCallback callback;

    Boolean continuePlaying() override {
        if (!fSource) return False;

        fSource->getNextFrame(fBuffer, 200000,
            afterGettingFrame, this, onSourceClosure, this);
        return True;
    }

    static void afterGettingFrame(void* clientData, unsigned frameSize,
                                  unsigned /*numTruncatedBytes*/,
                                  struct timeval /*presentationTime*/,
                                  unsigned /*durationInMicroseconds*/) {
        DummySink* sink = static_cast<DummySink*>(clientData);
        if (sink->callback) {
            sink->callback(sink->fBuffer, frameSize);
        }
        sink->continuePlaying();
    }
};

LiveRtspInput::LiveRtspInput(const std::string& rtsp_url)
    : m_url(rtsp_url), m_running(false) {}

LiveRtspInput::~LiveRtspInput() {
    m_running = false;
    if (m_thread.joinable()) m_thread.join();
}

void LiveRtspInput::set_on_frame(FrameCallback cb) {
    m_on_frame = cb;
}

void LiveRtspInput::start() {
    m_running = true;
    m_thread = std::thread(&LiveRtspInput::run, this);
}

void LiveRtspInput::run() {
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

    RTSPClient* client = RTSPClient::createNew(*env, m_url.c_str(), 1);
    if (!client) {
        (*env) << "Failed to create RTSPClient for URL " << m_url.c_str() << "\n";
        return;
    }

    // Minimal describe/play setup — simplify for now
    // This should be replaced with a full RTSP client handling if needed
    // For now could replace this part with ffmpeg for testing

    (*env).taskScheduler().doEventLoop(); // blocks forever
}

#pragma once
#include <liveMedia.hh>
#include <functional>

class CallbackFramedSource : public FramedSource {
public:
    using FrameCallback = std::function<void(uint8_t*, size_t)>;
  static CallbackFramedSource* createNew(UsageEnvironment& env,
                                         FrameCallback cb) {
    return new CallbackFramedSource(env, cb);
  }

protected:
  CallbackFramedSource(UsageEnvironment& env, FrameCallback cb)
    : FramedSource(env), fCallback(cb) {}

  virtual void doGetNextFrame() override {
    // ask upstream, e.g., underlying source, to deliver next frame
    // deliverFrame(); // stub
  }

  void afterGettingFrame(uint8_t* data, unsigned size) {
    fFrameSize = size;
    memcpy(fTo, data, size);
    // fPresentationTime = getCurrentTime();
    FramedSource::afterGetting(this);
    if (fCallback) fCallback(data, size);
  }

private:
  FrameCallback fCallback;
};

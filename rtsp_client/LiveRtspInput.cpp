
#include "LiveRtspInput.h"
#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <thread>
#include <iostream>


void setupNextSubsession(RTSPClient* rtspClient);
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);
void shutdownStream(RTSPClient* rtspClient, int exitCode = 1);
void subsessionAfterPlaying(void* clientData);

void subsessionByeHandler(void* clientData, char const* reason);
  // called when a RTCP "BYE" is received for a subsession
void streamTimerHandler(void* clientData);

// while testing, set this to True to use TCP for streaming
#define REQUEST_STREAMING_OVER_TCP True


static unsigned rtspClientCount = 0;
char eventLoopWatchVariable = 1;



class StreamClientState {
    public:
    StreamClientState();
    virtual ~StreamClientState();

    public:
    MediaSubsessionIterator* iter;
    MediaSession* session;
    MediaSubsession* subsession;
    TaskToken streamTimerTask;
    double duration;
    void* sessionUserData;
};


class ourRTSPClient: public RTSPClient {
    public:
    static ourRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,
                    int verbosityLevel = 0,
                    char const* applicationName = NULL,
                    portNumBits tunnelOverHTTPPortNum = 0);

    protected:
    ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
            int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
        // called only by createNew();
    virtual ~ourRTSPClient();

    public:
    StreamClientState scs;
};

class DummySink: public MediaSink {
    public:
    static DummySink* createNew(UsageEnvironment& env,
                    MediaSubsession& subsession, // identifies the kind of data that's being received
                    char const* streamId = NULL); // identifies the stream itself (optional)

    private:
    DummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId);
        // called only by "createNew()"
    virtual ~DummySink();

    static void afterGettingFrame(void* clientData, unsigned frameSize,
                                    unsigned numTruncatedBytes,
                    struct timeval presentationTime,
                                    unsigned durationInMicroseconds);
    void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
                struct timeval presentationTime, unsigned durationInMicroseconds);

    private:
    // redefined virtual functions:
    virtual Boolean continuePlaying();

    private:
    u_int8_t* fReceiveBuffer;
    MediaSubsession& fSubsession;
    char* fStreamId;
};
// --- LiveRtspInput implementation ---
LiveRtspInput::LiveRtspInput(const std::string& rtsp_url)
    : m_url(rtsp_url), m_running(false), m_on_frame(nullptr) {}

LiveRtspInput::~LiveRtspInput() {
    std::cout << "[RTSP] Stopping RTSP input for URL: " << m_url << "\n";
    stop();
}

void LiveRtspInput::set_on_frame(FrameCallback cb) {
    if (m_on_frame) {
        std::cerr << "[RTSP] Warning: Overriding existing frame callback.\n";
    }
    std::cout << "[RTSP] Setting frame callback.\n";
    m_on_frame = cb;
}

void LiveRtspInput::start() {
    if (m_running) return;
    std::cout << "[RTSP] Starting RTSP input for URL: " << m_url << "\n";
    m_running = true;
    eventLoopWatchVariable = 0;  
    m_thread = std::thread(&LiveRtspInput::run, this);
}

void LiveRtspInput::stop() {
    m_running = false;
    eventLoopWatchVariable = 1; 
    if (m_thread.joinable()) m_thread.join();
}

void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << rtspClient << "Failed to get a SDP description: " << resultString << "\n";
      delete[] resultString;
      break;
    }

    env << rtspClient << "SDP data:" << resultString << "\n";
    

    char* const sdpDescription = resultString;
    env << rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";

    // Create a media session object from this SDP description:
    scs.session = MediaSession::createNew(env, sdpDescription);
    delete[] sdpDescription; // because we don't need it anymore
    if (scs.session == NULL) {
      env << rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
      break;
    } else if (!scs.session->hasSubsessions()) {
      env << rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
      break;
    }
    scs.iter = new MediaSubsessionIterator(*scs.session);
    env << rtspClient << "Created a MediaSession object from the SDP description\n";
    setupNextSubsession(rtspClient);
    return;
  } while (0);

  // An unrecoverable error occurred with this stream.
  shutdownStream(rtspClient);
}

// void setupNextSubsession(RTSPClient* rtspClient) {
//   ourRTSPClient* client = static_cast<ourRTSPClient*>(rtspClient);
//   UsageEnvironment& env = rtspClient->envir(); 
//   StreamClientState& scs = client->scs;
//   std::cout << "Setting up next subsession...\n";

//   //scs.subsession->miscPtr = client;  
//   scs.subsession = scs.iter->next();

//   StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias



//   if (scs.subsession != NULL) {
//     if (!scs.subsession->initiate()) {
//       env << rtspClient << "Failed to initiate the \"" << scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
//       setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
//     } else {
//       env << rtspClient << "Initiated the \"" << scs.subsession << "\" subsession (";

//       if (scs.subsession->rtcpIsMuxed()) {
// 	env << "client port " << scs.subsession->clientPortNum();
//       } else {
// 	env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
//       }
//       env << ")\n";

//       // Continue setting up this subsession, by sending a RTSP "SETUP" command:
//       rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
//     }
//     return;
//   }

//   // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
//   if (scs.session->absStartTime() != NULL) {
//     // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
//     std::cout << "Stream is indexed by absolute time.\n";
//     rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
//   } else {
//     scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
//     std::cout << "Stream duration: " << scs.duration << " seconds\n";
//     rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
//   }
// }

void setupNextSubsession(RTSPClient* rtspClient) {
    ourRTSPClient* client = static_cast<ourRTSPClient*>(rtspClient);
    UsageEnvironment& env = client->envir();
    StreamClientState& scs = client->scs;

    std::cout << "Setting up next subsession...\n";
    scs.subsession = scs.iter->next();

    if (scs.subsession != NULL) {
        if (!scs.subsession->initiate()) {
            env << client << "Failed to initiate the \"" << scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
            setupNextSubsession(client);
        } else {
            env << client << "Initiated the \"" << scs.subsession << "\" subsession\n";

            // miscPtr orqali LiveRtspInput pointerini sink ichiga uzatamiz
            scs.subsession->miscPtr = scs.sessionUserData;

            client->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
        }
        return;
    }

    // agar subsessionlar tugasa → PLAY
    if (scs.session->absStartTime() != NULL) {
        client->sendPlayCommand(*scs.session, continueAfterPLAY,
                                scs.session->absStartTime(), scs.session->absEndTime());
    } else {
        scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
        client->sendPlayCommand(*scs.session, continueAfterPLAY);
    }
}

void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
  Boolean success = False;
    std::cout << "Continuing after PLAY...\n";
  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << rtspClient << "Failed to start playing session: " << resultString << "\n";
      break;
    }
    if (scs.duration > 0) {
        env << rtspClient << "Stream duration is " << scs.duration << " seconds; scheduling a timer to stop the stream at that time\n";
      unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
      scs.duration += delaySlop;
      unsigned uSecsToDelay = (unsigned)(scs.duration*1000000);
      scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
    }

    env << rtspClient << "Started playing session";
    if (scs.duration > 0) {
      env << " (for up to " << scs.duration << " seconds)";
    }
    env << "...\n";

    success = True;
  } while (0);
  delete[] resultString;

  if (!success) {
    // An unrecoverable error occurred with this stream.
    shutdownStream(rtspClient);
  }
}

void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << rtspClient << "Failed to set up the \"" << scs.subsession << "\" subsession: " << resultString << "\n";
      break;
    }

    env << rtspClient << "Set up the \"" << scs.subsession << "\" subsession (";
    if (scs.subsession->rtcpIsMuxed()) {
      env << "client port " << scs.subsession->clientPortNum();
    } else {
      env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
    }
    env << ")\n";

    // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
    // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
    // after we've sent a RTSP "PLAY" command.)

    scs.subsession->sink = DummySink::createNew(env, *scs.subsession, rtspClient->url());
      // perhaps use your own custom "MediaSink" subclass instead
    if (scs.subsession->sink == NULL) {
      env << rtspClient << "Failed to create a data sink for the \"" << scs.subsession
	  << "\" subsession: " << env.getResultMsg() << "\n";
      break;
    }

    env << rtspClient << "Created a data sink for the \"" << scs.subsession << "\" subsession\n";
    scs.subsession->miscPtr = rtspClient; // a hack to let subsession handler functions get the "RTSPClient" from the subsession 
    scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
				       subsessionAfterPlaying, scs.subsession);
    // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
    if (scs.subsession->rtcpInstance() != NULL) {
      scs.subsession->rtcpInstance()->setByeWithReasonHandler(subsessionByeHandler, scs.subsession);
    }
  } while (0);
  delete[] resultString;

  // Set up the next subsession, if any:
  setupNextSubsession(rtspClient);
}

void subsessionAfterPlaying(void* clientData) {
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

  // Begin by closing this subsession's stream:
  Medium::close(subsession->sink);
  subsession->sink = NULL;

  // Next, check whether *all* subsessions' streams have now been closed:
  MediaSession& session = subsession->parentSession();
  MediaSubsessionIterator iter(session);
  while ((subsession = iter.next()) != NULL) {
    if (subsession->sink != NULL) return; // this subsession is still active
  }

  // All subsessions' streams have now been closed, so shutdown the client:
  shutdownStream(rtspClient);
}

void subsessionByeHandler(void* clientData, char const* reason) {
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
  UsageEnvironment& env = rtspClient->envir(); // alias

  env << rtspClient << "Received RTCP \"BYE\"";
  if (reason != NULL) {
    env << " (reason:\"" << reason << "\")";
    delete[] (char*)reason;
  }
  env << " on \"" << subsession << "\" subsession\n";

  // Now act as if the subsession had closed:
  subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void* clientData) {
  ourRTSPClient* rtspClient = (ourRTSPClient*)clientData;
  StreamClientState& scs = rtspClient->scs; // alias

  scs.streamTimerTask = NULL;

  // Shut down the stream:
  shutdownStream(rtspClient);
}

void shutdownStream(RTSPClient* rtspClient, int exitCode) {
  UsageEnvironment& env = rtspClient->envir(); // alias
  StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
    env << rtspClient << "Shutting down the stream.\n";
  // First, check whether any subsessions have still to be closed:
  if (scs.session != NULL) { 
    std::cout << "Closing subsessions...\n";
    Boolean someSubsessionsWereActive = False;
    MediaSubsessionIterator iter(*scs.session);
    MediaSubsession* subsession;

    while ((subsession = iter.next()) != NULL) {
      if (subsession->sink != NULL) {
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	if (subsession->rtcpInstance() != NULL) {
	  subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
	}

	someSubsessionsWereActive = True;
      }
    }

    if (someSubsessionsWereActive) {
      // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
      // Don't bother handling the response to the "TEARDOWN".
      rtspClient->sendTeardownCommand(*scs.session, NULL);
    }
  }

  env << rtspClient << "Closing the stream.\n";
  Medium::close(rtspClient);
    // Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

  if (--rtspClientCount == 0) {
    eventLoopWatchVariable = 1; // signal the event loop to exit
    // The final stream has ended, so exit the application now.
    // (Of course, if you're embedding this code into your own application, you might want to comment this out,
    // and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
    //exit(exitCode);
  }
}

void LiveRtspInput::run() {
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);
    std::cout << "[RTSP] Starting RTSP client for URL: " << m_url.c_str() << "\n";
    // Create RTSP client
    // ourRTSPClient* rtspClient = ourRTSPClient::createNew(*env, m_url.c_str(), 1, "LiveRtspInput");
    // rtspClient->scs.sessionUserData = this;
    ourRTSPClient* rtspClient = ourRTSPClient::createNew(*env, m_url.c_str(), 1, "LiveRtspInput");
    if (!rtspClient) {
        (*env) << "[RTSP] Failed to create RTSPClient for URL " << m_url.c_str() << "\n";
        delete scheduler;
        return;
    }
    rtspClient->scs.sessionUserData = this;
    ++rtspClientCount;
    rtspClient->sendDescribeCommand(continueAfterDESCRIBE);

    (*env) << "[RTSP] Connected to " << m_url.c_str() << "\n";
    env->taskScheduler().doEventLoop(reinterpret_cast<EventLoopWatchVariable*>(&eventLoopWatchVariable)); // run until stop()
}


ourRTSPClient* ourRTSPClient::createNew(UsageEnvironment& env, char const* rtspURL,
					int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum) {
  return new ourRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}
ourRTSPClient::ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
			     int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
  : RTSPClient(env,rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1) {
}
ourRTSPClient::~ourRTSPClient() {
}


StreamClientState::StreamClientState()
  : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0),  sessionUserData(NULL)  {
}

StreamClientState::~StreamClientState() {
  delete iter;
  if (session != NULL) {
    // We also need to delete "session", and unschedule "streamTimerTask" (if set)
    UsageEnvironment& env = session->envir(); // alias

    env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
    Medium::close(session);
  }
}

// Even though we're not going to be doing anything with the incoming data, we still need to receive it.
// Define the size of the buffer that we'll use:
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 100000

DummySink* DummySink::createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId) {
  return new DummySink(env, subsession, streamId);
}

DummySink::DummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
  : MediaSink(env),
    fSubsession(subsession) {
  fStreamId = strDup(streamId);
  fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
}

DummySink::~DummySink() {
  delete[] fReceiveBuffer;
  delete[] fStreamId;
}

void DummySink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned durationInMicroseconds) {
  DummySink* sink = (DummySink*)clientData;
  sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1

void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
                                  struct timeval presentationTime, unsigned /*durationInMicroseconds*/) {
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
  if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
  envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() 
          << ":\tReceived " << frameSize << " bytes\n";
#endif

  LiveRtspInput* input = reinterpret_cast<LiveRtspInput*>(fSubsession.miscPtr);
  if (input && input->m_on_frame) {
      input->m_on_frame(fReceiveBuffer, frameSize);
  }

  continuePlaying();
}


Boolean DummySink::continuePlaying() {
  if (fSource == NULL) return False; // sanity check (should not happen)

  // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
  fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
                        afterGettingFrame, this,
                        onSourceClosure, this);
  return True;
}
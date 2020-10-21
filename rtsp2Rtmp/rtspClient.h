#ifndef _RTSPCLIENT_H
#define _RTSPCLIENT_H
#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include "rtmpClient.h"

#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 5000000
#define RTSP_CLIENT_VERBOSITY_LEVEL    1 
#define REQUEST_STREAMING_OVER_TCP     True
//#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1
extern char eventLoopWatchVariable;

void openURL(UsageEnvironment& env, char const* progName, char const* rtspURL, char const* User, char const* Password);
void setupNextSubsession(RTSPClient* rtspClient);

void shutdownStream(RTSPClient* rtspClient, int exitCode = 1);
void continueAfterOption(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterGET_PARAMETER(RTSPClient* rtspClient, int resultCode, char* resultString);

void subsessionAfterPlaying(void* clientData); 
void subsessionByeHandler(void* clientData); 
void streamTimerHandler(void* clientData);


static void scheduleSendHeartBeat(RTSPClient* rtspClient);



class StreamClientState 
{
public:
    StreamClientState();
    virtual ~StreamClientState();

public:
    MediaSubsessionIterator* iter;
    MediaSession* session;
    MediaSubsession* subsession;
    TaskToken streamTimerTask;
    double duration;
};


class ourRTSPClient: public RTSPClient 
{
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


class DummySink: public MediaSink 
{
public:
    static DummySink* createNew(UsageEnvironment& env,
            MediaSubsession& subsession, // identifies the kind of data that's being received
            char const* streamId = NULL); // identifies the stream itself (optional)
    int SendData(unsigned frameSize);

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

#endif


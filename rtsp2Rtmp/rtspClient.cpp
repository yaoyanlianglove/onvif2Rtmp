#include "rtspClient.h"
#include <iostream>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h> 
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <RTSPCommon.hh>

using namespace std;
static unsigned rtspClientCount = 0;
char eventLoopWatchVariable = 0;
Boolean isSupportGetParamter = false;
TaskToken sendHeartBeatTask; 
RTSPClient *myRtspClient;
//********************RTMP************
RTMP *rtmp;
RTMPPacket *packet;
int isRtmpConnect = 0;
//********************RTMP************
UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient) 
{
    return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}
UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession) 
{
    return env << subsession.mediumName() << "/" << subsession.codecName();
}
void openURL(UsageEnvironment& env, char const* progName, char const* rtspURL, char const* User, char const* Password) 
{
    
    Authenticator* authenticator;
    myRtspClient = ourRTSPClient::createNew(env, rtspURL, RTSP_CLIENT_VERBOSITY_LEVEL, progName);
    if (myRtspClient == NULL) 
    {
        env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
        return;
    }
    ++rtspClientCount;
    authenticator = new Authenticator(User, Password);
    myRtspClient->sendOptionsCommand(continueAfterOption, authenticator); 
    delete authenticator;
}
void continueAfterOption(RTSPClient* rtspClient, int resultCode, char* resultString)
{
    if (resultCode != 0)
    {
        delete[] resultString;
        shutdownStream(rtspClient);
        return;
    }
    isSupportGetParamter = RTSPOptionIsSupported("GET_PARAMETER", resultString);
    delete[] resultString;
    rtspClient->sendDescribeCommand(continueAfterDESCRIBE); 
}
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) 
{
    do 
    {
        UsageEnvironment& env = rtspClient->envir(); // alias
        StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
        if (resultCode != 0) 
        {
            env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
            delete[] resultString;
            break;
        }
        char* const sdpDescription = resultString;
        env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";
        // Create a media session object from this SDP description:
        scs.session = MediaSession::createNew(env, sdpDescription);
        delete[] sdpDescription; // because we don't need it anymore
        if (scs.session == NULL) 
        {
            env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
            break;
        } 
        else if (!scs.session->hasSubsessions()) 
        {
            env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
            break;
        }
        scs.iter = new MediaSubsessionIterator(*scs.session);
        setupNextSubsession(rtspClient);
        return;
    } while (0);

    // An unrecoverable error occurred with this stream.
    shutdownStream(rtspClient);
}
void setupNextSubsession(RTSPClient* rtspClient) 
{
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
  
    scs.subsession = scs.iter->next();
    if (scs.subsession != NULL) 
    {
        if (!scs.subsession->initiate()) 
        {
            env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
            setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
        } 
        else 
        {
            env << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession (";
            if (scs.subsession->rtcpIsMuxed()) 
            {
                env << "client port " << scs.subsession->clientPortNum();
            } 
            else 
            {
                env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
            }
            env << ")\n";

            // Continue setting up this subsession, by sending a RTSP "SETUP" command:
            rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
        }
        return;
    }

  // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
    if (scs.session->absStartTime() != NULL) 
    {
        // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
    } 
    else 
    {
        scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
    }
}
void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) 
{
    do 
    {
        UsageEnvironment& env = rtspClient->envir(); // alias
        StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

        if (resultCode != 0) 
        {
            env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
            break;
        }
        env << *rtspClient << "Set up the \"" << *scs.subsession << "\" subsession (";
        if (scs.subsession->rtcpIsMuxed()) 
        {
            env << "client port " << scs.subsession->clientPortNum();
        } 
        else 
        {
            env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
        }
        env << ")\n";
        scs.subsession->sink = DummySink::createNew(env, *scs.subsession, rtspClient->url());
        // perhaps use your own custom "MediaSink" subclass instead
        if (scs.subsession->sink == NULL)
        {
            env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
            << "\" subsession: " << env.getResultMsg() << "\n";
            break;
        }
        env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
        scs.subsession->miscPtr = rtspClient; // a hack to let subsession handle functions get the "RTSPClient" from the subsession 
        scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
                       subsessionAfterPlaying, scs.subsession);
        // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
        if (scs.subsession->rtcpInstance() != NULL) 
        {
            scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, scs.subsession);
        }
    } while (0);
    delete[] resultString;
    // Set up the next subsession, if any:
    setupNextSubsession(rtspClient);
}

void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) 
{
    Boolean success = False;
    do 
    {
        UsageEnvironment& env = rtspClient->envir(); // alias
        StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
        if (resultCode != 0) 
        {
            env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
            break;
        }
        if (scs.duration > 0) 
        {
            unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
            scs.duration += delaySlop;
            unsigned uSecsToDelay = (unsigned)(scs.duration*1000000);
            scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
        }
        env << *rtspClient << "Started playing session";
        if (scs.duration > 0) 
        {
            env << " (for up to " << scs.duration << " seconds)";
        }
        env << "...\n";
        success = True;
    } while (0);
    delete[] resultString;
    if(success)
    {
        scheduleSendHeartBeat(rtspClient);
    }
    if (!success) 
    {
    // An unrecoverable error occurred with this stream.
        shutdownStream(rtspClient);
    }
}

void continueAfterGET_PARAMETER(RTSPClient* rtspClient, int resultCode, char* resultString)
{
    Boolean success = False;
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
    
    if (resultCode != 0) 
    {
        env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
        success = False;
    }
    else
    {
        env << *rtspClient << "Send heartBeat success. "<< "\n";
        success = True;
    }
    delete[] resultString;
    if(success)
        scheduleSendHeartBeat(rtspClient);
    else
        shutdownStream(rtspClient);
}
void sendHeartBeat(void* clientData)
{
    ourRTSPClient* rtspClient = (ourRTSPClient*)clientData;
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
    if (isSupportGetParamter)
    {
        rtspClient->sendGetParameterCommand((*scs.session), continueAfterGET_PARAMETER, NULL);
    }
    else
    {
        rtspClient->sendOptionsCommand(continueAfterGET_PARAMETER);
    }
}
static void scheduleSendHeartBeat(RTSPClient* rtspClient)
{
    // 获取超时时间，设置超时一半的时间发送心跳
    unsigned sessionTimeOut = (rtspClient->sessionTimeoutParameter())/2;

    unsigned uSecsToDelay = (unsigned)(sessionTimeOut*1000000);

    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    sendHeartBeatTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)sendHeartBeat, rtspClient);
}
// Implementation of the other event handlers:

void subsessionAfterPlaying(void* clientData) 
{
    MediaSubsession* subsession = (MediaSubsession*)clientData;
    RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);
    // Begin by closing this subsession's stream:
    Medium::close(subsession->sink);
    subsession->sink = NULL;
    // Next, check whether *all* subsessions' streams have now been closed:
    MediaSession& session = subsession->parentSession();
    MediaSubsessionIterator iter(session);
    while ((subsession = iter.next()) != NULL) 
    {
        if (subsession->sink != NULL) 
        return; // this subsession is still active
    }
    // All subsessions' streams have now been closed, so shutdown the client:
    shutdownStream(rtspClient);
}

void subsessionByeHandler(void* clientData) 
{
    MediaSubsession* subsession = (MediaSubsession*)clientData;
    RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
    UsageEnvironment& env = rtspClient->envir(); // alias

    env << *rtspClient << "Received RTCP \"BYE\" on \"" << *subsession << "\" subsession\n";

    // Now act as if the subsession had closed:
    subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void* clientData) 
{
    ourRTSPClient* rtspClient = (ourRTSPClient*)clientData;
    StreamClientState& scs = rtspClient->scs; // alias
    scs.streamTimerTask = NULL;

    // Shut down the stream:
    shutdownStream(rtspClient);
}

void shutdownStream(RTSPClient* rtspClient, int exitCode) 
{
    Rtmp_Close(rtmp, packet);
    isRtmpConnect = 0;
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    // First, check whether any subsessions have still to be closed:
    if (scs.session != NULL) 
    { 
        Boolean someSubsessionsWereActive = False;
        MediaSubsessionIterator iter(*scs.session);
        MediaSubsession* subsession;

        while ((subsession = iter.next()) != NULL) 
        {
            if (subsession->sink != NULL) 
            {
                Medium::close(subsession->sink);
                subsession->sink = NULL;

                if (subsession->rtcpInstance() != NULL) 
                {
                    subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
                }

                someSubsessionsWereActive = True;
            }
        }

        if (someSubsessionsWereActive) 
        {
            // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
            // Don't bother handling the response to the "TEARDOWN".
            rtspClient->sendTeardownCommand(*scs.session, NULL);
        }
    }
    
    if(sendHeartBeatTask != NULL)
    {
        isSupportGetParamter = False;
        env.taskScheduler().unscheduleDelayedTask(sendHeartBeatTask);
        sendHeartBeatTask = NULL;
    }
    env << *rtspClient << "Closing the stream.\n";
    Medium::close(rtspClient);
    // Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

    if (--rtspClientCount == 0) 
    {
        // The final stream has ended, so exit the application now.
        // (Of course, if you're embedding this code into your own application, you might want to comment this out,
        // and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
        //exit(exitCode);  //modify by yyl
        cout<<"NO connect"<<endl;
    }
    eventLoopWatchVariable = 1;
}


// Implementation of "ourRTSPClient":

ourRTSPClient* ourRTSPClient::createNew(UsageEnvironment& env, char const* rtspURL,
                    int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum) 
{
    return new ourRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

ourRTSPClient::ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
                 int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
  : RTSPClient(env,rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1) 
{}

ourRTSPClient::~ourRTSPClient() 
{}


// Implementation of "StreamClientState":

StreamClientState::StreamClientState()
  : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0) 
{}

StreamClientState::~StreamClientState() 
{
    delete iter;
    if (session != NULL) 
    {
        // We also need to delete "session", and unschedule "streamTimerTask" (if set)
        UsageEnvironment& env = session->envir(); // alias

        env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
        Medium::close(session);
    }
}
DummySink* DummySink::createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId) 
{
    return new DummySink(env, subsession, streamId);
}

DummySink::DummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
  : MediaSink(env),
    fSubsession(subsession) 
{
    fStreamId = strDup(streamId);
    fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
}

DummySink::~DummySink() 
{
    delete[] fReceiveBuffer;
    delete[] fStreamId;
}

void DummySink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
                  struct timeval presentationTime, unsigned durationInMicroseconds) 
{
    DummySink* sink = (DummySink*)clientData;
    sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
                  struct timeval presentationTime, unsigned /*durationInMicroseconds*/) 
{
  // We've just received a frame of data.  (Optionally) print out information about it:
    #ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
    if (fStreamId != NULL) 
        envir() << "Stream \"" << fStreamId << "\"; ";
    envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
    if (numTruncatedBytes > 0) 
        envir() << " (with " << numTruncatedBytes << " bytes truncated)";
    char uSecsStr[6+1]; // used to output the 'microseconds' part of the presentation time
    sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
    envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
    if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) 
    {
        envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
    }
    #ifdef DEBUG_PRINT_NPT
        envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
    #endif
        envir() << "\n";
    #endif
    SendData(frameSize);
    // Then continue, to request the next frame of data:
    continuePlaying();
}

Boolean DummySink::continuePlaying() 
{
    if (fSource == NULL) return False; // sanity check (should not happen)

    // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
    fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
                        afterGettingFrame, this,
                        onSourceClosure, this);
    return True;
}

int DummySink::SendData(unsigned frameSize)
{
    char *serverAddr = "rtmp://192.168.1.202:1935/myapp/mp4";
    if(isRtmpConnect == 0)
    {
        if(Rtmp_Connect(rtmp, packet, serverAddr) != 0)
        {
            printf("Rtmp_Connect failed.\n");
            return -1;
        }
        printf("rtmp->m_sb.sb_socket is %d\n", rtmp->m_sb.sb_socket);
        isRtmpConnect = 1; 
        if(!RTMPPacket_Alloc(packet, PACKET_MAX_SIZE))
        {
            printf("RTMPPacket malloc failed.\n");
            return -1;
        }
    } 
    else if(isRtmpConnect == 1)
        Rtmp_Send(rtmp, packet, fReceiveBuffer, (long)frameSize);
    return 0; 
}



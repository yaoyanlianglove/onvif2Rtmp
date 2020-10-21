#include "main.h"

extern RTSPClient *myRtspClient;
extern char eventLoopWatchVariable;

extern RTMP *rtmp;
extern RTMPPacket *packet;
int flagExit = 0;
void handler(int sig)
{
    printf("capture a SIGALRM signal %d \n",sig);
    shutdownStream(myRtspClient);
    flagExit = 1;
}
int main(int argc, char **argv)
{
    printf("Rtsp2Rtmp start\n");
    //********************信号处理********************************
    struct sigaction act;
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if(sigaction(15, &act, NULL) == -1)
    {
        perror("sigaction\n");
        exit(1);
    }
    //********************信号处理********************************
    int res = 0;
    TaskScheduler *scheduler;
    UsageEnvironment *env;
    if(argc < 4)
    {
        printf("Please input like this: ./rtsp2Rtmp.exe rstp://xx.xx.xx.xx admin 123456\n");
        return -1;
    }
    const char *ProgName = "rtsp2Rtmp";
    //为结构体“RTMP”分配内存
    rtmp = RTMP_Alloc();
    if(!rtmp)
    {
        RTMP_LogPrintf("RTMP_Alloc failed.\n");
        return -1;
    }
    packet = (RTMPPacket *)malloc(sizeof(RTMPPacket));
    if(!packet)
    {
        RTMP_LogPrintf("RTMPPacket malloc failed.\n");
        free(rtmp);
        return -1;
    }
    while(1)
    {
        scheduler = BasicTaskScheduler::createNew();
        env = BasicUsageEnvironment::createNew(*scheduler);
        eventLoopWatchVariable = 0;
        openURL(*env, ProgName, argv[1], argv[2], argv[3]); 
        env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
        env->reclaim(); 
        env = NULL;
        delete scheduler; 
        scheduler = NULL;
        sleep(1);
        if(flagExit == 0)
            printf("doEventLoop: continue.\n");
        else
        {
            printf("doEventLoop: end.\n");
            break;
        }
    }
    return 0;
}

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rtmpClient.h"
#define  FLV_HEAD_SIZE  9
#define  AVC_HEAD_SIZE  9

long start_time    = 0;
int falgFirstFrame = 1;
long spsLen        = 0;
long ppsLen        = 0;
int flagSPSOK      = 0;
char *nalSPS, *nalPPS;

int Rtmp_Connect(RTMP *rtmp, RTMPPacket *packet, char *serverAddr)
{
    //初始化结构体“RTMP”中的成员变量
    RTMP_Init(rtmp);
    rtmp->Link.timeout = 30;
    rtmp->Link.lFlags |= RTMP_LF_LIVE;
    RTMP_SetBufferMS(rtmp, 3600 * 1000);
    if(!RTMP_SetupURL(rtmp, serverAddr))
    {
        RTMP_Log(RTMP_LOGERROR, "SetupURL failed.\n");
        RTMP_Free(rtmp);
        return -1;
    }
    //发布流的时候必须要使用。如果不使用则代表接收流。
    RTMP_EnableWrite(rtmp);
    //建立RTMP连接，创建一个RTMP协议规范中的NetConnection
    if (!RTMP_Connect(rtmp, NULL))
    {
        RTMP_Log(RTMP_LOGERROR, "Connect failed.\n");
        RTMP_Free(rtmp);
        return -1;
    }
    //创建一个RTMP协议规范中的NetStream
    if(!RTMP_ConnectStream(rtmp, 0))
    {
        RTMP_Log(RTMP_LOGERROR, "ConnectStream failed.\n");
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        return -1;
    }
    return 0;
}
/*
 *packet   :RTMP数据包
 *buff     :H264视频帧
 *len      :视频数据的长度
 */
int Rtmp_Send(RTMP *rtmp, RTMPPacket *packet, unsigned char *buff, long len)
{
    printf("rtmp->m_sb.sb_socket is %d\n", rtmp->m_sb.sb_socket);
    enum NAL_TYPE nalType;
    long timeoffset;
    unsigned char * body;
    RTMPPacket_Reset(packet);
    if(falgFirstFrame == 1)
    {
        falgFirstFrame = 0;
        start_time     = RTMP_GetTime();
    }
    timeoffset = RTMP_GetTime() - start_time;  /*start_time为开始直播时的时间戳*/
    printf("timeoffset is %ld\n", timeoffset);
    nalType = (enum NAL_TYPE)(buff[0]&0x1f);
    if(nalType == NAL_SPS)
    {
        spsLen    = len;
        if(flagSPSOK == 1)
        {
            if(nalSPS != NULL)
                free(nalSPS);
        }
        nalSPS = (char *)malloc(spsLen + 1);
        if(nalSPS == NULL)
            return -1;
        memcpy(nalSPS, buff, spsLen);
        flagSPSOK = 1;
        
        return 0;
    }
    else if(nalType == NAL_PPS)
    {
        if(flagSPSOK == 1)
        {
            ppsLen = len;
            nalPPS = (char *)malloc(ppsLen + 1);
            if(nalPPS == NULL)
                return -1;
            ppsLen = len;
            memcpy(nalPPS, buff, ppsLen);

//*******************发送SPS PPS**********************************************
            packet->m_nBodySize = spsLen + ppsLen + 16;
            
            packet->m_body[0] = 0x17;

            /*nal unit*/
            packet->m_body[1]    = 0x00;   
            packet->m_body[2]    = 0x00;
            packet->m_body[3]    = 0x00;
            packet->m_body[4]    = 0x00;
   
            packet->m_body[5]    = 0x01;
            packet->m_body[6]    = nalSPS[1];
            packet->m_body[7]    = nalSPS[2];
            packet->m_body[8]    = nalSPS[3];
            packet->m_body[9]    = 0xFF;

            /*sps*/
            packet->m_body[10]   = 0xe1;
            packet->m_body[11]   = (spsLen >> 8) & 0xff;
            packet->m_body[12]   = (spsLen) & 0xff;

            memcpy(&(packet->m_body[13]), nalSPS, spsLen);
            /*pps*/
            packet->m_body[13 + spsLen]   = 0x01;
            packet->m_body[14 + spsLen] = (ppsLen >> 8) & 0xff;
            packet->m_body[15 + spsLen] = (ppsLen) & 0xff;
            memcpy(&(packet->m_body[16 + spsLen]), nalPPS, ppsLen);
//*******************发送SPS PPS**********************************************            
            if(nalSPS != NULL)
                free(nalSPS);
            if(nalPPS != NULL)
                free(nalPPS);
            flagSPSOK = 0;
        }
        else
            return 0;
    }
    else
    {
//*******************发送AVC**********************************************    
        packet->m_nBodySize = len + AVC_HEAD_SIZE;
    
        /*send video packet*/
    
        /*key frame*/
        if (nalType == NAL_SLICE_IDR)
            packet->m_body[0] = 0x17;
        else
            packet->m_body[0] = 0x27;
    
        /*nal unit*/
        packet->m_body[1] = 0x01;   
        packet->m_body[2] = 0x00;
        packet->m_body[3] = 0x00;
        packet->m_body[4] = 0x00;
    
        packet->m_body[5] = (len >> 24) & 0xff;
        packet->m_body[6] = (len >> 16) & 0xff;
        packet->m_body[7] = (len >>  8) & 0xff;
        packet->m_body[8] = (len ) & 0xff;
        /*copy data*/
        memcpy(&packet->m_body[9], buff, len);
    }
    packet->m_hasAbsTimestamp = 0;
    packet->m_packetType      = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nInfoField2     = rtmp->m_stream_id;
    packet->m_nChannel        = 0x04;
    packet->m_headerType      = RTMP_PACKET_SIZE_LARGE;
    packet->m_nTimeStamp      = timeoffset;
    printf("rtmp->m_sb.sb_socket is %d\n", rtmp->m_sb.sb_socket);
    if(!RTMP_IsConnected(rtmp))
    {//确认连接
        RTMP_Log(RTMP_LOGERROR,"rtmp is not connect\n");
        return -1;
    }

    /*调用发送接口*/
    if(RTMP_SendPacket(rtmp, packet, TRUE) == 1)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

void Rtmp_Close(RTMP *rtmp, RTMPPacket *packet)
{
    if(packet != NULL)
        RTMPPacket_Free(packet);
    if(rtmp != NULL)
    {
        RTMP_Close(rtmp); 
        RTMP_Free(rtmp); 
    }
}




#ifndef _RTMPCLIENT_H_
#define _RTMPCLIENT_H_
#include <stdint.h>
#include <librtmp/rtmp.h>
#include <librtmp/log.h>


#ifdef __cplusplus
extern "C"{
#endif

#define  PACKET_MAX_SIZE 4096*4096
enum NAL_TYPE
{
    NAL_SLICE     = 1,
    NAL_SLICE_DPA = 2,
    NAL_SLICE_DPB = 3,
    NAL_SLICE_DPC = 4,
    NAL_SLICE_IDR = 5,
    NAL_SEI       = 6,
    NAL_SPS       = 7,
    NAL_PPS       = 8,
    NAL_AUD       = 9,
    NAL_FILLER    = 12,
};

int Rtmp_Connect(RTMP *rtmp, RTMPPacket *packet, char *serverAddr);
int Rtmp_Send(RTMP *rtmp, RTMPPacket *packet, unsigned char *buff, long len);
void Rtmp_Close(RTMP *rtmp, RTMPPacket *packet);

#ifdef __cplusplus
}
#endif
#endif


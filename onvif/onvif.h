#ifndef _ONVIF_H_ 
#define _ONVIF_H_

typedef struct Profiles
{
    char *token;
    char *ptzToken;
    char *encoding;
    int width;
    int height;
    int frameRateLimit;
    int bitrateLimit;
    int govLength;

    char *streamUri;
}Profiles;

typedef struct Device
{
    char *xAddr;
    char *xAddrAnalytics;
    char *xAddrDevice;
    char *xAddrEvents;
    char *xAddrImaging;
    char *xAddrMedia;
    char *xAddrPtz;

    char *username;
    char *password;
    int  deviceName;

    int profileNum;
    struct Profiles *profiles;
    struct Device *next;
}Device;

typedef struct DeviceList
{
    int num;                 //发现设备的数量
    struct Device *list;     //设备链表
}DeviceList;

int  ONVIF_Remote_Discovery_Detect_Device(DeviceList *deviceList);
int  ONVIF_Get_Capabilities(Device *device);
int  ONVIF_Get_Media_Profiles(Device *device);
int  ONVIF_Get_Media_Stream_Uri(Profiles *profiles, Device *device);


void ONVIF_Free(DeviceList *deviceList);

#endif

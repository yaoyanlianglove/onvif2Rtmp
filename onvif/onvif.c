#include "onvif.h"
#include <stdio.h>
#include <stdlib.h>
#include "./soap2C/wsaapi.h"
#include "soapDebug.h"
#include "soapMalloc.h"

#define SOAP_TO         "urn:schemas-xmlsoap-org:ws:2005:04:discovery"
#define SOAP_ACTION     "http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe"

#define SOAP_MCAST_ADDR "soap.udp://239.255.255.250:3702"                       

#define SOAP_ITEM       ""                                                     
#define SOAP_TYPES      "dn:NetworkVideoTransmitter"                           

                                              

static int ONVIF_Remote_Discovery_Init_Header(struct soap *soap)
{
    struct SOAP_ENV__Header *header = NULL;

    if(soap == NULL)
        return -1;

    header = (struct SOAP_ENV__Header *)ONVIF_Soap_Malloc(soap, sizeof(struct SOAP_ENV__Header));
    soap_default_SOAP_ENV__Header(soap, header);
    header->wsa__MessageID = (char*)soap_wsa_rand_uuid(soap);
    header->wsa__To        = (char*)ONVIF_Soap_Malloc(soap, strlen(SOAP_TO) + 1);
    header->wsa__Action    = (char*)ONVIF_Soap_Malloc(soap, strlen(SOAP_ACTION) + 1);
    strcpy(header->wsa__To, SOAP_TO);
    strcpy(header->wsa__Action, SOAP_ACTION);
    soap->header = header;

    return 0;
}

static int ONVIF_Remote_Discovery_Init_Probe_Type(struct soap *soap, struct wsdd__ProbeType *probe)
{
    struct wsdd__ScopesType *scope = NULL;                                      // 用于描述查找哪类的Web服务

    if(soap == NULL || probe == NULL)
        return -1;

    scope = (struct wsdd__ScopesType *)ONVIF_Soap_Malloc(soap, sizeof(struct wsdd__ScopesType));
    soap_default_wsdd__ScopesType(soap, scope);                                 // 设置寻找设备的范围
    scope->__item = (char*)ONVIF_Soap_Malloc(soap, strlen(SOAP_ITEM) + 1);
    strcpy(scope->__item, SOAP_ITEM);

    memset(probe, 0x00, sizeof(struct wsdd__ProbeType));
    soap_default_wsdd__ProbeType(soap, probe);
    probe->Scopes = scope;
    probe->Types  = (char*)ONVIF_Soap_Malloc(soap, strlen(SOAP_TYPES) + 1);     // 设置寻找设备的类型
    strcpy(probe->Types, SOAP_TYPES);

    return 0;
}

int ONVIF_Remote_Discovery_Detect_Device(DeviceList *deviceList)
{
    int i;
    int result = 0;
    unsigned int count = 0;                                                     // 搜索到的设备个数
    struct soap *soap = NULL;                                                   // soap环境变量
    struct wsdd__ProbeType      req;                                            // 用于发送Probe消息
    struct __wsdd__ProbeMatches rep;                                            // 用于接收Probe应答
    struct wsdd__ProbeMatchType *probeMatch;
    Device *pHead; 

    soap = ONVIF_Soap_New();
    if(soap == NULL)
        return -1;
    result = ONVIF_Remote_Discovery_Init_Header(soap);
    if(result != 0)
    {
        ONVIF_Soap_Free(soap);
        return -1;                                   
    }
    result = ONVIF_Remote_Discovery_Init_Probe_Type(soap, &req); 
    if(result != 0)
    {
        ONVIF_Soap_Free(soap);
        return result;                        
    }
    result = soap_send___wsdd__Probe(soap, SOAP_MCAST_ADDR, NULL, &req);        // 向组播地址广播Probe消息
    while (SOAP_OK == result)                                                   // 开始循环接收设备发送过来的消息
    {
        memset(&rep, 0x00, sizeof(rep));
        result = soap_recv___wsdd__ProbeMatches(soap, &rep);
        if (SOAP_OK == result) 
        {
            if (soap->error) 
            {
                SOAP_DEBUG(soap, "ONVIF_Remote_Discovery_Detect_Device\n");
            } 
            else 
            {                                                                   
                if (NULL != rep.wsdd__ProbeMatches) 
                {
                    count += rep.wsdd__ProbeMatches->__sizeProbeMatch;
                    for(i = 0; i < rep.wsdd__ProbeMatches->__sizeProbeMatch; i++) 
                    {
                        deviceList->num = i + 1;
                        probeMatch = rep.wsdd__ProbeMatches->ProbeMatch + i;
                        pHead = (Device *)malloc(sizeof(Device));
                        if(pHead == NULL)
                        {
                            ONVIF_Soap_Free(soap);
                            return -1;
                        }
                        pHead->xAddr = malloc(strlen(probeMatch->XAddrs) + 1);
                        if(pHead->xAddr == NULL)
                        {
                            ONVIF_Soap_Free(soap);
                            return -1;
                        }
                        strcpy(pHead->xAddr, probeMatch->XAddrs);
                        if(deviceList->list == NULL)
                        {
                            pHead->next = NULL;
                            deviceList->list = pHead;
                        }
                        else
                        {
                            pHead->next     = deviceList->list;
                            deviceList->list = pHead;
                        }
                        pHead->xAddrAnalytics = NULL;
                        pHead->xAddrDevice    = NULL;
                        pHead->xAddrEvents    = NULL;
                        pHead->xAddrImaging   = NULL;
                        pHead->xAddrMedia     = NULL;
                        pHead->xAddrPtz       = NULL;
                        pHead->username       = NULL;
                        pHead->password       = NULL;
                    }
                }
            }
        } 
        else if (soap->error) 
        {
            break;
        }
    }
    if (NULL != soap) 
    {
        ONVIF_Soap_Free(soap);
    }
    return 0;
}

/*
 * onvif的deviceList内存释放
 */
void ONVIF_Free(DeviceList *deviceList)
{
    Device *pHead, *p;
    pHead = deviceList->list;
    while(pHead != NULL)
    {
        p     = pHead;
        pHead = pHead->next;
        if(p->xAddr != NULL)
            free(p->xAddr);
        if(p->xAddrAnalytics != NULL)
            free(p->xAddrAnalytics);
        if(p->xAddrDevice != NULL)
            free(p->xAddrDevice);
        if(p->xAddrEvents != NULL)
            free(p->xAddrEvents);
        if(p->xAddrImaging != NULL)
            free(p->xAddrImaging);
        if(p->xAddrMedia != NULL)
            free(p->xAddrMedia);
        if(p->xAddrPtz != NULL)
            free(p->xAddrPtz);
        if(p->username != NULL)
            free(p->username);
        if(p->password != NULL)
            free(p->password);
        for(int i = 0; i < p->profileNum; i++)
        {
            if(p->profiles[i].token != NULL)
                free(p->profiles[i].token);
            if(p->profiles[i].ptzToken != NULL)
                free(p->profiles[i].ptzToken);
            if(p->profiles[i].encoding != NULL)
                free(p->profiles[i].encoding);
            if(p->profiles[i].streamUri != NULL)
                free(p->profiles[i].streamUri);
        }
        if(p->profiles != NULL)
            free(p->profiles);
        if(p != NULL)
            free(p);
    }
}
/*
 * 获取设备能力
 */
int ONVIF_Get_Capabilities(Device *device)
{
    int result = 0;
    char *p;
    struct SOAP_ENV__Header *header = NULL;
    struct soap *soap = NULL;
    struct _tds__GetCapabilities req;
    struct _tds__GetCapabilitiesResponse rep;

    char username[128] = "admin";
    char password[64] = "123456";

    enum tt__CapabilityCategory category;
    
    soap = ONVIF_Soap_New();
    if(soap == NULL)
        return -1;
    category = tt__CapabilityCategory__All;
    req.Category       = &category;
    req.__sizeCategory = 1;

    header = (struct SOAP_ENV__Header *)ONVIF_Soap_Malloc(soap, sizeof(struct SOAP_ENV__Header));
    soap_default_SOAP_ENV__Header(soap, header);
    result = soap_call___tds__GetCapabilities(soap, device->xAddr, NULL, &req, &rep);
    if(result != SOAP_OK) 
    {
        SOAP_DEBUG(soap, "ONVIF_Get_Capabilities");
    }
    else
    {
        if(rep.Capabilities->Analytics != NULL)
        {
            p = rep.Capabilities->Analytics->XAddr;
            if(p != NULL)
            {
                device->xAddrAnalytics = malloc(strlen(p) + 1);
                if(device->xAddrAnalytics == NULL)
                {
                    ONVIF_Soap_Free(soap);
                    return -1;
                }
                strcpy(device->xAddrAnalytics, p);
            }
            else
                printf("rep.Capabilities->Analytics->XAddr is NULL\n");
        }
        if(rep.Capabilities->Device != NULL)
        {
            p = rep.Capabilities->Device->XAddr;
            if(p != NULL)
            {
                device->xAddrDevice= malloc(strlen(p) + 1);
                if(device->xAddrDevice == NULL)
                {
                    ONVIF_Soap_Free(soap);
                    return -1;
                }
                strcpy(device->xAddrDevice, p);
            }
            else
                printf("rep.Capabilities->Device->XAddr is NULL\n");
        }
        if(rep.Capabilities->Events != NULL)
        {
            p = rep.Capabilities->Events->XAddr;
            if(p != NULL)
            {
                device->xAddrEvents= malloc(strlen(p) + 1);
                if(device->xAddrEvents == NULL)
                {
                    ONVIF_Soap_Free(soap);
                    return -1;
                }
                strcpy(device->xAddrEvents, p);
            }
            else
                printf("rep.Capabilities->Events->XAddr is NULL\n");
        }
        if(rep.Capabilities->Imaging != NULL)
        {
            p = rep.Capabilities->Imaging->XAddr;
            if(p != NULL)
            {
                device->xAddrImaging = malloc(strlen(p) + 1);
                if(device->xAddrImaging == NULL)
                {
                    ONVIF_Soap_Free(soap);
                    return -1;
                }
                strcpy(device->xAddrImaging, p);
            }
            else
                printf("rep.Capabilities->Imaging->XAddr is NULL\n");
        }
        if(rep.Capabilities->Media != NULL)
        {
            p = rep.Capabilities->Media->XAddr;
            if(p != NULL)
            {
                device->xAddrMedia = malloc(strlen(p) + 1);
                if(device->xAddrMedia == NULL)
                {
                    ONVIF_Soap_Free(soap);
                    return -1;
                }
                strcpy(device->xAddrMedia, p);
            }
            else
                printf("rep.Capabilities->Media->XAddr is NULL\n");
        }
        if(rep.Capabilities->PTZ != NULL)
        {
            p = rep.Capabilities->PTZ->XAddr;
            if(p != NULL)
            {
                device->xAddrPtz = malloc(strlen(p) + 1);
                if(device->xAddrPtz == NULL)
                {
                    ONVIF_Soap_Free(soap);
                    return -1;
                }
                strcpy(device->xAddrPtz, p);
            }
            else
                printf("rep.Capabilities->PTZ->XAddr is NULL\n");
        }
        device->username = malloc(128);
        if(device->username == NULL)
        {
            ONVIF_Soap_Free(soap);
            return -1;
        }
        strcpy(device->username, username);
        device->password = malloc(64);
        if(device->password == NULL)
        {
            ONVIF_Soap_Free(soap);
            return -1;
        }
        strcpy(device->password, password);
    }
    
    if (NULL != soap) 
    {
        ONVIF_Soap_Free(soap);
    }
    return 0;
}
/*
 * 获取视频profiles
 */
int ONVIF_Get_Media_Profiles(Device *device)
{
    int result = 0;
    struct SOAP_ENV__Header *header = NULL;
    struct soap *soap = NULL;
    struct _trt__GetProfiles req;
    struct _trt__GetProfilesResponse rep;
    char buff[64] = {0};
    
    soap = ONVIF_Soap_New();
    if(soap == NULL)
        return -1;
    
    header = (struct SOAP_ENV__Header *)ONVIF_Soap_Malloc(soap, sizeof(struct SOAP_ENV__Header));
    soap_default_SOAP_ENV__Header(soap, header);

    result = soap_wsse_add_UsernameTokenDigest(soap, NULL, device->username, device->password);
    if(result != SOAP_OK) 
    {
        SOAP_DEBUG(soap, "soap_wsse_add_UsernameTokenDigest");
        ONVIF_Soap_Free(soap);
        return -1;
    }
    result = soap_call___trt__GetProfiles(soap, device->xAddrMedia, NULL, &req, &rep);
    if(result != SOAP_OK) 
    {
        SOAP_DEBUG(soap, "ONVIF_Get_Media_Profiles");
    }
    else 
    {
        device->profileNum = rep.__sizeProfiles;
        device->profiles = (Profiles *)malloc(sizeof(Profiles) * device->profileNum);
        if(device->profiles == NULL)
        {
            ONVIF_Soap_Free(soap);
            return -1;
        }
        for(int i = 0; i < device->profileNum; i++)
        {
            device->profiles[i].token = malloc(strlen(rep.Profiles[i].token) + 1);
            if(device->profiles[i].token == NULL)
            {
                ONVIF_Soap_Free(soap);
                return -1;
            }
            strcpy(device->profiles[i].token, rep.Profiles[i].token);

            device->profiles[i].ptzToken = malloc(strlen(rep.Profiles[i].PTZConfiguration->token) + 1);
            if(device->profiles[i].ptzToken == NULL)
            {
                ONVIF_Soap_Free(soap);
                return -1;
            }
            strcpy(device->profiles[i].ptzToken, rep.Profiles[i].PTZConfiguration->token);

            switch(rep.Profiles[i].VideoEncoderConfiguration->Encoding)
            {
                case tt__VideoEncoding__JPEG:
                sprintf(buff, "JPEG\n");
                break;
                case tt__VideoEncoding__MPEG4:
                sprintf(buff, "MPEG4\n");
                break;
                case tt__VideoEncoding__H264:
                sprintf(buff, "H264\n");
                break;
                default:
                sprintf(buff, "H264\n");
            }

            device->profiles[i].encoding = malloc(strlen(buff) + 1);
            if(device->profiles[i].encoding == NULL)
            {
                ONVIF_Soap_Free(soap);
                return -1;
            }
            strcpy(device->profiles[i].encoding, buff);

            device->profiles[i].streamUri      = NULL;
            device->profiles[i].width          = rep.Profiles[i].VideoEncoderConfiguration->Resolution->Width;
            device->profiles[i].height         = rep.Profiles[i].VideoEncoderConfiguration->Resolution->Height;
            device->profiles[i].frameRateLimit = rep.Profiles[i].VideoEncoderConfiguration->RateControl->FrameRateLimit;
            device->profiles[i].bitrateLimit   = rep.Profiles[i].VideoEncoderConfiguration->RateControl->BitrateLimit;
            device->profiles[i].govLength      = rep.Profiles[i].VideoEncoderConfiguration->H264->GovLength;
        }
        printf("ONVIF_Get_Media_Profiles OK.\n");
    }
    if(NULL != soap) 
    {
        ONVIF_Soap_Free(soap);
    }
    return 0;
}
/*
 * 获取视频流
 */
int ONVIF_Get_Media_Stream_Uri(Profiles *profiles, Device *device)
{
    int result = 0;
    struct SOAP_ENV__Header *header = NULL;
    struct soap *soap = NULL;
    struct _trt__GetStreamUri req;
    struct _trt__GetStreamUriResponse rep;
    struct tt__StreamSetup streamSetup;
    struct tt__Transport transport;
    
    req.ProfileToken      = profiles->token;
    streamSetup.Transport = &transport;
    req.StreamSetup       = &streamSetup;

    req.StreamSetup->Stream              = tt__StreamType__RTP_Unicast;
    req.StreamSetup->Transport->Protocol = tt__TransportProtocol__RTSP;
    soap = ONVIF_Soap_New();
    if(soap == NULL)
        return -1;
    header = (struct SOAP_ENV__Header *)ONVIF_Soap_Malloc(soap, sizeof(struct SOAP_ENV__Header));
    soap_default_SOAP_ENV__Header(soap, header);

    result = soap_wsse_add_UsernameTokenDigest(soap, NULL, device->username, device->password);
    if(result != SOAP_OK) 
    {
        SOAP_DEBUG(soap, "soap_wsse_add_UsernameTokenDigest");
        ONVIF_Soap_Free(soap);
        return -1;
    }
    result = soap_call___trt__GetStreamUri(soap, device->xAddrMedia, NULL, &req, &rep);
    if(result != SOAP_OK) 
    {
        SOAP_DEBUG(soap, "ONVIF_Get_Media_Stream_Uri");
    }
    else 
    {
        profiles->streamUri = malloc(strlen(rep.MediaUri->Uri) + 1);
        if(profiles->streamUri == NULL)
        {
            ONVIF_Soap_Free(soap);
            return -1;
        }
        strcpy(profiles->streamUri, rep.MediaUri->Uri);
        printf("ONVIF_Get_Media_Stream_Uri OK.\n");
    }
    if(NULL != soap) 
    {
        ONVIF_Soap_Free(soap);
    }
    return 0;
}



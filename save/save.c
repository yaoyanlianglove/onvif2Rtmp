#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "save.h"
#include "iniparser.h"

int Save_Device_List(DeviceList *deviceList)
{
    FILE *fp;
    Device *p;
    fp = fopen("./deviceList.ini", "w+");  
    if(fp == NULL)  
    {
        printf("File deviceList open failed.\n");
        return -1;
    }
    p = deviceList->list;
    fprintf(fp, "[deviceList]\n"
                        "num                   = %d\n", deviceList->num);
    while(p != NULL)
    {
        fprintf(fp, "[device%d]\n"
                        "xAddr                 = %s\n", p->deviceName, p->xAddr);
        if(p->xAddrAnalytics != NULL)
        {
            fprintf(fp, "xAddrAnalytics        = %s\n", p->xAddrAnalytics); 
        }
        if(p->xAddrDevice != NULL)
        {
            fprintf(fp, "xAddrDevice           = %s\n", p->xAddrDevice); 
        }
        if(p->xAddrEvents != NULL)
        {
            fprintf(fp, "xAddrEvents           = %s\n", p->xAddrEvents); 
        }
        if(p->xAddrImaging != NULL)
        {
            fprintf(fp, "xAddrImaging          = %s\n", p->xAddrImaging); 
        }
        if(p->xAddrMedia != NULL)
        {
            fprintf(fp, "xAddrMedia            = %s\n", p->xAddrMedia); 
        }
        if(p->xAddrPtz != NULL)
        {
            fprintf(fp, "xAddrPtz              = %s\n", p->xAddrPtz); 
        }
        if(p->username != NULL)
        {
            fprintf(fp, "username              = %s\n", p->username); 
        }
        if(p->password != NULL)
        {
            fprintf(fp, "password              = %s\n", p->password); 
        }
        p = p->next;
    }
    fclose(fp);
    return 0;
}
static char* Read_Device_Ini(dictionary *ini, char *key, int name)
{
    char keyName[64] = {0};
    const char *buff;
    int length = 0;
    char *data;
    sprintf(keyName, "device%d:%s", name, key);
    buff = iniparser_getstring(ini, keyName, NULL);
    if(buff != NULL)
    {
        length = strlen(buff) + 1;
        data = malloc(length);
        if(data == NULL)
        {
            return NULL;
        }
        strncpy(data, buff, length);
    }
    else
        data = NULL;
    return data;

}
int Read_Device_List(DeviceList *deviceList)
{
    Device *pHead;
    int num   = 0;
    int count = 0;
    
    int i;
    dictionary *ini         = NULL;
    //加载文件
    ini = iniparser_load("./deviceList.ini");
    if(ini == NULL) 
    {
        printf("stone:iniparser_load error!\n");
        return -1;
    }
    num = iniparser_getint(ini, "deviceList:num", -1);
    if(num == -1)
    {
        iniparser_freedict(ini);
        return -1;
    }
    for(i = 0; i < num; i++)
    {
        pHead = (Device *)malloc(sizeof(Device));
        if(pHead == NULL)
        {
            iniparser_freedict(ini);
            return -1;
        }
        pHead->xAddr          = Read_Device_Ini(ini, "xAddr", i);
        pHead->xAddrAnalytics = Read_Device_Ini(ini, "xAddrAnalytics", i);
        pHead->xAddrDevice    = Read_Device_Ini(ini, "xAddrDevice", i);
        pHead->xAddrEvents    = Read_Device_Ini(ini, "xAddrEvents", i);
        pHead->xAddrImaging   = Read_Device_Ini(ini, "xAddrImaging", i);
        pHead->xAddrMedia     = Read_Device_Ini(ini, "xAddrMedia", i);
        pHead->xAddrPtz       = Read_Device_Ini(ini, "xAddrPtz", i);
        pHead->username       = Read_Device_Ini(ini, "username", i);
        pHead->password       = Read_Device_Ini(ini, "password", i);
        
        if(deviceList->list == NULL)
        {
            pHead->next = NULL;
            deviceList->list = pHead;
        }
        else
        {
            pHead->next      = deviceList->list;
            deviceList->list = pHead;
        }
        count++;
    }
    deviceList->num = count;
    iniparser_freedict(ini);
    return 0;
}
int Save_Media_Profiles(Profiles *profiles, int name, int profileNum)
{
    FILE *fp;
    Device *p;
    char fileName[64] = {0};
    sprintf(fileName, "device%d.ini", name);
    fp = fopen(fileName, "w+");  
    if(fp == NULL)  
    {
        printf("File device%d open failed.\n", name);
        return -1;
    }
    for(int i = 0; i < profileNum; i++)
    {
        fprintf(fp, "[%s]\n"
                    "width=%d\n"
                    "height=%d\n"
                    "frameRateLimit=%d\n"
                    "bitrateLimit=%d\n"
                    "govLength=%d\n#ptz\n"
                    "ptzToken=%s\n", profiles[i].token, profiles[i].width, profiles[i].height, 
                    profiles[i].frameRateLimit, profiles[i].bitrateLimit, profiles[i].govLength, 
                    profiles[i].ptzToken);
    }
    fclose(fp);
    return 0;
}
int Save_Media_Stream_Uri(Profiles *profiles, int name)
{
    int res = 0;
    FILE *fp;
    dictionary *ini         = NULL;
    char fileName[64] = {0};
    char keyName[64] = {0};
    sprintf(fileName, "device%d.ini", name);
    //加载文件
    ini = iniparser_load(fileName);
    if(ini == NULL) 
    {
        printf("stone:iniparser_load error!\n");
        return -1;
    }
    fp = fopen(fileName, "w+"); 
    if(fp == NULL)  
    {
        printf("File device%d open failed.\n", name);
        iniparser_freedict(ini);
        return -1;
    }
    sprintf(keyName, "%s:streamUri", profiles->token);
    res = iniparser_set(ini, keyName, profiles->streamUri);
    iniparser_dump_ini(ini, fp);
    iniparser_freedict(ini);
    fclose(fp);
    return res;
}

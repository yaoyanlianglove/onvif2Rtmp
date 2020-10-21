#include "main.h"

int main(int argc, char* argv[])
{
    printf("**********************onvif start**********************\n");
    int argvNum;
    DeviceList deviceList;
    deviceList.list = NULL;
    Device *p;
    int res = 0;
    int i;
    int deviceName = 0;
    if(argc < 2)
    {
        printf("please input like: ./onvif2Rtmp 1\n"
            "1---Remote_Discovery;\n"
            "2---Get_Profiles;\n");
        return 0;
    }
    argvNum = atoi(argv[1]);

    switch(argvNum)
    {
        case 1:
        printf("Remote_Discovery start\n");
        ONVIF_Remote_Discovery_Detect_Device(&deviceList);
        p = deviceList.list;
        while(p != NULL)
        {
            res = ONVIF_Get_Capabilities(p);
            if(res != 0)
            {
                printf("%s Get_Capabilities failed\n", p->xAddr);
                continue;
            }
            p->deviceName = deviceName;
            deviceName++;
            p = p->next;
        }
        res = Save_Device_List(&deviceList);
        if(res != 0)
        {
            printf("Save_Device_List failed\n");
        }
        break;
        case 2:
        printf("Get_Profiles start\n");
        res = Read_Device_List(&deviceList);
        if(res != 0)
        {
            printf("Read_Device_List failed\n");
            break;
        }
        p = deviceList.list;
        while(p != NULL)
        {
            res = ONVIF_Get_Media_Profiles(p);
            if(res != 0)
            {
                printf("%s ONVIF_Get_Media_Profiles failed\n", p->xAddr);
                continue;
            }
            res = Save_Media_Profiles(p->profiles, p->deviceName, p->profileNum);
            if(res != 0)
            {
                printf("Save_Media_Profiles failed\n");
            }
            for(i = 0; i < p->profileNum; i++)
            {
                res = ONVIF_Get_Media_Stream_Uri(&(p->profiles[i]), p);
                if(res == 0)
                {
                    Save_Media_Stream_Uri(&(p->profiles[i]), p->deviceName);
                }
            }
            p = p->next;
        }
        break;
        default:
        printf("Second parameter error\n");
    }
    ONVIF_Free(&deviceList);
    return 0;
}


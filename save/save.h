#ifndef _SAVE_H_ 
#define _SAVE_H_


#include "../onvif/onvif.h"


int Save_Device_List(DeviceList *deviceList);
int Read_Device_List(DeviceList *deviceList);
int Save_Media_Profiles(Profiles *profiles, int name, int profileNum);
int Save_Media_Stream_Uri(Profiles *profiles, int name);
#endif
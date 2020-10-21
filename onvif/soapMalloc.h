#ifndef _SOAP_MALLOC_H_ 
#define _SOAP_MALLOC_H_

#include "./soap2C/soapH.h"

#define SOAP_SOCK_TIMEOUT    (5) 



void* ONVIF_Soap_Malloc(struct soap *soap, unsigned int n);

void  ONVIF_Soap_Free(struct soap *soap);

struct soap *ONVIF_Soap_New(void);


#endif
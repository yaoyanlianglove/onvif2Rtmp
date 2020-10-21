#include "soapDebug.h"
#include <stdio.h>


void SOAP_DEBUG(struct soap *soap, const char *str)
{
#ifdef DEBUG_OPEN
    if (NULL == str) 
    {
        printf("[soap] error: %d, %s, %s\n", soap->error, *soap_faultcode(soap), *soap_faultstring(soap));
    } 
    else 
    {
        printf("[soap] %s error: %d, %s, %s\n", str, soap->error, *soap_faultcode(soap), *soap_faultstring(soap));
    }
#endif
}
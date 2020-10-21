
#include "soapMalloc.h"
#include <stdio.h>

void* ONVIF_Soap_Malloc(struct soap *soap, unsigned int n)
{
    void *p = NULL;

    if (n > 0) 
    {
        p = soap_malloc(soap, n);
        if(p == NULL)
        {
            printf("soap_malloc is NULL\n");
            return NULL;
        }
        memset(p, 0x00 ,n);
    }
    return p;
}

struct soap *ONVIF_Soap_New(void)
{
    struct soap *soap = NULL;                                                   // soap环境变量

    soap = soap_new();
    if(soap == NULL)
    {
        printf("soap_new is NULL\n");
        return soap;
    }

    soap_set_namespaces(soap, namespaces);                                      // 设置soap的namespaces
    soap->recv_timeout    = SOAP_SOCK_TIMEOUT;                                            // 设置超时（超过指定时间没有数据就退出）
    soap->send_timeout    = SOAP_SOCK_TIMEOUT;
    soap->connect_timeout = SOAP_SOCK_TIMEOUT;

#if defined(__linux__) || defined(__linux)                                      // 参考https://www.genivia.com/dev.html#client-c的修改：
    soap->socket_flags = MSG_NOSIGNAL;                                          // To prevent connection reset errors
#endif

    soap_set_mode(soap, SOAP_C_UTFSTRING);                                      // 设置为UTF-8编码，否则叠加中文OSD会乱码

    return soap;
}

void ONVIF_Soap_Free(struct soap *soap)
{
    soap_destroy(soap);                                                         // remove deserialized class instances (C++ only)
    soap_end(soap);                                                             // Clean up deserialized data (except class instances) and temporary data
    soap_done(soap);                                                            // Reset, close communications, and remove callbacks
    soap_free(soap);                                                            // Reset and deallocate the context created with soap_new or soap_copy
}

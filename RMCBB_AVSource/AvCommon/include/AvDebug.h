/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __AVDEBUG_H__
#define __AVDEBUG_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>


#define _AVD_APP_           0x01
#define _AVD_DEVICE_        0x02
#define _AVD_VI_            0x04
#define _AVD_VO_            0x08
#define _AVD_SYSTEM_        0x10
#define _AVD_MCC_           0x20
#define _AVD_VPSS_          0x40
#define _AVD_HDMI_          0x80
#define _AVD_PCIV_          0x100
#define _AVD_MSG_           0x200
#define _AVD_ENC_           0x400
#define _AVD_AI_            0x800
#define _AVD_AO_            0x1000
#define _AVD_VDA_           0x2000
#define _AVD_ISP_           0x4000
#define _AVD_SNAP_          0x8000


void Av_debug_level(unsigned long debug_info, unsigned long key_info);


/////////////////////////////////////////////////////////////////////////////////////

inline const char *Av_file_name(const char *pDir)
{
    const char *pTmp = strrchr(pDir, '/');

    if((pTmp != NULL) && (pTmp[1] != '\0'))
    {
        return pTmp + 1;
    }
    else
    {
        return pDir;
    }
}

extern unsigned long gAv_debug_info;
extern unsigned long gAv_key_info;


#define _AV_KEY_INFO_(module, fmt, args...) do{\
        if(gAv_key_info & module)\
        {\
            printf("[%s][%08d]", Av_file_name(__FILE__), __LINE__);\
            printf(fmt, ##args);\
        }\
    }while(0)


#define _AV_DEBUG_INFO_(module, fmt, args...) do{\
        if(gAv_debug_info & module)\
        {\
            printf("[%s][%08d]", Av_file_name(__FILE__), __LINE__);\
            printf(fmt, ##args);\
        }\
    }while(0)


#define _AV_FATAL_(exp, fmt, args...) do{\
        if(exp)\
        {\
            printf("FATAL:(FILE:[%s])(FUNC:[%s])(LINE:[%d])\n", Av_file_name(__FILE__), __FUNCTION__, __LINE__);\
            printf(fmt, ##args);\
            exit(1);\
        }\
    }while(0)


#define _AV_ASSERT_(exp, fmt, args...) do{\
        if(!(exp))\
        {\
            printf("ASSERT:(FILE:[%s])(FUNC:[%s])(LINE:[%d])\n", Av_file_name(__FILE__), __FUNCTION__, __LINE__);\
            printf(fmt, ##args);\
            exit(1);\
        }\
    }while(0)


#define _PRINT_CUR_TIME_(fmt, args...) do{\
            struct timeval cur_time;\
            gettimeofday(&cur_time, NULL);\
            printf("[%s][%08d]"fmt"[time:%ld, %ld]\n", Av_file_name(__FILE__), __LINE__, ##args, cur_time.tv_sec, cur_time.tv_usec);\
        }while(0)


#define _AV_POSITION_INFO_ do{\
        printf("POSITION:(FILE:[%s])(FUNC:[%s])(LINE:[%d])\n", Av_file_name(__FILE__), __FUNCTION__, __LINE__);\
    }while(0)


#define PRINT_TIME(fmt, args...) do{\
                struct timeval cur_time;\
                gettimeofday(&cur_time, NULL);\
                printf("[%s][%08d]"fmt"[time:%ld, %ld]\n", __FILE__, __LINE__, ##args, cur_time.tv_sec, cur_time.tv_usec);\
        }while(0)

#endif/*__AVDEBUG_H__*/


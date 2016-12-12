/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __AVCOMMONMACRO_H__
#define __AVCOMMONMACRO_H__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ALIGN_BACK(x, a) ((a) * (((x) / (a))))
#define ALIGN_UP(x, a)   ((a) * ((x+a-1)/(a)))

#define _AV_SAFE_FREE_(ptr) do{\
        if(ptr != NULL)\
        {\
            free(ptr);\
            ptr = NULL;\
        }\
    }while(0)


#define _AV_SAFE_DELETE_(ptr) do{\
        if(ptr != NULL)\
        {\
            delete ptr;\
            ptr = NULL;\
        }\
    }while(0)


#define _AV_SAFE_DELETE_ARRAY_(ptr) do{\
        if(ptr != NULL)\
        {\
            delete []ptr;\
            ptr = NULL;\
        }\
    }while(0)


#define _AV_SAFE_CLOSE_(fd) do{\
        if(fd >= 0)\
        {\
            close(fd);\
            fd = -1;\
        }\
    }while(0)


#define _AV_SAFE_FCLOSE_(fp) do{\
        if(fp != NULL)\
        {\
            fclose(fp);\
            fp = NULL;\
        }\
    }while(0)


#define _AV_POINTER_ASSIGNMENT_(ptr, value) do{\
    if(ptr != NULL)\
    {\
        *ptr = value;\
    }\
}while(0)

#define CHECK_RET_VOID(express,name)\
    do{\
        if (HI_SUCCESS != express)\
        {\
            printf("%s failed at %s: LINE: %d ! errno:%d \n",\
                name, __FUNCTION__, __LINE__, express);\
            return ;\
        }\
    }while(0)

#define CHECK_RET_INT(express,name)\
    do{\
        if (HI_SUCCESS != express)\
        {\
            printf("%s failed at %s: LINE: %d ! errno:%d \n",\
                name, __FUNCTION__, __LINE__, express);\
            return HI_FAILURE;\
        }\
    }while(0)		

#endif/*__AVCOMMONMACRO_H__*/


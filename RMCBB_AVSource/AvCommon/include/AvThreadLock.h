/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __AVTHREADLOCK_H__
#define __AVTHREADLOCK_H__
#include <pthread.h>
#include "AvConfig.h"
#include "AvDebug.h"
#include "CommonDebug.h"

class CAvThreadLock{
    public:
        CAvThreadLock();
        ~CAvThreadLock();

    public:
        int lock();
        int unlock();
        int trylock();

    private:
        pthread_mutex_t m_thread_lock;
};


#endif/*__AVTHREADLOCK_H__*/

#include "AvThreadLock.h"

CAvThreadLock::CAvThreadLock()
{
    _AV_FATAL_(pthread_mutex_init(&m_thread_lock, NULL) != 0, "CAvThreadLock::CAvThreadLock pthread_mutex_init FAILED\n");
}


CAvThreadLock::~CAvThreadLock()
{
    pthread_mutex_destroy(&m_thread_lock);
}


int CAvThreadLock::lock()
{
    if(pthread_mutex_lock(&m_thread_lock) != 0)
    {
        DEBUG_ERROR( "CAvThreadLock::lock pthread_mutex_lock FAILED\n");
        return -1;
    }

    return 0;
}

int CAvThreadLock::unlock()
{
    if(pthread_mutex_unlock(&m_thread_lock) != 0)
    {
        DEBUG_ERROR( "CAvThreadLock::unlock pthread_mutex_unlock FAILED\n");
        return -1;
    }

    return 0;
}

int CAvThreadLock::trylock()
{
    if(pthread_mutex_trylock(&m_thread_lock) != 0)
    {
        DEBUG_ERROR( "CAvThreadLock::trylock pthread_mutex_trylock FAILED\n");
        return -1;
    }

    return 0;
}


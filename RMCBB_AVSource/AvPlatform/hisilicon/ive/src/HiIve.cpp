#include <sys/mman.h>
#include <stdio.h>
#include "StreamBuffer.h"
#include "CommonDebug.h"
#include "HiIve.h"
#include "HiVpss.h"


CHiIve::CHiIve(data_sorce_e data_source_type, data_type_e data_type, void* data_source, CHiAvEncoder* mpEncoder):
    m_pData_source(data_source),m_eDate_source_type(data_source_type), m_eData_type(data_type)
{
    m_detection_result_notify_func = NULL;
    m_pEncoder = mpEncoder;
    m_Ive_process_func = NULL;
    m_thread_state = IVE_THREAD_EXIT;
    pthread_mutex_init(&m_mutex, NULL);;
    pthread_cond_init(&m_cond, NULL);
    m_thread_body = boost::bind(&CHiIve::process_body, this);
}
    
CHiIve::~CHiIve()
{
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
    ive_unregister_result_notify_cb();
}

void* CHiIve::thread_body(void* args)
{
    CHiIve* ive = static_cast<CHiIve*>(args);
    ive->m_thread_body();

    return NULL;
}


int CHiIve::_init()
{
    if((m_eData_type == _DATA_TYPE_YUV_420SP ||m_eData_type == _DATA_TYPE_YUV_422SP ) && m_eDate_source_type== _DATA_SRC_VI)
    {
         m_Ive_process_func = boost::bind(&CHiIve::process_raw_data_from_vi, this, _1);
    }
    else if((m_eData_type == _DATA_TYPE_YUV_420SP ||m_eData_type == _DATA_TYPE_YUV_422SP ) && m_eDate_source_type  == _DATA_SRC_VPSS)
    {
         m_Ive_process_func = boost::bind(&CHiIve::process_yuv_data_from_vpss, this, _1);
    }
    else
    {
         m_Ive_process_func = boost::bind(&CHiIve::process_raw_data_from_vi, this, _1);
    }
    
    init();

    return 0;
}
int CHiIve::_uninit()
{
    uninit();
    m_Ive_process_func = NULL;

    return 0;
}


int CHiIve::ive_start()
{
    if(m_thread_state == IVE_THREAD_EXIT)
    {
        _init();
        m_thread_state = IVE_THREAD_READY;
    }
        
    if(m_thread_state == IVE_THREAD_READY)
    {
        pthread_create(&m_thread_id, 0, &thread_body, this); 
    }

    return 0;
}
int CHiIve::ive_stop()
{
    if(m_thread_state == IVE_THREAD_START)
    {
        m_thread_state = IVE_THREAD_STOP;
        pthread_join(m_thread_id, NULL);
    }

    if(m_thread_state == IVE_THREAD_STOP)
    {
        _uninit();
        m_thread_state = IVE_THREAD_EXIT;
    }
    
    return 0;
}

void CHiIve::ive_register_result_notify_cb(boost::function< void (msgIPCIntelligentInfoNotice_t *)> notify_cb)
{
    m_detection_result_notify_func = notify_cb;
}

void CHiIve::ive_unregister_result_notify_cb()
{
    m_detection_result_notify_func = NULL;
}

int CHiIve::process_raw_data_from_vi(int phy_chn)
{
    return 0;
}

int CHiIve::process_yuv_data_from_vi(int phy_chn)
{
    return 0;
}

int CHiIve::process_yuv_data_from_vpss(int phy_chn)
{
    return 0;
}


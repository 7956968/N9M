/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __HI_IVE_H__
#define __HI_IVE_H__
#include "hi_comm_video.h"
#include "CommonTypes.h"
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include "ive_snap.h"
#include "HiAvEncoder.h"
#include "CommonDebug.h"


using namespace Common;

typedef enum _data_source_
{
    _DATA_SRC_VI = 0,
    _DATA_SRC_VPSS = 1,

    _DATA_SRC_UNKOWN = 100    
}data_sorce_e;

typedef enum _data_type_
{
    _DATA_TYPE_YUV_420SP = 0,
    _DATA_TYPE_YUV_422SP = 1,
    _DATA_TYPE_RAW_DATA = 2,

    _DATA_TYPE_UNKOWN = 100    
}data_type_e;

typedef enum thread_state
{
    IVE_THREAD_READY = 0,
    IVE_THREAD_START,
    IVE_THREAD_STOP,
    IVE_THREAD_EXIT
}ive_thread_state_e;

typedef enum ive_ctl_type
{
    IVE_CTL_SAVE_RESULT = 0,
    IVE_CTL_OVERLAY_OSD,
    IVE_CTL_SET_MATCH_DEGREE,
    IVE_CTL_UPDATE_BLACKLIST,

    IVE_CTL_UNDEFINED
}ive_ctl_type_e;

#define IVE_TYPE_HC (1)
#define IVE_TYPE_FD (2)
#define IVE_TYPE_LD (4)
#define IVE_TYPE_BLE (8)

#define IVE_SUPPORT_MAX_OSD_NUMBER (8)

class CHiIve{
    public:
        CHiIve(data_sorce_e data_source_type, data_type_e data_type, void* data_source, CHiAvEncoder* mpEncoder);
        virtual ~CHiIve();
    
    public:
        int ive_start();
        int ive_stop();
        void ive_register_result_notify_cb(boost::function< void (msgIPCIntelligentInfoNotice_t *)> notify_cb);
        void ive_unregister_result_notify_cb();
        virtual int ive_control(unsigned int ive_type, ive_ctl_type_e ctl_type, void* args) = 0;
        
    protected:
        virtual int init() = 0;
        virtual int uninit() = 0;
        virtual int process_body() = 0;
        virtual int process_raw_data_from_vi(int phy_chn);
        virtual int process_yuv_data_from_vi(int phy_chn);
        virtual int process_yuv_data_from_vpss(int phy_chn);  

    private:
        int _init();
        int _uninit();


   private:
        static void* thread_body(void* args);

    protected:
        void* m_pData_source;
        data_sorce_e m_eDate_source_type;
        data_type_e m_eData_type;
        boost::function<int (int)> m_Ive_process_func;
        boost::function<void (msgIPCIntelligentInfoNotice_t* pDetection_result)> m_detection_result_notify_func;
        CHiAvEncoder* m_pEncoder;

        pthread_t m_thread_id;
        ive_thread_state_e m_thread_state;
        pthread_mutex_t m_mutex;
        pthread_cond_t m_cond;   
        boost::function<void ()> m_thread_body;
};

#endif

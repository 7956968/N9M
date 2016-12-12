/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __HI_IVE_HC_H__
#define __HI_IVE_HC_H__

#include "HiIve.h"
#include "object_detection.h"

class CHiIveHc : public CHiIve
{

    public:
        //CHiIveHc();
        CHiIveHc(data_sorce_e data_source_type, data_type_e data_type, void* data_source, CHiAvEncoder* mpEncoder);
        virtual ~CHiIveHc();

    public:
        int ive_control(unsigned int ive_type, ive_ctl_type_e ctl_type, void* args);
        
    private:
        int hc_start_count();
        int hc_stop_count();

    private:
        int init();
        int uninit();
        int process_body();
        int process_raw_data_from_vi(int phy_chn);
        int process_yuv_data_from_vi(int phy_chn);
        int process_yuv_data_from_vpss(int phy_chn);
        
    private:
        int m_32Detect_handle;
        bool m_bHead_count_state;
        unsigned int m_u32Heads_number;
};

#endif


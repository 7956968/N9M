/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __HI_IVE_LD_H__
#define __HI_IVE_LD_H__

#include "HiIve.h"
#include "lane_departure.h"
#include "HiAvEncoder.h"
#include "ive_snap.h"

class CHiIveLd : public CHiIve
{

    public:
        //CHiIveHc();
        CHiIveLd(data_sorce_e data_source_type, data_type_e data_type, void* data_source, CHiAvEncoder* mpEncoder);
        virtual ~CHiIveLd();

    public:
        int ive_control(unsigned int ive_type, ive_ctl_type_e ctl_type, void* args);

    private:
        int init();
        int uninit();
        int process_body();
        int process_raw_data_from_vi(int phy_chn);
        int process_yuv_data_from_vi(int phy_chn);
        int process_yuv_data_from_vpss(int phy_chn); 
        
    private:
        int ive_recycle_resource(ive_snap_task_s* snap_info);
        int ive_ld_snap();
        int ive_single_snap(VIDEO_FRAME_INFO_S * frame_info, hi_snap_osd_t * snap_osd,int osd_num);  
     
    private:
        bool m_bCurrent_state;
        CIveSnap* m_ive_snap_thread;
        hi_snap_osd_t  m_snap_osd[IVE_SUPPORT_MAX_OSD_NUMBER]; //0:data time 1:GPS other:reserved
        
};


#endif


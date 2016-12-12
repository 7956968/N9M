/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __HIAVOD_H__
#define __HIAVOD_H__
#include "HiVpss.h"
#include "HiVi.h"
#include "HiAvIveOd.h"

#include <vector>

using namespace std;

typedef struct _av_od_chn_info_
{
    unsigned char chn_enable;//0:disable ,other value:enable
    unsigned char resevred[3];
    int chn_id;
    int chn_sensitivity;//the od sensotivity;
}av_od_chn_info_t;


class CHiAvOd
{
    public:
        CHiAvOd(CHiVpss *pHiVpss);
        CHiAvOd(CHiVi *pHiVi);
        ~CHiAvOd();

    public:
        int Av_start_ive_od(int phy_video_chn, unsigned char sensitivity);
        int Av_stop_ive_od(int phy_video_chn);
        
        unsigned int Av_get_vda_od_counter_value(int phy_video_Chn);
        int Av_reset_vda_od_counter(int phy_video_Chn);
        int Av_occlusion_detection_proc();

    private:
        CHiVpss* m_pHi_vpss;
        CHiVi* m_pHi_vi;
        CHiAvIveOd* m_pHi_od;
        unsigned int m_od_counter;
        av_od_chn_info_t* m_od_chn_info;
};

#endif


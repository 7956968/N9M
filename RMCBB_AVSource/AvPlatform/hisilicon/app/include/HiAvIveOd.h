/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __HIAVIVEOD_H__
#define __HIAVIVEOD_H__
#include <stdio.h>
#include <iostream>
#include <vector>

#include "hi_common.h"
#include "hi_comm_ive.h"

using namespace std;

typedef struct hiAvOD_LINEAR_DATA_S
{
    HI_S32 s32LinearNum;
    HI_S32 s32ThreshNum;
    POINT_S* pstLinearPoint;
}OD_LINEAR_DATA_S;


class CHiAvIveOd
{
    public:
        CHiAvIveOd();
        ~CHiAvIveOd();

    public:
        int av_od_init(HI_S32 occlusion_thresh_percent);
        int av_od_uninit();

        int av_od_change_linear_data(HI_S32 occlusion_thresh_percent);
        int av_od_analyse_frame(const VIDEO_FRAME_INFO_S& frame_info);

    private:
        int av_od_linear2D_classifer(POINT_S *pstChar, HI_S32 s32CharNum, POINT_S *pstLinearPoint, HI_S32 s32Linearnum);
        
    private:
        const unsigned short m_od_block_num_row;
        const unsigned short m_od_block_num_line;

#if defined(_AV_PLATFORM_HI3518C_)
        HI_U64 *m_pu64VirData;
        IVE_MEM_INFO_S m_dst_mem_info;
#else
        IVE_SRC_IMAGE_S m_src_image;
        IVE_DST_IMAGE_S m_dest_image;
#endif
        OD_LINEAR_DATA_S m_linear_data;
};

#endif


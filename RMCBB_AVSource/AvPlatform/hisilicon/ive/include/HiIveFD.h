/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __HI_IVE_FD_H__
#define __HI_IVE_FD_H__

#include "HiIve.h"
#include "object_detection.h"
#include "CommonDebug.h"
#include "CommonTypes.h"
#include "hi_comm_video.h"
#include "hi_comm_venc.h"

using namespace Common;

class CHiIveFd :public CHiIve
{
    public:
        CHiIveFd(data_sorce_e data_source_type, data_type_e data_type, void* data_source, CHiAvEncoder* mpEncoder);
        virtual ~CHiIveFd();

    public:
        int ive_control(unsigned int ive_type, ive_ctl_type_e ctl_type, void* args);
    private:
        int fd_start_dectection();
        int fd_stop_dectection();

    private:
        int init();
        int uninit();
        int process_body();
        int process_raw_data_from_vi(int phy_chn);
        int process_yuv_data_from_vi(int phy_chn);
        int process_yuv_data_from_vpss(int phy_chn);  
        int ive_fd_face_snap(VIDEO_FRAME_INFO_S* frame, RECT_S * crop_rect);
        int ive_fd_put_snap_frame_to_sm(VENC_STREAM_S* stream);

        
    private:
        int m_32Detect_handle;
        bool m_bFace_detection_state;
        HANDLE m_hSnapShareMemHandle;
};

#endif


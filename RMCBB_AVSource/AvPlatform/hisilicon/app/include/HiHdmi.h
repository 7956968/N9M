/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __HIHDMI_H__
#define __HIHDMI_H__
#include "AvConfig.h"
#include "AvDebug.h"
#include "AvCommonMacro.h"
#include "AvCommonType.h"
#include "AvDeviceInfo.h"
#include "mpi_hdmi.h"
#include "mpi_sys.h"
#include "CommonDebug.h"

class CHiHdmi{
    public:
        CHiHdmi();
        ~CHiHdmi();

    public:
        int HiHdmi_start_hdmi(av_vag_hdmi_resolution_t resolution);
        int HiHdmi_stop_hdmi();
		int HiHdmi_set_hdmi_mute(bool mute);

    private:
        HI_HDMI_VIDEO_FMT_E HiHdmi_get_video_format(av_vag_hdmi_resolution_t resolution);
};


#endif/*__HIHDMI_H__*/

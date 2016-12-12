/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __HIAVREGION_H__
#define __HIAVREGION_H__
#include "AvConfig.h"
#include "AvDebug.h"
#include "AvCommonMacro.h"
#include "AvCommonType.h"
#include "AvDeviceInfo.h"
#include "mpi_region.h"

/*clwang，新增默认参数extend_osd_id， 6AII_AV12机型osd叠加专用*/
int Har_get_region_handle_encoder(av_video_stream_type_t stream_type, int phy_video_num, av_osd_name_t osd_name, RGN_HANDLE *pHandle, rm_uint8_t extend_osd_id = 0);
int Har_get_region_handle_vi(int phy_video_num, int area_index, RGN_HANDLE *pHandle);



#endif/*__HIAVREGION_H__*/

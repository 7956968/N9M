/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __HIVO3515_H__
#define __HIVO3515_H__
#include "AvConfig.h"
#include "AvDebug.h"
#include "AvCommonMacro.h"
#include "AvCommonType.h"
#include "AvDeviceInfo.h"
#include "mpi_vo.h"
#include "mpi_sys.h"
#include "CommonDebug.h"

class CHiVo{
    public:
        CHiVo();
        ~CHiVo();

    public:
        int HiVo_memory_balance(VO_DEV vo_dev);
        
        VO_DEV HiVo_get_vo_dev(av_output_type_t output, int index, int para);
        VO_INTF_SYNC_E HiVo_get_vo_interface_sync(av_tvsystem_t tv_system);
        VO_INTF_SYNC_E HiVo_get_vo_interface_sync(av_vag_hdmi_resolution_t resolution);
        int HiVo_get_video_size(av_tvsystem_t tv_system, int *pVideo_layer_width, int *pVideo_layer_height, int *pVideo_output_width, int *pVideo_output_height);
        int HiVo_get_video_size(av_vag_hdmi_resolution_t resolution, int *pVideo_layer_width, int *pVideo_layer_height, int *pVideo_output_width, int *pVideo_output_height);

    public:
        /*video output channel*/
        int HiVo_start_vo_chn(VO_DEV vo_dev, VO_CHN vo_chn, const VO_CHN_ATTR_S *pVo_chn_attr);
        int HiVo_stop_vo_chn(VO_DEV vo_dev, VO_CHN vo_chn);
        /*video layer*/
        int HiVo_start_vo_video_layer(VO_DEV vo_dev, const VO_VIDEO_LAYER_ATTR_S *pVideo_layer_attr);
        int HiVo_stop_vo_video_layer(VO_DEV vo_dev);
        int HiVo_get_video_layer_attr(VO_DEV vo_dev, VO_VIDEO_LAYER_ATTR_S *pVideo_layer_attr);
        /*video output device*/
        int HiVo_start_vo_dev(VO_DEV vo_dev, const VO_PUB_ATTR_S *pVo_pub_attr);
        int HiVo_stop_vo_dev(VO_DEV vo_dev);
        /*wbc*/
        int HiVo_stop_wbc(VO_DEV source, VO_DEV destination);
        /*hide, show*/
        int HiVo_hide_chn(VO_DEV vo_dev, VO_CHN vo_chn);
        int HiVo_show_chn(VO_DEV vo_dev, VO_CHN vo_chn);
        /*set attr*/
        int HiVo_set_chn_attr(VO_DEV vo_dev, VO_CHN vo_chn, const VO_CHN_ATTR_S *pVo_chn_attr);

        int HiVo_get_chn_attr( VO_DEV vo_dev, VO_CHN vo_chn, VO_CHN_ATTR_S *pVo_chn_attr );

    	int HiVo_set_vo_framerate(VO_DEV vo_dev, int vo_chn, int framerate);

        int HiVo_get_vo_framerate(VO_DEV vo_dev, int vo_chn, int &framerate);

    	int HiVo_resume_vo(VO_DEV vo_dev, int vo_chn);

        int HiVo_clear_vo(VO_DEV vo_dev, int vo_chn);

    	int HiVo_pause_vo(VO_DEV vo_dev, int vo_chn);

        int HiVo_enable_pip(VO_DEV vo_dev, VO_VIDEO_LAYER_ATTR_S PipLayerAttr);

        int HiVo_disable_pip(VO_DEV vo_dev);

        int HiVo_set_dispos(VO_DEV vo_dev, int vo_chn, RECT_S pipPositon);

        int HiVo_refresh_vo(VO_DEV vo_dev, int vo_chn);

        int HiVo_set_attr_begin( VO_DEV vo_dev );

        int HiVo_set_attr_end( VO_DEV vo_dev );
#if defined(_AV_PLATFORM_HI3535_)||defined(_AV_PLATFORM_HI3520D_V300_)
		int HiVo_get_vo_layer(VO_DEV vo_dev, VO_LAYER &vo_layer);

		int HiVo_get_pip_layer();
		
		int HiVo_start_pip_chn(VO_CHN VoChn, const VO_CHN_ATTR_S *pChnAttr);

		int HiVo_stop_pip_chn(VO_CHN VoChn);
#endif
};


#endif/*__HIVO_H__*/

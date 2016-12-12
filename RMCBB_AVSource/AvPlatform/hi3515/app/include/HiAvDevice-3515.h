/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __HIAVDEVICE3515_H__
#define __HIAVDEVICE3515_H__
#include "AvDevice.h"
#include "HiSystem-3515.h"
#include "HiAvSysPicManager-3515.h"
#include "CommonMacro.h"
#if defined(_AV_HISILICON_VPSS_)
#include "HiVpss.h"
#endif
#if defined(_AV_HAVE_VIDEO_INPUT_)
#include "HiVi-3515.h"
#endif
#if defined(_AV_HAVE_VIDEO_DECODER_)
#include "HiAvDec-3515.h"
#endif
#if defined(_AV_HAVE_AUDIO_INPUT_)
#include "HiAi-3515.h"
#endif
#if defined(_AV_HAVE_VIDEO_ENCODER_)
#include "HiAvEncoder-3515.h"
#endif
#if defined(_AV_HAVE_ISP_)
#include "rm_isp_api.h"
#endif
#include <map>

#if defined(_AV_PRODUCT_CLASS_IPC_) && defined(_AV_PLATFORM_HI3516A_) && defined(_AV_SUPPORT_IVE_)
#include "HiIve.h"
#endif

class CHiAvDevice:public virtual CAvDevice{
    public:
        CHiAvDevice();
        virtual ~CHiAvDevice();

    public:
        int Ad_init();
        int Ad_wait_system_ready();
    protected:
        CAvSysPicManager *m_pSys_pic_manager;

/////////////////////////////////////////////////	
    /*video input*/
#if defined(_AV_HAVE_VIDEO_INPUT_)
    public:
        int Ad_start_vi(av_tvsystem_t tv_system, vi_config_set_t *pSource = NULL, vi_config_set_t *pPrimary_attr = NULL, vi_config_set_t *pMinor_attr = NULL);
        int Ad_start_selected_vi(av_tvsystem_t tv_system, unsigned long long int chn_mask, vi_config_set_t *pSource = NULL, vi_config_set_t *pPrimary_attr = NULL, vi_config_set_t *pMinor_attr = NULL);

        int Ad_stop_vi();
        int Ad_stop_selected_vi(unsigned long long int chn_mask);

        int Ad_vi_cover(int phy_chn_num, int x, int y, int width, int height, int cover_index);
        int Ad_vi_uncover(int phy_chn_num, int cover_index);
        int Ad_get_vi_max_size(int phy_chn_num, int *pMax_width, int *pMax_height, av_resolution_t *pResolution);
        int Ad_vi_insert_picture(unsigned int video_loss_bmp);

        int Ad_start_vda(vda_type_e vda_type, av_tvsystem_t tv_system, int phy_video_chn, const vda_chn_param_t* pVda_chn_param);
        int Ad_start_selected_vda(vda_type_e vda_type, av_tvsystem_t tv_system, int phy_video_chn, const vda_chn_param_t* pVda_chn_param);
        int Ad_stop_vda(vda_type_e vda_type, int phy_video_chn);
        int Ad_stop_selected_vda(vda_type_e vda_type, int phy_video_chn);
        int Ad_get_vda_result(vda_type_e vda_type, unsigned int *pVda_result, int phy_video_chn=-1, unsigned char *pVda_region=NULL);


        unsigned int Ad_get_vda_od_alarm_num(int phy_video_chn);
        int Ad_reset_vda_od_counter(int phy_video_chn);
#if defined(_AV_PRODUCT_CLASS_IPC_)
        int Ad_notify_framerate_changed(int phy_video_chn, int framerate);
#if defined(_AV_PLATFORM_HI3518C_)
        int Ad_set_vi_chn_capture_y_offset(int phy_video_chn, short offset);  
#endif
        bool Ad_get_vi_chn_interrut_cnt(int phy_video_chn, unsigned int& interrut_cnt);        
#endif


    protected:
        CHiVi *m_pHi_vi;
        CAvVda *m_pAv_vda;
#endif

/////////////////////////////////////////////////
    /*audio input*/
#if defined(_AV_HAVE_AUDIO_INPUT_)
    public:
        int Ad_start_ai(ai_type_e ai_type);
        int Ad_stop_ai(ai_type_e ai_type);

    protected:
        CHiAi *m_pHi_ai;
#endif

/////////////////////////////////////////////////
    /*vpss*/
#if defined(_AV_HISILICON_VPSS_)
    protected:
        CHiVpss *m_pHi_vpss;
#endif

    protected:
        CHiSystem *m_pHi_system;

#if defined(_AV_PRODUCT_CLASS_IPC_)
    public:
        int Ad_SetVideoColor( unsigned char u8Bright, unsigned char u8Hue, unsigned char u8Contrast, unsigned char u8Saturation );
        int Ad_SetCameraSharpen( unsigned char u8Sharpen );
        int Ad_SetCameraMirrorFlip( unsigned char u8IsMirror, unsigned char u8IsFlip );
        int Ad_SetCameraDayNightMode( unsigned char u8Mode );
        int Ad_SetCameraLampMode( unsigned char u8Mode );
        int Ad_SetCameraLightFrequency( unsigned char u8Value );
        int Ad_SetCameraBackLightLevel( unsigned char u8Level );

        int Ad_SetEnvironmentLuma( unsigned int u32Luma );
        int Ad_GetIspControlCmd( cmd2AControl_t* pstuCmd );
        int Ad_SetISPAEAttrStep(unsigned char u8StepModule);

        int Ad_SetCameraExposure( unsigned char u8Mode, unsigned char u8Value );
        int Ad_SetCameraGain( unsigned char u8Mode, unsigned char u8Value );
        int Ad_SetCameraOccasion( unsigned char u8Mode );

        int Ad_SetColorGrayMode( unsigned char u8ColorMode );

        int Ad_SetDayNightCutLuma( unsigned int u32Mask, unsigned int u32DayToNightValue, unsigned int u32NightToDayValue );

        int Ad_SetLdcParam( unsigned char  bEnable );

        int Ad_SetWdrParam(unsigned char u8WdrState, unsigned char u8Value );

        //!Added for IPC transplant, Nov 2014
        int Ad_SetHeightLightCompensation(unsigned char u8Value);  

        int Ad_SetIspFrameRate(unsigned char u8FrameRate);
        int Ad_SetApplicationScenario(unsigned char u8AppScenario);

#if defined(_AV_PLATFORM_HI3516A_)
        int Ad_GetWdrParam(WDRParam_t &stuWdrParam);
#if defined(_AV_SUPPORT_IVE_)
        int Ad_start_ive();
        int Ad_stop_ive();
#if defined(_AV_IVE_FD_)
        int Ad_start_ive_face_detection();
        int Ad_stop_ive_face_detection();
#endif

#if defined(_AV_IVE_BLE_) || defined(_AV_IVE_LD_)
        int Ad_register_ive_detection_result_notify_callback(boost::function< void (msgIPCIntelligentInfoNotice_t *)> notify_func);
        int Ad_unregister_ive_detection_result_notify_callback();
#endif

#endif
#endif
#endif

#ifdef _AV_PLATFORM_HI3535_
        int InitInnerAcodec(const char *AcodecFile, AUDIO_SAMPLE_RATE_E enSample, unsigned char u8Volum);
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_) && defined(_AV_PLATFORM_HI3516A_)
#if defined(_AV_SUPPORT_IVE_)   
    protected:
        CHiIve* m_pHiIve;
#endif
#endif

};

#endif/*__HIAVDEVICE_H__*/


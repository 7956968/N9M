/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __HIVPSS_H__
#define __HIVPSS_H__
#include "AvConfig.h"
#include "AvDebug.h"
#include "AvCommonMacro.h"
#include "AvCommonType.h"
#include "AvDeviceInfo.h"
#include "mpi_sys.h"
#include "mpi_vpss.h"
#include "AvThreadLock.h"
#if defined(_AV_HAVE_VIDEO_INPUT_)
#include "HiVi.h"
#endif
#if defined(_AV_HAVE_VIDEO_DECODER_)
class CHiAvDec;
#include "HiAvDec.h"
#endif

typedef struct _vpss_state_{
/*  
    bit0:ch0 of vpss
        ...
    bit4:ch4 of vpss

    each channel of vpss has only one purpose
*/
    unsigned long using_bitmap;
}vpss_state_t;


typedef enum _vpss_purpose_{
    _VP_PREVIEW_VI_,
    _VP_PREVIEW_VDEC_,
    _VP_SPOT_VI_,
    _VP_SPOT_VDEC_,/*preview*/
    _VP_MAIN_STREAM_,
    _VP_SUB_STREAM_,
    _VP_PREVIEW_VDEC_DIGITAL_ZOOM_,
    _VP_SPOT_VDEC_DIGITAL_ZOOM_,
    _VP_PLAYBACK_VDEC_DIGITAL_ZOOM_,
    _VP_VI_DIGITAL_ZOOM_,
    _VP_CUSTOM_LOGO_,
    _VP_ACCESS_LOGO_,
    _VP_PLAYBACK_VDEC_,
    _VP_SUB2_STREAM_,    
#if defined(_AV_SUPPORT_IVE_)
    _VP_SPEC_USE_
#endif
}vpss_purpose_t;



class CHiVpss{
    public:
       CHiVpss();
       ~CHiVpss();

    public:
        int HiVpss_set_customlogo_size(av_rect_t custom_size) { memcpy(&m_custom_size, &custom_size, sizeof(av_rect_t)); return 0;}
        int HiVpss_get_vpss(vpss_purpose_t purpose, int chn_num, VPSS_GRP *pVpss_grp, VPSS_CHN *pVpss_chn, void *pPurpose_para = NULL);
        bool Hivpss_whether_create_vpssgroup(VPSS_GRP vpss_grp);
        int HiVpss_release_vpss(vpss_purpose_t purpose, int chn_num, void *pPurpose_para = NULL);
        int HiVpss_find_vpss(vpss_purpose_t purpose, int chn_num, VPSS_GRP *pVpss_grp, VPSS_CHN *pVpss_chn, void *pPurpose_para = NULL);
#if defined(_AV_HAVE_VIDEO_INPUT_)
        int HiVpss_set_vi(CHiVi *pHi_vi){m_pHi_vi = pHi_vi; return 0;};
#endif
#if defined(_AV_HAVE_VIDEO_DECODER_)
        int HiVpss_set_vdec(CHiAvDec *pHi_av_dec){m_pHi_av_dec = pHi_av_dec; return 0;};
#endif
        int HiVpss_send_user_frame(VPSS_GRP vpss_grp, VIDEO_FRAME_INFO_S *pstVideoFrame);
        int HiVpss_ResetVpss( vpss_purpose_t purpose, int chn_num );
        int HiVpss_set_vpssgrp_cropcfg(bool enable, VPSS_GRP vpss_grp, av_rect_t *pRect, unsigned int maxw, unsigned int maxh);
        int HiVpss_get_frame_for_snap( int chn_num, VIDEO_FRAME_INFO_S *pstVideoFrame );

        int HiVpss_set_vpssgrp_param(av_video_image_para_t video_image);
        int HiVpss_get_vpssgrp_param(av_video_image_para_t &video_image);

#ifdef _AV_PRODUCT_CLASS_IPC_
#if defined(_AV_PLATFORM_HI3516A_)
        int HiVpss_3DNRFilter(vpss_purpose_t purpose, int chn_num, const vpss3DNRParam_t &stu3DNRParam);
#elif defined(_AV_PLATFORM_HI3518EV200_)
        int HiVpss_3DNRFilter(vpss_purpose_t purpose, int chn_num, const VpssGlb3DNRParamV1_t &stu3DNRParam);
#else
        int HiVpss_3DNoiseFilter( vpss_purpose_t purpose, int chn_num, unsigned int u32Sf, unsigned int u32Tf, unsigned int u32ChromaRg );
#endif
        //add by dhdong for IPC snap
        int HiVpss_get_frame(vpss_purpose_t purpose,int chn_num, VIDEO_FRAME_INFO_S *pstVideoFrame);
        int HiVpss_release_frame(vpss_purpose_t purpose,int chn_num, VIDEO_FRAME_INFO_S *pstVideoFrame);
#if defined(_AV_PLATFORM_HI3516A_) || defined(_AV_PLATFORM_HI3518EV200_)
        int HiVpss_set_ldc_attr( int phy_chn_num, const VPSS_LDC_ATTR_S* pstLDCAttr );
        int HiVpss_set_Rotate(int phy_chn_num, unsigned char rotate);  
        int HiVpss_set_mirror_flip(int phy_chn_num, bool is_mirror, bool is_flip);
#endif
#endif

    private:

#ifdef _AV_PRODUCT_CLASS_IPC_
        int HiVpss_allotter_IPC_REGULAR(vpss_purpose_t purpose, int chn_num, VPSS_GRP *pVpss_grp, VPSS_CHN *pVpss_chn, unsigned long *pGrp_chn_bitmap, void *pPurpose_para);
#else
        int HiVpss_allotter_DVR_REGULAR(vpss_purpose_t purpose, int chn_num, VPSS_GRP *pVpss_grp, VPSS_CHN *pVpss_chn, unsigned long *pGrp_chn_bitmap, void *pPurpose_para);
        int HiVpss_allotter_NVR_REGULAR(vpss_purpose_t purpose, int chn_num, VPSS_GRP *pVpss_grp, VPSS_CHN *pVpss_chn, unsigned long *pGrp_chn_bitmap, void *pPurpose_para);    
#endif
        int HiVpss_allotter(vpss_purpose_t purpose, int chn_num, VPSS_GRP *pVpss_grp, VPSS_CHN *pVpss_chn, unsigned long *pGrp_chn_bitmap, void *pPurpose_para);

    private:
        int HiVpss_memory_balance(vpss_purpose_t purpose, int chn_num, void *pPurpose_para, VPSS_GRP vpss_grp);
        int HiVpss_vpss_manager(vpss_purpose_t purpose, int chn_num, VPSS_GRP *pVpss_grp, VPSS_CHN *pVpss_chn, void *pPurpose_para, int flag);
        int HiVpss_start_vpss(vpss_purpose_t purpose, int chn_num, VPSS_GRP vpss_grp, unsigned long grp_chn_bitmap, void *pPurpose_para);
        int HiVpss_stop_vpss(vpss_purpose_t purpose, int chn_num, VPSS_GRP vpss_grp, unsigned long grp_chn_bitmap, void *pPurpose_para);
        int HiVpss_get_grp_param(VPSS_GRP vpss_grp, VPSS_GRP_PARAM_S &vpss_grp_para, vpss_purpose_t purpose);
        
    private:
        CAvThreadLock *m_pThread_lock;
        vpss_state_t *m_pVpss_state;
#if defined(_AV_HAVE_VIDEO_INPUT_)
        CHiVi *m_pHi_vi;
#endif
#if defined(_AV_HAVE_VIDEO_DECODER_)
        CHiAvDec *m_pHi_av_dec;
#endif
        av_video_image_para_t *m_pVideo_image;
        av_rect_t m_custom_size;

};


#endif/*__HIVPSS_H__*/

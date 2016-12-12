/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __AVCONFIG_H__
#define __AVCONFIG_H__

/*platform*/
#if defined(_AV_PLATFORM_HI3535_MK_)
    #define _AV_PLATFORM_HI3535_
#endif

#if defined(_AV_PLATFORM_HI3531_MK_)
    #define _AV_PLATFORM_HI3531_
#endif

#if defined(_AV_PLATFORM_HI3532_MK_)
    #define _AV_PLATFORM_HI3532_
#endif

#if defined(_AV_PLATFORM_HI3521_MK_)
    #define _AV_PLATFORM_HI3521_
#endif

#if defined(_AV_PLATFORM_HI3515_MK_)
    #define _AV_PLATFORM_HI3515_
#endif

#if defined(_AV_PLATFORM_HI3520A_MK_)
    #define _AV_PLATFORM_HI3520A_
#endif

#if defined(_AV_PLATFORM_HI3518C_MK_)
    #define _AV_PLATFORM_HI3518C_
#endif

#if defined(_AV_PLATFORM_HI3520D_MK_)
    #define _AV_PLATFORM_HI3520D_
#endif

#if defined(_AV_PLATFORM_HI3520D_V300_MK_)
    #define _AV_PLATFORM_HI3520D_V300_
#endif

#if defined(_AV_PLATFORM_HI3516C_MK_)
    #define _AV_PLATFORM_HI3518C_
#endif

#if defined(_AV_PLATFORM_HI3516A_MK_)
    #define _AV_PLATFORM_HI3516A_
#endif

#if defined(_AV_PLATFORM_HI3518EV200_MK_)
    #define _AV_PLATFORM_HI3518EV200_
#endif


/*product*/
#if defined(_AV_PRODUCT_CLASS_DVR_MK_)
    #define _AV_PRODUCT_CLASS_DVR_
#endif

#if defined(_AV_PRODUCT_CLASS_DVR_REI_MK_)
    #define _AV_PRODUCT_CLASS_DVR_REI_
#endif

#if defined(_AV_AUDIO_USE_AACENC_MK_)
    #define _AV_USE_AACENC_
#endif
#if defined(_AV_PRODUCT_CLASS_IPC_MK_)
    #define _AV_PRODUCT_CLASS_IPC_
#endif

#if defined(_AV_PRODUCT_CLASS_NVR_MK_)
    #define _AV_PRODUCT_CLASS_NVR_
#endif

#if defined(_AV_PRODUCT_CLASS_HDVR_MK_)
    #define _AV_PRODUCT_CLASS_HDVR_
#endif

#if defined(_AV_PRODUCT_CLASS_DECODER_MK_)
    #define _AV_PRODUCT_CLASS_DECODER_
#endif


/*There is no vpss on hi3515 platform*/
#if defined(_AV_HISILICON_VPSS_MK_)
    #define _AV_HISILICON_VPSS_
#endif

/*test mainboard*/
#if defined(_AV_TEST_MAINBOARD_MK_)
    #define _AV_TEST_MAINBOARD_
#endif

#if defined(_AV_PRODUCT_FUNCTION_IVE_MK_)
    #define _AV_SUPPORT_IVE_
#endif

#if defined(_AV_PRODUCT_FUNCTION_IVE_HC_MK_)
    #define _AV_IVE_HC_
#endif


#if defined(_AV_PRODUCT_FUNCTION_IVE_FD_MK_)
    #define _AV_IVE_FD_
#endif

#if defined(_AV_PRODUCT_FUNCTION_IVE_LD_MK_)
    #define _AV_IVE_LD_
#endif

#if defined(_AV_PRODUCT_FUNCTION_IVE_BLE_MK_)
    #define _AV_IVE_BLE_
#endif


/*module config*/
#if !(defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_NVR_))
    #define _AV_HAVE_VIDEO_ENCODER_
    #define _AV_HAVE_VIDEO_INPUT_
#endif

#if defined(_AV_REMOTE_TALK_MK_) // from make file, for NVR, IPC
    #define _AV_SUPPORT_REMOTE_TALK_
#endif

#if !defined(_AV_PRODUCT_CLASS_DECODER_)
    #define _AV_HAVE_AUDIO_INPUT_
#endif

//6AI_AV12不需要视频输出和解码
#if !(defined(_AV_PRODUCT_CLASS_IPC_) || defined(_AV_PRODUCT_FUNCTION_NO_VIDEO_OUTPUT_AND_DECODER))
    #define _AV_HAVE_VIDEO_OUTPUT_
    #define _AV_HAVE_VIDEO_DECODER_
#endif

#ifndef _AV_PRODUCT_CLASS_IPC_ //目前几款IPC，暂时无音频输出
#define _AV_HAVE_AUDIO_OUTPUT_
#endif

#if defined(_AV_PRODUCT_FUNCTION_ISP_MK_)
#define _AV_HAVE_ISP_
#endif

//!TTS function
#if defined(_AV_PRODUCT_FUNCTION_TTS_MK_)
#define _AV_HAVE_TTS_
#endif

#if defined(_AV_PRODUCT_FUNCTION_AHD_MK_)
#define _AV_SUPPORT_AHD_
#endif
//!Pure audio channel function,  current only use for plane_project
#if defined(_AV_PRODUCT_FUNCTION_PURE_AUDIO_MK_)
	#define _AV_HAVE_PURE_AUDIO_CHANNEL
#endif

#endif/*__AVCONFIG_H__*/

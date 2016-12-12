#include "HiHdmi.h"


CHiHdmi::CHiHdmi()
{

}


CHiHdmi::~CHiHdmi()
{

}


HI_HDMI_VIDEO_FMT_E CHiHdmi::HiHdmi_get_video_format(av_vag_hdmi_resolution_t resolution)
{
    switch(resolution)
    {
        case _AVHR_720p50_:return HI_HDMI_VIDEO_FMT_720P_50;
        case _AVHR_720p60_:return HI_HDMI_VIDEO_FMT_720P_60;
        case _AVHR_1080i50_:return HI_HDMI_VIDEO_FMT_1080i_50;
        case _AVHR_1080i60_:return HI_HDMI_VIDEO_FMT_1080i_60;
        case _AVHR_1080p50_:return HI_HDMI_VIDEO_FMT_1080P_50;
        case _AVHR_1080p60_:return HI_HDMI_VIDEO_FMT_1080P_60;
        case _AVHR_576p50_:return HI_HDMI_VIDEO_FMT_576P_50;
        case _AVHR_480p60_:return HI_HDMI_VIDEO_FMT_480P_60;
        case _AVHR_800_600_60_:return HI_HDMI_VIDEO_FMT_VESA_800X600_60;
        case _AVHR_1024_768_60_:return HI_HDMI_VIDEO_FMT_VESA_1024X768_60;
        case _AVHR_1280_1024_60_:return HI_HDMI_VIDEO_FMT_VESA_1280X1024_60;
        case _AVHR_1366_768_60_:return HI_HDMI_VIDEO_FMT_VESA_1366X768_60;
        case _AVHR_1440_900_60_:return HI_HDMI_VIDEO_FMT_VESA_1440X900_60;
        case _AVHR_1280_800_60_:return HI_HDMI_VIDEO_FMT_VESA_1280X800_60;
        default:_AV_FATAL_(1, "CHiHdmi::HiHdmi_get_video_format You must give the implement\n");break;
    }

    return HI_HDMI_VIDEO_FMT_BUTT;
}


int CHiHdmi::HiHdmi_start_hdmi(av_vag_hdmi_resolution_t resolution)
{
    HI_HDMI_INIT_PARA_S hdmi_init_para;
    HI_HDMI_ATTR_S      hdmi_attr;
    HI_S32 ret_val = -1;

    hdmi_init_para.enForceMode = HI_HDMI_FORCE_HDMI;
    hdmi_init_para.pCallBackArgs = NULL;
    hdmi_init_para.pfnHdmiEventCallback = NULL;
    if((ret_val = HI_MPI_HDMI_Init(&hdmi_init_para)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiHdmi::HiHdmi_start_hdmi HI_MPI_HDMI_Init FAILED(0x%08lx)\n", (unsigned long)ret_val);
        return -1;
    }

    if((ret_val = HI_MPI_HDMI_Open(HI_HDMI_ID_0)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiHdmi::HiHdmi_start_hdmi HI_MPI_HDMI_Open FAILED(0x%08lx)\n", (unsigned long)ret_val);
        return -1;
    }

    if((ret_val = HI_MPI_HDMI_GetAttr(HI_HDMI_ID_0, &hdmi_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiHdmi::HiHdmi_start_hdmi HI_MPI_HDMI_GetAttr FAILED(0x%08lx)\n", (unsigned long)ret_val);
        return -1;
    }

    hdmi_attr.bEnableHdmi = HI_TRUE;
    hdmi_attr.bEnableVideo = HI_TRUE;
    hdmi_attr.enVideoFmt = HiHdmi_get_video_format(resolution);
    hdmi_attr.enVidOutMode = HI_HDMI_VIDEO_MODE_RGB444;
    hdmi_attr.enDeepColorMode = HI_HDMI_DEEP_COLOR_OFF;
    hdmi_attr.bxvYCCMode = HI_FALSE;
    hdmi_attr.bIsMultiChannel = HI_FALSE;
    hdmi_attr.enBitDepth = HI_HDMI_BIT_DEPTH_16;
    hdmi_attr.bEnableSpdInfoFrame = HI_FALSE;
    hdmi_attr.bEnableMpegInfoFrame = HI_FALSE;
    hdmi_attr.bDebugFlag = HI_FALSE;
    hdmi_attr.bHDCPEnable = HI_FALSE;
    hdmi_attr.b3DEnable = HI_FALSE;
    hdmi_attr.bEnableAudio = HI_TRUE;
    hdmi_attr.enSoundIntf = HI_HDMI_SND_INTERFACE_I2S;
    hdmi_attr.enSampleRate = HI_HDMI_SAMPLE_RATE_32K;
    hdmi_attr.u8DownSampleParm = HI_FALSE;
    hdmi_attr.enBitDepth = HI_HDMI_BIT_DEPTH_16;
    hdmi_attr.u8I2SCtlVbit = 0;
    hdmi_attr.bEnableAviInfoFrame = HI_TRUE;
    hdmi_attr.bEnableAudInfoFrame = HI_TRUE;

#if 0//defined(_AV_PLATFORM_HI3521_) || defined(_AV_PLATFORM_HI3520A_) || defined(_AV_PLATFORM_HI3520D_)
    hdmi_attr.enVidOutMode = HI_HDMI_VIDEO_MODE_YCBCR444;
#else
    hdmi_attr.enVidOutMode = HI_HDMI_VIDEO_MODE_RGB444;
#endif

    if((ret_val = HI_MPI_HDMI_SetAttr(HI_HDMI_ID_0, &hdmi_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiHdmi::HiHdmi_start_hdmi HI_MPI_HDMI_SetAttr FAILED(0x%08lx)\n", (unsigned long)ret_val);
        return -1;
    }

    if((ret_val = HI_MPI_HDMI_Start(HI_HDMI_ID_0)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiHdmi::HiHdmi_start_hdmi HI_MPI_HDMI_Start FAILED(0x%08lx)\n", (unsigned long)ret_val);
        return -1;
    }
#if 1
    HI_HDMI_SINK_CAPABILITY_S Capability;
    if((ret_val = HI_MPI_HDMI_GetSinkCapability(HI_HDMI_ID_0, &Capability)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiHdmi::HiHdmi_start_hdmi HI_HDMI_SINK_CAPABILITY_S FAILED(0x%08lx)\n", (unsigned long)ret_val);
    }
    else
    {
        DEBUG_MESSAGE( "********************HDMI INFO*********************");
        DEBUG_MESSAGE( "bConnected:%d\n", Capability.bConnected);
        DEBUG_MESSAGE( "bSupportHdmi:%d\n", Capability.bSupportHdmi);
        DEBUG_MESSAGE( "bIsSinkPowerOn:%d\n", Capability.bIsSinkPowerOn);
        DEBUG_MESSAGE( "bIsRealEDID:%d\n", Capability.bIsRealEDID);
        DEBUG_MESSAGE( "enNativeVideoFormat:%d\n", Capability.enNativeVideoFormat);
        for(int index = 0; index < HI_HDMI_VIDEO_FMT_BUTT; index++)
        {
            DEBUG_MESSAGE( "bVideoFmtSupported[%d]:%d\n", index, Capability.bVideoFmtSupported[index]);
        }
        DEBUG_MESSAGE( "bSupportYCbCr:%d\n", Capability.bSupportYCbCr);
        DEBUG_MESSAGE( "********************HDMI INFO*********************");
    }
#endif    
    return 0;
}


int CHiHdmi::HiHdmi_stop_hdmi()
{
    HI_S32 retval = -1;

    retval = HI_MPI_HDMI_Stop(HI_HDMI_ID_0);
    if(retval != HI_SUCCESS)
    {
        DEBUG_ERROR("[%s,%d]:stop HDMI fialed(0x%08x)!\n", __FUNCTION__, __LINE__, retval);
    }
    retval = HI_MPI_HDMI_Close(HI_HDMI_ID_0);
    if(retval != HI_SUCCESS)
    {
        DEBUG_ERROR("[%s,%d]:close HDMI fialed(0x%08x)!\n", __FUNCTION__, __LINE__, retval);
    }
    retval = HI_MPI_HDMI_DeInit();
    if(retval != HI_SUCCESS)
    {
        DEBUG_ERROR("[%s,%d]:DeInit fialed(0x%08x)!\n", __FUNCTION__, __LINE__, retval);
    }

    return retval;
}

int CHiHdmi::HiHdmi_set_hdmi_mute(bool mute)
{
    HI_S32 retval = -1;
    HI_BOOL AvMute = HI_FALSE;

    if(mute)
    {
        AvMute = HI_TRUE;
    }
    DEBUG_MESSAGE("hdmi mute:%s\n", mute ? "true" : "false");
    retval = HI_MPI_HDMI_SetAVMute(HI_HDMI_ID_0, AvMute);
    if(retval != HI_SUCCESS)
    {
        DEBUG_MESSAGE("[%s,%d]:set mute fialed(0x%08x)!\n", __FUNCTION__, __LINE__, retval);
    }
    return retval;
}


#include "HiVo-3515.h"

CHiVo::CHiVo()
{

}


CHiVo::~CHiVo()
{

}


int CHiVo::HiVo_memory_balance(VO_DEV vo_dev)
{
#ifdef _AV_PLATFORM_HI3531_
    const char *pMemory_name = NULL;
    MPP_CHN_S mpp_chn;
    int ret_val = -1;
    
    pMemory_name = "ddr1";
    mpp_chn.enModId  = HI_ID_VOU;
    mpp_chn.s32DevId = vo_dev;
    mpp_chn.s32ChnId = 0;
    //同一设备号只能分同一个名称的内存
    if((ret_val = HI_MPI_SYS_SetMemConf(&mpp_chn, pMemory_name)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_memory_balance HI_MPI_SYS_SetMemConf FAILED(vo_dev:%d)(0x%08lx)\n", vo_dev, (unsigned long)ret_val);
        return -1;
    }
#endif
    return 0;
}


int CHiVo::HiVo_get_video_size(av_tvsystem_t tv_system, int *pVideo_layer_width, int *pVideo_layer_height, int *pVideo_output_width, int *pVideo_output_height)
{
    _AV_POINTER_ASSIGNMENT_(pVideo_layer_width,  720);
    _AV_POINTER_ASSIGNMENT_(pVideo_output_width, 720);

    if(tv_system == _AT_PAL_)
    {
        _AV_POINTER_ASSIGNMENT_(pVideo_layer_height, 576);
        _AV_POINTER_ASSIGNMENT_(pVideo_output_height, 576);
    }
    else
    {
        _AV_POINTER_ASSIGNMENT_(pVideo_layer_height, 480);
        _AV_POINTER_ASSIGNMENT_(pVideo_output_height, 480);
    }

    return 0;
}


int CHiVo::HiVo_get_video_size(av_vag_hdmi_resolution_t resolution, int *pVideo_layer_width, int *pVideo_layer_height, int *pVideo_output_width, int *pVideo_output_height)
{
    switch(resolution)
    {
        case _AVHR_1080p24_:
        case _AVHR_1080p25_:
        case _AVHR_1080p30_:
        case _AVHR_1080i50_:
        case _AVHR_1080i60_:
        case _AVHR_1080p50_:
        case _AVHR_1080p60_:
            _AV_POINTER_ASSIGNMENT_(pVideo_layer_width, 1920);
            _AV_POINTER_ASSIGNMENT_(pVideo_output_width, 1920);
            _AV_POINTER_ASSIGNMENT_(pVideo_layer_height, 1080);
            _AV_POINTER_ASSIGNMENT_(pVideo_output_height, 1080);
            break;
        case _AVHR_720p50_:
        case _AVHR_720p60_:
            _AV_POINTER_ASSIGNMENT_(pVideo_layer_width, 1280);
            _AV_POINTER_ASSIGNMENT_(pVideo_output_width, 1280);
            _AV_POINTER_ASSIGNMENT_(pVideo_layer_height, 720);
            _AV_POINTER_ASSIGNMENT_(pVideo_output_height, 720);
            break;
        case _AVHR_576p50_:
            _AV_POINTER_ASSIGNMENT_(pVideo_layer_width, 720);
            _AV_POINTER_ASSIGNMENT_(pVideo_output_width, 720);
            _AV_POINTER_ASSIGNMENT_(pVideo_layer_height, 576);
            _AV_POINTER_ASSIGNMENT_(pVideo_output_height, 576);
            break;
        case _AVHR_480p60_:
            _AV_POINTER_ASSIGNMENT_(pVideo_layer_width, 720);
            _AV_POINTER_ASSIGNMENT_(pVideo_output_width, 720);
            _AV_POINTER_ASSIGNMENT_(pVideo_layer_height, 480);
            _AV_POINTER_ASSIGNMENT_(pVideo_output_height, 480);
            break;
        case _AVHR_800_600_60_:
            _AV_POINTER_ASSIGNMENT_(pVideo_layer_width, 800);
            _AV_POINTER_ASSIGNMENT_(pVideo_output_width, 800);
            _AV_POINTER_ASSIGNMENT_(pVideo_layer_height, 600);
            _AV_POINTER_ASSIGNMENT_(pVideo_output_height, 600);
            break;
        case _AVHR_1024_768_60_:
            _AV_POINTER_ASSIGNMENT_(pVideo_layer_width, 1024);
            _AV_POINTER_ASSIGNMENT_(pVideo_output_width, 1024);
            _AV_POINTER_ASSIGNMENT_(pVideo_layer_height, 768);
            _AV_POINTER_ASSIGNMENT_(pVideo_output_height, 768);
            break;
        case _AVHR_1280_1024_60_:
            _AV_POINTER_ASSIGNMENT_(pVideo_layer_width, 1280);
            _AV_POINTER_ASSIGNMENT_(pVideo_output_width, 1280);
            _AV_POINTER_ASSIGNMENT_(pVideo_layer_height, 1024);
            _AV_POINTER_ASSIGNMENT_(pVideo_output_height, 1024);
            break;
        case _AVHR_1366_768_60_:
            _AV_POINTER_ASSIGNMENT_(pVideo_layer_width, 1366);
            _AV_POINTER_ASSIGNMENT_(pVideo_output_width, 1366);
            _AV_POINTER_ASSIGNMENT_(pVideo_layer_height, 768);
            _AV_POINTER_ASSIGNMENT_(pVideo_output_height, 768);
            break;
        case _AVHR_1440_900_60_:
            _AV_POINTER_ASSIGNMENT_(pVideo_layer_width, 1440);
            _AV_POINTER_ASSIGNMENT_(pVideo_output_width, 1440);
            _AV_POINTER_ASSIGNMENT_(pVideo_layer_height, 900);
            _AV_POINTER_ASSIGNMENT_(pVideo_output_height, 900);
            break;
        case _AVHR_1280_800_60_:
            _AV_POINTER_ASSIGNMENT_(pVideo_layer_width, 1280);
            _AV_POINTER_ASSIGNMENT_(pVideo_output_width, 1280);
            _AV_POINTER_ASSIGNMENT_(pVideo_layer_height, 800);
            _AV_POINTER_ASSIGNMENT_(pVideo_output_height, 800);
            break;
        default:
            _AV_FATAL_(1, "CHiAvDevice::Ad_get_video_layer_size You must give the implement\n");
            break;
    }

    return 0;
}

VO_INTF_SYNC_E CHiVo::HiVo_get_vo_interface_sync(av_tvsystem_t tv_system)
{
    if(tv_system == _AT_PAL_)
    {
        return VO_OUTPUT_PAL;
    }

    return VO_OUTPUT_NTSC;
}

VO_INTF_SYNC_E CHiVo::HiVo_get_vo_interface_sync(av_vag_hdmi_resolution_t resolution)
{
    switch(resolution)
    {
        case _AVHR_1080p25_:
            return VO_OUTPUT_1080P25;
        case _AVHR_1080p30_:
            return VO_OUTPUT_1080P30;
        case _AVHR_720p60_:
            return VO_OUTPUT_720P60;
        case _AVHR_1080i50_:
            return VO_OUTPUT_1080I50;
        case _AVHR_1080i60_:
            return VO_OUTPUT_1080I60;
        case _AVHR_800_600_60_:
            return VO_OUTPUT_800x600_60;
        case _AVHR_1024_768_60_:
            return VO_OUTPUT_1024x768_60;
        case _AVHR_1280_1024_60_:
            return VO_OUTPUT_1280x1024_60;
        case _AVHR_1366_768_60_:
            return VO_OUTPUT_1366x768_60;
        case _AVHR_1440_900_60_:
            return VO_OUTPUT_1440x900_60;
        default:
            _AV_FATAL_(1, "CHiVo::HiVo_get_vo_interface_sync You must give the implement\n");
            break;
    }

    return VO_OUTPUT_BUTT;
}

VO_DEV CHiVo::HiVo_get_vo_dev(av_output_type_t output, int index, int para)
{
#if defined(_AV_PLATFORM_HI3520D_V300_)
	switch(output)
    {
        case _AOT_MAIN_:/*vga&hdmi; cvbs(write-back)*/
            if(para == 0)
            {
                return 0;/*vag&hdmi*/
            }
            else
            {
                return 1;/*cvbs*/
            }
//            break;
		case _AOT_CVBS_:
            return 1;
        case _AOT_SPOT_:
            return 2;
//            break;
        default:
            _AV_FATAL_(1, "CHiVo::HiVo_get_vo_dev You must give the implement\n");
            break;
    }
#elif defined(_AV_PLATFORM_HI3520A_) || defined(_AV_PLATFORM_HI3521_) || defined(_AV_PLATFORM_HI3520D_)||defined(_AV_PLATFORM_HI3515_)
    switch(output)
    {
        case _AOT_MAIN_:/*vga&hdmi; cvbs(write-back)*/
            if(para == 0)
            {
                return 0;/*vag&hdmi*/
            }
            else
            {
                return 2;/*cvbs*/
            }
//            break;
		case _AOT_CVBS_:
            return 2;
        case _AOT_SPOT_:
            return 1;
//            break;
        default:
            _AV_FATAL_(1, "CHiVo::HiVo_get_vo_dev You must give the implement\n");
            break;
    }
#elif defined(_AV_PLATFORM_HI3515_)	
	switch(output)
    {
        case _AOT_MAIN_:/*vga&hdmi; cvbs(write-back)*/
            if(para == 0)
            {
                return 0;/*vag&hdmi*/
            }
            else
            {
                return 2;/*cvbs*/
            }
//            break;
		case _AOT_CVBS_:
            return 2;
        case _AOT_SPOT_:
            return 1;
//            break;
        default:
            _AV_FATAL_(1, "CHiVo::HiVo_get_vo_dev You must give the implement\n");
            break;
    }

#elif defined(_AV_PLATFORM_HI3535_)
    switch(output)
    {
        case _AOT_MAIN_:/*vga&hdmi; cvbs(write-back)*/
            if(para == 0)
            {
                return 0;/*vag&hdmi*/
            }
            else
            {
                return 2;/*cvbs*/
            }
            break;
		case _AOT_CVBS_:
            return 2;	
        case _AOT_SPOT_:
            return 3;
            break;
        default:
            _AV_FATAL_(1, "CHiVo::HiVo_get_vo_dev You must give the implement\n");
            break;
    }
#elif defined(_AV_PLATFORM_HI3532_) || defined(_AV_PLATFORM_HI3531_)
    switch(output)
    {
        case _AOT_MAIN_:/*vga&hdmi; cvbs(write-back)*/
            if(para == 0)
            {
                return 1;/*vag&hdmi*/
            }
            else
            {
                return 2;/*cvbs*/
            }
            break;
		case _AOT_CVBS_:
			return 2;/*cvbs*/
        case _AOT_SPOT_:
            return 3;
            break;
        default:
            _AV_FATAL_(1, "CHiVo::HiVo_get_vo_dev You must give the implement\n");
            break;
    }
#else
    _AV_FATAL_(1, "CHiVo::HiVo_get_vo_dev You must give the implement\n");
#endif

    return -1;
}


int CHiVo::HiVo_start_vo_dev(VO_DEV vo_dev, const VO_PUB_ATTR_S *pVo_pub_attr)
{
    HI_S32 ret_val = -1;

    HiVo_memory_balance(vo_dev);
    
    if((ret_val = HI_MPI_VO_SetPubAttr(vo_dev, pVo_pub_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_start_vo_dev HI_MPI_VO_SetPubAttr FAILED(%d)(0x%08lx)\n", vo_dev, (unsigned long)ret_val);
        return -1;
    }

    if((ret_val = HI_MPI_VO_Enable(vo_dev)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_start_vo_dev HI_MPI_VO_Enable FAILED(%d)(0x%08lx)\n", vo_dev, (unsigned long)ret_val);
        return -1;
    }
    
    DEBUG_MESSAGE("*********HiVo_start_vo_dev:%d********\n", vo_dev);
 #if 0   
    if((vo_dev == 0) || (vo_dev == 1) || (vo_dev == 2))
    {
        VO_CSC_S DevCsc;

        DEBUG_MESSAGE("*********set vo csc********\n");
        if(HI_MPI_VO_GetDevCSC(vo_dev, &DevCsc) != HI_SUCCESS)
        {
            DEBUG_MESSAGE("get vo csc failed!\n");
        }
        else
        {
        
            DEBUG_MESSAGE("csc:%d, u32Luma:%d, u32Contrast:%d, u32Hue:%d, u32Satuature:%d\n", DevCsc.enCscMatrix, DevCsc.u32Luma, DevCsc.u32Contrast, DevCsc.u32Hue, DevCsc.u32Satuature);
            DevCsc.u32Luma = 50;
            DevCsc.u32Contrast = 50;
            DevCsc.u32Hue = 50;
            DevCsc.u32Satuature = 50;
            if(HI_MPI_VO_SetDevCSC(vo_dev, &DevCsc) != HI_SUCCESS)
            {
                DEBUG_ERROR("set vo csc failed!\n");
            }
			
        }
		
    }
#endif	
    return 0;
}


int CHiVo::HiVo_stop_vo_dev(VO_DEV vo_dev)
{
    HI_S32 ret_val = -1;

    if((ret_val = HI_MPI_VO_Disable(vo_dev)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_stop_vo_dev HI_MPI_VO_Disable FAILED(%d)(0x%08lx)\n", vo_dev, (unsigned long)ret_val);
        return -1;
    }

    return 0;
}


int CHiVo::HiVo_start_vo_video_layer(VO_DEV vo_dev, const VO_VIDEO_LAYER_ATTR_S *pVideo_layer_attr)
{
    HI_S32 ret_val = -1;
#if defined (_AV_PLATFORM_HI3535_) ||defined (_AV_PLATFORM_HI3520D_V300_)
    VO_LAYER vo_layer;

    if(HiVo_get_vo_layer(vo_dev, vo_layer) != 0)
    {
        DEBUG_ERROR( "get vo layer failed(%d)!\n", vo_dev);
        return -1;
    }
    if((ret_val = HI_MPI_VO_SetVideoLayerAttr(vo_layer, pVideo_layer_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_start_vo_video_layer HI_MPI_VO_EnableVideoLayer FAILED(%d)(0x%08lx)\n", vo_dev, (unsigned long)ret_val);
        return -1;
    }

    if((ret_val = HI_MPI_VO_EnableVideoLayer(vo_layer)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_start_vo_video_layer HI_MPI_VO_EnableVideoLayer FAILED(%d)(0x%08lx)\n", vo_dev, (unsigned long)ret_val);
        return -1;
    }
#else
    DEBUG_CRITICAL( "vo_dev(%d),pVideo_layer_attr(%d,%d,%d,%d,%d,%d,%d,%d,%d)\n", vo_dev,
    			pVideo_layer_attr->stDispRect.s32X,pVideo_layer_attr->stDispRect.s32Y,pVideo_layer_attr->stDispRect.u32Width,
			pVideo_layer_attr->stDispRect.u32Height,pVideo_layer_attr->stImageSize.u32Width,pVideo_layer_attr->stImageSize.u32Height,pVideo_layer_attr->u32DispFrmRt,
			pVideo_layer_attr->enPixFormat,pVideo_layer_attr->s32PiPChn);
    if((ret_val = HI_MPI_VO_SetVideoLayerAttr(vo_dev, pVideo_layer_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_start_vo_video_layer HI_MPI_VO_EnableVideoLayer FAILED(%d)(0x%08lx)\n", vo_dev, (unsigned long)ret_val);
        return -1;
    }

    if((ret_val = HI_MPI_VO_EnableVideoLayer(vo_dev)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_start_vo_video_layer HI_MPI_VO_EnableVideoLayer FAILED(%d)(0x%08lx)\n", vo_dev, (unsigned long)ret_val);
        return -1;
    }
#endif

    return 0;
}


int CHiVo::HiVo_get_video_layer_attr(VO_DEV vo_dev, VO_VIDEO_LAYER_ATTR_S *pVideo_layer_attr)
{
    HI_S32 ret_val = -1;

#if defined(_AV_PLATFORM_HI3535_) ||defined(_AV_PLATFORM_HI3520D_V300_)

    VO_LAYER vo_layer;

    if(HiVo_get_vo_layer(vo_dev, vo_layer) != 0)
    {
        DEBUG_ERROR( "get vo layer failed(%d)!\n", vo_dev);
        return -1;
    }
    if((ret_val = HI_MPI_VO_GetVideoLayerAttr(vo_layer, pVideo_layer_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_get_video_layer_attr HI_MPI_VO_GetVideoLayerAttr FAILED(%d)(0x%08lx)\n", vo_dev, (unsigned long)ret_val);
        return -1;
    }
#else
    if((ret_val = HI_MPI_VO_GetVideoLayerAttr(vo_dev, pVideo_layer_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_get_video_layer_attr HI_MPI_VO_GetVideoLayerAttr FAILED(%d)(0x%08lx)\n", vo_dev, (unsigned long)ret_val);
        return -1;
    }
#endif

    return 0;
}

int CHiVo::HiVo_stop_vo_video_layer(VO_DEV vo_dev)
{
    HI_S32 ret_val = -1;

#if defined(_AV_PLATFORM_HI3535_) ||defined(_AV_PLATFORM_HI3520D_V300_)

    VO_LAYER vo_layer;

    if(HiVo_get_vo_layer(vo_dev, vo_layer) != 0)
    {
        DEBUG_ERROR( "get vo layer failed(%d)!\n", vo_dev);
        return -1;
    }
    if((ret_val = HI_MPI_VO_DisableVideoLayer(vo_layer)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_stop_vo_video_layer HI_MPI_VO_DisableVideoLayer FAILED(%d)(0x%08lx)\n", vo_dev, (unsigned long)ret_val);
        return -1;
    }
#else
    if((ret_val = HI_MPI_VO_DisableVideoLayer(vo_dev)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_stop_vo_video_layer HI_MPI_VO_DisableVideoLayer FAILED(%d)(0x%08lx)\n", vo_dev, (unsigned long)ret_val);
        return -1;
    }
#endif
    return 0;
}


int CHiVo::HiVo_start_vo_chn(VO_DEV vo_dev, VO_CHN vo_chn, const VO_CHN_ATTR_S *pVo_chn_attr)
{
    HI_S32 ret_val = -1;

#if defined(_AV_PLATFORM_HI3535_) ||defined(_AV_PLATFORM_HI3520D_V300_)
    VO_LAYER vo_layer;

    if(HiVo_get_vo_layer(vo_dev, vo_layer) != 0)
    {
        DEBUG_ERROR( "get vo layer failed(%d)!\n", vo_dev);
        return -1;
    }
    if((ret_val = HI_MPI_VO_SetChnAttr(vo_layer, vo_chn, pVo_chn_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_start_vo_chn HI_MPI_VO_SetChnAttr FAILED(%d, %d)(0x%08lx)\n", vo_dev, vo_chn, (unsigned long)ret_val);
        return -1;
    }
    if((ret_val = HI_MPI_VO_EnableChn(vo_layer, vo_chn)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_start_vo_chn HI_MPI_VO_EnableChn FAILED(%d, %d)(0x%08lx)\n", vo_dev, vo_chn, (unsigned long)ret_val);
        return -1;
    }
#else
#if defined(_AV_PLATFORM_HI3531_) || defined(_AV_PLATFORM_HI3532_)
    const char *pMemory_name = NULL;
    MPP_CHN_S mpp_chn;

    pMemory_name = "ddr1";
    mpp_chn.enModId  = HI_ID_VOU;
    mpp_chn.s32DevId = vo_dev;
    mpp_chn.s32ChnId = 0;

    if((ret_val = HI_MPI_SYS_SetMemConf(&mpp_chn, pMemory_name)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_start_vo_chn HI_MPI_SYS_SetMemConf FAILED(0x%08x)\n", ret_val);
    }
#endif

    if((ret_val = HI_MPI_VO_SetChnAttr(vo_dev, vo_chn, pVo_chn_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_start_vo_chn HI_MPI_VO_SetChnAttr FAILED(%d, %d)(0x%08lx)\n", vo_dev, vo_chn, (unsigned long)ret_val);
        return -1;
    }

    if((ret_val = HI_MPI_VO_EnableChn(vo_dev, vo_chn)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_start_vo_chn HI_MPI_VO_EnableChn FAILED(%d, %d)(0x%08lx)\n", vo_dev, vo_chn, (unsigned long)ret_val);
        return -1;
    }
#endif

    return 0;
}

int CHiVo::HiVo_set_chn_attr(VO_DEV vo_dev, VO_CHN vo_chn, const VO_CHN_ATTR_S *pVo_chn_attr)
{
    HI_S32 ret_val = -1;
#if defined(_AV_PLATFORM_HI3535_)||defined(_AV_PLATFORM_HI3520D_V300_)
    VO_LAYER vo_layer;

    if(HiVo_get_vo_layer(vo_dev, vo_layer) != 0)
    {
        DEBUG_ERROR( "get vo layer failed(%d)!\n", vo_dev);
        return -1;
    }
    if((ret_val = HI_MPI_VO_SetChnAttr(vo_layer, vo_chn, pVo_chn_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_set_chn_attr HI_MPI_VO_SetChnAttr FAILED(%d)(0x%08lx)\n", vo_dev, (unsigned long)ret_val);
        return -1;
    }
#else
    if((ret_val = HI_MPI_VO_SetChnAttr(vo_dev, vo_chn, pVo_chn_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_set_chn_attr HI_MPI_VO_SetChnAttr FAILED(%d)(0x%08lx)\n", vo_dev, (unsigned long)ret_val);
        return -1;
    }
#endif
    return 0;
}

int CHiVo::HiVo_get_chn_attr( VO_DEV vo_dev, VO_CHN vo_chn, VO_CHN_ATTR_S *pVo_chn_attr )
{
    HI_S32 ret_val = -1;
#if defined(_AV_PLATFORM_HI3535_)||defined(_AV_PLATFORM_HI3520D_V300_)
    VO_LAYER vo_layer;

    if(HiVo_get_vo_layer(vo_dev, vo_layer) != 0)
    {
        DEBUG_ERROR( "get vo layer failed(%d)!\n", vo_dev);
        return -1;
    }
    if((ret_val = HI_MPI_VO_GetChnAttr(vo_layer, vo_chn, pVo_chn_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_set_chn_attr HI_MPI_VO_SetChnAttr FAILED(%d)(0x%08lx)\n", vo_dev, (unsigned long)ret_val);
        return -1;
    }
#else
    if((ret_val = HI_MPI_VO_GetChnAttr(vo_dev, vo_chn, pVo_chn_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_set_chn_attr HI_MPI_VO_SetChnAttr FAILED(%d)(0x%08lx)\n", vo_dev, (unsigned long)ret_val);
        return -1;
    }
#endif
    return 0;
}

int CHiVo::HiVo_stop_vo_chn(VO_DEV vo_dev, VO_CHN vo_chn)
{
    HI_S32 ret_val = -1;
#if defined(_AV_PLATFORM_HI3535_)||defined(_AV_PLATFORM_HI3520D_V300_)
    VO_LAYER vo_layer;

    if(HiVo_get_vo_layer(vo_dev, vo_layer) != 0)
    {
        DEBUG_ERROR( "get vo layer failed(%d)!\n", vo_dev);
        return -1;
    }
    if((ret_val = HI_MPI_VO_DisableChn(vo_layer, vo_chn)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_stop_vo_chn HI_MPI_VO_DisableChn FAILED(%d)(0x%08lx)\n", vo_dev, (unsigned long)ret_val);
        return -1;
    }
#else
    if((ret_val = HI_MPI_VO_DisableChn(vo_dev, vo_chn)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_stop_vo_chn HI_MPI_VO_DisableChn FAILED(%d)(0x%08lx)\n", vo_dev, (unsigned long)ret_val);
        return -1;
    }
#endif
    return 0;
}

#if defined(_AV_PLATFORM_HI3535_)||defined(_AV_PLATFORM_HI3520D_V300_)
int CHiVo::HiVo_bind_wbc(VO_WBC VoWbc, VO_WBC_SOURCE_S *pstWbcSource)
{
    HI_S32 s32Ret = HI_SUCCESS;

    s32Ret = HI_MPI_VO_SetWbcSource(VoWbc, pstWbcSource);
    if (s32Ret != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_bind_wbc HiVo_bind_wbc FAILED(0x%08lx)!\n", (unsigned long)s32Ret);
        return -1;
    }
    return 0;
}
#endif



int CHiVo::HiVo_hide_chn(VO_DEV vo_dev, VO_CHN vo_chn)
{
    HI_S32 ret_val = -1;

    if ((vo_chn < 0) || (vo_dev < 0))
    {
        return 0;
    }
#if defined(_AV_PLATFORM_HI3535_)||defined(_AV_PLATFORM_HI3520D_V300_)
    VO_LAYER vo_layer;
    if(HiVo_get_vo_layer(vo_dev, vo_layer) != 0)
    {
        DEBUG_ERROR( "get vo layer failed(%d)!\n", vo_dev);
        return -1;
    }
    if((ret_val = HI_MPI_VO_HideChn(vo_layer, vo_chn)) != HI_SUCCESS)
#else
    if((ret_val = HI_MPI_VO_ChnHide(vo_dev, vo_chn)) != HI_SUCCESS)
#endif
    {
        DEBUG_ERROR( "CHiVo::HiVo_stop_vo_chn HI_MPI_VO_ChnHide FAILED(%d, %d)(0x%08lx)\n", vo_dev, vo_chn, (unsigned long)ret_val);
        return -1;
    }
    return 0;
}

int CHiVo::HiVo_show_chn(VO_DEV vo_dev, VO_CHN vo_chn)
{
    HI_S32 ret_val = -1;

    if ((vo_chn < 0) || (vo_dev < 0))
    {
        return 0;
    }
#if defined(_AV_PLATFORM_HI3535_)||defined(_AV_PLATFORM_HI3520D_V300_)
    VO_LAYER vo_layer;
    if(HiVo_get_vo_layer(vo_dev, vo_layer) != 0)
    {
        DEBUG_ERROR( "get vo layer failed(%d)!\n", vo_dev);
        return -1;
    }
    if((ret_val = HI_MPI_VO_ShowChn(vo_layer, vo_chn)) != HI_SUCCESS)
#else
    if((ret_val = HI_MPI_VO_ChnShow(vo_dev, vo_chn)) != HI_SUCCESS)
#endif
    {
        DEBUG_ERROR( "CHiVo::HiVo_stop_vo_chn HI_MPI_VO_ChnShow FAILED(%d, %d)(0x%08lx)\n", vo_dev, vo_chn, (unsigned long)ret_val);
        return -1;
    }

    return 0;
}

int CHiVo::HiVo_set_vo_framerate(VO_DEV vo_dev, int vo_chn, int framerate)
{
    int ret = 0;

    if ((vo_chn < 0) || (vo_dev < 0))
    {
        return 0;
    }
#if defined(_AV_PLATFORM_HI3535_)||defined(_AV_PLATFORM_HI3520D_V300_)
    VO_LAYER vo_layer;

    if(HiVo_get_vo_layer(vo_dev, vo_layer) != 0)
    {
        DEBUG_ERROR( "get vo layer failed(%d)!\n", vo_dev);
        return -1;
    }
    ret = HI_MPI_VO_SetChnFrameRate(vo_layer, vo_chn, framerate);
    if(ret != 0)
    {
        DEBUG_ERROR( "HI_MPI_VO_SetChnFrameRate err:0x%08x\n", ret);
        return -1;
    }
#else
    ret = HI_MPI_VO_SetChnFrameRate(vo_dev, vo_chn, framerate);
    if(ret != 0)
    {
        //DEBUG_ERROR( "HI_MPI_VO_SetChnFrameRate err:0x%08x\n", ret);
        return -1;
    }
#endif
    return 0;
}

int CHiVo::HiVo_get_vo_framerate(VO_DEV vo_dev, int vo_chn, int &framerate)
{
    int ret = 0;    

    if ((vo_chn < 0) || (vo_dev < 0))
    {
        return 0;
    }
#if defined(_AV_PLATFORM_HI3535_)||defined(_AV_PLATFORM_HI3520D_V300_)
    VO_LAYER vo_layer;

    if(HiVo_get_vo_layer(vo_dev, vo_layer) != 0)
    {
        DEBUG_ERROR( "get vo layer failed(%d)!\n", vo_dev);
        return -1;
    }
    ret = HI_MPI_VO_GetChnFrameRate(vo_layer, vo_chn, &framerate);
    if(ret != 0)
    {
        DEBUG_ERROR( "HI_MPI_VO_SetChnFrameRate err:0x%08x\n", ret);
        return -1;
    }
#else
    ret = HI_MPI_VO_GetChnFrameRate(vo_dev, vo_chn, &framerate);
    if(ret != 0)
    {
        DEBUG_ERROR( "HI_MPI_VO_SetChnFrameRate err:0x%08x\n", ret);
        return -1;
    }
#endif
    return 0;
}

int CHiVo::HiVo_resume_vo(VO_DEV vo_dev, int vo_chn)
{
    int ret = 0;

    if ((vo_chn < 0) || (vo_dev < 0))
    {
        return 0;
    }
#if defined(_AV_PLATFORM_HI3535_)||defined(_AV_PLATFORM_HI3520D_V300_)
    VO_LAYER vo_layer;
    if(HiVo_get_vo_layer(vo_dev, vo_layer) != 0)
    {
        DEBUG_ERROR( "get vo layer failed(%d)!\n", vo_dev);
        return -1;
    }
    ret = HI_MPI_VO_ResumeChn(vo_layer, vo_chn);
#else    
    ret = HI_MPI_VO_ChnResume(vo_dev, vo_chn);
#endif
    if(ret != 0)
    {
        DEBUG_ERROR("HI_MPI_VO_ChnResume err:0x%08x\n", ret);
        return -1;
    }
        
    return 0;
}

int CHiVo::HiVo_clear_vo(VO_DEV vo_dev, int vo_chn)
{
    int ret = 0;
    
    if ((vo_chn < 0) || (vo_dev < 0))
    {
        return 0;
    }
#if defined(_AV_PLATFORM_HI3535_)||defined(_AV_PLATFORM_HI3520D_V300_)
    VO_LAYER vo_layer;

    if (HiVo_get_vo_layer(vo_dev, vo_layer) != 0)
    {
        DEBUG_ERROR( "get vo layer failed(%d)!\n", vo_dev);
        return -1;
    }
    ret = HI_MPI_VO_ClearChnBuffer(vo_layer, vo_chn, HI_TRUE);
    if (ret != 0)
    {
        DEBUG_ERROR( "HI_MPI_VO_ClearChnBuffer err:0x%08x\n", ret);
    }
#else
    ret = HI_MPI_VO_ClearChnBuffer(vo_dev, vo_chn, HI_TRUE);
    if (ret != 0)
    {
        DEBUG_ERROR( "HI_MPI_VO_ClearChnBuffer err:0x%08x\n", ret);
    }
#endif
    return 0;
}


int CHiVo::HiVo_pause_vo(VO_DEV vo_dev, int vo_chn)
{
    int ret = 0;
    
    if ((vo_chn < 0) || (vo_dev < 0))
    {
        return 0;
    }
#if defined(_AV_PLATFORM_HI3535_)||defined(_AV_PLATFORM_HI3520D_V300_)
    VO_LAYER vo_layer;
    if(HiVo_get_vo_layer(vo_dev, vo_layer) != 0)
    {
        DEBUG_ERROR( "get vo layer failed(%d)!\n", vo_dev);
        return -1;
    }
    ret = HI_MPI_VO_PauseChn(vo_layer, vo_chn);
#else    
    ret = HI_MPI_VO_ChnPause(vo_dev, vo_chn);
#endif
    if(ret != 0)
    {
        DEBUG_ERROR( "HI_MPI_VO_ChnPause err:0x%08x\n", ret);
    }
    return 0;
}

int CHiVo::HiVo_enable_pip(VO_DEV vo_dev, VO_VIDEO_LAYER_ATTR_S PipLayerAttr)
{
    int ret = 0;

    ret = HI_MPI_VO_SetVideoLayerAttr(vo_dev, &PipLayerAttr);
    if(ret != 0)
    {
        DEBUG_ERROR("HI_MPI_VO_SetVideoLayerAttr err:0x%08x\n", ret);
        return -1;
    }

    ret = HI_MPI_VO_EnableVideoLayer(vo_dev);
    if(ret != 0)
    {
        DEBUG_ERROR("HI_MPI_VO_EnableVideoLayer err:0x%08x\n", ret);
        return -1;
    }
    return 0;
}


int CHiVo::HiVo_disable_pip(VO_DEV vo_dev)
{
    int ret = 0;

    ret = HI_MPI_VO_DisableVideoLayer(vo_dev);
    if(ret != 0)
    {
        DEBUG_ERROR("HI_MPI_VO_EnableVideoLayer err:0x%08x\n", ret);
        return -1;
    }

    return 0;
}

int CHiVo::HiVo_set_dispos(VO_DEV vo_dev, int vo_chn, RECT_S pipPositon)
{
    int ret = 0;

    DEBUG_MESSAGE("Position: (%d, %d)\n", pipPositon.s32X, pipPositon.s32Y);

    ret = HI_MPI_VO_SetDisplayRect(vo_dev,&pipPositon);
    if (ret !=0)
    {
        DEBUG_ERROR( "HI_MPI_VO_SetChnDispPos err:0x%08x\n", ret);
        return -1;
    }
    return 0;
}
int CHiVo::HiVo_refresh_vo(VO_DEV vo_dev, int vo_chn)
{
#if !defined(_AV_PLATFORM_HI3515_)

    int ret = 0;
    ret = HI_MPI_VO_ChnRefresh(vo_dev, vo_chn);
    if(ret != 0)
    {
        DEBUG_ERROR("HI_MPI_VO_ChnRefresh err:0x%08x\n", ret);
        return -1;
    }
    ret= ret;
#endif
    return 0;
}

int CHiVo::HiVo_set_attr_begin( VO_DEV vo_dev )
{
    HI_S32 sRet = HI_MPI_VO_SetAttrBegin( vo_dev );

    if( sRet != 0 )
    {
        DEBUG_ERROR("CHiVo::HiVo_set_attr_begin HI_MPI_VO_SetAttrBegin err:0x%08x\n", sRet);
        return -1;
    }

    return 0;
}

int CHiVo::HiVo_set_attr_end( VO_DEV vo_dev )
{
    HI_S32 sRet = HI_MPI_VO_SetAttrEnd( vo_dev );

    if( sRet != 0 )
    {
        DEBUG_ERROR("CHiVo::HiVo_set_attr_end HI_MPI_VO_SetAttrEnd err:0x%08x\n", sRet);
        return -1;
    }

    return 0;
}

#if defined(_AV_PLATFORM_HI3535_)||defined(_AV_PLATFORM_HI3520D_V300_)
int CHiVo::HiVo_get_vo_layer(VO_DEV vo_dev, VO_LAYER &vo_layer)
{
    switch(vo_dev)
    {
        case 0:
            vo_layer = 0;
            break;
        case 1:
#ifdef _AV_PLATFORM_HI3520D_V300_
            vo_layer = 2;
#else
            vo_layer = 1;
#endif
            break;
        case 2:
            vo_layer = 3;
            break;
        default:
            return -1;
            break;
    }

    return 0;
}

int CHiVo::HiVo_get_pip_layer()
{
    return 2;
}

int CHiVo::HiVo_start_pip_chn(VO_CHN VoChn, const VO_CHN_ATTR_S *pChnAttr)
{
    HI_S32 ret_val;
    VO_LAYER vo_layer = HiVo_get_pip_layer();

    if((ret_val = HI_MPI_VO_SetChnAttr(vo_layer, VoChn, pChnAttr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_start_vo_chn HI_MPI_VO_SetChnAttr FAILED(%d, %d)(0x%08lx)\n", vo_layer, VoChn, (unsigned long)ret_val);
        return -1;
    }
    if((ret_val = HI_MPI_VO_EnableChn(vo_layer, VoChn)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_start_vo_chn HI_MPI_VO_EnableChn FAILED(%d, %d)(0x%08lx)\n", vo_layer, VoChn, (unsigned long)ret_val);
        return -1;
    }
    return 0;
}

int CHiVo::HiVo_stop_pip_chn(VO_CHN VoChn)
{
    HI_S32 ret_val;
    VO_LAYER vo_layer = HiVo_get_pip_layer();

    if((ret_val = HI_MPI_VO_DisableChn(vo_layer, VoChn)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVo::HiVo_start_vo_chn HI_MPI_VO_EnableChn FAILED(%d, %d)(0x%08lx)\n", vo_layer, VoChn, (unsigned long)ret_val);
        return -1;
    }
    return 0;
}

#endif


#include "HiVpss.h"

#if defined(_AV_PLATFORM_HI3518C_)
#define VPSS_PRE0_CHN   2
#define VPSS_PRE1_CHN   3
#endif


CHiVpss::CHiVpss()
{
    m_pVpss_state = new vpss_state_t[VPSS_MAX_GRP_NUM];
    _AV_FATAL_(m_pVpss_state == NULL, "OUT OF MEMORY\n");

    for(int index = 0; index < VPSS_MAX_GRP_NUM; index ++)
    {
        m_pVpss_state[index].using_bitmap = 0;
    }

    m_pVideo_image = NULL;

    _AV_FATAL_((m_pThread_lock = new CAvThreadLock) == NULL, "OUT OF MEMORY\n");

#if defined(_AV_HAVE_VIDEO_INPUT_)
    m_pHi_vi = NULL;
#endif

#if defined(_AV_HAVE_VIDEO_DECODER_)
    m_pHi_av_dec = NULL;
#endif
}


CHiVpss::~CHiVpss()
{
    _AV_SAFE_DELETE_ARRAY_(m_pVpss_state);
    _AV_SAFE_DELETE_(m_pThread_lock);    
}


int CHiVpss::HiVpss_memory_balance(vpss_purpose_t purpose, int chn_num, void *pPurpose_para, VPSS_GRP vpss_grp)
{
#if defined(_AV_PLATFORM_HI3531_) || defined(_AV_PLATFORM_HI3532_)
    const char *pMemory_name = NULL;
    MPP_CHN_S mpp_chn;
    int ret_val = -1;

    if(((chn_num + 1) % 3) != 0)
    {
        pMemory_name = "ddr1";
    }
    mpp_chn.enModId  = HI_ID_VPSS;
    mpp_chn.s32DevId = vpss_grp;
    mpp_chn.s32ChnId = 0;

    if((ret_val = HI_MPI_SYS_SetMemConf(&mpp_chn, pMemory_name)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_memory_balance HI_MPI_SYS_SetMemConf FAILED(0x%08x)\n", ret_val);
    }
#endif

    return 0;
}

int CHiVpss::HiVpss_start_vpss(vpss_purpose_t purpose, int chn_num, VPSS_GRP vpss_grp, unsigned long grp_chn_bitmap, void *pPurpose_para)
{
    VPSS_GRP_ATTR_S vpss_grp_attr;
    VPSS_CHN_ATTR_S vpss_chn_attr;
    VPSS_GRP_PARAM_S vpss_grp_para;
#ifndef _AV_PLATFORM_HI3535_
    VPSS_CHN_MODE_S vpss_chn_mode;
    memset(&vpss_chn_mode,0,sizeof(VPSS_CHN_MODE_S));
#endif
    MPP_CHN_S chn_source;
    MPP_CHN_S chn_dest;
    HI_S32 ret_val = HI_SUCCESS;
    int max_width = 0;
    int max_height = 0;
    int vpss_grp_chn = 0;
#if defined(_AV_HAVE_VIDEO_INPUT_)
    VI_DEV vi_dev = 0;
    VI_CHN vi_chn = 0;
#endif
#if defined(_AV_HAVE_VIDEO_DECODER_)
    int vdec_dev = 0;
    VDEC_CHN vdec_chn;
#endif
	
    eProductType product_type = PTD_INVALID;
    product_type = pgAvDeviceInfo->Adi_product_type();
    /*collect information from source*/
    memset(&vpss_grp_attr, 0, sizeof(VPSS_GRP_ATTR_S));
    switch(purpose)
    {
#if defined(_AV_HAVE_VIDEO_INPUT_)
        case _VP_PREVIEW_VI_:
        case _VP_SPOT_VI_:
        case _VP_MAIN_STREAM_:
        case _VP_SUB_STREAM_:
        case _VP_SUB2_STREAM_:
        case _VP_VI_DIGITAL_ZOOM_:
            _AV_ASSERT_(m_pHi_vi != NULL, "You must call the function[HiVpss_set_vi] to set vi module pointer\n");
            if(m_pHi_vi->HiVi_get_vi_max_size(chn_num, &max_width, &max_height) != 0)
            {
                DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HiVi_get_vi_max_size(%d)\n", chn_num);
                return -1;
            }
            if(m_pHi_vi->HiVi_get_primary_vi_info(chn_num, &vi_dev, &vi_chn) != 0)
            {
                DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss m_pHi_vi->HiVi_get_primary_vi_info(%d)\n", chn_num);
                return -1;
            }
            /*source*/
            chn_source.enModId = HI_ID_VIU;
            chn_source.s32DevId = 0;
            chn_source.s32ChnId = vi_chn;
            /*vpss group attribute*/
            vpss_grp_attr.enPixFmt = pgAvDeviceInfo->Adi_get_vi_pixel_format();
            vpss_grp_attr.bNrEn = HI_TRUE;
#ifdef  _AV_PLATFORM_HI3520D_V300_
            vpss_grp_attr.enDieMode = VPSS_DIE_MODE_NODIE;
            vpss_grp_attr.bNrEn = HI_FALSE;
#else
            vpss_grp_attr.enDieMode = VPSS_DIE_MODE_AUTO;
#endif
            break;
#endif
        case _VP_CUSTOM_LOGO_:
            max_width = m_custom_size.w;
            max_height = m_custom_size.h;
            /*vpss group attribute*/
            vpss_grp_attr.enPixFmt = pgAvDeviceInfo->Adi_get_vi_pixel_format();
            vpss_grp_attr.bNrEn = HI_TRUE;
            vpss_grp_attr.enDieMode = VPSS_DIE_MODE_AUTO;
            break;
        case _VP_ACCESS_LOGO_:
            max_width = 704;
            max_height = 576;

            /*vpss group attribute*/
            vpss_grp_attr.enPixFmt = pgAvDeviceInfo->Adi_get_vi_pixel_format();
            vpss_grp_attr.bNrEn = HI_TRUE;
            vpss_grp_attr.enDieMode = VPSS_DIE_MODE_AUTO;
            break;
#if defined(_AV_HAVE_VIDEO_DECODER_)
        case _VP_PREVIEW_VDEC_:
        case _VP_SPOT_VDEC_:
        case _VP_PLAYBACK_VDEC_:
        case _VP_PREVIEW_VDEC_DIGITAL_ZOOM_:
        case _VP_SPOT_VDEC_DIGITAL_ZOOM_:
        case _VP_PLAYBACK_VDEC_DIGITAL_ZOOM_:
            {
                eVDecPurpose Vdecpurpose = VDP_PREVIEW_VDEC;
                if ((_VP_SPOT_VDEC_ == purpose) || (_VP_SPOT_VDEC_DIGITAL_ZOOM_ == purpose))
                {
                    Vdecpurpose = VDP_SPOT_VDEC;
                }
                else if ((_VP_PLAYBACK_VDEC_ == purpose) || (_VP_PLAYBACK_VDEC_DIGITAL_ZOOM_ == purpose))
                {
                    Vdecpurpose = VDP_PLAYBACK_VDEC;
                }
                _AV_ASSERT_(m_pHi_av_dec!= NULL, "You must call the function[HiVpss_set_vdec] to set vdec module pointer\n");
                if(m_pHi_av_dec->Ade_get_vdec_max_size(Vdecpurpose, chn_num, max_width, max_height) != 0)
                {
                    DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss m_pHi_av_dec->Ade_get_vdec_max_size(%d) failed\n", chn_num);
                    return -1;
                }
                if(m_pHi_av_dec->Ade_get_vdec_info(Vdecpurpose, chn_num, vdec_dev, vdec_chn) != 0)
                {
                    DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss m_pHi_av_dec->Ade_get_vdec_info(%d)\n", chn_num);
                    return -1;
                }
                /*source*/
                chn_source.enModId = HI_ID_VDEC;
                chn_source.s32DevId = 0;
                chn_source.s32ChnId = vdec_chn;
                /*vpss group attribute*/
                vpss_grp_attr.enPixFmt = pgAvDeviceInfo->Adi_get_vdec_pixel_format();
                vpss_grp_attr.bNrEn = HI_FALSE;
                vpss_grp_attr.enDieMode = VPSS_DIE_MODE_NODIE;
            }
            break;
#endif
        default:
            _AV_FATAL_(1, "CHiVpss::HiVpss_start_vpss You must give the implement(purpose:%d)\n", purpose);
            break;
    }

#ifdef _AV_PLATFORM_HI3518C_
    pgAvDeviceInfo->Adi_covert_vidoe_size_by_rotate( &max_width, &max_height );
#endif

#if defined(_AV_HAVE_VIDEO_INPUT_)
    vi_config_set_t vi_primary_config, vi_minor_config, vi_source_config;
    m_pHi_vi->HiVi_get_primary_vi_chn_config(chn_num, &vi_primary_config, &vi_minor_config, &vi_source_config);
#endif
    /*create vpss group*/
    if(_VP_PLAYBACK_VDEC_DIGITAL_ZOOM_ == purpose)
    {//由于回放时分辨率会发生变化导致电子放大异常，这里将其设为1080P
        max_width = 1920;
        max_height = 1080;
    }
    else
    if((purpose!=_VP_PREVIEW_VDEC_)&&(_VP_PLAYBACK_VDEC_!= purpose))
    {
#if defined(_AV_PLATFORM_HI3520D_V300_)||defined(_AV_PLATFORM_HI3520D_)
        if ((product_type == PTD_A5_AHD)||(product_type == PTD_M3)||(product_type == PTD_ES4206)||(product_type == PTD_ES8412))
        {
            max_width = 1920;
            max_height = 1080;
        }
        else
        {
            if((pgAvDeviceInfo->Adi_ad_type_isahd()==true))
            {
                max_width = 1280;
                max_height = 720;
            }
        }
#endif
    }
    if((pgAvDeviceInfo->Adi_ad_type_isahd()==true))
    {
        if(max_width<928)
        {
            max_width = 928;
        }
        if(max_height<576)
        {
            max_height = 576;
        }
    }

    vpss_grp_attr.u32MaxW = max_width;
    vpss_grp_attr.u32MaxH = max_height;
    //preview vi set Dr Db false
#if !defined(_AV_PLATFORM_HI3535_)&&!defined(_AV_PLATFORM_HI3520D_V300_)
    vpss_grp_attr.bDrEn = HI_FALSE;
    vpss_grp_attr.bDbEn = HI_FALSE;
#endif
    if(_VP_MAIN_STREAM_ == purpose || _VP_SUB_STREAM_ == purpose || _VP_SUB2_STREAM_ == purpose)
    {
        vpss_grp_attr.bHistEn = HI_TRUE;
    }
    else
    {
        vpss_grp_attr.bHistEn = HI_FALSE;
    }
#if defined(_AV_PLATFORM_HI3520D_V300_)
        vpss_grp_attr.bHistEn = HI_FALSE;
#endif
#if defined(_AV_PLATFORM_HI3531_) || defined(_AV_PLATFORM_HI3532_)
    vpss_grp_attr.bIeEn = HI_TRUE;
#else
    vpss_grp_attr.bIeEn = HI_FALSE;
#endif
    //vpss_grp_attr.bIeEn = HI_FALSE;
    //vpss_grp_attr.bNrEn = HI_FALSE;
#ifdef _AV_PLATFORM_HI3520D_V300_    
    printf("\nvpss_grp_attr[%d,%d,%d,%d,%d,%d,%d,%d,%d]\n",vpss_grp_attr.u32MaxW,vpss_grp_attr.u32MaxH,vpss_grp_attr.enPixFmt,vpss_grp_attr.bIeEn,
    vpss_grp_attr.bDciEn,vpss_grp_attr.bNrEn,vpss_grp_attr.bHistEn,vpss_grp_attr.bEsEn,vpss_grp_attr.enDieMode);
#endif
    if((ret_val = HI_MPI_VPSS_CreateGrp(vpss_grp, &vpss_grp_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HI_MPI_VPSS_CreateGrp FAILED(group:%d)(ret_val:0x%08lx)(w=%d,h=%d)\n", vpss_grp, (unsigned long)ret_val, max_width, max_height);
        return -1;
    }

    if (HiVpss_get_grp_param(vpss_grp, vpss_grp_para, purpose) < 0)
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HiVpss_get_grp_param FAILED(group:%d)\n", vpss_grp);
        return -1;
    }
#ifdef _AV_PLATFORM_HI3520D_V300_
    vpss_grp_para.u32IeStrength = 0;
#endif
   
    if((ret_val = HI_MPI_VPSS_SetGrpParam(vpss_grp, &vpss_grp_para)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HI_MPI_VPSS_SetGrpParam FAILED(group:%d)(ret_val:0x%08lx)\n", vpss_grp,(unsigned long)ret_val);
        return -1;
    }
    /*
        enable vpss group channel
    */
    for(vpss_grp_chn = 0; vpss_grp_chn < VPSS_MAX_CHN_NUM; vpss_grp_chn ++)
    {
        if(grp_chn_bitmap & (0x01ul << vpss_grp_chn))
        {
            memset(&vpss_chn_attr, 0, sizeof(VPSS_CHN_ATTR_S));
            vpss_chn_attr.bSpEn = HI_FALSE;
#if defined(_AV_PLATFORM_HI3535_)
            vpss_chn_attr.bBorderEn = HI_TRUE;
            vpss_chn_attr.stBorder.u32Color = 0xff00;
#elif defined(_AV_PLATFORM_HI3520D_V300_)
            vpss_chn_attr.bBorderEn = HI_FALSE;
            vpss_chn_attr.bUVInvert = HI_FALSE;
#else
            vpss_chn_attr.bFrameEn = HI_FALSE;
#endif

#if defined(_AV_PLATFORM_HI3521_) && defined(_AV_HAVE_VIDEO_INPUT_)
            VPSS_CHN_NR_PARAM_S vpss_chn_nr_param;
            if ((ret_val = HI_MPI_VPSS_GetChnNrParam(vpss_grp, vpss_grp_chn, &vpss_chn_nr_param)) != HI_SUCCESS)
            {
                DEBUG_ERROR( "CHiVpss::HI_MPI_VPSS_GetChnNrParam FAILED(group:%d)\n", vpss_grp);
                return -1;
            }
            
            vpss_chn_nr_param.u32SfStrength = 12;
            vpss_chn_nr_param.u32TfStrength = 3;

            if ((ret_val = HI_MPI_VPSS_SetChnNrParam(vpss_grp, vpss_grp_chn, &vpss_chn_nr_param)) != HI_SUCCESS)
            {
                DEBUG_ERROR( "CHiVpss::HI_MPI_VPSS_SetChnNrParam FAILED(group:%d)(vpss_chnnel: %d)(ret_val:0x%08lx)\n", vpss_grp, vpss_grp_chn, (unsigned long)ret_val);
                return -1;
            }

            VPSS_CHN_SP_PARAM_S vpss_chn_sp_param;
            if((ret_val =  HI_MPI_VPSS_GetChnSpParam(vpss_grp, vpss_grp_chn, &vpss_chn_sp_param)) != HI_SUCCESS)
            {
                DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HI_MPI_VPSS_GetChnSpParam FAILED(group:%d)(chn:%d)(ret_val:0x%08lx)\n", vpss_grp, vpss_grp_chn, (unsigned long)ret_val);
                return -1;
            }
            // set sp for encode stream
            switch (purpose)
            {
                case _VP_MAIN_STREAM_:
                case _VP_SUB_STREAM_:  
                case _VP_SUB2_STREAM_:
                    vpss_chn_attr.bSpEn = HI_TRUE;
                    vpss_chn_sp_param.u32LumaGain = 0;
                    break;
                    
                case _VP_PREVIEW_VI_:
                    vpss_chn_attr.bSpEn = HI_TRUE;
                    vpss_chn_sp_param.u32LumaGain = 0;
                    break;
                    
                default:
                    break;
            }

            if((ret_val =  HI_MPI_VPSS_SetChnSpParam(vpss_grp, vpss_grp_chn, &vpss_chn_sp_param)) != HI_SUCCESS)
            {
                DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HI_MPI_VPSS_GetChnSpParam FAILED(group:%d)(chn:%d)(ret_val:0x%08lx)\n", vpss_grp, vpss_grp_chn, (unsigned long)ret_val);
                return -1;
            }
#endif


            if((ret_val = HI_MPI_VPSS_SetChnAttr(vpss_grp, vpss_grp_chn, &vpss_chn_attr)) != HI_SUCCESS)
            {
                DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HI_MPI_VPSS_SetChnAttr FAILED(group:%d)(chn:%d)(ret_val:0x%08lx)\n", vpss_grp, vpss_grp_chn, (unsigned long)ret_val);
                return -1;
            }
            if((ret_val = HI_MPI_VPSS_EnableChn(vpss_grp, vpss_grp_chn)) != 0)
            {
                DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HI_MPI_VPSS_EnableChn FAILED(group:%d)(chn:%d)(ret_val:0x%08lx)\n", vpss_grp, vpss_grp_chn, (unsigned long)ret_val);
                return -1;
            }
#if !defined(_AV_PLATFORM_HI3535_)
#ifdef _AV_PLATFORM_HI3520D_V300_
            if(vpss_grp_chn == VPSS_CHN2)
#else
            if(vpss_grp_chn == VPSS_PRE0_CHN)
#endif
            {
                if((ret_val = HI_MPI_VPSS_GetChnMode(vpss_grp, vpss_grp_chn, &vpss_chn_mode)) != HI_SUCCESS)
                {
                    DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HI_MPI_VPSS_GetChnMode FAILED(group:%d)(chn:%d)(ret_val:0x%08lx)\n", vpss_grp, vpss_grp_chn, (unsigned long)ret_val);
                    return -1;
                }

#ifndef _AV_PRODUCT_CLASS_IPC_ 
#ifndef _AV_PRODUCT_CLASS_DVR_REI_
                if(((purpose==_VP_PREVIEW_VDEC_)||(purpose==_VP_PLAYBACK_VDEC_))&&(pgAvDeviceInfo->Adi_get_main_out_mode() == _AOT_CVBS_))
                {
                   // vpss_chn_mode.enCompressMode=COMPRESS_MODE_NONE;
                    vpss_chn_mode.enChnMode = VPSS_CHN_MODE_USER;

                    vpss_chn_mode.u32Width  = 720;
                    if(pgAvDeviceInfo->Adi_get_system() == _AT_PAL_)
                    {
                        vpss_chn_mode.u32Height = 576;
                    }
                    else
                    {
                        vpss_chn_mode.u32Height = 480;
                    }

                    vpss_chn_mode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
                }
                else
#endif
#endif
                {
                    /*Field-frame transfer，only valid for VPSS_PRE0_CHN*/
                    vpss_chn_mode.bDouble = HI_FALSE;
                }

                if((ret_val = HI_MPI_VPSS_SetChnMode(vpss_grp, vpss_grp_chn, &vpss_chn_mode)) != HI_SUCCESS)
                {
                    DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HI_MPI_VPSS_SetChnMode FAILED(group:%d)(chn:%d)(ret_val:0x%08lx)\n", vpss_grp, vpss_grp_chn, (unsigned long)ret_val);
                    return -1;
                }
            }
#ifdef _AV_PLATFORM_HI3520D_V300_ 
            if((vpss_grp_chn == VPSS_CHN3) && (pgAvDeviceInfo->Adi_get_main_out_mode() == _AOT_CVBS_)) 
#else
            if((vpss_grp_chn == VPSS_PRE1_CHN)  && (pgAvDeviceInfo->Adi_get_main_out_mode() == _AOT_CVBS_))
#endif
            {
                if((ret_val = HI_MPI_VPSS_GetChnMode(vpss_grp, vpss_grp_chn, &vpss_chn_mode)) != HI_SUCCESS)
                {
                    DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HI_MPI_VPSS_GetChnMode FAILED(group:%d)(chn:%d)(ret_val:0x%08lx)\n", vpss_grp, vpss_grp_chn, (unsigned long)ret_val);
                    return -1;
                }
                /*Field-frame transfer，only valid for VPSS_PRE0_CHN*/
                //vpss_chn_mode.bDouble = HI_FALSE;
                vpss_chn_mode.enChnMode = VPSS_CHN_MODE_USER;
                if(pgAvDeviceInfo->Adi_get_system() == _AT_PAL_)
                {
                    vpss_chn_mode.u32Height = 576;
                }
                else
                {
                    vpss_chn_mode.u32Height = 480;
                }
                if(pgAvDeviceInfo->Adi_ad_type_isahd()==true)
                {
                    vpss_chn_mode.u32Width = 928;
                }
                else
                {
                    vpss_chn_mode.u32Width = 720;
                }
                vpss_chn_mode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
                //vpss_chn_mode.
                if((ret_val = HI_MPI_VPSS_SetChnMode(vpss_grp, vpss_grp_chn, &vpss_chn_mode)) != HI_SUCCESS)
                {
                    DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HI_MPI_VPSS_SetChnMode FAILED(group:%d)(chn:%d)(ret_val:0x%08lx)\n", vpss_grp, vpss_grp_chn, (unsigned long)ret_val);
                    return -1;
                }
            }  
#endif          
        }

    }
    /*start group*/
    if((ret_val = HI_MPI_VPSS_StartGrp(vpss_grp)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HI_MPI_VPSS_StartGrp FAILED(group:%d)(ret_val:0x%08lx)\n", vpss_grp, (unsigned long)ret_val);
        return -1;
    }
    
    if ((purpose == _VP_CUSTOM_LOGO_) || (purpose == _VP_ACCESS_LOGO_))
    {
        return 0;
    }
    
    /*bind source*/
    chn_dest.enModId = HI_ID_VPSS;
    chn_dest.s32DevId = vpss_grp;
    chn_dest.s32ChnId = 0;
    if((ret_val = HI_MPI_SYS_Bind(&chn_source, &chn_dest)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HI_MPI_SYS_Bind FAILED(group:%d)(ret_val:0x%08lx)\n", vpss_grp, (unsigned long)ret_val);
        return -1;
    }

    return 0;
}


int CHiVpss::HiVpss_stop_vpss(vpss_purpose_t purpose, int chn_num, VPSS_GRP vpss_grp, unsigned long grp_chn_bitmap, void *pPurpose_para)
{
    MPP_CHN_S chn_source;
    MPP_CHN_S chn_dest;
    HI_S32 ret_val = HI_SUCCESS;
    int vpss_grp_chn = 0;
#if defined(_AV_HAVE_VIDEO_INPUT_)
    VI_DEV vi_dev = 0;
    VI_CHN vi_chn = 0;
#endif
#if defined(_AV_HAVE_VIDEO_DECODER_)
    int vdec_dev = 0;
    VDEC_CHN vdec_chn;
#endif

    /*confirm source*/
    switch(purpose)
    {
#if defined(_AV_HAVE_VIDEO_INPUT_)
        case _VP_PREVIEW_VI_:
        case _VP_SPOT_VI_:
        case _VP_MAIN_STREAM_:
        case _VP_SUB_STREAM_:
        case _VP_VI_DIGITAL_ZOOM_:
        case _VP_SUB2_STREAM_:
            _AV_ASSERT_(m_pHi_vi != NULL, "You must call the function[HiVpss_set_vi] to set vi module pointer\n");
            if(m_pHi_vi->HiVi_get_primary_vi_info(chn_num, &vi_dev, &vi_chn) != 0)
            {
                DEBUG_ERROR( "CHiVpss::HiVpss_stop_vpss m_pHi_vi->HiVi_get_primary_vi_info(%d)\n", chn_num);
                return -1;
            }
            /*source*/
            chn_source.enModId = HI_ID_VIU;
            chn_source.s32DevId = 0;
            chn_source.s32ChnId = vi_chn;
            break;
#endif
        case _VP_CUSTOM_LOGO_:
        case _VP_ACCESS_LOGO_:
            break;

#if defined(_AV_HAVE_VIDEO_DECODER_)
        case _VP_PREVIEW_VDEC_:
        case _VP_SPOT_VDEC_:
        case _VP_PLAYBACK_VDEC_:
        case _VP_PLAYBACK_VDEC_DIGITAL_ZOOM_:
        case _VP_PREVIEW_VDEC_DIGITAL_ZOOM_:
        case _VP_SPOT_VDEC_DIGITAL_ZOOM_:
        {
            eVDecPurpose Vdecpurpose = VDP_PREVIEW_VDEC;
            if ((_VP_SPOT_VDEC_ == purpose) || ((_VP_SPOT_VDEC_DIGITAL_ZOOM_== purpose)))
            {
                Vdecpurpose = VDP_SPOT_VDEC;
            }
            else if ((_VP_PLAYBACK_VDEC_ == purpose) || (_VP_PLAYBACK_VDEC_DIGITAL_ZOOM_ == purpose))
            {
                Vdecpurpose = VDP_PLAYBACK_VDEC;
            }
            _AV_ASSERT_(m_pHi_av_dec!= NULL, "You must call the function[HiVpss_set_vdec] to set vdec module pointer\n");
            if(m_pHi_av_dec->Ade_get_vdec_info(Vdecpurpose, chn_num, vdec_dev, vdec_chn) != 0)
            {
                DEBUG_ERROR( "CHiVpss::HiVpss_stop_vpss m_pHi_av_dec->Ade_get_vdec_info(%d)\n", chn_num);
                return -1;
            }
            /*source*/
            chn_source.enModId = HI_ID_VDEC;
            chn_source.s32DevId = 0;
            chn_source.s32ChnId = vdec_chn;
            break;
       }
#endif

        default:
            _AV_FATAL_(1, "CHiVpss::HiVpss_stop_vpss You must give the implement(purpose:%d)\n", purpose);
            break;
    }

    /*unbind*/
    if((purpose != _VP_CUSTOM_LOGO_) && (purpose != _VP_ACCESS_LOGO_))
    {
        chn_dest.enModId = HI_ID_VPSS;
        chn_dest.s32DevId = vpss_grp;
        chn_dest.s32ChnId = 0;
        if((ret_val = HI_MPI_SYS_UnBind(&chn_source, &chn_dest)) != HI_SUCCESS)
        {
            DEBUG_ERROR( "CHiVpss::HiVpss_stop_vpss HI_MPI_SYS_UnBind FAILED(group:%d)(ret_val:0x%08lx)\n", vpss_grp, (unsigned long)ret_val);
            return -1;
        }
    }
//#ifndef _AV_PLATFORM_HI3520D_V300_
    /*disable vpss grp channel*/
    for(vpss_grp_chn = 0; vpss_grp_chn < VPSS_MAX_CHN_NUM; vpss_grp_chn ++)
    {
        if(grp_chn_bitmap & (0x01ul << vpss_grp_chn))
        {
            if((ret_val = HI_MPI_VPSS_DisableChn(vpss_grp, vpss_grp_chn)) != 0)
            {
                DEBUG_ERROR( "CHiVpss::HiVpss_stop_vpss HI_MPI_VPSS_DisableChn FAILED(group:%d)(chn:%d)(ret_val:0x%08lx)\n", vpss_grp, vpss_grp_chn, (unsigned long)ret_val);
                return -1;
            }
        }
    }
//#endif
    /*stop group*/
    if((ret_val = HI_MPI_VPSS_StopGrp(vpss_grp)) != 0)
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_stop_vpss HI_MPI_VPSS_StopGrp FAILED(group:%d)(ret_val:0x%08lx)\n", vpss_grp, (unsigned long)ret_val);
        return -1;
    }
	
#if 0//def _AV_PLATFORM_HI3520D_V300_
    /*disable vpss grp channel*/
    for(vpss_grp_chn = 0; vpss_grp_chn < VPSS_MAX_CHN_NUM; vpss_grp_chn ++)
    {
        if(grp_chn_bitmap & (0x01ul << vpss_grp_chn))
        {
            if((ret_val = HI_MPI_VPSS_DisableChn(vpss_grp, vpss_grp_chn)) != 0)
            {
                DEBUG_ERROR( "CHiVpss::HiVpss_stop_vpss HI_MPI_VPSS_DisableChn FAILED(group:%d)(chn:%d)(ret_val:0x%08lx)\n", vpss_grp, vpss_grp_chn, (unsigned long)ret_val);
                return -1;
            }
        }
    }
#endif

    /*destory group*/
    if((ret_val = HI_MPI_VPSS_DestroyGrp(vpss_grp)) != 0)
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_stop_vpss HI_MPI_VPSS_DestroyGrp FAILED(group:%d)(ret_val:0x%08lx)\n", vpss_grp, (unsigned long)ret_val);
        return -1;
    }

    return 0;
}

#ifndef _AV_PRODUCT_CLASS_IPC_
int CHiVpss::HiVpss_allotter_DVR_REGULAR(vpss_purpose_t purpose, int chn_num, VPSS_GRP *pVpss_grp, VPSS_CHN *pVpss_chn, unsigned long *pGrp_chn_bitmap, void *pPurpose_para)
{
#ifndef _AV_PLATFORM_HI3535_
    int max_chn_num = pgAvDeviceInfo->Adi_max_channel_number();
    _AV_ASSERT_(chn_num < max_chn_num, "CHiVpss::HiVpss_allotter_DVR_REGULAR INVALID CHANNEL NUMBER(%d, %d)\n", max_chn_num, chn_num);
    switch(purpose)
    {
        case _VP_PREVIEW_VI_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num);
#ifdef  _AV_PLATFORM_HI3520D_V300_
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_CHN3);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_CHN2) | (0x01ul << VPSS_CHN0) | (0x01ul << VPSS_CHN1) | (0x01ul << VPSS_CHN3));
#else
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_PRE1_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_LSTR_CHN) | (0x01ul << VPSS_PRE1_CHN));
#endif
            break;
        case _VP_SPOT_VI_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num + max_chn_num * 2);
#ifdef  _AV_PLATFORM_HI3520D_V300_
            //_AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_CHN2);
            //_AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_CHN2) );
#else
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_BYPASS_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_BYPASS_CHN));
#endif
            break;

        case _VP_MAIN_STREAM_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num);
#ifdef _AV_PLATFORM_HI3520D_V300_
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_CHN0);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_CHN2) | (0x01ul << VPSS_CHN0) | (0x01ul << VPSS_CHN1) | (0x01ul << VPSS_CHN3));
#else
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_BSTR_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_LSTR_CHN) | (0x01ul << VPSS_PRE1_CHN));
#endif
            break;

        case _VP_SUB_STREAM_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num);
#ifdef _AV_PLATFORM_HI3520D_V300_
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_CHN1);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_CHN2) | (0x01ul << VPSS_CHN0) | (0x01ul << VPSS_CHN1) | (0x01ul << VPSS_CHN3));
#else
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_LSTR_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_LSTR_CHN) | (0x01ul << VPSS_PRE1_CHN));
#endif
            break;

        case _VP_SUB2_STREAM_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num);
#ifdef  _AV_PLATFORM_HI3520D_V300_
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_CHN2);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_CHN2) | (0x01ul << VPSS_CHN0) | (0x01ul << VPSS_CHN1) | (0x01ul << VPSS_CHN3));
#else
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_PRE0_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_LSTR_CHN) | (0x01ul << VPSS_PRE1_CHN));
#endif
            break;

        case _VP_PLAYBACK_VDEC_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num + max_chn_num);
#ifdef  _AV_PLATFORM_HI3520D_V300_
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_CHN2);
            //VPSS_LSTR_CHN which be used for playback digital zoom on paused condition
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_CHN2) | (0x01ul << VPSS_CHN1) );
#else
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_PRE0_CHN);
            //VPSS_LSTR_CHN which be used for playback digital zoom on paused condition
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_LSTR_CHN) );
#endif
            break;

        case _VP_PREVIEW_VDEC_DIGITAL_ZOOM_:
        case _VP_SPOT_VDEC_DIGITAL_ZOOM_:
        case _VP_PLAYBACK_VDEC_DIGITAL_ZOOM_:
        case _VP_VI_DIGITAL_ZOOM_:
        case _VP_CUSTOM_LOGO_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, max_chn_num * 3);
#ifdef _AV_PLATFORM_HI3520D_V300_
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_CHN2);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_CHN2));
#else
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_PRE0_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN));
#endif
            break;
        case _VP_ACCESS_LOGO_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, max_chn_num * 3 + 1);
#ifdef  _AV_PLATFORM_HI3520D_V300_
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_CHN2);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_CHN2));
#else
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_PRE0_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN));
#endif
            break;

            case _VP_PREVIEW_VDEC_:
#ifdef _AV_PRODUCT_CLASS_HDVR_
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num + max_chn_num);
#else
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num);
#endif
#ifdef _AV_PLATFORM_HI3535_
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_BSTR_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_PRE0_CHN));
#elif defined(_AV_PLATFORM_HI3520D_V300_)
			_AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_CHN2);
			_AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_CHN2) | (0x01ul << VPSS_CHN1));
#else
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_PRE0_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_LSTR_CHN));
#endif
        break;
        case _VP_SPOT_VDEC_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num + max_chn_num);
#ifdef _AV_PLATFORM_HI3535_
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_PRE0_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_BSTR_CHN) | 0x01ul << VPSS_PRE0_CHN));
#elif  defined(_AV_PLATFORM_HI3520D_V300_)
			_AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_CHN1);
			_AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_CHN2) | (0x01ul << VPSS_CHN1));
#else
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_LSTR_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_LSTR_CHN));
#endif
            break;
        default:
            _AV_FATAL_(1, "CHiVpss::HiVpss_allotter_DVR_REGULAR chn_num %d purpose %d You must give the implement\n", chn_num, purpose);
            break;
    }
#endif
    return 0;
}


int CHiVpss::HiVpss_allotter_NVR_REGULAR(vpss_purpose_t purpose, int chn_num, VPSS_GRP *pVpss_grp, VPSS_CHN *pVpss_chn, unsigned long *pGrp_chn_bitmap, void *pPurpose_para)
{
    int max_chn_num = pgAvDeviceInfo->Adi_max_channel_number();
    switch(purpose)
    {
        case _VP_PREVIEW_VDEC_:
#ifdef _AV_PRODUCT_CLASS_HDVR_
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num+max_chn_num);
#else
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num);
#endif
#if defined(_AV_PLATFORM_HI3535_)
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_BSTR_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_PRE0_CHN));
#elif defined(_AV_PLATFORM_HI3520D_V300_)
           _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_CHN0);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_CHN0) | (0x01ul << VPSS_CHN2));
#else
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_PRE0_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_LSTR_CHN));
#endif
            break;

        case _VP_SPOT_VDEC_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num);
#if defined(_AV_PLATFORM_HI3535_)
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_PRE0_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_PRE0_CHN));
#elif defined(_AV_PLATFORM_HI3520D_V300_)
           _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_CHN2);
			_AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_CHN0) | (0x01ul << VPSS_CHN0));
#else
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_LSTR_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_LSTR_CHN));
#endif
            break;

        case _VP_PLAYBACK_VDEC_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num + max_chn_num);
#if defined(_AV_PLATFORM_HI3535_)
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_BSTR_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, 0x01ul << VPSS_BSTR_CHN);
#elif defined(_AV_PLATFORM_HI3520D_V300_)
           _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_CHN0);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, 0x01ul << VPSS_CHN0);
#else
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_PRE0_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_LSTR_CHN));
#endif
            break;
   
        case _VP_PREVIEW_VDEC_DIGITAL_ZOOM_:
        case _VP_SPOT_VDEC_DIGITAL_ZOOM_:
        case _VP_PLAYBACK_VDEC_DIGITAL_ZOOM_:
        case _VP_VI_DIGITAL_ZOOM_:
        case _VP_CUSTOM_LOGO_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, max_chn_num * 3);
#ifdef _AV_PLATFORM_HI3520D_V300_
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_CHN2);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_CHN2));
#else
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_PRE0_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN));
#endif
            break;
        case _VP_ACCESS_LOGO_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, max_chn_num * 3 + 1);
#ifdef _AV_PLATFORM_HI3520D_V300_	
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_CHN2);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_CHN2));
#else
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_PRE0_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN));
#endif
            break;
        default:
            _AV_FATAL_(1, "CHiVpss::HiVpss_allotter_NVR_REGULAR You must give the implement\n");
            break;
    }
    return 0;
}
#else
int CHiVpss::HiVpss_allotter_IPC_REGULAR(vpss_purpose_t purpose, int chn_num, VPSS_GRP *pVpss_grp, VPSS_CHN *pVpss_chn, unsigned long *pGrp_chn_bitmap, void *pPurpose_para)
{
    int max_chn_num = pgAvDeviceInfo->Adi_max_channel_number();
    _AV_ASSERT_(chn_num < max_chn_num, "CHiVpss::HiVpss_allotter_IPC_REGULAR INVALID CHANNEL NUMBER(%d, %d)\n", max_chn_num, chn_num);

    switch(purpose)
    {
        case _VP_MAIN_STREAM_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num);
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_BSTR_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_LSTR_CHN));
            break;

        case _VP_SUB_STREAM_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num);
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_LSTR_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_LSTR_CHN));
            break;
        case _VP_SUB2_STREAM_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num);
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_PRE0_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_LSTR_CHN));
            break;
        default:
            _AV_FATAL_(1, "CHiVpss::HiVpss_allotter_DVR_REGULAR You must give the implement\n");
            break;
    }

    return 0;
}
#endif

int CHiVpss::HiVpss_allotter(vpss_purpose_t purpose, int chn_num, VPSS_GRP *pVpss_grp, VPSS_CHN *pVpss_chn, unsigned long *pGrp_chn_bitmap, void *pPurpose_para)
{
#if defined(_AV_PRODUCT_CLASS_DVR_) || defined(_AV_PRODUCT_CLASS_HDVR_)
    HiVpss_allotter_DVR_REGULAR(purpose, chn_num, pVpss_grp, pVpss_chn, pGrp_chn_bitmap, pPurpose_para);
#elif defined(_AV_PRODUCT_CLASS_NVR_)
    HiVpss_allotter_NVR_REGULAR(purpose, chn_num, pVpss_grp, pVpss_chn, pGrp_chn_bitmap, pPurpose_para);
#elif defined(_AV_PRODUCT_CLASS_IPC_)
    HiVpss_allotter_IPC_REGULAR(purpose, chn_num, pVpss_grp, pVpss_chn, pGrp_chn_bitmap, pPurpose_para);
#else
    _AV_FATAL_(1, "CHiVpss::HiVpss_allotter You must give the implement\n");
#endif
    return 0;
}

int CHiVpss::HiVpss_vpss_manager(vpss_purpose_t purpose, int chn_num, VPSS_GRP *pVpss_grp, VPSS_CHN *pVpss_chn, void *pPurpose_para, int flag)
{
    unsigned long using_bitmap = 0;
    unsigned long grp_chn_bitmap = 0;
    VPSS_GRP vpss_grp = 0;
    VPSS_CHN vpss_chn = 0; 
    // ret_val = 0; -1?
    int ret_val = 0;
    /*alloc vpss group and channel*/
    if(HiVpss_allotter(purpose, chn_num, &vpss_grp, &vpss_chn, &grp_chn_bitmap, pPurpose_para) != 0)
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_vpss_manager HiVpss_allotter FAILED(%d, %d)\n", purpose, chn_num);
        return -1;
    }

    _AV_ASSERT_(vpss_grp < VPSS_MAX_GRP_NUM, "CHiVpss::HiVpss_vpss_manager INVALID VPSS GROUP(%d)(%d)(%d)(%d)\n", purpose, chn_num, vpss_grp, VPSS_MAX_GRP_NUM);
    _AV_ASSERT_(vpss_chn < VPSS_MAX_CHN_NUM, "CHiVpss::HiVpss_vpss_manager INVALID VPSS CHN(%d)(%d)(%d)(%d)\n", purpose, chn_num, vpss_chn, VPSS_MAX_CHN_NUM);

    /*memory balance*/
    if((flag == 0) && (m_pVpss_state[vpss_grp].using_bitmap == 0))
    {
        HiVpss_memory_balance(purpose, chn_num, pPurpose_para, vpss_grp);
    }
    /*control*/
    switch(flag)
    {
        case 0:/*get*/
            if(m_pVpss_state[vpss_grp].using_bitmap == 0)
            {
                ret_val = HiVpss_start_vpss(purpose, chn_num, vpss_grp, grp_chn_bitmap, pPurpose_para);
                if(ret_val != 0)
                {
                    DEBUG_ERROR( "CHiVpss::HiVpss_vpss_manager HiVpss_start_vpss FAILED %d purpose=%d chnum=%d grp=%d bit=0x%lx\n", ret_val, purpose, chn_num, vpss_grp, grp_chn_bitmap);
                    HiVpss_stop_vpss(purpose, chn_num, vpss_grp, grp_chn_bitmap, pPurpose_para);
                }
            }
            if(ret_val == 0)
            {
                m_pVpss_state[vpss_grp].using_bitmap |= (0x01ul << vpss_chn);
            }
            break;
        case 1:/*release*/
            using_bitmap = m_pVpss_state[vpss_grp].using_bitmap & (~(0x01ul << vpss_chn));
            if((m_pVpss_state[vpss_grp].using_bitmap != 0) && (using_bitmap == 0))
            {
                ret_val = HiVpss_stop_vpss(purpose, chn_num, vpss_grp, grp_chn_bitmap, pPurpose_para);
            }
            if(ret_val == 0)
            {
                m_pVpss_state[vpss_grp].using_bitmap = using_bitmap;
            }
            break;
        default:
            _AV_FATAL_(1, "CHiVpss::HiVpss_vpss_manager You must give the implement\n");
            break;
    }

    _AV_POINTER_ASSIGNMENT_(pVpss_grp, vpss_grp);
    _AV_POINTER_ASSIGNMENT_(pVpss_chn, vpss_chn);

    return ret_val;
}

int CHiVpss::HiVpss_find_vpss(vpss_purpose_t purpose, int chn_num, VPSS_GRP *pVpss_grp, VPSS_CHN *pVpss_chn, void *pPurpose_para/* = NULL*/)
{
    int ret_val = -1;

    m_pThread_lock->lock();
    
    ret_val = HiVpss_allotter(purpose, chn_num, pVpss_grp, pVpss_chn, NULL, pPurpose_para);

    m_pThread_lock->unlock();

    return ret_val;
}


int CHiVpss::HiVpss_get_vpss(vpss_purpose_t purpose, int chn_num, VPSS_GRP *pVpss_grp, VPSS_CHN *pVpss_chn, void *pPurpose_para/* = NULL*/)
{
    int ret_val = -1;

    m_pThread_lock->lock();
    
    ret_val = HiVpss_vpss_manager(purpose, chn_num, pVpss_grp, pVpss_chn, pPurpose_para, 0);

    m_pThread_lock->unlock();

    //DEBUG_MESSAGE( "CHiVpss::HiVpss_get_vpss(purpose:%d)(chn_num:%d)(vpss_grp:%d)(vpss_chn:%d)\n", purpose, chn_num, (pVpss_grp != NULL)?*pVpss_grp:0xff, (pVpss_chn != NULL)?*pVpss_chn:0xff);

    return ret_val;
}

bool CHiVpss::Hivpss_whether_create_vpssgroup(VPSS_GRP vpss_grp)
{
    return m_pVpss_state[vpss_grp].using_bitmap != 0;
}

int CHiVpss::HiVpss_release_vpss(vpss_purpose_t purpose, int chn_num, void *pPurpose_para/* = NULL*/)
{
    int ret_val = -1;

    m_pThread_lock->lock();
    
    ret_val = HiVpss_vpss_manager(purpose, chn_num, NULL, NULL, pPurpose_para, 1);

    m_pThread_lock->unlock();

    //DEBUG_MESSAGE( "CHiVpss::HiVpss_release_vpss(purpose:%d)(chn_num:%d)\n", purpose, chn_num);

    return ret_val;
}


int CHiVpss::HiVpss_set_vpssgrp_cropcfg(bool enable, VPSS_GRP vpss_grp, av_rect_t *pRect, unsigned int maxw, unsigned int maxh)
{
    int ret = 0;
    VPSS_CROP_INFO_S VpssClipInfo;

#if !defined(_AV_PLATFORM_HI3535_)&&!defined(_AV_PLATFORM_HI3520D_V300_)
    VpssClipInfo.enCapSel = VPSS_CAPSEL_BOTH;
#endif
    if((pRect->w != 0) && (pRect->h != 0))
    {
        VpssClipInfo.bEnable = HI_TRUE;
        VpssClipInfo.enCropCoordinate = VPSS_CROP_ABS_COOR;
        VpssClipInfo.stCropRect.s32X = ALIGN_BACK(pRect->x, 4);
        VpssClipInfo.stCropRect.s32Y = ALIGN_BACK(pRect->y, 4);
        VpssClipInfo.stCropRect.u32Width = ALIGN_BACK(pRect->w, 16);
        VpssClipInfo.stCropRect.u32Height = ALIGN_BACK(pRect->h, 16);

        if (VpssClipInfo.stCropRect.u32Width > maxw)
        {
            VpssClipInfo.stCropRect.u32Width = maxw;
        }

        if (VpssClipInfo.stCropRect.u32Height > maxh)
        {
            VpssClipInfo.stCropRect.u32Height = maxh;
        }

        if((VpssClipInfo.stCropRect.s32X+VpssClipInfo.stCropRect.u32Width) > maxw)
        {
            VpssClipInfo.stCropRect.s32X = maxw - VpssClipInfo.stCropRect.u32Width + 4;
            VpssClipInfo.stCropRect.s32X = ALIGN_BACK(VpssClipInfo.stCropRect.s32X, 4);
        }

        if((VpssClipInfo.stCropRect.s32Y+VpssClipInfo.stCropRect.u32Height) > maxh)
        {
            VpssClipInfo.stCropRect.s32Y = maxh -VpssClipInfo.stCropRect.u32Height + 4;
            VpssClipInfo.stCropRect.s32Y = ALIGN_BACK(VpssClipInfo.stCropRect.s32Y, 4);
        }
    }
    else
    {
#if defined(_AV_PLATFORM_HI3535_) ||defined(_AV_PLATFORM_HI3520D_V300_)
        ret = HI_MPI_VPSS_GetGrpCrop(vpss_grp, &VpssClipInfo);
#else
        ret = HI_MPI_VPSS_GetCropCfg(vpss_grp, &VpssClipInfo);
#endif
        if(ret != 0)
        {
            DEBUG_MESSAGE( "HI_MPI_VPSS_GetCropCfg err:0x%08x, VpssGrp %d\n", ret, vpss_grp);
        }
    }

    if (true == enable)
    {
        VpssClipInfo.bEnable = HI_TRUE;
    }
    else
    {
        VpssClipInfo.bEnable = HI_FALSE;
    }
    DEBUG_MESSAGE("\n\nSetCropCfg [%d, %d, %d, %d] width:%d height:%d\n\n", VpssClipInfo.stCropRect.s32X, VpssClipInfo.stCropRect.s32Y,
                              VpssClipInfo.stCropRect.u32Width, VpssClipInfo.stCropRect.u32Height, maxw, maxh);
#if defined(_AV_PLATFORM_HI3535_)||defined(_AV_PLATFORM_HI3520D_V300_)
    ret = HI_MPI_VPSS_SetGrpCrop(vpss_grp, &VpssClipInfo);
#else
    ret = HI_MPI_VPSS_SetCropCfg(vpss_grp, &VpssClipInfo);
#endif
    if(ret != 0)
    {
        DEBUG_MESSAGE( "HI_MPI_VPSS_SetClipCfg err:0x%08x, VpssGrp %d\n", ret, vpss_grp);
        DEBUG_MESSAGE("HI_MPI_VPSS_SetClipCfg err:0x%08x, VpssGrp %d\n", ret, vpss_grp);
        return -1;
    }
    return 0;
}

int CHiVpss::HiVpss_send_user_frame(VPSS_GRP vpss_grp, VIDEO_FRAME_INFO_S *pstVideoFrame)
{
#if defined(_AV_PLATFORM_HI3535_)
    int ret_val = HI_MPI_VPSS_SendFrame(vpss_grp, pstVideoFrame, -1);
#elif defined(_AV_PLATFORM_HI3520D_V300_)
    int ret_val = HI_MPI_VPSS_SendFrame(vpss_grp, pstVideoFrame, -1);
#else
    int ret_val = HI_MPI_VPSS_UserSendFrame(vpss_grp, pstVideoFrame);
#endif
    if (ret_val != 0)
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_send_user_frame HI_MPI_VPSS_UserSendFrame failed with 0x%x,group=%d\n", ret_val, vpss_grp);
        //return -1;
    }
    
    return 0;
}


int CHiVpss::HiVpss_ResetVpss( vpss_purpose_t purpose, int chn_num )
{
    VPSS_GRP vpss_grp=-1;
    VPSS_CHN vpss_chn;
    unsigned long grp_chn_bitmap;

    m_pThread_lock->lock();
    if ( HiVpss_allotter( purpose, chn_num, &vpss_grp, &vpss_chn, &grp_chn_bitmap, NULL) != 0 )
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_vpss_manager HiVpss_allotter FAILED(%d, %d)\n", purpose, chn_num);
        m_pThread_lock->unlock();
        return -1;
    }

  //  DEBUG_MESSAGE("Reset vpss group=%d vpsschn=%d purpose=%d chn=%d\n", vpss_grp, vpss_chn, purpose, chn_num );
    HI_S32 sRet = HI_MPI_VPSS_ResetGrp(vpss_grp);

    if ( sRet != HI_SUCCESS )
    {
        m_pThread_lock->unlock();
        return -1;
    }
    m_pThread_lock->unlock();
    
    return 0;
}

int CHiVpss::HiVpss_get_frame_for_snap( int chn_num, VIDEO_FRAME_INFO_S *pstVideoFrame )
{
    VPSS_GRP vpssGrp=-1;
    VPSS_CHN vpssChn=-1;
    
    if( HiVpss_find_vpss( _VP_PREVIEW_VI_, chn_num, &vpssGrp, &vpssChn ) != 0 )
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_get_frame_for_snap HiVpss_find_vpss failed\n");
        return -1;
    }
//use this function, must do the following things:
// 1\HI_MPI_VPSS_SetChnAttr set vpss channel workmode user
// 2\HI_MPI_VPSS_SetDepth set depth > 0
#ifdef _AV_PLATFORM_HI3535_
    HI_S32 sRet = HI_MPI_VPSS_GetChnFrame( vpssGrp, vpssChn, pstVideoFrame );
#else
#if defined(_AV_PLATFORM_HI3520D_V300_)
    HI_S32 sRet = HI_MPI_VPSS_GetChnFrame( vpssGrp, vpssChn, pstVideoFrame,HI_FAILURE);
#else
    HI_S32 sRet = HI_MPI_VPSS_UserGetFrame( vpssGrp, vpssChn, pstVideoFrame );
#endif
#endif
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_get_frame_for_snap HI_MPI_VPSS_UserGetFrame failed width 0x%08x\n", sRet);
        return -1;
    }
    
    return 0;
}


int CHiVpss::HiVpss_set_vpssgrp_param(av_video_image_para_t video_image)
{
    if (m_pVideo_image == NULL)
    {
        _AV_FATAL_((m_pVideo_image = new av_video_image_para_t) == NULL, "OUT OF MEMORY\n");
    }
    memcpy(m_pVideo_image, &video_image, sizeof(av_video_image_para_t));
    return 0;
}

int CHiVpss::HiVpss_get_vpssgrp_param(av_video_image_para_t &video_image)
{
    if (m_pVideo_image)
    {
        memcpy(&video_image, m_pVideo_image, sizeof(av_video_image_para_t));
    }
    return 0;
}


int CHiVpss::HiVpss_get_grp_param(VPSS_GRP vpss_grp, VPSS_GRP_PARAM_S &vpss_grp_para, vpss_purpose_t purpose)
{
    HI_S32 ret_val = HI_SUCCESS;
    
    if((ret_val = HI_MPI_VPSS_GetGrpParam(vpss_grp, &vpss_grp_para)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_get_grp_param HI_MPI_VPSS_GetGrpParam FAILED(group:%d)(ret_val:0x%08lx)\n", vpss_grp, (unsigned long)ret_val);
        return -1;
    }
#if defined(_AV_PLATFORM_HI3521_)
    vpss_grp_para.u32IeStrength = 4;
#else 
#if !defined(_AV_PLATFORM_HI3535_)&&!defined(_AV_PLATFORM_HI3520D_V300_)
    vpss_grp_para.u32MotionThresh = 2;
    vpss_grp_para.u32DiStrength = 7;
    vpss_grp_para.u32BrightEnhance = 16;
    vpss_grp_para.u32DarkEnhance = 16;	
#endif
#if defined(_AV_PLATFORM_HI3531_)
    vpss_grp_para.u32SfStrength = 6;
    vpss_grp_para.u32TfStrength = 3;
#else 
    vpss_grp_para.u32SfStrength = 24;
    vpss_grp_para.u32TfStrength = 12;
#endif
    
    if (_VP_MAIN_STREAM_ == purpose || _VP_SUB_STREAM_ == purpose 
        || _VP_SUB2_STREAM_ == purpose || _VP_PREVIEW_VI_ == purpose)
    {
#if !defined(_AV_PLATFORM_HI3535_)&&!defined(_AV_PLATFORM_HI3520D_V300_)
        vpss_grp_para.u32BrightEnhance = 40;
        vpss_grp_para.u32DarkEnhance = 32;
#endif        
    }

    if (_VP_PREVIEW_VDEC_ == purpose || _VP_PLAYBACK_VDEC_ == purpose)
    {
#if !defined(_AV_PLATFORM_HI3535_)&&!defined(_AV_PLATFORM_HI3520D_V300_)
        vpss_grp_para.u32IeSharp = 0;
#endif
    }
#endif
    return 0;
}

#ifdef _AV_PRODUCT_CLASS_IPC_
int CHiVpss::HiVpss_3DNoiseFilter( vpss_purpose_t purpose, int chn_num, unsigned int u32Sf, unsigned int u32Tf, unsigned int u32ChromaRg )
{
    VPSS_GRP vpssGrp;
    if( HiVpss_find_vpss(purpose, chn_num, &vpssGrp, NULL, NULL) != 0 )
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_3DNoiseFilter HiVpss_find_vpss failed\n");
        return -1;
    }

    VPSS_GRP_PARAM_S stuVpssGrpParam;
    HI_S32 sRet = HI_SUCCESS;
    
    sRet = HI_MPI_VPSS_GetGrpParam( vpssGrp, &stuVpssGrpParam );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_3DNoiseFilter HI_MPI_VPSS_GetGrpParam failed width 0x%08x\n", sRet);
        return -1;
    }

    stuVpssGrpParam.u32SfStrength = u32Sf;
    stuVpssGrpParam.u32TfStrength = u32Tf;
    stuVpssGrpParam.u32ChromaRange = u32ChromaRg;
    
    DEBUG_MESSAGE( "CHiVpss::HiVpss_3DNoiseFilter vpss 3Dnoise: sf=%d tf=%d chromaRg=%d vpssGrp=%d chn=%d\n"
        , u32Sf, u32Tf, u32ChromaRg, vpssGrp, chn_num );
    sRet = HI_MPI_VPSS_SetGrpParam( vpssGrp, &stuVpssGrpParam );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_3DNoiseFilter HI_MPI_VPSS_SetGrpParam failed width 0x%08x\n", sRet);
        return -1;
    }

    return 0;
}

int CHiVpss::HiVpss_get_frame(vpss_purpose_t purpose, int chn_num, VIDEO_FRAME_INFO_S *pstVideoFrame)
{
    VPSS_GRP vpssGrp;
    VPSS_CHN vpssChn;
    VPSS_CHN_MODE_S stOrigVpssMode;
    VPSS_CHN_MODE_S stVpssMode;
    HI_U8 u8GetFrameTime = 3;
    HI_U32 chn_depth;
    HI_S32 eRet = HI_FAILURE;
    HI_U32 width;
    HI_U32 height;

    if(purpose == _VP_MAIN_STREAM_)
    {
        pgAvDeviceInfo->Adi_get_stream_size(chn_num,_AST_MAIN_VIDEO_, width, height);
    }
    else if(purpose == _VP_SUB_STREAM_)
    {
        pgAvDeviceInfo->Adi_get_stream_size(chn_num,_AST_SUB_VIDEO_, width, height);
    }
    else
    {
        pgAvDeviceInfo->Adi_get_stream_size(chn_num,_AST_SUB2_VIDEO_, width, height);
    }
    
    if( HiVpss_find_vpss( purpose, chn_num, &vpssGrp, &vpssChn ) != 0 )
    {
        _AV_KEY_INFO_(_AVD_VPSS_, "CHiVpss::HiVpss_get_frame_for_snap HiVpss_find_vpss failed\n");
        return -1;
    }
    //first change chn work mode
    memset(&stOrigVpssMode, 0, sizeof(stOrigVpssMode));
    memset(&stVpssMode, 0, sizeof(stVpssMode));


    if(0 != HI_MPI_VPSS_GetChnMode(vpssGrp, vpssChn, &stOrigVpssMode))
    {
        _AV_KEY_INFO_(_AVD_VPSS_, "HI_MPI_VPSS_GetChnMode failed! \n");
        return -1;
    }

    if(0 != HI_MPI_VPSS_GetDepth(vpssGrp, vpssChn, &chn_depth))
    {
        _AV_KEY_INFO_(_AVD_VPSS_, "HI_MPI_VPSS_GetChnMode failed! \n");
        return -1;
    }

    if((stOrigVpssMode.enChnMode != VPSS_CHN_MODE_USER) || \
        (width != stOrigVpssMode.u32Width || height != stOrigVpssMode.u32Height))
    {
        stVpssMode.enChnMode = VPSS_CHN_MODE_USER;

        if(width !=0 && height != 0)
        {
            stVpssMode.u32Width = width;
            stVpssMode.u32Height = height;
        }
        stVpssMode.enPixelFormat = pgAvDeviceInfo->Adi_get_vi_pixel_format();
        
        eRet = HI_MPI_VPSS_SetChnMode(vpssGrp,vpssChn,&stVpssMode);
        if (HI_SUCCESS != eRet)
        {
            _AV_KEY_INFO_(_AVD_VPSS_, "HI_MPI_VPSS_SetChnMode failed! errcode:%#x \n", eRet);
            return -1;
        }
    }

    if(chn_depth <1)
    {
        //secnod set depth
        if (0 != HI_MPI_VPSS_SetDepth(vpssGrp,vpssChn,1))
        {
            _AV_KEY_INFO_(_AVD_VPSS_, "HI_MPI_VPSS_SetDepth failed! \n");
            //the vpss frame for sub stream is getted frequently, so we keep user mode
            if(purpose != _VP_SUB_STREAM_)
            {
                stVpssMode.enChnMode = VPSS_CHN_MODE_AUTO;
                HI_MPI_VPSS_SetChnMode(vpssGrp,vpssChn,&stVpssMode);
            }
            return -1;
        }
    }

    while((u8GetFrameTime > 0) && (HI_SUCCESS != HI_MPI_VPSS_UserGetFrame( vpssGrp, vpssChn, pstVideoFrame )) )
    {
        --u8GetFrameTime;
        usleep(100*1000);
    }

    if(u8GetFrameTime > 0)
    {
        return 0;
    }
    else
    {
        if(purpose != _VP_SUB_STREAM_)
        {
            stVpssMode.enChnMode = VPSS_CHN_MODE_AUTO;
            HI_MPI_VPSS_SetChnMode(vpssGrp,vpssChn,&stVpssMode);
        }

        return -1;
    }
}

int CHiVpss::HiVpss_release_frame(vpss_purpose_t purpose, int chn_num, VIDEO_FRAME_INFO_S *pstVideoFrame)
{
    VPSS_GRP vpssGrp;
    VPSS_CHN vpssChn;
    VPSS_CHN_MODE_S stVpssMode;

    if( HiVpss_find_vpss( purpose, chn_num, &vpssGrp, &vpssChn ) != 0 )
    {
        _AV_KEY_INFO_(_AVD_VPSS_, "CHiVpss::HiVpss_get_frame_for_snap HiVpss_find_vpss failed\n");
        return -1;
    }

    HI_S32 sRet = HI_MPI_VPSS_UserReleaseFrame( vpssGrp, vpssChn, pstVideoFrame );

    if( sRet != HI_SUCCESS )
    {
        _AV_KEY_INFO_(_AVD_VPSS_, " HI_MPI_VPSS_UserReleaseFrame failed width 0x%08x\n", sRet);
        return -1;
    }
    
    memset(&stVpssMode, 0, sizeof(stVpssMode));
    if(0 != HI_MPI_VPSS_GetChnMode(vpssGrp, vpssChn, &stVpssMode))
    {
        _AV_KEY_INFO_(_AVD_VPSS_, "HI_MPI_VPSS_GetChnMode failed! \n");
        return -1;
    }
    
    stVpssMode.enChnMode = VPSS_CHN_MODE_AUTO;
    if (0 != HI_MPI_VPSS_SetChnMode(vpssGrp,vpssChn,&stVpssMode))
    {
        _AV_KEY_INFO_(_AVD_VPSS_, "HI_MPI_VPSS_SetChnMode failed! \n");
        return -1;
    }

    return 0;
}
#endif

#include "HiAvDevice.h"


CHiAvDevice::CHiAvDevice()
{
    m_pHi_system = NULL;

#if defined(_AV_HISILICON_VPSS_)
    m_pHi_vpss = NULL;
#endif

#if defined(_AV_HAVE_VIDEO_INPUT_)
    m_pHi_vi = NULL;
#if !defined(_AV_PLATFORM_HI3518EV200_)
    m_pAv_vda = NULL;
#if defined(_AV_PRODUCT_CLASS_IPC_)
    m_pAv_od = NULL;
#endif
#endif
#endif

#if defined(_AV_HAVE_AUDIO_INPUT_)
    m_pHi_ai = NULL;
#endif
}


CHiAvDevice::~CHiAvDevice()
{
    _AV_SAFE_DELETE_(m_pHi_system);

#if defined(_AV_HISILICON_VPSS_)
    _AV_SAFE_DELETE_(m_pHi_vpss);
#endif

#if defined(_AV_HAVE_VIDEO_INPUT_)
    _AV_SAFE_DELETE_(m_pHi_vi);
#if !defined(_AV_PLATFORM_HI3518EV200_)
    _AV_SAFE_DELETE_(m_pAv_vda);
#if defined(_AV_PRODUCT_CLASS_IPC_)
    _AV_SAFE_DELETE_(m_pAv_od);
#endif
#endif
#endif

#if defined(_AV_HAVE_AUDIO_INPUT_)
    _AV_SAFE_DELETE_(m_pHi_ai);
#endif

    _AV_SAFE_DELETE_(m_pSys_pic_manager);

#if defined(_AV_PRODUCT_CLASS_IPC_) && defined(_AV_PLATFORM_HI3516A_) && defined(_AV_SUPPORT_IVE_)  
    Ad_stop_ive();
    _AV_SAFE_DELETE_(m_pHiIve);
#endif

#if defined(_AV_HAVE_VIDEO_ENCODER_)
    _AV_SAFE_DELETE_(m_pAv_encoder);
#endif
}


int CHiAvDevice::Ad_init()
{
    /*system*/
    _AV_FATAL_((m_pHi_system = new CHiSystem) == NULL, "OUT OF MEMORY\n");
    if(m_pHi_system->HiSys_init() != 0)
    {
        DEBUG_ERROR( "CHiAvDevice::Ad_init() (m_pHi_system->HiSys_init() != NULL)\n");
        _AV_SAFE_DELETE_(m_pHi_system);
        return -1;
    }
#if !defined(_AV_PRODUCT_CLASS_IPC_)    
     /*yuv picture manager*/
    _AV_FATAL_((m_pSys_pic_manager = dynamic_cast<CAvSysPicManager*> (new CHiAvSysPicManager)) == NULL, "OUT OF MEMORY\n");
    m_pSys_pic_manager->Aspm_load_picture(_ASPT_VIDEO_LOST_);
    m_pSys_pic_manager->Aspm_load_picture(_ASPT_CUSTOMER_LOGO_);
    m_pSys_pic_manager->Aspm_load_picture(_ASPT_ACCESS_LOGO_);
#endif

    /*vpss*/
#if defined(_AV_HISILICON_VPSS_)
    _AV_FATAL_((m_pHi_vpss = new CHiVpss) == NULL, "OUT OF MEMORY\n");
#endif

    /*vi*/
#if defined(_AV_HAVE_VIDEO_INPUT_)
    _AV_FATAL_((m_pHi_vi = new CHiVi) == NULL, "OUT OF MEMORY\n");
#if defined(_AV_HISILICON_VPSS_)
    m_pHi_vpss->HiVpss_set_vi(m_pHi_vi);
#endif
#if !defined(_AV_PLATFORM_HI3518EV200_)
#if defined(_AV_PLATFORM_HI3516A_)  && defined(_AV_HISILICON_VPSS_)
    m_pAv_vda = new CHiAvVda(m_pHi_vpss);
#else
    _AV_FATAL_((m_pAv_vda = new CHiAvVda(m_pHi_vi)) == NULL, "OUT OF MEMORY\n");
#endif
#if defined(_AV_PRODUCT_CLASS_IPC_)
    m_pAv_od = new CHiAvOd(m_pHi_vpss);
#endif
#endif
#endif

    /*ai*/
#if defined(_AV_HAVE_AUDIO_INPUT_)
    _AV_FATAL_((m_pHi_ai = new CHiAi) == NULL, "OUT OF MEMORY\n");
#endif

    /*encoder*/
#if defined(_AV_HAVE_VIDEO_ENCODER_)
    CHiAvEncoder *pHi_av_encoder = new CHiAvEncoder;
    _AV_FATAL_(pHi_av_encoder == NULL, "OUT OF MEMORY\n");
#if defined(_AV_HAVE_AUDIO_INPUT_)
    pHi_av_encoder->Ae_set_ai_module(m_pHi_ai);
#endif
#if defined(_AV_HAVE_VIDEO_INPUT_)
    pHi_av_encoder->Ae_set_vi_module(m_pHi_vi);
#endif
#if defined(_AV_HISILICON_VPSS_)
    pHi_av_encoder->Ae_set_vpss_module(m_pHi_vpss);
#endif
    m_pAv_encoder = pHi_av_encoder;
#endif

    
#if defined(_AV_PLATFORM_HI3520D_) && (defined(_AV_PRODUCT_CLASS_DVR_) || defined(_AV_PRODUCT_CLASS_HDVR_)) /*for 20D vi*/
    //HI_MPI_SYS_SetReg(0x200f0004, 0x03);
    //HI_MPI_SYS_SetReg(0x200f0008, 0x03);
#endif

#ifdef _AV_PLATFORM_HI3535_
    InitInnerAcodec("/dev/acodec", AUDIO_SAMPLE_RATE_8000, 0x12);
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_) && defined(_AV_PLATFORM_HI3516A_) && defined(_AV_SUPPORT_IVE_)   
#if defined(_AV_IVE_HC_)
    m_pHiIve = new CHiIveHc(_DATA_SRC_VPSS, _DATA_TYPE_YUV_420SP, (void *)m_pHi_vpss,  (CHiAvEncoder *)m_pAv_encoder);
#elif defined(_AV_IVE_FD_)
    m_pHiIve = new CHiIveFd(_DATA_SRC_VPSS, _DATA_TYPE_YUV_420SP, (void *)m_pHi_vpss,  (CHiAvEncoder *)m_pAv_encoder);
#elif defined(_AV_IVE_LD_)
    m_pHiIve = new CHiIveLd(_DATA_SRC_VPSS, _DATA_TYPE_YUV_420SP, (void *)m_pHi_vpss,  (CHiAvEncoder *)m_pAv_encoder);    
#else
    m_pHiIve = new CHiIveBle(_DATA_SRC_VPSS, _DATA_TYPE_YUV_420SP, (void *)m_pHi_vpss,  (CHiAvEncoder *)m_pAv_encoder);
#endif
#endif

    return 0;
}


int CHiAvDevice::Ad_wait_system_ready()
{
    return 0;
}



#if defined(_AV_HAVE_VIDEO_INPUT_)
int CHiAvDevice::Ad_start_vi(av_tvsystem_t tv_system, vi_config_set_t *pSource/* = NULL*/, vi_config_set_t *pPrimary_attr/* = NULL*/, vi_config_set_t *pMinor_attr/* = NULL*/)
{
    DEBUG_MESSAGE( "CHiAvDevice::Ad_start_vi start\n");

    if(m_pHi_vi->HiVi_start_all_vi_dev() != 0)
    {
        DEBUG_ERROR( "CHiAvDevice::Ad_start_vi m_pHi_vi->HiVi_start_all_vi_dev FAIELD\n");
        return -1;
    }

    if(m_pHi_vi->HiVi_start_all_primary_vi_chn(tv_system, pSource, pPrimary_attr, pMinor_attr) != 0)
    {
        DEBUG_ERROR( "CHiAvDevice::Ad_start_vi m_pHi_vi->HiVi_start_all_primary_vi_chn FAIELD\n");
        return -1;
    }

#ifndef _AV_PRODUCT_CLASS_IPC_//3518 sdk don't support this function
    /*set usr picture to vi*/
    VIDEO_FRAME_INFO_S video_frame_info;
    m_pSys_pic_manager->Aspm_get_picture(_ASPT_VIDEO_LOST_, &video_frame_info);
#if defined(_AV_PLATFORM_HI3520D_V300_)
    DEBUG_CRITICAL("\n00cxliu,enCompressMode=%x,enVideoFormat=%x\n",video_frame_info.stVFrame.enCompressMode,video_frame_info.stVFrame.enVideoFormat);
#endif
    if (m_pHi_vi->HiVi_set_usr_picture(0, &video_frame_info) == -1)
    {
        DEBUG_ERROR( "CHiAvDevice::Ad_start_vi HiVi_set_usr_picture FAILED! \n");
        return -1;
    }  
#endif
    return 0;
}

int CHiAvDevice::Ad_start_selected_vi(av_tvsystem_t tv_system, unsigned long long int chn_mask, vi_config_set_t *pSource, vi_config_set_t *pPrimary_attr, vi_config_set_t *pMinor_attr)
{
    DEBUG_ERROR( "CHiAvDevice::Ad_start_selected_vi start\n");

    if(m_pHi_vi->HiVi_start_selected_primary_vi_chn(tv_system, chn_mask, pSource, pPrimary_attr, pMinor_attr) != 0)
    {
        DEBUG_ERROR( "CHiAvDevice::Ad_start_vi m_pHi_vi->HiVi_start_all_primary_vi_chn FAIELD\n");
        return -1;
    }
    
#ifndef _AV_PRODUCT_CLASS_IPC_//3518 sdk don't support this function
    /*set usr picture to vi*/
    VIDEO_FRAME_INFO_S video_frame_info;
    m_pSys_pic_manager->Aspm_get_picture(_ASPT_VIDEO_LOST_, &video_frame_info);
    if (m_pHi_vi->HiVi_set_usr_picture(0, &video_frame_info) == -1)
    {
        DEBUG_ERROR( "CHiAvDevice::Ad_start_vi HiVi_set_usr_picture FAILED! \n");
        return -1;
    }  
#endif

    return 0;
}


int CHiAvDevice::Ad_stop_vi()
{
    if(m_pHi_vi->HiVi_stop_all_primary_vi_chn() != 0)
    {
        DEBUG_ERROR( "CHiAvDevice::Ad_stop_vi m_pHi_vi->HiVi_stop_all_primary_vi_chn FAIELD\n");
        return -1;
    }
    
    if(m_pHi_vi->HiVi_stop_all_vi_dev() != 0)
    {
        DEBUG_ERROR( "CHiAvDevice::Ad_stop_vi m_pHi_vi->HiVi_stop_all_vi_dev FAIELD\n");
        return -1;
    }
    return 0;
}

int CHiAvDevice::Ad_stop_selected_vi(unsigned long long int chn_mask)
{
    if(m_pHi_vi->HiVi_stop_selected_primary_vi_chn(chn_mask) != 0)
    {
        DEBUG_ERROR( "CHiAvDevice::Ad_stop_selected_vi m_pHi_vi->HiVi_stop_all_primary_vi_chn FAIELD\n");
        return -1;
    }

    return 0;
}

int CHiAvDevice::Ad_get_vi_max_size(int phy_chn_num, int *pMax_width, int *pMax_height, av_resolution_t *pResolution)
{
    return m_pHi_vi->HiVi_get_vi_max_size(phy_chn_num, pMax_width, pMax_height, pResolution);
}


int CHiAvDevice::Ad_vi_cover(int phy_chn_num, int x, int y, int width, int height, int cover_index)
{
    if (_VS_DIGITAL_ == pgAvDeviceInfo->Adi_get_video_source(phy_chn_num))
    {
        DEBUG_ERROR( "CHiVi::Ad_vi_cover chn %d is digital!!! \n", phy_chn_num);
        return 0;
    }
    
    return m_pHi_vi->HiVi_vi_cover(phy_chn_num, x, y, width, height, cover_index);
}


int CHiAvDevice::Ad_vi_uncover(int phy_chn_num, int cover_index)
{
    if (_VS_DIGITAL_ == pgAvDeviceInfo->Adi_get_video_source(phy_chn_num))
    {
        DEBUG_ERROR( "CHiVi::Ad_vi_uncover chn %d is digital!!! \n", phy_chn_num);
        return 0;
    }
    
    return m_pHi_vi->HiVi_vi_uncover(phy_chn_num, cover_index);
}

int CHiAvDevice::Ad_vi_insert_picture(unsigned int video_loss_bmp)
{
    for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
    {
        if (_VS_DIGITAL_ == pgAvDeviceInfo->Adi_get_video_source(chn))
        {
            // DEBUG_ERROR( "CHiVi::Ad_vi_insert_picture chn %d is digital!!! \n", chn);
            continue;
        }
        m_pHi_vi->HiVi_insert_picture(chn, GCL_BIT_VAL_TEST(video_loss_bmp, chn));
    }
    /*set video lost info to encoder*/
    m_pAv_encoder->Ae_set_video_lost_flag(video_loss_bmp);

    return 0;
}
#if !defined(_AV_PLATFORM_HI3518EV200_)
int CHiAvDevice::Ad_start_vda(vda_type_e vda_type, av_tvsystem_t tv_system, int phy_video_chn, const vda_chn_param_t* pVda_chn_param)
{ 
    if (phy_video_chn != -1)
    {
        if (_VS_DIGITAL_ == pgAvDeviceInfo->Adi_get_video_source(phy_video_chn))
        {
            DEBUG_ERROR( "CHiAvDevice::Ad_start_vda(...) chn %d is digital!!! \n", phy_video_chn);
            return 0;
        }
        
        return m_pAv_vda->Av_start_vda(vda_type, tv_system, phy_video_chn, pVda_chn_param, _START_STOP_MD_BY_MESSAGE_);
    }
    else
    { 
        for (int chn = 0; chn != pgAvDeviceInfo->Adi_get_vi_chn_num(); ++chn)
        {          
            if (_VS_DIGITAL_ == pgAvDeviceInfo->Adi_get_video_source(chn))
            {
                DEBUG_ERROR( "CHiAvDevice::Ad_start_vda(...) chn %d is digital!!! \n", chn);
                continue;
            }
            m_pAv_vda->Av_start_vda(vda_type, tv_system, chn, pVda_chn_param+chn, _START_STOP_MD_BY_SWITCH_);
        }
    }

    return 0;
}

int CHiAvDevice::Ad_start_selected_vda(vda_type_e vda_type, av_tvsystem_t tv_system, int phy_video_chn, const vda_chn_param_t* pVda_chn_param)
{
    if (phy_video_chn != -1)
    {
        if (_VS_DIGITAL_ == pgAvDeviceInfo->Adi_get_video_source(phy_video_chn))
        {
            DEBUG_ERROR( "CHiAvDevice::Ad_start_vda(...) chn %d is digital!!! \n", phy_video_chn);
            return 0;
        }

        return m_pAv_vda->Av_start_vda(vda_type, tv_system, phy_video_chn, pVda_chn_param, _START_STOP_MD_BY_SWITCH_);

    }
    else
    {
        DEBUG_ERROR( "CHiAvDevice::Ad_start_selected_vda(...) chn %d invalid!!! \n", phy_video_chn);
        return -1;
    }

    return 0;
}


int CHiAvDevice::Ad_stop_vda(vda_type_e vda_type, int phy_video_chn)
{
    if (phy_video_chn != -1)
    {
        if (_VS_DIGITAL_ == pgAvDeviceInfo->Adi_get_video_source(phy_video_chn))
        {
            DEBUG_ERROR( "CHiAvDevice::Ad_start_vda(...) chn %d is digital!!! \n", phy_video_chn);
            return 0;
        }
        
        return m_pAv_vda->Av_stop_vda(vda_type, phy_video_chn, _START_STOP_MD_BY_MESSAGE_);
    }
    else
    {
        for (int chn = 0; chn != pgAvDeviceInfo->Adi_max_channel_number(); ++chn)
        {
            if (_VS_DIGITAL_ == pgAvDeviceInfo->Adi_get_video_source(chn))
            {
                DEBUG_ERROR( "CHiAvDevice::Ad_stop_vda(...) chn %d is digital!!! \n", chn);
                continue;
            }
            
            m_pAv_vda->Av_stop_vda(vda_type, chn, _START_STOP_MD_BY_SWITCH_);
        }
    }

    return 0;
}

int CHiAvDevice::Ad_stop_selected_vda(vda_type_e vda_type, int phy_video_chn)
{
    if (phy_video_chn != -1)
    {
        if (_VS_DIGITAL_ == pgAvDeviceInfo->Adi_get_video_source(phy_video_chn))
        {
            DEBUG_ERROR( "CHiAvDevice::Ad_start_vda(...) chn %d is digital!!! \n", phy_video_chn);
            return 0;
        }

        return m_pAv_vda->Av_stop_vda(vda_type, phy_video_chn, _START_STOP_MD_BY_SWITCH_);

    }
    else
    {
        DEBUG_ERROR( "CHiAvDevice::Ad_stop_selected_vda(...) chn %d is invalid!!! \n", phy_video_chn);
        return -1;
    }

    return 0;
}


int CHiAvDevice::Ad_get_vda_result(vda_type_e vda_type, unsigned int *pVda_result, int phy_video_chn/*=-1*/, unsigned char *pVda_region/*=NULL*/)
{
    if (pVda_result == NULL)
    {
        DEBUG_ERROR( "CHiAvDevice::Ad_get_vda_result parameter ERROR!!! \n");
        return -1;
    }

    for (int chn  = 0; chn != pgAvDeviceInfo->Adi_get_vi_chn_num(); ++chn)
    {
        if (_VS_DIGITAL_ == pgAvDeviceInfo->Adi_get_video_source(chn))
        {
            //DEBUG_ERROR( "CHiAvDevice::Ad_get_vda_result(...) chn %d is digital!!! \n", chn);
            continue;
        }
        
        vda_result_t vda_result;
        memset(&vda_result, 0, sizeof(vda_result_t));
        
        m_pAv_vda->Av_get_vda_result(vda_type, chn, &vda_result);
        
        if (vda_result.result)
        {
            *pVda_result |= (1L << chn);
        }

        if (chn == phy_video_chn && pVda_region != NULL)
        {
            memcpy(pVda_region, vda_result.region, sizeof(vda_result.region));
        }
    }

    return 0;
}



unsigned int CHiAvDevice::Ad_get_vda_od_alarm_num(int phy_video_chn)
{
    return m_pAv_vda->Av_get_vda_od_counter_value(phy_video_chn);
}

int CHiAvDevice::Ad_reset_vda_od_counter(int phy_video_chn)
{
    return m_pAv_vda->Av_reset_vda_od_counter(phy_video_chn);
}
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_)
#if !defined(_AV_PLATFORM_HI3518EV200_)
int CHiAvDevice::Ad_notify_framerate_changed(int phy_video_chn, int framerate)
{
    return m_pAv_vda->Av_modify_vda_attribute(phy_video_chn, 0, (void *)framerate);
}
#endif
#if defined(_AV_PLATFORM_HI3518C_)
int CHiAvDevice::Ad_set_vi_chn_capture_y_offset(int phy_video_chn, short offset)
{
    return m_pHi_vi->HiVi_set_chn_capture_coordinate_offset(phy_video_chn, 0, offset);
}
#endif

bool CHiAvDevice::Ad_get_vi_chn_interrut_cnt(int phy_video_chn, unsigned int& interrut_cnt)
{
    VI_CHN_STAT_S vi_chn_state;

    memset(&vi_chn_state, 0, sizeof(VI_CHN_STAT_S));
    m_pHi_vi->HiVi_vi_query_chn_state(phy_video_chn, vi_chn_state);

    if(vi_chn_state.bEnable == HI_FALSE)
    {
        return false;
    }
    else
    {
        interrut_cnt = vi_chn_state.u32IntCnt;
        return true;
    }
}
#if !defined(_AV_PLATFORM_HI3518EV200_)
int CHiAvDevice::Ad_start_ive_od(int phy_video_chn, unsigned char sensitivity)
{
    return m_pAv_od->Av_start_ive_od(phy_video_chn,sensitivity);
}
int CHiAvDevice::Ad_stop_ive_od(int phy_video_chn)
{
    return m_pAv_od->Av_stop_ive_od(phy_video_chn);
}
unsigned int CHiAvDevice::Ad_get_ive_od_counter(int phy_video_chn)
{
    return m_pAv_od->Av_get_vda_od_counter_value(phy_video_chn);
}
int CHiAvDevice::Ad_reset_ive_od_counter(int phy_video_chn)
{
    return m_pAv_od->Av_reset_vda_od_counter(phy_video_chn);
}
#endif
#endif
#endif

#if defined(_AV_HAVE_AUDIO_INPUT_)
int CHiAvDevice::Ad_start_ai(ai_type_e ai_type)
{
    if (m_pHi_ai->HiAi_start_ai(ai_type) != 0)
    {
        DEBUG_ERROR( "CHiAvDevice::Ad_start_ai m_pHi_ai->HiAi_start_ai FAIELD\n");
        return -1;
    }
    
    return 0;
}

int CHiAvDevice::Ad_stop_ai(ai_type_e ai_type)
{
    if (m_pHi_ai->HiAi_stop_ai(ai_type) != 0)
    {
        DEBUG_ERROR( "CHiAvDevice::Ad_stop_ai m_pHi_ai->HiAi_stop_ai FAIELD\n");
        return -1;
    }
    
    return 0;
}
#endif


#if defined(_AV_PRODUCT_CLASS_IPC_)
int CHiAvDevice::Ad_SetVideoColor( unsigned char u8Bright, unsigned char u8Hue, unsigned char u8Contrast, unsigned char u8Saturation )
{
    DEBUG_MESSAGE( "CHiAvDevice::Ad_SetVideoColor bright=%d hue=%d contrast=%d staturation=%d\n", u8Bright, u8Hue, u8Contrast, u8Saturation );
    
    return m_pHi_vi->HiVi_set_cscattr( 0, u8Bright, u8Hue, u8Contrast, u8Saturation );
}

int CHiAvDevice::Ad_SetCameraSharpen( unsigned char u8Sharpen )
{
    DEBUG_MESSAGE( "CHiAvDevice::Ad_SetCameraSharpen value=%d\n", u8Sharpen );
  
    return RM_ISP_API_SetSharpen( u8Sharpen );
}

int CHiAvDevice::Ad_SetCameraMirrorFlip( unsigned char u8IsMirror, unsigned char u8IsFlip )
{
    DEBUG_MESSAGE( "CHiAvDevice::Ad_SetCameraMirrorFlip mirror=%d flip=%d\n", u8IsMirror, u8IsFlip );
    int ret = -1;

#if defined(_AV_PLATFORM_HI3516A_)
    ret = m_pHi_vpss->HiVpss_set_mirror_flip(0, u8IsMirror, u8IsFlip);
#else
    ret = m_pHi_vi->HiVi_set_mirror_flip( 0, u8IsMirror, u8IsFlip );
#endif
    /*add by dhdong*/
    ret |= RM_ISP_API_SetIsp2AFlip(u8IsFlip);
    return ret;
}

int CHiAvDevice::Ad_SetCameraDayNightMode( unsigned char u8Mode )
{
    if(pgAvDeviceInfo->Adi_get_ircut_and_infrared_flag())
    {
        return RM_ISP_API_SetDayNightMode( u8Mode );
    }
    else
    {
        /*add by dhdong if device has no ir_cut set to day mode*/
        return RM_ISP_API_SetDayNightMode(2);
    }
    
}

int CHiAvDevice::Ad_SetCameraLampMode( unsigned char u8Mode )
{
    return RM_ISP_API_SetLampLightMode( u8Mode );
}

int CHiAvDevice::Ad_SetCameraExposure( unsigned char u8Mode, unsigned char u8Value )
{
    return RM_ISP_API_SetExposureParam( u8Mode, u8Value );
}

int CHiAvDevice::Ad_SetCameraGain( unsigned char u8Mode, unsigned char u8Value )
{
    return RM_ISP_API_SetGainParam( u8Mode, u8Value );
}

int CHiAvDevice::Ad_SetCameraOccasion( unsigned char u8Mode )
{
    return RM_ISP_API_SetOccasionMode( u8Mode );
}

int CHiAvDevice::Ad_SetCameraLightFrequency( unsigned char u8Value )
{
    DEBUG_MESSAGE( "CHiAvDevice::Ad_SetCameraLightFrequency value=%d\n", u8Value );

    return RM_ISP_API_SetAntiFlicker( u8Value );
}

int CHiAvDevice::Ad_SetCameraBackLightLevel( unsigned char u8Level )
{
    return RM_ISP_API_SetBackLightLevel( u8Level>3?3:u8Level );
}

int CHiAvDevice::Ad_SetEnvironmentLuma( unsigned int u32Luma )
{
    if( pgAvDeviceInfo->Adi_app_type() == APP_TYPE_TEST )
    {
        pgAvDeviceInfo->Adi_SetCurrentLum(u32Luma);
    }
    
    return RM_ISP_API_UpdateEnvironmentLuma( u32Luma );
}

int CHiAvDevice::Ad_GetIspControlCmd( cmd2AControl_t* pstuCmd )
{
    int iRtn = RM_ISP_API_GetIspControlCmd( pstuCmd );

#if defined(_AV_PLATFORM_HI3516A_) || defined(_AV_PLATFORM_HI3518EV200_)
    if( pstuCmd->stuControlVI.bChanged && pstuCmd->stuControlVI.bValid )
    {
     	m_pHi_vi->HiVi_vi_dev_DCI_Attr( 0, pstuCmd->stuControlVI.stuViDCIParam);  
        pstuCmd->stuControlVI.bChanged = false;
    }
#endif

    if( pstuCmd->stuControlVpss.bChanged && pstuCmd->stuControlVpss.bValid )
    {
#if defined(_AV_PLATFORM_HI3516A_)
        m_pHi_vpss->HiVpss_3DNRFilter(_VP_MAIN_STREAM_, 0, pstuCmd->stuControlVpss.stuVpss3DNRParam);
        m_pHi_vpss->HiVpss_3DNRFilter(_VP_SUB_STREAM_, 0, pstuCmd->stuControlVpss.stuVpss3DNRParam);
        m_pHi_vpss->HiVpss_3DNRFilter(_VP_SUB2_STREAM_, 0, pstuCmd->stuControlVpss.stuVpss3DNRParam);
#if defined(_AV_SUPPORT_IVE_)
        m_pHi_vpss->HiVpss_3DNRFilter(_VP_SPEC_USE_, 0, pstuCmd->stuControlVpss.stuVpss3DNRParam);
#endif
        pstuCmd->stuControlVpss.bChanged = false;
#elif defined(_AV_PLATFORM_HI3518EV200_)
        m_pHi_vpss->HiVpss_3DNRFilter(_VP_MAIN_STREAM_, 0, pstuCmd->stuControlVpss.stuGlbVpss3DNRParamV1);
        m_pHi_vpss->HiVpss_3DNRFilter(_VP_SUB_STREAM_, 0, pstuCmd->stuControlVpss.stuGlbVpss3DNRParamV1);
        //m_pHi_vpss->HiVpss_3DNRFilter(_VP_SUB2_STREAM_, 0, pstuCmd->stuControlVpss.stuVpss3DNRParam);        
#else
        m_pHi_vpss->HiVpss_3DNoiseFilter( _VP_MAIN_STREAM_, 0, pstuCmd->stuControlVpss.u32Sf, pstuCmd->stuControlVpss.u32Tf, pstuCmd->stuControlVpss.u32ChromaRg );
        m_pHi_vpss->HiVpss_3DNoiseFilter( _VP_SUB_STREAM_, 0, pstuCmd->stuControlVpss.u32Sf, pstuCmd->stuControlVpss.u32Tf, pstuCmd->stuControlVpss.u32ChromaRg );
        m_pHi_vpss->HiVpss_3DNoiseFilter( _VP_SUB2_STREAM_, 0, pstuCmd->stuControlVpss.u32Sf, pstuCmd->stuControlVpss.u32Tf, pstuCmd->stuControlVpss.u32ChromaRg );
#endif
    }

    return iRtn;
}
int CHiAvDevice::Ad_SetISPAEAttrStep(unsigned char u8StepModule)
{
    return RM_ISP_API_SetIspAEAttrStep(u8StepModule);
}

int CHiAvDevice::Ad_SetColorGrayMode( unsigned char u8ColorMode )
{
    //0 color mode, let wb auto
  //  m_pHi_Isp->HiIspSetWbType( u8ColorMode );
    return RM_ISP_API_ColorGrayMode( (u8ColorMode==0)?true:false );
    // because of night mode not bright enough
  //  return m_pHi_vi->Hivi_set_color_gray_mode( 0, u8ColorMode );
}

int CHiAvDevice::Ad_SetDayNightCutLuma( unsigned int u32Mask, unsigned int u32DayToNightValue, unsigned int u32NightToDayValue )
{
    return RM_ISP_API_SetDayNightCutLuma( u32Mask, u32DayToNightValue, u32NightToDayValue );
}

int CHiAvDevice::Ad_SetLdcParam( unsigned char bEnable )
{
    CameraLDCAttr_t stuLdcParam;
    memset( &stuLdcParam, 0, sizeof(stuLdcParam) );
    if( RM_ISP_API_GetLDCParam( &stuLdcParam ) != 0 || stuLdcParam.bEnable == false )
    {
         DEBUG_ERROR( "CHiAvDevice::Ad_SetLdcParam not support\n" );
        return -1;
    }
#if defined(_AV_PLATFORM_HI3516A_)
    //if the focus len >= 6.0mm, the LDC is disable
    if(pgAvDeviceInfo->Adi_get_focus_length() >= 5)
    {
    	stuLdcParam.bEnable = HI_FALSE;
    }
    if(pgAvDeviceInfo->Adi_get_vi_vpss_mode())
    {
        VPSS_LDC_ATTR_S ldc_attr;

        memset(&ldc_attr, 0, sizeof(ldc_attr));
        ldc_attr.bEnable = stuLdcParam.bEnable?HI_TRUE:HI_FALSE;
        ldc_attr.stAttr.enViewType = (LDC_VIEW_TYPE_E)stuLdcParam.eViewType;
        ldc_attr.stAttr.s32CenterXOffset = stuLdcParam.u32CenterXOffset;
        ldc_attr.stAttr.s32CenterYOffset = stuLdcParam.u32CenterYOffSet;
        if( stuLdcParam.bVariableRatio ) // this type of machine ,ui config ratio value
        {
            ldc_attr.stAttr.s32Ratio = bEnable*2;
            if( ldc_attr.stAttr.s32Ratio > 511 )
            {
                ldc_attr.stAttr.s32Ratio = 511;
            }
        }
        else
        {
            ldc_attr.stAttr.s32Ratio = stuLdcParam.u32Ratio;
        }
        DEBUG_MESSAGE( "CHiAvDevice::Ad_SetLdcParam ratio=%d enable=%d\n", ldc_attr.stAttr.s32Ratio,ldc_attr.bEnable);
        
        return m_pHi_vpss->HiVpss_set_ldc_attr( 0, &ldc_attr );
    }
#endif

    VI_LDC_ATTR_S stuAttr;
    memset( &stuAttr, 0, sizeof(stuAttr) );
    stuAttr.bEnable = stuLdcParam.bEnable?HI_TRUE:HI_FALSE;
    stuAttr.stAttr.enViewType = (LDC_VIEW_TYPE_E)stuLdcParam.eViewType;
    stuAttr.stAttr.s32CenterXOffset = stuLdcParam.u32CenterXOffset;
    stuAttr.stAttr.s32CenterYOffset = stuLdcParam.u32CenterYOffSet;
    if( stuLdcParam.bVariableRatio ) // this type of machine ,ui config ratio value
    {
        stuAttr.stAttr.s32Ratio = bEnable*2;
        if( stuAttr.stAttr.s32Ratio > 511 )
        {
            stuAttr.stAttr.s32Ratio = 511;
        }
    }
    else
    {
        stuAttr.stAttr.s32Ratio = stuLdcParam.u32Ratio;
    }

    DEBUG_MESSAGE( "CHiAvDevice::Ad_SetLdcParam ratio=%d enable=%d\n", stuAttr.stAttr.s32Ratio, stuAttr.bEnable);
    
    return m_pHi_vi->Hivi_set_ldc_attr( 0, &stuAttr );

}

int CHiAvDevice::Ad_SetWdrParam(unsigned char u8WdrState, unsigned char u8Value )
{
    if( u8WdrState >= 4 )
    {
        DEBUG_ERROR( "CHiAvDevice::Ad_SetWdrParam parameter error while set wdr param param=%d\n", u8WdrState);
        u8WdrState = 0;
    }
    
    WDRParam_t stuParam;
    memset( &stuParam, 0, sizeof(stuParam) );
    stuParam.u8Enable = u8WdrState;
    stuParam.u8Value = u8Value;

    DEBUG_MESSAGE( "CHiAvDevice::Ad_SetWdrParam switch=%d value=%d\n", stuParam.u8Enable, stuParam.u8Value);
    
    return RM_ISP_API_SetExParam( EXP_OPEN_WDR, (char*)&stuParam );
}

//! Added for IPC transplant, Nov 2014
int CHiAvDevice::Ad_SetHeightLightCompensation(unsigned char u8Value)
{
    _AV_DEBUG_INFO_(_AVD_DEVICE_, "CHiAvDevice::Ad_SetHeightLightCompensation value=%d\n", u8Value);
    return RM_ISP_API_SetExParam( EXP_HEIGHT_LIGHT_COMPENSATION, (char *)&u8Value);
}

int CHiAvDevice::Ad_SetIspFrameRate(unsigned char u8FrameRate)
{
     cmd2AControl_t control;

     memset(&control, 0, sizeof(cmd2AControl_t));  
     RM_ISP_API_GetIspControlCmd(&control);

     _AV_DEBUG_INFO_(_AVD_DEVICE_, "CHiAvDevice::Ad_SetIspFrameRate frameRate=%d, isp current framerate:%d \n", u8FrameRate, control.u8FrameSlowRate);

     if(u8FrameRate != control.u8FrameSlowRate)
     {
        return RM_ISP_API_SetIspFrameRate( u8FrameRate );   
     }
     
    return 0;
}

int CHiAvDevice::Ad_SetApplicationScenario(unsigned char u8AppScenario)
{
    return RM_ISP_API_SetAppScenario(u8AppScenario);
}

#if defined(_AV_PLATFORM_HI3516A_)
int CHiAvDevice::Ad_GetWdrParam(WDRParam_t &stuWdrParam)
{
    return RM_ISP_API_GetWDRParam(&stuWdrParam);
}
#endif
#endif //end #if defined(_AV_PRODUCT_CLASS_IPC_)

#ifdef _AV_PLATFORM_HI3535_
#include "acodec.h"
#include <sys/ioctl.h>
#include <fcntl.h>
int CHiAvDevice::InitInnerAcodec(const char *AcodecFile, AUDIO_SAMPLE_RATE_E enSample, unsigned char u8Volum)
{
    int retval = 0;
    HI_S32 fdAcodec = -1;
    unsigned int i2s_fs_sel = 0;
    unsigned int gain_mic = 0;
    unsigned int adc_hpf = 0;
    unsigned int dac_deemphasis = 0;

    if(NULL == AcodecFile)
    {
        DEBUG_ERROR( "init inner acodec, but acodec file is null!\n");
        retval = -1;
        goto INIT_OVER;
    }
    DEBUG_MESSAGE( "AcodecFile[%s],sample[%d],volum[0x%02x]!\n", AcodecFile, enSample, u8Volum);
    fdAcodec = open(AcodecFile, O_RDWR);
    if (fdAcodec < 0)
    {
        DEBUG_ERROR( "can't open Acodec,%s\n", AcodecFile);
        retval = -1;
        goto INIT_OVER;
    }
    if(ioctl(fdAcodec, ACODEC_SOFT_RESET_CTRL))
    {
        DEBUG_ERROR( "reset audio codec error!\n");
    }

     if((AUDIO_SAMPLE_RATE_8000 == enSample) || (AUDIO_SAMPLE_RATE_11025 == enSample) || (AUDIO_SAMPLE_RATE_12000 == enSample)) 
    {
        i2s_fs_sel = 0x18;
    } 
    else if((AUDIO_SAMPLE_RATE_16000 == enSample) || (AUDIO_SAMPLE_RATE_22050 == enSample) || (AUDIO_SAMPLE_RATE_24000 == enSample)) 
    {
        i2s_fs_sel = 0x19;
    } 
    else if((AUDIO_SAMPLE_RATE_32000 == enSample) || (AUDIO_SAMPLE_RATE_44100 == enSample) || (AUDIO_SAMPLE_RATE_48000 == enSample)) 
    {
        i2s_fs_sel = 0x1a;
    } 
    else
    {
        DEBUG_ERROR( "not support enSample:%d\n", enSample);
        retval = -1;
        goto INIT_OVER;
    }
    if(ioctl(fdAcodec, ACODEC_SET_I2S1_FS, &i2s_fs_sel)) 
    {
        DEBUG_ERROR( "set acodec sample rate failed\n");
        retval = -1;
        goto INIT_OVER;
    }
    
    gain_mic = u8Volum;
    if(ioctl(fdAcodec, ACODEC_SET_GAIN_MICL, &gain_mic))
    {
        DEBUG_ERROR( "set acodec micin volume failed\n");
        retval = -1;
        goto INIT_OVER;
    }
    if(ioctl(fdAcodec, ACODEC_SET_GAIN_MICR, &gain_mic))
    {
        DEBUG_ERROR( "set acodec micin volume failed\n");
        retval = -1;
        goto INIT_OVER;
    }

    adc_hpf = 0x1; 
    if(ioctl(fdAcodec, ACODEC_SET_ADC_HP_FILTER, &adc_hpf)) 
    { 
        DEBUG_ERROR( "ACODEC_SET_ADC_HP_FILTER failed!\n");
    }

    dac_deemphasis = 0x1;
    if(ioctl(fdAcodec, ACODEC_SET_DAC_DE_EMPHASIS, &dac_deemphasis)) 
    { 
        DEBUG_ERROR( "ACODEC_SET_DAC_DE_EMPHASIS failed!\n");
    }
    
INIT_OVER:
    if(fdAcodec >= 0)
    {
        close(fdAcodec);
    }
    
    return retval;
}
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_) && defined(_AV_PLATFORM_HI3516A_) && defined(_AV_SUPPORT_IVE_)  
int CHiAvDevice::Ad_start_ive()
{
    if(NULL != m_pHiIve)
   {
        return m_pHiIve->ive_start();
   }

    return -1;
}

int CHiAvDevice::Ad_stop_ive()
{
    if(NULL != m_pHiIve)
   {
        return m_pHiIve->ive_stop();
   }

    return -1;
}

int CHiAvDevice::Ad_set_ive_osd(char * osd)
{
   return m_pHiIve->ive_control(IVE_TYPE_BLE |IVE_TYPE_LD, IVE_CTL_OVERLAY_OSD, (void *)osd);
}

int CHiAvDevice::Ad_update_blacklist()
{
    return m_pHiIve->ive_control(IVE_TYPE_BLE, IVE_CTL_UPDATE_BLACKLIST, NULL);
}

int CHiAvDevice::Ad_notify_bus_front_door_state(int state)
{
    return m_pHiIve->ive_control(IVE_TYPE_HC | IVE_TYPE_FD, IVE_CTL_SAVE_RESULT, (void *)state);
}

int CHiAvDevice::Ad_register_ive_detection_result_notify_callback(boost::function< void (msgIPCIntelligentInfoNotice_t *)> notify_func)
{
    m_pHiIve->ive_register_result_notify_cb(notify_func);
    return 0;
}
int CHiAvDevice::Ad_unregister_ive_detection_result_notify_callback()
{
    m_pHiIve->ive_unregister_result_notify_cb();
    return 0;
}

#endif


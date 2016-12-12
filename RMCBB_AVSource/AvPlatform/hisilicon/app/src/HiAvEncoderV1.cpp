#include "HiAvEncoder.h"

CHiAvEncoder::CHiAvEncoder()
{
#if defined(_AV_HAVE_VIDEO_INPUT_)
    m_pHi_vi = NULL;
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_)
//Added for IPC transplant Nov 2014
#if defined(_AV_HAVE_AUDIO_INPUT_)
    m_pAudio_thread_info = NULL;
#endif
#endif

#if defined(_AV_HISILICON_VPSS_)
    m_pHi_vpss = NULL;
#endif

    gettimeofday(&m_sync_pts_time, NULL);

    m_bSnapEncoderInited = false;

    for(unsigned int i=0;i<(sizeof(m_stuOsdTemp)/sizeof(m_stuOsdTemp[0]));++i)
    {
        m_stuOsdTemp[i].bInited = false;
        m_stuOsdTemp[i].pszBitmap = NULL;
    }
#if defined(_AV_PRODUCT_CLASS_IPC_) &&defined(_AV_SUPPORT_IVE_)
    for(unsigned int i=0 ; i<sizeof(m_stIve_osd_region)/sizeof(m_stIve_osd_region[0]);++i)
    {
        memset(&m_stIve_osd_region[i], 0, sizeof(hi_ive_snap_osd_info_s));
    }
#endif

}

CHiAvEncoder::~CHiAvEncoder()
{
#if defined(_AV_PRODUCT_CLASS_IPC_)
#if defined(_AV_HAVE_AUDIO_INPUT_)

    if(NULL != m_pAudio_thread_info)
    {
        if(m_pAudio_thread_info->thread_state)
        {
            m_pAudio_thread_info->thread_state = false;
            pthread_join(m_pAudio_thread_info->thread_id, NULL);
        }
    }
    
    _AV_SAFE_DELETE_(m_pAudio_thread_info);

#endif
#endif

}

int CHiAvEncoder::Ae_init_system_pts(unsigned long long pts)
{
    int ret_val = 0;
    
    ret_val = HI_MPI_SYS_InitPtsBase(pts);
    if(ret_val != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_init_system_pts HI_MPI_SYS_InitPtsBase FAILED(0x%08x)\n", ret_val);
        return -1;
    }

    if(1)
    {
        unsigned long long cur_pts;

        HI_MPI_SYS_GetCurPts(&cur_pts);

        DEBUG_MESSAGE( "CHiAvEncoder::Ae_init_system_pts(set:%lld)(cur:%lld)\n", pts, cur_pts);
    }

    return 0;
}

int CHiAvEncoder::Ae_ai_aenc_bind_control(av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, bool bind_flag)
{
    AENC_CHN aenc_chn;

    if(Ae_audio_encoder_allotter(audio_stream_type, phy_audio_chn_num, &aenc_chn) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_ai_aenc_bind_control Ae_audio_encoder_allotter FAILED(type:%d)(chn:%d)\n", audio_stream_type, phy_audio_chn_num);
        return -1;
    }

    AUDIO_DEV ai_dev = 0;
    AI_CHN ai_chn = 0;

    /*get these information from AUDIO module*/
    if(audio_stream_type == _AAST_NORMAL_)
    {
        m_pHi_ai->HiAi_get_ai_info(_AI_NORMAL_, phy_audio_chn_num, &ai_dev, &ai_chn);
    }
    else if(audio_stream_type == _AAST_TALKBACK_)
    {
        m_pHi_ai->HiAi_get_ai_info(_AI_TALKBACK_, phy_audio_chn_num, &ai_dev, &ai_chn);
    }
    else
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_ai_aenc_bind_control invalid type(type:%d)(chn:%d)\n", audio_stream_type, phy_audio_chn_num);
        return -1;
    }

    MPP_CHN_S mpp_chn_src;
    MPP_CHN_S mpp_chn_dst;

    mpp_chn_src.enModId = HI_ID_AI;
    mpp_chn_src.s32DevId = ai_dev;
    mpp_chn_src.s32ChnId = ai_chn;
    mpp_chn_dst.enModId = HI_ID_AENC;
    mpp_chn_dst.s32DevId = 0;
    mpp_chn_dst.s32ChnId = aenc_chn;

    HI_S32 ret_val = -1;
    if(bind_flag == true)
    {
        if((ret_val = HI_MPI_SYS_Bind(&mpp_chn_src, &mpp_chn_dst)) != HI_SUCCESS)
        {
            DEBUG_ERROR( "CHiAvEncoder::Ae_ai_aenc_bind_control HI_MPI_SYS_Bind FAILED(type:%d)(chn:%d)(ret_val:0x%08lx)\n", audio_stream_type, phy_audio_chn_num, (unsigned long)ret_val);
            return -1;
        }
    }
    else
    {
        if((ret_val = HI_MPI_SYS_UnBind(&mpp_chn_src, &mpp_chn_dst)) != HI_SUCCESS)
        {
            DEBUG_ERROR( "CHiAvEncoder::Ae_ai_aenc_bind_control HI_MPI_SYS_UnBind FAILED(type:%d)(chn:%d)(ret_val:0x%08lx)\n", audio_stream_type, phy_audio_chn_num, (unsigned long)ret_val);
            return -1;
        }
    }

    return 0;
}

int CHiAvEncoder::Ae_destory_audio_encoder(av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, audio_encoder_id_t *pAudio_encoder_id)
{
    AENC_CHN aenc_chn;

    //!Modified for IPC transplant which employs VQE function, on Nov 2014
    if (Ae_ai_aenc_bind_control(audio_stream_type, phy_audio_chn_num, false) != 0)//!<Unbind
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destory_audio_encoder Ae_ai_aenc_bind_control FAILED(type:%d)(chn:%d)\n", audio_stream_type, phy_audio_chn_num);
        return -1;
    }


    if(Ae_audio_encoder_allotter(audio_stream_type, phy_audio_chn_num, &aenc_chn) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destory_audio_encoder Ae_audio_encoder_allotter FAILED(type:%d)(chn:%d)\n", audio_stream_type, phy_audio_chn_num);
        return -1;
    }

    HI_S32 ret_val = -1;
    if((ret_val = HI_MPI_AENC_DestroyChn(aenc_chn)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destory_audio_encoder HI_MPI_AENC_DestroyChn FAILED(type:%d)(chn:%d)(ret_val:0x%08lx)\n", audio_stream_type, phy_audio_chn_num, (unsigned long)ret_val);
        return -1;
    }

    pAudio_encoder_id->audio_encoder_fd = -1;
    if(pAudio_encoder_id->pAudio_encoder_info != NULL)
    {
        delete (hi_audio_encoder_info_t *)pAudio_encoder_id->pAudio_encoder_info;
        pAudio_encoder_id->pAudio_encoder_info = NULL;
    }

    return 0;
}


int CHiAvEncoder::Ae_create_audio_encoder(av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, av_audio_encoder_para_t *pAudio_para, audio_encoder_id_t *pAudio_encoder_id)
{
    AENC_CHN aenc_chn;

    if(Ae_audio_encoder_allotter(audio_stream_type, phy_audio_chn_num, &aenc_chn) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_audio_encoder Ae_audio_encoder_allotter FAILED(type:%d)(chn:%d)\n", audio_stream_type, phy_audio_chn_num);
        return -1;
    }

    AENC_CHN_ATTR_S aenc_chn_attr;
    AENC_ATTR_ADPCM_S aenc_attr_adpcm;
    AENC_ATTR_G726_S aenc_attr_g726;
    AENC_ATTR_G711_S aenc_attr_g711;
    HI_S32 ret_val = -1;

    memset(&aenc_chn_attr, 0, sizeof(AENC_CHN_ATTR_S));
    switch(pAudio_para->type)
    {
        case _AET_ADPCM_:
            aenc_chn_attr.enType = PT_ADPCMA;
            aenc_chn_attr.u32BufSize = 40;
            aenc_chn_attr.pValue = &aenc_attr_adpcm;
            aenc_attr_adpcm.enADPCMType = ADPCM_TYPE_DVI4;
            break;
        case _AET_G711A_:
            aenc_chn_attr.enType = PT_G711A;
            aenc_chn_attr.u32BufSize = 30;
            aenc_chn_attr.pValue = &aenc_attr_g711;
            break;
        case _AET_G711U_:
            aenc_chn_attr.enType = PT_G711U;
            aenc_chn_attr.u32BufSize = 30;
            aenc_chn_attr.pValue = &aenc_attr_g711;
            break;
        case _AET_G726_:
            aenc_chn_attr.enType = PT_G726;
            aenc_chn_attr.u32BufSize = 30;
            aenc_chn_attr.pValue = &aenc_attr_g726;
            aenc_attr_g726.enG726bps = MEDIA_G726_40K;
            break;
        default:
            _AV_FATAL_(1, "CHiAvEncoder::Ae_create_audio_encoder You must give the implement\n");
            break;
    }
#if defined(_AV_PLATFORM_HI3516A_) || defined(_AV_PLATFORM_HI3518EV200_)
    aenc_chn_attr.u32PtNumPerFrm = _AV_AUDIO_PTNUMPERFRAME_;
#endif
    if((ret_val = HI_MPI_AENC_CreateChn(aenc_chn, &aenc_chn_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_audio_encoder HI_MPI_AENC_CreateChn FAILED(type:%d)(chn:%d)(ret_val:0x%08lx)\n", audio_stream_type, phy_audio_chn_num, (unsigned long)ret_val);
        return -1;
    }

    if(Ae_ai_aenc_bind_control(audio_stream_type, phy_audio_chn_num, true) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_audio_encoder Ae_ai_aenc_bind_control FAILED(type:%d)(chn:%d)\n", audio_stream_type, phy_audio_chn_num);
        return -1;
    }

    memset(pAudio_encoder_id, 0, sizeof(audio_encoder_id_t));
    pAudio_encoder_id->audio_encoder_fd = HI_MPI_AENC_GetFd(aenc_chn);    
    hi_audio_encoder_info_t *pAei = new hi_audio_encoder_info_t;
    _AV_FATAL_(pAei == NULL, "CHiAvEncoder::Ae_create_audio_encoder OUT OF MEMORY\n");
    memset(pAei, 0, sizeof(hi_audio_encoder_info_t));
    pAei->aenc_chn = aenc_chn;
    pAudio_encoder_id->pAudio_encoder_info = (void *)pAei;

//!Added by dfz for IPC transplant on Nov 2014
#if 0
/*modifoed by dhdong because 3516A don't use inner codec*/ 
#if defined(_AV_PRODUCT_CLASS_IPC_) && defined(_AV_HAVE_AUDIO_INPUT_)
    sleep(1);
    m_pHi_ai->HiAi_config_acodec_volume();  //!< Set default value of analog gain of  audio input
#endif
#endif
    return 0;
}

int CHiAvEncoder::Ae_create_video_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, video_encoder_id_t *pVideo_encoder_id)
{
    if(NULL == pVideo_para)
    {
        DEBUG_ERROR( "[%s %d]create video encoder failed!the video para is NULL! \n", __FUNCTION__, __LINE__);
    }

    switch(pVideo_para->video_type )
    {
        case _AVT_H264_:
        default:
            return Ae_create_h264_encoder(video_stream_type, phy_video_chn_num, pVideo_para, pVideo_encoder_id);
            break;
        case _AVT_H265_:
            return Ae_create_h265_encoder(video_stream_type, phy_video_chn_num, pVideo_para, pVideo_encoder_id);
            break;    
    }
}


int CHiAvEncoder::Ae_destory_video_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, video_encoder_id_t *pVideo_encoder_id)
{
    VENC_CHN venc_chn = -1;
#if defined(_AV_HISILICON_VPSS_)
#if defined(_AV_SUPPORT_IVE_)
    if(1 == pgAvDeviceInfo->Adi_get_ive_debug_mode())
    {
#if defined(_AV_IVE_FD_)
        if( video_stream_type != _AST_MAIN_VIDEO_)
#else
        if( video_stream_type != _AST_SUB_VIDEO_)
#endif
        {
            if(Ae_venc_vpss_control(video_stream_type, phy_video_chn_num, false) != 0)
            {
                DEBUG_ERROR( "CHiAvEncoder::Ae_destory_video_encoder Ae_venc_vpss_control FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
                return -1;
            }
        }
    }
    else
    {
        if(Ae_venc_vpss_control(video_stream_type, phy_video_chn_num, false) != 0)
        {
            DEBUG_ERROR( "CHiAvEncoder::Ae_destory_video_encoder Ae_venc_vpss_control FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
            return -1;
        }
    }
#else
    if(Ae_venc_vpss_control(video_stream_type, phy_video_chn_num, false) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destory_video_encoder Ae_venc_vpss_control FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
        return -1;
    }
#endif
#endif

    if(Ae_video_encoder_allotter(video_stream_type, phy_video_chn_num, &venc_chn) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destory_video_encoder Ae_video_encoder_allotter FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
        return -1;
    }

    HI_S32 ret_val = -1;

    if((ret_val = HI_MPI_VENC_StopRecvPic(venc_chn)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destory_video_encoder HI_MPI_VENC_StopRecvPic FAILED(type:%d)(chn:%d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, (unsigned long)ret_val);
        return -1;
    }
#if  defined(_AV_PLATFORM_HI3516A_)
    if(HI_SUCCESS != HI_MPI_VENC_CloseFd(venc_chn))
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destory_video_encoder HI_MPI_VENC_CloseFd FAILED\n");        
    }        
#endif

    
    if((ret_val = HI_MPI_VENC_DestroyChn(venc_chn)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destory_video_encoder HI_MPI_VENC_DestroyChn FAILED(type:%d)(chn:%d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, (unsigned long)ret_val);
        return -1;
    }


    hi_video_encoder_info_t *pVei = (hi_video_encoder_info_t *)pVideo_encoder_id->pVideo_encoder_info;
    if(pVei != NULL)
    {
        _AV_SAFE_DELETE_ARRAY_(pVei->pPack_buffer);
        delete pVei;
        pVideo_encoder_id->pVideo_encoder_info = NULL;
    }

    return 0;
}

//N9M
int CHiAvEncoder::Ae_create_snap_encoder( av_resolution_t eResolution, av_tvsystem_t eTvSystem, unsigned int factor, hi_snap_osd_t pSnapOsd[6], int osdNum)
{
    if( m_bSnapEncoderInited )
    {
        return 0;
    }
    
    VENC_CHN snapChl;
    
    if( Ae_video_encoder_allotter_snap( &snapChl) != 0 )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder Ae_video_encoder_allotter_snap FAILED\n");
        return -1;
    }

    HI_S32 sRet = HI_SUCCESS;

    int iWidth, iHeight;
    pgAvDeviceInfo->Adi_get_video_size( eResolution, eTvSystem, &iWidth, &iHeight);
    pgAvDeviceInfo->Adi_covert_vidoe_size_by_rotate( &iWidth, &iHeight);

    VENC_CHN_ATTR_S stuVencChnAttr;
    VENC_ATTR_JPEG_S stuJpegAttr;
    stuVencChnAttr.stVeAttr.enType = PT_JPEG;
    stuJpegAttr.u32MaxPicWidth  = (iWidth + 3) / 4 * 4;
    stuJpegAttr.u32MaxPicHeight = (iHeight + 3)/ 4 * 4;
    stuJpegAttr.u32PicWidth  = stuJpegAttr.u32MaxPicWidth;
    stuJpegAttr.u32PicHeight = stuJpegAttr.u32MaxPicHeight;
    stuJpegAttr.u32BufSize = (stuJpegAttr.u32MaxPicWidth * stuJpegAttr.u32MaxPicHeight*2 + 63) /64 * 64;
    stuJpegAttr.bByFrame = HI_TRUE;/*get stream mode is field mode  or frame mode*/
    stuJpegAttr.bSupportDCF = HI_FALSE;
    memcpy( &stuVencChnAttr.stVeAttr.stAttrJpeg, &stuJpegAttr, sizeof(VENC_ATTR_JPEG_S) );

    sRet = HI_MPI_VENC_CreateChn( snapChl, &stuVencChnAttr );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder HI_MPI_VENC_CreateChn err:0x%08x\n", sRet);
        return -1;
    }
	 
    /* 设置JPEG图像质量 */
    VENC_PARAM_JPEG_S pstJpegParam;
    sRet = HI_MPI_VENC_GetJpegParam(snapChl, &pstJpegParam);
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder HI_MPI_VENC_GetJpegParam err:0x%08x\n", sRet);
        return -1;
    }
    pstJpegParam.u32Qfactor = factor;
    sRet = HI_MPI_VENC_SetJpegParam(snapChl, &pstJpegParam);
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder HI_MPI_VENC_GetJpegParam err:0x%08x\n", sRet);
        return -1;
    }
	
    sRet = HI_MPI_VENC_StartRecvPic( snapChl );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder HI_MPI_VENC_StartRecvPic err:0x%08x\n", sRet);
        return -1;
    }
	
    for(int i=0; i<osdNum; i++)
    {
#if defined(_AV_PRODUCT_CLASS_IPC_)
        if(pSnapOsd[i].bShow)
        {
            this->Ae_snap_regin_create( snapChl, eResolution, eTvSystem, i, pSnapOsd[i]);
        }
#else
        if(strlen(pSnapOsd[i].szStr) != 0)
        {
            this->Ae_snap_regin_create( snapGrp, eResolution, eTvSystem, i, pSnapOsd);
        }
#endif
    }

    m_bSnapEncoderInited = true;
    
    return 0;
}
#if defined(_AV_PRODUCT_CLASS_IPC_)
int CHiAvEncoder::Ae_destroy_snap_encoder()
{
    if( !m_bSnapEncoderInited )
    {
        return 0;
    }
    
    VENC_CHN snapChl;
    
    if( Ae_video_encoder_allotter_snap(&snapChl) != 0 )
    {
        _AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_destroy_snap_encoder Ae_video_encoder_allotter_snap FAILED\n");
        return -1;
    }
    
    for(unsigned int i=0; i< (sizeof(m_stuOsdTemp)/sizeof(m_stuOsdTemp[0])); ++i)
    {
        if(m_stuOsdTemp[i].bInited)
        {
            this->Ae_snap_regin_delete(i);
        }
    }

    HI_S32 sRet = HI_SUCCESS;

    sRet = HI_MPI_VENC_StopRecvPic( snapChl );
    if( sRet != HI_SUCCESS )
    {
        _AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_destroy_snap_encoder HI_MPI_VENC_StartRecvPic err:0x%08x\n", sRet);
        return -1;
    }

    sRet = HI_MPI_VENC_DestroyChn( snapChl );
    if( sRet != HI_SUCCESS )
    {
        _AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_destroy_snap_encoder HI_MPI_VENC_DestroyChn err:0x%08x\n", sRet);
    }

    m_bSnapEncoderInited = false;
    
    return 0;
}

#else
int CHiAvEncoder::Ae_destroy_snap_encoder(hi_snap_osd_t *stuOsdInfo)
{
    if( !m_bSnapEncoderInited )
    {
        return 0;
    }
    
    VENC_CHN snapChl;
    
    if( Ae_video_encoder_allotter_snap(&snapChl) != 0 )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destroy_snap_encoder Ae_video_encoder_allotter_snap FAILED\n");
        return -1;
    }
	
	
	for(int i=0; i<6; i++)
	{
		if(stuOsdInfo[i].bShow == 1 && strlen(stuOsdInfo[i].szStr) != 0)
		{
            //DEBUG_MESSAGE("Ae_snap_regin_delete index=%d\n", i);
            this->Ae_snap_regin_delete( i );
		}
	}
	
    //this->Ae_snap_regin_delete( 1 );

    HI_S32 sRet = HI_SUCCESS;

    sRet = HI_MPI_VENC_StopRecvPic( snapChl );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destroy_snap_encoder HI_MPI_VENC_StartRecvPic err:0x%08x\n", sRet);
        return -1;
    }

    sRet = HI_MPI_VENC_UnRegisterChn( snapChl );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destroy_snap_encoder HI_MPI_VENC_UnRegisterChn err:0x%08x\n", sRet);
    }

    sRet = HI_MPI_VENC_DestroyChn( snapChl );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destroy_snap_encoder HI_MPI_VENC_DestroyChn err:0x%08x\n", sRet);
    }

    sRet = HI_MPI_VENC_DestroyGroup( snapGrp );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destroy_snap_encoder HI_MPI_VENC_DestroyGroup err:0x%08x\n", sRet);
    }

    m_bSnapEncoderInited = false;
    
    return 0;
}
#endif
int CHiAvEncoder::Ae_snap_regin_set_osds( hi_snap_osd_t *stuOsdParam, int osdNum )
{
	//DEBUG_MESSAGE( "##### Ae_snap_regin_set_osds\n");
    if((NULL == stuOsdParam) || (osdNum < 0))
    {
        DEBUG_ERROR( "param is invalid,stuOsdParam(%p), osdNum(%d)\n", stuOsdParam, osdNum);   
        return -1;
    }
    //DEBUG_MESSAGE("set_osds date x= %d y= %d\n", stuOsdParam[0].x, stuOsdParam[0].y);
	//DEBUG_MESSAGE("set_osds bus number x= %d y= %d\n", stuOsdParam[1].x, stuOsdParam[1].y);
	//DEBUG_MESSAGE("set_osds speed x= %d y= %d\n", stuOsdParam[2].x, stuOsdParam[2].y);
	//DEBUG_MESSAGE("set_osds gps x= %d y= %d\n", stuOsdParam[3].x, stuOsdParam[3].y);
	//DEBUG_MESSAGE("set_osds chn name x= %d y= %d\n", stuOsdParam[4].x, stuOsdParam[4].y);
	//DEBUG_MESSAGE("set_osds self number x= %d y= %d\n", stuOsdParam[5].x, stuOsdParam[5].y);
    for(int i=0; i<osdNum; i++)
    {
       if(strlen(stuOsdParam[i].szStr) !=0)
       {
            Ae_snap_regin_set_osd( i, stuOsdParam[i].bShow, stuOsdParam[i].x, stuOsdParam[i].y, stuOsdParam[i].szStr ); 
       }
    }
   
    return 0; 
}
int CHiAvEncoder::Ae_snap_regin_set_osds( hi_snap_osd_t stuOsdParam[2] )
{
    Ae_snap_regin_set_osd( 0, stuOsdParam[0].bShow, stuOsdParam[0].x, stuOsdParam[0].y, stuOsdParam[0].szStr );
    Ae_snap_regin_set_osd( 1, stuOsdParam[1].bShow, stuOsdParam[1].x, stuOsdParam[1].y, stuOsdParam[1].szStr );
    
    return 0;
}

int CHiAvEncoder::Ae_snap_regin_set_osd( int index, bool bShow, unsigned short x, unsigned short y, char* pszStr )
{
    if( bShow && !m_stuOsdTemp[index].bInited )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_snap_regin_set_osd failed, osd not init\n");
        return -1;
    }

    MPP_CHN_S stuMppChn;
    RGN_CHN_ATTR_S stuRgnChnAttr;

    stuMppChn.enModId = HI_ID_VENC;
    stuMppChn.s32DevId = 0;
    stuMppChn.s32ChnId = m_stuOsdTemp[index].vencChn;
    
    HI_S32 sRet = HI_SUCCESS;

    if( bShow && ((sRet=HI_MPI_RGN_GetDisplayAttr(m_stuOsdTemp[index].hOsdRegin, &stuMppChn, &stuRgnChnAttr)) != HI_SUCCESS) )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_snap_regin_set_osd HI_MPI_RGN_GetDisplayAttr failed width 0x%08x\n", sRet);
        return -1;
    }

    if( !bShow )
    {
        stuRgnChnAttr.bShow = HI_FALSE;
        //if( (sRet=HI_MPI_RGN_SetDisplayAttr(m_stuOsdTemp[index].hOsdRegin, &stuMppChn, &stuRgnChnAttr)) != HI_SUCCESS )
        {
           // DEBUG_ERROR( "CHiAvEncoder::Ae_snap_regin_set_osd HI_MPI_RGN_SetDisplayAttr failed width 0x%08x\n", sRet);
           // return -1;
        }
        return 0;
    }

    stuRgnChnAttr.bShow = HI_TRUE;
    stuRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = x;
    stuRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = y;
/*
    int iPicWidth = 704;
    int iPicHeight = 576;
    pgAvDeviceInfo->Adi_get_video_size( m_stuOsdTemp[index].eResolution, _AT_UNKNOWN_, &iPicWidth, &iPicHeight );
#ifdef _AV_PLATFORM_HI3518C_
    pgAvDeviceInfo->Adi_covert_vidoe_size_by_rotate(&iPicWidth, &iPicHeight);
#endif

    if( stuRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32X + m_stuOsdTemp[index].u32BitmapWidth > iPicWidth )
    {
        stuRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = (iPicWidth - m_stuOsdTemp[index].u32BitmapWidth) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
    }
 */
    if( (sRet=HI_MPI_RGN_SetDisplayAttr(m_stuOsdTemp[index].hOsdRegin, &stuMppChn, &stuRgnChnAttr)) != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_snap_regin_set_osd HI_MPI_RGN_GetDisplayAttr failed width 0x%08x\n", sRet);
        return -1;
    }

    ///////////////////////////////////////////////////////////////////////////////
    if(Ae_utf8string_2_bmp( m_stuOsdTemp[index].eOsdName, m_stuOsdTemp[index].eResolution, m_stuOsdTemp[index].pszBitmap
        , m_stuOsdTemp[index].u32BitmapWidth, m_stuOsdTemp[index].u32BitmapHeight, pszStr, 0xffff, 0x00, 0x8000) != 0)
    {
        return -1;
    }

    BITMAP_S stuBitmap;
    stuBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
    stuBitmap.u32Width = m_stuOsdTemp[index].u32BitmapWidth;
    stuBitmap.u32Height = m_stuOsdTemp[index].u32BitmapHeight;    
    stuBitmap.pData = m_stuOsdTemp[index].pszBitmap;

    if( (sRet=HI_MPI_RGN_SetBitMap(m_stuOsdTemp[index].hOsdRegin, &stuBitmap)) != HI_SUCCESS )
    {        
        DEBUG_ERROR( "CHiAvEncoder::Ae_osd_overlay HI_MPI_RGN_SetBitMap FAILED(region_handle:0x%08x)(%d)\n", sRet, m_stuOsdTemp[index].hOsdRegin);
        return -1;
    }

    return 0;
}

#if defined(_AV_PRODUCT_CLASS_IPC_)
int CHiAvEncoder::Ae_snap_regin_create( VENC_CHN vencChn, av_resolution_t eResolution, av_tvsystem_t eTvSystem, int index, hi_snap_osd_t &pSnapOsd )     
{
    if( m_stuOsdTemp[index].bInited )
    {
        return 0;
    }

    HI_S32 sRet = HI_SUCCESS;
    RGN_ATTR_S stuRgnAttr;

    int fontName;
    int fontWidth;
    int fontHeight;
    int horScaler;
    int verScaler;
    int charactorNum = 0;

    switch(index)
    {
        case 0:
        {
            stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
            Ae_get_osd_parameter( _AON_DATE_TIME_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
            charactorNum = strlen("2013-03-23 11:10:00 AM");
            switch(eResolution)
            {
                case _AR_1080_:
                case _AR_720_:
                case _AR_960P_:
                    stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler) / 2 + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                    stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +4 ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                    break;
                default:
                    stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler)  + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                    stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +4 ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                    break;
            }
            m_stuOsdTemp[index].hOsdRegin = Har_get_region_handle_encoder(_AST_SNAP_VIDEO_, 0, _AON_DATE_TIME_, &(m_stuOsdTemp[index].hOsdRegin));
            m_stuOsdTemp[index].eOsdName = _AON_DATE_TIME_;
            
        }
            break;

        //vehcile
        case 1:
        {
            stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
            Ae_get_osd_parameter( _AON_BUS_NUMBER_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
            char default_vehicle[16];
            memset(default_vehicle, 47, sizeof(default_vehicle));
            default_vehicle[sizeof(default_vehicle)-1] = '\0';

            CAvUtf8String vehicle(default_vehicle);
            av_ucs4_t char_unicode;
            av_char_lattice_info_t lattice_char_info;
            while(true == vehicle.Next_char(&char_unicode))
            {
                if(m_pAv_font_library->Afl_get_char_lattice(fontName, char_unicode, &lattice_char_info) == 0)
                {
                    stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
                }
                else
                {
                    _AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                    return -1;
                }
            }
            if(0 == stuRgnAttr.unAttr.stOverlay.stSize.u32Width)
            {
                _AV_KEY_INFO_(_AVD_ENC_, "the osd regin(%d) width is 0 \n", index);
                return -1;
            }
            stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
            stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;  
            m_stuOsdTemp[index].hOsdRegin = Har_get_region_handle_encoder(_AST_SNAP_VIDEO_, 0, _AON_BUS_NUMBER_, &(m_stuOsdTemp[index].hOsdRegin));
            m_stuOsdTemp[index].eOsdName = _AON_BUS_NUMBER_;
        }
            break;
        //speed
        case 2:
        {
            stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
            Ae_get_osd_parameter( _AON_SPEED_INFO_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
            char default_speed[16];
            memset(default_speed, 47, sizeof(default_speed));
            default_speed[sizeof(default_speed)-1] = '\0';
        
            CAvUtf8String speed(default_speed);
            av_ucs4_t char_unicode;
            av_char_lattice_info_t lattice_char_info;
            while(true == speed.Next_char(&char_unicode))
            {
                if(m_pAv_font_library->Afl_get_char_lattice(fontName, char_unicode, &lattice_char_info) == 0)
                {
                    stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
                }
                else
                {
                    _AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                    return -1;
                }
            }
            if(0 == stuRgnAttr.unAttr.stOverlay.stSize.u32Width)
            {
                _AV_KEY_INFO_(_AVD_ENC_, "the osd regin(%d) width is 0 \n", index);
                return -1;
            }
            stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_* _AE_OSD_ALIGNMENT_BYTE_;
            stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;   
            m_stuOsdTemp[index].hOsdRegin = Har_get_region_handle_encoder(_AST_SNAP_VIDEO_, 0, _AON_SPEED_INFO_, &(m_stuOsdTemp[index].hOsdRegin));
            m_stuOsdTemp[index].eOsdName = _AON_SPEED_INFO_;
        }
            break;
        //gps
        case 3:
        {
            stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
            Ae_get_osd_parameter( _AON_GPS_INFO_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
            char default_gps[32];
            memset(default_gps, 47, sizeof(default_gps));
            default_gps[sizeof(default_gps)-1] = '\0';

            CAvUtf8String gps(default_gps);
            av_ucs4_t char_unicode;
            av_char_lattice_info_t lattice_char_info;
            while(true == gps.Next_char(&char_unicode))
            {
                if(m_pAv_font_library->Afl_get_char_lattice(fontName, char_unicode, &lattice_char_info) == 0)
                {
                    stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
                }
                else
                {
                    _AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                    return -1;
                }
            }
            if(0 == stuRgnAttr.unAttr.stOverlay.stSize.u32Width)
            {
                _AV_KEY_INFO_(_AVD_ENC_, "the osd regin(%d) width is 0 \n", index);
                return -1;
            }
            stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
            stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;    
            m_stuOsdTemp[index].hOsdRegin = Har_get_region_handle_encoder(_AST_SNAP_VIDEO_, 0, _AON_GPS_INFO_, &(m_stuOsdTemp[index].hOsdRegin));
            m_stuOsdTemp[index].eOsdName = _AON_GPS_INFO_;
        }
            break;
        //channel name
        case 4:
        {
            stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
            Ae_get_osd_parameter( _AON_CHN_NAME_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
            char *channel_name = pSnapOsd.szStr;
            CAvUtf8String chn_name(channel_name);
            av_ucs4_t char_unicode;
            av_char_lattice_info_t lattice_char_info;
            while(true == chn_name.Next_char(&char_unicode))
            {
                if(m_pAv_font_library->Afl_get_char_lattice(fontName, char_unicode, &lattice_char_info) == 0)
                {
                    stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
                }
                else
                {
                    _AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                    return -1;
                }
            }
            if(0 == stuRgnAttr.unAttr.stOverlay.stSize.u32Width)
            {
                _AV_KEY_INFO_(_AVD_ENC_, "the osd regin(%d) width is 0 \n", index);
                return -1;
            }
            if(PTD_712_VB == pgAvDeviceInfo->Adi_product_type())
            {
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + 31) / 16 * 16;
            }
            else
            {
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
            }
            stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;   
            m_stuOsdTemp[index].hOsdRegin = Har_get_region_handle_encoder(_AST_SNAP_VIDEO_, 0, _AON_CHN_NAME_, &(m_stuOsdTemp[index].hOsdRegin));
            m_stuOsdTemp[index].eOsdName = _AON_CHN_NAME_;
        }
            break;
        //device id
        case 5:
        {
            stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
            Ae_get_osd_parameter( _AON_SELFNUMBER_INFO_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
            char default_device_id[32];
            memset(default_device_id, 47, sizeof(default_device_id));
            default_device_id[sizeof(default_device_id)-1] = '\0';
        
            CAvUtf8String device_id(default_device_id);
            av_ucs4_t char_unicode;
            av_char_lattice_info_t lattice_char_info;
            while(true == device_id.Next_char(&char_unicode))
            {
                if(m_pAv_font_library->Afl_get_char_lattice(fontName, char_unicode, &lattice_char_info) == 0)
                {
                    stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
                }
                else
                {
                    _AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                    return -1;
                }
            }
            if(0 == stuRgnAttr.unAttr.stOverlay.stSize.u32Width)
            {
                _AV_KEY_INFO_(_AVD_ENC_, "the osd regin(%d) width is 0 \n", index);
                return -1;
            }
            stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
            stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;    
            m_stuOsdTemp[index].hOsdRegin = Har_get_region_handle_encoder(_AST_SNAP_VIDEO_, 0, _AON_SELFNUMBER_INFO_, &(m_stuOsdTemp[index].hOsdRegin));
            m_stuOsdTemp[index].eOsdName = _AON_SELFNUMBER_INFO_;
        }
            break;
         //user define osd 1
        case 6:
        {
            stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
            Ae_get_osd_parameter( _AON_COMMON_OSD1_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
            char *user_define_osd1 = pSnapOsd.szStr;

            CAvUtf8String osd1(user_define_osd1);
            av_ucs4_t char_unicode;
            av_char_lattice_info_t lattice_char_info;
            while(true == osd1.Next_char(&char_unicode))
            {
                if(m_pAv_font_library->Afl_get_char_lattice(fontName, char_unicode, &lattice_char_info) == 0)
                {
                    stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
                }
                else
                {
                    _AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                    return -1;
                }
            }
            if(0 == stuRgnAttr.unAttr.stOverlay.stSize.u32Width)
            {
                _AV_KEY_INFO_(_AVD_ENC_, "the osd regin(%d) width is 0 \n", index);
                return -1;
            }
            stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
            stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;     
            m_stuOsdTemp[index].hOsdRegin = Har_get_region_handle_encoder(_AST_SNAP_VIDEO_, 0, _AON_COMMON_OSD1_, &(m_stuOsdTemp[index].hOsdRegin));
            m_stuOsdTemp[index].eOsdName = _AON_COMMON_OSD1_;
        }
            break;
        //user define osd 2
        case 7:
        {
            stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
            Ae_get_osd_parameter( _AON_COMMON_OSD2_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
            char *user_define_osd2 = pSnapOsd.szStr;

            CAvUtf8String osd2(user_define_osd2);
            av_ucs4_t char_unicode;
            av_char_lattice_info_t lattice_char_info;
            while(true == osd2.Next_char(&char_unicode))
            {
                if(m_pAv_font_library->Afl_get_char_lattice(fontName, char_unicode, &lattice_char_info) == 0)
                {
                    stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
                }
                else
                {
                    _AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                    return -1;
                }
            }
            if(0 == stuRgnAttr.unAttr.stOverlay.stSize.u32Width)
            {
                _AV_KEY_INFO_(_AVD_ENC_, "the osd regin(%d) width is 0 \n", index);
                return -1;
            }
            stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
            stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;  
            m_stuOsdTemp[index].hOsdRegin = Har_get_region_handle_encoder(_AST_SNAP_VIDEO_, 0, _AON_COMMON_OSD2_, &(m_stuOsdTemp[index].hOsdRegin));
            m_stuOsdTemp[index].eOsdName = _AON_COMMON_OSD2_;
        }
            break;
        default:
        {
            _AV_KEY_INFO_(_AVD_ENC_, "sorry, we don't support this osd right now! \n");
            return -1;
        }
            break;
    }

    stuRgnAttr.enType = OVERLAY_RGN;
    stuRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
    stuRgnAttr.unAttr.stOverlay.u32BgColor = 0x00;

    if( (sRet = HI_MPI_RGN_Create(m_stuOsdTemp[index].hOsdRegin, &stuRgnAttr)) != HI_SUCCESS )
    {
        _AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_snap_create_regin HI_MPI_RGN_Create FAILED width 0x%08x, regin=%d index=%d\n", sRet, m_stuOsdTemp[index].hOsdRegin, index );
        return -1;
    }

    MPP_CHN_S stuMppChn;
    RGN_CHN_ATTR_S stuRgnChnAttr;
    memset( &stuRgnChnAttr, 0, sizeof(stuRgnChnAttr) );
    stuRgnChnAttr.bShow = HI_FALSE;
    stuRgnChnAttr.enType = OVERLAY_RGN;
    stuRgnChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 0;
    stuRgnChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128;
    stuRgnChnAttr.unChnAttr.stOverlayChn.u32Layer = 0;

    stuRgnChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
    stuRgnChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;

    stuRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = HI_FALSE;
    stuRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Width = 16;
    stuRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Height = 16;
    stuRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.u32LumThresh = 200;
    stuRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = MORETHAN_LUM_THRESH;

    stuMppChn.enModId = HI_ID_VENC;
    stuMppChn.s32DevId = 0;
    stuMppChn.s32ChnId = vencChn;

    if( (sRet=HI_MPI_RGN_AttachToChn(m_stuOsdTemp[index].hOsdRegin, &stuMppChn, &stuRgnChnAttr)) != HI_SUCCESS )
    {        
        _AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_snap_create_regin HI_MPI_RGN_AttachToChn FAILED(0x%08x)\n", sRet);

        HI_MPI_RGN_Destroy( m_stuOsdTemp[index].hOsdRegin );
        return -1;
    }

    m_stuOsdTemp[index].vencChn = vencChn;
    m_stuOsdTemp[index].eResolution = eResolution;
    /*bitmap*/
    m_stuOsdTemp[index].u32BitmapWidth = stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
    m_stuOsdTemp[index].u32BitmapHeight = stuRgnAttr.unAttr.stOverlay.stSize.u32Height;
    m_stuOsdTemp[index].pszBitmap = new unsigned char[m_stuOsdTemp[index].u32BitmapWidth * horScaler * m_stuOsdTemp[index].u32BitmapHeight * verScaler * 2];

    m_stuOsdTemp[index].bInited = true;

    return 0;
}
#else
int CHiAvEncoder::Ae_snap_regin_create( VENC_GRP vencGrp, av_resolution_t eResolution, av_tvsystem_t eTvSystem, int index, hi_snap_osd_t pSnapOsd[6] )
{
	//DEBUG_MESSAGE("#####CHiAvEncoder::Ae_snap_regin_create\n");
	if( m_stuOsdTemp[index].bInited )
    {
        return 0;
    }

    HI_S32 sRet = HI_SUCCESS;
    RGN_ATTR_S stuRgnAttr;

    int fontWidth, fontHeight;
    int horScaler, verScaler;
    int fontname = 0;
    int charactorNum = 0;

	if( (pSnapOsd[index].bShow ==1) && (strlen(pSnapOsd[index].szStr) != 0) && index == 0 )//date
    {
		stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
		Ae_get_osd_parameter( _AON_DATE_TIME_, eResolution, fontname, &fontWidth, &fontHeight, &horScaler, &verScaler );
		//DEBUG_MESSAGE("pSnapOsd[%d].szStr %s\n", index, pSnapOsd[index].szStr);
        charactorNum = strlen(pSnapOsd[index].szStr);
		//原来没有这部分代码，这样做主要为了抓拍时候叠加参数和编码时参数一样
		switch(eResolution)
        {
            case _AR_1080_:
            case _AR_720_:
            case _AR_960P_:
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler) / 2 + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +4 ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
            default:
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler)  + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +4 ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
        }
		//DEBUG_MESSAGE("index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
    }
    if( (pSnapOsd[index].bShow ==1) && (strlen(pSnapOsd[index].szStr) != 0) && index == 1)//bus_number
    {
		stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
		Ae_get_osd_parameter( _AON_BUS_NUMBER_, eResolution, fontname, &fontWidth, &fontHeight, &horScaler, &verScaler );
		char tempstring[128] = {0};
		//DEBUG_MESSAGE("pSnapOsd[%d].szStr %s\n", index, pSnapOsd[index].szStr);
        //strcat(tempstring,pSnapOsd[index].szStr);
        strncpy(tempstring, pSnapOsd[index].szStr, strlen(pSnapOsd[index].szStr));
        tempstring[strlen(pSnapOsd[index].szStr)] = '\0';
        
        CAvUtf8String bus_number(tempstring);
        av_ucs4_t char_unicode;
        av_char_lattice_info_t lattice_char_info;
        while(bus_number.Next_char(&char_unicode) == true)
        {
            if(m_pAv_font_library->Afl_get_char_lattice(fontname, char_unicode, &lattice_char_info) == 0)
            {
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
            }
            else
            {
                DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                return -1;
            }
        }
        if(stuRgnAttr.unAttr.stOverlay.stSize.u32Width == 0)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + 15) / 16 * 16;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + 15) / 16 * 16;
		//DEBUG_MESSAGE("index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
    }
	if((pSnapOsd[index].bShow ==1) && (strlen(pSnapOsd[index].szStr) != 0) && index == 2)//speed
    {
		stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
		Ae_get_osd_parameter( _AON_SPEED_INFO_, eResolution, fontname, &fontWidth, &fontHeight, &horScaler, &verScaler );
        charactorNum = 10;
		stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler)  + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +4 ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
		//DEBUG_MESSAGE("index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
    }
    if( (pSnapOsd[index].bShow ==1) && (strlen(pSnapOsd[index].szStr) != 0) && index == 3)//gps
    {
		stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
		Ae_get_osd_parameter( _AON_GPS_INFO_, eResolution, fontname, &fontWidth, &fontHeight, &horScaler, &verScaler );
        charactorNum = 25;
		stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler)  + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +4 ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        //DEBUG_MESSAGE("index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
    }
	
	if( (pSnapOsd[index].bShow ==1) && (strlen(pSnapOsd[index].szStr) != 0) && index == 4)//chn_name
    {
		stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
		Ae_get_osd_parameter( _AON_CHN_NAME_, eResolution, fontname, &fontWidth, &fontHeight, &horScaler, &verScaler );
		//DEBUG_MESSAGE("pSnapOsd[%d].szStr %s\n", index, pSnapOsd[index].szStr);
		char* channelname = pSnapOsd[index].szStr;
#ifdef _AV_PRODUCT_CLASS_IPC_
        if( pgAvDeviceInfo->Adi_app_type() == APP_TYPE_TEST )
        {
            channelname = (char*)"8888";
        }
#endif
        CAvUtf8String channel_name(channelname);
        av_ucs4_t char_unicode;
        av_char_lattice_info_t lattice_char_info;
        while(channel_name.Next_char(&char_unicode) == true)
        {
            if(m_pAv_font_library->Afl_get_char_lattice(fontname, char_unicode, &lattice_char_info) == 0)
            {
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
            }
            else
            {
                DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                return -1;
            }
        }
        if(stuRgnAttr.unAttr.stOverlay.stSize.u32Width == 0)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + 15) / 16 * 16;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + 15) / 16 * 16;
		//DEBUG_MESSAGE("index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
    }
	
	if( (pSnapOsd[index].bShow ==1) && (strlen(pSnapOsd[index].szStr) != 0) && index == 5) //selfnumber
    {
		stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
		Ae_get_osd_parameter( _AON_SELFNUMBER_INFO_, eResolution, fontname, &fontWidth, &fontHeight, &horScaler, &verScaler );
		char tempstring[128] = {0};
		//DEBUG_MESSAGE("pSnapOsd[%d].szStr %s\n", index, pSnapOsd[index].szStr);
        //strcat(tempstring,pSnapOsd[index].szStr);
        strncpy(tempstring, pSnapOsd[index].szStr, strlen(pSnapOsd[index].szStr));
        tempstring[strlen(pSnapOsd[index].szStr)] = '\0';
        
        CAvUtf8String bus_slefnumber(tempstring);
        av_ucs4_t char_unicode;
        av_char_lattice_info_t lattice_char_info;
        while(bus_slefnumber.Next_char(&char_unicode) == true)
        {
            if(m_pAv_font_library->Afl_get_char_lattice(fontname, char_unicode, &lattice_char_info) == 0)
            {
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
            }
            else
            {
                DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                return -1;
            }
        }
        if(stuRgnAttr.unAttr.stOverlay.stSize.u32Width == 0)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + 15) / 16 * 16;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + 15) / 16 * 16;
        //DEBUG_MESSAGE("index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
	}
	
	/*
	else if(index == 6)
    {
		stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
		Ae_get_osd_parameter( _AON_COMMON_OSD1_, eResolution, NULL, &fontWidth, &fontHeight, &horScaler, &verScaler );
        charactorNum = 5;
		stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler)  + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +4 ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
    }
	else 
    {
		stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
		Ae_get_osd_parameter( _AON_COMMON_OSD2_, eResolution, NULL, &fontWidth, &fontHeight, &horScaler, &verScaler );
        charactorNum = 5;
		stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler)  + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +4 ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
    }
    */

    if( (pSnapOsd[index].bShow ==1) && (strlen(pSnapOsd[index].szStr) != 0) )
    {
        stuRgnAttr.enType = OVERLAY_RGN;
        stuRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
        stuRgnAttr.unAttr.stOverlay.u32BgColor = 0x00;

        m_stuOsdTemp[index].hOsdRegin = /*pgAvDeviceInfo->Adi_max_channel_number() * 3 +*/ index;
        if( HI_MPI_RGN_Destroy(m_stuOsdTemp[index].hOsdRegin) == 0 )
        {
            if( (sRet = HI_MPI_RGN_Create(m_stuOsdTemp[index].hOsdRegin, &stuRgnAttr)) != HI_SUCCESS )
            {
                DEBUG_ERROR( "CHiAvEncoder::Ae_snap_create_regin HI_MPI_RGN_Create FAILED width 0x%08x, regin=%d index=%d\n", sRet, m_stuOsdTemp[index].hOsdRegin, index );
                return -1;
            }
        }
        else
        {
            if( (sRet = HI_MPI_RGN_Create(m_stuOsdTemp[index].hOsdRegin, &stuRgnAttr)) != HI_SUCCESS )
            {
                DEBUG_ERROR( "CHiAvEncoder::Ae_snap_create_regin HI_MPI_RGN_Create FAILED width 0x%08x, regin=%d index=%d\n", sRet, m_stuOsdTemp[index].hOsdRegin, index );
                return -1;
            }
        }
        MPP_CHN_S stuMppChn;
        RGN_CHN_ATTR_S stuRgnChnAttr;
        memset( &stuRgnChnAttr, 0, sizeof(stuRgnChnAttr) );
        stuRgnChnAttr.bShow = HI_TRUE;
        stuRgnChnAttr.enType = OVERLAY_RGN;
        stuRgnChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 0;
        stuRgnChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128;
        stuRgnChnAttr.unChnAttr.stOverlayChn.u32Layer = 0;

        stuRgnChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
        stuRgnChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;

        stuRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = HI_FALSE;
        stuRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Width = 16;
        stuRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Height = 16;
        stuRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.u32LumThresh = 200;
        stuRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = MORETHAN_LUM_THRESH;

        stuMppChn.enModId = HI_ID_GROUP;
        stuMppChn.s32DevId = vencGrp;
        stuMppChn.s32ChnId = 0;

        if( (sRet=HI_MPI_RGN_AttachToChn(m_stuOsdTemp[index].hOsdRegin, &stuMppChn, &stuRgnChnAttr)) != HI_SUCCESS )
        {        
            DEBUG_ERROR( "CHiAvEncoder::Ae_snap_create_regin HI_MPI_RGN_AttachToChn FAILED(0x%08x)\n", sRet);

            HI_MPI_RGN_Destroy( m_stuOsdTemp[index].hOsdRegin );
            return -1;
        }

        m_stuOsdTemp[index].vencGrp = vencGrp;
    	if( index == 0)
    	{
    		m_stuOsdTemp[index].eOsdName = _AON_DATE_TIME_;
    	}
    	if( index == 1)
    	{
    		m_stuOsdTemp[index].eOsdName = _AON_BUS_NUMBER_;
    		
    	}
    	if( index == 2)
    	{
    		m_stuOsdTemp[index].eOsdName = _AON_SPEED_INFO_;
    		
    	}
    	if( index == 3)
    	{
    		m_stuOsdTemp[index].eOsdName = _AON_GPS_INFO_;
    	}
    	
    	if( index == 4)
    	{
    		m_stuOsdTemp[index].eOsdName = _AON_CHN_NAME_;
    		
    	}
    	
    	if( index == 5)
    	{
    		m_stuOsdTemp[index].eOsdName = _AON_SELFNUMBER_INFO_;
    		
    	}
    	/*
    	else if(index == 6)
    	{
    		m_stuOsdTemp[index].eOsdName = _AON_COMMON_OSD1_;
    	}
    	else 
    	{
    		m_stuOsdTemp[index].eOsdName = _AON_COMMON_OSD2_;
    	}
    	*/
        //m_stuOsdTemp[index].eOsdName = (index==0? _AON_DATE_TIME_: _AON_CHN_NAME_);
        m_stuOsdTemp[index].eResolution = eResolution;

        /*bitmap*/
        m_stuOsdTemp[index].u32BitmapWidth = stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
        m_stuOsdTemp[index].u32BitmapHeight = stuRgnAttr.unAttr.stOverlay.stSize.u32Height;
        m_stuOsdTemp[index].pszBitmap = new unsigned char[m_stuOsdTemp[index].u32BitmapWidth * horScaler * m_stuOsdTemp[index].u32BitmapHeight * verScaler * 2];

        _AV_FATAL_(  m_stuOsdTemp[index].pszBitmap == NULL, "OUT OF MEMORY\n" );
        m_stuOsdTemp[index].bInited = true;
    }
    return 0;
}
#endif

int CHiAvEncoder::Ae_snap_regin_delete( int index )
{
    if( !m_stuOsdTemp[index].bInited )
    {
        return 0;
    }

    HI_S32 sRet = HI_FAILURE;
    
    if( (sRet=HI_MPI_RGN_Destroy(m_stuOsdTemp[index].hOsdRegin)) != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_snap_delete_regin HI_MPI_RGN_Destroy FAILED(0x%08x)\n", sRet);
    }
    if(m_stuOsdTemp[index].pszBitmap != NULL)
    {
        delete []m_stuOsdTemp[index].pszBitmap;
        m_stuOsdTemp[index].pszBitmap = NULL;
    }
    m_stuOsdTemp[index].bInited = false;

    return 0;
}

int CHiAvEncoder::Ae_encode_vi_to_picture( VIDEO_FRAME_INFO_S *pstuViFrame, hi_snap_osd_t stuOsdParam[], int osd_num,  VENC_STREAM_S* pstuPicStream)
{
    HI_S32 sRet = HI_SUCCESS;
    VENC_CHN snapChl;
    
    if( Ae_video_encoder_allotter_snap(&snapChl) != 0 )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_encode_vi_to_picture Ae_video_encoder_allotter_snap FAILED\n");
        return -1;
    }
    Ae_snap_regin_set_osds(stuOsdParam, osd_num);
    static unsigned int u32TimeRef = 2;
    pstuViFrame->stVFrame.u32TimeRef = u32TimeRef;
    u32TimeRef += 2;

    sRet = HI_MPI_VENC_SendFrame( snapChl, pstuViFrame, -1 );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_encode_vi_to_picture HI_MPI_VENC_SendFrame err:0x%08x\n", sRet);
        return -1;
    }

    HI_S32 VencFd = HI_MPI_VENC_GetFd(snapChl);
    if (VencFd <= 0)
    {
        DEBUG_ERROR("======================get fd failed\n");
        return -1;
    }

    fd_set read_fds;
    struct timeval TimeoutVal;

    FD_ZERO(&read_fds);
    FD_SET(VencFd,&read_fds);

    TimeoutVal.tv_sec = 2;
    TimeoutVal.tv_usec = 0;
    int ret = select(VencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
    if(ret <= 0)
    {
        DEBUG_ERROR("###################select err\n");
        return -1;
    }

    VENC_CHN_STAT_S stuVencChnStat;
    sRet = HI_MPI_VENC_Query( snapChl, &stuVencChnStat );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_encode_vi_to_picture HI_MPI_VENC_Query err:0x%08x\n", sRet);
#if  defined(_AV_PLATFORM_HI3516A_)
        HI_MPI_VENC_CloseFd(snapChl);
#endif
        return -1;
    }

    if( stuVencChnStat.u32CurPacks == 0 )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_encode_vi_to_picture err because packcount=%d\n", stuVencChnStat.u32CurPacks);
#if  defined(_AV_PLATFORM_HI3516A_)
        HI_MPI_VENC_CloseFd(snapChl);
#endif
        return -1;
    }

    static unsigned int u16PackCount = 0;
    static VENC_PACK_S* pPackAddr = NULL;
    if( stuVencChnStat.u32CurPacks > u16PackCount || pPackAddr == NULL )
    {
        if( pPackAddr )
        {
            delete [] pPackAddr;
            pPackAddr = NULL;
        }
        u16PackCount = stuVencChnStat.u32CurPacks;
        pPackAddr = new VENC_PACK_S[u16PackCount];
    }
    
    memset(pPackAddr, 0, sizeof(VENC_PACK_S)*u16PackCount);
    pstuPicStream->pstPack = pPackAddr;
    pstuPicStream->u32PackCount = stuVencChnStat.u32CurPacks;

    sRet = HI_MPI_VENC_GetStream( snapChl, pstuPicStream, HI_TRUE );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_encode_vi_to_picture HI_MPI_VENC_GetStream err:0x%08x\n", sRet);
#if  defined(_AV_PLATFORM_HI3516A_)
        HI_MPI_VENC_CloseFd(snapChl);
#endif
        return -1;
    }

    return 0;
}

int CHiAvEncoder::Ae_encoder_vi_to_picture_free( VENC_STREAM_S* pstuPicStream )
{
    VENC_CHN snapChl;
    if( Ae_video_encoder_allotter_snap(&snapChl) != 0 )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_encoder_vi_to_picture_free Ae_video_encoder_allotter_snap FAILED\n");
        return -1;
    }

    HI_S32 sRet = HI_MPI_VENC_ReleaseStream( snapChl, pstuPicStream );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_encoder_vi_to_picture_free HI_MPI_VENC_ReleaseStream FAILED width 0x%08x\n", sRet);
        return -1;
    }

    return 0;
}

#if defined(_AV_HISILICON_VPSS_)
int CHiAvEncoder::Ae_venc_vpss_control(av_video_stream_type_t video_stream_type, int phy_video_chn_num, bool bind_flag)
{
    VENC_CHN venc_chn = -1;

    if(Ae_video_encoder_allotter(video_stream_type, phy_video_chn_num, &venc_chn) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_venc_vpss_control Ae_video_encoder_allotter FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
        return -1;
    }

    vpss_purpose_t vpss_purpose;
    switch(video_stream_type)
    {
        case _AST_MAIN_VIDEO_:
            vpss_purpose = _VP_MAIN_STREAM_;
            break;

        case _AST_SUB_VIDEO_:
            vpss_purpose = _VP_SUB_STREAM_;
            break;
            
        case _AST_SUB2_VIDEO_:
            vpss_purpose = _VP_SUB2_STREAM_;
            break;
        default:
            _AV_FATAL_(1, "CHiAvEncoder::Ae_venc_vpss_control you must give the implement(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
            break;
    }

    VPSS_GRP vpss_grp;
    VPSS_CHN vpss_chn;
    if(m_pHi_vpss->HiVpss_get_vpss(vpss_purpose, phy_video_chn_num, &vpss_grp, &vpss_chn) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_venc_vpss_control m_pHi_vpss->HiVpss_get_vpss FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
        return -1;
    }

    MPP_CHN_S mpp_chn_src;
    MPP_CHN_S mpp_chn_dst;
    HI_S32 ret_val = -1;

    mpp_chn_src.enModId = HI_ID_VPSS;
    mpp_chn_src.s32DevId = vpss_grp;
    mpp_chn_src.s32ChnId = vpss_chn;

    mpp_chn_dst.enModId = HI_ID_VENC;
    mpp_chn_dst.s32DevId = 0;
    mpp_chn_dst.s32ChnId = venc_chn;

    if(bind_flag == true)
    {
        if((ret_val = HI_MPI_SYS_Bind(&mpp_chn_src, &mpp_chn_dst)) != HI_SUCCESS)
        {            
            DEBUG_ERROR( "CHiAvEncoder::Ae_venc_vpss_control HI_MPI_SYS_Bind FAILED(type:%d)(chn:%d)(venc chn:%d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, venc_chn, (unsigned long)ret_val);
            return -1;
        }
    }
    else
    {
        if((ret_val = HI_MPI_SYS_UnBind(&mpp_chn_src, &mpp_chn_dst)) != HI_SUCCESS)
        {
            DEBUG_ERROR( "CHiAvEncoder::Ae_venc_vpss_control HI_MPI_SYS_UnBind FAILED(type:%d)(chn:%d)(venc chn: %d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, venc_chn, (unsigned long)ret_val);
            return -1;
        }
        
        if(m_pHi_vpss->HiVpss_release_vpss(vpss_purpose, phy_video_chn_num) != 0)
        {
            DEBUG_ERROR( "CHiAvEncoder::Ae_venc_vpss_control m_pHi_vpss->HiVpss_release_vpss FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
            //return -1;
        }
        ;
    }

    return 0;
}
#endif


int CHiAvEncoder::Ae_dynamic_modify_encoder_local(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para)
{
    VENC_CHN venc_chn = -1;

    if(Ae_video_encoder_allotter(video_stream_type, phy_video_chn_num, &venc_chn) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_dynamic_modify_encoder_local Ae_video_encoder_allotter FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
        return -1;
    }
    VENC_CHN_ATTR_S venc_chn_attr;
    HI_S32 ret_val = -1;

    /*frame rate, bit rate, gop size*/
    if((ret_val = HI_MPI_VENC_GetChnAttr(venc_chn, &venc_chn_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_dynamic_modify_encoder_local HI_MPI_VENC_GetChnAttr FAILED(0x%08x)\n", ret_val);
        return -1;
    }

    switch(pVideo_para->bitrate_mode)
    {
        case _ABM_CBR_:
        {
            if(pVideo_para->video_type == _AVT_H265_)
            {
                venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
                venc_chn_attr.stRcAttr.stAttrH265Cbr.u32Gop = pVideo_para->gop_size;
                venc_chn_attr.stRcAttr.stAttrH265Cbr.u32SrcFrmRate = pVideo_para->source_frame_rate;
                venc_chn_attr.stRcAttr.stAttrH265Cbr.u32BitRate = pVideo_para->bitrate.hicbr.bitrate;
                venc_chn_attr.stRcAttr.stAttrH265Cbr.u32StatTime = 1;
                venc_chn_attr.stRcAttr.stAttrH265Cbr.u32FluctuateLevel = 0;
                venc_chn_attr.stRcAttr.stAttrH265Cbr.fr32DstFrmRate = pVideo_para->frame_rate;      
            }
            else
            {
                venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
                venc_chn_attr.stRcAttr.stAttrH264Cbr.u32Gop = pVideo_para->gop_size;
                venc_chn_attr.stRcAttr.stAttrH264Cbr.u32SrcFrmRate = pVideo_para->frame_rate;
                venc_chn_attr.stRcAttr.stAttrH264Cbr.u32BitRate = pVideo_para->bitrate.hicbr.bitrate;
                venc_chn_attr.stRcAttr.stAttrH264Cbr.u32StatTime = 1;
                venc_chn_attr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
                venc_chn_attr.stRcAttr.stAttrH264Cbr.fr32DstFrmRate = pVideo_para->source_frame_rate;                
            }
        }
            break;

        case _ABM_VBR_:
        {
            if(pVideo_para->video_type == _AVT_H265_)
            {
                venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H265VBR;
                venc_chn_attr.stRcAttr.stAttrH265Vbr.u32Gop = pVideo_para->gop_size;
                venc_chn_attr.stRcAttr.stAttrH265Vbr.fr32DstFrmRate = pVideo_para->frame_rate;
                venc_chn_attr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = pVideo_para->bitrate.hivbr.bitrate;
                venc_chn_attr.stRcAttr.stAttrH265Vbr.u32StatTime = 1;
                if(video_stream_type == _AST_SUB2_VIDEO_)
                {
                    venc_chn_attr.stRcAttr.stAttrH265Vbr.u32MinQp = 24;
                    switch(pVideo_para->net_transfe_mode)
                    {
                        case 0:
                            venc_chn_attr.stRcAttr.stAttrH265Vbr.u32MaxQp = 40;
                            break;
                        case 1:
                            venc_chn_attr.stRcAttr.stAttrH265Vbr.u32MaxQp = 39;
                            break;
                        case 2:
                            venc_chn_attr.stRcAttr.stAttrH265Vbr.u32MaxQp = 38;
                            break;
                        case 3:
                            venc_chn_attr.stRcAttr.stAttrH265Vbr.u32MaxQp = 37;
                            break;
                        default:
                            venc_chn_attr.stRcAttr.stAttrH265Vbr.u32MaxQp = 36;
                            break;
                    }
                }
                else
                {
                    venc_chn_attr.stRcAttr.stAttrH265Vbr.u32MinQp = 24;
                    venc_chn_attr.stRcAttr.stAttrH265Vbr.u32MaxQp = 51;
                }
                venc_chn_attr.stRcAttr.stAttrH265Vbr.u32SrcFrmRate = pVideo_para->source_frame_rate;

            }
            else
            {
                venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
                venc_chn_attr.stRcAttr.stAttrH264Vbr.u32Gop = pVideo_para->gop_size;
                venc_chn_attr.stRcAttr.stAttrH264Vbr.fr32DstFrmRate = pVideo_para->frame_rate;
                venc_chn_attr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = pVideo_para->bitrate.hivbr.bitrate;
                venc_chn_attr.stRcAttr.stAttrH264Vbr.u32StatTime = 1;
                if(video_stream_type == _AST_SUB2_VIDEO_)
                {
                    venc_chn_attr.stRcAttr.stAttrH264Vbr.u32MinQp = 24;
                    switch(pVideo_para->net_transfe_mode)
                    {
                        case 0:
                            venc_chn_attr.stRcAttr.stAttrH264Vbr.u32MaxQp = 40;
                            break;
                        case 1:
                            venc_chn_attr.stRcAttr.stAttrH264Vbr.u32MaxQp = 39;
                            break;
                        case 2:
                            venc_chn_attr.stRcAttr.stAttrH264Vbr.u32MaxQp = 38;
                            break;
                        case 3:
                            venc_chn_attr.stRcAttr.stAttrH264Vbr.u32MaxQp = 37;
                            break;
                        default:
                            venc_chn_attr.stRcAttr.stAttrH264Vbr.u32MaxQp = 36;
                            break;
                    }
                }
                else
                {
                    venc_chn_attr.stRcAttr.stAttrH264Vbr.u32MinQp = 24;
                    venc_chn_attr.stRcAttr.stAttrH264Vbr.u32MaxQp = 51;
                }
                venc_chn_attr.stRcAttr.stAttrH264Vbr.u32SrcFrmRate = pVideo_para->source_frame_rate;
            }
        }
            break;
        default:
            _AV_FATAL_(1, "CHiAvEncoder::Ae_dynamic_modify_encoder_local You must give the implement(%d)\n", pVideo_para->bitrate_mode);
            break;
    }

    if((ret_val = HI_MPI_VENC_SetChnAttr(venc_chn, &venc_chn_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_dynamic_modify_encoder_local HI_MPI_VENC_SetChnAttr FAILED(0x%08x)\n", ret_val);
        return -1;
    }

    return 0;
}


int CHiAvEncoder::Ae_create_h264_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, video_encoder_id_t *pVideo_encoder_id)
{
    VENC_CHN venc_chn = -1;
    HI_S32 ret_val = -1;
    if(Ae_video_encoder_allotter(video_stream_type, phy_video_chn_num, &venc_chn) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder Ae_video_encoder_allotter FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
        return -1;
    }

    VENC_CHN_ATTR_S venc_chn_attr;

    memset(&venc_chn_attr, 0, sizeof(VENC_CHN_ATTR_S));

    /****************video encoder attribute*****************/
    venc_chn_attr.stVeAttr.enType = PT_H264;
    /*maximum picture size*/
    if(pVideo_para->resolution != _AR_SIZE_)
    {
        pgAvDeviceInfo->Adi_get_video_size(pVideo_para->resolution, pVideo_para->tv_system, (int *)&venc_chn_attr.stVeAttr.stAttrH264e.u32MaxPicWidth, (int *)&venc_chn_attr.stVeAttr.stAttrH264e.u32MaxPicHeight);
    }
    else
    {
        venc_chn_attr.stVeAttr.stAttrH264e.u32MaxPicWidth = pVideo_para->resolution_w;
        venc_chn_attr.stVeAttr.stAttrH264e.u32MaxPicHeight = pVideo_para->resolution_h;
    }
    //align with 2
    venc_chn_attr.stVeAttr.stAttrH264e.u32MaxPicWidth = (venc_chn_attr.stVeAttr.stAttrH264e.u32MaxPicWidth + 1) / 2 * 2;
    venc_chn_attr.stVeAttr.stAttrH264e.u32MaxPicHeight = (venc_chn_attr.stVeAttr.stAttrH264e.u32MaxPicHeight + 1) / 2 * 2;
    
#ifdef _AV_PRODUCT_CLASS_IPC_
    pgAvDeviceInfo->Adi_covert_vidoe_size_by_rotate( (int*)&venc_chn_attr.stVeAttr.stAttrH264e.u32MaxPicWidth, (int*)&venc_chn_attr.stVeAttr.stAttrH264e.u32MaxPicHeight );
#endif

    /*buffer size*/
    venc_chn_attr.stVeAttr.stAttrH264e.u32BufSize = pgAvDeviceInfo->Adi_get_pixel_size() * venc_chn_attr.stVeAttr.stAttrH264e.u32MaxPicWidth * venc_chn_attr.stVeAttr.stAttrH264e.u32MaxPicHeight;
    /*profile*/
    switch(video_stream_type)
    {
        case _AST_MAIN_VIDEO_:
        default:
            venc_chn_attr.stVeAttr.stAttrH264e.u32Profile = 1;/*main profile*/
            break;
        case _AST_SUB_VIDEO_:
            venc_chn_attr.stVeAttr.stAttrH264e.u32Profile = 1;
            break;
        case _AST_SUB2_VIDEO_:
            venc_chn_attr.stVeAttr.stAttrH264e.u32Profile = 1;
            break;
    }
    
    /*get frame mode*/
    venc_chn_attr.stVeAttr.stAttrH264e.bByFrame = HI_TRUE;
    /**/
    venc_chn_attr.stVeAttr.stAttrH264e.u32BFrameNum = 0 ;
    venc_chn_attr.stVeAttr.stAttrH264e.u32RefNum = 0;
    /**/
    venc_chn_attr.stVeAttr.stAttrH264e.u32PicWidth = venc_chn_attr.stVeAttr.stAttrH264e.u32MaxPicWidth;
    venc_chn_attr.stVeAttr.stAttrH264e.u32PicHeight = venc_chn_attr.stVeAttr.stAttrH264e.u32MaxPicHeight;

#ifdef _AV_HAVE_VIDEO_INPUT_
    vi_config_set_t vi_primary_config, vi_minor_config, vi_source_config;
    m_pHi_vi->HiVi_get_primary_vi_chn_config(phy_video_chn_num, &vi_primary_config, &vi_minor_config, &vi_source_config);
#endif

    /****************bitrate*****************/
    switch(pVideo_para->bitrate_mode)
    {
        case _ABM_CBR_:
            venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
            venc_chn_attr.stRcAttr.stAttrH264Cbr.u32Gop = pVideo_para->gop_size;
            venc_chn_attr.stRcAttr.stAttrH264Cbr.u32StatTime = 1;
            venc_chn_attr.stRcAttr.stAttrH264Cbr.fr32DstFrmRate = pVideo_para->frame_rate;
            venc_chn_attr.stRcAttr.stAttrH264Cbr.u32BitRate = pVideo_para->bitrate.hicbr.bitrate;
            venc_chn_attr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;   
#ifdef _AV_PRODUCT_CLASS_IPC_
            venc_chn_attr.stRcAttr.stAttrH264Cbr.u32SrcFrmRate = pVideo_para->source_frame_rate; 
#else
            venc_chn_attr.stRcAttr.stAttrH264Cbr.u32SrcFrmRate = vi_source_config.frame_rate;
#endif
            break;

        case _ABM_VBR_:
            venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
            venc_chn_attr.stRcAttr.stAttrH264Vbr.u32Gop = pVideo_para->gop_size;
#if defined(_AV_PLATFORM_HI3518EV200_)
            venc_chn_attr.stRcAttr.stAttrH264Vbr.u32StatTime = 2;
#else
            venc_chn_attr.stRcAttr.stAttrH264Vbr.u32StatTime = 1;
#endif
            if(video_stream_type == _AST_SUB2_VIDEO_)
            {
                venc_chn_attr.stRcAttr.stAttrH264Vbr.u32MinQp = 24;
                switch(pVideo_para->net_transfe_mode)
                {
                    case 0:
                        venc_chn_attr.stRcAttr.stAttrH264Vbr.u32MaxQp = 40;
                        break;
                    case 1:
                        venc_chn_attr.stRcAttr.stAttrH264Vbr.u32MaxQp = 39;
                        break;
                    case 2:
                        venc_chn_attr.stRcAttr.stAttrH264Vbr.u32MaxQp = 38;
                        break;
                    case 3:
                        venc_chn_attr.stRcAttr.stAttrH264Vbr.u32MaxQp = 37;
                        break;
                    default:
                        venc_chn_attr.stRcAttr.stAttrH264Vbr.u32MaxQp = 36;
                        break;
                }
            }
            else
            {
                venc_chn_attr.stRcAttr.stAttrH264Vbr.u32MinQp = 24;
                venc_chn_attr.stRcAttr.stAttrH264Vbr.u32MaxQp = 51;
            }
            venc_chn_attr.stRcAttr.stAttrH264Vbr.fr32DstFrmRate = pVideo_para->frame_rate;
            venc_chn_attr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = pVideo_para->bitrate.hivbr.bitrate;     
#ifdef _AV_PRODUCT_CLASS_IPC_
            venc_chn_attr.stRcAttr.stAttrH264Vbr.u32SrcFrmRate = pVideo_para->source_frame_rate;
#else
            venc_chn_attr.stRcAttr.stAttrH264Vbr.u32SrcFrmRate = vi_source_config.frame_rate;
#endif
            break;

        default:
            _AV_FATAL_(1, "CHiAvEncoder::Ae_create_h264_encoder You must give the implement(%d)\n", pVideo_para->bitrate_mode);
            break;
    }

#if defined(_AV_PRODUCT_CLASS_IPC_)
        /*<the main stream don't keep same with venc has two reason: 1、if the main stream resolution is less than isp output
        resolution, the snap may be failed. 2、the vpss chn 0 can't do scale, so call vpss_set_chan_mode interface may be failed!>*/
         if(video_stream_type == _AST_MAIN_VIDEO_)
         {
             IspOutputMode_e iom = pgAvDeviceInfo->Adi_get_isp_output_mode();
             int width = 0;
             int height = 0;
             m_pHi_vi->HiVi_get_video_size(iom, &width, &height);
             pgAvDeviceInfo->Adi_set_stream_size(phy_video_chn_num ,video_stream_type, width,height);
         }
#endif

    if((ret_val = HI_MPI_VENC_CreateChn(venc_chn, &venc_chn_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder HI_MPI_VENC_CreateChn FAILED(type:%d)(chn:%d)(venc chn: %d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, venc_chn, (unsigned long)ret_val);
        return -1;
    }

    VENC_PARAM_REF_S  venc_h264_ref_para;

    memset(&venc_h264_ref_para, 0, sizeof(VENC_PARAM_REF_S));
    if (pVideo_para->ref_para.ref_type == _ART_REF_FOR_CUSTOM)
    {
        venc_h264_ref_para.bEnablePred = HI_FALSE;
        if (pVideo_para->ref_para.pred_enable == 1)
        {
            venc_h264_ref_para.bEnablePred = HI_TRUE;
        }
        venc_h264_ref_para.u32Base = pVideo_para->ref_para.base_interval;
        venc_h264_ref_para.u32Enhance = pVideo_para->ref_para.enhance_interval;
    }
    else
    {
        venc_h264_ref_para.bEnablePred = HI_TRUE;
        if (_ART_REF_FOR_2X == pVideo_para->ref_para.ref_type)
        { 
            venc_h264_ref_para.u32Base = 1;
            venc_h264_ref_para.u32Enhance = 1;
        }
        else if (_ART_REF_FOR_4X == pVideo_para->ref_para.ref_type)
        {
            venc_h264_ref_para.u32Base = 2;
            venc_h264_ref_para.u32Enhance = 1;
        }
        else
        {
            venc_h264_ref_para.u32Base = 1;
            venc_h264_ref_para.u32Enhance = 0;
        }
    }
    
    DEBUG_WARNING( "ref custom eable %d base %d enhance %d\n", venc_h264_ref_para.bEnablePred, venc_h264_ref_para.u32Base, venc_h264_ref_para.u32Enhance);
    if((ret_val = HI_MPI_VENC_SetRefParam(venc_chn, &venc_h264_ref_para)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::HI_MPI_VENC_SetH264eRefParam HI_MPI_VENC_SetH264eRefMode FAILED(type:%d)(chn:%d)(venc chn:%d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, venc_chn, (unsigned long)ret_val);
        return -1;
    }

    if((ret_val = HI_MPI_VENC_StartRecvPic(venc_chn)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder HI_MPI_VENC_StartRecvPic FAILED(type:%d)(chn:%d)(venc chn: %d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, venc_chn, (unsigned long)ret_val);
        return -1;
    }

#if defined(_AV_HISILICON_VPSS_)
#if defined(_AV_SUPPORT_IVE_)
    if(1 == pgAvDeviceInfo->Adi_get_ive_debug_mode())
    {
#if defined(_AV_IVE_FD_)
        if( video_stream_type != _AST_MAIN_VIDEO_)
#else
        if( video_stream_type != _AST_SUB_VIDEO_)
#endif
        {
            if(Ae_venc_vpss_control(video_stream_type, phy_video_chn_num, true) != 0)
            {
                DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder Ae_venc_vpss_control FAILED(type:%d)(chn:%d)(venc chn: %d)\n", video_stream_type, phy_video_chn_num, venc_chn);
                return -1;
            }        
        }
    }
    else
    {
        if(Ae_venc_vpss_control(video_stream_type, phy_video_chn_num, true) != 0)
        {
            DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder Ae_venc_vpss_control FAILED(type:%d)(chn:%d)(venc chn: %d)\n", video_stream_type, phy_video_chn_num, venc_chn);
            return -1;
        }        
    }
#else
    if(Ae_venc_vpss_control(video_stream_type, phy_video_chn_num, true) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder Ae_venc_vpss_control FAILED(type:%d)(chn:%d)(venc chn: %d)\n", video_stream_type, phy_video_chn_num, venc_chn);
        return -1;
    }
#endif
#endif

    memset(pVideo_encoder_id, 0, sizeof(video_encoder_id_t));
    pVideo_encoder_id->video_encoder_fd = HI_MPI_VENC_GetFd(venc_chn);
    hi_video_encoder_info_t *pVei = NULL;
    pVei = new hi_video_encoder_info_t;
    _AV_FATAL_(pVei == NULL, "CHiAvEncoder::Ae_create_h264_encoder OUT OF MEMORY\n");
    memset(pVei, 0, sizeof(hi_video_encoder_info_t));
    pVei->venc_chn = venc_chn;
    pVideo_encoder_id->pVideo_encoder_info = (void *)pVei;

    //DEBUG_MESSAGE( "CHiAvEncoder::Ae_create_h264_encoder(type:%d)(chn:%d)(res:%d)(frame_rate:%d)(bitrate:%d)(gop:%d)(fd:%d)\n", video_stream_type, phy_video_chn_num, pVideo_para->resolution, pVideo_para->frame_rate, pVideo_para->bitrate.hicbr.bitrate, pVideo_para->gop_size, pVideo_encoder_id->video_encoder_fd);

    return 0;
}


int CHiAvEncoder::Ae_create_h265_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, video_encoder_id_t *pVideo_encoder_id)
{
    VENC_CHN venc_chn = -1;
    HI_S32 ret_val = -1;
    VENC_PARAM_REF_S venc_param_ref;

    if(Ae_video_encoder_allotter(video_stream_type, phy_video_chn_num, &venc_chn) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h265_encoder Ae_video_encoder_allotter FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
        return -1;
    }
    
    VENC_CHN_ATTR_S venc_chn_attr;

    memset(&venc_chn_attr, 0, sizeof(VENC_CHN_ATTR_S));

    /****************video encoder attribute*****************/
    venc_chn_attr.stVeAttr.enType = PT_H265;
    /*maximum picture size*/

    if(pVideo_para->resolution != _AR_SIZE_)
    {
        pgAvDeviceInfo->Adi_get_video_size(pVideo_para->resolution, pVideo_para->tv_system, (int *)&venc_chn_attr.stVeAttr.stAttrH265e.u32MaxPicWidth, (int *)&venc_chn_attr.stVeAttr.stAttrH265e.u32MaxPicHeight);
    }
    else
    {
        venc_chn_attr.stVeAttr.stAttrH265e.u32MaxPicWidth = pVideo_para->resolution_w;
        venc_chn_attr.stVeAttr.stAttrH265e.u32MaxPicHeight = pVideo_para->resolution_h;
    }
    //align with 2
    venc_chn_attr.stVeAttr.stAttrH265e.u32MaxPicWidth = (venc_chn_attr.stVeAttr.stAttrH265e.u32MaxPicWidth + 1) / 2 * 2;
    venc_chn_attr.stVeAttr.stAttrH265e.u32MaxPicHeight = (venc_chn_attr.stVeAttr.stAttrH265e.u32MaxPicHeight + 1) / 2 * 2;
    
#ifdef _AV_PRODUCT_CLASS_IPC_
    pgAvDeviceInfo->Adi_covert_vidoe_size_by_rotate( (int*)&venc_chn_attr.stVeAttr.stAttrH265e.u32MaxPicWidth, (int*)&venc_chn_attr.stVeAttr.stAttrH265e.u32MaxPicHeight );
#endif

    /*buffer size*/
    venc_chn_attr.stVeAttr.stAttrH265e.u32BufSize = pgAvDeviceInfo->Adi_get_pixel_size() * venc_chn_attr.stVeAttr.stAttrH265e.u32MaxPicWidth * venc_chn_attr.stVeAttr.stAttrH265e.u32MaxPicHeight;
    //venc_chn_attr.stVeAttr.stAttrH265e.u32BufSize = 2 * venc_chn_attr.stVeAttr.stAttrH265e.u32MaxPicWidth * venc_chn_attr.stVeAttr.stAttrH265e.u32MaxPicHeight;
    /*profile*/
    switch(video_stream_type)
    {
        case _AST_MAIN_VIDEO_:
        case _AST_SUB_VIDEO_:
        case _AST_SUB2_VIDEO_:
        default:
            venc_chn_attr.stVeAttr.stAttrH265e.u32Profile = 0;/*main profile*/
            break;
    }
    
    /*get frame mode*/
    venc_chn_attr.stVeAttr.stAttrH265e.bByFrame = HI_TRUE;
    venc_chn_attr.stVeAttr.stAttrH265e.u32BFrameNum = 0;
    venc_chn_attr.stVeAttr.stAttrH265e.u32RefNum  = 0;
    /**/
    venc_chn_attr.stVeAttr.stAttrH265e.u32PicWidth = venc_chn_attr.stVeAttr.stAttrH265e.u32MaxPicWidth;
    venc_chn_attr.stVeAttr.stAttrH265e.u32PicHeight = venc_chn_attr.stVeAttr.stAttrH265e.u32MaxPicHeight;

#ifdef _AV_HAVE_VIDEO_INPUT_
    vi_config_set_t vi_primary_config, vi_minor_config, vi_source_config;
    m_pHi_vi->HiVi_get_primary_vi_chn_config(phy_video_chn_num, &vi_primary_config, &vi_minor_config, &vi_source_config);
#endif

    /****************bitrate*****************/
    switch(pVideo_para->bitrate_mode)
    {
        case _ABM_CBR_:
            venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
            venc_chn_attr.stRcAttr.stAttrH265Cbr.u32Gop = pVideo_para->gop_size;
            venc_chn_attr.stRcAttr.stAttrH265Cbr.u32StatTime = 1;
            venc_chn_attr.stRcAttr.stAttrH265Cbr.fr32DstFrmRate = pVideo_para->frame_rate;
            venc_chn_attr.stRcAttr.stAttrH265Cbr.u32BitRate = pVideo_para->bitrate.hicbr.bitrate;
            venc_chn_attr.stRcAttr.stAttrH265Cbr.u32FluctuateLevel = 0;
#ifdef _AV_PRODUCT_CLASS_IPC_
            venc_chn_attr.stRcAttr.stAttrH265Cbr.u32SrcFrmRate = pVideo_para->source_frame_rate;  
#else
            venc_chn_attr.stRcAttr.stAttrH265Cbr.u32SrcFrmRate = vi_source_config.frame_rate;
#endif
            break;

        case _ABM_VBR_:
            venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H265VBR;
            venc_chn_attr.stRcAttr.stAttrH265Vbr.u32Gop = pVideo_para->gop_size;
            venc_chn_attr.stRcAttr.stAttrH265Vbr.u32StatTime = 1;
            if(video_stream_type == _AST_SUB2_VIDEO_)
            {
                venc_chn_attr.stRcAttr.stAttrH265Vbr.u32MinQp = 24;
                switch(pVideo_para->net_transfe_mode)
                {
                    case 0:
                        venc_chn_attr.stRcAttr.stAttrH265Vbr.u32MaxQp = 40;
                        break;
                    case 1:
                        venc_chn_attr.stRcAttr.stAttrH265Vbr.u32MaxQp = 39;
                        break;
                    case 2:
                        venc_chn_attr.stRcAttr.stAttrH265Vbr.u32MaxQp = 38;
                        break;
                    case 3:
                        venc_chn_attr.stRcAttr.stAttrH265Vbr.u32MaxQp = 37;
                        break;
                    default:
                        venc_chn_attr.stRcAttr.stAttrH265Vbr.u32MaxQp = 36;
                        break;
                }
                
                
            }
            else
            {
                venc_chn_attr.stRcAttr.stAttrH265Vbr.u32MinQp = 10;
                venc_chn_attr.stRcAttr.stAttrH265Vbr.u32MaxQp = 40;
            }
            venc_chn_attr.stRcAttr.stAttrH265Vbr.fr32DstFrmRate = pVideo_para->frame_rate;
            venc_chn_attr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = pVideo_para->bitrate.hivbr.bitrate;
#ifdef _AV_PRODUCT_CLASS_IPC_
            venc_chn_attr.stRcAttr.stAttrH265Vbr.u32SrcFrmRate = pVideo_para->source_frame_rate;
#else
            venc_chn_attr.stRcAttr.stAttrH265Vbr.u32SrcFrmRate = vi_source_config.frame_rate;   
#endif
            break;

        default:
            _AV_FATAL_(1, "CHiAvEncoder::Ae_create_h265_encoder You must give the implement(%d)\n", pVideo_para->bitrate_mode);
            break;
    }
#if defined(_AV_PRODUCT_CLASS_IPC_)
    /*<the main stream don't keep same with venc has two reason: 1、if the main stream resolution is less than isp output
    resolution, the snap may be failed. 2、the vpss chn 0 can't do scale, so call vpss_set_chan_mode interface may be failed!>*/
     if(video_stream_type == _AST_MAIN_VIDEO_)
     {
         IspOutputMode_e iom = pgAvDeviceInfo->Adi_get_isp_output_mode();
         int width = 0;
         int height = 0;
         m_pHi_vi->HiVi_get_video_size(iom, &width, &height);
         pgAvDeviceInfo->Adi_set_stream_size(phy_video_chn_num ,video_stream_type, width,height);
     }
     else
     {
         pgAvDeviceInfo->Adi_set_stream_size(phy_video_chn_num, video_stream_type, venc_chn_attr.stVeAttr.stAttrH264e.u32PicWidth,venc_chn_attr.stVeAttr.stAttrH264e.u32PicHeight);
     } 
#endif

    if((ret_val = HI_MPI_VENC_CreateChn(venc_chn, &venc_chn_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h265_encoder HI_MPI_VENC_CreateChn FAILED(type:%d)(chn:%d)(venc chn:%d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, venc_chn, (unsigned long)ret_val);
        return -1;
    }

    memset(&venc_param_ref, 0, sizeof(VENC_PARAM_REF_S));
    if (pVideo_para->ref_para.ref_type == _ART_REF_FOR_CUSTOM)
    {

        venc_param_ref.bEnablePred = HI_FALSE;
        if (pVideo_para->ref_para.pred_enable == 1)
        {
            venc_param_ref.bEnablePred = HI_TRUE;
        }
        venc_param_ref.u32Base = pVideo_para->ref_para.base_interval;
        venc_param_ref.u32Enhance = pVideo_para->ref_para.enhance_interval;
    }
    else
    {
        venc_param_ref.bEnablePred = HI_TRUE;
        if (_ART_REF_FOR_2X == pVideo_para->ref_para.ref_type)
        {
            venc_param_ref.u32Base = 1;
            venc_param_ref.u32Enhance = 1;
        }
        else if (_ART_REF_FOR_4X == pVideo_para->ref_para.ref_type)
        {
            venc_param_ref.u32Base = 2;
            venc_param_ref.u32Enhance = 1;
        }
        else
        {
            venc_param_ref.u32Base = 0;
            venc_param_ref.u32Enhance = 1;
        }
    }

    if((ret_val = HI_MPI_VENC_SetRefParam(venc_chn, &venc_param_ref)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h265_encoder HI_MPI_VENC_SetRefParam FAILED(type:%d)(chn:%d)(venc chn: %d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num,  venc_chn,  (unsigned long)ret_val);
        return -1;
    }
    
    if((ret_val = HI_MPI_VENC_StartRecvPic(venc_chn)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h265_encoder HI_MPI_VENC_StartRecvPic FAILED(type:%d)(chn:%d)(venc chn: %d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, venc_chn, (unsigned long)ret_val);
        return -1;
    }

#if defined(_AV_HISILICON_VPSS_)
#if defined(_AV_SUPPORT_IVE_)
    if(1 == pgAvDeviceInfo->Adi_get_ive_debug_mode())
    {
#if defined(_AV_IVE_FD_)
        if( video_stream_type != _AST_MAIN_VIDEO_)
#else
        if( video_stream_type != _AST_SUB_VIDEO_)
#endif
        {
            if(Ae_venc_vpss_control(video_stream_type, phy_video_chn_num, true) != 0)
            {
                DEBUG_ERROR( "CHiAvEncoder::Ae_create_h265_encoder Ae_venc_vpss_control FAILED(type:%d)(chn:%d)(venc chn: %d)\n", video_stream_type, phy_video_chn_num, venc_chn);
                return -1;
            }        
        }
    }
    else
    {
        if(Ae_venc_vpss_control(video_stream_type, phy_video_chn_num, true) != 0)
        {
            DEBUG_ERROR( "CHiAvEncoder::Ae_create_h265_encoder Ae_venc_vpss_control FAILED(type:%d)(chn:%d)(venc chn: %d)\n", video_stream_type, phy_video_chn_num, venc_chn);
            return -1;
        }
    }
#else
    if(Ae_venc_vpss_control(video_stream_type, phy_video_chn_num, true) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h265_encoder Ae_venc_vpss_control FAILED(type:%d)(chn:%d)(venc chn: %d)\n", video_stream_type, phy_video_chn_num, venc_chn);
        return -1;
    }
#endif
#endif

    memset(pVideo_encoder_id, 0, sizeof(video_encoder_id_t));
    pVideo_encoder_id->video_encoder_fd = HI_MPI_VENC_GetFd(venc_chn);
    hi_video_encoder_info_t *pVei = NULL;
    pVei = new hi_video_encoder_info_t;
    _AV_FATAL_(pVei == NULL, "CHiAvEncoder::Ae_create_h265_encoder OUT OF MEMORY\n");
    memset(pVei, 0, sizeof(hi_video_encoder_info_t));
    pVei->venc_chn = venc_chn;
    pVideo_encoder_id->pVideo_encoder_info = (void *)pVei;

    //DEBUG_MESSAGE( "CHiAvEncoder::Ae_create_h265_encoder(type:%d)(chn:%d)(res:%d)(frame_rate:%d)(bitrate:%d)(gop:%d)(fd:%d)\n", video_stream_type, phy_video_chn_num, pVideo_para->resolution, pVideo_para->frame_rate, pVideo_para->bitrate.hicbr.bitrate, pVideo_para->gop_size, pVideo_encoder_id->video_encoder_fd);

    return 0;
}

int CHiAvEncoder::Ae_video_encoder_allotter_V1(av_video_stream_type_t video_stream_type, int phy_video_chn_num, VENC_CHN *pChn)
{   
    switch(video_stream_type)
    {
        case _AST_MAIN_VIDEO_:
            _AV_POINTER_ASSIGNMENT_(pChn, phy_video_chn_num);
            break;

        case _AST_SUB_VIDEO_:
            _AV_POINTER_ASSIGNMENT_(pChn, phy_video_chn_num + pgAvDeviceInfo->Adi_get_vi_chn_num());
            break;

        case _AST_SUB2_VIDEO_:
            _AV_POINTER_ASSIGNMENT_(pChn, phy_video_chn_num + pgAvDeviceInfo->Adi_get_vi_chn_num() * 2);
            break;

        default:
            _AV_FATAL_(1, "CHiAvEncoder::Ae_video_encoder_allotter_V1 You must give the implement\n");
            break;
    }

    return 0;
}

int CHiAvEncoder::Ae_video_encoder_allotter_MCV1(av_video_stream_type_t video_stream_type, int phy_video_chn_num, VENC_CHN *pChn)
{
    return -1;
}

int CHiAvEncoder::Ae_video_encoder_allotter(av_video_stream_type_t video_stream_type, int phy_video_chn_num, VENC_CHN *pChn)
{
    int retval = -1;

    retval = Ae_video_encoder_allotter_V1(video_stream_type, phy_video_chn_num, pChn);

    return retval;
}

int CHiAvEncoder::Ae_audio_encoder_allotter_V1(av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, AENC_CHN *pAenc_chn)
{
    switch(audio_stream_type)
    {
        case _AAST_NORMAL_:
            _AV_POINTER_ASSIGNMENT_(pAenc_chn, phy_audio_chn_num);
            break;

        case _AAST_TALKBACK_:
            _AV_POINTER_ASSIGNMENT_(pAenc_chn, pgAvDeviceInfo->Adi_max_channel_number());
            break;

        default:
            _AV_FATAL_(1, "CHiAvEncoder::Ae_audio_encoder_allotter_V1 You must give the implement\n");
            break;
    }

    return 0;
}

int CHiAvEncoder::Ae_video_encoder_allotter_snap( VENC_CHN *pChn )
{
    _AV_POINTER_ASSIGNMENT_(pChn, pgAvDeviceInfo->Adi_get_vi_chn_num() * 3 + 1 );

    return 0;
}

int CHiAvEncoder::Ae_audio_encoder_allotter_MCV1(av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, AENC_CHN *pAenc_chn)
{
    return -1;
}

int CHiAvEncoder::Ae_audio_encoder_allotter(av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, AENC_CHN *pAenc_chn)
{
    int retval = -1;
    
    if(0)
    {
        retval = Ae_audio_encoder_allotter_MCV1(audio_stream_type, phy_audio_chn_num, pAenc_chn);
    }
    else
    {
        retval = Ae_audio_encoder_allotter_V1(audio_stream_type, phy_audio_chn_num, pAenc_chn);
    }

    return retval;
}

int CHiAvEncoder::Ae_get_audio_stream(audio_encoder_id_t *pAudio_encoder_id, av_stream_data_t *pStream_data, bool blockflag/*= true*/)
{
    hi_audio_encoder_info_t *pAei = (hi_audio_encoder_info_t *)pAudio_encoder_id->pAudio_encoder_info;
    HI_S32 ret_val = -1;
    HI_BOOL bBlock = HI_TRUE;

    if (false == blockflag)
    {
        bBlock = HI_FALSE;
    }

    if((ret_val = HI_MPI_AENC_GetStream(pAei->aenc_chn, &pAei->audio_stream, bBlock)) != HI_SUCCESS)
    {
        if ((unsigned long)ret_val != 0xa017800e)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_get_audio_stream HI_MPI_AENC_GetStream FAILED(0x%08lx)\n", (unsigned long)ret_val);
        }
        return -1;
    }

    memset(pStream_data, 0, sizeof(av_stream_data_t));
    pStream_data->frame_type = _AFT_AUDIO_FRAME_;
    pStream_data->frame_len = pAei->audio_stream.u32Len;
    pStream_data->pack_number = 1;
    pStream_data->pack_list[0].pAddr[0] = pAei->audio_stream.pStream;
    pStream_data->pack_list[0].len[0] = pAei->audio_stream.u32Len;
    pStream_data->pts = pAei->audio_stream.u64TimeStamp;
    return 0;
}


int CHiAvEncoder::Ae_release_audio_stream(audio_encoder_id_t *pAudio_encoder_id, av_stream_data_t *pStream_data)
{
    hi_audio_encoder_info_t *pAei = (hi_audio_encoder_info_t *)pAudio_encoder_id->pAudio_encoder_info;
    HI_S32 ret_val = -1;

    if((ret_val = HI_MPI_AENC_ReleaseStream(pAei->aenc_chn, &pAei->audio_stream)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_get_audio_stream HI_MPI_AENC_ReleaseStream FAILED(0x%08lx)\n", (unsigned long)ret_val);
        return -1;
    }

    return 0;
}


int CHiAvEncoder::Ae_get_video_stream(video_encoder_id_t *pVideo_encoder_id, av_stream_data_t *pStream_data, bool blockflag/*= true*/)
{
    VENC_CHN_STAT_S venc_chn_state;
    hi_video_encoder_info_t *pVei = (hi_video_encoder_info_t *)pVideo_encoder_id->pVideo_encoder_info;
    HI_S32 ret_val = -1;
    HI_BOOL bBlock = HI_TRUE;

    if (false == blockflag)
    {
        bBlock = HI_FALSE;
    }

    memset(pStream_data, 0, sizeof(av_stream_data_t));

    if((ret_val = HI_MPI_VENC_Query(pVei->venc_chn, &venc_chn_state)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_get_video_stream HI_MPI_VENC_Query FAILED(0x%08lx)\n", (unsigned long)ret_val);
        return -1;
    }

    if(pVei->pack_number < (int)venc_chn_state.u32CurPacks)
    {
        if(pVei->pPack_buffer != NULL)
        {
            delete []pVei->pPack_buffer;
        }

        pVei->pack_number = venc_chn_state.u32CurPacks * 2;
        pVei->pPack_buffer = new VENC_PACK_S[pVei->pack_number];
    }

    pVei->venc_stream.u32PackCount = venc_chn_state.u32CurPacks;
    pVei->venc_stream.pstPack = pVei->pPack_buffer;
    
    if((ret_val = HI_MPI_VENC_GetStream(pVei->venc_chn, &pVei->venc_stream, bBlock)) != HI_SUCCESS)
    {
        if ((unsigned long)ret_val != 0xa007800e)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_get_video_stream chn %d HI_MPI_VENC_GetStream FAILED(0x%08lx)\n", pVei->venc_chn, (unsigned long)ret_val);
        }
        return -1;
    }

    if(pVei->venc_stream.u32PackCount > _AV_MAX_PACK_NUM_ONEFRAME_)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_get_video_stream TOO LENGTH(%d)\n", pVei->venc_stream.u32PackCount);
        Ae_release_video_stream(pVideo_encoder_id, pStream_data);
        return -1;
    }

    pStream_data->pts = pVei->venc_stream.pstPack[0].u64PTS;
    pStream_data->pack_number = pVei->venc_stream.u32PackCount;
    pStream_data->frame_len = 0;
    for(int i = 0; i < (int)pVei->venc_stream.u32PackCount; i ++)
    {
        pStream_data->pack_list[i].pAddr[0] = pVei->venc_stream.pstPack[i].pu8Addr+pVei->venc_stream.pstPack[i].u32Offset;
        pStream_data->pack_list[i].len[0] = pVei->venc_stream.pstPack[i].u32Len - pVei->venc_stream.pstPack[i].u32Offset;
        pStream_data->frame_len += pVei->venc_stream.pstPack[i].u32Len - pVei->venc_stream.pstPack[i].u32Offset;
    }

    if(pStream_data->frame_len < 5)
    {
        DEBUG_WARNING( "CHiAvEncoder::Ae_get_video_stream TOO SHORT(%d)\n", pStream_data->frame_len);
        Ae_release_video_stream(pVideo_encoder_id, pStream_data);
        return -1;
    }
    
#if 1
    unsigned char frame_type_byte = 0;
    if(pStream_data->pack_list[0].len[0])
    {
        frame_type_byte = *(pStream_data->pack_list[0].pAddr[0] + 4);    
    }
    else
    {
        DEBUG_ERROR("CHiAvEncoder::Ae_get_video_stream the frame head info is invalid!len is:%d \n", pStream_data->pack_list[0].len[0]);
    }

    switch(frame_type_byte)
    {
        /*H264 NALU I frame:SPS-PPS-Islice, SPS nal_unit_type:7. H265 NALU I frame:VPS-SPS-PPS-Islice. VPS nal_unit_type:32*/
        case 0x67:/*H264*/
        case 0x40:/*H265*/
            pStream_data->frame_type = _AFT_IFRAME_;
            break;

        default:
            pStream_data->frame_type = _AFT_PFRAME_;
            break;
    }
#endif

#if 0
    if(pVei->venc_chn == 0)
   {
        static FILE* fp =NULL;
        if(NULL == fp)
        {
            fp = fopen("./stream/stream_hisi.h265", "wb");
        }

        if(NULL != fp)
        {
            for(unsigned int i=0;i< pVei->venc_stream.u32PackCount; ++i)
            {
                fwrite(pVei->venc_stream.pstPack[i].pu8Addr+pVei->venc_stream.pstPack[i].u32Offset, pVei->venc_stream.pstPack[i].u32Len - pVei->venc_stream.pstPack[i].u32Offset, 1, fp);
                fflush(fp);
            }
        }
    }
#endif
#if 0
    if(pVei->venc_chn == 0)
    {
        static FILE* fp =NULL;
        if(NULL == fp)
        {
            fp = fopen("./stream/stream_sx.h265", "wb");
        }

        if(NULL != fp)
        {
            for( int i=0;i< pStream_data->pack_number; ++i)
            {
                if(pStream_data->pack_list[i].len[0] > 0)
                {
                    fwrite(pStream_data->pack_list[i].pAddr[0], pStream_data->pack_list[i].len[0], 1, fp);
                    fflush(fp);
                }

                if(pStream_data->pack_list[i].len[1] > 0)
                {
                    fwrite(pStream_data->pack_list[i].pAddr[1], pStream_data->pack_list[i].len[1], 1, fp);
                    fflush(fp);
                }
            }
        }
    }
#endif
    return 0;
}


int CHiAvEncoder::Ae_release_video_stream(video_encoder_id_t *pVideo_encoder_id, av_stream_data_t *pStream_data)
{
    hi_video_encoder_info_t *pVei = (hi_video_encoder_info_t *)pVideo_encoder_id->pVideo_encoder_info;
    HI_S32 ret_val = -1;

    if((ret_val = HI_MPI_VENC_ReleaseStream(pVei->venc_chn, &pVei->venc_stream)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_release_video_stream HI_MPI_VENC_ReleaseStream FAILED(0x%08lx)\n", (unsigned long)ret_val);
        return -1;
    }

    return 0;
}


int CHiAvEncoder::Ae_request_iframe_local(av_video_stream_type_t video_stream_type, int phy_video_chn_num)
{
    VENC_CHN venc_chn = -1;

    if(Ae_video_encoder_allotter(video_stream_type, phy_video_chn_num, &venc_chn) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_request_iframe_local Ae_video_encoder_allotter FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
        return -1;
    }

    HI_S32 ret_val = HI_MPI_VENC_RequestIDR(venc_chn, HI_FALSE);
    if(ret_val != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_request_iframe_local HI_MPI_VENC_RequestIDR FAILED(0x%08x)\n", ret_val);
        return -1;
    }

    DEBUG_MESSAGE( "CHiAvEncoder::Ae_request_iframe_local OK(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);

    return 0;
}

int CHiAvEncoder::Ae_create_osd_region(std::list<av_osd_item_t>::iterator &av_osd_item_it)
{
    RGN_ATTR_S region_attr;
    RGN_CHN_ATTR_S region_chn_attr;
    MPP_CHN_S mpp_chn;
    RGN_HANDLE region_handle;
    hi_osd_region_t *pOsd_region = NULL;
    hi_video_encoder_info_t *pVideo_encoder_info = (hi_video_encoder_info_t *)av_osd_item_it->video_stream.video_encoder_id.pVideo_encoder_info;
    HI_S32 ret_val = 0;
    int char_number = 0;
    int font_width = 0;
    int font_height = 0;
    int font_name = 0;
    int horizontal_scaler = 0;
    int vertical_scaler = 0;

    /*calculate the size of region*/
    Ae_get_osd_parameter(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution, &font_name, 
                                    &font_width, &font_height, &horizontal_scaler, &vertical_scaler);

    //DEBUG_MESSAGE( "CHiAvEncoder::Ae_create_osd_region(channel:%d)(osd_name:%d)(w:%d)(h:%d)(ws:%d)(hs:%d)\n", av_osd_item_it->video_stream.phy_video_chn_num, av_osd_item_it->osd_name, font_width, font_height, horizontal_scaler, vertical_scaler);

    if(av_osd_item_it->osd_name == _AON_DATE_TIME_)
    {
        if(av_osd_item_it->video_stream.video_encoder_para.time_mode == _AV_TIME_MODE_12_)
        {
            char_number = strlen("2013-03-23 11:10:00 AM");
        }
        else
        {
            char_number = strlen("2013-03-23 11:10:00");
        }

        switch(av_osd_item_it->video_stream.video_encoder_para.resolution)
        {
            case _AR_1080_:
            case _AR_720_:
            case _AR_960P_:
                region_attr.unAttr.stOverlay.stSize.u32Width  = ((font_width * char_number * horizontal_scaler) / 2 + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
            default:
                region_attr.unAttr.stOverlay.stSize.u32Width  = ((font_width * char_number * horizontal_scaler) * 2 / 3 + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
        }
    }
    else if(av_osd_item_it->osd_name == _AON_CHN_NAME_)
    {
        region_attr.unAttr.stOverlay.stSize.u32Width = 0;
        char* channelname = NULL;
        channelname = av_osd_item_it->video_stream.video_encoder_para.channel_name;

        CAvUtf8String channel_name(channelname);
        av_ucs4_t char_unicode;
        av_char_lattice_info_t lattice_char_info;
        while(channel_name.Next_char(&char_unicode) == true)
        {
            if(m_pAv_font_library->Afl_get_char_lattice(font_name, char_unicode, &lattice_char_info) == 0)
            {
                region_attr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
            }
            else
            {
                DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, font_name);
                return -1;
            }
        }
        if(region_attr.unAttr.stOverlay.stSize.u32Width == 0)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }
        region_attr.unAttr.stOverlay.stSize.u32Width  = (region_attr.unAttr.stOverlay.stSize.u32Width * horizontal_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
    }
    else if(av_osd_item_it->osd_name == _AON_BUS_NUMBER_)    
    { 
        region_attr.unAttr.stOverlay.stSize.u32Width = 0;
        char_number = 10;

        switch(av_osd_item_it->video_stream.video_encoder_para.resolution)
        {
            case _AR_1080_:
            case _AR_720_:
            case _AR_960P_:
                region_attr.unAttr.stOverlay.stSize.u32Width  = ((font_width * char_number * horizontal_scaler) / 2 + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
            default:
                region_attr.unAttr.stOverlay.stSize.u32Width  = ((font_width * char_number * horizontal_scaler)*2/3  + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
        }

        if(region_attr.unAttr.stOverlay.stSize.u32Width == 0)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }

    }    
    else if(av_osd_item_it->osd_name == _AON_SELFNUMBER_INFO_)    
    {        
        region_attr.unAttr.stOverlay.stSize.u32Width = 0;
        char_number = 10;

        switch(av_osd_item_it->video_stream.video_encoder_para.resolution)
        {
            case _AR_1080_:
            case _AR_720_:
            case _AR_960P_:
                region_attr.unAttr.stOverlay.stSize.u32Width  = ((font_width * char_number * horizontal_scaler) / 2 + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
            default:
                region_attr.unAttr.stOverlay.stSize.u32Width  = ((font_width * char_number * horizontal_scaler)*2/3  + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
        }

        if(region_attr.unAttr.stOverlay.stSize.u32Width == 0)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }
        region_attr.unAttr.stOverlay.stSize.u32Width  = (region_attr.unAttr.stOverlay.stSize.u32Width * horizontal_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
    }    	
    else if(av_osd_item_it->osd_name == _AON_GPS_INFO_)
    {        
        region_attr.unAttr.stOverlay.stSize.u32Width = 0;
        char_number = 32;

        switch(av_osd_item_it->video_stream.video_encoder_para.resolution)
        {
            case _AR_1080_:
            case _AR_720_:
            case _AR_960P_:
                region_attr.unAttr.stOverlay.stSize.u32Width  = ((font_width * char_number * horizontal_scaler) / 2 + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
            default:
                region_attr.unAttr.stOverlay.stSize.u32Width  = ((font_width * char_number * horizontal_scaler)*2/3  + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
        }
    }    
    else if(av_osd_item_it->osd_name == _AON_SPEED_INFO_)
    {        
        region_attr.unAttr.stOverlay.stSize.u32Width = 0;
        char_number = 16;
        switch(av_osd_item_it->video_stream.video_encoder_para.resolution)
        {
            case _AR_1080_:
            case _AR_720_:
            case _AR_960P_:
                region_attr.unAttr.stOverlay.stSize.u32Width  = ((font_width * char_number * horizontal_scaler) / 2 + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
            default:
                region_attr.unAttr.stOverlay.stSize.u32Width  = ((font_width * char_number * horizontal_scaler)*2/3  + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
        }
    }    
    else if(_AON_WATER_MARK == av_osd_item_it->osd_name)
    {
        region_attr.unAttr.stOverlay.stSize.u32Width = 0;
        char* watermarkname = av_osd_item_it->video_stream.video_encoder_para.water_mark_name;
        CAvUtf8String channel_name(watermarkname);
        av_ucs4_t char_unicode;
        av_char_lattice_info_t lattice_char_info;
        while(channel_name.Next_char(&char_unicode) == true)
        {
            if(m_pAv_font_library->Afl_get_char_lattice(font_name, char_unicode, &lattice_char_info) == 0)
            {
                region_attr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
            }
            else
            {
                DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, font_name);
                return -1;
            }
        }
        if(region_attr.unAttr.stOverlay.stSize.u32Width == 0)
        {
            DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }
        region_attr.unAttr.stOverlay.stSize.u32Width  = (region_attr.unAttr.stOverlay.stSize.u32Width * horizontal_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
    }
    else if(av_osd_item_it->osd_name == _AON_COMMON_OSD1_)
    {
#if defined(_AV_SUPPORT_IVE_)
        region_attr.unAttr.stOverlay.stSize.u32Width = 0;
        char_number = 32; //!< max 
        switch(av_osd_item_it->video_stream.video_encoder_para.resolution)
        {
            case _AR_1080_:
            case _AR_720_:
            case _AR_960P_:
            region_attr.unAttr.stOverlay.stSize.u32Width  = ((font_width * char_number * horizontal_scaler) / 2 + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
            region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_ ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
            default:
                region_attr.unAttr.stOverlay.stSize.u32Width  = ((font_width * char_number * horizontal_scaler)*2/3  + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_ ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
        }
#else
        region_attr.unAttr.stOverlay.stSize.u32Width = 0;
        char* user_define_osd1 = av_osd_item_it->video_stream.video_encoder_para.osd1_content;
        CAvUtf8String osd1(user_define_osd1);
        av_ucs4_t char_unicode;
        av_char_lattice_info_t lattice_char_info;
        while(osd1.Next_char(&char_unicode) == true)
        {
            if(m_pAv_font_library->Afl_get_char_lattice(font_name, char_unicode, &lattice_char_info) == 0)
            {
                region_attr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
            }
            else
            {
                _AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, font_name);
                return -1;
            }
        }
        if(region_attr.unAttr.stOverlay.stSize.u32Width == 0)
        {
            //_AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }
        region_attr.unAttr.stOverlay.stSize.u32Width  = (region_attr.unAttr.stOverlay.stSize.u32Width * horizontal_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;        
#endif
    }
    else if(av_osd_item_it->osd_name == _AON_COMMON_OSD2_)
    {
#if defined(_AV_SUPPORT_IVE_)
        region_attr.unAttr.stOverlay.stSize.u32Width = 0;
        char_number = 32; //!< max 
        switch(av_osd_item_it->video_stream.video_encoder_para.resolution)
        {
            case _AR_1080_:
            case _AR_720_:
            case _AR_960P_:
            region_attr.unAttr.stOverlay.stSize.u32Width  = ((font_width * char_number * horizontal_scaler) / 2 + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
            region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_ ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
            default:
                region_attr.unAttr.stOverlay.stSize.u32Width  = ((font_width * char_number * horizontal_scaler)*2/3  + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_ ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
        }
#else
        region_attr.unAttr.stOverlay.stSize.u32Width = 0;
        char* user_define_osd1 = av_osd_item_it->video_stream.video_encoder_para.osd2_content;
        CAvUtf8String osd2(user_define_osd1);
        av_ucs4_t char_unicode;
        av_char_lattice_info_t lattice_char_info;
        while(osd2.Next_char(&char_unicode) == true)
        {
            if(m_pAv_font_library->Afl_get_char_lattice(font_name, char_unicode, &lattice_char_info) == 0)
            {
                region_attr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
            }
            else
            {
                _AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, font_name);
                return -1;
            }
        }
        if(region_attr.unAttr.stOverlay.stSize.u32Width == 0)
        {
            return -1;
        }
        region_attr.unAttr.stOverlay.stSize.u32Width  = (region_attr.unAttr.stOverlay.stSize.u32Width * horizontal_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;        
#endif
    }
    else
    {
        DEBUG_WARNING( "CHiAvEncoder::Ae_create_osd_region You must give the implement\n");
        return -1;
    }
    int video_width = 0;
    int video_height = 0;

    pgAvDeviceInfo->Adi_get_video_size(av_osd_item_it->video_stream.video_encoder_para.resolution, av_osd_item_it->video_stream.video_encoder_para.tv_system, &video_width, &video_height);

    if(region_attr.unAttr.stOverlay.stSize.u32Width > (unsigned int)video_width)
    {
        region_attr.unAttr.stOverlay.stSize.u32Width = video_width;
    }

    if(region_attr.unAttr.stOverlay.stSize.u32Height >  (unsigned int)video_height)
    {
        region_attr.unAttr.stOverlay.stSize.u32Height = video_height;
    }
    region_attr.unAttr.stOverlay.stSize.u32Width = region_attr.unAttr.stOverlay.stSize.u32Width/_AE_OSD_ALIGNMENT_BYTE_*_AE_OSD_ALIGNMENT_BYTE_;
    region_attr.unAttr.stOverlay.stSize.u32Height  = region_attr.unAttr.stOverlay.stSize.u32Height/_AE_OSD_ALIGNMENT_BYTE_*_AE_OSD_ALIGNMENT_BYTE_;

    /*create region*/
    region_attr.enType = OVERLAY_RGN;
    region_attr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
    region_attr.unAttr.stOverlay.u32BgColor = 0x00;
    region_handle = Har_get_region_handle_encoder(av_osd_item_it->video_stream.type, av_osd_item_it->video_stream.phy_video_chn_num, av_osd_item_it->osd_name, &region_handle);

    if(HI_MPI_RGN_Destroy(region_handle) == 0)
    {
        if((ret_val = HI_MPI_RGN_Create(region_handle, &region_attr)) != HI_SUCCESS)
        {
            DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region HI_MPI_RGN_Create FAILED(0x%08x)(type:%d)(chn:%d)(region_handle:%d) av_osd_item_it.name:%d\n", ret_val, av_osd_item_it->video_stream.type, av_osd_item_it->video_stream.phy_video_chn_num, region_handle,av_osd_item_it->osd_name);
            return -1;
        }
    }
    else
    {
        if((ret_val = HI_MPI_RGN_Create(region_handle, &region_attr)) != HI_SUCCESS)
        {
            DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region HI_MPI_RGN_Create FAILED(0x%08x)(type:%d)(chn:%d)(region_handle:%d) av_osd_item_it.name:%d\n", ret_val, av_osd_item_it->video_stream.type, av_osd_item_it->video_stream.phy_video_chn_num, region_handle,av_osd_item_it->osd_name);
            return -1;
        }
    }
    /*attach to channel*/
    memset(&region_chn_attr, 0, sizeof(RGN_CHN_ATTR_S));
    region_chn_attr.bShow = HI_TRUE;
    region_chn_attr.enType = OVERLAY_RGN;

    Ae_translate_coordinate(av_osd_item_it, region_attr.unAttr.stOverlay.stSize.u32Width, region_attr.unAttr.stOverlay.stSize.u32Height);

    region_chn_attr.unChnAttr.stOverlayChn.stPoint.s32X = av_osd_item_it->display_x/_AE_OSD_ALIGNMENT_BYTE_*_AE_OSD_ALIGNMENT_BYTE_;
    region_chn_attr.unChnAttr.stOverlayChn.stPoint.s32Y = av_osd_item_it->display_y/_AE_OSD_ALIGNMENT_BYTE_*_AE_OSD_ALIGNMENT_BYTE_;

    region_chn_attr.unChnAttr.stOverlayChn.u32BgAlpha = 0;
    region_chn_attr.unChnAttr.stOverlayChn.u32FgAlpha = 128;
    region_chn_attr.unChnAttr.stOverlayChn.u32Layer = 0;

    if(_AON_WATER_MARK == av_osd_item_it->osd_name)
    {
        region_chn_attr.unChnAttr.stOverlayChn.u32FgAlpha = 32;
    }

    region_chn_attr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
    region_chn_attr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;

    region_chn_attr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = HI_FALSE;
    region_chn_attr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Width = 16;
    region_chn_attr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Height = 16;
    region_chn_attr.unChnAttr.stOverlayChn.stInvertColor.u32LumThresh = 150;
    region_chn_attr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = MORETHAN_LUM_THRESH;

    mpp_chn.enModId = HI_ID_VENC;
    mpp_chn.s32DevId = 0;
    mpp_chn.s32ChnId = pVideo_encoder_info->venc_chn;

    if((ret_val = HI_MPI_RGN_AttachToChn(region_handle, &mpp_chn, &region_chn_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region HI_MPI_RGN_AttachToChn FAILED(0x%08x)\n", ret_val);
        HI_MPI_RGN_Destroy(region_handle);
        return -1;
    }

    /*bitmap*/
    av_osd_item_it->bitmap_width = region_attr.unAttr.stOverlay.stSize.u32Width;
    av_osd_item_it->bitmap_height = region_attr.unAttr.stOverlay.stSize.u32Height;
    av_osd_item_it->pBitmap = new unsigned char[av_osd_item_it->bitmap_width * horizontal_scaler * av_osd_item_it->bitmap_height * vertical_scaler * 2];
    _AV_FATAL_(av_osd_item_it->pBitmap == NULL, "OUT OF MEMORY\n");

    /*region*/
    _AV_FATAL_((pOsd_region = new hi_osd_region_t) == NULL, "OUT OF MEMORY\n");
    pOsd_region->region_handle = region_handle;
    av_osd_item_it->pOsd_region = (void *)pOsd_region;

    return 0;
}

int CHiAvEncoder::Ae_osd_overlay(std::list<av_osd_item_t>::iterator &av_osd_item_it)
{
    hi_osd_region_t *pOsd_region = (hi_osd_region_t *)av_osd_item_it->pOsd_region;
    int font_width = 0;
    int font_height = 0;
    int font_name = 0;
    int horizontal_scaler = 0;
    int vertical_scaler = 0;
    HI_S32 ret_val = -1;
    char date_time_string[64];
#ifndef _AV_PRODUCT_CLASS_IPC_
    char string[32];
    char gpsstring[64] = {0};
    //strcpy(gpsstring,GuiTKGetText(NULL, NULL, "STR_GPS"));
    char speedstring[16] = {0};
    //char tempstring[128] = {0};
#endif
    char *pUtf8_string = NULL;
  
    if(av_osd_item_it->osd_name == _AON_DATE_TIME_)
    {        
        datetime_t date_time;

        pgAvDeviceInfo->Adi_get_date_time(&date_time);
        switch(av_osd_item_it->video_stream.video_encoder_para.date_mode)
        {
            case _AV_DATE_MODE_yymmdd_:
            default:
            sprintf(date_time_string, "20%02d-%02d-%02d", date_time.year, date_time.month, date_time.day);
            break;
            case _AV_DATE_MODE_ddmmyy_:
            sprintf(date_time_string, "%02d/%02d/20%02d", date_time.day, date_time.month, date_time.year);
            break;
            case _AV_DATE_MODE_mmddyy_:
            sprintf(date_time_string, "%02d/%02d/20%02d", date_time.month, date_time.day, date_time.year);
            break;
        }
        switch(av_osd_item_it->video_stream.video_encoder_para.time_mode)
        {
            case _AV_TIME_MODE_24_:
            default:
                sprintf(date_time_string, "%s %02d:%02d:%02d", date_time_string, date_time.hour, date_time.minute, date_time.second);
                break;
            case _AV_TIME_MODE_12_:
                if (date_time.hour > 12)
                {
                    sprintf(date_time_string, "%s %02d:%02d:%02d PM", date_time_string, date_time.hour - 12, date_time.minute, date_time.second);
                }
                else if(date_time.hour == 12)
                {
                    sprintf(date_time_string, "%s %02d:%02d:%02d PM", date_time_string, date_time.hour, date_time.minute, date_time.second);
                }
                else if(date_time.hour == 0)
                {
                    sprintf(date_time_string, "%s 12:%02d:%02d AM", date_time_string,  date_time.minute, date_time.second);
                }
                else
                {
                    sprintf(date_time_string, "%s %02d:%02d:%02d AM", date_time_string, date_time.hour, date_time.minute, date_time.second);                  
                }                
                break;
        }
        pUtf8_string = date_time_string;        
    }
    else if(av_osd_item_it->osd_name == _AON_CHN_NAME_)
    {
        pUtf8_string = av_osd_item_it->video_stream.video_encoder_para.channel_name;   
    }
    else if(av_osd_item_it->osd_name == _AON_BUS_NUMBER_)
    {
        pUtf8_string = av_osd_item_it->video_stream.video_encoder_para.bus_number;    
    }
    else if(av_osd_item_it->osd_name == _AON_SELFNUMBER_INFO_)
    {
        pUtf8_string = av_osd_item_it->video_stream.video_encoder_para.bus_selfnumber;    
    }	
    else if(av_osd_item_it->osd_name == _AON_SPEED_INFO_)
    {
        pUtf8_string = av_osd_item_it->video_stream.video_encoder_para.speed;
    }
    else if(av_osd_item_it->osd_name == _AON_GPS_INFO_)
    {
        pUtf8_string = av_osd_item_it->video_stream.video_encoder_para.gps;
    }
    else if(_AON_WATER_MARK == av_osd_item_it->osd_name)
    {
        pUtf8_string = av_osd_item_it->video_stream.video_encoder_para.water_mark_name; 
    }
    else if(av_osd_item_it->osd_name == _AON_COMMON_OSD1_)
    {
        pUtf8_string = av_osd_item_it->video_stream.video_encoder_para.osd1_content;
    }
    else if(av_osd_item_it->osd_name == _AON_COMMON_OSD2_)
     {
        pUtf8_string = av_osd_item_it->video_stream.video_encoder_para.osd2_content;
     }
    else
    {
        DEBUG_WARNING( "CHiAvEncoder::Ae_osd_overlay You must give the implement\n");
        return -1;
    }
    Ae_get_osd_parameter(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution, &font_name, &font_width, &font_height, &horizontal_scaler, &vertical_scaler);
    if(Ae_utf8string_2_bmp(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution,
        av_osd_item_it->pBitmap, av_osd_item_it->bitmap_width, av_osd_item_it->bitmap_height, pUtf8_string, 0xffff, 0x00, 0x8000) != 0)
    {
        DEBUG_ERROR("erooroeoooofooe\n");
        return -1;
    }
    
    BITMAP_S bitmap;
    
    bitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
    bitmap.u32Width = av_osd_item_it->bitmap_width;
    bitmap.u32Height = av_osd_item_it->bitmap_height;    
    bitmap.pData = av_osd_item_it->pBitmap;
    if((ret_val = HI_MPI_RGN_SetBitMap(pOsd_region->region_handle, &bitmap)) != HI_SUCCESS)
    {        
        DEBUG_ERROR( "CHiAvEncoder::Ae_osd_overlay HI_MPI_RGN_SetBitMap FAILED(region_handle:0x%08x)(%d)\n", ret_val, pOsd_region->region_handle);
        return -1;
    }
	
#if defined(_AV_PRODUCT_CLASS_IPC_)
    if(av_osd_item_it->osd_name != _AON_DATE_TIME_)
    {
        av_osd_item_it->is_update = false;
    }
#endif

    return 0;
}

int CHiAvEncoder::Ae_destroy_osd_region(std::list<av_osd_item_t>::iterator &av_osd_item_it)
{
    if(av_osd_item_it->overlay_state == _AOS_INIT_)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destroy_osd_region (av_osd_item_it->overlay_state == _AOS_INIT_)\n");
        return 0;
    }
    hi_osd_region_t *pOsd_region = (hi_osd_region_t *)av_osd_item_it->pOsd_region;
    HI_S32 ret_val = -1;

    if((ret_val = HI_MPI_RGN_Destroy(pOsd_region->region_handle)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destroy_osd_region HI_MPI_RGN_Destroy FAILED(0x%08x)\n", ret_val);
        return -1;
    }
    if(av_osd_item_it->pBitmap != NULL)
    {
        delete [](char *)av_osd_item_it->pBitmap;
        av_osd_item_it->pBitmap = NULL;
    }
    if(pOsd_region != NULL)
    {
        delete pOsd_region;
        av_osd_item_it->pOsd_region = NULL;
    }
    return 0;
}

int CHiAvEncoder::Ae_get_H264_info(const void *pVideo_encoder_info, RMFI2_H264INFO * ph264_info)
{
    hi_video_encoder_info_t *pInfo = (hi_video_encoder_info_t *)pVideo_encoder_info;
    
    ph264_info->lPicBytesNum     = pInfo->venc_stream.stH264Info.u32PicBytesNum;
    ph264_info->lPSkipMbNum      = pInfo->venc_stream.stH264Info.u32PSkipMbNum;
    ph264_info->lIpcmMbNum       = pInfo->venc_stream.stH264Info.u32IpcmMbNum;
    ph264_info->lInter16x8MbNum  = pInfo->venc_stream.stH264Info.u32Inter16x8MbNum;
    ph264_info->lInter16x16MbNum = pInfo->venc_stream.stH264Info.u32Inter16x16MbNum;
    ph264_info->lInter8x16MbNum  = pInfo->venc_stream.stH264Info.u32Inter8x16MbNum;
    ph264_info->lInter8x8MbNum   = pInfo->venc_stream.stH264Info.u32Inter8x8MbNum;
    ph264_info->lIntra16MbNum    = pInfo->venc_stream.stH264Info.u32Intra16MbNum;
    ph264_info->lIntra8MbNum     = pInfo->venc_stream.stH264Info.u32Intra8MbNum;
    ph264_info->lIntra4MbNum     = pInfo->venc_stream.stH264Info.u32Intra4MbNum;
    ph264_info->lRefSliceType    = pInfo->venc_stream.stH264Info.enRefSliceType;
    ph264_info->lRefType         = pInfo->venc_stream.stH264Info.enRefType;
    ph264_info->lReserve         = 0;
    ph264_info->lUpdateAttrCnt   = pInfo->venc_stream.stH264Info.u32UpdateAttrCnt;

    return 0;
}

#if defined(_AV_PLATFORM_HI3516A_)
int CHiAvEncoder::Ae_get_H265_info(const void *pVideo_encoder_info, RMFI2_H265INFO *pH265_info)
{
    hi_video_encoder_info_t *pInfo = (hi_video_encoder_info_t *)pVideo_encoder_info;
    
    pH265_info->lPicBytesNum = pInfo->venc_stream.stH265Info.u32PicBytesNum;
    pH265_info->lInter64x64CuNum = pInfo->venc_stream.stH265Info.u32Inter64x64CuNum;
    pH265_info->lInter32x32CuNum = pInfo->venc_stream.stH265Info.u32Inter32x32CuNum;
    pH265_info->lInter16x16CuNum = pInfo->venc_stream.stH265Info.u32Inter16x16CuNum;
    pH265_info->lInter8x8CuNum = pInfo->venc_stream.stH265Info.u32Inter8x8CuNum;
    pH265_info->lIntra32x32CuNum = pInfo->venc_stream.stH265Info.u32Intra32x32CuNum;
    pH265_info->lIntra16x16CuNum = pInfo->venc_stream.stH265Info.u32Intra16x16CuNum;
    pH265_info->lIntra8x8CuNum = pInfo->venc_stream.stH265Info.u32Intra8x8CuNum;
    pH265_info->lIntra4x4CuNum = pInfo->venc_stream.stH265Info.u32Intra4x4CuNum;
    pH265_info->lRefType = pInfo->venc_stream.stH265Info.enRefType;
    pH265_info->lReserve = 0;
    pH265_info->lUpdateAttrCnt = pInfo->venc_stream.stH264Info.u32UpdateAttrCnt;

    return 0;
}
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_)
#if defined(_AV_HAVE_AUDIO_INPUT_)
int CHiAvEncoder::Ae_ai_aenc_proc(av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num)
{
    if(NULL != m_pAudio_thread_info)
    {
        if(m_pAudio_thread_info->thread_state)
        {
            m_pAudio_thread_info->thread_state = false;
            pthread_join(m_pAudio_thread_info->thread_id, NULL);           
        }
        _AV_SAFE_DELETE_(m_pAudio_thread_info);
    }
    m_pAudio_thread_info = new hi_audio_thread_info_t();

    HI_S32 aenc_chn;
    if(Ae_audio_encoder_allotter(audio_stream_type, phy_audio_chn_num, &aenc_chn) != 0)
    {
        _AV_KEY_INFO_(_AVD_ENC_, "Ae_audio_encoder_allotter FAILED(type:%d)(chn:%d)\n",audio_stream_type, phy_audio_chn_num);
        return -1;
    }   
    m_pAudio_thread_info->audio_stream_type = audio_stream_type;
    m_pAudio_thread_info->phy_audio_chn_id = phy_audio_chn_num;
    m_pAudio_thread_info->aenc_chn = aenc_chn;
    m_pAudio_thread_info->thread_state = true;
    pthread_create(&(m_pAudio_thread_info->thread_id), 0, ai_aenc_thread_body, (void *)this);
    return 0;
}
void *CHiAvEncoder::ai_aenc_thread_body(void* args)
{
    CHiAvEncoder* pAv_encoder = reinterpret_cast<CHiAvEncoder *>(args);
    HI_S32 ai_fd;
    fd_set read_fds;
    struct timeval TimeoutVal;
    HI_S32 nRet; 
    AUDIO_FRAME_S audio_frame;
    AEC_FRAME_S aec_frame;
    memset(&audio_frame, 0, sizeof(AUDIO_FRAME_S));
    memset(&aec_frame, 0, sizeof(AEC_FRAME_S));
    if(NULL == pAv_encoder->m_pHi_ai)
    {
        _AV_KEY_INFO_(_AVD_ENC_, "the ai is null! \n");
        return NULL;
    }
    FD_ZERO(&read_fds);    
    ai_type_e ai_type;
    if(_AAST_NORMAL_ == pAv_encoder->m_pAudio_thread_info->audio_stream_type)
    {
        ai_type = _AI_NORMAL_;
    }
    else if(_AAST_TALKBACK_ == pAv_encoder->m_pAudio_thread_info->audio_stream_type)
    {
        ai_type = _AI_TALKBACK_;
    }
    else
    {
        ai_type = _AI_NORMAL_;
    }
    ai_fd =  pAv_encoder->m_pHi_ai->HiAi_get_fd(ai_type, pAv_encoder->m_pAudio_thread_info->phy_audio_chn_id);
    if(ai_fd < 0)
    {
        _AV_KEY_INFO_(_AVD_ENC_, "get ai fd failed! \n");
        return NULL;
    }
    FD_SET(ai_fd, &read_fds);
    while (pAv_encoder->m_pAudio_thread_info->thread_state)
    {   
        TimeoutVal.tv_sec = 1;
        TimeoutVal.tv_usec = 0;
        FD_ZERO(&read_fds);
        FD_SET(ai_fd,&read_fds);
        nRet = select(ai_fd+1, &read_fds, NULL, NULL, &TimeoutVal);
        if (nRet < 0) 
        {
            _AV_KEY_INFO_(_AVD_ENC_, "get aenc stream select failed! \n");
            sleep(1);
            continue;
        }
        else if (0 == nRet) 
        {
            _AV_KEY_INFO_(_AVD_ENC_, "get aenc stream select time out\n");
            sleep(1);
            continue;
        }
        if (FD_ISSET(ai_fd, &read_fds))
        {
            /* get stream from ai chn */
            nRet = pAv_encoder->m_pHi_ai->HiAi_get_ai_frame(ai_type, pAv_encoder->m_pAudio_thread_info->phy_audio_chn_id, &audio_frame, &aec_frame);
            if (0  != nRet )
            {
                //_AV_KEY_INFO_(_AVD_ENC_, "HiAi_get_ai_frame  failed \n");
                //continue;
            }
            nRet = HI_MPI_AENC_SendFrame( pAv_encoder->m_pAudio_thread_info->aenc_chn, &audio_frame, &aec_frame);
            if(0 != nRet)
            {
                _AV_KEY_INFO_(_AVD_ENC_, "HI_MPI_AENC_SendFrame failed, errorCode:0x%8x \n", nRet);
            }
            /* finally you must release the stream */
            pAv_encoder->m_pHi_ai->HiAi_release_ai_frame(ai_type,  pAv_encoder->m_pAudio_thread_info->phy_audio_chn_id, &audio_frame , NULL);
        }    
    }

        return NULL;
}
#endif

#if defined(_AV_SUPPORT_IVE_)
int CHiAvEncoder::Ae_encode_frame_to_picture(VIDEO_FRAME_INFO_S *pstuViFrame, RECT_S* pstPicture_size, VENC_STREAM_S* pstuPicStream)
{
    HI_S32 sRet = HI_SUCCESS;
    VENC_CHN snap_chn;
    HI_S32 VencFd = -1;
    
    if( Ae_get_ive_snap_encoder_chn(&snap_chn)  != 0 )
    {
        DEBUG_ERROR( "Ae_get_ive_snap_encoder_chn FAILED\n");
        return -1;
    }

    if(m_bIve_snap_osd_overlay_bitmap != 0)
    {
        //overlay osd
    }

    static unsigned int u32TimeRef = 2;
    pstuViFrame->stVFrame.u32TimeRef = u32TimeRef;
    u32TimeRef += 2;

    do
    {
        VENC_CROP_CFG_S crop_cfg;
        memset(&crop_cfg, 0, sizeof(VENC_CROP_CFG_S));
        crop_cfg.bEnable = HI_TRUE;
        crop_cfg.stRect.s32X = (pstPicture_size->s32X)/16*16;
        crop_cfg.stRect.s32Y = (pstPicture_size->s32Y)/2*2;
        crop_cfg.stRect.u32Width = pstPicture_size->u32Width;
        crop_cfg.stRect.u32Height = pstPicture_size->u32Height;
        
        sRet = HI_MPI_VENC_SetCrop(snap_chn, &crop_cfg);
        if( sRet != HI_SUCCESS )
        {
            DEBUG_ERROR( "HI_MPI_VENC_SetCrop err:0x%08x\n", sRet);
            sRet = -1;
            break;
        }
        
        sRet = HI_MPI_VENC_SendFrame( snap_chn, pstuViFrame, -1 );
        if( sRet != HI_SUCCESS )
        {
            DEBUG_ERROR( "HI_MPI_VENC_SendFrame err:0x%08x\n", sRet);
            sRet = -1;
            break;
        }

        VencFd = HI_MPI_VENC_GetFd(snap_chn);
        if (VencFd <= 0)
        {
            DEBUG_ERROR("HI_MPI_VENC_GetFd chn:%d\n", snap_chn);
            sRet = -1;
            break;

        }

        fd_set read_fds;
        struct timeval TimeoutVal;

        FD_ZERO(&read_fds);
        FD_SET(VencFd,&read_fds);

        TimeoutVal.tv_sec = 2;
        TimeoutVal.tv_usec = 0;
        int ret = select(VencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if(ret <= 0)
        {
            DEBUG_ERROR("###################select err\n");
            sRet = -1;
            break;
        }

        VENC_CHN_STAT_S stuVencChnStat;
        sRet = HI_MPI_VENC_Query( snap_chn, &stuVencChnStat );
        if( sRet != HI_SUCCESS )
        {
            DEBUG_ERROR( " HI_MPI_VENC_Query err:0x%08x\n", sRet);
            sRet = -1;
            break;

        }

        if( stuVencChnStat.u32CurPacks == 0 )
        {
            DEBUG_ERROR( "Ae_encode_frame_to_picture error because packcount=%d\n", stuVencChnStat.u32CurPacks);
            sRet = -1;
            break;
        }

        static unsigned int u16PackCount = 0;
        static VENC_PACK_S* pPackAddr = NULL;
        if( stuVencChnStat.u32CurPacks > u16PackCount || pPackAddr == NULL )
        {
            if( pPackAddr )
            {
                delete [] pPackAddr;
                pPackAddr = NULL;
            }
            u16PackCount = stuVencChnStat.u32CurPacks;
            pPackAddr = new VENC_PACK_S[u16PackCount];
        }
        
        memset(pPackAddr, 0, sizeof(VENC_PACK_S)*u16PackCount);
        pstuPicStream->pstPack = pPackAddr;
        pstuPicStream->u32PackCount = stuVencChnStat.u32CurPacks;

        sRet = HI_MPI_VENC_GetStream( snap_chn, pstuPicStream, HI_TRUE );
        if( sRet != HI_SUCCESS )
        {
            DEBUG_ERROR( "Ae_encode_frame_to_picture HI_MPI_VENC_GetStream err:0x%08x\n", sRet);
            sRet = -1;
            break;
        }
    }while(0);

#if  defined(_AV_PLATFORM_HI3516A_)
    if(VencFd > 0)
    {
        HI_MPI_VENC_CloseFd(snap_chn);
    }
#endif
    return sRet;    
}
int CHiAvEncoder::Ae_free_ive_snap_stream(VENC_STREAM_S* pstuPicStream)
{
    VENC_CHN snap_chn;
    if( Ae_get_ive_snap_encoder_chn(&snap_chn) != 0 )
    {
        DEBUG_ERROR( "Ae_free_ive_snap_stream Ae_video_encoder_allotter_snap FAILED\n");
        return -1;
    }

    HI_S32 sRet = HI_MPI_VENC_ReleaseStream( snap_chn, pstuPicStream );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "Ae_free_ive_snap_stream HI_MPI_VENC_ReleaseStream FAILED width 0x%08x\n", sRet);
        return -1;
    }

    return 0;

}
int CHiAvEncoder::Ae_create_ive_snap_encoder(unsigned int width, unsigned int height, unsigned short u16OsdBitmap)
{
    if(false == m_bIve_snap_encoder_created)
    {
        m_bIve_snap_osd_overlay_bitmap = u16OsdBitmap;
        
        VENC_CHN chn;
        HI_S32 sRet = HI_SUCCESS;
        
        Ae_get_ive_snap_encoder_chn(&chn);
    
        VENC_CHN_ATTR_S stuVencChnAttr;
        VENC_ATTR_JPEG_S stuJpegAttr;
        stuVencChnAttr.stVeAttr.enType = PT_JPEG;
        stuJpegAttr.u32MaxPicWidth  = (width+3)/4*4;
        stuJpegAttr.u32MaxPicHeight = (height+3)/4*4;
        stuJpegAttr.u32PicWidth  = stuJpegAttr.u32MaxPicWidth;
        stuJpegAttr.u32PicHeight = stuJpegAttr.u32MaxPicHeight;
        stuJpegAttr.u32BufSize = (stuJpegAttr.u32MaxPicWidth * stuJpegAttr.u32MaxPicHeight*2 + 63) /64 * 64;
        stuJpegAttr.bByFrame = HI_TRUE;/*get stream mode is field mode  or frame mode*/
        stuJpegAttr.bSupportDCF = HI_FALSE;
        memcpy( &stuVencChnAttr.stVeAttr.stAttrJpeg, &stuJpegAttr, sizeof(VENC_ATTR_JPEG_S) );
    
        sRet = HI_MPI_VENC_CreateChn( chn, &stuVencChnAttr );
        if( sRet != HI_SUCCESS )
        {
            DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder HI_MPI_VENC_CreateChn err:0x%08x\n", sRet);
            return -1;
        }
        
        sRet = HI_MPI_VENC_StartRecvPic(chn);
        if( sRet != HI_SUCCESS )
        {
            DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder HI_MPI_VENC_StartRecvPic err:0x%08x\n", sRet);
            return -1;
        }
        
        if(u16OsdBitmap != 0)
        {
            //osd overlay implements now is reserved.
        }
    
        m_bIve_snap_encoder_created = true;
    }
    
    return 0;
}
int CHiAvEncoder::Ae_encode_frame_to_picture(VIDEO_FRAME_INFO_S *frame, unsigned char osd_num, ive_snap_osd_display_attr_s * osd, VENC_STREAM_S* pstuPicStream)
{
    HI_S32 sRet = HI_SUCCESS;
    VENC_CHN snap_chn;
    
    if( Ae_get_ive_snap_encoder_chn(&snap_chn)  != 0 )
    {
        DEBUG_ERROR( "Ae_get_ive_snap_encoder_chn FAILED\n");
        return -1;
    }

    for(unsigned char i=0; i<osd_num; ++i)
    {
        Ae_ive_snap_regin_overlay(i, osd[i].u16X, osd[i].u16Y, osd[i].pContent);
    }

    static unsigned int u32TimeRef = 2;
    frame->stVFrame.u32TimeRef = u32TimeRef;
    u32TimeRef += 2;

    do
    {
        sRet = HI_MPI_VENC_SendFrame( snap_chn, frame, -1 );
        if( sRet != HI_SUCCESS )
        {
            DEBUG_ERROR( "HI_MPI_VENC_SendFrame err:0x%08x\n", sRet);
            sRet = -1;
            break;
        }

        HI_S32 VencFd = HI_MPI_VENC_GetFd(snap_chn);
        if (VencFd <= 0)
        {
            DEBUG_ERROR("HI_MPI_VENC_GetFd chn:%d\n", snap_chn);
            sRet = -1;
            break;

        }

        fd_set read_fds;
        struct timeval TimeoutVal;

        FD_ZERO(&read_fds);
        FD_SET(VencFd,&read_fds);

        TimeoutVal.tv_sec = 2;
        TimeoutVal.tv_usec = 0;
        int ret = select(VencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if(ret <= 0)
        {
            DEBUG_ERROR("###################select err\n");
            sRet = -1;
            break;

        }

        VENC_CHN_STAT_S stuVencChnStat;
        sRet = HI_MPI_VENC_Query( snap_chn, &stuVencChnStat );
        if( sRet != HI_SUCCESS )
        {
            DEBUG_ERROR( " HI_MPI_VENC_Query err:0x%08x\n", sRet);
            sRet = -1;
            break;

        }

        if( stuVencChnStat.u32CurPacks == 0 )
        {
            DEBUG_ERROR( "Ae_encode_frame_to_picture error because packcount=%d\n", stuVencChnStat.u32CurPacks);
            sRet = -1;
            break;

        }

        static unsigned int u16PackCount = 0;
        static VENC_PACK_S* pPackAddr = NULL;
        if( stuVencChnStat.u32CurPacks > u16PackCount || pPackAddr == NULL )
        {
            if( pPackAddr )
            {
                delete [] pPackAddr;
                pPackAddr = NULL;
            }
            u16PackCount = stuVencChnStat.u32CurPacks;
            pPackAddr = new VENC_PACK_S[u16PackCount];
        }
        
        memset(pPackAddr, 0, sizeof(VENC_PACK_S)*u16PackCount);
        pstuPicStream->pstPack = pPackAddr;
        pstuPicStream->u32PackCount = stuVencChnStat.u32CurPacks;

        sRet = HI_MPI_VENC_GetStream( snap_chn, pstuPicStream, HI_TRUE );
        if( sRet != HI_SUCCESS )
        {
            DEBUG_ERROR( "Ae_encode_frame_to_picture HI_MPI_VENC_GetStream err:0x%08x\n", sRet);
            sRet = -1;
            break;
        }
    }while(0);
    
    return sRet;    
}

int CHiAvEncoder::Ae_get_osd_size(av_resolution_t resolution, unsigned short osd_char_num, SIZE_S* size)
{
    int font_width = 0;
    int font_height = 0;
    int font_name = 0;
    int horizontal_scaler = 0;
    int vertical_scaler = 0;

    /*calculate the size of region*/
    Ae_get_osd_parameter(_AON_DATE_TIME_, resolution, &font_name, 
                                    &font_width, &font_height, &horizontal_scaler, &vertical_scaler);

    switch(resolution)
    {
        case _AR_1080_:
        case _AR_720_:
        case _AR_960P_:
            size->u32Width  = ((font_width * osd_char_num * horizontal_scaler) / 2 + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
            size->u32Height= (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
            break;
        default:
            size->u32Width  = ((font_width * osd_char_num * horizontal_scaler) * 2 / 3 + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
            size->u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
            break;
    }

    return 0;
}

int CHiAvEncoder::Ae_create_ive_snap_encoder(unsigned int width, unsigned int height, unsigned char osd_num,const  OVERLAY_ATTR_S* osd_attr)
{
    if(false == m_bIve_snap_encoder_created)
    {
        
        VENC_CHN chn;
        HI_S32 sRet = HI_SUCCESS;
        
        Ae_get_ive_snap_encoder_chn(&chn);
    
        VENC_CHN_ATTR_S stuVencChnAttr;
        VENC_ATTR_JPEG_S stuJpegAttr;
        stuVencChnAttr.stVeAttr.enType = PT_JPEG;
        stuJpegAttr.u32MaxPicWidth  = (width+3)/4*4;
        stuJpegAttr.u32MaxPicHeight = (height+3)/4*4;
        stuJpegAttr.u32PicWidth  = stuJpegAttr.u32MaxPicWidth;
        stuJpegAttr.u32PicHeight = stuJpegAttr.u32MaxPicHeight;
        stuJpegAttr.u32BufSize = (stuJpegAttr.u32MaxPicWidth * stuJpegAttr.u32MaxPicHeight*2 + 63) /64 * 64;
        stuJpegAttr.bByFrame = HI_TRUE;/*get stream mode is field mode  or frame mode*/
        stuJpegAttr.bSupportDCF = HI_FALSE;
        memcpy( &stuVencChnAttr.stVeAttr.stAttrJpeg, &stuJpegAttr, sizeof(VENC_ATTR_JPEG_S) );
    
        sRet = HI_MPI_VENC_CreateChn( chn, &stuVencChnAttr );
        if( sRet != HI_SUCCESS )
        {
            DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder HI_MPI_VENC_CreateChn err:0x%08x\n", sRet);
            return -1;
        }
        
        sRet = HI_MPI_VENC_StartRecvPic(chn);
        if( sRet != HI_SUCCESS )
        {
            DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder HI_MPI_VENC_StartRecvPic err:0x%08x\n", sRet);
            return -1;
        }
        
        for(unsigned char i=0; i<osd_num; ++i)
        {
            Ae_ive_snap_regin_create(chn, i, osd_attr[i]);
        }

        m_bIve_snap_encoder_created = true;
    }
    
    return 0;
}


int CHiAvEncoder::Ae_ive_snap_regin_create(VENC_CHN venc_chn, int index, const OVERLAY_ATTR_S& attr)
{
    if( index >= IVE_SNAP_MAX_OSD_NUM || m_stIve_osd_region[index].is_valid)
    {
        return 0;
    }

    HI_S32 sRet = HI_SUCCESS;
    RGN_ATTR_S stuRgnAttr;
    
    stuRgnAttr.enType = OVERLAY_RGN;
    stuRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
    stuRgnAttr.unAttr.stOverlay.u32BgColor = attr.u32BgColor;
    stuRgnAttr.unAttr.stOverlay.stSize.u32Width = (attr.stSize.u32Width +1)/2*2;
    stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (attr.stSize.u32Height +1)/2*2;

   // Har_get_region_handle_ive(0,index,&(m_stIve_osd_region[index].handle));
    m_stIve_osd_region[index].handle = index + 200;
    if( (sRet = HI_MPI_RGN_Create(m_stIve_osd_region[index].handle, &stuRgnAttr)) != HI_SUCCESS )
    {
        DEBUG_ERROR("CHiAvEncoder::Ae_snap_create_regin HI_MPI_RGN_Create FAILED width 0x%08x, regin=%d index=%d\n", sRet, m_stIve_osd_region[index].handle, index );
        return -1;
    }

    MPP_CHN_S stuMppChn;
    RGN_CHN_ATTR_S stuRgnChnAttr;
    memset( &stuRgnChnAttr, 0, sizeof(stuRgnChnAttr) );
    stuRgnChnAttr.bShow = HI_FALSE;
    stuRgnChnAttr.enType = OVERLAY_RGN;
    stuRgnChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 0;
    stuRgnChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128;
    stuRgnChnAttr.unChnAttr.stOverlayChn.u32Layer = 0;

    stuRgnChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
    stuRgnChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;

    stuRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = HI_FALSE;
    stuRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Width = 16;
    stuRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Height = 16;
    stuRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.u32LumThresh = 200;
    stuRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = MORETHAN_LUM_THRESH;

    stuMppChn.enModId = HI_ID_VENC;
    stuMppChn.s32DevId = 0;
    stuMppChn.s32ChnId = venc_chn;

    if( (sRet=HI_MPI_RGN_AttachToChn(m_stIve_osd_region[index].handle, &stuMppChn, &stuRgnChnAttr)) != HI_SUCCESS )
    {        
        _AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_snap_create_regin HI_MPI_RGN_AttachToChn FAILED(0x%08x)\n", sRet);
        HI_MPI_RGN_Destroy( m_stuOsdTemp[index].hOsdRegin );
        return -1;
    }

    m_stIve_osd_region[index].venc_chn = venc_chn;
    m_stIve_osd_region[index].bg_color = attr.u32BgColor;
    m_stIve_osd_region[index].width = stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
    m_stIve_osd_region[index].height = stuRgnAttr.unAttr.stOverlay.stSize.u32Height;
    m_stIve_osd_region[index].bitmap = new unsigned char[stuRgnAttr.unAttr.stOverlay.stSize.u32Width * stuRgnAttr.unAttr.stOverlay.stSize.u32Height *2*4];
    memset(m_stIve_osd_region[index].content, 0, sizeof(m_stIve_osd_region[index].content));
    m_stIve_osd_region[index].is_valid = true;

    return 0;
}

int CHiAvEncoder::Ae_ive_snap_regin_destory(int index)
{
    if( !m_stIve_osd_region[index].is_valid )
    {
        return 0;
    }

    HI_S32 sRet = HI_FAILURE;
    
    if( (sRet=HI_MPI_RGN_Destroy(m_stIve_osd_region[index].handle)) != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_snap_delete_regin HI_MPI_RGN_Destroy FAILED(0x%08x)\n", sRet);
    }
    
    if(m_stIve_osd_region[index].bitmap != NULL)
    {
        _AV_SAFE_DELETE_ARRAY_(m_stIve_osd_region[index].bitmap);
    }
    m_stIve_osd_region[index].is_valid = false;

    return 0;
}

int CHiAvEncoder::Ae_destroy_ive_snap_encoder()
{
    if(m_bIve_snap_encoder_created)
    {    
        VENC_CHN snap_chn;
        HI_S32 sRet = HI_SUCCESS;
        
        if( Ae_get_ive_snap_encoder_chn(&snap_chn) != 0 )
        {
            _AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder:: Ae_get_ive_snap_encoder_chn FAILED\n");
            return -1;
        }
        
        if(m_bIve_snap_osd_overlay_bitmap != 0)
        {
            //destory snap osd regin
        }
        //! for singapore bus
        for(int i=0; i<IVE_SNAP_MAX_OSD_NUM;++i)
        {
            Ae_ive_snap_regin_destory(i);
        }

        //!end singapore bus
        sRet = HI_MPI_VENC_StopRecvPic( snap_chn );
        if( sRet != HI_SUCCESS )
        {
            _AV_KEY_INFO_(_AVD_ENC_, "HI_MPI_VENC_StopRecvPic err:0x%08x\n", sRet);
            return -1;
        }
        
        sRet = HI_MPI_VENC_DestroyChn( snap_chn );
        if( sRet != HI_SUCCESS )
        {
            _AV_KEY_INFO_(_AVD_ENC_, "HI_MPI_VENC_DestroyChn err:0x%08x\n", sRet);
            return -1;
        }
        
        m_bIve_snap_encoder_created = false;
    }

    return 0;
}

int CHiAvEncoder::Ae_send_frame_to_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, void* pFrame_info, int milli_second)
{
    VENC_CHN venc_chn = -1;
    HI_S32 ret_val = -1;

    if(Ae_video_encoder_allotter(video_stream_type, phy_video_chn_num, &venc_chn) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder Ae_video_encoder_allotter FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
        return -1;
    }
    VIDEO_FRAME_INFO_S * frame = (VIDEO_FRAME_INFO_S *)pFrame_info;
    ret_val = HI_MPI_VENC_SendFrame(venc_chn, frame, milli_second);
    if(HI_SUCCESS != ret_val)
    {
        DEBUG_ERROR( "HI_MPI_VENC_SendFrame failed!frame info[w:%d h:%d] errcode:0x%x\n", frame->stVFrame.u32Width, frame->stVFrame.u32Height, ret_val);
        return -1;
    }

    return 0;
}

int CHiAvEncoder::Ae_get_ive_snap_encoder_chn(VENC_CHN* ive_snap_chn)
{
    _AV_POINTER_ASSIGNMENT_(ive_snap_chn, pgAvDeviceInfo->Adi_get_vi_chn_num() * 3 + 2 );
    return 0;
}

int CHiAvEncoder::Ae_ive_snap_regin_overlay(int index, unsigned short x, unsigned short y, char* pszStr )
{
    if( !m_stIve_osd_region[index].is_valid )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_snap_regin_set_osd failed, osd not init\n");
        return -1;
    }

    HI_S32 sRet = HI_SUCCESS;
    MPP_CHN_S stuMppChn;
    RGN_CHN_ATTR_S stuRgnChnAttr;

    stuMppChn.enModId = HI_ID_VENC;
    stuMppChn.s32DevId = 0;
    stuMppChn.s32ChnId = m_stIve_osd_region[index].venc_chn;
    
    if((sRet=HI_MPI_RGN_GetDisplayAttr(m_stIve_osd_region[index].handle, &stuMppChn, &stuRgnChnAttr)) != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_snap_regin_set_osd HI_MPI_RGN_GetDisplayAttr failed chn:%d,with 0x%08x\n", sRet, stuMppChn.s32ChnId);
        return -1;
    }

    stuRgnChnAttr.bShow = HI_TRUE;
    stuRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = x;
    stuRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = y;

    if( (sRet=HI_MPI_RGN_SetDisplayAttr(m_stIve_osd_region[index].handle, &stuMppChn, &stuRgnChnAttr)) != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_snap_regin_set_osd HI_MPI_RGN_SetDisplayAttr failed(x:%d, y:%d) width 0x%08x\n", x,y, sRet);
        return -1;
    }

    if(0 != strcmp((char *)m_stIve_osd_region[index].content, pszStr))
    {
        /*if(utf8string_2_bmp(m_stIve_osd_region[index].bitmap, m_stIve_osd_region[index].width, \
             m_stIve_osd_region[index].height, pszStr, 0xffff, m_stIve_osd_region[index].bg_color, 0x8000) != 0)*/
             
     	if(Ae_utf8string_2_bmp( _AON_DATE_TIME_, _AR_1080_, m_stIve_osd_region[index].bitmap,\
            m_stIve_osd_region[index].width, m_stIve_osd_region[index].height, pszStr, 0xffff, 0x00, 0x8000) != 0)
        {
            return -1;
        }

        strncpy((char *)m_stIve_osd_region[index].content, pszStr, sizeof(m_stIve_osd_region[index].content));

        BITMAP_S stuBitmap;
        stuBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
        stuBitmap.u32Width = m_stIve_osd_region[index].width;
        stuBitmap.u32Height = m_stIve_osd_region[index].height;    
        stuBitmap.pData = m_stIve_osd_region[index].bitmap;

        if( (sRet=HI_MPI_RGN_SetBitMap(m_stIve_osd_region[index].handle, &stuBitmap)) != HI_SUCCESS )
        {        
            DEBUG_ERROR( "CHiAvEncoder::Ae_osd_overlay HI_MPI_RGN_SetBitMap FAILED(errorcode:0x%08x)(region_handle:%d)\n", sRet, m_stIve_osd_region[index].handle);
            return -1;
        }
    }

    return 0;
}
#endif
#endif

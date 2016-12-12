#include "HiAvEncoder.h"
#include "audio_amr_adp.h" 

#ifndef _AV_PRODUCT_CLASS_IPC_
#ifdef _AV_USE_AACENC_
#include "audio_aac_adp.h"
#endif
#endif

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
	//m_stuOsdTemp[6].bInited = false;
    //m_stuOsdTemp[7].bInited = false;
    //m_stuOsdTemp[6].pszBitmap = NULL;
    //m_stuOsdTemp[7].pszBitmap = NULL;
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
#if defined(_AV_PRODUCT_CLASS_IPC_)	
    if (NULL != m_pAudio_thread_info)
    {
        if(m_pAudio_thread_info->thread_state)
        {
            m_pAudio_thread_info->thread_state = false;
            pthread_join(m_pAudio_thread_info->thread_id, NULL);
        }
        _AV_SAFE_DELETE_(m_pAudio_thread_info);
    }
#else
    if (Ae_ai_aenc_bind_control(audio_stream_type, phy_audio_chn_num, false) != 0)//!<Unbind
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destory_audio_encoder Ae_ai_aenc_bind_control FAILED(type:%d)(chn:%d)\n", audio_stream_type, phy_audio_chn_num);
        return -1;
    }
#endif

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
    AENC_ATTR_AMR_S aenc_attr_amr;
#ifdef _AV_USE_AACENC_
    AENC_ATTR_AAC_S aenc_attr_aac;
#endif
	AENC_ATTR_LPCM_S aenc_attr_pcm;
    HI_S32 ret_val = -1;

    memset(&aenc_chn_attr, 0, sizeof(AENC_CHN_ATTR_S));
    DEBUG_WARNING("audio type=%d, streamType=%d, chn=%d\n", pAudio_para->type, audio_stream_type, phy_audio_chn_num);
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
            {
                aenc_chn_attr.enType = PT_G726;
                aenc_chn_attr.u32BufSize = 30;
                aenc_chn_attr.pValue = &aenc_attr_g726;
#if defined(_AV_PRODUCT_CLASS_IPC_)
                char customer_name[32] = {0};
                pgAvDeviceInfo->Adi_get_customer_name(customer_name,sizeof(customer_name));
                if (0 == strcmp(customer_name, "CUSTOMER_CB"))
                {
                    aenc_attr_g726.enG726bps = G726_16K;//CHUANBIAO USES G726_16K
                }
                else
                {
                    aenc_attr_g726.enG726bps = MEDIA_G726_16K;
                }               
#else
                //! for CB check criterion, G726_16k format is only used
                char check_criterion[32]={0};
                pgAvDeviceInfo->Adi_get_check_criterion(check_criterion, sizeof(check_criterion));
                char customer_name[32] = {0};
                pgAvDeviceInfo->Adi_get_customer_name(customer_name,sizeof(customer_name));
                if ((!strcmp(check_criterion, "CHECK_CB"))||(!strcmp(customer_name, "CUSTOMER_04.1062")))

                {
                    aenc_attr_g726.enG726bps = G726_16K;//MEDIA_G726_16K;//MEDIA_G726_40K;
                }
                else if (!strcmp(customer_name, "CUSTOMER_JIUTONG"))
                {
                    aenc_attr_g726.enG726bps = G726_32K;
                }
                else
                {
                    aenc_attr_g726.enG726bps = MEDIA_G726_16K;
                }
#endif

                break;
            }   
        case _AET_AMR_:            //!
            aenc_chn_attr.enType = PT_AMR;
            aenc_chn_attr.u32BufSize = 30;
            aenc_chn_attr.pValue = &aenc_attr_amr;
            aenc_attr_amr.enFormat = AMR_FORMAT_MMS ;
            aenc_attr_amr.enMode = AMR_MODE_MR122;    // bitrate
            aenc_attr_amr.s32Dtx = 0;
            //HI_MPI_AENC_AmrInit();
            break;
#ifdef _AV_USE_AACENC_
        case _AET_AAC_:            //!
			memset(&aenc_attr_aac, 0, sizeof(AENC_ATTR_AAC_S));
            aenc_chn_attr.enType = PT_AAC;
            aenc_chn_attr.u32BufSize = 30;
            aenc_chn_attr.pValue = &aenc_attr_aac;
            aenc_attr_aac.enAACType = AAC_TYPE_AACLC;
            aenc_attr_aac.enBitRate = AAC_BPS_32K;
            aenc_attr_aac.enBitWidth = AUDIO_BIT_WIDTH_16;
            aenc_attr_aac.enSmpRate = AUDIO_SAMPLE_RATE_8000;
            aenc_attr_aac.enSoundMode = AUDIO_SOUND_MODE_MONO;
            HI_MPI_AENC_AacInit();
            break;
#endif
		case _AET_LPCM_:
			aenc_chn_attr.enType = PT_LPCM;
			aenc_chn_attr.u32BufSize = 30;
			aenc_chn_attr.pValue = &aenc_attr_pcm;
			aenc_attr_pcm.resv = 0;
			break;
        default:
            _AV_FATAL_(1, "CHiAvEncoder::Ae_create_audio_encoder You must give the implement\n");
            break;
    }
#ifdef _AV_PLATFORM_HI3520D_V300_
#ifdef _AV_USE_AACENC_
	aenc_chn_attr.u32PtNumPerFrm = 1024;
#elif defined(_AV_HAVE_PURE_AUDIO_CHANNEL)
	aenc_chn_attr.u32PtNumPerFrm = 2048;
#else
	aenc_chn_attr.u32PtNumPerFrm = 320;
#endif
#endif
    if((ret_val = HI_MPI_AENC_CreateChn(aenc_chn, &aenc_chn_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_audio_encoder HI_MPI_AENC_CreateChn FAILED(type:%d)(chn:%d)(ret_val:0x%08lx)\n", audio_stream_type, phy_audio_chn_num, (unsigned long)ret_val);
        return -1;
    }

#if defined(_AV_PRODUCT_CLASS_IPC_)
    Ae_ai_aenc_proc(audio_stream_type, phy_audio_chn_num);
#else
    if(Ae_ai_aenc_bind_control(audio_stream_type, phy_audio_chn_num, true) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_audio_encoder Ae_ai_aenc_bind_control FAILED(type:%d)(chn:%d)\n", audio_stream_type, phy_audio_chn_num);
        return -1;
    }
#endif
    memset(pAudio_encoder_id, 0, sizeof(audio_encoder_id_t));
    pAudio_encoder_id->audio_encoder_fd = HI_MPI_AENC_GetFd(aenc_chn);    
    hi_audio_encoder_info_t *pAei = new hi_audio_encoder_info_t;
    _AV_FATAL_(pAei == NULL, "CHiAvEncoder::Ae_create_audio_encoder OUT OF MEMORY\n");
    memset(pAei, 0, sizeof(hi_audio_encoder_info_t));
    pAei->aenc_chn = aenc_chn;
    pAudio_encoder_id->pAudio_encoder_info = (void *)pAei;

//!Added by dfz for IPC transplant on Nov 2014
#if defined(_AV_PLATFORM_HI3518C_) && defined(_AV_PRODUCT_CLASS_IPC_) && defined(_AV_HAVE_AUDIO_INPUT_)
    sleep(1);
    m_pHi_ai->HiAi_config_acodec_volume();  //!< Set default value of analog gain of  audio input
#endif

    return 0;
}

int CHiAvEncoder::Ae_create_video_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, video_encoder_id_t *pVideo_encoder_id)
{
    return Ae_create_h264_encoder(video_stream_type, phy_video_chn_num, pVideo_para, pVideo_encoder_id);
}


int CHiAvEncoder::Ae_destory_video_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, video_encoder_id_t *pVideo_encoder_id)
{
    VENC_GRP venc_grp = -1;
    VENC_CHN venc_chn = -1;

#if defined(_AV_HISILICON_VPSS_)
    if(Ae_venc_vpss_control(video_stream_type, phy_video_chn_num, false) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destory_video_encoder Ae_venc_vpss_control FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
        return -1;
    }
#endif

    if(Ae_video_encoder_allotter(video_stream_type, phy_video_chn_num, &venc_grp, &venc_chn) != 0)
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

#ifndef _AV_PLATFORM_HI3520D_V300_
    if((ret_val = HI_MPI_VENC_UnRegisterChn(venc_chn)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destory_video_encoder HI_MPI_VENC_UnRegisterChn FAILED(type:%d)(chn:%d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, (unsigned long)ret_val);
        return -1;
    }
#endif	
    if((ret_val = HI_MPI_VENC_DestroyChn(venc_chn)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destory_video_encoder HI_MPI_VENC_DestroyChn FAILED(type:%d)(chn:%d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, (unsigned long)ret_val);
        return -1;
    }
#ifndef _AV_PLATFORM_HI3520D_V300_	
    if((ret_val = HI_MPI_VENC_DestroyGroup(venc_grp)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destory_video_encoder HI_MPI_VENC_DestroyGroup FAILED(type:%d)(chn:%d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, (unsigned long)ret_val);
        return -1;
    }
#endif
    hi_video_encoder_info_t *pVei = (hi_video_encoder_info_t *)pVideo_encoder_id->pVideo_encoder_info;
    if(pVei != NULL)
    {
        _AV_SAFE_DELETE_ARRAY_(pVei->pPack_buffer);
        delete pVei;
        pVideo_encoder_id->pVideo_encoder_info = NULL;
    }

    return 0;
}

int CHiAvEncoder::Ae_create_snap_encoder( av_resolution_t eResolution, av_tvsystem_t eTvSystem, unsigned int u32OsdNameLen )
{
#if 0
	DEBUG_MESSAGE("###### ... Ae_create_snap_encoder\n");
	if( m_bSnapEncoderInited )
    {
        return 0;
    }
    
    VENC_GRP snapGrp;
    VENC_CHN snapChl;
    
    if( Ae_video_encoder_allotter_snap(&snapGrp, &snapChl) != 0 )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder Ae_video_encoder_allotter_snap FAILED\n");
        return -1;
    }

    HI_S32 sRet = HI_SUCCESS;
    
    sRet = HI_MPI_VENC_CreateGroup( snapGrp );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder HI_MPI_VENC_CreateGroup FAILED width 0x%x, group=%d\n", sRet, snapGrp);
        return -1;
    }

    int iWidth, iHeight;
    pgAvDeviceInfo->Adi_get_video_size( eResolution, eTvSystem, &iWidth, &iHeight);
//#ifdef _AV_PLATFORM_HI3518C_
    pgAvDeviceInfo->Adi_covert_vidoe_size_by_rotate( &iWidth, &iHeight);
//#endif

    VENC_CHN_ATTR_S stuVencChnAttr;
    VENC_ATTR_JPEG_S stuJpegAttr;
    stuVencChnAttr.stVeAttr.enType = PT_JPEG;

    stuJpegAttr.u32MaxPicWidth  = (iWidth + 2) / 4 * 4;
    stuJpegAttr.u32MaxPicHeight = (iHeight + 2)/ 4 * 4;
    stuJpegAttr.u32PicWidth  = stuJpegAttr.u32MaxPicWidth;
    stuJpegAttr.u32PicHeight = stuJpegAttr.u32MaxPicHeight;
    stuJpegAttr.u32BufSize = (stuJpegAttr.u32MaxPicWidth * stuJpegAttr.u32MaxPicHeight*2 + 32) /64 * 64;
    stuJpegAttr.bByFrame = HI_TRUE;/*get stream mode is field mode  or frame mode*/
    stuJpegAttr.bVIField = HI_FALSE;/*the sign of the VI picture is field or frame?*/
    stuJpegAttr.u32Priority = 0;/*channels precedence level unused*/
    memcpy( &stuVencChnAttr.stVeAttr.stAttrJpeg, &stuJpegAttr, sizeof(VENC_ATTR_JPEG_S) );

    sRet = HI_MPI_VENC_CreateChn( snapChl, &stuVencChnAttr );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder HI_MPI_VENC_CreateChn err:0x%08x\n", sRet);
        return -1;
    }

    sRet = HI_MPI_VENC_RegisterChn( snapGrp, snapChl );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder HI_MPI_VENC_RegisterChn err:0x%08x\n", sRet);
        return -1;
    } 

    sRet = HI_MPI_VENC_StartRecvPic( snapChl );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder HI_MPI_VENC_StartRecvPic err:0x%08x\n", sRet);
        return -1;
    }

    
    this->Ae_snap_regin_create( snapGrp, eResolution, eTvSystem, 0, u32OsdNameLen );
    this->Ae_snap_regin_create( snapGrp, eResolution, eTvSystem, 1, u32OsdNameLen );

    m_bSnapEncoderInited = true;
#endif  
    return 0;
}
//N9M
int CHiAvEncoder::Ae_create_snap_encoder( av_resolution_t eResolution, av_tvsystem_t eTvSystem, unsigned int factor, hi_snap_osd_t pSnapOsd[], int osdNum)
{
    //DEBUG_MESSAGE("###### N9M Ae_create_snap_encoder\n");
    if( m_bSnapEncoderInited )
    {
        return 0;
    }
    
    VENC_GRP snapGrp;
    VENC_CHN snapChl;
    
    if( Ae_video_encoder_allotter_snap(&snapGrp, &snapChl) != 0 )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder Ae_video_encoder_allotter_snap FAILED\n");
        return -1;
    }

    HI_S32 sRet = HI_SUCCESS;
#ifndef _AV_PLATFORM_HI3520D_V300_
    sRet = HI_MPI_VENC_CreateGroup( snapGrp );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder HI_MPI_VENC_CreateGroup FAILED width 0x%x, group=%d\n", sRet, snapGrp);
        return -1;
    }
#endif
    int iWidth, iHeight;
    pgAvDeviceInfo->Adi_get_video_size( eResolution, eTvSystem, &iWidth, &iHeight);
#ifdef _AV_PLATFORM_HI3518C_
    pgAvDeviceInfo->Adi_covert_vidoe_size_by_rotate( &iWidth, &iHeight);
#endif

    VENC_CHN_ATTR_S stuVencChnAttr;
    VENC_ATTR_JPEG_S stuJpegAttr;
    stuVencChnAttr.stVeAttr.enType = PT_JPEG;
    stuJpegAttr.u32MaxPicWidth  = (iWidth + 2) / 4 * 4;
    stuJpegAttr.u32MaxPicHeight = (iHeight + 2)/ 4 * 4;
    stuJpegAttr.u32PicWidth  = stuJpegAttr.u32MaxPicWidth;
    stuJpegAttr.u32PicHeight = stuJpegAttr.u32MaxPicHeight;
    stuJpegAttr.u32BufSize = (stuJpegAttr.u32MaxPicWidth * stuJpegAttr.u32MaxPicHeight*2 + 32) /64 * 64;
    stuJpegAttr.bByFrame = HI_TRUE;/*get stream mode is field mode  or frame mode*/
#ifndef _AV_PLATFORM_HI3520D_V300_
    stuJpegAttr.bVIField = HI_FALSE;/*the sign of the VI picture is field or frame?*/
    stuJpegAttr.u32Priority = 0;/*channels precedence level unused*/
#else
    stuJpegAttr.bSupportDCF = HI_FALSE;
#endif
    memcpy( &stuVencChnAttr.stVeAttr.stAttrJpeg, &stuJpegAttr, sizeof(VENC_ATTR_JPEG_S) );

    sRet = HI_MPI_VENC_CreateChn( snapChl, &stuVencChnAttr );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder HI_MPI_VENC_CreateChn err:0x%08x\n", sRet);
        return -1;
    }
#ifndef _AV_PLATFORM_HI3520D_V300_ 
    sRet = HI_MPI_VENC_RegisterChn( snapGrp, snapChl );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder HI_MPI_VENC_RegisterChn err:0x%08x\n", sRet);
        return -1;
    } 
#endif	
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
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder HI_MPI_VENC_SetJpegParam err:0x%08x\n", sRet);
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
            this->Ae_snap_regin_create( snapGrp, eResolution, eTvSystem, i, pSnapOsd[i]);
        }
#else
        if(strlen(pSnapOsd[i].szStr) != 0)
        {
            //DEBUG_MESSAGE("Ae_snap_regin_create index=%d\n",i);
#if defined (_AV_PLATFORM_HI3520D_V300_)
            this->Ae_snap_regin_create( snapChl, eResolution, eTvSystem, i, pSnapOsd);
#else
            this->Ae_snap_regin_create( snapGrp, eResolution, eTvSystem, i, pSnapOsd);
#endif
        }
#endif
    }
    //this->Ae_snap_regin_create( snapGrp, eResolution, 0, u32OsdNameLen );
    //this->Ae_snap_regin_create( snapGrp, eResolution, 1, u32OsdNameLen );
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
    
    VENC_GRP snapGrp;
    VENC_CHN snapChl;
    
    if( Ae_video_encoder_allotter_snap(&snapGrp, &snapChl) != 0 )
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

    sRet = HI_MPI_VENC_UnRegisterChn( snapChl );
    if( sRet != HI_SUCCESS )
    {
        _AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_destroy_snap_encoder HI_MPI_VENC_UnRegisterChn err:0x%08x\n", sRet);
    }

    sRet = HI_MPI_VENC_DestroyChn( snapChl );
    if( sRet != HI_SUCCESS )
    {
        _AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_destroy_snap_encoder HI_MPI_VENC_DestroyChn err:0x%08x\n", sRet);
    }

    sRet = HI_MPI_VENC_DestroyGroup( snapGrp );
    if( sRet != HI_SUCCESS )
    {
        _AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_destroy_snap_encoder HI_MPI_VENC_DestroyGroup err:0x%08x\n", sRet);
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
    
    VENC_GRP snapGrp;
    VENC_CHN snapChl;
    
    if( Ae_video_encoder_allotter_snap(&snapGrp, &snapChl) != 0 )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destroy_snap_encoder Ae_video_encoder_allotter_snap FAILED\n");
        return -1;
    }
	
	for(int i=0; i <(int)(sizeof(m_stuOsdTemp)/sizeof(m_stuOsdTemp[0])); i++)
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
#ifndef _AV_PLATFORM_HI3520D_V300_
    sRet = HI_MPI_VENC_UnRegisterChn( snapChl );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destroy_snap_encoder HI_MPI_VENC_UnRegisterChn err:0x%08x\n", sRet);
    }
#endif	

    sRet = HI_MPI_VENC_DestroyChn( snapChl );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destroy_snap_encoder HI_MPI_VENC_DestroyChn err:0x%08x\n", sRet);
    }

#ifndef _AV_PLATFORM_HI3520D_V300_
    sRet = HI_MPI_VENC_DestroyGroup( snapGrp );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destroy_snap_encoder HI_MPI_VENC_DestroyGroup err:0x%08x\n", sRet);
    }
#endif
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
        DEBUG_ERROR( "CHiAvEncoder::Ae_snap_regin_set_osd failed, osd not init, index = %d, show = %d, init = %d\n", index, bShow, m_stuOsdTemp[index].bInited);
        return -1;
    }

    MPP_CHN_S stuMppChn;
    RGN_CHN_ATTR_S stuRgnChnAttr;
#if defined (_AV_PLATFORM_HI3520D_V300_)
    stuMppChn.enModId = HI_ID_VENC;
    stuMppChn.s32DevId = 0;
    stuMppChn.s32ChnId = m_stuOsdTemp[index].vencChn;
#else
    stuMppChn.enModId = HI_ID_GROUP;
    stuMppChn.s32DevId = m_stuOsdTemp[index].vencGrp;
    stuMppChn.s32ChnId = 0;
#endif
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

    int textColor = 0xffff;
#if defined(_AV_PLATFORM_HI3520D_V300_)
	if(OSD_TEST_FLAG == pgAvDeviceInfo->Adi_product_type())
	{
		if(m_stuOsdTemp[index].osdTextColor == 1)
		{
			textColor = 0x0;
		}
		else if(m_stuOsdTemp[index].osdTextColor == 2)
		{
			textColor = 0xfc00;
		}
	}
#endif

    ///////////////////////////////////////////////////////////////////////////////
    if(Ae_utf8string_2_bmp( m_stuOsdTemp[index].eOsdName, m_stuOsdTemp[index].eResolution, m_stuOsdTemp[index].pszBitmap
        , m_stuOsdTemp[index].u32BitmapWidth, m_stuOsdTemp[index].u32BitmapHeight, pszStr, textColor, 0x00, 0x8000) != 0)
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
        DEBUG_ERROR( "CHiAvEncoder::Ae_osd_overlay HI_MPI_RGN_SetBitMap FAILED(errcode:0x%08x)(region_handle:%d)\n", sRet, m_stuOsdTemp[index].hOsdRegin);
        return -1;
    }

    return 0;
}

#if defined(_AV_PRODUCT_CLASS_IPC_)
int CHiAvEncoder::Ae_snap_regin_create( VENC_GRP vencGrp, av_resolution_t eResolution, av_tvsystem_t eTvSystem, int index, hi_snap_osd_t &pSnapOsd)
{
	//printf("#####CHiAvEncoder::Ae_snap_regin_create\n");
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
                    stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                    break;
                default:
                    stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler)  + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                    stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
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
                    //_AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                    continue;//return -1;
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
                    //_AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                    continue;//return -1;
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
                    //_AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_snap_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                    continue;//return -1;
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
                    //_AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                    continue;//return -1;
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
                    //_AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                    continue;//return -1;
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
                    //_AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                    continue;//return -1;
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
                    //_AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                    continue;//return -1;
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

        stuMppChn.enModId = HI_ID_GROUP;
        stuMppChn.s32DevId = vencGrp;
        stuMppChn.s32ChnId = 0;

        if( (sRet=HI_MPI_RGN_AttachToChn(m_stuOsdTemp[index].hOsdRegin, &stuMppChn, &stuRgnChnAttr)) != HI_SUCCESS )
        {        
            _AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_snap_create_regin HI_MPI_RGN_AttachToChn FAILED(0x%08x)\n", sRet);

            HI_MPI_RGN_Destroy( m_stuOsdTemp[index].hOsdRegin );
            return -1;
        }

        m_stuOsdTemp[index].vencGrp = vencGrp;
    m_stuOsdTemp[index].eResolution = eResolution;
    /*bitmap*/
    m_stuOsdTemp[index].u32BitmapWidth = stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
    m_stuOsdTemp[index].u32BitmapHeight = stuRgnAttr.unAttr.stOverlay.stSize.u32Height;
    m_stuOsdTemp[index].pszBitmap = new unsigned char[m_stuOsdTemp[index].u32BitmapWidth * horScaler * m_stuOsdTemp[index].u32BitmapHeight * verScaler * 2];

    m_stuOsdTemp[index].bInited = true;

    return 0;
}
#else
#if defined (_AV_PLATFORM_HI3520D_V300_)
int CHiAvEncoder::Ae_snap_regin_create( VENC_CHN vencChn, av_resolution_t eResolution, av_tvsystem_t eTvSystem, int index, hi_snap_osd_t pSnapOsd[] )
{
    //DEBUG_MESSAGE("#####CHiAvEncoder::Ae_snap_regin_create\n");
    if( m_stuOsdTemp[index].bInited )
    {
        return 0;
    }

    HI_S32 sRet = HI_SUCCESS;
    RGN_ATTR_S stuRgnAttr;
    memset(&stuRgnAttr, 0, sizeof(RGN_ATTR_S));
    int fontWidth, fontHeight;
    int horScaler, verScaler;
    int font;
    int charactorNum = 0;

	if(OSD_TEST_FLAG == pgAvDeviceInfo->Adi_product_type())	//6AII_AV12 product snap osd
	{
		if((pSnapOsd[index].bShow == 1) && (0 != strlen(pSnapOsd[index].szStr)))
		{
			stuRgnAttr.unAttr.stOverlay.stSize.u32Width = 0;
			Ae_get_osd_parameter(_AON_EXTEND_OSD, eResolution, &font, &fontWidth, &fontHeight, &horScaler, &verScaler);
			charactorNum = strlen(pSnapOsd[index].szStr);
			if(pSnapOsd[index].type == 0)	//time date is different
			{
				switch(eResolution)
				{
					case _AR_1080_:
					case _AR_720_:
					case _AR_960P_:
						stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler) / 2 + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
						stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + 4) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
						break;
					default:
						stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler) * 2 / 3 + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
						stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + 4) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
						break;
				}
			}
			else
			{
				CAvUtf8String bus_number(pSnapOsd[index].szStr);
				av_ucs4_t char_unicode;
				av_char_lattice_info_t lattice_char_info;
				while(bus_number.Next_char(&char_unicode) == true)
				{
					if(m_pAv_font_library->Afl_get_char_lattice(font, char_unicode, &lattice_char_info) == 0)
					{
						stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
					}
					else
					{
						//DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
						continue;//return -1;
					}
				}
				if(stuRgnAttr.unAttr.stOverlay.stSize.u32Width == 0)
				{
					return -1;
				}
				stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
				stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;	
			}
		}
		goto _SNAP_REGION_CREATE_;
	}

    if( (pSnapOsd[index].bShow ==1) && (strlen(pSnapOsd[index].szStr) != 0) && index == 0 )//date
    {
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
        Ae_get_osd_parameter( _AON_DATE_TIME_, eResolution, &font, &fontWidth, &fontHeight, &horScaler, &verScaler );
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
        Ae_get_osd_parameter( _AON_BUS_NUMBER_, eResolution, &font, &fontWidth, &fontHeight, &horScaler, &verScaler );
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
            if(m_pAv_font_library->Afl_get_char_lattice(font, char_unicode, &lattice_char_info) == 0)
            {
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
            }
            else
            {
                //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                continue;//return -1;
            }
        }
        if(stuRgnAttr.unAttr.stOverlay.stSize.u32Width == 0)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }
        //stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + 15) / 16 * 16;
        //stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + 15) / 16 * 16;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;  
        //DEBUG_MESSAGE("index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
    }
    if((pSnapOsd[index].bShow ==1) && (strlen(pSnapOsd[index].szStr) != 0) && index == 2)//speed
    {
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
        Ae_get_osd_parameter( _AON_SPEED_INFO_, eResolution, &font, &fontWidth, &fontHeight, &horScaler, &verScaler );
        charactorNum = 10;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler)  + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        //DEBUG_MESSAGE("index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
    }
    if( (pSnapOsd[index].bShow ==1) && (strlen(pSnapOsd[index].szStr) != 0) && index == 3)//gps
    {
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
        Ae_get_osd_parameter( _AON_GPS_INFO_, eResolution, &font, &fontWidth, &fontHeight, &horScaler, &verScaler );
        charactorNum = 32;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler)  + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        //DEBUG_MESSAGE("index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
    }
    
    if( (pSnapOsd[index].bShow ==1) && (strlen(pSnapOsd[index].szStr) != 0) && index == 4)//chn_name
    {
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
        Ae_get_osd_parameter( _AON_CHN_NAME_, eResolution, &font, &fontWidth, &fontHeight, &horScaler, &verScaler );
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
            if(m_pAv_font_library->Afl_get_char_lattice(font, char_unicode, &lattice_char_info) == 0)
            {
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
            }
            else
            {
                //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                continue;//return -1;
            }
        }
        if(stuRgnAttr.unAttr.stOverlay.stSize.u32Width == 0)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;  
        //DEBUG_MESSAGE("index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
    }
    
    if( (pSnapOsd[index].bShow ==1) && (strlen(pSnapOsd[index].szStr) != 0) && index == 5) //selfnumber
    {
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
        Ae_get_osd_parameter( _AON_SELFNUMBER_INFO_, eResolution, &font, &fontWidth, &fontHeight, &horScaler, &verScaler );
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
            if(m_pAv_font_library->Afl_get_char_lattice(font, char_unicode, &lattice_char_info) == 0)
            {
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
            }
            else
            {
                //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                continue;//return -1;
            }
        }
        if(stuRgnAttr.unAttr.stOverlay.stSize.u32Width == 0)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;  
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
_SNAP_REGION_CREATE_:
    if( (pSnapOsd[index].bShow ==1) && (strlen(pSnapOsd[index].szStr) != 0) )
    {
        stuRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
        stuRgnAttr.unAttr.stOverlay.u32BgColor = 0x00;

        m_stuOsdTemp[index].hOsdRegin = /*pgAvDeviceInfo->Adi_max_channel_number() * 3 +*/ index;
        m_stuOsdTemp[index].vencChn = vencChn;
        
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

        stuMppChn.enModId = HI_ID_VENC;
        stuMppChn.s32DevId = 0;
        stuMppChn.s32ChnId = vencChn;

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
        
        if( (sRet=HI_MPI_RGN_AttachToChn(m_stuOsdTemp[index].hOsdRegin, &stuMppChn, &stuRgnChnAttr)) != HI_SUCCESS )
        {        
            DEBUG_ERROR( "CHiAvEncoder::Ae_snap_create_regin HI_MPI_RGN_AttachToChn FAILED(0x%08x)\n", sRet);

            HI_MPI_RGN_Destroy( m_stuOsdTemp[index].hOsdRegin );
            return -1;
        }

		if(OSD_TEST_FLAG == pgAvDeviceInfo->Adi_product_type())
		{
            m_stuOsdTemp[index].eOsdName = _AON_EXTEND_OSD;
		}
		else
		{
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
		}
        m_stuOsdTemp[index].eResolution = eResolution;

        /*bitmap*/
        m_stuOsdTemp[index].u32BitmapWidth = stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
        m_stuOsdTemp[index].u32BitmapHeight = stuRgnAttr.unAttr.stOverlay.stSize.u32Height;
        m_stuOsdTemp[index].pszBitmap = new unsigned char[m_stuOsdTemp[index].u32BitmapWidth * horScaler * m_stuOsdTemp[index].u32BitmapHeight * verScaler * 2];

        _AV_FATAL_(  m_stuOsdTemp[index].pszBitmap == NULL, "OUT OF MEMORY\n" );
        
        m_stuOsdTemp[index].bInited = true;
        m_stuOsdTemp[index].osdTextColor = pSnapOsd[index].color;
    }
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
    memset(&stuRgnAttr, 0, sizeof(RGN_ATTR_S));
    int fontWidth, fontHeight;
    int horScaler, verScaler;
    int font;
    int charactorNum = 0;

	if( (pSnapOsd[index].bShow ==1) && (strlen(pSnapOsd[index].szStr) != 0) && index == 0 )//date
    {
		stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
		Ae_get_osd_parameter( _AON_DATE_TIME_, eResolution, &font, &fontWidth, &fontHeight, &horScaler, &verScaler );
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
        Ae_get_osd_parameter( _AON_BUS_NUMBER_, eResolution, &font, &fontWidth, &fontHeight, &horScaler, &verScaler );
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
            if(m_pAv_font_library->Afl_get_char_lattice(font, char_unicode, &lattice_char_info) == 0)
            {
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
            }
            else
            {
                //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                continue;//return -1;
            }
        }
        if(stuRgnAttr.unAttr.stOverlay.stSize.u32Width == 0)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }
        //stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + 15) / 16 * 16;
        //stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + 15) / 16 * 16;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;  
		//DEBUG_MESSAGE("index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
    }
	if((pSnapOsd[index].bShow ==1) && (strlen(pSnapOsd[index].szStr) != 0) && index == 2)//speed
    {
		stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
		Ae_get_osd_parameter( _AON_SPEED_INFO_, eResolution, &font, &fontWidth, &fontHeight, &horScaler, &verScaler );
        charactorNum = 10;
		stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler)  + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
		//DEBUG_MESSAGE("index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
    }
    if( (pSnapOsd[index].bShow ==1) && (strlen(pSnapOsd[index].szStr) != 0) && index == 3)//gps
    {
		stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
		Ae_get_osd_parameter( _AON_GPS_INFO_, eResolution, &font, &fontWidth, &fontHeight, &horScaler, &verScaler );
        charactorNum = 32;
		stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler)  + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        //DEBUG_MESSAGE("index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
    }
	
	if( (pSnapOsd[index].bShow ==1) && (strlen(pSnapOsd[index].szStr) != 0) && index == 4)//chn_name
    {
		stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
		Ae_get_osd_parameter( _AON_CHN_NAME_, eResolution, &font, &fontWidth, &fontHeight, &horScaler, &verScaler );
		//DEBUG_MESSAGE("pSnapOsd[%d].szStr %s\n", index, pSnapOsd[index].szStr);
		char* channelname = pSnapOsd[index].szStr;
        channelname[strlen(pSnapOsd[index].szStr)] ='\0';
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
            if(m_pAv_font_library->Afl_get_char_lattice(font, char_unicode, &lattice_char_info) == 0)
            {
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
            }
            else
            {
                //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                continue;//return -1;
            }
        }
        if(stuRgnAttr.unAttr.stOverlay.stSize.u32Width == 0)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((stuRgnAttr.unAttr.stOverlay.stSize.u32Width+10) * horScaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;  
		//DEBUG_MESSAGE("index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
    }
	
	if( (pSnapOsd[index].bShow ==1) && (strlen(pSnapOsd[index].szStr) != 0) && index == 5) //selfnumber
    {
		stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
		Ae_get_osd_parameter( _AON_SELFNUMBER_INFO_, eResolution, &font, &fontWidth, &fontHeight, &horScaler, &verScaler );
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
            if(m_pAv_font_library->Afl_get_char_lattice(font, char_unicode, &lattice_char_info) == 0)
            {
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
            }
            else
            {
                //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                continue;//return -1;
            }
        }
        if(stuRgnAttr.unAttr.stOverlay.stSize.u32Width == 0)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;  
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

        
        //printf("index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
        stuRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
        stuRgnAttr.unAttr.stOverlay.u32BgColor = 0x00;

        m_stuOsdTemp[index].hOsdRegin = /*pgAvDeviceInfo->Adi_max_channel_number() * 3 +*/ index;
        m_stuOsdTemp[index].vencGrp = vencGrp;
        
       
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
        if( (sRet=HI_MPI_RGN_AttachToChn(m_stuOsdTemp[index].hOsdRegin, &stuMppChn, &stuRgnChnAttr)) != HI_SUCCESS )
        {        
            DEBUG_ERROR( "CHiAvEncoder::Ae_snap_create_regin HI_MPI_RGN_AttachToChn FAILED(0x%08x)\n", sRet);

            HI_MPI_RGN_Destroy( m_stuOsdTemp[index].hOsdRegin );
            return -1;
        }

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
#endif

int CHiAvEncoder::Ae_snap_regin_delete( int index )
{
    if( !m_stuOsdTemp[index].bInited )
    {
        return 0;
    }

    MPP_CHN_S stuMppChn;
#ifdef _AV_PLATFORM_HI3520D_V300_
    stuMppChn.enModId = HI_ID_VENC;
    stuMppChn.s32DevId = 0;
    stuMppChn.s32ChnId = m_stuOsdTemp[index].vencChn;
#else
    stuMppChn.enModId = HI_ID_GROUP;
    stuMppChn.s32DevId = m_stuOsdTemp[index].vencGrp;
    stuMppChn.s32ChnId = 0;
#endif

    HI_S32 sRet = HI_SUCCESS;
#ifdef _AV_PLATFORM_HI3520D_V300_
    if( (sRet=HI_MPI_RGN_DetachFromChn( m_stuOsdTemp[index].hOsdRegin, &stuMppChn)) != HI_SUCCESS )
#else
    if( (sRet=HI_MPI_RGN_DetachFrmChn( m_stuOsdTemp[index].hOsdRegin, &stuMppChn)) != HI_SUCCESS )
#endif
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_snap_delete_regin HI_MPI_RGN_DetachFrmChn FAILED(0x%08x)\n", sRet);
    }

    if( (sRet=HI_MPI_RGN_Destroy(m_stuOsdTemp[index].hOsdRegin)) != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_snap_delete_regin HI_MPI_RGN_Destroy FAILED(0x%08x)\n", sRet);
    }
    if(m_stuOsdTemp[index].pszBitmap != NULL)
    {
        delete m_stuOsdTemp[index].pszBitmap;
        m_stuOsdTemp[index].pszBitmap = NULL;
    }
    m_stuOsdTemp[index].bInited = false;

    return 0;
}

int CHiAvEncoder::Ae_encode_vi_to_picture( int vi_chn, VIDEO_FRAME_INFO_S *pstuViFrame, hi_snap_osd_t stuOsdParam[], int osd_num,  VENC_STREAM_S* pstuPicStream)
{
    HI_S32 sRet = HI_SUCCESS;
    VENC_GRP snapGrp;
    VENC_CHN snapChl;
    if( Ae_video_encoder_allotter_snap(&snapGrp, &snapChl) != 0 )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_encode_vi_to_picture Ae_video_encoder_allotter_snap FAILED\n");
        return -1;
    }
    Ae_snap_regin_set_osds(stuOsdParam, osd_num);
    static unsigned int u32TimeRef = 2;
    pstuViFrame->stVFrame.u32TimeRef = u32TimeRef;
    u32TimeRef += 2;
#ifdef _AV_PLATFORM_HI3520D_V300_
    sRet = HI_MPI_VENC_SendFrame( snapGrp, pstuViFrame, 0);
#else
    sRet = HI_MPI_VENC_SendFrame( snapGrp, pstuViFrame );
#endif
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
        return -1;
    }

    if( stuVencChnStat.u32CurPacks == 0 )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_encode_vi_to_picture err because packcount=%d\n", stuVencChnStat.u32CurPacks);
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
        return -1;
    }

    return 0;
}

int CHiAvEncoder::Ae_encoder_vi_to_picture_free( VENC_STREAM_S* pstuPicStream )
{
    VENC_GRP snapGrp;
    VENC_CHN snapChl;
    if( Ae_video_encoder_allotter_snap(&snapGrp, &snapChl) != 0 )
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
    VENC_GRP venc_grp = -1;
    VENC_CHN venc_chn = -1;

    if(Ae_video_encoder_allotter(video_stream_type, phy_video_chn_num, &venc_grp, &venc_chn) != 0)
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
#if defined(_AV_PLATFORM_HI3520D_V300_)
    mpp_chn_dst.enModId = HI_ID_VENC;
    mpp_chn_dst.s32DevId = 0;
    mpp_chn_dst.s32ChnId = venc_chn;
#else
    mpp_chn_dst.enModId = HI_ID_GROUP;
    mpp_chn_dst.s32DevId = venc_grp;
    mpp_chn_dst.s32ChnId = 0;
#endif
    if(bind_flag == true)
    {
        if((ret_val = HI_MPI_SYS_Bind(&mpp_chn_src, &mpp_chn_dst)) != HI_SUCCESS)
        {
            DEBUG_ERROR( "CHiAvEncoder::Ae_venc_vpss_control HI_MPI_SYS_Bind FAILED(type:%d)(chn:%d)(grp:%d, %d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, venc_grp, venc_chn, (unsigned long)ret_val);
            return -1;
        }
    }
    else
    {
        if((ret_val = HI_MPI_SYS_UnBind(&mpp_chn_src, &mpp_chn_dst)) != HI_SUCCESS)
        {
            DEBUG_ERROR( "CHiAvEncoder::Ae_venc_vpss_control HI_MPI_SYS_UnBind FAILED(type:%d)(chn:%d)(grp:%d, %d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, venc_grp, venc_chn, (unsigned long)ret_val);
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
    VENC_GRP venc_grp = -1;
    VENC_CHN venc_chn = -1;

    if(Ae_video_encoder_allotter(video_stream_type, phy_video_chn_num, &venc_grp, &venc_chn) != 0)
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
#ifdef _AV_PLATFORM_HI3520D_V300_
            venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
            venc_chn_attr.stRcAttr.stAttrH264Cbr.fr32DstFrmRate = pVideo_para->frame_rate;
#else
            venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBRv2;
            venc_chn_attr.stRcAttr.stAttrH264Cbr.fr32TargetFrmRate = pVideo_para->frame_rate;
#endif
	     venc_chn_attr.stRcAttr.stAttrH264Cbr.u32Gop = pVideo_para->gop_size;
            venc_chn_attr.stRcAttr.stAttrH264Cbr.u32BitRate = pVideo_para->bitrate.hicbr.bitrate;
            venc_chn_attr.stRcAttr.stAttrH264Cbr.u32StatTime = 1;
            venc_chn_attr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
#ifdef _AV_PRODUCT_CLASS_IPC_
            venc_chn_attr.stRcAttr.stAttrH264Cbr.u32ViFrmRate = pVideo_para->source_frame_rate;
#endif
            break;

        case _ABM_VBR_:
#ifdef _AV_PLATFORM_HI3520D_V300_
            venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
            venc_chn_attr.stRcAttr.stAttrH264Vbr.fr32DstFrmRate = pVideo_para->frame_rate;
#else
            venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBRv2;
            venc_chn_attr.stRcAttr.stAttrH264Vbr.fr32TargetFrmRate = pVideo_para->frame_rate;
#endif
            venc_chn_attr.stRcAttr.stAttrH264Vbr.u32Gop = pVideo_para->gop_size;
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
#ifdef _AV_PRODUCT_CLASS_IPC_
            venc_chn_attr.stRcAttr.stAttrH264Vbr.u32ViFrmRate = pVideo_para->source_frame_rate;
#endif            
            break;

        default:
            _AV_FATAL_(1, "CHiAvEncoder::Ae_dynamic_modify_encoder_local You must give the implement(%d)\n", pVideo_para->bitrate_mode);
            break;
    }

#if defined(_AV_HAVE_VIDEO_INPUT_)
#ifndef _AV_PLATFORM_HI3520D_V300_
    GROUP_FRAME_RATE_S group_frame_rate;
#ifdef _AV_PRODUCT_CLASS_IPC_
    group_frame_rate.s32ViFrmRate = pVideo_para->source_frame_rate;
#else
    vi_config_set_t vi_primary_config, vi_minor_config, vi_source_config;
    m_pHi_vi->HiVi_get_primary_vi_chn_config(phy_video_chn_num, &vi_primary_config, &vi_minor_config, &vi_source_config);
    group_frame_rate.s32ViFrmRate = vi_source_config.frame_rate;
#endif
    group_frame_rate.s32VpssFrmRate = pVideo_para->frame_rate;
    if (group_frame_rate.s32VpssFrmRate > group_frame_rate.s32ViFrmRate)
    {
        DEBUG_WARNING( "CHiAvEncoder::Ae_dynamic_modify_encoder_local vpssfr beyond vifr(type:%d)(chn:%d)(grp:%d, %d)(vifr:%d)(vpssfr:%d)(fr %d)\n", video_stream_type, phy_video_chn_num, venc_grp, venc_chn, group_frame_rate.s32ViFrmRate, group_frame_rate.s32VpssFrmRate, pVideo_para->frame_rate);
        group_frame_rate.s32VpssFrmRate = group_frame_rate.s32ViFrmRate;
        venc_chn_attr.stRcAttr.stAttrH264Cbr.fr32TargetFrmRate = group_frame_rate.s32ViFrmRate;
    }
    
    DEBUG_WARNING( "CHiAvEncoder::Ae_dynamic_modify_encoder_local (type:%d)(chn:%d)(grp:%d, %d)(s32ViFrmRate:%d)(s32VpssFrmRate:%d)(fr %d)(fr32TargetFrmRate %d)\n", 
        video_stream_type, phy_video_chn_num, venc_grp, venc_chn, group_frame_rate.s32ViFrmRate, group_frame_rate.s32VpssFrmRate, pVideo_para->frame_rate, venc_chn_attr.stRcAttr.stAttrH264Cbr.fr32TargetFrmRate);

    
    if((ret_val = HI_MPI_VENC_SetGrpFrmRate(venc_grp, &group_frame_rate)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder HI_MPI_VENC_SetGrpFrmRate FAILED(type:%d)(chn:%d)(grp:%d, %d)(ret_val:0x%08lx)(vifr:%d)(vpssfr:%d)(fr %d)\n", video_stream_type, phy_video_chn_num, venc_grp, venc_chn, (unsigned long)ret_val, group_frame_rate.s32ViFrmRate, group_frame_rate.s32VpssFrmRate, pVideo_para->frame_rate);
//        return -1;
    }

#else
#endif
#endif

    if((ret_val = HI_MPI_VENC_SetChnAttr(venc_chn, &venc_chn_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_dynamic_modify_encoder_local HI_MPI_VENC_SetChnAttr FAILED(0x%08x)!(cbr:%d,vbr:%d, res:%d, fr:%d)\n",\
            ret_val,pVideo_para->bitrate.hicbr.bitrate,pVideo_para->bitrate.hivbr.bitrate,  pVideo_para->resolution,pVideo_para->frame_rate);
        
        return -1;
    }

    return 0;
}


int CHiAvEncoder::Ae_create_h264_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, video_encoder_id_t *pVideo_encoder_id)
{
    VENC_GRP venc_grp = -1;
    VENC_CHN venc_chn = -1;
    HI_S32 ret_val = -1;
#ifndef _AV_PLATFORM_HI3520D_V300_
    VENC_ATTR_H264_REF_MODE_E enRefMode = H264E_REF_MODE_1X;
#endif
    if(Ae_video_encoder_allotter(video_stream_type, phy_video_chn_num, &venc_grp, &venc_chn) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder Ae_video_encoder_allotter FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
        return -1;
    }

#if defined(_AV_PLATFORM_HI3531_) || defined(_AV_PLATFORM_HI3532_)
    const char *pMemory_name = NULL;
    MPP_CHN_S mpp_chn;

    if(((venc_chn + 1) % 3) != 0)
    {
        pMemory_name = "ddr1";
    }
    mpp_chn.enModId  = HI_ID_GROUP;
    mpp_chn.s32DevId = venc_grp;
    mpp_chn.s32ChnId = 0;
    if((ret_val = HI_MPI_SYS_SetMemConf(&mpp_chn, pMemory_name)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder HI_MPI_SYS_SetMemConf FAILED(0x%08x)\n", ret_val);
    }
    mpp_chn.enModId = HI_ID_VENC;
    mpp_chn.s32DevId = 0;
    mpp_chn.s32ChnId = venc_chn;
    if((ret_val = HI_MPI_SYS_SetMemConf(&mpp_chn, pMemory_name)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder HI_MPI_SYS_SetMemConf FAILED(0x%08x)\n", ret_val);
    }
#endif

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
    char custoer_name[32] = {0};
    pgAvDeviceInfo->Adi_get_customer_name(custoer_name, sizeof(custoer_name));

    char platform_name[32] = {0};
    pgAvDeviceInfo->Adi_get_platform_name(platform_name, sizeof(platform_name));
    
    if((0 == strcmp(custoer_name, "CUSTOMER_HST"))||(0 == strcmp(custoer_name, "CUSTOMER_BOKANG"))||\
        (0 == strcmp(platform_name, "HONGXIN")))
    {
        switch(video_stream_type)
        {
                case _AST_MAIN_VIDEO_:
                default:
                    venc_chn_attr.stVeAttr.stAttrH264e.u32Profile = 0;/*baseline*/
                    break;
                case _AST_SUB_VIDEO_:
                    venc_chn_attr.stVeAttr.stAttrH264e.u32Profile = 0;
                    break;
                case _AST_SUB2_VIDEO_:
                    venc_chn_attr.stVeAttr.stAttrH264e.u32Profile = 0;
                    break;           
        }
    }
    else
    {
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
    }
    
    /*get frame mode*/
    venc_chn_attr.stVeAttr.stAttrH264e.bByFrame = HI_TRUE;
#ifndef _AV_PLATFORM_HI3520D_V300_
    /*field*/
    venc_chn_attr.stVeAttr.stAttrH264e.bField = HI_FALSE;
    /*Is main stream*/
    venc_chn_attr.stVeAttr.stAttrH264e.bMainStream = HI_TRUE;
    /**/
    venc_chn_attr.stVeAttr.stAttrH264e.u32Priority = 0;
    /**/
    venc_chn_attr.stVeAttr.stAttrH264e.bVIField = HI_FALSE;
#else
    venc_chn_attr.stVeAttr.stAttrH264e.u32BFrameNum = 0 ;
    venc_chn_attr.stVeAttr.stAttrH264e.u32RefNum = 0;
#endif	
    /**/
    venc_chn_attr.stVeAttr.stAttrH264e.u32PicWidth = venc_chn_attr.stVeAttr.stAttrH264e.u32MaxPicWidth;
    venc_chn_attr.stVeAttr.stAttrH264e.u32PicHeight = venc_chn_attr.stVeAttr.stAttrH264e.u32MaxPicHeight;

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

#ifdef _AV_HAVE_VIDEO_INPUT_
    vi_config_set_t vi_primary_config, vi_minor_config, vi_source_config;
    memset(&vi_source_config, 0, sizeof(vi_config_set_t));
    m_pHi_vi->HiVi_get_primary_vi_chn_config(phy_video_chn_num, &vi_primary_config, &vi_minor_config, &vi_source_config);
    if ( 0 == vi_source_config.frame_rate)
    {
        DEBUG_ERROR(" Obtain primary vi chn config failed!\n");
        return -1;
    }
#endif

    /****************bitrate*****************/
    switch(pVideo_para->bitrate_mode)
    {
        case _ABM_CBR_:
#ifdef _AV_PLATFORM_HI3520D_V300_
            venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
            venc_chn_attr.stRcAttr.stAttrH264Cbr.fr32DstFrmRate = pVideo_para->frame_rate;
            venc_chn_attr.stRcAttr.stAttrH264Cbr.u32SrcFrmRate = vi_source_config.frame_rate;			
#else
            venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBRv2;
            venc_chn_attr.stRcAttr.stAttrH264Cbr.fr32TargetFrmRate = pVideo_para->frame_rate;
            venc_chn_attr.stRcAttr.stAttrH264Cbr.u32ViFrmRate = vi_source_config.frame_rate;			
#endif
            venc_chn_attr.stRcAttr.stAttrH264Cbr.u32Gop = pVideo_para->gop_size;
            venc_chn_attr.stRcAttr.stAttrH264Cbr.u32StatTime = 1;
            if (pVideo_para->bitrate.hicbr.bitrate < 2)
            {
                DEBUG_ERROR(" Bitrate(%d) invalid\n!",pVideo_para->bitrate.hicbr.bitrate);
                pVideo_para->bitrate.hicbr.bitrate = 2;              
            }
            venc_chn_attr.stRcAttr.stAttrH264Cbr.u32BitRate = pVideo_para->bitrate.hicbr.bitrate;
            venc_chn_attr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
#ifdef _AV_PRODUCT_CLASS_IPC_
            venc_chn_attr.stRcAttr.stAttrH264Cbr.u32ViFrmRate = pVideo_para->source_frame_rate;     
#endif
            break;

        case _ABM_VBR_:
#ifdef _AV_PLATFORM_HI3520D_V300_
            venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
            venc_chn_attr.stRcAttr.stAttrH264Vbr.fr32DstFrmRate = pVideo_para->frame_rate;
            venc_chn_attr.stRcAttr.stAttrH264Vbr.u32SrcFrmRate = vi_source_config.frame_rate;			
#else
            venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBRv2;
            venc_chn_attr.stRcAttr.stAttrH264Vbr.fr32TargetFrmRate = pVideo_para->frame_rate;
            venc_chn_attr.stRcAttr.stAttrH264Vbr.u32ViFrmRate = vi_source_config.frame_rate;
#endif
            venc_chn_attr.stRcAttr.stAttrH264Vbr.u32Gop = pVideo_para->gop_size;
            if(PTD_712_VF == pgAvDeviceInfo->Adi_product_type())
            {
                venc_chn_attr.stRcAttr.stAttrH264Vbr.u32StatTime = 4;
            }
            else
            {
                venc_chn_attr.stRcAttr.stAttrH264Vbr.u32StatTime = 1;
            }
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
            if (pVideo_para->bitrate.hivbr.bitrate < 2)
            {
                DEBUG_ERROR(" Bitrate:%d   invalid\n!",pVideo_para->bitrate.hivbr.bitrate);
                pVideo_para->bitrate.hivbr.bitrate = 2;               
            }
            venc_chn_attr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = pVideo_para->bitrate.hivbr.bitrate;
#ifndef _AV_PLATFORM_HI3520D_V300_
            venc_chn_attr.stRcAttr.stAttrH264Vbr.fr32TargetFrmRate = pVideo_para->frame_rate;
            venc_chn_attr.stRcAttr.stAttrH264Vbr.u32ViFrmRate = vi_source_config.frame_rate;
#endif

#ifdef _AV_PRODUCT_CLASS_IPC_
            venc_chn_attr.stRcAttr.stAttrH264Vbr.u32ViFrmRate = pVideo_para->source_frame_rate;
#endif
            break;

        default:
            _AV_FATAL_(1, "CHiAvEncoder::Ae_create_h264_encoder You must give the implement(%d)\n", pVideo_para->bitrate_mode);
            break;
    }

#ifndef _AV_PLATFORM_HI3520D_V300_
    if((ret_val = HI_MPI_VENC_CreateGroup(venc_grp)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder HI_MPI_VENC_CreateGroup FAILED(type:%d)(chn:%d)(grp:%d, %d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, venc_grp, venc_chn, (unsigned long)ret_val);

        return -1;

    }
#if defined(_AV_HAVE_VIDEO_INPUT_) 
    GROUP_FRAME_RATE_S group_frame_rate;
#ifdef _AV_PRODUCT_CLASS_IPC_
    group_frame_rate.s32ViFrmRate = pVideo_para->source_frame_rate;
#else
    group_frame_rate.s32ViFrmRate = vi_source_config.frame_rate;
#endif
    group_frame_rate.s32VpssFrmRate = pVideo_para->frame_rate;
    if (group_frame_rate.s32VpssFrmRate > group_frame_rate.s32ViFrmRate)
    {
        DEBUG_WARNING( "CHiAvEncoder::Ae_create_h264_encoder vpssfr beyond vifr(type:%d)(chn:%d)(grp:%d, %d)(vifr:%d)(vpssfr:%d)(fr %d)\n", video_stream_type, phy_video_chn_num, venc_grp, venc_chn, group_frame_rate.s32ViFrmRate, group_frame_rate.s32VpssFrmRate, pVideo_para->frame_rate);
        group_frame_rate.s32VpssFrmRate = group_frame_rate.s32ViFrmRate;
        venc_chn_attr.stRcAttr.stAttrH264Cbr.fr32TargetFrmRate = group_frame_rate.s32ViFrmRate;
    }
    
    //DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder (type:%d)(chn:%d)(grp:%d, %d)(s32ViFrmRate:%d)(s32VpssFrmRate:%d)(fr %d)(fr32TargetFrmRate %d)\n", 
    //    video_stream_type, phy_video_chn_num, venc_grp, venc_chn, group_frame_rate.s32ViFrmRate, group_frame_rate.s32VpssFrmRate, pVideo_para->frame_rate, venc_chn_attr.stRcAttr.stAttrH264Cbr.fr32TargetFrmRate);

    if((ret_val = HI_MPI_VENC_SetGrpFrmRate(venc_grp, &group_frame_rate)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder HI_MPI_VENC_SetGrpFrmRate FAILED(type:%d)(chn:%d)(grp:%d, %d)(ret_val:0x%08lx)(vifr:%d)(vpssfr:%d)(fr %d)\n", video_stream_type, phy_video_chn_num, venc_grp, venc_chn, (unsigned long)ret_val, group_frame_rate.s32ViFrmRate, group_frame_rate.s32VpssFrmRate, pVideo_para->frame_rate);
        // return -1;
    }
#endif
#endif
    if((ret_val = HI_MPI_VENC_CreateChn(venc_chn, &venc_chn_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder HI_MPI_VENC_CreateChn FAILED(type:%d)(chn:%d)(grp:%d, %d)(ret_val:0x%08lx)\
            (cbr:%d)(vbr:%d)(res:%d)(fr:%d) \n", \
             video_stream_type, phy_video_chn_num, venc_grp, venc_chn, (unsigned long)ret_val, pVideo_para->bitrate.hicbr.bitrate,
             pVideo_para->bitrate.hivbr.bitrate, pVideo_para->resolution, pVideo_para->frame_rate);

#ifndef _AV_PLATFORM_HI3520D_V300_
        HI_MPI_VENC_DestroyGroup(venc_grp); //  destroy in case of being occupied
#endif
        return -1;
    }

#ifdef _AV_PLATFORM_HI3520D_V300_

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
        HI_MPI_VENC_DestroyChn(venc_chn);  
	return -1;
    }

    if((ret_val = HI_MPI_VENC_StartRecvPic(venc_chn)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder HI_MPI_VENC_StartRecvPic FAILED(type:%d)(chn:%d)(venc chn: %d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, venc_chn, (unsigned long)ret_val);
        //!  destroy in case of being occupied
        HI_MPI_VENC_DestroyChn(venc_chn);      
        return -1;
    }

#if defined(_AV_HISILICON_VPSS_)
    if(Ae_venc_vpss_control(video_stream_type, phy_video_chn_num, true) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder Ae_venc_vpss_control FAILED(type:%d)(chn:%d)(venc chn: %d)\n", video_stream_type, phy_video_chn_num, venc_chn);
        HI_MPI_VENC_DestroyChn(venc_chn);      
        return -1;
    }
#endif

#else
    if (pVideo_para->ref_para.ref_type == _ART_REF_FOR_CUSTOM)
    {
        VENC_ATTR_H264_REF_PARAM_S venc_h264_ref_para;

        venc_h264_ref_para.bEnablePred = HI_FALSE;
        if (pVideo_para->ref_para.pred_enable == 1)
        {
            venc_h264_ref_para.bEnablePred = HI_TRUE;
        }
        venc_h264_ref_para.u32Base = pVideo_para->ref_para.base_interval;
        venc_h264_ref_para.u32Enhance = pVideo_para->ref_para.enhance_interval;
        DEBUG_WARNING( "ref custom eable %d base %d enhance %d\n", venc_h264_ref_para.bEnablePred, venc_h264_ref_para.u32Base, venc_h264_ref_para.u32Enhance);
        if((ret_val = HI_MPI_VENC_SetH264eRefParam(venc_chn, &venc_h264_ref_para)) != HI_SUCCESS)
        {
            DEBUG_ERROR( "CHiAvEncoder::HI_MPI_VENC_SetH264eRefParam HI_MPI_VENC_SetH264eRefMode FAILED(type:%d)(chn:%d)(grp:%d, %d)(enRefMode:%d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, venc_grp, venc_chn, (int)enRefMode, (unsigned long)ret_val);
            //!  destroy in case of being occupied
            
            HI_MPI_VENC_DestroyChn(venc_chn);  
            HI_MPI_VENC_DestroyGroup(venc_grp);//
            return -1;
        }
    }
    else
    {
        if (_ART_REF_FOR_2X == pVideo_para->ref_para.ref_type)
        {
            enRefMode = H264E_REF_MODE_2X;
        }
        else if (_ART_REF_FOR_4X == pVideo_para->ref_para.ref_type)
        {
            enRefMode = H264E_REF_MODE_4X;
        }
        else
        {
            enRefMode = H264E_REF_MODE_1X;
        }
        
        if((ret_val = HI_MPI_VENC_SetH264eRefMode(venc_chn, enRefMode)) != HI_SUCCESS)
        {
            DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder HI_MPI_VENC_SetH264eRefMode FAILED(type:%d)(chn:%d)(grp:%d, %d)(enRefMode:%d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, venc_grp, venc_chn, (int)enRefMode, (unsigned long)ret_val);
            HI_MPI_VENC_DestroyChn(venc_chn); 
            HI_MPI_VENC_DestroyGroup(venc_grp);//
            return -1;
        }
    }
    if((ret_val = HI_MPI_VENC_RegisterChn(venc_grp, venc_chn)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder HI_MPI_VENC_RegisterChn FAILED(type:%d)(chn:%d)(grp:%d, %d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, venc_grp, venc_chn, (unsigned long)ret_val);
        //!  destroy in case of being occupied
        HI_MPI_VENC_DestroyChn(venc_chn); 
        HI_MPI_VENC_DestroyGroup(venc_grp);//
        return -1;
    }

    if((ret_val = HI_MPI_VENC_StartRecvPic(venc_chn)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder HI_MPI_VENC_StartRecvPic FAILED(type:%d)(chn:%d)(grp:%d, %d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, venc_grp, venc_chn, (unsigned long)ret_val);
        //!  destroy in case of being occupied
        HI_MPI_VENC_UnRegisterChn(venc_chn);
        HI_MPI_VENC_DestroyChn(venc_chn);      
	HI_MPI_VENC_DestroyGroup(venc_grp);//
        return -1;
    }

#if defined(_AV_HISILICON_VPSS_)
    if(Ae_venc_vpss_control(video_stream_type, phy_video_chn_num, true) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder Ae_venc_vpss_control FAILED(type:%d)(chn:%d)(grp:%d, %d)\n", video_stream_type, phy_video_chn_num, venc_grp, venc_chn);
        HI_MPI_VENC_UnRegisterChn(venc_chn);
        HI_MPI_VENC_DestroyChn(venc_chn);      
        HI_MPI_VENC_DestroyGroup(venc_grp);//
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
    pVei->venc_grp = venc_grp;
    pVideo_encoder_id->pVideo_encoder_info = (void *)pVei;

    //DEBUG_MESSAGE( "CHiAvEncoder::Ae_create_h264_encoder(type:%d)(chn:%d)(res:%d)(frame_rate:%d)(bitrate:%d)(gop:%d)(fd:%d)\n", video_stream_type, phy_video_chn_num, pVideo_para->resolution, pVideo_para->frame_rate, pVideo_para->bitrate.hicbr.bitrate, pVideo_para->gop_size, pVideo_encoder_id->video_encoder_fd);

    return 0;
}

int CHiAvEncoder::Ae_video_encoder_allotter_V1(av_video_stream_type_t video_stream_type, int phy_video_chn_num, VENC_GRP *pGrp, VENC_CHN *pChn)
{   
    switch(video_stream_type)
    {
        case _AST_MAIN_VIDEO_:
            _AV_POINTER_ASSIGNMENT_(pGrp, phy_video_chn_num);
            _AV_POINTER_ASSIGNMENT_(pChn, phy_video_chn_num);
            break;

        case _AST_SUB_VIDEO_:
            _AV_POINTER_ASSIGNMENT_(pGrp, phy_video_chn_num + pgAvDeviceInfo->Adi_get_vi_chn_num());
            _AV_POINTER_ASSIGNMENT_(pChn, phy_video_chn_num + pgAvDeviceInfo->Adi_get_vi_chn_num());
            break;

        case _AST_SUB2_VIDEO_:
            _AV_POINTER_ASSIGNMENT_(pGrp, phy_video_chn_num + pgAvDeviceInfo->Adi_get_vi_chn_num() * 2);
            _AV_POINTER_ASSIGNMENT_(pChn, phy_video_chn_num + pgAvDeviceInfo->Adi_get_vi_chn_num() * 2);
            break;

        default:
            _AV_FATAL_(1, "CHiAvEncoder::Ae_video_encoder_allotter_V1 You must give the implement\n");
            break;
    }

    return 0;
}

int CHiAvEncoder::Ae_video_encoder_allotter_MCV1(av_video_stream_type_t video_stream_type, int phy_video_chn_num, VENC_GRP *pGrp, VENC_CHN *pChn)
{
    return -1;
}

int CHiAvEncoder::Ae_video_encoder_allotter(av_video_stream_type_t video_stream_type, int phy_video_chn_num, VENC_GRP *pGrp, VENC_CHN *pChn)
{
    int retval = -1;
    
    if(0)
    {
        retval = Ae_video_encoder_allotter_MCV1(video_stream_type, phy_video_chn_num, pGrp, pChn);
    }
    else
    {
        retval = Ae_video_encoder_allotter_V1(video_stream_type, phy_video_chn_num, pGrp, pChn);
    }
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

int CHiAvEncoder::Ae_video_encoder_allotter_snap( VENC_GRP *pGrp, VENC_CHN *pChn )
{
    _AV_POINTER_ASSIGNMENT_(pGrp, pgAvDeviceInfo->Adi_get_vi_chn_num() * 3 + 1 );
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
        //DEBUG_ERROR( "CHiAvEncoder::Ae_get_video_stream HI_MPI_VENC_Query FAILED(0x%08lx)\n", (unsigned long)ret_val);
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
#ifdef _AV_PLATFORM_HI3520D_V300_
        pStream_data->pack_list[i].pAddr[0] = pVei->venc_stream.pstPack[i].pu8Addr+pVei->venc_stream.pstPack[i].u32Offset;
        pStream_data->pack_list[i].len[0] = pVei->venc_stream.pstPack[i].u32Len - pVei->venc_stream.pstPack[i].u32Offset;
        pStream_data->frame_len += pVei->venc_stream.pstPack[i].u32Len - pVei->venc_stream.pstPack[i].u32Offset;
#else
        pStream_data->pack_list[i].pAddr[0] = pVei->venc_stream.pstPack[i].pu8Addr[0];
        pStream_data->pack_list[i].len[0] = pVei->venc_stream.pstPack[i].u32Len[0];

        pStream_data->pack_list[i].pAddr[1] = pVei->venc_stream.pstPack[i].pu8Addr[1];
        pStream_data->pack_list[i].len[1] = pVei->venc_stream.pstPack[i].u32Len[1];

        pStream_data->frame_len += (pVei->venc_stream.pstPack[i].u32Len[0] + pVei->venc_stream.pstPack[i].u32Len[1]);
#endif		
    }

    if(pStream_data->frame_len < 5)
    {
        DEBUG_WARNING( "CHiAvEncoder::Ae_get_video_stream TOO SHORT(%d)\n", pStream_data->frame_len);
        Ae_release_video_stream(pVideo_encoder_id, pStream_data);
        return -1;
    }

    unsigned char frame_type_byte = 0;
#ifdef _AV_PLATFORM_HI3520D_V300_
    if(pStream_data->pack_list[0].len[0])
    {
        frame_type_byte = *(pStream_data->pack_list[0].pAddr[0] + 4);    
    }
    else
    {
        DEBUG_ERROR("CHiAvEncoder::Ae_get_video_stream the frame head info is invalid!len is:%d \n", pStream_data->pack_list[0].len[0]);
    }
#else
    if(pStream_data->pack_list[0].len[0] > 4)
    {
        frame_type_byte = *(pStream_data->pack_list[0].pAddr[0] + 4);
    }
    else
    {
        frame_type_byte = *(pStream_data->pack_list[0].pAddr[1] + (4 - pStream_data->pack_list[0].len[0]));
    }
#endif
    switch(frame_type_byte)
    {
        case 0x67:
            pStream_data->frame_type = _AFT_IFRAME_;
            break;

        default:
            pStream_data->frame_type = _AFT_PFRAME_;
            break;
    }

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

    if(Ae_video_encoder_allotter(video_stream_type, phy_video_chn_num, NULL, &venc_chn) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_request_iframe_local Ae_video_encoder_allotter FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
        return -1;
    }
#ifdef _AV_PLATFORM_HI3520D_V300_
    HI_S32 ret_val = HI_MPI_VENC_RequestIDR(venc_chn,HI_FALSE);
#else
    HI_S32 ret_val = HI_MPI_VENC_RequestIDR(venc_chn);
#endif
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
    char product_group[32] = {0};
    pgAvDeviceInfo->Adi_get_product_group(product_group, sizeof(product_group));

    char customer_name[32] = {0};
    pgAvDeviceInfo->Adi_get_customer_name(customer_name,sizeof(customer_name));

    /*calculate the size of region*/
    Ae_get_osd_parameter(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution, &font_name, 
                                    &font_width, &font_height, &horizontal_scaler, &vertical_scaler);

    //DEBUG_MESSAGE( "CHiAvEncoder::Ae_create_osd_region(channel:%d)(osd_name:%d)(w:%d)(h:%d)(ws:%d)(hs:%d)\n", av_osd_item_it->video_stream.phy_video_chn_num, av_osd_item_it->osd_name, font_width, font_height, horizontal_scaler, vertical_scaler);

    if(av_osd_item_it->osd_name == _AON_DATE_TIME_)
    {
        if (!strcmp(customer_name, "CUSTOMER_04.935"))
        {
            if(av_osd_item_it->video_stream.video_encoder_para.time_mode == _AV_TIME_MODE_12_)
            {

                char_number = strlen("GPS: 2013-03-23  11:10:00 AM");

            }
            else
            {
                char_number = strlen("GPS: 2013-03-23  11:10:00");               
            }

            switch(av_osd_item_it->video_stream.video_encoder_para.resolution)
            {
                case _AR_1080_:
                case _AR_720_:
                case _AR_960P_:
                    region_attr.unAttr.stOverlay.stSize.u32Width  = ((font_width * char_number * horizontal_scaler) / 2 + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                    region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_ ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                    break;
                default:
                    region_attr.unAttr.stOverlay.stSize.u32Width  = ((font_width * char_number * horizontal_scaler) * 2 / 3 + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                    region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                    break;
            }
            region_attr.unAttr.stOverlay.stSize.u32Height *=2; //!< 04.935
        }
        else
    {
        if(av_osd_item_it->video_stream.video_encoder_para.time_mode == _AV_TIME_MODE_12_)
        {
            if(strcmp(product_group, "ITS") != 0)
            {
                    char_number = strlen("2013-03-23  11:10:00 AM");
                }
                else
                {
                    char_number = strlen("2013-03-23  11:10:00 AM");
                }
            }
            else
            {
                if(strcmp(product_group, "ITS") != 0)
                {
                    char_number = strlen("2013-03-23  11:10:00");
            }
            else
            {
                char_number = strlen("2013-03-23  11:10:00");
            }
            
        }

        switch(av_osd_item_it->video_stream.video_encoder_para.resolution)
        {
            case _AR_1080_:
            case _AR_720_:
            case _AR_960P_:
                region_attr.unAttr.stOverlay.stSize.u32Width  = ((font_width * char_number * horizontal_scaler) / 2 + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_ ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
            default:
                region_attr.unAttr.stOverlay.stSize.u32Width  = ((font_width * char_number * horizontal_scaler) * 2 / 3 + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
            }
        }
    }
    else if(av_osd_item_it->osd_name == _AON_CHN_NAME_)
    {
        region_attr.unAttr.stOverlay.stSize.u32Width = 0;
        char* channelname = NULL;
#if defined(_AV_PRODUCT_CLASS_IPC_)	
        char special_channel_name[32];
        if(PTD_712_VB == pgAvDeviceInfo->Adi_product_type())
        {
            memset(special_channel_name, 0, sizeof(special_channel_name));
            memcpy(special_channel_name, av_osd_item_it->video_stream.video_encoder_para.channel_name, sizeof(special_channel_name));
            int len = strlen(special_channel_name);
            if(0 == len)
            {
                pgAvDeviceInfo->Adi_get_mac_address_as_string(special_channel_name, sizeof(special_channel_name));
            }
            else
            {
                special_channel_name[len] = '-';
                ++len;
                pgAvDeviceInfo->Adi_get_mac_address_as_string(special_channel_name+len, sizeof(special_channel_name)-len);
            }

            special_channel_name[sizeof(special_channel_name)-1] =  '\0';
            channelname = special_channel_name;
        }
        else
        {
            channelname = av_osd_item_it->video_stream.video_encoder_para.channel_name;
        }
#else
        channelname = av_osd_item_it->video_stream.video_encoder_para.channel_name;
#endif
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
            if(m_pAv_font_library->Afl_get_char_lattice(font_name, char_unicode, &lattice_char_info) == 0)
            {
                region_attr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
            }
            else
            {
                //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, font_name);
                //return -1;
                continue;
            }
        }
        if(region_attr.unAttr.stOverlay.stSize.u32Width == 0)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }
        if(PTD_712_VB == pgAvDeviceInfo->Adi_product_type())
        {
            //712_VB does like this because if only up byte alignment,the osd sometimes may be unordered
            region_attr.unAttr.stOverlay.stSize.u32Width  = (region_attr.unAttr.stOverlay.stSize.u32Width * horizontal_scaler+ 31) / 16 * 16;
        }
        else
        {
            region_attr.unAttr.stOverlay.stSize.u32Width  = (region_attr.unAttr.stOverlay.stSize.u32Width * horizontal_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        }
        region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_* _AE_OSD_ALIGNMENT_BYTE_;
    }
    else if(av_osd_item_it->osd_name == _AON_BUS_NUMBER_)    
    { 
        region_attr.unAttr.stOverlay.stSize.u32Width = 0;
#if defined(_AV_PRODUCT_CLASS_IPC_)
        char_number = 10;

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
        char tempstring[128] = {0};
        strncpy(tempstring, av_osd_item_it->video_stream.video_encoder_para.bus_number, strlen(av_osd_item_it->video_stream.video_encoder_para.bus_number));
        tempstring[strlen(av_osd_item_it->video_stream.video_encoder_para.bus_number)] = '\0';
        //DEBUG_MESSAGE("lshlsh tempstring %s strlen %d sizeof %d\n",tempstring, strlen(tempstring),sizeof(tempstring));
        //DEBUG_MESSAGE("lshlsh _AON_BUS_NUMBER_ %s sizeof %d strlen %d\n", av_osd_item_it->video_stream.video_encoder_para.bus_number, sizeof(av_osd_item_it->video_stream.video_encoder_para.bus_number), strlen(av_osd_item_it->video_stream.video_encoder_para.bus_number));
        region_attr.unAttr.stOverlay.stSize.u32Width = 0;
        CAvUtf8String bus_number(tempstring);
        av_ucs4_t char_unicode;
        av_char_lattice_info_t lattice_char_info;
        while(bus_number.Next_char(&char_unicode) == true)
        {
            if(m_pAv_font_library->Afl_get_char_lattice(font_name, char_unicode, &lattice_char_info) == 0)
            {
                region_attr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
            }
            else
            {
                //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, font_name);
                //return -1;
                continue;
            }
        }
        //! 0330
        region_attr.unAttr.stOverlay.stSize.u32Width  = (region_attr.unAttr.stOverlay.stSize.u32Width * horizontal_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;

#endif
        if(region_attr.unAttr.stOverlay.stSize.u32Width == 0)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }

    }    
    else if(av_osd_item_it->osd_name == _AON_SELFNUMBER_INFO_)    
    {        
        region_attr.unAttr.stOverlay.stSize.u32Width = 0;
#if defined(_AV_PRODUCT_CLASS_IPC_)
        char_number = 10;

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
        char tempstring[128] = {0};
        //strcat(tempstring,av_osd_item_it->video_stream.video_encoder_para.bus_selfnumber);
        strncpy(tempstring, av_osd_item_it->video_stream.video_encoder_para.bus_selfnumber, strlen(av_osd_item_it->video_stream.video_encoder_para.bus_selfnumber));
        tempstring[strlen(av_osd_item_it->video_stream.video_encoder_para.bus_selfnumber)] = '\0';
        
        region_attr.unAttr.stOverlay.stSize.u32Width = 0;
        CAvUtf8String bus_slefnumber(tempstring);
        av_ucs4_t char_unicode;
        av_char_lattice_info_t lattice_char_info;
        while(bus_slefnumber.Next_char(&char_unicode) == true)
        {
            if(m_pAv_font_library->Afl_get_char_lattice(font_name, char_unicode, &lattice_char_info) == 0)
            {
                region_attr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
            }
            else
            {
                //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, font_name);
                //return -1;
                continue;
            }
        }
 #endif
        if(region_attr.unAttr.stOverlay.stSize.u32Width == 0)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }
        region_attr.unAttr.stOverlay.stSize.u32Width  = (region_attr.unAttr.stOverlay.stSize.u32Width * horizontal_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_* _AE_OSD_ALIGNMENT_BYTE_;
    }    
    else if(av_osd_item_it->osd_name == _AON_GPS_INFO_)
    {        
        region_attr.unAttr.stOverlay.stSize.u32Width = 0;
#if defined(_AV_PRODUCT_CLASS_IPC_)
            char customer[32];

            memset(customer, 0 ,sizeof(customer));
            pgAvDeviceInfo->Adi_get_customer_name(customer, sizeof(customer));
            if(0 == strcmp(customer, "CUSTOMER_CNR"))  
            {
                char_number = 64;
            }
            else
            {
                char_number = 32;
            }
        
#else
        if (!strcmp(product_group, "BUS")) //! overlay altitude for Bus 20160602
        {
            char_number = 32; 
        }
        else if(0 == strcmp(customer_name, "CUSTOMER_04.1129"))
        {
            char_number = 36; 
        }
        else
        {
            char_number = 25;
        }
#endif

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
                region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
        }
    }    
    else if(av_osd_item_it->osd_name == _AON_SPEED_INFO_)
    {        
        region_attr.unAttr.stOverlay.stSize.u32Width = 0;
#if defined(_AV_PRODUCT_CLASS_IPC_)
        char_number = 16;
#else
        char_number = 8;
#endif
        if (!strcmp(customer_name, "CUSTOMER_BJGJ"))
        {
            char_number = 26;
        }


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
        if (!strcmp(customer_name, "CUSTOMER_BJGJ"))
        {
            //! split lines according to the wid
            unsigned int u32Width = 0;
            CAvUtf8String channel_name(pgAvDeviceInfo->Adi_get_beijing_station_info().c_str());
            av_ucs4_t char_unicode;
            av_char_lattice_info_t lattice_char_info;
            while(channel_name.Next_char(&char_unicode) == true)
            {
                if(m_pAv_font_library->Afl_get_char_lattice(font_name, char_unicode, &lattice_char_info) == 0)
                {
                    u32Width += lattice_char_info.width;
                }
                else
                {
                    continue;
                }
            }
            //! three lines and combine speed
            region_attr.unAttr.stOverlay.stSize.u32Height *= 3; //!<3 lines display
            unsigned int speed_len = 8;
            if ((u32Width > (unsigned int)(speed_len * font_width * horizontal_scaler)) &&\
                (u32Width < (region_attr.unAttr.stOverlay.stSize.u32Width - font_width)))
            {
                region_attr.unAttr.stOverlay.stSize.u32Width = u32Width + font_width*horizontal_scaler*1.5;
            }
            else if (u32Width <= (unsigned int)(speed_len * font_width * horizontal_scaler))
            {
                region_attr.unAttr.stOverlay.stSize.u32Width = speed_len * font_width * horizontal_scaler;
            }
            else if (u32Width >= (region_attr.unAttr.stOverlay.stSize.u32Width - font_width))
            {
            }
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
                //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, font_name);
                //return -1;
                continue;
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
#if defined(_AV_PRODUCT_CLASS_IPC_)
    else if(av_osd_item_it->osd_name == _AON_COMMON_OSD1_ || av_osd_item_it->osd_name == _AON_COMMON_OSD2_)
    {
        if (0 == strcmp(customer_name, "CUSTOMER_04.471"))
        {
            if(av_osd_item_it->osd_name == _AON_COMMON_OSD1_)
            {
                region_attr.unAttr.stOverlay.stSize.u32Width = 0;
                char_number = 20; //!< max 
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
                region_attr.unAttr.stOverlay.stSize.u32Height *= 3; //!< 3 lines display
            }
            else
             {
                region_attr.unAttr.stOverlay.stSize.u32Width = 0;
                char_number = 16; //!< max 
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
             }
        }
        else
        {
            region_attr.unAttr.stOverlay.stSize.u32Width = 0;
            char_number = 64;
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
        }
    }
#else
    else if(av_osd_item_it->osd_name == _AON_COMMON_OSD1_)
    {  
        if (!strcmp(customer_name, "CUSTOMER_04.471")) //! compound three lines osd
        {
            region_attr.unAttr.stOverlay.stSize.u32Width = 0;
            char_number = 20; //!< max 
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
            region_attr.unAttr.stOverlay.stSize.u32Height *= 3; //!< 3 lines display
        }
        else
        {
            region_attr.unAttr.stOverlay.stSize.u32Width = 0;
            char_number = 64;
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
        }
    } 
    else if(av_osd_item_it->osd_name == _AON_COMMON_OSD2_ )
    {  
        region_attr.unAttr.stOverlay.stSize.u32Width = 0;
        char_number = 64;
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
    } 
#endif
    else
    {
        DEBUG_WARNING( "CHiAvEncoder::Ae_create_osd_region You must give the implement\n");
        return -1;
    }
#if defined(_AV_PLATFORM_HI3518C_)
    if(PTD_712_VB == pgAvDeviceInfo->Adi_product_type())
    {
        int video_width = 0;
        int video_height = 0;
        pgAvDeviceInfo->Adi_get_video_size(av_osd_item_it->video_stream.video_encoder_para.resolution, av_osd_item_it->video_stream.video_encoder_para.tv_system, &video_width, &video_height);
        pgAvDeviceInfo->Adi_covert_vidoe_size_by_rotate(&video_width, &video_height);    

       if(av_osd_item_it->osd_name == _AON_DATE_TIME_)
        {
            av_osd_item_it->video_stream.video_encoder_para.date_time_osd_x = video_width - region_attr.unAttr.stOverlay.stSize.u32Width;
            av_osd_item_it->video_stream.video_encoder_para.date_time_osd_y = video_height - region_attr.unAttr.stOverlay.stSize.u32Height;
        }
       else if(av_osd_item_it->osd_name == _AON_CHN_NAME_)
        {
            av_osd_item_it->video_stream.video_encoder_para.channel_name_osd_x= video_width - region_attr.unAttr.stOverlay.stSize.u32Width;
            av_osd_item_it->video_stream.video_encoder_para.channel_name_osd_y= video_height - region_attr.unAttr.stOverlay.stSize.u32Height;
        }
    }
#endif

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
#if defined(_AV_PLATFORM_HI3531_) || defined(_AV_PLATFORM_HI3532_)
    const char *pMemory_name = NULL;

    pMemory_name = "ddr1";
    mpp_chn.enModId  = HI_ID_RGN;
    mpp_chn.s32DevId = 0;
    mpp_chn.s32ChnId = 0;
    if((ret_val = HI_MPI_SYS_SetMemConf(&mpp_chn, pMemory_name)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region HI_MPI_SYS_SetMemConf FAILED(0x%08x)\n", ret_val);
    }
#endif
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

#if defined(_AV_PRODUCT_CLASS_IPC_)
    char customer[32];

    memset(customer, 0 ,sizeof(customer));
    pgAvDeviceInfo->Adi_get_customer_name(customer, sizeof(customer));
    if(0 == strcmp(customer, "CUSTOMER_CNR"))  
    {
        Ae_translate_coordinate_for_cnr(*av_osd_item_it, region_attr.unAttr.stOverlay.stSize.u32Width);                
    }
    else
    {
        Ae_translate_coordinate(av_osd_item_it, region_attr.unAttr.stOverlay.stSize.u32Width, region_attr.unAttr.stOverlay.stSize.u32Height);
    }        
#else
    Ae_translate_coordinate(av_osd_item_it, region_attr.unAttr.stOverlay.stSize.u32Width, region_attr.unAttr.stOverlay.stSize.u32Height);
#endif
    region_chn_attr.unChnAttr.stOverlayChn.stPoint.s32X = av_osd_item_it->display_x/_AE_OSD_ALIGNMENT_BYTE_*_AE_OSD_ALIGNMENT_BYTE_;
    region_chn_attr.unChnAttr.stOverlayChn.stPoint.s32Y = av_osd_item_it->display_y/_AE_OSD_ALIGNMENT_BYTE_*_AE_OSD_ALIGNMENT_BYTE_;

    region_chn_attr.unChnAttr.stOverlayChn.u32BgAlpha = 0;
    region_chn_attr.unChnAttr.stOverlayChn.u32FgAlpha = 128; 
    region_chn_attr.unChnAttr.stOverlayChn.u32Layer = 0;

#if defined(_AV_PRODUCT_CLASS_IPC_)
    if((_AON_COMMON_OSD2_ == av_osd_item_it->osd_name) && (0 == strcmp(customer_name, "CUSTOMER_04.471")))
    {
        region_chn_attr.unChnAttr.stOverlayChn.u32FgAlpha = 32;
    }
#else
    if(_AON_WATER_MARK == av_osd_item_it->osd_name)
    {
        region_chn_attr.unChnAttr.stOverlayChn.u32FgAlpha = 32;
    }
#endif

    region_chn_attr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
    region_chn_attr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;

    region_chn_attr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = HI_FALSE;
    region_chn_attr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Width = 16;
    region_chn_attr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Height = 16;
    region_chn_attr.unChnAttr.stOverlayChn.stInvertColor.u32LumThresh = 200;
    region_chn_attr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = MORETHAN_LUM_THRESH;

#if defined(_AV_PLATFORM_HI3520D_V300_)
    mpp_chn.enModId = HI_ID_VENC;
    mpp_chn.s32DevId = 0;
    mpp_chn.s32ChnId = pVideo_encoder_info->venc_chn;
#else
    mpp_chn.enModId = HI_ID_GROUP;
    mpp_chn.s32DevId = pVideo_encoder_info->venc_grp;
    mpp_chn.s32ChnId = 0;
#endif

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
    char gps_date_time_string[64];
    char product_group[32] = {0};
    pgAvDeviceInfo->Adi_get_product_group(product_group, sizeof(product_group));
    char customer_name[32] = {0};
    pgAvDeviceInfo->Adi_get_customer_name(customer_name,sizeof(customer_name));

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
        if (!strcmp(customer_name, "CUSTOMER_04.935"))
        {
            //! gps time
            msgGpsInfo_t gpsinfo;
            memset(&gpsinfo,0,sizeof(gpsinfo));
            pgAvDeviceInfo->Adi_Get_Gps_Info(&gpsinfo);
                
            //gpsinfo.GpsTime
            datetime_t date_time;
            memset(&date_time, 0, sizeof(date_time));
            date_time.year = gpsinfo.GpsTime.year;
            date_time.month = gpsinfo.GpsTime.month;
            date_time.day = gpsinfo.GpsTime.day;
            date_time.hour = gpsinfo.GpsTime.hour;
            date_time.minute = gpsinfo.GpsTime.minute;
            date_time.second = gpsinfo.GpsTime.second;

            switch(av_osd_item_it->video_stream.video_encoder_para.date_mode)
            {
                case _AV_DATE_MODE_yymmdd_:
                default:
                sprintf(gps_date_time_string, "GPS: 20%02d-%02d-%02d", date_time.year, date_time.month, date_time.day);
                break;
                case _AV_DATE_MODE_ddmmyy_:
                sprintf(gps_date_time_string, "GPS: %02d/%02d/20%02d", date_time.day, date_time.month, date_time.year);
                break;
                case _AV_DATE_MODE_mmddyy_:
                sprintf(gps_date_time_string, "GPS: %02d/%02d/20%02d", date_time.month, date_time.day, date_time.year);
                break;
            }

            switch(av_osd_item_it->video_stream.video_encoder_para.time_mode)
            {
                case _AV_TIME_MODE_24_:
                default:
                    sprintf(gps_date_time_string, "%s %02d:%02d:%02d", gps_date_time_string, date_time.hour, date_time.minute, date_time.second);
                    break;
                case _AV_TIME_MODE_12_:
                    if (date_time.hour > 12)
                    {
                        sprintf(gps_date_time_string, "%s %02d:%02d:%02d PM", gps_date_time_string, date_time.hour - 12, date_time.minute, date_time.second);
                    }
                    else if(date_time.hour == 12)
                    {
                        sprintf(gps_date_time_string, "%s %02d:%02d:%02d PM", gps_date_time_string, date_time.hour, date_time.minute, date_time.second);
                    }
                    else if(date_time.hour == 0)
                    {
                        sprintf(gps_date_time_string, "%s 12:%02d:%02d AM", gps_date_time_string,  date_time.minute, date_time.second);
                    }
                    else
                    {
                        sprintf(gps_date_time_string, "%s %02d:%02d:%02d AM", gps_date_time_string, date_time.hour, date_time.minute, date_time.second);                  
                    }                
                    break;
            }
        //! normal time
            memset(&date_time, 0, sizeof(date_time));
            pgAvDeviceInfo->Adi_get_date_time(&date_time);
            
            Ae_get_osd_parameter(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution, &font_name, &font_width, &font_height, &horizontal_scaler, &vertical_scaler);   
            if( _AV_FONT_NAME_24_ == font_name)
            {
                switch(av_osd_item_it->video_stream.video_encoder_para.date_mode)
                {
                    case _AV_DATE_MODE_yymmdd_:
                    default:
                    sprintf(date_time_string, "          20%02d-%02d-%02d",date_time.year, date_time.month, date_time.day);
                    break;
                    case _AV_DATE_MODE_ddmmyy_:
                    sprintf(date_time_string, "          %02d/%02d/20%02d", date_time.day, date_time.month, date_time.year);
                    break;
                    case _AV_DATE_MODE_mmddyy_:
                    sprintf(date_time_string, "          %02d/%02d/20%02d", date_time.month, date_time.day, date_time.year);
                    break;
                }
            }
            else if( _AV_FONT_NAME_12_ == font_name)
            {
                switch(av_osd_item_it->video_stream.video_encoder_para.date_mode)
                {
                    case _AV_DATE_MODE_yymmdd_:
                    default:
                    sprintf(date_time_string, "      20%02d-%02d-%02d",date_time.year, date_time.month, date_time.day);
                    break;
                    case _AV_DATE_MODE_ddmmyy_:
                    sprintf(date_time_string, "      %02d/%02d/20%02d", date_time.day, date_time.month, date_time.year);
                    break;
                    case _AV_DATE_MODE_mmddyy_:
                    sprintf(date_time_string, "      %02d/%02d/20%02d", date_time.month, date_time.day, date_time.year);
                    break;
                }
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
        else
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
        if(strcmp(product_group, "ITS") != 0)
        {
            switch(av_osd_item_it->video_stream.video_encoder_para.time_mode)
            {
                case _AV_TIME_MODE_24_:
                default:
                    sprintf(date_time_string, "%s  %02d:%02d:%02d", date_time_string, date_time.hour, date_time.minute, date_time.second);
                    break;
                case _AV_TIME_MODE_12_:
                    if (date_time.hour > 12)
                    {
                        sprintf(date_time_string, "%s  %02d:%02d:%02d PM", date_time_string, date_time.hour - 12, date_time.minute, date_time.second);
                    }
                    else if(date_time.hour == 12)
                    {
                        sprintf(date_time_string, "%s  %02d:%02d:%02d PM", date_time_string, date_time.hour, date_time.minute, date_time.second);
                    }
                    else if(date_time.hour == 0)
                    {
                        sprintf(date_time_string, "%s  12:%02d:%02d AM", date_time_string,  date_time.minute, date_time.second);
                    }
                    else
                    {
                        sprintf(date_time_string, "%s  %02d:%02d:%02d AM", date_time_string, date_time.hour, date_time.minute, date_time.second);                  
                    }                
                    break;
            }
        }
        else //兆益项目
        {
            switch(av_osd_item_it->video_stream.video_encoder_para.time_mode)
            {
                case _AV_TIME_MODE_24_:
                default:
                    sprintf(date_time_string, "%s  %02d:%02d:%02d", date_time_string, date_time.hour, date_time.minute, date_time.second);
                    break;
                case _AV_TIME_MODE_12_:
                    if (date_time.hour > 12)
                    {
                        sprintf(date_time_string, "%s  %02d:%02d:%02d PM", date_time_string, date_time.hour - 12, date_time.minute, date_time.second);
                    }
                    else if(date_time.hour == 12)
                    {
                        sprintf(date_time_string, "%s  %02d:%02d:%02d PM", date_time_string, date_time.hour, date_time.minute, date_time.second);
                    }
                    else if(date_time.hour == 0)
                    {
                        sprintf(date_time_string, "%s  12:%02d:%02d AM", date_time_string,  date_time.minute, date_time.second);
                    }
                    else
                    {
                        sprintf(date_time_string, "%s  %02d:%02d:%02d AM", date_time_string, date_time.hour, date_time.minute, date_time.second);                  
                    }                
                    break;
            }
        }
        pUtf8_string = date_time_string;        
    }
     
    }
    else if(av_osd_item_it->osd_name == _AON_CHN_NAME_)
    {
        //712_VB treat channel name as vehicle

#if defined(_AV_PLATFORM_HI3518C_)
        char vecicle [32];
        if(PTD_712_VB == pgAvDeviceInfo->Adi_product_type())
        {
            memset(vecicle, 0, sizeof(vecicle));
            memcpy(vecicle, av_osd_item_it->video_stream.video_encoder_para.channel_name, sizeof(vecicle));
            int len = strlen(vecicle);
            if(0 == len)
            {
                pgAvDeviceInfo->Adi_get_mac_address_as_string(vecicle, sizeof(vecicle));
            }
            else
            {
                vecicle[len] = '-';
                ++len;
                pgAvDeviceInfo->Adi_get_mac_address_as_string(vecicle+len, sizeof(vecicle)-len);
            }

            vecicle[sizeof(vecicle)-1] =  '\0';
            pUtf8_string = vecicle;
        }
        else
        {
            pUtf8_string = av_osd_item_it->video_stream.video_encoder_para.channel_name;   
        }   
#else
        pUtf8_string = av_osd_item_it->video_stream.video_encoder_para.channel_name;   
#endif
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
#if defined(_AV_PRODUCT_CLASS_IPC_)
        pUtf8_string = av_osd_item_it->video_stream.video_encoder_para.speed;
#else
        msgSpeedState_t speedinfo;
        Tax2DataList tax2speed;
        memset(&speedinfo,0,sizeof(speedinfo));
        memset(&tax2speed, 0, sizeof(Tax2DataList));
        paramSpeedAlarmSetting_t speedsource;
        memset(&speedsource,0,sizeof(speedsource));
        pgAvDeviceInfo->Adi_Get_SpeedSource_Info(&speedsource);
        //printf("[GPS] ucSpeedFrom =%d\n", speedsource.ucSpeedFrom);
        if(speedsource.ucSpeedFrom == 1)
        {
            
            pgAvDeviceInfo->Adi_Get_Speed_Info(&speedinfo);
            //printf("[GPS] ucSpeedFrom =%d usSpeedUnit =%d uiKiloSpeed =%d uiMiliSpeed =%d\n", speedsource.ucSpeedFrom, speedinfo.usSpeedUnit, speedinfo.uiKiloSpeed,speedinfo.uiMiliSpeed);
            memset(string, 0, sizeof(string));
            if(speedinfo.usSpeedUnit == 0)
            {
                sprintf(string," %dKM/H",speedinfo.uiKiloSpeed);
            }
            else
            {
                sprintf(string," %dMPH",speedinfo.uiMiliSpeed);
            }
        }
        else if(speedsource.ucSpeedFrom   == 2)       //from 卫星 and 脉冲///
        {
            pgAvDeviceInfo->Adi_Get_Speed_Info(&speedinfo);
            //printf("[GPS] ucSpeedFrom =%d usSpeedUnit =%d uiKiloSpeed =%d uiMiliSpeed =%d\n", speedsource.ucSpeedFrom, speedinfo.usSpeedUnit, speedinfo.uiKiloSpeed,speedinfo.uiMiliSpeed);
            memset(string, 0, sizeof(string));
            if(speedinfo.usSpeedUnit == 0)
            {
                sprintf(string," %dKM/H",speedinfo.uiKiloSpeed);
            }
            else
            {
                sprintf(string," %dMPH",speedinfo.uiMiliSpeed);
            }
            if(speedinfo.uiKiloSpeed == 0 || speedinfo.uiMiliSpeed == 0)
            {
                msgGpsInfo_t gpsinfo;
                memset(&gpsinfo,0,sizeof(gpsinfo));
                pgAvDeviceInfo->Adi_Get_Gps_Info(&gpsinfo);
                if(speedsource.ucUnit == 0)
                {
                    sprintf(string," %dKM/H",gpsinfo.usSpeedKMH);
                }
                else
                {
                    sprintf(string," %dMPH",gpsinfo.usSpeedMPH);
                }
            }
    
        }
        //0703
        else if(speedsource.ucSpeedFrom == 3)
        {
            
            pgAvDeviceInfo->Adi_Get_Speed_Info_from_Tax2(&tax2speed);
            //printf("[GPS] Tax2 ucSpeedFrom =%d usSpeedUnit =%d uiKiloSpeed =%d uiMiliSpeed =%d\n", speedsource.ucSpeedFrom, speedsource.ucUnit, tax2speed.usRealSpeed,tax2speed.usRealSpeed* 62 / 100);
            memset(string, 0, sizeof(string));
            if(speedsource.ucUnit == 0)
            {
                sprintf(string," %dKM/H",tax2speed.usRealSpeed);
            }
            else
            {
                sprintf(string," %dMPH", tax2speed.usRealSpeed* 62 / 100);
            }
        }
        else
        {
            msgGpsInfo_t gpsinfo;
            memset(&gpsinfo,0,sizeof(gpsinfo));
            pgAvDeviceInfo->Adi_Get_Gps_Info(&gpsinfo);
            if(speedsource.ucUnit == 0)
            {
                sprintf(string," %dKM/H",gpsinfo.usSpeedKMH);
            }
            else
            {
                sprintf(string," %dMPH",gpsinfo.usSpeedMPH);
            }
        }
    //! novel speed    
#if !defined(_AV_PRODUCT_CLASS_DVR_REI_)

        msgSpeedInfo_t speed_uni;
        memset(&speed_uni, 0, sizeof(speed_uni));
        pgAvDeviceInfo->Adi_Get_Speed_Info(&speed_uni);
        if (speed_uni.uiCurSpeed != 0)
        {
            if (speed_uni.uiSpeedUnit == 0)
            {
                sprintf(string," %dKM/H",speed_uni.uiCurSpeed);
            }
            else
            {
                sprintf(string," %dMPH",speed_uni.uiCurSpeed);
            }
        }
#endif
  //! endof novel      
        char customer[32] = {0};
        memset(customer, 0 ,sizeof(customer));
        pgAvDeviceInfo->Adi_get_customer_name(customer, sizeof(customer));
        if(0 == strcmp(customer, "CUSTOMER_BJGJ"))
        {
			sprintf(speedstring, "%s%s", GuiTKGetText(NULL, NULL, "STR_GPS_SPEED"), string);
			speedstring[sizeof(speedstring) - 1] = '\0';
        }
        else
        {
			strncpy(speedstring, string, strlen(string));
			speedstring[strlen(string)] = '\0';
        }
        pUtf8_string = speedstring;
#endif
    }
    else if(av_osd_item_it->osd_name == _AON_GPS_INFO_)
    {
#if defined(_AV_PRODUCT_CLASS_IPC_)
        memset(date_time_string, 0, sizeof(date_time_string));
        snprintf(date_time_string, sizeof(date_time_string), "%s", av_osd_item_it->video_stream.video_encoder_para.gps);

        char customer[32];
        memset(customer, 0 ,sizeof(customer));
        pgAvDeviceInfo->Adi_get_customer_name(customer, sizeof(customer));
        if(0 == strcmp(customer, "CUSTOMER_CNR"))  
        {
            datetime_t date_time;
            
            pgAvDeviceInfo->Adi_get_date_time(&date_time);
       
            snprintf(date_time_string, sizeof(date_time_string), "%s  20%02d-%02d-%02d %02d:%02d:%02d", date_time_string, \
                date_time.year, date_time.month, date_time.day, date_time.hour, date_time.minute, date_time.second);
        }
        pUtf8_string = date_time_string;
#else
        msgGpsInfo_t gpsinfo;
        memset(&gpsinfo,0,sizeof(gpsinfo));
        pgAvDeviceInfo->Adi_Get_Gps_Info(&gpsinfo);

        if (!strcmp(customer_name, "CUSTOMER_04.935"))
        {
            //if(gpsinfo.ucGpsStatus == 0)
            {
                char temp[48] = {0};
                sprintf(temp,"%d.%d.%04u%c %d.%d.%04u%c",gpsinfo.ucLatitudeDegree,gpsinfo.ucLatitudeCent,gpsinfo.ulLatitudeSec,gpsinfo.ucDirectionLatitude ? 'S':'N',gpsinfo.ucLongitudeDegree,gpsinfo.ucLongitudeCent,gpsinfo.ulLongitudeSec,gpsinfo.ucDirectionLongitude ? 'W':'E');
                //strcat(gpsstring,temp);
                strncpy(gpsstring, temp, strlen(temp));
                gpsstring[strlen(temp)] = '\0';
            }
        }
        /*else if(!strcmp(customer_name, "CUSTOMER_BJGJ"))
        {
        	std::string strStation = pgAvDeviceInfo->Adi_get_beijing_station_info();
        	snprintf(gpsstring, sizeof(gpsstring) - 1, "%s", strStation.c_str());

        }*/
        else if(!strcmp(customer_name, "CUSTOMER_04.1129"))
        {
            msgEthernetDevApcNumToServer_t apc_num = pgAvDeviceInfo->Adi_get_apc_num();
            if (gpsinfo.ucGpsStatus == 0)
            {
                char temp[48] = {0};
                
                sprintf(temp,"%d.%d.%04u%c %d.%d.%04u%c Onboard:%d",gpsinfo.ucLatitudeDegree,gpsinfo.ucLatitudeCent,gpsinfo.ulLatitudeSec,\
                    gpsinfo.ucDirectionLatitude ? 'S':'N',gpsinfo.ucLongitudeDegree,gpsinfo.ucLongitudeCent,gpsinfo.ulLongitudeSec,\
                    gpsinfo.ucDirectionLongitude ? 'W':'E', apc_num.onboardingnum);

                strncpy(gpsstring, temp, strlen(temp));
                gpsstring[strlen(temp)] = '\0';
            }
            else if(gpsinfo.ucGpsStatus == 1)
            {
                unsigned int gps_len = strlen(GuiTKGetText(NULL, NULL, "STR_GPS INVALID"));
                strncpy(gpsstring, GuiTKGetText(NULL, NULL, "STR_GPS INVALID"),gps_len);
             
                sprintf(gpsstring + gps_len, " Onboard:%d",apc_num.onboardingnum);
                gpsstring[strlen(gpsstring)] = '\0';
                
            }
            else
            {
                unsigned int gps_len = strlen(GuiTKGetText(NULL, NULL, "STR_GPS NOT_EXIST"));
                strncpy(gpsstring, GuiTKGetText(NULL, NULL, "STR_GPS NOT_EXIST"), gps_len);
                
                sprintf(gpsstring + gps_len, " Onboard:%d",apc_num.onboardingnum);
                gpsstring[strlen(gpsstring)] = '\0';

            }
            pUtf8_string = gpsstring;            
        }
        else
        {
        if(gpsinfo.ucGpsStatus == 0)
        {
                if (!strcmp(product_group, "BUS")) //! overlay altitude for Bus 20160602
                {
                    char temp[48] = {0};
                    sprintf(temp,"%d.%d.%04u%c %d.%d.%04u%c %dm",gpsinfo.ucLatitudeDegree,gpsinfo.ucLatitudeCent,gpsinfo.ulLatitudeSec,\
                        gpsinfo.ucDirectionLatitude ? 'S':'N',gpsinfo.ucLongitudeDegree,\
                        gpsinfo.ucLongitudeCent,gpsinfo.ulLongitudeSec,gpsinfo.ucDirectionLongitude ? 'W':'E',\
                        gpsinfo.sAltitude);

                    strncpy(gpsstring, temp, strlen(temp));
                    gpsstring[strlen(temp)] = '\0';
                }
                else
                {
                    char temp[48] = {0};
                    sprintf(temp,"%d.%d.%04u%c %d.%d.%04u%c",gpsinfo.ucLatitudeDegree,gpsinfo.ucLatitudeCent,gpsinfo.ulLatitudeSec,gpsinfo.ucDirectionLatitude ? 'S':'N',gpsinfo.ucLongitudeDegree,gpsinfo.ucLongitudeCent,gpsinfo.ulLongitudeSec,gpsinfo.ucDirectionLongitude ? 'W':'E');
                    //strcat(gpsstring,temp);
                    strncpy(gpsstring, temp, strlen(temp));
                    gpsstring[strlen(temp)] = '\0';
                }
            }
        else if(gpsinfo.ucGpsStatus == 1)
        {
            //strcat(gpsstring,GuiTKGetText(NULL, NULL, "STR_GPS INVALID"));
            strncpy(gpsstring, GuiTKGetText(NULL, NULL, "STR_GPS INVALID"), strlen(GuiTKGetText(NULL, NULL, "STR_GPS INVALID")));
            gpsstring[strlen(GuiTKGetText(NULL, NULL, "STR_GPS INVALID"))] = '\0';
            
        }
        else
        {
            //strcat(gpsstring,GuiTKGetText(NULL, NULL, "STR_GPS NOT_EXIST"));
            strncpy(gpsstring, GuiTKGetText(NULL, NULL, "STR_GPS NOT_EXIST"), strlen(GuiTKGetText(NULL, NULL, "STR_GPS NOT_EXIST")));
            gpsstring[strlen(GuiTKGetText(NULL, NULL, "STR_GPS NOT_EXIST"))] = '\0';
            }
        }
        pUtf8_string = gpsstring;
#endif
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

#if defined(_AV_PRODUCT_CLASS_IPC_)
    char customer[32];

    memset(customer, 0 ,sizeof(customer));
    pgAvDeviceInfo->Adi_get_customer_name(customer, sizeof(customer));
    if(0 == strcmp(customer, "CUSTOMER_CNR"))  
    {
        Ae_get_osd_parameter(av_osd_item_it->osd_name, _AR_QVGA_, &font_name, &font_width, &font_height, &horizontal_scaler, &vertical_scaler);            
    }
    else
    {
        Ae_get_osd_parameter(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution, &font_name, &font_width, &font_height, &horizontal_scaler, &vertical_scaler);   
    }
#else
    Ae_get_osd_parameter(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution, &font_name, &font_width, &font_height, &horizontal_scaler, &vertical_scaler);      
#endif

    if (!strcmp(customer_name, "CUSTOMER_04.935"))
    {
        if(av_osd_item_it->osd_name == _AON_DATE_TIME_)
        {   
            if(Ae_utf8string_2_bmp(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution,
                av_osd_item_it->pBitmap, av_osd_item_it->bitmap_width, av_osd_item_it->bitmap_height/2, gps_date_time_string, 0xffff, 0x00, 0x8000) != 0)
            {
                DEBUG_ERROR("erooroeoooofooe osd_name =%d\n", av_osd_item_it->osd_name);
                return -1;
            }
            unsigned char * pBmp2 = NULL;
            if ((2 == horizontal_scaler)&&(1 == vertical_scaler))
            {
                pBmp2 = av_osd_item_it->pBitmap + av_osd_item_it->bitmap_width * horizontal_scaler * av_osd_item_it->bitmap_height * vertical_scaler/2;
            }
            else
            {
                pBmp2 = av_osd_item_it->pBitmap + av_osd_item_it->bitmap_width * horizontal_scaler * av_osd_item_it->bitmap_height * vertical_scaler;
            }                
            if(Ae_utf8string_2_bmp(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution,
                pBmp2, av_osd_item_it->bitmap_width, av_osd_item_it->bitmap_height/2, pUtf8_string, 0xffff, 0x00, 0x8000) != 0)
            {
                DEBUG_ERROR("erooroeoooofooe osd_name =%d\n", av_osd_item_it->osd_name);
                return -1;
            }
  
        }
        else
        {
            if(Ae_utf8string_2_bmp(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution,
                av_osd_item_it->pBitmap, av_osd_item_it->bitmap_width, av_osd_item_it->bitmap_height, pUtf8_string, 0xffff, 0x00, 0x8000) != 0)
            {
                DEBUG_ERROR("erooroeoooofooe osd_name =%d\n", av_osd_item_it->osd_name);
                return -1;
            }
        }

    }
    else if (!strcmp(customer_name, "CUSTOMER_04.471"))
    {
        if ((av_osd_item_it->osd_name == _AON_COMMON_OSD1_) &&(pUtf8_string != NULL))
        {   
            //DEBUG_CRITICAL("common osd1:%s\n", pUtf8_string);
            char * pCommon_string_line = NULL;
            pCommon_string_line = pUtf8_string; 
            pCommon_string_line[19] = 0;
            if(Ae_utf8string_2_bmp(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution,
                av_osd_item_it->pBitmap, av_osd_item_it->bitmap_width, av_osd_item_it->bitmap_height/3, pCommon_string_line, 0xffff, 0x00, 0x8000) != 0)
            {
                DEBUG_ERROR("erooroeoooofooe osd_name =%d\n", av_osd_item_it->osd_name);
                return -1;
            }      
            unsigned char * pBmp2 = NULL;
            //! second line
            pCommon_string_line += 20;
            pCommon_string_line[19] = 0;
            //DEBUG_CRITICAL("common osd2:%s\n", pCommon_string_line);
            if ((2 == horizontal_scaler)&&(1 == vertical_scaler))
            {
                pBmp2 = av_osd_item_it->pBitmap + av_osd_item_it->bitmap_width * horizontal_scaler * av_osd_item_it->bitmap_height * vertical_scaler/3;
            }
            else
            {
                pBmp2 = av_osd_item_it->pBitmap + av_osd_item_it->bitmap_width * horizontal_scaler * av_osd_item_it->bitmap_height * vertical_scaler*2/3;
            }                
            if(Ae_utf8string_2_bmp(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution,
                pBmp2, av_osd_item_it->bitmap_width, av_osd_item_it->bitmap_height/3, pCommon_string_line, 0xffff, 0x00, 0x8000) != 0)
            {
                DEBUG_ERROR("erooroeoooofooe osd_name =%d\n", av_osd_item_it->osd_name);
                return -1;
            }
            
            //! third line
            pCommon_string_line += 20;
            pCommon_string_line[19] = 0;
            //DEBUG_CRITICAL("common osd3:%s\n", pCommon_string_line);
            if ((2 == horizontal_scaler)&&(1 == vertical_scaler))
            {
                pBmp2 += av_osd_item_it->bitmap_width * horizontal_scaler * av_osd_item_it->bitmap_height * vertical_scaler/3;
            }
            else
            {
                pBmp2 += av_osd_item_it->bitmap_width * horizontal_scaler * av_osd_item_it->bitmap_height * vertical_scaler*2/3;
            }                
            if(Ae_utf8string_2_bmp(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution,
                pBmp2, av_osd_item_it->bitmap_width, av_osd_item_it->bitmap_height/3, pCommon_string_line, 0xffff, 0x00, 0x8000) != 0)
            {
                DEBUG_ERROR("erooroeoooofooe osd_name =%d\n", av_osd_item_it->osd_name);
                return -1;
            }
        }
        else
        {
            if(Ae_utf8string_2_bmp(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution,
                av_osd_item_it->pBitmap, av_osd_item_it->bitmap_width, av_osd_item_it->bitmap_height, pUtf8_string, 0xffff, 0x00, 0x8000) != 0)
            {
                DEBUG_ERROR("erooroeoooofooe osd_name =%d\n", av_osd_item_it->osd_name);
                return -1;
            }
        }
        
    }
    else if (!strcmp(customer_name, "CUSTOMER_BJGJ"))
    {
        if ((av_osd_item_it->osd_name == _AON_SPEED_INFO_) &&(pUtf8_string != NULL))
        {   
            //! obtain location
        	std::string strStation = pgAvDeviceInfo->Adi_get_beijing_station_info();
                        
            //! analyze utf8 string
            int u32Width = 0;
            CAvUtf8String channel_name(strStation.c_str());
            av_ucs4_t char_unicode;
            av_char_lattice_info_t lattice_char_info;
            while(channel_name.Next_char(&char_unicode) == true)
            {
                if(m_pAv_font_library->Afl_get_char_lattice(font_name, char_unicode, &lattice_char_info) == 0)
                {
                    u32Width += lattice_char_info.width;
                    
                }
                else
                {
                    continue;
                }
            }
            int line_bytes = channel_name.Chars_bytes_current();
            //!end 
#if 1  //! three lines 
            //! if gps single line, speed set in second line
            char pString_line[132] = {0};
            int hei = av_osd_item_it->bitmap_height / 3;

            if (line_bytes > 128)
            {
                line_bytes = 128;
            }

        	snprintf(pString_line, sizeof(pString_line) - 1, "%s", strStation.c_str());
            
            
            int osd_wid_valid = av_osd_item_it->bitmap_width -  font_width;

            if ((u32Width <= osd_wid_valid))
            {
                //DEBUG_CRITICAL(" two lines!\n");
                //! first line gps

                pString_line[line_bytes] = 0;
                if(Ae_utf8string_2_bmp(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution,
                    av_osd_item_it->pBitmap, av_osd_item_it->bitmap_width, hei, pString_line, 0xffff, 0x00, 0x8000) != 0)
                {
                    DEBUG_ERROR("erooroeoooofooe osd_name =%d\n", av_osd_item_it->osd_name);
                    return -1;
                }

                 //! second line speed
                unsigned char * pBmp2 = NULL;

                pBmp2 = av_osd_item_it->pBitmap + av_osd_item_it->bitmap_width * horizontal_scaler * av_osd_item_it->bitmap_height * vertical_scaler*2/3;
              
                if(Ae_utf8string_2_bmp(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution,
                    pBmp2, av_osd_item_it->bitmap_width, hei, speedstring, 0xffff, 0x00, 0x8000) != 0)
                {
                    DEBUG_ERROR("erooroeoooofooe osd_name =%d\n", av_osd_item_it->osd_name);
                    return -1;
                }  

                //! third line empty
                //printf("third line!\n");
                unsigned char * pBmp3 = NULL;

                pBmp3 = av_osd_item_it->pBitmap + av_osd_item_it->bitmap_width * horizontal_scaler * av_osd_item_it->bitmap_height * vertical_scaler*4/3;
                memset(pBmp3, 0, av_osd_item_it->bitmap_width*hei*horizontal_scaler*vertical_scaler*2); 
             
            }

            //! gps double line, speed set in third line
            if (u32Width > osd_wid_valid)
            {
                //DEBUG_CRITICAL("three lines!\n");
                int wid = 0;
                channel_name.Reset(NULL);
                
                while (( wid < osd_wid_valid &&(channel_name.Next_char(&char_unicode) == true)))
                {
                    if(m_pAv_font_library->Afl_get_char_lattice(font_name, char_unicode, &lattice_char_info) == 0)
                    {
                        wid += lattice_char_info.width;
                        
                    }
                    else
                    {
                        continue;
                    }
                }
                line_bytes = channel_name.Chars_bytes_current();
   
                pString_line[line_bytes] = 0;
                //! first line
                //printf("first line!\n");
                if(Ae_utf8string_2_bmp(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution,
                    av_osd_item_it->pBitmap, av_osd_item_it->bitmap_width, hei, pString_line, 0xffff, 0x00, 0x8000) != 0)
                {
                    DEBUG_ERROR("erooroeoooofooe osd_name =%d\n", av_osd_item_it->osd_name);
                    return -1;
                }
                //! second line
                //printf("second line!\n");
                int line_bytes2 = 0;
                wid = 0;
                CAvUtf8String name(strStation.c_str() + line_bytes);
                while ((wid < osd_wid_valid) &&(name.Next_char(&char_unicode) == true))
                {
                    if(m_pAv_font_library->Afl_get_char_lattice(font_name, char_unicode, &lattice_char_info) == 0)
                    {
                        wid += lattice_char_info.width;
                        
                    }
                    else
                    {
                        continue;
                    }
                }
                line_bytes2 = name.Chars_bytes_current();

                if(line_bytes2 > 0)
                {
                    pString_line[line_bytes+ line_bytes2 +1] = 0;               
                    
                    memcpy(pString_line + line_bytes + 1, strStation.c_str() + line_bytes, line_bytes2);

                    
                    unsigned char * pBmp2 = NULL;

                    pBmp2 = av_osd_item_it->pBitmap + av_osd_item_it->bitmap_width * horizontal_scaler * av_osd_item_it->bitmap_height * vertical_scaler*2/3;
                  
                    if(Ae_utf8string_2_bmp(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution,
                        pBmp2, av_osd_item_it->bitmap_width, hei, pString_line + line_bytes +1, 0xffff, 0x00, 0x8000) != 0)
                    {
                        DEBUG_ERROR("erooroeoooofooe osd_name =%d\n", av_osd_item_it->osd_name);
                        return -1;
                    }

                    //! third line speed
                    //printf("third line!\n");
                    unsigned char * pBmp3 = NULL;

                    pBmp3 = av_osd_item_it->pBitmap + av_osd_item_it->bitmap_width * horizontal_scaler * av_osd_item_it->bitmap_height * vertical_scaler*4/3;
                  
                    if(Ae_utf8string_2_bmp(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution,
                        pBmp3, av_osd_item_it->bitmap_width, hei, speedstring, 0xffff, 0x00, 0x8000) != 0)
                    {
                        DEBUG_ERROR("erooroeoooofooe osd_name =%d\n", av_osd_item_it->osd_name);
                        return -1;
                    }    
                }
                else
                {
                  //! second line speed
                    unsigned char * pBmp2 = NULL;

                    pBmp2 = av_osd_item_it->pBitmap + av_osd_item_it->bitmap_width * horizontal_scaler * av_osd_item_it->bitmap_height * vertical_scaler*2/3;
                  
                    if(Ae_utf8string_2_bmp(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution,
                        pBmp2, av_osd_item_it->bitmap_width, hei, speedstring, 0xffff, 0x00, 0x8000) != 0)
                    {
                        DEBUG_ERROR("erooroeoooofooe osd_name =%d\n", av_osd_item_it->osd_name);
                        return -1;
                    }  

                    //! third line empty
                    //printf("third line!\n");
                    unsigned char * pBmp3 = NULL;

                    pBmp3 = av_osd_item_it->pBitmap + av_osd_item_it->bitmap_width * horizontal_scaler * av_osd_item_it->bitmap_height * vertical_scaler*4/3;
                    memset(pBmp3, 0, av_osd_item_it->bitmap_width*hei*horizontal_scaler*vertical_scaler*2);                    
                }
                
            }
#endif
        }
        else
        {
            if(Ae_utf8string_2_bmp(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution,
                av_osd_item_it->pBitmap, av_osd_item_it->bitmap_width, av_osd_item_it->bitmap_height, pUtf8_string, 0xffff, 0x00, 0x8000) != 0)
            {
                DEBUG_ERROR("erooroeoooofooe osd_name =%d\n", av_osd_item_it->osd_name);
                return -1;
            }
        }
        
    }
    else
    {
        int font_color = 0xffff;
#if defined(_AV_PRODUCT_CLASS_IPC_)
    //! for T2 ipc only to display red color
        char rec_osd[16] = {0};
        char customer_name[32] = {0};
        pgAvDeviceInfo->Adi_get_customer_name(customer_name, sizeof(customer_name));
        if ((_AON_BUS_NUMBER_ == av_osd_item_it->osd_name)&&(strcmp(customer_name, "CUSTOMER_RL") == 0)) //!< for T2)
        {
            font_color = 0xfc00;
            
            rec_osd[0] = 0xE2;
            rec_osd[1] = 0x97;
            rec_osd[2] = 0x8F;
            sprintf(rec_osd +3, "REC");

            pUtf8_string = rec_osd;
        }
#endif

        if(Ae_utf8string_2_bmp(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution,
            av_osd_item_it->pBitmap, av_osd_item_it->bitmap_width, av_osd_item_it->bitmap_height, pUtf8_string, font_color, 0x00, 0x8000) != 0)
        {
            DEBUG_ERROR("erooroeoooofooe osd_name =%d\n", av_osd_item_it->osd_name);
            return -1;
        }
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
    if((av_osd_item_it->osd_name != _AON_DATE_TIME_) && (0 != strcmp(customer, "CUSTOMER_CNR")))
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
    hi_video_encoder_info_t *pVideo_encoder_info = (hi_video_encoder_info_t *)av_osd_item_it->video_stream.video_encoder_id.pVideo_encoder_info;
    hi_osd_region_t *pOsd_region = (hi_osd_region_t *)av_osd_item_it->pOsd_region;
    MPP_CHN_S mpp_chn;
    HI_S32 ret_val = -1;

#ifdef _AV_PLATFORM_HI3520D_V300_
    mpp_chn.enModId = HI_ID_VENC;
    mpp_chn.s32DevId = 0;
    mpp_chn.s32ChnId = pVideo_encoder_info->venc_chn;
    if((ret_val = HI_MPI_RGN_DetachFromChn(pOsd_region->region_handle, &mpp_chn)) != HI_SUCCESS)
#else
    mpp_chn.enModId = HI_ID_GROUP;
    mpp_chn.s32DevId = pVideo_encoder_info->venc_grp;
    mpp_chn.s32ChnId = 0;
    if((ret_val = HI_MPI_RGN_DetachFrmChn(pOsd_region->region_handle, &mpp_chn)) != HI_SUCCESS)
#endif
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destroy_osd_region HI_MPI_RGN_DetachFrmChn FAILED(0x%08x)\n", ret_val);
        return -1;
    }
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

#if defined(_AV_PLATFORM_HI3520D_V300_)	/*6AII_AV12机型专用osd叠加*/
int CHiAvEncoder::Ae_create_extend_osd_region(std::list<av_osd_item_t>::iterator &av_osd_item_it)
{
    RGN_ATTR_S region_attr;
    memset(&region_attr, 0x0, sizeof(RGN_ATTR_S));
    RGN_CHN_ATTR_S region_chn_attr;
    MPP_CHN_S mpp_chn;
    RGN_HANDLE region_handle;
    hi_osd_region_t *pOsd_region = NULL;
	hi_video_encoder_info_t* pVideo_encoder_info = (hi_video_encoder_info_t*)av_osd_item_it->video_stream.video_encoder_id.pVideo_encoder_info;
	int font_name = 0, font_width = 0, font_height = 0;
	int horizontal_scaler = 0, vertical_scaler = 0;
	HI_S32 ret_val = 0;
	
    /*calculate the size of region*/
    Ae_get_osd_parameter(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution, &font_name, 
                                    &font_width, &font_height, &horizontal_scaler, &vertical_scaler);	

	int char_number = 0;
	OsdItem_t* pOsdItem = &(av_osd_item_it->video_stream.video_encoder_para.stuExtendOsdInfo[av_osd_item_it->extend_osd_id]);
	
	if(pOsdItem->ucOsdType == 0)	//日期，特殊处理
	{
		char_number = strlen("16.03.23   11:10:00");
		switch(av_osd_item_it->video_stream.video_encoder_para.resolution)
		{
			case _AR_1080_:
			case _AR_720_:
			case _AR_960P_:
				region_attr.unAttr.stOverlay.stSize.u32Width = (font_width * char_number * horizontal_scaler / 2 + font_width * horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
				region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_ ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
				break;
			default:
				region_attr.unAttr.stOverlay.stSize.u32Width  = ((font_width * char_number * horizontal_scaler) * 2 / 3 + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
				region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
				break;
		}
	}
	else
	{
		CAvUtf8String channel_name(pOsdItem->szContent);
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
				//DEBUG_ERROR( "EXTEND_OSD: Afl_get_char_lattice failed, unicode = %ld, font_name = %d\n", char_unicode, font_name);
				continue;//return -1;
			}
		}
		if(region_attr.unAttr.stOverlay.stSize.u32Width == 0)
		{
			DEBUG_ERROR( "EXTEND_OSD: Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
			return -1;
		}
		region_attr.unAttr.stOverlay.stSize.u32Width  = (region_attr.unAttr.stOverlay.stSize.u32Width * horizontal_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
		region_attr.unAttr.stOverlay.stSize.u32Height = (font_height * vertical_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
	}
	
	int video_width = 0;
	int video_height = 0;
	pgAvDeviceInfo->Adi_get_video_size(av_osd_item_it->video_stream.video_encoder_para.resolution, av_osd_item_it->video_stream.video_encoder_para.tv_system, &video_width, &video_height);

	if(region_attr.unAttr.stOverlay.stSize.u32Width > (unsigned int)video_width)
	{
		region_attr.unAttr.stOverlay.stSize.u32Width = video_width;
	}

	if(region_attr.unAttr.stOverlay.stSize.u32Height >	(unsigned int)video_height)
	{
		region_attr.unAttr.stOverlay.stSize.u32Height = video_height;
	}
	region_attr.unAttr.stOverlay.stSize.u32Width = region_attr.unAttr.stOverlay.stSize.u32Width / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
	region_attr.unAttr.stOverlay.stSize.u32Height  = region_attr.unAttr.stOverlay.stSize.u32Height / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
	
	//create region
	region_attr.enType = OVERLAY_RGN;
	region_attr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
	region_attr.unAttr.stOverlay.u32BgColor = 0x00;
	region_handle = Har_get_region_handle_encoder(av_osd_item_it->video_stream.type, av_osd_item_it->video_stream.phy_video_chn_num, av_osd_item_it->osd_name, &region_handle, av_osd_item_it->extend_osd_id);

	HI_MPI_RGN_Destroy(region_handle);
	if((ret_val = HI_MPI_RGN_Create(region_handle, &region_attr)) != HI_SUCCESS)
	{
		DEBUG_ERROR( "EXTEND_OSD: HI_MPI_RGN_Create FAILED(0x%08x), id = %d, type = %d, color = %d\n", ret_val, av_osd_item_it->extend_osd_id, pOsdItem->ucOsdType, pOsdItem->ucColor);
		return -1;
	}
	//attach to channel
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
	region_chn_attr.unChnAttr.stOverlayChn.stInvertColor.u32LumThresh = 200;
	region_chn_attr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = MORETHAN_LUM_THRESH;

	mpp_chn.enModId = HI_ID_VENC;
	mpp_chn.s32DevId = 0;
	mpp_chn.s32ChnId = pVideo_encoder_info->venc_chn;

	if((ret_val = HI_MPI_RGN_AttachToChn(region_handle, &mpp_chn, &region_chn_attr)) != HI_SUCCESS)
	{
		DEBUG_ERROR( "EXTEND_OSD: Ae_create_osd_region HI_MPI_RGN_AttachToChn FAILED(0x%08x)\n", ret_val);
		HI_MPI_RGN_Destroy(region_handle);
		return -1;
	}

	//bitmap
	av_osd_item_it->bitmap_width = region_attr.unAttr.stOverlay.stSize.u32Width;
	av_osd_item_it->bitmap_height = region_attr.unAttr.stOverlay.stSize.u32Height;
	av_osd_item_it->pBitmap = new unsigned char[av_osd_item_it->bitmap_width * horizontal_scaler * av_osd_item_it->bitmap_height * vertical_scaler * 2];
	_AV_FATAL_(av_osd_item_it->pBitmap == NULL, "OUT OF MEMORY\n");

	//region
	_AV_FATAL_((pOsd_region = new hi_osd_region_t) == NULL, "OUT OF MEMORY\n");
	pOsd_region->region_handle = region_handle;
	av_osd_item_it->pOsd_region = (void *)pOsd_region;

	return 0;
}

int CHiAvEncoder::Ae_extend_osd_overlay(std::list<av_osd_item_t>::iterator &av_osd_item_it)
{
	hi_osd_region_t *pOsd_region = (hi_osd_region_t *)av_osd_item_it->pOsd_region;
	HI_S32 ret_val = -1;
	char date_time_string[64] = {0};
	char *pUtf8_string = NULL;
	HI_U32 font_color = 0xffff;

	OsdItem_t* pOsdItem = &(av_osd_item_it->video_stream.video_encoder_para.stuExtendOsdInfo[av_osd_item_it->extend_osd_id]);
  
	if(pOsdItem->ucOsdType == 0)	//日期特殊处理，av自己取当前时间叠加
	{		 
		datetime_t date_time;
		pgAvDeviceInfo->Adi_get_date_time(&date_time);
		sprintf(date_time_string, "%02d.%02d.%02d   %02d:%02d:%02d", date_time.year, date_time.month, date_time.day, date_time.hour, date_time.minute, date_time.second);
				
		pUtf8_string = date_time_string;
	}
	else
	{
		pUtf8_string = pOsdItem->szContent;
	}
	
	//Ae_get_osd_parameter(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution, &font_name, &font_width, &font_height, &horizontal_scaler, &vertical_scaler);	  

	//颜色处理///
	if(pOsdItem->ucColor == 0)
	{
		font_color = 0xffff;
	}
	else if(pOsdItem->ucColor == 1)
	{
		font_color = 0x8000;
	}
	else if(pOsdItem->ucColor == 2)
	{
		font_color = 0xfc00;
	}

	if(Ae_utf8string_2_bmp(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution,
			av_osd_item_it->pBitmap, av_osd_item_it->bitmap_width, av_osd_item_it->bitmap_height, pUtf8_string, font_color, 0x00, 0x8000) != 0)
	{
		DEBUG_ERROR("EXTEND_OSD: Ae_extend_osd_overlay failed, osd_name =%d\n", av_osd_item_it->osd_name);
		return -1;
	}
	
	BITMAP_S bitmap;
	bitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
	bitmap.u32Width = av_osd_item_it->bitmap_width;
	bitmap.u32Height = av_osd_item_it->bitmap_height;	 
	bitmap.pData = av_osd_item_it->pBitmap;
	if((ret_val = HI_MPI_RGN_SetBitMap(pOsd_region->region_handle, &bitmap)) != HI_SUCCESS)
	{		 
		DEBUG_ERROR( "EXTEND_OSD: Ae_extend_osd_overlay HI_MPI_RGN_SetBitMap FAILED(region_handle:0x%08x)(%d)\n", ret_val, pOsd_region->region_handle);
		return -1;
	}

	return 0;
}
#endif

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
    //printf("[debug]phy_audio_chn:%d aenc_chen:%d \n", pAv_encoder->m_pAudio_thread_info->phy_audio_chn_id, pAv_encoder->m_pAudio_thread_info->aenc_chn);
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
#endif

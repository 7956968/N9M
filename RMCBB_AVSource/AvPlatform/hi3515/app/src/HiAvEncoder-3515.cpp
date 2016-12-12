#include "HiAvEncoder-3515.h"
#if !defined(_AV_PLATFORM_HI3515_)
#include "audio_amr_adp.h" 
#endif
#include "mpi_vpp.h"
#ifndef _AV_PRODUCT_CLASS_IPC_
#ifdef _AV_USE_AACENC_
#ifndef _AV_PLATFORM_HI3515_
#include "audio_aac_adp.h"
#endif
#endif
#endif
#include "CommonMacro.h"
#include "loadbmp.h"

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
        memset(&m_stuOsdTemp[i], 0x0, sizeof(m_stuOsdTemp[i]));
    }
    
#if defined(_AV_PLATFORM_HI3515_)     
	memset(m_SameOsdCache, 0x0, sizeof(m_SameOsdCache));
	m_OsdNameToIdMap.clear();
	if(PTD_6A_I == pgAvDeviceInfo->Adi_product_type())
	{
		m_OsdNameToIdMap[OSD_DATEANDTIME] = 0;
		m_OsdNameToIdMap[OSD_USBSTATUS] = 1;
	}
	else if(PTD_A5_II == pgAvDeviceInfo->Adi_product_type())
	{
		m_OsdNameToIdMap[_AON_DATE_TIME_] = 0;
		m_OsdNameToIdMap[_AON_GPS_INFO_] = 1;
		m_OsdNameToIdMap[_AON_SPEED_INFO_] = 2;
	}
	
	memset(m_stuOsdProperty, 0x0, sizeof(m_stuOsdProperty));
/*	memset(m_stuOsdRecBitmap, 0x0, sizeof(OSD_BITMAPINFO));

	OSD_BITMAPFILEHEADER pBmpFileHeader;
	GetBmpInfo("time1.bmp", &pBmpFileHeader, &m_stuOsdRecBitmap);
    if(av_osd_item_it->extend_osd_id == 2)	//REC使用图片
    {
		OSD_SURFACE_S Surface;
		Surface.enColorFmt = OSD_COLOR_FMT_RGB1555;
		pParam->stBitmap.pData = malloc(2*(bmpInfo.bmiHeader.biWidth)*(bmpInfo.bmiHeader.biHeight));
		if(NULL == pParam->stBitmap.pData)
		{
			printf("malloc osd memroy err!\n");
			return HI_FAILURE;
		}
		
		CreateSurfaceByBitMap(filename,&Surface,(HI_U8*)(pParam->stBitmap.pData));
		
		pParam->stBitmap.u32Width = Surface.u16Width;
		pParam->stBitmap.u32Height = Surface.u16Height;
		pParam->stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
    	
    }*/
#endif    
}

CHiAvEncoder::~CHiAvEncoder()
{
#if defined(_AV_PRODUCT_CLASS_IPC_)
#if defined(_AV_HAVE_AUDIO_INPUT_)

    if(NULL != m_pAudio_thread_info)
    {
        m_pAudio_thread_info->thread_state = false;
        pthread_join(m_pAudio_thread_info->thread_id, NULL);
    }
    
    _AV_SAFE_DELETE_(m_pAudio_thread_info);

#endif
#endif

#if defined(_AV_PLATFORM_HI3515_)    
	for(int i = 0 ; i < SAME_OSD_CACHE_ITEM_NUM ; i++)
	{
		for(int j = 0 ; j < SAME_OSD_CACHE_RESOLUTION_NUM ; j++)
		{
			if(m_SameOsdCache[i][j].szData != NULL)
			{
				delete[] m_SameOsdCache[i][j].szData;
				m_SameOsdCache[i][j].szData = NULL;
			}
		}
	}
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
#if !defined(_AV_PLATFORM_HI3515_)
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
#endif
int CHiAvEncoder::Ae_destory_audio_encoder(av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, audio_encoder_id_t *pAudio_encoder_id)
{
    AENC_CHN aenc_chn;

//!Modified for IPC transplant which employs VQE function, on Nov 2014
#if defined(_AV_PRODUCT_CLASS_IPC_)	
    if (NULL != m_pAudio_thread_info)
    {
        m_pAudio_thread_info->thread_state = false;
        pthread_join(m_pAudio_thread_info->thread_id, NULL);
        _AV_SAFE_DELETE_(m_pAudio_thread_info);
    }
#else
#if !defined(_AV_PLATFORM_HI3515_)
    if (Ae_ai_aenc_bind_control(audio_stream_type, phy_audio_chn_num, false) != 0)//!<Unbind
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destory_audio_encoder Ae_ai_aenc_bind_control FAILED(type:%d)(chn:%d)\n", audio_stream_type, phy_audio_chn_num);
        return -1;
    }
#endif
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
    DEBUG_ERROR("audio type=%d\n", pAudio_para->type);
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
            aenc_attr_g726.enG726bps = MEDIA_G726_16K;//MEDIA_G726_40K;
            break;
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
            
#ifdef _AV_PLATFORM_HI3515_
			aenc_attr_aac.enSoundMode = AUDIO_SOUND_MODE_MOMO;
            if(PTD_6A_I == pgAvDeviceInfo->Adi_product_type())
            {
				aenc_attr_aac.enBitRate = AAC_BPS_48K;
				aenc_attr_aac.enSmpRate = AUDIO_SAMPLE_RATE_16000;
            }
#else
			aenc_attr_aac.enSoundMode = AUDIO_SOUND_MODE_MONO;
			HI_MPI_AENC_AacInit();
#endif            
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
	    aenc_chn_attr.u32PtNumPerFrm = 320;
#endif
	DEBUG_WARNING("CHiAvEncoder::Ae_create_audio_encoder, aencChn = %d, phyChn = %d, type = %d\n", aenc_chn, phy_audio_chn_num, audio_stream_type);
    if((ret_val = HI_MPI_AENC_CreateChn(aenc_chn, &aenc_chn_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_audio_encoder HI_MPI_AENC_CreateChn FAILED(type:%d)(chn:%d)(ret_val:0x%08lx)\n", audio_stream_type, phy_audio_chn_num, (unsigned long)ret_val);
        return -1;
    }

#if defined(_AV_PRODUCT_CLASS_IPC_)
    Ae_ai_aenc_proc(audio_stream_type, phy_audio_chn_num);
#else
#if !defined(_AV_PLATFORM_HI3515_)
    if(Ae_ai_aenc_bind_control(audio_stream_type, phy_audio_chn_num, true) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_audio_encoder Ae_ai_aenc_bind_control FAILED(type:%d)(chn:%d)\n", audio_stream_type, phy_audio_chn_num);
        return -1;
    }
#endif	
#endif
    AUDIO_DEV ai_dev;
    AI_CHN ai_chn;
    m_pHi_ai->HiAi_get_ai_info(_AI_NORMAL_,phy_audio_chn_num,&ai_dev, &ai_chn, NULL);

    if((ret_val = HI_MPI_AENC_BindAi(aenc_chn, ai_dev, ai_chn)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_audio_encoder HI_MPI_AENC_CreateChn FAILED(type:%d)(chn:%d)(ret_val:0x%08lx)\n", audio_stream_type, phy_audio_chn_num, (unsigned long)ret_val);
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
    
    DEBUG_WARNING( "CHiAvEncoder::Ae_destory_video_encoder (type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);

    VENC_GRP venc_grp = -1;
    VENC_CHN venc_chn = -1;

#if defined(_AV_HISILICON_VPSS_)
    if(Ae_venc_vpss_control(video_stream_type, phy_video_chn_num, false) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destory_video_encoder Ae_venc_vpss_control FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
        return -1;
    }
#endif
#if defined(_AV_PLATFORM_HI3515_)
    if(video_stream_type == _AST_MAIN_VIDEO_) //!< for main sub 3515
    {
    	if(Ae_bind_vi_to_venc(video_stream_type, phy_video_chn_num, false) != 0)
    	{
    		DEBUG_ERROR( "CHiAvEncoder::Ae_destory_video_encoder Ae_venc_vpss_control FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
    		return -1;
    	}
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
        //return -1;
    }
#endif	
    if((ret_val = HI_MPI_VENC_DestroyChn(venc_chn)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destory_video_encoder HI_MPI_VENC_DestroyChn FAILED(type:%d)(chn:%d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, (unsigned long)ret_val);
        return -1;
    }
#ifndef _AV_PLATFORM_HI3520D_V300_	
    if(video_stream_type == _AST_MAIN_VIDEO_) //!< for main sub 3515
    {
        if((ret_val = HI_MPI_VENC_DestroyGroup(venc_grp)) != HI_SUCCESS)
        {
            DEBUG_ERROR( "CHiAvEncoder::Ae_destory_video_encoder HI_MPI_VENC_DestroyGroup FAILED(type:%d)(chn:%d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, (unsigned long)ret_val);
            return -1;
        }
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
int CHiAvEncoder::Ae_create_snap_encoder(int vi_chn, av_resolution_t eResolution, av_tvsystem_t eTvSystem, unsigned int factor, hi_snap_osd_t pSnapOsd[6], int osdNum)
{
    DEBUG_MESSAGE("###### N9M Ae_create_snap_encoder\n");
    if( m_bSnapEncoderInited )
    {
    	DEBUG_ERROR("Ae_create_snap_encoder, snap encoder has created\n");
        return 0;
    }
    
    VENC_GRP snapGrp = -1;
    VENC_CHN snapChl = -1;
     
    if( Ae_video_encoder_allotter_snap(&snapGrp, &snapChl) != 0 )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder Ae_video_encoder_allotter_snap FAILED\n");
        return -1;
    }
	int my_vi_chn = 0;
	VI_DEV my_vi_dev = 0;
	if(m_pHi_vi->HiVi_get_primary_vi_info(vi_chn, &my_vi_dev, &my_vi_chn, NULL) != 0)
	{
		DEBUG_ERROR( "CHiVi::Hivi_get_vi_frame HiVi_get_primary_vi_info failed\n" );
		return -1;
	}

    HI_S32 sRet = HI_SUCCESS;
    sRet = HI_MPI_VENC_CreateGroup( snapGrp );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder HI_MPI_VENC_CreateGroup FAILED width 0x%x, group=%d\n", sRet, snapGrp);
        return -1;
    }
    int iWidth = 0, iHeight = 0;
    pgAvDeviceInfo->Adi_get_video_size( eResolution, eTvSystem, &iWidth, &iHeight);

    VENC_CHN_ATTR_S stuVencChnAttr;
    VENC_ATTR_JPEG_S stuJpegAttr;
	memset(&stuVencChnAttr,0,sizeof(stuVencChnAttr));
	memset(&stuJpegAttr,0,sizeof(stuJpegAttr));

	stuJpegAttr.u32PicWidth	= iWidth;  //编码图像宽度
	stuJpegAttr.u32PicHeight = iHeight; //编码图像高度
	stuJpegAttr.u32BufSize   = iHeight * iWidth * 2;   //配置BUFFER大小
	stuJpegAttr.bVIField     = HI_FALSE;   //输入图像的帧场标志，TRUE是场，FALSE是帧
	stuJpegAttr.bByFrame     = HI_TRUE;  //获取码流模式，帧或包，TRUE按帧获取，FALSE按包获取
	stuJpegAttr.u32MCUPerECS = iHeight * iWidth / 256;  //每个ECS中的MCU个数
	stuJpegAttr.u32Priority	= 0;  //通道优先级，目前不使用
	stuJpegAttr.u32ImageQuality = 1;  //抓拍图像质量
	
#if defined(_AV_PLATFORM_HI3515_)
	if(PTD_6A_I == pgAvDeviceInfo->Adi_product_type())
	{
		if(factor < 70)	//画质6 、7 、8等级时，画质为1///
		{
			stuJpegAttr.u32ImageQuality = 2;
		}
	}
#endif

	stuVencChnAttr.enType = PT_JPEG;
	stuVencChnAttr.pValue = &stuJpegAttr;
    sRet = HI_MPI_VENC_CreateChn( snapChl, &stuVencChnAttr, NULL);

    if( sRet != HI_SUCCESS )
    {
		HI_MPI_VENC_DestroyGroup(snapGrp);
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder HI_MPI_VENC_CreateChn err:0x%08x\n", sRet);
        return -1;
    }

    /*note: bind snap group to vichn, not unbind while snapping*/
    DEBUG_CRITICAL("bind videv:%d, vichn:%d \n", my_vi_dev, my_vi_chn);
    sRet = HI_MPI_VENC_BindInput(snapGrp, my_vi_dev, my_vi_chn);
    if (sRet != HI_SUCCESS)
    {
        printf("HI_MPI_VENC_BindInput err 0x%x\n", sRet);
        return -1;
    }
	
    sRet = HI_MPI_VENC_RegisterChn( snapGrp, snapChl );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder HI_MPI_VENC_RegisterChn err:0x%08x\n", sRet);
        return -1;
    }
    
#if !defined(_AV_PLATFORM_HI3515_)
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
#else
    /* This is recommended if you snap one picture each time. */
    sRet = HI_MPI_VENC_SetMaxStreamCnt(snapChl, 1);
    if (sRet != HI_SUCCESS)
    {
        DEBUG_ERROR("HI_MPI_VENC_SetMaxStreamCnt(%d) err 0x%x\n", snapChl, sRet);
        return HI_FAILURE;
    }
#endif
#if !defined(_AV_PLATFORM_HI3515_)
    sRet = HI_MPI_VENC_StartRecvPic( snapChl );
    if( sRet != HI_SUCCESS )
    {
		HI_MPI_VENC_UnbindInput(snapGrp);
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder HI_MPI_VENC_StartRecvPic err:0x%08x\n", sRet);
        return -1;
    }
#endif
    for(int i=0; i<osdNum; i++)
    {
        if(strlen(pSnapOsd[i].szStr) > 0)
        {
            DEBUG_MESSAGE("Ae_snap_regin_create index=%d\n",i);
            this->Ae_snap_regin_create( snapGrp, eResolution, eTvSystem, i, pSnapOsd);
        }
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
	
#if defined(_AV_PLATFORM_HI3515_)
	for(int i=0; i<4; i++)
#else
	for(int i=0; i<6; i++)
#endif
	{
		if(stuOsdInfo[i].bShow == 1 && strlen(stuOsdInfo[i].szStr) > 0)
		{
			DEBUG_MESSAGE("Ae_snap_regin_delete index=%d\n", i);
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

    sRet = HI_MPI_VENC_UnbindInput(snapGrp);
    if (sRet != HI_SUCCESS)
    {
        printf("HI_MPI_VENC_UnbindInput err 0x%x\n", sRet);
    }
	
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

    if((NULL == stuOsdParam) || (osdNum < 0))
    {
        DEBUG_ERROR( "param is invalid,stuOsdParam(%p), osdNum(%d)\n", stuOsdParam, osdNum);   
        return -1;
    }
#if defined(_AV_PLATFORM_HI3515_)
	if(PTD_6A_I == pgAvDeviceInfo->Adi_product_type())
	{
		for(int index = 0 ; index < osdNum ; ++index)
		{
			DEBUG_MESSAGE("Ae_snap_regin_set_osds, index = %d, num = %d, show = %d, str = %s\n", index, osdNum, stuOsdParam[index].bShow, stuOsdParam[index].szStr);
			if (NULL == m_stuOsdTemp[index].pszBitmap)
			{
				continue;
			}
			
            Ae_snap_regin_set_osd( index, stuOsdParam[index].bShow, stuOsdParam[index].x, stuOsdParam[index].y, stuOsdParam[index].szStr); 
		}
		return 0;
	}
#endif
    //DEBUG_MESSAGE("set_osds date x= %d y= %d\n", stuOsdParam[0].x, stuOsdParam[0].y);
	//DEBUG_MESSAGE("set_osds bus number x= %d y= %d\n", stuOsdParam[1].x, stuOsdParam[1].y);
	//DEBUG_MESSAGE("set_osds speed x= %d y= %d\n", stuOsdParam[2].x, stuOsdParam[2].y);
	//DEBUG_MESSAGE("set_osds gps x= %d y= %d\n", stuOsdParam[3].x, stuOsdParam[3].y);
	//DEBUG_MESSAGE("set_osds chn name x= %d y= %d\n", stuOsdParam[4].x, stuOsdParam[4].y);
	//DEBUG_MESSAGE("set_osds self number x= %d y= %d\n", stuOsdParam[5].x, stuOsdParam[5].y);
	char comb_osd[128] = {0};
    //osdNum = 0;
    for(int i=0; i<osdNum; i++)
    {
        if ((NULL == m_stuOsdTemp[i].pszBitmap)||(0 == m_stuOsdTemp[i].hOsdRegin))
        {
            continue;
        }
        if ((i == 0)||(i ==2)||(i == 4))
        {
            printf("set osd index:%d\n", i);
            //DEBUG_CRITICAL("to create!osd0:%s, osd2:%s, osd4:%s!\n", stuOsdParam[0].szStr,stuOsdParam[2].szStr,stuOsdParam[4].szStr);
            memset(comb_osd, 0, sizeof(comb_osd));
            sprintf(comb_osd,"this is osd1");
            //sprintf(comb_osd,"%s  %s   %s",  stuOsdParam[4].szStr, stuOsdParam[0].szStr,stuOsdParam[2].szStr);
           
            Ae_snap_regin_set_osd( i, stuOsdParam[i].bShow, stuOsdParam[i].x, stuOsdParam[i].y, comb_osd ); 

        }
        else if ((i == 1)||(i ==3)||(i == 5))
        {
            //DEBUG_CRITICAL("1, osd1:%s, osd3:%s, osd5:%s!\n", stuOsdParam[1].szStr,stuOsdParam[3].szStr,stuOsdParam[5].szStr);
            memset(comb_osd, 0, sizeof(comb_osd));
            sprintf(comb_osd,"this is osd2");

            //sprintf( comb_osd, "%s  %s   %s",stuOsdParam[3].szStr, stuOsdParam[1].szStr,stuOsdParam[5].szStr);
            Ae_snap_regin_set_osd( i, stuOsdParam[i].bShow, stuOsdParam[i].x, stuOsdParam[i].y, comb_osd ); 
        }
        else
        {
            if(strlen(stuOsdParam[i].szStr) !=0)
            {
                //Ae_snap_regin_set_osd( i, stuOsdParam[i].bShow, stuOsdParam[i].x, stuOsdParam[i].y, stuOsdParam[i].szStr ); 
            }
        }
    }
    //DEBUG_ERROR("set osd conplete!\n");
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

    int ret_val;

    int textColor = 0xffff;
#if (defined(_AV_PLATFORM_HI3520D_V300_)||defined(_AV_PLATFORM_HI3515_))
	if((OSD_TEST_FLAG == pgAvDeviceInfo->Adi_product_type())||(PTD_6A_I == pgAvDeviceInfo->Adi_product_type()))
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
    DEBUG_MESSAGE("to bmp:%s, osd:%d, res:%d,width:%d,height:%d, len:%d, map:%p\n", pszStr, m_stuOsdTemp[index].eOsdName, m_stuOsdTemp[index].eResolution,m_stuOsdTemp[index].u32BitmapWidth, m_stuOsdTemp[index].u32BitmapHeight, strlen(pszStr), m_stuOsdTemp[index].pszBitmap);
    if(Ae_utf8string_2_bmp( m_stuOsdTemp[index].eOsdName, m_stuOsdTemp[index].eResolution, m_stuOsdTemp[index].pszBitmap,\
        m_stuOsdTemp[index].u32BitmapWidth, m_stuOsdTemp[index].u32BitmapHeight, pszStr, textColor, 0x00, 0x8000) != 0)
    {
    	DEBUG_ERROR("Ae_utf8string_2_bmp failed\n");
        return -1;
    }

    //! set bitmap
    REGION_CRTL_CODE_E enCtrl; 
    REGION_CTRL_PARAM_U unParam;   
    memset(&unParam, 0, sizeof(unParam));
    unParam.stRegionAttr.enType = OVERLAY_REGION;
    
	unParam.u32Alpha = 0;
	ret_val = HI_MPI_VPP_ControlRegion(m_stuOsdTemp[index].hOsdRegin,REGION_SET_ALPHA0, &unParam);
	if(ret_val != SUCCESS)
	{
		DEBUG_ERROR("show region REGION_SET_ALPHA0 faild %d!\n",ret_val);
		return -1;
	}

	//show region
	ret_val = HI_MPI_VPP_ControlRegion(m_stuOsdTemp[index].hOsdRegin,REGION_SHOW, &unParam);
	if(ret_val != SUCCESS)
	{
		DEBUG_ERROR("show region REGION_SHOW faild %d!\n",ret_val);
		return -1;
	}
    
    enCtrl = REGION_SET_BITMAP;   

    unParam.stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
    unParam.stBitmap.u32Height = m_stuOsdTemp[index].u32BitmapHeight; 
    unParam.stBitmap.u32Width = m_stuOsdTemp[index].u32BitmapWidth;
    unParam.stBitmap.pData = m_stuOsdTemp[index].pszBitmap;

    if ((ret_val = HI_MPI_VPP_ControlRegion(m_stuOsdTemp[index].hOsdRegin, enCtrl, &unParam)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_osd_overlay HI_MPI_VPP_ControlRegion FAILED(region_handle:0x%08x)(%d)\n", ret_val, m_stuOsdTemp[index].hOsdRegin);
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
                    _AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_snap_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
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
                DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                return -1;
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
                DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                return -1;
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
                DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                return -1;
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

        
        //printf("index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
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
        printf("-------------lshhh\n");
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

#else//!< non ipc or 3520

int CHiAvEncoder::Ae_snap_regin_create( VENC_GRP vencGrp, av_resolution_t eResolution, av_tvsystem_t eTvSystem, int index, hi_snap_osd_t pSnapOsd[] )
{
	if( m_stuOsdTemp[index].bInited )
    {
        return 0;
    }
    
    REGION_ATTR_S stuRgnAttr;
    memset(&stuRgnAttr, 0, sizeof(REGION_ATTR_S));
    int fontWidth, fontHeight;
    int horScaler, verScaler;
    int font;

    int iWidth = 0, iHeight = 0;
    pgAvDeviceInfo->Adi_get_video_size( eResolution, eTvSystem, &iWidth, &iHeight);
    //iHeight = 288;
    
#if defined(_AV_PLATFORM_HI3515_)
		if(PTD_6A_I == pgAvDeviceInfo->Adi_product_type())
		{
			Ae_get_osd_parameter( _AON_EXTEND_OSD, eResolution, &font, &fontWidth, &fontHeight, &horScaler, &verScaler, index );
			
			stuRgnAttr.unAttr.stOverlay.stRect.u32Width  = 1024;
			//3515叠加区域高度适当增大，以防创建的区域在赋值的时候越界///
			stuRgnAttr.unAttr.stOverlay.stRect.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_ ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
			
			if (stuRgnAttr.unAttr.stOverlay.stRect.u32Width > (HI_U32)iWidth)
			{
				stuRgnAttr.unAttr.stOverlay.stRect.u32Width = iWidth;
			}
			if (stuRgnAttr.unAttr.stOverlay.stRect.u32Height > (HI_U32)iHeight)
			{
				stuRgnAttr.unAttr.stOverlay.stRect.u32Height = iHeight;
			}
			DEBUG_MESSAGE("Ae_snap_regin_create, index = %d, show = %d, str = %s\n", index, pSnapOsd[index].bShow, pSnapOsd[index].szStr);
	        if((pSnapOsd[index].bShow == 1) && (strlen(pSnapOsd[index].szStr) > 0))
	        {    
	            m_stuOsdTemp[index].vencGrp = vencGrp;

	            /*create region*/
	            stuRgnAttr.enType = OVERLAY_REGION;
	            stuRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;

	            stuRgnAttr.unAttr.stOverlay.stRect.s32X= (pSnapOsd[index].x + 7) / 8 * 8 ;
	            stuRgnAttr.unAttr.stOverlay.stRect.s32Y= (pSnapOsd[index].y + + 7) / 8 * 8;
	            
	            stuRgnAttr.unAttr.stOverlay.stRect.u32Width = (stuRgnAttr.unAttr.stOverlay.stRect.u32Width + 1) / 2 * 2;
	            stuRgnAttr.unAttr.stOverlay.stRect.u32Height = (stuRgnAttr.unAttr.stOverlay.stRect.u32Height + 1) / 2 * 2;
	    	
	            stuRgnAttr.unAttr.stOverlay.u32BgAlpha = 0;//0:背景完全透明
	            stuRgnAttr.unAttr.stOverlay.u32FgAlpha = 128;//128:完全不透明
	            stuRgnAttr.unAttr.stOverlay.u32BgColor = 0x0;
	        
	            stuRgnAttr.unAttr.stOverlay.VeGroup = vencGrp;
	        
	            stuRgnAttr.unAttr.stOverlay.bPublic = HI_FALSE;

	            //--------------------------------------------------------------------    
	            int retval = -1;
	            retval = HI_MPI_VPP_CreateRegion(&stuRgnAttr, &(m_stuOsdTemp[index].hOsdRegin)); // handle is output
	            if(retval != SUCCESS)
	            {
	                DEBUG_ERROR("HI_MPI_VPP_CreateRegion err 0x%x!\n",retval);
	                DEBUG_ERROR("osd:%d, [%d, %d,%d,%d,%d,%d,%d,%d,%d,%d,%d] \n",
			  index, stuRgnAttr.unAttr.stOverlay.stRect.s32X, stuRgnAttr.unAttr.stOverlay.stRect.s32Y, 
	                stuRgnAttr.unAttr.stOverlay.stRect.u32Width, stuRgnAttr.unAttr.stOverlay.stRect.u32Height,
	                stuRgnAttr.unAttr.stOverlay.u32BgColor, stuRgnAttr.unAttr.stOverlay.VeGroup,
	                stuRgnAttr.unAttr.stOverlay.bPublic,stuRgnAttr.unAttr.stOverlay.enPixelFmt,
	                stuRgnAttr.unAttr.stOverlay.u32BgAlpha,stuRgnAttr.unAttr.stOverlay.u32BgColor,
	                stuRgnAttr.unAttr.stOverlay.u32FgAlpha);
	                
	                return -1;
	            }

	            //------------------------- control region-----------------------------------------
	            REGION_CRTL_CODE_E enCtrl;
	            enCtrl = REGION_SHOW;/*show region*/
	            REGION_CTRL_PARAM_U unParam;

	            retval = HI_MPI_VPP_ControlRegion(m_stuOsdTemp[index].hOsdRegin, REGION_SHOW, &unParam);
	            if(retval != SUCCESS)
	            {
	                DEBUG_ERROR("show region %d faild 0x%x at 2!\n",index,retval);
	                return -1;
	            }

        		m_stuOsdTemp[index].eOsdName = _AON_EXTEND_OSD;
	            m_stuOsdTemp[index].eResolution = eResolution;

	            /*bitmap*/
	            m_stuOsdTemp[index].u32BitmapWidth = stuRgnAttr.unAttr.stOverlay.stRect.u32Width;
	            m_stuOsdTemp[index].u32BitmapHeight = stuRgnAttr.unAttr.stOverlay.stRect.u32Height;
	            m_stuOsdTemp[index].pszBitmap = new unsigned char[m_stuOsdTemp[index].u32BitmapWidth * horScaler * m_stuOsdTemp[index].u32BitmapHeight * verScaler * 2];

	            DEBUG_MESSAGE("CHiAvEncoder::Ae_snap_regin_create, w:%d, h:%d, x:%d, y:%d, vs:%d, hs:%d, bit map size:%d \n", m_stuOsdTemp[index].u32BitmapWidth,m_stuOsdTemp[index].u32BitmapHeight,\
	                stuRgnAttr.unAttr.stOverlay.stRect.s32X, stuRgnAttr.unAttr.stOverlay.stRect.s32Y, verScaler,horScaler,m_stuOsdTemp[index].u32BitmapWidth * horScaler * m_stuOsdTemp[index].u32BitmapHeight * verScaler * 2);

	            _AV_FATAL_(  m_stuOsdTemp[index].pszBitmap == NULL, "OUT OF MEMORY\n" );
	            
	            m_stuOsdTemp[index].bInited = true;
		        m_stuOsdTemp[index].osdTextColor = pSnapOsd[index].color;
	        }
	        return 0;
		}
#endif

    //for (int index = 0; index < 6; index++)
    {      
		Ae_get_osd_parameter( _AON_DATE_TIME_, eResolution, &font, &fontWidth, &fontHeight, &horScaler, &verScaler );
		
        stuRgnAttr.unAttr.stOverlay.stRect.u32Width  = 1024;
        stuRgnAttr.unAttr.stOverlay.stRect.u32Height = (fontHeight * verScaler +4 ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;

        if (stuRgnAttr.unAttr.stOverlay.stRect.u32Width> (HI_U32)iWidth)
        {
            stuRgnAttr.unAttr.stOverlay.stRect.u32Width = iWidth;
        }
        if (stuRgnAttr.unAttr.stOverlay.stRect.u32Height> (HI_U32)iHeight)
        {
            stuRgnAttr.unAttr.stOverlay.stRect.u32Height = iHeight;
        }

#if defined(_AV_PLATFORM_HI3515_)
        if((0 == index)||(2 == index)||(4 == index))
        {
            pSnapOsd[index].x = 0;
            pSnapOsd[index].y = 0;
        }
        if((1 == index)||(3 == index)||(5 == index))
        {
            pSnapOsd[index].x = 0;
            pSnapOsd[index].y = iHeight - fontHeight * verScaler;
        }
#endif
        /*
                	
    	if(index == 6)
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
            m_stuOsdTemp[index].vencGrp = vencGrp;

            /*create region*/
            stuRgnAttr.enType = OVERLAY_REGION;
            stuRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;

            stuRgnAttr.unAttr.stOverlay.stRect.s32X= (pSnapOsd[index].x + 7) / 8 * 8 ;
            stuRgnAttr.unAttr.stOverlay.stRect.s32Y= (pSnapOsd[index].y + + 7) / 8 * 8;
            
            stuRgnAttr.unAttr.stOverlay.stRect.u32Width = (stuRgnAttr.unAttr.stOverlay.stRect.u32Width + 1) / 2 * 2;
            stuRgnAttr.unAttr.stOverlay.stRect.u32Height = (stuRgnAttr.unAttr.stOverlay.stRect.u32Height + 1) / 2 * 2;
    	
            stuRgnAttr.unAttr.stOverlay.u32BgAlpha = 0;//0:背景完全透明
            stuRgnAttr.unAttr.stOverlay.u32FgAlpha = 128;//128:完全不透明
            stuRgnAttr.unAttr.stOverlay.u32BgColor = 0x00;
        
            stuRgnAttr.unAttr.stOverlay.VeGroup = vencGrp;
        
            stuRgnAttr.unAttr.stOverlay.bPublic = HI_FALSE;

            //--------------------------------------------------------------------    
            int retval = -1;
            
            retval = HI_MPI_VPP_CreateRegion(&stuRgnAttr, &(m_stuOsdTemp[index].hOsdRegin)); // handle is output
		
            if(retval != SUCCESS)
            {
                DEBUG_ERROR("HI_MPI_VPP_CreateRegion err 0x%x!\n",retval);
                DEBUG_ERROR("osd:%d, [%d, %d,%d,%d,%d,%d,%d,%d,%d,%d,%d] \n",
		  index, stuRgnAttr.unAttr.stOverlay.stRect.s32X, stuRgnAttr.unAttr.stOverlay.stRect.s32Y, 
                stuRgnAttr.unAttr.stOverlay.stRect.u32Width, stuRgnAttr.unAttr.stOverlay.stRect.u32Height,
                stuRgnAttr.unAttr.stOverlay.u32BgColor, stuRgnAttr.unAttr.stOverlay.VeGroup,
                stuRgnAttr.unAttr.stOverlay.bPublic,stuRgnAttr.unAttr.stOverlay.enPixelFmt,
                stuRgnAttr.unAttr.stOverlay.u32BgAlpha,stuRgnAttr.unAttr.stOverlay.u32BgColor,
                stuRgnAttr.unAttr.stOverlay.u32FgAlpha);
                
                return -1;
            }

            //------------------------- control region-----------------------------------------
            REGION_CRTL_CODE_E enCtrl;
            enCtrl = REGION_SHOW;/*show region*/
            REGION_CTRL_PARAM_U unParam;

            retval = HI_MPI_VPP_ControlRegion(m_stuOsdTemp[index].hOsdRegin, enCtrl, &unParam);
            if(retval != SUCCESS)
            {
                DEBUG_ERROR("show region %d faild 0x%x at 2!\n",index,retval);
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
        	}*/
        	
            //m_stuOsdTemp[index].eOsdName = (index==0? _AON_DATE_TIME_: _AON_CHN_NAME_);
            m_stuOsdTemp[index].eResolution = eResolution;


            /*bitmap*/
            m_stuOsdTemp[index].u32BitmapWidth = stuRgnAttr.unAttr.stOverlay.stRect.u32Width;
            m_stuOsdTemp[index].u32BitmapHeight = stuRgnAttr.unAttr.stOverlay.stRect.u32Height;
            m_stuOsdTemp[index].pszBitmap = new unsigned char[m_stuOsdTemp[index].u32BitmapWidth * horScaler * m_stuOsdTemp[index].u32BitmapHeight * verScaler * 2];

            DEBUG_DEBUG("w:%d, h:%d, vs:%d, hs:%d, bit map size:%d \n", m_stuOsdTemp[index].u32BitmapWidth,m_stuOsdTemp[index].u32BitmapHeight,\
                verScaler,horScaler,\
                m_stuOsdTemp[index].u32BitmapWidth * horScaler * m_stuOsdTemp[index].u32BitmapHeight * verScaler * 2);


            _AV_FATAL_(  m_stuOsdTemp[index].pszBitmap == NULL, "OUT OF MEMORY\n" );
            m_stuOsdTemp[index].bInited = true;
        m_stuOsdTemp[index].osdTextColor = pSnapOsd[index].color;
            if ((index ==0)|| (index ==2)||(index ==4))
            {
               m_stuOsdTemp[0].bInited = true;
               m_stuOsdTemp[2].bInited = true;
               m_stuOsdTemp[4].bInited = true;

            }
            if ((index ==1)|| (index ==3)||(index ==5))
            {
               m_stuOsdTemp[1].bInited = true;
               m_stuOsdTemp[3].bInited = true;
               m_stuOsdTemp[5].bInited = true;

            }

        }
    }
    return 0;
}
#endif

#endif

int CHiAvEncoder::Ae_snap_regin_delete( int index )
{
	DEBUG_MESSAGE("CHiAvEncoder::Ae_snap_regin_delete, index = %d, init = %d, bitmap = %p, regin = %p\n", index, m_stuOsdTemp[index].bInited, m_stuOsdTemp[index].pszBitmap, m_stuOsdTemp[index].hOsdRegin);
    if(( !m_stuOsdTemp[index].bInited )||(0 == m_stuOsdTemp[index].hOsdRegin))
    {
        return 0;
    }
    HI_S32 sRet = HI_SUCCESS;

    if(m_stuOsdTemp[index].pszBitmap != NULL)
    {
        delete[] m_stuOsdTemp[index].pszBitmap;
        m_stuOsdTemp[index].pszBitmap = NULL;
    }
    m_stuOsdTemp[index].bInited = false;
    if( (sRet=HI_MPI_VPP_DestroyRegion(m_stuOsdTemp[index].hOsdRegin)) != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_snap_delete_regin HI_MPI_RGN_Destroy FAILED(0x%08x)\n", sRet);
    }
    m_stuOsdTemp[index].hOsdRegin = 0;
    return 0;
}

int CHiAvEncoder::Ae_encode_vi_to_picture(int vi_chn, VIDEO_FRAME_INFO_S *pstuViFrame, hi_snap_osd_t stuOsdParam[], int osd_num,  VENC_STREAM_S* pstuPicStream)
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

#if defined(_AV_PLATFORM_HI3515_)
	int my_vi_chn = 0;
	VI_DEV my_vi_dev = 0;
	if(m_pHi_vi->HiVi_get_primary_vi_info(vi_chn, &my_vi_dev, &my_vi_chn, NULL) != 0)
	{
		DEBUG_ERROR( "CHiAvEncoder::Ae_encode_vi_to_picture HiVi_get_primary_vi_info failed\n" );
		return -1;
	}
	DEBUG_MESSAGE("CHiAvEncoder::Ae_encode_vi_to_picture, dev = %d, chn = %d, grp = %d, chn = %d\n", my_vi_dev, my_vi_chn, snapGrp, snapChl);
    sRet = HI_MPI_VENC_SendFrame(my_vi_dev, my_vi_chn, pstuViFrame->u32PoolId, (VIDEO_FRAME_S *)pstuViFrame, HI_TRUE);
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_encode_vi_to_picture HI_MPI_VENC_SendFrame err:0x%08x\n", sRet);
        return -1;
    }
#endif
#if defined(_AV_PLATFORM_HI3515_)
	sRet = HI_MPI_VENC_StartRecvPic( snapChl );
	if( sRet != HI_SUCCESS )
	{
		HI_MPI_VENC_UnbindInput(snapGrp);
		DEBUG_ERROR( "CHiAvEncoder::Ae_create_snap_encoder HI_MPI_VENC_StartRecvPic err:0x%08x\n", sRet);
		return -1;
	}
#endif

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

    TimeoutVal.tv_sec = 2;//!< 2 
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
	printf("mpp_chn_src.s32DevId:%d  mpp_chn_src.s32ChnId:%d mpp_chn_dst.s32DevId:%d mpp_chn_dst.s32ChnId:%d\n",mpp_chn_src.s32DevId,mpp_chn_src.s32ChnId,mpp_chn_dst.s32DevId,mpp_chn_dst.s32ChnId);
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
int CHiAvEncoder::Ae_bind_vi_to_venc(av_video_stream_type_t video_stream_type, int phy_video_chn_num, bool bind_flag)
{
	VENC_GRP venc_grp = -1;
	VENC_CHN venc_chn = -1;
	VI_DEV videv = 0;
	VI_CHN vichn = 0;
	HI_S32 s32ret = -1;
	if(Ae_video_encoder_allotter(video_stream_type, phy_video_chn_num, &venc_grp, &venc_chn) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_venc_vpss_control Ae_video_encoder_allotter FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
        return -1;
    }
	m_pHi_vi->HiVi_get_primary_vi_info(phy_video_chn_num,&videv,&vichn,NULL);
	
	if(bind_flag)
	{
		printf("hswangvenc_grp:%d videv:%d vichn:%d\n",venc_grp,videv,vichn);
		s32ret = HI_MPI_VENC_BindInput(venc_grp, videv, vichn);  //绑定 VI 到通道组。 
		if(s32ret != HI_SUCCESS)
		{
			DEBUG_ERROR("HI_MPI_VENC_BindInput err 0x%x index=%d chn%d \n",s32ret,venc_grp,vichn);
		}
	}
	else
	{
		s32ret = HI_MPI_VENC_UnbindInput(venc_grp);  //去除绑定 VI 到通道组。 
		if(s32ret != HI_SUCCESS)
		{
			DEBUG_ERROR("HI_MPI_VENC_BindInput err 0x%x index=%d chn%d \n",s32ret,venc_grp,vichn);
		}
	}

	return 0;
}

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
    VENC_ATTR_H264_S venc_attr_h264;
    VENC_ATTR_H264_RC_S venc_attr_h264_rc;
    memset(&venc_chn_attr, 0, sizeof(VENC_CHN_ATTR_S));
    memset(&venc_attr_h264, 0, sizeof(VENC_ATTR_H264_S));
    memset(&venc_attr_h264_rc,0,sizeof(VENC_ATTR_H264_RC_S));
	
    HI_S32 ret_val = -1;

    /*frame rate, bit rate, gop size*/
    venc_chn_attr.enType = PT_H264;
    venc_chn_attr.pValue = (HI_VOID *)&venc_attr_h264;
    if((ret_val = HI_MPI_VENC_GetChnAttr(venc_chn, &venc_chn_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_dynamic_modify_encoder_local venc_chn:%d HI_MPI_VENC_GetChnAttr FAILED(0x%08x)\n", venc_chn, ret_val);
        return -1;
    }

    switch(pVideo_para->bitrate_mode)
    {
        case _ABM_CBR_:

            venc_attr_h264.enRcMode = RC_MODE_CBR;
            venc_attr_h264.u32TargetFramerate = pVideo_para->frame_rate;
            venc_attr_h264.u32Gop = pVideo_para->gop_size;
            venc_attr_h264.u32Bitrate = pVideo_para->bitrate.hicbr.bitrate;
            //venc_attr_h264.u32StatTime = 1;
            venc_attr_h264.u32PicLevel = 0;
            break;

        case _ABM_VBR_:

            venc_attr_h264.enRcMode = RC_MODE_VBR;
            venc_attr_h264.u32TargetFramerate = pVideo_para->frame_rate;
            venc_attr_h264.u32Gop = pVideo_para->gop_size;
            venc_attr_h264.u32Bitrate = pVideo_para->bitrate.hivbr.bitrate;
            //venc_chn_attr.stRcAttr.stAttrH264Vbr.u32StatTime = 1;
            if(video_stream_type == _AST_SUB2_VIDEO_)
			{
				venc_attr_h264_rc.s32MinQP= 24;
				switch(pVideo_para->net_transfe_mode)
				{
					case 0:
						venc_attr_h264_rc.s32MaxQP = 40;
						break;
					case 1:
						venc_attr_h264_rc.s32MaxQP = 39;
						break;
					case 2:
						venc_attr_h264_rc.s32MaxQP = 38;
						break;
					case 3:
						venc_attr_h264_rc.s32MaxQP = 37;
						break;
					default:
						venc_attr_h264_rc.s32MaxQP = 36;
						break;
				}
				
            	
			}
			else
			{
            	venc_attr_h264_rc.s32MinQP = 24;
            	venc_attr_h264_rc.s32MaxQP = 51;
			}
            break;

        default:
            _AV_FATAL_(1, "CHiAvEncoder::Ae_dynamic_modify_encoder_local You must give the implement(%d)\n", pVideo_para->bitrate_mode);
            break;
    }
    venc_chn_attr.enType = PT_H264;
    venc_chn_attr.pValue = (HI_VOID *)&venc_attr_h264;

    if((ret_val = HI_MPI_VENC_SetChnAttr(venc_chn, &venc_chn_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_dynamic_modify_encoder_local HI_MPI_VENC_SetChnAttr FAILED(0x%08x)!(cbr:%d,vbr:%d, res:%d, fr:%d)\n",\
            ret_val,pVideo_para->bitrate.hicbr.bitrate,pVideo_para->bitrate.hivbr.bitrate,  pVideo_para->resolution,pVideo_para->frame_rate);
        
        return -1;
    }
    return 0;
}

int VencTemporalDenoise(VENC_GRP VeGroup)
{
	HI_S32 s32Ret;
	VIDEO_PREPROC_CONF_S stVppCfg;

	s32Ret = HI_MPI_VPP_GetConf(VeGroup, &stVppCfg);
	if(HI_SUCCESS != s32Ret)
	{
		printf("HI_MPI_VPP_GetConf failed 0x%x!\n", s32Ret); 
		return HI_FAILURE; 
	}

	stVppCfg.bTemporalDenoise = HI_TRUE;

	s32Ret = HI_MPI_VPP_SetConf(VeGroup, &stVppCfg);
	if (HI_SUCCESS != s32Ret) 
	{ 
		printf("HI_MPI_VPP_SetConf failed 0x%x!\n",s32Ret); 
		return HI_FAILURE; 
	}

	return HI_SUCCESS;
}

int CHiAvEncoder::Ae_create_h264_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, video_encoder_id_t *pVideo_encoder_id)
{
    VENC_GRP venc_grp = -1;
    VENC_CHN venc_chn = -1;
    HI_S32 ret_val = -1;
    VENC_ATTR_H264_REF_MODE_E enRefMode = H264E_REF_MODE_1X;
    if(Ae_video_encoder_allotter(video_stream_type, phy_video_chn_num, &venc_grp, &venc_chn) != 0)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder Ae_video_encoder_allotter FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
        return -1;
    }

    VENC_CHN_ATTR_S venc_chn_attr;
    VENC_ATTR_H264_S venc_attr_h264;
    VENC_WM_ATTR_S venc_wm_attr;
	
    memset(&venc_chn_attr, 0, sizeof(VENC_CHN_ATTR_S));
    memset(&venc_attr_h264, 0, sizeof(VENC_ATTR_H264_S));
    memset(&venc_wm_attr,0,sizeof(VENC_WM_ATTR_S));

    /****************video encoder attribute*****************/
    venc_chn_attr.enType = PT_H264;
    /*maximum picture size*/

    if(pVideo_para->resolution != _AR_SIZE_)
    {
        pgAvDeviceInfo->Adi_get_video_size(pVideo_para->resolution, pVideo_para->tv_system, (int *)&venc_attr_h264.u32PicWidth, (int *)&venc_attr_h264.u32PicHeight);
    }
    else
    {
        venc_attr_h264.u32PicWidth = pVideo_para->resolution_w;
        venc_attr_h264.u32PicHeight = pVideo_para->resolution_h;
    }
    //align with 2
    venc_attr_h264.u32PicWidth = (venc_attr_h264.u32PicWidth + 7) / 8 * 8;
    venc_attr_h264.u32PicHeight = (venc_attr_h264.u32PicHeight + 7) / 8 * 8;
    
    /*buffer size*/
//    venc_attr_h264.u32BufSize = pgAvDeviceInfo->Adi_get_pixel_size() * venc_attr_h264.u32PicWidth * venc_attr_h264.u32PicHeight;
    venc_attr_h264.u32BufSize = (HI_U32)(pgAvDeviceInfo->Adi_get_pixel_size() * venc_attr_h264.u32PicWidth * venc_attr_h264.u32PicHeight*2);

    printf("\ncxliu000,resolution[%d],u32PicWidth[%d],u32PicHeight[%d],u32BufSize[%d]\n",pVideo_para->resolution,venc_attr_h264.u32PicWidth,
	venc_attr_h264.u32PicHeight,venc_attr_h264.u32BufSize);
    
    /*get frame mode*/
    venc_attr_h264.bByFrame = HI_TRUE;
    /*field*/
    venc_attr_h264.bField = HI_FALSE;
    /*Is main stream*/
    if(video_stream_type == _AST_MAIN_VIDEO_)
    {
        venc_attr_h264.bMainStream = HI_TRUE;
    }
    else
    {
        venc_attr_h264.bMainStream = HI_FALSE;
    }
    /**/
    venc_attr_h264.u32Priority = 0;
    /**/
    venc_attr_h264.bVIField = HI_TRUE;//HI_FALSE;

    venc_attr_h264.u32MaxDelay = 100;
    /**/
//    venc_chn_attr.stVeAttr.stAttrH264e.u32PicWidth = venc_chn_attr.stVeAttr.stAttrH264e.u32MaxPicWidth;
//    venc_chn_attr.stVeAttr.stAttrH264e.u32PicHeight = venc_chn_attr.stVeAttr.stAttrH264e.u32MaxPicHeight;

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

            venc_attr_h264.enRcMode = RC_MODE_CBR;
            venc_attr_h264.u32TargetFramerate = pVideo_para->frame_rate;
            venc_attr_h264.u32ViFramerate = vi_source_config.frame_rate;			
            venc_attr_h264.u32Gop = pVideo_para->gop_size;
            if (pVideo_para->bitrate.hicbr.bitrate < 2)
            {
                DEBUG_ERROR(" Bitrate(%d) invalid\n!",pVideo_para->bitrate.hicbr.bitrate);
                pVideo_para->bitrate.hicbr.bitrate = 2;              
            }
            venc_attr_h264.u32Bitrate = pVideo_para->bitrate.hicbr.bitrate;
            venc_attr_h264.u32PicLevel = 0;
            break;

        case _ABM_VBR_:
            venc_attr_h264.enRcMode = RC_MODE_VBR;
            venc_attr_h264.u32TargetFramerate = pVideo_para->frame_rate;
            venc_attr_h264.u32ViFramerate = vi_source_config.frame_rate;
            venc_attr_h264.u32Gop = pVideo_para->gop_size;          
            if (pVideo_para->bitrate.hivbr.bitrate < 2)
            {
                DEBUG_ERROR(" Bitrate:%d   invalid\n!",pVideo_para->bitrate.hivbr.bitrate);
                pVideo_para->bitrate.hivbr.bitrate = 2;               
            }
            venc_attr_h264.u32Bitrate = pVideo_para->bitrate.hicbr.bitrate;
            venc_attr_h264.u32PicLevel = 0;
			
            break;

        default:
            _AV_FATAL_(1, "CHiAvEncoder::Ae_create_h264_encoder You must give the implement(%d)\n", pVideo_para->bitrate_mode);
            break;
    }
    printf("venc_attr_h264.u32ViFramerate:%d\n",venc_attr_h264.u32ViFramerate);
    if(video_stream_type == _AST_MAIN_VIDEO_)  //! for 3515 substream
    {
        if((ret_val = HI_MPI_VENC_CreateGroup(venc_grp)) != HI_SUCCESS)
        {
            DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder HI_MPI_VENC_CreateGroup FAILED(type:%d)(chn:%d)(grp:%d, %d)(ret_val:0x%08lx)\n", video_stream_type, phy_video_chn_num, venc_grp, venc_chn, (unsigned long)ret_val);

            return -1;

        }
   
	/*
#if defined(_AV_PLATFORM_HI3515_)
	if(Ae_bind_vi_to_venc(video_stream_type, phy_video_chn_num, true) != 0)
	{
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder Ae_venc_vpss_control FAILED(type:%d)(chn:%d)(grp:%d, %d)\n", video_stream_type, phy_video_chn_num, venc_grp, venc_chn);
        HI_MPI_VENC_UnRegisterChn(venc_chn);
        HI_MPI_VENC_DestroyChn(venc_chn);      
        HI_MPI_VENC_DestroyGroup(venc_grp);//
		return -1;
    }
#endif
*/
    }//! end of video_stream_type == _AST_MAIN_VIDEO_

	
    venc_chn_attr.pValue = (HI_VOID *)&venc_attr_h264;
	
//	venc_chn_attr.pValue = new char[sizeof(venc_attr_h264)];
//	memset(venc_chn_attr.pValue,0,sizeof(venc_attr_h264));
//	memcpy(venc_chn_attr.pValue,&venc_attr_h264,sizeof(venc_attr_h264));

    if((ret_val = HI_MPI_VENC_CreateChn(venc_chn, &venc_chn_attr,HI_NULL)) != HI_SUCCESS)//!&venc_wm_attr
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder HI_MPI_VENC_CreateChn FAILED(type:%d)(chn:%d)(grp:%d, %d)(ret_val:0x%08lx)\
            (cbr:%d)(vbr:%d)(res:%d)(fr:%d)(brm:%d) \n", \
             video_stream_type, phy_video_chn_num, venc_grp, venc_chn, (unsigned long)ret_val, pVideo_para->bitrate.hicbr.bitrate,
             pVideo_para->bitrate.hivbr.bitrate, pVideo_para->resolution, pVideo_para->frame_rate, pVideo_para->bitrate_mode);

        HI_MPI_VENC_DestroyGroup(venc_grp); //  destroy in case of being occupied
        
//	_AV_SAFE_DELETE_(venc_chn_attr.pValue);	
        return -1;
    }
//	_AV_SAFE_DELETE_(venc_chn_attr.pValue);
	
    if (pVideo_para->ref_para.ref_type == _ART_REF_FOR_CUSTOM)
    {

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
        enRefMode = H264E_REF_MODE_1X;
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
    if(video_stream_type == _AST_MAIN_VIDEO_) 
    {
#if defined(_AV_PLATFORM_HI3515_)
	if(Ae_bind_vi_to_venc(video_stream_type, phy_video_chn_num, true) != 0)
	{
        DEBUG_ERROR( "CHiAvEncoder::Ae_create_h264_encoder Ae_venc_vpss_control FAILED(type:%d)(chn:%d)(grp:%d, %d)\n", video_stream_type, phy_video_chn_num, venc_grp, venc_chn);
        HI_MPI_VENC_UnRegisterChn(venc_chn);
        HI_MPI_VENC_DestroyChn(venc_chn);      
        HI_MPI_VENC_DestroyGroup(venc_grp);//
		return -1;
    }
#endif
    }
    memset(pVideo_encoder_id, 0, sizeof(video_encoder_id_t));
    pVideo_encoder_id->video_encoder_fd = HI_MPI_VENC_GetFd(venc_chn);
    hi_video_encoder_info_t *pVei = NULL;
    pVei = new hi_video_encoder_info_t;
    _AV_FATAL_(pVei == NULL, "CHiAvEncoder::Ae_create_h264_encoder OUT OF MEMORY\n");
    memset(pVei, 0, sizeof(hi_video_encoder_info_t));
    pVei->venc_chn = venc_chn;
    pVei->venc_grp = venc_grp;
    pVideo_encoder_id->pVideo_encoder_info = (void *)pVei;

    DEBUG_CRITICAL( "CHiAvEncoder::Ae_create_h264_encoder(type:%d)(group:%d)(phy_chn:%d)(chn:%d)(res:%d)(frame_rate:%d)(bitrate:%d)(gop:%d)(fd:%d)\n", video_stream_type, venc_grp, phy_video_chn_num, venc_chn, pVideo_para->resolution, pVideo_para->frame_rate, pVideo_para->bitrate.hicbr.bitrate, pVideo_para->gop_size, pVideo_encoder_id->video_encoder_fd);
    return 0;
}

int CHiAvEncoder::Ae_video_encoder_allotter_V1(av_video_stream_type_t video_stream_type, int phy_video_chn_num, VENC_GRP *pGrp, VENC_CHN *pChn)
{   
    switch(video_stream_type)
    {
        case _AST_MAIN_VIDEO_:
            _AV_POINTER_ASSIGNMENT_(pGrp, phy_video_chn_num);
            _AV_POINTER_ASSIGNMENT_(pChn, phy_video_chn_num*2);
            break;

        case _AST_SUB_VIDEO_:
            _AV_POINTER_ASSIGNMENT_(pGrp, phy_video_chn_num);
            //_AV_POINTER_ASSIGNMENT_(pGrp, phy_video_chn_num + pgAvDeviceInfo->Adi_get_vi_chn_num());
            _AV_POINTER_ASSIGNMENT_(pChn, phy_video_chn_num + pgAvDeviceInfo->Adi_get_vi_chn_num());
            break;

        case _AST_SUB2_VIDEO_:
            _AV_POINTER_ASSIGNMENT_(pGrp, phy_video_chn_num);
            //_AV_POINTER_ASSIGNMENT_(pGrp, phy_video_chn_num + pgAvDeviceInfo->Adi_get_vi_chn_num());
            //_AV_POINTER_ASSIGNMENT_(pChn, phy_video_chn_num + pgAvDeviceInfo->Adi_get_vi_chn_num() * 2);
            _AV_POINTER_ASSIGNMENT_(pChn, phy_video_chn_num*2 +1);
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

int CHiAvEncoder::Ae_video_encoder_allotter_snap( VENC_GRP *pGrp, VENC_CHN *pChn)
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
        bBlock = HI_FALSE; // false HI_IO_BLOCK
    }

    memset(pStream_data, 0, sizeof(av_stream_data_t));

    if((ret_val = HI_MPI_VENC_Query(pVei->venc_chn, &venc_chn_state)) != HI_SUCCESS)
    {
        //DEBUG_ERROR( "CHiAvEncoder::Ae_get_video_stream HI_MPI_VENC_Query FAILED(0x%08lx)\n", (unsigned long)ret_val);
        return -1;
    }   
    if (( venc_chn_state.u32LeftStreamBytes ==0 )&&(venc_chn_state.u32CurPacks == 0))
    {
         //printf("chn:%d leftbytes:%d, packcnt:%d\n",pVei->venc_chn,venc_chn_state.u32LeftStreamBytes , (int)venc_chn_state.u32CurPacks);
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
#if defined(_AV_PLATFORM_HI3515_)

int CHiAvEncoder::Ae_create_osd_region(std::list<av_osd_item_t> *av_osd_list)
{
	if(PTD_6A_I == pgAvDeviceInfo->Adi_product_type())
	{
		return Ae_create_6a1_extend_osd_region(av_osd_list);
	}
	
    REGION_ATTR_S region_attr;
    memset(&region_attr, 0, sizeof(region_attr));
	
    int font_width = 0;
    int font_height = 0;
    int font_name = 0;
    int horizontal_scaler = 0;
    int vertical_scaler = 0;
    char product_group[32] = {0};
    int video_width = 0;
    int video_height = 0;
    
    pgAvDeviceInfo->Adi_get_product_group(product_group, sizeof(product_group));
    eProductType product_type = pgAvDeviceInfo->Adi_product_type();
    
    //! obtain osd info
    int osd_enable1 = 0; //!< bit0: osd0, bit1:osd1, bit2: common osd1, bit3: common osd2
    int osd_enbale2 = 0;

    int osd_created[16][4] = {0}; //!< if  each osd for chn[] is created 
    
    std::list<av_osd_item_t>::iterator av_osd_item_it;
    av_osd_item_t osd_item1;
    av_osd_item_t osd_item2;
    memset(&osd_item1, 0, sizeof(av_osd_item_t));
    memset(&osd_item2, 0, sizeof(av_osd_item_t));
    av_osd_item_it = av_osd_list->begin();
    while(av_osd_item_it != av_osd_list->end())
    { 
        unsigned int chn = (unsigned int)av_osd_item_it->video_stream.phy_video_chn_num;
        if(chn >16)
        {
            continue;
        }
        //DEBUG_MESSAGE("chn:%d,to create osd:%d, state:%d\n", chn, av_osd_item_it->osd_name , av_osd_item_it->overlay_state);
        if ((product_type == PTD_A5_II)&&((av_osd_item_it->osd_name == _AON_DATE_TIME_)||(av_osd_item_it->osd_name == _AON_CHN_NAME_)||(av_osd_item_it->osd_name == _AON_SPEED_INFO_)))
        {
            
            if(osd_created[chn][0])
            {
                av_osd_item_it->overlay_state = _AOS_CREATED_;
                
                av_osd_item_it->bitmap_width = osd_item1.bitmap_width;
                av_osd_item_it->bitmap_height = osd_item1.bitmap_height;
                av_osd_item_it->pBitmap = osd_item1.pBitmap;
                av_osd_item_it->display_x = osd_item1.display_x;
                av_osd_item_it->display_y = osd_item1.display_y;
                av_osd_item_it->pOsd_region = NULL;
                av_osd_item_it++;
                continue;
            }

            //printf("osd 1 has created! osd s:%d\n", av_osd_item_it->overlay_state);
            if(av_osd_item_it->overlay_state == _AOS_INIT_)
            {
                
                Ae_get_osd_parameter(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution, &font_name, 
                                                &font_width, &font_height, &horizontal_scaler, &vertical_scaler);

                region_attr.unAttr.stOverlay.stRect.u32Width  = 1024;
                region_attr.unAttr.stOverlay.stRect.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;

                pgAvDeviceInfo->Adi_get_video_size(av_osd_item_it->video_stream.video_encoder_para.resolution, av_osd_item_it->video_stream.video_encoder_para.tv_system, &video_width, &video_height);

                if(region_attr.unAttr.stOverlay.stRect.u32Width > (unsigned int)video_width)
                {
                    region_attr.unAttr.stOverlay.stRect.u32Width = video_width;
                }
                if(region_attr.unAttr.stOverlay.stRect.u32Height >  (unsigned int)video_height)
                {
                    region_attr.unAttr.stOverlay.stRect.u32Height = video_height;
                }
                /*bitmap*/
                av_osd_item_it->bitmap_width = region_attr.unAttr.stOverlay.stRect.u32Width;
                av_osd_item_it->bitmap_height = region_attr.unAttr.stOverlay.stRect.u32Height;
                av_osd_item_it->pBitmap = new unsigned char[av_osd_item_it->bitmap_width * horizontal_scaler * av_osd_item_it->bitmap_height * vertical_scaler * 2];
                _AV_FATAL_(av_osd_item_it->pBitmap == NULL, "OUT OF MEMORY\n");
                
                av_osd_item_it->display_x = 0;
                av_osd_item_it->display_y = 0;  
         
                Ae_translate_coordinate(av_osd_item_it, region_attr.unAttr.stOverlay.stRect.u32Width, region_attr.unAttr.stOverlay.stRect.u32Height);

                Ae_create_osd_item(av_osd_item_it, region_attr);   
                
                osd_item1.bitmap_width = av_osd_item_it->bitmap_width ;
                osd_item1.bitmap_height= av_osd_item_it->bitmap_height;
                osd_item1.pBitmap= av_osd_item_it->pBitmap;
                osd_item1.display_x= av_osd_item_it->display_x;
                osd_item1.display_y= av_osd_item_it->display_y;
                       
                //! set the state            
                av_osd_item_it->overlay_state = _AOS_CREATED_;
                osd_enable1= 1;
                osd_created[chn][0] = 1;
                av_osd_item_it++;
            }
            else
            {   
                osd_enable1= 1;
                osd_created[chn][0] = 1;
                
                osd_item1.bitmap_width = av_osd_item_it->bitmap_width ;
                osd_item1.bitmap_height= av_osd_item_it->bitmap_height;
                osd_item1.pBitmap= av_osd_item_it->pBitmap;
                osd_item1.display_x= av_osd_item_it->display_x;
                osd_item1.display_y= av_osd_item_it->display_y;
                
                av_osd_item_it++;
                continue;
            }
            
        }

		if ((product_type == PTD_A5_II)&&((av_osd_item_it->osd_name == _AON_GPS_INFO_)||(av_osd_item_it->osd_name == _AON_BUS_NUMBER_)||(av_osd_item_it->osd_name == _AON_SELFNUMBER_INFO_)))
        {
            
            if(osd_created[chn][1])
            {
                osd_created[chn][1] = 1;
                av_osd_item_it->overlay_state = _AOS_CREATED_;
                av_osd_item_it->bitmap_width = osd_item2.bitmap_width;
                av_osd_item_it->bitmap_height = osd_item2.bitmap_height;
                av_osd_item_it->pBitmap = osd_item2.pBitmap;
                av_osd_item_it->display_x = osd_item2.display_x;
                av_osd_item_it->display_y = osd_item2.display_y;
          
                av_osd_item_it++;
                continue;
            }
            if(av_osd_item_it->overlay_state == _AOS_INIT_)
            {
                Ae_get_osd_parameter(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution, &font_name, 
                                &font_width, &font_height, &horizontal_scaler, &vertical_scaler);
                
                region_attr.unAttr.stOverlay.stRect.u32Width  = 1024;
                region_attr.unAttr.stOverlay.stRect.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;

                pgAvDeviceInfo->Adi_get_video_size(av_osd_item_it->video_stream.video_encoder_para.resolution, av_osd_item_it->video_stream.video_encoder_para.tv_system, &video_width, &video_height);

                if(region_attr.unAttr.stOverlay.stRect.u32Width > (unsigned int)video_width)
                {
                    region_attr.unAttr.stOverlay.stRect.u32Width = video_width;
                }
                if(region_attr.unAttr.stOverlay.stRect.u32Height >  (unsigned int)video_height)
                {
                    region_attr.unAttr.stOverlay.stRect.u32Height = video_height;
                }
                /*bitmap*/
                av_osd_item_it->bitmap_width = region_attr.unAttr.stOverlay.stRect.u32Width;
                av_osd_item_it->bitmap_height = region_attr.unAttr.stOverlay.stRect.u32Height;
                av_osd_item_it->pBitmap = new unsigned char[av_osd_item_it->bitmap_width * horizontal_scaler * av_osd_item_it->bitmap_height * vertical_scaler * 2];
                _AV_FATAL_(av_osd_item_it->pBitmap == NULL, "OUT OF MEMORY\n");
                
                av_osd_item_it->display_x = 0;
                av_osd_item_it->display_y = video_height - region_attr.unAttr.stOverlay.stRect.u32Height; 
                
                Ae_translate_coordinate(av_osd_item_it, region_attr.unAttr.stOverlay.stRect.u32Width, region_attr.unAttr.stOverlay.stRect.u32Height);
                Ae_create_osd_item(av_osd_item_it, region_attr);  

                osd_item2.bitmap_width = av_osd_item_it->bitmap_width ;
                osd_item2.bitmap_height= av_osd_item_it->bitmap_height;
                osd_item2.pBitmap= av_osd_item_it->pBitmap;
                osd_item2.display_x= av_osd_item_it->display_x;
                osd_item2.display_y= av_osd_item_it->display_y;
                
                //! set the state  
                av_osd_item_it->overlay_state = _AOS_CREATED_;
                osd_enbale2 = 1;
                osd_created[chn][1] = 1;
                av_osd_item_it++;
            }
            else
            {   
                osd_created[chn][1] = 1;

                osd_item2.bitmap_width = av_osd_item_it->bitmap_width ;
                osd_item2.bitmap_height= av_osd_item_it->bitmap_height;
                osd_item2.pBitmap= av_osd_item_it->pBitmap;
                osd_item2.display_x= av_osd_item_it->display_x;
                osd_item2.display_y= av_osd_item_it->display_y;
                av_osd_item_it++;
                continue;
            }  
        }

		if ((product_type == PTD_A5_II)&&(av_osd_item_it->osd_name == _AON_COMMON_OSD1_))
        {
            if(av_osd_item_it->overlay_state == _AOS_INIT_)
            {
                Ae_get_osd_parameter(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution, &font_name, 
                                &font_width, &font_height, &horizontal_scaler, &vertical_scaler);
                region_attr.unAttr.stOverlay.stRect.u32Width  = 1024;
                region_attr.unAttr.stOverlay.stRect.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;

                pgAvDeviceInfo->Adi_get_video_size(av_osd_item_it->video_stream.video_encoder_para.resolution, av_osd_item_it->video_stream.video_encoder_para.tv_system, &video_width, &video_height);

                if(region_attr.unAttr.stOverlay.stRect.u32Width > (unsigned int)video_width)
                {
                    region_attr.unAttr.stOverlay.stRect.u32Width = video_width;
                }
                if(region_attr.unAttr.stOverlay.stRect.u32Height >  (unsigned int)video_height)
                {
                    region_attr.unAttr.stOverlay.stRect.u32Height = video_height;
                }
                /*bitmap*/
                av_osd_item_it->bitmap_width = region_attr.unAttr.stOverlay.stRect.u32Width;
                av_osd_item_it->bitmap_height = region_attr.unAttr.stOverlay.stRect.u32Height;
                av_osd_item_it->pBitmap = new unsigned char[av_osd_item_it->bitmap_width * horizontal_scaler * av_osd_item_it->bitmap_height * vertical_scaler * 2];
                _AV_FATAL_(av_osd_item_it->pBitmap == NULL, "OUT OF MEMORY\n");
                
                Ae_translate_coordinate(av_osd_item_it, region_attr.unAttr.stOverlay.stRect.u32Width, region_attr.unAttr.stOverlay.stRect.u32Height);
                Ae_create_osd_item(av_osd_item_it, region_attr);   
                
                //! set the state  
                av_osd_item_it->overlay_state = _AOS_CREATED_;
                av_osd_item_it++;
            }
            else
            {   
                av_osd_item_it++;
                continue;
            }
   
        }

		if ((product_type == PTD_A5_II)&&(av_osd_item_it->osd_name == _AON_COMMON_OSD2_))
        {
            if(av_osd_item_it->overlay_state == _AOS_INIT_)
            {
                Ae_get_osd_parameter(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution, &font_name, 
                                                &font_width, &font_height, &horizontal_scaler, &vertical_scaler);

                region_attr.unAttr.stOverlay.stRect.u32Width  = 1024;
                region_attr.unAttr.stOverlay.stRect.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;

                pgAvDeviceInfo->Adi_get_video_size(av_osd_item_it->video_stream.video_encoder_para.resolution, av_osd_item_it->video_stream.video_encoder_para.tv_system, &video_width, &video_height);

                if(region_attr.unAttr.stOverlay.stRect.u32Width > (unsigned int)video_width)
                {
                    region_attr.unAttr.stOverlay.stRect.u32Width = video_width;
                }
                if(region_attr.unAttr.stOverlay.stRect.u32Height >  (unsigned int)video_height)
                {
                    region_attr.unAttr.stOverlay.stRect.u32Height = video_height;
                }
                /*bitmap*/
                av_osd_item_it->bitmap_width = region_attr.unAttr.stOverlay.stRect.u32Width;
                av_osd_item_it->bitmap_height = region_attr.unAttr.stOverlay.stRect.u32Height;
                av_osd_item_it->pBitmap = new unsigned char[av_osd_item_it->bitmap_width * horizontal_scaler * av_osd_item_it->bitmap_height * vertical_scaler * 2];
                _AV_FATAL_(av_osd_item_it->pBitmap == NULL, "OUT OF MEMORY\n");
                
                Ae_translate_coordinate(av_osd_item_it, region_attr.unAttr.stOverlay.stRect.u32Width, region_attr.unAttr.stOverlay.stRect.u32Height);
                Ae_create_osd_item(av_osd_item_it, region_attr);   
                
                //! set the state  
                av_osd_item_it->overlay_state = _AOS_CREATED_;

                av_osd_item_it++;
            }
            else
            {   
                av_osd_item_it++;
                continue;
            } 
        }
  
    } 

    //! for each combined osd, its param should be copied from the created one
    
    return 0;
}

int CHiAvEncoder::Ae_create_osd_item(std::list<av_osd_item_t>::iterator & av_osd_item_it, REGION_ATTR_S region_attr0)
{
    hi_osd_region_t *pOsd_region = NULL;     
    hi_video_encoder_info_t *pVideo_encoder_info = (hi_video_encoder_info_t *)av_osd_item_it->video_stream.video_encoder_id.pVideo_encoder_info;
    REGION_ATTR_S region_attr;
    REGION_HANDLE region_handle;
    
    region_attr.unAttr.stOverlay.stRect.u32Height = region_attr0.unAttr.stOverlay.stRect.u32Height;
    region_attr.unAttr.stOverlay.stRect.u32Width = region_attr0.unAttr.stOverlay.stRect.u32Width;

    /*create region*/
    region_attr.enType = OVERLAY_REGION;
    region_attr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;

 //chn related

	region_attr.unAttr.stOverlay.stRect.s32X= av_osd_item_it->display_x ;
	region_attr.unAttr.stOverlay.stRect.s32Y= av_osd_item_it->display_y;

	region_attr.unAttr.stOverlay.u32BgAlpha = 0;//20;//0:背景完全透明
	region_attr.unAttr.stOverlay.u32FgAlpha = 128;//128:完全不透明
	region_attr.unAttr.stOverlay.u32BgColor = 0x00;

    region_attr.unAttr.stOverlay.VeGroup = pVideo_encoder_info->venc_grp;

    region_attr.unAttr.stOverlay.bPublic = HI_FALSE;


    if(_AON_WATER_MARK == av_osd_item_it->osd_name)
    {
        region_attr.unAttr.stOverlay.u32FgAlpha = 32;
    }
    //--------------------------------------------------------------------        
	int retval = -1;	
	retval = HI_MPI_VPP_CreateRegion(&region_attr, &region_handle); // handle is output
	
	if(retval != SUCCESS)
	{
		DEBUG_ERROR("osd:%d, x:%d, y:%d, X:%d, Y:%d, W:%d, H:%d, C:%d, G:%d \n",av_osd_item_it->osd_name, av_osd_item_it->display_x,av_osd_item_it->display_y, region_attr.unAttr.stOverlay.stRect.s32X, region_attr.unAttr.stOverlay.stRect.s32Y, 
		 region_attr.unAttr.stOverlay.stRect.u32Width, region_attr.unAttr.stOverlay.stRect.u32Height, region_attr.unAttr.stOverlay.u32BgColor, 
		 region_attr.unAttr.stOverlay.VeGroup);
		DEBUG_ERROR("HI_MPI_VPP_CreateRegion err 0x%x!\n",retval);
        if (0xa0078003 == (HI_U32)retval)
        {
            return 0;
        }
        else
        {
            DEBUG_ERROR("osd:%d, x:%d, y:%d, X:%d, Y:%d, W:%d, H:%d, C:%d, G:%d \n",av_osd_item_it->osd_name, av_osd_item_it->display_x,av_osd_item_it->display_y, region_attr.unAttr.stOverlay.stRect.s32X, region_attr.unAttr.stOverlay.stRect.s32Y, 
            region_attr.unAttr.stOverlay.stRect.u32Width, region_attr.unAttr.stOverlay.stRect.u32Height, region_attr.unAttr.stOverlay.u32BgColor, 
            region_attr.unAttr.stOverlay.VeGroup);
        }
		return -1;
	}

    /*region*/
    _AV_FATAL_((pOsd_region = new hi_osd_region_t) == NULL, "OUT OF MEMORY\n");
    pOsd_region->region_handle = region_handle;
    av_osd_item_it->pOsd_region = (void *)pOsd_region;
    GCL_BIT_VAL_SET(m_stuOsdProperty[av_osd_item_it->video_stream.phy_video_chn_num][Ae_6a1_translate_osd_region_id((OSD_TYPE)av_osd_item_it->extend_osd_id)].subMask, av_osd_item_it->extend_osd_id);

//------------------------- control region-----------------------------------------
    REGION_CRTL_CODE_E enCtrl;
	enCtrl = REGION_SHOW;/*show region*/
	REGION_CTRL_PARAM_U unParam;

	retval = HI_MPI_VPP_ControlRegion(region_handle, enCtrl, &unParam);
	if(retval != SUCCESS)
	{
		DEBUG_ERROR("show region %d faild 0x%x at 2!\n",av_osd_item_it->osd_name,retval);
		return -1;
	}
    return 0;
}


int CHiAvEncoder::Ae_osd_overlay(std::list<av_osd_item_t> &av_osd_list)
{
   	if(PTD_6A_I == pgAvDeviceInfo->Adi_product_type())
   	{
   		return Ae_6a1_extend_osd_overlay(av_osd_list);
   	}
   	
    char product_group[32] = {0};
    pgAvDeviceInfo->Adi_get_product_group(product_group, sizeof(product_group));

    char *pUtf8_string = NULL;

    char osd1[8][128] = {0};
    char osd2[8][128] = {0};  //!< gps bus number selfnumber 
  
    char date_time_string[8][64] = {0};
    char chn_string[8][32] = {0};
    char speedstring[8][16] = {0};
    
    char string[32];
    
    char gpsstring[8][64] = {0};
    char bus_number_string[8][32] = {0};
    char selfnumber_string[8][32] = {0};

    std::list<av_osd_item_t>::iterator av_osd_item_it;
    av_osd_item_it = av_osd_list.begin();

    while(av_osd_item_it != av_osd_list.end())
    {   
        unsigned int chn = (unsigned int)av_osd_item_it->video_stream.phy_video_chn_num;
        if(chn > 8)
        {
            continue;
        }
        if(av_osd_item_it->osd_name == _AON_DATE_TIME_)
        {        
            datetime_t date_time;
    
            pgAvDeviceInfo->Adi_get_date_time(&date_time);
            switch(av_osd_item_it->video_stream.video_encoder_para.date_mode)
            {
                case _AV_DATE_MODE_yymmdd_:
                default:
                sprintf(date_time_string[chn], "20%02d-%02d-%02d", date_time.year, date_time.month, date_time.day);
                break;
                case _AV_DATE_MODE_ddmmyy_:
                sprintf(date_time_string[chn], "%02d/%02d/20%02d", date_time.day, date_time.month, date_time.year);
                break;
                case _AV_DATE_MODE_mmddyy_:
                sprintf(date_time_string[chn], "%02d/%02d/20%02d", date_time.month, date_time.day, date_time.year);
                break;
            }

            switch(av_osd_item_it->video_stream.video_encoder_para.time_mode)
            {
                case _AV_TIME_MODE_24_:
                default:
                    sprintf(date_time_string[chn], "%s %02d:%02d:%02d", date_time_string[chn], date_time.hour, date_time.minute, date_time.second);
                    break;
                case _AV_TIME_MODE_12_:
                    if (date_time.hour > 12)
                    {
                        sprintf(date_time_string[chn], "%s %02d:%02d:%02d PM", date_time_string[chn], date_time.hour - 12, date_time.minute, date_time.second);
                    }
                    else if(date_time.hour == 12)
                    {
                        sprintf(date_time_string[chn], "%s %02d:%02d:%02d PM", date_time_string[chn], date_time.hour, date_time.minute, date_time.second);
                    }
                    else if(date_time.hour == 0)
                    {
                        sprintf(date_time_string[chn], "%s 12:%02d:%02d AM", date_time_string[chn],  date_time.minute, date_time.second);
                    }
                    else
                    {
                        sprintf(date_time_string[chn], "%s %02d:%02d:%02d AM", date_time_string[chn], date_time.hour, date_time.minute, date_time.second);                  
                    }                
                    break;
            }
                         
            //pUtf8_string = date_time_string;        
            }
            else if(av_osd_item_it->osd_name == _AON_CHN_NAME_)
            {
                memcpy(chn_string[chn], av_osd_item_it->video_stream.video_encoder_para.channel_name, sizeof(chn_string[chn]));
                //pUtf8_string = av_osd_item_it->video_stream.video_encoder_para.channel_name;   
            }
            else if(av_osd_item_it->osd_name == _AON_BUS_NUMBER_)
            {
                memcpy(bus_number_string[chn], av_osd_item_it->video_stream.video_encoder_para.bus_number, sizeof(bus_number_string[chn]));
                //pUtf8_string = av_osd_item_it->video_stream.video_encoder_para.bus_number;    
            }
            else if(av_osd_item_it->osd_name == _AON_SELFNUMBER_INFO_)
            {
                memcpy(selfnumber_string[chn], av_osd_item_it->video_stream.video_encoder_para.bus_selfnumber, sizeof(selfnumber_string[chn]));
                //pUtf8_string = av_osd_item_it->video_stream.video_encoder_para.bus_selfnumber;    
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
                strncpy(speedstring[chn], string, strlen(string));
                speedstring[chn][strlen(string)] = '\0';

#endif
            }
            else if(av_osd_item_it->osd_name == _AON_GPS_INFO_)
            {
        
                msgGpsInfo_t gpsinfo;
                memset(&gpsinfo,0,sizeof(gpsinfo));
                pgAvDeviceInfo->Adi_Get_Gps_Info(&gpsinfo);
                if(gpsinfo.ucGpsStatus == 0)
                {
                    char temp[48] = {0};
                    sprintf(temp,"%d.%d.%04u%c %d.%d.%04u%c",gpsinfo.ucLatitudeDegree,gpsinfo.ucLatitudeCent,gpsinfo.ulLatitudeSec,gpsinfo.ucDirectionLatitude ? 'S':'N',gpsinfo.ucLongitudeDegree,gpsinfo.ucLongitudeCent,gpsinfo.ulLongitudeSec,gpsinfo.ucDirectionLongitude ? 'W':'E');
                    //strcat(gpsstring,temp);
                    strncpy(gpsstring[chn], temp, strlen(temp));
                    gpsstring[chn][strlen(temp)] = '\0';
                }
                else if(gpsinfo.ucGpsStatus == 1)
                {
                    //strcat(gpsstring,GuiTKGetText(NULL, NULL, "STR_GPS INVALID"));
                    strncpy(gpsstring[chn], GuiTKGetText(NULL, NULL, "STR_GPS INVALID"), strlen(GuiTKGetText(NULL, NULL, "STR_GPS INVALID")));
                    gpsstring[chn][strlen(GuiTKGetText(NULL, NULL, "STR_GPS INVALID"))] = '\0';
                    
                }
                else
                {
                    //strcat(gpsstring,GuiTKGetText(NULL, NULL, "STR_GPS NOT_EXIST"));
                    strncpy(gpsstring[chn], GuiTKGetText(NULL, NULL, "STR_GPS NOT_EXIST"), strlen(GuiTKGetText(NULL, NULL, "STR_GPS NOT_EXIST")));
                    gpsstring[chn][strlen(GuiTKGetText(NULL, NULL, "STR_GPS NOT_EXIST"))] = '\0';
                }
                //pUtf8_string = gpsstring;
        
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


        av_osd_item_it++;
    }

   // memset(bus_number_string, 0, sizeof(bus_number_string));
    for (int i=0; i<8; i++)
    {
        /*
        printf("data time:%s\n", date_time_string[i]);
        memcpy(osd1[i],chn_string[i], sizeof(chn_string[i])); 
        memcpy(&osd1[i][32],date_time_string[i], 64); 
        memcpy(osd1[i]+96,speedstring[i], 32);
        printf("osd1:%s\n", osd1[i]);
        memcpy(osd2[i],gpsstring[i], sizeof(gpsstring[i])); 
        memcpy(osd2[i]+sizeof(gpsstring[i]),bus_number_string[i], sizeof(bus_number_string[i])); 
        memcpy(osd2[i]+96,selfnumber_string[i], sizeof(selfnumber_string[i])); 
        */
        sprintf(osd1[i], "%s    %s    %s", chn_string[i],date_time_string[i], speedstring[i]);
        sprintf(osd2[i], "%s     %s    %s",gpsstring[i],bus_number_string[i], selfnumber_string[i]);
    }

    int overlayed1[16] = {0};
    int overlayed2[16] = {0};
    av_osd_item_it = av_osd_list.begin();   
    while(av_osd_item_it != av_osd_list.end())
    {

        unsigned int chn = (unsigned int)av_osd_item_it->video_stream.phy_video_chn_num;
        if(chn >16)
        {
            continue;
        }

        if ((av_osd_item_it->osd_name == _AON_DATE_TIME_)\
            ||(av_osd_item_it->osd_name == _AON_CHN_NAME_)||\
            (av_osd_item_it->osd_name == _AON_SPEED_INFO_))
        {
            if((av_osd_item_it->overlay_state == _AOS_INIT_)||(overlayed1[chn]))
            {

                av_osd_item_it++;
                av_osd_item_it->overlay_state = _AOS_OVERLAYED_;
                continue;
            }
            else 
            {
                Ae_set_osd(av_osd_item_it,osd1[chn]);
                overlayed1[chn] = 1;
                av_osd_item_it->overlay_state = _AOS_OVERLAYED_;
            }

        }

        if ((av_osd_item_it->osd_name == _AON_GPS_INFO_)\
            ||(av_osd_item_it->osd_name == _AON_BUS_NUMBER_)\
            ||(av_osd_item_it->osd_name == _AON_SELFNUMBER_INFO_))
        {
            if((av_osd_item_it->overlay_state == _AOS_INIT_)||(overlayed2[chn]))
            {
                av_osd_item_it++;
                av_osd_item_it->overlay_state = _AOS_OVERLAYED_;
                continue;
            }
            else
            {
                Ae_set_osd(av_osd_item_it,osd2[chn]);
                overlayed2[chn] = 1;
                av_osd_item_it->overlay_state = _AOS_OVERLAYED_;
            }

        }

        if (av_osd_item_it->osd_name == _AON_COMMON_OSD1_)
        {
            if(av_osd_item_it->overlay_state == _AOS_INIT_)
            {
                av_osd_item_it++;
                continue;
            }
            else
            {
                Ae_set_osd(av_osd_item_it,av_osd_item_it->video_stream.video_encoder_para.osd1_content);
                av_osd_item_it->overlay_state = _AOS_OVERLAYED_;
            }

        }

        if (av_osd_item_it->osd_name == _AON_COMMON_OSD2_)
        {
            if(av_osd_item_it->overlay_state == _AOS_INIT_)
            {
                av_osd_item_it++;
                continue;
            }
            else
            {
                //Ae_set_osd(av_osd_item_it,av_osd_item_it->video_stream.video_encoder_para.osd2_content);
                av_osd_item_it->overlay_state = _AOS_OVERLAYED_;
            }

        }

        av_osd_item_it++;
    }

    av_osd_item_it = av_osd_list.begin();   
    while(av_osd_item_it != av_osd_list.end())
    {
        //printf("ffffffffffosd name: %d, wid:%d, bmp:0x%x\n", av_osd_item_it->osd_name, av_osd_item_it->bitmap_width, (int)av_osd_item_it->pBitmap);
        av_osd_item_it++;
    }

    return 0;
}

int CHiAvEncoder::Ae_set_osd(std::list<av_osd_item_t>::iterator av_osd_item_it, char * pUtf8_string)
{
	int ret_val = 0;
    hi_osd_region_t *pOsd_region = (hi_osd_region_t *)av_osd_item_it->pOsd_region;
	if(NULL == pOsd_region || NULL == pUtf8_string)
	{
		DEBUG_ERROR("Ae_set_osd failed\n");
		return -1;
	}
	bool bCacheString = false;
	unsigned short text_color = 0xffff;
	eProductType productType = pgAvDeviceInfo->Adi_product_type();
	
	unsigned char osdItem = 0xff;
	unsigned char res = _AR_UNKNOWN_;
	unsigned char max_num = 0;
	unsigned char itemId = Ae_6a1_translate_osd_region_id((OSD_TYPE)av_osd_item_it->extend_osd_id);
	if(PTD_A5_II == productType)
	{
		if(av_osd_item_it->osd_name == _AON_DATE_TIME_ || av_osd_item_it->osd_name == _AON_GPS_INFO_ || av_osd_item_it->osd_name == _AON_SPEED_INFO_)
		{
			osdItem = m_OsdNameToIdMap[av_osd_item_it->osd_name];
			res = av_osd_item_it->video_stream.video_encoder_para.resolution;
			max_num = 3;
		}
	}
	else if(PTD_6A_I == productType)
	{
		if(itemId == 0 || itemId == 2)
		{
			osdItem = m_OsdNameToIdMap[itemId];
			res = av_osd_item_it->video_stream.video_encoder_para.resolution;
			max_num = 2;
		}
		
		OsdItem_t* pOsdItem = &(av_osd_item_it->video_stream.video_encoder_para.stuExtendOsdInfo[av_osd_item_it->extend_osd_id]);
		//颜色处理///
		if(pOsdItem->ucColor == 0)
		{
			text_color = 0xffff;
		}
		else if(pOsdItem->ucColor == 1)
		{
			text_color = 0x8000;
		}
		else if(pOsdItem->ucColor == 2)
		{
			text_color = 0xfc00;	//****如果修改此值，必须将Ae_utf8string_2_bmp中的红色值一起修改///
		}
	}
	else
	{
		res = _AR_UNKNOWN_;
	}
	
	if((osdItem < max_num) && (res == _AR_D1_ || res == _AR_HD1_ || res == _AR_CIF_))
	{
		res -= 1;	//血淋淋的教训，d1从1 开始////
		if(strcmp(m_SameOsdCache[osdItem][res].szTitle, pUtf8_string) == 0)
		{
			if(m_SameOsdCache[osdItem][res].szData != NULL)
			{
				REGION_CRTL_CODE_E enCtrl = REGION_SET_BITMAP;
				
				REGION_CTRL_PARAM_U unParam;
				memset(&unParam, 0, sizeof(unParam));
				
				unParam.stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
				unParam.stBitmap.u32Height = av_osd_item_it->bitmap_height; 
				unParam.stBitmap.u32Width = av_osd_item_it->bitmap_width;
				memcpy(av_osd_item_it->pBitmap, m_SameOsdCache[osdItem][res].szData, m_SameOsdCache[osdItem][res].nDataLen);
				unParam.stBitmap.pData = av_osd_item_it->pBitmap;
				if ((ret_val = HI_MPI_VPP_ControlRegion(pOsd_region->region_handle, enCtrl, &unParam)) != HI_SUCCESS)
				{
					DEBUG_ERROR( "CHiAvEncoder::Ae_osd_overlay HI_MPI_VPP_ControlRegion FAILED(region_handle:)(%d)\n", pOsd_region->region_handle);
					return -1; 
				}
				return 0;
			}
			else
			{
				bCacheString = true;
			}
		}
		else
		{
			if(m_SameOsdCache[osdItem][res].szData != NULL)
			{
				delete[] m_SameOsdCache[osdItem][res].szData;
				m_SameOsdCache[osdItem][res].szData = NULL;
			}
			memset(m_SameOsdCache[osdItem][res].szTitle, 0x0, MAX_EXTEND_OSD_LENGTH);
			m_SameOsdCache[osdItem][res].nDataLen = 0;
			bCacheString = true;
		}
	}
	/*char buffer11[6] = {0};
	text_color = 0xfc00;
	
	buffer11[0] = 0xE2;
	buffer11[1] = 0x97;
	buffer11[2] = 0x8F;
	sprintf(buffer11 +3, "REC");

	pUtf8_string = buffer11;*/

    DEBUG_MESSAGE("osd name: %d, wid:%d, bmp:0x%x, osd1:%s, %d\n", av_osd_item_it->osd_name, av_osd_item_it->bitmap_width, (int)av_osd_item_it->pBitmap, pUtf8_string, strlen(pUtf8_string));
    if(Ae_utf8string_2_bmp(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution,
        av_osd_item_it->pBitmap, av_osd_item_it->bitmap_width, av_osd_item_it->bitmap_height, pUtf8_string, text_color, 0x00, 0x8000) != 0)
    {
        DEBUG_ERROR("erooroeoooofooe osd_name =%d\n", av_osd_item_it->osd_name);
        return -1;
    }

	if(PTD_6A_I == productType || PTD_A5_II == productType)
	{
	    if(bCacheString)
	    {
			int font_width = 0;
			int font_height = 0;
			int font_name = 0;
			int horizontal_scaler = 0;
			int vertical_scaler = 0;
	        Ae_get_osd_parameter(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution, &font_name, &font_width, &font_height, &horizontal_scaler, &vertical_scaler, osdItem);   
			//unsigned char osdItem = m_OsdNameToIdMap[av_osd_item_it->osd_name];
			//unsigned char res = av_osd_item_it->video_stream.video_encoder_para.resolution - 1;
	    	int strLen = 2 * av_osd_item_it->bitmap_width * av_osd_item_it->bitmap_height * horizontal_scaler * vertical_scaler;
	    	m_SameOsdCache[osdItem][res].szData = new char[strLen];
	    	if(m_SameOsdCache[osdItem][res].szData != NULL)
	    	{
				memcpy(m_SameOsdCache[osdItem][res].szData, av_osd_item_it->pBitmap, strLen);
				strcpy(m_SameOsdCache[osdItem][res].szTitle, pUtf8_string);
				m_SameOsdCache[osdItem][res].nDataLen = strLen;
	    	}
	    	else
	    	{
	    		DEBUG_ERROR("Ae_set_osd: malloc failed\n\n");
	    	}
	    }
    }

    // show region
    REGION_CRTL_CODE_E enCtrl = REGION_SET_BITMAP;
    
    REGION_CTRL_PARAM_U unParam;
    memset(&unParam, 0, sizeof(unParam));

    unParam.stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
    unParam.stBitmap.u32Height = av_osd_item_it->bitmap_height; 
    unParam.stBitmap.u32Width = av_osd_item_it->bitmap_width;
    unParam.stBitmap.pData = av_osd_item_it->pBitmap;

    if ((ret_val = HI_MPI_VPP_ControlRegion(pOsd_region->region_handle, enCtrl, &unParam)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_osd_overlay HI_MPI_VPP_ControlRegion FAILED(region_handle:0x%08x)(%d)\n", ret_val, pOsd_region->region_handle);
        return -1; 
    }

    return 0;
}

int CHiAvEncoder::Ae_destroy_osd_region(std::list<av_osd_item_t>::iterator &av_osd_item_it)
{
	if(PTD_6A_I == pgAvDeviceInfo->Adi_product_type())
	{
		int chn = av_osd_item_it->video_stream.phy_video_chn_num;
		int region = Ae_6a1_translate_osd_region_id((OSD_TYPE)av_osd_item_it->extend_osd_id);
		GCL_BIT_VAL_CLEAR(m_stuOsdProperty[chn][region].subMask, av_osd_item_it->extend_osd_id);
 		if(m_stuOsdProperty[chn][region].subMask != 0x0)	//此区域还有子项在使用，则保留
		{
 			av_osd_item_it->pBitmap = NULL;
			av_osd_item_it->pOsd_region = NULL;
			av_osd_item_it->overlay_state = _AOS_UNKNOWN_;
			return 0;
		}
		m_stuOsdProperty[chn][region].osd_item.pBitmap = NULL;
		m_stuOsdProperty[chn][region].osd_item.pOsd_region = NULL;
		m_stuOsdProperty[chn][region].osd_item.overlay_state = _AOS_UNKNOWN_;
	}
    if(av_osd_item_it->overlay_state == _AOS_INIT_)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_destroy_osd_region (av_osd_item_it->overlay_state == _AOS_INIT_)\n");
        return 0;
    }
    hi_osd_region_t *pOsd_region = (hi_osd_region_t *)av_osd_item_it->pOsd_region;

    if(NULL == pOsd_region)
    {
        DEBUG_WARNING("pOsd_region is null \n");
        return 0;
    }
    HI_S32 ret_val = -1;
    
    if((ret_val = HI_MPI_VPP_DestroyRegion(pOsd_region->region_handle)) != HI_SUCCESS)
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

int CHiAvEncoder::Ae_create_6a1_extend_osd_region(std::list<av_osd_item_t> *av_osd_list)
{
    REGION_ATTR_S region_attr;
    memset(&region_attr, 0, sizeof(region_attr));
	
    int font_width = 0;
    int font_height = 0;
    int font_name = 0;
    int horizontal_scaler = 0;
    int vertical_scaler = 0;
    int video_width = 0;
    int video_height = 0;
    
    std::list<av_osd_item_t>::iterator av_osd_item_it = av_osd_list->begin();
    while(av_osd_item_it != av_osd_list->end())
    { 
        unsigned int chn = (unsigned int)av_osd_item_it->video_stream.phy_video_chn_num;
        if(chn > 8)
        {
            continue;
        }
		int region = Ae_6a1_translate_osd_region_id((OSD_TYPE)av_osd_item_it->extend_osd_id);		

		if(m_stuOsdProperty[chn][region].subMask != 0)	//此区域仍存在
		{
			GCL_BIT_VAL_SET(m_stuOsdProperty[chn][region].subMask, av_osd_item_it->extend_osd_id);
			
			av_osd_item_it->overlay_state = m_stuOsdProperty[chn][region].osd_item.overlay_state;
			av_osd_item_it->bitmap_width = m_stuOsdProperty[chn][region].osd_item.bitmap_width;
			av_osd_item_it->bitmap_height = m_stuOsdProperty[chn][region].osd_item.bitmap_height;
			av_osd_item_it->pBitmap = m_stuOsdProperty[chn][region].osd_item.pBitmap;
			av_osd_item_it->display_x = m_stuOsdProperty[chn][region].osd_item.display_x;
			av_osd_item_it->display_y = m_stuOsdProperty[chn][region].osd_item.display_y;
			av_osd_item_it->pOsd_region = m_stuOsdProperty[chn][region].osd_item.pOsd_region;
			//DEBUG_ERROR("Ae_create_6a1_extend_osd_region: Osd region has created [CREATE], set osd, chn = %d, region = %d\n", chn, region);
		}
		else
		{
            if(av_osd_item_it->overlay_state == _AOS_INIT_)
            {
				Ae_get_osd_parameter(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution, &font_name, 
												&font_width, &font_height, &horizontal_scaler, &vertical_scaler, region);
				
				region_attr.unAttr.stOverlay.stRect.u32Width  = 1024;
				region_attr.unAttr.stOverlay.stRect.u32Height = (font_height * vertical_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
				
				pgAvDeviceInfo->Adi_get_video_size(av_osd_item_it->video_stream.video_encoder_para.resolution, av_osd_item_it->video_stream.video_encoder_para.tv_system, &video_width, &video_height);
				
				if(region_attr.unAttr.stOverlay.stRect.u32Width > (unsigned int)video_width)
				{
					region_attr.unAttr.stOverlay.stRect.u32Width = video_width;
				}
				if(region_attr.unAttr.stOverlay.stRect.u32Height >	(unsigned int)video_height)
				{
					region_attr.unAttr.stOverlay.stRect.u32Height = video_height;
				}
				
				av_osd_item_it->display_x = 0;
				av_osd_item_it->display_y = 0;
				if(av_osd_item_it->extend_osd_id == OSD_DATEANDTIME || av_osd_item_it->extend_osd_id == OSD_SPEEDANDMIL)
				{
					av_osd_item_it->display_x = av_osd_item_it->video_stream.video_encoder_para.stuExtendOsdInfo[OSD_DATEANDTIME].usOsdX;
					//av_osd_item_it->display_y = av_osd_item_it->video_stream.video_encoder_para.stuExtendOsdInfo[OSD_DATEANDTIME].usOsdY;
				}
				else if(av_osd_item_it->extend_osd_id == OSD_RECSTATUS)
				{
					av_osd_item_it->display_x = av_osd_item_it->video_stream.video_encoder_para.stuExtendOsdInfo[OSD_RECSTATUS].usOsdX;
					//av_osd_item_it->display_y = av_osd_item_it->video_stream.video_encoder_para.stuExtendOsdInfo[OSD_RECSTATUS].usOsdY;
				}
				else if(av_osd_item_it->extend_osd_id == OSD_USBSTATUS || av_osd_item_it->extend_osd_id == OSD_FIREMONITOR)
				{
					av_osd_item_it->display_x = av_osd_item_it->video_stream.video_encoder_para.stuExtendOsdInfo[OSD_USBSTATUS].usOsdX;
					//av_osd_item_it->display_y = av_osd_item_it->video_stream.video_encoder_para.stuExtendOsdInfo[OSD_USBSTATUS].usOsdY;
				}
				else if(av_osd_item_it->extend_osd_id == OSD_TRAINANDNUM || av_osd_item_it->extend_osd_id == OSD_CHNNAME)
				{
					av_osd_item_it->display_x = av_osd_item_it->video_stream.video_encoder_para.stuExtendOsdInfo[OSD_TRAINANDNUM].usOsdX;
					//av_osd_item_it->display_y = av_osd_item_it->video_stream.video_encoder_para.stuExtendOsdInfo[OSD_TRAINANDNUM].usOsdY;
				}
				av_osd_item_it->display_y = av_osd_item_it->video_stream.video_encoder_para.stuExtendOsdInfo[av_osd_item_it->extend_osd_id].usOsdY;
				
				/* 根据不同分辨率坐标转换 */
				pgAvDeviceInfo->Adi_covert_coordinate(av_osd_item_it->video_stream.video_encoder_para.resolution
						, av_osd_item_it->video_stream.video_encoder_para.tv_system
						, &av_osd_item_it->display_x, &av_osd_item_it->display_y, 1);
				
				pgAvDeviceInfo->Adi_get_video_size(av_osd_item_it->video_stream.video_encoder_para.resolution, av_osd_item_it->video_stream.video_encoder_para.tv_system, &video_width, &video_height);
				
				av_osd_item_it->display_x = (av_osd_item_it->display_x	+ _AE_OSD_ALIGNMENT_BYTE_ - 1)/ _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
				av_osd_item_it->display_y = (av_osd_item_it->display_y	+ 1)/ 2 * 2;
				
				/*bitmap*/
				av_osd_item_it->bitmap_width = region_attr.unAttr.stOverlay.stRect.u32Width;
				av_osd_item_it->bitmap_height = region_attr.unAttr.stOverlay.stRect.u32Height;
				unsigned int map_size = av_osd_item_it->bitmap_width * horizontal_scaler * av_osd_item_it->bitmap_height * vertical_scaler * 2;
				av_osd_item_it->pBitmap = new unsigned char[map_size];
				_AV_FATAL_(av_osd_item_it->pBitmap == NULL, "OUT OF MEMORY\n");
				memset(av_osd_item_it->pBitmap, 0x0, map_size);
				
				Ae_create_osd_item(av_osd_item_it, region_attr);
				//DEBUG_ERROR("Ae_create_6a1_extend_osd_region: Osd region create [INIT], set osd, chn = %d, region = %d, state = %d\n", chn, region, osd_created[region][chn]);
            }
			
			av_osd_item_it->overlay_state = _AOS_CREATED_;

            m_stuOsdProperty[chn][region].osd_item.bitmap_width = av_osd_item_it->bitmap_width ;
            m_stuOsdProperty[chn][region].osd_item.bitmap_height= av_osd_item_it->bitmap_height;
            m_stuOsdProperty[chn][region].osd_item.pBitmap= av_osd_item_it->pBitmap;
            m_stuOsdProperty[chn][region].osd_item.display_x= av_osd_item_it->display_x;
            m_stuOsdProperty[chn][region].osd_item.display_y= av_osd_item_it->display_y;
            m_stuOsdProperty[chn][region].osd_item.pOsd_region = av_osd_item_it->pOsd_region;
            m_stuOsdProperty[chn][region].osd_item.overlay_state = av_osd_item_it->overlay_state;
		}
		av_osd_item_it++;
    } 
    return 0;
}

int CHiAvEncoder::Ae_6a1_extend_osd_overlay(std::list<av_osd_item_t> &av_osd_list)
{
    int font_width = 0;
    int font_height = 0;
    int font_name = 0;
    int horizontal_scaler = 0;
    int vertical_scaler = 0;
    int video_width[8] = {0};
    int video_height[8] = {0};
    
    char update[4][8] = {0};    //记录每个区域是否有更新///
    
    std::list<av_osd_item_t>::iterator av_osd_item_it;
    av_osd_item_it = av_osd_list.begin();
    while(av_osd_item_it != av_osd_list.end())
    {
		unsigned int chn = (unsigned int)av_osd_item_it->video_stream.phy_video_chn_num;
		rm_uint8_t osdId = av_osd_item_it->extend_osd_id;
		if(osdId == OSD_DATEANDTIME)
		{
			av_osd_item_it->osdChange = 1;
		}
		update[Ae_6a1_translate_osd_region_id((OSD_TYPE)osdId)][chn] |= av_osd_item_it->osdChange;
		
		av_osd_item_it->osdChange = 0;
		av_osd_item_it ++;
    }

	char regionosd[4][8][MAX_EXTEND_OSD_LENGTH * 2] = {0};
	char regionstr[8][8][MAX_EXTEND_OSD_LENGTH] = {0};
	int osdStringLen[9][8] = {0};	//每个串的长度，用于合并字符串使用，最后一个是空格的长度///
	
	datetime_t date_time;
	pgAvDeviceInfo->Adi_get_date_time(&date_time);
    av_osd_item_it = av_osd_list.begin();
	while(av_osd_item_it != av_osd_list.end())
	{	
		unsigned int chn = (unsigned int)av_osd_item_it->video_stream.phy_video_chn_num;
		if(chn > 8)
		{
			av_osd_item_it ++;
			continue;
		}
		
		rm_uint8_t osdId = av_osd_item_it->extend_osd_id;
		if(update[Ae_6a1_translate_osd_region_id((OSD_TYPE)osdId)][chn] != 1)	//如果没变化，就跳过//
		{
			av_osd_item_it ++;
			continue;
		}
		
		//计算每个条目的长度///
		Ae_get_osd_parameter(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution, &font_name, 
										&font_width, &font_height, &horizontal_scaler, &vertical_scaler, Ae_6a1_translate_osd_region_id((OSD_TYPE)osdId));


		if(osdId == OSD_DATEANDTIME)	//自己取时间，其他项采用其他模块下发的///
		{
			sprintf(regionstr[osdId][chn], "%02d.%02d.%02d  %02d:%02d:%02d", date_time.year, date_time.month, date_time.day, date_time.hour, date_time.minute, date_time.second);
			osdStringLen[osdId][chn] = Ae_get_osd_string_width("16.26.26  26:26:26  ", font_name, horizontal_scaler);
		}
		else
		{
			strcpy(regionstr[osdId][chn], av_osd_item_it->video_stream.video_encoder_para.stuExtendOsdInfo[osdId].szContent);
			//printf("nnnnnnnnnn %s, %s\n", av_osd_item_it->video_stream.video_encoder_para.stuExtendOsdInfo[osdId].szContent, regionstr[osdId][chn]);
			osdStringLen[osdId][chn] = Ae_get_osd_string_width(regionstr[osdId][chn], font_name, horizontal_scaler);
		}
		
		osdStringLen[8][chn] = Ae_get_osd_string_width(" ", font_name, horizontal_scaler, false);
		pgAvDeviceInfo->Adi_get_video_size(av_osd_item_it->video_stream.video_encoder_para.resolution, av_osd_item_it->video_stream.video_encoder_para.tv_system, &(video_width[chn]), &(video_height[chn]));

		av_osd_item_it ++;
	}
	//计算合并两个osd条目///

	for(int index = 0 ; index < 8 ; index ++)
	{
		int nSpaceNum = 0;
		int pos = 0;
		osdStringLen[8][index] = ((osdStringLen[8][index] <= 0) ? 1 : osdStringLen[8][index]);
		//日期和速度里程凑成一对///
		if(update[0][index] == 1)
		{
			if(strlen(regionstr[OSD_DATEANDTIME][index]) <= 0 && strlen(regionstr[OSD_SPEEDANDMIL][index]) <= 0)
			{
				//无事可做///
			}
			else
			{
				if(strlen(regionstr[OSD_DATEANDTIME][index]) > 0 && strlen(regionstr[OSD_SPEEDANDMIL][index]) <= 0)
				{
					sprintf(regionosd[0][index], "%s  %s", regionstr[OSD_DATEANDTIME][index], regionstr[OSD_SPEEDANDMIL][index]);
				}
				else 
				{
					if(strlen(regionstr[OSD_DATEANDTIME][index]) <= 0 && strlen(regionstr[OSD_SPEEDANDMIL][index]) > 0)
					{
						memset(regionstr[OSD_DATEANDTIME][index], 0x0, sizeof(regionstr[OSD_DATEANDTIME][index]));
						regionstr[OSD_DATEANDTIME][index][0] = ' ';
					}
					
					if(osdStringLen[OSD_DATEANDTIME][index] + osdStringLen[OSD_SPEEDANDMIL][index] > video_width[index])
					{
						sprintf(regionosd[0][index], "%s  %s", regionstr[OSD_DATEANDTIME][index], regionstr[OSD_SPEEDANDMIL][index]);
					}
					else
					{
						nSpaceNum = (video_width[index] - osdStringLen[OSD_DATEANDTIME][index] - osdStringLen[OSD_SPEEDANDMIL][index]) / osdStringLen[8][index] - 1;
						nSpaceNum = ((nSpaceNum <= 0) ? 2 : nSpaceNum);
						pos = strlen(regionstr[OSD_DATEANDTIME][index]);
						memcpy(regionosd[0][index], regionstr[OSD_DATEANDTIME][index], pos);
						memset(regionosd[0][index] + pos, ' ', nSpaceNum);
						pos += nSpaceNum;
						memcpy(regionosd[0][index] + pos, regionstr[OSD_SPEEDANDMIL][index], strlen(regionstr[OSD_SPEEDANDMIL][index]));
					}
				}
				//printf("555555555, %s, %s, %d, %d, %d\n", regionosd[0][index], regionstr[OSD_SPEEDANDMIL][index], nSpaceNum, Ae_get_osd_string_width(regionosd[0][index], _AV_FONT_NAME_24_, 1), video_width[index]);
			}
		}
		if(update[1][index] == 1)
		{
			//REC自己占个区域///
			sprintf(regionosd[1][index], "%s", regionstr[OSD_RECSTATUS][index]);
		}
		if(update[2][index] == 1)
		{
			//USB状态和报警状态凑一对///
			if(strlen(regionstr[OSD_USBSTATUS][index]) <= 0 && strlen(regionstr[OSD_FIREMONITOR][index]) <= 0)
			{
				//无事可做///
			}
			else
			{
				//printf("22222222, %s, %s, %d, %d\n", regionstr[OSD_USBSTATUS][index], regionstr[OSD_FIREMONITOR][index], osdStringLen[OSD_USBSTATUS][index], osdStringLen[OSD_FIREMONITOR][index]);
				if(strlen(regionstr[OSD_USBSTATUS][index]) > 0 && strlen(regionstr[OSD_FIREMONITOR][index]) <= 0)
				{
					sprintf(regionosd[2][index], "%s  %s", regionstr[OSD_USBSTATUS][index], regionstr[OSD_FIREMONITOR][index]);
				}
				else
				{
					if(strlen(regionstr[OSD_USBSTATUS][index]) <= 0 && strlen(regionstr[OSD_FIREMONITOR][index]) > 0)
					{
						memset(regionstr[OSD_USBSTATUS][index], 0x0, sizeof(regionstr[OSD_USBSTATUS][index]));
						regionstr[OSD_USBSTATUS][index][0] = ' ';
					}
					
					if(osdStringLen[OSD_USBSTATUS][index] + osdStringLen[OSD_FIREMONITOR][index] > video_width[index])
					{
						sprintf(regionosd[2][index], "%s  %s", regionstr[OSD_USBSTATUS][index], regionstr[OSD_FIREMONITOR][index]);
					}
					else
					{
						nSpaceNum = (video_width[index] - osdStringLen[OSD_USBSTATUS][index] - osdStringLen[OSD_FIREMONITOR][index]) / osdStringLen[8][index] - 3;
						nSpaceNum = ((nSpaceNum <= 0) ? 2 : nSpaceNum);
						pos = strlen(regionstr[OSD_USBSTATUS][index]);
						memcpy(regionosd[2][index], regionstr[OSD_USBSTATUS][index], pos);
						memset(regionosd[2][index] + pos, ' ', nSpaceNum);
						pos += nSpaceNum;
						memcpy(regionosd[2][index] + pos, regionstr[OSD_FIREMONITOR][index], strlen(regionstr[OSD_FIREMONITOR][index]));
					}
					//printf("66666666, %s, %s, %d, %d, %d\n", regionosd[2][index], regionstr[OSD_CHNNAME][index], nSpaceNum, Ae_get_osd_string_width(regionosd[2][index], _AV_FONT_NAME_24_, 1), video_width[index]);
				}
			}
		}

		if(update[3][index] == 1)
		{
			//车次号和通道名凑一对///
			if(strlen(regionstr[OSD_TRAINANDNUM][index]) <= 0 && strlen(regionstr[OSD_CHNNAME][index]) <= 0)
			{
				//无事可做///
			}
			else
			{
				if(strlen(regionstr[OSD_TRAINANDNUM][index]) > 0 && strlen(regionstr[OSD_CHNNAME][index]) <= 0)
				{
					sprintf(regionosd[3][index], "%s  %s", regionstr[OSD_TRAINANDNUM][index], regionstr[OSD_CHNNAME][index]);
				}
				else
				{
					if(strlen(regionstr[OSD_TRAINANDNUM][index]) <= 0 && strlen(regionstr[OSD_CHNNAME][index]) > 0)
					{
						memset(regionstr[OSD_TRAINANDNUM][index], 0x0, sizeof(regionstr[OSD_TRAINANDNUM][index]));
						regionstr[OSD_TRAINANDNUM][index][0] = ' ';
					}
					if(osdStringLen[OSD_TRAINANDNUM][index] + osdStringLen[OSD_CHNNAME][index] > video_width[index])
					{
						sprintf(regionosd[3][index], "%s  %s", regionstr[OSD_TRAINANDNUM][index], regionstr[OSD_CHNNAME][index]);
					}
					else
					{
						nSpaceNum = (video_width[index] - osdStringLen[OSD_TRAINANDNUM][index] - osdStringLen[OSD_CHNNAME][index]) / osdStringLen[8][index] - 3;
						nSpaceNum = ((nSpaceNum <= 0) ? 2 : nSpaceNum);
						pos = strlen(regionstr[OSD_TRAINANDNUM][index]);
						memcpy(regionosd[3][index], regionstr[OSD_TRAINANDNUM][index], pos);
						memset(regionosd[3][index] + pos, ' ', nSpaceNum);
						pos += nSpaceNum;
						memcpy(regionosd[3][index] + pos, regionstr[OSD_CHNNAME][index], strlen(regionstr[OSD_CHNNAME][index]));
					}
				}
			}
		}
	}

	av_osd_item_it = av_osd_list.begin();
	char osdCreate[4][8] = {0};
	while(av_osd_item_it != av_osd_list.end())
	{
		unsigned char chn = (unsigned char)av_osd_item_it->video_stream.phy_video_chn_num;
		if(chn > 8)
		{
			break;
		}
		int region = Ae_6a1_translate_osd_region_id((OSD_TYPE)av_osd_item_it->extend_osd_id);

		if(av_osd_item_it->overlay_state != _AOS_INIT_)
		{
			if(osdCreate[region][chn] == 1)
			{
				av_osd_item_it->overlay_state == _AOS_OVERLAYED_;
			}
			else
			{
				osdCreate[region][chn] = 1;
				av_osd_item_it->overlay_state == _AOS_OVERLAYED_;
				memset(regionosd[region][chn] + MAX_EXTEND_OSD_LENGTH - 1, 0x0, MAX_EXTEND_OSD_LENGTH);
				//printf("=====, region = %d, chn = %d, update = %d\n", region, chn, update[region][chn]);
				if(update[region][chn] == 1)
				{
					Ae_set_osd(av_osd_item_it, regionosd[region][chn]);
				}
			}
		}
		av_osd_item_it ++;
	}
	return 0;
}

int CHiAvEncoder::Ae_6a1_translate_osd_region_id(OSD_TYPE type)
{
	int region = 0;
	switch(type)
	{
		case OSD_DATEANDTIME:
		case OSD_SPEEDANDMIL:
		default:
			region = 0;
			break;		
		case OSD_RECSTATUS:
			region = 1;
			break;
		case OSD_USBSTATUS:
		case OSD_FIREMONITOR:
			region = 2;
			break;
		case OSD_TRAINANDNUM:
		case OSD_CHNNAME:
			region = 3;
			break;
	}
	return region;
}

#else //!< non 3515
int CHiAvEncoder::Ae_create_osd_region(std::list<av_osd_item_t>::iterator &av_osd_item_it)
{
    REGION_ATTR_S region_attr;
    REGION_HANDLE region_handle;
    hi_osd_region_t *pOsd_region = NULL;
    memset(&region_attr, 0, sizeof(region_attr));
	
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

    /*calculate the size of region*/
    Ae_get_osd_parameter(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution, &font_name, 
                                    &font_width, &font_height, &horizontal_scaler, &vertical_scaler);

    //DEBUG_MESSAGE( "CHiAvEncoder::Ae_create_osd_region(channel:%d)(osd_name:%d)(w:%d)(h:%d)(ws:%d)(hs:%d)\n", av_osd_item_it->video_stream.phy_video_chn_num, av_osd_item_it->osd_name, font_width, font_height, horizontal_scaler, vertical_scaler);

    if(av_osd_item_it->osd_name == _AON_DATE_TIME_)
    {
        if(av_osd_item_it->video_stream.video_encoder_para.time_mode == _AV_TIME_MODE_12_)
        {
            char_number = strlen("2013-03-23  11:10:00 AM");
        }
        else
        {
            char_number = strlen("2013-03-23  11:10:00");            
        }

        switch(av_osd_item_it->video_stream.video_encoder_para.resolution)
        {
            case _AR_1080_:
            case _AR_720_:
            case _AR_960P_:
                region_attr.unAttr.stOverlay.stRect.u32Width  = ((font_width * char_number * horizontal_scaler) / 2 + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                region_attr.unAttr.stOverlay.stRect.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_ ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
            default:
                region_attr.unAttr.stOverlay.stRect.u32Width  = ((font_width * char_number * horizontal_scaler) * 2 / 3 + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                region_attr.unAttr.stOverlay.stRect.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
        }
    }
    else if(av_osd_item_it->osd_name == _AON_CHN_NAME_)
    {
        region_attr.unAttr.stOverlay.stRect.u32Width = 0;
        char* channelname = NULL;
        channelname = av_osd_item_it->video_stream.video_encoder_para.channel_name;
        CAvUtf8String channel_name(channelname);
        av_ucs4_t char_unicode;
        av_char_lattice_info_t lattice_char_info;
        while(channel_name.Next_char(&char_unicode) == true)
        {
            if(m_pAv_font_library->Afl_get_char_lattice(font_name, char_unicode, &lattice_char_info) == 0)
            {
                region_attr.unAttr.stOverlay.stRect.u32Width += lattice_char_info.width;
            }
            else
            {
                DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, font_name);
                return -1;
            }
        }
        if(region_attr.unAttr.stOverlay.stRect.u32Width == 0)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }

        region_attr.unAttr.stOverlay.stRect.u32Width  = (region_attr.unAttr.stOverlay.stRect.u32Width * horizontal_scaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        
        region_attr.unAttr.stOverlay.stRect.u32Height = (font_height * vertical_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_* _AE_OSD_ALIGNMENT_BYTE_;
    }
    else if(av_osd_item_it->osd_name == _AON_BUS_NUMBER_)    
    { 
        region_attr.unAttr.stOverlay.stRect.u32Width = 0;
        
        char tempstring[128] = {0};
        strncpy(tempstring, av_osd_item_it->video_stream.video_encoder_para.bus_number, strlen(av_osd_item_it->video_stream.video_encoder_para.bus_number));
        tempstring[strlen(av_osd_item_it->video_stream.video_encoder_para.bus_number)] = '\0';
        //DEBUG_MESSAGE("lshlsh tempstring %s strlen %d sizeof %d\n",tempstring, strlen(tempstring),sizeof(tempstring));
        //DEBUG_MESSAGE("lshlsh _AON_BUS_NUMBER_ %s sizeof %d strlen %d\n", av_osd_item_it->video_stream.video_encoder_para.bus_number, sizeof(av_osd_item_it->video_stream.video_encoder_para.bus_number), strlen(av_osd_item_it->video_stream.video_encoder_para.bus_number));
        region_attr.unAttr.stOverlay.stRect.u32Width = 0;
        CAvUtf8String bus_number(tempstring);
        av_ucs4_t char_unicode;
        av_char_lattice_info_t lattice_char_info;
        while(bus_number.Next_char(&char_unicode) == true)
        {
            if(m_pAv_font_library->Afl_get_char_lattice(font_name, char_unicode, &lattice_char_info) == 0)
            {
                region_attr.unAttr.stOverlay.stRect.u32Width += lattice_char_info.width;
            }
            else
            {
                DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, font_name);
                return -1;
            }
        }
        region_attr.unAttr.stOverlay.stRect.u32Width  = (region_attr.unAttr.stOverlay.stRect.u32Width * horizontal_scaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        region_attr.unAttr.stOverlay.stRect.u32Height = (font_height * vertical_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;

        if(region_attr.unAttr.stOverlay.stRect.u32Width == 0)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }

    }    
    else if(av_osd_item_it->osd_name == _AON_SELFNUMBER_INFO_)    
    {        
        region_attr.unAttr.stOverlay.stRect.u32Width = 0;
        char tempstring[128] = {0};
        //strcat(tempstring,av_osd_item_it->video_stream.video_encoder_para.bus_selfnumber);
        strncpy(tempstring, av_osd_item_it->video_stream.video_encoder_para.bus_selfnumber, strlen(av_osd_item_it->video_stream.video_encoder_para.bus_selfnumber));
        tempstring[strlen(av_osd_item_it->video_stream.video_encoder_para.bus_selfnumber)] = '\0';
        
        region_attr.unAttr.stOverlay.stRect.u32Width = 0;
        CAvUtf8String bus_slefnumber(tempstring);
        av_ucs4_t char_unicode;
        av_char_lattice_info_t lattice_char_info;
        while(bus_slefnumber.Next_char(&char_unicode) == true)
        {
            if(m_pAv_font_library->Afl_get_char_lattice(font_name, char_unicode, &lattice_char_info) == 0)
            {
                region_attr.unAttr.stOverlay.stRect.u32Width += lattice_char_info.width;
            }
            else
            {
                //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, font_name);
                return -1;
            }
        }
        if(region_attr.unAttr.stOverlay.stRect.u32Width == 0)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }
        region_attr.unAttr.stOverlay.stRect.u32Width  = (region_attr.unAttr.stOverlay.stRect.u32Width * horizontal_scaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        region_attr.unAttr.stOverlay.stRect.u32Height = (font_height * vertical_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_* _AE_OSD_ALIGNMENT_BYTE_;
    }    
    else if(av_osd_item_it->osd_name == _AON_GPS_INFO_)
    {        
        region_attr.unAttr.stOverlay.stRect.u32Width = 0;

        char_number = 25;

        switch(av_osd_item_it->video_stream.video_encoder_para.resolution)
        {
            case _AR_1080_:
            case _AR_720_:
            case _AR_960P_:
                region_attr.unAttr.stOverlay.stRect.u32Width  = ((font_width * char_number * horizontal_scaler) / 2 + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                region_attr.unAttr.stOverlay.stRect.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_ ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
            default:
                region_attr.unAttr.stOverlay.stRect.u32Width  = ((font_width * char_number * horizontal_scaler)*2/3  + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                region_attr.unAttr.stOverlay.stRect.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
        }
    }    
    else if(av_osd_item_it->osd_name == _AON_SPEED_INFO_)
    {        
        region_attr.unAttr.stOverlay.stRect.u32Width = 0;

        char_number = 8;
        
        switch(av_osd_item_it->video_stream.video_encoder_para.resolution)
        {
            case _AR_1080_:
            case _AR_720_:
            case _AR_960P_:
                region_attr.unAttr.stOverlay.stRect.u32Width  = ((font_width * char_number * horizontal_scaler) / 2 + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                region_attr.unAttr.stOverlay.stRect.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_ ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
            default:
                region_attr.unAttr.stOverlay.stRect.u32Width  = ((font_width * char_number * horizontal_scaler)*2/3  + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                region_attr.unAttr.stOverlay.stRect.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_ ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
        }
    }    
    else if(_AON_WATER_MARK == av_osd_item_it->osd_name)
    {
        region_attr.unAttr.stOverlay.stRect.u32Width = 0;
        char* watermarkname = av_osd_item_it->video_stream.video_encoder_para.water_mark_name;
        CAvUtf8String channel_name(watermarkname);
        av_ucs4_t char_unicode;
        av_char_lattice_info_t lattice_char_info;
        while(channel_name.Next_char(&char_unicode) == true)
        {
            if(m_pAv_font_library->Afl_get_char_lattice(font_name, char_unicode, &lattice_char_info) == 0)
            {
                region_attr.unAttr.stOverlay.stRect.u32Width += lattice_char_info.width;
            }
            else
            {
                DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, font_name);
                return -1;
            }
        }
        if(region_attr.unAttr.stOverlay.stRect.u32Width == 0)
        {
            DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }
        region_attr.unAttr.stOverlay.stRect.u32Width  = (region_attr.unAttr.stOverlay.stRect.u32Width * horizontal_scaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        region_attr.unAttr.stOverlay.stRect.u32Height = (font_height * vertical_scaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
    }
    else if(av_osd_item_it->osd_name == _AON_COMMON_OSD1_)
    {  
        region_attr.unAttr.stOverlay.stRect.u32Width = 0;
        char_number = 64;
        switch(av_osd_item_it->video_stream.video_encoder_para.resolution)
        {
        case _AR_1080_:
        case _AR_720_:
        case _AR_960P_:
        region_attr.unAttr.stOverlay.stRect.u32Width  = ((font_width * char_number * horizontal_scaler) / 2 + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        region_attr.unAttr.stOverlay.stRect.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_ ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
            break;
        default:
            region_attr.unAttr.stOverlay.stRect.u32Width  = ((font_width * char_number * horizontal_scaler)*2/3  + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
            region_attr.unAttr.stOverlay.stRect.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_ ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
            break;
        }
    } 
    else if(av_osd_item_it->osd_name == _AON_COMMON_OSD2_ )
    {  
        region_attr.unAttr.stOverlay.stRect.u32Width = 0;
        char_number = 64;
        switch(av_osd_item_it->video_stream.video_encoder_para.resolution)
        {
        case _AR_1080_:
        case _AR_720_:
        case _AR_960P_:
            region_attr.unAttr.stOverlay.stRect.u32Width  = ((font_width * char_number * horizontal_scaler) / 2 + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
            region_attr.unAttr.stOverlay.stRect.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_ ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
            break;
        default:
            region_attr.unAttr.stOverlay.stRect.u32Width  = ((font_width * char_number * horizontal_scaler)*2/3  + font_width*horizontal_scaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
            region_attr.unAttr.stOverlay.stRect.u32Height = (font_height * vertical_scaler +_AE_OSD_ALIGNMENT_BYTE_ ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
            break;
        }
    } 
    else
    {
        DEBUG_WARNING( "CHiAvEncoder::Ae_create_osd_region You must give the implement\n");
        return -1;
    }
    int video_width = 0;
    int video_height = 0;

    pgAvDeviceInfo->Adi_get_video_size(av_osd_item_it->video_stream.video_encoder_para.resolution, av_osd_item_it->video_stream.video_encoder_para.tv_system, &video_width, &video_height);

    if(region_attr.unAttr.stOverlay.stRect.u32Width > (unsigned int)video_width)
    {
        region_attr.unAttr.stOverlay.stRect.u32Width = video_width;
    }

    if(region_attr.unAttr.stOverlay.stRect.u32Height >  (unsigned int)video_height)
    {
        region_attr.unAttr.stOverlay.stRect.u32Height = video_height;
    }

    /*create region*/
    region_attr.enType = OVERLAY_REGION;
    region_attr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
    region_attr.unAttr.stOverlay.u32BgColor = 0x10;
        /*region_handle = Har_get_region_handle_encoder(av_osd_item_it->video_stream.type, \
            av_osd_item_it->video_stream.phy_video_chn_num, av_osd_item_it->osd_name);*/

 //chn related

    Ae_translate_coordinate(av_osd_item_it, region_attr.unAttr.stOverlay.stRect.u32Width, region_attr.unAttr.stOverlay.stRect.u32Height);

	region_attr.unAttr.stOverlay.stRect.s32X= av_osd_item_it->display_x ;
	region_attr.unAttr.stOverlay.stRect.s32Y= av_osd_item_it->display_y;

	region_attr.unAttr.stOverlay.u32BgAlpha = 20;//0:背景完全透明
	region_attr.unAttr.stOverlay.u32FgAlpha = 128;//128:完全不透明
	region_attr.unAttr.stOverlay.u32BgColor = 0x00;

    region_attr.unAttr.stOverlay.VeGroup = pVideo_encoder_info->venc_grp;

    region_attr.unAttr.stOverlay.bPublic = HI_FALSE;


    if(_AON_WATER_MARK == av_osd_item_it->osd_name)
    {
        region_attr.unAttr.stOverlay.u32FgAlpha = 32;
    }
    //--------------------------------------------------------------------    
	int retval = -1;
	
	retval = HI_MPI_VPP_CreateRegion(&region_attr, &region_handle); // handle is output
	
	if(retval != SUCCESS)
	{
		DEBUG_ERROR("HI_MPI_VPP_CreateRegion err 0x%x!\n",retval);
        DEBUG_ERROR("osd:%d, x:%d, y:%d, X:%d, Y:%d, W:%d, H:%d, C:%d, G:%d \n",av_osd_item_it->osd_name, av_osd_item_it->display_x,av_osd_item_it->display_y, region_attr.unAttr.stOverlay.stRect.s32X, region_attr.unAttr.stOverlay.stRect.s32Y, 
        region_attr.unAttr.stOverlay.stRect.u32Width, region_attr.unAttr.stOverlay.stRect.u32Height, region_attr.unAttr.stOverlay.u32BgColor, 
        region_attr.unAttr.stOverlay.VeGroup);
        
		return -1;
	}

    /*bitmap*/
    av_osd_item_it->bitmap_width = region_attr.unAttr.stOverlay.stRect.u32Width;
    av_osd_item_it->bitmap_height = region_attr.unAttr.stOverlay.stRect.u32Height;
    av_osd_item_it->pBitmap = new unsigned char[av_osd_item_it->bitmap_width * horizontal_scaler * av_osd_item_it->bitmap_height * vertical_scaler * 2];
    _AV_FATAL_(av_osd_item_it->pBitmap == NULL, "OUT OF MEMORY\n");

    /*region*/
    _AV_FATAL_((pOsd_region = new hi_osd_region_t) == NULL, "OUT OF MEMORY\n");
    pOsd_region->region_handle = region_handle;
    av_osd_item_it->pOsd_region = (void *)pOsd_region;

//------------------------- control region-----------------------------------------
    REGION_CRTL_CODE_E enCtrl;
	enCtrl = REGION_SHOW;/*show region*/
	REGION_CTRL_PARAM_U unParam;

	retval = HI_MPI_VPP_ControlRegion(region_handle, enCtrl, &unParam);
	if(retval != SUCCESS)
	{
		DEBUG_ERROR("show region %d faild 0x%x at 2!\n",index,retval);
		return -1;
	}
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
    char product_group[32] = {0};
    pgAvDeviceInfo->Adi_get_product_group(product_group, sizeof(product_group));
    
        char *pUtf8_string = NULL;

        char string[32];
        char gpsstring[64] = {0};

        char speedstring[16] = {0};
    
  
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
        if(strcmp(product_group, "ITS") != 0)
        {
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
        strncpy(speedstring, string, strlen(string));
        speedstring[strlen(string)] = '\0';
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
        //printf("[alex]the gps info is:%s \n", pUtf8_string);
#else
        msgGpsInfo_t gpsinfo;
        memset(&gpsinfo,0,sizeof(gpsinfo));
        pgAvDeviceInfo->Adi_Get_Gps_Info(&gpsinfo);
        if(gpsinfo.ucGpsStatus == 0)
        {
            char temp[48] = {0};
            sprintf(temp,"%d.%d.%04u%c %d.%d.%04u%c",gpsinfo.ucLatitudeDegree,gpsinfo.ucLatitudeCent,gpsinfo.ulLatitudeSec,gpsinfo.ucDirectionLatitude ? 'S':'N',gpsinfo.ucLongitudeDegree,gpsinfo.ucLongitudeCent,gpsinfo.ulLongitudeSec,gpsinfo.ucDirectionLongitude ? 'W':'E');
            //strcat(gpsstring,temp);
            strncpy(gpsstring, temp, strlen(temp));
            gpsstring[strlen(temp)] = '\0';
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

    if(Ae_utf8string_2_bmp(av_osd_item_it->osd_name, av_osd_item_it->video_stream.video_encoder_para.resolution,
        av_osd_item_it->pBitmap, av_osd_item_it->bitmap_width, av_osd_item_it->bitmap_height, pUtf8_string, 0xffff, 0x00, 0x8000) != 0)
    {
        DEBUG_ERROR("erooroeoooofooe osd_name =%d\n", av_osd_item_it->osd_name);
        return -1;
    }
        
#if defined(_AV_PRODUCT_CLASS_IPC_)
    if((av_osd_item_it->osd_name != _AON_DATE_TIME_) && (0 != strcmp(customer, "CUSTOMER_CNR")))
    {
        av_osd_item_it->is_update = false;
    }
#endif

    //DEBUG_CRITICAL("ready to set bitmap!\n");
    // show region
    REGION_CRTL_CODE_E enCtrl = REGION_SET_BITMAP;
    
    REGION_CTRL_PARAM_U unParam;
    memset(&unParam, 0, sizeof(unParam));

    unParam.stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
    unParam.stBitmap.u32Height = av_osd_item_it->bitmap_height; 
    unParam.stBitmap.u32Width = av_osd_item_it->bitmap_width;
    unParam.stBitmap.pData = av_osd_item_it->pBitmap;
    
    if ((ret_val = HI_MPI_VPP_ControlRegion(pOsd_region->region_handle, enCtrl, &unParam)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAvEncoder::Ae_osd_overlay HI_MPI_VPP_ControlRegion FAILED(region_handle:0x%08x)(%d)\n", ret_val, pOsd_region->region_handle);
        return -1; 
    }
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
    HI_S32 ret_val = -1;

    if((ret_val = HI_MPI_VPP_DestroyRegion(pOsd_region->region_handle)) != HI_SUCCESS)
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


#endif

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
#if !defined(_AV_PLATFORM_HI3515_)
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
#endif
    return 0;
}

#if defined(_AV_PRODUCT_CLASS_IPC_)
#if defined(_AV_HAVE_AUDIO_INPUT_)
int CHiAvEncoder::Ae_ai_aenc_proc(av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num)
{
    if(NULL != m_pAudio_thread_info)
    {
        m_pAudio_thread_info->thread_state = false;
        pthread_join(m_pAudio_thread_info->thread_id, NULL);
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

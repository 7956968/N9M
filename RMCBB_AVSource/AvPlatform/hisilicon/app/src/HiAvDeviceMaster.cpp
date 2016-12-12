#include "HiAvDeviceMaster.h"
#include "SystemConfig.h"
using namespace Common;

CHiAvDeviceMaster::CHiAvDeviceMaster()
{
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
    m_pip_create_status = false;
    m_pip_main_video_phy_chn = -1;
    m_pip_sub_video_phy_chn = -1;
    m_digital_zoom_status = false;
    m_video_preview_switch.set();
    m_pHi_vo = NULL;
    m_pHi_hdmi = NULL;
    for( int i = 0; i < 8; i++ )
    {
        m_preview_enable[i] = 1;
    }	
#endif


#if defined(_AV_HAVE_VIDEO_DECODER_)
    m_pHi_AvDec = NULL;
    
    m_talkback_cntrl.Stream_buffer_handle = NULL;
#endif

#if defined(_AV_HAVE_AUDIO_OUTPUT_)
    m_pHi_ao = NULL;
#endif

    m_audio_output_state.adec_chn = 0;
    m_audio_output_state.audio_mute = false;

#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)

    memset( m_u8DevOlState, 1, sizeof(m_u8DevOlState) );
#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_HDVR_)
    memset( m_c8ChannelMap, -1, sizeof(m_c8ChannelMap) );
 #else
    for( int i = 0; i < SUPPORT_MAX_VIDEO_CHANNEL_NUM; i++ )
    {
        m_c8ChannelMap[i] = i;
    }
 #endif

#endif

#if defined(_AV_HAVE_VIDEO_ENCODER_)
    m_pSnapTask = NULL;
#endif

#ifdef _AV_SUPPORT_REMOTE_TALK_
    m_pRemoteTalk = NULL;
#endif

#if !defined(_AV_PRODUCT_CLASS_IPC_)
//!Added for TTS
#if defined(_AV_HAVE_TTS_)
    m_pAv_tts = NULL;
#endif
    m_pAvStation = NULL;
#endif

#if defined(_AV_HAVE_AUDIO_INPUT_) && defined(_AV_HAVE_AUDIO_OUTPUT_)
	m_preview_audio_ctrl.phy_chn = -1;
	m_preview_audio_ctrl.ai_type = _AI_NORMAL_;
	m_preview_audio_ctrl.audio_start = false;
	m_preview_audio_ctrl.thread_start = false;
	m_preview_audio_ctrl.thread_exit = true;
	m_preview_audio_ctrl.bAudioPauseState = true;
	m_bstart = true;
	m_bInterupt = false;//优先级比预览高时用此变量停预览
	m_bPreviewAoStart = false;
	m_audio_preview_running = false;
#endif


}

CHiAvDeviceMaster::~CHiAvDeviceMaster()
{
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
    _AV_SAFE_DELETE_(m_pHi_vo);
    _AV_SAFE_DELETE_(m_pHi_hdmi);
#endif

#if defined(_AV_HAVE_VIDEO_DECODER_)
    _AV_SAFE_DELETE_(m_pHi_AvDec);
   
#endif

#if defined(_AV_HAVE_AUDIO_OUTPUT_)
    _AV_SAFE_DELETE_(m_pHi_ao);
#endif

#if defined(_AV_HAVE_VIDEO_ENCODER_)
    _AV_SAFE_DELETE_(m_pSnapTask);
#endif

#ifdef _AV_SUPPORT_REMOTE_TALK_
    _AV_SAFE_DELETE_(m_pRemoteTalk);
#endif

#if !defined(_AV_PRODUCT_CLASS_IPC_)
//!Added for TTS
#if defined(_AV_HAVE_TTS_)
    _AV_SAFE_DELETE_(m_pAv_tts);
#endif
    _AV_SAFE_DELETE_(m_pAvStation);
#endif
}


int CHiAvDeviceMaster::Ad_init()
{
    /*basic*/
    if(CHiAvDevice::Ad_init() != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_init (CHiAvDevice::Ad_init() != 0)\n");
        return -1;
    }

    /*vo*/
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
   _AV_FATAL_((m_pHi_vo = new CHiVo) == NULL, "OUT OF MEMORY\n");
   _AV_FATAL_((m_pHi_hdmi = new CHiHdmi) == NULL, "OUT OF MEMORY\n");
#endif

    /*ao*/
#if defined(_AV_HAVE_AUDIO_OUTPUT_)
   _AV_FATAL_((m_pHi_ao = new CHiAo) == NULL, "OUT OF MEMORY\n");
#endif

    /*av decoder*/
#if defined(_AV_HAVE_VIDEO_DECODER_)
    Ad_init_dec();
#endif

#if !defined(_AV_PRODUCT_CLASS_IPC_)
//!Added for TTS
#if defined(_AV_HAVE_TTS_)
    _AV_FATAL_((m_pAv_tts= new CAvTTS) == NULL, "OUT OF MEMORY\n");
#endif
    _AV_FATAL_((m_pAvStation= new CAvReportStation) == NULL, "OUT OF MEMORY\n");
#endif
    return 0;
}

#if defined(_AV_HISILICON_VPSS_) && defined(_AV_HAVE_VIDEO_OUTPUT_)
int CHiAvDeviceMaster::Ad_get_vpss_purpose(av_output_type_t output, int index, int type, int video_phy_chn_num, vpss_purpose_t &vpss_purpose, bool digitalvpss/* = false*/)
{
    switch(type)
    {
        case 0:
        {
#if defined(_AV_HAVE_VIDEO_DECODER_)
            if (_VS_DIGITAL_ == pgAvDeviceInfo->Adi_get_video_source(video_phy_chn_num))
            {
                if((output == _AOT_MAIN_) || (output == _AOT_CVBS_))/*_AOT_MAIN_: hdmi&vga, cvbs(write-back)*/
                {
                    vpss_purpose = _VP_PREVIEW_VDEC_;
                    if (true == digitalvpss)
                    {
                        vpss_purpose = _VP_PREVIEW_VDEC_DIGITAL_ZOOM_;
                    }
                }
                else if(output == _AOT_SPOT_)/*spot*/
                {
                    vpss_purpose = _VP_SPOT_VDEC_;
                    if (true == digitalvpss)
                    {
                        vpss_purpose = _VP_SPOT_VDEC_DIGITAL_ZOOM_;
                    }
                }
                else
                {
                    _AV_FATAL_(1, "CHiAvDeviceMaster::Ad_start_video_output_V1 You must give the implement\n");
                }  
                return 0;
            }
            else
#endif
            {
                if((output == _AOT_MAIN_) || (output == _AOT_CVBS_))/*_AOT_MAIN_: hdmi&vga, cvbs(write-back)*/
                {
                    vpss_purpose = _VP_PREVIEW_VI_;
                }
                else if(output == _AOT_SPOT_)/*spot*/
                {
                    vpss_purpose = _VP_SPOT_VI_;
                }
                else
                {
                    _AV_FATAL_(1, "CHiAvDeviceMaster::Ad_start_video_output_V1 You must give the implement\n");
                }
                if (true == digitalvpss)
                {
                    vpss_purpose = _VP_VI_DIGITAL_ZOOM_;
                }
            }
            break;
        }
        case 1:
            vpss_purpose = _VP_PLAYBACK_VDEC_;
            if (true == digitalvpss)
            {
                vpss_purpose = _VP_PLAYBACK_VDEC_DIGITAL_ZOOM_;
            }
            return 0;
            break;
        default:
            _AV_FATAL_(1, "CHiAvDeviceMaster::Ad_start_video_output_V1 You must give the implement\n");
            break;
    }

    return 0;
}

int CHiAvDeviceMaster::Ad_control_vpss_vo(av_output_type_t output, int index, vpss_purpose_t vpss_purpose, int phy_video_chn_num, int vo_chn, int control_flag)
{   
    MPP_CHN_S mpp_chn_src;
    MPP_CHN_S mpp_chn_dst;
    VPSS_GRP vpss_grp;
    VPSS_CHN vpss_chn;

    HI_S32 ret_val = -1;
    //DEBUG_ERROR( "CHiAvDeviceMaster::Ad_control_vpss_vo (output:%d) (index:%d)(vpss_purpose:%d)(layout_chn:%d)(phy_chn %d), control_flag %d\n", output, index, vpss_purpose, vo_chn, phy_video_chn_num, control_flag);

    if ((phy_video_chn_num >= pgAvDeviceInfo->Adi_max_channel_number()) || (phy_video_chn_num < 0))
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_control_vpss_vo (output:%d) (index:%d)(vpss_purpose:%d)(layout_chn:%d)(phy_chn %d), control_flag %d is illegal\n", output, index, vpss_purpose, vo_chn, phy_video_chn_num, control_flag);
        return -1;
    }
    if(m_pHi_vpss->HiVpss_get_vpss(vpss_purpose, phy_video_chn_num, &vpss_grp, &vpss_chn, NULL) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_control_vpss_vo m_pHi_vpss->HiVpss_get_vpss FAILED\n");
        return -1;
    }

    mpp_chn_src.enModId = HI_ID_VPSS;
    mpp_chn_src.s32DevId = vpss_grp;
    mpp_chn_src.s32ChnId = vpss_chn;

    mpp_chn_dst.enModId = HI_ID_VOU;
#if defined(_AV_PLATFORM_HI3535_)||defined(_AV_PLATFORM_HI3520D_V300_)
    VO_LAYER vo_layer;
    if(m_pHi_vo->HiVo_get_vo_layer(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), vo_layer) != 0)
    {
        DEBUG_ERROR( "get vo layer failed(%d)!\n", m_pHi_vo->HiVo_get_vo_dev(output, index, 0));
        return -1;
    }
    mpp_chn_dst.s32DevId = vo_layer;
#else
    mpp_chn_dst.s32DevId = m_pHi_vo->HiVo_get_vo_dev(output, index, 0);	
#endif
    mpp_chn_dst.s32ChnId = vo_chn;

#if !defined(_AV_PRODUCT_CLASS_DVR_REI_)
    if(control_flag == 0)
#else
    if((control_flag == 0)&&(Ad_get_preview_enable_switch(phy_video_chn_num)==1))
#endif
    {

        if((ret_val = HI_MPI_SYS_UnBind(&mpp_chn_src, &mpp_chn_dst)) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_source_control_V1 HI_MPI_SYS_UnBind FAILED(0x%08lx)\n", (unsigned long)ret_val);
            //return -1;
        }
		
    	DEBUG_MESSAGE("\n111,srcid:%d srcch:%d dstid:%d dstch:%d\n",mpp_chn_src.s32DevId,mpp_chn_src.s32ChnId,mpp_chn_dst.s32DevId,mpp_chn_dst.s32ChnId);
        if((ret_val = HI_MPI_SYS_Bind(&mpp_chn_src, &mpp_chn_dst)) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_source_control_V1 HI_MPI_SYS_Bind FAILED(0x%08lx)\n", (unsigned long)ret_val);
            return -1;
        }

		if((0 == control_flag) && ((_VP_PLAYBACK_VDEC_ == vpss_purpose) || (_VP_PREVIEW_VDEC_ == vpss_purpose)))
		{
			av_split_t split_mode = m_vo_chn_state[output][index].cur_split_mode;
			av_vag_hdmi_resolution_t resolution = Ad_get_resolution(output, index);
			av_rect_t video_rect;
			av_rect_t vo_chn_rect;
			VO_CHN_ATTR_S vo_chn_attr;
			if(output == _AOT_MAIN_)
			{
				m_pHi_vo->HiVo_get_video_size(resolution, &video_rect.w, &video_rect.h, NULL, NULL);
			}
			else if(output == _AOT_CVBS_)
			{
				m_pHi_vo->HiVo_get_video_size(pgAvDeviceInfo->Adi_get_system(), &video_rect.w, &video_rect.h, NULL, NULL);
			}
		
			Ad_calculate_video_rect(video_rect.w, video_rect.h, m_vo_chn_state[output][index].margin, &video_rect);
			pgAvDeviceInfo->Adi_get_split_chn_rect(&video_rect, split_mode, vo_chn, &vo_chn_rect, m_pHi_vo->HiVo_get_vo_dev(output, index, 0));

#if !defined(_AV_PLATFORM_HI3520D_V300_)
			if(HI_SUCCESS != (ret_val = HI_MPI_VO_GetChnAttr(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), vo_chn, &vo_chn_attr)))
			{
				DEBUG_ERROR( "CHiAvDeviceMaster::Ad_control_vpss_vo HI_MPI_VO_GetChnAttr FAILED(0x%08lx)\n", (unsigned long)ret_val);
				return -1;
			}
			if(((int)vo_chn_attr.stRect.s32X != vo_chn_rect.x) || ((int)vo_chn_attr.stRect.s32Y != vo_chn_rect.y) || ((int)vo_chn_attr.stRect.u32Width != vo_chn_rect.w) || ((int)vo_chn_attr.stRect.u32Height != vo_chn_rect.h))
			{
				DEBUG_MESSAGE("vodev:%d vo_chn:%d src: x=%d y=%d w=%d h=%d dst: x=%d y=%d w=%d h=%d\n",m_pHi_vo->HiVo_get_vo_dev(output, index, 0),vo_chn,vo_chn_attr.stRect.s32X,vo_chn_attr.stRect.s32Y,vo_chn_attr.stRect.u32Width,vo_chn_attr.stRect.u32Height,vo_chn_rect.x,vo_chn_rect.y,vo_chn_rect.w,vo_chn_rect.h); 	   
			
				vo_chn_attr.stRect.s32X = vo_chn_rect.x;
				vo_chn_attr.stRect.s32Y = vo_chn_rect.y;
				vo_chn_attr.stRect.u32Width = vo_chn_rect.w;
				vo_chn_attr.stRect.u32Height = vo_chn_rect.h;
				HI_MPI_VO_DisableChn(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), vo_chn);
				if(HI_SUCCESS != (ret_val = HI_MPI_VO_SetChnAttr(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), vo_chn, &vo_chn_attr)))
				{
					DEBUG_ERROR( "CHiAvDeviceMaster::Ad_control_vpss_vo HI_MPI_VO_SetChnAttr FAILED(0x%08lx)\n", (unsigned long)ret_val);
					return -1;
				}
            		}
            		HI_MPI_VO_EnableChn(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), vo_chn);
#else
			if(HI_SUCCESS != (ret_val = HI_MPI_VO_GetChnAttr(vo_layer, vo_chn, &vo_chn_attr)))
			{
				DEBUG_ERROR( "CHiAvDeviceMaster::Ad_control_vpss_vo HI_MPI_VO_GetChnAttr FAILED(0x%08lx)\n", (unsigned long)ret_val);
				return -1;
			}
			DEBUG_CRITICAL("111vo_layer:%d vo_chn:%d src: x=%d y=%d w=%d h=%d dst: x=%d y=%d w=%d h=%d\n",vo_layer,vo_chn,vo_chn_attr.stRect.s32X,vo_chn_attr.stRect.s32Y,vo_chn_attr.stRect.u32Width,vo_chn_attr.stRect.u32Height,vo_chn_rect.x,vo_chn_rect.y,vo_chn_rect.w,vo_chn_rect.h); 	   
#if 0
			if(((int)vo_chn_attr.stRect.s32X != vo_chn_rect.x) || ((int)vo_chn_attr.stRect.s32Y != vo_chn_rect.y) || ((int)vo_chn_attr.stRect.u32Width != vo_chn_rect.w) || ((int)vo_chn_attr.stRect.u32Height != vo_chn_rect.h))
			{
				DEBUG_CRITICAL("222vo_layer:%d vo_chn:%d src: x=%d y=%d w=%d h=%d dst: x=%d y=%d w=%d h=%d\n",vo_layer,vo_chn,vo_chn_attr.stRect.s32X,vo_chn_attr.stRect.s32Y,vo_chn_attr.stRect.u32Width,vo_chn_attr.stRect.u32Height,vo_chn_rect.x,vo_chn_rect.y,vo_chn_rect.w,vo_chn_rect.h); 	   
			
				vo_chn_attr.stRect.s32X = vo_chn_rect.x;
				vo_chn_attr.stRect.s32Y = vo_chn_rect.y;
				vo_chn_attr.stRect.u32Width = vo_chn_rect.w;
				vo_chn_attr.stRect.u32Height = vo_chn_rect.h;
				HI_MPI_VO_DisableChn(vo_layer, vo_chn);
				if(HI_SUCCESS != (ret_val = HI_MPI_VO_SetChnAttr(vo_layer, vo_chn, &vo_chn_attr)))
				{
					DEBUG_ERROR( "CHiAvDeviceMaster::Ad_control_vpss_vo HI_MPI_VO_SetChnAttr FAILED(0x%08lx)\n", (unsigned long)ret_val);
					return -1;
				}
            		}
#endif
            		HI_MPI_VO_EnableChn(vo_layer, vo_chn);
#endif
		}
    }
    else
    {
        if ((_VP_PREVIEW_VDEC_ == vpss_purpose) || (_VP_SPOT_VDEC_ == vpss_purpose))
        {/*暂停解码*/

        }
        /*it must be Hidden channel first, then vo can be correctly unbinded  with vpss */
        if(m_pHi_vo->HiVo_hide_chn(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), vo_chn) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_switch_output_V1 m_pHi_vo->HiVo_hide_chn FAILED\n");
            // return -1;
        }
    	DEBUG_MESSAGE("\n222, srcid:%d srcch:%d dstid:%d dstch:%d\n",mpp_chn_src.s32DevId,mpp_chn_src.s32ChnId,mpp_chn_dst.s32DevId,mpp_chn_dst.s32ChnId);

        if((ret_val = HI_MPI_SYS_UnBind(&mpp_chn_src, &mpp_chn_dst)) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_source_control_V1 HI_MPI_SYS_UnBind FAILED(0x%08lx)\n", (unsigned long)ret_val);
            return -1;
        }

        if(m_pHi_vo->HiVo_show_chn(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), vo_chn) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_drag_chn_V2 m_pHi_vo->HiVo_show_chn FAILED\n");
            // return -1;
        } 

        if(m_pHi_vpss->HiVpss_release_vpss(vpss_purpose, phy_video_chn_num, NULL) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_source_control_V1 m_pHi_vpss->HiVpss_get_vpss FAILED\n");
            return -1;
        }
    }
        
    return 0;
}

#if defined(_AV_HISILICON_VPSS_) && defined(_AV_HAVE_VIDEO_OUTPUT_) 
int CHiAvDeviceMaster::Ad_show_custormer_logo(int video_chn, bool show_or_hide)
{
    int ret_val = 0;
    VPSS_GRP vpss_grp = 0;
    VPSS_CHN vpss_chn = 0;
    VIDEO_FRAME_INFO_S stuVideoInfo;
    
    DEBUG_MESSAGE( "start CHiAvDeviceMaster::Ad_show_custormer_logo start video_chn %d, show_or_hide %d\n", video_chn, show_or_hide);
    ret_val = m_pSys_pic_manager->Aspm_get_picture(_ASPT_CUSTOMER_LOGO_, &stuVideoInfo);
    if (ret_val != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_show_custormer_logo m_pSys_pic_manager->Aspm_get_picture failed! \n");
        return -1;
    }
    
    if ((stuVideoInfo.stVFrame.u32Width == 0) || (stuVideoInfo.stVFrame.u32Height == 0))
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_show_custormer_logo w=%d, h=%d! \n", stuVideoInfo.stVFrame.u32Width, stuVideoInfo.stVFrame.u32Height);
        return -1;
    }
    
    if (show_or_hide)
    {
        av_rect_t logosize;
        memset(&logosize, 0, sizeof(av_rect_t));
        

        logosize.w = stuVideoInfo.stVFrame.u32Width;
        logosize.h = stuVideoInfo.stVFrame.u32Height;


        m_pHi_vpss->HiVpss_set_customlogo_size(logosize);
        if (m_pHi_vpss->HiVpss_get_vpss(_VP_CUSTOM_LOGO_, 0, &vpss_grp, &vpss_chn, NULL) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_show_custormer_logo FAILED\n");
            return -1;
        }
    }
    else
    {
        if (m_pHi_vpss->HiVpss_find_vpss(_VP_CUSTOM_LOGO_, 0, &vpss_grp, &vpss_chn, NULL) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_show_custormer_logo FAILED\n");
            return -1;
        }
        if(!m_pHi_vpss->Hivpss_whether_create_vpssgroup(vpss_grp))
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_show_custormer_logo forbid to release vpssgroup(%d)\n",vpss_grp);
            return 0;
        }
    }

    MPP_CHN_S mpp_chn_src;
    MPP_CHN_S mpp_chn_dst;
    mpp_chn_src.enModId = HI_ID_VPSS;
    mpp_chn_src.s32DevId = vpss_grp;
    mpp_chn_src.s32ChnId = vpss_chn;
    mpp_chn_dst.enModId = HI_ID_VOU;
    mpp_chn_dst.s32DevId = m_pHi_vo->HiVo_get_vo_dev(pgAvDeviceInfo->Adi_get_main_out_mode(), 0, 0);
    mpp_chn_dst.s32ChnId = video_chn;

    if (show_or_hide)
    {       
        if((ret_val = HI_MPI_SYS_Bind(&mpp_chn_src, &mpp_chn_dst)) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_show_custormer_logo HI_MPI_SYS_Bind vpss:(%d,%d), vo:(%d,%d)FAILED(0x%08lx)\n", vpss_grp, vpss_chn, mpp_chn_dst.s32DevId, video_chn, (unsigned long)ret_val);
            return -1;
        }

        
        DEBUG_MESSAGE( "CHiAvDeviceMaster::Ad_show_custormer_logo w %d, h %d\n", stuVideoInfo.stVFrame.u32Width, stuVideoInfo.stVFrame.u32Height);
        
        ret_val = m_pHi_vpss->HiVpss_send_user_frame(vpss_grp, &stuVideoInfo);
        if (ret_val != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_show_custormer_logo m_pHi_vpss->HiVpss_send_user_frame failed! \n");
            return -1;
        }
    }
    else
    {
        if ((ret_val = HI_MPI_SYS_UnBind(&mpp_chn_src, &mpp_chn_dst)) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_show_custormer_logo HI_MPI_SYS_UnBind FAILED(0x%08lx)\n", (unsigned long)ret_val);
            return -1;
        }

        if (m_pHi_vpss->HiVpss_release_vpss(_VP_CUSTOM_LOGO_, 0, NULL) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_show_custormer_logo FAILED\n");
            return -1;
        }
    }
    DEBUG_MESSAGE( "end CHiAvDeviceMaster::Ad_show_custormer_logo start video_chn %d, show_or_hide %d\n", video_chn, show_or_hide);

    return 0;
}

int CHiAvDeviceMaster::Ad_show_access_logo(int vo_dev, int video_chn, bool show_or_hide)
{
    if(show_or_hide)
    {
        int ret_val = 0;
        VPSS_GRP vpss_grp = 0;
        VPSS_CHN vpss_chn = 0;
        VIDEO_FRAME_INFO_S stuVideoInfo;
        av_rect_t logosize;
        MPP_CHN_S mpp_chn_src, mpp_chn_dst;

        ret_val = m_pSys_pic_manager->Aspm_get_picture(_ASPT_ACCESS_LOGO_, &stuVideoInfo);
        if (ret_val != 0)
        {
            //DEBUG_ERROR( "CHiAvDeviceMaster::Ad_show_access_logo m_pSys_pic_manager->Aspm_get_picture failed! \n");
            return -1;
        }
        if ((stuVideoInfo.stVFrame.u32Width == 0) || (stuVideoInfo.stVFrame.u32Height == 0))
        {
            DEBUG_WARNING( "CHiAvDeviceMaster::Ad_show_access_logo w=%d, h=%d! \n", stuVideoInfo.stVFrame.u32Width, stuVideoInfo.stVFrame.u32Height);
            return -1;
        }
        memset(&logosize, 0, sizeof(av_rect_t));       
        logosize.w = stuVideoInfo.stVFrame.u32Width;
        logosize.h = stuVideoInfo.stVFrame.u32Height;

        if(m_pHi_vpss->HiVpss_get_vpss(_VP_ACCESS_LOGO_, 0, &vpss_grp, &vpss_chn, NULL) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_show_custormer_logo FAILED\n");
            return -1;
        }
        mpp_chn_src.enModId = HI_ID_VPSS;
        mpp_chn_src.s32DevId = vpss_grp;
        mpp_chn_src.s32ChnId = vpss_chn;
        mpp_chn_dst.enModId = HI_ID_VOU;
        mpp_chn_dst.s32DevId = vo_dev;
        mpp_chn_dst.s32ChnId = video_chn;

        if((ret_val = HI_MPI_SYS_Bind(&mpp_chn_src, &mpp_chn_dst)) != 0)
        {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_show_access_logo HI_MPI_SYS_Bind vpss:(%d,%d), vo:(%d,%d)FAILED(0x%08lx)\n", vpss_grp, vpss_chn, mpp_chn_dst.s32DevId, video_chn, (unsigned long)ret_val);
        return -1;
        }
        DEBUG_MESSAGE( "CHiAvDeviceMaster::Ad_show_access_logo w %d, h %d(vochn:%d)\n", stuVideoInfo.stVFrame.u32Width, stuVideoInfo.stVFrame.u32Height, video_chn);
        ret_val = m_pHi_vpss->HiVpss_send_user_frame(vpss_grp, &stuVideoInfo);
        if (ret_val != 0)
        {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_show_access_logo m_pHi_vpss->HiVpss_send_user_frame failed! \n");
        return -1;
        }

        if ((ret_val = HI_MPI_SYS_UnBind(&mpp_chn_src, &mpp_chn_dst)) != 0)
        {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_show_custormer_logo HI_MPI_SYS_UnBind FAILED(0x%08lx)\n", (unsigned long)ret_val);
        return -1;
        }
        if (m_pHi_vpss->HiVpss_release_vpss(_VP_ACCESS_LOGO_, 0, NULL) != 0)
        {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_show_custormer_logo FAILED\n");
        return -1;
        }
    }
    else
    {
        m_pHi_vo->HiVo_clear_vo(vo_dev, video_chn);
    }
    //DEBUG_ERROR( "end CHiAvDeviceMaster::Ad_show_access_logo start video_chn %d, show_or_hide %d\n", video_chn, show_or_hide);

    return 0;
}

#endif


#endif

#if defined(_AV_HAVE_VIDEO_OUTPUT_)
int CHiAvDeviceMaster::Ad_start_output_V1(av_output_type_t output, int index, av_tvsystem_t tv_system, av_vag_hdmi_resolution_t resolution)
{
    VO_PUB_ATTR_S vo_pub_attr;

    if(output == _AOT_MAIN_)/*_AOT_MAIN_: hdmi&vga, cvbs(write-back)*/
    {
        /*start vga&hdmi device*/
        memset(&vo_pub_attr, 0, sizeof(VO_PUB_ATTR_S));
        vo_pub_attr.u32BgColor = pgAvDeviceInfo->Adi_get_vo_background_color( output, index );
        vo_pub_attr.enIntfType = VO_INTF_VGA | VO_INTF_HDMI;/*vga & hdmi*/
        vo_pub_attr.enIntfSync = m_pHi_vo->HiVo_get_vo_interface_sync(resolution);

#if !defined(_AV_PLATFORM_HI3535_)&&!defined(_AV_PLATFORM_HI3520D_V300_)
#if defined(_AV_PLATFORM_HI3520D_) || defined(_AV_PLATFORM_HI3520A_) || defined(_AV_PLATFORM_HI3521_)
        vo_pub_attr.bDoubleFrame = HI_FALSE;
#else
        vo_pub_attr.bDoubleFrame = HI_TRUE;
#endif
#endif

        if(m_pHi_vo->HiVo_start_vo_dev(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), &vo_pub_attr) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_output_V1 m_pHi_vo->HiVo_start_vo_dev FAILED\n");
            return -1;
        }

        /*start hdmi output*/
        if(m_pHi_hdmi->HiHdmi_start_hdmi(resolution) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_output_V1 m_pHi_hdmi->HiHdmi_start_hdmi FAILED\n");
            return -1;
        }

        /*start cvbs(write-back) device*/
        memset(&vo_pub_attr, 0, sizeof(VO_PUB_ATTR_S));
        vo_pub_attr.u32BgColor = pgAvDeviceInfo->Adi_get_vo_background_color(output, index);
        vo_pub_attr.enIntfType = VO_INTF_CVBS;
        vo_pub_attr.enIntfSync = m_pHi_vo->HiVo_get_vo_interface_sync(tv_system);
#if !(defined _AV_PLATFORM_HI3535_)&&!(defined _AV_PLATFORM_HI3520D_V300_)
        vo_pub_attr.bDoubleFrame = HI_FALSE;
#endif
        if(m_pHi_vo->HiVo_start_vo_dev(m_pHi_vo->HiVo_get_vo_dev(output, index, 1), &vo_pub_attr) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_output_V1 m_pHi_vo->HiVo_start_vo_dev FAILED\n");
            return -1;
        }
        
#if defined(_AV_PLATFORM_HI3520D_) /*720P---74MHZ*/
        if (resolution == _AVHR_720p50_ || resolution == _AVHR_720p60_)
        {
            HI_MPI_SYS_SetReg(0x20030008, 0x14aaaaaa);
            HI_MPI_SYS_SetReg(0x2003000c, 0x007c2062);
        }
#endif
        
    }
	else if(output == _AOT_CVBS_)
	{
		
	 	memset(&vo_pub_attr, 0, sizeof(VO_PUB_ATTR_S));
		vo_pub_attr.u32BgColor = pgAvDeviceInfo->Adi_get_vo_background_color(output, index);
		vo_pub_attr.enIntfType = VO_INTF_CVBS;
		vo_pub_attr.enIntfSync = m_pHi_vo->HiVo_get_vo_interface_sync(tv_system);
#if !(defined _AV_PLATFORM_HI3535_)&&!(defined _AV_PLATFORM_HI3520D_V300_)
		vo_pub_attr.bDoubleFrame = HI_FALSE;
#endif
		if(m_pHi_vo->HiVo_start_vo_dev(m_pHi_vo->HiVo_get_vo_dev(output, index, 1), &vo_pub_attr) != 0)
		{
			DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_output_V1 m_pHi_vo->HiVo_start_vo_dev FAILED\n");
			return -1;
		}
				
	}
    else if(output == _AOT_SPOT_)/*spot*/
    {
        memset(&vo_pub_attr, 0, sizeof(VO_PUB_ATTR_S));
        vo_pub_attr.u32BgColor = pgAvDeviceInfo->Adi_get_vo_background_color(output, index);
        vo_pub_attr.enIntfType = VO_INTF_CVBS;
        vo_pub_attr.enIntfSync = m_pHi_vo->HiVo_get_vo_interface_sync(tv_system);
#if !(defined _AV_PLATFORM_HI3535_)&&!(defined _AV_PLATFORM_HI3520D_V300_)
        vo_pub_attr.bDoubleFrame = HI_FALSE;
#endif
        if(m_pHi_vo->HiVo_start_vo_dev(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), &vo_pub_attr) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_output_V1 m_pHi_vo->HiVo_start_vo_dev FAILED\n");
            return -1;
        }
    }
    else
    {
        _AV_FATAL_(1, "CHiAvDeviceMaster::Ad_start_output_V1 You must give the implement\n");
    }

    return 0;
}


int CHiAvDeviceMaster::Ad_start_output(av_output_type_t output, int index, av_tvsystem_t tv_system, av_vag_hdmi_resolution_t resolution)
{
    int ret_val = -1;

    /*The output have been started by uboot, so we must stop it first*/
    static bool first_start = true;
    if(first_start == true)
    {
        Ad_stop_output(output, index);
        first_start = false;
    }

    DEBUG_MESSAGE( "CHiAvDeviceMaster::Ad_start_output(output:%d)(index:%d)(tv_system:%d)(resolution:%d)\n", output, index, tv_system, resolution);

    /*main output: vga&hdmi, cvbs(write-back); spot output: cvbs*/
    ret_val = Ad_start_output_V1(output, index, tv_system, resolution);

    if(ret_val == 0)
    {
        Ad_set_tv_system(output, index, tv_system);
        Ad_set_resolution(output, index, resolution);
    }

    return ret_val;
}


int CHiAvDeviceMaster::Ad_stop_output_V1(av_output_type_t output, int index)
{
    if(output == _AOT_MAIN_)/*_AOT_MAIN_: hdmi&vga, cvbs(write-back)*/
    {
        /*stop cvbs(write-back) device*/
        if(m_pHi_vo->HiVo_stop_vo_dev(m_pHi_vo->HiVo_get_vo_dev(output, index, 1)) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_output_V1 m_pHi_vo->HiVo_stop_vo_dev FAILED\n");
            return -1;
        }

        /*stop hdmi*/
        if(m_pHi_hdmi->HiHdmi_stop_hdmi() != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_output_V1 m_pHi_hdmi->HiHdmi_stop_hdmi FAILED\n");
            return -1;
        }

        /*stop vga&hdmi device*/
        if(m_pHi_vo->HiVo_stop_vo_dev(m_pHi_vo->HiVo_get_vo_dev(output, index, 0)) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_output_V1 m_pHi_vo->HiVo_stop_vo_dev FAILED\n");
            return -1;
        }                        
    }
	else if(output == _AOT_CVBS_)
	{
		if(m_pHi_vo->HiVo_stop_vo_dev(m_pHi_vo->HiVo_get_vo_dev(output, index, 1)) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_output_V1 m_pHi_vo->HiVo_stop_vo_dev FAILED\n");
            return -1;
        }
	}
    else if(output == _AOT_SPOT_)/*spot*/
    {
        if(m_pHi_vo->HiVo_stop_vo_dev(m_pHi_vo->HiVo_get_vo_dev(output, index, 0)) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_output_V1 m_pHi_vo->HiVo_stop_vo_dev FAILED\n");
            return -1;
        }
    }
    else
    {
        _AV_FATAL_(1, "CHiAvDeviceMaster::Ad_stop_output_V1 You must give the implement\n");
    }

    return 0;
}


int CHiAvDeviceMaster::Ad_stop_output(av_output_type_t output, int index)
{
    int ret_val = -1;

    DEBUG_MESSAGE( "CHiAvDeviceMaster::Ad_stop_output(output:%d)(index:%d)\n", output, index);

    /*main output: vga&hdmi, cvbs(write-back); spot output: cvbs*/
    ret_val = Ad_stop_output_V1(output, index);

    if(ret_val == 0)
    {
        Ad_unset_tv_system(output, index);
        Ad_unset_resolution(output, index);
    }

    return ret_val;
}


void CHiAvDeviceMaster::Ad_set_tv_system(av_output_type_t output, int index, av_tvsystem_t tv_system)
{
    m_output_tv_system[output][index] = tv_system;
}


void CHiAvDeviceMaster::Ad_unset_tv_system(av_output_type_t output, int index)
{
    m_output_tv_system[output][index] = _AT_UNKNOWN_;
}


av_tvsystem_t CHiAvDeviceMaster::Ad_get_tv_system(av_output_type_t output, int index)
{
    std::map<av_output_type_t, std::map<int, av_tvsystem_t> >::iterator output_it = m_output_tv_system.find(output);

    if(output_it == m_output_tv_system.end())
    {
        return _AT_UNKNOWN_;
    }

    if(output_it->second.find(index) != output_it->second.end())
    {
        return m_output_tv_system[output][index];
    }

    return _AT_UNKNOWN_;
}


void CHiAvDeviceMaster::Ad_set_resolution(av_output_type_t output, int index, av_vag_hdmi_resolution_t resolution)
{
    m_output_resolution[output][index] = resolution;
}


void CHiAvDeviceMaster::Ad_unset_resolution(av_output_type_t output, int index)
{
    m_output_resolution[output][index] = _AVHR_UNKNOWN_;
}


av_vag_hdmi_resolution_t CHiAvDeviceMaster::Ad_get_resolution(av_output_type_t output, int index)
{
    std::map<av_output_type_t, std::map<int, av_vag_hdmi_resolution_t> >::iterator output_it = m_output_resolution.find(output);

    if(output_it == m_output_resolution.end())
    {
        return _AVHR_UNKNOWN_;
    }

    if(output_it->second.find(index) != output_it->second.end())
    {
        return m_output_resolution[output][index];
    }

    return _AVHR_UNKNOWN_;
}


int CHiAvDeviceMaster::Ad_get_vo_chn(av_output_type_t output, int index, int chn_num)
{
    std::map<av_output_type_t, std::map<int, av_vag_hdmi_resolution_t> >::iterator output_it = m_output_resolution.find(output);

    if(output_it == m_output_resolution.end())
    {
        
        return -1;
    }

    if(output_it->second.find(index) == output_it->second.end())
    {
        return -1;
    }

    if (m_vo_chn_state[output][index].split_mode == _AS_UNKNOWN_)
    {
        return -1;
    }
    int max_vo_chn_num = pgAvDeviceInfo->Adi_get_split_chn_info(m_vo_chn_state[output][index].split_mode);

    for(int i = 0; i < max_vo_chn_num; i ++)
    {
        if(m_vo_chn_state[output][index].chn_layout[i] == chn_num)
        {
            return i;
        }
    }

    return -1;
}

int CHiAvDeviceMaster::Ad_set_video_preview(unsigned int preview_enable, bool deal_right_now)
{
    DEBUG_CRITICAL( "[%s,%d]:Preview:0x%x, now:%d\n", __FUNCTION__, __LINE__, preview_enable, deal_right_now);
    if(deal_right_now)
    {
        std::bitset<_AV_SPLIT_MAX_CHN_> PreviewMask = preview_enable;
        int chn_index = 0;
        for(chn_index = 0; chn_index < pgAvDeviceInfo->Adi_max_channel_number(); chn_index++)
        {
            if(PreviewMask.test(chn_index))
            {
                if(!m_video_preview_switch.test(chn_index))
                {
                    m_video_preview_switch.set(chn_index);
                    DEBUG_MESSAGE( "[%s,%d]:start chn %d preview!\n", __FUNCTION__, __LINE__, chn_index);
                    Ad_start_preview_output_chn(pgAvDeviceInfo->Adi_get_main_out_mode(), 0, chn_index);
                    Ad_start_preview_output_chn(_AOT_SPOT_, 0, chn_index);
                }
            }
            else
            {
                if(m_video_preview_switch.test(chn_index))
                {
                    DEBUG_MESSAGE( "[%s,%d]:stop chn %d preview!\n", __FUNCTION__, __LINE__, chn_index);
                    Ad_stop_preview_output_chn(pgAvDeviceInfo->Adi_get_main_out_mode(), 0, chn_index);
                    Ad_stop_preview_output_chn(_AOT_SPOT_, 0, chn_index);
                    m_video_preview_switch.reset(chn_index);
                }
            }
        }
    }
    else
    {
        m_video_preview_switch = preview_enable;
    }
    return 0;
}

int CHiAvDeviceMaster::Ad_set_video_preview_flag(int index, bool setornot)
{
        DEBUG_CRITICAL( "[%s,%d]:index:0x%x, setornot:%d\n", __FUNCTION__, __LINE__, index, setornot);

        if(setornot)
        {
            if(!m_video_preview_switch.test(index))
            {
                m_video_preview_switch.set(index);
                DEBUG_CRITICAL( "[%s,%d]:start chn %d preview!\n", __FUNCTION__, __LINE__, index);
            }
        }
        else
        {
            if(m_video_preview_switch.test(index))
            {
                DEBUG_CRITICAL( "[%s,%d]:stop chn %d preview!\n", __FUNCTION__, __LINE__, index);
                m_video_preview_switch.reset(index);
            }
        }

    return 0;
}

int CHiAvDeviceMaster::Ad_start_video_layer_V1(av_output_type_t output, int index, av_tvsystem_t tv_system, av_vag_hdmi_resolution_t resolution)
{
    VO_VIDEO_LAYER_ATTR_S video_layer_attr;
    VO_WBC_ATTR_S vo_wbc_attr;
    av_rect_t av_rect;
    unsigned int display_frm_rt = (tv_system == _AT_PAL_)?25:30;
#if defined(_AV_PRODUCT_CLASS_DECODER_)//defined(_AV_PRODUCT_CLASS_NVR_) ||  || defined(_AV_PRODUCT_CLASS_HDVR_) // for cvbs DISPLAY
    display_frm_rt = 30;
#endif

    if(output == _AOT_MAIN_)/*_AOT_MAIN_: hdmi&vga, cvbs(write-back)*/
    {
        /*hdmi&vga video layer*/
        memset(&video_layer_attr, 0, sizeof(VO_VIDEO_LAYER_ATTR_S));

        video_layer_attr.stDispRect.s32X = 0;
        video_layer_attr.stDispRect.s32Y = 0;
        m_pHi_vo->HiVo_get_video_size(resolution, (int *)&video_layer_attr.stImageSize.u32Width, (int *)&video_layer_attr.stImageSize.u32Height, 
                                                (int *)&video_layer_attr.stDispRect.u32Width, (int *)&video_layer_attr.stDispRect.u32Height);
        video_layer_attr.u32DispFrmRt = display_frm_rt;
        video_layer_attr.enPixFormat = pgAvDeviceInfo->Adi_get_video_layer_pixel_format();
        if(m_pHi_vo->HiVo_start_vo_video_layer(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), &video_layer_attr) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_video_layer_V1 m_pHi_vo->HiVo_start_vo_video_layer FAILED\n");
            return -1;
        }

        /*cvbs(write-back) video layer*/
        memset(&video_layer_attr, 0, sizeof(VO_VIDEO_LAYER_ATTR_S));

        video_layer_attr.stDispRect.s32X = 0;
        video_layer_attr.stDispRect.s32Y = 8;
        m_pHi_vo->HiVo_get_video_size(tv_system, (int *)&video_layer_attr.stImageSize.u32Width, (int *)&video_layer_attr.stImageSize.u32Height, NULL, NULL);
        video_layer_attr.stDispRect.u32Width = video_layer_attr.stImageSize.u32Width;
        video_layer_attr.stDispRect.u32Height = video_layer_attr.stImageSize.u32Height;
        video_layer_attr.u32DispFrmRt = (tv_system == _AT_PAL_) ? 25 : 30;
        video_layer_attr.enPixFormat = pgAvDeviceInfo->Adi_get_video_layer_pixel_format();
        if(m_pHi_vo->HiVo_start_vo_video_layer(m_pHi_vo->HiVo_get_vo_dev(output, index, 1), &video_layer_attr) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_video_layer_V1 m_pHi_vo->HiVo_start_vo_video_layer FAILED\n");
            return -1;
        }

        /*create cvbs(write-back) vo channel*/
        av_rect.x = 0;
        av_rect.y = 0;
        m_pHi_vo->HiVo_get_video_size(tv_system, (int *)&av_rect.w, (int *)&av_rect.h, NULL, NULL);
        if(Ad_create_vo_layout(m_pHi_vo->HiVo_get_vo_dev(output, index, 1), &av_rect, _AS_SINGLE_) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_video_layer_V1 Ad_create_vo_layout FAILED\n");
            return -1;
        }

#if defined(_AV_PLATFORM_HI3535_)||defined(_AV_PLATFORM_HI3520D_V300_)
        VO_WBC VoWbc;
        VO_WBC_SOURCE_S WbcSource;

        VoWbc = 0;
        WbcSource.enSourceType = VO_WBC_SOURCE_DEV;
        WbcSource.u32SourceId = 0;
        if(m_pHi_vo->HiVo_bind_wbc(VoWbc, &WbcSource) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_output_V1 m_pHi_vo->HiVo_bind_wbc FAILED\n");
            return -1;
        }
#endif

        /*start write-back*/
        m_pHi_vo->HiVo_get_video_size(tv_system, (int *)&vo_wbc_attr.stTargetSize.u32Width, (int *)&vo_wbc_attr.stTargetSize.u32Height, NULL, NULL);
        vo_wbc_attr.u32FrameRate = (tv_system == _AT_PAL_) ? 25 : 30;
        vo_wbc_attr.enPixelFormat = pgAvDeviceInfo->Adi_get_cvbs_vo_pixel_format();
#if defined(_AV_PLATFORM_HI3520D_)
        vo_wbc_attr.enDataSource = VO_WBC_DATASOURCE_MIXER;
#endif
        if(m_pHi_vo->HiVo_start_wbc(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), m_pHi_vo->HiVo_get_vo_dev(output, index, 1), &vo_wbc_attr) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_output_V1 m_pHi_vo->HiVo_start_wbc FAILED\n");
            return -1;
        }
    }
	else if(output == _AOT_CVBS_)
	{
        memset(&video_layer_attr, 0, sizeof(VO_VIDEO_LAYER_ATTR_S));

        video_layer_attr.stDispRect.s32X = (720 -  pgAvDeviceInfo->Adi_get_D1_width()) / 2;
        video_layer_attr.stDispRect.s32Y = 0;
        m_pHi_vo->HiVo_get_video_size(tv_system, (int *)&video_layer_attr.stImageSize.u32Width, (int *)&video_layer_attr.stImageSize.u32Height, NULL, NULL);
        video_layer_attr.stDispRect.u32Width = video_layer_attr.stImageSize.u32Width;
        video_layer_attr.stDispRect.u32Height = video_layer_attr.stImageSize.u32Height;
        video_layer_attr.u32DispFrmRt = (tv_system == _AT_PAL_)?25:30;
        video_layer_attr.enPixFormat = pgAvDeviceInfo->Adi_get_video_layer_pixel_format();
        if(m_pHi_vo->HiVo_start_vo_video_layer(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), &video_layer_attr) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_video_layer_V1 m_pHi_vo->HiVo_start_vo_video_layer FAILED\n");
            return -1;
        }
	}
    else if(output == _AOT_SPOT_)/*spot*/
    {
        memset(&video_layer_attr, 0, sizeof(VO_VIDEO_LAYER_ATTR_S));

        video_layer_attr.stDispRect.s32X = (720 -  pgAvDeviceInfo->Adi_get_D1_width()) / 2;
        video_layer_attr.stDispRect.s32Y = 0;
        m_pHi_vo->HiVo_get_video_size(tv_system, (int *)&video_layer_attr.stImageSize.u32Width, (int *)&video_layer_attr.stImageSize.u32Height, NULL, NULL);
        video_layer_attr.stDispRect.u32Width = video_layer_attr.stImageSize.u32Width;
        video_layer_attr.stDispRect.u32Height = video_layer_attr.stImageSize.u32Height;
        video_layer_attr.u32DispFrmRt = (tv_system == _AT_PAL_)?25:30;
        video_layer_attr.enPixFormat = pgAvDeviceInfo->Adi_get_video_layer_pixel_format();
        if(m_pHi_vo->HiVo_start_vo_video_layer(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), &video_layer_attr) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_video_layer_V1 m_pHi_vo->HiVo_start_vo_video_layer FAILED\n");
            return -1;
        }
    }
    else
    {
        _AV_FATAL_(1, "CHiAvDeviceMaster::Ad_start_video_layer_V1 You must give the implement\n");
    }      

    return 0;
}


int CHiAvDeviceMaster::Ad_start_video_layer(av_output_type_t output, int index)
{
    av_tvsystem_t tv_system = Ad_get_tv_system(output, index);
    av_vag_hdmi_resolution_t resolution = Ad_get_resolution(output, index);
    DEBUG_MESSAGE( "CHiAvDeviceMaster::Ad_start_video_layer(output:%d)(index:%d)(tv_system:%d)(resolution:%d)\n", output, index, tv_system, resolution);

    /*main output: vga&hdmi, cvbs(write-back); spot output: cvbs*/
    return Ad_start_video_layer_V1(output, index, tv_system, resolution);
}


int CHiAvDeviceMaster::Ad_stop_video_layer_V1(av_output_type_t output, int index)
{
    if(output == _AOT_MAIN_)/*_AOT_MAIN_: hdmi&vga, cvbs(write-back)*/
    {
        /*stop cvbs(write-back)*/
        if(m_pHi_vo->HiVo_stop_wbc(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), m_pHi_vo->HiVo_get_vo_dev(output, index, 1)) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_output_V1 m_pHi_vo->HiVo_stop_wbc write-back FAILED\n");
            return -1;
        }

        /*destory cvbs(write-back) vo channel*/
        if(Ad_destory_vo_layout(m_pHi_vo->HiVo_get_vo_dev(output, index, 1), _AS_SINGLE_) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_video_layer_V1 Ad_destory_vo_layout FAILED\n");
            return -1;
        }

        /*stop cvbs video layer*/
        if(m_pHi_vo->HiVo_stop_vo_video_layer(m_pHi_vo->HiVo_get_vo_dev(output, index, 1)) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_video_layer_V1 m_pHi_vo->HiVo_stop_vo_video_layer FAILED\n");
            return -1;
        }

        /*stop vga&hdmi video layer*/
        if(m_pHi_vo->HiVo_stop_vo_video_layer(m_pHi_vo->HiVo_get_vo_dev(output, index, 0)) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_video_layer_V1 m_pHi_vo->HiVo_stop_vo_video_layer FAILED\n");
            return -1;
        }
    }
	else if(output == _AOT_CVBS_)
	{
		if(m_pHi_vo->HiVo_stop_vo_video_layer(m_pHi_vo->HiVo_get_vo_dev(output, index, 0)) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_video_layer_V1 m_pHi_vo->HiVo_stop_vo_video_layer FAILED\n");
            return -1;
        }
	}
    else if(output == _AOT_SPOT_)/*spot*/
    {
        if(m_pHi_vo->HiVo_stop_vo_video_layer(m_pHi_vo->HiVo_get_vo_dev(output, index, 0)) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_video_layer_V1 m_pHi_vo->HiVo_stop_vo_video_layer FAILED\n");
            return -1;
        }
    }
    else
    {
        _AV_FATAL_(1, "CHiAvDeviceMaster::Ad_stop_video_layer_V1 You must give the implement\n");
    }

    return 0;
}


int CHiAvDeviceMaster::Ad_stop_video_layer(av_output_type_t output, int index)
{
    DEBUG_MESSAGE( "CHiAvDeviceMaster::Ad_stop_video_layer(output:%d)(index:%d)\n", output, index);

    /*main output: vga&hdmi, cvbs(write-back); spot output: cvbs*/
    return Ad_stop_video_layer_V1(output, index);
}


int CHiAvDeviceMaster::Ad_calculate_video_rect(int video_layer_width, int video_layer_height, int margin[4], av_rect_t *pVideo_rect)
{
    pVideo_rect->x = 0;
    pVideo_rect->y = 0;
    pVideo_rect->w = video_layer_width;
    pVideo_rect->h = video_layer_height;
    if(margin != NULL)
    {
        /*top*/
        pVideo_rect->y = margin[_AV_MARGIN_TOP_INDEX_];
        pVideo_rect->h -= margin[_AV_MARGIN_TOP_INDEX_];
        /*bottom*/
        pVideo_rect->h -= margin[_AV_MARGIN_BOTTOM_INDEX_];
        /*left*/
        pVideo_rect->x = margin[_AV_MARGIN_LEFT_INDEX_];
        pVideo_rect->w -= margin[_AV_MARGIN_LEFT_INDEX_];
        /*right*/
        pVideo_rect->w -= margin[_AV_MARGIN_RIGHT_INDEX_];
    }

    return 0;
}
int CHiAvDeviceMaster::Ad_calculate_video_rect_V2(int video_layer_width, int video_layer_height, const int margin[4], av_rect_t *pVideo_rect,av_vag_hdmi_resolution_t resolution)
{
    int adjust_margin[4];
    adjust_margin[_AV_MARGIN_TOP_INDEX_] = margin[_AV_MARGIN_TOP_INDEX_];
    adjust_margin[_AV_MARGIN_BOTTOM_INDEX_] = margin[_AV_MARGIN_BOTTOM_INDEX_];
    adjust_margin[_AV_MARGIN_LEFT_INDEX_] = margin[_AV_MARGIN_LEFT_INDEX_];
    adjust_margin[_AV_MARGIN_RIGHT_INDEX_] = margin[_AV_MARGIN_RIGHT_INDEX_];

    pVideo_rect->x = 0;
    pVideo_rect->y = 0;
    pVideo_rect->w = video_layer_width;
    pVideo_rect->h = video_layer_height;
    calculate_margin_for_limit_resource_machine(resolution, adjust_margin);

    if(margin != NULL)
    {
        /*top*/
        pVideo_rect->y = adjust_margin[_AV_MARGIN_TOP_INDEX_];
        pVideo_rect->h -= adjust_margin[_AV_MARGIN_TOP_INDEX_];
        /*bottom*/
        pVideo_rect->h -= adjust_margin[_AV_MARGIN_BOTTOM_INDEX_];
        /*left*/
        pVideo_rect->x = adjust_margin[_AV_MARGIN_LEFT_INDEX_];
        pVideo_rect->w -= adjust_margin[_AV_MARGIN_LEFT_INDEX_];
        /*right*/
        pVideo_rect->w -= adjust_margin[_AV_MARGIN_RIGHT_INDEX_];
    }

    return 0;

}

int CHiAvDeviceMaster::Ad_create_vo_layout(VO_DEV vo_dev, av_rect_t *pVideo_rect, av_split_t split_mode)
{
    int max_vo_chn_num = pgAvDeviceInfo->Adi_get_split_chn_info(split_mode);
    av_rect_t vo_chn_rect;
    VO_CHN_ATTR_S vo_chn_attr;
    int vo_chn = -1;
    
    for(vo_chn = 0; vo_chn < max_vo_chn_num; vo_chn ++)
    {
        memset(&vo_chn_attr, 0, sizeof(VO_CHN_ATTR_S));

        vo_chn_attr.u32Priority = 0;
        vo_chn_attr.bDeflicker = HI_FALSE;

        pgAvDeviceInfo->Adi_get_split_chn_rect(pVideo_rect, split_mode, vo_chn, &vo_chn_rect, vo_dev);
        vo_chn_attr.stRect.s32X = vo_chn_rect.x;
        vo_chn_attr.stRect.s32Y = vo_chn_rect.y;
        vo_chn_attr.stRect.u32Width = vo_chn_rect.w;
        vo_chn_attr.stRect.u32Height = vo_chn_rect.h;
//        vo_chn_attr.bDeflicker = HI_TRUE; //使能通道抗闪烁,会对xmd3104机器视频清晰度产生影响20160107

#if defined(_AV_PLATFORM_HI3521_)
        vo_chn_attr.bDeflicker = HI_TRUE;
#endif

        vo_chn_attr.bDeflicker = HI_TRUE;   //!<20160607

        if(m_pHi_vo->HiVo_start_vo_chn(vo_dev, vo_chn, &vo_chn_attr) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_create_vo_layout m_pHi_vo->HiVo_start_vo_chn %d FAILED\n", vo_chn);
            return -1;
        }

        if (_AS_PIP_ == split_mode)
        {
           // DEBUG_ERROR( "CHiAvDeviceMaster::Ad_create_vo_layout m_pHi_vo->HiVo_hide_chn %d\n", vo_chn);
            if (m_pHi_vo->HiVo_hide_chn(vo_dev, vo_chn) != 0)
            {
                DEBUG_ERROR( "CHiAvDeviceMaster::Ad_create_vo_layout m_pHi_vo->HiVo_hide_chn %d FAILED\n", vo_chn);
                return -1;
            }
        }
    }

    return 0;
}

int CHiAvDeviceMaster::Ad_create_selected_vo_layout(VO_DEV vo_dev, av_rect_t *pVideo_rect, av_split_t split_mode, int vo_chn, int vo_index)
{
    av_rect_t vo_chn_rect;
    VO_CHN_ATTR_S vo_chn_attr;
    
    memset(&vo_chn_attr, 0, sizeof(VO_CHN_ATTR_S));

    vo_chn_attr.u32Priority = 0;
    vo_chn_attr.bDeflicker = HI_FALSE;

    pgAvDeviceInfo->Adi_get_split_chn_rect(pVideo_rect, split_mode, vo_index, &vo_chn_rect, vo_dev);
    vo_chn_attr.stRect.s32X = vo_chn_rect.x;
    vo_chn_attr.stRect.s32Y = vo_chn_rect.y;
    vo_chn_attr.stRect.u32Width = vo_chn_rect.w;
    vo_chn_attr.stRect.u32Height = vo_chn_rect.h;
 #if defined(_AV_PLATFORM_HI3521_)
    vo_chn_attr.bDeflicker = HI_TRUE;
#endif   

    vo_chn_attr.bDeflicker = HI_TRUE;//!<20160607
    
    if(m_pHi_vo->HiVo_start_vo_chn(vo_dev, vo_chn, &vo_chn_attr) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_create_vo_layout m_pHi_vo->HiVo_start_vo_chn %d FAILED\n", vo_chn);
        return -1;
    }     

    if (_AS_PIP_ == split_mode)
    {
       // DEBUG_ERROR( "CHiAvDeviceMaster::Ad_create_vo_layout m_pHi_vo->HiVo_hide_chn %d\n", vo_chn);
        if (m_pHi_vo->HiVo_hide_chn(vo_dev, vo_chn) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_create_vo_layout m_pHi_vo->HiVo_hide_chn %d FAILED\n", vo_chn);
            return -1;
        }
    }

    return 0;
}

int CHiAvDeviceMaster::Ad_destory_vo_layout(VO_DEV vo_dev, av_split_t split_mode)
{
    int max_vo_chn_num = pgAvDeviceInfo->Adi_get_split_chn_info(split_mode);    
    int vo_chn = -1;
    for(vo_chn = 0; vo_chn < max_vo_chn_num; vo_chn ++)
    {
        if(m_pHi_vo->HiVo_stop_vo_chn(vo_dev, vo_chn) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_destory_vo_layout m_pHi_vo->HiVo_stop_vo_chn FAILED\n");
            return -1;
        }
        
        if(m_pHi_vo->HiVo_clear_vo(vo_dev, vo_chn) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_destory_vo_layout m_pHi_vo->HiVo_clear_vo FAILED\n");
            return -1;
        }
        else
        {
            //printf("HiVo_clear_vo success...\n");
            
        }
    }

    return 0;
}

int CHiAvDeviceMaster::Ad_start_video_output_V1(av_output_type_t output, int index, int type, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], int margin[4]/* = NULL*/)
{
    av_tvsystem_t tv_system = Ad_get_tv_system(output, index);
    av_vag_hdmi_resolution_t resolution = Ad_get_resolution(output, index);
    int max_vo_chn_num = pgAvDeviceInfo->Adi_get_split_chn_info(split_mode);
    av_rect_t video_rect;
    int chn_index = -1;
    
    pgAvDeviceInfo->Adi_get_cur_display_chn(chn_layout);
    
    /*create vo channel layout, first get the size of video layer*/
    if(output == _AOT_MAIN_)/*_AOT_MAIN_: hdmi&vga, cvbs(write-back)*/
    {
        m_pHi_vo->HiVo_get_video_size(resolution, &video_rect.w, &video_rect.h, NULL, NULL);
    }
	else if(output == _AOT_CVBS_)
	{
		m_pHi_vo->HiVo_get_video_size(tv_system, &video_rect.w, &video_rect.h, NULL, NULL);
	}
    else if(output == _AOT_SPOT_)/*spot*/
    {
        m_pHi_vo->HiVo_get_video_size(tv_system, &video_rect.w, &video_rect.h, NULL, NULL);
    }
    else
    {
        _AV_FATAL_(1, "CHiAvDeviceMaster::Ad_start_video_output_V1 You must give the implement\n");
    }

    calculate_margin_for_limit_resource_machine(resolution,margin);

    Ad_calculate_video_rect(video_rect.w, video_rect.h, margin, &video_rect);
       
    if(Ad_create_vo_layout(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), &video_rect, split_mode) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_video_output_V1 Ad_create_vo_layout FAILED\n");
        return -1;
    }

    int maxChlNum = pgAvDeviceInfo->Adi_max_channel_number();
    for(chn_index = 0; chn_index < max_vo_chn_num && chn_index < maxChlNum; chn_index ++)
    {
        if(chn_layout[chn_index] < 0)
        {
            continue;
        }
        if(!m_video_preview_switch.test(chn_layout[chn_index]))
        {
            Ad_show_access_logo(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), chn_index, true);
            DEBUG_ERROR( "start all: chn %d have not preview!\n", chn_layout[chn_index]);
            continue;
        }
#if defined(_AV_HISILICON_VPSS_)
        vpss_purpose_t vpss_purpose;
        Ad_get_vpss_purpose(output, index, type, chn_layout[chn_index], vpss_purpose, false);
        if ((vpss_purpose == _VP_SPOT_VDEC_)
            || (vpss_purpose == _VP_PREVIEW_VI_)
            || (vpss_purpose == _VP_SPOT_VI_))
        {
            Ad_control_vpss_vo(output, index, vpss_purpose, chn_layout[chn_index], chn_index, 0);
        }   
#endif
    }
    
#if defined(_AV_HISILICON_VPSS_) && defined(_AV_HAVE_VIDEO_OUTPUT_) 
    if (pgAvDeviceInfo->Adi_max_channel_number() == 8)
    {
        if ((split_mode == _AS_9SPLIT_) && (type != 1))
        {
            Ad_show_custormer_logo(8, true);
        }
    }
#endif
    return 0;
}

int CHiAvDeviceMaster::Ad_start_selected_video_output_V1(av_output_type_t output, int index, int type, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], unsigned long long int chn_mask, int margin[4])
{
    av_tvsystem_t tv_system = Ad_get_tv_system(output, index);
    av_vag_hdmi_resolution_t resolution = Ad_get_resolution(output, index);
    int max_vo_chn_num = pgAvDeviceInfo->Adi_get_split_chn_info(split_mode);
    av_rect_t video_rect;
    int chn_index = -1;
    int maxChlNum = pgAvDeviceInfo->Adi_max_channel_number();
    int VoChn;
    /*create vo channel layout, first get the size of video layer*/
    if(output == _AOT_MAIN_)/*_AOT_MAIN_: hdmi&vga, cvbs(write-back)*/
    {
        m_pHi_vo->HiVo_get_video_size(resolution, &video_rect.w, &video_rect.h, NULL, NULL);
    }
	else if(output == _AOT_CVBS_)
	{
		m_pHi_vo->HiVo_get_video_size(tv_system, &video_rect.w, &video_rect.h, NULL, NULL);
	}
    else if(output == _AOT_SPOT_)/*spot*/
    {
        m_pHi_vo->HiVo_get_video_size(tv_system, &video_rect.w, &video_rect.h, NULL, NULL);
    }
    else
    {
        _AV_FATAL_(1, "CHiAvDeviceMaster::Ad_start_selected_video_output_V1 You must give the implement\n");
    }
    
    calculate_margin_for_limit_resource_machine(resolution,margin);
    
    Ad_calculate_video_rect(video_rect.w, video_rect.h, margin, &video_rect);
    for(chn_index = 0; chn_index < max_vo_chn_num && chn_index < maxChlNum; chn_index ++)
    {
        if(chn_layout[chn_index] < 0)
        {
            continue;
        }

        if(0 != ((chn_mask) & (1ll<<chn_layout[chn_index])))
        {
            VoChn = Ad_get_vo_chn(output, index, chn_layout[chn_index]);
            if(Ad_create_selected_vo_layout(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), &video_rect, split_mode, VoChn, chn_index) != 0)
            {
                DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_selected_video_output_V1 Ad_create_vo_layout FAILED\n");
                return -1;
            }
            if(!m_video_preview_switch.test(chn_layout[chn_index]))
            {
                Ad_show_access_logo(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), VoChn, true);
                DEBUG_ERROR( "start all: chn %d have not preview!\n", chn_layout[chn_index]);
                continue;
            }
            
#if defined(_AV_HISILICON_VPSS_)
            vpss_purpose_t vpss_purpose;
            Ad_get_vpss_purpose(output, index, type, chn_layout[chn_index], vpss_purpose, false);

            if ((vpss_purpose == _VP_SPOT_VDEC_)
                || (vpss_purpose == _VP_PREVIEW_VI_)
                || (vpss_purpose == _VP_SPOT_VI_))
            {
                Ad_control_vpss_vo(output, index, vpss_purpose, chn_layout[chn_index], VoChn, 0);
            }   
#endif
            continue;
        }
    }
    
#if defined(_AV_HISILICON_VPSS_) && defined(_AV_HAVE_VIDEO_OUTPUT_) 
    if (pgAvDeviceInfo->Adi_max_channel_number() == 8)
    {
        if ((split_mode == _AS_9SPLIT_) && (type != 1))
        {
            Ad_show_custormer_logo(8, true);
        }
    }
#endif
    return 0;

}

int CHiAvDeviceMaster::Ad_start_video_output(av_output_type_t output, int index, int type, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], int margin[4]/* = NULL*/)
{
    DEBUG_MESSAGE( "CHiAvDeviceMaster::Ad_start_video_output(output:%d)(index:%d)(type:%d)(split_mode:%d)\n", output, index, type, split_mode);

    std::map<av_output_type_t, std::map<int, vo_chn_state_info_t> >::iterator vo_chn_state_it = m_vo_chn_state.find(output);

    if(!((vo_chn_state_it == m_vo_chn_state.end()) || (vo_chn_state_it->second.find(index) == vo_chn_state_it->second.end()) || (m_vo_chn_state[output][index].split_mode == _AS_UNKNOWN_)))
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_video_output WARNING: You must stop current video output first output=%d index=%d mode=%d\n"
            , output, index, m_vo_chn_state[output][index].split_mode);
        return -1;
    }

    int szMargin[4];
    /*check the parameter*/
    if(margin != NULL)
    {
        for(int i = 0; i < 4; i ++)
        {
            szMargin[i] = margin[i];
            if(!((margin[i] >= 0) && (margin[i] <= 100)))
            {
                DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_video_output INVALID MARGIN PARAMETER(%d), Set margin to 0\n", margin[i]);
                //margin[i] = 0;
            }
        }
    }
    else
    {
        for( int i = 0; i < 4; i++ ) //for playback
        {
            szMargin[i] = m_vo_chn_state[output][index].margin[i];
        }
    }

    m_vo_chn_state[output][index].split_mode = _AS_UNKNOWN_;
    for(int i = 0; i < _AV_SPLIT_MAX_CHN_; i ++)
    {
        m_vo_chn_state[output][index].set_vi_same_as_source[i] = false;
    }
    DEBUG_CRITICAL( "CHiAvDeviceMaster::Ad_start_video_output(output:%d)(index:%d)(type:%d)(split_mode:%d)\n", output, index, type, split_mode);
    
    /*start video output*/
    int ret_val = -1;
#if (defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_HDVR_)) && defined(_AV_HAVE_VIDEO_DECODER_)
    if (type == 0)
    {
        eVDecPurpose purpose = VDP_PREVIEW_VDEC;   
        if( _AOT_SPOT_ == output)
        {
            purpose = VDP_SPOT_VDEC;
        }
        m_pHi_AvDec->Ade_pre_init_decoder_param(purpose, chn_layout);
        //Ad_start_preview_dec(output, index, split_mode, chn_layout);
    }
#endif

    ret_val = Ad_start_video_output_V1(output, index, type, split_mode, chn_layout, szMargin);  //! start preview dec only when this succeed
    if(ret_val == 0)
    {
        m_vo_chn_state[output][index].vo_type = type;

        m_vo_chn_state[output][index].split_mode = split_mode;
        m_vo_chn_state[output][index].cur_split_mode = split_mode;        

        for(int i = 0; i < _AV_SPLIT_MAX_CHN_; i ++)
        {
            m_vo_chn_state[output][index].chn_layout[i] = chn_layout[i];
            m_vo_chn_state[output][index].cur_chn_layout[i] = chn_layout[i];
        }

        if( margin != NULL )
        {
            for( int i=0; i<4; i++ )
            {
                m_vo_chn_state[output][index].margin[i] = margin[i];
            }
        }

#if (defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_HDVR_)) && defined(_AV_HAVE_VIDEO_DECODER_)
        if (type == 0)
        {
            Ad_start_preview_dec(output, index, split_mode, chn_layout);
        }
#endif

    }
    else
    {
         DEBUG_MESSAGE("CHiAvDeviceMaster::Ad_start_video_output Ad_start_video_output_V1 failed result=%d type=%d split=%d out=%d ind=%d\n"
        , ret_val, type, split_mode, output, index);
    }

    return ret_val;
}

int CHiAvDeviceMaster::Ad_start_selected_video_output(av_output_type_t output, int index, int type, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], unsigned long long int chn_mask, bool brelate_decoder, int margin[4])
{
    DEBUG_MESSAGE( "CHiAvDeviceMaster::Ad_start_selected_video_output(output:%d)(index:%d)(type:%d)(split_mode:%d)\n", output, index, type, split_mode);

    int szMargin[4];
    /*check the parameter*/
    if(margin != NULL)
    {
        for(int i = 0; i < 4; i ++)
        {
            szMargin[i] = margin[i];
            if(!((margin[i] >= 0) && (margin[i] <= 100)))
            {
                DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_selected_video_output INVALID MARGIN PARAMETER(%d)\n", margin[i]);
                margin[i] = 0;
            }
        }
    }
    else
    {
        for( int i = 0; i < 4; i++ ) //for playback
        {
            szMargin[i] = m_vo_chn_state[output][index].margin[i];
        }
    }

    for(int i = 0; i < _AV_SPLIT_MAX_CHN_; i ++)
    {
        m_vo_chn_state[output][index].set_vi_same_as_source[i] = false;
    }
    
    /*start video output*/
    int ret_val = -1;

#if (defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_HDVR_)) && defined(_AV_HAVE_VIDEO_DECODER_)
    if (type == 0 && brelate_decoder)
    {
        eVDecPurpose purpose = VDP_PREVIEW_VDEC;   
        if( _AOT_SPOT_ == output)
        {
            purpose = VDP_SPOT_VDEC;
        }
        m_pHi_AvDec->Ade_pre_init_decoder_param(purpose, chn_layout);

        //Ad_start_preview_dec(output, index, split_mode, chn_layout);
    }
#endif

    switch(pgAvDeviceInfo->Adi_product_type())
    {
        default:/*main output: vga&hdmi, cvbs(write-back); spot output: cvbs*/
            ret_val = Ad_start_selected_video_output_V1(output, index, type, split_mode, chn_layout, chn_mask, szMargin);
            break;
    }

    if(ret_val == 0)
    {
        if( margin != NULL )
        {
            for( int i=0; i<4; i++ )
            {
                m_vo_chn_state[output][index].margin[i] = margin[i];
            }
        }

#if (defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_HDVR_)) && defined(_AV_HAVE_VIDEO_DECODER_)
        if (type == 0 && brelate_decoder)
        {
            printf("Ad_start_preview_dec index: %d \n", index);
            Ad_start_preview_dec(output, index, split_mode, chn_layout);
        }
#endif

    }
    else
    {
         DEBUG_MESSAGE("CHiAvDeviceMaster::Ad_start_selected_video_output Ad_start_video_output_V1 failed result=%d type=%d split=%d out=%d ind=%d\n"
        , ret_val, type, split_mode, output, index);
    }

    return ret_val;
}

int CHiAvDeviceMaster::Ad_output_source_control_V1(av_output_type_t output, int index, int layout_chn, int phy_chn, int control_flag)
{
    int type = m_vo_chn_state[output][index].vo_type;

#if defined(_AV_HISILICON_VPSS_)
    vpss_purpose_t vpss_purpose;
    Ad_get_vpss_purpose(output, index, type, phy_chn, vpss_purpose, false);
  //  DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_source_control_V1 (output:%d) (index:%d)(vpss_purpose:%d)(layout_chn:%d)(phy_chn %d)\n", output, index, vpss_purpose, layout_chn, phy_chn);
    if(!m_video_preview_switch.test(phy_chn))
    {
        Ad_show_access_logo(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), layout_chn, true);
        DEBUG_ERROR( "source control: chn %d have not preview!\n", phy_chn);
        return 0;
    }
    if ((_VP_PREVIEW_VI_ == vpss_purpose) || (_VP_SPOT_VI_ == vpss_purpose) 
        || (_VP_PREVIEW_VDEC_ == vpss_purpose) || (_VP_PREVIEW_VDEC_ == vpss_purpose)
        //|| (_VP_PLAYBACK_VDEC_ == vpss_purpose))  
        || (_VP_PLAYBACK_VDEC_ == vpss_purpose) || (_VP_SPOT_VDEC_ == vpss_purpose))  //for spot , will dynamic change vpss w and h
    {
        return Ad_control_vpss_vo(output, index, vpss_purpose, phy_chn, layout_chn, control_flag);
    }
#endif
    return 0;
}


int CHiAvDeviceMaster::Ad_output_source_control(av_output_type_t output, int index, int layout_chn, int phy_chn, int control_flag)
{
    std::map<av_output_type_t, std::map<int, vo_chn_state_info_t> >::iterator vo_chn_state_it = m_vo_chn_state.find(output);

    /*if (!((vo_chn_state_it == m_vo_chn_state.end()) || (vo_chn_state_it->second.find(index) == vo_chn_state_it->second.end()) || (m_vo_chn_state[output][index].split_mode == _AS_UNKNOWN_)))
    {
        if (0 == control_flag)
        {
        }
    }*/
    
    if ((vo_chn_state_it == m_vo_chn_state.end()) || (vo_chn_state_it->second.find(index) == vo_chn_state_it->second.end()) || (m_vo_chn_state[output][index].split_mode == _AS_UNKNOWN_))
    {
        int ret_val = -1;
        if (control_flag)
        {
            ret_val = 0;
             DEBUG_WARNING( "aa CHiAvDeviceMaster::Ad_output_source_control WARNING: There is no video output (output:%d) (index:%d) (control_flag:%d)\n", output, index, control_flag);
            return ret_val;
        }
        else
        {
            DEBUG_WARNING( "bb CHiAvDeviceMaster::Ad_output_source_control WARNING: There is no video output (output:%d) (index:%d) (control_flag:%d)\n", output, index, control_flag);
        }
    }

    if (m_vo_chn_state[output][index].split_mode == _AS_UNKNOWN_)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_source_control invalid parameter(%d)(%d)\n", layout_chn, m_vo_chn_state[output][index].split_mode);
        return -1;
    }
    
    if((layout_chn < 0) || (layout_chn >= pgAvDeviceInfo->Adi_get_split_chn_info(m_vo_chn_state[output][index].split_mode)))
    {
        //DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_source_control invalid parameter(%d)(%d)\n", layout_chn, pgAvDeviceInfo->Adi_get_split_chn_info(m_vo_chn_state[output][index].split_mode));
        return -1;
    }

    if(control_flag == 0)
    {
        if(m_vo_chn_state[output][index].chn_layout[layout_chn] != -1)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_source_control You must disconnect first\n");
            return -1;
        }
    }
    else
    {
        if(m_vo_chn_state[output][index].chn_layout[layout_chn] == -1)
        {
            return 0;
        }
    }

    int ret_val = -1;
    /*main output: vga&hdmi, cvbs(write-back); spot output: cvbs*/
    ret_val = Ad_output_source_control_V1(output, index, layout_chn, phy_chn, control_flag);
    if(ret_val == 0)
    {
        if(control_flag == 0)
        {
            m_vo_chn_state[output][index].chn_layout[layout_chn] = phy_chn;
        }
        else
        {
            m_vo_chn_state[output][index].chn_layout[layout_chn] = -1;
        }
    }

    return ret_val;
}


int CHiAvDeviceMaster::Ad_output_connect_source(av_output_type_t output, int index, int layout_chn, int phy_chn)
{
    return Ad_output_source_control(output, index, layout_chn, phy_chn, 0);
}


int CHiAvDeviceMaster::Ad_output_disconnect_source(av_output_type_t output, int index, int layout_chn, int phy_chn)
{
    return Ad_output_source_control(output, index, layout_chn, phy_chn, 1);
}


int CHiAvDeviceMaster::Ad_stop_video_output_V1(av_output_type_t output, int index, int type, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_])
{
    int max_vo_chn_num = pgAvDeviceInfo->Adi_get_split_chn_info(split_mode);
    int max_video_chn_num = pgAvDeviceInfo->Adi_max_channel_number();
    int chn_index = -1;
#if defined(_AV_HISILICON_VPSS_)
    vpss_purpose_t vpss_purpose;
#endif
    for(chn_index = 0; chn_index < max_vo_chn_num && chn_index<max_video_chn_num; chn_index ++)
    {
        if(chn_layout[chn_index] < 0)
        {
            continue;
        }
        if(!m_video_preview_switch.test(chn_layout[chn_index]))
        {
            Ad_show_access_logo(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), chn_index, true);
            DEBUG_ERROR( "stop all: chn %d have not preview!\n", chn_layout[chn_index]);
            continue;
        }

#if defined(_AV_HISILICON_VPSS_)
        Ad_get_vpss_purpose(output, index, type, chn_layout[chn_index], vpss_purpose, false);

        if ((_VP_PREVIEW_VI_ == vpss_purpose) || (_VP_SPOT_VI_ == vpss_purpose)
            || (_VP_PREVIEW_VDEC_ == vpss_purpose) || (_VP_SPOT_VDEC_ == vpss_purpose)
            || (_VP_PLAYBACK_VDEC_ == vpss_purpose))
        {
            Ad_control_vpss_vo(output, index, vpss_purpose, chn_layout[chn_index], chn_index, 1);
        }

        if((vpss_purpose == _VP_PREVIEW_VI_) || (vpss_purpose == _VP_SPOT_VI_))/*preview*/
        {
#if defined(_AV_HAVE_VIDEO_INPUT_)
            if(m_vo_chn_state[output][index].set_vi_same_as_source[chn_index] == true)
            {
                m_pHi_vi->HiVi_preview_control(chn_layout[chn_index], false);
                m_vo_chn_state[output][index].set_vi_same_as_source[chn_index] = false;
            }
#endif
        }
#endif
    }

#if defined(_AV_HISILICON_VPSS_) && defined(_AV_HAVE_VIDEO_OUTPUT_) 
    if (pgAvDeviceInfo->Adi_max_channel_number() == 8)
    {
        Ad_show_custormer_logo(8, false);
    }
#endif
    /*destory vo channel layout accord to mannual*/
    if(Ad_destory_vo_layout(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), split_mode) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_video_output_V1 Ad_destory_vo_layout FAILED\n");
        return -1;
    }

    return 0;
}

int CHiAvDeviceMaster::Ad_stop_selected_video_output_V1(av_output_type_t output, int index, int type, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], unsigned long long int chn_mask)
{
    int max_vo_chn_num = pgAvDeviceInfo->Adi_get_split_chn_info(split_mode);
    int max_video_chn_num = pgAvDeviceInfo->Adi_max_channel_number();
    int chn_index = -1;
    /*ADD BY WANGBIN*/
#if defined(_AV_HISILICON_VPSS_)
    vpss_purpose_t vpss_purpose;
#endif

    for(chn_index = 0; chn_index < max_vo_chn_num && chn_index < max_video_chn_num; chn_index++)
    {
        if(chn_layout[chn_index] < 0)
        {
            continue;
        }
        /*chn_layout[chn_index]:: chn_index represents vo_chn, value represents phy_chn*/
        if(0 != ((chn_mask) & (1ll<<(chn_layout[chn_index]))))
        {
            if(m_pHi_vo->HiVo_stop_vo_chn(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), chn_index) != 0)
            {
                DEBUG_ERROR( "CHiAvDeviceMaster::Ad_destory_vo_layout m_pHi_vo->HiVo_stop_vo_chn FAILED\n");
                return -1;
            }
            if(!m_video_preview_switch.test(chn_layout[chn_index]))
            {
                Ad_show_access_logo(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), chn_index, true);
                DEBUG_ERROR( "stop all: chn %d have not preview!\n", chn_layout[chn_index]);
                continue;
            }
#if defined(_AV_HISILICON_VPSS_)
            Ad_get_vpss_purpose(output, index, type, chn_layout[chn_index], vpss_purpose, false);

            if ((_VP_PREVIEW_VI_ == vpss_purpose) || (_VP_SPOT_VI_ == vpss_purpose)
                    || (_VP_PREVIEW_VDEC_ == vpss_purpose) || (_VP_SPOT_VDEC_ == vpss_purpose)
                    || (_VP_PLAYBACK_VDEC_ == vpss_purpose))
            {
                Ad_control_vpss_vo(output, index, vpss_purpose, chn_layout[chn_index], chn_index, 1);
            }
            
            if((vpss_purpose == _VP_PREVIEW_VI_) || (vpss_purpose == _VP_SPOT_VI_))/*preview*/
            {
#if defined(_AV_HAVE_VIDEO_INPUT_)
                if(m_vo_chn_state[output][index].set_vi_same_as_source[chn_index] == true)
                {
                    m_pHi_vi->HiVi_preview_control(chn_layout[chn_index], false);
                    m_vo_chn_state[output][index].set_vi_same_as_source[chn_index] = false;
                }
#endif
            }
#endif
        }

    }

#if defined(_AV_HISILICON_VPSS_) && defined(_AV_HAVE_VIDEO_OUTPUT_) 
    if (pgAvDeviceInfo->Adi_max_channel_number() == 8)
    {
        Ad_show_custormer_logo(8, false);
    }
#endif

    return 0;    
}

int CHiAvDeviceMaster::Ad_stop_video_output(av_output_type_t output, int index, int type)
{
    std::map<av_output_type_t, std::map<int, vo_chn_state_info_t> >::iterator vo_chn_state_it = m_vo_chn_state.find(output);

    if((vo_chn_state_it == m_vo_chn_state.end()) || (vo_chn_state_it->second.find(index) == vo_chn_state_it->second.end()) || (m_vo_chn_state[output][index].split_mode == _AS_UNKNOWN_))
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_video_output WARNING: There is no video output output=%d index=%d type=%d\n", output, index, type);
        return 0;
    }

    if (m_vo_chn_state[output][index].split_mode == _AS_UNKNOWN_)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_video_output WARNING: split_mode %d error output=%d index=%d type=%d\n", m_vo_chn_state[output][index].split_mode, output, index, type);
        return 0;
    }
    
    DEBUG_CRITICAL( "CHiAvDeviceMaster::Ad_stop_video_output(output:%d)(index:%d)(type:%d)(split_mode:%d)\n", output, index, type, m_vo_chn_state[output][index].split_mode);

    int ret_val = -1;
    
#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)    
    if (0 == type)
    {
        Ad_stop_preview_dec(output, index);
    }
#endif

    /*main output: vga&hdmi, cvbs(write-back); spot output: cvbs*/
    ret_val = Ad_stop_video_output_V1(output, index, type, m_vo_chn_state[output][index].split_mode, m_vo_chn_state[output][index].chn_layout);
    if(ret_val == 0)
    {
        m_vo_chn_state[output][index].split_mode = _AS_UNKNOWN_;

        m_vo_chn_state[output][index].cur_split_mode = _AS_UNKNOWN_;        

        for(int i = 0; i < _AV_SPLIT_MAX_CHN_; i ++)
        {
            m_vo_chn_state[output][index].chn_layout[i] = -1;
            m_vo_chn_state[output][index].cur_chn_layout[i] = -1;
        }
    }

    return ret_val;
}

int CHiAvDeviceMaster::Ad_stop_selected_video_output(av_output_type_t output, int index, unsigned long long int chn_mask, int type, bool brelate_decoder)
{
    std::map<av_output_type_t, std::map<int, vo_chn_state_info_t> >::iterator vo_chn_state_it = m_vo_chn_state.find(output);

    if((vo_chn_state_it == m_vo_chn_state.end()) || (vo_chn_state_it->second.find(index) == vo_chn_state_it->second.end()))
    {
        DEBUG_WARNING( "CHiAvDeviceMaster::Ad_stop_selected_video_output WARNING: There is no video output output=%d index=%d type=%d\n", output, index, type);
        return 0;
    }

    if (m_vo_chn_state[output][index].split_mode == _AS_UNKNOWN_)
    {
        DEBUG_WARNING( "CHiAvDeviceMaster::Ad_stop_selected_video_output WARNING: split_mode %d error output=%d index=%d type=%d\n", m_vo_chn_state[output][index].split_mode, output, index, type);
        return 0;
    }
    
    DEBUG_MESSAGE( "CHiAvDeviceMaster::Ad_stop_selected_video_output(output:%d)(index:%d)(type:%d)(split_mode:%d)\n", output, index, type, m_vo_chn_state[output][index].split_mode);

    int ret_val = -1;

#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)    
    if (0 == type && brelate_decoder)
    {
        Ad_stop_preview_dec(output, index);
    }
#endif

    switch(pgAvDeviceInfo->Adi_product_type())
    {
        default:/*main output: vga&hdmi, cvbs(write-back); spot output: cvbs*/
            ret_val = Ad_stop_selected_video_output_V1(output, index, type, m_vo_chn_state[output][index].split_mode, m_vo_chn_state[output][index].chn_layout, chn_mask);
            break;
    }

    return ret_val;
}

int CHiAvDeviceMaster::Ad_start_preview_output(av_output_type_t output, int index, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], int margin[4]/* = NULL*/)
{
    return Ad_start_video_output(output, index, 0, split_mode, chn_layout, margin);
}

int CHiAvDeviceMaster::Ad_start_selected_preview_output(av_output_type_t output, int index, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], unsigned long long int chn_mask, bool brelate_decoder, int margin[4])
{
    return Ad_start_selected_video_output(output, index, 0, split_mode, chn_layout, chn_mask, brelate_decoder, margin);
}

int CHiAvDeviceMaster::Ad_stop_preview_output(av_output_type_t output, int index)
{
    return Ad_stop_video_output(output, index, 0);
}

int CHiAvDeviceMaster::Ad_stop_preview_Dec(av_output_type_t output, int index)
{
    return Ad_stop_preview_dec(output, index);
}
int CHiAvDeviceMaster::Ad_set_preview_dec_start_flag(bool isStartFlag)
{
    return m_pHi_AvDec->Ade_set_start_send_data_flag(isStartFlag);
    
}


int CHiAvDeviceMaster::Ad_stop_selected_preview_output(av_output_type_t output, int index, unsigned long long int chn_mask, bool brelate_decoder)
{
    return Ad_stop_selected_video_output(output, index, chn_mask, 0, brelate_decoder);
}

int CHiAvDeviceMaster::Ad_start_playback_output(av_output_type_t output, int index, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], int margin[4]/* = NULL*/)
{
    DEBUG_CRITICAL("split_mode =%d index =%d split_mode =%d\n",split_mode, index, split_mode);
    
    return Ad_start_video_output(output, index, 1, split_mode, chn_layout, margin);
}


int CHiAvDeviceMaster::Ad_stop_playback_output(av_output_type_t output, int index)
{
    return Ad_stop_video_output(output, index, 1);
}


int CHiAvDeviceMaster::Ad_switch_output_V1(av_output_type_t output, int index, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_])
{
    int vo_type = m_vo_chn_state[output][index].vo_type;
    int ret_val = -1;
    int vo_chn = 0;
    int chn_index = 0;
    VO_CHN_ATTR_S vo_chn_attr;
    av_rect_t video_rect;
    av_rect_t vo_chn_rect;
    VO_VIDEO_LAYER_ATTR_S video_layer_attr;
    av_vag_hdmi_resolution_t resolution =  Ad_get_resolution(output,index);
    int adjust_margin[4];
    memcpy(adjust_margin, m_vo_chn_state[output][index].margin, sizeof(adjust_margin));
    
    pgAvDeviceInfo->Adi_get_cur_display_chn(chn_layout);
    
    switch(vo_type)
    {
        case 0:/*preview*/
            goto _STOP_START_;
            break;
        case 1:/*playback*/
            goto _SWITCH_DECODER_;
            break;
        default:
            _AV_FATAL_(1, "CHiAvDeviceMaster::Ad_switch_output_V1 You must give the implement(%d)\n", m_vo_chn_state[output][index].vo_type);
            break;
    }

_SWITCH_DECODER_:
    if(pgAvDeviceInfo->Adi_get_split_chn_info(split_mode) > pgAvDeviceInfo->Adi_get_split_chn_info(m_vo_chn_state[output][index].split_mode))
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_switch_output_V1 invalid split mode(%d)(%d)\n", pgAvDeviceInfo->Adi_get_split_chn_info(split_mode), pgAvDeviceInfo->Adi_get_split_chn_info(m_vo_chn_state[output][index].split_mode));
        goto _OVER_;
    }

    /*Hide all channel*/
    for(vo_chn = 0; vo_chn < pgAvDeviceInfo->Adi_get_split_chn_info(m_vo_chn_state[output][index].split_mode); vo_chn ++)
    {
        if(m_pHi_vo->HiVo_hide_chn(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), vo_chn) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_switch_output_V1 m_pHi_vo->HiVo_hide_chn FAILED\n");
            goto _OVER_;
        }
    }

    /*calculate display rectangle*/
    if(m_pHi_vo->HiVo_get_video_layer_attr(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), &video_layer_attr) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_switch_output_V1 m_pHi_vo->HiVo_get_video_layer_attr FAILED\n");
        goto _OVER_;
    }

    Ad_calculate_video_rect_V2(video_layer_attr.stImageSize.u32Width, video_layer_attr.stImageSize.u32Height, adjust_margin, &video_rect, resolution);

    /*modify vo layout*/
    for(chn_index = 0; chn_index < pgAvDeviceInfo->Adi_get_split_chn_info(split_mode); chn_index ++)
    {
        if((chn_layout[chn_index] == -1) || ((vo_chn = Ad_get_vo_chn(output, index, chn_layout[chn_index])) < 0))
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_switch_output_V1 Invalid index =%d chn(%d) continue...\n", chn_index, chn_layout[chn_index]);
            //goto _OVER_;
            continue;
        }

        memset(&vo_chn_attr, 0, sizeof(VO_CHN_ATTR_S));

        vo_chn_attr.u32Priority = 0;
        vo_chn_attr.bDeflicker = HI_FALSE;
        pgAvDeviceInfo->Adi_get_split_chn_rect(&video_rect, split_mode, chn_index, &vo_chn_rect, m_pHi_vo->HiVo_get_vo_dev(output, index, 0));
        vo_chn_attr.stRect.s32X = vo_chn_rect.x;
        vo_chn_attr.stRect.s32Y = vo_chn_rect.y;
        vo_chn_attr.stRect.u32Width = vo_chn_rect.w;
        vo_chn_attr.stRect.u32Height = vo_chn_rect.h;
#if defined(_AV_PLATFORM_HI3521_)
        vo_chn_attr.bDeflicker = HI_TRUE;
#endif

        vo_chn_attr.bDeflicker = HI_TRUE;   //!< 20160607
        
        printf("vo s32X =%d s32Y =%d u32Width =%d  u32Height =%d\n",vo_chn_attr.stRect.s32X, vo_chn_attr.stRect.s32Y, vo_chn_attr.stRect.u32Width, vo_chn_attr.stRect.u32Height);
        if(m_pHi_vo->HiVo_set_chn_attr(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), vo_chn, &vo_chn_attr) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_switch_output_V1 m_pHi_vo->HiVo_set_chn_attr FAILED\n");
            return -1;
        }

        if(m_pHi_vo->HiVo_show_chn(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), vo_chn) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_switch_output_V1 m_pHi_vo->HiVo_show_chn FAILED\n");
            goto _OVER_;
        }
    }

    ret_val = 0;
    /*current information*/
    m_vo_chn_state[output][index].cur_split_mode = split_mode;
    for(int i = 0; i < pgAvDeviceInfo->Adi_get_split_chn_info(split_mode); i ++)
    {
        m_vo_chn_state[output][index].cur_chn_layout[i] = chn_layout[i];
    }
    goto _OVER_;

_STOP_START_:   
    if(Ad_stop_video_output(output, index, m_vo_chn_state[output][index].vo_type) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_switch_output_V1 Ad_stop_video_output FAILED\n");
        goto _OVER_;
    }
    if(Ad_start_video_output(output, index, vo_type, split_mode, chn_layout, m_vo_chn_state[output][index].margin) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_switch_output_V1 Ad_start_video_output FAILED\n");
        goto _OVER_;
    }
    ret_val = 0;
    goto _OVER_;
    
_OVER_:
    return ret_val;
}

int CHiAvDeviceMaster::Ad_switch_output(av_output_type_t output, int index, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], bool bswitch_channel)
{
    std::map<av_output_type_t, std::map<int, vo_chn_state_info_t> >::iterator vo_chn_state_it = m_vo_chn_state.find(output);

    if((vo_chn_state_it == m_vo_chn_state.end()) || (vo_chn_state_it->second.find(index) == vo_chn_state_it->second.end()) || (m_vo_chn_state[output][index].split_mode == _AS_UNKNOWN_))
    {
        DEBUG_WARNING( "CHiAvDeviceMaster::Ad_switch_output WARNING: There is no video output\n");
        return 0;
    }
    
#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
    if( output != _AOT_SPOT_ && m_pHi_AvDec )
    {
        av_tvsystem_t tv_system = Ad_get_tv_system(output, index);
        
        m_pHi_AvDec->Ade_set_dec_dynamic_fr( VDP_PLAYBACK_VDEC, tv_system, split_mode, chn_layout, bswitch_channel);
    }
#endif

    /*main output: vga&hdmi, cvbs(write-back); spot output: cvbs*/
    return Ad_switch_output_V1(output, index, split_mode, chn_layout);
}


/*拖动实现方法:拖动并不更改VO通道与数据源的绑定关系，仅仅更改VO通道的位置属性*/
int CHiAvDeviceMaster::Ad_output_drag_chn_V1(av_output_type_t output, int index, int chn_layout[_AV_SPLIT_MAX_CHN_])
{
    int chn_index = 0;
    int vo_chn = 0;
    int ret_val = -1;
    VO_CHN_ATTR_S vo_chn_attr;
    av_rect_t video_rect;
    av_rect_t vo_chn_rect;
    VO_VIDEO_LAYER_ATTR_S video_layer_attr;
    VO_DEV vo_dev = m_pHi_vo->HiVo_get_vo_dev(output, index, 0);
    
    if(  m_pHi_vo->HiVo_set_attr_begin(vo_dev) < 0)
    {                
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_drag_chn_V1 HiVo_set_attr_begin vo_dev %d failed\n", vo_dev);
        //return -1;
    }
    
    /*hide all different channels*/
    for(chn_index = 0; chn_index < pgAvDeviceInfo->Adi_get_split_chn_info(m_vo_chn_state[output][index].cur_split_mode); chn_index ++)
    {
        if(chn_layout[chn_index] == -1)
        {
            continue;
        }

        if(chn_layout[chn_index] != m_vo_chn_state[output][index].cur_chn_layout[chn_index])
        {
            /*为chn_layout[chn_index]空出位置来*/
            if((vo_chn = Ad_get_vo_chn(output, index, m_vo_chn_state[output][index].cur_chn_layout[chn_index])) < 0)
            {
                DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_drag_chn_V1 Invalid chn(%d)\n", m_vo_chn_state[output][index].cur_chn_layout[chn_index]);
                goto _OVER_;
            }

            if(m_pHi_vo->HiVo_hide_chn(vo_dev, vo_chn) != 0)
            {
                DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_drag_chn_V1 m_pHi_vo->HiVo_hide_chn FAILED\n");
                goto _OVER_;
            }
        }        
    }

    /*calculate display rectangle*/
    if(m_pHi_vo->HiVo_get_video_layer_attr(vo_dev, &video_layer_attr) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_drag_chn_V1 m_pHi_vo->HiVo_get_video_layer_attr FAILED\n");
        goto _OVER_;
    }
    Ad_calculate_video_rect(video_layer_attr.stImageSize.u32Width, video_layer_attr.stImageSize.u32Height, m_vo_chn_state[output][index].margin, &video_rect);

    /*show all different channels*/   
    for(chn_index = 0; chn_index < pgAvDeviceInfo->Adi_get_split_chn_info(m_vo_chn_state[output][index].cur_split_mode); chn_index ++)
    {
        if(chn_layout[chn_index] == -1)
        {
            continue;
        }

        if(chn_layout[chn_index] != m_vo_chn_state[output][index].cur_chn_layout[chn_index])
        {
            if((vo_chn = Ad_get_vo_chn(output, index, chn_layout[chn_index])) < 0)
            {
                DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_drag_chn_V1 Invalid chn(%d)\n", chn_layout[chn_index]);
                goto _OVER_;
            }

            memset(&vo_chn_attr, 0, sizeof(VO_CHN_ATTR_S));

            vo_chn_attr.u32Priority = 0;
            vo_chn_attr.bDeflicker = HI_TRUE; //!<20160607
            pgAvDeviceInfo->Adi_get_split_chn_rect(&video_rect, m_vo_chn_state[output][index].cur_split_mode, chn_index, &vo_chn_rect, vo_dev);
            vo_chn_attr.stRect.s32X = vo_chn_rect.x;
            vo_chn_attr.stRect.s32Y = vo_chn_rect.y;
            vo_chn_attr.stRect.u32Width = vo_chn_rect.w;
            vo_chn_attr.stRect.u32Height = vo_chn_rect.h;

            if(m_pHi_vo->HiVo_set_chn_attr(vo_dev, vo_chn, &vo_chn_attr) != 0)
            {
                DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_drag_chn_V1 m_pHi_vo->HiVo_set_chn_attr FAILED\n");
                return -1;
            }
            
            if(m_pHi_vo->HiVo_show_chn(vo_dev, vo_chn) != 0)
            {
                DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_drag_chn_V1 m_pHi_vo->HiVo_show_chn FAILED\n");
                goto _OVER_;
            }

            m_vo_chn_state[output][index].cur_chn_layout[chn_index] = chn_layout[chn_index];
        }
    }
   
    ret_val = 0;
_OVER_:
    
    if(m_pHi_vo->HiVo_set_attr_end(vo_dev) < 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_drag_chn_V1 HiVo_set_attr_end vo_dev %d failed\n", vo_dev);
    }
    return ret_val;   
}



/*拖动实现方法:拖动并不更改VO通道与数据源的绑定关系，更改VO通道的位置属性*/
int CHiAvDeviceMaster::Ad_output_drag_chn_V2(av_output_type_t output, int index, int chn_layout[_AV_SPLIT_MAX_CHN_])
{
    int chn_index = 0;
    int vo_chn = 0;
    int ret_val = -1;
    VO_CHN_ATTR_S vo_chn_attr;
    av_rect_t video_rect;
    av_rect_t vo_chn_rect;
    VO_VIDEO_LAYER_ATTR_S video_layer_attr;
    av_vag_hdmi_resolution_t resolution = Ad_get_resolution(output, index);


    VO_DEV vo_dev = m_pHi_vo->HiVo_get_vo_dev(output, index, 0);
    
    if(  m_pHi_vo->HiVo_set_attr_begin(vo_dev) < 0)
    {                
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_drag_chn_V2 HiVo_set_attr_begin vo_dev %d failed\n", vo_dev);
        //return -1;
    }

    /*Disable all channels*/
    for(chn_index = 0; chn_index < pgAvDeviceInfo->Adi_get_split_chn_info(m_vo_chn_state[output][index].cur_split_mode); chn_index ++)
    {
        if(m_vo_chn_state[output][index].cur_chn_layout[chn_index] == -1)
        {
            continue;
        }
        
        if((vo_chn = Ad_get_vo_chn(output, index, m_vo_chn_state[output][index].cur_chn_layout[chn_index])) < 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_drag_chn_V2 Invalid chn(%d)\n", m_vo_chn_state[output][index].cur_chn_layout[chn_index]);
            goto _OVER_;
        }

        if(m_pHi_vo->HiVo_stop_vo_chn(vo_dev, vo_chn) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_destory_vo_layout m_pHi_vo->HiVo_stop_vo_chn FAILED\n");
            return -1;
        }

    }
    memset(&video_layer_attr, 0, sizeof(VO_VIDEO_LAYER_ATTR_S));
    /*calculate display rectangle*/
    if(m_pHi_vo->HiVo_get_video_layer_attr(vo_dev, &video_layer_attr) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_drag_chn_V2 m_pHi_vo->HiVo_get_video_layer_attr FAILED\n");
        goto _OVER_;
    }

    Ad_calculate_video_rect_V2(video_layer_attr.stImageSize.u32Width, video_layer_attr.stImageSize.u32Height, m_vo_chn_state[output][index].margin, &video_rect,resolution);

    /*set different channels pos*/ 
    for(chn_index = 0; chn_index < pgAvDeviceInfo->Adi_get_split_chn_info(m_vo_chn_state[output][index].cur_split_mode); chn_index ++)
    {
        if(chn_layout[chn_index] == -1)
        {
            continue;
        }

        if(chn_layout[chn_index] != m_vo_chn_state[output][index].cur_chn_layout[chn_index])
        {
            if((vo_chn = Ad_get_vo_chn(output, index, chn_layout[chn_index])) < 0)
            {
                DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_drag_chn_V2 Invalid chn(%d)\n", chn_layout[chn_index]);
                goto _OVER_;
            }

            memset(&vo_chn_attr, 0, sizeof(VO_CHN_ATTR_S));

            vo_chn_attr.u32Priority = 0;
            vo_chn_attr.bDeflicker = HI_TRUE;//!<20160607
            pgAvDeviceInfo->Adi_get_split_chn_rect(&video_rect, m_vo_chn_state[output][index].cur_split_mode, chn_index, &vo_chn_rect, vo_dev);
            vo_chn_attr.stRect.s32X = vo_chn_rect.x;
            vo_chn_attr.stRect.s32Y = vo_chn_rect.y;
            vo_chn_attr.stRect.u32Width = vo_chn_rect.w;
            vo_chn_attr.stRect.u32Height = vo_chn_rect.h;

            if(m_pHi_vo->HiVo_set_chn_attr(vo_dev, vo_chn, &vo_chn_attr) != 0)
            {
                DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_drag_chn_V2 m_pHi_vo->HiVo_set_chn_attr FAILED\n");
                return -1;
            }

            m_vo_chn_state[output][index].cur_chn_layout[chn_index] = chn_layout[chn_index];
        }
    }

   /*Eable all channels*/
    for(chn_index = 0; chn_index < pgAvDeviceInfo->Adi_get_split_chn_info(m_vo_chn_state[output][index].cur_split_mode); chn_index ++)
    {
        if(m_vo_chn_state[output][index].cur_chn_layout[chn_index] == -1)
        {
            continue;
        }
        if((vo_chn = Ad_get_vo_chn(output, index, m_vo_chn_state[output][index].cur_chn_layout[chn_index])) < 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_drag_chn_V2 Invalid chn(%d)\n", m_vo_chn_state[output][index].cur_chn_layout[chn_index]);
            goto _OVER_;
        }
        if((ret_val = HI_MPI_VO_EnableChn(vo_dev, vo_chn)) != HI_SUCCESS)
        {
            DEBUG_ERROR( "CHiVo::HiVo_stop_vo_chn HI_MPI_VO_DisableChn FAILED(%d)(0x%08lx)\n", vo_dev, (unsigned long)ret_val);
            return -1;
        }
    }
    
    ret_val = 0;
_OVER_:
    
    if(m_pHi_vo->HiVo_set_attr_end(vo_dev) < 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_drag_chn_V2 HiVo_set_attr_end vo_dev %d failed\n", vo_dev);
    }
    return ret_val;   
}



int CHiAvDeviceMaster::Ad_output_drag_chn(av_output_type_t output, int index, int chn_layout[_AV_SPLIT_MAX_CHN_])
{
    std::map<av_output_type_t, std::map<int, vo_chn_state_info_t> >::iterator vo_chn_state_it = m_vo_chn_state.find(output);
    int max_index = pgAvDeviceInfo->Adi_get_split_chn_info(m_vo_chn_state[output][index].split_mode);
    if((vo_chn_state_it == m_vo_chn_state.end()) || (vo_chn_state_it->second.find(index) == vo_chn_state_it->second.end()) || (m_vo_chn_state[output][index].split_mode == _AS_UNKNOWN_))
    {
        DEBUG_WARNING( "CHiAvDeviceMaster::Ad_output_drag_chn WARNING: There is no video output\n");
        return 0;
    }

    if (max_index > pgAvDeviceInfo->Adi_max_channel_number())
    {
        max_index = pgAvDeviceInfo->Adi_max_channel_number();
    }
    
    for(int chn_index = 0; chn_index < max_index; chn_index ++)
    {
        if(((chn_layout[chn_index] == -1) || (m_vo_chn_state[output][index].cur_chn_layout[chn_index] == -1)) && 
                                            (chn_layout[chn_index] != m_vo_chn_state[output][index].cur_chn_layout[chn_index]))
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_drag_chn INVALID PARAMETER(%d)(%d)\n", chn_layout[chn_index], m_vo_chn_state[output][index].cur_chn_layout[chn_index]);
            return -1;
        }
    }

    int ret_val = -1;
    if(0)
    {
        ret_val = Ad_output_drag_chn_V1(output, index, chn_layout);
    }
    else
    {
        ret_val = Ad_output_drag_chn_V2(output, index, chn_layout);
    }

    return ret_val;
}

int CHiAvDeviceMaster::Ad_output_swap_chn(av_output_type_t output, int index, int first_chn, int second_chn)
{
    int retval = -1;
    std::map<av_output_type_t, std::map<int, vo_chn_state_info_t> >::iterator vo_chn_state_it = m_vo_chn_state.find(output);
    if((vo_chn_state_it == m_vo_chn_state.end()) || (vo_chn_state_it->second.find(index) == vo_chn_state_it->second.end()) || (m_vo_chn_state[output][index].split_mode == _AS_UNKNOWN_))
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_swap_chn WARNING: There is no video output\n");
        return -1;
    }

    int max_vo_chn = pgAvDeviceInfo->Adi_get_split_chn_info(m_vo_chn_state[output][index].cur_split_mode);
    if((first_chn < 0) || (first_chn >= _AV_SPLIT_MAX_CHN_) || (second_chn < 0) || (second_chn >= _AV_SPLIT_MAX_CHN_))
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_swap_chn Invalid parameter(%d)(%d)(%d)\n", first_chn, second_chn, max_vo_chn);
        return -1;
    }
    int chn_layout[_AV_SPLIT_MAX_CHN_];
    int first_index = -1, second_index = -1;

    memset(chn_layout, -1, sizeof(int) * _AV_SPLIT_MAX_CHN_);
    for(int chn_index = 0; chn_index < max_vo_chn; chn_index ++)
    {
        chn_layout[chn_index] = m_vo_chn_state[output][index].cur_chn_layout[chn_index];
        if (first_chn == m_vo_chn_state[output][index].cur_chn_layout[chn_index])
        {
            first_index = chn_index;
        }
         
        if (second_chn == m_vo_chn_state[output][index].cur_chn_layout[chn_index])
        {
            second_index = chn_index;
        }
    }
    
    if((first_index < 0) || (first_index >= max_vo_chn) || (second_index < 0) || (second_index >= max_vo_chn))
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_output_swap_chn Invalid parameter phychn(%d,%d) vo_chn(%d,%d)(%d)\n", first_chn, second_chn, first_index, second_index, max_vo_chn);
        return -1;
    }
    DEBUG_MESSAGE( "CHiAvDeviceMaster::Ad_output_swap_chn curvolist:%d to %d\n", first_index, second_index);
    
    chn_layout[first_index] = m_vo_chn_state[output][index].cur_chn_layout[second_index];
    chn_layout[second_index] = m_vo_chn_state[output][index].cur_chn_layout[first_index];

    retval = Ad_output_drag_chn(output, index, chn_layout);
    if(retval == 0)
    {
        if(!m_video_preview_switch.test(chn_layout[first_index]))
        {
            Ad_show_access_logo(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), Ad_get_vo_chn(output, index, chn_layout[first_index]), true);
        }
        if(!m_video_preview_switch.test(chn_layout[second_index]))
        {
            Ad_show_access_logo(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), Ad_get_vo_chn(output, index, chn_layout[second_index]), true);
        }
    }
    return retval;
}

int CHiAvDeviceMaster::Ad_output_swap_chn_reset( av_output_type_t output, int index )
{
    std::map<av_output_type_t, std::map<int, vo_chn_state_info_t> >::iterator vo_chn_state_it = m_vo_chn_state.find(output);
    if((vo_chn_state_it == m_vo_chn_state.end()) || (vo_chn_state_it->second.find(index) == vo_chn_state_it->second.end()) || (m_vo_chn_state[output][index].split_mode == _AS_UNKNOWN_))
    {
        DEBUG_WARNING( "CHiAvDeviceMaster::Ad_output_swap_chn WARNING: There is no video output\n");
        return -1;
    }

    int chn_layout[_AV_SPLIT_MAX_CHN_];
    memset(chn_layout, -1, sizeof(int)*_AV_SPLIT_MAX_CHN_);
    int max_vo_chn = pgAvDeviceInfo->Adi_get_split_chn_info(m_vo_chn_state[output][index].cur_split_mode);
    for(int chn_index = 0; chn_index < max_vo_chn; chn_index ++)
    {
        chn_layout[chn_index] = chn_index;        
    }

    return Ad_output_drag_chn(output, index, chn_layout);
}

int CHiAvDeviceMaster::Ad_zoomin_screen(av_zoomin_para_t av_zoomin_para)
{
    av_output_type_t output = pgAvDeviceInfo->Adi_get_main_out_mode();
    int index = 0;
    int ret = 0;
    int videoChn = av_zoomin_para.videophychn;
    av_split_t split_mode = _AS_DIGITAL_PIP_;

    if (true == av_zoomin_para.enable)
    {        
        ret = Ad_set_pip(av_zoomin_para.enable, videoChn,  split_mode, av_zoomin_para.stuMainPipArea, av_zoomin_para.stuSlavePipArea);
        if (ret < 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_zoomin_screen Ad_set_pip FAILED\n");
            return -1;
        }
        
        ret = Ad_digital_zoom(output, index, av_zoomin_para.enable, videoChn, av_zoomin_para.stuZoomArea);
        if (ret < 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_zoomin_screen Ad_digital_zoom FAILED\n");
            return -1;
        }   
    }
    else
    {
        ret = Ad_digital_zoom(output, index, av_zoomin_para.enable, videoChn, av_zoomin_para.stuZoomArea);
        if (ret < 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_zoomin_screen Ad_digital_zoom FAILED\n");
            return -1;
        }
        
        ret = Ad_set_pip(av_zoomin_para.enable, videoChn, split_mode, av_zoomin_para.stuMainPipArea, av_zoomin_para.stuSlavePipArea);
        if (ret < 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_zoomin_screen Ad_set_pip FAILED\n");
            return -1;
        }
    }
    return 0;
}

int CHiAvDeviceMaster::Ad_set_pip_screen(av_pip_para_t av_pip_para)
{
    av_output_type_t output = pgAvDeviceInfo->Adi_get_main_out_mode();
    int index = 0;
    int ret = 0;
    int main_video_phychn = av_pip_para.video_phychn[0];
    int sub_video_phychn = av_pip_para.video_phychn[1];
    av_split_t split_mode = _AS_PIP_;
    VO_DEV vo_dev = m_pHi_vo->HiVo_get_vo_dev(output, index, 0);
    DEBUG_MESSAGE( "CHiAvDeviceMaster::Ad_set_pip_screen main chn %d, sub chn %d, enable %d\n", main_video_phychn, sub_video_phychn, av_pip_para.enable);

    if ((m_pip_main_video_phy_chn != main_video_phychn)
            || (m_pip_sub_video_phy_chn != sub_video_phychn)
            || (false == av_pip_para.enable))
    {
        if (m_pip_main_video_phy_chn >= 0)
        {
            int vo_chn = Ad_get_vo_chn(output, index, m_pip_main_video_phy_chn);
            if (vo_chn >= 0)
            {
                if(m_pHi_vo->HiVo_hide_chn(vo_dev, vo_chn) != 0)
                {
                    DEBUG_ERROR( "CHiAvDeviceMaster::Ad_set_pip_screen m_pHi_vo->HiVo_show_chn FAILED\n");
                }
            }
            else
            {            
                DEBUG_ERROR( "CHiAvDeviceMaster::Ad_set_pip_screen chn %d not bind vo\n", m_pip_main_video_phy_chn);
            }
        }

        if (m_pip_sub_video_phy_chn >= 0)
        {
            ret = Ad_set_pip(false, m_pip_sub_video_phy_chn, split_mode, av_pip_para.stuMainPipArea, av_pip_para.stuSlavePipArea);
            if (ret < 0)
            {
                DEBUG_ERROR( "CHiAvDeviceMaster::Ad_set_pip_screen Ad_set_pip FAILED\n");
                return -1;
            }
        }
    }

    if (true == av_pip_para.enable)
    {      
        bool show_vo_chn = false, reset_vo = false;
        
        /*set pip pos*/
        if ((m_pip_main_video_phy_chn != main_video_phychn)
            || (m_pip_sub_video_phy_chn != sub_video_phychn))
        {         
            if ((m_pip_main_video_phy_chn == sub_video_phychn) && (m_pip_sub_video_phy_chn == main_video_phychn))
            {
                DEBUG_WARNING( "CHiAvDeviceMaster::Ad_set_pip_screen only swap\n");
            }
            else if ((m_pip_main_video_phy_chn != main_video_phychn) || (m_pip_sub_video_phy_chn != sub_video_phychn))
            {     
                reset_vo = true;
            }

            if (true == reset_vo)
            {     
                int chn_layout[_AV_SPLIT_MAX_CHN_];
                memset(chn_layout, -1, sizeof(chn_layout));
                av_rect_t PipRect;
                
                chn_layout[0] = main_video_phychn;
                chn_layout[1] = sub_video_phychn;
                Ad_stop_preview_output( output, index );
                if(0)
                {
                    /*PIP area have been processed in the Ad_set_pip.Just create a video layer,but PIP area can not exceed the range*/
                    PipRect.x = 0;
                    PipRect.y = 0;
                    PipRect.w = 64;
                    PipRect.h = 64;
                }
                else
                {
                    memcpy(&PipRect, &av_pip_para.stuMainPipArea, sizeof(av_rect_t));
                }
                
                pgAvDeviceInfo->Adi_set_split_chn_rect_PIP(PipRect, sub_video_phychn, PipRect);
                Ad_start_preview_output( output, index, split_mode, chn_layout);/*create vo chn*/
            }
            show_vo_chn = true;
        }
        
        ret = Ad_set_pip(av_pip_para.enable, sub_video_phychn, split_mode, av_pip_para.stuMainPipArea, av_pip_para.stuSlavePipArea);
        if (ret < 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_set_pip_screen Ad_set_pip FAILED\n");
            return -1;
        }

        if (true == show_vo_chn)
        {
            for (int chn = 0; chn < 2; chn++)
            {
                int vo_chn = -1;
                if (av_pip_para.video_phychn[chn] < 0)
                {
                    continue;
                }
               
                vo_chn = Ad_get_vo_chn(output, index, av_pip_para.video_phychn[chn]);               
                if (vo_chn < 0)
                {
                    DEBUG_ERROR( "CHiAvDeviceMaster::Ad_set_pip_screen chn %d not bind vo\n", av_pip_para.video_phychn[chn]);
                    continue;
                }
                
                if(m_pHi_vo->HiVo_show_chn(vo_dev, vo_chn) != 0)
                {
                    DEBUG_ERROR( "CHiAvDeviceMaster::Ad_set_pip_screen m_pHi_vo->HiVo_show_chn FAILED\n");
                }
            }
        }
        
        m_pip_main_video_phy_chn = main_video_phychn;
        m_pip_sub_video_phy_chn = sub_video_phychn;
        
    }
    else
    {   
        m_pip_main_video_phy_chn = -1;
        m_pip_sub_video_phy_chn = -1;
    }
    DEBUG_MESSAGE( "end CHiAvDeviceMaster::Ad_set_pip_screen main chn %d, sub chn %d, enable %d\n", m_pip_main_video_phy_chn, m_pip_sub_video_phy_chn, av_pip_para.enable);
    return 0;
}

int CHiAvDeviceMaster::Ad_set_pip(bool enable, int video_phy_chnnum, av_split_t split_mode, av_rect_t av_main_pip_area, av_rect_t av_slave_pip_area)
{
    av_output_type_t output = pgAvDeviceInfo->Adi_get_main_out_mode();
    int index = 0;
    int ret = 0;
    if ((true == enable)&& ((false == m_pip_create_status) || (_AS_PIP_ == split_mode)))
    {
        ret = Ad_start_pip(output, index, split_mode, video_phy_chnnum, av_main_pip_area);
        if (ret < 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_set_pip Ad_start_pip FAILED\n");
            return -1;
        }
        m_pip_create_status = true;
    }
    else if ((false == enable) && (true == m_pip_create_status))
    {
        ret = Ad_stop_pip(output, index, video_phy_chnnum);
        if (ret < 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_set_pip Ad_stop_pip FAILED\n");
            return -1;
        }
        m_pip_create_status = false;
    }
    return ret;
}


int CHiAvDeviceMaster::Ad_start_pip(av_output_type_t output, int index, av_split_t split_mode, int video_phy_chnnum, av_rect_t pip_area)
{
    av_tvsystem_t tv_system = Ad_get_tv_system(output, index);
    int vo_type = m_vo_chn_state[output][index].vo_type;/*1-playback, 0-preview*/
    VO_DEV vo_dev = m_pHi_vo->HiVo_get_vo_dev(output, index, 0);
    VO_CHN vo_chn = Ad_get_vo_chn(output, index, video_phy_chnnum);  
    VO_VIDEO_LAYER_ATTR_S PipLayerAttr;
    VO_CHN_ATTR_S ChnAttr;
#ifdef _AV_PLATFORM_HI3535_
    bool usenewzoon = false;
#else
    bool usenewzoon = true;
#endif
    av_rect_t video_rect;   

    memset(&ChnAttr, 0, sizeof(VO_CHN_ATTR_S));
    memset(&PipLayerAttr, 0, sizeof(VO_VIDEO_LAYER_ATTR_S));

    if (vo_chn < 0)
    {           
        DEBUG_MESSAGE( "CHiAvDeviceMaster::Ad_start_pip vo_chn %d, video_phy_chnnum %d\n", vo_chn, video_phy_chnnum);

        int max_vo_chn_num = pgAvDeviceInfo->Adi_get_split_chn_info(m_vo_chn_state[output][index].split_mode);
        for(int i = 0; i < max_vo_chn_num; i ++)
        {
            DEBUG_MESSAGE( "CHiAvDeviceMaster::Ad_start_pip vo_chn %d, video_phy_chnnum %d\n", i, m_vo_chn_state[output][index].chn_layout[i]);            
        }
        return -1;
    }
    
    m_pHi_vo->HiVo_get_video_layer_attr(vo_dev, &PipLayerAttr);
    /*set pip pos*/
    pgAvDeviceInfo->Adi_set_split_chn_rect_PIP(pip_area, video_phy_chnnum, pip_area);

    if (true == usenewzoon)
    {
        m_pHi_vo->HiVo_get_video_size(tv_system, (int *)&PipLayerAttr.stImageSize.u32Width, (int *)&PipLayerAttr.stImageSize.u32Height, NULL, NULL);
    }
    
    if (false == m_pip_create_status)
    {
        m_pHi_vo->HiVo_enable_pip(vo_dev, PipLayerAttr);
        m_pip_create_status = true;
    }

    {/*set pip win pos and size*/ 
        int left = m_vo_chn_state[output][index].margin[_AV_MARGIN_LEFT_INDEX_];
        int top = m_vo_chn_state[output][index].margin[_AV_MARGIN_TOP_INDEX_];
        double coef_w = 0, coef_h = 0;
        int w = PipLayerAttr.stDispRect.u32Width;
        int h = PipLayerAttr.stDispRect.u32Height;
        
        Ad_calculate_video_rect(w, h, m_vo_chn_state[output][index].margin, &video_rect);
        
        coef_w = (double)(video_rect.w)/(double)w;
        coef_h = (double)(video_rect.h)/(double)h;

        if (false == usenewzoon)
        {
            ChnAttr.stRect.s32X = ALIGN_UP((int)(pip_area.x*coef_w+left), 2);
            ChnAttr.stRect.s32Y = ALIGN_BACK((int)(pip_area.y*coef_h+top), 2);
        }

        ChnAttr.stRect.u32Width  = ALIGN_BACK((int)(pip_area.w*coef_w), 2);
        ChnAttr.stRect.u32Height = ALIGN_BACK((int)(pip_area.h*coef_h), 2);
        ChnAttr.u32Priority      = 1;
        ChnAttr.bDeflicker       = HI_FALSE;
      
        if(m_pHi_vo->HiVo_set_chn_attr(vo_dev, vo_chn, &ChnAttr) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_pip m_pHi_vo->HiVo_set_chn_attr FAILED\n");
            return -1;
        }
        
        if (true == usenewzoon)
        {/*set pip win pos*/
            POINT_S pipPositon;
            pipPositon.s32X = ALIGN_UP((int)(pip_area.x * coef_w + left), 2);
            pipPositon.s32Y = ALIGN_BACK((int)(pip_area.y * coef_h + top), 2);

            m_pHi_vo->HiVo_set_dispos(vo_dev, vo_chn, pipPositon);
        }
    }
    
 #if defined(_AV_HAVE_VIDEO_DECODER_)
    if( (1 == vo_type) && (PS_PAUSE == m_pHi_AvDec->Ade_get_cur_playstate(VDP_PLAYBACK_VDEC)) )
    {
        m_pHi_vo->HiVo_refresh_vo(vo_dev, vo_chn);
        usleep(20 * 1000);
    }
 #endif
    return 0;
}

int CHiAvDeviceMaster::Ad_stop_pip(av_output_type_t output, int index, int video_phy_chnnum)
{
    VO_VIDEO_LAYER_ATTR_S VideoLayerAttr;
    VO_CHN_ATTR_S ChnAttr;
    int vo_type = m_vo_chn_state[output][index].vo_type;/*1-playback, 0-preview*/
    VO_DEV vo_dev = m_pHi_vo->HiVo_get_vo_dev(output, index, 0);
    av_rect_t video_rect; 
    VO_CHN vo_chn = Ad_get_vo_chn(output, index, video_phy_chnnum);      
    int w = 0, h = 0;
    av_vag_hdmi_resolution_t resolution = Ad_get_resolution(output, index);
    if (vo_chn < 0)
    {
        return -1;
    }
    m_pHi_vo->HiVo_get_video_layer_attr(vo_dev, &VideoLayerAttr);

    w = VideoLayerAttr.stImageSize.u32Width;
    h = VideoLayerAttr.stImageSize.u32Height; 
    
    Ad_calculate_video_rect_V2(w, h, m_vo_chn_state[output][index].margin, &video_rect,resolution);

    ChnAttr.stRect.s32X = ALIGN_BACK(video_rect.x, 2);
    ChnAttr.stRect.s32Y = ALIGN_BACK(video_rect.y, 2);
    ChnAttr.stRect.u32Width = ALIGN_BACK(video_rect.w, 2);
    ChnAttr.stRect.u32Height = ALIGN_BACK(video_rect.h, 2);
    ChnAttr.u32Priority = 0;
    ChnAttr.bDeflicker = HI_FALSE;

    if(m_pHi_vo->HiVo_set_chn_attr(vo_dev, vo_chn, &ChnAttr) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_pip m_pHi_vo->HiVo_set_chn_attr FAILED\n");
        return -1;
    }

    m_pHi_vo->HiVo_disable_pip(vo_dev);

 #if defined(_AV_HAVE_VIDEO_DECODER_)
    if ((1 == vo_type) 
            && (PS_PAUSE == m_pHi_AvDec->Ade_get_cur_playstate(VDP_PLAYBACK_VDEC)))
    {
        m_pHi_vo->HiVo_refresh_vo(vo_dev, vo_chn);
    }
 #endif

    return 0;
}

int CHiAvDeviceMaster::Ad_create_digital_zoom_chn(av_output_type_t output, int index, int video_phy_chnnum, int digital_vo_chn)
{
    VO_DEV vo_dev = m_pHi_vo->HiVo_get_vo_dev(output, index, 0);
    VO_CHN vo_chn = Ad_get_vo_chn(output, index, video_phy_chnnum);
    VO_VIDEO_LAYER_ATTR_S video_layer_attr;
    VO_CHN_ATTR_S vo_chn_attr;
    av_rect_t video_rect;
    av_rect_t vo_chn_rect;
    HI_S32 ret_val = -1;
    av_vag_hdmi_resolution_t resolution =  Ad_get_resolution(output,index);

#if defined(_AV_HISILICON_VPSS_)
    vpss_purpose_t vpss_purpose;
    MPP_CHN_S mpp_chn_src;
    MPP_CHN_S mpp_chn_dst;
    VPSS_GRP vpss_grp;
    VPSS_CHN vpss_chn;
#endif

    if (vo_chn < 0)
    {
        return -1;
    }
    
    if (true == m_digital_zoom_status)
    {
        return 0;
    }
#if defined(_AV_HISILICON_VPSS_) && defined(_AV_HAVE_VIDEO_OUTPUT_) 
    int vo_type = m_vo_chn_state[output][index].vo_type;/*1-playback, 0-preview*/
    Ad_get_vpss_purpose(output, index, vo_type, video_phy_chnnum, vpss_purpose, true);

#if defined(_AV_HAVE_VIDEO_DECODER_)
    if( (_VP_PLAYBACK_VDEC_DIGITAL_ZOOM_ == vpss_purpose) && (PS_PAUSE == m_pHi_AvDec->Ade_get_cur_playstate(VDP_PLAYBACK_VDEC)) )
    {/*use playback old vpss*/
        if(m_pHi_vpss->HiVpss_find_vpss(_VP_PLAYBACK_VDEC_, video_phy_chnnum, &vpss_grp, &vpss_chn, NULL) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_create_digital_zoom_chn m_pHi_vpss->HiVpss_find_vpss FAILED\n");
            return -1;
        }
        //vpss workmode is auto , so we need another channel, make sure the channel is aviable while created vpss
#ifdef _AV_PLATFORM_HI3520D_V300_
        vpss_chn = VPSS_CHN1;
#else
        vpss_chn = VPSS_LSTR_CHN;
#endif
    }
    else
#endif
    {/*create new zoom vpss*/
        if( m_pHi_vpss->HiVpss_get_vpss(vpss_purpose, video_phy_chnnum, &vpss_grp, &vpss_chn, NULL) != 0 )
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_create_digital_zoom_chn m_pHi_vpss->HiVpss_get_vpss FAILED\n");
            return -1;
        }
    }

    /*calculate display rectangle*/
    if( m_pHi_vo->HiVo_get_video_layer_attr( vo_dev, &video_layer_attr) != 0 )
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_create_digital_zoom_chn m_pHi_vo->HiVo_get_video_layer_attr FAILED\n");
        return -1;
    }
    
    /*set digital zoom vo chn win size and pos*/
    Ad_calculate_video_rect_V2(video_layer_attr.stImageSize.u32Width, video_layer_attr.stImageSize.u32Height, m_vo_chn_state[output][index].margin, &video_rect,resolution);
    vo_chn_attr.u32Priority = 0;
    vo_chn_attr.bDeflicker = HI_FALSE;
    pgAvDeviceInfo->Adi_get_split_chn_rect(&video_rect, _AS_SINGLE_, 0, &vo_chn_rect, vo_dev);
    vo_chn_attr.stRect.s32X = vo_chn_rect.x;
    vo_chn_attr.stRect.s32Y = vo_chn_rect.y;
    vo_chn_attr.stRect.u32Width = vo_chn_rect.w;
    vo_chn_attr.stRect.u32Height = vo_chn_rect.h;

#ifdef _AV_PLATFORM_HI3535_
    if(m_pHi_vo->HiVo_start_pip_chn(digital_vo_chn, &vo_chn_attr) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_create_digital_zoom_chn m_pHi_vo->HiVo_set_chn_attr FAILED\n");
        return -1;
    }
#else
    if( m_pHi_vo->HiVo_start_vo_chn(vo_dev, digital_vo_chn, &vo_chn_attr) != 0 )
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_create_digital_zoom_chn m_pHi_vo->HiVo_set_chn_attr FAILED\n");
        return -1;
    }
#endif
    
 #if defined(_AV_HAVE_VIDEO_DECODER_)
    if( (_VP_PLAYBACK_VDEC_DIGITAL_ZOOM_ == vpss_purpose) && (PS_PAUSE == m_pHi_AvDec->Ade_get_cur_playstate(VDP_PLAYBACK_VDEC)) )
    {/*cur playback is pause, so pause the digital zoom vo chn*/
        m_pHi_vo->HiVo_pause_vo(vo_dev, digital_vo_chn);
    }
 #endif

    /*bind vpss to zoom vo win*/
    mpp_chn_src.enModId = HI_ID_VPSS;
    mpp_chn_src.s32DevId = vpss_grp;
    mpp_chn_src.s32ChnId = vpss_chn;
    mpp_chn_dst.enModId = HI_ID_VOU;
 #ifdef _AV_PLATFORM_HI3535_
    mpp_chn_dst.s32DevId = m_pHi_vo->HiVo_get_pip_layer();
 #else
    mpp_chn_dst.s32DevId = vo_dev;
 #endif
    mpp_chn_dst.s32ChnId = digital_vo_chn;

    if((ret_val = HI_MPI_SYS_Bind(&mpp_chn_src, &mpp_chn_dst)) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_create_digital_zoom_chn HI_MPI_SYS_Bind FAILED(0x%08lx)\n", (unsigned long)ret_val);
        return -1;
    }
    
    m_digital_zoom_status = true;
#endif
 
#if defined(_AV_HAVE_VIDEO_DECODER_)
    if( (_VP_PLAYBACK_VDEC_DIGITAL_ZOOM_ == vpss_purpose) || (_VP_PREVIEW_VDEC_DIGITAL_ZOOM_ == vpss_purpose) || (_VP_SPOT_VDEC_DIGITAL_ZOOM_ == vpss_purpose) ) 
    {
        int framerate = 0;
        if( m_pHi_vo->HiVo_get_vo_framerate(vo_dev, vo_chn, framerate) == 0 )
        {/*set digital zoom vo chn play framerate*/
            m_pHi_vo->HiVo_set_vo_framerate(vo_dev, digital_vo_chn, framerate);
        }

        m_pHi_vo->HiVo_show_chn(vo_dev, digital_vo_chn);
    }
#endif
    return 0;
}

int CHiAvDeviceMaster::Ad_destory_digital_zoom_chn(av_output_type_t output, int index, int video_phy_chnnum, int digital_vo_chn)
{
#if defined(_AV_HISILICON_VPSS_) && defined(_AV_HAVE_VIDEO_OUTPUT_) 
    HI_S32 ret_val = -1;
    VO_DEV vo_dev = m_pHi_vo->HiVo_get_vo_dev(output, index, 0);
    VO_CHN vo_chn = Ad_get_vo_chn(output, index, video_phy_chnnum);
    vpss_purpose_t vpss_purpose;
    vpss_purpose_t find_vpss_purpose;
    VPSS_GRP vpss_grp;
    VPSS_CHN vpss_chn;
    
    MPP_CHN_S mpp_chn_src;
    MPP_CHN_S mpp_chn_dst;
    int vo_type = m_vo_chn_state[output][index].vo_type;/*1-playback, 0-preview*/

    if (vo_chn < 0)
    {
        return -1;
    }
    
    if (false == m_digital_zoom_status)
    {
        return 0;
    }

    Ad_get_vpss_purpose(output, index, vo_type, video_phy_chnnum, vpss_purpose, true);

    find_vpss_purpose = vpss_purpose;

#if defined(_AV_HAVE_VIDEO_DECODER_)
    if ((_VP_PLAYBACK_VDEC_DIGITAL_ZOOM_ == vpss_purpose) 
            && (PS_PAUSE == m_pHi_AvDec->Ade_get_cur_playstate(VDP_PLAYBACK_VDEC)))
    {/*when playback is pause, use the old vpss*/
        find_vpss_purpose = _VP_PLAYBACK_VDEC_;
    }
#endif

    if(m_pHi_vpss->HiVpss_find_vpss(find_vpss_purpose, video_phy_chnnum, &vpss_grp, &vpss_chn) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceSlave::Ad_digital_zoom m_pHi_vpss->HiVpss_find_vpss FAILED\n");
        return -1;
    }

    mpp_chn_src.enModId = HI_ID_VPSS;
    mpp_chn_src.s32DevId = vpss_grp;
    mpp_chn_src.s32ChnId = vpss_chn;
    mpp_chn_dst.enModId = HI_ID_VOU;
    
#ifdef _AV_PLATFORM_HI3535_
    mpp_chn_dst.s32DevId = m_pHi_vo->HiVo_get_pip_layer();
#else
    mpp_chn_dst.s32DevId = m_pHi_vo->HiVo_get_vo_dev(output, index, 0);
#endif
    mpp_chn_dst.s32ChnId = digital_vo_chn;

    if((ret_val = HI_MPI_SYS_UnBind(&mpp_chn_src, &mpp_chn_dst)) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_video_output_V1 HI_MPI_SYS_Bind FAILED(0x%08lx)\n", (unsigned long)ret_val);
        return -1;
    }

#ifdef _AV_PLATFORM_HI3535_
    m_pHi_vo->HiVo_resume_vo(vo_dev, vo_chn);
    m_pHi_vo->HiVo_stop_pip_chn(digital_vo_chn);
#else
    m_pHi_vo->HiVo_resume_vo(vo_dev, digital_vo_chn);
    m_pHi_vo->HiVo_stop_vo_chn(vo_dev, digital_vo_chn);
#endif

#if defined(_AV_HAVE_VIDEO_DECODER_)
    if ((_VP_PLAYBACK_VDEC_DIGITAL_ZOOM_ == vpss_purpose) && (PS_PAUSE == m_pHi_AvDec->Ade_get_cur_playstate(VDP_PLAYBACK_VDEC)))
    {
        /*when playback is pause, use the old vpss, so needn't to release vpss*/
    }
    else
#endif
    {
        if(m_pHi_vpss->HiVpss_release_vpss(vpss_purpose, video_phy_chnnum, NULL) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_digital_zoom m_pHi_vpss->HiVpss_get_vpss FAILED\n");
            return -1;
        }
    }

    m_digital_zoom_status = false;
#endif
    
    return 0;
}

int CHiAvDeviceMaster::Ad_digital_zoom(av_output_type_t output, int index, bool enable, int video_phy_chnnum, av_rect_t av_zoom_area)
{
#if defined(_AV_HISILICON_VPSS_) && defined(_AV_HAVE_VIDEO_OUTPUT_)    
    int ClipVoChn = pgAvDeviceInfo->Adi_max_channel_number();
    int max_width = 1920;
    int max_height = 1080;
    VPSS_GRP vpss_grp;
    VPSS_CHN vpss_chn;
    vpss_purpose_t vpss_purpose;
    int vo_type = m_vo_chn_state[output][index].vo_type;/*1-playback, 0-preview*/
    
    Ad_get_vpss_purpose(output, index, vo_type, video_phy_chnnum, vpss_purpose, true);

    if( (m_digital_zoom_status == false) && (true == enable) )
    {
        Ad_create_digital_zoom_chn(output, index, video_phy_chnnum, ClipVoChn);
    }

    /*get max width and height*/
    if( _VP_VI_DIGITAL_ZOOM_ == vpss_purpose )
    {
#if defined(_AV_HAVE_VIDEO_INPUT_)
        if(m_pHi_vi->HiVi_get_vi_max_size(video_phy_chnnum, &max_width, &max_height) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_digital_zoom HiVi_get_vi_max_size(%d)\n", video_phy_chnnum);
            return -1;
        }
#endif
    }   
#if defined(_AV_HAVE_VIDEO_DECODER_)
    else if (_VP_PLAYBACK_VDEC_DIGITAL_ZOOM_ == vpss_purpose)
    {
        int purpose = VDP_PLAYBACK_VDEC;
        m_pHi_AvDec->Ade_get_vdec_max_size(purpose, video_phy_chnnum, max_width, max_height);
    }
    else if (_VP_PREVIEW_VDEC_DIGITAL_ZOOM_ == vpss_purpose)
    {
        int purpose = VDP_PREVIEW_VDEC;
        m_pHi_AvDec->Ade_get_vdec_max_size(purpose, video_phy_chnnum, max_width, max_height);
    }
    else if (_VP_SPOT_VDEC_DIGITAL_ZOOM_ == vpss_purpose)
    {
        int purpose = VDP_SPOT_VDEC;
        m_pHi_AvDec->Ade_get_vdec_max_size(purpose, video_phy_chnnum, max_width, max_height);
    }   
    
    if( (_VP_PLAYBACK_VDEC_DIGITAL_ZOOM_ == vpss_purpose) 
            && (PS_PAUSE == m_pHi_AvDec->Ade_get_cur_playstate(VDP_PLAYBACK_VDEC)) )
    {
        vpss_purpose = _VP_PLAYBACK_VDEC_;
    }
#endif

    if(m_pHi_vpss->HiVpss_find_vpss(vpss_purpose, video_phy_chnnum, &vpss_grp, &vpss_chn) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceSlave::Ad_digital_zoom m_pHi_vpss->HiVpss_find_vpss FAILED\n");
        return -1;
    }
    if (m_pHi_vpss->HiVpss_set_vpssgrp_cropcfg(enable, vpss_grp, &av_zoom_area, max_width, max_height) < 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_digital_zoom m_pHi_vpss->HiVpss_set_vpssgrp_cropcfg FAILED\n");
        return -1;
    }

    m_pHi_vo->HiVo_refresh_vo( m_pHi_vo->HiVo_get_vo_dev(output, index, 0), ClipVoChn );

    if( (m_digital_zoom_status == true) && (false == enable) )
    {
        Ad_destory_digital_zoom_chn(output, index, video_phy_chnnum, ClipVoChn);
    }
#endif
    return 0;
}

int CHiAvDeviceMaster::Ad_set_vo_margin( av_output_type_t output, int index, unsigned short u16LeftSpace, unsigned short u16RightSpace
            , unsigned short u16TopSpace, unsigned short u16BottomSpace, bool bResetSlave/*=true*/)
{
    av_tvsystem_t tv_system = Ad_get_tv_system(output, index);
    av_vag_hdmi_resolution_t resolution = Ad_get_resolution(output, index);
    av_rect_t video_rect;
    av_split_t eSplitMode = m_vo_chn_state[output][index].cur_split_mode;

    /*create vo channel layout, first get the size of video layer*/
    if(output == _AOT_MAIN_)/*_AOT_MAIN_: hdmi&vga, cvbs(write-back)*/
    {
        m_pHi_vo->HiVo_get_video_size(resolution, &video_rect.w, &video_rect.h, NULL, NULL);
    }
	else if(output == _AOT_CVBS_)
	{
		m_pHi_vo->HiVo_get_video_size(tv_system, &video_rect.w, &video_rect.h, NULL, NULL);
	}
    else if(output == _AOT_SPOT_)/*spot*/
    {
        m_pHi_vo->HiVo_get_video_size(tv_system, &video_rect.w, &video_rect.h, NULL, NULL);
    }
    else
    {
        _AV_FATAL_(1, "CHiAvDeviceMaster::Ad_set_vo_margin You must give the implement\n");
    }

    if (_AS_UNKNOWN_ == eSplitMode)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_set_vo_margin eSplitMode %d FAILED\n", eSplitMode);
        return -1;
    }

    int margin[4];
    margin[_AV_MARGIN_TOP_INDEX_] = u16TopSpace;
    margin[_AV_MARGIN_BOTTOM_INDEX_] = u16BottomSpace;
    margin[_AV_MARGIN_LEFT_INDEX_] = u16LeftSpace;
    margin[_AV_MARGIN_RIGHT_INDEX_] = u16RightSpace;

    m_vo_chn_state[output][index].margin[0] = margin[0];
    m_vo_chn_state[output][index].margin[1] = margin[1];
    m_vo_chn_state[output][index].margin[2] = margin[2];
    m_vo_chn_state[output][index].margin[3] = margin[3];

    calculate_margin_for_limit_resource_machine(resolution,margin);

    Ad_calculate_video_rect( video_rect.w, video_rect.h, margin, &video_rect );
    if (eSplitMode != _AS_PIP_)
    {
        if(Ad_reset_vo_layout(output, index, &video_rect, eSplitMode) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_set_vo_margin Ad_create_vo_layout FAILED\n");
            return -1;
        }
    }

    if ((eSplitMode == _AS_PIP_) && (true == bResetSlave))
    {/*reset pip*/
        int vo_chn = -1;
        av_pip_para_t av_pip_para;
        
        av_pip_para.enable = false;
        av_pip_para.video_phychn[0] = m_pip_main_video_phy_chn;
        av_pip_para.video_phychn[1] = m_pip_sub_video_phy_chn;
        
        vo_chn = Ad_get_vo_chn(output, index, av_pip_para.video_phychn[1]);    
        
        pgAvDeviceInfo->Adi_get_split_chn_rect(&video_rect, eSplitMode, vo_chn, &av_pip_para.stuMainPipArea, m_pHi_vo->HiVo_get_vo_dev(output, index, 0));
        Ad_set_pip_screen(av_pip_para);
        
        av_pip_para.enable = true;
        Ad_set_pip_screen(av_pip_para);
    }

    for(int chn_index = 0; chn_index < pgAvDeviceInfo->Adi_get_split_chn_info(m_vo_chn_state[output][index].split_mode); chn_index++)
    {
        if(m_vo_chn_state[output][index].chn_layout[chn_index] == -1)
        {
            continue;
        }
        if(!m_video_preview_switch.test(m_vo_chn_state[output][index].chn_layout[chn_index]))
        {
            Ad_show_access_logo(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), chn_index, true);
        }
    }
    
    return 0;
}


int CHiAvDeviceMaster::Ad_reset_vo_layout(av_output_type_t output, int index, av_rect_t *pVideo_rect, av_split_t split_mode )
{
    int max_chn_num = pgAvDeviceInfo->Adi_get_split_chn_info(split_mode);
    av_rect_t vo_chn_rect;
    VO_CHN_ATTR_S vo_chn_attr;
    int vo_chn = -1;
    int iRet = 0;
    VO_DEV vo_dev = m_pHi_vo->HiVo_get_vo_dev(output, index, 0);
    
    if(  m_pHi_vo->HiVo_set_attr_begin(vo_dev) )
    {
        return -1;
    }
    
    for(int chn_index = 0; chn_index < max_chn_num; chn_index ++)
    {
        if (m_vo_chn_state[output][index].cur_chn_layout[chn_index] < 0)
        {
            vo_chn = chn_index;
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_reset_vo_layout vo_chn %d, chn_index %d, phy_chn %d\n", vo_chn, chn_index, m_vo_chn_state[output][index].cur_chn_layout[chn_index]);
        }
        else if((vo_chn = Ad_get_vo_chn(output, index, m_vo_chn_state[output][index].cur_chn_layout[chn_index])) < 0)
        {
            vo_chn = chn_index;
        }

        memset(&vo_chn_attr, 0, sizeof(VO_CHN_ATTR_S));

        if( m_pHi_vo->HiVo_get_chn_attr(vo_dev, vo_chn, &vo_chn_attr ) != 0 )
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_create_vo_layout m_pHi_vo->HiVo_get_chn_attr FAILED\n");
            iRet = -1;
        }

        pgAvDeviceInfo->Adi_get_split_chn_rect(pVideo_rect, split_mode, chn_index, &vo_chn_rect, vo_dev);       

        vo_chn_attr.stRect.s32X = vo_chn_rect.x;
        vo_chn_attr.stRect.s32Y = vo_chn_rect.y;
        vo_chn_attr.stRect.u32Width = vo_chn_rect.w;
        vo_chn_attr.stRect.u32Height = vo_chn_rect.h;
        if(m_pHi_vo->HiVo_set_chn_attr(vo_dev, vo_chn, &vo_chn_attr) != 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_create_vo_layout m_pHi_vo->HiVo_start_vo_chn FAILED\n");
            iRet = -1;
            break;
        }
    }

    m_pHi_vo->HiVo_set_attr_end(vo_dev);

    return iRet;
}

#endif //end  defined(_AV_HAVE_VIDEO_OUTPUT_)


int CHiAvDeviceMaster::Ad_clear_init_logo()
{
#if defined(_AV_PLATFORM_HI3520D_V300_)
    HI_MPI_SYS_SetReg(0x13026800,0x00000349);
    HI_MPI_SYS_SetReg(0x13026804,0x1);	
#elif defined(_AV_PLATFORM_HI3520A_) || defined(_AV_PLATFORM_HI3521_) || defined(_AV_PLATFORM_HI3520D_)
    HI_MPI_SYS_SetReg(0x205c9000,0x00000349);/*G0 VGA+HDMI*/
    HI_MPI_SYS_SetReg(0x205c9004,0x00000001);
    
    HI_MPI_SYS_SetReg(0x205c9600,0x00000349);/*G3 CVBS*/
    HI_MPI_SYS_SetReg(0x205c9604,0x00000001);
#elif defined(_AV_PLATFORM_HI3531_) || defined(_AV_PLATFORM_HI3532_)
    HI_MPI_SYS_SetReg(0x205c9200,0x00000349);/*G1 VGA+HDMI*/
    HI_MPI_SYS_SetReg(0x205c9204,0x00000001);

    HI_MPI_SYS_SetReg(0x205c9400,0x00000349);/*G2 CVBS*/
    HI_MPI_SYS_SetReg(0x205c9404,0x00000001);
#elif defined(_AV_PLATFORM_HI3535_)
    HI_MPI_SYS_SetReg(0x205c6000,0x00000349);/*G0 VGA+HDMI*/
    HI_MPI_SYS_SetReg(0x205c6004,0x00000001);

    HI_MPI_SYS_SetReg(0x205c6800,0x00000349);/*G2 CVBS*/
    HI_MPI_SYS_SetReg(0x205c6804,0x00000001);
#elif defined(_AV_PRODUCT_CLASS_IPC_)
#endif

    return 0;
}


#if defined(_AV_HAVE_VIDEO_INPUT_)
int CHiAvDeviceMaster::Ad_vi_cover(int phy_chn_num, int x, int y, int width, int height, int cover_index)
{
    m_pHi_vi->HiVi_vi_cover(phy_chn_num, x, y, width, height, cover_index);
    return 0;
}

int CHiAvDeviceMaster::Ad_vi_uncover(int phy_chn_num, int cover_index)
{
    m_pHi_vi->HiVi_vi_uncover(phy_chn_num, cover_index);
    return 0;
}

int CHiAvDeviceMaster::Ad_start_vi(av_tvsystem_t tv_system, vi_config_set_t *pSource/* = NULL*/, vi_config_set_t *pPrimary_attr/* = NULL*/, vi_config_set_t *pMinor_attr/* = NULL*/)
{
    if(CHiAvDevice::Ad_start_vi(tv_system, pSource, pPrimary_attr, pMinor_attr) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_vi (CHiAvDevice::Ad_start_vi(...) != 0)\n");
        return -1;
    }
    return 0;
}

int CHiAvDeviceMaster::Ad_start_selected_vi(av_tvsystem_t tv_system, unsigned long long int chn_mask, vi_config_set_t *pSource/* = NULL*/, vi_config_set_t *pPrimary_attr/* = NULL*/, vi_config_set_t *pMinor_attr/* = NULL*/)
{
    if(CHiAvDevice::Ad_start_selected_vi(tv_system, chn_mask, pSource, pPrimary_attr, pMinor_attr) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_selected_vi (CHiAvDevice::Ad_start_selected_vi(...) != 0)\n");
        return -1;
    }
    
    return 0;
}


int CHiAvDeviceMaster::Ad_stop_vi()
{
    if(CHiAvDevice::Ad_stop_vi() != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_vi ( CHiAvDevice::Ad_stop_vi() != 0)\n");
        return -1;
    }
    return 0;
}

int CHiAvDeviceMaster::Ad_stop_selected_vi(unsigned long long int chn_mask)
{
    if(CHiAvDevice::Ad_stop_selected_vi(chn_mask) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_vi ( CHiAvDevice::Ad_stop_vi() != 0)\n");
        return -1;
    }

    return 0;
}

int CHiAvDeviceMaster::Ad_vi_insert_picture(unsigned int video_loss_bmp)
{
    CHiAvDevice::Ad_vi_insert_picture(video_loss_bmp);
    return 0;
}

#if !defined(_AV_PLATFORM_HI3518EV200_)
int CHiAvDeviceMaster::Ad_start_vda(vda_type_e vda_type, av_tvsystem_t tv_system, int phy_video_chn, const vda_chn_param_t* pVda_chn_param)
{
    if (CHiAvDevice::Ad_start_vda(vda_type, tv_system, phy_video_chn, pVda_chn_param) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_vda (CHiAvDevice::Ad_start_vda(...) != 0)\n");
        return -1;
    }

    return 0;
}

int CHiAvDeviceMaster::Ad_start_selected_vda(vda_type_e vda_type, av_tvsystem_t tv_system, int phy_video_chn, const vda_chn_param_t* pVda_chn_param)
{
    if (CHiAvDevice::Ad_start_selected_vda(vda_type, tv_system, phy_video_chn, pVda_chn_param) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_vda (CHiAvDevice::Ad_start_vda(...) != 0)\n");
        return -1;
    }

    return 0;
}


int CHiAvDeviceMaster::Ad_stop_vda(vda_type_e vda_type, int phy_video_chn)
{
    if(CHiAvDevice::Ad_stop_vda(vda_type, phy_video_chn) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_vda ( CHiAvDevice::Ad_stop_vda() != 0)\n");
        return -1;
    }

    return 0;
}

int CHiAvDeviceMaster::Ad_stop_selected_vda(vda_type_e vda_type, int phy_video_chn)
{
    if(CHiAvDevice::Ad_stop_selected_vda(vda_type, phy_video_chn) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_vda ( CHiAvDevice::Ad_stop_vda() != 0)\n");
        return -1;
    }

    return 0;
}


int CHiAvDeviceMaster::Ad_get_vda_result(vda_type_e vda_type, unsigned int *pVda_result, int phy_video_chn, unsigned char *pVda_region)
{
    if(CHiAvDevice::Ad_get_vda_result(vda_type, pVda_result, phy_video_chn, pVda_region) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_get_vda_result ( CHiAvDevice::Ad_get_vda_result(...) != 0)\n");
        return -1;
    }

    return 0;
}
#endif

int CHiAvDeviceMaster::Ad_set_video_attr( int chn, unsigned char u8IsMirror, unsigned char u8IsFlip )
{
    DEBUG_CRITICAL( "CHiAvDevice::Ad_SetVideoAttr channel=%d mirror=%d flip=%d\n", chn, u8IsMirror, u8IsFlip );
    int ret = -1;

    ret = m_pHi_vi->HiVi_set_mirror_flip( chn, u8IsMirror, u8IsFlip );

    return ret;
}


int CHiAvDeviceMaster::Ad_set_videoinput_image(av_video_image_para_t av_video_image)
{
#if defined(_AV_HISILICON_VPSS_)
    return m_pHi_vpss->HiVpss_set_vpssgrp_param(av_video_image);
#endif
    return 0;
}
#endif

#if defined(_AV_HAVE_VIDEO_DECODER_)
void CHiAvDeviceMaster::Ad_init_dec()
{
    AvDec_Operation_t HiAvDecOpe;

    HiAvDecOpe.ConnectToVo = boost::bind(&CHiAvDeviceMaster::Ad_dec_connect_vo, this, _1, _2);
    HiAvDecOpe.DisconnectToVo = boost::bind(&CHiAvDeviceMaster::Ad_dec_disconnect_vo, this, _1, _2);
    HiAvDecOpe.GetTvSystem = boost::bind(&CHiAvDeviceMaster::Ad_get_tv_system, this, _1, _2);
#if defined(_AV_HAVE_VIDEO_ENCODER_)
    HiAvDecOpe.Get_Encoder_Used_Resource = boost::bind(&CHiAvDeviceMaster::Ad_get_encoder_used_resource, this, _1);
    HiAvDecOpe.WhetherEncoderHaveThisResolution = boost::bind(&CHiAvDeviceMaster::Ad_check_encoder_resolution_exist, this, _1, _2);
#endif
    _AV_FATAL_((m_pHi_AvDec = new CHiAvDec) == NULL, "OUT OF MEMORY\n");
    m_pHi_AvDec->Ade_set_dec_operate_callback(&HiAvDecOpe);

#if defined(_AV_HAVE_VIDEO_OUTPUT_)
    m_pHi_AvDec->Ade_set_vo(m_pHi_vo);
#endif

#if defined(_AV_HAVE_AUDIO_OUTPUT_)
    m_pHi_AvDec->Ade_set_ao(m_pHi_ao);
#endif

#if defined(_AV_HISILICON_VPSS_)
    m_pHi_vpss->HiVpss_set_vdec(m_pHi_AvDec);
    m_pHi_AvDec->Ade_set_vpss( m_pHi_vpss );
#endif

    for (int pur = 0; pur < _AV_DECODER_MODE_NUM_; pur++)
    {
        memset(m_av_dec_chn_map[pur], -1, sizeof(m_av_dec_chn_map[pur]));
    }
    
    return;
}

bool CHiAvDeviceMaster::Ad_switch_dec_auido_chn(int purpose, int chn)
{
    int audio_chn = chn;
    
    if (m_pHi_AvDec == NULL)
    {
        return false;
    }
    if (m_audio_output_state.audio_mute)
    {
        audio_chn = -1;
    }
    m_pHi_AvDec->Ade_switch_ao(purpose, audio_chn);
    
    return true;
}

bool CHiAvDeviceMaster::Ad_check_reset_decoder(int purpose, int chn[_AV_SPLIT_MAX_CHN_], int chnum)
{
    bool resetdecoder = false;
    int maxchn = pgAvDeviceInfo->Adi_max_channel_number();
    bool findchn[_AV_SPLIT_MAX_CHN_], decodemap[_AV_SPLIT_MAX_CHN_];
    
    if ((purpose != VDP_SPOT_VDEC) && (purpose != VDP_PREVIEW_VDEC))
    {
        //return true;
    }

    for (int index = 0; index < maxchn; index++)
    {
        findchn[index] = false;
        decodemap[index] = false;
    }
    
    for(int index = 0; index < maxchn; index ++)
    {
        if ((chn[index] < 0) || (chn[index] >= _AV_SPLIT_MAX_CHN_))
        {
            continue;
        }
        
        for (int i = 0; i < maxchn; i++)
        {
            if (m_av_dec_chn_map[purpose][i] == chn[index])
             {
                 findchn[index] = true;
                decodemap[i] = true;
                 break;
             } 
        }

        if (findchn[index] == false)     
        {/*当出现当前切换的解码通道
             与已经创建的解码通道不符的情况则重新销毁创建*/
            resetdecoder = true;
        } 
        if (decodemap[index] == false)     
        {
		decodemap[index] = false;
	}
    }

    return resetdecoder;
}

//#define TEST_9004_VERSION
#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
int CHiAvDeviceMaster::Ad_start_preview_dec(av_output_type_t output, int index, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_])
{
    eVDecPurpose purpose = VDP_PREVIEW_VDEC;
    int maxchnnum = pgAvDeviceInfo->Adi_max_channel_number();
    eProductType product_type = PTD_INVALID;
    int voNum = 0;
    int szChlLay[_AV_SPLIT_MAX_CHN_];
    int szRealVoChlLay[_AV_VDEC_MAX_CHN_] ={-1};
    //default sub-streamtype
    unsigned char u8StreamType = 1; 
    
    memset( szChlLay, -1, sizeof(szChlLay) );
    
    if( _AOT_SPOT_ == output)
    {
        purpose = VDP_SPOT_VDEC;
    }
    
    for( int i=0; i<_AV_SPLIT_MAX_CHN_; i++ )
    {
        if ((chn_layout[i] < 0) 
            || (chn_layout[i] >= pgAvDeviceInfo->Adi_max_channel_number())
            || (pgAvDeviceInfo->Adi_get_video_source(chn_layout[i]) != _VS_DIGITAL_))
        {
            continue;
        }
        
        szChlLay[voNum] = chn_layout[i];
	    szRealVoChlLay[voNum] = i;	
        voNum++;
    }
    
    if (voNum <= 0)
    {
        DEBUG_MESSAGE( "CHiAvDeviceMaster::Ad_start_preview_dec voNum %d\n", voNum);
        return -1;
    }

#if defined(_AV_PRODUCT_CLASS_HDVR_)
    if ((_AS_SINGLE_ == split_mode) && (VDP_PREVIEW_VDEC == purpose) 
        && (pgAvDeviceInfo->Adi_get_video_source(chn_layout[0]) == _VS_DIGITAL_))
#else
    if ((_AS_SINGLE_ == split_mode) && (VDP_PREVIEW_VDEC == purpose))            
#endif
    {
		/*device type*/
    	N9M_GetProductType(product_type);
        Ad_switch_dec_auido_chn(purpose, chn_layout[0]);
		if(product_type == PTD_X1)
		{
			u8StreamType = 1;
		}
		else
		{
        	u8StreamType = 0;
		}
        char customer_name[32] = {0};
        pgAvDeviceInfo->Adi_get_customer_name(customer_name, sizeof(customer_name));
        if (strcmp(customer_name, "CUSTOMER_ZHANTING") == 0)
        {
            u8StreamType = 1;
        }
    }

    if (m_pHi_AvDec)
    {
        m_pHi_AvDec->Ade_set_dec_info(purpose, szChlLay, split_mode);
    }  
    
    memset(m_av_dec_chn_map[purpose], -1, sizeof(m_av_dec_chn_map[purpose]));
    for (int index = 0; index < maxchnnum; index++)
    {
        m_av_dec_chn_map[purpose][index] = szChlLay[index];
    }
	
 #if 0//def _AV_PLATFORM_HI3520D_V300_
	 HI_S32 PicSize=0;
        VB_CONF_S stModVbConf;	 
        memset(&stModVbConf, 0, sizeof(VB_CONF_S));
        VB_PIC_BLK_SIZE(1920, 1080, PT_H264, PicSize);	

        stModVbConf.u32MaxPoolCnt = 2;
        stModVbConf.astCommPool[0].u32BlkSize = PicSize;
        stModVbConf.astCommPool[0].u32BlkCnt  = 4*4;
		
	 HI_MPI_VB_ExitModCommPool(VB_UID_VDEC);
//	 HI_MPI_VB_SetModPoolConf(VB_UID_VDEC, &stModVbConf);
//	 HI_MPI_VB_InitModCommPool(VB_UID_VDEC);
        CHECK_RET_INT(HI_MPI_VB_SetModPoolConf(VB_UID_VDEC, &stModVbConf), "HI_MPI_VB_SetModPoolConf");
        CHECK_RET_INT(HI_MPI_VB_InitModCommPool(VB_UID_VDEC), "HI_MPI_VB_InitModCommPool");
#endif
    
    m_pHi_AvDec->Ade_switch_preview_dec(purpose, voNum, szChlLay, m_c8ChannelMap, m_u8DevOlState, u8StreamType, m_output_tv_system[pgAvDeviceInfo->Adi_get_main_out_mode()][0],szRealVoChlLay);
   
    return 0;
}

int CHiAvDeviceMaster::Ad_stop_preview_dec(av_output_type_t output, int index)
{
    eVDecPurpose purpose = VDP_PREVIEW_VDEC;
    if( _AOT_SPOT_ == output)
    {/*0:预览输出 1:解码输出 2:Spot输出*/
        purpose = VDP_SPOT_VDEC;
    }
    memset(m_av_dec_chn_map[purpose], -1, sizeof(m_av_dec_chn_map[purpose]));
    
    return m_pHi_AvDec->Ade_stop_preview_dec(purpose);
}
#endif

int CHiAvDeviceMaster::Ad_start_playback_dec(void *pPara)
{
    av_dec_set_t *pAvDec = (av_dec_set_t *)pPara;
    
    DEBUG_CRITICAL("av_dec_purpose =%d chnbmp =0x%x  chnnum =%d split_mode =%d\n",\
        pAvDec->av_dec_purpose, \
        pAvDec->chnbmp, \
        pAvDec->chnnum,\
        pAvDec->split_mode
        );
    int maxchnnum = pgAvDeviceInfo->Adi_max_channel_number();
    eVDecPurpose purpose = VDP_PLAYBACK_VDEC;
    av_output_type_t output = pgAvDeviceInfo->Adi_get_main_out_mode();
    int ret = 0, chn_list[_AV_SPLIT_MAX_CHN_];
    memset(m_av_dec_chn_map[purpose], -1, sizeof(m_av_dec_chn_map[purpose]));
    memset(chn_list, -1, sizeof(chn_list));
    for (int index = 0; index < maxchnnum; index++)
    {
        m_av_dec_chn_map[purpose][index] = pAvDec->chnlist[index];
        
        if(pAvDec->chnlist[index] >= 0)
        {
            chn_list[index] = index;
        }
        else
        {
            chn_list[index] = -1;
        }
    }
    
   /*
    for (int index = 0; index < maxchnnum; index++)
    {
        if(pAvDec->chnlist[index] >= 0)
        {
            chn_list[index] = index;
        }
        else
        {
            chn_list[index] = -1;
        }
    }
*/
    if (m_vo_chn_state[output][0].split_mode == _AS_PIP_)
    {
        av_pip_para_t av_pip_para;
        memset(&av_pip_para, 0, sizeof(av_pip_para));
        av_pip_para.enable = false;
        Ad_set_pip_screen(av_pip_para);
    }
    
    Ad_stop_preview_output( output, 0 );

    char BufferName[32];
    unsigned int BufferSize = 0;
    unsigned int FrameCount = 0;
    VdecPara_t stuVdecParam;
    memset(&stuVdecParam, 0, sizeof(VdecPara_t));
    stuVdecParam.chnnum = pAvDec->chnnum;
    if(pgAvDeviceInfo->Adi_get_stream_buffer_info(BUFFER_TYPE_MSSP_STREAM, 0, BufferName, BufferSize, FrameCount) != 0)
    {
        DEBUG_ERROR( "get mssp buffer info failed!\n");
        return -1;
    }
    stuVdecParam.stuVdecInfo[0].smi_size = BufferSize;
    stuVdecParam.stuVdecInfo[0].smi_framenum = FrameCount;

    m_pHi_AvDec->Ade_pre_init_decoder_param(purpose, m_av_dec_chn_map[purpose]);
    /*create vo chn*/
    Ad_start_playback_output(output, 0, pAvDec->split_mode, chn_list);
#if 0//def _AV_PLATFORM_HI3520D_V300_
	 int mymaxchnnum = pgAvDeviceInfo->Adi_max_channel_number();
	 HI_S32 PicSize=0;
        VB_CONF_S stModVbConf;	 
        memset(&stModVbConf, 0, sizeof(VB_CONF_S));
        VB_PIC_BLK_SIZE(928, 576, PT_H264, PicSize);	

        stModVbConf.u32MaxPoolCnt = 2;
        stModVbConf.astCommPool[0].u32BlkSize = PicSize;
        stModVbConf.astCommPool[0].u32BlkCnt  = 4*mymaxchnnum;
		
        VB_PIC_BLK_SIZE(1920, 1080, PT_H264, PicSize);	
        stModVbConf.astCommPool[1].u32BlkSize = PicSize;
        stModVbConf.astCommPool[1].u32BlkCnt  = 4*1;
		
	 HI_MPI_VB_ExitModCommPool(VB_UID_VDEC);
        CHECK_RET_INT(HI_MPI_VB_SetModPoolConf(VB_UID_VDEC, &stModVbConf), "HI_MPI_VB_SetModPoolConf");
        CHECK_RET_INT(HI_MPI_VB_InitModCommPool(VB_UID_VDEC), "HI_MPI_VB_InitModCommPool"); 
#endif
    //start playback
    ret = m_pHi_AvDec->Ade_start_playback_dec(purpose, Ad_get_tv_system(output, 0), m_av_dec_chn_map[purpose], stuVdecParam);

    Ad_switch_dec_auido_chn(purpose, Ad_set_audio_output_to_zero());
    return ret;
}

int CHiAvDeviceMaster::Ad_stop_playback_dec()
{
    m_pHi_AvDec->Ade_pause_playback_dec(true);
    Ad_stop_playback_output(pgAvDeviceInfo->Adi_get_main_out_mode(), 0);
    memset(m_av_dec_chn_map[VDP_PLAYBACK_VDEC], -1, sizeof(m_av_dec_chn_map[VDP_PLAYBACK_VDEC]));
    
    return m_pHi_AvDec->Ade_stop_playback_dec();
}

int CHiAvDeviceMaster::Ad_operate_dec(int purpose, int opcode, bool operate)
{
    if (m_pHi_AvDec)
    {
        return m_pHi_AvDec->Ade_operate_decoder(purpose, opcode, operate);
    }
    
    return -1;
}

int CHiAvDeviceMaster::Ad_control_vda_thread(vda_type_e vda_type, thread_control_e control_command)
{
#if defined(_AV_HAVE_VIDEO_INPUT_)
    if(m_pAv_vda)
    {
        return m_pAv_vda->Av_control_vda_thread(vda_type, control_command);
    }
#endif
    return -1;
}

int CHiAvDeviceMaster::Ad_operate_dec_for_switch_channel(int purpose)
{
    if (m_pHi_AvDec)
    {
        return m_pHi_AvDec->Ade_operate_decoder_for_switch_channel(purpose);
    }

    return -1;
}


int CHiAvDeviceMaster::Ad_get_dec_image_size(int purpose, int phy_chn, int &width, int &height)
{
    if (m_pHi_AvDec)
    {
        return m_pHi_AvDec->Ade_get_vdec_video_size_for_ui(purpose, phy_chn, width, height);
    }
    return -1;
}

int CHiAvDeviceMaster::Ad_dynamic_modify_vdec_framerate(int purpose)
{
    if (m_pHi_AvDec)
    {
        return m_pHi_AvDec->Ade_dynamic_modify_vdec_framerate(purpose);
    }
    return -1;
}

bool CHiAvDeviceMaster::Ad_get_dec_playtime(int purpose, void *playtime)
{
    int ret = -1;
    datetime_t *pTime = (datetime_t *)playtime;

    if (NULL == playtime)
    {
        return false;
    }

    if (m_pHi_AvDec)
    {
        ret = m_pHi_AvDec->Ade_get_dec_playtime(purpose, pTime);
    }

    if(ret >= 0)
    {
        return true;
    }
    else
    {
        return false;
    }

    return true;
}

int CHiAvDeviceMaster::Ad_dec_connect_vo(int purpose, int phy_chn)
{
    int vo_chn = m_pHi_AvDec->Ade_get_layout_chn(purpose, phy_chn);
    
    if (vo_chn < 0)
    {
        DEBUG_ERROR("CHiAvDeviceMaster::Ad_dec_connect_vo chn %d is not found in list\n", phy_chn);
        return -1;
    }
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
    int index = 0;
    av_output_type_t output = pgAvDeviceInfo->Adi_get_main_out_mode();
    
    if (VDP_SPOT_VDEC == purpose)
    {
        output = _AOT_SPOT_;
    }
    //DEBUG_MESSAGE("[Ad_dec_connect_vo]purpose %d phychn %d, index %d, bind=====\n", purpose, phy_chn, vo_chn);
    Ad_output_connect_source(output, index, vo_chn, phy_chn);
    //DEBUG_MESSAGE("bb [Ad_dec_connect_vo]purpose %d phychn %d, vo_chn %d, bind=====retval %d\n", purpose, phy_chn, vo_chn, retval);

    m_pHi_AvDec->Ade_set_vo_chn_framerate(purpose, phy_chn);
#endif
    return vo_chn;
}

int CHiAvDeviceMaster::Ad_dec_disconnect_vo(int purpose, int phy_chn)
{  
#if defined(_AV_HAVE_VIDEO_OUTPUT_) 
    int index = 0;
    av_output_type_t output = pgAvDeviceInfo->Adi_get_main_out_mode();
    int layout_ch = -1;
    
    if (VDP_SPOT_VDEC == purpose)
    {
        output = _AOT_SPOT_;
    }

    int vo_chn = m_pHi_AvDec->Ade_get_layout_chn(purpose, phy_chn);   
    if (vo_chn < 0)
    {
        DEBUG_ERROR("CHiAvDeviceMaster::Ad_dec_disconnect_vo chn %d is not found in list\n", phy_chn);
        return -1;
    }
    
    layout_ch = Ad_get_vo_chn(output, index, phy_chn);
    //DEBUG_ERROR("CHiAvDeviceMaster::Ad_dec_disconnect_vo chn %d layout_ch %d, vo_chn %d\n", phy_chn, layout_ch, vo_chn);

    //if (layout_ch >= 0)
    {
        Ad_output_disconnect_source(output, index, vo_chn, phy_chn);
    }
#endif
    return layout_ch;
}

void CHiAvDeviceMaster::Ad_get_phy_layout_chn(int purpose, int phy_ch, int &layout_ch)
{ 
    layout_ch = -1;
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
    av_output_type_t output = pgAvDeviceInfo->Adi_get_main_out_mode();
    int index = 0;     
    layout_ch = Ad_get_vo_chn(output, index, phy_ch);   
#endif
    return;
}

void * talkback_pthread_entry(void *param)
{
     CHiAvDeviceMaster *pHiAvDeviceMaster = (CHiAvDeviceMaster *)param;

     pHiAvDeviceMaster->Ad_talkback_thread_body();

     return NULL;
}

int CHiAvDeviceMaster::Ad_talkback_thread_body()
{
    const int buffer_size = 512;
    SShareMemFrameInfo *pFrameInfo = NULL;
    int ret = 0;
    RMSTREAM_HEADER *pStream_header = NULL;
    char * pBuffer = new char[buffer_size];
    AUDIO_STREAM_S audio_stream;
        
    while (m_talkback_cntrl.thread_run)
    {
        if(N9M_SHGetOneFrame(m_talkback_cntrl.Stream_buffer_handle, pBuffer, buffer_size, &pFrameInfo) > 0)
        {
            pStream_header = (RMSTREAM_HEADER *)pBuffer;
            memset(&audio_stream, 0, sizeof(AUDIO_STREAM_S));
            audio_stream.pStream = (unsigned char *)pBuffer + sizeof(RMSTREAM_HEADER) + pStream_header->lExtendLen;
            audio_stream.u32Len = pStream_header->lFrameLen;

            if ((ret = HI_MPI_ADEC_SendStream(m_talkback_cntrl.adec_chn, &audio_stream, HI_IO_BLOCK)) != 0)
            {
                DEBUG_ERROR( "CHiAvDeviceMaster::Ad_talkback_thread_body HI_MPI_ADEC_SendStream FAILED! (0x%08x)\n", ret);
            }
        }
        else
        {
            usleep(10*1000);
        }
    }

    m_talkback_cntrl.thread_exit = true;
    delete []pBuffer;
    
    return 0;
}

int CHiAvDeviceMaster::Ad_start_talkback_adec()
{
    if (m_pHi_AvDec->Ade_create_adec_handle(_AT_ADEC_TALKBACK, APT_ADPCM) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_talkback_adec Ade_create_adec_handle FAILED! \n");
        return -1;
    }

    m_talkback_cntrl.adec_chn = m_pHi_AvDec->Ade_get_adec_chn(_AT_ADEC_TALKBACK);
    m_talkback_cntrl.thread_run = true;
    m_talkback_cntrl.thread_exit = false;
    
    if (m_talkback_cntrl.Stream_buffer_handle == NULL)
    {
        unsigned int stream_buffer_size = 0;
        unsigned int stream_buffer_frame = 0;
        char stream_buffer_name[32];

        if(pgAvDeviceInfo->Adi_get_stream_buffer_info(BUFFER_TYPE_TALKSET_STREAM, 0, stream_buffer_name, stream_buffer_size, stream_buffer_frame) != 0)
        {
            DEBUG_ERROR( "get buffer failed!\n");
            return -1;
        }
        
        m_talkback_cntrl.Stream_buffer_handle = N9M_SHCreateShareBuffer("avstream", stream_buffer_name, stream_buffer_size, stream_buffer_frame, SHAREMEM_READ_MODE);
        N9M_SHGotoLatestIFrame(m_talkback_cntrl.Stream_buffer_handle);
    }
    
    Av_create_normal_thread(talkback_pthread_entry, (void *)this, NULL);

    return 0;
}

int CHiAvDeviceMaster::Ad_stop_talkback_adec()
{
    m_talkback_cntrl.thread_run = false;
    while (!m_talkback_cntrl.thread_exit)
    {
        usleep(20*1000);
    }

    if (m_talkback_cntrl.Stream_buffer_handle != NULL)
    {
        N9M_SHDestroyShareBuffer(m_talkback_cntrl.Stream_buffer_handle);
        m_talkback_cntrl.Stream_buffer_handle = NULL;
    }

    if (m_pHi_AvDec->Ade_destory_adec_handle(_AT_ADEC_TALKBACK) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_talkback_adec  Ade_destory_adec_handle FAILED! \n");
        return -1;
    }

    return 0;

}

#endif //end #if defined(_AV_HAVE_VIDEO_DECODER_)


#if defined(_AV_SUPPORT_REMOTE_TALK_)
int CHiAvDeviceMaster::Ad_Start_Remote_Talk( msgRequestAudioNetPara_t *pstuAudioNetPara )
{
    if( !m_pRemoteTalk )
    {
#if defined(_AV_HAVE_AUDIO_OUTPUT_)  
#if defined(_AV_HAVE_AUDIO_INPUT_)
    m_pRemoteTalk = new CAvRemoteTalk( m_pHi_ao, m_pHi_ai );
#else
    m_pRemoteTalk = new CAvRemoteTalk( m_pHi_ao, NULL );
#endif
#endif
        if( !m_pRemoteTalk )
        {
            DEBUG_ERROR( "CAvAppMaster::Ad_Start_Remote_Talk memory not enough\n");
            return -1;
        }
    }

    return m_pRemoteTalk->StartTalk( pstuAudioNetPara );
}

int CHiAvDeviceMaster::Ad_Stop_Remote_Talk(const msgRequestAudioNetPara_t* pstuTalkParam)
{
    if( m_pRemoteTalk )
    {
        return m_pRemoteTalk->StopTalk(pstuTalkParam->talkback_type);
    }

    return -1;
}
#endif

#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
int CHiAvDeviceMaster::Ad_Init_AudioDev_Salve()
{
#if defined(_AV_HAVE_AUDIO_INPUT_)
    if( m_pHi_ai )
    {
        m_pHi_ai->HiAi_ai_init_aidev_mode(_AI_TALKBACK_);
    }
#else
    CHiAi ai;
    ai.HiAi_ai_init_aidev_mode(_AI_TALKBACK_);
#endif

    return 0;
}
#endif

#if defined(_AV_HAVE_AUDIO_INPUT_)
int CHiAvDeviceMaster::Ad_start_ai(ai_type_e ai_type)
{
    if (CHiAvDevice::Ad_start_ai(ai_type) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_ai (CHiAvDevice::Ad_start_ai) FAILED!\n");
        return -1;
    }
    return 0;
}

int CHiAvDeviceMaster::Ad_stop_ai(ai_type_e ai_type)
{
    if (CHiAvDevice::Ad_stop_ai(ai_type)!= 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_ai (CHiAvDevice::Ad_start_ai) FAILED!\n");
        return -1;
    }
    return 0;
}
#endif

#if defined(_AV_HAVE_AUDIO_OUTPUT_)
int CHiAvDeviceMaster::Ad_start_ao(ao_type_e ao_type)
{
    if (m_pHi_ao->HiAo_start_ao(ao_type) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_start_ao FAIELD (ao_type:%d)\n", ao_type);
        return -1;
    }
    
    return 0;
}

int CHiAvDeviceMaster::Ad_stop_ao(ao_type_e ao_type)
{
    if (m_pHi_ao->HiAo_stop_ao(ao_type) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_ao FAIELD (ao_type:%d)\n", ao_type);
        return -1;
    }
    
    return 0;
}
int CHiAvDeviceMaster::Ad_set_preview_start(bool start_or_stop)
{
    m_bPreviewAoStart = start_or_stop;
    m_bInterupt = !start_or_stop;//当预览有语音优先级模块停止预览时，UI界面没有发送状态标志，av根据stop消息自动设置播放状态
    
    if(start_or_stop == false)
    {
        while(m_audio_preview_running !=false)
        {
            usleep(100);
            printf("\nm_audio_preview_running\n");
        }
        this->Ad_stop_ao(_AO_PLAYBACK_CVBS_);//_AO_PLAYBACK_CVBS_
    }
    else
    {
        this->Ad_start_ao(_AO_PLAYBACK_CVBS_);
    }
    return 0;
}
int CHiAvDeviceMaster::Ad_set_preview_state(bool state)
{
    m_bstart = state;
    return 0;
}

#endif

int CHiAvDeviceMaster::Ad_switch_audio_chn(int type, unsigned int audio_chn)
{
    if (type == 0) // preview
    {
#if defined(_AV_HAVE_AUDIO_INPUT_) && defined(_AV_HAVE_AUDIO_OUTPUT_) && (defined(_AV_PRODUCT_CLASS_DVR_) || defined(_AV_PRODUCT_CLASS_HDVR_))
        m_preview_audio_ctrl.phy_chn = audio_chn;
#endif
        
#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
        m_audio_output_state.adec_chn = audio_chn;
#if  defined(_AV_PRODUCT_CLASS_HDVR_)
        if (pgAvDeviceInfo->Adi_get_video_source(audio_chn) == _VS_DIGITAL_)
        {
            Ad_preview_audio_ctrl(false);
        }
        else
        {
            Ad_preview_audio_ctrl(true);
        }
#endif
        
        Ad_switch_dec_auido_chn(VDP_PREVIEW_VDEC, audio_chn);
        Ad_switch_dec_auido_chn(VDP_SPOT_VDEC, audio_chn);
#endif
    }
    else if (type == 1) // playback
    {
#ifdef _AV_HAVE_VIDEO_DECODER_
        m_audio_output_state.adec_chn = audio_chn;
        Ad_switch_dec_auido_chn(VDP_PLAYBACK_VDEC, audio_chn);
#endif
    }
    
    return 0;
}

int CHiAvDeviceMaster::Ad_set_audio_mute(bool mute_flag)
{
     m_audio_output_state.audio_mute = mute_flag;
#ifdef _AV_HAVE_VIDEO_DECODER_
     Ad_switch_dec_auido_chn(VDP_PLAYBACK_VDEC, m_audio_output_state.adec_chn);
#endif

#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
    Ad_switch_dec_auido_chn( VDP_PREVIEW_VDEC, m_audio_output_state.adec_chn );
    Ad_switch_dec_auido_chn( VDP_SPOT_VDEC, m_audio_output_state.adec_chn );

    m_pHi_AvDec->Ade_set_mute_flag(VDP_PREVIEW_VDEC, m_audio_output_state.audio_mute);
    m_pHi_AvDec->Ade_set_mute_flag(VDP_SPOT_VDEC, m_audio_output_state.audio_mute);
#endif
    
    return 0;
}
bool CHiAvDeviceMaster::Ad_get_audio_mute()
{
    return m_audio_output_state.audio_mute;
}
int CHiAvDeviceMaster::Ad_set_audio_volume(unsigned char volume)
{
#ifdef _AV_PLATFORM_HI3535_
    AUDIO_DEV ao_dev;

    if(volume == 0)
    {
        if(m_audio_output_state.audio_mute == false)
        {
            Ad_set_audio_mute(true);
        }
    }
    else
    {
        if(!m_audio_output_state.audio_mute)
        {
            Ad_set_audio_mute(false);
        }
        // m_pHi_ao->HiAo_get_ao_info(_AO_HDMI_, &ao_dev, NULL, NULL);
        // m_pHi_ao->HiAo_set_ao_volume(ao_dev, volume);
        m_pHi_ao->HiAo_get_ao_info(_AO_PLAYBACK_CVBS_, &ao_dev, NULL, NULL);
        m_pHi_ao->HiAo_set_ao_volume(ao_dev, volume);
    }
#else
    if (!m_audio_output_state.audio_mute) // previously demute
    {
        Ad_set_audio_mute(volume == 0);
    }
    else //when mute 
    {
        Ad_set_audio_mute(volume == 0);
    }
#endif
    return 0;
}

#if defined(_AV_HAVE_AUDIO_INPUT_) && defined(_AV_HAVE_AUDIO_OUTPUT_)
void * Ad_preview_audio_thread_entry(void * pPara)
{
    CHiAvDeviceMaster *pHiAvDeviceMaster = static_cast<CHiAvDeviceMaster *>(pPara);
    pHiAvDeviceMaster->Ad_preview_audio_thread_body();

    return NULL;
}
int CHiAvDeviceMaster::Ad_start_preview_audio()
{
    m_preview_audio_ctrl.phy_chn = -1;
    m_preview_audio_ctrl.ai_type = _AI_NORMAL_;
    m_preview_audio_ctrl.audio_start = true;
    m_preview_audio_ctrl.thread_start = true;
    m_preview_audio_ctrl.thread_exit = false;
    m_preview_audio_ctrl.bAudioPauseState = false;
    m_bstart = true;
    m_bInterupt = false;//优先级比预览高时用此变量停预览
    m_bPreviewAoStart = false;
    m_audio_preview_running = false;
    DEBUG_MESSAGE( "CHiAvDeviceMaster::Ad_start_preview_audio start preview audio thread\n");
    Av_create_normal_thread(Ad_preview_audio_thread_entry, (void *)this, NULL);

    return 0;
}

int CHiAvDeviceMaster::Ad_preview_audio_ctrl(bool start_or_stop)
{
    if( start_or_stop )
    {
        m_preview_audio_ctrl.audio_start = start_or_stop;
    }
    else
    {
        m_preview_audio_ctrl.bAudioPauseState = false;
        m_preview_audio_ctrl.audio_start = start_or_stop;
        while(m_preview_audio_ctrl.thread_start && m_preview_audio_ctrl.bAudioPauseState == false )
        {
            DEBUG_MESSAGE("wait audio thread pause\n");
            usleep(10000);
        }
    }
    return 0;
}

void CHiAvDeviceMaster::Ad_preview_audio_thread_body()
{
    fd_set read_fds;
    timeval timeout;
    int ai_fd = -1;
    AUDIO_FRAME_S audio_frame;
    int phy_audio_chn = 0;
    ai_type_e ai_type = m_preview_audio_ctrl.ai_type;
    bool bLastPlay = false;
    bool bGetAudioFrame = false;	//避免未获取到音频帧，也释放音频帧的情况。

    while (m_preview_audio_ctrl.thread_start)
    {
        if (m_bstart || m_bInterupt || (m_preview_audio_ctrl.phy_chn == -1) || (m_preview_audio_ctrl.phy_chn > pgAvDeviceInfo->Adi_get_audio_chn_number()-1) || !m_preview_audio_ctrl.audio_start || m_audio_output_state.audio_mute) //wait for Gui's command!
        {
            bLastPlay = false;
            usleep(30 * 1000);
            m_preview_audio_ctrl.bAudioPauseState = true;
            m_audio_preview_running = false;//用于判断此值为false 时才停止AO 
            continue;
        }
        m_audio_preview_running = true;
        if( !pgAvDeviceInfo->Adi_get_audio_cvbs_ctrl_switch() )
        {
            usleep(30*1000);
            continue;
        }
        
        phy_audio_chn = m_preview_audio_ctrl.phy_chn;
        FD_ZERO(&read_fds);
        ai_fd = m_pHi_ai->HiAi_get_fd(ai_type, phy_audio_chn);
        FD_SET(ai_fd, &read_fds);
        timeout.tv_sec  = 1;
        timeout.tv_usec = 0;
        int ret_val = select(ai_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (ret_val < 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_preview_audio_thread_body get ai frame error!\n");
            sleep(1);
            continue;
        }
        else if (ret_val == 0)
        {
            DEBUG_ERROR( "CHiAvDeviceMaster::Ad_preview_audio_thread_body get ai frame time out\n");
            usleep(100 * 1000);
            continue;
        }
		bGetAudioFrame = false; //get audio frame failed
        if (FD_ISSET(ai_fd, &read_fds))
        {
           
            if (!m_bstart && m_bPreviewAoStart && m_preview_audio_ctrl.audio_start)
            {
				if (m_pHi_ai->HiAi_get_ai_frame(ai_type, phy_audio_chn, &audio_frame) != 0)
				{
					DEBUG_ERROR( "CHiAvDeviceMaster::Ad_preview_audio_thread_body HiAi_get_ai_frame error!\n");
					bGetAudioFrame = false; //get audio frame failed
				}
				else
				{
					bGetAudioFrame = true;
				}
            
                if( bLastPlay != true )
                {
                    bLastPlay = true;
#if 0
                    m_pHi_ao->HiAo_clear_ao_chnbuf(_AO_HDMI_);
// #else
                    m_pHi_ao->HiAo_stop_ao(_AO_HDMI_);
                    m_pHi_ao->HiAo_start_ao(_AO_HDMI_);
#endif
                    
                    DEBUG_MESSAGE( "CHiAvDeviceMaster::Ad_preview_audio_thread_body clear HDMI ao\n");
                }
                /*
                if (m_pHi_ao->HiAo_send_ao_frame(_AO_HDMI_, &audio_frame) != 0)
                {
                    DEBUG_ERROR( "CHiAvDeviceMaster::Ad_preview_audio_thread_body HiAo_send_ao_frame hdmi error!\n");
                }*/
                //DEBUG_MESSAGE("send audio chn=%d\n", phy_audio_chn);
               // if (true == pgAvDeviceInfo->Adi_get_audio_cvbs_ctrl_switch())
				if(bGetAudioFrame && !m_bstart)
				{
					if (m_pHi_ao->HiAo_send_ao_frame(_AO_PLAYBACK_CVBS_, &audio_frame) != 0)
					{
						DEBUG_ERROR( "CHiAvDeviceMaster::Ad_preview_audio_thread_body HiAo_send_ao_frame cvbs error!\n");
					}
				}
            }
            else
            {
                bLastPlay = false;
            }
            
            if(bGetAudioFrame && (m_pHi_ai->HiAi_release_ai_frame(ai_type, phy_audio_chn, &audio_frame) != 0))
            {
                DEBUG_ERROR( "CHiAvDeviceMaster::Ad_preview_audio_thread_body HiAi_release_ai_frame error!\n");
            }

        }
    }

    m_preview_audio_ctrl.thread_exit = true;

    return;
}
#endif

#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
int CHiAvDeviceMaster::Ad_UpdateDevOnlineState( unsigned char szState[SUPPORT_MAX_VIDEO_CHANNEL_NUM] )
{
    for( int ch = 0; ch < pgAvDeviceInfo->Adi_max_channel_number(); ch++ )
    {   
        if( szState[ch] != 3)
        {
            m_u8DevOlState[ch] = szState[ch];
        }
    }

    if( m_pHi_AvDec )
    {
        m_pHi_AvDec->Ade_set_online_state( VDP_PREVIEW_VDEC, szState );
        m_pHi_AvDec->Ade_set_online_state( VDP_SPOT_VDEC, szState );
    }
    
    return 0;
}

int CHiAvDeviceMaster::Ad_UpdateDevMap( char szChannelMap[SUPPORT_MAX_VIDEO_CHANNEL_NUM] )
{
	DEBUG_MESSAGE( "#######CHiAvDeviceMaster::Ad_UpdateDevMap\n");
	
	memcpy( m_c8ChannelMap, szChannelMap, sizeof(m_c8ChannelMap) );

    if( m_pHi_AvDec )
    {
        m_pHi_AvDec->Ade_set_prewdec_channel_map( VDP_PREVIEW_VDEC, m_c8ChannelMap );
        m_pHi_AvDec->Ade_set_prewdec_channel_map( VDP_SPOT_VDEC, m_c8ChannelMap );
    }
#if defined(_AV_HAVE_VIDEO_ENCODER_)

	if( !m_pSnapTask && m_pHi_vi && m_pAv_encoder && m_pHi_vpss)
    {
        m_pSnapTask = new CAvSnapTask( m_pHi_vi, (CHiAvEncoder*)m_pAv_encoder, m_pHi_vpss );
        
    }
#endif	
    return 0;
}

int CHiAvDeviceMaster::Ad_ObtainChannelMap( char szChannelMap[SUPPORT_MAX_VIDEO_CHANNEL_NUM])
{
   memcpy( szChannelMap, m_c8ChannelMap, sizeof(m_c8ChannelMap) ); 
   return 0;
}


int CHiAvDeviceMaster::Ad_RestartDecoder()
{
    if( m_pHi_AvDec )
    {
        m_pHi_AvDec->Ade_restart_preview_decoder( VDP_PREVIEW_VDEC );
        m_pHi_AvDec->Ade_restart_preview_decoder( VDP_SPOT_VDEC );
    }

    return 0;
}

int CHiAvDeviceMaster::Ad_SetSystemUpgradeState( int state )
{
    if( m_pHi_AvDec )
    {
        m_pHi_AvDec->Ade_set_system_upgrade_state( VDP_PREVIEW_VDEC, state );
        m_pHi_AvDec->Ade_set_system_upgrade_state( VDP_SPOT_VDEC, state );
    }

    return 0;
}

#endif //endif #if defined(_AV_PRODUCT_CLASS_NVR_)

#if defined(_AV_PRODUCT_CLASS_IPC_)
int CHiAvDeviceMaster::Ad_SetCameraAttribute( unsigned int u32Mask, const paramCameraAttribute_t* pstuParam )
{
    DEBUG_MESSAGE( "CHiAvDeviceMaster::Ad_SetCameraAttribute mask 0x%x\n", u32Mask);
    int iResult = 0;

    if( (u32Mask>>6) & 0x1 )
    {
        iResult += CHiAvDevice::Ad_SetCameraLightFrequency( pstuParam->u8LightFrequency );
    }
    if( ((u32Mask>>11) & 0x1) || ((u32Mask>>12) & 0x1) )
    {
        iResult += CHiAvDevice::Ad_SetCameraMirrorFlip( pstuParam->u8Mirror, pstuParam->u8Flip );
    }
    if( ((u32Mask>>1) & 0x1) )
    {
        iResult += CHiAvDevice::Ad_SetCameraDayNightMode( pstuParam->u8DayNight);
    }
    if( ((u32Mask>>14) & 0x1) )
    {
        iResult += CHiAvDevice::Ad_SetCameraLampMode( pstuParam->u8Light);
    }
    if( ((u32Mask) & 0x1) )
    {
        iResult += CHiAvDevice::Ad_SetCameraOccasion( pstuParam->u8WhiteBlance );
    }
    if( ((u32Mask>>2) & 0x1) )
    {
        iResult += CHiAvDevice::Ad_SetCameraExposure( pstuParam->u8ExposureMode?1:0, pstuParam->u8ExposureMode );
    }
    if( ((u32Mask>>9) & 0x1) )
    {
        iResult += CHiAvDevice::Ad_SetCameraGain( pstuParam->u8GainMode?1:0, pstuParam->u8GainMode );
    }
    if( ((u32Mask>>7) & 0x1) )
    {
        iResult += Ad_SetCameraBackLightLevel( pstuParam->u8BackLight );
    }

    if( ((u32Mask>>16) & 0x1) )
    {
        iResult += CHiAvDevice::Ad_SetLdcParam( pstuParam->u8LDCEnable );    
    }

    if( pgAvDeviceInfo->Adi_get_wdr_control_switch() )
    {
        if( ((u32Mask>>17) & 0x1) || ((u32Mask>>18) & 0x1) )
        {
            iResult += CHiAvDevice::Ad_SetWdrParam( pstuParam->u8WDREnable, pstuParam->u8WDRStrength );
        }
    }
//!Added Nov 2014   
    if(((u32Mask>>20) & 0x1))
    {
        if(pstuParam->u8SpecMode  >  1)
        {
            _AV_KEY_INFO_(_AVD_DEVICE_, "the spec mode(%d ) is invalid! \n", pstuParam->u8SpecMode);
        }
        else
        {
            iResult += CHiAvDevice::Ad_SetApplicationScenario(pstuParam->u8SpecMode);
        }
    }

    if(true == pgAvDeviceInfo->Adi_get_height_light_comps_switch())
    {
        if(((u32Mask>>19) & 0x1))
        {
            iResult += CHiAvDevice::Ad_SetHeightLightCompensation(pstuParam->u8HLC);
        }
    }

    return iResult;
}

int CHiAvDeviceMaster::Ad_SetCameraColor( const paramVideoImage_t* pstuParam )
{
    int iResult = 0;

    iResult += CHiAvDevice::Ad_SetVideoColor( pstuParam->ucLuminosity, pstuParam->ucChromaticity, pstuParam->ucContrast, pstuParam->ucSaturation );
    iResult += CHiAvDevice::Ad_SetCameraSharpen( pstuParam->ucSharpness );

    return iResult;
}

int CHiAvDeviceMaster::Ad_SetEnvironmentLuma( unsigned int u32Luma )
{
    return CHiAvDevice::Ad_SetEnvironmentLuma( u32Luma );
}

int CHiAvDeviceMaster::Ad_GetIspControl( unsigned char& u8OpenIrCut, unsigned char& u8CameraLampLevel, unsigned char& u8IrisLevel, unsigned char& u8IspFrameRate, unsigned int& u32PwmHz )
{
    cmd2AControl_t stuParam;

    if( CHiAvDevice::Ad_GetIspControlCmd( &stuParam ) != 0 )
    {
        return -1;
    }

    u8OpenIrCut = stuParam.bOpenIrCut;
    u8CameraLampLevel = stuParam.u8LampLightLevel;
    u8IrisLevel = stuParam.u8IrisLevel;
    u8IspFrameRate = stuParam.u8FrameSlowRate;
    u32PwmHz =stuParam.u32PwmHz;

    return 0;
}

int CHiAvDeviceMaster::Ad_SetVideoColorGrayMode( unsigned char u8ColorMode )
{
    return CHiAvDevice::Ad_SetColorGrayMode( u8ColorMode );
}

int CHiAvDeviceMaster:: Ad_SetAEAttrStep(unsigned char u8StepModule)
{
    return CHiAvDevice::Ad_SetISPAEAttrStep( u8StepModule );
}

int CHiAvDeviceMaster::Ad_SetDayNightCutLuma( unsigned int u32Mask, unsigned int u32Day2NightValue, unsigned int u32Night2DayValue )
{
    return CHiAvDevice::Ad_SetDayNightCutLuma( u32Mask, u32Day2NightValue, u32Night2DayValue );
}

int CHiAvDeviceMaster::Ad_set_macin_volum( unsigned char u8Volume )
{
#if defined(_AV_PRODUCT_CLASS_IPC_)
    if(PTD_712_VD == pgAvDeviceInfo->Adi_product_type())
    {
        return m_pHi_ai->HiAi_ai_set_input_volume( 42-u8Volume );
    }
    else
    {
        return m_pHi_ai->HiAi_ai_set_input_volume( 36-u8Volume );
    }
#else
    if( u8Volume > 15 )
    {
        u8Volume = 15;
    }
    
    return m_pHi_ai->HiAi_ai_set_input_volume( 15 - u8Volume );
#endif
}

#endif // end #if defined(_AV_PRODUCT_CLASS_IPC_)

#if defined(_AV_HAVE_VIDEO_ENCODER_)
int CHiAvDeviceMaster:: Ad_get_encoder_used_resource(int *pCifCount)
{
    return m_pAv_encoder->Ae_get_encoder_used_resource(pCifCount) ;
}

bool CHiAvDeviceMaster:: Ad_check_encoder_resolution_exist( av_resolution_t eResolution, int *pResolutionNum)
{
    return m_pAv_encoder->Ae_whether_encoder_have_this_resolution( eResolution, pResolutionNum);
}

int CHiAvDeviceMaster::Ad_start_snap()
{
	DEBUG_MESSAGE( "#########CHiAvDeviceMaster::Ad_start_snap\n");
    if( !m_pSnapTask )
    {
        m_pSnapTask = new CAvSnapTask( m_pHi_vi, (CHiAvEncoder*)m_pAv_encoder, m_pHi_vpss );
    }

    m_pSnapTask->StartSnapThread();

    return 0;
}

int CHiAvDeviceMaster::Ad_stop_snap()
{
    DEBUG_MESSAGE( "Ad_stop_snap\n");
    if( !m_pSnapTask )
    {
        m_pSnapTask = new CAvSnapTask( m_pHi_vi, (CHiAvEncoder*)m_pAv_encoder, m_pHi_vpss );
    }

    m_pSnapTask->StopSnapThread();

    return 0;
}
int CHiAvDeviceMaster::Ad_Set_Chn_state(int flag)
{
    DEBUG_MESSAGE( "#########CHiAvDeviceMaster::Ad_start_snap\n");
    if( !m_pSnapTask )
    {
        m_pSnapTask = new CAvSnapTask( m_pHi_vi, (CHiAvEncoder*)m_pAv_encoder, m_pHi_vpss );
    }
    m_pSnapTask->SetChnState(flag);
    return 0;
}

int CHiAvDeviceMaster::Ad_Get_Chn_state()
{
    if( !m_pSnapTask )
    {
        m_pSnapTask = new CAvSnapTask( m_pHi_vi, (CHiAvEncoder*)m_pAv_encoder, m_pHi_vpss );
    }
    return m_pSnapTask->GetChnState();
}
int CHiAvDeviceMaster::Ad_modify_snap_param( av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para )
{
    if( video_stream_type != _AST_MAIN_VIDEO_ ) 
    {
        return 0;
    }
    
    if( !m_pSnapTask )
    {
        m_pSnapTask = new CAvSnapTask( m_pHi_vi, (CHiAvEncoder*)m_pAv_encoder, m_pHi_vpss );
    }
    //DEBUG_MESSAGE("===========#################################################modify snap param\n");
    if( m_pSnapTask )
    {
        if( video_stream_type == _AST_MAIN_VIDEO_ )
        {
            return m_pSnapTask->UpdateSnapParam( phy_video_chn_num, pVideo_para );
        }
        return 0;
    }
    
    return -1;
}

int CHiAvDeviceMaster::Ad_cmd_snap_picutres( /*const MsgSnapPicture_t* pstuSnapParam*/ )
{
#if 0
    MsgSnapPicture_t stuSnapParam;
    memcpy( &stuSnapParam, pstuSnapParam, sizeof(MsgSnapPicture_t) );

    for( int ch=0; ch<pgAvDeviceInfo->Adi_max_channel_number(); ch++ )
    {
        if( (pstuSnapParam->Channel >> ch ) & 0x01 )
        {
            if (pgAvDeviceInfo->Adi_get_video_source(ch) == _VS_DIGITAL_)
            {
                stuSnapParam.Channel = (stuSnapParam.Channel & (~(1<<ch)) );
                return 0;
            }   
        }
    }

    if( stuSnapParam.Channel == 0 )
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_cmd_snap_picutres no channel valid, 0x%x->0x%x\n", pstuSnapParam->Channel, stuSnapParam.Channel );
        return 0;
    }

    if( !m_pSnapTask )
    {
        m_pSnapTask = new CAvSnapTask( m_pHi_vi, (CHiAvEncoder*)m_pAv_encoder, m_pHi_vpss );
    }

    if( m_pSnapTask )
    {
        m_pSnapTask->AddSnapCmd( &stuSnapParam );
    }
#endif
    return 0;
}

int CHiAvDeviceMaster::Ad_cmd_signal_snap_picutres( AvSnapPara_t &pstuSignalSnapParam )
{

    DEBUG_WARNING("Ad_cmd_signal_snap_picutres\n");
    //AvSnapPara_t stuSignalSnapParam;
    //memcpy( &stuSignalSnapParam, pstuSignalSnapParam, sizeof(AvSnapPara_t) );

    if( pstuSignalSnapParam.uiChannel == 0 )
    {
        //DEBUG_ERROR( "CHiAvDeviceMaster::Ad_cmd_signal_snap_picutres no channel valid, 0x%x->0x%x\n", stuSignalSnapParam.uiChannel, pstuSignalSnapParam->uiChannel );
        return 0;
    }

    if( !m_pSnapTask )
    {
        m_pSnapTask = new CAvSnapTask( m_pHi_vi, (CHiAvEncoder*)m_pAv_encoder, m_pHi_vpss );
    }
    
    for( int ch=0; ch<pgAvDeviceInfo->Adi_max_channel_number(); ch++ )
    {
        if( (pstuSignalSnapParam.uiChannel>> ch ) & 0x01 )
        {
            if (pgAvDeviceInfo->Adi_get_video_source(ch) == _VS_DIGITAL_) // 1表示的模拟通道
            {
                if(pstuSignalSnapParam.uiSnapType != 5)
                {
                    pstuSignalSnapParam.uiChannel = (pstuSignalSnapParam.uiChannel & (~(1<<ch)) );
                }
                
                //DEBUG_MESSAGE("lsh snap chn =%d is digital share memory lock state =%d\n", ch, m_pSnapTask->GetShareMenLockState(ch));
#ifdef _AV_PRODUCT_CLASS_IPC_                
                if(m_pSnapTask->GetShareMenLockState(ch) == 0)//数字解锁
                {
                    m_pSnapTask->SetShareMenLock(ch, 1);
                }
                else
                {
                    //m_pSnapTask->SetShareMenLock(ch, 1);
                }
                //DEBUG_MESSAGE("lsh after snap chn =%d is digital share memory lock state =%d\n", ch, m_pSnapTask->GetShareMenLockState(ch));
#endif
            } 
            else
            {
#ifdef _AV_PRODUCT_CLASS_IPC_
                //DEBUG_MESSAGE("lsh snap chn =%d is analog share memory lock state =%d\n", ch, m_pSnapTask->GetShareMenLockState(ch));
                if(m_pSnapTask->GetShareMenLockState(ch) == 0)//模拟加锁
                {
                    //m_pSnapTask->SetShareMenLock(ch, 0);
                }
                else
                {
                    m_pSnapTask->SetShareMenLock(ch, 0);
                }
#endif
                //m_pSnapTask->SetShareMenLockState(ch, 0);
               // DEBUG_MESSAGE("lsh after snap chn =%d is analog share memory lock state =%d\n", ch, m_pSnapTask->GetShareMenLockState(ch));
            }
        }
    }
    
    if( m_pSnapTask )
    {
        m_pSnapTask->AddSignalSnapCmd( pstuSignalSnapParam );
    }
    
    return 0;
}

int CHiAvDeviceMaster::Ad_get_snap_serial_number( msgMachineStatistics_t* pstuMachineStatistics)
{
	if(pstuMachineStatistics == NULL)
	{
		return -1;
	}
	else
	{
		if( !m_pSnapTask )
	    {
	        m_pSnapTask = new CAvSnapTask( m_pHi_vi, (CHiAvEncoder*)m_pAv_encoder, m_pHi_vpss );
	    }

	    if( m_pSnapTask )
	    {
	        m_pSnapTask->SetSnapSerialNumber( pstuMachineStatistics );
	    }

	    return 0;
	}
}
int CHiAvDeviceMaster::Ad_get_snap_result(msgSnapResultBoardcast *snapresultitem)
{
	//std::list<msgSnapResultBoardcast *> snapresult;
	//if( !m_pSnapTask )
    //{
       // m_pSnapTask = new CAvSnapTask( m_pHi_vi, (CHiAvEncoder*)m_pAv_encoder, m_pHi_vpss );
    //}

    //if( m_pSnapTask )
    //{
        //snapresult = m_pSnapTask->GetSnapResult();
    //}
    if(snapresultitem)
    {
    	return m_pSnapTask->GetSnapResult(snapresultitem);
    }

	return -1;
} 

int CHiAvDeviceMaster::Ad_set_osd_info(av_video_encoder_para_t  *pVideo_para, int chn, rm_uint16_t usOverlayBitmap)
{
	//DEBUG_MESSAGE( "CHiAvDeviceMaster::Ad_set_osd_info \n");
	if( m_pSnapTask )
    {
        return m_pSnapTask->SetOsdInfo(pVideo_para, chn, usOverlayBitmap);
    }
	return -1;
}
#ifdef _AV_PRODUCT_CLASS_IPC_
int CHiAvDeviceMaster::Ad_snap_clear_all_pictures()
{
    if( m_pSnapTask )
    {
        return m_pSnapTask->ClearAllSnapPictures();
    }
    return -1;
}
#endif

#endif

#if defined(_AV_TEST_MAINBOARD_)
int CHiAvDeviceMaster::Ad_test_audio_IO()
{
#ifdef _AV_HAVE_AUDIO_INPUT_
    m_pHi_ai->HiAi_start_ai(_AI_TALKBACK_);
#endif

#if defined(_AV_HAVE_AUDIO_INPUT_) && defined(_AV_HAVE_AUDIO_OUTPUT_)
    int ret_val = 0;
    MPP_CHN_S src_chn, dest_chn;
    src_chn.enModId = HI_ID_AI;
    m_pHi_ai->HiAi_get_ai_info(_AI_TALKBACK_, 0, &src_chn.s32DevId, &src_chn.s32ChnId);

    dest_chn.enModId = HI_ID_AO;
    m_pHi_ao->HiAo_get_ao_info(_AO_TALKBACK_, &dest_chn.s32DevId, &dest_chn.s32ChnId);

    if ((ret_val = HI_MPI_SYS_Bind(&src_chn, &dest_chn)) != 0)
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_test_audio_IO HI_MPI_SYS_Bind FAILED! (0x%08lx) \n", (unsigned long)ret_val);
        return -1;
    }
    
#ifdef _AV_PRODUCT_CLASS_NVR_
    Ad_start_talkback_audio();
#endif

#endif

    return 0;
}

int CHiAvDeviceMaster::Ad_start_talkback_audio()
{
    if (pgAvDeviceInfo->Adi_app_type() != APP_TYPE_TEST)
    {
        return 0;
    }
    
    m_preview_audio_ctrl.phy_chn = 0;
    m_preview_audio_ctrl.ai_type = _AI_TALKBACK_;
    m_preview_audio_ctrl.audio_start = true;
    m_preview_audio_ctrl.thread_start = true;
    m_preview_audio_ctrl.thread_exit = false;
    DEBUG_MESSAGE( "CHiAvDeviceMaster::Ad_start_talkback_audio start preview audio thread\n");
    Av_create_normal_thread(Ad_preview_audio_thread_entry, (void *)this, NULL);

    return 0;
}
#endif

#if defined(_AV_HAVE_VIDEO_OUTPUT_)
int CHiAvDeviceMaster::Ad_stop_one_chn_output(av_output_type_t output, int index, int type, int chn)
{
    std::map<av_output_type_t, std::map<int, vo_chn_state_info_t> >::iterator vo_chn_state_it = m_vo_chn_state.find(output);
    int max_vo_chn_num, max_video_chn_num, chn_index;
    HI_S32 ret_val = 0;
#if defined(_AV_HISILICON_VPSS_)
    vpss_purpose_t vpss_purpose;
#endif

    if(!m_video_preview_switch.test(chn))
    {
        DEBUG_ERROR( "stop: chn %d have not preview!\n", chn);
        return 0;
    }
    if((vo_chn_state_it == m_vo_chn_state.end()) || (vo_chn_state_it->second.find(index) == vo_chn_state_it->second.end()) || (m_vo_chn_state[output][index].split_mode == _AS_UNKNOWN_))
    {
        DEBUG_WARNING( "CHiAvDeviceMaster::Ad_stop_video_output WARNING: There is no video output output=%d index=%d type=%d\n", output, index, type);
        return 0;
    }

    if(m_vo_chn_state[output][index].split_mode == _AS_UNKNOWN_)
    {
        DEBUG_WARNING( "CHiAvDeviceMaster::Ad_stop_video_output WARNING: split_mode %d error output=%d index=%d type=%d\n", m_vo_chn_state[output][index].split_mode, output, index, type);
        return 0;
    }
    max_video_chn_num = pgAvDeviceInfo->Adi_max_channel_number();
    if((chn < 0) || (chn >= max_video_chn_num))
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_video_output error: chn %d is invalid\n", chn);
        return -1;
    }
    max_vo_chn_num = pgAvDeviceInfo->Adi_get_split_chn_info(m_vo_chn_state[output][index].split_mode);
    for(chn_index = 0; chn_index < max_vo_chn_num && chn_index < max_video_chn_num; chn_index++)
    {
        if(m_vo_chn_state[output][index].chn_layout[chn_index] == chn)
        {
            Ad_get_vpss_purpose(output, index, type, chn, vpss_purpose, false);
            if((vpss_purpose == _VP_PREVIEW_VDEC_) || (vpss_purpose == _VP_SPOT_VDEC_) || (vpss_purpose == _VP_PREVIEW_VI_)\
                || (vpss_purpose == _VP_SPOT_VI_) || (vpss_purpose == _VP_PLAYBACK_VDEC_))
            {
                ret_val = Ad_control_vpss_vo(output, index, vpss_purpose, chn, chn_index, 1);           
            }
            m_pHi_vo->HiVo_clear_vo(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), chn_index);
            Ad_show_access_logo(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), chn_index, true);
            break;
        }
    }
    return ret_val;
}

int CHiAvDeviceMaster::Ad_stop_preview_output_chn(av_output_type_t output, int index, int chn)
{
    return Ad_stop_one_chn_output(output, index, 0, chn);
}

int CHiAvDeviceMaster::Ad_set_preview_enable_switch(int chn_index, int enable)	
{
	m_preview_enable[chn_index] = enable;

	return 0;
}

int CHiAvDeviceMaster::Ad_get_preview_enable_switch(int chn_index)	
{
	return m_preview_enable[chn_index];
}

int CHiAvDeviceMaster::Ad_start_one_chn_output(av_output_type_t output, int index, int type, int chn)
{
    std::map<av_output_type_t, std::map<int, vo_chn_state_info_t> >::iterator vo_chn_state_it = m_vo_chn_state.find(output);
    int max_vo_chn_num, max_video_chn_num, chn_index;
    HI_S32 ret_val = 0;
#if defined(_AV_HISILICON_VPSS_)
    vpss_purpose_t vpss_purpose;
#endif

    if(!m_video_preview_switch.test(chn))
    {
        DEBUG_ERROR( "start: chn %d have not preview!\n", chn);
        return 0;
    }
    if((vo_chn_state_it == m_vo_chn_state.end()) || (vo_chn_state_it->second.find(index) == vo_chn_state_it->second.end()) || (m_vo_chn_state[output][index].split_mode == _AS_UNKNOWN_))
    {
        DEBUG_WARNING( "CHiAvDeviceMaster::Ad_stop_video_output WARNING: There is no video output output=%d index=%d type=%d\n", output, index, type);
        return 0;
    }

    if(m_vo_chn_state[output][index].split_mode == _AS_UNKNOWN_)
    {
        DEBUG_WARNING( "CHiAvDeviceMaster::Ad_stop_video_output WARNING: split_mode %d error output=%d index=%d type=%d\n", m_vo_chn_state[output][index].split_mode, output, index, type);
        return 0;
    }
    max_video_chn_num = pgAvDeviceInfo->Adi_max_channel_number();
    if((chn < 0) || (chn >= max_video_chn_num))
    {
        DEBUG_ERROR( "CHiAvDeviceMaster::Ad_stop_video_output error: chn %d is invalid\n", chn);
        return -1;
    }
    max_vo_chn_num = pgAvDeviceInfo->Adi_get_split_chn_info(m_vo_chn_state[output][index].split_mode);
    for(chn_index = 0; chn_index < max_vo_chn_num && chn_index < max_video_chn_num; chn_index++)
    {
        if(m_vo_chn_state[output][index].chn_layout[chn_index] == chn)
        {
            Ad_show_access_logo(m_pHi_vo->HiVo_get_vo_dev(output, index, 0), chn_index, false);

            Ad_get_vpss_purpose(output, index, type, chn, vpss_purpose, false);
            if((vpss_purpose == _VP_PREVIEW_VDEC_) || (vpss_purpose == _VP_SPOT_VDEC_) || (vpss_purpose == _VP_PREVIEW_VI_)\
                || (vpss_purpose == _VP_SPOT_VI_) || (vpss_purpose == _VP_PLAYBACK_VDEC_))
            {
                ret_val = Ad_control_vpss_vo(output, index, vpss_purpose, chn, chn_index, 0);
            }
            break;
        }
    }
    return ret_val;
}

int CHiAvDeviceMaster::Ad_start_preview_output_chn(av_output_type_t output, int index, int chn)
{
    return Ad_start_one_chn_output(output, index, 0, chn);
}
#endif

#if defined(_AV_HAVE_VIDEO_OUTPUT_)
int CHiAvDeviceMaster::calculate_margin_for_limit_resource_machine(av_vag_hdmi_resolution_t resolution,int margin[4])
{
    if(0)
    {
        if(_AVHR_1080p24_ == resolution || _AVHR_1080p25_ == resolution || _AVHR_1080p30_ == resolution || \
            _AVHR_1080i50_ == resolution || _AVHR_1080i60_ == resolution || _AVHR_1080p50_ == resolution || \
            _AVHR_1080p60_ == resolution)
        {
            double  hor_scales = (double)1280 / 1920;
            double  ver_scales = (double)720 / 1080;
            margin[_AV_MARGIN_TOP_INDEX_] = (int) (margin[_AV_MARGIN_TOP_INDEX_] * ver_scales);
            margin[_AV_MARGIN_BOTTOM_INDEX_] = (int) (margin[_AV_MARGIN_BOTTOM_INDEX_] * ver_scales);
            margin[_AV_MARGIN_LEFT_INDEX_] = (int)(margin[_AV_MARGIN_LEFT_INDEX_] * hor_scales);
            margin[_AV_MARGIN_RIGHT_INDEX_] = (int) (margin[_AV_MARGIN_RIGHT_INDEX_] * hor_scales);
        }
    }
    return 0;
}
#endif

#if !defined(_AV_PRODUCT_CLASS_IPC_)

//!for TTS
#if defined(_AV_HAVE_TTS_)
int CHiAvDeviceMaster::Ad_start_tts(msgTTSPara_t * pTTS_param)
{
    //it might be unnecessary to check the param again
    return m_pAv_tts->Start_tts(pTTS_param);
}
bool CHiAvDeviceMaster::Ad_SET_TTS_LOOP_STATE(bool state)
{
    return m_pAv_tts->SetTTSLoopState(state);
}
int CHiAvDeviceMaster::Ad_stop_tts()
{
    return m_pAv_tts->Stop_tts();
}

int CHiAvDeviceMaster::Ad_query_tts()
{
    return m_pAv_tts->Obtain_tts_status();
}

int CHiAvDeviceMaster::Ad_init_tts()
{
    return m_pAv_tts->Init_tts();
}

#endif

int CHiAvDeviceMaster::Ad_start_report_station()
{
    DEBUG_WARNING("CHiAvDeviceMaster::Ad_start_report_station\n");
    if( !m_pAvStation )
    {
        m_pAvStation = new CAvReportStation();
    }
    //2015 0306 
    //m_pAvStation->SetTaskRuningState(false);
    m_pAvStation->StartReportStationThread();
    return 0;
}

int CHiAvDeviceMaster::Ad_stop_report_station()
{
	DEBUG_WARNING("CHiAvDeviceMaster::Ad_stop_report_station\n");
    if( !m_pAvStation )
    {
        m_pAvStation = new CAvReportStation();
    }
    m_pAvStation->StopReportStationThread();
    return 0;
}

int CHiAvDeviceMaster::Ad_loop_report_station(bool bloop)
{
	DEBUG_WARNING("CHiAvDeviceMaster::Ad_loop_report_station\n");
    if( !m_pAvStation )
    {
        //m_pAvStation = new CAvReportStation();
        return -1;
    }
    m_pAvStation->SetLoopTask(bloop);
    return 0;
}

int CHiAvDeviceMaster::Ad_add_report_station_task(msgReportStation_t * pstuStationParam)
{
    DEBUG_WARNING("CHiAvDeviceMaster::Ad_add_report_station_task\n");
    
    if(pstuStationParam == NULL)
	{
		return -1;
	}
	else
	{
        //msgReportStation_t stuStationParam;
        //memcpy( &stuStationParam, pstuStationParam, sizeof(msgReportStation_t) );
        if(!m_pAvStation)
        {
            m_pAvStation = new CAvReportStation(); 
        }
        m_pAvStation->ClearListTask();
        for(int i=0; i<pstuStationParam->audioCount; ++i)
        {
	        //m_pAvStation->AddReportStationTask(stuStationParam);
	        //DEBUG_WARNING("---chn=%d\n",pstuStationParam->RightLeftChn);
	        DEBUG_WARNING("---addr =%s\n",pstuStationParam->AudioAddr[0]+i*256);
	        m_pAvStation->AddReportStationTask(pstuStationParam->AudioAddr[0]+i*256, pstuStationParam->RightLeftChn);
	    }
        
	    //return 0;
	}
    return 0;
}
int CHiAvDeviceMaster::Ad_get_report_station_result()
{
    
    DEBUG_DEBUG("-----Ad_get_report_station_result-----\n");
    int result = AUDIO_NO_PLAY;
    
    if(!m_pAvStation)
    {
        m_pAvStation = new CAvReportStation();
    }
    
    bool bstate = m_pAvStation->GetReportState();
    
    if(m_pAvStation->GetReportStationResult() >0 )
    {
        DEBUG_DEBUG("-----PLAYING------\n");
        result = AUDIO_PLAY_PLAYING;
        
    }
    else
    {
        if(bstate) /**>!如果没有发停止报站消息前查询播放状态时为播放完成，如果停止报站后查询此状态为未播放状态*/
        {
            DEBUG_DEBUG("-----PLAY FINISH------\n");
            result = AUDIO_PLAY_FINISH;
        }
        else
        {
            DEBUG_DEBUG("-----NO PLAY------\n");
            result = AUDIO_NO_PLAY;
        }
    }
    
	return result;
}



#endif




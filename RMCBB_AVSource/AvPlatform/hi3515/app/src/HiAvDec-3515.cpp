#include <stdio.h>
#include <string.h>
#include "HiAvDec-3515.h"
#if !defined(_AV_PLATFORM_HI3515_)
#include "audio_amr_adp.h"
#endif
#ifdef _AV_USE_AACENC_
#ifndef _AV_PLATFORM_HI3515_
#include "audio_aac_adp.h"
#endif
#endif

using namespace Common;
#define PRINTF(fmt...) do{printf("[CHiAvDec %s line:%d]", __FUNCTION__, __LINE__); \
                                     printf(fmt);}while(0);
CHiAvDec::CHiAvDec()
{
    for (int chn = 0; chn < MAX_AUDIO_CHNNUM; chn++)
    {
        m_eAudioPlayLoad[chn] = APT_INVALID;
    }

    m_pHi_vpss = NULL;
    m_pHi_Ao = NULL;
    m_pHi_Vo = NULL;
    m_bStartFlag = true;
    m_pMuiltThreadDecLock = new CAvThreadLock();
}

CHiAvDec::~CHiAvDec()
{
    if( m_pMuiltThreadDecLock )
    {
        delete m_pMuiltThreadDecLock;
        m_pMuiltThreadDecLock = NULL;
    }
}

int CHiAvDec::Ade_get_vdec_max_size(int purpose, int chn, int &max_width, int &max_height)
{
    AVDecInfo_t *pAvDecInfo = NULL;
    if (VDP_PLAYBACK_VDEC != purpose)
    {
        int index = -1;
        for (int i = 0; i < pgAvDeviceInfo->Adi_max_channel_number(); i++)
        {
            if (Ade_get_avdev_info(purpose, i, &pAvDecInfo) < 0)
            {
                continue;
            }
            
            if (pAvDecInfo->phy_chn == chn)
            {
                index = i;
                break;
            }
        }
        
        if (index < 0)
        {
            DEBUG_ERROR("purpose %d, chn %d get illegal\n", purpose, chn);
            return -1;
        }
    }
    
    else
    {
        if (Ade_get_avdev_info(purpose, chn, &pAvDecInfo) < 0)
        {
            return -1;
        }
    }

    if (pAvDecInfo->VdecChn < 0 || pAvDecInfo->PicWidth < 0 || pAvDecInfo->PicHeight < 0)
    {
        return -1;
    }

    max_width = pAvDecInfo->PicWidth;
    max_height = pAvDecInfo->PicHeight;

    return 0;
}

int CHiAvDec::Ade_get_vdec_video_size_for_ui(int purpose, int phychn, int &max_width, int &max_height)
{
    if( purpose == VDP_PREVIEW_VDEC )
    {
        if( m_pAvPreviewDecoder[purpose] )
        {
            phychn = m_pAvPreviewDecoder[purpose]->GetChannelIndex(phychn);
            if( phychn < 0 )
            {
                return -1;
            }
        }
    }

    AVDecInfo_t *pAvDecInfo = NULL;

    if (Ade_get_avdev_info(purpose, phychn, &pAvDecInfo) < 0)
    {
        return -1;
    }

    if (pAvDecInfo->VdecChn < 0 || pAvDecInfo->PicWidth < 0 || pAvDecInfo->PicHeight < 0)
    {
        return -1;
    }
    max_width = pAvDecInfo->PicWidth;
    max_height = pAvDecInfo->PicHeight;

    return 0;
}

int CHiAvDec::Ade_get_decocer_chn_index( int purpose, int phychn )
{
    if( purpose != VDP_PREVIEW_VDEC )
    {
        return phychn;
    }
    
    return m_pAvPreviewDecoder[purpose]->GetChannelIndex(phychn);
}


int CHiAvDec::Ade_get_vdec_info(int purpose, int chn, int &Vdec_dev, VDEC_CHN &Vdec_chn)
{
    AVDecInfo_t *pAvDecInfo = NULL;

    if (VDP_PLAYBACK_VDEC != purpose)
   {
        int index = -1;
        for (int i = 0; i < pgAvDeviceInfo->Adi_max_channel_number(); i++)
        {
            if (Ade_get_avdev_info(purpose, i, &pAvDecInfo) < 0)
            {
                continue;
            }
            
            if (pAvDecInfo->phy_chn == chn)
            {
                index = i;
                break;
            }
        }
        
        if (index < 0)
        {
            DEBUG_ERROR("purpose %d, chn %d get illegal\n", purpose, chn);
            return -1;
        }
    }
    else
    {
        if (Ade_get_avdev_info(purpose, chn, &pAvDecInfo) < 0)
        {
            DEBUG_ERROR("purpose %d, chn %d get avdev_info failed\n", purpose, chn);
            return -1;
        }
    }

    if (pAvDecInfo->VdecChn < 0)
    {
        DEBUG_ERROR("purpose %d, chn %d get VdecChn %d illegal\n", purpose, chn, pAvDecInfo->VdecChn);
        return -1;
    }

    Vdec_dev = 0;
    Vdec_chn = pAvDecInfo->VdecChn;

//    DEBUG_MESSAGE("purpose %d, chn %d, VdecChn %d, layout %d, phy_chn %d\n", purpose, chn, Vdec_chn, pAvDecInfo->LayoutChn, pAvDecInfo->phy_chn);
    return 0;
}

int CHiAvDec::Ade_set_vo_chn_framerate(int purpose, int chn)
{
    AVDecInfo_t *pAvDecInfo = NULL;
    int playfr = 30;
    int vochn = -1;
    
    if (VDP_PLAYBACK_VDEC != purpose)
    {
        int index = -1;
        for (int i = 0; i < pgAvDeviceInfo->Adi_max_channel_number(); i++)
        {
            if (Ade_get_avdev_info(purpose, i, &pAvDecInfo) < 0)
            {
                continue;
            }
            
            if (pAvDecInfo->phy_chn == chn)
            {
                index = i;
                break;
            }
        }
        
        if (index < 0)
        {
            DEBUG_ERROR("purpose %d, chn %d get illegal\n", purpose, chn);
            return -1;
        }
    }
    else
    {
        if (Ade_get_avdev_info(purpose, chn, &pAvDecInfo) < 0)
        {
            return -1;
        }
    }

    vochn = pAvDecInfo->LayoutChn;

    Ade_get_vdec_frame(purpose, chn, playfr);
    if( purpose == VDP_PREVIEW_VDEC )
    {
        int vo_dev = Ade_get_vo_dev(purpose);
        
        if (pAvDecInfo->LayoutChn < 0)
        {
            return -1;
        }
        
        //DEBUG_MESSAGE("set frame chn %d, vo_dev %d vo chn %d, fr %d\n", chn, vo_dev, pAvDecInfo->LayoutChn, framerate);
        m_pHi_Vo->HiVo_set_vo_framerate(vo_dev, vochn, playfr);
    }
    else
    {
        /*if (pAvDecInfo->PicWidth > 720)
        {
            Ade_set_play_framerate(purpose, chn, 32);
        }
        else
        {
            playfr *= pAvDecInfo->PlayFactor;
            Ade_set_play_framerate(purpose, chn, playfr);	
        }*/
        playfr *= pAvDecInfo->PlayFactor;
       // DEBUG_MESSAGE("set play frame rate =%d faactor=%d\n", playfr, pAvDecInfo->PlayFactor);
        Ade_set_play_framerate(purpose, chn, playfr);
      //  Ade_set_play_framerate(purpose, chn, 30);
    }
    return 0;
}

int CHiAvDec::Ade_set_vo_chn_framerate_for_nvr_preview(int chn, int fr)
{
    int vo_dev = Ade_get_vo_dev(VDP_PREVIEW_VDEC);

    return m_pHi_Vo->HiVo_set_vo_framerate(vo_dev, chn, fr);
}

int CHiAvDec::Ade_get_vo_chn_framerate_for_nvr_preview(int chn, int& fr)
{
    int vo_dev = Ade_get_vo_dev(VDP_PREVIEW_VDEC);

    if( m_pHi_Vo->HiVo_get_vo_framerate(vo_dev, chn, fr) != 0 )
    {
        return -1;
    }

    return 0;
}

PlayState_t CHiAvDec::Ade_get_cur_playstate(int purpose)
{
    if (VDP_PLAYBACK_VDEC == purpose)
    {
        return m_PlayState;
    }
    
    return PS_INVALID;
}

int CHiAvDec::Ade_set_play_framerate(int purpose, int chn, int framerate)
{
    AVDecInfo_t *pAvDecInfo = NULL;

    if (Ade_get_avdev_info(purpose, chn, &pAvDecInfo) < 0)
    {
        return -1;
    }
    if (m_pHi_Vo)
    {
        int vo_dev = Ade_get_vo_dev(purpose);
        if (pAvDecInfo->LayoutChn < 0)
        {
            return -1;
        }
        DEBUG_MESSAGE("set frame chn %d, vo_dev %d vo chn %d, fr %d\n", chn, vo_dev, pAvDecInfo->LayoutChn, framerate);
        m_pHi_Vo->HiVo_set_vo_framerate(vo_dev, pAvDecInfo->LayoutChn, framerate);
        return vo_dev;
    }
    return -1;
}

int CHiAvDec::Ade_get_vo_dev(int purpose)
{
    av_output_type_t output = pgAvDeviceInfo->Adi_get_main_out_mode();

    if (VDP_SPOT_VDEC == purpose)
    {
        output = _AOT_SPOT_;
    }
    if (m_pHi_Vo)
    {
        VO_DEV vo_dev = m_pHi_Vo->HiVo_get_vo_dev(output, 0, 0);
        return vo_dev;
    }
    return -1;
}

int CHiAvDec::Ade_replay_vdec_chn(int purpose, int chn)
{
    int ret = 0;

    AVDecInfo_t *pAvDecInfo = NULL;
    int VdecChn = -1;

    if (Ade_get_avdev_info(purpose, chn, &pAvDecInfo) < 0)
    {
        return -1;
    }
    VdecChn = pAvDecInfo->VdecChn;

    if (m_pHi_Vo)
    {
        VO_DEV vo_dev = m_pHi_Vo->HiVo_get_vo_dev(pgAvDeviceInfo->Adi_get_main_out_mode(), 0, 0);
        m_pHi_Vo->HiVo_resume_vo(vo_dev, pAvDecInfo->LayoutChn);
    }

    ret = HI_MPI_VDEC_StartRecvStream(VdecChn);
    if(ret != 0)
    {
        DEBUG_ERROR("HI_MPI_VDEC_StartRecvStream err:0x%08x\n", ret);
    }

    return 0;
}

int CHiAvDec::Ade_pause_vdec_chn(int purpose, int chn)
{
    int ret = 0;
    AVDecInfo_t *pAvDecInfo = NULL;

    if (Ade_get_avdev_info(purpose, chn, &pAvDecInfo) < 0)
    {
        return -1;
    }
    
    if (m_pHi_Vo)
    {
        VO_DEV vo_dev = m_pHi_Vo->HiVo_get_vo_dev(pgAvDeviceInfo->Adi_get_main_out_mode(), 0, 0);
        m_pHi_Vo->HiVo_pause_vo(vo_dev, pAvDecInfo->LayoutChn);
    }

    return ret;
}

int CHiAvDec::Ade_clear_vdec_chn(int purpose, int chn)
{
    int ret = 0;
    AVDecInfo_t *pAvDecInfo = NULL;
    if (Ade_get_avdev_info(purpose, chn, &pAvDecInfo) < 0)
    {
        return -1;
    }
    if (m_pHi_Vo)
    {

        VO_DEV vo_dev = m_pHi_Vo->HiVo_get_vo_dev(pgAvDeviceInfo->Adi_get_main_out_mode(), 0, 0);
        m_pHi_Vo->HiVo_clear_vo(vo_dev, pAvDecInfo->LayoutChn);
    }

    return ret;
}

int CHiAvDec::Ade_clear_destroy_vdec(int purpose, int chn)
{
    int ret = 0;
    AVDecInfo_t *pAvDecInfo = NULL;
    if (Ade_get_avdev_info(purpose, chn, &pAvDecInfo) < 0)
    {
        DEBUG_MESSAGE( "Ade_get_avdev_info(purpose, chn, &pAvDecInfo) < 0 \n");
        return -1;
    }

    if( pAvDecInfo->bCreateDecChl )
    {
        ret = this->Ade_destory_vdec(pAvDecInfo->VdecChn);
        if(ret != 0)
        {
            DEBUG_ERROR("Ade_destory_vdec failed VdecChn =%d\n", pAvDecInfo->VdecChn);
        }
        pAvDecInfo->bCreateDecChl = false;
    }
#if !defined(_AV_PLATFORM_HI3515_)    
    if( m_pHi_vpss )
    {
        DEBUG_MESSAGE("HiVpss_ResetVpss chn %d pAvDecInfo->LayoutChn %d, phy_chn %d\n", chn, pAvDecInfo->LayoutChn, pAvDecInfo->phy_chn);
        m_pHi_vpss->HiVpss_ResetVpss( _VP_PREVIEW_VDEC_, pAvDecInfo->phy_chn);
    }
#endif

    if( m_pHi_Vo )
    {
        VO_DEV vo_dev = m_pHi_Vo->HiVo_get_vo_dev(pgAvDeviceInfo->Adi_get_main_out_mode(), 0, 0);
        m_pHi_Vo->HiVo_clear_vo(vo_dev, pAvDecInfo->LayoutChn);
    }

    return ret;
}

int CHiAvDec::Ade_reset_vdec_chn(int purpose, int chn)
{
    AVDecInfo_t *pAvDecInfo = NULL;
    
    if (Ade_get_avdev_info(purpose, chn, &pAvDecInfo) < 0)
    {
        DEBUG_ERROR("chn %d reset get avdev failed\n", chn);
        return -1;
    }

    if( !pAvDecInfo->bCreateDecChl )
    {
        return 0;
    }
    //DEBUG_MESSAGE("reset chn %d realchn %d\n", chn, pAvDecInfo->phy_chn);

    return Ade_reset_vdec(pAvDecInfo->VdecChn);
}

int CHiAvDec::Ade_get_dec_wait_frcnt(int purpose, int chn, int *pFrCnt, int *pDataLen, int Type)
{
    int ret = 0;
    int VdecChn = -1;
    AVDecInfo_t *pAvDecInfo = NULL;
    
    *pFrCnt = 0;
    if (Ade_get_avdev_info(purpose, chn, &pAvDecInfo) < 0)
    {
        DEBUG_ERROR("Ade_get_dec_wait_frcnt dec ch %d not exist\n", chn);
        return -1;
    }
    
    VdecChn = pAvDecInfo->VdecChn;

    pDataLen = pDataLen;

    if (DECDATA_AFRAME == Type)
    {
        *pFrCnt = 0;
    }
    else
    {
        if( !pAvDecInfo->bCreateDecChl )
        {
            //DEBUG_ERROR("Ade_get_dec_wait_frcnt dec ch%d not exist\n", VdecChn);
            return -1;
        }
        VDEC_CHN_STAT_S stat;

        ret = HI_MPI_VDEC_Query(VdecChn, &stat);
        if(ret != 0)
        {
            DEBUG_ERROR("HI_MPI_VDEC_Query err:0x%08x ch=%d\n", ret, VdecChn);
            return -1;
        }

        *pFrCnt = stat.u32LeftStreamFrames;
    }
    
    return ret;
}

int CHiAvDec::Ade_send_data_to_decoder(int purpose, int chn, unsigned char * pBuffer, int DataLen, int Type)
{
    int retval = -1;

    if ((purpose < 0) || (purpose >= _VDEC_MAX_VDP_NUM_)
        || (chn < 0) || (chn >= _AV_VDEC_MAX_CHN_))
    {
        return 0;
    }

    if (false == Ade_check_dec_send_data(purpose, chn))
    {
        return 0;
    }
     
    if (DECDATA_AFRAME == Type)
    {      
        return Ade_send_data_to_adec(_AT_ADEC_NORMAL_, pBuffer, DataLen);
    }
    else
    {
        return Ade_send_data_to_vdec(purpose, chn, pBuffer, DataLen);
    }

    return retval;
}

int CHiAvDec::Ade_send_data_to_decoderEx(int purpose, int chn, unsigned char * pBuffer, int DataLen, const PlaybackFrameinfo_t* pstuFrameInfo )
{
    if( (purpose < 0) || (purpose >= _VDEC_MAX_VDP_NUM_)
        || (chn < 0) || (chn >= _AV_VDEC_MAX_CHN_) )
    {
        return 0;
    }

    if( false == Ade_check_dec_send_data(purpose, chn) )
    {
        return 0;
    }
     
    if( DECDATA_AFRAME == pstuFrameInfo->u8FrameType )
    {      
        return Ade_send_data_to_adec(_AT_ADEC_NORMAL_, pBuffer, DataLen);
    }

    return Ade_send_data_to_vdecEx(purpose, chn, pBuffer, DataLen, pstuFrameInfo);
}

int CHiAvDec::Ade_operate_decoder(int purpose, int opcode, bool operate)
{
    PlayOperate_t ope = PO_INVALID;
    
    Ade_get_optype_by_opcode(opcode, ope);
    
    if (PO_INVALID == ope)
    {
        return -1;
    }

    if (VDP_PLAYBACK_VDEC == purpose)
    {
        return Ade_operate_playback(purpose, ope, operate);
    }

    return 0;
}

int CHiAvDec::Ade_operate_decoder_for_switch_channel(int purpose)
{
    int chnnum = m_CurVdecChNum[purpose];
    
    if (VDP_PLAYBACK_VDEC == purpose)
    {
        m_pDdsp->WaitToSepcialPlay(m_VdecChmap[purpose], true);
        
        datetime_t playtime;
        playtime.year = 0xff;
        playtime.month = 0xff;
        playtime.day = 0xff;
        playtime.hour = 0xff;
        playtime.minute = 0xff;
        playtime.second = 0xff;
        for(int i = 0; i < chnnum; i++)
        {
            Ade_reset_vdec_chn(purpose, i);
#if !defined(_AV_PLATFORM_HI3515_)
	     if(m_pHi_vpss)
	     {
	     		VPSS_GRP pVpss_grp;
			VPSS_CHN pVpss_chn;
	     		m_pHi_vpss->HiVpss_get_vpss(_VP_PLAYBACK_VDEC_,i,&pVpss_grp,&pVpss_chn,NULL);
	     		HI_MPI_VPSS_ResetGrp(pVpss_grp);
	     }
#endif		 
	     Ade_clear_vdec_chn(purpose, i);
            Ade_reset_mmsp_frame_info(i);
        }
        Ade_set_dec_playtime(purpose, playtime, true);
	
        //m_pDdsp->WaitToSepcialPlay(m_VdecChmap[purpose], false); 
    }
    
    return 0;
}


int CHiAvDec::Ade_operate_playback(int purpose, PlayOperate_t ope, bool operate)
{
    int factor = 1;
    int dirvalue = 1;
    bool setframe = false;
    int chnnum = m_CurVdecChNum[purpose];

    if (NULL == m_pDdsp)
    {
        return -1;
    }
    
    if (m_PrePlayState != m_CurPlayState)
    {
        m_PrePlayState = m_CurPlayState;
    }
   
    m_CurPlayState = ope;
    
    DEBUG_MESSAGE("m_PrePlayState %d, m_CurPlayState %d, m_VdecChnBmp %x\n", m_PrePlayState, m_CurPlayState, m_VdecChmap[purpose]);

    if (false == operate)
    {    
        if (ope == PO_SEEK)
        {
            DEBUG_MESSAGE("Get Start Seek Cmd, Idle....%x\n", m_VdecChmap[purpose]);
        }

        m_pDdsp->WaitToSepcialPlay(m_VdecChmap[purpose], true);

        if (ope == PO_SEEK)
        {           
            datetime_t playtime;
            playtime.year = 0xff;
            playtime.month = 0xff;
            playtime.day = 0xff;
            playtime.hour = 0xff;
            playtime.minute = 0xff;
            playtime.second = 0xff;

            for(int i=0; i<chnnum; i++)
            {
                Ade_reset_vdec_chn(purpose, i);
                Ade_clear_vdec_chn(purpose, i);
                this->Ade_reset_mmsp_frame_info(i);
            }
            Ade_set_dec_playtime(purpose, playtime, true);
        }

        if (m_PlayState >= PS_FASTFORWARD2X && m_PlayState <= PS_FASTBACKWARD16X)
        {
            if (ope != PO_FASTFORWARD && ope != PO_FASTBACKWARD)
            {
                for(int i = 0; i < chnnum; i++)
                {
                    Ade_reset_vdec_chn(purpose, i);
                }
            }
        }
        
        m_pDdsp->DecSepcialPlayOperate(m_VdecChmap[purpose], ope);
        
        return 0;
    }
    
    switch(ope)
    {
        case PO_SEEK:
        case PO_SLOWPLAY:
        case PO_PLAY:
        case PO_STEP:
        {   
            setframe = true;
            m_PlayState = PS_NORMAL;
            break;
        }
        case PO_PAUSE:
            m_PlayState = PS_PAUSE;
            break;
        case PO_FASTFORWARD:
        {
            setframe = true;
            switch(m_PlayState)
            {
                case PS_FASTFORWARD2X:
                    m_PlayState = PS_FASTFORWARD4X;
                    factor = 4;
                    break;
                case PS_FASTFORWARD4X:
                    m_PlayState = PS_FASTFORWARD8X;
                    factor = 8;
                    break;
                case PS_FASTFORWARD8X:
                    m_PlayState = PS_FASTFORWARD16X;
                    factor = 16;
                    break;
                case PS_FASTFORWARD16X:
                default:
                    m_PlayState = PS_FASTFORWARD2X;
                    factor = 2;
                    break;
            }
            break;
        }
        case PO_FASTBACKWARD:
        {
            setframe = true;
            dirvalue = -1;
            switch(m_PlayState)
            {
                case PS_FASTBACKWARD2X:
                    m_PlayState = PS_FASTBACKWARD4X;
                    factor = 4;
                    break;
                case PS_FASTBACKWARD4X:
                    m_PlayState = PS_FASTBACKWARD8X;
                    factor = 8;
                    break;
                case PS_FASTBACKWARD8X:
                    m_PlayState = PS_FASTBACKWARD16X;
                    factor = 16;
                    break;
                case PS_FASTBACKWARD16X:
                default:
                    m_PlayState = PS_FASTBACKWARD2X;
                    factor = 2;
                    break;
            }
            break;
        }
        default:
            break;
    }
    
    if ((ope == PO_STEP) && (m_CurPlayState != ope))
    {
        Ade_reset_mssp_frame_jump_level(true);
    }
    if ((m_CurPlayState == PO_STEP && ope != PO_STEP) || ope == PO_SEEK)
    {
        Ade_reset_mssp_frame_jump_level(false);
    }
    
    /*设置帧率*/
    if (true == setframe)
    {
        int play_factor = factor*dirvalue;
        DEBUG_MESSAGE("play_factor %d, ope %d, factor %d, dirvalue %d\n", play_factor, ope, factor, dirvalue);
        for(int i = 0; i < chnnum; i++)
        {
            Ade_set_play_framerate_factor(purpose, i, play_factor);
            Ade_set_vo_chn_framerate(purpose, i);
        }
    }
    //
    m_pDdsp->WaitToSepcialPlay(m_VdecChmap[purpose], false);

    return 0;
}

int CHiAvDec::Ade_clear_ao_chn(int AdecChn)
{
    int ret_val = -1;
    if (m_pHi_Ao)
    {
        DEBUG_MESSAGE("CHiAvDec::Ade_clear_ao_chn clear adecchn %d!\n", AdecChn);
        // m_pHi_Ao->HiAo_clear_ao_chnbuf(_AO_HDMI_);
        m_pHi_Ao->HiAo_clear_ao_chnbuf(_AO_PLAYBACK_CVBS_);
    }
    if(AdecChn < 0)
    {
        DEBUG_ERROR("CHiAo::HiAo_clear_ao_chnbuf chn is invalid(%d)!\n", AdecChn);
        return -1;
    }
    if ((ret_val = HI_MPI_ADEC_ClearChnBuf(AdecChn)) != 0)
    {
        DEBUG_ERROR( "CHiAo::HiAo_clear_ao_chnbuf HI_MPI_ADEC_ClearChnBuf FAILED(AdecChn:%d)(0x%08lx)\n", AdecChn, (unsigned long)ret_val);
        return -1;
    }

    return 0;
}


int CHiAvDec::Ade_create_adec_handle(adec_type_e adec_type, eAudioPayloadType audiotype)
{
    int adec_chn = 0;

    if ((adec_chn = Ade_get_adec_chn(adec_type)) == -1)
    {
        DEBUG_ERROR("CHiAvDec::Ade_create_adec_handle Ade_get_adec_chn failed! (adec_type:%d)\n", adec_type);
        return -1;
    }

    if (m_eAudioPlayLoad[adec_chn] != APT_INVALID)
    {       
        DEBUG_ERROR("CHiAvDec::Ade_create_adec_handle m_eAudioPlayLoad[%d]= %d error!\n", adec_chn, m_eAudioPlayLoad[adec_chn]);
        return -1;
    }
        
    if (Ade_create_adec(adec_chn, audiotype) != 0)
    {
        DEBUG_ERROR("Ade_create_adec failed! (adec_chn:%d) (audiotype:%d)\n", adec_chn, audiotype);
        return -1;
    }
#if 1//!defined(_AV_PLATFORM_HI3515_)    
    if (m_pHi_Ao)
    {
        if (adec_type == _AT_ADEC_NORMAL_)
        {        
            // m_pHi_Ao->HiAo_adec_bind_ao(_AO_HDMI_, adec_chn);
            m_pHi_Ao->HiAo_adec_bind_ao(_AO_PLAYBACK_CVBS_, adec_chn);
        }
        else if (adec_type == _AT_ADEC_TALKBACK)
        {
            m_pHi_Ao->HiAo_adec_bind_ao(_AO_TALKBACK_, adec_chn);
            m_eAudioPlayLoad[adec_chn] = audiotype;
        }
    }
#endif


    return 0;
}

int CHiAvDec::Ade_destory_adec_handle(adec_type_e adec_type)
{
    int adec_chn = 0;
    
    if ((adec_chn = Ade_get_adec_chn(adec_type)) == -1)
    {
        DEBUG_ERROR("CHiAvDec::Ade_destory_adec_handle Ade_get_adec_chn failed! (adec_type:%d)\n", adec_type);
        return -1;
    }
    
    if (m_eAudioPlayLoad[adec_chn] == APT_INVALID)
    {
        //DEBUG_ERROR("CHiAvDec::Ade_destory_adec_handle m_eAudioPlayLoad[%d]= %d error!\n", adec_chn, m_eAudioPlayLoad[adec_chn]);
        return -1;
    }
#if 1//!defined(_AV_PLATFORM_HI3515_)    
    if (m_pHi_Ao)
    {
        if (adec_type == _AT_ADEC_NORMAL_)
        {
            // m_pHi_Ao->HiAo_adec_unbind_ao(_AO_HDMI_, adec_chn);
            m_pHi_Ao->HiAo_adec_unbind_ao(_AO_PLAYBACK_CVBS_, adec_chn);
        }
        else if (adec_type == _AT_ADEC_TALKBACK)
        {
            m_pHi_Ao->HiAo_adec_unbind_ao(_AO_TALKBACK_, adec_chn);
        }
    }
#endif
    if (Ade_destroy_adec(adec_chn) != 0)
    {
        DEBUG_ERROR("Ade_destroy_adec FAILED! (adec_chn:%d) \n", adec_chn);
        return -1;
    }
    m_eAudioPlayLoad[adec_chn] = APT_INVALID;
    
    return 0;
}

int CHiAvDec::Ade_create_vdec_handle(int purpose, int chn, RMFI2_VIDEOINFO *pVideoInfo)
{
    AVDecInfo_t *pAvDecInfo = NULL; 
    int w = pVideoInfo->lWidth;
    int h = pVideoInfo->lHeight;
        
    if (Ade_get_avdev_info(purpose, chn, &pAvDecInfo) < 0)
    {
        return -1;
    }

    if ( !pAvDecInfo->bCreateDecChl || ((pAvDecInfo->PicWidth != w) || (pAvDecInfo->PicHeight != h)))
    {
        if( VDP_PREVIEW_VDEC == purpose)
        {
            pgAvDeviceInfo->Adi_set_cur_chn_res(pAvDecInfo->phy_chn, w, h);
            pgAvDeviceInfo->Adi_set_created_preview_decoder(pAvDecInfo->phy_chn, true);
        }
        else if (VDP_PLAYBACK_VDEC == purpose)
        {
             pgAvDeviceInfo->Adi_set_cur_chn_res(pAvDecInfo->LayoutChn, w, h);
        }
         
        m_pMuiltThreadDecLock->lock();
        
        /*DEBUG_MESSAGE("index=%d ch=%d layout=%d Decreated=%d w(%d->%d) h(%d->%d), AvRefType %d\n", 
            chn, pAvDecInfo->VdecChn, pAvDecInfo->LayoutChn, 
            pAvDecInfo->bCreateDecChl, pAvDecInfo->PicWidth, w, pAvDecInfo->PicHeight, h, pAvDecInfo->AvRefType);
        */
        if (pAvDecInfo->LayoutChn >= 0)
        {
            DEBUG_MESSAGE("disconnect vo index=%d ch=%d layout=%d Decreated=%d w(%d=%d) h(%d=%d)\n", 
                chn, pAvDecInfo->VdecChn, pAvDecInfo->LayoutChn,
                pAvDecInfo->bCreateDecChl, pAvDecInfo->PicWidth, w, pAvDecInfo->PicHeight, h );
            if (VDP_PLAYBACK_VDEC == purpose)
            {
                m_AvDecOpe.DisconnectToVo(purpose, pAvDecInfo->LayoutChn);
            }
            else
            {
                m_AvDecOpe.DisconnectToVo(purpose, pAvDecInfo->phy_chn);
            }
            /*DEBUG_MESSAGE("bb disconnect vo index=%d ch=%d layout=%d Decreated=%d w(%d=%d) h(%d=%d)\n", 
                chn, pAvDecInfo->VdecChn, pAvDecInfo->LayoutChn,
                pAvDecInfo->bCreateDecChl, pAvDecInfo->PicWidth, w, pAvDecInfo->PicHeight, h );*/
        }
            
        if( pAvDecInfo->bCreateDecChl)
        {/*release vdec*/
            Ade_destory_vdec(pAvDecInfo->VdecChn);
            pAvDecInfo->VdecChn = -1;
            pAvDecInfo->bCreateDecChl = false;
        }

        if (Ade_create_vdec(chn, w, h) < 0)
        {
            DEBUG_ERROR("create vdecchn %d, w %d, h %d failed\n", chn, w, h);
            
             m_pMuiltThreadDecLock->unlock();
             
            return -1;
        }
        else
        {
            Ade_set_dec_display_mode(chn, purpose);
            pAvDecInfo->bCreateDecChl = true;
            pAvDecInfo->VdecChn = chn;
            pAvDecInfo->PicWidth = w;
            pAvDecInfo->PicHeight = h;
#ifndef _AVDEC_ADJUST_JUMP_
            pAvDecInfo->RealPlayFrameRate = pVideoInfo->lFPS;
#endif
            //DEBUG_MESSAGE("create purpose %d ch %d vdecchn %d, phychn %d, w %d , h %d , fr %ld success\n", purpose, chn, chn, pAvDecInfo->phy_chn, w, h, pVideoInfo->lFPS);
            if (VDP_PLAYBACK_VDEC == purpose)
            {
                m_AvDecOpe.ConnectToVo(purpose, pAvDecInfo->LayoutChn);  
            }
            else
            {
                m_AvDecOpe.ConnectToVo(purpose, pAvDecInfo->phy_chn);
            }
            Ade_clear_vdec_chn(purpose, chn);
            DEBUG_MESSAGE("bb create purpose %d ch %d vdecchn %d, phychn %d, w %d , h %d , fr %ld success\n", purpose, chn, chn, pAvDecInfo->phy_chn, w, h, pVideoInfo->lFPS);       
#ifdef _AVDEC_ADJUST_JUMP_
            if (VDP_PLAYBACK_VDEC == purpose)
            {
                pAvDecInfo->RealPlayFrameRate = pVideoInfo->lFPS;
                m_ResetFrameRate[purpose] = true;        
            }
#endif        
        }
        m_pMuiltThreadDecLock->unlock();
    }
    
#ifdef _AVDEC_ADJUST_JUMP_
    if (((unsigned int)pAvDecInfo->RealPlayFrameRate != pVideoInfo->lFPS) && (VDP_PLAYBACK_VDEC == purpose))
    {
        m_ResetFrameRate[purpose] = true;
        pAvDecInfo->RealPlayFrameRate = pVideoInfo->lFPS;
    }
#endif

#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
    if( pAvDecInfo->u8RealFrameRate != pVideoInfo->lFPS )
    {
        if( ( purpose != VDP_PLAYBACK_VDEC ) && (m_pAvPreviewDecoder[purpose] != NULL))
        {
            m_pAvPreviewDecoder[purpose]->SetFrameRate( chn, pVideoInfo->lFPS );
        }  
        pAvDecInfo->u8RealFrameRate = pVideoInfo->lFPS;
    }
#endif     
    return 0;
}
int CHiAvDec::Ade_set_start_send_data_flag(bool isStartFlag)
{
    m_bStartFlag = isStartFlag;
    return 0;
}

int CHiAvDec::Ade_release_vdec(int purpose, int chn)
{
    int AvChn = chn;
    AVDecInfo_t *pAvDecInfo = NULL;
    if ((AvChn >= 0) && (AvChn < _AV_VDEC_MAX_CHN_))
    {
        if (Ade_get_avdev_info(purpose, AvChn, &pAvDecInfo) < 0)
        {
            DEBUG_ERROR("chn %d Ade_get_avdev_info failed\n", chn);
            return -1;
        }
        
        int VdecChn = pAvDecInfo->VdecChn;
        // nvr 中，有IPC异常导致频繁上下线，出现多线程启停异常，顾先做此修改 2013-7-20
        if( pAvDecInfo->bCreateDecChl )
        {   
            this->Ade_destory_vdec(VdecChn);
            pAvDecInfo->bCreateDecChl = false;
            pAvDecInfo->VdecChn = -1;
            pAvDecInfo->DataBufLen = 0;
            pAvDecInfo->bReleaseBuf = false;
            pAvDecInfo->phy_chn = -1;
        }
    }
    return 0;
}

int CHiAvDec::Ade_create_vdec(int VdecChn, int w, int h)
{
    VDEC_CHN_ATTR_S VdecChnAttr;
    int ret;
    
#if defined(_AV_PLATFORM_HI3531_) || defined(_AV_PLATFORM_HI3532_)
    MPP_CHN_S mpp_chn;
    const char *pDdrName = NULL;

    if(((VdecChn + 1) % 3) != 0)
    {
        pDdrName = "ddr1";
    }
    mpp_chn.enModId = HI_ID_VDEC;
    mpp_chn.s32DevId = 0;
    mpp_chn.s32ChnId = VdecChn;
    if ((ret = HI_MPI_SYS_SetMemConf(&mpp_chn, pDdrName)) != 0)
    {
        DEBUG_ERROR( "CHiAvDec::Ade_create_vdec HI_MPI_SYS_SetMemConf (vdecChn:%d)(0x%08lx)\n", VdecChn, (unsigned long)ret);
        return -1;
    }
#endif
#if !defined(_AV_PLATFORM_HI3515_)
    VdecChnAttr.enType = PT_H264;
    VdecChnAttr.u32Priority = 1;
    VdecChnAttr.u32PicWidth  = w;
    VdecChnAttr.u32PicHeight = h;
    VdecChnAttr.u32BufSize = VdecChnAttr.u32PicWidth * VdecChnAttr.u32PicHeight * 1.5;
    VdecChnAttr.stVdecVideoAttr.u32RefFrameNum = 2;
    VdecChnAttr.stVdecVideoAttr.enMode = VIDEO_MODE_FRAME;
#ifdef _AV_PLATFORM_HI3520D_V300_
    VdecChnAttr.stVdecVideoAttr.bTemporalMvpEnable= HI_FALSE;
    VdecChnAttr.stVdecVideoAttr.u32RefFrameNum = 1;
    VdecChnAttr.u32Priority = 5;
    VdecChnAttr.u32BufSize = VdecChnAttr.u32PicWidth * VdecChnAttr.u32PicHeight * 3;

#else
    VdecChnAttr.stVdecVideoAttr.s32SupportBFrame = 0;
#endif
    //DEBUG_MESSAGE("start create decorder channel %d\n", VdecChn);
    ret = HI_MPI_VDEC_CreateChn(VdecChn, &VdecChnAttr);
    if(ret != 0)
    {
        DEBUG_ERROR( "HI_MPI_VDEC_CreateChn err:0x%08x, chn %d, w %d, h %d\n", ret, VdecChn, w, h);
        return -1;
    }
#else
	VDEC_ATTR_H264_S stH264Attr;
    VdecChnAttr.enType = PT_H264;
	VdecChnAttr.u32BufSize = (w * h * 3/2);
	stH264Attr.u32PicWidth = w;
	stH264Attr.u32PicHeight = h;
	stH264Attr.u32Priority = 0;
	stH264Attr.u32RefFrameNum = 2;
	stH264Attr.enMode = H264D_MODE_FRAME;
	VdecChnAttr.pValue = (void *)&stH264Attr;
	
    ret = HI_MPI_VDEC_CreateChn(VdecChn, &VdecChnAttr, NULL);
    if(ret != 0)
    {
        DEBUG_ERROR( "HI_MPI_VDEC_CreateChn err:0x%08x, chn %d, w %d, h %d\n", ret, VdecChn, w, h);
        return -1;
    }
#endif
    ret = HI_MPI_VDEC_StartRecvStream(VdecChn);
    if(ret != 0)
    {
        DEBUG_ERROR("HI_MPI_VDEC_StartRecvStream err:0x%08x\n", ret);
        return -1;
    }
    return 0;
}

int CHiAvDec::Ade_destory_vdec(int VdecChn)
{
    int ret = 0;

    ret = HI_MPI_VDEC_StopRecvStream(VdecChn);
    if(ret != 0)
    {
        DEBUG_ERROR("CHiAvDec::Ade_destory_vdec HI_MPI_VDEC_StopRecvStream err:0x%08x ch=%d\n", ret, VdecChn);
     //   return -1;
    }

    ret = HI_MPI_VDEC_DestroyChn(VdecChn);
    if(ret != 0)
    {
        DEBUG_ERROR("CHiAvDec::Ade_destory_vdec HI_MPI_VDEC_DestroyChn err:0x%08x ch=%d\n", ret, VdecChn);
        return -1;
    }

    return 0;
}

int CHiAvDec::Ade_reset_vdec(int chn)
{
    int ret = 0;

    if (chn < 0)
    {
        DEBUG_ERROR("ch %d is illegal\n", chn);
        return -1;
    }

    ret = HI_MPI_VDEC_StopRecvStream(chn);
    if(ret != 0)
    {
        DEBUG_ERROR("CHiAvDec::Ade_reset_vdec HI_MPI_VDEC_StopRecvStream err:0x%08x chn:%d\n", ret, chn);
    }

    ret = HI_MPI_VDEC_ResetChn(chn);
    if(ret != 0)
    {
        DEBUG_ERROR("CHiAvDec::Ade_reset_vdec HI_MPI_VDEC_ResetChn err:0x%08x chn:%d\n", ret, chn);
    }

    ret = HI_MPI_VDEC_StartRecvStream(chn);
    if(ret != 0)
    {
        DEBUG_ERROR("HI_MPI_VDEC_StartRecvStream err:0x%08x chn:%d\n", ret, chn);
    }

    return 0;
}

int CHiAvDec::Ade_send_data_to_vdec(int purpose, int chn, unsigned char *pBuffer, int DataLen)
{
    int retval = -1;
    VDEC_STREAM_S VideoStream;
    int VdecChn = -1;
    AVDecInfo_t *pAvDecInfo = NULL;
    Stm_ExternFrameInfo_t stuExternInfo;

    if (Ade_get_avdev_info(purpose, chn, &pAvDecInfo) < 0)
    {
        return -1;
    }
    memset(&stuExternInfo, 0, sizeof(Stm_ExternFrameInfo_t));    
    if (Get_extern_frameinfo(pBuffer, DataLen, stuExternInfo) < 0)
    {
        DEBUG_ERROR("Get_extern_frameinfo failed\n");
        return -1;
    }
    if(Ade_get_frame_type(pBuffer) == DPT_IFRAME)
    {
        datetime_t playtime;
        //DEBUG_MESSAGE("w=%ld h=%ld pts=%lld len=%ld\n", pVideoInfo->lWidth, pVideoInfo->lHeight
        //    , pVirtualTime->llVirtualTime, pStreamHeader->lExtendLen);
        if (stuExternInfo.Mask & EXTERN_FRAMEINFO_VIDEO_VIDEOINFO)
        {
            Ade_create_vdec_handle(purpose, chn, stuExternInfo.pVideoInfo);
        }
        
        if (stuExternInfo.Mask & EXTERN_FRAMEINFO_VIDEO_RTCTIME)
        {
            playtime.year   = stuExternInfo.pRtcTime->stuRtcTime.cYear;
            playtime.month  = stuExternInfo.pRtcTime->stuRtcTime.cMonth;
            playtime.day    = stuExternInfo.pRtcTime->stuRtcTime.cDay;
            playtime.hour   = stuExternInfo.pRtcTime->stuRtcTime.cHour;
            playtime.minute = stuExternInfo.pRtcTime->stuRtcTime.cMinute;
            playtime.second = stuExternInfo.pRtcTime->stuRtcTime.cSecond;
            // DEBUG_MESSAGE("Sendo data to bdecoder, time=%d:%d:%d\n", playtime.hour, playtime.minute, playtime.second);
            Ade_set_dec_playtime(purpose, playtime, false);
        }
    }
    VdecChn = pAvDecInfo->VdecChn;
    if (VdecChn < 0)
    {
        DEBUG_ERROR("#######################error ch[%d]=%d\n", chn, VdecChn);
        return 0;
    }
    VideoStream.pu8Addr = pBuffer + sizeof(RMSTREAM_HEADER) + stuExternInfo.pHeader->lExtendLen;
    VideoStream.u32Len = stuExternInfo.pHeader->lFrameLen;
    if (stuExternInfo.Mask & EXTERN_FRAMEINFO_VIDEO_VIRTUALTIME)
    {       
        VideoStream.u64PTS = stuExternInfo.pVTime->llVirtualTime*1000;
    }
#ifdef _AV_PLATFORM_HI3535_
    VideoStream.bEndOfFrame = HI_TRUE;
    VideoStream.bEndOfStream = HI_FALSE;
#endif
    retval = HI_MPI_VDEC_SendStream(VdecChn, &VideoStream, HI_IO_NOBLOCK);
    //DEBUG_MESSAGE("send data retval=%d\n", retval);
    if(retval != 0)
    {
        DEBUG_ERROR("HI_MPI_VDEC_SendStream chn %d err:0x%08x\n", chn, retval);
#ifdef _AV_PLATFORM_HI3520D_
        if ((HI_U32)retval == 0xa005800f)
#else
        if ((HI_U32)retval == 0xa004800f)
#endif
        {
            Ade_reset_vdec(VdecChn);
            return -2;
        }
        return -1;
    }
    return retval;
}

int CHiAvDec::Ade_send_data_to_vdecEx(int purpose, int chn, unsigned char *pBuffer, int DataLen, const PlaybackFrameinfo_t* pstuFrameInfo)
{
    int retval = -1;

    VDEC_STREAM_S VideoStream;
    int VdecChn = -1;
    AVDecInfo_t *pAvDecInfo = NULL;
    Stm_ExternFrameInfo_t stuExternInfo;

    if (Ade_get_avdev_info(purpose, chn, &pAvDecInfo) < 0)
    {

        return -1;
    }
    memset(&stuExternInfo, 0, sizeof(Stm_ExternFrameInfo_t));    
    if (Get_extern_frameinfo(pBuffer, DataLen, stuExternInfo) < 0)
    {
        DEBUG_ERROR("Get_extern_frameinfo failed\n");
        return 0;
    }
    if( pstuFrameInfo->u8FrameType == DPT_IFRAME)
    {
        datetime_t playtime;
        if (stuExternInfo.Mask & EXTERN_FRAMEINFO_VIDEO_VIDEOINFO)
        {
            Ade_create_vdec_handle(purpose, chn, stuExternInfo.pVideoInfo);
        }

        playtime.year   = pstuFrameInfo->year;;
        playtime.month  = pstuFrameInfo->month;
        playtime.day    = pstuFrameInfo->day;
        playtime.hour   = pstuFrameInfo->hour;
        playtime.minute = pstuFrameInfo->minute;
        playtime.second = pstuFrameInfo->second;
        // DEBUG_MESSAGE("Chn %d send framecnt %d, TotalFrameCnt %d, send mode %d\n", chn, pAvDecInfo->SendFrameCnt, pAvDecInfo->TotalFrameCnt, m_SendRefFrame[purpose]);
        // DEBUG_MESSAGE("Sendo data to bdecoder, time=%d:%d:%d\n", playtime.hour, playtime.minute, playtime.second);
        pAvDecInfo->SendFrameCnt = 0;
        pAvDecInfo->TotalFrameCnt = 0;
        Ade_set_dec_playtime(purpose, playtime, false);
#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
        pAvDecInfo->ullLastIFramePts = pstuFrameInfo->ullPts + 30;
        pAvDecInfo->u8LastYear = pstuFrameInfo->year;
        pAvDecInfo->u8LastMonth = pstuFrameInfo->month;
        pAvDecInfo->u8LastDay = pstuFrameInfo->day;
        pAvDecInfo->u8LastHour = pstuFrameInfo->hour;
        pAvDecInfo->u8LastMinute = pstuFrameInfo->minute;
        pAvDecInfo->u8LastSecond = pstuFrameInfo->second;

        pAvDecInfo->u8LastSendSec = pstuFrameInfo->second;
#endif
    }
#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
    else
    {
        int timeSec = pstuFrameInfo->ullPts - pAvDecInfo->ullLastIFramePts;
        if( timeSec > 900 && timeSec < 10000 ) // 1~10second
        {
            timeSec = timeSec/1000;

            if( pAvDecInfo->u8LastSendSec != pAvDecInfo->u8LastSecond + timeSec )
            {
                pAvDecInfo->u8LastSendSec = pAvDecInfo->u8LastSecond + timeSec;
                datetime_t playtime;
                playtime.year   = pAvDecInfo->u8LastYear;;
                playtime.month  = pAvDecInfo->u8LastMonth;
                playtime.day    = pAvDecInfo->u8LastDay;
                playtime.hour   = pAvDecInfo->u8LastHour;
                playtime.minute = pAvDecInfo->u8LastMinute;
                playtime.second = pAvDecInfo->u8LastSendSec;
                if( playtime.second > 59 )
                {
                    playtime.second = 0;
                    playtime.minute++;
                    if( playtime.minute > 59 )
                    {
                        playtime.hour++;
                        if( playtime.hour > 23 )
                        {
                            DEBUG_ERROR("$$$$$$$$$$$$$$$$$$$$$$$index=%d unexcept error(%d:%d:%d)\n", chn, playtime.hour
                                , playtime.minute, playtime.second);
                        }
                    }
                }
                // DEBUG_MESSAGE("index=%d time=%d:%d:%d\n", chn, playtime.hour, playtime.minute, playtime.second);
                Ade_set_dec_playtime(purpose, playtime, false);
            }
        }
    }
#endif

#ifdef _AVDEC_ADJUST_JUMP_
    if (stuExternInfo.Mask & EXTERN_FRAMEINFO_VIDEO_STATE)
    {/*video ref mode*/
        av_ref_type_t av_ref_type = _ART_REF_FOR_1X;
        if (stuExternInfo.pVideoState->lRefType == 1)
        {
            av_ref_type = _ART_REF_FOR_2X;
        }
        else if (stuExternInfo.pVideoState->lRefType == 2)
        {
            av_ref_type = _ART_REF_FOR_4X;
        }
        if (pAvDecInfo->AvRefType != av_ref_type)
        {
            pAvDecInfo->AvRefType = av_ref_type;
            if (VDP_PLAYBACK_VDEC == purpose)
            {
                m_ResetFrameRate[purpose] = true;
            }
        }
    }
#endif     

    pAvDecInfo->TotalFrameCnt++;
    VdecChn = pAvDecInfo->VdecChn;
    if (VdecChn < 0)
    {
        return 0;
    }
    VideoStream.pu8Addr = pBuffer + sizeof(RMSTREAM_HEADER) + stuExternInfo.pHeader->lExtendLen;
    VideoStream.u32Len = stuExternInfo.pHeader->lFrameLen;
    VideoStream.u64PTS = pstuFrameInfo->ullPts * 1000ULL;
#ifdef _AV_PLATFORM_HI3535_
    VideoStream.bEndOfFrame = HI_TRUE;
    VideoStream.bEndOfStream = HI_FALSE;
#endif
    //send stream data to video decoder
    retval = HI_MPI_VDEC_SendStream( VdecChn, &VideoStream, HI_IO_NOBLOCK );
    if(retval != 0)
    {
        DEBUG_ERROR("HI_MPI_VDEC_SendStream chn %d err:0x%08x\n", chn, retval);
        if ((HI_U32)retval == 0xa004800f)
        {
            Ade_reset_vdec(VdecChn);
            return -2;
        }
        return -1;
    }
    pAvDecInfo->SendFrameCnt++;

    return 0;
}

int CHiAvDec::Ade_create_adec(int achn, eAudioPayloadType type)
{
    ADEC_ATTR_ADPCM_S AdpcmAttr;
    ADEC_ATTR_G711_S G711Attr;
    ADEC_ATTR_G726_S G726Attr;
    ADEC_ATTR_LPCM_S Lpcm;
    ADEC_ATTR_AMR_S AmrAttr;
    ADEC_CHN_ATTR_S AdecAttr;
    int ret = 0;
#ifdef _AV_USE_AACENC_
    ADEC_ATTR_AAC_S AacAttr;
#endif    
#ifdef _AV_PLATFORM_HI3531_
    MPP_CHN_S mpp_chn;
    const char *pDdrName = NULL;
    
    pDdrName = "ddr1";
    mpp_chn.enModId = HI_ID_ADEC;
    mpp_chn.s32DevId = 0;
    mpp_chn.s32ChnId = achn;
    if ((ret = HI_MPI_SYS_SetMemConf(&mpp_chn, pDdrName)) != 0)
    {
        DEBUG_ERROR( "CHiAvDec::Ade_create_adec HI_MPI_SYS_SetMemConf (aChn:%d)(0x%08lx)\n", achn, (unsigned long)ret);
        return -1;
    }
#endif

    AdecAttr.u32BufSize = MAX_AUDIO_FRAME_NUM;
    AdecAttr.enMode = ADEC_MODE_PACK;
    switch(type)
    {
        case APT_ADPCM:
            AdecAttr.enType = PT_ADPCMA;
            AdpcmAttr.enADPCMType = ADPCM_TYPE_DVI4;
            AdecAttr.pValue = &AdpcmAttr;
            break;
            
        case APT_G726:
            AdecAttr.enType = PT_G726;
            G726Attr.enG726bps = MEDIA_G726_16K;//MEDIA_G726_40K ;
            AdecAttr.pValue = &G726Attr;
            break;
            
        case APT_G711A:
            AdecAttr.enType = PT_G711A;
            AdecAttr.pValue = &G711Attr;
            AdecAttr.u32BufSize = 10;
            break;
            
        case APT_G711U:
            AdecAttr.enType = PT_G711U;
            AdecAttr.pValue = &G711Attr;
            AdecAttr.u32BufSize = 10;
            break;
        case APT_LPCM:
            AdecAttr.enType = PT_LPCM;
            AdecAttr.pValue = &Lpcm;
            AdecAttr.enMode = ADEC_MODE_PACK;
            AdecAttr.u32BufSize = 10;
            break;          
        case APT_AMR:
            AdecAttr.enType = PT_AMR;
            AdecAttr.pValue = &AmrAttr;
            AmrAttr.enFormat = AMR_FORMAT_MMS;           
           // HI_MPI_ADEC_AmrInit();
            break;
#ifdef _AV_USE_AACENC_
		case APT_AAC:
			AdecAttr.enType = PT_AAC;
			AdecAttr.enMode = ADEC_MODE_STREAM;			
			AdecAttr.pValue = &AacAttr;
#ifndef _AV_PLATFORM_HI3515_
			HI_MPI_ADEC_AacInit();
#endif
			break;
#endif
        default:
            DEBUG_ERROR("invalid audio type:%d\n", type);
            break;
    }
    
    ret = HI_MPI_ADEC_CreateChn(achn, &AdecAttr);
    if(ret != 0)
    {
        DEBUG_ERROR("HI_MPI_ADEC_CreateChn err:0x%08x\n", ret);
        return -1;
    }
    return 0;
}

int CHiAvDec::Ade_destroy_adec(int achn)
{
    int ret = 0;

    ret = HI_MPI_ADEC_DestroyChn(achn);
    if(ret != 0)
    {
        DEBUG_ERROR("HI_MPI_ADEC_DestroyChn err:0x%08x\n", ret);
        return -1;
    }

    return 0;
}

int CHiAvDec::Ade_send_data_to_adec(int achn, unsigned char *pBuffer, int DataLen)
{
    int retval = -1;

    AUDIO_STREAM_S AudioStream;
    RMSTREAM_HEADER *pStreamHeader;
    eAudioPayloadType audiotype= APT_INVALID;
    //unsigned char TmpData[1024];
    Stm_ExternFrameInfo_t stuExternInfo;
    pStreamHeader = (RMSTREAM_HEADER *)pBuffer;

    Get_extern_frameinfo(pBuffer, DataLen, stuExternInfo);

    if (stuExternInfo.Mask & EXTERN_FRAMEINFO_AUDIO_INFO)
    {
        if (stuExternInfo.pAudioInfo->lPayloadType == 0)
        {
            audiotype = APT_ADPCM;
        }
        else if (stuExternInfo.pAudioInfo->lPayloadType == 1)
        {
            audiotype = APT_G726;
        }
        else if (stuExternInfo.pAudioInfo->lPayloadType == 2)
        {
            audiotype = APT_G711A;
        }
        else if (stuExternInfo.pAudioInfo->lPayloadType == 3)
        {
            audiotype = APT_G711U;
        }
        else if (stuExternInfo.pAudioInfo->lPayloadType == 5)
        {
            audiotype = APT_AAC;
        }
        else if (stuExternInfo.pAudioInfo->lPayloadType == 6)
        {
            audiotype = APT_LPCM;
        }		
		
        if (audiotype != m_eAudioPlayLoad[achn])
        {
            DEBUG_MESSAGE("create audiotype %d, lBitWidth %d, lSampleRate %ld\n", audiotype, stuExternInfo.pAudioInfo->lBitWidth, stuExternInfo.pAudioInfo->lSampleRate);
            Ade_destory_adec_handle(_AT_ADEC_NORMAL_);
            Ade_create_adec_handle(_AT_ADEC_NORMAL_, audiotype);
            m_eAudioPlayLoad[achn] = audiotype;
        }
    }
   /* 
   unsigned char * pDat = new unsigned char[169];
    memset(pDat, 0, 169);
    

    memcpy(pDat+1,pBuffer+sizeof(RMSTREAM_HEADER)+pStreamHeader->lExtendLen,pStreamHeader->lFrameLen);
    for(int i=0;i<168;i++)
    {
        //printf("copied data[%d]:0x%x \n", i, pDat[i]);
    }
    */
    AudioStream.pStream = pBuffer+sizeof(RMSTREAM_HEADER)+pStreamHeader->lExtendLen;
    AudioStream.u32Len = pStreamHeader->lFrameLen;
    AudioStream.u64TimeStamp = 0;
    AudioStream.u32Seq = 0;

    if ( NULL == AudioStream.pStream)
    {
        return -1;
    }
    for(int i=0;i<168;i++)
    {
        //printf("data[%d]:0x%x addr:%p\n", i, AudioStream.pStream[i],AudioStream.pStream);
    }
    
    retval = HI_MPI_ADEC_SendStream(achn, &AudioStream, HI_IO_NOBLOCK);  //HI_IO_NOBLOCK
    //printf("addr after:%p\n",AudioStream.pStream);

     for(int i=0;i<168;i++)
    {
        //printf("data[%d]:0x%x addr:%p\n", i, AudioStream.pStream[i],AudioStream.pStream);
    }
        
    if(retval != 0)
    {
        PRINTF("HI_MPI_ADEC_SendStream err:0x%08x, lFrameLen %ld\n", retval, pStreamHeader->lFrameLen);
        if((HI_U32)retval == 0xa018800f)
        {
            Ade_clear_ao_chn(achn);
        }
        return -1;
    }
    return 0;
}

int CHiAvDec::Ade_show_video_loss( int purpose, int chn )
{
    return 0;
}

int CHiAvDec::Ade_set_dec_display_mode(VDEC_CHN VdecChn, int purpose)
{
#ifdef _AV_PLATFORM_HI3535_
    HI_S32 ret = -1;
    VIDEO_DISPLAY_MODE_E DisplayMode = VIDEO_DISPLAY_MODE_PLAYBACK;

    if(VDP_PREVIEW_VDEC == purpose)
    {
        DisplayMode = VIDEO_DISPLAY_MODE_PREVIEW;
    }
    DEBUG_MESSAGE("SET PLAY DISPLAY MODE(%d)!\n", DisplayMode);
    ret = HI_MPI_VDEC_SetDisplayMode(VdecChn, DisplayMode);
    if(ret != HI_SUCCESS)
    {
        DEBUG_ERROR( "set display mode failed!\n");
        return -1;
    }
#endif
    return 0;
}


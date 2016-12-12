#include "AvDecoder.h"
#include "SystemConfig.h"
#define PRINTF(fmt...) do{printf("[CAvDecoder %s line:%d]", __FUNCTION__, __LINE__); \
                                      printf(fmt);}while(0);

#define _AVDEC_JUMP_MAX_LEVEL_ 14

CAvDecoder::CAvDecoder()
{
    for (int id = 0; id < _VDEC_MAX_VDP_NUM_; id++)
    {
        m_CurSplitMode[id] = _AS_UNKNOWN_;
        memset(m_CurPlayList[id], -1, _AV_SPLIT_MAX_CHN_*sizeof(int));
    
        m_CurVdecChNum[id] = 0;
        m_pAvPreviewDecoder[id] = NULL;
        for (int index = 0; index < _AV_VDEC_MAX_CHN_; index++)
        {
            memset(&m_AvDecInfo[id][index], 0, sizeof(AVDecInfo_t));
            m_AvDecInfo[id][index].pDataBuf = NULL;

            m_AvDecInfo[id][index].phy_chn = -1;
            m_AvDecInfo[id][index].VdecChn = -1;
            m_AvDecInfo[id][index].bReleaseBuf = true;
            m_AvDecInfo[id][index].bCreateDecChl = false;
            m_AvDecInfo[id][index].PlayFactor = 1;
        }
        m_VdecChmap[id] = 0;

        m_CurDecTime[id].year = 0xff;
        m_CurDecTime[id].month = 0xff;
        m_CurDecTime[id].day = 0xff;
        m_CurDecTime[id].hour = 0xff;
        m_CurDecTime[id].minute = 0xff;
        m_CurDecTime[id].second = 0xff;
    }

    
    m_CurPlayState = PO_INVALID;
    m_PrePlayState = PO_INVALID;
    m_PlayState = PS_INVALID;

    m_pDdsp = NULL;
    m_pMSSP = NULL;

    //the operate func that deal channel decoder to preview
    m_ApdOperate.ClearAoBuf = boost::bind(&CAvDecoder::Ade_clear_ao_chn, this, _1);
    m_ApdOperate.RePlayVideo = boost::bind(&CAvDecoder::Ade_replay_vdec_chn, this, _1, _2);
    m_ApdOperate.PauseVideo = boost::bind(&CAvDecoder::Ade_pause_vdec_chn, this, _1, _2);
    m_ApdOperate.RePlayAudio = boost::bind(&CAvDecoder::Ade_replay_adec, this);
    m_ApdOperate.PauseAudio = boost::bind(&CAvDecoder::Ade_pause_adec, this);
    m_ApdOperate.GetDecWaitFrCnt = boost::bind(&CAvDecoder::Ade_get_dec_wait_frcnt, this, _1, _2, _3, _4, _5);
    m_ApdOperate.SendDataToDec = boost::bind(&CAvDecoder::Ade_send_data_to_decoder, this, _1, _2, _3, _4, _5);
    m_ApdOperate.SetDecPlayFrameRate = boost::bind(&CAvDecoder::Ade_set_play_framerate, this, _1, _2, _3);
    m_ApdOperate.CloseDecoder = boost::bind(&CAvDecoder::Ade_clear_destroy_vdec, this, _1, _2);
    m_ApdOperate.ResetDecoderChannel = boost::bind(&CAvDecoder::Ade_reset_vdec_chn, this, _1, _2);
    m_ApdOperate.SetVoChannelFramerate = boost::bind(&CAvDecoder::Ade_set_vo_chn_framerate_for_nvr_preview, this, _1, _2);
    m_ApdOperate.GetVoChannelFramerate = boost::bind(&CAvDecoder::Ade_get_vo_chn_framerate_for_nvr_preview, this, _1, _2);

    //the operate func that deal multi channel sync to playback
    m_DdspOperate.GetBaseLinePts = boost::bind(&CAvDecoder::Ade_get_mssp_baseline_pts, this, _1, _2, _3);
    m_DdspOperate.RePlayVideo = boost::bind(&CAvDecoder::Ade_replay_vdec_chn, this, _1, _2);
    m_DdspOperate.PauseVideo = boost::bind(&CAvDecoder::Ade_pause_vdec_chn, this, _1, _2);
    m_DdspOperate.StepPlayVideo = boost::bind(&CAvDecoder::Ade_step_play_vdec_chn, this, _1, _2);
    m_DdspOperate.RePlayAudio = boost::bind(&CAvDecoder::Ade_replay_adec, this);
    m_DdspOperate.PauseAudio = boost::bind(&CAvDecoder::Ade_pause_adec, this);
    m_DdspOperate.CheckIndexStatus = boost::bind(&CAvDecoder::Ade_get_mssp_chn_status, this, _1);
    m_DdspOperate.ReleaseFrameData = boost::bind(&CAvDecoder::Ade_release_mssp_frame_info, this, _1);
    m_DdspOperate.GetFrameBufferInfo =  boost::bind(&CAvDecoder::Ade_get_mssp_frame_infoEx, this, _1, _2, _3, _4, _5);
    m_DdspOperate.GetDecWaitFrCnt = boost::bind(&CAvDecoder::Ade_get_dec_wait_frcnt, this, _1, _2, _3, _4, _5);
    m_DdspOperate.SendDataToDecEx = boost::bind(&CAvDecoder::Ade_send_data_to_decoderEx, this, _1, _2, _3, _4, _5);
    m_DdspOperate.ClearAoBuf = boost::bind(&CAvDecoder::Ade_clear_ao_chn, this, _1);
    m_DdspOperate.GetFrameInfo = boost::bind(&CAvDecoder::Ade_get_mssp_frame_info, this, _1, _2);

    m_MaxFrameLen = MAX_FRAME_BUF_LEN;

}


CAvDecoder::~CAvDecoder()
{

}

int CAvDecoder::Ade_destory_allvdec(int purpose)
{
    for (int chn = 0; chn < _AV_VDEC_MAX_CHN_; chn++)
    {
        Ade_release_vdec(purpose, chn);
    }
    return 0;
}

int CAvDecoder::Ade_stop_preview_dec(int purpose)
{
    if( !m_pAvPreviewDecoder[purpose] )
    {
        return -1;
    }

    m_pAvPreviewDecoder[purpose]->StopDecoder(true);

    DEBUG_MESSAGE("CAvDecoder::Ade_stop_preview_dec Ade_destory_allvdec purpose %d\n", purpose);
    Ade_destory_allvdec(purpose);
    
    return 0;
}

int CAvDecoder::Ade_switch_ao(int purpose, int chn)
{
    //DEBUG_MESSAGE("Ade_switch_ao======purpose %d, chn %d\n", purpose, chn);
    
    if( VDP_PLAYBACK_VDEC == purpose )
    {
        if( m_pDdsp )
        {
            m_pDdsp->SwitchAudioChn(chn);
        }
    }
    else 
    {
#if defined(_AV_PRODUCT_CLASS_HDVR_)
        if(chn >= 0)
        {
        	//未在回放状态下才关闭音频解码,否则会出现解码通道创建销毁的情况
        	if(m_VdecChmap[VDP_PLAYBACK_VDEC] == 0)
        	{
		        if (pgAvDeviceInfo->Adi_get_video_source(chn) != _VS_DIGITAL_)
		        {
		            Ade_destory_adec_handle(_AT_ADEC_NORMAL_);
		        }
        	}
        }
#endif       
        if( m_pAvPreviewDecoder[purpose] )
        {
            m_pAvPreviewDecoder[purpose]->SwitchAudioChn(chn);
        }
    }
    
    return 0;
}

int CAvDecoder::Ade_set_mute_flag( int purpose, bool bMute )
{
    if( m_pAvPreviewDecoder[purpose] )
    {
        return m_pAvPreviewDecoder[purpose]->MuteSound(bMute);
    }

    return -1;
}

int CAvDecoder::Ade_start_playback_dec(int purpose, av_tvsystem_t tv_system, int chn_layout[_AV_SPLIT_MAX_CHN_], VdecPara_t stuVdecParam)
{
    int FullFrameRate = 30, PlayFrameRate = 30;
    int layout_index = 0;
    int playFrameRate = 30;
    m_tvsystem = tv_system;

#if !(defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)) 
    if( tv_system == _AT_PAL_ )
    {
        playFrameRate = 25;
    }
#endif

    memset(m_CurPlayList[purpose], -1, sizeof(int)*_AV_SPLIT_MAX_CHN_);   
    for(int index=0; index < stuVdecParam.chnnum; index++)
    {
        int chn = chn_layout[index];
        if ((chn < 0) || (chn >= _AV_VDEC_MAX_CHN_))
        {
            continue;
        }
        if (m_AvDecInfo[purpose][index].pDataBuf != NULL)
        {
            free(m_AvDecInfo[purpose][index].pDataBuf);
            m_AvDecInfo[purpose][index].pDataBuf = NULL;
        }

        if (NULL == m_AvDecInfo[purpose][index].pDataBuf)
        {
            m_AvDecInfo[purpose][index].pDataBuf = (unsigned char *)malloc(m_MaxFrameLen);
        }

        m_AvDecInfo[purpose][index].bCreateDecChl = false;
        m_AvDecInfo[purpose][index].phy_chn = chn;
        m_AvDecInfo[purpose][index].LayoutChn = layout_index;
        m_AvDecInfo[purpose][index].bReleaseBuf = true;
        m_AvDecInfo[purpose][index].AvRefType = _ART_REF_FOR_1X;
        m_AvDecInfo[purpose][index].RealPlayFrameRate = 0;
        GCL_BIT_VAL_SET(m_VdecChmap[purpose], index);
        m_CurPlayList[purpose][index] = chn;
        layout_index++;
        //for fast -> exit
        Ade_set_play_framerate_factor(purpose, index, 1);
        this->Ade_set_play_framerate(purpose, index, playFrameRate);
    }

    m_CurDecTime[purpose].year = 0xff;
    m_CurDecTime[purpose].month = 0xff;
    m_CurDecTime[purpose].day = 0xff;
    m_CurDecTime[purpose].hour = 0xff;
    m_CurDecTime[purpose].minute = 0xff;
    m_CurDecTime[purpose].second = 0xff;

    m_CurPlayState = PO_INVALID;
    m_PrePlayState = PO_INVALID;
    m_PlayState = PS_INVALID;

    DEBUG_CRITICAL("Decode chn num:%d\n", stuVdecParam.chnnum);
    fflush(stdout);

	
    unsigned int buf_size;
    unsigned int buf_cnt;
    char buf_name[32] = {0};

    DEBUG_ERROR("N9M_GetStreamBufferName before\n");
    if(N9M_GetStreamBufferName(BUFFER_TYPE_MSSP_STREAM, 0, buf_name, buf_size, buf_cnt) == 0)
    {
        /*create mssp*/
    DEBUG_ERROR("CMultiStreamSyncParser before,%d,%d,%d,%s\n",stuVdecParam.chnnum,stuVdecParam.stuVdecInfo[0].smi_size,
    								stuVdecParam.stuVdecInfo[0].smi_framenum,buf_name);
        m_pMSSP = new CMultiStreamSyncParser(0, stuVdecParam.chnnum ,buf_name,stuVdecParam.stuVdecInfo[0].smi_size,stuVdecParam.stuVdecInfo[0].smi_framenum, NULL,NULL);		
    DEBUG_ERROR("CMultiStreamSyncParser after\n");

	 if(m_pMSSP==NULL)
	 {
	        DEBUG_ERROR("new CMultiStreamSyncParser faild\n");		
	 }
    }
    else
    {
        DEBUG_ERROR("N9M_GetStreamBufferName faild\n");
    }
    DEBUG_ERROR("N9M_GetStreamBufferName after\n");
    m_CurVdecChNum[purpose] = stuVdecParam.chnnum;
    av_split_t split_mode = _AS_UNKNOWN_;
    if( stuVdecParam.chnnum == 1 )
    {
        split_mode = _AS_SINGLE_;
    }
    else if( stuVdecParam.chnnum <= 4)
    {
        split_mode = _AS_4SPLIT_;
    }
    else if (( stuVdecParam.chnnum > 4) && ( stuVdecParam.chnnum <= 9))
    {
        split_mode = _AS_9SPLIT_;
    }
    else if ( stuVdecParam.chnnum > 9)
    {
        split_mode = _AS_16SPLIT_;
    }
    m_CurSplitMode[purpose] = split_mode; 
    DEBUG_CRITICAL("m_CurSplitMode:%d\n", m_CurSplitMode[purpose]);

    Ade_get_play_framerate(purpose, PlayFrameRate, FullFrameRate);

    m_pDdsp = new CDecDealSyncPlay( purpose, stuVdecParam.chnnum, stuVdecParam.chnnum, &m_DdspOperate );    
    if (m_pDdsp != NULL)
    {      
        /*init the playback framerate parameter*/
        m_CurPlayFrameRate[purpose] = PlayFrameRate;
        m_ResetFrameRate[purpose] = false;
        m_pDdsp->SetFrameRate(0xffffffff, PlayFrameRate, FullFrameRate,true);
    }
    return 0;
}

int CAvDecoder::Ade_pause_playback_dec(bool wait)
{
    eVDecPurpose purpose = VDP_PLAYBACK_VDEC;
    if (m_pDdsp != NULL)
    {
        m_pDdsp->WaitToSepcialPlay(m_VdecChmap[purpose], wait);
    }
    
    return 0;
}
int CAvDecoder::Ade_stop_playback_dec()
{
    eVDecPurpose purpose = VDP_PLAYBACK_VDEC;

    if (m_pDdsp)
    {
        delete m_pDdsp;
        m_pDdsp = NULL;
    }

    if (m_pMSSP)
    {
        delete m_pMSSP;
        m_pMSSP = NULL;
    }

    for(int index=0; index < _AV_VDEC_MAX_CHN_; index++)
    {
        if (m_AvDecInfo[purpose][index].pDataBuf != NULL)
        {
            free(m_AvDecInfo[purpose][index].pDataBuf);
            m_AvDecInfo[purpose][index].pDataBuf = NULL;
        }
    }

    Ade_destory_allvdec(purpose);
    Ade_destory_adec_handle(_AT_ADEC_NORMAL_);

    m_CurPlayState = PO_INVALID;
    m_PrePlayState = PO_INVALID;
    m_PlayState = PS_INVALID;
    m_ResetFrameRate[purpose] = false;
    return 0;
}

int CAvDecoder::Ade_step_play_vdec_chn(int purpose, int chn)
{
    return 0;
}

int CAvDecoder::Ade_replay_adec(void)
{
    return 0;
}

int CAvDecoder::Ade_pause_adec(void)
{
    return 0;
}

int CAvDecoder::Ade_get_mssp_baseline_pts(unsigned long long *pPts, int flag, int direct)
{
    int Type = 0;
    unsigned long long Pts;
    int index = 0;
    static int count = 0;

    if (m_pMSSP == NULL)
    {
        return -1;
    }
    while(1)
    {
#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
        index = m_pMSSP->GetBaselinePts( &Type, &Pts, direct, true );
#else
        index = m_pMSSP->GetBaselinePts( &Type, &Pts, direct, false );
#endif
        if(index >= 0)
        {
            if(flag == GETBASEPTS_FRAME_HOLD)
            {
                if(MSSP_IFRAME == Type || MSSP_PFRAME == Type)
                {
                    *pPts = Pts*1000;
                    break;
                }
                else
                {
                    m_pMSSP->ReleaseFrame(index);
                    if (++count == 10)
                    {
                        count = 0;
                        DEBUG_MESSAGE("GetBaseLinePts failed 10 times, return\n");
                        return -1;
                    }
                }
            }
            else
            {
                if(MSSP_IFRAME == Type)
                {
                    *pPts = Pts*1000;
                    break;
                }
                else
                {
                    m_pMSSP->ReleaseFrame(index);
                    if (++count == 10)
                    {
                        count = 0;
                        DEBUG_ERROR("GetBaseLinePts failed 10 times, return\n");
                        return -1;
                    }
                }
            }
        }
        else
        {
            return -1;
        }
    }

    return 0;
}

int CAvDecoder::Ade_get_mssp_chn_status(int chn)
{
    if (m_pMSSP)
    {
#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
        return m_pMSSP->GetChnStatusEx( chn, 1 );
#else
        return m_pMSSP->GetChnStatusEx( chn, 0 );
#endif
    }

    return -1;
}

int CAvDecoder::Ade_release_mssp_frame_info(int chn)
{
    eVDecPurpose purpose = VDP_PLAYBACK_VDEC;
    if( !m_AvDecInfo[purpose][chn].bReleaseBuf )
    {
        m_AvDecInfo[purpose][chn].bReleaseBuf = true;
       // DEBUG_MESSAGE("############release ch=%d\n", chn);
        return 0;
    }
    
    if( m_pMSSP == NULL )
    {
        return -1;
    }
    //DEBUG_MESSAGE("############release ch normal=%d\n", chn);
    m_pMSSP->ReleaseFrame(chn);
    
    return 0;
}

int CAvDecoder::Ade_reset_mmsp_frame_info( int chn )
{
    DEBUG_MESSAGE("####################release frame ch=%d\n", chn);
    m_AvDecInfo[VDP_PLAYBACK_VDEC][chn].bReleaseBuf = true;
    return 0;
}

int CAvDecoder::Ade_get_mssp_frame_info( int index,PlaybackFrameinfo_t* pInfo)
{
    if (m_pMSSP == NULL)
    {
        DEBUG_ERROR("index %d Mssp is null\n", index);
        return -1;
    }
    int retval = m_pMSSP->GetFrameInfo( index, pInfo);
    if( pInfo->u8FrameType == MSSP_IFRAME )
    {
        pInfo->u8FrameType = DECDATA_IFRAME;
    }
    else if( pInfo->u8FrameType == MSSP_PFRAME )
    {
        pInfo->u8FrameType = DECDATA_PFRAME;
    }
    else if( pInfo->u8FrameType == MSSP_AFRAME )
    {
        pInfo->u8FrameType = DECDATA_AFRAME;
    }
    else
    {
        pInfo->u8FrameType = DECDATA_INVALID;
    }
    return retval;

}

int CAvDecoder::Ade_get_mssp_frame_infoEx( int index, unsigned char**pBuffer, int *pDataLen, PlaybackFrameinfo_t* pInfo, int direction )
{
    unsigned char *pData1 = NULL;
    unsigned char *pData2 = NULL;
    int Len1 = 0, Len2 = 0;
    int retval = 0;

    AVDecInfo_t* pDecInfo = &m_AvDecInfo[VDP_PLAYBACK_VDEC][index];
    if (m_pMSSP == NULL)
    {
        DEBUG_ERROR("index %d Mssp is null\n", index);
        return -1;
    }
    if( !pDecInfo->bReleaseBuf )
    {
        *pBuffer = pDecInfo->pDataBuf;
        *pDataLen = pDecInfo->DataBufLen;
        memcpy( pInfo, &pDecInfo->stuFrameInfo, sizeof(PlaybackFrameinfo_t) );
        //DEBUG_MESSAGE("#########################get frame cp ex index=%d\n", index);
        return 0;
    }
#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
    retval = m_pMSSP->GetFrameEx( index, &pData1, &Len1, &pData2, &Len2, pInfo, true );
#else
    retval = m_pMSSP->GetFrameEx( index, &pData1, &Len1, &pData2, &Len2, pInfo, false );
#endif
    if(retval >= 0)
    {
        if ((Len2 > 0) && (pData2 != NULL))
        {
            unsigned int len = Len1+Len2;
            if ((len < m_MaxFrameLen) && (pDecInfo->pDataBuf != NULL))
            {
                memcpy( pDecInfo->pDataBuf, pData1, Len1 );
                memcpy( pDecInfo->pDataBuf+Len1, pData2, Len2 );
                pDecInfo->DataBufLen = len;
#ifdef _AV_PLATFORM_HI3515_
				const int align = 4;
				int left = (Len1 + Len2) % align;
				if(left > 0)
				{
					memcpy(pDecInfo->pDataBuf + len, 0x0, align - left);
					pDecInfo->DataBufLen += (align - left);
				}
#endif
                memcpy( &pDecInfo->stuFrameInfo, pInfo, sizeof(PlaybackFrameinfo_t) );
                *pBuffer = pDecInfo->pDataBuf;
                *pDataLen = pDecInfo->DataBufLen;
                m_pMSSP->ReleaseFrame( index );
                pDecInfo->bReleaseBuf = false;
                //   DEBUG_MESSAGE("#########################get frame ex index=%d\n", index);
            }
            else
            {
                m_pMSSP->ReleaseFrame( index );
                DEBUG_MESSAGE("\n!!!!!!!!!CAvDecoder::Ade_get_mssp_frame_info err:len:%d pDataBuf:%p index=%d\n\n"
                    , len, pDecInfo->pDataBuf, index);
                return -1;
            }
        }
        else
        {
            //DEBUG_MESSAGE("############get ch normal=%d\n", index);
            *pBuffer = pData1;
            *pDataLen = Len1;
#ifdef _AV_PLATFORM_HI3515_
			const int align = 4;
			int left = Len1 % align;
			if(left > 0)
			{
				memcpy(*pBuffer + Len1, 0x0, align - left);
				*pDataLen += (align - left);
			}
#endif
        }
    }
    else
    {
        return -1;
    }
    if( pInfo->u8FrameType == MSSP_IFRAME )
    {
        pInfo->u8FrameType = DECDATA_IFRAME;
    }
    else if( pInfo->u8FrameType == MSSP_PFRAME )
    {
        pInfo->u8FrameType = DECDATA_PFRAME;
    }
    else if( pInfo->u8FrameType == MSSP_AFRAME )
    {
        pInfo->u8FrameType = DECDATA_AFRAME;
    }
    else
    {
        pInfo->u8FrameType = DECDATA_INVALID;
    }

    return retval;
}

int CAvDecoder::Ade_get_avdev_info(int purpose, int chn, AVDecInfo_t **pAvDecInfo)
{
    if ((purpose < 0) || (purpose >= _VDEC_MAX_VDP_NUM_)
        || (chn < 0) || (chn >= _AV_VDEC_MAX_CHN_))
    {
        DEBUG_MESSAGE("\nAde_get_avdev_info Err purpose=%d ch=%d\n", purpose, chn);
        return -1;
    }

    *pAvDecInfo = &m_AvDecInfo[purpose][chn];
 
    return 0;
}


int CAvDecoder::Ade_set_play_framerate_factor(int purpose, int chn, int factor)
{
    if ((purpose < 0) || (purpose >= _VDEC_MAX_VDP_NUM_)
        || (chn < 0) || (chn >= _AV_VDEC_MAX_CHN_))
    {
        DEBUG_ERROR("\nAde_get_avdev_info Err purpose=%d ch=%d\n", purpose, chn);
        return -1;
    }

    m_AvDecInfo[purpose][chn].PlayFactor = factor;
 
    return 0;
}

eDataPayloadType CAvDecoder::Ade_get_frame_type(unsigned char *pBuffer)
{
    int headerpos = 1;

    if(memcmp(pBuffer+headerpos, "2dc", 3) == 0)//I Frame
    {
        return DPT_IFRAME;
    }
    else if(memcmp(pBuffer+headerpos, "3dc", 3) == 0)//P Frame
    {
        return DPT_PFRAME;
    }
    else if(memcmp(pBuffer+headerpos, "4dc", 3) == 0)//Audio Frame
    {
        return DPT_AFRAME;
    }
    return DPT_INVALID;
}

void CAvDecoder::Ade_get_optype_by_opcode(int opcode, PlayOperate_t &optype)
{
    switch(opcode)
    {
        case 0:
            optype = PO_PLAY;
            break;
        case 1:
            optype = PO_SEEK;
            break;
        case 2:
            optype = PO_PAUSE;
            break;
        case 3:
            optype = PO_FASTFORWARD;
            break;
        case 4:
            optype = PO_FASTBACKWARD;
            break;
        case 5:
            optype = PO_SLOWPLAY;
            break;
        case 6:
            optype = PO_STEP;
            break;
        default:
            optype = PO_INVALID;
            break;
    }
    return;
}

int CAvDecoder::Ade_get_vdec_frame(int purpose, int chn, int &framerate)
{
#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_HDVR_)
    framerate = (m_tvsystem==_AT_PAL_) ? 25 : 30;
#else
    framerate = 30;
#endif
    return 0;
}

int CAvDecoder::Ade_get_dec_playtime(int purpose, datetime_t *playtime)
{
    //DEBUG_MESSAGE("PlayTime Send %02d:%02d:%02d\n", m_DateTime.hour, m_DateTime.minute, m_DateTime.second);

    if (NULL == playtime)
    {
        return -1;
    }

    if(m_CurDecTime[purpose].hour == 0xff)
    {
        DEBUG_ERROR("hour error, PlayTime Send %02d:%02d:%02d\n", 
        m_CurDecTime[purpose].hour, m_CurDecTime[purpose].minute, m_CurDecTime[purpose].second);
        return -1;
    }

    memcpy(playtime, &m_CurDecTime[purpose], sizeof(datetime_t));

    if ((m_CurDecTime[purpose].hour >= 24) 
        || (m_CurDecTime[purpose].minute >= 60) 
        || (m_CurDecTime[purpose].second >= 60))
    {
        DEBUG_ERROR("time error, PlayTime Send %02d:%02d:%02d\n", 
        m_CurDecTime[purpose].hour, m_CurDecTime[purpose].minute, m_CurDecTime[purpose].second);
        return -1;
    }

    return 0;
}

int CAvDecoder::Ade_set_dec_playtime(int purpose, datetime_t playtime, bool force)
{
    bool savetime = false;
    if (VDP_PLAYBACK_VDEC == purpose)
    {
        savetime = false;
        if((m_PlayState >= PS_FASTFORWARD2X) && (m_PlayState <= PS_FASTFORWARD16X) && (m_CurDecTime[purpose].hour < 24))
        {
            if (Ade_time_compare(&playtime, &m_CurDecTime[purpose]) > 0)
            {
                savetime = true;
            }
        }
        else if((m_PlayState >= PS_FASTBACKWARD2X) && (m_PlayState <= PS_FASTBACKWARD16X) && (m_CurDecTime[purpose].hour < 24))
        {
            if(Ade_time_compare(&playtime, &m_CurDecTime[purpose]) < 0)
            {
                savetime = true;
            }
        }
        else
        {
            savetime = true;
        }
    }
    if ((true == savetime) || (true == force))
    {
        memcpy(&m_CurDecTime[purpose], &playtime, sizeof(datetime_t));
    }
    return 0;
}

int CAvDecoder::Ade_time_compare(datetime_t *time1, datetime_t *time2)
{
    int ret = 0;
    ret = (time1->hour * 3600 + time1->minute * 60 + time1->second) - (time2->hour * 3600 + time2->minute * 60 + time2->second);
    return ret;
}

int CAvDecoder::Ade_set_dec_dynamic_fr(int purpose, av_tvsystem_t tv_system, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], bool bswitch_channel)
{
    int chnnum = m_CurVdecChNum[purpose];
    if (purpose != VDP_PLAYBACK_VDEC)
    {
        return 0;
    }
    
    if(true == pgAvDeviceInfo->Adi_is_dynamic_playback_by_split())
    {
        DEBUG_MESSAGE("m_PrePlayState %d, m_CurPlayState %d, mosaic %d\n", m_PrePlayState, m_CurPlayState, split_mode);
        if ((m_PrePlayState == PO_STEP) && (m_CurPlayState == PO_PAUSE) && (split_mode == _AS_SINGLE_))
        {
            DEBUG_MESSAGE("Start Zoom In.......\n");
        }
        else if (m_pDdsp != NULL)
        {
        
            m_pDdsp->WaitToSepcialPlay(m_VdecChmap[purpose], true);
            m_pDdsp->ClearPts(m_VdecChmap[purpose]);
            
            Ade_set_dec_info(purpose, chn_layout, split_mode);
            if (!bswitch_channel)
            {
                m_ResetFrameRate[purpose] = true;
                Ade_dynamic_modify_vdec_framerate(purpose);
            }
            if (m_PlayState != PS_PAUSE && m_CurPlayState != PO_STEP)
            {
                for(int i=0; i<chnnum; i++)
                { 
                    Ade_reset_vdec_chn(purpose, i);
                }
            }
            m_pDdsp->WaitToSepcialPlay(m_VdecChmap[purpose], false);
        
        }
    }
    
    return 0;
}

int CAvDecoder::Ade_set_dec_playback_framerate(int purpose)
{
    int play_fr = 30, full_fr = 30;
    if ((purpose != VDP_PLAYBACK_VDEC) || (false == pgAvDeviceInfo->Adi_is_dynamic_playback_by_split()))
    {
        return 0;
    }
    Ade_get_play_framerate(purpose, play_fr, full_fr);
    
    if (play_fr != m_CurPlayFrameRate[purpose])
    {
        m_pDdsp->SetFrameRate(0xffffffff, play_fr, full_fr);
            
        m_CurPlayFrameRate[purpose] = play_fr;
        DEBUG_MESSAGE("playfr %d, fullfr %d\n", play_fr, full_fr);
    }

    return 0;
}


int CAvDecoder::Ade_pre_init_decoder_param(int purpose, int chn_layout[_AV_SPLIT_MAX_CHN_])
{
    unsigned short u16Width;
    unsigned short u16Height;
    int layout_index = 0;
    int dec_index = 0;

    memset(m_CurPlayList[purpose], -1, sizeof(int)*_AV_SPLIT_MAX_CHN_);
    
    for( int index = 0; index < _AV_SPLIT_MAX_CHN_; index++ )
    {
        int chn = chn_layout[index];

        if ((chn < 0) || (chn >= _AV_VDEC_MAX_CHN_))
        {
            layout_index++;
            continue;
        }
        
        if ((pgAvDeviceInfo->Adi_get_video_source(chn) == _VS_ANALOG_) && ((VDP_PREVIEW_VDEC == purpose) || (VDP_SPOT_VDEC == purpose)))
        {
            //DEBUG_MESSAGE("chn %d is analog layout %d\n", chn, layout_index);
            layout_index++;
            continue;
        }
        //DEBUG_MESSAGE("chn %d is digital layout %d\n", chn, layout_index);
        //printf("----layout_index =%d index =%d----\n",layout_index, index);
        
        
        m_AvDecInfo[purpose][dec_index].VdecChn = index; //set as physical channel
   /*    
        eProductType prod_typ = PTD_INVALID;      
        N9M_GetProductType(prod_typ);
        if ((VDP_PLAYBACK_VDEC == purpose)&& (PTD_X7_I == prod_typ))
        {
            int chn_max = pgAvDeviceInfo->Adi_max_channel_number();
            if (chn_max < 0)
            {
                chn_max = 0;
            }
            m_AvDecInfo[purpose][dec_index].VdecChn = chn + chn_max; // to avoid the collision with spot preview
        }*/

        m_AvDecInfo[purpose][dec_index].LayoutChn = layout_index;
        m_AvDecInfo[purpose][dec_index].phy_chn = chn;
        m_CurPlayList[purpose][dec_index] = chn;    
        m_AvDecInfo[purpose][dec_index].AvRefType = _ART_REF_FOR_1X;
 #if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
        m_AvDecInfo[purpose][dec_index].ullLastIFramePts = 0;
        m_AvDecInfo[purpose][dec_index].u8RealFrameRate = 30;
 #endif

        this->Ade_get_decoder_max_resolution(dec_index, &u16Width, &u16Height );
        m_AvDecInfo[purpose][dec_index].PicWidth = u16Width;
        m_AvDecInfo[purpose][dec_index].PicHeight = u16Height;
        layout_index++;
        dec_index++;
    }
    
    for( int index = dec_index; index < _AV_SPLIT_MAX_CHN_; index++ )
    {
        int chn = chn_layout[index];
        //   DEBUG_MESSAGE("Pre init decoder, index=%d ch=%d\n", index, chn);
        if ((chn < 0) || (chn >= _AV_VDEC_MAX_CHN_))
        {
            //DEBUG_MESSAGE(" lay out[%d]=%d not sutable\n", index, chn);
            m_AvDecInfo[purpose][index].PicWidth = -1;
            m_AvDecInfo[purpose][index].PicHeight = -1;
            m_AvDecInfo[purpose][index].VdecChn = -1;
            m_AvDecInfo[purpose][index].LayoutChn = -1;
            m_AvDecInfo[purpose][index].phy_chn = -1;
            m_AvDecInfo[purpose][index].PlayFactor = 1;
            m_AvDecInfo[purpose][index].AvRefType = _ART_REF_FOR_1X;
        }
    }
    
    return 0;
}

int CAvDecoder::Ade_get_decoder_max_resolution( int ch, unsigned short* pu16Width, unsigned short* pu16Height )
{
    _AV_POINTER_ASSIGNMENT_(pu16Width, 1920);
    _AV_POINTER_ASSIGNMENT_(pu16Height, 1080);

    return 0;
}

int CAvDecoder::Ade_get_encode_analog_video_num(int *iencode_analog_res_num)
{
    int itotal_analog_num = 0;
    for(int res_index = 1; res_index < 11; res_index++)
    {
        int res_num = 0;
        if(res_index == 4 || 8 == res_index)
        {
            continue;
        }
        else
        {
            if (m_AvDecOpe.WhetherEncoderHaveThisResolution((av_resolution_t)res_index, &res_num))
            {
                if (NULL != iencode_analog_res_num)
                {
                    iencode_analog_res_num[res_index] = res_num;
                }
                itotal_analog_num += res_num;
            }
        }
    }
    
    return itotal_analog_num;
}

int CAvDecoder::Ade_get_playback_specified_res_video_num(int purpose, int *res_num)
{
    int u32PlayChlCout = m_CurVdecChNum[purpose];
    int index = 0;
    int result = 0;
    int failedTime = 0;
    
    while( index < u32PlayChlCout)
    {
        MSSP_GetFrameInfo_t stuInfo;
        result = m_pMSSP->GetFrameInfo( index, &stuInfo);
        
        if ( -2 == result )
        {
            index++;
            continue;
        }
        else if ( 0 == result)
        {
            ;
        }
        else
        {
            if ( (failedTime++) < 80) // 4 second
            {
                mSleep( 40 ); //40msec
                // DEBUG_MESSAGE("**********index[%d] Waiting date prapared failed times=%d\n", index, failedTime);
                continue;
            }
            index++;
            continue;
        }
        
        int res = Get_Resvalue_By_Size(stuInfo.u16Width, stuInfo.u16Height);
        
        if (res < 0)
        {
            index++;
            continue;
        }
        else
        {
            if (NULL != res_num)
            {
                (*(res_num + res))++;
            }
        }
        index++;
    }
    
    return 0;
}

int CAvDecoder::Ade_get_playback_framerate_by_split(int purpose, int &playfr, int &fullfr)
{
    fullfr = (m_tvsystem==_AT_PAL_)?25:30;
    playfr = (m_tvsystem==_AT_PAL_)?25:30;

    switch(m_CurSplitMode[purpose])
    {
        case _AS_SINGLE_:
            break;
        case _AS_4SPLIT_:
            break;
        case _AS_6SPLIT_:
            break;
        case _AS_8SPLIT_:
            break;
        case _AS_9SPLIT_:
            break;
        case _AS_10SPLIT_:
            break;
        case _AS_13SPLIT_:
            break;
        case _AS_16SPLIT_:
            break;
        case _AS_25SPLIT_:
            break;
        case _AS_36SPLIT_:
            break;
        default:
            break;
    }
    return 0;
}

int CAvDecoder::Ade_get_play_framerate(int purpose, int &play_fr, int &full_fr)
{
    if(0)
    {
        Ade_get_playback_framerate_by_split(purpose, play_fr, full_fr);
    }
    else
    {
        Ade_get_play_framerate_by_resource(purpose, play_fr, full_fr, true);
    }
    return 0;
}

#ifdef _AVDEC_ADJUST_JUMP_
int CAvDecoder::Ade_get_play_framerate_by_resource(int purpose, int &play_fr, int &full_fr, bool bWait)
{
    int total_resource = 0;
    av_tvsystem_t tvSystem = m_tvsystem;
    int itotal_analog_num = 0;
    
#ifdef _AV_PRODUCT_CLASS_NVR_ 
    tvSystem = _AT_NTSC_;
#endif
    play_fr = (tvSystem ==_AT_PAL_) ? 25 : 30;
    full_fr = play_fr;
    
    /*0-cif;1-hd1;2-d1;3-Qcif;4-QVGA;5-VGA;6-720P;7-1080P, 
     *10-wqcif, 11-wcif, 12-whd1, 13-wd1, 15-540p
     */
    int iplayback_res_num[16] = {0};
    
    /*(index,resolution)
     *0:_AR_SIZE_ 1:_AR_D1_ 2:_AR_HD1_ 3:_AR_CIF_ 4:_AR_QCIF_ 
     *5:_AR_960H_WD1_ 6:_AR_960H_WHD1_ 7:_AR_960H_WCIF_ 8:_AR_960H_WQCIF_
     *9:_AR_1080_ 10:_AR_720_
     */
    int iencode_analog_res_num[11] = {0};
#if !defined(_AV_PRODUCT_CLASS_NVR_)	
		itotal_analog_num = Ade_get_encode_analog_video_num(iencode_analog_res_num);
#else
		itotal_analog_num = 0;
#endif
    Ade_get_playback_specified_res_video_num(purpose, iplayback_res_num);
    if (pgAvDeviceInfo->Adi_get_max_resource(tvSystem, total_resource, itotal_analog_num, iplayback_res_num, iencode_analog_res_num) < 0)
    {
        DEBUG_ERROR("get max resource failed\n");
        return 0;
    }

#if defined(_AV_HAVE_VIDEO_ENCODER_)
    int encoder_used_resource = 0;
    m_AvDecOpe.Get_Encoder_Used_Resource(&encoder_used_resource);
    DEBUG_MESSAGE("total source=%d encoder source=%d\n", total_resource, encoder_used_resource );
    total_resource -= encoder_used_resource;
#endif

    int u32PlayChlCount = m_CurVdecChNum[purpose];
    DEBUG_MESSAGE("##############m_CurVdecChNum[%d]=%d m_CurSplitMode=%d source=%d\n", purpose,u32PlayChlCount, m_CurSplitMode[purpose], total_resource);

    if(m_CurSplitMode[purpose] == _AS_SINGLE_ || m_CurPlayState == PO_STEP || total_resource <= 0)
    {
        for( int index = 0; index < u32PlayChlCount; index++ )
        {
            m_pMSSP->SetFrameJumpLevel(index, 14);
        }
        return 0;
    }

    unsigned char u8RefType[_AV_SPLIT_MAX_CHN_];
    unsigned char u8FrameRate[_AV_SPLIT_MAX_CHN_];
    int multiply_factor[_AV_SPLIT_MAX_CHN_];
    int result = 0;
    int index = 0;
    int failedTime = 0;
    while( index < u32PlayChlCount )
    {
        MSSP_GetFrameInfo_t stuInfo;
        result = m_pMSSP->GetFrameInfo( index, &stuInfo );
        if( result == -2 ) // over
        {
            u8FrameRate[index] = 0;
            u8RefType[index] = 0;
            //failedTime = 0;
            multiply_factor[index] = 0;
            index++;
            continue;
        }
        else if( result == 0 )
        {
            u8FrameRate[index] = stuInfo.u8FrameRate;
            u8RefType[index] = stuInfo.refType;
            //failedTime = 0;
        }
        else
        {
            if( bWait )
            {
                if( (failedTime++) < 80 ) // 4 second
                {
                    mSleep( 40); //40msec
                    //DEBUG_MESSAGE("**********index[%d] Waiting date prapared failed times=%d\n", index, failedTime);
                    continue;
                }
                u8FrameRate[index] = full_fr;
                u8RefType[index] = 1;
                multiply_factor[index] = 0;
                index++;
                continue;
            }
            else
            {
                return -1;
            }
        }

        
        int res = Get_Resvalue_By_Size(stuInfo.u16Width, stuInfo.u16Height);
        // DEBUG_MESSAGE("get resolution, res=%d w=%d h=%d\n", res, pAvDecInfo->PicWidth, pAvDecInfo->PicHeight);
        if (res < 0)
        {
            DEBUG_MESSAGE("index=%d result=%d, res=%d w=%d h=%d ref=%d failedTime=%d\n", index, result, res, stuInfo.u16Width, stuInfo.u16Height, stuInfo.refType, failedTime);
            multiply_factor[index] = 0;
            index++;
            continue;
        }

        av_resolution_t av_resolution = _AR_SIZE_;
        if (pgAvDeviceInfo->Adi_get_video_resolution((uint8_t *)&res, &av_resolution, true) < 0)
        {
            multiply_factor[index] = 0;
            index++;
            continue;
        }
        pgAvDeviceInfo->Adi_get_factor_by_cif(av_resolution, multiply_factor[index]);

        //DEBUG_MESSAGE("index=%d res=%d fact=%d\n", index, av_resolution, multiply_factor[index]);

        index++;
    }

    int encodeSource = 0;
    int playSource = 0;
    int leftSource = total_resource - encodeSource;
    int jumpFrameRate = 0;
    unsigned char jumpLevel = 14;//[_AV_SPLIT_MAX_CHN_];
    while ( true )
    {
        playSource = 0;
        for( int index = 0; index < u32PlayChlCount; index++ )
        {
            jumpFrameRate = m_pMSSP->GetFrameRateByLevel(jumpLevel, u8RefType[index], u8FrameRate[index]);
                
            playSource += ( jumpFrameRate * multiply_factor[index] * play_fr / full_fr );
            //DEBUG_MESSAGE("######################level=%d playfr=%d fullfr=%d source=%d/%d jumpFr=%d oriFr=%d\n"
            //    , jumpLevel, play_fr, full_fr, playSource, leftSource, jumpFrameRate, u8FrameRate[index]);
            if( playSource > leftSource )
            {
                break;
            }
        }

        if( playSource <= leftSource )
        {
            break;
        }
        if( jumpLevel > 2 )
        {
            jumpLevel--; 
        }
        else
        {
            play_fr--;
        }
    }

    for (int index = 0; index < u32PlayChlCount; index++)
    {
        m_pMSSP->SetFrameJumpLevel(index, jumpLevel);
    }

    DEBUG_MESSAGE("######################level=%d playfr=%d fullfr=%d\n", jumpLevel, play_fr, full_fr);
    
    return 0;
}

int CAvDecoder::Ade_reset_mssp_frame_jump_level(bool bFullOrJump)
{
    if( bFullOrJump )
    {
        if( m_pMSSP )
        {
            int u32PlayChlCount = m_CurVdecChNum[VDP_PLAYBACK_VDEC];
            for( int ch=0; ch<u32PlayChlCount; ch++ )
            {
                m_pMSSP->SetFrameJumpLevel(ch, _AVDEC_JUMP_MAX_LEVEL_);
            }
        }
    }
    else
    {
        int play_fr = 30;
        int full_fr = 30;
        if( Ade_get_play_framerate_by_resource(VDP_PLAYBACK_VDEC, play_fr, full_fr, true) == 0 )
        {
            m_CurPlayFrameRate[VDP_PLAYBACK_VDEC] = play_fr;
            m_ResetFrameRate[VDP_PLAYBACK_VDEC] = false;
            m_pDdsp->SetFrameRate(0xffffffff, play_fr, full_fr);
        }
    }

    return 0;
}
#endif

int CAvDecoder::Ade_dynamic_modify_vdec_framerate(int purpose)
{
  //   int chnnum = m_CurVdecChNum[purpose];
    int playfr = 25, fullfr = 25;
    if ((purpose != VDP_PLAYBACK_VDEC) || (m_ResetFrameRate[purpose] == false))
    {
        return 0;
    }
    
    if(false == pgAvDeviceInfo->Adi_is_dynamic_playback_by_split())
    {
        return 0;
    }
    
    Ade_get_play_framerate(purpose, playfr, fullfr);

    if ((m_pDdsp != NULL) && (m_CurPlayFrameRate[purpose] != playfr))
    {
        m_pDdsp->SetFrameRate(0xffffffff, playfr, fullfr);               
        m_CurPlayFrameRate[purpose] = playfr;
        DEBUG_MESSAGE("playfr %d, fullfr %d, mosaic %d\n", playfr, fullfr, m_CurSplitMode[purpose]);           
    }
    
    m_ResetFrameRate[purpose] = false;
    
    return 0;
}

int CAvDecoder::Ade_get_adec_chn(adec_type_e adec_type)
{
    switch (adec_type)
    {
        case _AT_ADEC_NORMAL_: 
            return ADEC_CHN_PLAYBACK;
        case _AT_ADEC_TALKBACK: 
            return 1;
        default:
            DEBUG_ERROR("CAvDecoder::Ade_get_adec_chn (adec_type:%d) ERROR\n", adec_type);
            return -1;
    }
    return 0;
}

bool CAvDecoder::Ade_check_dec_send_data(int purpose, int chn)
{
    bool sendflag = true;

    AVDecInfo_t *pAvDecInfo = NULL; 
    
    if ((VDP_PLAYBACK_VDEC == purpose) && (true == pgAvDeviceInfo->Adi_is_dynamic_playback_by_split()))
    {
        sendflag = false;
        if (Ade_get_avdev_info(purpose, chn, &pAvDecInfo) < 0)
        {
            return sendflag;
        }
        for (int i = 0; i < _AV_SPLIT_MAX_CHN_; i++)
        {
            if (m_CurPlayList[purpose][i] < 0)
            {
                continue;
            }
            if (m_CurPlayList[purpose][i] == pAvDecInfo->phy_chn)
            {
                sendflag = true;
                break;
            }
        }
    }
    return sendflag;
}

int CAvDecoder::Ade_set_dec_info(int purpose,int chn_list[_AV_SPLIT_MAX_CHN_], av_split_t slipmode)
{
    m_CurSplitMode[purpose] = slipmode;
    if (VDP_PLAYBACK_VDEC == purpose)
    {
        memset(m_CurPlayList[purpose], -1, sizeof(int)*_AV_SPLIT_MAX_CHN_);
        DEBUG_MESSAGE("play list:");
        for (int index = 0; index < _AV_SPLIT_MAX_CHN_; index++)
        {
            int chn = chn_list[index];
            if ((chn < 0) || (chn > _AV_SPLIT_MAX_CHN_))
            {
                continue;
            }
            m_CurPlayList[purpose][index] = m_AvDecInfo[purpose][chn].phy_chn;
            DEBUG_MESSAGE("%d ", m_CurPlayList[purpose][index]);
        }
        DEBUG_MESSAGE("\n");
    }
    else
    {        
        memcpy(m_CurPlayList[purpose], chn_list, sizeof(int)*_AV_SPLIT_MAX_CHN_);
    }
    return 0;
}


int CAvDecoder::Ade_get_layout_chn(int purpose, int chn)
{
    AVDecInfo_t *pAvDecInfo = NULL;

    if (VDP_PLAYBACK_VDEC == purpose)
    {
        if (Ade_get_avdev_info(purpose, chn, &pAvDecInfo) < 0)
        {
            return -1;
        }
    }
    else
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

    return pAvDecInfo->LayoutChn;
}

#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
int CAvDecoder::Ade_set_prewdec_channel_map( int purpose, char c8ChlMap[SUPPORT_MAX_VIDEO_CHANNEL_NUM] )
{
    if( !m_pAvPreviewDecoder[purpose] )
    {
        DEBUG_ERROR( "CAvDecoder::Ade_set_prewdec_channel_map not in preview scream purpose=%d\n", purpose );
        return -1;
    }

    m_pAvPreviewDecoder[purpose]->ResetPreviewChnMap( c8ChlMap );
    
    return 0;
}

int CAvDecoder::Ade_set_online_state( int purpose, unsigned char szState[SUPPORT_MAX_VIDEO_CHANNEL_NUM] )
{
    if( !m_pAvPreviewDecoder[purpose] )
    {
        //DEBUG_MESSAGE( "CAvDecoder::Ade_set_online_state not in preview scream purpose=%d\n", purpose );
        return -1;
    }

    return m_pAvPreviewDecoder[purpose]->ResetPreviewOnlineState( szState );
}

int CAvDecoder::Ade_switch_preview_dec(int purpose, unsigned int u32PrewMaxChl, int chn_layout[_AV_SPLIT_MAX_CHN_]
    , char szChlMap[SUPPORT_MAX_VIDEO_CHANNEL_NUM], unsigned char szOnlineState[SUPPORT_MAX_VIDEO_CHANNEL_NUM], unsigned char u8StreamType, av_tvsystem_t eTvSystem,int realvochn_layout[_AV_VDEC_MAX_CHN_])
{
    if( !m_pAvPreviewDecoder[purpose] )
    {
        unsigned int u32ChlNumPerThread = 1;
        m_pAvPreviewDecoder[purpose] = new CAVPreviewDecCtrl( pgAvDeviceInfo->Adi_max_channel_number(), u32ChlNumPerThread, purpose, &m_ApdOperate, eTvSystem);
    }
    
    m_pAvPreviewDecoder[purpose]->ChangePreviewChannel( u32PrewMaxChl, chn_layout, szChlMap, szOnlineState, u8StreamType,realvochn_layout);

    return 0;
}

int CAvDecoder::Ade_restart_preview_decoder( int purpose )
{
    if( !m_pAvPreviewDecoder[purpose] )
    {
        return -1;
    }

    return m_pAvPreviewDecoder[purpose]->RestartDecoder();
}

int CAvDecoder::Ade_set_system_upgrade_state( int purpose, int state )
{
    if( !m_pAvPreviewDecoder[purpose] )
    {
        return -1;
    }

    if( state == 1 )
    {
        m_pAvPreviewDecoder[purpose]->SystemUpgrade();
    }
    else if( state == 2 )
    {
        m_pAvPreviewDecoder[purpose]->SystemUpgradeFailed();
    }
    else
    {
        DEBUG_ERROR( "CAvDecoder::Ade_set_system_upgrade_state purpose=%d, unknow state=%d\n", purpose, state );
        return -1;
    }

    return 0;
}

#endif


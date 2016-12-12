#include "HiAo-3515.h"
#include "AvDebug.h"
#include "mpi_ao.h"
#include "mpi_sys.h"
#include "AvCommonMacro.h"
#include "AvDeviceInfo.h"

CHiAo::CHiAo()
{
#if defined(_AV_SUPPORT_REMOTE_TALK_)
    m_lastAdecChn = -1;
    m_bCvbsForTalking = false;
#endif
}

CHiAo::~CHiAo()
{
}


int CHiAo::HiAo_start_ao(ao_type_e ao_type)
{
    AUDIO_DEV ao_dev = 0;
    AIO_ATTR_S ao_attr;
    
    memset(&ao_attr, 0, sizeof(AIO_ATTR_S));  
    HiAo_get_ao_info(ao_type, &ao_dev, NULL, &ao_attr);
    //0717
#ifdef _AV_PLATFORM_HI3521_
    if(HI_MPI_AO_ClrPubAttr(ao_dev) != 0)
    {
       DEBUG_ERROR( "CHiAo::HiAo_start_ao  HI_MPI_AO_ClrPubAttr FAILED !(ao_dev:%d)", ao_dev);
    }
#endif    
    if (HiAo_memory_balance(ao_dev) != 0)
    {
        DEBUG_ERROR( "CHiAo::HiAo_start_ao  HiAo_memory_balance FAILED !(ao_dev:%d)", ao_dev);
        return -1;
    }
    
    if (HiAo_start_ao_dev(ao_dev, &ao_attr) != 0)
    {
        DEBUG_ERROR( "CHiAo::HiAo_start_ao HiAo_start_ao_dev FAILED! (ao_type:%d) (ao_dev:%d)", ao_type, ao_dev);
        return -1;
    }

    for (unsigned int chn = 0; chn < ao_attr.u32ChnCnt; ++chn)
    {
        if (HiAo_start_ao_chn(ao_dev, chn) != 0)
        {
            DEBUG_ERROR( "CHiAo::HiAo_start_ao HiAo_start_ao_chn FAILED !(ao_dev:%d) (ao_chn:%d)", ao_dev, chn);
            return -1;
        }

#if !defined(_AV_PLATFORM_HI3516A_)        
        if (ao_type == _AO_HDMI_)
        {
            AUDIO_RESAMPLE_ATTR_S ao_resample_attr;
            ao_resample_attr.enInSampleRate = AUDIO_SAMPLE_RATE_8000;
#ifdef _AV_PLATFORM_HI3520D_V300_
	    ao_resample_attr.enOutSampleRate = AUDIO_SAMPLE_RATE_32000;
#else
            ao_resample_attr.enReSampleType = AUDIO_RESAMPLE_1X4;
#endif
            ao_resample_attr.u32InPointNum  = 320;

            if (HiAo_enable_ao_resample(ao_dev, chn, &ao_resample_attr) != 0)
            {
                DEBUG_ERROR( "CHiAo::HiAo_start_ao HiAo_enable_ao_resample (ao_dev:%d) (ao_chn:%d) FAILED!\n", ao_dev, chn);
                return -1;
            }
        }
#endif
    }

#if 0// defined(_AV_PLATFORM_HI3515_)
    /* bind AI to AO channle */
    HI_S32 s32Ret = HI_MPI_AO_BindAi(ao_dev, 0, 0, 0);
    if (s32Ret != HI_SUCCESS)
    {
        DEBUG_ERROR("\nHI_MPI_AO_BindAi error,s32Ret=%d\n",s32Ret);
        return HI_FAILURE;
    }
#endif
    return 0;
}

int CHiAo::HiAo_stop_ao(ao_type_e ao_type)
{
    AUDIO_DEV ao_dev = 0;
    AIO_ATTR_S ao_attr;
    
    memset(&ao_attr, 0, sizeof(AIO_ATTR_S));
    HiAo_get_ao_info(ao_type, &ao_dev, NULL, &ao_attr);

    for (unsigned int chn = 0; chn < ao_attr.u32ChnCnt; ++chn)
    {
        if (ao_type == _AO_HDMI_ && HiAo_disable_ao_resample(ao_dev, chn) != 0)
        {
            DEBUG_ERROR( "CHiAo::HiAo_stop_ao HiAo_disable_ao_resample (ao_dev:%d) (ao_chn:%d) FAILED!\n", ao_dev, chn);
            return -1;
        }

        if (HiAo_stop_ao_chn(ao_dev, chn) != 0)
        {
            DEBUG_ERROR( "CHiAo::HiAo_stop_ao HiAo_stop_ao_chn (ao_dev:%d) (ao_chn:%d) FAILED!\n", ao_dev, chn);
            return -1;
        }
    }

    if (HiAo_stop_ao_dev(ao_dev) != 0)
    {
        DEBUG_ERROR( "CHiAo::HiAo_stop_ao HiAo_stop_ao_dev (ao_dev:%d) FAILED!\n", ao_dev);
        return -1;
    }
    
    //0717
#ifdef _AV_PLATFORM_HI3521_
    if(HI_MPI_AO_ClrPubAttr(ao_dev) != 0)
    {
        DEBUG_ERROR( "CHiAo::HiAo_stop_ao HI_MPI_AO_ClrPubAttr (ao_dev:%d) FAILED!\n", ao_dev);
        return -1;  
    }
#endif    
    return 0;
}

int CHiAo::HiAo_get_ao_info(ao_type_e ao_type, AUDIO_DEV *pAo_dev, AO_CHN *pAo_chn/*=NULL*/, AIO_ATTR_S *pAo_attr/*=NULL*/) const
{
    if (pAo_dev != NULL || pAo_chn != NULL)
    {
#if defined(_AV_PLATFORM_HI3531_) || defined(_AV_PLATFORM_HI3532_)
        switch (ao_type)
        {
            case _AO_HDMI_:
                _AV_POINTER_ASSIGNMENT_(pAo_dev, 5);
                _AV_POINTER_ASSIGNMENT_(pAo_chn, 0);
                break;
            case _AO_TALKBACK_:
                _AV_POINTER_ASSIGNMENT_(pAo_dev, 4);
                _AV_POINTER_ASSIGNMENT_(pAo_chn, 0);
                break;
            case _AO_PLAYBACK_CVBS_:
                _AV_POINTER_ASSIGNMENT_(pAo_dev, 4);
                _AV_POINTER_ASSIGNMENT_(pAo_chn, 0);
                break;
            default:
                DEBUG_ERROR( "CHiAo::HiAi_get_ao_info FAILED(ao_type:%d) no this audio output type\n", ao_type);
                return -1;
        }
#elif defined(_AV_PLATFORM_HI3535_)
        switch(ao_type)
        {
            case _AO_HDMI_:
                _AV_POINTER_ASSIGNMENT_(pAo_dev, 1);
                _AV_POINTER_ASSIGNMENT_(pAo_chn, 0);
                break;  
            case _AO_TALKBACK_:
                _AV_POINTER_ASSIGNMENT_(pAo_dev, 0);
                _AV_POINTER_ASSIGNMENT_(pAo_chn, 0);
                break;
            case _AO_PLAYBACK_CVBS_:
                _AV_POINTER_ASSIGNMENT_(pAo_dev, 0);
                _AV_POINTER_ASSIGNMENT_(pAo_chn, 0);
                break;
            default:
                DEBUG_ERROR( "CHiAo::HiAi_get_ao_info FAILED(ao_type:%d) no this audio output type\n", ao_type);
                return -1;
        }
#elif defined(_AV_PLATFORM_HI3520A_) || defined(_AV_PLATFORM_HI3521_)
        switch (ao_type)
        {
            case _AO_HDMI_:
                _AV_POINTER_ASSIGNMENT_(pAo_dev, 3);
                _AV_POINTER_ASSIGNMENT_(pAo_chn, 0);
                break;  
            case _AO_TALKBACK_:
                _AV_POINTER_ASSIGNMENT_(pAo_dev, 2);
                _AV_POINTER_ASSIGNMENT_(pAo_chn, 0);
                break;
            case _AO_PLAYBACK_CVBS_:
                _AV_POINTER_ASSIGNMENT_(pAo_dev, 2);
                _AV_POINTER_ASSIGNMENT_(pAo_chn, 0);
                break;
            default:
                DEBUG_ERROR( "CHiAo::HiAi_get_ao_info FAILED(ao_type:%d) no this audio output type\n", ao_type);
                return -1;
        }
#elif defined(_AV_PLATFORM_HI3520D_)
        if( ao_type == _AO_HDMI_ )
        {
            _AV_POINTER_ASSIGNMENT_(pAo_dev, 1);
            _AV_POINTER_ASSIGNMENT_(pAo_chn, 0);
        }
        else if( ao_type == _AO_TALKBACK_ || ao_type == _AO_PLAYBACK_CVBS_ )
        {
            _AV_POINTER_ASSIGNMENT_(pAo_dev, 0);
            _AV_POINTER_ASSIGNMENT_(pAo_chn, 0);
        }
        else
        {
            DEBUG_ERROR( "CHiAo::HiAi_get_ao_info FAILED(ao_type:%d) no this audio output type\n", ao_type);
            return -1;
        }
#elif defined(_AV_PLATFORM_HI3520D_V300_)
        if( ao_type == _AO_HDMI_ )
        {
            _AV_POINTER_ASSIGNMENT_(pAo_dev, 1);
            _AV_POINTER_ASSIGNMENT_(pAo_chn, 0);
        }
        else if( ao_type == _AO_TALKBACK_ || ao_type == _AO_PLAYBACK_CVBS_ )
        {
            _AV_POINTER_ASSIGNMENT_(pAo_dev, 0);
            _AV_POINTER_ASSIGNMENT_(pAo_chn, 0);
        }
        else
        {
            DEBUG_ERROR( "CHiAo::HiAi_get_ao_info FAILED(ao_type:%d) no this audio output type\n", ao_type);
            return -1;
        }		
#elif defined(_AV_PLATFORM_HI3518C_) || defined(_AV_PLATFORM_HI3516A_)
        switch (ao_type)
        {
            case _AO_TALKBACK_:
                _AV_POINTER_ASSIGNMENT_(pAo_dev, 0);
                _AV_POINTER_ASSIGNMENT_(pAo_chn, 0);
                break;
                
            default:
                DEBUG_ERROR( "CHiAo::HiAi_get_ao_info FAILED(ao_type:%d) not support this audio output type\n", ao_type);
                return -1;
        }
#elif defined(_AV_PLATFORM_HI3515_)
        switch(ao_type)
        {
            case _AO_HDMI_:
                _AV_POINTER_ASSIGNMENT_(pAo_dev, 0);
                _AV_POINTER_ASSIGNMENT_(pAo_chn, 0);
                break;  
            case _AO_TALKBACK_:
                _AV_POINTER_ASSIGNMENT_(pAo_dev, 0);
                _AV_POINTER_ASSIGNMENT_(pAo_chn, 0);
                break;
            case _AO_PLAYBACK_CVBS_:
                _AV_POINTER_ASSIGNMENT_(pAo_dev, 0);
                _AV_POINTER_ASSIGNMENT_(pAo_chn, 0);
                break;
            default:
                DEBUG_ERROR( "CHiAo::HiAi_get_ao_info FAILED(ao_type:%d) no this audio output type\n", ao_type);
                return -1;
        }		
#endif
    }

    eProductType product_type = PTD_INVALID;
    product_type = pgAvDeviceInfo->Adi_product_type();
	
    if (pAo_attr != NULL)
    {
        pAo_attr->enBitwidth = AUDIO_BIT_WIDTH_16;
        pAo_attr->enWorkmode = AIO_MODE_I2S_MASTER;
#if !defined(_AV_PLATFORM_HI3515_)
        pAo_attr->enSoundmode = AUDIO_SOUND_MODE_MONO;
#else
        pAo_attr->enSoundmode = AUDIO_SOUND_MODE_MOMO;
#endif
        pAo_attr->u32FrmNum = 5;
        pAo_attr->u32ChnCnt = 2;
#ifdef _AV_PLATFORM_HI3520D_V300_
	pAo_attr->u32ClkChnCnt = 2;
#endif
#ifdef _AV_PLATFORM_HI3535_
        pAo_attr->u32ClkSel = 0;
        pAo_attr->u32EXFlag = 1;		
#else 
	if(product_type == PTD_D5_35)
	{
        	pAo_attr->u32ClkSel = 0;
	}
	else
	{
        	pAo_attr->u32ClkSel = 1;
	}
        pAo_attr->u32EXFlag = 0;
#endif
        switch (ao_type)
        {
            case _AO_PLAYBACK_CVBS_:
                HiAo_get_cvbs_workmode( pAo_attr->enWorkmode );
                pAo_attr->enSamplerate = AUDIO_SAMPLE_RATE_8000;
                pAo_attr->u32PtNumPerFrm = 320;
                break;
            
            case _AO_HDMI_:
                pAo_attr->enSamplerate   = AUDIO_SAMPLE_RATE_32000;
                pAo_attr->u32PtNumPerFrm = 320 * 4;
                break;
            
            case _AO_TALKBACK_:
                HiAo_get_cvbs_workmode( pAo_attr->enWorkmode );
                pAo_attr->enSamplerate = AUDIO_SAMPLE_RATE_8000;
                pAo_attr->u32PtNumPerFrm = 320;
                break;
            
            default:
                DEBUG_ERROR( "CHiAo::HiAi_get_ao_info FAILED(ao_type:%d) no this audio output type\n", ao_type);
                return -1;
        }

	if((product_type == PTD_A5_II) || (product_type == PTD_6A_I))
	{
        pAo_attr->u32FrmNum = 40;
        pAo_attr->u32ClkSel = 0;
        pAo_attr->u32EXFlag = 1;		
		if (*pAo_dev == 0)
		{
			pAo_attr->enSamplerate = AUDIO_SAMPLE_RATE_8000;
		}
		else
		{
			pAo_attr->enSamplerate = AUDIO_SAMPLE_RATE_8000;
		}
        }
        DEBUG_WARNING("ao attr samplerate:%d, framenum:%d, clksel:%d, extflag:%d, ptnum:%d, workmode:%d, bitwidth:%d, soudmod:%d\n",\
            pAo_attr->enSamplerate, pAo_attr->u32FrmNum, pAo_attr->u32ClkSel, pAo_attr->u32EXFlag, pAo_attr->u32PtNumPerFrm,\
            pAo_attr->enWorkmode, pAo_attr->enBitwidth,pAo_attr->enSoundmode);
       
    }
 
    return 0;
}

int CHiAo::HiAo_start_ao_dev(AUDIO_DEV ao_dev, const AIO_ATTR_S *pAo_attr) const
{
    int ret_val = -1;
    eProductType product_type = PTD_INVALID;
    product_type = pgAvDeviceInfo->Adi_product_type();
    if(product_type == PTD_D5_35)
    {
	    if ((ret_val = HI_MPI_AO_Disable(ao_dev)) != 0)
	    {
	        DEBUG_ERROR( "CHiAo::HiAo_start_ao_dev HI_MPI_AO_Disable FAILED(ao_dev:%d)(0x%08lx)\n", ao_dev, (unsigned long)ret_val);
	        return -1;
	    }
    }
    if ((ret_val = HI_MPI_AO_SetPubAttr(ao_dev, pAo_attr)) != 0)
    {
        DEBUG_ERROR( "CHiAo::HiAo_start_ao_dev HI_MPI_AO_SetPubAttr FAILED(ao_dev:%d)(0x%08lx)\n", ao_dev, (unsigned long)ret_val);
        return -1;
    }
#if defined(_AV_PLATFORM_HI3515_)
    HI_MPI_SYS_SetReg(0x20050064,0x83218DEF);
#endif
    if ((ret_val = HI_MPI_AO_Enable(ao_dev)) != 0)
    {
        DEBUG_ERROR( "CHiAo::HiAo_start_ao_dev HI_MPI_AO_Enable FAILED(ao_dev:%d)(0x%08lx)\n", ao_dev, (unsigned long)ret_val);
        return -1;
    }
     
    return 0;
}

int CHiAo::HiAo_stop_ao_dev(AUDIO_DEV ao_dev) const
{
    int ret_val = -1;
        
    if ((ret_val = HI_MPI_AO_Disable(ao_dev)) != 0)
    {
        DEBUG_ERROR( "CHiAo::HiAo_stop_ao_dev HI_MPI_AO_Disable FAILED(ao_dev:%d)(0x%08lx)\n", ao_dev, (unsigned long)ret_val);
        return -1;
    }
    
    return 0;
}

int CHiAo::HiAo_start_ao_chn(AUDIO_DEV ao_dev, AO_CHN ao_chn) const
{
    HI_S32 ret_val = -1;

    if ((ret_val = HI_MPI_AO_EnableChn(ao_dev, ao_chn)) != 0)
    {
         DEBUG_ERROR( "CHiAo::HiAo_start_ao_chn HI_MPI_AO_EnableChn FAILED(ao_dev:%d)(ao_chn:%d)(0x%08lx)\n", ao_dev, ao_chn, (unsigned long)ret_val);
        return -1;
    }
    
    return 0;
}

int CHiAo::HiAo_stop_ao_chn(AUDIO_DEV ao_dev, AO_CHN ao_chn) const
{
    int ret_val = -1;

    if ((ret_val = HI_MPI_AO_DisableChn(ao_dev, ao_chn)) != 0)
    {
         DEBUG_ERROR( "CHiAo::HiAo_stop_ao_chn HI_MPI_AO_DisableChn FAILED(ao_dev:%d)(ao_chn:%d)(0x%08lx)\n", ao_dev, ao_chn, (unsigned long)ret_val);
        return -1;
    }
    
    return 0;
}
#if !defined(_AV_PLATFORM_HI3515_)
int CHiAo::HiAo_send_ao_frame(ao_type_e ao_type, const AUDIO_FRAME_S *pAudioframe) const
{
    int ret_val = -1;
    int ao_dev = 0;
    int ao_chn = 0;

    HiAo_get_ao_info(ao_type, &ao_dev, &ao_chn);
#if defined(_AV_PLATFORM_HI3535_)
    if ((ret_val = HI_MPI_AO_SendFrame(ao_dev, ao_chn, pAudioframe, -1)) != 0) // for HI3535, -1  block mode
#elif defined(_AV_PLATFORM_HI3520D_V300_)
    if ((ret_val = HI_MPI_AO_SendFrame(ao_dev, ao_chn, pAudioframe, 1000)) != 0) // for HI3535, -1  block mode
#else
    if ((ret_val = HI_MPI_AO_SendFrame(ao_dev, ao_chn, pAudioframe, HI_TRUE)) != 0)
#endif
    {
         DEBUG_ERROR( "CHiAo::HiAo_send_ao_frame HI_MPI_AO_SendFrame FAILED(ao_dev:%d)(ao_chn:%d)(0x%08lx)\n", ao_dev, ao_chn, (unsigned long)ret_val);
        return -1;
    }
    
    return 0;
}
#endif
int CHiAo::HiAo_clear_ao_chnbuf(ao_type_e ao_type) const
{
    int ret_val = -1;
    AUDIO_DEV ao_dev = 0;
    AO_CHN ao_chn = 0;
    
    HiAo_get_ao_info(ao_type, &ao_dev, &ao_chn);
    
    if ((ret_val = HI_MPI_AO_ClearChnBuf(ao_dev, ao_chn)) != 0)
    {
         DEBUG_ERROR( "CHiAo::HiAo_clear_ao_chnbuf HI_MPI_AO_ClearChnBuf FAILED(ao_dev:%d)(ao_chn:%d)(0x%08lx)\n", ao_dev, ao_chn, (unsigned long)ret_val);
        return -1;
    }
    
    return 0;
}

#if !defined(_AV_PLATFORM_HI3515_)
int CHiAo::HiAo_adec_bind_ao(ao_type_e ao_type, ADEC_CHN adec_chn)
{
#if defined(_AV_PRODUCT_CLASS_NVR_)
    if (pgAvDeviceInfo->Adi_app_type() == APP_TYPE_TEST)
    {
        return 0;
    }
    
    if( ao_type == _AO_PLAYBACK_CVBS_ )
    {
        m_lastAdecChn = adec_chn;
        if(0)
        {
            if (m_bCvbsForTalking)
            {
                return 0;
            }
        }
    }
#endif

    int ret_val = -1;
    MPP_CHN_S stSrcChn,stDestChn;
    stSrcChn.enModId = HI_ID_ADEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = adec_chn;
    stDestChn.enModId = HI_ID_AO;
    HiAo_get_ao_info(ao_type, &stDestChn.s32DevId, &stDestChn.s32ChnId);
    printf("HiAo_adec_bind_ao:ao_type=%d,dev=%d,chn=%d!\n", ao_type, stDestChn.s32DevId, stDestChn.s32ChnId);
    if ((ret_val = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn)) != 0)
    {
         DEBUG_ERROR( "CHiAo::HiAo_adec_bind_ao HI_MPI_SYS_Bind FAILED (ao_type:%d) (adec_chn:%d) (ao_dev:%d) (ao_chn:%d)(0x%08lx)\n", ao_type, adec_chn, stDestChn.s32DevId, stDestChn.s32ChnId, (unsigned long)ret_val);
        return -1;
    } 
    return 0;
}

int CHiAo::HiAo_adec_unbind_ao(ao_type_e ao_type, ADEC_CHN adec_chn)
{
#if defined(_AV_PRODUCT_CLASS_NVR_)
    if (pgAvDeviceInfo->Adi_app_type() == APP_TYPE_TEST)
    {
        return 0;
    }
    
    if( ao_type == _AO_PLAYBACK_CVBS_ )
    {
        m_lastAdecChn = -1;
        if(1)
        {
            if( m_bCvbsForTalking )
            {
                return 0;
            }
        }
    }
#endif

    int ret_val = -1;
    MPP_CHN_S stSrcChn,stDestChn;
    
    stSrcChn.enModId = HI_ID_ADEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = adec_chn;
    
    stDestChn.enModId = HI_ID_AO;
    HiAo_get_ao_info(ao_type, &stDestChn.s32DevId, &stDestChn.s32ChnId);
    if ((ret_val = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn)) != 0)
    {
        DEBUG_ERROR( "CHiAo::HiAo_adec_bind_ao HI_MPI_SYS_UnBind FAILED (ao_type:%d) (adec_chn:%d) (ao_dev:%d) (ao_chn:%d)(0x%08lx)\n", ao_type, adec_chn, stDestChn.s32DevId, stDestChn.s32ChnId, (unsigned long)ret_val);
        return -1;
    }

    return 0;
}
#else //HI3515
int CHiAo::HiAo_adec_bind_ao(ao_type_e ao_type,AO_CHN ao_chn, ADEC_CHN adec_chn)
{
    int ret_val = -1;
    AUDIO_DEV ao_dev;
   // AO_CHN ao_chn;
    HiAo_get_ao_info(ao_type, &ao_dev);
    
    printf("HiAo_adec_bind_ao:ao_type=%d,dev=%d,chn=%d!\n", ao_type, ao_dev, ao_chn);
    if ((ret_val = HI_MPI_AO_BindAdec(ao_dev, ao_chn, adec_chn)) != 0)
    {
         DEBUG_ERROR( "CHiAo::HiAo_adec_bind_ao HI_MPI_SYS_Bind FAILED (ao_type:%d) (adec_chn:%d) (ao_dev:%d) (ao_chn:%d)(0x%08lx)\n", ao_type, adec_chn, ao_dev, ao_chn, (unsigned long)ret_val);
        return -1;
    } 
    return 0;
}

int CHiAo::HiAo_adec_unbind_ao(ao_type_e ao_type,AO_CHN ao_chn, ADEC_CHN adec_chn)
{
    int ret_val = -1;
    AUDIO_DEV ao_dev;
    //AO_CHN ao_chn;
    HiAo_get_ao_info(ao_type, &ao_dev);
    
    printf("HiAo_adec_unbind_ao:ao_type=%d,dev=%d,chn=%d!\n", ao_type, ao_dev, ao_chn);
    if ((ret_val = HI_MPI_AO_UnBindAdec(ao_dev, ao_chn, adec_chn)) != 0)
    {
         DEBUG_ERROR( "CHiAo::HiAo_adec_bind_ao HI_MPI_SYS_Bind FAILED (ao_type:%d) (adec_chn:%d) (ao_dev:%d) (ao_chn:%d)(0x%08lx)\n", ao_type, adec_chn, ao_dev, ao_chn, (unsigned long)ret_val);
        return -1;
    } 

    return 0;
}
int CHiAo::HiAo_adec_bind_ao(ao_type_e ao_type, ADEC_CHN adec_chn)
{
    int ret_val = -1;
    AUDIO_DEV ao_dev;
    AO_CHN ao_chn;
    HiAo_get_ao_info(ao_type, &ao_dev, &ao_chn);
    
    printf("HiAo_adec_bind_ao:ao_type=%d,dev=%d,chn=%d!\n", ao_type, ao_dev, ao_chn);
    if ((ret_val = HI_MPI_AO_BindAdec(ao_dev, ao_chn, adec_chn)) != 0)
    {
         DEBUG_ERROR( "CHiAo::HiAo_adec_bind_ao HI_MPI_SYS_Bind FAILED (ao_type:%d) (adec_chn:%d) (ao_dev:%d) (ao_chn:%d)(0x%08lx)\n", ao_type, adec_chn, ao_dev, ao_chn, (unsigned long)ret_val);
        return -1;
    } 
    return 0;
}

int CHiAo::HiAo_adec_unbind_ao(ao_type_e ao_type,ADEC_CHN adec_chn)
{
    int ret_val = -1;
    AUDIO_DEV ao_dev;
    AO_CHN ao_chn;
    HiAo_get_ao_info(ao_type, &ao_dev, &ao_chn);
    
    printf("HiAo_adec_unbind_ao:ao_type=%d,dev=%d,chn=%d!\n", ao_type, ao_dev, ao_chn);
    if ((ret_val = HI_MPI_AO_UnBindAdec(ao_dev, ao_chn, adec_chn)) != 0)
    {
         DEBUG_ERROR( "CHiAo::HiAo_adec_bind_ao HI_MPI_SYS_Bind FAILED (ao_type:%d) (adec_chn:%d) (ao_dev:%d) (ao_chn:%d)(0x%08lx)\n", ao_type, adec_chn, ao_dev, ao_chn, (unsigned long)ret_val);
        return -1;
    } 

    return 0;
}


#endif

#if defined(_AV_PLATFORM_HI3516A_)
 int CHiAo::HiAo_enable_ao_resample(AUDIO_DEV ao_dev, AO_CHN ao_chn, AUDIO_SAMPLE_RATE_E enInSampleRate) const
{
    int ret_val = -1;

    if ((ret_val = HI_MPI_AO_EnableReSmp(ao_dev, ao_chn, enInSampleRate)) != 0)
    {
         DEBUG_ERROR( "CHiAo::HiAo_enable_ao_resample HI_MPI_AO_EnableReSmp FAILED(ao_dev:%d)(ao_chn:%d)(0x%08lx)\n", ao_dev, ao_chn, (unsigned long)ret_val);
        return -1;
    }
    
    return 0;
} 
#else   
int CHiAo::HiAo_enable_ao_resample(AUDIO_DEV ao_dev, AO_CHN ao_chn, AUDIO_RESAMPLE_ATTR_S *pResampleAttr) const
{
    int ret_val = -1;

#ifdef _AV_PLATFORM_HI3520D_V300_
    if ((ret_val = HI_MPI_AO_EnableReSmp(ao_dev, ao_chn, pResampleAttr->enInSampleRate)) != 0)
#else
    if ((ret_val = HI_MPI_AO_EnableReSmp(ao_dev, ao_chn, pResampleAttr)) != 0)
#endif    
    {
         DEBUG_ERROR( "CHiAo::HiAo_enable_ao_resample HI_MPI_AO_EnableReSmp FAILED(ao_dev:%d)(ao_chn:%d)(0x%08lx)\n", ao_dev, ao_chn, (unsigned long)ret_val);
        return -1;
    }
    
    return 0;
}
#endif

int CHiAo::HiAo_disable_ao_resample(AUDIO_DEV ao_dev, AO_CHN ao_chn) const
{
    int ret_val = -1;
    
    if ((ret_val = HI_MPI_AO_DisableReSmp(ao_dev, ao_chn)) != 0)
    {
         DEBUG_ERROR( "CHiAo::HiAo_disable_ao_resample HI_MPI_AO_DisableReSmp FAILED(ao_dev:%d)(ao_chn:%d)(0x%08lx)\n", ao_dev, ao_chn, (unsigned long)ret_val);
        return -1;
    }
    
    return 0;
}

int CHiAo::HiAo_memory_balance(AUDIO_DEV ao_dev) const
{
#ifdef _AV_PLATFORM_HI3531_
    int ret_val = -1;
    MPP_CHN_S mpp_chn;
    const char *pDdrName = NULL;

    pDdrName = "ddr1";
    mpp_chn.enModId = HI_ID_AO;
    mpp_chn.s32DevId = ao_dev;
    mpp_chn.s32ChnId = 0;
    
    if ((ret_val = HI_MPI_SYS_SetMemConf(&mpp_chn, pDdrName)) != 0)
    {
        DEBUG_ERROR( "CHiAo::HiAo_memory_balance HI_MPI_SYS_SetMemConf FAILED(ao_dev:%d) (0x%08lx)\n", ao_dev, (unsigned long)ret_val);
        return -1;
    }
#endif

    return 0;
}

int CHiAo::HiAo_get_cvbs_workmode(AIO_MODE_E& mode) const
{
#if defined(_AV_PLATFORM_HI3535_) || defined(_AV_PLATFORM_HI3521_) || defined(_AV_PLATFORM_HI3531_)|| defined(_AV_PLATFORM_HI3515_)
    mode = AIO_MODE_I2S_MASTER;
#else
    mode = AIO_MODE_I2S_SLAVE;
#endif
    return 0;
}

#if defined(_AV_SUPPORT_REMOTE_TALK_)
int CHiAo::HiAo_unbind_cvbs_ao_for_talk( bool bUnBind )
{
    if( 1)
    {
        return 0;
    }
    
    if( bUnBind )
    {        
        ADEC_CHN temp = m_lastAdecChn;
        
        if( m_lastAdecChn == -1 )
        {
            m_bCvbsForTalking = true;
            return 0;
        }
    
        this->HiAo_adec_unbind_ao(_AO_PLAYBACK_CVBS_, temp );
        m_lastAdecChn = temp;

        m_bCvbsForTalking = true;
    }
    else
    {
        m_bCvbsForTalking = false;
        if( m_lastAdecChn == -1 )
        {
            return 0;
        }
    
        this->HiAo_adec_bind_ao(_AO_PLAYBACK_CVBS_, m_lastAdecChn );
    }

    return 0;

}
#endif

int CHiAo::HiAo_set_ao_volume(AUDIO_DEV dev, unsigned char volume)
{
#ifdef _AV_PLATFORM_HI3535_
    HI_S32 ret = 0, AdjustVolume = 0;
    if(volume > 63)
    {
        volume = 63;
    }
    if(volume >= 40)
    {
        AdjustVolume = volume - 63;
    }
    else
    {
        AdjustVolume = (26 + volume / 3) - 63;
    }
    ret = HI_MPI_AO_SetVolume(dev, AdjustVolume);
    if(ret != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiAo::HiAo_set_ao_volume set ao volume failed(dev=%d, volume=%d, ret:0x%x)!\n", dev, volume,ret);
        return -1;
    }
#endif
    return 0;
}

#if defined(_AV_HAVE_AUDIO_INPUT_)

int CHiAo::HiAo_ai_bind_ao(ao_type_e ao_type,AUDIO_DEV ai_dev,AI_CHN ai_chn)
{
    int ret_val = -1;
    AUDIO_DEV ao_dev;
    AO_CHN ao_chn;
    //AIO_ATTR_S ao_attr;

    HiAo_get_ao_info(ao_type,&ao_dev, &ao_chn, NULL);

    ret_val = HI_MPI_AO_BindAi(ao_dev,ao_chn, ai_dev, ai_chn);
 
    if (ret_val != 0)
    {
        DEBUG_ERROR("HI_MPI_AO_BindAi failed(0x%08x)!ao_dev:%d, ao_chn:%d, ai_dev:%d, ai_chn:%d\n",ret_val,ao_dev,ao_chn, ai_dev, ai_chn);
        return -1;
    }
    return 0;
}

int CHiAo::HiAo_ai_unbind_ao(ao_type_e ao_type,AUDIO_DEV ai_dev,AI_CHN ai_chn)
{
    int ret_val = -1;
    AUDIO_DEV ao_dev;
    AO_CHN ao_chn;
    //AIO_ATTR_S ao_attr;

    HiAo_get_ao_info(ao_type,&ao_dev, &ao_chn, NULL);

    if ((ret_val = HI_MPI_AO_UnBindAi(ao_dev,ao_chn, ai_dev, ai_chn)) != 0)
    {
        DEBUG_ERROR("HI_MPI_AO_UnBindAi failed(0x%08x)!ao_dev:%d, ao_chn:%d, ai_dev:%d, ai_chn:%d\n",ret_val,ao_dev,ao_chn, ai_dev, ai_chn);
        return -1;
    }

    return 0;
}

#endif

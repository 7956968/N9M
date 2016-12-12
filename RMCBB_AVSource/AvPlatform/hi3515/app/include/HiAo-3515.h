/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __HIAO3515_H__
#define __HIAO3515_H__

#include "AvConfig.h"
#include "AvCommonType.h"
#include "hi_common.h"
#include "hi_comm_aio.h"
#include "HiAi-3515.h"


class CHiAo
{
public:
    CHiAo();
	~CHiAo();

public:
    int HiAo_start_ao(ao_type_e ao_type);
    int HiAo_stop_ao(ao_type_e ao_type);
    int HiAo_clear_ao_chnbuf(ao_type_e ao_type) const;
    int HiAo_send_ao_frame(ao_type_e ao_type, const AUDIO_FRAME_S *pAudioframe) const;
#if defined (_AV_PLATFORM_HI3515_)
    int HiAo_adec_bind_ao(ao_type_e ao_type, AO_CHN ao_chn, ADEC_CHN adec_chn);
    int HiAo_adec_unbind_ao(ao_type_e ao_type,AO_CHN ao_chn, ADEC_CHN adec_chn);
	int HiAo_adec_bind_ao(ao_type_e ao_type, ADEC_CHN adec_chn);
	int HiAo_adec_unbind_ao(ao_type_e ao_type,ADEC_CHN adec_chn);
#else
	int HiAo_adec_bind_ao(ao_type_e ao_type, ADEC_CHN adec_chn);
	int HiAo_adec_unbind_ao(ao_type_e ao_type,ADEC_CHN adec_chn);

#endif

    int HiAo_get_ao_info(ao_type_e ao_type, AUDIO_DEV *pAo_dev, AO_CHN *pAo_chn=NULL, AIO_ATTR_S *pAo_attr=NULL) const;

    //for the machine, use cvbs ao for line-out
#if defined(_AV_SUPPORT_REMOTE_TALK_)
    int HiAo_unbind_cvbs_ao_for_talk( bool bUnBind );
#endif

	int HiAo_set_ao_volume(AUDIO_DEV dev, unsigned char volume);

private:
    int HiAo_start_ao_dev(AUDIO_DEV ao_dev, const AIO_ATTR_S *pAo_attr) const;
    int HiAo_stop_ao_dev(AUDIO_DEV ao_dev) const;
    int HiAo_start_ao_chn(AUDIO_DEV ao_dev, AO_CHN ao_chn) const;
    int HiAo_stop_ao_chn(AUDIO_DEV ao_dev, AO_CHN ao_chn) const;

#if defined(_AV_PLATFORM_HI3516A_)
    int HiAo_enable_ao_resample(AUDIO_DEV ao_dev, AO_CHN ao_chn, AUDIO_SAMPLE_RATE_E enInSampleRate) const;
#else
    int HiAo_enable_ao_resample(AUDIO_DEV ao_dev, AO_CHN ao_chn, AUDIO_RESAMPLE_ATTR_S *pResampleAttr) const;
#endif
    int HiAo_disable_ao_resample(AUDIO_DEV ao_dev, AO_CHN ao_chn) const;
    int HiAo_memory_balance(AUDIO_DEV ao_dev) const;
    int HiAo_get_cvbs_workmode(AIO_MODE_E& mode) const;

public:
#if defined(_AV_HAVE_AUDIO_INPUT_)
	int HiAo_ai_bind_ao(ao_type_e ao_type, AUDIO_DEV ao_dev,AI_CHN ai_chn);
	int HiAo_ai_unbind_ao(ao_type_e ao_type, AUDIO_DEV ao_dev,AI_CHN ai_chn);
#endif

#if defined(_AV_SUPPORT_REMOTE_TALK_)
private:
    ADEC_CHN m_lastAdecChn; 
    bool m_bCvbsForTalking;
#endif
};

#endif /*__HIAO_H__*/

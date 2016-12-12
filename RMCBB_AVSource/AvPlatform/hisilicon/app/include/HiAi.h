/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __HIAI_H__
#define __HIAI_H__

#include "AvConfig.h"
#include "AvCommonType.h"
#include "hi_common.h"
#include "hi_comm_aio.h"
#include "CommonDebug.h"
typedef struct _ai_chn_map_
{
    int phy_chn;
    AI_CHN ai_chn;
} ai_chn_map_t;


class CHiAi
{
public:
    CHiAi();
	~CHiAi();

public:
    int HiAi_start_ai(ai_type_e ai_type);
    int HiAi_stop_ai(ai_type_e ai_type);
#ifdef _AV_PLATFORM_HI3531_
    int HiAi_start_preview_ai_2chip();
    int HiAi_stop_preview_ai_2chip();
#endif
    int HiAi_get_fd(ai_type_e ai_type, int phy_chn) const;
    int HiAi_get_ai_info(ai_type_e ai_type, int phy_chn, AUDIO_DEV *pAi_dev, AI_CHN *pAi_chn=NULL, AIO_ATTR_S *pAi_attr=NULL) const;
    int HiAi_get_ai_frame(ai_type_e ai_type, int phy_chn, AUDIO_FRAME_S *pAiframe, AEC_FRAME_S *pAecframe=NULL) const;
    int HiAi_release_ai_frame(ai_type_e ai_type, int phy_chn, AUDIO_FRAME_S *pAiframe, AEC_FRAME_S *pAecframe=NULL) const;
    int HiAi_ai_bind_aenc( ai_type_e eAiType, int phy_chn, int vencChn );
    int HiAi_ai_unbind_aenc( ai_type_e eAiType, int phy_chn, int vencChn );
#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
    //for some audio chip, must init hisiCPU as slave mode first,but nvr would init when starting a talk
    int HiAi_ai_init_aidev_mode(ai_type_e eAiType);
#endif
#if defined(_AV_PRODUCT_CLASS_IPC_)
    int HiAi_ai_set_input_volume( unsigned char u8Volume );
//! Added for IPC transplant on Nov 2014
	int HiAi_config_acodec_volume(); 
private:
    unsigned char m_u8InputVolume;
#endif

private:
    int HiAi_start_ai_dev(AUDIO_DEV ai_dev, const AIO_ATTR_S *pAi_attr) const;
    int HiAi_stop_ai_dev(AUDIO_DEV ai_dev) const;
    int HiAi_start_ai_chn(AUDIO_DEV ai_dev, AI_CHN ai_chn) const;
    int HiAi_stop_ai_chn(AUDIO_DEV ai_dev, AI_CHN ai_chn) const;
    int HiAi_set_ai_chn_param(AUDIO_DEV ai_dev, AI_CHN ai_chn, unsigned int usrFrmDepth) const;
    int HiAi_memory_balance(AUDIO_DEV ai_dev);

 #if defined(_AV_PRODUCT_CLASS_IPC_)
    int HiAi_config_acodec( bool bMacIn, AUDIO_SAMPLE_RATE_E enSample, unsigned char u8Volum );//no longer be called
//!Added for IPC transplant on Nov 2014
    int HiAi_config_acodec(AUDIO_SAMPLE_RATE_E enSample);
    int HiAi_start_vqe(AUDIO_DEV ai_dev, AI_CHN ai_chn, AI_VQE_CONFIG_S* pVqe_config) const;
    int HiAi_stop_vqe(AUDIO_DEV ai_dev, AI_CHN ai_chn)const;    
#if defined(_AV_PLATFORM_HI3518C_)
    int HiAi_start_hpf(AUDIO_DEV ai_dev, AI_CHN ai_chn,const  AI_HPF_ATTR_S *pstHpfAttr) const;
    int HiAi_stop_hpf(AUDIO_DEV ai_dev, AI_CHN ai_chn) const;
#endif
 #endif

};

#endif /*__HIAI_H__*/

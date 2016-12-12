/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __HIAVDEC3515_H__
#define __HIAVDEC3515_H__

#include "Defines.h"
#include "System.h"
#include "AvCommonType.h"
#include "mpi_vdec.h"
#include "mpi_adec.h"

#include "boost/function.hpp"
#include "boost/bind.hpp"
#include "HiVo-3515.h"
#include "HiAo-3515.h"
//#include "HiAvDeviceMaster-3515.h"
#include "AvDecoder.h"
#include "CommonDebug.h"
#include "AvThreadLock.h"

class CHiVpss;
class CHiAvDec:public CAvDecoder{
    public:
        CHiAvDec();
        virtual ~CHiAvDec();

        int Ade_get_vdec_max_size(int purpose, int chn, int &max_width, int &max_height);

        int Ade_get_vdec_video_size_for_ui(int purpose, int phychn, int &max_width, int &max_height);

        int Ade_get_decocer_chn_index( int purpose, int phychn );

        int Ade_get_vdec_info(int purpose, int chn, int & Vdec_dev, VDEC_CHN & Vdec_chn);

        int Ade_set_vo_chn_framerate(int purpose, int chn);
        int Ade_set_vo_chn_framerate_for_nvr_preview(int chn, int fr);
        int Ade_get_vo_chn_framerate_for_nvr_preview(int chn, int& fr);

        int Ade_set_vo(CHiVo *pHi_Vo){m_pHi_Vo = pHi_Vo; return 0;};
        
        int Ade_set_ao(CHiAo *pHi_Ao){m_pHi_Ao = pHi_Ao; return 0;};

        int Ade_set_vpss( CHiVpss* pHi_vpss ) {m_pHi_vpss = pHi_vpss; return 0;   }
        

        int Ade_create_adec_handle(adec_type_e adec_type, eAudioPayloadType audiotype);

        int Ade_destory_adec_handle(adec_type_e adec_type);

        int Ade_create_vdec_handle(int purpose, int chn, RMFI2_VIDEOINFO *pVideoInfo);
        int Ade_set_start_send_data_flag(bool isStartFlag);
        
        int Ade_replay_vdec_chn(int purpose, int chn);

        int Ade_pause_vdec_chn(int purpose, int chn);

        int Ade_get_dec_wait_frcnt(int purpose, int chn, int *pFrCnt, int *pDataLen, int Type);

        int Ade_send_data_to_decoder(int purpose, int chn, unsigned char * pBuffer, int DataLen, int Type);

        int Ade_send_data_to_decoderEx(int purpose, int chn, unsigned char * pBuffer, int DataLen, const PlaybackFrameinfo_t* pstuFrameInfo);
        
        int Ade_clear_ao_chn(int AdecChn);

        int Ade_operate_decoder(int purpose, int opcode, bool operate);

		int Ade_operate_decoder_for_switch_channel(int purpose);

        int Ade_release_vdec(int purpose, int chn);

        int Ade_set_play_framerate(int purpose, int chn, int framerate);

        int Ade_get_vo_dev(int purpose);
		
        PlayState_t Ade_get_cur_playstate(int purpose);
private:
        int Ade_create_vdec(int VdecChn, int w, int h);
        
        int Ade_destory_vdec(int VdecChn);
        
        int Ade_reset_vdec(int chn);
        
        int Ade_clear_vdec_chn(int purpose, int chn);

        //for nvr
        int Ade_clear_destroy_vdec(int purpose, int chn);

        int Ade_reset_vdec_chn(int purpose, int chn);
        
        int Ade_send_data_to_vdec(int purpose, int chn, unsigned char * pBuffer, int DataLen);
        
        int Ade_send_data_to_vdecEx(int purpose, int chn, unsigned char * pBuffer, int DataLen, const PlaybackFrameinfo_t* pstuFrameInfo);
public:
        int Ade_create_adec(int achn, eAudioPayloadType type);
        
        int Ade_destroy_adec(int achn);
private:        
        int Ade_send_data_to_adec(int achn, unsigned char * pBuffer, int DataLen);

        int Ade_operate_playback(int purpose, PlayOperate_t ope, bool operate);

        int Ade_show_video_loss( int purpose, int chn );

		int Ade_set_dec_display_mode(VDEC_CHN VdecChn, int purpose);

    private:
    	CHiVo *m_pHi_Vo;
    	CHiAo *m_pHi_Ao;
        CHiVpss *m_pHi_vpss;
       // CHiAvDeviceMaster *m_pHi_AvDeviceMaster;
        eAudioPayloadType m_eAudioPlayLoad[MAX_AUDIO_CHNNUM];
        bool m_bStartFlag;
       CAvThreadLock* m_pMuiltThreadDecLock;
        
};

#endif/*__HIAVDEC_H__*/



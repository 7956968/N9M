/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __AVDECODER_H__
#define __AVDECODER_H__
#include "AvConfig.h"
#include "AvDebug.h"
#include "AvCommonMacro.h"
#include "AvCommonType.h"
#include "AvDeviceInfo.h"

#include "StreamBuffer.h"
#include "SystemTime.h"
#include "CommonMacro.h"
#include "MultiStreamSyncParser.h"
#include "AvDecDealSyncPlay.h"
#include "AvPreviewDecoder.h"

using namespace Common;

#define _AVDEC_ADJUST_JUMP_ 

#define MAX_FRAME_BUF_LEN 384*1024*4;
#define ADEC_CHN_PLAYBACK   0


typedef struct
{
	/**
	 * @brief 绑定vo通道
	 * @param-purpose, phy_chn
	 * @return 0:成功 -1:失败
	 */
	boost::function<int(int, int)>ConnectToVo;
	boost::function<int(int, int)>DisconnectToVo;
	boost::function<av_tvsystem_t(av_output_type_t , int)>GetTvSystem;
	boost::function<int(int *)>Get_Encoder_Used_Resource;
    boost::function<bool(av_resolution_t, int *)> WhetherEncoderHaveThisResolution;
} AvDec_Operation_t;


typedef enum {
    VDP_PREVIEW_VDEC = 0,
    VDP_PLAYBACK_VDEC,
    VDP_SPOT_VDEC,
    VDP_INVALID
}eVDecPurpose;

typedef enum _adec_type_
{
    _AT_ADEC_NORMAL_, // preview and playback audio decode
    _AT_ADEC_TALKBACK,
} adec_type_e;

typedef enum
{
	APT_ADPCM = 0,
	APT_G726,
	APT_G711A,
	APT_G711U,	
	APT_AMR,
	APT_AAC,
	APT_LPCM,
	APT_INVALID
} eAudioPayloadType;

typedef enum
{
	DPT_IFRAME = 0,
	DPT_PFRAME,
	DPT_AFRAME,
	DPT_INVALID
} eDataPayloadType;

typedef struct
{
	int VdecChn;
	int LayoutChn;
	int PicWidth;
	int PicHeight;
	int AudioFd;
	int RealPlayFrameRate;

    unsigned char *pDataBuf;
	int DataBufLen;
	bool bReleaseBuf; //是否已释放
    PlaybackFrameinfo_t stuFrameInfo;
    
	bool RequestIFrame;
	bool Run;
    bool bCreateDecChl; // mark for if the decoder channel is created
    int phy_chn; 
    int PlayFactor;
    av_ref_type_t AvRefType;
    int SendFrameCnt;
    int TotalFrameCnt;
 #if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
    unsigned char u8LastYear;
    unsigned char u8LastMonth;
    unsigned char u8LastDay;
    unsigned char u8LastHour;
    unsigned char u8LastMinute;
    unsigned char u8LastSecond;
    unsigned char u8LastSendSec;
    unsigned char u8RealFrameRate;
    unsigned long long ullLastIFramePts; // for nvr,,time playback time calc 
 #endif
} AVDecInfo_t;

typedef struct
{
	int PicWidth;
	int PicHeight;
	int smi_size;
	int smi_framenum;
	char smi_name[32];
} VdecInfo_t;

typedef struct
{
	int chnnum;
	VdecInfo_t stuVdecInfo[_AV_SPLIT_MAX_CHN_]; 
}VdecPara_t;

class CAvDecoder{
    public:
        CAvDecoder();
        virtual ~CAvDecoder();

    public:
        
        int Ade_start_playback_dec(int purpose, av_tvsystem_t tv_system, int chn_layout[_AV_SPLIT_MAX_CHN_], VdecPara_t stuVdecParam);
	
        int Ade_stop_playback_dec();
             
        int Ade_stop_preview_dec(int purpose);

        int Ade_switch_ao(int purpose, int chn);

        int Ade_get_layout_chn(int purpose, int chn);
		
        /**
         * @brief set mute flag, used for nvr preview
         **/
        int Ade_set_mute_flag( int purpose, bool bMute );

        int Ade_get_avdev_info(int purpose, int chn, AVDecInfo_t **pAvDecInfo);

        int Ade_set_play_framerate_factor(int purpose, int chn, int factor);
        
        int Ade_get_mssp_baseline_pts(unsigned long long *pPts, int flag, int direct);

        int Ade_get_mssp_frame_infoEx( int index, unsigned char**pBuffer, int *pDataLen, PlaybackFrameinfo_t* pInfo, int direction );

        int Ade_get_mssp_frame_info( int index, PlaybackFrameinfo_t* pInfo);

        int Ade_release_mssp_frame_info(int chn);

        int Ade_reset_mmsp_frame_info( int chn );

        int Ade_get_mssp_chn_status(int chn);
        
        int Ade_destory_allvdec(int purpose);

        int Ade_step_play_vdec_chn(int purpose, int chn);

        int Ade_replay_adec(void);

        int Ade_pause_adec(void);

        eDataPayloadType Ade_get_frame_type(unsigned char *pBuffer);

        void Ade_get_optype_by_opcode(int opcode, PlayOperate_t &optype);

        int Ade_get_vdec_frame(int purpose, int chn, int &framerate);

        int Ade_get_dec_playtime(int purpose, datetime_t *playtime);

        int Ade_set_dec_playtime(int purpose, datetime_t playtime, bool force);

        int Ade_time_compare(datetime_t *time1, datetime_t *time2);

#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
        int Ade_switch_preview_dec( int purpose, unsigned int u32PrewMaxChl, int chn_layout[_AV_SPLIT_MAX_CHN_]
             , char szChlMap[SUPPORT_MAX_VIDEO_CHANNEL_NUM], unsigned char szOnlineState[SUPPORT_MAX_VIDEO_CHANNEL_NUM], unsigned char u8StreamType, av_tvsystem_t eTvSystem,int realvochn_layout[_AV_VDEC_MAX_CHN_]);
        int Ade_set_prewdec_channel_map( int purpose, char c8ChlMap[SUPPORT_MAX_VIDEO_CHANNEL_NUM] );
        int Ade_set_online_state( int purpose, unsigned char szState[SUPPORT_MAX_VIDEO_CHANNEL_NUM] );
        int Ade_restart_preview_decoder( int purpose );
        int Ade_set_system_upgrade_state( int purpose, int state );
#endif
        int Ade_get_adec_chn(adec_type_e adec_type);

        int Ade_set_dec_info(int purpose,int chn_list[_AV_SPLIT_MAX_CHN_], av_split_t slipmode);

        bool Ade_check_dec_send_data(int purpose, int chn);

        int Ade_set_dec_dynamic_fr(int purpose, av_tvsystem_t tv_system, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], bool bswitch_channel = false);

        int Ade_set_dec_operate_callback(AvDec_Operation_t *pOperation){m_AvDecOpe = *pOperation; return 0;};

        int Ade_dynamic_modify_vdec_framerate(int purpose);

        int Ade_pre_init_decoder_param(int purpose, int chn_layout[_AV_SPLIT_MAX_CHN_]);

        int Ade_pause_playback_dec(bool wait);
#ifdef _AVDEC_ADJUST_JUMP_
        int Ade_reset_mssp_frame_jump_level(bool bFullOrJump);
#endif
        av_tvsystem_t Ade_get_tvsystem(){   return m_tvsystem;  }

  public:
        virtual int Ade_create_adec_handle(adec_type_e adec_type, eAudioPayloadType audiotype) = 0;

        virtual int Ade_destory_adec_handle(adec_type_e adec_type) = 0;
        
        virtual int Ade_create_vdec_handle(int purpose, int chn, RMFI2_VIDEOINFO *pVideoInfo) = 0;
        virtual int Ade_set_start_send_data_flag(bool isStartFlag) = 0;
        
        virtual int Ade_set_play_framerate(int purpose, int chn, int framerate) = 0;
        
        virtual int Ade_release_vdec(int purpose, int chn) = 0;

        virtual int Ade_send_data_to_decoder(int purpose, int chn, unsigned char * pBuffer, int DataLen, int Type) = 0;

        virtual int Ade_send_data_to_decoderEx(int purpose, int chn, unsigned char * pBuffer, int DataLen, const PlaybackFrameinfo_t* pstuFrameInfo) = 0;

        virtual int Ade_replay_vdec_chn(int purpose, int chn) = 0;

        virtual int Ade_pause_vdec_chn(int purpose, int chn) = 0;

        virtual int Ade_reset_vdec_chn(int purpose, int chn) = 0;

        virtual int Ade_get_dec_wait_frcnt(int purpose, int chn, int *pFrCnt, int *pDataLen, int Type) = 0;

        virtual int Ade_clear_ao_chn(int AdecChn) = 0;

        virtual int Ade_operate_decoder(int purpose, int opcode, bool operate) = 0;

		virtual int Ade_operate_decoder_for_switch_channel(int purpose) = 0;

        virtual int Ade_show_video_loss( int purpose, int chn ) = 0;

        virtual int Ade_clear_vdec_chn( int purpose, int chn ) = 0;

        virtual int Ade_clear_destroy_vdec(int purpose, int chn) = 0;

        virtual int Ade_set_vo_chn_framerate_for_nvr_preview(int chn, int fr) = 0;
        virtual int Ade_get_vo_chn_framerate_for_nvr_preview(int chn, int& fr) = 0;

    private:
        int Ade_get_decoder_max_resolution( int ch, unsigned short* pu16Width, unsigned short* pu16Height );
#ifdef _AVDEC_ADJUST_JUMP_
        int Ade_get_play_framerate_by_resource(int purpose, int &play_fr, int &full_fr, bool bWait);
#endif
        int Ade_get_playback_framerate_by_split(int purpose, int &playfr, int &fullfr);

		int Ade_get_playback_specified_res_video_num(int purpose, int *res_num = NULL);

		int Ade_get_encode_analog_video_num(int *iencode_analog_res_num = NULL);
        
        int Ade_get_play_framerate(int purpose, int &play_fr, int &full_fr);
        
        int Ade_set_dec_playback_framerate(int purpose);
        
    private:
        DDSP_Operation_t m_DdspOperate;
    	APD_Operation_t m_ApdOperate;
    	
    	CMultiStreamSyncParser *m_pMSSP;
    	unsigned int m_MaxFrameLen;
    	
    	AVDecInfo_t m_AvDecInfo[_VDEC_MAX_VDP_NUM_][_AV_VDEC_MAX_CHN_];
        datetime_t m_CurDecTime[_VDEC_MAX_VDP_NUM_];
        int m_CurPlayFrameRate[_VDEC_MAX_VDP_NUM_];
       
        av_split_t m_CurSplitMode[_VDEC_MAX_VDP_NUM_];
        int m_CurPlayList[_VDEC_MAX_VDP_NUM_][_AV_SPLIT_MAX_CHN_];
        av_tvsystem_t m_tvsystem;     
        
   public:
        CAVPreviewDecCtrl *m_pAvPreviewDecoder[_VDEC_MAX_VDP_NUM_];
    	CDecDealSyncPlay *m_pDdsp;
    	int m_CurVdecChNum[_VDEC_MAX_VDP_NUM_];
        bool m_ResetFrameRate[_VDEC_MAX_VDP_NUM_];
        PlayState_t m_PlayState;
        PlayOperate_t m_CurPlayState;	
        PlayOperate_t m_PrePlayState;
        unsigned int m_VdecChmap[_VDEC_MAX_VDP_NUM_];
        AvDec_Operation_t m_AvDecOpe;
        
};



#endif/*__AVENCODER_H__*/

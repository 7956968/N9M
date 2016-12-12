/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __AVDEVICEMASTER_H__
#define __AVDEVICEMASTER_H__
#include "AvDevice.h"

#if defined(_AV_PRODUCT_CLASS_IPC_)
#include <Parameter.h>
#endif

/*margin array index*/
#define _AV_MARGIN_TOP_INDEX_       0
#define _AV_MARGIN_BOTTOM_INDEX_    1
#define _AV_MARGIN_LEFT_INDEX_      2
#define _AV_MARGIN_RIGHT_INDEX_     3


class CAvDeviceMaster:public virtual CAvDevice{
    public:
        CAvDeviceMaster();
        ~CAvDeviceMaster();

#if defined(_AV_HAVE_VIDEO_DECODER_)
    public:
        virtual int Ad_start_playback_dec(void *pPara) = 0;
        virtual int Ad_operate_dec(int purpose, int opcode, bool operate) = 0;
        virtual int Ad_control_vda_thread(vda_type_e vda_type, thread_control_e control_command)=0;
        virtual int Ad_operate_dec_for_switch_channel(int purpose) = 0;
        virtual bool Ad_get_dec_playtime(int purpose, void *playtime) = 0;
        virtual int Ad_dynamic_modify_vdec_framerate(int purpose) = 0;
        virtual int Ad_get_dec_image_size(int purpose, int phy_chn, int &width, int &height) = 0;
        virtual int Ad_stop_playback_dec() = 0;
        virtual void Ad_get_phy_layout_chn(int purpose, int phy_ch, int &layout_ch) = 0;
        virtual int Ad_start_talkback_adec() = 0;
        virtual int Ad_stop_talkback_adec() = 0;

#endif

#if defined(_AV_HAVE_VIDEO_INPUT_)
        virtual int Ad_set_videoinput_image(av_video_image_para_t av_video_image) = 0;
        virtual int Ad_set_video_attr( int chn, unsigned char u8IsMirror, unsigned char u8IsFlip )=0;
#endif

#if defined(_AV_SUPPORT_REMOTE_TALK_)
        virtual int Ad_Start_Remote_Talk( msgRequestAudioNetPara_t *pstuAudioNetPara ) = 0;
        virtual int Ad_Stop_Remote_Talk(const msgRequestAudioNetPara_t* pstuTalkParam) = 0;
#endif
#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
        virtual int Ad_Init_AudioDev_Salve() = 0;
#endif

#if defined(_AV_HAVE_VIDEO_OUTPUT_)
    public:
        /*video output device*/
        virtual int Ad_start_output(av_output_type_t output, int index, av_tvsystem_t tv_system, av_vag_hdmi_resolution_t resolution) = 0;
        virtual int Ad_stop_output(av_output_type_t output, int index) = 0;

        /*video layer, maybe there is on video layer*/
        virtual int Ad_start_video_layer(av_output_type_t output, int index){return 0;};
        virtual int Ad_stop_video_layer(av_output_type_t output, int index){return 0;};

        /*preview*/
        virtual int Ad_start_preview_output(av_output_type_t output, int index, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], int margin[4] = NULL) = 0;
        virtual int Ad_start_selected_preview_output(av_output_type_t output, int index, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], unsigned long long int chn_mask, bool brelate_decoder, int margin[4] = NULL) = 0;

        virtual int Ad_stop_preview_output(av_output_type_t output, int index) = 0;
        virtual int Ad_stop_preview_Dec(av_output_type_t output, int index) = 0;
        virtual int Ad_set_preview_dec_start_flag(bool isStartFlag) = 0;
        virtual int Ad_stop_selected_preview_output(av_output_type_t output, int index, unsigned long long int chn_mask, bool brelate_decoder) = 0;

        virtual int Ad_stop_preview_output_chn(av_output_type_t output, int index, int chn) = 0;
        virtual int Ad_start_preview_output_chn(av_output_type_t output, int index, int chn) = 0;
        virtual int Ad_set_preview_enable_switch(int chn_index, int enable) = 0;		
        virtual int Ad_get_preview_enable_switch(int chn_index) = 0;		

        /*playback*/
        virtual int Ad_start_playback_output(av_output_type_t output, int index, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], int margin[4] = NULL) = 0;
        virtual int Ad_stop_playback_output(av_output_type_t output, int index) = 0;

        /*switch output*/
        virtual int Ad_switch_output(av_output_type_t output, int index, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], bool bswitch_channel) = 0;
        /**/
        virtual int Ad_output_connect_source(av_output_type_t output, int index, int layout_chn, int phy_chn) = 0;
        virtual int Ad_output_disconnect_source(av_output_type_t output, int index, int layout_chn, int phy_chn) = 0;
        /**/
        virtual int Ad_output_swap_chn(av_output_type_t output, int index, int first_chn, int second_chn) = 0;

        virtual int Ad_output_swap_chn_reset( av_output_type_t output, int index ) = 0;
        
        virtual int Ad_set_pip_screen(av_pip_para_t av_pip_para) = 0;
        
        /*vo channel*/
        virtual int Ad_get_vo_chn(av_output_type_t output, int index, int chn_num){return -1;};

        virtual int Ad_set_vo_margin( av_output_type_t output, int index, unsigned short u16LeftSpace, unsigned short u16RightSpace, unsigned short u16TopSpace, unsigned short u16BottomSpace, bool bResetSlave = true) = 0;

        virtual int Ad_set_video_preview(unsigned int preview_enable, bool deal_right_now) = 0;
        virtual int Ad_set_video_preview_flag(int index, bool setornot) = 0;

#endif

#if defined(_AV_HAVE_AUDIO_OUTPUT_)
    public:
        virtual int Ad_start_ao(ao_type_e ao_type) = 0;
        virtual int Ad_stop_ao(ao_type_e ao_type) = 0;
       
#endif

    public:
        virtual int Ad_switch_audio_chn(int type, unsigned int audio_chn) = 0;
        virtual int Ad_set_audio_mute(bool mute_flag) = 0;
        virtual bool Ad_get_audio_mute() = 0;
        virtual int Ad_set_audio_volume(unsigned char volume) = 0;

#if defined(_AV_HAVE_AUDIO_INPUT_) && defined(_AV_HAVE_AUDIO_OUTPUT_) && (defined(_AV_PRODUCT_CLASS_DVR_) || defined(_AV_PRODUCT_CLASS_HDVR_))
    public:
        virtual int Ad_start_preview_audio() = 0;
        virtual int Ad_preview_audio_ctrl(bool start_or_stop) = 0;
#if defined(_AV_PLATFORM_HI3515_)
		virtual int Ad_bind_preview_ao(int audio_chn) = 0;
		virtual int Ad_unbind_preview_ao() = 0;
#endif
        //virtual int Ad_obtain_preview_audio_state() = 0;
#endif

#if defined(_AV_TEST_MAINBOARD_)
    public:
        virtual int Ad_test_audio_IO() = 0;
#endif

    public:
        virtual int Ad_clear_init_logo() = 0;

#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
    public:
        virtual int Ad_UpdateDevOnlineState( unsigned char szState[SUPPORT_MAX_VIDEO_CHANNEL_NUM] ) = 0;
        virtual int Ad_UpdateDevMap( char szChannelMap[SUPPORT_MAX_VIDEO_CHANNEL_NUM] ) = 0;
        virtual int Ad_RestartDecoder() = 0;
        virtual int Ad_SetSystemUpgradeState( int state ) = 0;
        virtual int Ad_ObtainChannelMap( char szChannelMap[SUPPORT_MAX_VIDEO_CHANNEL_NUM]) = 0;
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_)
    public:
        virtual int Ad_SetCameraAttribute( unsigned int u32Mask, const paramCameraAttribute_t* pstuParam ) = 0;
        virtual int Ad_SetCameraColor( const paramVideoImage_t* pstuParam ) = 0;
        virtual int Ad_SetEnvironmentLuma( unsigned int u32Luma ) = 0;
        virtual int Ad_GetIspControl( unsigned char& u8OpenIrCut, unsigned char& u8CameraLampLevel, unsigned char& u8IrisLevel, unsigned char& u8IspFrameRate, unsigned int& u32PwmHz ) = 0;
        virtual int Ad_SetVideoColorGrayMode( unsigned char u8ColorMode ) = 0;
        virtual int Ad_SetAEAttrStep( unsigned char u8StepMode) = 0;
        virtual int Ad_SetDayNightCutLuma( unsigned int u32Mask, unsigned int u32Day2NightValue, unsigned int u32Night2DayValue ) = 0;
        virtual int Ad_set_macin_volum( unsigned char u8Volume ) = 0;
#endif
    public://! TTS related
#if !defined(_AV_PRODUCT_CLASS_IPC_)

#if defined(_AV_HAVE_TTS_)
        virtual int Ad_start_tts(msgTTSPara_t * pstu_tts_param ) = 0;
        virtual int Ad_stop_tts() = 0;
        virtual int Ad_query_tts() = 0;
		virtual int Ad_init_tts() = 0;
        virtual bool Ad_SET_TTS_LOOP_STATE(bool state) = 0;
#endif		
        virtual int Ad_get_report_station_result() = 0;
        virtual int Ad_add_report_station_task( msgReportStation_t* pstuStationParam) = 0;
        virtual int Ad_start_report_station() = 0;
        virtual int Ad_stop_report_station() = 0;		
        virtual int Ad_loop_report_station(bool bloop) = 0;
        virtual int Ad_set_preview_state(bool state) = 0;
        virtual int Ad_set_preview_start(bool start_or_stop) = 0;
 #endif
        
};


#endif/*__AVDEVICEMASTER_H__*/


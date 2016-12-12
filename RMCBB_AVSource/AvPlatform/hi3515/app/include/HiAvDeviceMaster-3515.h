/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __HIAVDEVICEMASTER3515_H__
#define __HIAVDEVICEMASTER3515_H__
#include "HiAvDevice-3515.h"
#include "AvDeviceMaster.h"
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
#include "HiVo-3515.h"
#endif
#if defined(_AV_HAVE_VIDEO_DECODER_)
#include "HiAvDec-3515.h"
#include "StreamBuffer.h"

#endif
#if defined(_AV_HAVE_AUDIO_OUTPUT_)
#include "HiAo-3515.h"
#endif 
#if defined(_AV_HAVE_VIDEO_ENCODER_)
#include "AvSnapTask.h"
#endif
#ifdef _AV_SUPPORT_REMOTE_TALK_
#include "AvRemoteTalk.h"
#endif
#if !defined(_AV_PRODUCT_CLASS_IPC_)
//!for TTS
#if defined(_AV_HAVE_TTS_)
#include "AvTTS.h"
#endif
#include "AvReportStation.h"
#endif
#include "hi_common_extend.h"

/*video output channel layout*/
typedef struct _vo_chn_state_info_{
    av_split_t split_mode;
    int vo_type;/*0:preview; 1:playback*/

    /*index is the channel number of vo; value is the phyical channel number*/
    int chn_layout[_AV_SPLIT_MAX_CHN_];
    bool set_vi_same_as_source[_AV_SPLIT_MAX_CHN_];
    int margin[4];

    av_split_t cur_split_mode;
    int cur_chn_layout[_AV_SPLIT_MAX_CHN_];

}vo_chn_state_info_t;

typedef struct _talkback_cntrl_param_
{
    bool thread_run;
    bool thread_exit;
    int adec_chn;
    HANDLE Stream_buffer_handle;
} talckback_cntrl_param_t;

class CHiAvDeviceMaster:public CHiAvDevice, public CAvDeviceMaster{
    public:
       CHiAvDeviceMaster();
       ~CHiAvDeviceMaster();

    public:
        int Ad_init();
        int Ad_clear_init_logo();

#if defined(_AV_HAVE_VIDEO_INPUT_)
    public:
        int Ad_start_vi(av_tvsystem_t tv_system, vi_config_set_t *pSource = NULL, vi_config_set_t *pPrimary_attr = NULL, vi_config_set_t *pMinor_attr = NULL);
        int Ad_start_selected_vi(av_tvsystem_t tv_system, unsigned long long int chn_mask, vi_config_set_t *pSource = NULL, vi_config_set_t *pPrimary_attr = NULL, vi_config_set_t *pMinor_attr = NULL);

        int Ad_stop_vi();
        int Ad_stop_selected_vi(unsigned long long int chn_mask);

        int Ad_vi_cover(int phy_chn_num, int x, int y, int width, int height, int cover_index);
        int Ad_vi_uncover(int phy_chn_num, int cover_index);
        int Ad_vi_insert_picture(unsigned int video_loss_bmp);

        int Ad_start_vda(vda_type_e vda_type, av_tvsystem_t tv_system, int phy_video_chn, const vda_chn_param_t* pVda_chn_param);
        int Ad_start_selected_vda(vda_type_e vda_type, av_tvsystem_t tv_system, int phy_video_chn, const vda_chn_param_t* pVda_chn_param);

        int Ad_stop_vda(vda_type_e vda_type, int phy_video_chn);
        int Ad_stop_selected_vda(vda_type_e vda_type, int phy_video_chn);
        int Ad_get_vda_result(vda_type_e vda_type, unsigned int *pVda_result, int phy_video_chn=-1, unsigned char *pVda_region=NULL);
        //!Added
        int Ad_set_video_attr( int chn, unsigned char u8IsMirror, unsigned char u8IsFlip );

#if !defined(_AV_PRODUCT_CLASS_IPC_)
        int Ad_set_video_format( int chn, unsigned int chn_mask, unsigned char av_video_format);
        int Ad_is_ahd_video_format_changed();
#endif

#endif

#if defined(_AV_HAVE_VIDEO_DECODER_)
        int Ad_start_playback_dec(void *pPara);
        int Ad_stop_playback_dec();
        int Ad_operate_dec(int purpose, int opcode, bool operate);
        int Ad_control_vda_thread(vda_type_e vda_type, thread_control_e control_command);
        int Ad_operate_dec_for_switch_channel(int purpose);
        bool Ad_get_dec_playtime(int purpose, void *playtime);
        int Ad_get_dec_image_size(int purpose, int phy_chn, int &width, int &height);
        int Ad_dynamic_modify_vdec_framerate(int purpose);

        int Ad_dec_connect_vo(int purpose, int phy_chn);
        int Ad_dec_disconnect_vo(int purpose, int phy_chn);
        void Ad_get_phy_layout_chn(int purpose, int phy_ch, int &layout_ch);
        int Ad_start_talkback_adec();
        int Ad_stop_talkback_adec();
        int Ad_talkback_thread_body();

        int Ad_start_preview_dec(av_output_type_t output, int index, av_split_t split_mode, int chn_layout [ _AV_SPLIT_MAX_CHN_ ]);
        int Ad_stop_preview_dec(av_output_type_t output, int index);
#endif

#if defined(_AV_HAVE_VIDEO_INPUT_)
        int Ad_set_videoinput_image(av_video_image_para_t av_video_image);
#endif

#if defined(_AV_SUPPORT_REMOTE_TALK_)
    public:
        int Ad_Start_Remote_Talk( msgRequestAudioNetPara_t *pstuAudioNetPara );
        int Ad_Stop_Remote_Talk(const msgRequestAudioNetPara_t* pstuTalkParam);
#endif
#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
    public:
        int Ad_Init_AudioDev_Salve();
#endif
    int Ad_Set_Chn_state(int flag);

#if defined(_AV_HAVE_VIDEO_ENCODER_)
    public:
        int Ad_get_encoder_used_resource(int *pCifCount);
        //for playback
        bool Ad_check_encoder_resolution_exist( av_resolution_t eResolution, int *pResolutionNum = NULL);
        int Ad_start_snap();
        int Ad_stop_snap();
        int Ad_Get_Chn_state();

        int Ad_modify_snap_param( av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para );
        int Ad_cmd_snap_picutres( /*const MsgSnapPicture_t* pstuSnapParam*/ );
        int Ad_cmd_signal_snap_picutres( AvSnapPara_t &pstuSignalSnapParam);
        int Ad_get_snap_serial_number( msgMachineStatistics_t* pstuMachineStatistics);
        int Ad_get_snap_result(msgSnapResultBoardcast *snapresultitem);
        int Ad_set_osd_info(av_video_encoder_para_t  *pVideo_para, int chn, rm_uint16_t  usOverlayBitmap);
 #ifdef _AV_PRODUCT_CLASS_IPC_
        int Ad_snap_clear_all_pictures();
 #endif
#endif

#if defined(_AV_HISILICON_VPSS_) && defined(_AV_HAVE_VIDEO_OUTPUT_) 
        int Ad_get_vpss_purpose(av_output_type_t output, int index, int type, int video_phy_chn_num, vpss_purpose_t &vpss_purpose, bool digitalvpss/* = false*/);
        int Ad_show_custormer_logo(int video_chn, bool show_or_hide);
        int Ad_show_access_logo(int vo_dev, int video_chn, bool show_or_hide);
        int Ad_control_vpss_vo(av_output_type_t output, int index, vpss_purpose_t vpss_purpose, int phy_video_chn_num, int vo_chn, int control_flag);
#endif
		
    /*output, preview output, playback output, spot output*/
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
    public:
    
		//int Ad_show_custormer_logo(int video_chn, bool show_or_hide);
		//int Ad_show_access_logo(int vo_dev, int video_chn, bool show_or_hide);
		int Ad_control_vpss_vo(av_output_type_t output, int index, vpss_purpose_t vpss_purpose, int phy_video_chn_num, int vo_chn, int control_flag);
		int Ad_control_3515_vo(av_output_type_t output, int index, vpss_purpose_t vpss_purpose, int phy_video_chn_num, int vo_chn, int control_flag);
		int Ae_bind_vdec_to_vo(int vdecchn, int vodev,int vochn, bool bind_flag);
		int Ae_bind_vi_to_vo(int videv,int vichn,int vodev, int vochn, bool bind_flag);
        int Ad_zoomin_screen(av_zoomin_para_t av_zoomin_para);
        int Ad_set_pip_screen(av_pip_para_t av_pip_para);
        /*output device*/
        int Ad_start_output(av_output_type_t output, int index, av_tvsystem_t tv_system, av_vag_hdmi_resolution_t resolution);
        int Ad_stop_output(av_output_type_t output, int index);
        /*video layer*/
        int Ad_start_video_layer(av_output_type_t output, int index);
        int Ad_stop_video_layer(av_output_type_t output, int index);
        /*preview*/
        int Ad_start_preview_output(av_output_type_t output, int index, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], int margin[4] = NULL);
        int Ad_start_selected_preview_output(av_output_type_t output, int index, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], unsigned long long int chn_mask, bool brelate_decoder, int margin[4] = NULL);

        int Ad_stop_preview_output(av_output_type_t output, int index);
        int Ad_stop_preview_Dec(av_output_type_t output, int index);
        int Ad_set_preview_dec_start_flag(bool isStartFlag);
        int Ad_stop_selected_preview_output(av_output_type_t output, int index, unsigned long long int chn_mask, bool brelate_decoder);
        int Ad_stop_preview_output_chn(av_output_type_t output, int index, int chn);
        int Ad_set_preview_enable_switch(int chn_index, int enable);		
        int Ad_get_preview_enable_switch(int chn_index);		

        int Ad_start_preview_output_chn(av_output_type_t output, int index, int chn);
        /*playback*/
        int Ad_start_playback_output(av_output_type_t output, int index, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], int margin[4] = NULL);
        int Ad_stop_playback_output(av_output_type_t output, int index);
        /**/
        int Ad_switch_output(av_output_type_t output, int index, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], bool bswitch_channel);
        /**/
        int Ad_output_drag_chn(av_output_type_t output, int index, int chn_layout[_AV_SPLIT_MAX_CHN_]);
        int Ad_output_swap_chn(av_output_type_t output, int index, int first_chn, int second_chn);
        int Ad_output_swap_chn_reset( av_output_type_t output, int index );
        /**/
        int Ad_output_connect_source(av_output_type_t output, int index, int layout_chn, int phy_chn);
        int Ad_output_disconnect_source(av_output_type_t output, int index, int layout_chn, int phy_chn);
        /**/
        int Ad_get_vo_chn(av_output_type_t output, int index, int chn_num);
        /*video preview*/
        int Ad_set_video_preview(unsigned int preview_enable, bool deal_right_now);
        int Ad_set_video_preview_flag(int index, bool setornot);
        

    private:
        int Ad_start_output_V1(av_output_type_t output, int index, av_tvsystem_t tv_system, av_vag_hdmi_resolution_t resolution);
        int Ad_stop_output_V1(av_output_type_t output, int index);
        int Ad_start_video_layer_V1(av_output_type_t output, int index, av_tvsystem_t tv_system, av_vag_hdmi_resolution_t resolution);
        int Ad_stop_video_layer_V1(av_output_type_t output, int index);
	 HI_S32 Hi3515_ViBindVo(HI_S32 s32ViChnTotal, VO_DEV VoDev);

        int Ad_start_video_output_V1(av_output_type_t output, int index, int type, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], int margin[4] = NULL);
        int Ad_start_selected_video_output_V1(av_output_type_t output, int index, int type, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], unsigned long long int chn_mask, int margin[4] = NULL);


        int Ad_stop_video_output_V1(av_output_type_t output, int index, int type, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_]);
        int Ad_stop_selected_video_output_V1(av_output_type_t output, int index, int type, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], unsigned long long int chn_mask);

        int Ad_switch_output_V1(av_output_type_t output, int index, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_]);
        int Ad_output_drag_chn_V1(av_output_type_t output, int index, int chn_layout[_AV_SPLIT_MAX_CHN_]);
        int Ad_output_drag_chn_V2(av_output_type_t output, int index, int chn_layout[_AV_SPLIT_MAX_CHN_]);

        int Ad_start_video_output(av_output_type_t output, int index, int type, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], int margin[4] = NULL);
        int Ad_start_selected_video_output(av_output_type_t output, int index, int type, av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_], unsigned long long int chn_mask, bool brelate_decoder, int margin[4] = NULL);

        int Ad_stop_video_output(av_output_type_t output, int index, int type);
        int Ad_stop_selected_video_output(av_output_type_t output, int index, unsigned long long int chn_mask, int type, bool brelate_decoder);

        int Ad_create_vo_layout(VO_DEV vo_dev, av_rect_t *pVideo_rect, av_split_t split_mode);
        int Ad_create_selected_vo_layout(VO_DEV vo_dev, av_rect_t *pVideo_rect, av_split_t split_mode, int vo_chn, int vo_index);

        int Ad_destory_vo_layout(VO_DEV vo_dev, av_split_t split_mode); 
        int Ad_reset_vo_layout(av_output_type_t output, int index, av_rect_t *pVideo_rect, av_split_t split_mode);
        int Ad_calculate_video_rect(int video_layer_width, int video_layer_height, int margin[4], av_rect_t *pVideo_rect);
        int Ad_calculate_video_rect_V2(int video_layer_width, int video_layer_height, const int margin[4], av_rect_t *pVideo_rect,av_vag_hdmi_resolution_t resolution);

        int Ad_output_source_control(av_output_type_t output, int index, int layout_chn, int phy_chn, int control_flag);
        int Ad_output_source_control_V1(av_output_type_t output, int index, int layout_chn, int phy_chn, int control_flag);
        int calculate_margin_for_limit_resource_machine(av_vag_hdmi_resolution_t resolution, int margin[4]);
        /*PIP*/
        int Ad_set_pip(bool enable, int video_phy_chnnum, av_split_t split_mode, av_rect_t av_main_pip_area, av_rect_t av_slave_pip_area);
        int Ad_start_pip(av_output_type_t output, int index, av_split_t split_mode, int video_phy_chnnum, av_rect_t pip_area);
        int Ad_stop_pip(av_output_type_t output, int index, int video_phy_chnnum);
        /*zoom*/
        int Ad_create_digital_zoom_chn(av_output_type_t output, int index, int video_phy_chnnum, int digital_vo_chn);
        int Ad_destory_digital_zoom_chn(av_output_type_t output, int index, int video_phy_chnnum, int digital_vo_chn);
        int Ad_digital_zoom(av_output_type_t output, int index, bool enable, int video_phy_chnnum, av_rect_t av_zoom_area);
        int Ad_set_vo_margin( av_output_type_t output, int index, unsigned short u16LeftSpace, unsigned short u16RightSpace, unsigned short u16TopSpace, unsigned short u16BottomSpace, bool bResetSlave = true);
        int Ad_stop_one_chn_output(av_output_type_t output, int index, int type, int chn);
        int Ad_start_one_chn_output(av_output_type_t output, int index, int type, int chn);

    private:
        void Ad_set_tv_system(av_output_type_t output, int index, av_tvsystem_t tv_system);
        void Ad_unset_tv_system(av_output_type_t output, int index);
        av_tvsystem_t Ad_get_tv_system(av_output_type_t output, int index);
        void Ad_set_resolution(av_output_type_t output, int index, av_vag_hdmi_resolution_t resolution);
        av_vag_hdmi_resolution_t Ad_get_resolution(av_output_type_t output, int index);
        void Ad_unset_resolution(av_output_type_t output, int index);
#if defined(_AV_HAVE_VIDEO_DECODER_)
        void Ad_init_dec();
        bool Ad_check_reset_decoder(int purpose, int chn[_AV_SPLIT_MAX_CHN_], int chnum);
        bool Ad_switch_dec_auido_chn(int purpose, int chn);
#endif

    private:
        std::map<av_output_type_t, std::map<int, av_tvsystem_t> > m_output_tv_system;
        std::map<av_output_type_t, std::map<int, av_vag_hdmi_resolution_t> > m_output_resolution;
        std::map<av_output_type_t, std::map<int, vo_chn_state_info_t> > m_vo_chn_state;
        bool m_pip_create_status;
        int m_pip_main_video_phy_chn;
        int m_pip_sub_video_phy_chn;
        std::bitset<_AV_SPLIT_MAX_CHN_> m_video_preview_switch;
#if defined(_AV_HISILICON_VPSS_)
        bool m_digital_zoom_status;
#endif
	char m_preview_enable[8];

    private:
        CHiVo *m_pHi_vo;
#endif

#if defined(_AV_HAVE_VIDEO_DECODER_)
//weather machine support talk back, depended machine type
    private:
        CHiAvDec *m_pHi_AvDec;
        
        talckback_cntrl_param_t m_talkback_cntrl;
        int m_av_dec_chn_map[_AV_DECODER_MODE_NUM_][_AV_SPLIT_MAX_CHN_];
#endif

#if defined(_AV_HAVE_AUDIO_INPUT_)
    public:
        int Ad_start_ai(ai_type_e ai_type);
        int Ad_stop_ai(ai_type_e ai_type);
#endif

#if defined(_AV_HAVE_AUDIO_OUTPUT_)
    public:
        int Ad_start_ao(ao_type_e ao_type);
        int Ad_stop_ao(ao_type_e ao_type);

    private:
        CHiAo *m_pHi_ao; 
#endif

#if defined(_AV_TEST_MAINBOARD_)
    public:
        int Ad_test_audio_IO();
    private:
        int Ad_start_talkback_audio();
#endif

    public:
        int Ad_switch_audio_chn(int type, unsigned int audio_chn);
        int Ad_set_audio_mute(bool mute_flag);
        bool Ad_get_audio_mute();
        int Ad_set_audio_volume(unsigned char volume);
        audio_output_state_t m_audio_output_state;

#if defined(_AV_HAVE_AUDIO_INPUT_) && defined(_AV_HAVE_AUDIO_OUTPUT_)
    public:
#if defined(_AV_PLATFORM_HI3515_)
    	int Ad_bind_preview_ao(int audio_chn);
		int Ad_unbind_preview_ao();
#endif		
        int Ad_start_preview_audio();
        void Ad_preview_audio_thread_body();
        int Ad_preview_audio_ctrl(bool start_or_stop);

        //inline int Ad_obtain_preview_audio_state(){return m_bstart;}
    private:
        preview_audio_ctrl_t m_preview_audio_ctrl;
        bool m_bstart;
        bool m_bInterupt;
        bool m_bPreviewAoStart;
        bool m_audio_preview_running;
#endif

#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
    public:
        int Ad_UpdateDevOnlineState( unsigned char szState[SUPPORT_MAX_VIDEO_CHANNEL_NUM] );
        int Ad_UpdateDevMap( char szChannelMap[SUPPORT_MAX_VIDEO_CHANNEL_NUM] );
        // for nvr, changed vga resolution, decoder is delay
        int Ad_RestartDecoder();

        int Ad_SetSystemUpgradeState( int state );
        int Ad_ObtainChannelMap( char szChannelMap[SUPPORT_MAX_VIDEO_CHANNEL_NUM]);
        
    private:
        unsigned char m_u8DevOlState[SUPPORT_MAX_VIDEO_CHANNEL_NUM]; // 1:online else offline
        char m_c8ChannelMap[SUPPORT_MAX_VIDEO_CHANNEL_NUM]; //-1:invalid else is channel 
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_)
    public:
        int Ad_SetCameraAttribute( unsigned int u32Mask, const paramCameraAttribute_t* pstuParam );
        int Ad_SetCameraColor( const paramVideoImage_t* pstuParam );
        int Ad_SetEnvironmentLuma( unsigned int u32Luma );
        int Ad_GetIspControl( unsigned char& u8OpenIrCut, unsigned char& u8CameraLampLevel, unsigned char& u8IrisLevel, unsigned char& u8IspFrameRate, unsigned int& u32PwmHz );
        int Ad_SetVideoColorGrayMode( unsigned char u8ColorMode );
        int Ad_SetAEAttrStep( unsigned char u8StepMode );
        int Ad_SetDayNightCutLuma( unsigned int u32Mask, unsigned int u32Day2NightValue, unsigned int u32Night2DayValue );
        int Ad_set_macin_volum( unsigned char u8Volume );
#endif

#if defined(_AV_HAVE_VIDEO_ENCODER_)
    private:
        CAvSnapTask* m_pSnapTask;
#endif

#ifdef _AV_SUPPORT_REMOTE_TALK_
    private:
        CAvRemoteTalk* m_pRemoteTalk;
#endif
    private:
        int Ad_set_audio_output_to_zero(){return m_audio_output_state.adec_chn = 0;}
//!Added for TTS
    public:
#if !defined(_AV_PRODUCT_CLASS_IPC_)

#if defined(_AV_HAVE_TTS_)
        int Ad_start_tts(msgTTSPara_t* tts_param);
        int Ad_stop_tts();
        int Ad_query_tts();
		int Ad_init_tts();
        bool Ad_SET_TTS_LOOP_STATE(bool state);
#endif
        int Ad_get_report_station_result();
        int Ad_add_report_station_task( msgReportStation_t* pstuStationParam);
        int Ad_start_report_station();
        int Ad_stop_report_station();       
        int Ad_loop_report_station(bool bloop);
       
#endif
        int Ad_set_preview_start(bool start_or_stop);
        int Ad_set_preview_state(bool state);

    private:
 #if !defined(_AV_PRODUCT_CLASS_IPC_)
 #if defined(_AV_HAVE_TTS_)
        CAvTTS * m_pAv_tts;
 #endif
        CAvReportStation*m_pAvStation;
#endif
};


#endif/*__HIAVDEVICEMASTER_H__*/


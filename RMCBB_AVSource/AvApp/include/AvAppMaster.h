/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __AVAPPMASTER_H__
#define __AVAPPMASTER_H__
#include "AvApp.h"
#include "MessageClient.h"
#include "AvDeviceMaster.h"
#include "Message.h"
#include "AvCommonFunc.h"
#include "AvDevCommType.h"
//#include "SnapManage.h"
#include "SystemConfig.h"/*json config file*/
#include <bitset>
//0703 黑匣子GPS
#include "FileHeader.h"

#include "gb2312ToUft8.h"

using namespace Common;


class CAvAppMaster:public CAvApp{
    public:
        CAvAppMaster();
        virtual ~CAvAppMaster();

    public:
        int Aa_load_system_config(int argc = 0, char *argv[] = NULL);
        int Aa_init();

        int Aa_message_thread();
        int Aa_construct_communication_component();

    private:
        int Aa_parse_vi_info_from_system_config();
        int Aa_parse_stream_buffer_info_from_system_config();
        int Aa_load_special_info();

#if defined(_AV_HAVE_VIDEO_OUTPUT_)
    public:
        /*video output device*/
        int Aa_start_output(av_output_type_t output, int index, av_tvsystem_t tv_system, av_vag_hdmi_resolution_t resolution);
        int Aa_stop_output(av_output_type_t output, int index);
#endif

    private:
        void Aa_message_handle_SetDebugLevel(const char *msg, int length);
        void Aa_message_handle_GETSTATUS(const char *msg, int length);
        void Aa_message_handle_SYSTEMPARA(const char *msg, int length);
        void Aa_message_handle_REBOOT(const char *msg, int length);
        void Aa_message_handle_CHANGE_SYSTEM_TIME( const char* msg, int length );
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
        void Aa_message_handle_ZOOMINSCREEN(const char *msg, int length);
        void Aa_message_handle_PIPSCREEN(const char *msg, int length);
        void Aa_message_handle_SCREEN_DISPLAY_ADJUST(const char *msg, int length);
        void Aa_message_handle_SCREENMOSIAC(const char *msg, int length);        
        void Aa_message_handle_START_SWAPCHN( const char *msg, int length );
        void Aa_message_handle_STOP_SWAPCHN( const char *msg, int length );
        void Aa_message_handle_SWAPCHN( const char *msg, int length );

        void Aa_message_handle_MARGIN(const char *msg, int length);

        av_split_t Aa_get_split_mode(int iMode);

        int Ad_get_split_iMode(av_split_t slipmode);

        av_vag_hdmi_resolution_t Aa_get_vga_hdmi_resolution(int resolution);
        int Aa_switch_output(msgScreenMosaic_t *pScreen_mosaic, bool bswitch_channel = false);

        int Aa_GotoPreviewOutput(msgScreenMosaic_t* pScreen_mosaic);
        int Aa_GotoSelectedPreviewOutput(msgScreenMosaic_t* pScreen_mosaic, unsigned long long int chn_mask, bool brelate_decoder);

        int Aa_Change_VoResolution_TvSystem( unsigned char u8TvSys, unsigned char u8VoRes );


/*spot**********************************************/
    private:/**/
#define _SPOT_OUTPUT_CONTROL_output_        0
#define _SPOT_OUTPUT_CONTROL_stop_          1
        void Aa_message_handle_SPOTSEQUENCE(const char *msg, int length);
        int Aa_spot_output_control(int control, msgScreenMosaic_t *pScreen_mosaic);
        int Aa_start_spot(av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_]);
        int Aa_stop_spot();

    private:
#define _SPOT_OUTPUT_STATE_uninit_   0
#define _SPOT_OUTPUT_STATE_running_  1
        int m_spot_output_state;
        msgScreenMosaic_t *m_pSpot_output_mosaic;
        av_pip_para_t m_av_pip_para;
#endif

#if defined(_AV_HAVE_VIDEO_DECODER_)
        void Aa_message_handle_PLAYBACK_START(const char *msg, int length);
        void Aa_message_handle_PLAYBACK_STOP(const char *msg, int length);
        void Aa_message_handle_PLAYBACK_OPERATE(const char *msg, int length);
        void Aa_message_handle_DECODER_STREAMSTART(const char *msg, int length);
        void Aa_message_handle_DECODER_STREAMSTOP(const char *msg, int length);
        void Aa_message_handle_DECODER_STREAM_OPERATE(const char * msg, int length);
        void Aa_message_handle_PLAYBACK_SWITCH_CHANNELS(const char *msg, int length);
        void Aa_message_handle_DECODER_SWITCH_CHANNELS(const char *msg, int length);
        void Aa_check_video_image_res(int purpose);

        //added by 2015.3.17
        void Aa_message_handle_START_PLAYBACK_AUDIO(const char* msg, int length );
        void Aa_message_handle_STOP_PLAYBACK_AUDIO(const char* msg, int length );
    private:
        int m_play_source_id;
	 int m_play_streamtype_isMirror;	//用于判断回放的码流是否为镜像码流
	 int m_playback_zoom_flag;	//用于判断回放的时候是否单通道放大
#endif
        
        void Aa_check_status();

        void Aa_get_snap_result();
        void Aa_check_playback_status();
#if !defined(_AV_PLATFORM_HI3518EV200_)
        void Aa_send_av_alarm();
#endif
#if defined(_AV_PRODUCT_CLASS_IPC_)        
        void Aa_IPC_check_adjust();
#if defined(_AV_PLATFORM_HI3518C_)
        int Aa_set_isp_framerate(av_tvsystem_t norm, int main_stream_framerate);
#endif
        IspOutputMode_e Aa_calculate_isp_out_put_mode(unsigned char resolution, unsigned char framerate);
        /*get framerate from IOM(isp output framerate)*/
        unsigned short Aa_convert_framerate_from_IOM(IspOutputMode_e isp_output_mode);

#if defined(_AV_PLATFORM_HI3516A_)
        /**< resolution: 6-720P 7-1080P 8-3MP(2048*1536) 9-5MP(2592*1920) */
        bool Aa_is_reset_isp(unsigned char resolution, unsigned char framerate, IspOutputMode_e &isp_output_mode);
        int Aa_reset_isp(IspOutputMode_e isp_output_mode);
#endif
#endif
        
#if defined(_AV_HAVE_VIDEO_ENCODER_)
    private:
        void Aa_get_snap_result_message();//snap
        void Aa_check_net_stream_level();
        int Aa_get_net_stream_level(unsigned int *chnmask,unsigned char chnvalue[SUPPORT_MAX_VIDEO_CHANNEL_NUM]);
        void Aa_set_osd_info(const AvSnapPara_t &pSignalSnapParam );

#if !defined(_AV_PRODUCT_CLASS_DVR_REI_)
        void Aa_message_handle_MAINSTREAMPARA(const char *msg, int length);
#else
        void Aa_message_handle_MAINSTREAMPARA_REI(const char *msg, int length);
#endif		
        void Aa_message_handle_SUBSTREAMPARA(const char *msg, int length);
        void Aa_message_handle_VIDEOOSD(const char *msg, int length);
#if defined(_AV_PRODUCT_CLASS_IPC_)
        void Aa_message_handle_BusInfo(const char *msg, int length);
#else
        void Aa_message_handle_RegisterInfo(const char *msg, int length);
#endif

#if !defined(_AV_PRODUCT_CLASS_IPC_)
        //0628
        void Aa_message_handle_GetStationName(const char *msg, int length);
        void Aa_message_handle_Enable_Common_OSD(const char *msg, int length);
#endif

#if (defined(_AV_PLATFORM_HI3520D_V300_) || defined(_AV_PLATFORM_HI3515_))
		void Aa_message_handle_EXTEND_OSD(const char *msg, int length);
#endif

        void Aa_message_handle_ViewOsdInfo(const char *msg, int length);
#if !defined(_AV_PRODUCT_CLASS_IPC_)
        void Aa_message_handle_SpeedSource(const char *msg, int length);
#endif
        void Aa_message_handle_TIMESETTING(const char *msg, int length);
        void Aa_message_handle_SNAPSHOT(const char *msg, int length);
        void Aa_message_handle_auto_adapt_encoder( const char* msg, int length );
        void Aa_message_handle_request_iframe( const char* msg, int length );
        void Aa_message_handle_GET_ENCODER_SOURCE(const char* msg, int length );
        void Aa_message_handle_MOBSTREAMPARA(const char *msg, int length);
#if defined(_AV_PRODUCT_CLASS_IPC_)
        void Aa_message_handle_update_gpsInfo(const char* msg, int length);
        void Aa_message_handle_update_speedInfo(const char* msg, int length);
#else
        void Aa_message_handle_update_gps(const char* msg, int length );
        void Gps_time_to_local_time(msgTime_t gps_time_m, msgTime_t *local_time);
		#if !defined(_AV_PRODUCT_CLASS_DVR_REI_)
		void Aa_message_handle_update_speed_info(const char* msg, int length );
		#endif
        void Aa_message_handle_update_pulse_speed(const char* msg, int length );
#endif
        void Aa_message_handle_SIGNALSNAP(const char* msg, int length );
        void Aa_message_handle_SNAPSERIALNUMBER(const char* msg, int length );
        int Aa_modify_encoder_para(av_video_stream_type_t stream_type, msgVideoEncodeSetting_t *p_encode_para);
        //0703
        void Aa_message_handle_TAX2_GPS_SPEED(const char* msg, int length );
#if !defined(_AV_PRODUCT_CLASS_IPC_)&&!defined(_AV_PRODUCT_CLASS_DVR_REI_)
		//Beijing customer
        void Aa_message_handle_Beijing_bus_station(const char* msg, int length );
		void Aa_message_handle_APC_Number(const char* msg, int length );
#endif
#ifdef _AV_PRODUCT_CLASS_IPC_
        void Aa_message_handle_NetSnap( const char* msg, int length );
        void Ae_message_handle_IPCWorkMode( const char* msg, int length );
        void Aa_message_handle_update_video_osd(const char* msg, int length) ;
        void Ae_message_handle_GetIpAddress( const char* msg, int length );
#endif
        
    private:
        int Aa_calculate_bitrate( unsigned char u8Resolution, unsigned char u8Quality, unsigned char u8FrameRate, int n32ParaBitRate=-1 );
        int Aa_get_local_encoder_chn_mask(av_video_stream_type_t video_stream_type, unsigned long long int *pulllocal_encoder_chn_mask = NULL);
        int Aa_start_selected_streamtype_encoders(unsigned long long int chn_mask, unsigned char streamtype_mask = 0, bool bcheck_flag = true);
        int Aa_start_encoders_for_parameter_change(unsigned long long int start_encoder_chn_mask[_AST_UNKNOWN_], bool bcheck_flag = true);
        int Aa_set_encoders_state(unsigned long long int chnmask, int max_vi_chn_num, bool start_or_stop, av_video_stream_type_t video_stream_type = _AST_UNKNOWN_, av_audio_stream_type_t audio_stream_type = _AAST_UNKNOWN_);
        int Aa_stop_selected_streamtype_encoders(unsigned long long int chn_mask, unsigned int streamtype_mask = 0, bool bcheck_flag = true);
        int Aa_stop_encoders_for_parameters_change(unsigned long long int stop_encoder_chn_mask[_AST_UNKNOWN_], bool bcheck_flag = true);

        int Aa_set_encoder_param(av_video_stream_type_t video_stream_type, int pyh_chn_num, int n32BitRate=-1, unsigned char u8FrameRate=0xff, unsigned char u8Res=0xff, bool bEnable=true );

        int Aa_adapt_encoder_param( unsigned int u32StreamMask, unsigned int u32ChnMask, int n32BitRate=-1, unsigned char u8FrameRate=0xff, unsigned char u8Res=0xff, unsigned char u8SrcFrameRate = 0xff );
        int Aa_get_encoder_para(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, int phy_audio_chn_num, av_audio_encoder_para_t *pAudio_para);
        int Aa_check_and_reset_encoder_param(paramVideoEncode_t* pstuEncoderParam);
        int Aa_init_video_audio_para(av_video_encoder_para_t *pVideo_para, av_audio_encoder_para_t *pAudio_para, unsigned long long int chn_mask, int max_vi_chn_num, av_video_stream_type_t video_stream_type = _AST_UNKNOWN_);

        void Aa_message_handle_start_encoder( const char* msg, int length );
        void Aa_message_handle_stop_encoder( const char* msg, int length );
        void Aa_message_handle_get_net_stream_level(const char* msg, int length );
        void Aa_message_handle_record_mode(const char* msg, int length );
        unsigned char Aa_get_all_streamtype_mask();
#if  !defined(_AV_PRODUCT_CLASS_IPC_)
        bool GetLanguageSysConfigID(int language, int &languageId);
        bool LoadLanguage(Json::Value m_SystemConfig, int languageID);
#endif

    private:
        unsigned char m_sub_record_type;
        msgVideoEncodeSetting_t *m_pMain_stream_encoder;
        msgVideoEncodeSetting_t *m_pSub_stream_encoder;
        msgEncodeOsdSetting_t *m_pVideo_encoder_osd;
#if (defined(_AV_PLATFORM_HI3520D_V300_) || defined(_AV_PLATFORM_HI3515_))	//6AII_AV12机型专用osd叠加
		OsdItem_t				m_stuExtendOsdParam[SUPPORT_MAX_VIDEO_CHANNEL_NUM][OVERLAY_MAX_NUM_VENC];	//extend osd param from net
		unsigned char			m_bOsdChanged[SUPPORT_MAX_VIDEO_CHANNEL_NUM][OVERLAY_MAX_NUM_VENC];
#endif
        unsigned char m_record_type[SUPPORT_MAX_VIDEO_CHANNEL_NUM][2];//value-0: normal type //1:alarm type;  
        unsigned char m_record_type_bak[SUPPORT_MAX_VIDEO_CHANNEL_NUM][2];//value-0: normal type //1:alarm type; 
        char m_bus_number[MAX_OSD_NAME_SIZE];
        char m_bus_selfsequeue_number[MAX_OSD_NAME_SIZE];
        //0628
        char m_station_name[MAX_EXT_OSD_NAME_SIZE];
        char m_chn_name[SUPPORT_MAX_VIDEO_CHANNEL_NUM][MAX_OSD_NAME_SIZE];
        //0730

        msgTimeSetting_t *m_pTime_setting;
        std::bitset<32> m_main_encoder_state;
        std::bitset<32> m_sub_encoder_state;
        unsigned char m_net_transfe;
        msgVideoEncodeSetting_t *m_pSub2_stream_encoder;
        unsigned char m_net_start_substream_enable[SUPPORT_MAX_VIDEO_CHANNEL_NUM];
        std::bitset<32> m_sub2_encoder_state;
        char m_chn_state[SUPPORT_MAX_VIDEO_CHANNEL_NUM]; //!<  Added for channel switch  0,off, 1 enable
#ifdef _AV_PRODUCT_CLASS_IPC_
        unsigned char m_u8SourceFrameRate; // for ipc, low luma, slow ISP frame rate
        unsigned char m_u8IPCSnapWorkMode; //IPC的工作模式，0常规，1-对接NVR，测试不再接收事件抓拍
        char m_osd_content[4][MAX_OSD_NAME_SIZE]; 
        char m_ext_osd_content[2][MAX_EXT_OSD_NAME_SIZE];
        msgAutoIpInfo_t *m_pIpAddress;
#endif

#ifndef _AV_PRODUCT_CLASS_IPC_
        char m_ext_osd_content[2][MAX_EXT_OSD_NAME_SIZE];
        int  m_ext_osd_x[2];
        int  m_ext_osd_y[2];
#endif

#endif

#if defined(_AV_HAVE_VIDEO_INPUT_)
    public:
        int Aa_vi_interrput_detect_proc();
    private:

#if !defined(_AV_PRODUCT_CLASS_IPC_)
        int Aa_boardcast_vi_resolution();
#endif
        int Aa_get_video_input_format(int phy_video_chn_num, av_resolution_t *pResolution, int *pFrame_rate, bool *pProgress);
        int Aa_get_vi_config_set(int phy_video_chn_num, vi_config_set_t &vi_config, void *pVi_Para = NULL, vi_config_set_t *pPrimary_attr = NULL, vi_config_set_t *pMinor_attr = NULL);

        int Aa_start_vi();
        int Aa_start_selected_vi(unsigned long long int chn_mask);

        int Aa_stop_vi();
        int Aa_stop_selected_vi(unsigned long long int chn_mask);

        int Aa_reset_vi();
        int Aa_reset_selected_vi(unsigned long long int chn_mask, bool brelate_decoder = true);

        void Aa_restart_vi();
        void Aa_restart_selected_vi(unsigned long long int chn_mask, bool brelate_decoder = true);

        int Aa_stop_selected_vi_for_analog_to_digital(unsigned long long int chn_mask);
        int Aa_start_selected_vi_for_digital_to_analog(unsigned long long int chn_mask);

        void Aa_release_vi();
        void Aa_release_selected_vi(unsigned long long int chn_mask, bool brelate_decoder = true);

        bool Aa_weather_reset_vi(void *pVi_para = NULL);
        void Aa_send_video_status(int type);
#if !defined(_AV_PLATFORM_HI3518EV200_)
        int Aa_start_vda(vda_type_e vda_type, int phy_video_chn=-1, const vda_chn_param_t* pVda_param=NULL);
        int Aa_start_selected_vda(vda_type_e vda_type, unsigned long long int start_chn_mask);
        int Aa_stop_vda(vda_type_e vda_type, int phy_video_chn=-1);
        int Aa_stop_selected_vda(vda_type_e vda_type, unsigned long long int stop_chn_mask);
        int Aa_get_vda_result(vda_type_e vda_type, unsigned int *pVda_result, int phy_video_chn=-1, unsigned char *pVda_region=NULL);
        void Aa_message_handle_MOTIONDETECTPARA(const char *msg, int length);
        void Aa_message_handle_GET_MD_BLOCKSTATE(const char *msg, int length);
        void Aa_message_handle_VIDEOSHIELDPARA(const char *msg, int length);
#endif
        void Aa_message_handle_GET_VIDEO_STATUS(const char *msg, int length);
        void Aa_message_handle_VIDEOINPUTFORMAT(const char *msg, int length);
        void Aa_message_handle_COVERPARA(const char *msg, int length);
        //int Aa_cover_area(msgVideoCover_t *pNew_cover, msgVideoCover_t *pOld_cover);
#if !defined(_AV_PRODUCT_CLASS_IPC_)
        void Aa_message_handle_VIRESOLUTION(const char *msg, int length);
#endif
        void Aa_message_handle_DEAL_VIDEOLOSS_STATE(const char *msg, int length);
        void Aa_message_handle_VIDEOINPUTIMG(const char *msg, int length);
        void Aa_message_handle_GET_MAINBOARD_VERSION(const char *msg, int length);

#if defined(_AV_PRODUCT_CLASS_HDVR_)
        void Aa_message_handle_SET_VIDEO_ATTR(const char *msg, int length);
        void Aa_message_handle_SET_VIDEO_FORMAT(const char *msg, int length);
#endif

#if !defined(_AV_PRODUCT_CLASS_IPC_)   
    void Aa_send_analog_od_alarm_info( int  alarm_interval);
#endif


#if defined(_AV_PLATFORM_HI3515_)	//6AI音频故障检测上报///
		int Aa_6a1_check_audio_fault(int chn, unsigned int* pval);
		int Aa_6a1_broadcast_audio_fault(int fault);
		void Aa_message_handle_Get_6a1_audio_fault(const char *msg, int length);
		int Get_alinedata(char *socbuf, char *disbuf,int size);
#endif


#if defined(_AV_PRODUCT_CLASS_IPC_)      
    private:

#if !defined(_AV_PLATFORM_HI3518EV200_)
    /*<*add by dhdong 2014-11-13 14:50
    *brief:send od detect alarm info to other modules
    * param:alarm_interval.every alarm_interval seconds to send alarm info*>
    */
        void Aa_send_od_alarm_info( int  alarm_interval);
        void Aa_send_od_alarm_info_for_cnr(int  alarm_interval);
#endif
        bool Aa_is_customer_cnr();
        bool Aa_is_customer_dc();
#if defined(_AV_PLATFORM_HI3518C_)
        int Aa_sensor_read_reg_value(int reg_addr);
        /*
        *712XX projects need to support sensor imx238 and imx 225 at the same time.
        *we read the register 0x3008,if the value is 0x10,the sensor is imx238, if the value
        *is 0x0,the sensor is imx225,but avoid the software can't work,the value we read is 
        *neither 0x10 nor 0x0,we treat it as imx238
        */        
        sensor_e Aa_sensor_get_type();
#endif
        int Aa_start_vi_interrput_monitor();
        int Aa_stop_vi_interrput_monitor();
        int Aa_reset_sensor();
#endif   
    private:
        //msgInputVideoFormat_t *m_pInput_video_format;
        paramMDAlarmSetting_t *m_pMotion_detect_para;
        //msgVideoCover_t *m_pCover_area_para;
        paramVideoShieldAlarmSetting_t *m_pVideo_shield_para;
        vi_config_set_t m_vi_config_set[32];
        vi_config_set_t *m_pVi_primary_config;
        vi_config_set_t *m_pVi_minor_config;
#endif

#if defined(_AV_HAVE_AUDIO_INPUT_)
    private:
        int Aa_start_ai();
        int Aa_stop_ai();
#if defined(_AV_PRODUCT_CLASS_IPC_)
        void Aa_message_handle_AUDIO_PARAM(const char *msg, int length);
        int Aa_outer_codec_fm1288_init(eProductType product);


    private:
        msgAudioSetting_t *m_pAudio_param;
#endif
#endif

#if defined (_AV_HAVE_AUDIO_OUTPUT_)
    public:
        void Aa_message_handle_Set_PreviewAudio_State(const char* msg, int length );
        void Aa_message_handle_STARTPREVIEWAUDIO(const char* msg, int length );
        void Aa_message_handle_STOPPREVIEWAUDIO(const char* msg, int length );
        
    private:
        int Aa_start_ao();
        int Aa_stop_ao();
#endif

#if defined(_AV_TEST_MAINBOARD_)
    public:
        int Aa_test_audio_IO();
#endif

    private:
        int Aa_talkback_ctrl(bool start_flag, msgRequestAudioNetPara_t *pAudioNetPara=NULL);
        //add 2015.3.23
        int Aa_talkback_ctrl(bool start_flag, msgTalkbackAduio_t* pTalkbackPara=NULL);
        void Aa_message_handle_START_TALKBACK(const char *msg, int length);
        void Aa_message_handle_STOP_TALKBACK(const char *msg, int length);
        void Aa_message_handle_AUDIO_OUTPUT(const char *msg, int length);
        void Aa_message_handle_AUDIO_MUTE(const char *msg, int length);
        void Aa_message_handle_AUDIO_VOLUMN_SET(const char *msg, int length);
        inline bool Aa_get_talkback_audio_state(){return m_talkback_start;}
    private:
        bool m_talkback_start;
        bool m_reportstation_start;

#if defined(_AV_HAVE_AUDIO_INPUT_) && defined(_AV_HAVE_AUDIO_OUTPUT_) && (defined(_AV_PRODUCT_CLASS_DVR_) || defined(_AV_PRODUCT_CLASS_HDVR_))
    private:
        int Aa_start_preview_audio();
#endif

#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
    private:
        void Aa_message_handle_DeviceOnlineState( const char* msg, int length );
        void Aa_message_handle_RemoteDeviceVideoLoss( const char* msg, int length );
        void Aa_message_handle_ChannelMap( const char* msg, int length );
        
    private:
        struct {
            unsigned char u8OnlineState[_AV_VIDEO_MAX_CHN_];
        } m_stuModuleEncoderInfo;
        bool m_reset_video_source;
        unsigned char m_remote_vl[SUPPORT_MAX_VIDEO_CHANNEL_NUM];
        int m_last_chn_state[_AV_VIDEO_MAX_CHN_]; // 0: analog signal 1: digital signal
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_)
    private:
        void Aa_message_handle_CameraAttr( const char* msg, int length );
        void Aa_message_handle_CameraColor( const char* msg, int length );
        //
        void Aa_message_handle_LightLeval( const char* msg, int lenght );
        void Ae_message_handle_GetDayNightCutLuma( const char* msg, int length );

        void Aa_message_handle_MacAddress(const char* msg, int length);
        void Aa_message_handle_AuthSerial(const char* msg, int length);
#if defined(_AV_SUPPORT_IVE_)
#if defined(_AV_IVE_FD_) || defined(_AV_IVE_HC_)
        void Aa_message_handle_IveFaceDectetion(const char* msg, int length);
#endif
#if defined(_AV_IVE_BLE_)
        void Aa_message_handle_chongqing_blacklist(const char* msg, int length);
#endif
        void Aa_clear_common_osd();
        void Aa_ive_detection_result_notify_func(msgIPCIntelligentInfoNotice_t* pIve_detection_result);
#endif
        
    private:
        msgCameraAttributeQuickSet_t* m_pstuCameraAttrParam;//
        msgVideoImageSetting_t* m_pstuCameraColorParam;//
#endif

    private:
        int Aa_everything_is_ready();
        int Aa_start_work();
        int Aa_stop_work();
        int Aa_restart_work();
    public:        
        int Aa_construct_message_client();
    private: 
        int Aa_register_message();
        inline bool Aa_get_playback_audin_state(){return m_StartPlayCmd;}
    private:/*system config parameters*/
        av_tvsystem_t m_av_tvsystem;
        msgGeneralSetting_t *m_pSystem_setting;
        paramScreenMargin_t *m_pScreen_Margin;
        msgScreenMosaic_t *m_pScreenMosaic;
        unsigned long m_entityReady;
        bool m_StartPlayCmd;
#if defined(_AV_HAVE_VIDEO_DECODER_)
        PlayState_t m_playState;
        datetime_t m_last_playbcak_time;
        av_dec_set_t m_av_dec_set;
        msgVideoResolution_t m_vdec_image_res[_VDEC_MAX_VDP_NUM_];
#endif
	
#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
			unsigned char m_ChnProtocolType[SUPPORT_MAX_VIDEO_CHANNEL_NUM];
#endif

    private:
        std::map<enumSystemMessage, bool> m_start_work_env;
        int m_start_work_flag;
        Av_video_state_t m_alarm_status;
        SWatchdogMsg m_watchdog;

    private:
        Common::HANDLE m_message_client_handle;
        CAvDeviceMaster *m_pAv_device_master;
#if !defined(_AV_PRODUCT_CLASS_IPC_)

        int m_report_method;
        rm_uint32_t  m_uiAudioType;
        rm_int32_t m_source; /**< 数据源标示 */
#endif
        int m_upgrade_flag;
#if defined(_AV_PLATFORM_HI3515_)
		int m_nPickupStat;	//6A一代拾音器状态///
#endif
    public:
#if !defined(_AV_PRODUCT_CLASS_IPC_)

#if defined(_AV_HAVE_TTS_)
        void Aa_message_handle_StartTTS(const char * msg,int length);
        void Aa_message_handle_StopTTS(const char * msg,int length);
        void Aa_message_handle_CallPhoneTTS(const char * msg,int length);
#endif		
        void Aa_message_handle_STARTREPORTSTATION(const char* msg, int length );
        void Aa_message_handle_STOPREPORTSTATION(const char* msg, int length );
        void Aa_message_handle_INQUIRE_AUDIO_STATE(const char* msg, int length );
        void Aa_SendReportEndResultMessage();
        inline void Aa_Set_ReportStation_AudioType(rm_uint32_t audiotype){m_uiAudioType =            audiotype;}
        inline rm_uint32_t Aa_Get_ReportStation_AudioType(){return m_uiAudioType;}
#endif
};


#endif/*__AVAPPMASTER_H__*/


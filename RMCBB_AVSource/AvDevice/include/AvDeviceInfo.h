/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __AVDEVICEINFO_H__
#define __AVDEVICEINFO_H__
#include "AvConfig.h"
#include "AvDebug.h"
#include "ConfigCommonDefine.h"
#include "AvCommonType.h"
#include "AvDebug.h"
#include "AvCommonMacro.h"
#include "SystemTime.h"
#include <string>
#include <map>
#if defined(_AV_PRODUCT_CLASS_IPC_) || defined(_AV_PLATFORM_HI3520A_) || defined(_AV_PLATFORM_HI3521_) || defined(_AV_PLATFORM_HI3531_) || defined(_AV_PLATFORM_HI3532_) || defined(_AV_PLATFORM_HI3520D_) || defined(_AV_PLATFORM_HI3535_) || defined(_AV_PLATFORM_HI3520D_V300_)||defined(_AV_PLATFORM_HI3515_)
#include "hi_comm_video.h"
#endif
#if defined(_AV_PRODUCT_CLASS_IPC_)
#include "hi_common.h"
#endif
#if defined(_AV_PRODUCT_CLASS_IPC_)
#include "rm_isp_api.h"
#endif
#include <bitset>

using namespace Common;

#define OSD_TEST_FLAG		PTD_6A_II_AVX	///*6AII_AV12 product special osd overlay, include snap osd*/

#if defined(_AV_PRODUCT_CLASS_IPC_) 

#define MAX_SUPPORT_SENSOR_NUM (2)

typedef enum sensor_type
{
    ST_SENSOR_DEFAULT = 0,
    ST_SENSOR_IMX238,
    ST_SENSOR_IMX225,
    ST_SENSOR_IMX290,
    ST_SENSOR_IMX178,
    ST_SENSOR_IMX222,
    ST_SENSOR_OV9732,

    ST_SENSOR_UMKOWN = 100
}sensor_e;

typedef struct vi_capture_rect
{
    unsigned char m_res;
    unsigned char resvered[3];
    unsigned short m_vi_capture_width;
    unsigned short m_vi_capture_height;
    unsigned short m_vi_capture_x;
    unsigned short m_vi_capture_y; 
}vi_capture_rect_s;

typedef struct vi_cfg
{
    typedef std::vector<vi_capture_rect_s> vi_capture_rect_cfg;
    sensor_e m_eSensor_type;
    std::vector<vi_capture_rect_cfg> vi_capture_chn_cfg;
}vi_cfg_s;

typedef struct encoder_size
{
    struct _size_{
        unsigned short width;
        unsigned short height;
    }stuSize[3];
}encoder_size_s;
#endif

class CAvDeviceInfo{
    public:
        CAvDeviceInfo(eProductType product_type,eSystemType sub_product_type, eAppType app_type = APP_TYPE_FORMAL)
        {
            m_product_type = product_type;
            m_app_type = app_type;
            m_sub_product_type = sub_product_type;
            memset(&m_pip_size, 0, sizeof(av_rect_t));
            m_sys_pic_path.clear();
            m_audio_chn_num = SUPPORT_MAX_AUDIO_CHANNEL_NUM;
            m_hardware_version = 0xff;
            m_watermark_switch = false;
            m_custom_bitrate = true;
            m_wd1_width = 928;
            m_audio_cvbs_ctrl_switch = false;
            m_screen_display_adjust = false;
#if defined(_AV_PRODUCT_CLASS_IPC_)
            m_u32CurrentLum = 0;
            m_bWdrControlEnable = false;
            //!Added Nov 2014			
            m_bHeightLightCompEnable = false;

            m_bSupport960P = false;
            m_u16FrameRate = 30;
            //default device has ir_cut and infrared
            m_bIrcutSupportFlag = true;
            //default is slave IPC
            m_u8IpcWorkMode = 1;

            memset(m_u8MacAddress, 0, sizeof(m_u8MacAddress));
            memset(m_u8MacAddressStr, 0, sizeof(m_u8MacAddressStr));
            
            m_eSensor = ST_SENSOR_DEFAULT;
            m_u8FocusLen = 4;//default focus len is :4mm
#if defined(_AV_PLATFORM_HI3516A_) || defined(_AV_PLATFORM_HI3518EV200_)
            /*default is offline mode*/
            m_u8ViVpssMode = 0;
#if defined(_AV_SUPPORT_IVE_)
            m_uIveDebugMode = 0;
#endif
#endif
#if defined(_AV_PLATFORM_HI3518C_) || defined(_AV_PLATFORM_HI3518EV200_)
            /*isp default output mode is 720P@30fps*/
            m_eIspOutputMode = ISP_OUTPUT_MODE_720P30FPS;
#else
            /*isp default output mode is 5M@30FPS*/
            m_eIspOutputMode = ISP_OUTPUT_MODE_1080P30FPS;
#endif
            m_cVi_chn_capture_y_offset = 0;
            memset(m_stuEncoder_size, 0, sizeof(m_stuEncoder_size));
#endif

#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_)
            memset(m_video_source, _VS_DIGITAL_, sizeof(m_video_source));
#else
            memset(m_video_source, _VS_ANALOG_, sizeof(m_video_source));
#endif
            for (int type = 0; type < _AV_VIDEO_STREAM_MAX_NUM_; type++)
            {
                m_channel_record_cut[type].reset();
            }
            m_system_change_count = 0xff;
            m_screenmode=13;
            m_vi_num = 0;
            m_vi_max_width = 1920;
            m_vi_max_height = 1080;
            m_screen_width = 800;
            m_screen_height = 480;
            memset(m_cur_display_chn, -1, sizeof(int) * 36);
            m_stream_buffer_info.clear();
            m_cur_preview_res.clear();
            m_digital_num = 0;
            memset(&m_speed,0,sizeof(m_speed));
#if !defined(_AV_PRODUCT_CLASS_DVR_REI_)
			memset(&m_speed_uni,0,sizeof(m_speed_uni));
#endif
            memset(&m_tax2_speed, 0, sizeof(Tax2DataList));
            memset(&m_gpsinfo,0,sizeof(msgGpsInfo_t));
            memset(&m_Speed_Info,0,sizeof(m_Speed_Info));
            memset(m_created_preview_decoder, false, sizeof(m_created_preview_decoder));
            m_main_out_mode = _AOT_MAIN_;
            if(m_product_type == PTD_XMD3104)
            {
                m_main_out_mode = _AOT_CVBS_;
            }
            if((m_product_type == PTD_A5_AHD) || ((m_product_type == PTD_ES4206) && (m_sub_product_type == SYSTEM_TYPE_ES4206_V2)) || ((m_product_type == PTD_ES8412) && (m_sub_product_type == SYSTEM_TYPE_ES8412_V2))
                ||(m_product_type == PTD_M0026)||(m_product_type == PTD_M3))
            {
                m_support_mixtrue = 1;
            }
            else
            {
                m_support_mixtrue = 0;
            }

#if !defined(_AV_PRODUCT_CLASS_IPC_)
            m_isvideoinputformatchanged = false;
            m_ahdinputformatvalidmask =0;
            m_inputFormatValidMask = 0;
            m_inputFormatValidMask_bak = 0;
            m_videoinputformat[0]=m_videoinputformat_bak[0]=_VIDEO_INPUT_INVALID_;
            m_videoinputformat[1]=m_videoinputformat_bak[1]=_VIDEO_INPUT_INVALID_;
            m_videoinputformat[2]=m_videoinputformat_bak[2]=_VIDEO_INPUT_INVALID_;//_VIDEO_INPUT_AHD_25P_;
            m_videoinputformat[3]=m_videoinputformat_bak[3]=_VIDEO_INPUT_INVALID_;
#endif
            m_tv_system = _AT_UNKNOWN_;
            m_waternark_trans = 128;
            //default customer is CUSTOMER_NULL
            strncpy(m_customer_name, "CUSTOMER_NULL", sizeof(m_customer_name));
			memset(m_platform_name, 0, sizeof(m_platform_name));
			
			m_strBeiJingStationInfo = "";
			memset(&m_ApcNum,0, sizeof(m_ApcNum));
        }

        ~CAvDeviceInfo()
        {
        }

    public:
        eProductType Adi_product_type(){return m_product_type;}
        eSystemType Adi_sub_product_type(){return m_sub_product_type;}
        eAppType Adi_app_type(){return m_app_type;}
        int Adi_get_D1_width(){return 704;}
        int Adi_get_WD1_width(){return m_wd1_width;}
        unsigned char Adi_get_camera_insert_mode(){return m_support_mixtrue;};
        void Adi_set_hardware_version(unsigned int hardware_version) {m_hardware_version=hardware_version; return;}
        unsigned int Adi_get_hardware_version(){return m_hardware_version;}
        int Adi_get_video_size(av_resolution_t resolution, av_tvsystem_t tv_system, int *pWidth, int *pHeight);
#if defined(_AV_PLATFORM_HI3515_)
	 	int Adi_get_vi_video_size(av_resolution_t resolution, av_tvsystem_t tv_system, int *pWidth, int *pHeight);
#endif
#if defined(_AV_PRODUCT_CLASS_IPC_)
        int Adi_get_video_size(IspOutputMode_e iom, int *pWidth, int *pHeight);
#else
        int Adi_get_video_size_input_mix(unsigned char chn,av_resolution_t resolution, av_tvsystem_t tv_system, int *pWidth, int *pHeight);
#endif
        int Adi_get_video_resolution(uint8_t *piResolution, av_resolution_t *paResolution, bool from_i_to_a);
        int Adi_covert_coordinate( av_resolution_t resolution, av_tvsystem_t tv_system, int *pDirX, int *pDirY, int align=16 );
        int Adi_covert_coordinateEx( unsigned short u16PicWidth, unsigned short u16PicHeight, int *pDirX, int *pDirY, int align=16 );

        int Adi_get_audio_chn_id(int phy_audio_chn_num, int &phy_audio_chn_id);

        unsigned long long int Aa_get_all_analog_chn_mask();
        int Aa_set_screenmode(unsigned int screenmode);
        unsigned int Aa_get_screenmode();

#if defined(_AV_PRODUCT_CLASS_IPC_) || defined(_AV_PLATFORM_HI3520A_) || defined(_AV_PLATFORM_HI3521_) || defined(_AV_PLATFORM_HI3531_) || defined(_AV_PLATFORM_HI3532_) || defined(_AV_PLATFORM_HI3520D_) || defined(_AV_PLATFORM_HI3535_) || defined(_AV_PLATFORM_HI3520D_V300_)||defined(_AV_PLATFORM_HI3515_)
    public:
        PIXEL_FORMAT_E Adi_get_vi_pixel_format(){return PIXEL_FORMAT_YUV_SEMIPLANAR_420;}
        PIXEL_FORMAT_E Adi_get_video_layer_pixel_format(){return PIXEL_FORMAT_YUV_SEMIPLANAR_420;}
        PIXEL_FORMAT_E Adi_get_cvbs_vo_pixel_format(){return PIXEL_FORMAT_YUV_SEMIPLANAR_420;}
        PIXEL_FORMAT_E Adi_get_vdec_pixel_format(){return PIXEL_FORMAT_YUV_SEMIPLANAR_420;}
        PIXEL_FORMAT_E Adi_get_pciv_pixel_format(){return PIXEL_FORMAT_YUV_SEMIPLANAR_420;}
        float Adi_get_pixel_size(){return 1.5;}
#endif

    public:
        unsigned int Adi_get_vo_background_color(av_output_type_t output, int index);
        int Adi_have_spot_function();

    public:
        inline void Adi_set_sys_pic_path(av_sys_pic_type_e pic_type, std::string pic_path) {m_sys_pic_path[pic_type] = pic_path;}
        inline std::string Adi_get_sys_pic_path(av_sys_pic_type_e pic_type) {return m_sys_pic_path[pic_type];}
        inline void Adi_set_audio_chn_number(int num) { m_audio_chn_num = num; }
        inline int Adi_get_audio_chn_number() const { return m_audio_chn_num; }
        inline void Adi_set_watermark_switch(bool watermark_switch) {m_watermark_switch = watermark_switch;}
        inline bool Adi_get_watermark_switch() const {return m_watermark_switch;}
        inline void Adi_set_custom_bitrate(bool custom_bitrate) {m_custom_bitrate = custom_bitrate;}
        inline bool Adi_get_custom_bitrate() const {return m_custom_bitrate;}
        
        inline void Adi_set_video_source(int phy_video_chn_num, unsigned char video_resource){m_video_source[phy_video_chn_num] = video_resource;};
        inline unsigned char Adi_get_video_source(int phy_video_chn_num){return m_video_source[phy_video_chn_num];};
        inline void Adi_set_WD1_size(int width){m_wd1_width = width; return;}
        //as global preview audio switch;
        inline void Adi_set_audio_cvbs_ctrl_switch(bool ctrl_switch){m_audio_cvbs_ctrl_switch = ctrl_switch;return;}
        inline bool Adi_get_audio_cvbs_ctrl_switch(){return m_audio_cvbs_ctrl_switch;}
    public:
        /*split*/
        int Adi_get_split_chn_rect(av_rect_t *pVideo_rect, av_split_t split_mode, int chn_num, av_rect_t *pChn_rect, VO_DEV vo_dev);
        int Adi_get_split_chn_info(av_split_t split_mode);
        av_split_t Adi_get_split_mode(int chn_num);
        int Adi_set_split_chn_rect_PIP(av_rect_t Video_rect, int chn_num, av_rect_t Chn_rect);
        int Adi_screen_display_is_adjust(bool adjust);
        /*function list*/
        int Adi_max_channel_number();
        bool Adi_weather_phychn_sdi(int phy_chn_num);
        bool Adi_is_dynamic_playback_by_split();
        int Adi_get_sub_encoder_resource(int &encode_resoure, av_tvsystem_t av_tvsystem);
        int Adi_get_ref_para(av_ref_para_t &av_ref_para, av_video_stream_type_t eStreamType=_AST_MAIN_VIDEO_);
        int Adi_get_factor_by_cif(av_resolution_t av_resolution, int &factor);
        int Adi_get_max_resource(av_tvsystem_t tvsystem, int  &cif_total_count, int itotal_analog_num = 0, int *iplayback_res_num = NULL, int *iencode_analog_res_num = NULL);
        unsigned long long int Adi_get_all_chn_mask();
        int Adi_calculate_encode_source(av_tvsystem_t tvsystem, int itotal_analog_num = 0, int *iencode_analog_res_num = NULL);
        int Adi_calculate_playback_source(av_tvsystem_t tvsystem, int iplayback_frame_rate, int *iplayback_res_num = NULL);
#if defined(_AV_PRODUCT_CLASS_IPC_)
        void Adi_SetCurrentLum( unsigned int u32Lum ){m_u32CurrentLum = u32Lum; }
        unsigned int Adi_GetCurrentLum(){   return m_u32CurrentLum; }
        int Adi_set_system_rotate( unsigned char u8Rorate );
        int Adi_get_system_rotate() {   return m_u8Rotate;  }
        int Adi_covert_vidoe_size_by_rotate( int* pWidth, int* pHeight );
        void Adi_set_wdr_control_switch( bool bEnable ) {  m_bWdrControlEnable = bEnable;   }
        int Adi_get_wdr_control_switch() {  return m_bWdrControlEnable;   }
//! Added for IPC transplant, Nov 2014
        void Adi_set_height_light_comps_switch(bool bEnable){ m_bHeightLightCompEnable = bEnable; }
        bool Adi_get_height_light_comps_switch () const{ return  m_bHeightLightCompEnable; };
        
        void Adi_set_960P_switch( bool bSupport ) { m_bSupport960P = bSupport;  }
        bool Adi_get_960p_switch()  {   return m_bSupport960P;  }

        void Adi_set_framerate(unsigned short u16Framerate){m_u16FrameRate = u16Framerate;}
        unsigned short Adi_get_framerate()const{return m_u16FrameRate;}

        void Adi_set_ircut_and_infrared_flag(bool bEnable){ m_bIrcutSupportFlag = bEnable; }
        bool Adi_get_ircut_and_infrared_flag () const{ return  m_bIrcutSupportFlag; };

        void Adi_set_ipc_work_mode(unsigned char  u8Work_mode){ m_u8IpcWorkMode =  u8Work_mode;}
        unsigned char Adi_get_ipc_work_mode() const{return m_u8IpcWorkMode;};

        void Adi_set_mac_address(const rm_uint8_t mac_address[], unsigned char array_len);
        void Adi_get_mac_address (rm_uint8_t mac_address[], unsigned char array_len)const;
        void Adi_get_mac_address_as_string (char mac_address[], unsigned char array_len)const;

        void Adi_set_vi_config(sensor_e sensor_type, unsigned char phy_chn_num, vi_capture_rect_s& vi_cap_rect);
        void Adi_get_vi_config(sensor_e sensor_type, unsigned char phy_chn_num, unsigned char resolution, vi_capture_rect_s& vi_cap_rect);
       
        unsigned char Adi_get_focus_length() const {return m_u8FocusLen;};
        void Adi_set_focus_length(unsigned char focus_len){m_u8FocusLen = focus_len;};

        void Adi_set_sensor_type(sensor_e sensor_type){ m_eSensor = sensor_type;};
        sensor_e Adi_get_sensor_type()const{ return m_eSensor;};
#if defined(_AV_PLATFORM_HI3516A_) || defined(_AV_PLATFORM_HI3518EV200_)
        void Adi_set_vi_vpss_mode(unsigned char u8ViVpssMode){m_u8ViVpssMode = u8ViVpssMode;}
        /*1:online 0:offline*/
        unsigned char Adi_get_vi_vpss_mode() const{return m_u8ViVpssMode;}
#if defined(_AV_SUPPORT_IVE_)
        void Adi_set_ive_debug_mode(unsigned char u8DebugMode){m_uIveDebugMode = u8DebugMode;}
        unsigned char Adi_get_ive_debug_mode()const{return  m_uIveDebugMode;}
#endif
#endif
       void Adi_set_isp_output_mode(IspOutputMode_e isp_output_mode){m_eIspOutputMode = isp_output_mode;}
       IspOutputMode_e Adi_get_isp_output_mode() const{ return m_eIspOutputMode;}
       unsigned char Adi_get_vi_max_resolution();

        void Adi_set_vi_chn_capture_y_offset(short offset){m_cVi_chn_capture_y_offset = offset;};
        short Adi_get_vi_chn_capture_y_offset(){return m_cVi_chn_capture_y_offset;};
        
        void Adi_get_stream_size(unsigned char phy_chn, av_video_stream_type_t streamType, unsigned int& width, unsigned int &height);
        void Adi_set_stream_size(unsigned char phy_chn, av_video_stream_type_t streamType, unsigned int width, unsigned int height);       
#endif
        av_output_type_t Adi_get_main_out_mode();
        rm_int32_t Adi_set_system(av_tvsystem_t system);
        av_tvsystem_t Adi_get_system();
        int Adi_ad_type_isahd();

        int Adi_get_date_time(datetime_t *pDate_time);
        int Adi_get_record_cut(int type, int phy_chn);

        int Adi_set_vi_info(unsigned int vi_num, unsigned int vi_max_width, unsigned int vi_max_height);
        int Adi_get_vi_info(unsigned int &vi_num, unsigned int &vi_max_width, unsigned int &vi_max_height);
        int Adi_get_vi_chn_num();


        int Adi_set_cur_chn_res(int phy_chn_num, unsigned int width, unsigned int height);
        int Adi_get_cur_display_chn(int cur_display_chn[36] = NULL);

        void Adi_backup_cur_preview_res();
        void Adi_set_cur_preview_res_with_backup_res();

        void Adi_set_created_preview_decoder(int phy_chn, bool flag) {m_created_preview_decoder[phy_chn] = flag;}
        bool Adi_get_created_preview_decoder(int phy_chn) {return m_created_preview_decoder[phy_chn];}

        int Adi_set_stream_buffer_info(eStreamBufferType stream_type, unsigned int chn_id, char buf_name[32], unsigned int buf_size, unsigned int buf_cnt);
        int Adi_get_stream_buffer_info(eStreamBufferType stream_type, unsigned int chn_id, char buf_name[32], unsigned int &buf_size, unsigned int &buf_cnt);

        int Adi_set_digital_chn_num(unsigned int digital_chn_num);
        int Adi_get_digital_chn_num();
        int Adi_Set_Speed_Info(msgSpeedState_t *speedinfo);
        int Adi_Get_Speed_Info(msgSpeedState_t *speedinfo);
		//! novel speed
#if !defined(_AV_PRODUCT_CLASS_DVR_REI_)
        int Adi_Set_Speed_Info(msgSpeedInfo_t *speedinfo);
        int Adi_Get_Speed_Info(msgSpeedInfo_t *speedinfo);
#endif
        //0703
        int Adi_Set_Speed_Info_from_Tax2(Tax2DataList*speedinfo);
        int Adi_Get_Speed_Info_from_Tax2(Tax2DataList*speedinfo);
        int Adi_Set_Gps_Info(msgGpsInfo_t *gpsinfo);
        int Adi_Get_Gps_Info(msgGpsInfo_t *gpsinfo);
        int Adi_Set_SpeedSource_Info(paramSpeedAlarmSetting_t *speedsource);
        int Adi_Get_SpeedSource_Info(paramSpeedAlarmSetting_t *speedsource);
        /*customer name*/
        void Adi_set_customer_name( const char* pszCustomerName){ if(NULL != pszCustomerName){ strncpy(m_customer_name, pszCustomerName, sizeof(m_customer_name)); } }
        void Adi_get_customer_name( char* pszCustomerName, unsigned char u8BuffLen){ if(NULL != pszCustomerName){ strncpy(pszCustomerName, m_customer_name, u8BuffLen); } }
        /**product group*/
        void Adi_set_product_group( const char* pszProductGrooup){ if(NULL != pszProductGrooup){ strncpy(m_product_group, pszProductGrooup, sizeof(m_product_group)); } }
        void Adi_get_product_group( char* pszProductGrooup, unsigned char u8BuffLen){ if(NULL != pszProductGrooup){ strncpy(pszProductGrooup, m_product_group, u8BuffLen); } }
        /*check criterion*/
        void Adi_set_check_criterion( const char* pszCriterionType){ if(NULL != pszCriterionType){ strncpy(m_check_criterion, pszCriterionType, sizeof(m_check_criterion)); } }
        void Adi_get_check_criterion( char* pszCriterionType, unsigned char u8BuffLen){ if(NULL != pszCriterionType){ strncpy(pszCriterionType, m_check_criterion, u8BuffLen); } }
        /*platform name*/
        void Adi_set_platform_name( const char* pszPlatformName){ if(NULL != pszPlatformName){ strncpy(m_platform_name, pszPlatformName, sizeof(m_check_criterion)); } }
        void Adi_get_platform_name( char* pszPlatformName, unsigned char u8BuffLen){ if(NULL != pszPlatformName){ strncpy(pszPlatformName, m_platform_name, u8BuffLen); } }

#if !defined(_AV_PRODUCT_CLASS_IPC_)
		int Adi_set_ahd_video_format( int phy_chn_num,  unsigned int chn_mask,unsigned char av_video_format);
		int Adi_is_ahd_video_format_changed();
		int Adi_get_ahd_video_format(int chn);
		/**
			@return: 0-normal 1-720P(25\30) 2-1080P	 3-720P(50\60)
		**/ 
		int Adi_videoformat_isahd(int chnno);
		
		int Adi_get_ahd_chn_mask();
#endif
        void Adi_set_watermark_trans(unsigned char trans){m_waternark_trans = trans;}
        unsigned char Adi_get_watermark_trans(){return m_waternark_trans;}
		std::string Adi_get_beijing_station_info(){return m_strBeiJingStationInfo;};
		int Adi_set_beijing_station_info(const char* strStationInfo){m_strBeiJingStationInfo = strStationInfo; return 0;};
		
		msgEthernetDevApcNumToServer_t Adi_get_apc_num(){return m_ApcNum;};
		int Adi_set_apc_num(msgEthernetDevApcNumToServer_t apc_num){m_ApcNum = apc_num; return 0;};
    private:
        /*split rectangle*/
        int Adi_get_split_chn_rect_REGULAR(av_rect_t *pVideo_rect, av_split_t split_mode, int chn_num, av_point_t *pLeft_top_point, av_point_t *pRight_bottom_point);
        int Adi_get_split_chn_rect_2SPLIT(av_rect_t *pVideo_rect, int chn_num, av_point_t *pLeft_top_point, av_point_t *pRight_bottom_point);
        int Adi_get_split_chn_rect_3SPLIT(av_rect_t *pVideo_rect, int chn_num, av_point_t *pLeft_top_point, av_point_t *pRight_bottom_point);
        int Adi_get_split_chn_rect_PIP(av_rect_t *pVideo_rect, av_split_t split_mode, int chn_num, av_rect_t *pChn_rect, VO_DEV vo_dev);
        int Adi_get_split_chn_rect_6SPLIT(av_rect_t *pVideo_rect, int chn_num, av_point_t *pLeft_top_point, av_point_t *pRight_bottom_point);
        int Adi_get_split_chn_rect_8SPLIT(av_rect_t *pVideo_rect, int chn_num, av_point_t *pLeft_top_point, av_point_t *pRight_bottom_point);
        int Adi_get_split_chn_rect_10SPLIT(av_rect_t *pVideo_rect, int chn_num, av_point_t *pLeft_top_point, av_point_t *pRight_bottom_point);
        int Adi_get_split_chn_rect_12SPLIT(av_rect_t *pVideo_rect, int chn_num, av_point_t *pLeft_top_point, av_point_t *pRight_bottom_point);
        int Adi_get_split_chn_rect_13SPLIT(av_rect_t *pVideo_rect, int chn_num, av_point_t *pLeft_top_point, av_point_t *pRight_bottom_point);
        int Adi_adjust_split_chn_rect(int chn_num, av_point_t *pLeft_top_point, av_point_t *pRight_bottom_point, VO_DEV vo_dev = 0);
    private:
        eProductType m_product_type;
        eSystemType m_sub_product_type;
        eAppType m_app_type;
        av_rect_t m_pip_size;
        std::map<av_sys_pic_type_e, std::string> m_sys_pic_path;
        unsigned int m_audio_chn_num;
        unsigned int m_hardware_version;
        unsigned int m_screenmode;
        bool m_watermark_switch;
        bool m_custom_bitrate;
        unsigned char m_video_source[_AV_SPLIT_MAX_CHN_];/*0：数字，1：模拟*/
        int m_wd1_width;
        bool m_audio_cvbs_ctrl_switch;        
        std::bitset<_AV_VIDEO_MAX_CHN_> m_channel_record_cut[_AV_VIDEO_STREAM_MAX_NUM_];
        unsigned char m_system_change_count;
        
 #if defined(_AV_PRODUCT_CLASS_IPC_) // for test main board
        unsigned int m_u32CurrentLum;
        unsigned int m_u8Rotate; // rotate flag
        unsigned int m_bWdrControlEnable; //IPC customer wdr switch, default:false
//!Added Nov 2014      
        bool m_bHeightLightCompEnable;

        bool m_bSupport960P; //for 720P series IPC(IMX138) true:SENSOR is 960P MODE
        /*add by dhdong 2014 11-13 13:50 */
        unsigned short m_u16FrameRate;

        /*add by dhdong 2015-01-14*/
        bool m_bIrcutSupportFlag;

        /*add by dhdong 2015-01-17*/
        unsigned char m_u8IpcWorkMode;

        /*add by dhdong 2015-01-22*/
        rm_uint8_t m_u8MacAddress[6];
        char m_u8MacAddressStr[18];

        std::vector<vi_cfg_s> m_sVi_cfg;
        unsigned char m_u8FocusLen;
        sensor_e m_eSensor;
#if defined(_AV_PLATFORM_HI3516A_) || defined(_AV_PLATFORM_HI3518EV200_)
        unsigned char m_u8ViVpssMode;
#if defined(_AV_SUPPORT_IVE_)
        unsigned char m_uIveDebugMode;/*0:normal 1:debug*/
#endif
#endif
        IspOutputMode_e m_eIspOutputMode;

        short m_cVi_chn_capture_y_offset;

        encoder_size_s m_stuEncoder_size[SUPPORT_MAX_VIDEO_CHANNEL_NUM];
        unsigned int m_stream_width[3];
        unsigned int m_stream_height[3];
 #endif
        unsigned int m_vi_num;
        unsigned int m_vi_max_width;
        unsigned int m_vi_max_height;
        unsigned int m_screen_width;
        unsigned int m_screen_height;
        std::map< eStreamBufferType, std::map<unsigned int, av_stream_buffer_info_t> > m_stream_buffer_info;
        unsigned int m_digital_num;
        std::map<int, std::pair<int,int> > m_cur_preview_res;
        std::map<int, std::pair<int,int> > m_cur_preview_res_backup;
        int m_cur_display_chn[36];
        int m_created_preview_decoder[_AV_SPLIT_MAX_CHN_];
        msgSpeedState_t m_speed;
#if !defined(_AV_PRODUCT_CLASS_DVR_REI_)
		msgSpeedInfo_t m_speed_uni;  //!< novel speed, with various speed source  processed
#endif
        Tax2DataList m_tax2_speed;
        msgGpsInfo_t m_gpsinfo;
        bool m_screen_display_adjust;
        paramSpeedAlarmSetting_t m_Speed_Info;

        /*customer name*/
        char m_customer_name[32];
        /*product group*/
        char m_product_group[32];
        /*check criterion*/
        char m_check_criterion[32];	
		char m_platform_name[32];   //!< for Hongxin
        av_output_type_t m_main_out_mode;
        av_tvsystem_t m_tv_system;
        unsigned char m_waternark_trans;
        unsigned char m_support_mixtrue;/*0:不支持混合插入1:支持混插*/

#if !defined(_AV_PRODUCT_CLASS_IPC_)
        unsigned int m_inputFormatValidMask;
        unsigned int m_inputFormatValidMask_bak;	
        av_videoinputformat_t m_videoinputformat[SUPPORT_MAX_VIDEO_CHANNEL_NUM];
        av_videoinputformat_t m_videoinputformat_bak[SUPPORT_MAX_VIDEO_CHANNEL_NUM]; 
        unsigned char m_isvideoinputformatchanged;
        int m_ahdinputformatvalidmask;
#endif
		std::string		m_strBeiJingStationInfo;	//北京公交站点信息，替换gps所在区域///

		//! 04.1129
		msgEthernetDevApcNumToServer_t m_ApcNum;
};


extern CAvDeviceInfo *pgAvDeviceInfo;


#endif/*__AVDEVICEINFO_H__*/


/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __AVDEVICE_H__
#define __AVDEVICE_H__
#include "AvConfig.h"
#include "AvCommonMacro.h"
#include "AvDeviceInfo.h"
#include "SystemTime.h"
#include "AvDebug.h"
#if defined(_AV_HAVE_VIDEO_ENCODER_)
#include "AvEncoder.h"
#endif
#include "AvVda.h"

#if defined(_AV_PLATFORM_HI3516A_) && defined(_AV_SUPPORT_IVE_)
#include <boost/bind.hpp>
#include <boost/function.hpp>
#endif

class CAvDevice{
    public:
        CAvDevice();
        virtual ~CAvDevice();

#if defined(_AV_HAVE_VIDEO_INPUT_)
    public:
        virtual int Ad_start_vi(av_tvsystem_t tv_system, vi_config_set_t *pSource = NULL, vi_config_set_t *pPrimary_attr = NULL, vi_config_set_t *pMinor_attr = NULL) = 0;
        virtual int Ad_start_selected_vi(av_tvsystem_t tv_system, unsigned long long int chn_mask, vi_config_set_t *pSource = NULL, vi_config_set_t *pPrimary_attr = NULL, vi_config_set_t *pMinor_attr = NULL) = 0;

        virtual int Ad_stop_vi() = 0;
        virtual int Ad_stop_selected_vi(unsigned long long int chn_mask) = 0;

        virtual int Ad_get_vi_max_size(int phy_chn_num, int *pMax_width, int *pMax_height, av_resolution_t *pResolution) = 0;

        virtual int Ad_vi_cover(int phy_chn_num, int x, int y, int width, int height, int cover_index) = 0;
        virtual int Ad_vi_uncover(int phy_chn_num, int cover_index) = 0;

        virtual int Ad_vi_insert_picture(unsigned int video_loss_bmp) = 0;

#if !defined(_AV_PLATFORM_HI3518EV200_)
        virtual int Ad_start_vda(vda_type_e vda_type, av_tvsystem_t tv_system, int phy_video_chn, const vda_chn_param_t* pVda_chn_param) = 0;
        virtual int Ad_start_selected_vda(vda_type_e vda_type, av_tvsystem_t tv_system, int phy_video_chn, const vda_chn_param_t* pVda_chn_param) = 0;
        virtual int Ad_stop_vda(vda_type_e vda_type, int phy_video_chn) = 0;
        virtual int Ad_stop_selected_vda(vda_type_e vda_type, int phy_video_chn) = 0;
        virtual int Ad_get_vda_result(vda_type_e vda_type, unsigned int *pVda_result, int phy_video_chn=-1, unsigned char *pVda_region=NULL) = 0;
        

        virtual unsigned int Ad_get_vda_od_alarm_num(int phy_video_chn) = 0;
        virtual int Ad_reset_vda_od_counter(int phy_video_chn) = 0;
#if defined(_AV_PRODUCT_CLASS_IPC_)
        virtual int Ad_notify_framerate_changed(int phy_video_chn, int framerate) = 0;
        virtual unsigned int Ad_get_ive_od_counter(int phy_video_chn) = 0;
        virtual int Ad_reset_ive_od_counter(int phy_video_chn) = 0;
        virtual int Ad_start_ive_od(int phy_video_chn, unsigned char sensitivity) = 0;
        virtual int Ad_stop_ive_od(int phy_video_chn) = 0;
#endif
#endif
#if defined(_AV_PRODUCT_CLASS_IPC_)
#if defined(_AV_PLATFORM_HI3518C_)
        virtual int Ad_set_vi_chn_capture_y_offset(int phy_video_chn, short offset) = 0; 
#endif
        /*param:phy_video_chn[in]:the input phy video channel id.
                       interrut_cnt[out]:the interrupt number
            return:the interrut_cnt is valid or not.if get vi interrupt failure or the vi chn is disable,the interrut_cnt is invalid
        */
        virtual bool Ad_get_vi_chn_interrut_cnt(int phy_video_chn, unsigned int& interrut_cnt) = 0;
#endif

#endif

#if defined(_AV_HAVE_AUDIO_INPUT_)
    public:
        virtual int Ad_start_ai(ai_type_e ai_type) = 0;
        virtual int Ad_stop_ai(ai_type_e ai_type) = 0;
#endif

#if defined(_AV_HAVE_VIDEO_ENCODER_)
    protected:
        CAvEncoder *m_pAv_encoder;

    public:
        virtual int Ad_start_encoder(av_video_stream_type_t video_stream_type = _AST_UNKNOWN_, int phy_video_chn_num = -1, av_video_encoder_para_t *pVideo_para = NULL, av_audio_stream_type_t audio_stream_type = _AAST_UNKNOWN_, int phy_audio_chn_num = -1, av_audio_encoder_para_t *pAudio_para = NULL, int audio_only = -1);
        virtual int Ad_modify_encoder(av_video_stream_type_t video_stream_type = _AST_UNKNOWN_, int phy_video_chn_num = -1, av_video_encoder_para_t *pVideo_para = NULL, bool bosd_need_change = false);
        virtual int Ad_stop_encoder(av_video_stream_type_t video_stream_type = _AST_UNKNOWN_, int phy_video_chn_num = -1, av_audio_stream_type_t audio_stream_type = _AAST_UNKNOWN_, int phy_audio_chn_num = -1, int normal_audio = -1);
        int Ad_stop_selected_encoder(unsigned long long int chn_mask, av_video_stream_type_t video_stream_type = _AST_UNKNOWN_, av_audio_stream_type_t audio_stream_type = _AAST_UNKNOWN_);
        virtual int Ad_set_encoder_attr(av_video_stream_type_t video_stream_type = _AST_UNKNOWN_, int phy_video_chn_num = -1, av_video_encoder_para_t *pVideo_para = NULL, av_audio_stream_type_t audio_stream_type = _AAST_UNKNOWN_, int phy_audio_chn_num = -1, av_audio_encoder_para_t *pAudio_para = NULL);
        virtual int Ad_request_iframe(av_video_stream_type_t video_stream_type, int phy_video_chn_num);
        virtual int Ad_start_snap() = 0;
        virtual int Ad_stop_snap() = 0;
        virtual int Ad_Get_Chn_state() = 0;
        virtual int Ad_Set_Chn_state(int flag) = 0;
        virtual int Ad_modify_snap_param( av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para ) = 0;
        virtual int Ad_cmd_snap_picutres( /*const MsgSnapPicture_t* pstuSnapParam*/ ) = 0;
        virtual int Ad_cmd_signal_snap_picutres( AvSnapPara_t & pstuSignalSnapParam) = 0;
        virtual int Ad_get_snap_serial_number( msgMachineStatistics_t* pstuMachineStatistics) = 0;
        virtual int Ad_get_snap_result(msgSnapResultBoardcast *snapresultitem) = 0;
        virtual int Ad_set_osd_info(av_video_encoder_para_t * pVideo_para, int chn, rm_uint16_t usOverlayBitmap) = 0;
        int Ad_start_selected_encoder(unsigned long long int chn_mask, av_video_encoder_para_t *pVideo_para = NULL, av_audio_encoder_para_t *pAudio_para = NULL, av_video_stream_type_t video_stream_type = _AST_UNKNOWN_, av_audio_stream_type_t audio_stream_type = _AAST_UNKNOWN_);
        CAvEncoder* Ad_get_AvEncoder(){return m_pAv_encoder;}
        int Ad_update_common_osd_content(av_video_stream_type_t stream_type, int phy_video_chn_num, av_osd_name_t osd_name, char osd_content[], unsigned int osd_len);
 #ifdef _AV_PRODUCT_CLASS_IPC_
        int Ad_update_extra_osd_content(av_video_stream_type_t stream_type, int phy_video_chn_num, av_osd_name_t osd_name, char osd_content[], unsigned int osd_len);
        virtual int Ad_snap_clear_all_pictures() = 0;
#if defined(_AV_SUPPORT_IVE_)
        virtual int Ad_start_ive() = 0;
        virtual int Ad_stop_ive() = 0;
        virtual int Ad_set_ive_osd(char * osd) = 0;
        virtual int Ad_notify_bus_front_door_state(int state) = 0;
        virtual int Ad_register_ive_detection_result_notify_callback(boost::function< void (msgIPCIntelligentInfoNotice_t *)> notify_func) = 0;
        virtual int Ad_unregister_ive_detection_result_notify_callback() = 0;
        virtual int Ad_update_blacklist() = 0;
#endif
 #endif
 #ifndef _AV_PRODUCT_CLASS_IPC_
         virtual int Ad_ChangeLanguageCacheChar();
 #endif
        virtual int Ad_get_net_stream_level(unsigned int *chnmask,unsigned char chnvalue[SUPPORT_MAX_VIDEO_CHANNEL_NUM]);
#endif

    public:
        virtual int Ad_init();
        virtual int Ad_wait_system_ready(){return 0;}

 #ifdef _AV_HAVE_VIDEO_OUTPUT_
        virtual int Ad_zoomin_screen(av_zoomin_para_t av_zoomin_para) = 0;
 #endif
};


#endif/*__AVDEVICE_H__*/


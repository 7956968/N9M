/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __AVCOMMONTYPE_H__
#define __AVCOMMONTYPE_H__
#include <stdio.h>
#include <stdlib.h>
#include "Defines.h"
#include "Message.h"

#define _VS_DIGITAL_ 1
#define _VS_ANALOG_ 0

#define MAX_AUDIO_CHNNUM 8


typedef struct _av_rect_{
    int x, y, w, h;
}av_rect_t;

typedef struct _av_point_
{
    int x, y;
} av_point_t;


#define _AV_VDEC_MAX_CHN_  36
#define _AV_SPLIT_MAX_CHN_  36
#define _AV_DECODER_MODE_NUM_ 3
#if defined(_AV_PRODUCT_CLASS_IPC_)
#define _AV_VIDEO_MAX_CHN_ 2
#else
#define _AV_VIDEO_MAX_CHN_ 32
#endif
#define _AV_VIDEO_STREAM_MAX_NUM_ 5

#if defined(_AV_PRODUCT_CLASS_IPC_)
#define MAX_OSD_SPEED_SIZE  (16)
#endif

#define MAX_EXT_OSD_NAME_SIZE (64)

typedef enum _av_split_{
    _AS_SINGLE_,
    _AS_PIP_,
    _AS_DIGITAL_PIP_,
    _AS_2SPLIT_,
    _AS_3SPLIT_,
    _AS_4SPLIT_,
    _AS_6SPLIT_,
    _AS_8SPLIT_,
    _AS_9SPLIT_,
    _AS_10SPLIT_,
    _AS_12SPLIT_,
    _AS_13SPLIT_,
    _AS_16SPLIT_,
    _AS_25SPLIT_,
    _AS_36SPLIT_,
    _AS_UNKNOWN_
}av_split_t;


typedef enum _av_tvsystem_{
    _AT_PAL_,
    _AT_NTSC_,
    _AT_UNKNOWN_
}av_tvsystem_t;

typedef enum _av_videoinputformat_{
	_VIDEO_INPUT_INVALID_ = 0,
	_VIDEO_INPUT_SD_PAL_ = 1,
	_VIDEO_INPUT_SD_NTSC_ = 2,
	_VIDEO_INPUT_AHD_25P_ = 3,
	_VIDEO_INPUT_AHD_30P_ = 4,
	_VIDEO_INPUT_AHD_50P_ = 5,
	_VIDEO_INPUT_AHD_60P_ = 6,
	_VIDEO_INPUT_AFHD_25P_ = 7,
	_VIDEO_INPUT_AFHD_30P_ = 8,
}av_videoinputformat_t;


typedef enum _av_resolution_{/*for vi, venc, vdec*/
    _AR_SIZE_,
    _AR_D1_,
    _AR_HD1_,
    _AR_CIF_,
    _AR_QCIF_,
    _AR_960H_WD1_,
    _AR_960H_WHD1_,
    _AR_960H_WCIF_,
    _AR_960H_WQCIF_,
    _AR_1080_,
    _AR_720_,
    _AR_VGA_,
    _AR_QVGA_,
    _AR_960P_,
    _AR_Q1080P_, //FOR nvr 
    _AR_3M_,
    _AR_5M_,
    _AR_SVGA_,
    _AR_XGA_,
    _AR_UNKNOWN_
}av_resolution_t;


#define _VI_MAX_CHN_NUM_    32
typedef struct _vi_config_set_{
    av_resolution_t resolution;
    int w, h;/*for _AR_SIZE_*/
    bool progress;
    int frame_rate;
#if defined(_AV_PRODUCT_CLASS_IPC_)
    unsigned char u8Rotate;
#endif
	unsigned char u8Flip;
	unsigned char u8Mirror;
}vi_config_set_t;


typedef struct _av_dec_set_{
    int av_dec_purpose;
    int chnlist[_AV_SPLIT_MAX_CHN_];
    int chnnum;
    unsigned int chnbmp;
    av_split_t split_mode;
    int smi_size;
    int smi_framenum;
    char smi_name[32];
}av_dec_set_t;


typedef enum _av_output_type_{
    _AOT_MAIN_,
    _AOT_SPOT_,
    _AOT_CVBS_,
}av_output_type_t;


typedef enum _av_vga_hdmi_resolution_{/*for vo*/
    _AVHR_1080p24_,
    _AVHR_1080p25_,
    _AVHR_1080p30_,
    _AVHR_720p50_,
    _AVHR_720p60_,
    _AVHR_1080i50_,
    _AVHR_1080i60_,
    _AVHR_1080p50_,
    _AVHR_1080p60_,
    _AVHR_576p50_,
    _AVHR_480p60_,
    _AVHR_800_600_60_,
    _AVHR_1024_768_60_,
    _AVHR_1280_1024_60_,
    _AVHR_1366_768_60_,
    _AVHR_1440_900_60_,
    _AVHR_1280_800_60_,    
    _AVHR_UNKNOWN_
}av_vag_hdmi_resolution_t;


typedef enum _ai_type_{
    _AI_NORMAL_ = 0,
    _AI_TALKBACK_,
} ai_type_e;


typedef enum _audio_output_type_{
    _AO_HDMI_ = 0,
    _AO_TALKBACK_,
    _AO_PLAYBACK_CVBS_,
    _AO_REPORT_,
    _AO_TTS_,
} ao_type_e;


/********************************************************************/
/*av encoder*/
//////////////////////////////////////////////////////////////////
/*audio*/
typedef enum _av_audio_stream_type_{
    _AAST_NORMAL_,/*for main stream, sub stream and so on*/
    _AAST_TALKBACK_,/*talkback*/
    _AAST_UNKNOWN_
}av_audio_stream_type_t;


typedef enum _audio_encoder_type_{
    _AET_ADPCM_,
    _AET_G711A_,
    _AET_G711U_,
    _AET_G726_,
    _AET_AMR_,          //!< Mar. 19 2015
    _AET_AAC_,		//!<Feb. 22 2016
    _AET_LPCM_,		//!<April, 2 2016, current only used for plane_project
    _AET_UNKNOWN_
}audio_encoder_type_t;


typedef struct _av_audio_encoder_para_{
    audio_encoder_type_t type;
}av_audio_encoder_para_t;
//////////////////////////////////////////////////////////////////
/*video*/
typedef enum _av_video_stream_type_{
    _AST_MAIN_VIDEO_,
    _AST_SUB_VIDEO_,
    _AST_SNAP_VIDEO_,
    _AST_SUB2_VIDEO_, //IPC��Ҫ��������ͬʱ����
    _AST_UNKNOWN_,
#if 1
    _AST_MAIN_IFRAME_VIDEO_,
#endif    
}av_video_stream_type_t;


typedef enum _av_bitrate_mode_{
    _ABM_CBR_,
    _ABM_VBR_,
    _ABM_UNKNOWN_
}av_bitrate_mode_t;

typedef enum _av_venc_profile_
{
    _AVP_BP_,
    _AVP_MP_,
    _AVP_HP_,
} av_venc_profile_t;


typedef enum _av_ref_type_{
    _ART_REF_FOR_1X = 0,
    _ART_REF_FOR_2X,
    _ART_REF_FOR_4X,
    _ART_REF_FOR_CUSTOM,
    _ART_REF_FOR_BULT
}av_ref_type_t;

typedef enum _av_video_type_{
    _AVT_H264_,
    _AVT_H265_,
    _AVT_JPEG_,
    _AVT_MJPEG_,
    _AVT_MPEG4_,
    
    _AVT_UNKNOWN_
}av_video_type_t;

typedef struct _av_ref_para_{
    /*ref type*/    
    av_ref_type_t ref_type;/*��֡�ο���ģʽ*/
    unsigned int pred_enable;/*���� base ���֡�Ƿ� base ������֡�����ο���1-base�������֡���ο� IDR ֡*/
    unsigned int base_interval;/*base�������*/
    unsigned int enhance_interval;/*ehance�������*/
}av_ref_para_t;

#define OVERLAY_MAX_NUM_VENC 8

typedef struct _av_video_encoder_para_{
    /*for frame header*/
    int virtual_chn_num;
    /*tv system*/
    av_tvsystem_t tv_system;
    /*ref para*/
    av_ref_para_t ref_para;
    /*resolution*/
    av_resolution_t resolution;
    int resolution_w;int resolution_h;/*only used for _AR_SIZE_*/
    /*frame rate*/
    int frame_rate;
    /*gop size*/
    int gop_size;
    /*bit rate mode*/
    av_bitrate_mode_t bitrate_mode;
    unsigned char bitrate_level;
    unsigned char net_transfe_mode;
    /*bit rate*/
    union _bitrate_{
        struct _hicbr_{
            int bitrate;
        }hicbr;/*for hisilicon cbr*/
        struct _hivbr_{
            int bitrate;
        }hivbr;/*for hisilicon vbr*/
    }bitrate;
    /*have audio*/
    int have_audio;
	int record_audio;/*�Ƿ�¼����Ƶ0:��¼1:¼GK��Ŀ��Ҫ*/
    /*date time osd*/
    int have_date_time_osd;
#define _AV_DATE_MODE_yymmdd_   0
#define _AV_DATE_MODE_ddmmyy_   1
#define _AV_DATE_MODE_mmddyy_   2
    int date_mode;
#define _AV_TIME_MODE_24_   0
#define _AV_TIME_MODE_12_   1
    int time_mode;
    int date_time_osd_x;
    int date_time_osd_y;
    /*channel name osd*/
    int have_channel_name_osd;
    int channel_name_osd_x;
    int channel_name_osd_y;
    char channel_name[MAX_OSD_NAME_SIZE];
    int source_frame_rate;

    int have_bus_number_osd;
    int bus_number_osd_x;
    int  bus_number_osd_y;
    char bus_number[MAX_OSD_NAME_SIZE];
    int have_gps_osd;
    int gps_osd_x;
    int gps_osd_y;
#if defined(_AV_PRODUCT_CLASS_IPC_)
    char gps[MAX_OSD_NAME_SIZE];
#endif

    int have_speed_osd;
    int speed_osd_x;
    int speed_osd_y;
#if defined(_AV_PRODUCT_CLASS_IPC_)
    char speed[MAX_OSD_SPEED_SIZE];
#endif
    unsigned char speed_unit;/*0:KM/H   1:MPH*/
    int have_common1_osd;
    unsigned char osd1_contentfix;/*�����Ƿ�̶�0:���̶� 1:�̶�*/
    unsigned char osd1_content_byte;/*������ǹ̶�ģʽ,�ж��ٸ��ֽ���*/
    char osd1_content[MAX_EXT_OSD_NAME_SIZE];
    int have_common2_osd;
    unsigned char osd2_contentfix;/*�����Ƿ�̶�0:���̶� 1:�̶�*/
    unsigned char osd2_content_byte;/*������ǹ̶�ģʽ,�ж��ٸ��ֽ���*/
    char osd2_content[MAX_EXT_OSD_NAME_SIZE];
    int common1_osd_x;
    int common1_osd_y;
    int common2_osd_x;
    int common2_osd_y;   
    bool have_water_mark_osd;
    char water_mark_name[MAX_OSD_NAME_SIZE];
    int  water_mark_osd_x;
    int  water_mark_osd_y;

    int have_bus_selfnumber_osd;
    int bus_selfnumber_osd_x;
    int  bus_selfnumber_osd_y;
    char bus_selfnumber[MAX_OSD_NAME_SIZE];
	unsigned char osd_transparency;      /*OSD ͸���ȣ�1-128*/
    int  i_frame_record;/*�Ƿ�I ֡¼��0:�� 1:��*/

#if defined(_AV_PRODUCT_CLASS_IPC_)
    av_video_type_t video_type;
#endif
#if (defined(_AV_PLATFORM_HI3520D_V300_)||defined(_AV_PLATFORM_HI3515_))	/*6AII_AV12����ר��osd*/
	OsdItem_t stuExtendOsdInfo[OVERLAY_MAX_NUM_VENC];	/*8��osd����������*/
	unsigned char is_osd_change[OVERLAY_MAX_NUM_VENC];
#endif
}av_video_encoder_para_t;

typedef struct _av_zoomin_para_{
    bool enable;/*enable zoom func, 0-disable,1-enable*/
    int videophychn;/*video phy chn num*/
    av_rect_t stuMainPipArea;/*vga pip win's pos*/
    av_rect_t stuSlavePipArea;/*cvbs pip win's pos*/
    av_rect_t stuZoomArea;/*zoom area*/
}av_zoomin_para_t;


typedef struct _av_pip_para_{
    bool enable;/*enable zoom func, 0-disable,1-enable*/
    int video_phychn[2];/*0-main video phy chn num; 1-sub*/
    av_rect_t stuMainPipArea;/*vga pip win's pos*/
    av_rect_t stuSlavePipArea;/*cvbs pip win's pos*/
}av_pip_para_t;

typedef struct _av_video_image_para_{
    int luminance;
    int contrast;
    int dark_enhance;
    int bright_enhance;
    int ie_strength;
    int ie_sharp;
    int sf_strength;
    int tf_Strength;
    int motion_thresh;
    int di_strength;
    int chromaRange;
    int nr_wf_or_tsr;
    int sf_window;
    int dis_mode;
}av_video_image_para_t;

//////////////////////////////////////////////////////////////////
/********************************************************************/


/********************************************************************/
typedef unsigned long av_ucs4_t;

typedef enum _av_osd_name_{
    _AON_DATE_TIME_,
    _AON_CHN_NAME_,
    _AON_BUS_NUMBER_,
    _AON_GPS_INFO_,
    _AON_SPEED_INFO_,
    _AON_SELFNUMBER_INFO_,
    _AON_COMMON_OSD1_, 
    _AON_COMMON_OSD2_, 
    _AON_WATER_MARK,
    _AON_EXTEND_OSD,
}av_osd_name_t;
/********************************************************************/


/**********************************************************************/
typedef enum _av_sys_pic_type_
{
    _ASPT_VIDEO_LOST_ = 0,
    _ASPT_CUSTOMER_LOGO_,
    _ASPT_ACCESS_LOGO_,
    _ASPT_MAX_PIC_TYPE_
} av_sys_pic_type_e;

/*********************************************************************/
typedef struct _preview_audio_ctrl_
{
    int phy_chn;
    bool audio_start;
    bool thread_start;
    bool thread_exit;
	bool bAudioPauseState; //true-paused
    ai_type_e ai_type;
} preview_audio_ctrl_t;

typedef struct _audio_output_state_
{
    int adec_chn;
    bool audio_mute;
} audio_output_state_t;

/********************************************************************/
#define MAX_VDA_CHN_ON_SLAVE 32
typedef enum _vda_type_
{
    _VDA_MD_ = 0,
    _VDA_OD_,
    _VDA_MAX_TYPE_,
} vda_type_e;

typedef enum _start_stop_md_mode_
{
    _START_STOP_MD_BY_SWITCH_ = 0,
    _START_STOP_MD_BY_MESSAGE_,
} start_stop_md_mode_e;

typedef struct _vda_chn_param_
{
    unsigned char enable;
    unsigned char sensitivity;
    unsigned char region[50];
}vda_chn_param_t;

typedef struct _av_stream_buffer_info_
{
    char buf_name[32];
    unsigned int buf_size;
    unsigned int buf_cnt;
}av_stream_buffer_info_t;

typedef struct tag_AvSnapPara{
    rm_uint32_t uiSnapType;			    /*ץ��������  0:�ֶ�ץ��  1:����ץ��  2:��ʱץ��  3:����ץ�� 4:���������汾�����·���ʱץ�� 5:��ʷץ��(��flash����)*/
    rm_uint32_t uiSnapTaskNumber;	    /*ץ�������,ͳһ��event ģ�����,���ڷ���ץ�Ľ��ʱ��Ψһ����*/
    rm_uint32_t uiChannel;                 //ץ��ͨ����ʹ�ñ���λ��ʾ��
    rm_uint64_t ullSubType;			     /* ץ�������ͣ����������Ͳ�ͬ����ͬ����λ��ʾ��
                                            * �ֶ�ץ��:bit0--�����·���bit1--����
                                            * ����ץ��:���屨�����ͣ����enumAlarmTypeBit
                                            * ��ʱץ��:bit0--�����·���ʱ��bit1--���ض�ʱ
                                            * ����ץ��:bit0--�����·����࣬bit1--���ض���
                                            * ��ʷץ��:bit0--����ģ���·���
                                            */
    rm_uint8_t  ucResolution;           //ץ�ķֱ��� /**< �ֱ��� 0-CIF 1-HD1 2-D1 3-QCIF 4-QVGA 5-VGA 6-720P 7-1080P 8-3MP(2048*1536) 9-5MP(2592*1920) 10 WQCIF��11 WCIF��12 WHD1��13 WD1(960H) 14-960P 15-Q1080P*/
    rm_uint8_t  ucQuality;              //ץ�Ļ���/**< ������1-���� 2-�� 3-�� 4-�� 8-���*/
    rm_uint16_t usNumber;               //һ��ץ�Ķ�����ץͼƬ����(ץ�ļ��Ϊ0ʱʹ��)
    rm_uint32_t uiUser;                 //ץ��ͼƬ��;����λ��ʾ��bit0:���ش洢 bit1:ftp�ϴ� bit2:ͨ��Э���ϴ�
    rm_uint16_t  usOverlayBitmap;       //bit0:ʱ�� bit1:���� bit2:���� bit3:GPS��Ϣ bit4:ͨ������ bit5:�Ա�� bit6:�û��Զ���1 bit7:�û��Զ���2
    rm_uint8_t  ucPrivateDataType;          /*˽����������: 0 ��׼�汾��1 ���⳵��2 ������3 ���ˣ�4 У����5 ����*/
    rm_uint8_t ucReserved[1];
    SnapSharePara_t stuSnapSharePara;   /*ץ�ĵ�����Ϣ��˽�����ݣ�������Ϣֻ�����Զ��岿��*/
    msgTime_t       stuSnapTime;         /*ץ��ʱ���*/
    rm_uint8_t      ucSnapMode;          /*����ʱ��� T ��ץ��ģʽ��0--ץ3�ţ�T-1��T��T+1��1--ץ3�ţ�T-2��T-1��T��*/
    rm_uint8_t ucReserved1[7];
        
}AvSnapPara_t;
#endif/*__AVCOMMONTYPE_H__*/


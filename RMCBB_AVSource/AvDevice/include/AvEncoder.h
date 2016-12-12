/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __AVENCODER_H__
#define __AVENCODER_H__
#include "AvConfig.h"
#include "AvDebug.h"
#include "AvCommonMacro.h"
#include "AvCommonType.h"
#include "AvDeviceInfo.h"
#include "StreamBuffer.h"
#include "SystemTime.h"
#include "AvThreadLock.h"
#include "AvCommonFunc.h"
#include "Defines.h"
#include "AvUtf8String.h"
#include "AvFontLibrary.h"
#include "System.h"
#include <map>
#include <list>
#include <bitset>
#include <vector>
#include <queue>
#ifndef _AV_PRODUCT_CLASS_IPC_
#include "GuiTKi18n.h"
#endif
#include "CommonDebug.h"

using namespace Common;
//////////////////////////////////////////////////////////////////
typedef enum _av_thread_control_{
    _ATC_REQUEST_RUNNING_,
    _ATC_REQUEST_PAUSE_,
    _ATC_REQUEST_EXIT_,
    _ATC_REQUEST_UNKNOWN_,

    _ATC_RESPONSE_RUNNING_,
    _ATC_RESPONSE_PAUSED_,
    _ATC_RESPONSE_EXITED_,
    _ATC_RESPONSE_UNKNOWN_
}av_thread_control_t;


typedef enum _encoder_thread_type_{
    _ETT_STREAM_REC_,
    _ETT_STREAM_SEND_,
    _ETT_OSD_OVERLAY_
}encoder_thread_type_t;


typedef struct _video_encoder_id_{/*the description of video encoder(hardware)*/
    int video_encoder_fd;
    void *pVideo_encoder_info;/*depend on platform*/
}video_encoder_id_t;

#define _AV_FRAME_HEADER_BUFFER_LEN_    256

#if defined(_AV_PLATFORM_HI3516A_) || defined(_AV_PLATFORM_HI3518EV200_)
#define _AV_AUDIO_PTNUMPERFRAME_  (320)
#endif

typedef struct _av_video_stream_{
    /*stream type & video channel number(bnc channel number), they make up the ID of encoder for application*/
    av_video_stream_type_t type;
    int phy_video_chn_num;
    /*encoder parameter*/
    av_video_encoder_para_t video_encoder_para;
    /*encoder id(hardware)*/
    video_encoder_id_t video_encoder_id;
    /*stream buffer(not only for video stream, also for audio stream)*/
    HANDLE stream_buffer_handle;
    HANDLE iframe_stream_buffer_handle;/*I ?buffer*/
    /**/
    unsigned char *pFrame_header_buffer;
	
    /**/
    bool first_frame;
    /**/
    int video_frame_id;
	
	/*vbr wave level:0:high rate1:mid rate2:low rate*/
	unsigned char vbr_wave_level;
	/*number of times*/
	unsigned int high_count_number_percent;
	unsigned int middle_count_number_percent;
	unsigned int low_count_number_percent;
	unsigned int count_total_rate;
	unsigned int count_total_frame;
}av_video_stream_t;


typedef struct _audio_encoder_id_{/*the description of audio encoder(hardware)*/
    int audio_encoder_fd;
    void *pAudio_encoder_info;/*depend on platform*/
}audio_encoder_id_t;

#if defined(_AV_PLATFORM_HI3515_)
#define OSD_CACHE_ITEM_NUM		20
#define OSD_CACHE_FONT_LIB_NUM		2

typedef struct _osd_cache_info_
{
	unsigned char	hScale;
	unsigned char	vScale;
	unsigned short	width;
	unsigned short	height;
	unsigned short*			szOsd;
}osd_cache_info_t;
#endif

typedef struct _av_audio_stream_info_{
    /*stream type & audio channel number(bnc channel number), they make up the ID of encoder for application*/
    av_audio_stream_type_t type;
    int phy_audio_chn_num;
    /*encoder parameter*/
    av_audio_encoder_para_t audio_encoder_para;
    /*encoder id(hardware)*/
    audio_encoder_id_t audio_encoder_id;
    /*stream buffer, only for _AAST_TALKBACK_*/
    HANDLE stream_buffer_handle;
    /**/
    unsigned char *pFrame_header_buffer;

}av_audio_stream_t;


/*if we(video encoder) share the audio encoder, we must be in the same group(also in the same thread)*/
typedef struct _av_audio_video_group_{
    av_audio_stream_t audio_stream;/*only one or no audio stream*/
    std::list<av_video_stream_t> video_stream;/*no(talkback), one or more video stream*/
}av_audio_video_group_t;


typedef struct _av_recv_thread_task_{
    /*thread control*/
    av_thread_control_t thread_request;
    av_thread_control_t thread_response;
    /*thread state*/
    int thread_running;
    /*the tasks below share the thread*/
    std::list<av_audio_video_group_t> local_task_list;
}av_recv_thread_task_t;
//////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////
typedef enum _av_osd_state_{
    _AOS_INIT_,
    _AOS_CREATED_,
    _AOS_OVERLAYED_,
    _AOS_FONT_TRANSFER_OK_,
    _AOS_UNKNOWN_,
}av_osd_state_t;


typedef struct _av_osd_item_{
    av_video_stream_t video_stream;
    av_osd_name_t osd_name;
    av_osd_state_t overlay_state;
#if defined(_AV_PRODUCT_CLASS_IPC_)
    bool  is_update;//this flag is used to mark the field is updated
#endif
#if defined(_AV_PLATFORM_HI3515_)
	unsigned int osd_mask; //!< enable for each line osd, osd1:bit 0 chn,bit1 datatime, bit2 speed ; osd2:bit0 gps, bit1 bus num,bit2 selfnum
#endif
    /*coordinate*/
    int display_x;
    int display_y;
    /*bitmap*/
    int bitmap_width;
    int bitmap_height;
    unsigned char *pBitmap;
    /*region*/
    void *pOsd_region;
#if (defined(_AV_PLATFORM_HI3520D_V300_)||defined(_AV_PLATFORM_HI3515_))
	rm_uint8_t extend_osd_id;	/*扩展osd区域id标识*/
	rm_uint8_t osd_region_have_created;	//6AI机型中，标识此区域是否已被创建，用于只有叠加内容修改///
	rm_uint8_t osdChange;	/*osd区域内容是否发生*/
#endif
}av_osd_item_t;

typedef struct _av_osd_thread_task_{
    /*thread control*/
    av_thread_control_t thread_request;
    av_thread_control_t thread_response;
    CAvThreadLock * thread_lock;
    /*thread state*/
    int thread_running;
    /*osd*/
    std::list<av_osd_item_t> osd_list;

    _av_osd_thread_task_()
    {
        thread_lock = new CAvThreadLock();
    }

    ~_av_osd_thread_task_()
    {
        _AV_SAFE_DELETE_(thread_lock);
    }
    
}av_osd_thread_task_t;
//////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////
typedef enum _av_frame_type_{
    _AFT_IFRAME_,
    _AFT_PFRAME_,
    _AFT_AUDIO_FRAME_
}av_frame_type_t;


#define _AV_MAX_PACK_NUM_ONEFRAME_   10
typedef struct _av_stream_data_{
    /*frame information*/
    av_frame_type_t frame_type;
    int frame_len;
    long long pts;
    /*data*/
    int pack_number;
    struct _pack_{
        unsigned char *pAddr[2];
        int len[2];
    }pack_list[_AV_MAX_PACK_NUM_ONEFRAME_];
}av_stream_data_t;

/*color inversion not enable, so we use 4 byte alignment*/
#if defined(_AV_PLATFORM_HI3518C_)
	#define _AE_OSD_ALIGNMENT_BYTE_ 4
#elif defined(_AV_PLATFORM_HI3515_)
	#define _AE_OSD_ALIGNMENT_BYTE_ 8
#else
	#define _AE_OSD_ALIGNMENT_BYTE_ 4
#endif

typedef struct _code_rate_count_
{
	int beforesecondrate;/*上一秒码率:单位kbps*/
	int currentrate;	  /* 当前秒码率:单位kbps*/
}code_rate_count;

//////////////////////////////////////////////////////////////////


class CAvEncoder{
    public:
        CAvEncoder();
        virtual ~CAvEncoder();

    public:
        /*composite encoder*/
        int Ae_start_encoder(av_video_stream_type_t video_stream_type = _AST_UNKNOWN_, int phy_video_chn_num = -1, av_video_encoder_para_t *pVideo_para = NULL, av_audio_stream_type_t audio_stream_type = _AAST_UNKNOWN_, int phy_audio_chn_num = -1, av_audio_encoder_para_t *pAudio_para = NULL, int audio_only = -1);
        int Ae_stop_encoder(av_video_stream_type_t video_stream_type = _AST_UNKNOWN_, int phy_video_chn_num = -1, av_audio_stream_type_t audio_stream_type = _AAST_UNKNOWN_, int phy_audio_chn_num = -1, int normal_audio = -1);
        int Ae_dynamic_modify_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, bool bosd_need_change = false);
        int Ae_request_iframe(av_video_stream_type_t video_stream_type, int phy_video_chn_num);
        int Ae_get_encoder_used_resource(int *pCif_count);
        bool Ae_whether_encoder_have_this_resolution( av_resolution_t eResolution, int *pResolutionNum = NULL);
        int Ae_get_net_stream_level(unsigned int *chnmask,unsigned char chnvalue[SUPPORT_MAX_VIDEO_CHANNEL_NUM]);
        int Ae_start_talkback_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, av_audio_encoder_para_t *pAudio_para, bool thread_control);
        int Ae_stop_talkback_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, bool thread_control);
#ifndef _AV_PRODUCT_CLASS_IPC_ 
        int ChangeLanguageCacheOsdChar();
#endif
    public:
        int Ae_encoder_recv_thread(int task_id);
        int Ae_encoder_send_thread(int task_id);
        int Ae_osd_thread(int task_id);
        inline void Ae_set_video_lost_flag(unsigned int video_loss_bmp) { std::bitset<64> temp(video_loss_bmp); m_video_lost_flag = temp;}
        inline int Ae_get_video_lost_flag(int video_chn) { return m_video_lost_flag[video_chn]; }
    private:
        /*video encoder*/
        virtual int Ae_create_video_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, video_encoder_id_t *pVideo_encoder_id) = 0;
        virtual int Ae_destory_video_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, video_encoder_id_t *pVideo_encoder_id) = 0;
        /*audio encoder*/
        virtual int Ae_create_audio_encoder(av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, av_audio_encoder_para_t *pAudio_para, audio_encoder_id_t *pAudio_encoder_id) = 0;
        virtual int Ae_destory_audio_encoder(av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, audio_encoder_id_t *pAudio_encoder_id) = 0;
        /*get data*/
        virtual int Ae_get_audio_stream(audio_encoder_id_t *pAudio_encoder_id, av_stream_data_t *pStream_data, bool blockflag = true) = 0;
        virtual int Ae_release_audio_stream(audio_encoder_id_t *pAudio_encoder_id, av_stream_data_t *pStream_data) = 0;
        virtual int Ae_get_video_stream(video_encoder_id_t *pVideo_encoder_id, av_stream_data_t *pStream_data, bool blockflag = true) = 0;
        virtual int Ae_release_video_stream(video_encoder_id_t *pVideo_encoder_id, av_stream_data_t *pStream_data) = 0;
        /**/
        virtual int Ae_request_iframe_local(av_video_stream_type_t video_stream_type, int phy_video_chn_num) = 0;
        virtual int Ae_init_system_pts(unsigned long long pts) = 0;
        /**/
#if defined(_AV_PLATFORM_HI3515_)
		virtual int Ae_create_osd_region(std::list<av_osd_item_t> *av_osd_item_list) = 0;
		virtual int Ae_osd_overlay(std::list<av_osd_item_t> &av_osd_item_list) = 0;
#else
        virtual int Ae_create_osd_region(std::list<av_osd_item_t>::iterator &av_osd_item_it) = 0;
        virtual int Ae_osd_overlay(std::list<av_osd_item_t>::iterator &av_osd_item_it) = 0;
#endif
        virtual int Ae_destroy_osd_region(std::list<av_osd_item_t>::iterator &av_osd_item_it) = 0;
        /**/
        virtual int Ae_dynamic_modify_encoder_local(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para) = 0;

#if defined(_AV_PLATFORM_HI3520D_V300_)	/*6AII_AV12机型专用osd叠加*/
		virtual int Ae_create_extend_osd_region(std::list<av_osd_item_t>::iterator &av_osd_item_it) = 0;
		virtual int Ae_extend_osd_overlay(std::list<av_osd_item_t>::iterator &av_osd_item_it) = 0;
#endif

        /*get h264 info for watermark*/
        virtual int Ae_get_H264_info(const void *pVideo_encoder_info, RMFI2_H264INFO * ph264_info) = 0;
#if defined(_AV_PLATFORM_HI3516A_)
        /*get h265 info*/
        virtual int Ae_get_H265_info(const void *pVideo_encoder_info, RMFI2_H265INFO* pH265_info) = 0;
#endif
    public:
        /*thread*/
        int Ae_get_task_id(av_video_stream_type_t video_stream_type, int phy_video_chn_num, int *pRecv_task_id = NULL, int *pSend_task_id = NULL);
        int Ae_get_task_id(av_audio_stream_type_t audio_stream_type = _AAST_TALKBACK_, int phy_audio_chn_num = -1, int *pRecv_task_id = NULL, int *pSend_task_id = NULL);
        int Ae_thread_control(encoder_thread_type_t task_type, int task_id, av_thread_control_t request);
        CAvThreadLock* Ae_get_AvThreadLock(){return m_pThread_lock;};
        av_recv_thread_task_t* Ae_get_av_recv_thread_task_t(){return m_pRecv_thread_task;}
        av_osd_thread_task_t* Ae_get_av_osd_thread_task_t(){return m_pOsd_thread;}
        int Ae_get_osd_content(int phy_video_chn_num, av_osd_name_t osd_name, char osd_content[],unsigned int osd_len);
        int Ae_get_osd_parameter(av_osd_name_t osd_name, av_resolution_t resolution, int *pFont_name, int *pWidth, int *pHeight, int *pHScaler, int *pVScaler, int regionId = 0);
        int Ae_update_common_osd_content(av_video_stream_type_t  stream_type, int phy_video_chn_num, av_osd_name_t osd_name, char osd_content[], unsigned int osd_len);
 #if defined(_AV_PRODUCT_CLASS_IPC_)       
        int Ae_update_osd_content(av_video_stream_type_t  stream_type, int phy_video_chn_num, av_osd_name_t osd_name, char osd_content[], unsigned int osd_len);
 #endif

 #if defined(_AV_PLATFORM_HI3515_)
 		int Ae_get_osd_string_width(const char* szOsdName, int fontName, int horScale, bool align = true);
 #endif
    private:
        /*stream buffer*/
        HANDLE Ae_create_stream_buffer(av_audio_stream_type_t audio_stream_type = _AAST_TALKBACK_, int phy_audio_chn_num = -1, EnumSharememoryMode mode = SHAREMEM_WRITE_MODE);
        int Ae_destory_stream_buffer(HANDLE Stream_buffer_handle, av_audio_stream_type_t audio_stream_type = _AAST_TALKBACK_, int phy_audio_chn_num = -1);
        HANDLE Ae_create_stream_buffer(av_video_stream_type_t video_stream_type, int phy_video_chn_num, EnumSharememoryMode mode = SHAREMEM_WRITE_MODE);
        int Ae_destory_stream_buffer(HANDLE Stream_buffer_handle, av_video_stream_type_t video_stream_type, int phy_video_chn_num);    
        /*local encoder*/
        int Ae_start_encoder_local(av_video_stream_type_t video_stream_type = _AST_UNKNOWN_, int phy_video_chn_num = -1, av_video_encoder_para_t *pVideo_para = NULL, av_audio_stream_type_t audio_stream_type = _AAST_UNKNOWN_, int phy_audio_chn_num = -1, av_audio_encoder_para_t *pAudio_para = NULL, int audio_only = -1);
        int Ae_stop_encoder_local(av_video_stream_type_t video_stream_type = _AST_UNKNOWN_, int phy_video_chn_num = -1, av_audio_stream_type_t audio_stream_type = _AAST_UNKNOWN_, int phy_audio_chn_num = -1, int normal_audio = -1);
        int Ae_dynamic_modify_local_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t * pVideo_para, bool bosd_need_change);

#if (defined(_AV_PLATFORM_HI3520D_V300_) || defined(_AV_PLATFORM_HI3515_))	/*6AII_AV12机型osd叠加专用*/
        int Ae_dynamic_modify_extend_osd_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t * pVideo_para, bool bosd_need_change);
#endif
        /*encoder*/
        int Ae_start_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, av_audio_encoder_para_t *pAudio_para, bool thread_control, int audio_only = -1);
        int Ae_stop_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, bool thread_control, int normal_audio = -1);
        /**/
        unsigned char Ae_calculate_stream_CheckSum(av_stream_data_t *pAv_stream_data);
        int Ae_put_video_stream(std::list<av_video_stream_t>::iterator &av_video_stream, av_audio_stream_t *pAudio_stream, av_stream_data_t *pAv_stream_data, SShareMemData &share_mem_data);
        int Ae_put_audio_stream(av_audio_stream_t *pAudio_stream, av_stream_data_t *pAv_stream_data, SShareMemData &share_mem_data);
        int Ae_construct_audio_frame(av_stream_data_t *pAv_stream_data, int frame_header_chn, av_audio_encoder_para_t *pAudio_para, int have_audio, unsigned char *pFrame_header_buffer, int *pFrame_header_len, datetime_t& datetime);        
        /**/
        int Ae_init_system_pts();
  public:
        int CacheOsdFond( const char* pszStr, bool bosd_thread_control_flag = true);
  private:    
        /*
	*@breif:this function is to detect the regin is compared with the new regin before.
	*@param:point [in] : the regin coordinate
	 *				 passed_path[in]:the path the new regin passed
	 *@return: if passed return true, else return false.
	*/
    bool Ae_is_regin_passed(const av_osd_item_t &regin, const std::vector<av_osd_item_t> &passed_path) const;
    /*
	*@breif:this function is to detect the new regin is overlaped with the exist regins.
	*@param:rect [in] : the regin coordinate
	 *				 overlap_regin[out]: if not NULL it will return the overlap regin.
	 *@return: if overlap return true, else return false.
	*/
	bool Ae_is_regin_overlap_with_exist_regins(const av_osd_item_t &regin, av_osd_item_t *overlap_regin) const;
	
	/*
	*@breif:this function is to find a valid coordinate for the overlap regin
	*@param:osd_item [in] : the regin info
	 *@return: if succcess return 0.
	*/	
	int Ae_get_overlay_coordinate(av_osd_item_t &osd_item, int video_width, int video_height);
	/*
	*@breif:this function is to find a valid coordinate for the overlap regin in possible path
	*@param:point [out] : the final coodinate of overlaying regin
	*					passed_path[in]:the passed path
	*				possible_path:[in]:the possible path
	 *@return: if succcess return 0.		
	*/
        int Ae_search_overlay_coordinate(av_osd_item_t &osd_item, std::vector<av_osd_item_t> &passed_path, std::queue<av_osd_item_t> &possible_path,int video_width, int video_height);
    protected:
        int Ae_utf8string_2_bmp(av_osd_name_t eOsdName, av_resolution_t eResolution, unsigned char* pszBitmapAddr, unsigned short u16BitMapWidth, unsigned short u16BitmapHeight
                                    , char *pUtf8_string, unsigned short text_color, unsigned short bg_color, unsigned short edge_color);
        //int Ae_get_osd_parameter(av_osd_name_t osd_name, av_resolution_t resolution, int *pFont_name, int *pWidth, int *pHeight, int *pHScaler, int *pVScaler);
        int Ae_translate_coordinate(std::list<av_osd_item_t>::iterator &av_osd_item_it, int osd_width, int osd_height);
#if defined(_AV_PRODUCT_CLASS_IPC_)
        int Ae_translate_coordinate_for_cnr(av_osd_item_t &osd_item, int osd_width);
#endif

    private:
        /*if we aren't allowed to destory the stream buffer when destory the encoder, we must backup the stream buffer*/
        std::map<av_video_stream_type_t, std::map<int, HANDLE> > m_video_stream_buffer;
        std::map<av_audio_stream_type_t, std::map<int, HANDLE> > m_audio_stream_buffer;

        std::map<av_video_stream_type_t, std::map<int, HANDLE> > m_read_video_stream_buffer;
        std::map<av_audio_stream_type_t, std::map<int, HANDLE> > m_read_audio_stream_buffer;

        bool m_locked_video_stream_buffer[_AST_MAIN_IFRAME_VIDEO_ + 1][32];

    private:
        CAvThreadLock *m_pThread_lock;
        std::bitset<64> m_video_lost_flag;
        av_recv_thread_task_t *m_pRecv_thread_task;
        
    private:
        av_osd_thread_task_t *m_pOsd_thread;
 #if defined(_AV_PLATFORM_HI3515_)       
        osd_cache_info_t	m_OsdCache[OSD_CACHE_ITEM_NUM][OSD_CACHE_FONT_LIB_NUM][_AR_CIF_ + 1][_AV_FONT_NAME_16_ + 1];	/*缓存特殊字符，包括: 0123456789 - /*/
		std::map<unsigned long, unsigned char>		m_OsdCharMap;
		std::map<unsigned short, unsigned char>		m_OsdFontLibMap;
#endif		
    protected:
        CAvFontLibrary *m_pAv_font_library;
        code_rate_count m_code_rate_count[SUPPORT_MAX_VIDEO_CHANNEL_NUM][3]; /*通道+  码流类型*/ 
};



#endif/*__AVENCODER_H__*/

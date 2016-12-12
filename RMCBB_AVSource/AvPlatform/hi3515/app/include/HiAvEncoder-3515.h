/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __HIAVENCODER3515_H__
#define __HIAVENCODER3515_H__
#include "AvConfig.h"
#if defined(_AV_HAVE_VIDEO_INPUT_)
#include "HiVi-3515.h"
#endif
#if defined(_AV_HISILICON_VPSS_)
#include "HiVpss.h"
#endif
#if defined(_AV_HAVE_AUDIO_INPUT_)
#include "HiAi-3515.h"

#include<pthread.h>

#endif
#include "AvEncoder.h"
#include "mpi_aenc.h"
#include "mpi_venc.h"
#include "HiAvRegion-3515.h"
#include "hi_common_extend.h"
#include "mpi_vpp.h"
typedef struct _hi_video_encoder_info_{
    int pack_number;
    VENC_PACK_S *pPack_buffer;
    VENC_STREAM_S venc_stream;
    VENC_GRP venc_grp;
    VENC_CHN venc_chn;
}hi_video_encoder_info_t;

typedef HI_U32 RGN_HANDLE;

typedef struct _hi_audio_encoder_info_{
    AUDIO_STREAM_S audio_stream;
    AENC_CHN aenc_chn;
}hi_audio_encoder_info_t;


typedef struct _hi_osd_region_{
    RGN_HANDLE region_handle;    
}hi_osd_region_t;

typedef struct _hi_snap_osd_{
    bool bShow;
    char reserved[3];
    int x;
    int y;
    char szStr[MAX_EXTEND_OSD_LENGTH];
#if (defined(_AV_PLATFORM_HI3520D_V300_)||defined(_AV_PLATFORM_HI3515_))
	rm_uint8_t type;//区域类型，0-日期(使能一次之后，av自己取日期)；1-其它
	rm_uint8_t align;//0 -左对齐，1-右对齐，2-居中对齐，其他无效
	rm_uint8_t color;//叠加字体颜色，0-白色；1-黑色；2-红色；其它-系统颜色
	rm_uint8_t reserved1;
#endif
} hi_snap_osd_t;

#if defined(_AV_PRODUCT_CLASS_IPC_)
//!Added for IPC with VQE
typedef struct _hi_audio_thread_info_{
    av_audio_stream_type_t  audio_stream_type;
    int phy_audio_chn_id;
    HI_S32 aenc_chn;
    bool thread_state;
    pthread_t thread_id;
}hi_audio_thread_info_t;
#endif

#if defined(_AV_PLATFORM_HI3515_)
#define SAME_OSD_CACHE_ITEM_NUM			3	//time,  gps,  speed///
#define SAME_OSD_CACHE_RESOLUTION_NUM	3	//D1,  HD1,  CIF///

typedef struct _same_osd_cache_
{
	char			szTitle[MAX_EXTEND_OSD_LENGTH];
	char*			szData;
	rm_uint32_t		nDataLen;
}same_osd_cache_t;

/*6A_I代osd叠加类型*/
typedef enum osd_type
{
	OSD_DATEANDTIME=0,
	OSD_SPEEDANDMIL,
	OSD_RECSTATUS,
	OSD_USBSTATUS,
	OSD_FIREMONITOR,
	OSD_TRAINANDNUM,
	OSD_CHNNAME,
}OSD_TYPE;

typedef struct _osd_overlay_property_
{
	rm_uint32_t		subMask;	//此区域包含的子项掩码，因为3515每个区域有至少两项组成
	av_osd_item_t	osd_item;
}osd_overlay_property_t;

#endif

class CHiAvEncoder:public CAvEncoder{
    public:
       CHiAvEncoder();
        virtual ~CHiAvEncoder();

    private:
        /*video encoder*/
        int Ae_create_video_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, video_encoder_id_t *pVideo_encoder_id);
        int Ae_create_h264_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, video_encoder_id_t *pVideo_encoder_id);
        int Ae_dynamic_modify_encoder_local(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para);
        int Ae_destory_video_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, video_encoder_id_t *pVideo_encoder_id);

        int Ae_create_h265_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, video_encoder_id_t *pVideo_encoder_id);


        /*audio encoder*/
        int Ae_ai_aenc_bind_control(av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, bool bind_flag);
        int Ae_create_audio_encoder(av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, av_audio_encoder_para_t *pAudio_para, audio_encoder_id_t *pAudio_encoder_id);
        int Ae_destory_audio_encoder(av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, audio_encoder_id_t *pAudio_encoder_id);
        /*video encoder allotter*/
#if defined(_AV_PLATFORM_HI3516A_)
        int Ae_video_encoder_allotter(av_video_stream_type_t video_stream_type, int phy_video_chn_num, VENC_CHN *pChn);
        int Ae_video_encoder_allotter_V1(av_video_stream_type_t video_stream_type, int phy_video_chn_num, VENC_CHN *pChn);
        int Ae_video_encoder_allotter_MCV1(av_video_stream_type_t video_stream_type, int phy_video_chn_num, VENC_CHN *pChn);
        int Ae_video_encoder_allotter_snap( VENC_CHN *pChn );
#else
        int Ae_video_encoder_allotter(av_video_stream_type_t video_stream_type, int phy_video_chn_num, VENC_GRP *pGrp, VENC_CHN *pChn);
        int Ae_video_encoder_allotter_V1(av_video_stream_type_t video_stream_type, int phy_video_chn_num, VENC_GRP *pGrp, VENC_CHN *pChn);
        int Ae_video_encoder_allotter_MCV1(av_video_stream_type_t video_stream_type, int phy_video_chn_num, VENC_GRP *pGrp, VENC_CHN *pChn);
        int Ae_video_encoder_allotter_snap( VENC_GRP *pGrp, VENC_CHN *pChn );
#endif
        /*audio encoder allotter*/
        int Ae_audio_encoder_allotter(av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, AENC_CHN *pAenc_chn);
        int Ae_audio_encoder_allotter_MCV1(av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, AENC_CHN *pAenc_chn);
        int Ae_audio_encoder_allotter_V1(av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, AENC_CHN *pAenc_chn);
        /*get data*/
        int Ae_get_audio_stream(audio_encoder_id_t *pAudio_encoder_id, av_stream_data_t *pStream_data, bool blockflag = true);
        int Ae_release_audio_stream(audio_encoder_id_t *pAudio_encoder_id, av_stream_data_t *pStream_data);
        int Ae_get_video_stream(video_encoder_id_t *pVideo_encoder_id, av_stream_data_t *pStream_data, bool blockflag = true);
        int Ae_release_video_stream(video_encoder_id_t *pVideo_encoder_id, av_stream_data_t *pStream_data);
        /**/
        int Ae_request_iframe_local(av_video_stream_type_t video_stream_type, int phy_video_chn_num);
        int Ae_init_system_pts(unsigned long long pts);
        /*osd*/
#if defined(_AV_PLATFORM_HI3520D_V300_)	/*6AII_AV12机型专用osd叠加*/
		int Ae_create_extend_osd_region(std::list<av_osd_item_t>::iterator &av_osd_item_it);
		int Ae_extend_osd_overlay(std::list<av_osd_item_t>::iterator &av_osd_item_it);
#endif
#if defined(_AV_PLATFORM_HI3515_)
        int Ae_create_osd_region(std::list<av_osd_item_t> *av_osd_item_list);
        int Ae_osd_overlay(std::list<av_osd_item_t> &av_osd_item_list);
		int Ae_create_osd_item(std::list<av_osd_item_t>::iterator &av_osd_item_it, REGION_ATTR_S region_attr);
		int Ae_set_osd(std::list<av_osd_item_t>::iterator av_osd_item_it, char* pUtf8_string);

		int Ae_create_6a1_extend_osd_region(std::list<av_osd_item_t> *av_osd_item_list);
        int Ae_6a1_extend_osd_overlay(std::list<av_osd_item_t> &av_osd_item_list);
		int Ae_6a1_translate_osd_region_id(OSD_TYPE type);
#else
		int Ae_create_osd_region(std::list<av_osd_item_t>::iterator &av_osd_item_it);
		int Ae_osd_overlay(std::list<av_osd_item_t>::iterator &av_osd_item_it);
#endif
        int Ae_destroy_osd_region(std::list<av_osd_item_t>::iterator &av_osd_item_it);

        /*h264 info*/
        int Ae_get_H264_info(const void *pVideo_encoder_info, RMFI2_H264INFO * ph264_info);
#if defined(_AV_PLATFORM_HI3516A_)
        /*h265 info*/
        int Ae_get_H265_info(const void *pVideo_encoder_info, RMFI2_H265INFO *pH265_info);
#endif        
        
#if defined(_AV_HAVE_VIDEO_INPUT_)
    private:
        CHiVi *m_pHi_vi;
        
    public:
        void Ae_set_vi_module(CHiVi *pHi_vi){m_pHi_vi = pHi_vi;}
#endif

#if defined(_AV_HISILICON_VPSS_)
    private:
        CHiVpss *m_pHi_vpss;

    private:
        int Ae_venc_vpss_control(av_video_stream_type_t video_stream_type, int phy_video_chn_num, bool bind_flag);

    public:
        void Ae_set_vpss_module(CHiVpss *pHi_vpss){m_pHi_vpss = pHi_vpss;}
#endif
		
		int Ae_bind_vi_to_venc(av_video_stream_type_t video_stream_type, int phy_video_chn_num, bool bind_flag);
#if defined(_AV_HAVE_AUDIO_INPUT_)
    private:
        CHiAi *m_pHi_ai;

    public:
        void Ae_set_ai_module(CHiAi *pHi_ai){m_pHi_ai = pHi_ai;}
#if defined(_AV_PRODUCT_CLASS_IPC_)
//!Added for IPC with VQE
    private:
        int Ae_ai_aenc_proc(av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num);
        static void *ai_aenc_thread_body(void* args);
    private:
        hi_audio_thread_info_t *m_pAudio_thread_info;
#endif
#endif
    public: //for snap
        int Ae_create_snap_encoder( av_resolution_t eResolution, av_tvsystem_t eTvSystem, unsigned int u32OsdNameLen );
		//X5III
#if defined(_AV_PRODUCT_CLASS_IPC_)
    int Ae_destroy_snap_encoder();
#else
    int Ae_destroy_snap_encoder(hi_snap_osd_t *stuOsdInfo);
#endif
    int Ae_create_snap_encoder( int vi_chn, av_resolution_t eResolution, av_tvsystem_t eTvSystem, unsigned int factor, hi_snap_osd_t pSnapOsd[], int osdNum );
    int Ae_encode_vi_to_picture(int vi_chn, VIDEO_FRAME_INFO_S *pstuViFrame, hi_snap_osd_t stuOsdParam[], int osd_num,  VENC_STREAM_S* pstuPicStream );
    int Ae_encoder_vi_to_picture_free( VENC_STREAM_S* pstuPicStream );
#if defined(_AV_PRODUCT_CLASS_IPC_)
#if defined(_AV_SUPPORT_IVE_)
    int Ae_encode_frame_to_picture(VIDEO_FRAME_INFO_S *pstuViFrame, RECT_S* pstPicture_size, VENC_STREAM_S* pstuPicStream);
    int Ae_free_ive_snap_stream(VENC_STREAM_S* pstuPicStream);
    int Ae_create_ive_snap_encoder(unsigned int width, unsigned int height,unsigned short u16OsdBitmap);
    int Ae_destroy_ive_snap_encoder();
    int Ae_send_frame_to_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, void* pFrame_info, int milli_second);

private:
    int Ae_get_ive_snap_encoder_chn(VENC_CHN* ive_snap_chn);
#endif
#endif

    private:
		int Ae_snap_regin_set_osds( hi_snap_osd_t* stuOsdParam, int osdNum);
        int Ae_snap_regin_set_osds( hi_snap_osd_t stuOsdParam[2] );
        int Ae_snap_regin_set_osd( int index, bool bShow, unsigned short x, unsigned short y, char* pszStr );
#if defined(_AV_PRODUCT_CLASS_IPC_)   
#if defined(_AV_PLATFORM_HI3516A_)
        int Ae_snap_regin_create( VENC_CHN vencChn, av_resolution_t eResolution, av_tvsystem_t eTvSystem, int index, hi_snap_osd_t &pSnapOsd );             
#else
        int Ae_snap_regin_create( VENC_GRP vencGrp, av_resolution_t eResolution, av_tvsystem_t eTvSystem, int index, hi_snap_osd_t &pSnapOsd );  
#endif
#else
#if defined ( _AV_PLATFORM_HI3520D_V300_ )       
        int Ae_snap_regin_create( VENC_CHN vencChn, av_resolution_t eResolution, av_tvsystem_t eTvSystem, int index, hi_snap_osd_t pSnapOsd[6] );
#else
        int Ae_snap_regin_create( VENC_GRP vencGrp, av_resolution_t eResolution, av_tvsystem_t eTvSystem, int index, hi_snap_osd_t pSnapOsd[6] );
#endif
#endif
        int Ae_snap_regin_delete( int index );
#if defined(_AV_PLATFORM_HI3515_)        
    private:
		same_osd_cache_t		m_SameOsdCache[SAME_OSD_CACHE_ITEM_NUM][SAME_OSD_CACHE_RESOLUTION_NUM];
		std::map<unsigned char, unsigned char> m_OsdNameToIdMap;
		//rm_uint8_t				m_nOsdRegionPtrRefCnt[SUPPORT_MAX_VIDEO_CHANNEL_NUM][4];	//6AI，因为每个region是由两个id合成的，两个id都不overlay时，才能delete///
		osd_overlay_property_t	m_stuOsdProperty[SUPPORT_MAX_VIDEO_CHANNEL_NUM][4];
		//OSD_BITMAPINFO			m_stuOsdRecBitmap;
#endif

    private:
        struct _snap_osd_temp_{
            bool bInited;
            REGION_HANDLE hOsdRegin;			
#if defined(_AV_PLATFORM_HI3516A_) || defined(_AV_PLATFORM_HI3520D_V300_)
            VENC_CHN vencChn;
#else
            VENC_GRP vencGrp;
#endif  
            unsigned char* pszBitmap;
            unsigned short u32BitmapWidth;
            unsigned short u32BitmapHeight;

            av_osd_name_t eOsdName;
            av_resolution_t eResolution;
#if (defined(_AV_PLATFORM_HI3520D_V300_)||defined(_AV_PLATFORM_HI3515_))	//2016-3-18, 6AII_AV12 product snap osd overlay text color; 0--white, 1--black,  2--red,  others--white
			HI_U32 osdTextColor;
#endif
        } m_stuOsdTemp[8]; //0-time osd 1-name osd 2 speed 3 gps 4 chnname 5 SELFNUMBER 6 common osd1 7 common osd2
        bool m_bSnapEncoderInited;

#if defined(_AV_PRODUCT_CLASS_IPC_)
#if defined(_AV_SUPPORT_IVE_)
        bool m_bIve_snap_osd_overlay_bitmap;
        bool m_bIve_snap_encoder_created;
#endif
#endif
        
    private:
        struct timeval m_sync_pts_time;
        
};

#endif/*__HIAVENCODER_H__*/

/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __HIVI3515_H__
#define __HIVI3515_H__
#include "AvConfig.h"
#include "AvDebug.h"
#include "AvCommonMacro.h"
#include "AvCommonType.h"
#include "AvDeviceInfo.h"
#include "mpi_vi.h"
#include "mpi_sys.h"
#include "AvThreadLock.h"
#include "HiAvRegion-3515.h"
#include <map>
#include <bitset>

#if defined(_AV_PLATFORM_HI3515_)
#include "hi_comm_vi.h"
#endif

#define _MAX_VI_DEV_NUM_ 32
class CHiVi{
    public:
       CHiVi();
       ~CHiVi();

    public:
        /*vi device*/
        int HiVi_start_all_vi_dev();
        int HiVi_stop_all_vi_dev();
        /*primary vi channel*/
        int HiVi_start_all_primary_vi_chn(av_tvsystem_t tv_system, vi_config_set_t *pSource = NULL/*for each channel*/, vi_config_set_t *pPrimary_attr = NULL, vi_config_set_t *pMinor_attr = NULL);
        int HiVi_start_selected_primary_vi_chn(av_tvsystem_t tv_system,unsigned long long int chn_mask, vi_config_set_t *pSource = NULL/*for each channel*/, vi_config_set_t *pPrimary_attr = NULL, vi_config_set_t *pMinor_attr = NULL);
        int HiVi_stop_all_primary_vi_chn();
        int HiVi_stop_selected_primary_vi_chn(unsigned long long int chn_mask);
        /*information of primary channel*/
        int HiVi_get_primary_vi_info(int phy_chn_num, VI_DEV *pVi_dev = NULL, VI_CHN *pVi_chn = NULL, av_rect_t *pRect = NULL);
        /*information of minor channel*/
        int HiVi_get_minor_vi_info(int phy_chn_num, VI_DEV *pVi_dev = NULL, VI_CHN *pVi_chn = NULL);
        /*preview*/
        int HiVi_preview_control(int phy_chn_num, bool enter_preview);
        /*information of vi source*/
        int HiVi_get_vi_max_size(int phy_chn_num, int *pMax_width, int *pMax_height, av_resolution_t *pResolution = NULL);
        /**/
        int HiVi_get_primary_vi_chn_config(int phy_chn_num, vi_config_set_t *pPrimary_config, vi_config_set_t *pMinor_config = NULL, vi_config_set_t *pSource_config = NULL);
        /**/
        int HiVi_vi_cover(int phy_chn_num, int x, int y, int width, int height, int cover_index);
        int HiVi_vi_uncover(int phy_chn_num, int cover_index);

        int HiVi_set_usr_picture(int phy_chn_num, const VIDEO_FRAME_INFO_S *pVideo_frame_info);
        int HiVi_insert_picture(int phy_chn_num, bool show_or_hide);

        /**
         * @brief set vi csc attribute
         * @param u8Luma, u8Hue, u8Constr, u8Satu [0~64]
         **/
        int HiVi_set_cscattr( int phy_chn_num, unsigned char u8Luma, unsigned char u8Hue
            , unsigned char u8Contr, unsigned char u8Satu );

        /**
         * @brief set vi channel attribute:mirror and flip
         **/
        int HiVi_set_mirror_flip( int phy_chn_num, bool bIsMirror, bool bIsFlip );
#if !defined(_AV_PRODUCT_CLASS_IPC_)
        int HiVi_set_ahd_video_format( int phy_chn_num,  unsigned int chn_mask,unsigned char av_video_format);
        int HiVi_is_ahd_video_format_changed();
#endif

        /**
         * @brief set video color mode
         * @param 0:color mode 1:gray mode
         **/
        int Hivi_set_color_gray_mode( int phy_chn_num, unsigned char u8Mode );

        /**
        * @brief for snap
        **/
        int Hivi_get_vi_frame( int phy_chn_num, VIDEO_FRAME_INFO_S* pstuFrameInfo );

        /**
        * @brief for snap,release frame data created by Hivi_get_vi_frame
        **/
        int Hivi_release_vi_frame( int phy_chn_num, VIDEO_FRAME_INFO_S* pstuFrameInfo );

	 int Hivi_set_vi_norm(VIDEO_NORM_E vi_norm);
	 
    private:


#if defined(_AV_PLATFORM_HI3516A_)
        int HiVi_vi_dev_control_3516_v1(bool start_flag);
#endif

        int HiVi_one_vi_dev_control_by_chn(bool start_flag, int phy_chn_num, vi_config_set_t *pSource = NULL);

    private:
        int HiVi_get_vi_dev_info(std::bitset<_MAX_VI_DEV_NUM_> &vi_dev);
#if defined(_AV_PRODUCT_CLASS_IPC_)
        int HiVi_get_video_size(IspOutputMode_e iom, int *pWidth, int *pHeight);
#endif
        int HiVi_get_video_size(vi_config_set_t *pConfig, int *pWidth, int *pHeight);    
        int HiVi_all_vi_dev_control(bool start_flag);       
	int HiVi_start_vi_dev_3515(VI_DEV vi_dev, const VI_PUB_ATTR_S *pVi_dev_attr);
	int HiVi_stop_vi_dev_3515(VI_DEV vi_dev);
	int HiVi_vi_dev_control_3515(VI_DEV vi_dev, bool start_flag, VI_INPUT_MODE_E input_mode= VI_MODE_BUTT, VI_WORK_MODE_E work_mode= VI_WORK_MODE_BUTT, VIDEO_NORM_E video_mode= VIDEO_ENCODING_MODE_BUTT);    
       int HiVi_vi_dev_control_3515_V1(bool start_flag);
	int HiVi_start_vi_chn_3515(VI_DEV vi_dev, VI_CHN vi_chn, const VI_CHN_ATTR_S *pVi_chn_attr,int framerate);
	int HiVi_stop_vi_chn_3515(VI_DEV vi_dev,VI_CHN vi_chn);	   

#if defined(_AV_PLATFORM_HI3518C_) || defined(_AV_PLATFORM_HI3516A_)
        int HiVi_vi_dev_control_V2(VI_DEV vi_dev, bool start_flag, VI_INTF_MODE_E inte_mode = VI_MODE_BUTT, VI_WORK_MODE_E work_mode = VI_WORK_MODE_BUTT, VI_SCAN_MODE_E scan_mode = VI_SCAN_BUTT, HI_U32 mask_0 = 0, HI_U32 mask_1 = 0);
#endif
        int HiVi_vi_source_default_config(av_tvsystem_t tv_system, int phy_chn_num, vi_config_set_t *pVi_source_config);    

    private:
        int HiVi_stop_vi_dev(VI_DEV vi_dev);

        int HiVi_start_vi_chn(int phy_chn_num, VI_CHN vi_chn, const VI_CHN_ATTR_S *pVi_chn_attr, unsigned char rotate = 0 );
        int HiVi_stop_vi_chn(VI_CHN vi_chn);

    private:
        av_tvsystem_t m_tv_system;
	 VIDEO_NORM_E   m_enViNorm;	
	 unsigned int m_inputFormatValidMask;
        av_videoinputformat_t m_videoinputformat[SUPPORT_MAX_VIDEO_CHANNEL_NUM];
        av_videoinputformat_t m_videoinputformat_bak[SUPPORT_MAX_VIDEO_CHANNEL_NUM];	 
        unsigned char m_isvideoinputformatchanged;
        std::map<int, vi_config_set_t> m_source_config;
        std::map<int, vi_config_set_t> m_primary_channel_primary_attr;
        std::map<int, vi_config_set_t> m_primary_channel_minor_attr;
        std::map<int, vi_config_set_t> m_minor_channel_primary_attr;
        std::map<int, vi_config_set_t> m_minor_channel_minor_attr;
        std::map<int, int> m_preview_counter;
        CAvThreadLock *m_pThread_lock;

        unsigned char m_u8LastSatu; //record the last saturation for recover color,gray mode
        unsigned char m_u8LastColorGrayMode; //0:color mode, 1:gray mode
};


#endif/*__HIVI_H__*/


/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __HIVI_H__
#define __HIVI_H__
#include "AvConfig.h"
#include "AvDebug.h"
#include "AvCommonMacro.h"
#include "AvCommonType.h"
#include "AvDeviceInfo.h"
#include "mpi_vi.h"
#include "mpi_sys.h"
#include "AvThreadLock.h"
#include "HiAvRegion.h"
#include <map>
#include <bitset>

#if defined(_AV_PLATFORM_HI3516A_)
#include "hi_mipi.h"
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
        /*minor vi channel*/
        int HiVi_start_all_minor_vi_chn(vi_config_set_t *pPrimary_attr = NULL, vi_config_set_t *pMinor_attr = NULL);
        int HiVi_stop_all_minor_vi_chn();
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

        /**
         * @brief set video color mode
         * @param 0:color mode 1:gray mode
         **/
        int Hivi_set_color_gray_mode( int phy_chn_num, unsigned char u8Mode );

#if defined(_AV_PRODUCT_CLASS_IPC_)
        int Hivi_set_ldc_attr( int phy_chn_num, const VI_LDC_ATTR_S* pstLDCAttr );
#if defined(_AV_PLATFORM_HI3518C_)
        int HiVi_set_chn_capture_coordinate_offset(int phy_chn_num, int x_offset, int y_offset);
#endif
#endif

        /**
        * @brief for snap
        **/
        int Hivi_get_vi_frame( int phy_chn_num, VIDEO_FRAME_INFO_S* pstuFrameInfo );

        /**
        * @brief for snap,release frame data created by Hivi_get_vi_frame
        **/
        int Hivi_release_vi_frame( int phy_chn_num, VIDEO_FRAME_INFO_S* pstuFrameInfo );
#if defined(_AV_PLATFORM_HI3516A_) || defined(_AV_PLATFORM_HI3518EV200_)
        int HiVi_vi_dev_DCI_Attr(VI_DEV vi_dev,const viDCIParam_t &stDciParam);
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_)
        int HiVi_vi_query_chn_state(int phy_chn_num, VI_CHN_STAT_S& chn_state);
        int HiVi_get_video_size(IspOutputMode_e iom, int *pWidth, int *pHeight);
#endif

    private:
#if defined(_AV_PLATFORM_HI3531_) || defined(_AV_PLATFORM_HI3532_)
        int HiVi_vi_dev_control_3531_v1(bool start_flag);
#endif

#if defined(_AV_PLATFORM_HI3521_)
        int HiVi_vi_dev_control_3521_v1(bool start_flag);
#endif

#if defined(_AV_PLATFORM_HI3520D_)||defined(_AV_PLATFORM_HI3520D_V300_)
        int HiVi_vi_dev_control_3520D_v1(bool start_flag);
#endif

#if defined(_AV_PLATFORM_HI3518C_)
        int HiVi_vi_dev_control_3518_v1(bool start_flag);
#endif

#if defined(_AV_PLATFORM_HI3516A_)
        int HiVi_set_mipi_attr(combo_dev_attr_t* pAttr);
        int HiVi_vi_dev_control_3516_v1(bool start_flag);
        int HiVi_vi_dev_control_3516_for_bt1120(bool start_flag);
#endif

#if defined(_AV_PLATFORM_HI3518EV200_)
        int HiVi_vi_dev_control_3518EV200_v1(bool start_flag); 
        int HiVi_vi_dev_control_3518EV200_1080P(bool start_flag); 
#endif

        int HiVi_one_vi_dev_control_by_chn(bool start_flag, int phy_chn_num, vi_config_set_t *pSource = NULL);

    private:
        int HiVi_get_vi_dev_info(std::bitset<_MAX_VI_DEV_NUM_> &vi_dev);
        int HiVi_get_video_size(vi_config_set_t *pConfig, int *pWidth, int *pHeight);    
        int HiVi_all_vi_dev_control(bool start_flag);       
        int HiVi_vi_dev_control_V1(VI_DEV vi_dev, bool start_flag, VI_INTF_MODE_E inte_mode = VI_MODE_BUTT, VI_WORK_MODE_E work_mode = VI_WORK_MODE_BUTT, VI_SCAN_MODE_E scan_mode = VI_SCAN_BUTT, HI_U32 mask_0 = 0, HI_U32 mask_1 = 0);
#if defined(_AV_PRODUCT_CLASS_IPC_)
        int HiVi_vi_dev_control_V2(VI_DEV vi_dev, bool start_flag, VI_INTF_MODE_E inte_mode = VI_MODE_BUTT, VI_WORK_MODE_E work_mode = VI_WORK_MODE_BUTT, VI_SCAN_MODE_E scan_mode = VI_SCAN_BUTT, HI_U32 mask_0 = 0, HI_U32 mask_1 = 0);
#endif
        int HiVi_vi_source_default_config(av_tvsystem_t tv_system, int phy_chn_num, vi_config_set_t *pVi_source_config);    

    private:
        int HiVi_start_vi_dev(VI_DEV vi_dev, const VI_DEV_ATTR_S *pVi_dev_attr);
        int HiVi_stop_vi_dev(VI_DEV vi_dev);

        int HiVi_start_vi_chn(int phy_chn_num, VI_CHN vi_chn, const VI_CHN_ATTR_S *pVi_chn_attr, unsigned char rotate = 0 );
        int HiVi_stop_vi_chn(VI_CHN vi_chn);

    private:
        av_tvsystem_t m_tv_system;
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


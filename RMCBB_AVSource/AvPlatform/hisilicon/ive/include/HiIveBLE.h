/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __HI_IVE_BLE_
#define __HI_IVE_BLE_

#include "HiIve.h"
#include "bus_lane_enforcement.h"
#include "ive_snap.h"

#include "hi_comm_video.h"

#include <vector>
#include <string>

using namespace std;

typedef struct
{
    unsigned char start_week;
    unsigned char end_week;
    unsigned char start_hour1;
    unsigned char end_hour1;
    unsigned char start_hour2;
    unsigned char end_hour2;
    unsigned char start_min1;
    unsigned char end_min1;
    unsigned char start_min2;
    unsigned char end_min2;
}bus_time_t;


class CHiIveBle:public CHiIve
{
    public: 
        CHiIveBle(data_sorce_e data_source_type, data_type_e data_type, void* data_source, CHiAvEncoder* mpEncoder);
        virtual ~CHiIveBle();

    public:
        int ive_control(unsigned int ive_type, ive_ctl_type_e ctl_type, void* args); 

    private:
        int init();
        int uninit();
        int process_body();
        int process_raw_data_from_vi(int phy_chn);
        int process_yuv_data_from_vi(int phy_chn);
        int process_yuv_data_from_vpss(int phy_chn);  
        int ive_ble_process_frame_SG(int phy_chn);
        int ive_ble_process_frame_for_SZGJ(int phy_chn);
        int ive_ble_process_frame_for_CQJY(int phy_chn);
        int ive_ble_process_frame(int phy_chn);

        int ive_is_singapore_bus_valid_time(int time_type);
        int ive_obtain_osd(hi_snap_osd_t * snap_osd,int osd_num);
        int ive_stich_snap_for_singapore_bus(int frame_num,
                                                        hi_snap_osd_t * snap_osd,int osd_num,RECT_S roi,
                                                        unsigned long long int snap_subtype);
        int  ive_single_snap(VIDEO_FRAME_INFO_S * frame_info,int phy_chn);
        int ive_single_snap(VIDEO_FRAME_INFO_S * frame_info, hi_snap_osd_t * snap_osd,int osd_num);  
        int ive_recycle_resource(ive_snap_task_s* snap_info);
        int ive_recycle_resource2(ive_snap_task_s* snap_info);
        int  ive_clear_snap();
        
        int update_blacklist_for_cqjy();
        bool is_vehicle_in_blacklist(const char vehicle_number[], unsigned char vehicle_len, char vehicle_info[], unsigned char info_len);
        bool is_vehicle_detected(const char vehicle_number[], unsigned char vehicle_len);
        int add_vehicle_number_to_blacklist(char vehicle_number[], unsigned char vehicle_len);
        bool vehicle_strncmp(const char* src, const char* dest, unsigned int size);
        void clear_history_vehicle_numbers();
        
        
    private:
       CIveSnap* m_ive_snap_thread;
        int m_single_snapped; 
        vector<VIDEO_FRAME_INFO_S> m_vFrames; 
        vector<hi_snap_osd_t> m_vSnap_osd;
        hi_snap_osd_t  m_snap_osd[IVE_SUPPORT_MAX_OSD_NUMBER];
        unsigned char m_bCurrent_detection_state;

        bus_time_t m_yellow_lane_time;
        bus_time_t m_pink_lane_time;

        HI_U32 m_u32Blk_handle;
        HI_U32 m_u32Pool;
        HI_U32 m_u32Frame_phy_addr;
        HI_U8* m_u32Frame_vir_addr;
        char m_pOsd[32];
        HI_U8 m_u8Matching_degree;
        vector<string> m_blacklist;
};

#endif


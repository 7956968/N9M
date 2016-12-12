#ifndef __AV_VDA_H__
#define __AV_VDA_H__

#include <list>
#include <vector>
#include "AvConfig.h"
#include "AvCommonType.h"
#include "CommonDebug.h"
typedef struct _vda_chn_info_
{
    start_stop_md_mode_e start_stop_mode;
    int phy_video_chn;
    int vda_chn_handle;
    int vda_chn_fd;
    vda_chn_param_t vda_chn_param;
} vda_chn_info_t;

typedef struct _vda_data_
{
    bool result;
    int sad_value[18][22];
} vda_data_t;

typedef struct _vda_result_
{
    bool result;
    unsigned char region[50];
} vda_result_t;

typedef enum _thread_control_
{
    _THREAD_CONTROL_RUNNING_ = 0,
    _THREAD_CONTROL_PAUSE_,
    _THREAD_CONTROL_EXIT_,
} thread_control_e;

typedef enum _thread_state_
{
    _THREAD_STATE_RUNNING_ = 0,
    _THREAD_STATE_PAUSE_,
    _THREAD_STATE_EXIT_,
} thread_state_e;

typedef struct _vda_thread_task_
{
    thread_control_e thread_control;
    thread_state_e thread_state;
    std::list<vda_chn_info_t> vda_chn_info_list;
    std::vector<vda_result_t> vda_result;
} vda_thread_task_t;

class CAvVda
{ 
public:
    static const int vda_handle_max;
    
public:
    CAvVda();
    virtual ~CAvVda();
    
    int Av_start_vda(vda_type_e vda_type, av_tvsystem_t tv_system, int phy_video_chn, const vda_chn_param_t* pVda_chn_param, start_stop_md_mode_e start_stop_mode=_START_STOP_MD_BY_SWITCH_);
    int Av_stop_vda(vda_type_e vda_type, int phy_video_chn, start_stop_md_mode_e start_stop_mode=_START_STOP_MD_BY_SWITCH_);
    int Av_get_vda_result(vda_type_e vda_type, int phy_video_chn, vda_result_t *pVda_result);
    int Av_vda_thread_body(vda_type_e vda_type);


    unsigned int Av_get_vda_od_counter_value(int phy_video_Chn);
    int Av_reset_vda_od_counter(int phy_video_Chn);
#if defined(_AV_PRODUCT_CLASS_IPC_)
    /*add by dhdong 2014-10-23 14:10
    *由于OD侦测底层获取侦测结果会覆盖上一次获取的结果，而此过程很短，导致应用
    *获取的侦测结果不准确，因此增加一个计数器，上层通过计算器来判断是否发生了
    *遮挡。
    */

    int Av_modify_vda_attribute(int phy_video_chn, unsigned char attr_type,void* attribute);
#endif
    private:
        std::vector<unsigned int> m_od_alarm_counters;


private:
    int Av_start_vda_detector(vda_type_e vda_type, int phy_video_chn, const vda_chn_param_t* pVda_chn_param);
    int Av_stop_vda_detector(vda_type_e vda_type, int phy_video_chn);
    int Av_get_detector_handle(vda_type_e vda_type, int phy_video_chn);
    int Av_get_sad_value(vda_type_e vda_type, int sensitivity);
    int Av_analyse_vda_result(vda_type_e vda_type, int phy_video_chn, const vda_data_t *pVda_data);
public:
    int Av_control_vda_thread(vda_type_e vda_type, thread_control_e control_command);
private:
    virtual int Av_create_detector(vda_type_e vda_type, int detector_handle, const vda_chn_param_t* pVda_chn_param) = 0;
    virtual int Av_destroy_detector(vda_type_e vda_type, int detector_handle) = 0;
    virtual int Av_detector_control(int phy_video_chn, int detector_handle, bool start_or_stop) = 0;
    virtual int Av_get_detector_fd(int detector_handle) = 0;
    virtual int Av_get_vda_data(vda_type_e vda_type, int detector_handle, vda_data_t *pVda_data) = 0;

#if defined(_AV_PRODUCT_CLASS_IPC_)
    virtual int Av_set_vda_attribut(int vda_type, int detector_handle, unsigned char attr_type, void* attribute) = 0;
#endif

protected:    
    static av_tvsystem_t m_vda_tv_system;
    
private:
    vda_thread_task_t *m_pVda_thread_task;
};


#endif /*__AV_VDA_H__*/

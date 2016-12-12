#include "AvVda.h"
#include "AvDebug.h"
#include "AvCommonMacro.h"
#include "AvCommonFunc.h"
#include "AvDeviceInfo.h"
#include "CommonMacro.h"

typedef struct _vda_thread_prarm_
{
    vda_type_e vda_type;
    CAvVda *pAvVda;
} vda_thread_param_t;


const int CAvVda::vda_handle_max = 32;
av_tvsystem_t CAvVda::m_vda_tv_system = _AT_UNKNOWN_;

CAvVda::CAvVda()
{
    vda_result_t vda_result;
    memset(&vda_result, 0, sizeof(vda_result_t));
    
    _AV_FATAL_((m_pVda_thread_task = new vda_thread_task_t[_VDA_MAX_TYPE_]) == NULL, "OUT OF MEMORY\n");
    
    for (int vda_type = 0; vda_type != _VDA_MAX_TYPE_; ++vda_type)
    {
        m_pVda_thread_task[vda_type].thread_control = _THREAD_CONTROL_EXIT_;
        m_pVda_thread_task[vda_type].thread_state = _THREAD_STATE_EXIT_;
        m_pVda_thread_task[vda_type].vda_chn_info_list.clear();

        for (int i = 0; i != vda_handle_max; ++i)
        {
            m_pVda_thread_task[vda_type].vda_result.push_back(vda_result);
        }
    }

#if 1//defined(_AV_PRODUCT_CLASS_IPC_)
    m_od_alarm_counters.clear();

    unsigned int counter = 0;
    for(int i=0; i<pgAvDeviceInfo->Adi_max_channel_number();++i)
    {
        m_od_alarm_counters.push_back(counter);
    }
#endif
}

CAvVda::~CAvVda()
{
    if (m_pVda_thread_task != NULL)
    {
        for (int vda_type = 0; vda_type != _VDA_MAX_TYPE_; ++vda_type)
        {
            m_pVda_thread_task[vda_type].vda_chn_info_list.clear();
            m_pVda_thread_task[vda_type].vda_result.clear();
        }
    }

#if 1//defined(_AV_PRODUCT_CLASS_IPC_)
        m_od_alarm_counters.clear();
#endif


    _AV_SAFE_DELETE_ARRAY_(m_pVda_thread_task);
}

void * Av_vda_thread_entry(void * pPara)
{
	prctl(PR_SET_NAME, "Av_vda");
    vda_thread_param_t *pVda_thread_param = (vda_thread_param_t *)pPara;
    pVda_thread_param->pAvVda->Av_vda_thread_body(pVda_thread_param->vda_type);
    _AV_SAFE_DELETE_(pVda_thread_param); /*new in Av_start_vda(...)*/
    
    return NULL;
}

int CAvVda::Av_start_vda(vda_type_e vda_type, av_tvsystem_t tv_system, int phy_video_chn, const vda_chn_param_t* pVda_chn_param, start_stop_md_mode_e start_stop_mode /*=_START_STOP_MD_BY_SWITCH_*/)
{
    if ((vda_type != _VDA_MD_ && vda_type != _VDA_OD_) || tv_system == _AT_UNKNOWN_ || pVda_chn_param == NULL)
    {
        DEBUG_ERROR( "CAvVda::Av_start_vda(...) parameter ERROR!!! \n");
        return -1;
    }


    /*wether vda channel start or not*/
    if (!m_pVda_thread_task[vda_type].vda_chn_info_list.empty())
    {
         std::list<vda_chn_info_t>::const_iterator iter = m_pVda_thread_task[vda_type].vda_chn_info_list.begin();
         while (iter != m_pVda_thread_task[vda_type].vda_chn_info_list.end())
         {
             if (iter->phy_video_chn == phy_video_chn)
             {
                 return 0;
             }
             ++iter;
         }
    }
   
    m_vda_tv_system = tv_system;
    /*pause vda thread*/
    Av_control_vda_thread(vda_type, _THREAD_CONTROL_PAUSE_);
    
    vda_chn_info_t vda_chn_info;
    memset(&vda_chn_info, 0, sizeof(vda_chn_info_t));

    if (pVda_chn_param->enable)
    {
        vda_chn_info.start_stop_mode = start_stop_mode;
        vda_chn_info.phy_video_chn = phy_video_chn;
        vda_chn_info.vda_chn_handle = Av_get_detector_handle(vda_type, vda_chn_info.phy_video_chn);
        memcpy(&vda_chn_info.vda_chn_param, pVda_chn_param, sizeof(vda_chn_param_t));

        /*start vda detector*/
        if (Av_start_vda_detector(vda_type, vda_chn_info.phy_video_chn, &vda_chn_info.vda_chn_param) != 0)
        {
             DEBUG_ERROR( "CAvVda::Av_start_vda Av_start_vda_detector FAILED!!! (vda_type:%d) (phy_video_chn:%d) \n", vda_type, vda_chn_info.phy_video_chn);
             return -1;
        }

        if ((vda_chn_info.vda_chn_fd = Av_get_detector_fd(vda_chn_info.vda_chn_handle)) == -1)
        {
             DEBUG_ERROR( "CAvVda::Av_start_vda Av_get_detector_fd FAILED!!! (vda_chn_handle:%d) \n", vda_chn_info.vda_chn_handle);
             return -1;
        }
        
        m_pVda_thread_task[vda_type].vda_chn_info_list.push_back(vda_chn_info);
    }
    
    /*run vda thread*/
    Av_control_vda_thread(vda_type, _THREAD_CONTROL_RUNNING_);
    
    /*start vda thread*/
    if (m_pVda_thread_task[vda_type].thread_state == _THREAD_STATE_EXIT_)
    {
        m_pVda_thread_task[vda_type].thread_control = _THREAD_CONTROL_RUNNING_;
        m_pVda_thread_task[vda_type].thread_state = _THREAD_STATE_RUNNING_;
        vda_thread_param_t *pVda_thread_param = new vda_thread_param_t; /*delete in Av_vda_thread_entry(...)*/
        pVda_thread_param->vda_type = vda_type;
        pVda_thread_param->pAvVda = this;
        Av_create_normal_thread(Av_vda_thread_entry, (void *)pVda_thread_param, NULL);
    }

    return 0;
}

int CAvVda::Av_stop_vda(vda_type_e vda_type, int phy_video_chn, start_stop_md_mode_e start_stop_mode/*=_START_STOP_MD_BY_SWITCH_*/)
{
    
    if (vda_type != _VDA_MD_ && vda_type != _VDA_OD_)
    {
        DEBUG_ERROR( "CAvVda::Av_stop_vda ,parameter ERROR!!! \n");
        return -1;
    }

    if (m_pVda_thread_task[vda_type].vda_chn_info_list.empty())
    { 
        return 0;
    }
    
    std::list<vda_chn_info_t>::iterator iter = m_pVda_thread_task[vda_type].vda_chn_info_list.begin();
    while (iter->phy_video_chn != phy_video_chn)
    {
        if (++iter == m_pVda_thread_task[vda_type].vda_chn_info_list.end())
        {
            DEBUG_ERROR( "CAvVda::Av_stop_vda phy_video_chn:%d vda has been stopped !!! \n", phy_video_chn);
            return 0;
        }
    }

    if (start_stop_mode != iter->start_stop_mode)
    {
        return 0;
    }

    /*pause vda thread*/
    Av_control_vda_thread(vda_type, _THREAD_CONTROL_PAUSE_);
    if (Av_stop_vda_detector(vda_type, phy_video_chn) != 0)
    {
        DEBUG_ERROR( "CAvVda::Av_stop_vda Av_stop_vda_detector FAILED!!! (vda_type:%d) (phy_video_chn:%d)\n", vda_type, phy_video_chn);
        return -1;
    }

#if 1//defined(_AV_PRODUCT_CLASS_IPC_)
        if(phy_video_chn <=  pgAvDeviceInfo->Adi_max_channel_number())
        {
            m_od_alarm_counters[phy_video_chn] = 0;
        }
#endif

    memset(&m_pVda_thread_task[vda_type].vda_result.at(phy_video_chn), 0, sizeof(vda_result_t));
    m_pVda_thread_task[vda_type].vda_chn_info_list.erase(iter);
    
    /*run vda thread*/
    Av_control_vda_thread(vda_type, _THREAD_CONTROL_RUNNING_);
    
    return 0;
}

int CAvVda::Av_get_vda_result(vda_type_e vda_type, int phy_video_chn, vda_result_t *pVda_result)
{
    memcpy(pVda_result, &m_pVda_thread_task[vda_type].vda_result.at(phy_video_chn), sizeof(vda_result_t));
    
    return 0;
}

int CAvVda::Av_start_vda_detector(vda_type_e vda_type, int phy_video_chn, const vda_chn_param_t* pVda_chn_param)
{
    int detector_handle = Av_get_detector_handle(vda_type, phy_video_chn);
    
    if (pVda_chn_param == NULL)
    {
        DEBUG_ERROR( "CAvVda::Av_start_vda_detector parameter ERROR!!! \n");
        return -1;
    }
    
    if (Av_create_detector(vda_type, detector_handle, pVda_chn_param) != 0)
    {
        DEBUG_ERROR( "CAvVda::Av_start_vda_detector Av_create_detector FAILED!!! (vda_type:%d) (detector_handle:%d)\n", vda_type, detector_handle);
        return -1;
    }
    if (Av_detector_control(phy_video_chn, detector_handle, true) != 0)
    {
        DEBUG_ERROR( "CAvVda::Av_start_vda_detector Av_detector_control FAILED!!! (vda_type:%d) (detector_handle:%d)\n", vda_type, detector_handle);
        return -1;
    }
    
    return 0;
}

int CAvVda::Av_stop_vda_detector(vda_type_e vda_type, int phy_video_chn)
{
    int detector_handle = Av_get_detector_handle(vda_type, phy_video_chn);
    
    if (Av_detector_control(phy_video_chn, detector_handle, false) != 0)
    {
        DEBUG_ERROR( "CAvVda::Av_stop_vda_detector Av_detector_control FAILED!!! (vda_type:%d) (detector_handle:%d)\n", vda_type, detector_handle);
        return -1;
    }

    if (Av_destroy_detector(vda_type, detector_handle) != 0)
    {
        DEBUG_ERROR( "CAvVda::Av_stop_vda_detector Av_destroy_detector FAILED!!! (vda_type:%d) (detector_handle:%d)\n", vda_type, detector_handle);
        return -1;
    }
    
    return 0;
}

int CAvVda::Av_get_detector_handle(vda_type_e vda_type, int phy_video_chn)
{
    int vda_detector_handle = -1;
    
    switch (vda_type)
    {
        case _VDA_MD_: 
            vda_detector_handle = phy_video_chn;
            break;
        case _VDA_OD_: 
            vda_detector_handle = phy_video_chn + pgAvDeviceInfo->Adi_max_channel_number();
            break;
        default:
            return -1;
    }

    if (vda_detector_handle > vda_handle_max)
    {
        DEBUG_ERROR( "CAvVda::Av_get_detector_handle vda_detector_handle is ERROR!!!! \n");
        vda_detector_handle = -1;
    }

    return vda_detector_handle;
}

int CAvVda::Av_get_sad_value(vda_type_e vda_type, int sensitivity)
{
    int sad = 0;

    if (vda_type == _VDA_MD_)
    { 
#if defined(_AV_PRODUCT_CLASS_IPC_)
        return sensitivity*9+2;
#else
        switch (sensitivity)
        {
            case 0:
                sad = 2;
                break;
            case 1:
                sad = 8;
                break;
            case 2:
                sad = 14;
                break;
            case 3:
                sad = 20;
                break;
            case 4:
                sad = 26;
                break;
            case 5:
                sad = 32;
                break;
            case 6:
                sad = 38;
                break;
            case 7:
                sad = 44;
                break;
            default:
                sad = 32;
                break;
        }
#endif
    }
    else if (vda_type == _VDA_OD_)
    {
        DEBUG_ERROR( "CAvVda::Av_get_sad_value you must give the implement !!! \n");
    }
    return sad;
}

int CAvVda::Av_analyse_vda_result(vda_type_e vda_type, int phy_video_chn, const vda_data_t *pVda_data)
{
    const int squares_per_line = 22;
    const int squares_per_column = (m_vda_tv_system == _AT_PAL_) ? 18 : 15;
#ifndef _AV_PRODUCT_CLASS_IPC_
    const int alarm_factor = 1;
#endif

    std::list<vda_chn_info_t>::iterator iter = m_pVda_thread_task[vda_type].vda_chn_info_list.begin();
    while (iter->phy_video_chn != phy_video_chn)
    {
        if (++iter == m_pVda_thread_task[vda_type].vda_chn_info_list.end())
        {
            return -1;
        }
    }

    memset(&m_pVda_thread_task[vda_type].vda_result[iter->phy_video_chn], 0, sizeof(vda_result_t));
    
    if (vda_type == _VDA_MD_)
    {
        int valid_count = 0, all_valid_count = 0;                    
        int cmp_sad_value = Av_get_sad_value(vda_type, iter->vda_chn_param.sensitivity);

        for (int i = 0; i < squares_per_column; ++i)
        {
            for (int j = 0; j < squares_per_line; ++j)
            {
                int index = i * squares_per_line + j;
                if (GCL_BIT_VAL_TEST(iter->vda_chn_param.region[index / 8], (index % 8)))
                {
                    all_valid_count++;
                    if (pVda_data->sad_value[i][j] > cmp_sad_value)
                    {
                        valid_count++;
#if defined(_AV_PRODUCT_CLASS_IPC_)
                        GCL_BIT_VAL_SET(m_pVda_thread_task[vda_type].vda_result[iter->phy_video_chn].region[index / 8], (index % 8));
#endif
                    }
                }
            }
        }

#if defined(_AV_PRODUCT_CLASS_IPC_)
        if( valid_count > 0 )
        {
            m_pVda_thread_task[vda_type].vda_result[iter->phy_video_chn].result = 1;
        }
#else
        if (all_valid_count != 0)
        {
            m_pVda_thread_task[vda_type].vda_result[iter->phy_video_chn].result = (valid_count * 100 / all_valid_count) >= alarm_factor;
        }
#endif
    }
    else if (vda_type == _VDA_OD_)
    {
        m_pVda_thread_task[vda_type].vda_result[iter->phy_video_chn].result = pVda_data->result;
        
#if 1//defined(_AV_PRODUCT_CLASS_IPC_)     
        if(pVda_data->result)
        {
            ++m_od_alarm_counters[phy_video_chn];
        }
#endif
    }

    return 0;
}

int CAvVda::Av_control_vda_thread(vda_type_e vda_type, thread_control_e control_command)
{
    if (m_pVda_thread_task[vda_type].thread_state != _THREAD_STATE_EXIT_)
    {
        thread_state_e thread_state;
        switch (control_command)
        {
            case _THREAD_CONTROL_RUNNING_:
                thread_state = _THREAD_STATE_RUNNING_;
                break;
            case _THREAD_CONTROL_PAUSE_:
                thread_state = _THREAD_STATE_PAUSE_;
                break;
            case _THREAD_CONTROL_EXIT_: 
            default: 
                thread_state = _THREAD_STATE_EXIT_;
                break;
        }
        
        m_pVda_thread_task[vda_type].thread_control = control_command;
        while (m_pVda_thread_task[vda_type].thread_state != thread_state && m_pVda_thread_task[vda_type].thread_state != _THREAD_STATE_EXIT_) 
        {
            usleep(10);
        }
    }
    
    return 0;
}

int CAvVda::Av_vda_thread_body(vda_type_e vda_type)
{
    int ret = -1;
    int max_fd = -1;
    fd_set vda_fd_set;
    vda_data_t vda_data;
    struct timeval timeout;
    std::list<vda_chn_info_t>::iterator iter;
    
    while (true)
    {
        switch (m_pVda_thread_task[vda_type].thread_control)
        {
            case _THREAD_CONTROL_RUNNING_:
                m_pVda_thread_task[vda_type].thread_state = _THREAD_STATE_RUNNING_;
                break;
            case _THREAD_CONTROL_PAUSE_:
                m_pVda_thread_task[vda_type].thread_state = _THREAD_STATE_PAUSE_;
                mSleep(50);
                continue;
                break;
            case _THREAD_CONTROL_EXIT_:
            default:
                goto _EXIT_VDA_THREAD_;
                break;
        }

        /*no vda channel is enabled, so stop vda thread*/
        if (m_pVda_thread_task[vda_type].vda_chn_info_list.empty())
        {
            goto _EXIT_VDA_THREAD_;
        }

        max_fd = -1;
        FD_ZERO(&vda_fd_set);
        
        for (iter = m_pVda_thread_task[vda_type].vda_chn_info_list.begin(); iter != m_pVda_thread_task[vda_type].vda_chn_info_list.end(); ++iter)
        {
            FD_SET(iter->vda_chn_fd, &vda_fd_set);
            max_fd = (iter->vda_chn_fd > max_fd) ? iter->vda_chn_fd : max_fd;
        }

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        ret = select(max_fd+1, &vda_fd_set, NULL, NULL, &timeout);
        if (ret < 0)
        {
            DEBUG_ERROR( "CAvVda::Av_vda_thread_body ERROR!!! (vda_type:%d) \n", vda_type);
        }
        else if (ret == 0)
        {
            //DEBUG_ERROR( "CAvVda::Av_vda_thread_body time out!!! (vda_type:%d) \n", vda_type);
        }
        else
        {
            for (iter = m_pVda_thread_task[vda_type].vda_chn_info_list.begin(); iter != m_pVda_thread_task[vda_type].vda_chn_info_list.end(); ++iter)
            {
                if (FD_ISSET(iter->vda_chn_fd, &vda_fd_set))
                {
                    memset(&vda_data, 0, sizeof(vda_data_t));
                    if (Av_get_vda_data(vda_type, iter->vda_chn_handle, &vda_data) == 0)
                    {
                        if (Av_analyse_vda_result(vda_type, iter->phy_video_chn, &vda_data) != 0)
                        {
                            DEBUG_ERROR( "CAvVda::Av_vda_thread_body Av_analyse_vda_result FAILED!!! (vda_type:%d)\n", vda_type);
                        }
                    }
                    else
                    {
                        DEBUG_ERROR( "CAvVda::Av_vda_thread_body Av_get_vda_data FAILED!!! (phy_video_chn:%d) (vda_chn_handle:%d)\n", iter->phy_video_chn, iter->vda_chn_handle);
                    }  
                }
            }
        }
        
        mSleep(50);
    }

_EXIT_VDA_THREAD_:
    
    m_pVda_thread_task[vda_type].thread_state = _THREAD_STATE_EXIT_;
    
    return 0;
}


unsigned int CAvVda::Av_get_vda_od_counter_value(int phy_video_Chn)
{
    if(phy_video_Chn <= pgAvDeviceInfo->Adi_max_channel_number())
    {
        return m_od_alarm_counters[phy_video_Chn];
    }

    return 0;
}

int CAvVda::Av_reset_vda_od_counter(int phy_video_Chn)
{
    if(phy_video_Chn <= pgAvDeviceInfo->Adi_max_channel_number())
    {
        m_od_alarm_counters[phy_video_Chn] = 0;
        return 0;
    }

    return -1;
}
#if defined(_AV_PRODUCT_CLASS_IPC_)

int CAvVda::Av_modify_vda_attribute(int phy_video_chn, unsigned char attr_type,void* attribute)
{

    for(unsigned char i = 0; i< _VDA_MAX_TYPE_; ++i)
    {
        if (!m_pVda_thread_task[i].vda_chn_info_list.empty())
        {
            std::list<vda_chn_info_t>::const_iterator iter = m_pVda_thread_task[i].vda_chn_info_list.begin();
            while (iter != m_pVda_thread_task[i].vda_chn_info_list.end())
            {
                if(iter->phy_video_chn == phy_video_chn)
                {
                    Av_set_vda_attribut(i, iter->vda_chn_handle, attr_type, attribute);
                    break;
                }
            }
        }
    }
    
    return 0;
}

#endif


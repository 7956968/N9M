#include "AvDevice.h"


CAvDevice::CAvDevice()
{
}


CAvDevice::~CAvDevice()
{

}


#if defined(_AV_HAVE_VIDEO_ENCODER_)
int CAvDevice::Ad_start_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, 
av_audio_stream_type_t audio_stream_type/* = _AAST_NORMAL_*/, int phy_audio_chn_num/* = -1*/, av_audio_encoder_para_t *pAudio_para/* = NULL*/, int audio_only)
{  
    this->Ad_modify_snap_param( video_stream_type, phy_video_chn_num, pVideo_para );
    
    return m_pAv_encoder->Ae_start_encoder(video_stream_type, phy_video_chn_num, pVideo_para, audio_stream_type, phy_audio_chn_num, pAudio_para, audio_only);
}

int CAvDevice::Ad_start_selected_encoder(unsigned long long int chn_mask, av_video_encoder_para_t *pVideo_para, av_audio_encoder_para_t *pAudio_para, av_video_stream_type_t video_stream_type, av_audio_stream_type_t audio_stream_type)
{
    if(NULL == pVideo_para || NULL == pAudio_para || _AST_UNKNOWN_ == video_stream_type || _AAST_UNKNOWN_ == audio_stream_type)
    {
        DEBUG_MESSAGE( "CAvDevice::Ad_start_selected_encoder(video_stream_type(%d), audio_stream_type(%d) or pAudio || pVideo == NULL) parameters invalid\n", video_stream_type, audio_stream_type);
        return -1;
    }

    /*map<pair<recv_task_id, send_task_id>, list<chn_num> >*/
    std::map<std::pair<int,int>, std::list<int> > chn_num_in_thread_task;
    std::map<std::pair<int,int>, std::list<int> >::iterator map_it;
    std::list<int> chn_list;
    std::list<int>::iterator list_it;
    int recv_task_id = -1;
    int send_task_id = -1;
        
    int max_vi_chn_num = pgAvDeviceInfo->Adi_get_vi_chn_num();
    /*get task_element and corresponding chn_nums*/
    for(int chn_num = 0; chn_num < max_vi_chn_num; chn_num++)
    {
        if(((chn_mask) & (1LL<<chn_num))!=0)
        {
            m_pAv_encoder->Ae_get_task_id(video_stream_type, chn_num, &recv_task_id, &send_task_id);
            std::pair<int,int> temp_pair(recv_task_id, send_task_id);
            if(- 1 != recv_task_id)
            {
                
                if((map_it = chn_num_in_thread_task.find(temp_pair)) != chn_num_in_thread_task.end())
                {
                    map_it->second.push_back(chn_num);
                }
                else
                {   
                    chn_list.push_back(chn_num);
                    chn_num_in_thread_task[temp_pair] = chn_list;
                    chn_list.clear();
                }
            }
        }
    }
    /*loop thread task outside loop*/
    m_pAv_encoder->Ae_get_AvThreadLock()->lock();
    m_pAv_encoder->Ae_thread_control(_ETT_OSD_OVERLAY_, 0, _ATC_REQUEST_PAUSE_);
    
    map_it = chn_num_in_thread_task.begin();
    while(map_it != chn_num_in_thread_task.end())
    {
        recv_task_id = map_it->first.first;
        send_task_id = map_it->first.second;
        
        /*pasused thread task*/
        m_pAv_encoder->Ae_thread_control(_ETT_STREAM_REC_, recv_task_id, _ATC_REQUEST_PAUSE_);
        
        /*loop chn_list inside loop*/
        list_it = map_it->second.begin();
        while(list_it != map_it->second.end())
        {
            /*main stream*/
            int chn = *list_it;
            int phy_video_chn_num = chn;
            int phy_audio_chn_num = chn;
            
            pgAvDeviceInfo->Adi_get_audio_chn_id(chn , phy_audio_chn_num);
            if(Ad_start_encoder(video_stream_type, phy_video_chn_num, pVideo_para + phy_video_chn_num, _AAST_NORMAL_, phy_audio_chn_num, pAudio_para + phy_video_chn_num) != 0)
            {
                DEBUG_ERROR( "int CAvAppMaster::Ad_start_selected_encoder() m_pAv_device->Ad_start_encoder FAIELD(chn:%d)\n", chn);
                //return -1;/*continue start next channel*/
            }
            list_it++;
        }
        
        /*start thread task*/
        m_pAv_encoder->Ae_thread_control(_ETT_STREAM_REC_, recv_task_id, _ATC_REQUEST_RUNNING_);

        map_it++;
    }
    
    if(m_pAv_encoder->Ae_get_av_osd_thread_task_t()->osd_list.size() != 0)
    {
        m_pAv_encoder->Ae_thread_control(_ETT_OSD_OVERLAY_, 0, _ATC_REQUEST_RUNNING_);
    }
    
    m_pAv_encoder->Ae_get_AvThreadLock()->unlock();
    
    return 0;
}

int CAvDevice::Ad_stop_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num,
    av_audio_stream_type_t audio_stream_type/* = _AAST_NORMAL_*/, int phy_audio_chn_num/* = -1*/, int normal_audio)
{
    return m_pAv_encoder->Ae_stop_encoder(video_stream_type, phy_video_chn_num, audio_stream_type, phy_audio_chn_num, normal_audio);
}

int CAvDevice::Ad_stop_selected_encoder(unsigned long long int chn_mask, av_video_stream_type_t video_stream_type, av_audio_stream_type_t audio_stream_type)
{
    std::map<std::pair<int,int>, std::list<int> > chn_num_in_thread_task;
    std::map<std::pair<int,int>, std::list<int> >::iterator map_it;
    std::list<int> chn_list;
    std::list<int>::iterator list_it;
    int recv_task_id = -1;
    int send_task_id = -1;

    int max_chn_num = pgAvDeviceInfo->Adi_max_channel_number();
    /*get task_element and corresponding chn_nums*/
    for(int chn_num = 0; chn_num < max_chn_num; chn_num++)
    {
        if(((chn_mask) & (1LL<<chn_num))!=0)
        {
            m_pAv_encoder->Ae_get_task_id(video_stream_type, chn_num, &recv_task_id, &send_task_id);
            std::pair<int,int> temp_pair(recv_task_id, send_task_id);
            if(- 1 != recv_task_id)
            {
                if((map_it = chn_num_in_thread_task.find(temp_pair)) != chn_num_in_thread_task.end())
                {
                    map_it->second.push_back(chn_num);
                }
                else
                {   
                    chn_list.push_back(chn_num);
                    chn_num_in_thread_task[temp_pair] = chn_list;
                    chn_list.clear();
                }
            }
        }
    }
    /*loop thread task*/
    m_pAv_encoder->Ae_get_AvThreadLock()->lock();
    m_pAv_encoder->Ae_thread_control(_ETT_OSD_OVERLAY_, 0, _ATC_REQUEST_PAUSE_);
    
    map_it = chn_num_in_thread_task.begin();
    while(map_it != chn_num_in_thread_task.end())
    {
        recv_task_id = map_it->first.first;
        send_task_id = map_it->first.second;
        /*pasused thread task*/
        m_pAv_encoder->Ae_thread_control(_ETT_STREAM_REC_, recv_task_id, _ATC_REQUEST_PAUSE_);
        /*loop chn_list*/
        list_it = map_it->second.begin();
        while(list_it != map_it->second.end())
        {
            /*main stream*/
            int chn = *list_it;
            int phy_video_chn_num = chn;
            int phy_audio_chn_num = chn;
            
            pgAvDeviceInfo->Adi_get_audio_chn_id(chn , phy_audio_chn_num);
            
            if(Ad_stop_encoder(video_stream_type, phy_video_chn_num, audio_stream_type, phy_audio_chn_num) != 0)
            {
                DEBUG_ERROR( "int CAvAppMaster::Ad_stop_selected_encoder() m_pAv_device->Ad_stop_encoder FAIELD(chn:%d)\n", chn);
                //return -1;/*continue stop next channel*/
            }       
            list_it++;
        }
        
        if((*(m_pAv_encoder->Ae_get_av_recv_thread_task_t() + recv_task_id)).local_task_list.size() == 0)
        {
            m_pAv_encoder->Ae_thread_control(_ETT_STREAM_REC_, recv_task_id, _ATC_REQUEST_EXIT_);
        }
        else
        {
             m_pAv_encoder->Ae_thread_control(_ETT_STREAM_REC_, recv_task_id, _ATC_REQUEST_RUNNING_);
        }
        
        map_it++;
    }
    
    /*osd thread control*/
    if(m_pAv_encoder->Ae_get_av_osd_thread_task_t()->osd_list.size() == 0)
    {
        m_pAv_encoder->Ae_thread_control(_ETT_OSD_OVERLAY_, 0, _ATC_REQUEST_EXIT_);
    }
    else
    {
        m_pAv_encoder->Ae_thread_control(_ETT_OSD_OVERLAY_, 0, _ATC_REQUEST_RUNNING_);
    }
    
    m_pAv_encoder->Ae_get_AvThreadLock()->unlock();

    return 0;
}

int CAvDevice::Ad_set_encoder_attr(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, av_audio_stream_type_t audio_stream_type/* = _AAST_NORMAL_*/, int phy_audio_chn_num/* = -1*/, av_audio_encoder_para_t *pAudio_para/* = NULL*/)
{
    return 0;
}

int CAvDevice::Ad_request_iframe(av_video_stream_type_t video_stream_type, int phy_video_chn_num)
{
    if (pgAvDeviceInfo->Adi_get_video_source(phy_video_chn_num) == _VS_DIGITAL_)
    {        
        DEBUG_MESSAGE( "CAvDevice::Ad_modify_encoder(type:%d)(chn:%d) is from digital\n", video_stream_type, phy_video_chn_num);
        return 0;
    }
    
    return m_pAv_encoder->Ae_request_iframe(video_stream_type, phy_video_chn_num);
}


int CAvDevice::Ad_modify_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, bool bosd_need_change)
{
    if (pgAvDeviceInfo->Adi_get_video_source(phy_video_chn_num) == _VS_DIGITAL_)
    {        
        DEBUG_MESSAGE( "CAvDevice::Ad_modify_encoder(type:%d)(chn:%d) is from digital\n", video_stream_type, phy_video_chn_num);
        return 0;
    }
    
    this->Ad_modify_snap_param( video_stream_type, phy_video_chn_num, pVideo_para );
    
    return m_pAv_encoder->Ae_dynamic_modify_encoder(video_stream_type, phy_video_chn_num, pVideo_para, bosd_need_change);
}

#ifndef _AV_PRODUCT_CLASS_IPC_
int CAvDevice::Ad_ChangeLanguageCacheChar()
{
	return m_pAv_encoder->ChangeLanguageCacheOsdChar();
}
#endif

int CAvDevice::Ad_get_net_stream_level(unsigned int *chnmask, unsigned char chnvalue[SUPPORT_MAX_VIDEO_CHANNEL_NUM])
{
    return m_pAv_encoder->Ae_get_net_stream_level(chnmask,chnvalue);
}
int CAvDevice::Ad_update_common_osd_content(av_video_stream_type_t stream_type, int phy_video_chn_num, av_osd_name_t osd_name, char osd_content[], unsigned int osd_len)
{
    return m_pAv_encoder->Ae_update_common_osd_content(stream_type, phy_video_chn_num, osd_name, osd_content, osd_len);
}
#endif


int CAvDevice::Ad_init()
{
    return 0;
}

#ifdef _AV_PRODUCT_CLASS_IPC_

int CAvDevice::Ad_update_extra_osd_content(av_video_stream_type_t stream_type, int phy_video_chn_num, av_osd_name_t osd_name, char osd_content[], unsigned int osd_len)
{
    return m_pAv_encoder->Ae_update_osd_content(stream_type, phy_video_chn_num, osd_name, osd_content, osd_len);
}

#endif


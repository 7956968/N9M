#include "HiIveHc.h"

using namespace Common;

#define IVE_CONFIG_PATH "/usr/local/app/extend/ive/bin/hc_config/face_detection_config.xml"

#if 0
CHiIveHc::CHiIveHc()
{
}
#endif

CHiIveHc::CHiIveHc(data_sorce_e data_source_type, data_type_e data_type, void* data_source, CHiAvEncoder* mpEncoder):
    CHiIve(data_source_type, data_type, data_source, mpEncoder)
{ 
    m_32Detect_handle = -1;
    m_bHead_count_state = false;
    m_u32Heads_number = 0;
}
CHiIveHc:: ~CHiIveHc()
{
    m_32Detect_handle = -1;
}


int CHiIveHc::ive_control(unsigned int ive_type, ive_ctl_type_e ctl_type, void* args)
{
    if(ive_type  & IVE_TYPE_HC)
    {
        switch(ctl_type)
        {
            case IVE_CTL_SAVE_RESULT:
            default:
            {
                
                int start_flag = (int)args;
                if(start_flag)
                {
                    return hc_start_count();
                }
                else
                {
                    return hc_stop_count();
                }
            }
                break;
        }
    }

    //DEBUG_ERROR("unkown ive type:%d or contrl type:%d !", ive_type, ctl_type);
    return -1;
}


int CHiIveHc::hc_start_count()
{
    m_bHead_count_state = true;
    m_u32Heads_number = 0;
    return 0;
}

int CHiIveHc::hc_stop_count()
{
    if(!m_bHead_count_state)
    {
        DEBUG_ERROR("the head count is not started! \n");
        return 0;
    }
   pthread_mutex_lock(&m_mutex); 
    if(m_detection_result_notify_func != NULL)
    {
        DEBUG_MESSAGE("close the door,detected %d heads this time! \n", m_u32Heads_number);
        if(NULL != m_detection_result_notify_func)
        {
            msgIPCIntelligentInfoNotice_t notice;
            memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
            notice.cmdtype = 2;
            notice.happen = 1;
            notice.headcount = m_u32Heads_number;
            
            notice.happentime = N9M_TMGetShareSecond();
            m_detection_result_notify_func(&notice);
        }
    }

    m_bHead_count_state = false;
    m_u32Heads_number = 0;
    pthread_mutex_unlock(&m_mutex);
    return 0;
}


int CHiIveHc::init()
{
    if(HI_SUCCESS !=  ObjectDetectionInit((char *)IVE_CONFIG_PATH, &m_32Detect_handle))
    {
        DEBUG_ERROR("ObjectDetectionInit failed!  \n");
        return -1;
    }
    
    return 0;
}

int CHiIveHc::uninit()
{
    if(HI_SUCCESS != ObjectDetectionRelease())
    {
        DEBUG_ERROR("ObjectCountingRelease failed!  \n");
        return -1;
    }  
    return 0;
}

int CHiIveHc::process_body()
{
    DEBUG_MESSAGE("head count thread run!\n");
    m_thread_state = IVE_THREAD_START;
    while(m_thread_state == IVE_THREAD_START)
    {
        m_Ive_process_func(0);
        usleep(10*1000);
    }

    return 0;
}

int CHiIveHc::process_raw_data_from_vi(int phy_chn)
{
    return 0;
}

int CHiIveHc::process_yuv_data_from_vi(int phy_chn)
{
    return 0;
}

int CHiIveHc::process_yuv_data_from_vpss(int phy_chn)
{
    CHiVpss* hivpss = static_cast<CHiVpss *>(m_pData_source); 
    VIDEO_FRAME_INFO_S  video_frame_540p;
    IVE_IMAGE_S src_image_540p;
    int face_num = 0;
    HI_S32 nRet = HI_FAILURE;
    RECT_S roi;

    memset(&roi, 0, sizeof(RECT_S));
    memset(&video_frame_540p, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&src_image_540p, 0, sizeof(IVE_IMAGE_S));
    
    if(0 != hivpss->HiVpss_get_frame( _VP_SPEC_USE_,phy_chn, &video_frame_540p))
    {
        DEBUG_ERROR("vpss get frame failed ! \n");
        return -1;
    }
    
    if(video_frame_540p.stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_422)
    {
        src_image_540p.enType = IVE_IMAGE_TYPE_YUV422SP;
    }
    else
    {
        src_image_540p.enType = IVE_IMAGE_TYPE_YUV420SP;
    }
    for(int i=0; i<3; ++i)
    {
        src_image_540p.u32PhyAddr[i] = video_frame_540p.stVFrame.u32PhyAddr[i];
        src_image_540p.u16Stride[i] = (HI_U16)video_frame_540p.stVFrame.u32Stride[i];
        src_image_540p.pu8VirAddr[i]= (HI_U8*)video_frame_540p.stVFrame.pVirAddr[i];
    }
    src_image_540p.u16Width = (HI_U16)video_frame_540p.stVFrame.u32Width;
    src_image_540p.u16Height = (HI_U16)video_frame_540p.stVFrame.u32Height;

    roi.s32X = 0;
    roi.s32Y = 0;
    roi.u32Width = src_image_540p.u16Width;
    roi.u32Height = src_image_540p.u16Height;
    
    //unsigned long long time_start = N9M_TMGetSystemMicroSecond();
    if(1 == pgAvDeviceInfo->Adi_get_ive_debug_mode()) 
    {
        nRet = ObjectDetectionStart(&src_image_540p, roi, 1, m_32Detect_handle, &face_num, NULL, NULL, NULL);
    }
    else
    {
        nRet = ObjectDetectionStart(&src_image_540p, roi, 0, m_32Detect_handle, &face_num, NULL,NULL, NULL);
    }

    if ( HI_SUCCESS !=  nRet)
    {
        DEBUG_ERROR("AlgLaneDectAndSnapshot failed ! \n");
        if(0 != hivpss->HiVpss_release_frame(_VP_SPEC_USE_,phy_chn, &video_frame_540p))
        {
            DEBUG_ERROR("vpss release 540p frame failed ! \n");
        }
        return -1;
    }

    if((m_bHead_count_state) && (NULL != m_detection_result_notify_func) && (face_num>0))
    {
        pthread_mutex_lock(&m_mutex); 
        if(m_bHead_count_state)
        {
            m_u32Heads_number += face_num;
            DEBUG_MESSAGE("detected %d heads this time, the heads count is:%d  \n", face_num, m_u32Heads_number);

            msgIPCIntelligentInfoNotice_t notice;
            memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
            notice.cmdtype = 2;
            notice.happen = 0;
            notice.headcount = m_u32Heads_number;
            
            notice.happentime = N9M_TMGetShareSecond();
            m_detection_result_notify_func(&notice);
        }
        pthread_mutex_unlock(&m_mutex);
    }
    

    if(1 == pgAvDeviceInfo->Adi_get_ive_debug_mode())
    {
        if(m_pEncoder != NULL)    
        {        
            m_pEncoder->Ae_send_frame_to_encoder(_AST_SUB_VIDEO_, 0, (void *)&video_frame_540p, -1); 
        }
     }
    
    if(0 != hivpss->HiVpss_release_frame(_VP_SPEC_USE_,phy_chn, &video_frame_540p))
    {
        DEBUG_ERROR("vpss release 540p frame failed ! \n");
        return -1;
    }

    //unsigned long long time_end = N9M_TMGetSystemMicroSecond();  
    //printf("[alex]the face detetion speed :%llu ms \n", time_end-time_start);    
    return 0;  
}



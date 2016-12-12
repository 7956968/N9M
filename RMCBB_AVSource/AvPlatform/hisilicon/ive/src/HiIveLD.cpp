#include "HiIveLD.h"

using namespace Common;


CHiIveLd::CHiIveLd(data_sorce_e data_source_type, data_type_e data_type, void* data_source, CHiAvEncoder* mpEncoder):
    CHiIve(data_source_type, data_type, data_source, mpEncoder)
{ 
    m_bCurrent_state = false;
    m_ive_snap_thread = new CIveSnap(mpEncoder);   

    memset(m_snap_osd, 0, sizeof(hi_snap_osd_t)*IVE_SUPPORT_MAX_OSD_NUMBER);

    //GPS defalut value
    strncpy(m_snap_osd[1].szStr, "000.000.0000E 000.000.0000N",32);
    m_snap_osd[1].szStr[31] = '\0';  
}
CHiIveLd:: ~CHiIveLd()
{
    _AV_SAFE_DELETE_(m_ive_snap_thread);
}


int CHiIveLd::ive_control(unsigned int ive_type, ive_ctl_type_e ctl_type, void* args)
{
    if(ive_type  & IVE_TYPE_LD) 
    {
        switch(ctl_type)
        {
            case IVE_CTL_OVERLAY_OSD:
            {
                snprintf(m_snap_osd[1].szStr, sizeof(m_snap_osd[1].szStr),"%s", (char *)args);
                m_snap_osd[1].szStr[sizeof(m_snap_osd[1].szStr)-1] = '\0';
            }
                break;
             default:
                break;
        }
    }
    
    return 0;
}

int CHiIveLd::init()
{
    m_ive_snap_thread->StartIveSnapThreadEx(boost::bind(&CHiIveLd::ive_recycle_resource, this, _1));
    
    if(HI_SUCCESS !=  IveAlgInit((char *)"./"))
    {
        DEBUG_ERROR("lane departure init  failed!  \n");
        return -1;
    }

    return 0;
}

int CHiIveLd::uninit()
{
    if(HI_SUCCESS != IveAlgDeInit())
    {
        DEBUG_ERROR("LD IveAlgDeInit failed!  \n");
        return -1;
    }  
    m_ive_snap_thread->StopIveSnapThread();
    
    return 0;
}

int CHiIveLd::process_body()
{
    DEBUG_MESSAGE("LD thread run!\n");
    m_thread_state = IVE_THREAD_START;
    while(m_thread_state == IVE_THREAD_START)
    {
        m_Ive_process_func(0);
        usleep(10*1000);
    }

    return 0;
}
int CHiIveLd::process_raw_data_from_vi(int phy_chn)
{
    return 0;
}

int CHiIveLd::process_yuv_data_from_vi(int phy_chn)
{
    return 0;
}


#define OSD_VERTICAL_INTERVAL 50
#define DATA_TIME_OSD_X   (1200)
#define DATA_TIME_OSD_Y    (10)
#define GPS_OSD_X  (1200)
#define GPS_OSD_Y  (DATA_TIME_OSD_Y+OSD_VERTICAL_INTERVAL)

int CHiIveLd::process_yuv_data_from_vpss(int phy_chn)
{
    int ret = 0;
    CHiVpss* hivpss = static_cast<CHiVpss *>(m_pData_source);
    VIDEO_FRAME_INFO_S  frame_540p;
    IVE_IMAGE_S image_540p;
    static rm_uint64_t last_alram_time= 0;
   
    memset(&frame_540p, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&image_540p, 0, sizeof(IVE_IMAGE_S));

        
    if(0 != hivpss->HiVpss_get_frame( _VP_SPEC_USE_,phy_chn, &frame_540p))
    {
        DEBUG_ERROR("vpss get frame failed ! \n");
        return -1;
    }

    image_540p.enType = (frame_540p.stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_422)?IVE_IMAGE_TYPE_YUV422SP:IVE_IMAGE_TYPE_YUV420SP;

    for(int i=0; i<3; ++i)
    {
        image_540p.u32PhyAddr[i] = frame_540p.stVFrame.u32PhyAddr[i];
        image_540p.u16Stride[i] = (HI_U16)frame_540p.stVFrame.u32Stride[i];
        image_540p.pu8VirAddr[i]= (HI_U8*)frame_540p.stVFrame.pVirAddr[i];
    }
    image_540p.u16Width = (HI_U16)frame_540p.stVFrame.u32Width;
    image_540p.u16Height = (HI_U16)frame_540p.stVFrame.u32Height;    

    if(1 == pgAvDeviceInfo->Adi_get_ive_debug_mode()) 
    {
        ret = IveAlgStart(&image_540p, 1);
    }
    else
    {
        ret = IveAlgStart(&image_540p, 0);
    }
    
    if(0 != ret && 1 != ret)
    {
        DEBUG_ERROR("IveAlgStart  failed! \n");
        IveAlgParametersReset((char *)"./");
    }

    if((NULL != m_detection_result_notify_func))
    {
        if(!m_bCurrent_state)
        {
            if(ret == 1)
            {
                //snap
                ive_ld_snap();
                //alrm
                m_bCurrent_state = true;
                msgIPCIntelligentInfoNotice_t notice;
                memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                
                notice.cmdtype = 0;
                notice.happen = 1; 
                notice.happentime = N9M_TMGetShareSecond();
            
                m_detection_result_notify_func(&notice);
                last_alram_time = N9M_TMGetSystemMicroSecond();                
            }
        }
        else
        {
            rm_uint64_t current_time = N9M_TMGetSystemMicroSecond();
            if(current_time - last_alram_time >= 5000)
            {
                if(ret == 1)
                {
                    msgIPCIntelligentInfoNotice_t notice;
                    memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                    
                    notice.cmdtype = 0;
                    notice.happen = 1; 
                    notice.happentime = N9M_TMGetShareSecond();
                    
                    m_detection_result_notify_func(&notice);
                    last_alram_time = current_time;                    
                }
                else
                {
                    m_bCurrent_state = 0;
                }
            }
        }
    }

    if(1 == pgAvDeviceInfo->Adi_get_ive_debug_mode())
    {
        if(m_pEncoder != NULL)    
        {        
            m_pEncoder->Ae_send_frame_to_encoder(_AST_SUB_VIDEO_, 0, (void *)&frame_540p, -1); 
        }
    }
    if(0 != hivpss->HiVpss_release_frame(_VP_SPEC_USE_,phy_chn, &frame_540p))
    {
        DEBUG_ERROR("vpss release frame failed! \n");
        ret = -1;
    }
    
    return ret;
}

int CHiIveLd::ive_recycle_resource(ive_snap_task_s* snap_info)
{
    if(NULL != snap_info)
    {
        _AV_SAFE_DELETE_ARRAY_(snap_info->pOsd);
        if(m_eDate_source_type == _DATA_SRC_VPSS)
        {
            CHiVpss* hivpss = static_cast<CHiVpss *>(m_pData_source);
            if(0 != hivpss->HiVpss_release_frame(_VP_MAIN_STREAM_,0, snap_info->pFrame))
            {
                DEBUG_ERROR("vpss release frame failed ! \n");
                
            }
             _AV_SAFE_DELETE_(snap_info->pFrame);       
        }
    }
    return 0;
}

int CHiIveLd::ive_ld_snap()
{
    VIDEO_FRAME_INFO_S single_frame;
    datetime_t date_time;
    
    memset(&single_frame,0,sizeof(single_frame));
    pgAvDeviceInfo->Adi_get_date_time(&date_time);
    snprintf(m_snap_osd[0].szStr,32, "20%02d-%02d-%02d %02d:%02d:%02d", date_time.year, date_time.month, \
        date_time.day, date_time.hour, date_time.minute, date_time.second);

    CHiVpss* hivpss = static_cast<CHiVpss *>(m_pData_source);
    if(0 != hivpss->HiVpss_get_frame( _VP_MAIN_STREAM_,0, &single_frame))
    {
        DEBUG_ERROR("get  frame failed ! \n");
        return -1;
    }       

    m_snap_osd[0].osd_type =_AON_DATE_TIME_;
    m_snap_osd[0].x = GPS_OSD_X;
    m_snap_osd[0].y = GPS_OSD_Y;
    m_snap_osd[1].osd_type = _AON_GPS_INFO_;
    m_snap_osd[1].x = DATA_TIME_OSD_X;
    m_snap_osd[1].y = DATA_TIME_OSD_Y;     
    ive_single_snap(&single_frame, m_snap_osd, 2);    

    return 0;
}

int CHiIveLd::ive_single_snap(VIDEO_FRAME_INFO_S * frame_info, hi_snap_osd_t * snap_osd,int osd_num)
{
    if(NULL == frame_info)
    {
        DEBUG_ERROR("the frame data is invalid! \n");
        return -1;
    }
     ive_snap_task_s task;
     memset(&task, 0, sizeof(ive_snap_task_s));

     if(NULL != snap_osd && 0 != osd_num)
    {
         task.u8Osd_num = osd_num;
         task.u8Snap_type = 1;//0x08
         task.u8Snap_subtype = 2;//LD
         task.pOsd = new ive_snap_osd_display_attr_s[osd_num];   
         for(int i=0; i<osd_num;++i)
         {
            task.pOsd[i].u8type = snap_osd[i].osd_type;
            task.pOsd[i].u8Index = i;
            task.pOsd[i].u16X = snap_osd[i].x;
            task.pOsd[i].u16Y = snap_osd[i].y;
            strncpy(task.pOsd[i].pContent, snap_osd[i].szStr, sizeof(task.pOsd[i].pContent));
         }
    }
     
     task.pFrame = new ive_snap_video_frame_info_s;
     memcpy(task.pFrame, frame_info, sizeof(VIDEO_FRAME_INFO_S));
     DEBUG_MESSAGE("Send snap task! \n");
     m_ive_snap_thread->AddIveSnapTask(task);

    return 0;
}


#include "HiAvOd.h"
#include "SystemConfig.h"

using namespace Common;

static pthread_t thread_od_id;
static bool thread_od_state = false;

static void *av_occlusion_detection_body(void *args);

static void *av_occlusion_detection_body(void *args)
{
    CHiAvOd* adapter = (CHiAvOd*)args;
    adapter->Av_occlusion_detection_proc();

    return NULL;
}

CHiAvOd::CHiAvOd(CHiVpss *pHiVpss):
    m_pHi_vpss(pHiVpss), m_pHi_vi(NULL)
{
    m_pHi_od = new CHiAvIveOd();
    m_od_counter = 0;
    m_od_chn_info = new av_od_chn_info_t();
    
    memset(m_od_chn_info, 0, sizeof(av_od_chn_info_t)); 
}
CHiAvOd::CHiAvOd(CHiVi *pHiVi):
    m_pHi_vpss(NULL), m_pHi_vi(pHiVi)
{
    m_pHi_od = new CHiAvIveOd();
    m_od_counter = 0;
    m_od_chn_info = new av_od_chn_info_t();

    memset(m_od_chn_info, 0, sizeof(av_od_chn_info_t));
}
    
CHiAvOd::~CHiAvOd()
{
    if(thread_od_state)
    {
        Av_stop_ive_od(m_od_chn_info->chn_id);
    }
    _AV_SAFE_DELETE_(m_pHi_od);
    _AV_SAFE_DELETE_(m_od_chn_info);
}

int CHiAvOd::Av_start_ive_od(int phy_video_chn, unsigned char sensitivity)
{
    unsigned short od_block_percent = 50;
    if(phy_video_chn<0 || phy_video_chn>=pgAvDeviceInfo->Adi_max_channel_number())
    {
        DEBUG_ERROR("the phy_video_chn(%d) is invalid! \n", phy_video_chn);
        return 0;
    }
    //m_od_thread_info.thread_state = _THREAD_STATE_RUNNING_;
    m_od_chn_info->chn_enable = 1;
    m_od_chn_info->chn_id = phy_video_chn;
    m_od_chn_info->chn_sensitivity = sensitivity;
    switch(m_od_chn_info->chn_sensitivity)
    {
        case 0:
            od_block_percent = 60;
            break;
        case 1:
        default:
            od_block_percent = 70;
            break;
        case 2:
            od_block_percent = 80;
            break;
    }
    m_pHi_od->av_od_init(od_block_percent);
    if(thread_od_state == false)
    {
        thread_od_state = true;
        pthread_create(&thread_od_id, 0, av_occlusion_detection_body, this);
    }
    
    return 0;
}

int CHiAvOd::Av_stop_ive_od(int phy_video_chn)
{
    if( (phy_video_chn == m_od_chn_info->chn_id) && (true == thread_od_state) )
    {
        thread_od_state = false;
        pthread_join(thread_od_id, NULL);
        m_pHi_od->av_od_uninit();
    }

    return 0;
}


unsigned int CHiAvOd::Av_get_vda_od_counter_value(int phy_video_Chn)
{
    if(phy_video_Chn == m_od_chn_info->chn_id)
    {
        return m_od_counter;
    }

    return 0;
}

int CHiAvOd::Av_reset_vda_od_counter(int phy_video_Chn)
{
    if(phy_video_Chn == m_od_chn_info->chn_id)
    {
        return m_od_counter = 0;
    }

    return 0;    
}

int CHiAvOd::Av_occlusion_detection_proc()
{
    const HI_U8 unocc_frame_threshold = 1;
    HI_U64 first_detect_frame_pts_per_secnod = 0;
    HI_U8 frame_count_per_secnod = 0;
    HI_U8 frame_occ_count_per_secnod = 0;
    sleep(5);
    while(thread_od_state)
    {
        do
        {
            if(NULL != m_pHi_vpss)
            {
                VIDEO_FRAME_INFO_S frame;
                memset(&frame, 0, sizeof(VIDEO_FRAME_INFO_S));
                
                if(0 != m_pHi_vpss->HiVpss_get_frame( _VP_SUB_STREAM_,m_od_chn_info->chn_id, &frame))
                {
                    DEBUG_ERROR("vpss get frame failed ! \n");
                    break;
                }

                if(frame.stVFrame.u64pts - first_detect_frame_pts_per_secnod >= 1000000)
                {
                    DEBUG_MESSAGE("1s passed frame count is:%d occ frame count is:%d \n", frame_count_per_secnod, frame_occ_count_per_secnod);
                    first_detect_frame_pts_per_secnod = frame.stVFrame.u64pts;
                    if((frame_count_per_secnod > unocc_frame_threshold) && (frame_count_per_secnod - frame_occ_count_per_secnod <= unocc_frame_threshold))
                    {
                        DEBUG_MESSAGE("this frame is occ happend! \n");
                        ++m_od_counter;
                    }
                    frame_count_per_secnod = 0;
                    frame_occ_count_per_secnod = 0;
                }

                ++frame_count_per_secnod;
                if(1 == m_pHi_od->av_od_analyse_frame(frame))
                {
                    ++frame_occ_count_per_secnod;
                }
                
                if(HI_SUCCESS != m_pHi_vpss->HiVpss_release_frame(_VP_SUB_STREAM_,m_od_chn_info->chn_id,&frame))
                {
                    DEBUG_ERROR("vpss get frame failed ! \n");
                    break;
                }
            }
        }while(0);
        usleep(80*1000);
    }

    return 0;
}


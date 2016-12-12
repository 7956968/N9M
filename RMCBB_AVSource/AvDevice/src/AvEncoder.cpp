#include "AvEncoder.h"
#if defined(_AV_PRODUCT_CLASS_IPC_)
#if defined(_AV_PLATFORM_HI3518C_)
#include "hi_mem.h"
#endif
#else
#ifndef _AV_PLATFORM_HI3520D_V300_
#ifndef _AV_PLATFORM_HI3515_
#include "hi_mem.h"
#endif
#endif
#endif

#ifndef _AV_PLATFORM_HI3515_
#include "HiAvDeviceMaster.h"
#else
#include "HiAvDeviceMaster-3515.h"
#endif

/*used for source insight*/
using namespace Common;


/*receive thread*/
#define _AE_ENCODER_MAX_RECV_THREAD_NUMBER_  9


/*send thread*/
#define _AE_ENCODER_MAX_SEND_THREAD_NUMBER_  4

#define _COUNT_VBR_RATE_TIME_	60	
typedef struct _av_encoder_thread_para_{
    CAvEncoder *pAv_encoder;
    encoder_thread_type_t task_type;
    int task_id;
}av_encoder_thread_para_t;

#if defined(_AV_PLATFORM_HI3515_)

//#define AAC_6AI_3515_TEST

#ifdef AAC_6AI_3515_TEST
	FILE* g_pAudioFile = NULL;
#endif

#endif


CAvEncoder::CAvEncoder()
{

    _AV_FATAL_((m_pRecv_thread_task = new av_recv_thread_task_t[_AE_ENCODER_MAX_RECV_THREAD_NUMBER_]) == NULL, "OUT OF MEMORY\n");
    for(int index = 0; index < _AE_ENCODER_MAX_RECV_THREAD_NUMBER_; index ++)
    {
        m_pRecv_thread_task[index].thread_request = _ATC_REQUEST_UNKNOWN_;
        m_pRecv_thread_task[index].thread_response = _ATC_RESPONSE_UNKNOWN_;
        m_pRecv_thread_task[index].thread_running = 0;
        m_pRecv_thread_task[index].local_task_list.clear();
    }


    m_video_lost_flag.reset();
    _AV_FATAL_((m_pOsd_thread = new av_osd_thread_task_t) == NULL, "OUT OF MEMORY\n");
    m_pOsd_thread->thread_request = _ATC_REQUEST_UNKNOWN_;
    m_pOsd_thread->thread_response = _ATC_RESPONSE_UNKNOWN_;
    m_pOsd_thread->thread_running = 0;

    memset(&m_locked_video_stream_buffer[0][0], 0, sizeof(bool) * 32 * (_AST_MAIN_IFRAME_VIDEO_ + 1));

    _AV_FATAL_((m_pThread_lock = new CAvThreadLock) == NULL, "OUT OF MEMORY\n");
    _AV_FATAL_((m_pAv_font_library = CAvFontLibrary::Afl_get_instance()) == NULL, "OUT OF MEMORY\n");
    memset(m_code_rate_count,0,sizeof(m_code_rate_count));

#if defined(_AV_PLATFORM_HI3515_)
    memset(m_OsdCache, 0x0, sizeof(m_OsdCache));
	m_OsdCharMap.clear();
	m_OsdFontLibMap.clear();

	m_OsdCharMap['0'] = 0;
	m_OsdCharMap['1'] = 1;
	m_OsdCharMap['2'] = 2;
	m_OsdCharMap['3'] = 3;
	m_OsdCharMap['4'] = 4;
	m_OsdCharMap['5'] = 5;
	m_OsdCharMap['6'] = 6;
	m_OsdCharMap['7'] = 7;
	m_OsdCharMap['8'] = 8;
	m_OsdCharMap['9'] = 9;
	m_OsdCharMap['-'] = 10;
	m_OsdCharMap[':'] = 11;
	m_OsdCharMap['/'] = 12;
	m_OsdCharMap['.'] = 13;
	if(PTD_6A_I == pgAvDeviceInfo->Adi_product_type())
	{
		m_OsdCharMap['U'] = 14;
		m_OsdCharMap['S'] = 15;
		m_OsdCharMap['B'] = 16;
		m_OsdCharMap['h'] = 17;
		m_OsdCharMap['m'] = 18;
		m_OsdCharMap['k'] = 19;
	}
	else
	{
		m_OsdCharMap['G'] = 14;
		m_OsdCharMap['P'] = 15;
		m_OsdCharMap['S'] = 16;
		m_OsdCharMap['A'] = 17;
		m_OsdCharMap['M'] = 18;
		m_OsdCharMap['K'] = 19;
	}
	
	m_OsdFontLibMap[0x01] = 0;
	m_OsdFontLibMap[0xffff] = 1;
#ifdef AAC_6AI_3515_TEST
	g_pAudioFile = fopen("aac_audio_test.aac", "w+");
#endif	
#endif
}


CAvEncoder::~CAvEncoder()
{
    _AV_SAFE_DELETE_ARRAY_(m_pRecv_thread_task);

    _AV_SAFE_DELETE_(m_pOsd_thread);

    _AV_SAFE_DELETE_(m_pThread_lock);
    //_AV_SAFE_DELETE_(m_pAv_font_library);
#if defined(_AV_PLATFORM_HI3515_)
	for(int i = 0 ; i < OSD_CACHE_ITEM_NUM ; i++)
	{
		for(int j = 0 ; j < OSD_CACHE_FONT_LIB_NUM ; j++)
		{
			for(int k = 0 ; k < _AR_CIF_ + 1 ; k++)
			{
				for(int m = 0 ; m < _AV_FONT_NAME_16_ + 1 ; m++)
				{
					if(m_OsdCache[i][j][k][m].szOsd != NULL)
					{
						delete[] m_OsdCache[i][j][k][m].szOsd;
						m_OsdCache[i][j][k][m].szOsd = NULL;
					}
				}
			}
		}
	}
#endif
}


int CAvEncoder::Ae_get_task_id(av_video_stream_type_t video_stream_type, int phy_video_chn_num, int *pRecv_task_id/* = NULL*/, int *pSend_task_id/* = NULL*/)
{
    _AV_POINTER_ASSIGNMENT_(pRecv_task_id, -1);
    _AV_POINTER_ASSIGNMENT_(pSend_task_id, -1);

    if( video_stream_type == _AST_SNAP_VIDEO_ )
    {
        //_AE_ENCODER_MAX_RECV_THREAD_NUMBER_-2 maybe is for talk back
        _AV_POINTER_ASSIGNMENT_(pRecv_task_id, _AE_ENCODER_MAX_RECV_THREAD_NUMBER_ - 2);
        _AV_POINTER_ASSIGNMENT_(pSend_task_id, _AE_ENCODER_MAX_SEND_THREAD_NUMBER_ - 2);
    }
    else
    {
        _AV_POINTER_ASSIGNMENT_(pRecv_task_id, phy_video_chn_num / 4);
        _AV_POINTER_ASSIGNMENT_(pSend_task_id, 0);
    }
    _AV_FATAL_((pRecv_task_id != NULL) && (*pRecv_task_id >= _AE_ENCODER_MAX_RECV_THREAD_NUMBER_), "CAvEncoder::Ae_get_task_id Invalid recv task id(%d)(%d)\n", *pRecv_task_id, _AE_ENCODER_MAX_RECV_THREAD_NUMBER_);
    _AV_FATAL_((pSend_task_id != NULL) && (*pSend_task_id >= _AE_ENCODER_MAX_SEND_THREAD_NUMBER_), "CAvEncoder::Ae_get_task_id Invalid send task id(%d)(%d)\n", *pSend_task_id, _AE_ENCODER_MAX_SEND_THREAD_NUMBER_);

    return 0;
}


int CAvEncoder::Ae_get_task_id(av_audio_stream_type_t audio_stream_type/* = _AAST_TALKBACK_*/, int phy_audio_chn_num /*= -1*/, int *pRecv_task_id /*= NULL*/, int *pSend_task_id /*= NULL*/)
{
    _AV_ASSERT_(audio_stream_type == _AAST_TALKBACK_, "CAvEncoder::Ae_get_task_id I only support talkback\n");

    _AV_POINTER_ASSIGNMENT_(pSend_task_id, -1);
    _AV_POINTER_ASSIGNMENT_(pRecv_task_id, (_AE_ENCODER_MAX_RECV_THREAD_NUMBER_ - 1));

    return 0;
}


void *Ae_encoder_thread_entry(void *pPara)
{
    av_encoder_thread_para_t *pAv_encoder_para = (av_encoder_thread_para_t *)pPara;

    switch(pAv_encoder_para->task_type)
    {
        case _ETT_STREAM_REC_:/*recv*/			
			prctl(PR_SET_NAME, "Ae_encoder_recv");
            pAv_encoder_para->pAv_encoder->Ae_encoder_recv_thread(pAv_encoder_para->task_id);
            break;
        case _ETT_OSD_OVERLAY_:/*osd*/			
			prctl(PR_SET_NAME, "Ae_encoder_osd");
            pAv_encoder_para->pAv_encoder->Ae_osd_thread(pAv_encoder_para->task_id);
            break;

        default:
            _AV_FATAL_(1, "Ae_encoder_thread_entry Invalid task type(%d)\n", pAv_encoder_para->task_type);
            break;
    }

    delete pAv_encoder_para;

    return NULL;
}

int CAvEncoder::Ae_osd_thread(int task_id)
{
    av_osd_thread_task_t *pAv_thread_task = m_pOsd_thread;
#if defined(_AV_PLATFORM_HI3515_)
    std::list<av_osd_item_t> *av_osd_list;
#else
    std::list<av_osd_item_t>::iterator av_osd_item;
#endif
    datetime_t last_date_time;
    datetime_t cur_date_time;
    
    last_date_time.second = 0xff;

    DEBUG_MESSAGE( "CAvEncoder::Ae_osd_thread\n");

    while(1)
    {
        /******************************thread control*******************************/
_CHECK_THREAD_STATE_:
        switch(pAv_thread_task->thread_request)
        {
            case _ATC_REQUEST_RUNNING_:
                break;

            case _ATC_REQUEST_PAUSE_:
                pAv_thread_task->thread_response = _ATC_RESPONSE_PAUSED_;
                usleep(1000 * 10);
                goto _CHECK_THREAD_STATE_;
                break;

            case _ATC_REQUEST_EXIT_:
                pAv_thread_task->thread_response = _ATC_RESPONSE_EXITED_;
                goto _EXIT_THREAD_;
                break;

            default:
                _AV_FATAL_(1, "CAvEncoder::Ae_osd_thread Invalid request(%d)\n", pAv_thread_task->thread_request);
                break;
        }

        /******************************task*******************************/
        pgAvDeviceInfo->Adi_get_date_time(&cur_date_time);        
        if(cur_date_time.second != last_date_time.second)
        {
            memcpy(&last_date_time, &cur_date_time, sizeof(datetime_t));
#if defined(_AV_PLATFORM_HI3515_)
            av_osd_list = &(pAv_thread_task->osd_list);

            Ae_create_osd_region(av_osd_list);
            //DEBUG_MESSAGE("create osd complete!\n");
            Ae_osd_overlay(*av_osd_list);
            //DEBUG_MESSAGE("overlay osd complete!\n");

#else //! non 3515
            av_osd_item = pAv_thread_task->osd_list.begin();
            while(av_osd_item != pAv_thread_task->osd_list.end())
            {
_AGAIN_:
                switch(av_osd_item->overlay_state)
                {
                    case _AOS_INIT_:
#if defined(_AV_PLATFORM_HI3520D_V300_)		//6AII_AV12机型的OSD叠加特殊处理
						if(OSD_TEST_FLAG == pgAvDeviceInfo->Adi_product_type())
						{
	                        if(Ae_create_extend_osd_region(av_osd_item) == 0)
							{
								av_osd_item->overlay_state = _AOS_CREATED_;
								goto _AGAIN_;
							}
						}
						else
#endif						
						{
	                        if(Ae_create_osd_region(av_osd_item) == 0)
	                        {
	                            av_osd_item->overlay_state = _AOS_CREATED_;
	                            goto _AGAIN_;
	                        }
                        }
                        break;
                    case _AOS_CREATED_:
#if defined(_AV_PLATFORM_HI3520D_V300_)	//6AII_AV12机型的OSD叠加特殊处理
						if(OSD_TEST_FLAG == pgAvDeviceInfo->Adi_product_type())
						{
							if(Ae_extend_osd_overlay(av_osd_item) == 0)
							{
								av_osd_item->overlay_state = _AOS_OVERLAYED_;
							}
						}
						else
#endif
						{
	                        if(Ae_osd_overlay(av_osd_item) == 0)
	                        {
	                            av_osd_item->overlay_state = _AOS_OVERLAYED_;
	                        }
                        }
                        break;
                    case _AOS_OVERLAYED_:
#if defined(_AV_PLATFORM_HI3520D_V300_)	//6AII_AV12机型的OSD叠加特殊处理
						if(OSD_TEST_FLAG == pgAvDeviceInfo->Adi_product_type())
						{
							Ae_extend_osd_overlay(av_osd_item);
						}
						else
#endif
						{
#if defined(_AV_PRODUCT_CLASS_IPC_)
	                        if(av_osd_item->is_update)
#else
	                        if((av_osd_item->osd_name == _AON_DATE_TIME_) || (av_osd_item->osd_name == _AON_GPS_INFO_) || (av_osd_item->osd_name == _AON_SPEED_INFO_)|| (av_osd_item->osd_name == _AON_COMMON_OSD1_) || (av_osd_item->osd_name == _AON_COMMON_OSD2_))
#endif
	                        {
								Ae_osd_overlay(av_osd_item);
	                        }
	                    }
                        break;
                    default:
                        break;
                }
                av_osd_item ++;
            }                
#endif //!< end of 3515
            usleep(50*1000);
        }
        usleep(1000 * 50);
    }

_EXIT_THREAD_:

    return 0;
}

int CAvEncoder::Ae_thread_control(encoder_thread_type_t task_type, int task_id, av_thread_control_t request)
{
    av_encoder_thread_para_t *pAv_encoder_para = NULL;
    av_thread_control_t *pThread_request = NULL;
    av_thread_control_t *pThread_response = NULL;
    int *pThread_running = NULL;

    switch(task_type)
    {
        case _ETT_STREAM_REC_:/*recv*/
            pThread_request = &m_pRecv_thread_task[task_id].thread_request;
            pThread_response = &m_pRecv_thread_task[task_id].thread_response;
            pThread_running = &m_pRecv_thread_task[task_id].thread_running;
            break;
        case _ETT_OSD_OVERLAY_:/*osd*/
            pThread_request = &m_pOsd_thread->thread_request;
            pThread_response = &m_pOsd_thread->thread_response;
            pThread_running = &m_pOsd_thread->thread_running;
            break;
        default:
            _AV_FATAL_(1, "CAvEncoder::Ae_thread_control You must give the implement\n");
            break;
    }

    switch(request)
    {
        case _ATC_REQUEST_RUNNING_:
            *pThread_request = _ATC_REQUEST_RUNNING_;
            if(*pThread_running == 0)
            {
                _AV_FATAL_((pAv_encoder_para = new av_encoder_thread_para_t) == NULL, "OUT OF MEMORY\n");
                pAv_encoder_para->task_id = task_id;
                pAv_encoder_para->pAv_encoder = this;
                pAv_encoder_para->task_type = task_type;
                if(Av_create_normal_thread(Ae_encoder_thread_entry, (void *)pAv_encoder_para, NULL) == 0)
                {
                    *pThread_running = 1;
                }
                else
                {
                    DEBUG_ERROR( "CAvEncoder::Ae_thread_control Av_create_normal_thread FAILED\n");
                    return -1;
                }
            }
            else
            {
                return 0;
            }
            break;

        case _ATC_REQUEST_PAUSE_:
            if(*pThread_running != 0)
            {
                *pThread_response = _ATC_RESPONSE_UNKNOWN_;
                *pThread_request = _ATC_REQUEST_PAUSE_;
                while(*pThread_response != _ATC_RESPONSE_PAUSED_)
                {
                    usleep(0);
                }
            }
            else
            {
                return 0;
            }
            break;

        case _ATC_REQUEST_EXIT_:
            if(*pThread_running != 0)
            {
                *pThread_response = _ATC_RESPONSE_UNKNOWN_;
                *pThread_request = _ATC_REQUEST_EXIT_;
                while(*pThread_response != _ATC_RESPONSE_EXITED_)
                {
                    usleep(0);
                }
                *pThread_running = 0;
            }
            else
            {
                return 0;
            }
            break;

        default:
            _AV_FATAL_(1, "CAvEncoder::Ae_thread_control You must give the implement\n");
            break;
    }

    return 0;
}


HANDLE CAvEncoder::Ae_create_stream_buffer(av_audio_stream_type_t audio_stream_type/* = _AAST_TALKBACK_*/, int phy_audio_chn_num /*= -1*/, EnumSharememoryMode mode /*= SHAREMEM_WRITE_MODE*/)
{
    _AV_ASSERT_(audio_stream_type == _AAST_TALKBACK_, "CAvEncoder::Ae_create_stream_buffer I only support talkback\n");
    phy_audio_chn_num = 0;
    std::map<av_audio_stream_type_t, std::map<int, HANDLE> >::iterator audio_type_it;
    std::map<int, HANDLE>::iterator stream_buffer_it;

    if(mode == SHAREMEM_WRITE_MODE)
    {
        audio_type_it = m_audio_stream_buffer.find(audio_stream_type);
        if(audio_type_it != m_audio_stream_buffer.end())
        {
            stream_buffer_it = audio_type_it->second.find(phy_audio_chn_num);
            if(stream_buffer_it != audio_type_it->second.end())
            {
                N9M_SHLockWriter(m_audio_stream_buffer[audio_stream_type][phy_audio_chn_num]);
                N9M_SHClearAllFrames(m_audio_stream_buffer[audio_stream_type][phy_audio_chn_num]);
                return m_audio_stream_buffer[audio_stream_type][phy_audio_chn_num];
            }
        }
    }
    else
    {
        audio_type_it = m_read_audio_stream_buffer.find(audio_stream_type);
        if(audio_type_it != m_read_audio_stream_buffer.end())
        {
            stream_buffer_it = audio_type_it->second.find(phy_audio_chn_num);
            if(stream_buffer_it != audio_type_it->second.end())
            {
                return m_read_audio_stream_buffer[audio_stream_type][phy_audio_chn_num];
            }
        }
    }

    char stream_buffer_id[32];
    char stream_buffer_name[32];
    unsigned int stream_buffer_size = 0;
    unsigned int stream_buffer_frame = 0;

    sprintf(stream_buffer_id, "avstream");
    if(pgAvDeviceInfo->Adi_get_stream_buffer_info(BUFFER_TYPE_TALKGET_STREAM, 0, stream_buffer_name, stream_buffer_size, stream_buffer_frame) != 0)
    {
        DEBUG_ERROR( "get buffer failed!\n");
        return NULL;
    }

    HANDLE buffer_handle = N9M_SHCreateShareBuffer(stream_buffer_id, stream_buffer_name, stream_buffer_size, stream_buffer_frame, mode);
    if(mode == SHAREMEM_WRITE_MODE)
    {
        m_audio_stream_buffer[audio_stream_type][phy_audio_chn_num] = buffer_handle;
        N9M_SHLockWriter(m_audio_stream_buffer[audio_stream_type][phy_audio_chn_num]);
        N9M_SHClearAllFrames(m_audio_stream_buffer[audio_stream_type][phy_audio_chn_num]);
    }
    else
    {
        m_read_audio_stream_buffer[audio_stream_type][phy_audio_chn_num] = buffer_handle;
    }
    return buffer_handle;
}

int CAvEncoder::Ae_destory_stream_buffer(HANDLE Stream_buffer_handle, av_audio_stream_type_t audio_stream_type/* = _AAST_TALKBACK_*/, int phy_audio_chn_num/* = -1*/)
{
    N9M_SHUnlockWriter(Stream_buffer_handle);
    /*do nothing*/
    return 0;
}

HANDLE CAvEncoder::Ae_create_stream_buffer(av_video_stream_type_t video_stream_type, int phy_video_chn_num, EnumSharememoryMode mode /*= SHAREMEM_WRITE_MODE*/)
{
    std::map<av_video_stream_type_t, std::map<int, HANDLE> >::iterator video_type_it;
    std::map<int, HANDLE>::iterator stream_buffer_it;

    if(mode == SHAREMEM_WRITE_MODE)
    {
        video_type_it = m_video_stream_buffer.find(video_stream_type);
        if(video_type_it != m_video_stream_buffer.end())
        {
            stream_buffer_it = video_type_it->second.find(phy_video_chn_num);
            if(stream_buffer_it != video_type_it->second.end())
            {
                if(0 == N9M_SHLockWriter(m_video_stream_buffer[video_stream_type][phy_video_chn_num]))
                {
                    m_locked_video_stream_buffer[video_stream_type][phy_video_chn_num] = true;
                }
                // DEBUG_MESSAGE("\n@@+++++CAvEncoder::Ae_create_stream_buffer(Find shared buffer)::m_locked_video_stream_buffer[%d][%d] = %d+++++++\n", video_stream_type, phy_video_chn_num,m_locked_video_stream_buffer[video_stream_type][phy_video_chn_num]);

                if(m_locked_video_stream_buffer[video_stream_type][phy_video_chn_num])
                {
                    // DEBUG_MESSAGE("\n@@++++++++CAvEncoder::Ae_create_stream_buffer(Find shared buffer) ClearAllFrames++++++++++\n");
                    N9M_SHClearAllFrames(m_video_stream_buffer[video_stream_type][phy_video_chn_num]);
                }
                return m_video_stream_buffer[video_stream_type][phy_video_chn_num];
            }
        }
    }
    else
    {
        video_type_it = m_read_video_stream_buffer.find(video_stream_type);
        if(video_type_it != m_read_video_stream_buffer.end())
        {
            stream_buffer_it = video_type_it->second.find(phy_video_chn_num);
            if(stream_buffer_it != video_type_it->second.end())
            {
                return m_read_video_stream_buffer[video_stream_type][phy_video_chn_num];
            }
        }
    }

    char stream_buffer_id[32];
    char stream_buffer_name[32];
    unsigned int stream_buffer_size = 0;
    unsigned int stream_buffer_frame = 0;

    sprintf(stream_buffer_id, "avstream");
    switch(video_stream_type)
    {
        case _AST_MAIN_VIDEO_:
            if(pgAvDeviceInfo->Adi_get_stream_buffer_info(BUFFER_TYPE_MAIN_STREAM, phy_video_chn_num, stream_buffer_name, stream_buffer_size, stream_buffer_frame) != 0)
            {
                DEBUG_ERROR( "get buffer failed!\n");
                return NULL;
            }
            break;
         case _AST_MAIN_IFRAME_VIDEO_:
             if(pgAvDeviceInfo->Adi_get_stream_buffer_info(BUFFER_TYPE_MAIN_STREAM_IFRAME, phy_video_chn_num, stream_buffer_name, stream_buffer_size, stream_buffer_frame) != 0)
            {
                DEBUG_ERROR( "get buffer failed!\n");
                return NULL;
            }
             break;        
        case _AST_SUB_VIDEO_:
            if(pgAvDeviceInfo->Adi_get_stream_buffer_info(BUFFER_TYPE_ASSIST_STREAM, phy_video_chn_num, stream_buffer_name, stream_buffer_size, stream_buffer_frame) != 0)
            {
                DEBUG_ERROR( "get buffer failed!\n");
                return NULL;
            }
            break;
        case _AST_SUB2_VIDEO_:
            if(pgAvDeviceInfo->Adi_get_stream_buffer_info(BUFFER_TYPE_NET_TRANSFER_STREAM, phy_video_chn_num, stream_buffer_name, stream_buffer_size, stream_buffer_frame) != 0)
            {
                DEBUG_ERROR( "get buffer failed!\n");
                return NULL;
            }
            break;
        case _AST_SNAP_VIDEO_:
#ifdef _AV_PRODUCT_CLASS_HDVR_
            if(pgAvDeviceInfo->Adi_get_stream_buffer_info(BUFFER_TYPE_SNAP_STREAM, phy_video_chn_num, stream_buffer_name, stream_buffer_size, stream_buffer_frame) != 0)
            {
                DEBUG_ERROR( "get buffer failed!\n");
                return NULL;
            }
#else
            if(pgAvDeviceInfo->Adi_get_stream_buffer_info(BUFFER_TYPE_SNAP_STREAM, 0, stream_buffer_name, stream_buffer_size, stream_buffer_frame) != 0)
            {
                DEBUG_ERROR( "get buffer failed!\n");
                return NULL;
            }
#endif
            break;
        default:
            _AV_FATAL_(1, "CAvEncoder::Ae_create_stream_buffer You must give the implement\n");
            break;
    }    

    DEBUG_MESSAGE("stream_buffer_id:%s, stream_buffer_name:%s, mode=%d\n", stream_buffer_id, stream_buffer_name, mode);
    HANDLE buffer_handle = N9M_SHCreateShareBuffer(stream_buffer_id, stream_buffer_name, stream_buffer_size, stream_buffer_frame, mode);
    if(mode == SHAREMEM_WRITE_MODE)
    {
        m_video_stream_buffer[video_stream_type][phy_video_chn_num] = buffer_handle;
        if(0 == N9M_SHLockWriter(m_video_stream_buffer[video_stream_type][phy_video_chn_num]))
        {
            m_locked_video_stream_buffer[video_stream_type][phy_video_chn_num] = true;
        }
        // DEBUG_MESSAGE("\n@@+++++CAvEncoder::Ae_create_stream_buffer(CREATE shared buffer)::m_locked_video_stream_buffer[%d][%d] = %d+++++++\n", video_stream_type, phy_video_chn_num,m_locked_video_stream_buffer[video_stream_type][phy_video_chn_num]);
        if(m_locked_video_stream_buffer[video_stream_type][phy_video_chn_num])
        {
            // DEBUG_MESSAGE("\n@@+++++++CAvEncoder::Ae_create_stream_buffer(CREATE shared buffer)::cleat all frames++++++++\n");
            N9M_SHClearAllFrames(m_video_stream_buffer[video_stream_type][phy_video_chn_num]);
        }
    }
    else
    {
        m_read_video_stream_buffer[video_stream_type][phy_video_chn_num] = buffer_handle;
    }
    return buffer_handle;
}

int CAvEncoder::Ae_destory_stream_buffer(HANDLE Stream_buffer_handle, av_video_stream_type_t video_stream_type, int phy_video_chn_num)
{
    if(0 == N9M_SHUnlockWriter(Stream_buffer_handle))
    {
        m_locked_video_stream_buffer[video_stream_type][phy_video_chn_num] = false;
    }
    else
    {
        DEBUG_ERROR( "CAvEncoder::Ae_destory_stream_buffer()::unlockwriter failed\n");
    }
    /*do nothing*/
    return 0;
}

int CAvEncoder::Ae_start_encoder_local(av_video_stream_type_t video_stream_type/* = _AST_UNKNOWN_*/, int phy_video_chn_num/* = -1*/,
    av_video_encoder_para_t *pVideo_para/* = NULL*/, av_audio_stream_type_t audio_stream_type/* = _AAST_UNKNOWN_*/, 
    int phy_audio_chn_num/* = -1*/, av_audio_encoder_para_t *pAudio_para/* = NULL*/, int audio_only)
{
    std::list<av_audio_video_group_t>::iterator av_group_it;
    std::list<av_video_stream_t>::iterator video_stream_it;
    av_audio_video_group_t av_audio_video_group;
    av_video_stream_t av_video_stream;
    av_osd_item_t av_osd_item;
    int recv_task_id = -1;
    int send_task_id = -1;
    int ret_val = -1;
    char product_group[32] = {0};
    pgAvDeviceInfo->Adi_get_product_group(product_group, sizeof(product_group));
    char cutomername[32] = {0};
    pgAvDeviceInfo->Adi_get_customer_name(cutomername, sizeof(cutomername));
    DEBUG_CRITICAL("customer_name =%s\n",cutomername);//CUSTOMER_NULL CUSTOMER_ZHAOYI
    
    memset(&av_video_stream, 0, sizeof(av_video_stream_t));
        
    if((!((video_stream_type == _AST_UNKNOWN_) || (phy_video_chn_num < 0) || (pVideo_para == NULL))) )//||(pVideo_para->have_audio == 1)) 
    {
        //printf(" start Ae_start_encoder_local \n");
        /*composite encoder(video and audio)*/
        if(Ae_get_task_id(video_stream_type, phy_video_chn_num, &recv_task_id, &send_task_id) != 0)
        {
            DEBUG_ERROR( "CAvEncoder::Ae_start_encoder_local Ae_get_task_id FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
            goto _OVER_;
        }

        //!
        //printf("-----------00000000000--------, recvId = %d, sendId = %d\n", recv_task_id, send_task_id);
        /*create video encoder*/
        if(Ae_create_video_encoder(video_stream_type, phy_video_chn_num, pVideo_para, &av_video_stream.video_encoder_id) != 0)
        {
            DEBUG_ERROR( "CAvEncoder::Ae_start_encoder_local Ae_create_video_encoder FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
            goto _OVER_; //
        }
        //Ae_get_AvThreadLock()->unlock();
        
        /*video stream information*/
        av_video_stream.type = video_stream_type;
        av_video_stream.phy_video_chn_num = phy_video_chn_num;
        memcpy(&av_video_stream.video_encoder_para, pVideo_para, sizeof(av_video_encoder_para_t));
        av_video_stream.stream_buffer_handle = Ae_create_stream_buffer(video_stream_type, phy_video_chn_num, SHAREMEM_WRITE_MODE);
        if(video_stream_type == _AST_MAIN_VIDEO_)
        {
            av_video_stream.iframe_stream_buffer_handle = Ae_create_stream_buffer(_AST_MAIN_IFRAME_VIDEO_, phy_video_chn_num, SHAREMEM_WRITE_MODE);;
        }
        av_video_stream.pFrame_header_buffer = new unsigned char[_AV_FRAME_HEADER_BUFFER_LEN_];
        av_video_stream.first_frame = true;
        
        /*find it's group, they share the audio encoder*/
        av_group_it = m_pRecv_thread_task[recv_task_id].local_task_list.begin();
       while(av_group_it != m_pRecv_thread_task[recv_task_id].local_task_list.end())
        {
            video_stream_it = av_group_it->video_stream.begin();
            while(video_stream_it != av_group_it->video_stream.end())
            {
                if(video_stream_it->phy_video_chn_num == phy_video_chn_num)
                {
                    goto _FIND_OVER_;
                }
                video_stream_it ++;
            }
            av_group_it ++;
        }

_FIND_OVER_:
        /*create audio encoder*/
        if(av_group_it == m_pRecv_thread_task[recv_task_id].local_task_list.end() )// || (pVideo_para->have_audio == 1))
        {
            /*no av group, so I'm sure there is no audio encoder*/ 

             //! only create audio when have audio
            if((video_stream_type == _AST_MAIN_VIDEO_)&&\
                ((!((audio_stream_type == _AAST_UNKNOWN_) || (phy_audio_chn_num < 0) || (pAudio_para == NULL))) && 
                (pVideo_para->have_audio != 0)))

            {
                /*create audio encoder*/                
                if(Ae_create_audio_encoder(audio_stream_type, phy_audio_chn_num, pAudio_para, &av_audio_video_group.audio_stream.audio_encoder_id) != 0)
                {
                    DEBUG_ERROR( "CAvEncoder::Ae_start_encoder_local Ae_create_audio_encoder FAILED\n");
                    goto _OVER_;
                }
                av_audio_video_group.audio_stream.type = audio_stream_type;
                av_audio_video_group.audio_stream.phy_audio_chn_num = phy_audio_chn_num;
                memcpy(&av_audio_video_group.audio_stream.audio_encoder_para, pAudio_para, sizeof(av_audio_encoder_para_t));
                av_audio_video_group.audio_stream.stream_buffer_handle = NULL;
                av_audio_video_group.audio_stream.pFrame_header_buffer = NULL;
            }
            else
            {
                av_audio_video_group.audio_stream.type = _AAST_UNKNOWN_;
                av_audio_video_group.audio_stream.phy_audio_chn_num = -1;
                av_audio_video_group.audio_stream.audio_encoder_id.audio_encoder_fd = -1;
                av_audio_video_group.audio_stream.audio_encoder_id.pAudio_encoder_info = NULL;
                av_audio_video_group.audio_stream.stream_buffer_handle = NULL;
                av_audio_video_group.audio_stream.pFrame_header_buffer = NULL;
            }
            av_audio_video_group.video_stream.push_back(av_video_stream);
            m_pRecv_thread_task[recv_task_id].local_task_list.push_back(av_audio_video_group);
        }
        else  //av group has been created
        {
            if(av_group_it->audio_stream.phy_audio_chn_num < 0)
            {
                if(((phy_audio_chn_num >= 0) && (pAudio_para != NULL)) && (pVideo_para->have_audio != 0))
                {
                    if(Ae_create_audio_encoder(audio_stream_type, phy_audio_chn_num, pAudio_para, &av_group_it->audio_stream.audio_encoder_id) != 0)
                    {
                        DEBUG_ERROR( "CAvEncoder::Ae_start_encoder_local Ae_create_audio_encoder FAILED\n");
                        goto _OVER_;
                    }

                    av_group_it->audio_stream.type = audio_stream_type;
                    av_group_it->audio_stream.phy_audio_chn_num = phy_audio_chn_num;
                    memcpy(&av_group_it->audio_stream.audio_encoder_para, pAudio_para, sizeof(av_audio_encoder_para_t));
                    av_group_it->audio_stream.stream_buffer_handle = NULL;
                    av_group_it->audio_stream.pFrame_header_buffer = NULL;
                }
            }
            else
            {
                if (av_group_it->audio_stream.phy_audio_chn_num == phy_audio_chn_num)
                {
                    //DEBUG_MESSAGE( "CAvEncoder::Ae_start_encoder_local AUDIO CHANNEL(%d)(%d) has started\n", av_group_it->audio_stream.phy_audio_chn_num, phy_audio_chn_num);
                }
                //_AV_ASSERT_(av_group_it->audio_stream.phy_audio_chn_num != phy_audio_chn_num, "CAvEncoder::Ae_start_encoder_local INVALID AUDIO CHANNEL(%d)(%d)\n", av_group_it->audio_stream.phy_audio_chn_num, phy_audio_chn_num);
            }

            av_group_it->video_stream.push_back(av_video_stream);
            //printf("-----push_back(av_video_stream)------\n");
        }

        //if(!((audio_stream_type == _AAST_UNKNOWN_) || (phy_audio_chn_num < 0) || (pAudio_para == NULL)))
        {
        /*osd*/
        m_pOsd_thread->thread_lock->lock();

#if (defined(_AV_PLATFORM_HI3520D_V300_)||defined(_AV_PLATFORM_HI3515_))
		if((pgAvDeviceInfo->Adi_product_type() == OSD_TEST_FLAG)||(pgAvDeviceInfo->Adi_product_type() == PTD_6A_I))	//6AII_AV12机型不使用标准的osd叠加项///
		{
			if((pgAvDeviceInfo->Adi_product_type() == OSD_TEST_FLAG) || (video_stream_type == _AST_MAIN_VIDEO_ && pgAvDeviceInfo->Adi_product_type() == PTD_6A_I))
			{
				for(int index = 0 ; index < OVERLAY_MAX_NUM_VENC ; index++)
				{
					if(pVideo_para->stuExtendOsdInfo[index].ucEnable == 1)
					{
						memset(&av_osd_item, 0x0, sizeof(av_osd_item_t));
						av_osd_item.osd_name = _AON_EXTEND_OSD;
						av_osd_item.extend_osd_id = index;
						av_osd_item.overlay_state = _AOS_INIT_;
						av_osd_item.osdChange = 1;
						memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));
						
						m_pOsd_thread->osd_list.push_back(av_osd_item);
					}
				}
			}
			m_pOsd_thread->thread_lock->unlock();
			ret_val = 0;
			goto _OVER_;
		}
#endif
#if defined(_AV_PLATFORM_HI3515_)
        if (video_stream_type == _AST_MAIN_VIDEO_)
        {
            if(pVideo_para->have_date_time_osd != 0)
            {
                memset(&av_osd_item, 0, sizeof(av_osd_item_t));
                av_osd_item.osd_name = _AON_DATE_TIME_;
                av_osd_item.overlay_state = _AOS_INIT_;

                memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));

                m_pOsd_thread->osd_list.push_back(av_osd_item);
            }

            
            if(pVideo_para->have_channel_name_osd != 0 && video_stream_type != _AST_SUB2_VIDEO_)
            {
                memset(&av_osd_item, 0, sizeof(av_osd_item_t));
                av_osd_item.osd_name = _AON_CHN_NAME_;
                av_osd_item.overlay_state = _AOS_INIT_;

                memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));

                m_pOsd_thread->osd_list.push_back(av_osd_item);
            }
              
            if(pVideo_para->have_gps_osd != 0 && video_stream_type != _AST_SUB2_VIDEO_)
            {
                memset(&av_osd_item, 0, sizeof(av_osd_item_t));
                av_osd_item.osd_name = _AON_GPS_INFO_;
                av_osd_item.overlay_state = _AOS_INIT_;
                memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));

                m_pOsd_thread->osd_list.push_back(av_osd_item);
            }    

            if(pVideo_para->have_speed_osd != 0 && video_stream_type != _AST_SUB2_VIDEO_)
            {
                memset(&av_osd_item, 0, sizeof(av_osd_item_t));
                av_osd_item.osd_name = _AON_SPEED_INFO_;
                av_osd_item.overlay_state = _AOS_INIT_;
                memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));
                m_pOsd_thread->osd_list.push_back(av_osd_item);

            }

            if(pVideo_para->have_bus_number_osd != 0 && video_stream_type != _AST_SUB2_VIDEO_)
            {
                memset(&av_osd_item, 0, sizeof(av_osd_item_t));
                av_osd_item.osd_name = _AON_BUS_NUMBER_;
                av_osd_item.overlay_state = _AOS_INIT_;
                memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));
                m_pOsd_thread->osd_list.push_back(av_osd_item);
            }

            if(pVideo_para->have_bus_selfnumber_osd != 0 && video_stream_type != _AST_SUB2_VIDEO_)
            {
                memset(&av_osd_item, 0, sizeof(av_osd_item_t));
                av_osd_item.osd_name = _AON_SELFNUMBER_INFO_;
                av_osd_item.overlay_state = _AOS_INIT_;
                memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));
                m_pOsd_thread->osd_list.push_back(av_osd_item);
            }

        
            if(pVideo_para->have_common1_osd != 0 && video_stream_type != _AST_SUB2_VIDEO_)
            {
                memset(&av_osd_item, 0, sizeof(av_osd_item_t));
                av_osd_item.osd_name = _AON_COMMON_OSD1_;
                av_osd_item.overlay_state = _AOS_INIT_;
               
                memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));

                m_pOsd_thread->osd_list.push_back(av_osd_item);
            }

            
            if(pVideo_para->have_common2_osd != 0 && video_stream_type != _AST_SUB2_VIDEO_) 
            {
                memset(&av_osd_item, 0, sizeof(av_osd_item_t));
                av_osd_item.osd_name = _AON_COMMON_OSD2_;
                av_osd_item.overlay_state = _AOS_INIT_;

                memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));

                m_pOsd_thread->osd_list.push_back(av_osd_item);
            }        

            if(pVideo_para->have_water_mark_osd != 0)
            {
                memset(&av_osd_item, 0, sizeof(av_osd_item_t));
                av_osd_item.osd_name = _AON_WATER_MARK;
                av_osd_item.overlay_state = _AOS_INIT_;
                memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));
                m_pOsd_thread->osd_list.push_back(av_osd_item);
            }
        }

#else  //!< non 3515
        if(pVideo_para->have_date_time_osd != 0)
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_DATE_TIME_;
            av_osd_item.overlay_state = _AOS_INIT_;
#if defined(_AV_PRODUCT_CLASS_IPC_)
            av_osd_item.is_update = true;
#endif
            memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));

            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }

        
#if defined(_AV_PRODUCT_CLASS_IPC_)
        if(0 != pVideo_para->have_channel_name_osd)
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_CHN_NAME_;
            av_osd_item.overlay_state = _AOS_INIT_;
            av_osd_item.is_update = false;
            memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));

            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
#else

        if((strcmp(product_group, "ITS") != 0) && (strcmp(cutomername, "CUSTOMER_04.862") != 0)&& (strcmp(cutomername, "CUSTOMER_04.471") != 0))

        {
            if(pVideo_para->have_channel_name_osd != 0 && video_stream_type != _AST_SUB2_VIDEO_)
            {
                memset(&av_osd_item, 0, sizeof(av_osd_item_t));
                av_osd_item.osd_name = _AON_CHN_NAME_;
                av_osd_item.overlay_state = _AOS_INIT_;

                memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));

                m_pOsd_thread->osd_list.push_back(av_osd_item);
            }
        }
        else
        {
            if(0 != pVideo_para->have_channel_name_osd)
            {
                memset(&av_osd_item, 0, sizeof(av_osd_item_t));
                av_osd_item.osd_name = _AON_CHN_NAME_;
                av_osd_item.overlay_state = _AOS_INIT_;
                memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));

                m_pOsd_thread->osd_list.push_back(av_osd_item);
            }  
        }
        
#endif
             
#if defined(_AV_PRODUCT_CLASS_IPC_)
    if(0 != pVideo_para->have_gps_osd)
    {
        memset(&av_osd_item, 0, sizeof(av_osd_item_t));
        av_osd_item.osd_name = _AON_GPS_INFO_;
        av_osd_item.overlay_state = _AOS_INIT_;

        char customer[32];
        memset(customer, 0, sizeof(customer));
        pgAvDeviceInfo->Adi_get_customer_name(customer, sizeof(customer));

        if(0 == strcmp(customer, "CUSTOMER_CNR"))
        {
            av_osd_item.is_update = true;
        }
        else
        {
            av_osd_item.is_update = false;
        }

        memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));
        m_pOsd_thread->osd_list.push_back(av_osd_item);
    }
#else

    if((strcmp(product_group, "ITS") != 0) && (strcmp(cutomername, "CUSTOMER_04.862") != 0)&& (strcmp(cutomername, "CUSTOMER_04.471") != 0))

    {
        if(pVideo_para->have_gps_osd != 0 && video_stream_type != _AST_SUB2_VIDEO_)
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_GPS_INFO_;
            av_osd_item.overlay_state = _AOS_INIT_;
            memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));
            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
    }
    else
    {
        if(0 != pVideo_para->have_gps_osd)
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_GPS_INFO_;
            av_osd_item.overlay_state = _AOS_INIT_;
            memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));
            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
    }
#endif
    

#if defined(_AV_PRODUCT_CLASS_IPC_)
    if(0 != pVideo_para->have_speed_osd)
    {
        memset(&av_osd_item, 0, sizeof(av_osd_item_t));
        av_osd_item.osd_name = _AON_SPEED_INFO_;
        av_osd_item.overlay_state = _AOS_INIT_;
        av_osd_item.is_update = false;
        memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));
        m_pOsd_thread->osd_list.push_back(av_osd_item);
    }
#else

    if((strcmp(product_group, "ITS") != 0) && (strcmp(cutomername, "CUSTOMER_04.862") != 0)&& (strcmp(cutomername, "CUSTOMER_04.471") != 0))

    {
        if(pVideo_para->have_speed_osd != 0 && video_stream_type != _AST_SUB2_VIDEO_)
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_SPEED_INFO_;
            av_osd_item.overlay_state = _AOS_INIT_;
            memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));
            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
    }
    else
    {
        if(0 != pVideo_para->have_speed_osd)
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_SPEED_INFO_;
            av_osd_item.overlay_state = _AOS_INIT_;
            memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));
            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
    }
 #endif
    

#if defined(_AV_PRODUCT_CLASS_IPC_)
    if(0 != pVideo_para->have_bus_number_osd)
    {
        memset(&av_osd_item, 0, sizeof(av_osd_item_t));
        av_osd_item.osd_name = _AON_BUS_NUMBER_;
        av_osd_item.overlay_state = _AOS_INIT_;
        av_osd_item.is_update = false;
        memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));
        m_pOsd_thread->osd_list.push_back(av_osd_item);
    }
 #else

     if((strcmp(product_group, "ITS") != 0) && (strcmp(cutomername, "CUSTOMER_04.862") != 0)&& (strcmp(cutomername, "CUSTOMER_04.471") != 0))

    {
        if(pVideo_para->have_bus_number_osd != 0 && video_stream_type != _AST_SUB2_VIDEO_)
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_BUS_NUMBER_;
            av_osd_item.overlay_state = _AOS_INIT_;
            memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));
            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
    }
    else
    {
        if(0 != pVideo_para->have_bus_number_osd)
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_BUS_NUMBER_;
            av_osd_item.overlay_state = _AOS_INIT_;
            memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));
            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
    }
 #endif
    

#if defined(_AV_PRODUCT_CLASS_IPC_)
    if(0 != pVideo_para->have_bus_selfnumber_osd)
    {
        memset(&av_osd_item, 0, sizeof(av_osd_item_t));
        av_osd_item.osd_name = _AON_SELFNUMBER_INFO_;
        av_osd_item.overlay_state = _AOS_INIT_;

        av_osd_item.is_update = false;

        memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));
        m_pOsd_thread->osd_list.push_back(av_osd_item);
    }
#else

    if((strcmp(product_group, "ITS") != 0) && (strcmp(cutomername, "CUSTOMER_04.862") != 0)&& (strcmp(cutomername, "CUSTOMER_04.471") != 0))

    {
        if(pVideo_para->have_bus_selfnumber_osd != 0 && video_stream_type != _AST_SUB2_VIDEO_)
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_SELFNUMBER_INFO_;
            av_osd_item.overlay_state = _AOS_INIT_;
            memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));
            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
    }
    else
    {
        if(0 != pVideo_para->have_bus_selfnumber_osd)
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_SELFNUMBER_INFO_;
            av_osd_item.overlay_state = _AOS_INIT_;
            memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));
            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
    }
#endif
    

#if defined(_AV_PRODUCT_CLASS_IPC_)
    if(0 != pVideo_para->have_common1_osd)
    {
        memset(&av_osd_item, 0, sizeof(av_osd_item_t));
        av_osd_item.osd_name = _AON_COMMON_OSD1_;
        av_osd_item.overlay_state = _AOS_INIT_;

        av_osd_item.is_update = false;
           
        memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));

        m_pOsd_thread->osd_list.push_back(av_osd_item);
    }
#else

     if((strcmp(product_group, "ITS") != 0) && (strcmp(cutomername, "CUSTOMER_04.862") != 0)&& (strcmp(cutomername, "CUSTOMER_04.471") != 0))

     {
        if(pVideo_para->have_common1_osd != 0 && video_stream_type != _AST_SUB2_VIDEO_)
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_COMMON_OSD1_;
            av_osd_item.overlay_state = _AOS_INIT_;
           
            memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));

            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
     }
     else
     {
        if(0 != pVideo_para->have_common1_osd)
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_COMMON_OSD1_;
            av_osd_item.overlay_state = _AOS_INIT_;
            
            memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));

            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
     }
#endif
        

#if defined(_AV_PRODUCT_CLASS_IPC_)
    if(0 != pVideo_para->have_common2_osd)
    {
        memset(&av_osd_item, 0, sizeof(av_osd_item_t));
        av_osd_item.osd_name = _AON_COMMON_OSD2_;
        av_osd_item.overlay_state = _AOS_INIT_;

        av_osd_item.is_update = false;
        memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));

        m_pOsd_thread->osd_list.push_back(av_osd_item);
    }
#else

     if((strcmp(product_group, "ITS") != 0) && (strcmp(cutomername, "CUSTOMER_04.862") != 0)&& (strcmp(cutomername, "CUSTOMER_04.471") != 0))
     {
        if(pVideo_para->have_common2_osd != 0 && video_stream_type != _AST_SUB2_VIDEO_) 
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_COMMON_OSD2_;
            av_osd_item.overlay_state = _AOS_INIT_;

            memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));

            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
     }
     else
     {
        if(0 != pVideo_para->have_common2_osd)
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_COMMON_OSD2_;
            av_osd_item.overlay_state = _AOS_INIT_;
            memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));

            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
     }
 #endif
        

        if(pVideo_para->have_water_mark_osd != 0)
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_WATER_MARK;
            av_osd_item.overlay_state = _AOS_INIT_;
            memcpy(&av_osd_item.video_stream, &av_video_stream, sizeof(av_video_stream_t));
            
            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
 #endif  //! <non 3515
        m_pOsd_thread->thread_lock->unlock();
        }
    }
    else
    {
        /*only audio, talkback*/
        if(((audio_stream_type != _AAST_TALKBACK_) || (pAudio_para == NULL)) && (pVideo_para->have_audio != 0)) //!< modified
        {
            DEBUG_ERROR( "CAvEncoder::Ae_start_encoder_local Invalid parameter\n");
            goto _OVER_;
        }

        if(Ae_get_task_id(audio_stream_type, phy_audio_chn_num, &recv_task_id, &send_task_id) != 0)
        {
            DEBUG_ERROR( "CAvEncoder::Ae_start_encoder_local Ae_get_task_id FAILED\n");
            goto _OVER_;
        }

        /*create audio encoder*/
        if(Ae_create_audio_encoder(audio_stream_type, phy_audio_chn_num, pAudio_para, &av_audio_video_group.audio_stream.audio_encoder_id) != 0)
        {
            DEBUG_ERROR( "CAvEncoder::Ae_start_encoder_local Ae_create_audio_encoder FAILED\n");
            goto _OVER_;
        }
        av_audio_video_group.video_stream.clear();
        av_audio_video_group.audio_stream.type = audio_stream_type;
        av_audio_video_group.audio_stream.phy_audio_chn_num = phy_audio_chn_num;
        memcpy(&av_audio_video_group.audio_stream.audio_encoder_para, pAudio_para, sizeof(av_audio_encoder_para_t));
        av_audio_video_group.audio_stream.stream_buffer_handle = Ae_create_stream_buffer(audio_stream_type, phy_audio_chn_num, SHAREMEM_WRITE_MODE);
        av_audio_video_group.audio_stream.pFrame_header_buffer = new unsigned char[_AV_FRAME_HEADER_BUFFER_LEN_];
        m_pRecv_thread_task[recv_task_id].local_task_list.push_back(av_audio_video_group);
    }
    ret_val = 0;

_OVER_:

    return ret_val; 
}

int CAvEncoder::Ae_request_iframe(av_video_stream_type_t video_stream_type, int phy_video_chn_num)
{
    return Ae_request_iframe_local(video_stream_type, phy_video_chn_num);
}

int CAvEncoder::Ae_start_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para,
    av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, av_audio_encoder_para_t *pAudio_para, bool thread_control, int audio_only)
{
    int ret_val = -1; 
    bool bosd_thread_control_flag = true;
    
    if(true == thread_control)
    {
        bosd_thread_control_flag = false;
    }
#ifdef _AV_PRODUCT_CLASS_IPC_
    if( pVideo_para )
    {
        char osd_string[256];

        memset(osd_string, 0, sizeof(osd_string));
        snprintf(osd_string, sizeof(osd_string), "%s%s%s%s%s%s", pVideo_para->bus_number, pVideo_para->channel_name, \
            pVideo_para->gps, pVideo_para->bus_selfnumber, pVideo_para->osd1_content, pVideo_para->osd2_content);
        osd_string[sizeof(osd_string)-1] = '\0';
        
        this->CacheOsdFond(osd_string, bosd_thread_control_flag);

    }
    else
    {
        this->CacheOsdFond( "", bosd_thread_control_flag);
    }
#else

    if( pVideo_para )
    {
#if (defined(_AV_PLATFORM_HI3520D_V300_)||defined(_AV_PLATFORM_HI3515_))		//6AII_AV12机型专用osd叠加///
		if((OSD_TEST_FLAG == pgAvDeviceInfo->Adi_product_type())||(PTD_6A_I == pgAvDeviceInfo->Adi_product_type()))
		{
			for(int index = 0 ; index < OVERLAY_MAX_NUM_VENC ; index++)
			{
				if(pVideo_para->stuExtendOsdInfo[index].ucOsdType == 0)	//日期特殊处理///
				{
					this->CacheOsdFond("0123456789:. ", bosd_thread_control_flag);
				}
				else
				{
					this->CacheOsdFond(pVideo_para->stuExtendOsdInfo[index].szContent, bosd_thread_control_flag);
				}
			}
		}
		else
#endif
		{
	        if(pVideo_para->have_date_time_osd == 1)
	        {
	            this->CacheOsdFond( ":1234567890/-APM ", bosd_thread_control_flag);
	        }
	        if(pVideo_para->have_gps_osd == 1)
	        {
	            this->CacheOsdFond("0123456789: ,.ENSW none invaild", bosd_thread_control_flag);
	            this->CacheOsdFond("LAT LON", bosd_thread_control_flag);
	            this->CacheOsdFond(GuiTKGetText(NULL, NULL, "STR_GPS"), bosd_thread_control_flag);
	            this->CacheOsdFond(GuiTKGetText(NULL, NULL, "STR_GPS INVALID"), bosd_thread_control_flag);
	            this->CacheOsdFond(GuiTKGetText(NULL, NULL, "STR_GPS NOT_EXIST"), bosd_thread_control_flag);
	        }
	        if(pVideo_para->have_bus_number_osd == 1)
	        {
	            this->CacheOsdFond( pVideo_para->bus_number, bosd_thread_control_flag);
	        }
	        if(pVideo_para->have_speed_osd == 1)
	        {
	            this->CacheOsdFond( "0123456789 KM/HMPH", bosd_thread_control_flag);
	        }
		 	if(pVideo_para->have_bus_selfnumber_osd == 1)
	        {
	            this->CacheOsdFond( pVideo_para->bus_selfnumber, bosd_thread_control_flag);
	        }
	        if(pVideo_para->have_channel_name_osd == 1)
	        {
	            this->CacheOsdFond(pVideo_para->channel_name, bosd_thread_control_flag);
	        }
 	        if(pVideo_para->have_water_mark_osd == 1) //!< for 04.471
	        {
	            this->CacheOsdFond(pVideo_para->water_mark_name , bosd_thread_control_flag);
	        }           
	        if(pVideo_para->have_common1_osd == 1 && pVideo_para->osd1_contentfix == 1)
	        {
	            this->CacheOsdFond( pVideo_para->osd1_content, bosd_thread_control_flag);
	        }

	        if(pVideo_para->have_common2_osd == 1 && pVideo_para->osd2_contentfix == 1)
	        {
	            this->CacheOsdFond( pVideo_para->osd2_content, bosd_thread_control_flag);
	        }
	    }
    }
    else
    {
        this->CacheOsdFond( "", bosd_thread_control_flag);
    }
#endif

    //printf("pVideo_para->have_audio=%d\n", pVideo_para->have_audio);
    if((!((video_stream_type == _AST_UNKNOWN_) || (phy_video_chn_num == -1) || (pVideo_para == NULL))) || pVideo_para->have_audio != 0)
    {
        /*start local channel*/
        if(Ae_start_encoder_local(video_stream_type, phy_video_chn_num, pVideo_para, audio_stream_type, phy_audio_chn_num, pAudio_para, audio_only) != 0)
        {
            DEBUG_ERROR( "CAvEncoder::Ae_start_encoder Ae_start_composite_encoder_local FAILED(%d)\n", phy_video_chn_num);
            goto _OVER_;
        }
    }
    else
    {
        if((audio_stream_type != _AAST_TALKBACK_) || (pAudio_para == NULL))
        {
            DEBUG_ERROR( "CAvEncoder::Ae_start_encoder Invalid parameter(type:%d)\n", audio_stream_type);
            goto _OVER_;
        }
        else
        {
            /*only audio, talkback*/
            if(Ae_start_encoder_local(video_stream_type, phy_video_chn_num, pVideo_para, audio_stream_type, phy_audio_chn_num, pAudio_para) != 0)
            {
                DEBUG_ERROR( "CAvEncoder::Ae_start_encoder Ae_start_composite_encoder_local FAILED(%d)\n", phy_video_chn_num);
                goto _OVER_;
            }
        }
    }

    ret_val = 0;

_OVER_:

    return ret_val;
}

int CAvEncoder::Ae_get_osd_content(int phy_video_chn_num, av_osd_name_t osd_name, char *osd_content, unsigned int osd_len)
{
    std::list<av_osd_item_t>::iterator osd_item;

#if !defined(_AV_PRODUCT_CLASS_IPC_)
    char string[32];
    //char gpsstring[64] = {0};
    //strcpy(gpsstring,GuiTKGetText(NULL, NULL, "STR_GPS"));
    //char speedstring[16] = {0};
#endif
    m_pOsd_thread->thread_lock->lock();
    osd_item = m_pOsd_thread->osd_list.begin();

    while(osd_item != m_pOsd_thread->osd_list.end())
    {
        //keep snap osd same with main stream 
        if((_AST_MAIN_VIDEO_ == osd_item->video_stream.type) && (phy_video_chn_num == osd_item->video_stream.phy_video_chn_num) && (osd_name == osd_item->osd_name))
        {
            switch(osd_name)
            {
#if defined(_AV_PRODUCT_CLASS_IPC_)
                case _AON_SPEED_INFO_:
                {
                    int len = sizeof(osd_item->video_stream.video_encoder_para.speed)>osd_len?osd_len:sizeof(osd_item->video_stream.video_encoder_para.speed);
                    memset(osd_content, 0, osd_len);
                    memcpy(osd_content, osd_item->video_stream.video_encoder_para.speed, len);
                    osd_content[osd_len-1] = '\0';
                }
                    break;
                case _AON_GPS_INFO_:
                {
                    int len = sizeof(osd_item->video_stream.video_encoder_para.gps)>osd_len?osd_len:sizeof(osd_item->video_stream.video_encoder_para.gps);
                    memset(osd_content, 0, osd_len);
                    memcpy(osd_content, osd_item->video_stream.video_encoder_para.gps, len);
                    osd_content[osd_len-1] = '\0';  
                }
                    break;
#else
                case _AON_SPEED_INFO_:
                {
                    msgSpeedState_t speedinfo;
                    Tax2DataList tax2speed;
                    memset(&speedinfo,0,sizeof(speedinfo));
                    memset(&tax2speed, 0, sizeof(Tax2DataList));
                    paramSpeedAlarmSetting_t speedsource;
                    memset(&speedsource,0,sizeof(speedsource));
                    pgAvDeviceInfo->Adi_Get_SpeedSource_Info(&speedsource);
                    //printf("[GPS] ucSpeedFrom =%d\n", speedsource.ucSpeedFrom);
                    if(speedsource.ucSpeedFrom == 1)
                    {
                        
                        pgAvDeviceInfo->Adi_Get_Speed_Info(&speedinfo);
                        //printf("[GPS] ucSpeedFrom =%d usSpeedUnit =%d uiKiloSpeed =%d uiMiliSpeed =%d\n", speedsource.ucSpeedFrom, speedinfo.usSpeedUnit, speedinfo.uiKiloSpeed,speedinfo.uiMiliSpeed);
                        memset(string, 0, sizeof(string));
                        if(speedinfo.usSpeedUnit == 0)
                        {
                            snprintf(string, sizeof(string)," %dKM/H",speedinfo.uiKiloSpeed);
                            string[sizeof(string)-1] = '\0';
                        }
                        else
                        {
                            snprintf(string, sizeof(string), " %dMPH",speedinfo.uiMiliSpeed);
                            string[sizeof(string)-1] = '\0';
                        }
                    }
                    else if(speedsource.ucSpeedFrom   == 2)       //from 卫星 and 脉冲///
                    {
                        pgAvDeviceInfo->Adi_Get_Speed_Info(&speedinfo);
                        //printf("[GPS] ucSpeedFrom =%d usSpeedUnit =%d uiKiloSpeed =%d uiMiliSpeed =%d\n", speedsource.ucSpeedFrom, speedinfo.usSpeedUnit, speedinfo.uiKiloSpeed,speedinfo.uiMiliSpeed);
                        memset(string, 0, sizeof(string));
                        if(speedinfo.usSpeedUnit == 0)
                        {
                            snprintf(string, sizeof(string)," %dKM/H",speedinfo.uiKiloSpeed);
                            string[sizeof(string)-1] = '\0';
                        }
                        else
                        {
                            snprintf(string, sizeof(string), " %dMPH",speedinfo.uiMiliSpeed);
                            string[sizeof(string)-1] = '\0';
                        }
                        if(speedinfo.uiKiloSpeed == 0 || speedinfo.uiMiliSpeed == 0)
                        {
                            msgGpsInfo_t gpsinfo;
                            memset(&gpsinfo,0,sizeof(gpsinfo));
                            pgAvDeviceInfo->Adi_Get_Gps_Info(&gpsinfo);
                            if(speedsource.ucUnit == 0)
                            {
                                snprintf(string, sizeof(string)," %dKM/H",gpsinfo.usSpeedKMH);
                                string[sizeof(string)-1] = '\0';
                            }
                            else
                            {
                                snprintf(string, sizeof(string), " %dMPH",gpsinfo.usSpeedMPH);
                                string[sizeof(string)-1] = '\0';
                            }
                        }
                
                    }
                    //0703
                    else if(speedsource.ucSpeedFrom == 3)
                    {
                        
                        pgAvDeviceInfo->Adi_Get_Speed_Info_from_Tax2(&tax2speed);
                        //printf("[GPS] Tax2 ucSpeedFrom =%d usSpeedUnit =%d uiKiloSpeed =%d uiMiliSpeed =%d\n", speedsource.ucSpeedFrom, speedsource.ucUnit, tax2speed.usRealSpeed,tax2speed.usRealSpeed* 62 / 100);
                        memset(string, 0, sizeof(string));
                        if(speedsource.ucUnit == 0)
                        {
                            snprintf(string, sizeof(string)," %dKM/H",tax2speed.usRealSpeed);
                            string[sizeof(string)-1] = '\0';
                        }
                        else
                        {
                            snprintf(string, sizeof(string), " %dMPH",tax2speed.usRealSpeed* 62 / 100);
                            string[sizeof(string)-1] = '\0';
                        }
                    }
                    else
                    {
                        msgGpsInfo_t gpsinfo;
                        memset(&gpsinfo,0,sizeof(gpsinfo));
                        pgAvDeviceInfo->Adi_Get_Gps_Info(&gpsinfo);
                        if(speedsource.ucUnit == 0)
                        {
                            snprintf(string, sizeof(string)," %dKM/H",gpsinfo.usSpeedKMH);
                            string[sizeof(string)-1] = '\0';
                        }
                        else
                        {
                            snprintf(string, sizeof(string), " %dMPH",gpsinfo.usSpeedMPH);
                            string[sizeof(string)-1] = '\0';
                            
                        }
                    }
                    
					char customer[32] = {0};
					pgAvDeviceInfo->Adi_get_customer_name(customer, sizeof(customer));
					if(!strcmp(customer, "CUSTOMER_BJGJ"))	//北京公交站点信息替换gps，速度前加"速度"///
					{
						sprintf(osd_content, "%s%s", GuiTKGetText(NULL, NULL, "STR_GPS_SPEED"), string);
					}
					else
					{
						strncpy(osd_content, string, osd_len);
					}
					
                    osd_content[osd_len-1] = '\0';
                    
                }
                    break;
                case _AON_GPS_INFO_ :
                {
                	char customer[32] = {0};
                	pgAvDeviceInfo->Adi_get_customer_name(customer, sizeof(customer));
                	if(!strcmp(customer, "CUSTOMER_BJGJ"))
                	{
                		std::string strStation = pgAvDeviceInfo->Adi_get_beijing_station_info();
                		strncpy(osd_content, strStation.c_str(), osd_len);
                	}
                	else
                	{
						msgGpsInfo_t gps_info;
						memset(&gps_info,0,sizeof(gps_info));
						pgAvDeviceInfo->Adi_Get_Gps_Info(&gps_info);
						 if(gps_info.ucGpsStatus == 0)
						{
							snprintf(osd_content, osd_len,"%d.%d.%04u%c %d.%d.%04u%c",gps_info.ucLatitudeDegree,gps_info.ucLatitudeCent,gps_info.ulLatitudeSec,gps_info.ucDirectionLatitude ? 'S':'N',gps_info.ucLongitudeDegree,gps_info.ucLongitudeCent,gps_info.ulLongitudeSec,gps_info.ucDirectionLongitude ? 'W':'E');
						}
						else if(gps_info.ucGpsStatus == 1)
						{
							strncpy(osd_content, GuiTKGetText(NULL, NULL, "STR_GPS INVALID"), osd_len);
						}
						else
						{
							strncpy(osd_content, GuiTKGetText(NULL, NULL, "STR_GPS NOT_EXIST"), osd_len);
						}
                	}
                    osd_content[osd_len-1] = '\0';  
                }
                    break;
#endif
                case _AON_BUS_NUMBER_ :
                {
                    strncpy(osd_content, osd_item->video_stream.video_encoder_para.bus_number, osd_len); 
                    osd_content[osd_len-1] = '\0';    
                }
                    break;
                case _AON_CHN_NAME_ :
                {
                    strncpy(osd_content,osd_item->video_stream.video_encoder_para.channel_name, osd_len);  
                    osd_content[osd_len-1] = '\0';  
                }
                    break;

                case _AON_SELFNUMBER_INFO_:
                {
                    strncpy(osd_content, osd_item->video_stream.video_encoder_para.bus_selfnumber, osd_len);  
                    osd_content[osd_len-1] = '\0';  
                }
                    break;
                
                default:
                    DEBUG_ERROR( "the osd:%d  is undefined! \n", osd_name);
                    m_pOsd_thread->thread_lock->unlock();
                    return -1;
                    break;
            } 
            m_pOsd_thread->thread_lock->unlock();
            return 0;
        }
        osd_item++;
    }
    DEBUG_MESSAGE( "the channelNum:%d  is invalid! \n", phy_video_chn_num);
    m_pOsd_thread->thread_lock->unlock();
    return -1;    
}

//static bool gInit_system_pts = true;
int CAvEncoder::Ae_start_encoder(av_video_stream_type_t video_stream_type/* = _AST_UNKNOWN_*/, int phy_video_chn_num/* = -1*/,
av_video_encoder_para_t *pVideo_para/* = NULL*/, av_audio_stream_type_t audio_stream_type/* = _AAST_UNKNOWN_*/, 
int phy_audio_chn_num/* = -1*/, av_audio_encoder_para_t *pAudio_para/* = NULL*/, int audio_only)
{
    return Ae_start_encoder(video_stream_type, phy_video_chn_num, pVideo_para, audio_stream_type, phy_audio_chn_num, pAudio_para, true, audio_only);
}

int CAvEncoder::Ae_dynamic_modify_local_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, bool bosd_need_change)
{
    std::list<av_audio_video_group_t>::iterator group_it;
    std::list<av_video_stream_t>::iterator video_it;
    std::list<av_osd_item_t>::iterator osd_item;
    av_osd_item_t av_osd_item;
    int recv_task_id = -1;
    bool change_osd = false;
    int ret_val = -1;
    
    char customer[32];
    memset(customer, 0, sizeof(customer));
    pgAvDeviceInfo->Adi_get_customer_name(customer, sizeof(customer));//!<  for BJGJ, 20160615

    if(Ae_get_task_id(video_stream_type, phy_video_chn_num, &recv_task_id) != 0)
    {
        DEBUG_ERROR( "CAvEncoder::Ae_dynamic_modify_local_encoder Ae_get_task_id FAILED(type:%d, chn:%d)\n", video_stream_type, phy_video_chn_num);
        return -1;
    }

    /*local*/
    group_it = m_pRecv_thread_task[recv_task_id].local_task_list.begin();
    while(group_it != m_pRecv_thread_task[recv_task_id].local_task_list.end())
    {
        video_it = group_it->video_stream.begin();
        while(video_it != group_it->video_stream.end())
        {
            if((video_it->type == video_stream_type) && (video_it->phy_video_chn_num == phy_video_chn_num))
            {
                break;
            }

            video_it ++;
        }

        if(video_it != group_it->video_stream.end())
        {
            break;
        }

        group_it ++;
    }

    if(group_it == m_pRecv_thread_task[recv_task_id].local_task_list.end())
    {
        DEBUG_ERROR( "CAvEncoder::Ae_dynamic_modify_local_encoder NO ECODER(%d)(%d)\n", video_stream_type, phy_video_chn_num);
        return -1;
    }
    /*add for adapt encoder function*/
    if(true == bosd_need_change)
    {
        goto _MODIFY_OSD_;
    }
    if(video_it->video_encoder_para.frame_rate != pVideo_para->frame_rate)
    {
        goto _DYNAMIC_ADJUST_;
    }

    if(video_it->video_encoder_para.gop_size != pVideo_para->gop_size)
    {
        goto _DYNAMIC_ADJUST_;
    }

    if(video_it->video_encoder_para.bitrate_mode != pVideo_para->bitrate_mode)
    {
        goto _DYNAMIC_ADJUST_;
    }
#ifdef _AV_PRODUCT_CLASS_IPC_
    if(video_it->video_encoder_para.source_frame_rate!= pVideo_para->source_frame_rate)
    {
        goto _DYNAMIC_ADJUST_;
    }
#endif

    switch(pVideo_para->bitrate_mode)
    {
        case _ABM_CBR_:
        default:
            if(video_it->video_encoder_para.bitrate.hicbr.bitrate != pVideo_para->bitrate.hicbr.bitrate)
            {
                goto _DYNAMIC_ADJUST_;
            }
            break;

        case _ABM_VBR_:
            if(video_it->video_encoder_para.bitrate.hivbr.bitrate != pVideo_para->bitrate.hivbr.bitrate)
            {
                goto _DYNAMIC_ADJUST_;
            }
            break;
    }
    
    goto _MODIFY_OSD_;

_DYNAMIC_ADJUST_:
    ret_val = Ae_dynamic_modify_encoder_local(video_stream_type, phy_video_chn_num, pVideo_para);
    if(ret_val != 0)
    {
        return -1;
    }

_MODIFY_OSD_:
#ifdef _AV_PRODUCT_CLASS_IPC_
    if((video_it->video_encoder_para.have_channel_name_osd != pVideo_para->have_channel_name_osd) || \
                (video_it->video_encoder_para.channel_name_osd_x != pVideo_para->channel_name_osd_x) || \
                        (video_it->video_encoder_para.channel_name_osd_y != pVideo_para->channel_name_osd_y) || \
                            (memcmp(video_it->video_encoder_para.channel_name, pVideo_para->channel_name, MAX_OSD_NAME_SIZE) != 0))	
#else

#if defined (_AV_PLATFORM_HI3515_)
    if( (video_it->type == _AST_MAIN_VIDEO_) && ((video_it->video_encoder_para.have_channel_name_osd != pVideo_para->have_channel_name_osd) || \
            (video_it->video_encoder_para.channel_name_osd_x != pVideo_para->channel_name_osd_x) || \
                    (video_it->video_encoder_para.channel_name_osd_y != pVideo_para->channel_name_osd_y) || \
                        (memcmp(video_it->video_encoder_para.channel_name, pVideo_para->channel_name, MAX_OSD_NAME_SIZE) != 0)))

#else

    if( /*(video_it->type != _AST_SUB2_VIDEO_) && */((video_it->video_encoder_para.have_channel_name_osd != pVideo_para->have_channel_name_osd) || \
                (video_it->video_encoder_para.channel_name_osd_x != pVideo_para->channel_name_osd_x) || \
                        (video_it->video_encoder_para.channel_name_osd_y != pVideo_para->channel_name_osd_y) || \
                            (memcmp(video_it->video_encoder_para.channel_name, pVideo_para->channel_name, MAX_OSD_NAME_SIZE) != 0)))
#endif 

#endif							
    {
        m_pOsd_thread->thread_lock->lock();
        /*channel name*/
        Ae_thread_control(_ETT_OSD_OVERLAY_, 0, _ATC_REQUEST_PAUSE_);
        change_osd = true;
        if(video_it->video_encoder_para.have_channel_name_osd != 0)
        {
            osd_item = m_pOsd_thread->osd_list.begin();
            while(osd_item != m_pOsd_thread->osd_list.end())
            {
                if((osd_item->video_stream.type == video_stream_type) && (osd_item->video_stream.phy_video_chn_num == phy_video_chn_num) && \
                                        (osd_item->osd_name == _AON_CHN_NAME_))
                {
                    Ae_destroy_osd_region(osd_item);
                    osd_item = m_pOsd_thread->osd_list.erase(osd_item);
                    break;
                }
                osd_item ++;
            }
        }

        if(0 != pVideo_para->have_channel_name_osd)
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_CHN_NAME_;
            av_osd_item.overlay_state = _AOS_INIT_;
#if defined(_AV_PRODUCT_CLASS_IPC_)
                        av_osd_item.is_update = false;
#endif
            memcpy(&av_osd_item.video_stream, &(*video_it), sizeof(av_osd_item.video_stream));
            memcpy(&av_osd_item.video_stream.video_encoder_para, pVideo_para, sizeof(av_video_encoder_para_t));
            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
        m_pOsd_thread->thread_lock->unlock();
    }
#if defined (_AV_PLATFORM_HI3515_)
    if( (video_it->type == _AST_MAIN_VIDEO_)&&(video_it->video_encoder_para.have_date_time_osd != pVideo_para->have_date_time_osd) || \
                (video_it->video_encoder_para.date_mode != pVideo_para->date_mode) || \
                            (video_it->video_encoder_para.time_mode != pVideo_para->time_mode) || \
                                    (video_it->video_encoder_para.date_time_osd_x != pVideo_para->date_time_osd_x) || \
                                            (video_it->video_encoder_para.date_time_osd_y != pVideo_para->date_time_osd_y))
#else
    if((video_it->video_encoder_para.have_date_time_osd != pVideo_para->have_date_time_osd) || \
                (video_it->video_encoder_para.date_mode != pVideo_para->date_mode) || \
                            (video_it->video_encoder_para.time_mode != pVideo_para->time_mode) || \
                                    (video_it->video_encoder_para.date_time_osd_x != pVideo_para->date_time_osd_x) || \
                                            (video_it->video_encoder_para.date_time_osd_y != pVideo_para->date_time_osd_y))
#endif
    {
        /*date/time*/
        m_pOsd_thread->thread_lock->lock();
        Ae_thread_control(_ETT_OSD_OVERLAY_, 0, _ATC_REQUEST_PAUSE_);
        change_osd = true;
        osd_item = m_pOsd_thread->osd_list.begin();
        while(osd_item != m_pOsd_thread->osd_list.end())
        {
            if((osd_item->video_stream.type == video_stream_type) && (osd_item->video_stream.phy_video_chn_num == phy_video_chn_num) && \
                                    (osd_item->osd_name == _AON_DATE_TIME_))
            {
                Ae_destroy_osd_region(osd_item);
                osd_item = m_pOsd_thread->osd_list.erase(osd_item);
               break;
            }
            osd_item ++;
        }

        if(pVideo_para->have_date_time_osd != 0)
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_DATE_TIME_;
            av_osd_item.overlay_state = _AOS_INIT_;
#if defined(_AV_PRODUCT_CLASS_IPC_)
            av_osd_item.is_update = true;
#endif
            memcpy(&av_osd_item.video_stream, &(*video_it), sizeof(av_osd_item.video_stream));
            memcpy(&av_osd_item.video_stream.video_encoder_para, pVideo_para, sizeof(av_video_encoder_para_t));
            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
        m_pOsd_thread->thread_lock->unlock();
    }            

#if defined(_AV_PRODUCT_CLASS_IPC_)
     if((video_it->video_encoder_para.have_bus_number_osd != pVideo_para->have_bus_number_osd) || \
                (video_it->video_encoder_para.bus_number_osd_x != pVideo_para->bus_number_osd_x) || \
                        (video_it->video_encoder_para.bus_number_osd_y != pVideo_para->bus_number_osd_y))   
#else

#if defined (_AV_PLATFORM_HI3515_)
    if( (video_it->type == _AST_MAIN_VIDEO_)&&(video_it->video_encoder_para.have_bus_number_osd != pVideo_para->have_bus_number_osd) || \
                (video_it->video_encoder_para.bus_number_osd_x != pVideo_para->bus_number_osd_x) || \
                        (video_it->video_encoder_para.bus_number_osd_y != pVideo_para->bus_number_osd_y) || \
                            (memcmp(video_it->video_encoder_para.bus_number, pVideo_para->bus_number, MAX_OSD_NAME_SIZE) != 0))
#else
    if((video_it->video_encoder_para.have_bus_number_osd != pVideo_para->have_bus_number_osd) || \
                (video_it->video_encoder_para.bus_number_osd_x != pVideo_para->bus_number_osd_x) || \
                        (video_it->video_encoder_para.bus_number_osd_y != pVideo_para->bus_number_osd_y) || \
                            (memcmp(video_it->video_encoder_para.bus_number, pVideo_para->bus_number, MAX_OSD_NAME_SIZE) != 0))
#endif
#endif
    {
        m_pOsd_thread->thread_lock->lock();
        /*bus number*/
        Ae_thread_control(_ETT_OSD_OVERLAY_, 0, _ATC_REQUEST_PAUSE_);
        change_osd = true;
        if(video_it->video_encoder_para.have_bus_number_osd != 0)
        {
            osd_item = m_pOsd_thread->osd_list.begin();
            while(osd_item != m_pOsd_thread->osd_list.end())
            {
                if((osd_item->video_stream.type == video_stream_type) && (osd_item->video_stream.phy_video_chn_num == phy_video_chn_num) && \
                                        (osd_item->osd_name == _AON_BUS_NUMBER_))
                {
                    Ae_destroy_osd_region(osd_item);
                    osd_item = m_pOsd_thread->osd_list.erase(osd_item);
                    break;
                }
                osd_item ++;
            }
        }

        if(pVideo_para->have_bus_number_osd != 0)
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_BUS_NUMBER_;
            av_osd_item.overlay_state = _AOS_INIT_;
#if defined(_AV_PRODUCT_CLASS_IPC_)
            av_osd_item.is_update = false;
#endif
            memcpy(&av_osd_item.video_stream, &(*video_it), sizeof(av_osd_item.video_stream));
            memcpy(&av_osd_item.video_stream.video_encoder_para, pVideo_para, sizeof(av_video_encoder_para_t));
            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
        m_pOsd_thread->thread_lock->unlock();
    }    

#if defined(_AV_PRODUCT_CLASS_IPC_)
if((video_it->video_encoder_para.have_bus_selfnumber_osd != pVideo_para->have_bus_selfnumber_osd) || \
            (video_it->video_encoder_para.bus_selfnumber_osd_x != pVideo_para->bus_selfnumber_osd_x) || \
                    (video_it->video_encoder_para.bus_selfnumber_osd_y != pVideo_para->bus_selfnumber_osd_y))
#else

#if defined (_AV_PLATFORM_HI3515_)
    if( (video_it->type == _AST_MAIN_VIDEO_)&&(video_it->video_encoder_para.have_bus_selfnumber_osd != pVideo_para->have_bus_selfnumber_osd) || \
                (video_it->video_encoder_para.bus_selfnumber_osd_x != pVideo_para->bus_selfnumber_osd_x) || \
                        (video_it->video_encoder_para.bus_selfnumber_osd_y != pVideo_para->bus_selfnumber_osd_y) || \
                            (memcmp(video_it->video_encoder_para.bus_selfnumber, pVideo_para->bus_selfnumber, MAX_OSD_NAME_SIZE) != 0))

#else
    if((video_it->video_encoder_para.have_bus_selfnumber_osd != pVideo_para->have_bus_selfnumber_osd) || \
                (video_it->video_encoder_para.bus_selfnumber_osd_x != pVideo_para->bus_selfnumber_osd_x) || \
                        (video_it->video_encoder_para.bus_selfnumber_osd_y != pVideo_para->bus_selfnumber_osd_y) || \
                            (memcmp(video_it->video_encoder_para.bus_selfnumber, pVideo_para->bus_selfnumber, MAX_OSD_NAME_SIZE) != 0))
#endif

#endif
    {
        m_pOsd_thread->thread_lock->lock();
        /*bus number*/
        Ae_thread_control(_ETT_OSD_OVERLAY_, 0, _ATC_REQUEST_PAUSE_);
        change_osd = true;
        if(video_it->video_encoder_para.have_bus_selfnumber_osd != 0)
        {
            osd_item = m_pOsd_thread->osd_list.begin();
            while(osd_item != m_pOsd_thread->osd_list.end())
            {
                if((osd_item->video_stream.type == video_stream_type) && (osd_item->video_stream.phy_video_chn_num == phy_video_chn_num) && \
                                        (osd_item->osd_name == _AON_SELFNUMBER_INFO_))
                {
                    Ae_destroy_osd_region(osd_item);
                    osd_item = m_pOsd_thread->osd_list.erase(osd_item);
                    break;
                }
                osd_item ++;
            }
        }
        if(0 != pVideo_para->have_bus_selfnumber_osd )
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_SELFNUMBER_INFO_;
            av_osd_item.overlay_state = _AOS_INIT_;
#if defined(_AV_PRODUCT_CLASS_IPC_)
            av_osd_item.is_update = false;
#endif            
            memcpy(&av_osd_item.video_stream, &(*video_it), sizeof(av_osd_item.video_stream));
            memcpy(&av_osd_item.video_stream.video_encoder_para, pVideo_para, sizeof(av_video_encoder_para_t));
            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
        m_pOsd_thread->thread_lock->unlock();
    }    

#ifdef _AV_PRODUCT_CLASS_IPC_
	    if((video_it->video_encoder_para.have_gps_osd != pVideo_para->have_gps_osd) || \
                (video_it->video_encoder_para.gps_osd_x != pVideo_para->gps_osd_x) || \
                        (video_it->video_encoder_para.gps_osd_y != pVideo_para->gps_osd_y))
#else

#if defined (_AV_PLATFORM_HI3515_)
    if( (video_it->type == _AST_MAIN_VIDEO_)&&((video_it->video_encoder_para.have_gps_osd != pVideo_para->have_gps_osd) || \
                (video_it->video_encoder_para.gps_osd_x != pVideo_para->gps_osd_x) || \
                        (video_it->video_encoder_para.gps_osd_y != pVideo_para->gps_osd_y)))

#else
    if(/*(video_it->type != _AST_SUB2_VIDEO_) &&*/ ((video_it->video_encoder_para.have_gps_osd != pVideo_para->have_gps_osd) || \
                (video_it->video_encoder_para.gps_osd_x != pVideo_para->gps_osd_x) || \
                        (video_it->video_encoder_para.gps_osd_y != pVideo_para->gps_osd_y)))
#endif

#endif
    {
        m_pOsd_thread->thread_lock->lock();
        /*gps info*/
        Ae_thread_control(_ETT_OSD_OVERLAY_, 0, _ATC_REQUEST_PAUSE_);
        change_osd = true;
        if(video_it->video_encoder_para.have_gps_osd != 0)
        {
            osd_item = m_pOsd_thread->osd_list.begin();
            while(osd_item != m_pOsd_thread->osd_list.end())
            {
                if((osd_item->video_stream.type == video_stream_type) && (osd_item->video_stream.phy_video_chn_num == phy_video_chn_num) && \
                                        (osd_item->osd_name == _AON_GPS_INFO_))
                {
                    Ae_destroy_osd_region(osd_item);
                    osd_item = m_pOsd_thread->osd_list.erase(osd_item);
                    break;
                }
                osd_item ++;
            }
        }

        if(pVideo_para->have_gps_osd != 0)
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_GPS_INFO_;
            av_osd_item.overlay_state = _AOS_INIT_;
#if defined(_AV_PRODUCT_CLASS_IPC_)
            char customer[32];
            memset(customer, 0, sizeof(customer));
            pgAvDeviceInfo->Adi_get_customer_name(customer, sizeof(customer));

            if(0 == strcmp(customer, "CUSTOMER_CNR"))
            {
                av_osd_item.is_update = true;
            }
            else
            {
                av_osd_item.is_update = false;
            }
#endif            
            memcpy(&av_osd_item.video_stream, &(*video_it), sizeof(av_osd_item.video_stream));
            memcpy(&av_osd_item.video_stream.video_encoder_para, pVideo_para, sizeof(av_video_encoder_para_t));
            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
        m_pOsd_thread->thread_lock->unlock();
    }    

#ifdef _AV_PRODUCT_CLASS_IPC_
     if((video_it->video_encoder_para.have_speed_osd != pVideo_para->have_speed_osd) || \
                (video_it->video_encoder_para.speed_osd_x != pVideo_para->speed_osd_x) || \
                        (video_it->video_encoder_para.speed_osd_y != pVideo_para->speed_osd_y))
#else
#if defined (_AV_PLATFORM_HI3515_)
     if( (video_it->type == _AST_MAIN_VIDEO_)&&((video_it->video_encoder_para.have_speed_osd != pVideo_para->have_speed_osd) || \
                (video_it->video_encoder_para.speed_osd_x != pVideo_para->speed_osd_x) || \
                        (video_it->video_encoder_para.speed_osd_y != pVideo_para->speed_osd_y)))

#else

     if(/*(video_it->type != _AST_SUB2_VIDEO_) &&*/ ((video_it->video_encoder_para.have_speed_osd != pVideo_para->have_speed_osd) || \
                (video_it->video_encoder_para.speed_osd_x != pVideo_para->speed_osd_x) || \
         (video_it->video_encoder_para.speed_osd_y != pVideo_para->speed_osd_y)||\
         (0 == strcmp(customer, "CUSTOMER_BJGJ"))))
#endif

#endif
    {
        m_pOsd_thread->thread_lock->lock();
        /*speed*/
        Ae_thread_control(_ETT_OSD_OVERLAY_, 0, _ATC_REQUEST_PAUSE_);
        change_osd = true;
        if(video_it->video_encoder_para.have_speed_osd != 0)
        {
            osd_item = m_pOsd_thread->osd_list.begin();
            while(osd_item != m_pOsd_thread->osd_list.end())
            {
                if((osd_item->video_stream.type == video_stream_type) && (osd_item->video_stream.phy_video_chn_num == phy_video_chn_num) && \
                                        (osd_item->osd_name == _AON_SPEED_INFO_))
                {
                    Ae_destroy_osd_region(osd_item);
                    osd_item = m_pOsd_thread->osd_list.erase(osd_item);
                    break;
                }
                osd_item ++;
            }
        }

        if(pVideo_para->have_speed_osd != 0)
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_SPEED_INFO_;
            av_osd_item.overlay_state = _AOS_INIT_;
#if defined(_AV_PRODUCT_CLASS_IPC_)
            av_osd_item.is_update = false;
#endif
            memcpy(&av_osd_item.video_stream, &(*video_it), sizeof(av_osd_item.video_stream));
            memcpy(&av_osd_item.video_stream.video_encoder_para, pVideo_para, sizeof(av_video_encoder_para_t));
            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
        m_pOsd_thread->thread_lock->unlock();
    }    

//! watermark 
#ifdef _AV_PRODUCT_CLASS_IPC_
         if((video_it->video_encoder_para.have_water_mark_osd != pVideo_para->have_water_mark_osd) || \
                    (video_it->video_encoder_para.water_mark_osd_x != pVideo_para->water_mark_osd_x) || \
                            (video_it->video_encoder_para.water_mark_osd_y != pVideo_para->water_mark_osd_y))
#else
         if(/*(video_it->type != _AST_SUB2_VIDEO_) &&*/ ((video_it->video_encoder_para.have_water_mark_osd != pVideo_para->have_water_mark_osd) || \
                    (video_it->video_encoder_para.water_mark_osd_x != pVideo_para->water_mark_osd_x) || \
                            (video_it->video_encoder_para.water_mark_osd_y != pVideo_para->water_mark_osd_y))||\
                            (memcmp(video_it->video_encoder_para.water_mark_name, pVideo_para->water_mark_name, MAX_OSD_NAME_SIZE) != 0))
#endif
        {
            m_pOsd_thread->thread_lock->lock();
            /*speed*/
            Ae_thread_control(_ETT_OSD_OVERLAY_, 0, _ATC_REQUEST_PAUSE_);
            change_osd = true;
            if(video_it->video_encoder_para.have_water_mark_osd != 0)
            {
                osd_item = m_pOsd_thread->osd_list.begin();
                while(osd_item != m_pOsd_thread->osd_list.end())
                {
                    if((osd_item->video_stream.type == video_stream_type) && (osd_item->video_stream.phy_video_chn_num == phy_video_chn_num) && \
                                            (osd_item->osd_name == _AON_WATER_MARK))
                    {
                        Ae_destroy_osd_region(osd_item);
                        osd_item = m_pOsd_thread->osd_list.erase(osd_item);
                        break;
                    }
                    osd_item ++;
                }
            }
    
            if(pVideo_para->have_water_mark_osd != 0)
            {
                memset(&av_osd_item, 0, sizeof(av_osd_item_t));
                av_osd_item.osd_name = _AON_WATER_MARK;
                av_osd_item.overlay_state = _AOS_INIT_;
#if defined(_AV_PRODUCT_CLASS_IPC_)
                av_osd_item.is_update = false;
#endif
                memcpy(&av_osd_item.video_stream, &(*video_it), sizeof(av_osd_item.video_stream));
                memcpy(&av_osd_item.video_stream.video_encoder_para, pVideo_para, sizeof(av_video_encoder_para_t));
                m_pOsd_thread->osd_list.push_back(av_osd_item);
            }
            m_pOsd_thread->thread_lock->unlock();
        } 


    /*通用码流叠加1*/
#ifdef _AV_PRODUCT_CLASS_IPC_
     if((video_it->video_encoder_para.have_common1_osd != pVideo_para->have_common1_osd) || \
                (video_it->video_encoder_para.common1_osd_x != pVideo_para->common1_osd_x) || \
                        (video_it->video_encoder_para.common1_osd_y != pVideo_para->common1_osd_y))
#else
#if defined (_AV_PLATFORM_HI3515_)
      if( (video_it->type == _AST_MAIN_VIDEO_)&&((video_it->video_encoder_para.have_common1_osd != pVideo_para->have_common1_osd) || \
                (video_it->video_encoder_para.common1_osd_x != pVideo_para->common1_osd_x) || \
                        (video_it->video_encoder_para.common1_osd_y != pVideo_para->common1_osd_y) /*|| \
                        (memcmp(video_it->video_encoder_para.osd1_content, pVideo_para->osd1_content, MAX_EXT_OSD_NAME_SIZE) != 0)*/))

#else

     if(/*(video_it->type != _AST_SUB2_VIDEO_) && */((video_it->video_encoder_para.have_common1_osd != pVideo_para->have_common1_osd) || \
                (video_it->video_encoder_para.common1_osd_x != pVideo_para->common1_osd_x) || \
                        (video_it->video_encoder_para.common1_osd_y != pVideo_para->common1_osd_y) /*|| \
                        (memcmp(video_it->video_encoder_para.osd1_content, pVideo_para->osd1_content, MAX_EXT_OSD_NAME_SIZE) != 0)*/))
#endif

#endif
    {
        m_pOsd_thread->thread_lock->lock();
        Ae_thread_control(_ETT_OSD_OVERLAY_, 0, _ATC_REQUEST_PAUSE_);
        change_osd = true;
        if(video_it->video_encoder_para.have_common1_osd != 0)
        {
            osd_item = m_pOsd_thread->osd_list.begin();
            while(osd_item != m_pOsd_thread->osd_list.end())
            {
                if((osd_item->video_stream.type == video_stream_type) && (osd_item->video_stream.phy_video_chn_num == phy_video_chn_num) && \
                                        (osd_item->osd_name == _AON_COMMON_OSD1_))
                {
                    Ae_destroy_osd_region(osd_item);
                    osd_item = m_pOsd_thread->osd_list.erase(osd_item);
                    break;
                }
                osd_item ++;
            }
        }

        if(pVideo_para->have_common1_osd != 0)
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_COMMON_OSD1_;
            av_osd_item.overlay_state = _AOS_INIT_;
#if defined(_AV_PRODUCT_CLASS_IPC_)
            av_osd_item.is_update = false;
#endif
            memcpy(&av_osd_item.video_stream, &(*video_it), sizeof(av_osd_item.video_stream));
            memcpy(&av_osd_item.video_stream.video_encoder_para, pVideo_para, sizeof(av_video_encoder_para_t));
            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
        m_pOsd_thread->thread_lock->unlock();
    }
     
    /*通用码流叠加2*/
#ifdef _AV_PRODUCT_CLASS_IPC_
    if((video_it->video_encoder_para.have_common2_osd != pVideo_para->have_common2_osd) || \
                (video_it->video_encoder_para.common2_osd_x != pVideo_para->common2_osd_x) || \
                        (video_it->video_encoder_para.common2_osd_y != pVideo_para->common2_osd_y) )
#else
#if defined (_AV_PLATFORM_HI3515_)
    if( (video_it->type == _AST_MAIN_VIDEO_)&&((video_it->video_encoder_para.have_common2_osd != pVideo_para->have_common2_osd) || \
                (video_it->video_encoder_para.common2_osd_x != pVideo_para->common2_osd_x) || \
                        (video_it->video_encoder_para.common2_osd_y != pVideo_para->common2_osd_y) /*|| \
                        (memcmp(video_it->video_encoder_para.osd2_content, pVideo_para->osd2_content, MAX_EXT_OSD_NAME_SIZE) != 0)*/))
#else

    if(/*(video_it->type != _AST_SUB2_VIDEO_) && */((video_it->video_encoder_para.have_common2_osd != pVideo_para->have_common2_osd) || \
                (video_it->video_encoder_para.common2_osd_x != pVideo_para->common2_osd_x) || \
                        (video_it->video_encoder_para.common2_osd_y != pVideo_para->common2_osd_y) /*|| \
                        (memcmp(video_it->video_encoder_para.osd2_content, pVideo_para->osd2_content, MAX_EXT_OSD_NAME_SIZE) != 0)*/))
#endif

#endif
    {
        m_pOsd_thread->thread_lock->lock();
        Ae_thread_control(_ETT_OSD_OVERLAY_, 0, _ATC_REQUEST_PAUSE_);
        change_osd = true;
        if(video_it->video_encoder_para.have_common2_osd != 0)
        {
            osd_item = m_pOsd_thread->osd_list.begin();
            while(osd_item != m_pOsd_thread->osd_list.end())
            {
                if((osd_item->video_stream.type == video_stream_type) && (osd_item->video_stream.phy_video_chn_num == phy_video_chn_num) && \
                                        (osd_item->osd_name == _AON_COMMON_OSD2_))
                {
                    Ae_destroy_osd_region(osd_item);
                    osd_item = m_pOsd_thread->osd_list.erase(osd_item);
                    break;
                }
                osd_item ++;
            }
        }

        if(pVideo_para->have_common2_osd != 0)
        {
            memset(&av_osd_item, 0, sizeof(av_osd_item_t));
            av_osd_item.osd_name = _AON_COMMON_OSD2_;
            av_osd_item.overlay_state = _AOS_INIT_;
#if defined(_AV_PRODUCT_CLASS_IPC_)
            av_osd_item.is_update = false;
#endif
            memcpy(&av_osd_item.video_stream, &(*video_it), sizeof(av_osd_item.video_stream));
            memcpy(&av_osd_item.video_stream.video_encoder_para, pVideo_para, sizeof(av_video_encoder_para_t));
            m_pOsd_thread->osd_list.push_back(av_osd_item);
        }
        m_pOsd_thread->thread_lock->unlock();
    }


     
    if((change_osd == true) && (m_pOsd_thread->osd_list.size() != 0))
    {
        Ae_thread_control(_ETT_OSD_OVERLAY_, 0, _ATC_REQUEST_RUNNING_);
    }

    memcpy(&video_it->video_encoder_para, pVideo_para, sizeof(av_video_encoder_para_t));

    ret_val = 0;

    return ret_val;
}

int CAvEncoder::Ae_dynamic_modify_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, bool bosd_need_change)
{
    std::list<av_audio_video_group_t>::iterator group_it;
    std::list<av_video_stream_t>::iterator video_it;
    std::list<av_osd_item_t>::iterator osd_item;
    int recv_task_id = -1;
    int ret_val = -1;
    
#ifdef _AV_PRODUCT_CLASS_IPC_
    if( pVideo_para )
    {
        char osd_string[256];

        memset(osd_string, 0, sizeof(osd_string));
        snprintf(osd_string, sizeof(osd_string), "%s%s%s%s%s%s", pVideo_para->bus_number, pVideo_para->channel_name, \
            pVideo_para->gps, pVideo_para->bus_selfnumber, pVideo_para->osd1_content, pVideo_para->osd2_content);
        osd_string[sizeof(osd_string)-1] = '\0';
        this->CacheOsdFond(osd_string);
    }
    else
    {
        this->CacheOsdFond( "" );
    }
#else
    
    if( pVideo_para )
    {
#if (defined(_AV_PLATFORM_HI3520D_V300_) || defined(_AV_PLATFORM_HI3515_))
	    if(OSD_TEST_FLAG == pgAvDeviceInfo->Adi_product_type() || PTD_6A_I == pgAvDeviceInfo->Adi_product_type())	/*6AII_AV12机型osd叠加特殊处理*/
	    {
	    	for(int index = 0 ; index < OVERLAY_MAX_NUM_VENC ; ++index)
	    	{
				if(pVideo_para->is_osd_change[index] == 1 && pVideo_para->stuExtendOsdInfo[index].ucOsdChn == phy_video_chn_num && pVideo_para->stuExtendOsdInfo[index].ucEnable == 1)
				{
					DEBUG_WARNING("EXTEND_OSD: Cache osd, index = %d, chn = %d, enable = %d, content = %s\n", index, phy_video_chn_num, pVideo_para->stuExtendOsdInfo[index].ucEnable, pVideo_para->stuExtendOsdInfo[index].szContent);
					if(pVideo_para->stuExtendOsdInfo[index].ucOsdType == 0)	/*日期特殊处理*/
					{
						this->CacheOsdFond(":1234567890. ");
					}
					else
					{
						this->CacheOsdFond(pVideo_para->stuExtendOsdInfo[index].szContent);
					}
				}
	    	}
	    }
	    else
#endif	    
		{
			if(pVideo_para->have_date_time_osd == 1)
			{
				this->CacheOsdFond( ":1234567890/-APM ");
			}
			if(pVideo_para->have_gps_osd == 1)
			{
				this->CacheOsdFond("0123456789: ,.ENSW none invaild");
				this->CacheOsdFond("LAT LON");
				this->CacheOsdFond(GuiTKGetText(NULL, NULL, "STR_GPS"));
				this->CacheOsdFond(GuiTKGetText(NULL, NULL, "STR_GPS INVALID"));
				this->CacheOsdFond(GuiTKGetText(NULL, NULL, "STR_GPS NOT_EXIST"));
			}
			if(pVideo_para->have_bus_number_osd == 1)
			{
				this->CacheOsdFond( pVideo_para->bus_number);
			}
			if(pVideo_para->have_speed_osd == 1)
			{
				this->CacheOsdFond( "0123456789 KM/HMPH");
			}
			if(pVideo_para->have_bus_selfnumber_osd == 1)
			{
				this->CacheOsdFond( pVideo_para->bus_selfnumber);
			}
			if(pVideo_para->have_channel_name_osd == 1)
			{
				this->CacheOsdFond(pVideo_para->channel_name);
			}
			if(pVideo_para->have_common1_osd == 1 && pVideo_para->osd1_contentfix == 1)
			{
				this->CacheOsdFond( pVideo_para->osd1_content );
			}
			
			if(pVideo_para->have_common2_osd == 1 && pVideo_para->osd2_contentfix == 1)
			{
				this->CacheOsdFond( pVideo_para->osd2_content );
			}
			if(pVideo_para->have_water_mark_osd == 1) //! 0419
			{
				this->CacheOsdFond( pVideo_para->water_mark_name);
			}
	    }
    }
    else
    {
        this->CacheOsdFond( "" );
    }
#endif

    m_pThread_lock->lock();

    if(Ae_get_task_id(video_stream_type, phy_video_chn_num, &recv_task_id) != 0)
    {
        DEBUG_ERROR( "CAvEncoder::Ae_dynamic_modify_encoder Ae_get_task_id FAILED(type:%d, chn:%d)\n", video_stream_type, phy_video_chn_num);
        goto _OVER_;
    }
#if (defined(_AV_PLATFORM_HI3520D_V300_) || defined(_AV_PLATFORM_HI3515_))
	if(OSD_TEST_FLAG == pgAvDeviceInfo->Adi_product_type() || PTD_6A_I == pgAvDeviceInfo->Adi_product_type()) /*6AII_AV12机型osd叠加特殊处理*/
	{
    	ret_val = Ae_dynamic_modify_extend_osd_encoder(video_stream_type, phy_video_chn_num, pVideo_para, bosd_need_change);
    }
    else
#endif
    {
    	ret_val = Ae_dynamic_modify_local_encoder(video_stream_type, phy_video_chn_num, pVideo_para, bosd_need_change);
    }
_OVER_:

    m_pThread_lock->unlock();

    return ret_val;
}

#if (defined(_AV_PLATFORM_HI3520D_V300_) || defined(_AV_PLATFORM_HI3515_))
int CAvEncoder::Ae_dynamic_modify_extend_osd_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, bool bosd_need_change)
{
	int recv_task_id = -1;
	if(Ae_get_task_id(video_stream_type, phy_video_chn_num, &recv_task_id) != 0)
	{
		DEBUG_ERROR("EXTEND_OSD: Get video task failed, stream_type = %d, chn = %d\n", video_stream_type, phy_video_chn_num);
		return -1;
	}

	std::list<av_audio_video_group_t>::iterator group_it = m_pRecv_thread_task[recv_task_id].local_task_list.begin();
	std::list<av_video_stream_t>::iterator video_it;
	std::list<av_osd_item_t>::iterator osd_item;
	bool change_osd = false;
	bool only_name_diff = false;
	bool have_create = false;
    int ret_val = -1;
	while(group_it != m_pRecv_thread_task[recv_task_id].local_task_list.end())	//搜索当前码流通道所在的实例
	{
		video_it = group_it->video_stream.begin();
		while(video_it != group_it->video_stream.end())
		{
			if(video_it->type == video_stream_type && video_it->phy_video_chn_num == phy_video_chn_num)
			{
				break;
			}
			video_it++;
		}
		if(video_it != group_it->video_stream.end())
		{
			break;
		}
		group_it++;
	}

	if(group_it == m_pRecv_thread_task[recv_task_id].local_task_list.end())
	{
		DEBUG_ERROR("EXTEND_OSD: Find video task failed, video_type = %d, chn = %d\n", video_stream_type, phy_video_chn_num);
		return  -1;
	}

	if(bosd_need_change)
	{
		goto _MODIFY_OSD_;
	}
	
	if(pVideo_para->gop_size != video_it->video_encoder_para.gop_size || pVideo_para->frame_rate != video_it->video_encoder_para.frame_rate || \
		pVideo_para->bitrate_mode != video_it->video_encoder_para.bitrate_mode)
	{
		goto _DYNAMIC_ADJUST;
	}
	switch(pVideo_para->bitrate_mode)
	{
		case _ABM_CBR_:
			if(pVideo_para->bitrate.hicbr.bitrate != video_it->video_encoder_para.bitrate.hicbr.bitrate)
			{
				goto _DYNAMIC_ADJUST;
			}
			break;
		case _ABM_VBR_:
			if(pVideo_para->bitrate.hivbr.bitrate != video_it->video_encoder_para.bitrate.hivbr.bitrate)
			{
				goto _DYNAMIC_ADJUST;
			}
			break;
		default:
			DEBUG_ERROR("EXTEND_OSD: birate mode error(%d)\n", pVideo_para->bitrate_mode);
			break;
	}
	goto _MODIFY_OSD_;

_DYNAMIC_ADJUST:
	if((ret_val = Ae_dynamic_modify_encoder_local(video_stream_type, phy_video_chn_num, pVideo_para)))
	{
		DEBUG_ERROR("EXTEND_OSD: Ae_dynamic_modify_encoder_local failed\n");
		return ret_val;
	}

_MODIFY_OSD_:
	for(int index = 0 ; index < OVERLAY_MAX_NUM_VENC ; index ++)
	{
#if defined(_AV_PLATFORM_HI3515_)
	if((video_stream_type == _AST_MAIN_VIDEO_) && ((video_it->video_encoder_para.stuExtendOsdInfo[index].ucEnable != pVideo_para->stuExtendOsdInfo[index].ucEnable) || \
		(video_it->video_encoder_para.stuExtendOsdInfo[index].usOsdX != pVideo_para->stuExtendOsdInfo[index].usOsdX) || \
		(video_it->video_encoder_para.stuExtendOsdInfo[index].usOsdY != pVideo_para->stuExtendOsdInfo[index].usOsdY) || \
		(video_it->video_encoder_para.stuExtendOsdInfo[index].ucColor != pVideo_para->stuExtendOsdInfo[index].ucColor) || \
		memcmp(video_it->video_encoder_para.stuExtendOsdInfo[index].szContent, pVideo_para->stuExtendOsdInfo[index].szContent, MAX_EXTEND_OSD_LENGTH) != 0))
#else
	if((video_it->video_encoder_para.stuExtendOsdInfo[index].ucEnable != pVideo_para->stuExtendOsdInfo[index].ucEnable) || \
		(video_it->video_encoder_para.stuExtendOsdInfo[index].usOsdX != pVideo_para->stuExtendOsdInfo[index].usOsdX) || \
		(video_it->video_encoder_para.stuExtendOsdInfo[index].usOsdY != pVideo_para->stuExtendOsdInfo[index].usOsdY) || \
		(video_it->video_encoder_para.stuExtendOsdInfo[index].ucColor != pVideo_para->stuExtendOsdInfo[index].ucColor) || \
		memcmp(video_it->video_encoder_para.stuExtendOsdInfo[index].szContent, pVideo_para->stuExtendOsdInfo[index].szContent, MAX_EXTEND_OSD_LENGTH) != 0)
#endif
		{
		only_name_diff = false;
		if((video_stream_type == _AST_MAIN_VIDEO_) && ((video_it->video_encoder_para.stuExtendOsdInfo[index].ucEnable == pVideo_para->stuExtendOsdInfo[index].ucEnable) && \
			(video_it->video_encoder_para.stuExtendOsdInfo[index].usOsdX == pVideo_para->stuExtendOsdInfo[index].usOsdX) && \
			(video_it->video_encoder_para.stuExtendOsdInfo[index].usOsdY == pVideo_para->stuExtendOsdInfo[index].usOsdY) && \
			(video_it->video_encoder_para.stuExtendOsdInfo[index].ucColor == pVideo_para->stuExtendOsdInfo[index].ucColor)))
		{
			only_name_diff = true;
		}

			m_pOsd_thread->thread_lock->lock();
			Ae_thread_control(_ETT_OSD_OVERLAY_, recv_task_id, _ATC_REQUEST_PAUSE_);

			change_osd = true;
			osd_item = m_pOsd_thread->osd_list.begin();
			if(video_it->video_encoder_para.stuExtendOsdInfo[index].ucEnable == 1)
			{
				while(osd_item != m_pOsd_thread->osd_list.end())
				{	//现在osd列表中查找，找到之后先删除，再重新创建///
					if(osd_item->extend_osd_id == index && osd_item->video_stream.type == video_stream_type && osd_item->video_stream.phy_video_chn_num == phy_video_chn_num)
					{
						if(only_name_diff && pgAvDeviceInfo->Adi_product_type() == PTD_6A_I)
						{
							have_create = true;	//已经创建了，此时只用更换内容///
							if(pVideo_para->stuExtendOsdInfo[index].ucEnable == 1)
							{
								break;
							}
						}
						have_create = false;
						DEBUG_MESSAGE("EXTEND_OSD: Delete osd item, id = %d, type = %d, chn = %d, listSize = %d\n", index, video_stream_type, phy_video_chn_num, m_pOsd_thread->osd_list.size());
						Ae_destroy_osd_region(osd_item);
						osd_item = m_pOsd_thread->osd_list.erase(osd_item);
						break;
					}
					osd_item++;
				}
			}

			if(pVideo_para->stuExtendOsdInfo[index].ucEnable == 1)
			{
				av_osd_item_t av_osd_item;
				memset(&av_osd_item, 0x0, sizeof(av_osd_item_t));
				av_osd_item.osd_name = _AON_EXTEND_OSD;
				av_osd_item.extend_osd_id = index;
				av_osd_item.overlay_state = _AOS_INIT_;
				av_osd_item.osd_region_have_created = 0;
				av_osd_item.osdChange = 1;	//changed///
				memcpy(&(av_osd_item.video_stream), &(*video_it), sizeof(av_video_stream_t));
				memcpy(&(av_osd_item.video_stream.video_encoder_para), pVideo_para, sizeof(av_video_encoder_para_t));

				if(osd_item != m_pOsd_thread->osd_list.end() && have_create)
				{
					osd_item->osd_region_have_created = 1;
					osd_item->osd_name = _AON_EXTEND_OSD;
					osd_item->extend_osd_id = index;
					osd_item->overlay_state = _AOS_CREATED_;
					osd_item->osdChange = 1;
					memcpy(&(osd_item->video_stream), &(*video_it), sizeof(av_video_stream_t));
					memcpy(&(osd_item->video_stream.video_encoder_para), pVideo_para, sizeof(av_video_encoder_para_t));
					DEBUG_MESSAGE("EXTEND_OSD: Change osd item, id = %d, type = %d, chn = %d, align = %d, listSize = %d\n", index, video_stream_type, phy_video_chn_num, osd_item->video_stream.video_encoder_para.stuExtendOsdInfo[index].ucAlign, m_pOsd_thread->osd_list.size());
				}
				else
				{
					m_pOsd_thread->osd_list.push_back(av_osd_item);
					DEBUG_MESSAGE("EXTEND_OSD: Add osd item, id = %d, type = %d, chn = %d, align = %d, listSize = %d\n", index, video_stream_type, phy_video_chn_num, av_osd_item.video_stream.video_encoder_para.stuExtendOsdInfo[index].ucAlign, m_pOsd_thread->osd_list.size());
				}
			}

			m_pOsd_thread->thread_lock->unlock();
		}
	}

	if(change_osd && m_pOsd_thread->osd_list.size() > 0)
	{
		Ae_thread_control(_ETT_OSD_OVERLAY_, recv_task_id, _ATC_REQUEST_RUNNING_);
	}

    memcpy(&video_it->video_encoder_para, pVideo_para, sizeof(av_video_encoder_para_t));
	
	return 0;
}
#endif

int CAvEncoder::Ae_stop_encoder_local(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, int normal_audio)
{
	std::list<av_audio_video_group_t>::iterator av_group_it;
	std::list<av_video_stream_t>::iterator video_stream_it;
	std::list<av_video_stream_t>::iterator audio_stream_it;
	std::list<av_osd_item_t>::iterator osd_item;
	av_audio_video_group_t av_audio_video_group;
	int recv_task_id = -1;
	int send_task_id = -1;
	int ret_val = -1;

	if((!((video_stream_type == _AST_UNKNOWN_) || (phy_video_chn_num < 0))) || normal_audio == 1)
	{
		/*destory osd*/
		if ( normal_audio == -1) // destroy video
		{
			m_pOsd_thread->thread_lock->lock();
			osd_item = m_pOsd_thread->osd_list.begin();
			while(osd_item != m_pOsd_thread->osd_list.end())
			{
				if((osd_item->video_stream.type == video_stream_type) && (osd_item->video_stream.phy_video_chn_num == phy_video_chn_num))
				{
					Ae_destroy_osd_region(osd_item);
					osd_item = m_pOsd_thread->osd_list.erase(osd_item);
					continue;
				}
				osd_item ++;
			}
			m_pOsd_thread->thread_lock->unlock();
		}

		if(Ae_get_task_id(video_stream_type, phy_video_chn_num, &recv_task_id, &send_task_id) != 0)
		{
			DEBUG_ERROR( "CAvEncoder::Ae_stop_encoder_local Ae_get_task_id FAILED(type:%d)(chn:%d)\n", video_stream_type, phy_video_chn_num);
			goto _OVER_;
		}

		av_group_it = m_pRecv_thread_task[recv_task_id].local_task_list.begin();
		while(av_group_it != m_pRecv_thread_task[recv_task_id].local_task_list.end())
		{
			video_stream_it = av_group_it->video_stream.begin();
			while(video_stream_it != av_group_it->video_stream.end())
			{
				if((video_stream_it->phy_video_chn_num == phy_video_chn_num) && (video_stream_it->type == video_stream_type))
				{
					goto _FIND_OVER_;
				}
				video_stream_it ++;
			}
			av_group_it ++;
		}

_FIND_OVER_:
		if(av_group_it != m_pRecv_thread_task[recv_task_id].local_task_list.end())
		{
            //printf("----av_group_it------\n");
			if ( normal_audio == -1) // destroy video
			{
				/*destory hardware encoder*/
				if(Ae_destory_video_encoder(video_stream_type, phy_video_chn_num, &video_stream_it->video_encoder_id) != 0)
				{
					DEBUG_ERROR( "CAvEncoder::Ae_stop_encoder_local Ae_destory_video_encoder(type:%d)(channel:%d)\n", video_stream_type, phy_video_chn_num);
					goto _OVER_;
				}

				/*destory stream buffer*/
				if(Ae_destory_stream_buffer(video_stream_it->stream_buffer_handle, video_stream_type, phy_video_chn_num) != 0)
				{
					DEBUG_ERROR( "CAvEncoder::Ae_stop_encoder_local Ae_destory_stream_buffer(type:%d)(channel:%d)\n", video_stream_type, phy_video_chn_num);
					goto _OVER_;
				}

				if(video_stream_it->type == _AST_MAIN_VIDEO_)
				{
					if(Ae_destory_stream_buffer(video_stream_it->iframe_stream_buffer_handle, _AST_MAIN_IFRAME_VIDEO_, phy_video_chn_num) != 0)
					{
						  DEBUG_ERROR( "CAvEncoder::Ae_stop_encoder_local Ae_destory_stream_buffer(type:%d)(channel:%d)\n", _AST_MAIN_IFRAME_VIDEO_, phy_video_chn_num);
						  goto _OVER_;
					}
				}
			 
				_AV_SAFE_DELETE_ARRAY_(video_stream_it->pFrame_header_buffer);
			 
				/*remove task from recv list*/
				av_group_it->video_stream.erase(video_stream_it);
			}
			//if((av_group_it->video_stream.size() == 0))
			{
                //printf("------Ae_destory_audio_encoder-----------\n");
				/*destory audio encoder*/
				if((video_stream_type == _AST_MAIN_VIDEO_) && (av_group_it->audio_stream.phy_audio_chn_num >= 0))
				{
					if(Ae_destory_audio_encoder(av_group_it->audio_stream.type, av_group_it->audio_stream.phy_audio_chn_num, &av_group_it->audio_stream.audio_encoder_id) != 0)
					{
						DEBUG_ERROR( "CAvEncoder::Ae_stop_encoder_local Ae_destory_audio_encoder(type:%d)(channel:%d)\n", av_group_it->audio_stream.type, av_group_it->audio_stream.phy_audio_chn_num);
						goto _OVER_;
					}
					av_group_it->audio_stream.phy_audio_chn_num = -1;  // added 
				}
				if ((normal_audio == -1) && (av_group_it->video_stream.size() == 0)) //! not erase list due to video
				{
					m_pRecv_thread_task[recv_task_id].local_task_list.erase(av_group_it);

				}
			}
		}
		else
		{
			ret_val = 0;
			goto _OVER_;
		}
	}
	else
	{
		/*only audio, talkback*/
		if(audio_stream_type != _AAST_TALKBACK_)
		{
			DEBUG_ERROR( "CAvEncoder::Ae_stop_encoder_local Invalid parameter\n");
			goto _OVER_;
		}

        if(Ae_get_task_id(audio_stream_type, phy_audio_chn_num, &recv_task_id, &send_task_id) != 0)
        {
            DEBUG_ERROR( "CAvEncoder::Ae_stop_encoder_local Ae_get_task_id FAILED\n");
            goto _OVER_;
        }

        av_group_it = m_pRecv_thread_task[recv_task_id].local_task_list.begin();
        while(av_group_it != m_pRecv_thread_task[recv_task_id].local_task_list.end())
        {
            if(av_group_it->audio_stream.type == audio_stream_type)
            {
                break;
            }
            av_group_it ++;
        }
        if(av_group_it == m_pRecv_thread_task[recv_task_id].local_task_list.end())
        {
            ret_val = 0;
            goto _OVER_;
        }

        /*destroy hardware encoder*/
        if(Ae_destory_audio_encoder(audio_stream_type, phy_audio_chn_num, &av_group_it->audio_stream.audio_encoder_id) != 0)
        {
            DEBUG_ERROR( "CAvEncoder::Ae_stop_encoder_local Ae_destory_audio_encoder(type:%d)(channel:%d)\n", audio_stream_type, phy_audio_chn_num);
            goto _OVER_;
        }

        /*destory stream buffer*/
        if(Ae_destory_stream_buffer(av_group_it->audio_stream.stream_buffer_handle, video_stream_type, phy_video_chn_num) != 0)
        {
            DEBUG_ERROR( "CAvEncoder::Ae_stop_encoder_local Ae_destory_stream_buffer(type:%d)(channel:%d)\n", audio_stream_type, phy_audio_chn_num);
            goto _OVER_;
        }
        _AV_SAFE_DELETE_ARRAY_(av_group_it->audio_stream.pFrame_header_buffer);
        av_group_it->audio_stream.phy_audio_chn_num = -1;
        
        m_pRecv_thread_task[recv_task_id].local_task_list.erase(av_group_it);
    }
    ret_val = 0;

_OVER_:

    return ret_val; 
}

int CAvEncoder::Ae_stop_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, 
    av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, bool thread_control, int normal_audio)
{
    int ret_val = -1;

    if((!((video_stream_type == _AST_UNKNOWN_) || (phy_video_chn_num == -1))) || normal_audio == 1)
    {
        /*composite encoder*/
        if(Ae_stop_encoder_local(video_stream_type, phy_video_chn_num, audio_stream_type, phy_audio_chn_num, normal_audio) != 0)
        {
            DEBUG_ERROR( "CAvEncoder::Ae_stop_encoder Ae_start_composite_encoder_local FAILED(%d)\n", phy_video_chn_num);
            goto _OVER_;
        }
    }
    else
    {
        /*only audio, talkback*/
        if (audio_stream_type != _AAST_TALKBACK_)
        {
            DEBUG_ERROR( "CAvEncoder::Ae_stop_encoder Invalid parameter(type:%d)\n", audio_stream_type);
            goto _OVER_;
        }
        else
        {
            /*only audio, talkback*/
            if(Ae_stop_encoder_local(video_stream_type, phy_video_chn_num, audio_stream_type, phy_audio_chn_num, normal_audio) != 0)
            {
                DEBUG_ERROR( "CAvEncoder::Ae_stop_encoder Ae_start_composite_encoder_local FAILED(%d)\n", phy_video_chn_num);
                goto _OVER_;
            }
        }
    }

    ret_val = 0;

_OVER_:
    
    return ret_val;
}

int CAvEncoder::Ae_stop_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, 
    av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, int normal_audio)
{
    return Ae_stop_encoder(video_stream_type, phy_video_chn_num, audio_stream_type, phy_audio_chn_num, true, normal_audio);
}

#ifdef _AV_PRODUCT_CLASS_IPC_
int CAvEncoder::Ae_get_osd_parameter(av_osd_name_t osd_name, av_resolution_t resolution, int *pFont_name, int *pWidth, int *pHeight, int *pHScaler, int *pVScaler, int regionId /* = 0 */)
{
    int font_name = 0;
    char customer_name[32] = {0};
    pgAvDeviceInfo->Adi_get_customer_name(customer_name,sizeof(customer_name));
    switch(resolution)
    {
        case _AR_1080_:
            font_name = _AV_FONT_NAME_24_;
            _AV_POINTER_ASSIGNMENT_(pHScaler, 2);
            _AV_POINTER_ASSIGNMENT_(pVScaler, 2);
            break;
        case _AR_Q1080P_:
        case _AR_SVGA_:
        case _AR_XGA_:
            font_name = _AV_FONT_NAME_24_;
            _AV_POINTER_ASSIGNMENT_(pHScaler, 1);
            _AV_POINTER_ASSIGNMENT_(pVScaler, 1);
            break;
        case _AR_720_:
        case _AR_960P_:
            font_name = _AV_FONT_NAME_36_;
            _AV_POINTER_ASSIGNMENT_(pHScaler, 1);
            _AV_POINTER_ASSIGNMENT_(pVScaler, 1);
            break;
        case _AR_VGA_:
        default:
            font_name = _AV_FONT_NAME_18_;
            _AV_POINTER_ASSIGNMENT_(pHScaler, 1);
            _AV_POINTER_ASSIGNMENT_(pVScaler, 1);
            break;
        case _AR_QVGA_:
        case _AR_CIF_:
        case _AR_QCIF_: 
            font_name = _AV_FONT_NAME_12_;
            _AV_POINTER_ASSIGNMENT_(pHScaler, 1);
            _AV_POINTER_ASSIGNMENT_(pVScaler, 1);
            break;
         case _AR_3M_:
         case _AR_5M_:
        {
            font_name = _AV_FONT_NAME_36_;
            _AV_POINTER_ASSIGNMENT_(pHScaler, 2);
            _AV_POINTER_ASSIGNMENT_(pVScaler, 2);
        }
            break;
    }

    if (strcmp(customer_name, "CUSTOMER_RL") == 0)
    {
        switch(resolution)
        {   
            case _AR_VGA_:
            case _AR_QVGA_:
            case _AR_CIF_:
            case _AR_D1_: 
                switch(osd_name)
                {
                    case _AON_BUS_NUMBER_:
                    case _AON_COMMON_OSD1_:
                    case _AON_COMMON_OSD2_:   
                        font_name = _AV_FONT_NAME_24_;
                        break;            
                    default:
                        font_name = _AV_FONT_NAME_18_;
                        break;
                } 
                break;
            default:
                break;
        }
    }

    _AV_POINTER_ASSIGNMENT_(pFont_name, font_name);
    m_pAv_font_library->Afl_get_font_size(font_name, pWidth, pHeight);

    return 0;
}
#else
int CAvEncoder::Ae_get_osd_parameter(av_osd_name_t osd_name, av_resolution_t resolution, int *pFont_name, int *pWidth, int *pHeight, int *pHScaler, int *pVScaler, int regionId /* = 0 */)
{
    int font_name = 0;

    switch(resolution)
    {
        case _AR_1080_:
        default:
            switch(osd_name)
            {
                case _AON_CHN_NAME_:
                    font_name = _AV_FONT_NAME_36_;
                    _AV_POINTER_ASSIGNMENT_(pHScaler, 2);
                    _AV_POINTER_ASSIGNMENT_(pVScaler, 2);
                    break;
                case _AON_WATER_MARK:
                    font_name = _AV_FONT_NAME_12_;
                    _AV_POINTER_ASSIGNMENT_(pHScaler, 1);
                    _AV_POINTER_ASSIGNMENT_(pVScaler, 1);
                    break;
                case _AON_DATE_TIME_:
                case _AON_COMMON_OSD1_:
                case _AON_COMMON_OSD2_:    
                case _AON_BUS_NUMBER_:
                case _AON_SELFNUMBER_INFO_:
                case _AON_GPS_INFO_:
                case _AON_SPEED_INFO_:
                case _AON_EXTEND_OSD:
                    font_name = _AV_FONT_NAME_24_;
                    _AV_POINTER_ASSIGNMENT_(pHScaler, 2);
                    _AV_POINTER_ASSIGNMENT_(pVScaler, 2);
                    break;            
                default:
                    //_AV_FATAL_(1, "CAvEncoder::Ae_get_osd_parameter You must give the implement\n");
                    break;
            }
            break;
        case _AR_720_:
        case _AR_960P_:
        case _AR_Q1080P_:
            switch(osd_name)
            {
                case _AON_CHN_NAME_:
                    font_name = _AV_FONT_NAME_36_;
                    _AV_POINTER_ASSIGNMENT_(pHScaler, 1);
                    _AV_POINTER_ASSIGNMENT_(pVScaler, 1);
                    break;
                case _AON_WATER_MARK:
                    font_name = _AV_FONT_NAME_12_;
                    _AV_POINTER_ASSIGNMENT_(pHScaler, 1);
                    _AV_POINTER_ASSIGNMENT_(pVScaler, 1);
                    break;
                case _AON_DATE_TIME_:
                case _AON_COMMON_OSD1_:
                case _AON_COMMON_OSD2_:    
                case _AON_BUS_NUMBER_:
                case _AON_SELFNUMBER_INFO_:
                case _AON_GPS_INFO_:
                case _AON_SPEED_INFO_:
                case _AON_EXTEND_OSD:
		     font_name = _AV_FONT_NAME_36_;
                    _AV_POINTER_ASSIGNMENT_(pHScaler, 1);
                    _AV_POINTER_ASSIGNMENT_(pVScaler, 1);
                    break;
                default:
                    //_AV_FATAL_(1, "CAvEncoder::Ae_get_osd_parameter You must give the implement\n");
                    break;
            }
            break;
        case _AR_D1_:
        case _AR_960H_WD1_:
        case _AR_VGA_:
            switch(osd_name)
            {
                case _AON_CHN_NAME_:
				#ifdef _AV_PRODUCT_CLASS_IPC_
					font_name = _AV_FONT_NAME_18_;
				#else
                    font_name = _AV_FONT_NAME_24_;
				#endif
                    _AV_POINTER_ASSIGNMENT_(pHScaler, 1);
                    _AV_POINTER_ASSIGNMENT_(pVScaler, 1);
                    break;
                case _AON_WATER_MARK:
                    font_name = _AV_FONT_NAME_36_; //! for 04.471 origin 12
                    _AV_POINTER_ASSIGNMENT_(pHScaler, 1);
                    _AV_POINTER_ASSIGNMENT_(pVScaler, 1);
                    break;
                case _AON_DATE_TIME_:
                case _AON_COMMON_OSD1_:
                case _AON_COMMON_OSD2_:    
                case _AON_BUS_NUMBER_:
                case _AON_SELFNUMBER_INFO_:
                case _AON_GPS_INFO_:
                case _AON_SPEED_INFO_:
                    font_name = _AV_FONT_NAME_24_;
                    _AV_POINTER_ASSIGNMENT_(pHScaler, 1);
                    _AV_POINTER_ASSIGNMENT_(pVScaler, 1);
                    break;
				case _AON_EXTEND_OSD:
					if(PTD_6A_I == pgAvDeviceInfo->Adi_product_type())
					{
						font_name = _AV_FONT_NAME_12_;
						_AV_POINTER_ASSIGNMENT_(pHScaler, 2);
						_AV_POINTER_ASSIGNMENT_(pVScaler, 2);
					}
					else
					{
						font_name = _AV_FONT_NAME_24_;
						_AV_POINTER_ASSIGNMENT_(pHScaler, 1);
						_AV_POINTER_ASSIGNMENT_(pVScaler, 1);
					}
					break;                
                default:
                    //_AV_FATAL_(1, "CAvEncoder::Ae_get_osd_parameter You must give the implement\n");
                    break;
            }
            break;

        case _AR_HD1_:
        case _AR_960H_WHD1_:
            switch(osd_name)
            {
                case _AON_CHN_NAME_:
                font_name = _AV_FONT_NAME_12_;
                    _AV_POINTER_ASSIGNMENT_(pHScaler, 2);
                    _AV_POINTER_ASSIGNMENT_(pVScaler, 1);
                    break;
                case _AON_WATER_MARK:
                    font_name = _AV_FONT_NAME_12_;
                    _AV_POINTER_ASSIGNMENT_(pHScaler, 1);
                    _AV_POINTER_ASSIGNMENT_(pVScaler, 1);
                    break;
                case _AON_DATE_TIME_:
                case _AON_BUS_NUMBER_:
                case _AON_SELFNUMBER_INFO_:
                case _AON_GPS_INFO_:    
                case _AON_SPEED_INFO_:    
                case _AON_COMMON_OSD1_:
                case _AON_COMMON_OSD2_:
				case _AON_EXTEND_OSD:
                    font_name = _AV_FONT_NAME_12_;
                    _AV_POINTER_ASSIGNMENT_(pHScaler, 2);
                    _AV_POINTER_ASSIGNMENT_(pVScaler, 1);
                    break;
                default:
                    //_AV_FATAL_(1, "CAvEncoder::Ae_get_osd_parameter You must give the implement\n");
                    break;
            }            
            break;
        case _AR_CIF_:
        case _AR_QCIF_:
        case _AR_960H_WCIF_:
        case _AR_960H_WQCIF_:
        case _AR_QVGA_:
            switch(osd_name)
            {
                case _AON_CHN_NAME_:
                    font_name = _AV_FONT_NAME_12_;
                    _AV_POINTER_ASSIGNMENT_(pHScaler, 1);
                    _AV_POINTER_ASSIGNMENT_(pVScaler, 1);
                    break;
                case _AON_WATER_MARK:
                    font_name = _AV_FONT_NAME_12_;
                    _AV_POINTER_ASSIGNMENT_(pHScaler, 1);
                    _AV_POINTER_ASSIGNMENT_(pVScaler, 1);
                    break;
                case _AON_DATE_TIME_:
                case _AON_BUS_NUMBER_:
                case _AON_SELFNUMBER_INFO_:
                case _AON_GPS_INFO_:    
                case _AON_SPEED_INFO_:
                case _AON_COMMON_OSD1_:
                case _AON_COMMON_OSD2_:
                //case _AON_EXTEND_OSD:
                    font_name = _AV_FONT_NAME_12_;
                    _AV_POINTER_ASSIGNMENT_(pHScaler, 1);
                    _AV_POINTER_ASSIGNMENT_(pVScaler, 1);
                    break;
				case _AON_EXTEND_OSD:
					{
						font_name = _AV_FONT_NAME_12_;
						_AV_POINTER_ASSIGNMENT_(pHScaler, 1);
						_AV_POINTER_ASSIGNMENT_(pVScaler, 1);
					}
					break;
                default:
                    //_AV_FATAL_(1, "CAvEncoder::Ae_get_osd_parameter You must give the implement\n");
                    break;
            }
            break;
    }
    
    char customer_name[32] = {0};
    pgAvDeviceInfo->Adi_get_customer_name(customer_name,sizeof(customer_name));

    if (strcmp(customer_name, "CUSTOMER_BJGJ") == 0)
    {
        switch(resolution)
        {   
            case _AR_D1_: 
                switch(osd_name)
                {
                    case _AON_GPS_INFO_:  
                    case _AON_SPEED_INFO_:  
                        font_name = _AV_FONT_NAME_16_;
                        break;            
                    default:
                        break;
                } 
                break;
            default:
                break;
        }
    }
        
    _AV_POINTER_ASSIGNMENT_(pFont_name, font_name);
    m_pAv_font_library->Afl_get_font_size(font_name, pWidth, pHeight);
    return 0;
}
#endif

int CAvEncoder::Ae_update_common_osd_content(av_video_stream_type_t  stream_type, int phy_video_chn_num, av_osd_name_t osd_name, char osd_content[], unsigned int osd_len)
{
    std::list<av_osd_item_t>::iterator osd_item;

    osd_item = m_pOsd_thread->osd_list.begin();
    
    while(osd_item != m_pOsd_thread->osd_list.end())
    {
        if((stream_type == osd_item->video_stream.type) &&(phy_video_chn_num == osd_item->video_stream.phy_video_chn_num) &&\
            osd_name == osd_item->osd_name)
        {
        
            if(osd_name == _AON_COMMON_OSD1_)
            {
                
                if((_AON_COMMON_OSD1_ == osd_item->osd_name) && (0 != osd_item->video_stream.video_encoder_para.have_common1_osd))
                {
                    int len = (sizeof(osd_item->video_stream.video_encoder_para.osd1_content)-1)>osd_len? \
                        osd_len:(sizeof(osd_item->video_stream.video_encoder_para.osd1_content)-1);
                    memcpy(osd_item->video_stream.video_encoder_para.osd1_content, osd_content, len);
                    osd_item->video_stream.video_encoder_para.osd1_content[len] =  '\0';
                    
                }                         
            }
             if(osd_name == _AON_COMMON_OSD2_)
            {
                
                if((_AON_COMMON_OSD2_ == osd_item->osd_name) && (0 != osd_item->video_stream.video_encoder_para.have_common2_osd))
                {
                    int len = (sizeof(osd_item->video_stream.video_encoder_para.osd2_content)-1)>osd_len? \
                        osd_len:(sizeof(osd_item->video_stream.video_encoder_para.osd2_content)-1);
                    memcpy(osd_item->video_stream.video_encoder_para.osd2_content, osd_content, len);
                    osd_item->video_stream.video_encoder_para.osd2_content[len] =  '\0';
                   
                }                         
            }
             
        
        
            return 0;
        }
        osd_item++;
    }
      
    return -1;

}

bool CAvEncoder::Ae_is_regin_passed(const av_osd_item_t &regin, const std::vector<av_osd_item_t> &passed_path) const

{
    if(passed_path.empty())
     {
        return false;
     }

    std::vector<av_osd_item_t>::const_iterator iter = passed_path.begin();
    while(iter != passed_path.end())
     {
        if((regin.video_stream.phy_video_chn_num == iter->video_stream.phy_video_chn_num) && \
                (regin.video_stream.type == iter->video_stream.type) &&\
                (regin.display_x == iter->display_x) && (regin.display_y == iter->display_y) )
        {
            return true;
        }

        ++iter;
     }

    return false;
}

bool CAvEncoder::Ae_is_regin_overlap_with_exist_regins(const av_osd_item_t &regin, av_osd_item_t *overlap_regin) const

{
    av_osd_thread_task_t *pAv_thread_task = m_pOsd_thread;
    std::list<av_osd_item_t>::const_iterator av_osd_merge_item_it;

    av_osd_merge_item_it = pAv_thread_task->osd_list.begin();
    while(av_osd_merge_item_it != pAv_thread_task->osd_list.end())
    {
        if((_AOS_OVERLAYED_ == av_osd_merge_item_it->overlay_state || _AOS_CREATED_ == av_osd_merge_item_it->overlay_state) && \
            (regin.video_stream.phy_video_chn_num == av_osd_merge_item_it->video_stream.phy_video_chn_num) && \
                (regin.video_stream.type == av_osd_merge_item_it->video_stream.type))
        {
            if(!((regin.display_x >= av_osd_merge_item_it->display_x + av_osd_merge_item_it->bitmap_width) ||\
                    (av_osd_merge_item_it->display_x >= regin.display_x + regin.bitmap_width) ||\
                        (regin.display_y >= av_osd_merge_item_it->display_y + av_osd_merge_item_it->bitmap_height) || \
                            (av_osd_merge_item_it->display_y >= regin.display_y + regin.bitmap_height)))
            {
                if(NULL != overlap_regin)
                {
                    overlap_regin->display_x = av_osd_merge_item_it->display_x;
                    overlap_regin->display_y = av_osd_merge_item_it->display_y;
                    overlap_regin->bitmap_width = av_osd_merge_item_it->bitmap_width;
                    overlap_regin->bitmap_height = av_osd_merge_item_it->bitmap_height;
                    overlap_regin->video_stream.phy_video_chn_num = av_osd_merge_item_it->video_stream.phy_video_chn_num;
                    overlap_regin->video_stream.type = av_osd_merge_item_it->video_stream.type;  
                }
                
                return true;
            }
     
        }
        ++av_osd_merge_item_it;
    }

    return false;
}

int CAvEncoder::Ae_search_overlay_coordinate(av_osd_item_t &osd_item, std::vector<av_osd_item_t> &passed_path, \
    std::queue<av_osd_item_t> &possible_path,int video_width, int video_height)
{
    if(possible_path.empty())
    {
        DEBUG_ERROR( "the possible path is null\n");
        return -1;
    }

    while(!possible_path.empty())
     {
        //DEBUG_MESSAGE("[ddh]possible path num:%d \n", possible_path.size());
         av_osd_item_t regin = possible_path.front();
         av_osd_thread_task_t *pAv_thread_task = m_pOsd_thread;
         std::list<av_osd_item_t>::const_iterator osd_item_it;
         
         osd_item_it = pAv_thread_task->osd_list.begin();
         while(osd_item_it != pAv_thread_task->osd_list.end())
          {
                 
                if((_AOS_OVERLAYED_ == osd_item_it->overlay_state || _AOS_CREATED_ == osd_item_it->overlay_state) && \
                        (regin.video_stream.phy_video_chn_num == osd_item_it->video_stream.phy_video_chn_num) && \
                            (regin.video_stream.type == osd_item_it->video_stream.type))
                {
                    if(!((regin.display_x >= osd_item_it->display_x + osd_item_it->bitmap_width) ||\
                            (osd_item_it->display_x >= regin.display_x + regin.bitmap_width) ||\
                                (regin.display_y >= osd_item_it->display_y + osd_item_it->bitmap_height) || \
                                    (osd_item_it->display_y >= regin.display_y + regin.bitmap_height))) 
                    {
                        av_osd_item_t overlap_regin; 
                        memset(&overlap_regin, 0, sizeof(av_osd_item_t));
                        overlap_regin.display_x = osd_item_it->display_x;
                        overlap_regin.display_y = osd_item_it->display_y;
                        overlap_regin.bitmap_width = osd_item_it->bitmap_width;
                        overlap_regin.bitmap_height = osd_item_it->bitmap_height;
                        overlap_regin.video_stream.phy_video_chn_num = osd_item_it->video_stream.phy_video_chn_num;
                        overlap_regin.video_stream.type = osd_item_it->video_stream.type;
                        
                        if(!Ae_is_regin_passed(overlap_regin, passed_path))
                        {
                            // search towards up
                            //printf("[ddh]turn up! \n");
                            if(osd_item_it->display_y - regin.bitmap_height >= 0)
                            {
                                regin.display_y = osd_item_it->display_y - regin.bitmap_height;
                                regin.display_x = possible_path.front().display_x;
                                regin.bitmap_width = possible_path.front().bitmap_width;
                                regin.bitmap_height = possible_path.front().bitmap_height;
                                regin.video_stream.phy_video_chn_num = possible_path.front().video_stream.phy_video_chn_num;
                                regin.video_stream.type = possible_path.front().video_stream.type;


                                if(Ae_is_regin_overlap_with_exist_regins(regin, NULL))
                                {
                                    possible_path.push(regin);
                                }
                                else
                                {
                                    osd_item.display_x = regin.display_x;
                                    osd_item.display_y = regin.display_y;
                                   // printf("[ddh] find valid coordinate[x:%d y:%d] \n", osd_item.display_x , osd_item.display_y);
                                    return 0;
                                }                                 
                            }

                            // search towards down
                           // printf("[ddh]turn down! \n");
                            if(osd_item_it->display_y + osd_item_it->bitmap_height + regin.bitmap_height <= video_height)
                            {
                                regin.display_y = osd_item_it->display_y + osd_item_it->bitmap_height;
                                regin.display_x = possible_path.front().display_x;
                                regin.bitmap_width = possible_path.front().bitmap_width;
                                regin.bitmap_height = possible_path.front().bitmap_height;
                                regin.video_stream.phy_video_chn_num = possible_path.front().video_stream.phy_video_chn_num;
                                regin.video_stream.type = possible_path.front().video_stream.type;


                                if(Ae_is_regin_overlap_with_exist_regins(regin, NULL))
                                {
                                    possible_path.push(regin);
                                }
                                else
                                {
                                    osd_item.display_x = regin.display_x;
                                    osd_item.display_y = regin.display_y;
                                    //printf("[ddh] find valid coordinate[x:%d y:%d] \n", osd_item.display_x , osd_item.display_y);
                                    return 0;
                                }                                  
                            }
                            
                            // we search the overlay coordinate towards right hand side
                           // DEBUG_MESSAGE("[ddh]turn right! \n");
                            if(osd_item_it->display_x + osd_item_it->bitmap_width + regin.bitmap_width <= video_width)
                            {
                                regin.display_x = osd_item_it->display_x + osd_item_it->bitmap_width;
                                regin.display_y = possible_path.front().display_y;
                                regin.bitmap_width = possible_path.front().bitmap_width;
                                regin.bitmap_height = possible_path.front().bitmap_height;
                                regin.video_stream.phy_video_chn_num = possible_path.front().video_stream.phy_video_chn_num;
                                regin.video_stream.type = possible_path.front().video_stream.type;

                                if(Ae_is_regin_overlap_with_exist_regins(regin, NULL))
                                {
                                    possible_path.push(regin);
                                }
                                else
                                {
                                    osd_item.display_x = regin.display_x;
                                    osd_item.display_y = regin.display_y;
                                    //DEBUG_MESSAGE("[ddh] find valid coordinate[x:%d y:%d] \n", osd_item.display_x , osd_item.display_y);
                                    return 0;
                                }
                            }

                            //encond search towards left hand side
                           // DEBUG_MESSAGE("[ddh]turn left! \n");
                            if(osd_item_it->display_x - regin.bitmap_width >= 0)
                            {
                                regin.display_x  = osd_item_it->display_x - regin.bitmap_width;
                                regin.display_y = possible_path.front().display_y;
                                regin.bitmap_width = possible_path.front().bitmap_width;
                                regin.bitmap_height = possible_path.front().bitmap_height;
                                regin.video_stream.phy_video_chn_num = possible_path.front().video_stream.phy_video_chn_num;
                                regin.video_stream.type = possible_path.front().video_stream.type;


                                if(Ae_is_regin_overlap_with_exist_regins(regin, NULL))
                                {
                                    possible_path.push(regin);
                                }
                                else
                                {
                                    osd_item.display_x = regin.display_x;
                                    osd_item.display_y = regin.display_y;
                                   // printf("[ddh] find valid coordinate[x:%d y:%d] \n", osd_item.display_x , osd_item.display_y);
                                    return 0;
                                }                                 
                            }

                            passed_path.push_back(overlap_regin);
                        }
                    }
                }

                ++osd_item_it;
          }
          possible_path.pop();
     }

    return -1;
}


int CAvEncoder::Ae_get_overlay_coordinate(av_osd_item_t &osd_item, int video_width, int video_height)
{
    std::vector<av_osd_item_t> passed_path;
    std::queue<av_osd_item_t> possible_path;

    possible_path.push(osd_item);

    return Ae_search_overlay_coordinate(osd_item, passed_path, possible_path, video_width, video_height);
    
}


int CAvEncoder::Ae_translate_coordinate(std::list<av_osd_item_t>::iterator &av_osd_item_it, int osd_width, int osd_height)
{
    int video_width = -1;
    int video_height = -1;
    
    //av_osd_thread_task_t *pAv_thread_task = m_pOsd_thread;
    std::list<av_osd_item_t>::iterator av_osd_merge_item_it;
    //av_osd_name_t find_osd_name = _AON_DATE_TIME_;
    //bool merge_osd = false;

    switch(av_osd_item_it->osd_name)
    {
    case _AON_DATE_TIME_:
            //find_osd_name = _AON_WATER_MARK;
            av_osd_item_it->display_x = av_osd_item_it->video_stream.video_encoder_para.date_time_osd_x;
            av_osd_item_it->display_y = av_osd_item_it->video_stream.video_encoder_para.date_time_osd_y;
            break;
    case _AON_BUS_NUMBER_:
        //find_osd_name = _AON_SELFNUMBER_INFO_;
        av_osd_item_it->display_x = av_osd_item_it->video_stream.video_encoder_para.bus_number_osd_x;
        av_osd_item_it->display_y = av_osd_item_it->video_stream.video_encoder_para.bus_number_osd_y;
        break;
     case _AON_SELFNUMBER_INFO_:
        //find_osd_name = _AON_DATE_TIME_;
        av_osd_item_it->display_x = av_osd_item_it->video_stream.video_encoder_para.bus_selfnumber_osd_x;
        av_osd_item_it->display_y = av_osd_item_it->video_stream.video_encoder_para.bus_selfnumber_osd_y;
        break;
    case _AON_GPS_INFO_:
        //find_osd_name = _AON_BUS_NUMBER_;
        av_osd_item_it->display_x = av_osd_item_it->video_stream.video_encoder_para.gps_osd_x;
        av_osd_item_it->display_y = av_osd_item_it->video_stream.video_encoder_para.gps_osd_y;
        break;
    case _AON_SPEED_INFO_:
        //find_osd_name = _AON_GPS_INFO_;
        av_osd_item_it->display_x = av_osd_item_it->video_stream.video_encoder_para.speed_osd_x;
        av_osd_item_it->display_y = av_osd_item_it->video_stream.video_encoder_para.speed_osd_y;
        break;        
    case _AON_COMMON_OSD1_:
        //find_osd_name = _AON_SPEED_INFO_;
        av_osd_item_it->display_x = av_osd_item_it->video_stream.video_encoder_para.common1_osd_x;
        av_osd_item_it->display_y = av_osd_item_it->video_stream.video_encoder_para.common1_osd_y;
        break;
    case _AON_COMMON_OSD2_:
        //find_osd_name = _AON_COMMON_OSD1_;
        av_osd_item_it->display_x = av_osd_item_it->video_stream.video_encoder_para.common2_osd_x;
        av_osd_item_it->display_y = av_osd_item_it->video_stream.video_encoder_para.common2_osd_y;
        break;        
    case _AON_CHN_NAME_:
        //find_osd_name = _AON_COMMON_OSD2_;
        av_osd_item_it->display_x = av_osd_item_it->video_stream.video_encoder_para.channel_name_osd_x;
        av_osd_item_it->display_y = av_osd_item_it->video_stream.video_encoder_para.channel_name_osd_y;
        break;
    case _AON_WATER_MARK:
        //find_osd_name = _AON_CHN_NAME_;
        av_osd_item_it->display_x = av_osd_item_it->video_stream.video_encoder_para.water_mark_osd_x;
        av_osd_item_it->display_y = av_osd_item_it->video_stream.video_encoder_para.water_mark_osd_y;
        break;
	case _AON_EXTEND_OSD:
#if (defined(_AV_PLATFORM_HI3520D_V300_) || defined(_AV_PLATFORM_HI3515_))
		//find_osd_name = _AON_EXTEND_OSD;
		av_osd_item_it->display_x = av_osd_item_it->video_stream.video_encoder_para.stuExtendOsdInfo[av_osd_item_it->extend_osd_id].usOsdX;
		av_osd_item_it->display_y = av_osd_item_it->video_stream.video_encoder_para.stuExtendOsdInfo[av_osd_item_it->extend_osd_id].usOsdY;
#endif
		break;		  
        default:
            _AV_FATAL_(1, "CAvEncoder::Ae_translate_coordinate You must give the implement\n");
            break;
    }
    /* 根据不同分辨率坐标转换 */
    pgAvDeviceInfo->Adi_covert_coordinate(av_osd_item_it->video_stream.video_encoder_para.resolution
            , av_osd_item_it->video_stream.video_encoder_para.tv_system
            , &av_osd_item_it->display_x, &av_osd_item_it->display_y, 1);

    pgAvDeviceInfo->Adi_get_video_size(av_osd_item_it->video_stream.video_encoder_para.resolution, av_osd_item_it->video_stream.video_encoder_para.tv_system, &video_width, &video_height);

#if defined(_AV_PRODUCT_CLASS_IPC_)
    pgAvDeviceInfo->Adi_covert_vidoe_size_by_rotate(&video_width, &video_height);
#endif

#if defined(_AV_PLATFORM_HI3520D_V300_)		//6AII_AV12机型新增osd内容对齐功能///
	if(OSD_TEST_FLAG == pgAvDeviceInfo->Adi_product_type())
	{
		OsdItem_t* pOsdItem = &(av_osd_item_it->video_stream.video_encoder_para.stuExtendOsdInfo[av_osd_item_it->extend_osd_id]);
		if(pOsdItem->ucAlign == 0)		//left align///
		{
			//已经是左对齐，无需进一步处理///
		}
		else if(pOsdItem->ucAlign == 1)		//right align///
		{
			if((av_osd_item_it->display_x + osd_width) < (video_width - 5))
			{
				av_osd_item_it->display_x = video_width - osd_width;
			}
		}
		else if(pOsdItem->ucAlign == 2)		//middle align///
		{
			//暂时按照左对齐处理///
		}
	}
	else
#endif
	{
		if((av_osd_item_it->display_x + osd_width) > video_width)
		{
			av_osd_item_it->display_x = video_width - osd_width;
		}
	}

    if(av_osd_item_it->display_x < 0)
    {
        av_osd_item_it->display_x = 0;
    }
    if((av_osd_item_it->display_x + osd_width) > video_width)
    {
        av_osd_item_it->display_x = video_width - osd_width;
    }

    if(av_osd_item_it->display_y < 0)
    {
        av_osd_item_it->display_y = 0;
    }
    if((av_osd_item_it->display_y + osd_height) > video_height)
    {
        av_osd_item_it->display_y = video_height - osd_height;
    }
    if(av_osd_item_it->display_y < 0)
    {
        av_osd_item_it->display_y = 0;
    }

#if 0
    /*find which osd region need to merge*/ 
    av_osd_merge_item_it = pAv_thread_task->osd_list.begin();
    while(av_osd_merge_item_it != pAv_thread_task->osd_list.end())
    {
        if ((av_osd_merge_item_it->osd_name == find_osd_name) 
            && (av_osd_merge_item_it->video_stream.phy_video_chn_num == av_osd_item_it->video_stream.phy_video_chn_num)
            && (av_osd_merge_item_it->video_stream.type == av_osd_item_it->video_stream.type))
        {
            merge_osd = true;
            break;
        }
        av_osd_merge_item_it++;
    }
    /*two osd region merge*/ 
    if (true == merge_osd)
    {
        bool row_overlay = true;
        bool col_overlay = true;
        if ((av_osd_item_it->display_y > (av_osd_merge_item_it->display_y + av_osd_merge_item_it->bitmap_height)) 
            || (av_osd_merge_item_it->display_y > (av_osd_item_it->display_y + osd_height)))
        {/*列无重叠*/
            row_overlay = false;
        }

        if ((av_osd_item_it->display_x > (/*rightpos*/av_osd_merge_item_it->display_x + av_osd_merge_item_it->bitmap_width))
            || (av_osd_merge_item_it->display_x > (/*rightpos*/av_osd_item_it->display_x + osd_width)))
        {/*行无重叠*/
            col_overlay = false; 
        }

        if ((true == row_overlay) && (true == col_overlay))
        {/*行与列重叠，移同行位置*/
            if(av_osd_item_it->display_x >= av_osd_merge_item_it->display_x)
            {
                av_osd_item_it->display_x = av_osd_merge_item_it->display_x + av_osd_merge_item_it->bitmap_width;
                if((av_osd_item_it->display_x + osd_width) > video_width)
                {
                    av_osd_item_it->display_x = video_width - osd_width;
                }
            }
            else
            {
                av_osd_item_it->display_x = av_osd_merge_item_it->display_x - osd_width;
                if (av_osd_item_it->display_x < 0)
                {
                    av_osd_item_it->display_x = 0;
                }
            }

            if ((av_osd_item_it->display_x > (/*rightpos*/av_osd_merge_item_it->display_x + av_osd_merge_item_it->bitmap_width))
                || (av_osd_merge_item_it->display_x > (/*rightpos*/av_osd_item_it->display_x + osd_width)))
            {/*行无重叠*/
                col_overlay = false; 
            }
        }

        if((true == row_overlay) && (true == col_overlay))
        {/*调整之后，行与列还有重叠，做错行处理*/
            if(av_osd_item_it->display_y >= av_osd_merge_item_it->display_y)
            {
                av_osd_item_it->display_y = av_osd_merge_item_it->display_y + av_osd_merge_item_it->bitmap_height;                
                if((av_osd_item_it->display_y + osd_height) > video_height)
                {
                    av_osd_item_it->display_y = video_height - osd_height;
                }
            }
            else
            {
                av_osd_item_it->display_y = av_osd_merge_item_it->display_y - osd_height;               
                if(av_osd_item_it->display_y < 0)
                {
                    av_osd_item_it->display_y = 0;
                }
            }
        }
    }
#else
    av_osd_item_t temp_regin;

    memset(&temp_regin, 0, sizeof(av_osd_item_t));
    temp_regin.display_x = av_osd_item_it->display_x;
    temp_regin.display_y = av_osd_item_it->display_y;
    temp_regin.bitmap_width = osd_width;
    temp_regin.bitmap_height = osd_height;
    temp_regin.video_stream.phy_video_chn_num = av_osd_item_it->video_stream.phy_video_chn_num;
    temp_regin.video_stream.type = av_osd_item_it->video_stream.type;
    temp_regin.osd_name = av_osd_item_it->osd_name;
    Ae_get_overlay_coordinate(temp_regin, video_width, video_height);

    av_osd_item_it->display_x = temp_regin.display_x;
    av_osd_item_it->display_y = temp_regin.display_y;            
#endif
    av_osd_item_it->display_x = av_osd_item_it->display_x / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
    av_osd_item_it->display_y = av_osd_item_it->display_y / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;

    return 0;
}


#if defined(_AV_PRODUCT_CLASS_IPC_)
int CAvEncoder::Ae_translate_coordinate_for_cnr(av_osd_item_t &osd_item, int osd_width)
{
    int video_width = -1;
    int video_height = -1;

    pgAvDeviceInfo->Adi_get_video_size(osd_item.video_stream.video_encoder_para.resolution, osd_item.video_stream.video_encoder_para.tv_system, &video_width, &video_height);
#ifdef _AV_PLATFORM_HI3518C_
    pgAvDeviceInfo->Adi_covert_vidoe_size_by_rotate(&video_width, &video_height);
#endif

         av_osd_thread_task_t *pAv_thread_task = m_pOsd_thread;
         std::list<av_osd_item_t>::const_iterator osd_item_it;
         
         osd_item_it = pAv_thread_task->osd_list.begin();
         while(osd_item_it != pAv_thread_task->osd_list.end())
         {
             if((_AOS_OVERLAYED_ == osd_item_it->overlay_state) && \
                     (osd_item.video_stream.phy_video_chn_num == osd_item_it->video_stream.phy_video_chn_num) && \
                         (osd_item.video_stream.type == osd_item_it->video_stream.type))
             {
                osd_item.display_x = osd_item_it->bitmap_width;
                break;
             }

            ++osd_item_it;
         }

         //first overlay
         if(osd_item_it == pAv_thread_task->osd_list.end())
         {
            osd_item.display_x = 0;
         }
          
        osd_item.display_y = 10;
        
        osd_item.display_x = osd_item.display_x / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        osd_item.display_y = osd_item.display_y / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;

        _AV_KEY_INFO_(_AVD_ENC_, "the new coordinate of osd:%d  is:[x:%d y:%d] \n", osd_item.osd_name, osd_item.display_x, osd_item.display_y);
        return 0;
}
#endif

int CAvEncoder::Ae_utf8string_2_bmp( av_osd_name_t eOsdName, av_resolution_t eResolution, unsigned char* pszBitmapAddr, unsigned short u16BitMapWidth, unsigned short u16BitmapHeight
            , char *pUtf8_string, unsigned short text_color, unsigned short bg_color, unsigned short edge_color )
{
    CAvUtf8String utf8_string(pUtf8_string, true);
    av_ucs4_t char_unicode = 0;
    av_char_lattice_info_t char_lattice;
    int char_pixel_width_index = 0;
    int char_pixel_height_index = 0;
    int horizontal_scale_index = 0;
    int vertical_scale_index = 0;
    int font_width = 0;
    int font_height = 0;
    int font_name = 0;
    int horizontal_scaler = 0;
    int vertical_scaler = 0;
    unsigned short lib_mark = 0x01;
    unsigned short *pBitmap_pixel = NULL;
    int horizontal_pixel = 0;
    int bitmap_x = 0;
    int bitmap_y = 0;
    bool draw_edge = false;
	bool bBold = false;	//是否加粗,解决6A一代叠加REC红色时，叠加不全问题

    if(eOsdName == _AON_DATE_TIME_)
    {
        lib_mark = 0xffff;
    }
    else
    {
        lib_mark = 0x1;
    }
    
	if(text_color == 0xfc00)
	{
		Ae_get_osd_parameter(eOsdName, eResolution, &font_name, &font_width, &font_height, &horizontal_scaler, &vertical_scaler, 1);
	}
	else
	{
		Ae_get_osd_parameter(eOsdName, eResolution, &font_name, &font_width, &font_height, &horizontal_scaler, &vertical_scaler);
	}

#if defined(_AV_PLATFORM_HI3515_)
	unsigned short overlayLen = 0;
	bool specialOsdChar = false;
	_osd_cache_info_* pOsdInfo = NULL;
	int temp = 0;
	//roductType productType = pgAvDeviceInfo->Adi_product_type();
	//(PTD_6A_I != productType)
	{
		if(eResolution != _AR_D1_ && eResolution != _AR_HD1_ && eResolution != _AR_CIF_)
		{
			DEBUG_ERROR("CAvEncoder::Ae_utf8string_2_bmp, resolution(%d) is illegal\n");
			return -1;
		}
	}
#endif

    /*transparent*/
#define _AV_TRANSPARENT_COLOR_  0x00   
	int mapLen = u16BitMapWidth* horizontal_scaler * u16BitmapHeight * vertical_scaler * 2;
	//printf("----------size = %d, w = %d, h = %d, v = %d, h = %d\n", u16BitMapWidth* horizontal_scaler * u16BitmapHeight * vertical_scaler * 2, u16BitMapWidth, u16BitmapHeight, vertical_scaler, horizontal_scaler);
    memset(pszBitmapAddr, _AV_TRANSPARENT_COLOR_, mapLen);
    while(utf8_string.Next_char( &char_unicode) != false)
    {
        if(m_pAv_font_library->Afl_get_char_lattice(font_name, char_unicode, &char_lattice, lib_mark) != 0)
        {
            //DEBUG_ERROR( "CAvEncoder::Ae_utf8string_2_bmp m_pAv_font_library->Afl_get_char_lattice FAILED(0x%08lx)(%d)(0x%04x)(%s)\n", char_unicode, font_name, lib_mark, pUtf8_string);
            continue;//return -1;
        }
		//printf("cha = %lx, w = %d, h = %d, v = %d, h = %d\n", char_unicode, char_lattice.width, char_lattice.height, vertical_scaler, horizontal_scaler);
        
#if defined(_AV_PLATFORM_HI3515_)    
        overlayLen += char_lattice.width * horizontal_scaler;
        if(overlayLen >= u16BitMapWidth)	//超出范围，返回///
        {
        	return 0;
        }
		//(PTD_6A_I != productType)
		{
			specialOsdChar = false;
			pOsdInfo = NULL;
			
			if(m_OsdCharMap.find(char_unicode) != m_OsdCharMap.end())
			{
				pOsdInfo = &(m_OsdCache[(m_OsdCharMap[char_unicode])][m_OsdFontLibMap[lib_mark]][eResolution][font_name]);

				temp = pOsdInfo->width * pOsdInfo->hScale;
				if(pOsdInfo->szOsd != NULL) //exist in cache
				{
	                //DEBUG_CRITICAL("char_unicode = %lx, latic height:%d, osd height:%d\n", char_unicode, pOsdInfo->height, u16BitmapHeight);
					for(int lineId = 0 ; lineId < pOsdInfo->height* pOsdInfo->vScale ; lineId++) //
					{
						memcpy(pszBitmapAddr + 2* (u16BitMapWidth * lineId + horizontal_pixel), (char*)pOsdInfo->szOsd +2*(lineId * temp), temp * 2);
					}

					horizontal_pixel += temp;
					continue;
				}
				specialOsdChar = true;
			}
		}
#endif		
        
#if defined(_AV_PLATFORM_HI3515_)
		//if(PTD_6A_I != productType)
		{
	        if(specialOsdChar)
	        {
				unsigned int char_size = 2 * char_lattice.width * char_lattice.height * horizontal_scaler * vertical_scaler;
				if((pOsdInfo->szOsd = (unsigned short*)new char[char_size]) == NULL)
				{
					DEBUG_ERROR("CAvEncoder::Ae_utf8string_2_bmp, malloc failed, char_code = 0x%lx, size = %d\n", char_unicode, char_size);
					continue;
				}
				
				memset((char*)(pOsdInfo->szOsd), 0x0, char_size);
				pOsdInfo->width = char_lattice.width;
				pOsdInfo->height = char_lattice.height;
				pOsdInfo->hScale = horizontal_scaler;
				pOsdInfo->vScale = vertical_scaler;
	        }
        }
 #endif       

        for(char_pixel_height_index = 0; char_pixel_height_index < char_lattice.height; char_pixel_height_index ++)/*very row*/
        {
            for(vertical_scale_index = 0; vertical_scale_index < vertical_scaler; vertical_scale_index ++)
            {
                for(char_pixel_width_index = 0; char_pixel_width_index < char_lattice.width; char_pixel_width_index ++)
                {
                    for(horizontal_scale_index = 0; horizontal_scale_index < horizontal_scaler; horizontal_scale_index ++)
                    {
                        bitmap_x = horizontal_pixel + char_pixel_width_index * horizontal_scaler + horizontal_scale_index;
                        bitmap_y = char_pixel_height_index * vertical_scaler + vertical_scale_index;
                        pBitmap_pixel = (unsigned short *)(pszBitmapAddr + u16BitMapWidth * 2 * bitmap_y + bitmap_x * 2);
                        if(u16BitMapWidth * 2 * bitmap_y + bitmap_x * 2 >= mapLen)
                        {
                        	DEBUG_ERROR("111111111111111, curLen = %d, totalLen = %d\n", u16BitMapWidth * 2 * bitmap_y + bitmap_x * 2, mapLen);
                        }
						//printf("=====, w = %d, h = %d, pos = %d\n", char_pixel_width_index, char_pixel_height_index, u16BitMapWidth * 2 * bitmap_y + bitmap_x * 2);
                        if((char_lattice.pLattice[char_pixel_height_index * char_lattice.stride + char_pixel_width_index / 8] & (0x01 << (7 - char_pixel_width_index % 8))) == 0)
                        {                           
                            draw_edge = false;
                            bBold = false;
                            /*top*/
                            if((draw_edge == false) && (char_pixel_height_index > 0) && 
                                    ((char_lattice.pLattice[(char_pixel_height_index - 1) * char_lattice.stride + char_pixel_width_index / 8] & 
                                            (0x01 << (7 - char_pixel_width_index % 8))) != 0))
                            {
                                draw_edge = true;
                            }

                            /*bottom*/
                            if((draw_edge == false) && (char_pixel_height_index < (char_lattice.height - 1)) && 
                                        ((char_lattice.pLattice[(char_pixel_height_index + 1) * char_lattice.stride + char_pixel_width_index / 8] & 
                                            (0x01 << (7 - char_pixel_width_index % 8))) != 0))
                            {
                                draw_edge = true;
                                bBold = true;
                            }

                            /*left*/
                            if((draw_edge == false) && (char_pixel_width_index > 0) && 
                                    ((char_lattice.pLattice[char_pixel_height_index * char_lattice.stride + (char_pixel_width_index - 1) / 8] & 
                                            (0x01 << (7 - (char_pixel_width_index - 1) % 8))) != 0))
                            {
                                draw_edge = true;
                            }

                            /*right*/
                            if((draw_edge == false) && (char_pixel_width_index < (char_lattice.width - 1)) && 
                                    ((char_lattice.pLattice[char_pixel_height_index * char_lattice.stride + (char_pixel_width_index + 1) / 8] & 
                                            (0x01 << (7 - (char_pixel_width_index + 1) % 8))) != 0))
                            {
                                draw_edge = true;
                                bBold = true;
                            }

                            if(draw_edge == false)
                            {
                                *pBitmap_pixel = bg_color;
                            }
                            else
                            {
                                *pBitmap_pixel = edge_color;
                            }
#ifndef _AV_PLATFORM_HI3515_	//after change font lib, red region display is ok, so not use this method///
							if(bBold && text_color == 0xfc00)	//if color is red, bottom and right edge use text-color to bold the text///
							{
								*pBitmap_pixel = text_color;
							}
#endif
                        }
                        else
                        {
                            *pBitmap_pixel = text_color;
                        }
#if defined(_AV_PLATFORM_HI3515_)
						//(PTD_6A_I != productType)
						{
	                        if(specialOsdChar)
	                        {
								memcpy((char*)pOsdInfo->szOsd + 2* (char_lattice.width * horizontal_scaler * bitmap_y + bitmap_x - horizontal_pixel), (char*)pBitmap_pixel, sizeof(unsigned short));

	                        }
                        }
#endif                        
                    }
                }
            }
        }
        horizontal_pixel += (char_lattice.width * horizontal_scaler);
    }

    return 0;
}


int CAvEncoder::Ae_init_system_pts()
{
    return Ae_init_system_pts(1000ull * 1000ull * (time(NULL) - 3600 * 24 * 365 * 30));
}

int CAvEncoder::Ae_construct_audio_frame(av_stream_data_t *pAv_stream_data, int frame_header_chn, av_audio_encoder_para_t *pAudio_para, int have_audio, unsigned char *pFrame_header_buffer, int *pFrame_header_len, datetime_t& date_time)
{
    RMSTREAM_HEADER *pRm_frame_header = NULL;
    RMFI2_VTIME *pRm_vtime = NULL;
    RMFI2_AUDIOINFO *pRm_audio_info = NULL;

    *pFrame_header_len = 0;

    /*frame header*/
    pRm_frame_header = (RMSTREAM_HEADER *)pFrame_header_buffer;
    memset(pRm_frame_header, 0, sizeof(RMSTREAM_HEADER));
    pFrame_header_buffer[0] = frame_header_chn;
    pFrame_header_buffer[1] = '4';
    pFrame_header_buffer[2] = 'd';
    pFrame_header_buffer[3] = 'c';
    pRm_frame_header->lFrameLen = pAv_stream_data->frame_len;
    pRm_frame_header->lStreamExam = Ae_calculate_stream_CheckSum(pAv_stream_data);
    pRm_frame_header->lExtendLen = sizeof(RMFI2_VTIME) + sizeof(RMFI2_AUDIOINFO);
    pRm_frame_header->lExtendCount = 2;
    *pFrame_header_len += sizeof(RMSTREAM_HEADER);

    /*extend data:RMFI2_VTIME*/
    pRm_vtime = (RMFI2_VTIME *)(pFrame_header_buffer + *pFrame_header_len);
    pRm_vtime->stuInfoType.lInfoType = RMS_INFOTYPE_VIRTUALTIME;
    pRm_vtime->stuInfoType.lInfoLength = sizeof(RMFI2_VTIME);
    pRm_vtime->llVirtualTime = pAv_stream_data->pts / 1000;
    *pFrame_header_len += sizeof(RMFI2_VTIME);

    /*extend data:RMFI2_AUDIOINFO*/
    pRm_audio_info = (RMFI2_AUDIOINFO *)(pFrame_header_buffer + *pFrame_header_len);
    pRm_audio_info->stuInfoType.lInfoType = RMS_INFOTYPE_AUDIOINFO;
    pRm_audio_info->stuInfoType.lInfoLength = sizeof(RMFI2_AUDIOINFO);
    switch(pAudio_para->type)
    {
        case _AET_ADPCM_:
        default:
            pRm_audio_info->lPayloadType = 0;
            break;
        case _AET_G711A_:
            pRm_audio_info->lPayloadType = 2;
            break;
        case _AET_G711U_:
            pRm_audio_info->lPayloadType = 3;
            break;
        case _AET_G726_:
            pRm_audio_info->lPayloadType = 1;
            break;
        case _AET_AMR_:
            pRm_audio_info->lPayloadType = 4;
            break;
		case _AET_AAC_:
			pRm_audio_info->lPayloadType = 5;
			break;
		case _AET_LPCM_:	//current only used for plane_project///
			pRm_audio_info->lPayloadType = 6;
            break;
    }    
    pRm_audio_info->lSoundMode = 0;
    if(have_audio != 0)
    {
        pRm_audio_info->lPlayAudio = 1;
    }
    else
    {
        pRm_audio_info->lPlayAudio = 0;
    }
    pRm_audio_info->lBitWidth = 16;
    pRm_audio_info->lSampleRate = 8000;
    if(PTD_P1 == pgAvDeviceInfo->Adi_product_type())
    {
		pRm_audio_info->lSampleRate = 44100;
    }

    *pFrame_header_len += sizeof(RMFI2_AUDIOINFO);
    
#if defined(_AV_HAVE_PURE_AUDIO_CHANNEL)	//纯音频通道的音频数据，存储时共享缓存帧头类型改成I帧，并在扩展数据中加上时间///
	if(frame_header_chn >= 2 && frame_header_chn < 8)
	{
		pRm_frame_header->lExtendLen += sizeof(RMFI2_RTCTIME);
		RMFI2_RTCTIME* pRm_rtc_time = (RMFI2_RTCTIME*)(pFrame_header_buffer + *pFrame_header_len);
		//datetime_t date_time;
		//pgAvDeviceInfo->Adi_get_date_time(&date_time);
		pRm_rtc_time->stuInfoType.lInfoType = RMS_INFOTYPE_RTCTIME;
		pRm_rtc_time->stuInfoType.lInfoLength = sizeof(RMFI2_RTCTIME);
		pRm_rtc_time->stuRtcTime.cYear = date_time.year;
		pRm_rtc_time->stuRtcTime.cMonth = date_time.month;
		pRm_rtc_time->stuRtcTime.cDay = date_time.day;
		pRm_rtc_time->stuRtcTime.cHour = date_time.hour;
		pRm_rtc_time->stuRtcTime.cMinute = date_time.minute;
		pRm_rtc_time->stuRtcTime.cSecond = date_time.second;
		pRm_rtc_time->stuRtcTime.usMilliSecond = 0;
		pRm_rtc_time->stuRtcTime.usMilliValidate = 0;
		pRm_rtc_time->stuRtcTime.usWeek = date_time.week;
		pRm_rtc_time->stuRtcTime.usReserved = 0;
		
		pRm_frame_header->lExtendCount += 1;
		*pFrame_header_len += sizeof(RMFI2_RTCTIME);
		//printf("Ae_construct_audio_frame, chn = %d, headLen = %d, pts = %llu, time = %d-%d-%d  %d:%d:%d\n", frame_header_chn,
		//	*pFrame_header_len, pRm_vtime->llVirtualTime, date_time.year, date_time.month, date_time.day, date_time.hour, date_time.minute, date_time.second);
	}
#endif

    return 0;
}

int CAvEncoder::Ae_put_audio_stream(av_audio_stream_t *pAudio_stream, av_stream_data_t *pAv_stream_data, SShareMemData &share_mem_data)
{
    _AV_ASSERT_((pAudio_stream->type == _AAST_TALKBACK_) && (pAv_stream_data->frame_type == _AFT_AUDIO_FRAME_), "CAvEncoder::Ae_put_audio_stream INVALID PARAMETER(%d)(%d)\n", pAudio_stream->type, pAv_stream_data->frame_type);
    SShareMemFrameInfo share_mem_frame_info;
    unsigned int u32DataLen = 0;

    /*frame header*/
    datetime_t date_time;
    memset(&date_time, 0x0, sizeof(datetime_t));
    share_mem_data.iPackCount = 1;
    Ae_construct_audio_frame(pAv_stream_data, 0, &pAudio_stream->audio_encoder_para, 1, pAudio_stream->pFrame_header_buffer, (int *)&share_mem_data.pstuPack[0].iLen, date_time);
    share_mem_data.pstuPack[0].pAddr = (char *)pAudio_stream->pFrame_header_buffer;
    u32DataLen += share_mem_data.pstuPack[0].iLen;

    /*data*/
    for(int index = 0; index < pAv_stream_data->pack_number; index ++)
    {
        share_mem_data.pstuPack[share_mem_data.iPackCount].pAddr = (char *)pAv_stream_data->pack_list[index].pAddr[0];
        share_mem_data.pstuPack[share_mem_data.iPackCount].iLen = pAv_stream_data->pack_list[index].len[0];
        share_mem_data.iPackCount ++;
        u32DataLen += pAv_stream_data->pack_list[index].len[0];

        if(pAv_stream_data->pack_list[index].len[1] != 0)
        {
            share_mem_data.pstuPack[share_mem_data.iPackCount].pAddr = (char *)pAv_stream_data->pack_list[index].pAddr[1];
            share_mem_data.pstuPack[share_mem_data.iPackCount].iLen = pAv_stream_data->pack_list[index].len[1];
            share_mem_data.iPackCount ++;
            u32DataLen += pAv_stream_data->pack_list[index].len[1];
        }        
    }

    share_mem_frame_info.year = 0;
    share_mem_frame_info.month = 0;
    share_mem_frame_info.day = 0;
    share_mem_frame_info.hour = 0;
    share_mem_frame_info.minute = 0;
    share_mem_frame_info.second = 0;
    share_mem_frame_info.flag = SHAREMEM_FLAG_AFRAME;
    share_mem_frame_info.cut = 0;

    N9M_SHPutOneFrame(pAudio_stream->stream_buffer_handle, &share_mem_data, &share_mem_frame_info, u32DataLen);

    return 0;
}

int CAvEncoder::Ae_put_video_stream(std::list<av_video_stream_t>::iterator &av_video_stream, av_audio_stream_t *pAudio_stream, av_stream_data_t *pAv_stream_data, SShareMemData &share_mem_data)
{
    SShareMemFrameInfo share_mem_frame_info;
    RMSTREAM_HEADER *pRm_frame_header = NULL;
    RMFI2_VTIME *pRm_vtime = NULL;
    RMFI2_VIDEOINFO *pRm_video_info = NULL;
    RMFI2_RTCTIME *pRtc_time = NULL;
    RMFI2_VIDEOSTATE *pVideo_state = NULL;
    RMFI2_VIDEOREFINFO *pVideo_ref_info = NULL;
    datetime_t date_time;
    int frame_header_len = 0;   
    unsigned int  u32DataLen = 0;
    uint8_t video_resolution = 0;

    unsigned int avagebitrate = 0;
    unsigned int totalframe = 0;
    unsigned int realbitrate = 0;
#if defined(_AV_HAVE_PURE_AUDIO_CHANNEL)
	static datetime_t s_date_time_bak[SUPPORT_MAX_VIDEO_CHANNEL_NUM] = {0};
	if((av_video_stream->first_frame == true) && (pAv_stream_data->frame_type != _AFT_IFRAME_) && (pAv_stream_data->frame_type != _AFT_AUDIO_FRAME_))
#else
    if((av_video_stream->first_frame == true) && (pAv_stream_data->frame_type != _AFT_IFRAME_))
#endif
    {
        return 0;
    }
    memset(&share_mem_frame_info, 0, sizeof(SShareMemFrameInfo));
#if defined(_AV_HAVE_PURE_AUDIO_CHANNEL)
    if((PTD_P1 == pgAvDeviceInfo->Adi_product_type()) \
    	&& (pAv_stream_data->frame_type == _AFT_IFRAME_ || pAv_stream_data->frame_type == _AFT_PFRAME_) \
    	&& (av_video_stream->phy_video_chn_num == 2 || av_video_stream->phy_video_chn_num == 3 || av_video_stream->phy_video_chn_num == 4 \
    	 || av_video_stream->phy_video_chn_num == 5 || av_video_stream->phy_video_chn_num == 6 || av_video_stream->phy_video_chn_num == 7))
    {	//P1机型2、 3、 4、 5、 6、 7通道只存储音频，视频丢掉///
    	return 0;
    }
#endif
    switch(pAv_stream_data->frame_type)
    {
        case _AFT_IFRAME_:
            pgAvDeviceInfo->Adi_get_date_time(&date_time);
            if((pgAvDeviceInfo->Adi_get_record_cut(av_video_stream->type, av_video_stream->phy_video_chn_num) == 1) \
                || (av_video_stream->first_frame == true))
            {
                share_mem_frame_info.cut = 1;
                av_video_stream->first_frame = false;
            }
            else
            {
                share_mem_frame_info.cut = 0;
            }
            share_mem_frame_info.flag = SHAREMEM_FLAG_IFRAME;
            av_video_stream->video_frame_id = 0;
            goto _CONSTRUCT_VIDEO_FRAME_;
            break;
        case _AFT_PFRAME_:
            share_mem_frame_info.cut = 0;
            share_mem_frame_info.flag = SHAREMEM_FLAG_PFRAME;
            av_video_stream->video_frame_id++;
            goto _CONSTRUCT_VIDEO_FRAME_;
            break;
        case _AFT_AUDIO_FRAME_:
			pgAvDeviceInfo->Adi_get_date_time(&date_time);
            Ae_construct_audio_frame(pAv_stream_data, av_video_stream->video_encoder_para.virtual_chn_num, &pAudio_stream->audio_encoder_para, av_video_stream->video_encoder_para.have_audio, av_video_stream->pFrame_header_buffer, &frame_header_len, date_time);
            share_mem_frame_info.cut = 0;
            share_mem_frame_info.flag = SHAREMEM_FLAG_AFRAME;
			
			/*if(av_video_stream->phy_video_chn_num == 3)
			{
				DEBUG_ERROR("Put data, chn = %d, type = %d, dataLen = %d, pts = %llu, time = (%d:%d:%d)\n", av_video_stream->phy_video_chn_num,
					av_video_stream->type, pAv_stream_data->frame_len, pAv_stream_data->pts, date_time.hour, date_time.minute, date_time.second);
			}*/

#if  defined(_AV_HAVE_PURE_AUDIO_CHANNEL)	//纯音频通道的音频数据，存储时共享缓存帧头类型改成I帧，并在扩展数据中加上时间///
			if(av_video_stream->video_encoder_para.virtual_chn_num >= 2 && av_video_stream->video_encoder_para.virtual_chn_num < 8)
			{
				if(s_date_time_bak[av_video_stream->video_encoder_para.virtual_chn_num].second != date_time.second || av_video_stream->first_frame)	//每秒放一个I帧，其它时候放A帧///
				{
					memcpy(&(s_date_time_bak[av_video_stream->video_encoder_para.virtual_chn_num]), &date_time, sizeof(datetime_t));

					share_mem_frame_info.flag = SHAREMEM_FLAG_IFRAME;
					share_mem_frame_info.standard = (av_video_stream->video_encoder_para.tv_system == _AT_PAL_)?0:1;
				}
				else
				{
					share_mem_frame_info.flag = SHAREMEM_FLAG_AFRAME;
				}
				
				//2016-6-8 delete
				if((pgAvDeviceInfo->Adi_get_record_cut(av_video_stream->type, av_video_stream->phy_video_chn_num) == 1) || av_video_stream->first_frame)
				{
					av_video_stream->first_frame = false;
					share_mem_frame_info.cut = 1;
				}
				else
				{
					share_mem_frame_info.cut = 0;
				}
				
				/*SShareMemFrameInfo*/
				share_mem_frame_info.year = date_time.year;
				share_mem_frame_info.month = date_time.month;
				share_mem_frame_info.day = date_time.day;
				share_mem_frame_info.hour = date_time.hour;
				share_mem_frame_info.minute = date_time.minute;
				share_mem_frame_info.second = date_time.second;
			}
#endif
            goto _PUT_DATA_;
            break;
        default:
            _AV_FATAL_(1, "CAvEncoder::Ae_put_video_stream You must give the implement(%d)\n", pAv_stream_data->frame_type);
            break;
    }
    
_CONSTRUCT_VIDEO_FRAME_:
    /*RMSTREAM_HEADER*/
    pRm_frame_header = (RMSTREAM_HEADER *)av_video_stream->pFrame_header_buffer;
    av_video_stream->pFrame_header_buffer[0] = av_video_stream->video_encoder_para.virtual_chn_num;
    av_video_stream->pFrame_header_buffer[2] = 'd';
    av_video_stream->pFrame_header_buffer[3] = 'c';
    if(pAv_stream_data->frame_type == _AFT_IFRAME_)
    {
#if defined(_AV_PRODUCT_CLASS_IPC_)
        switch(av_video_stream->video_encoder_para.video_type)
        {
            case _AVT_H265_:
                av_video_stream->pFrame_header_buffer[1] = '5';
                break;
            case _AVT_H264_:
            default:
                av_video_stream->pFrame_header_buffer[1] = '2';
                break;
        }
#else
        av_video_stream->pFrame_header_buffer[1] = '2';
#endif
        pRm_frame_header->lExtendCount = 5;
        pRm_frame_header->lExtendLen = sizeof(RMFI2_VTIME) + sizeof(RMFI2_VIDEOINFO) + sizeof(RMFI2_RTCTIME) + sizeof(RMFI2_VIDEOSTATE) + sizeof(RMFI2_VIDEOREFINFO);
        if (pgAvDeviceInfo->Adi_get_watermark_switch())
        {
            pRm_frame_header->lExtendCount += 1;
            pRm_frame_header->lExtendLen += sizeof(RMFI2_H264INFO);
        }
    }
    else if(pAv_stream_data->frame_type == _AFT_PFRAME_)
    {
#if defined(_AV_PRODUCT_CLASS_IPC_)
        switch(av_video_stream->video_encoder_para.video_type)
        {
            case _AVT_H265_:
                av_video_stream->pFrame_header_buffer[1] = '6';
                break;
            case _AVT_H264_:
            default:
                av_video_stream->pFrame_header_buffer[1] = '3';
                break;
        }
#else
        av_video_stream->pFrame_header_buffer[1] = '3';
#endif
        pRm_frame_header->lExtendCount = 4;
        pRm_frame_header->lExtendLen = sizeof(RMFI2_VTIME) + sizeof(RMFI2_VIDEOINFO) + sizeof(RMFI2_VIDEOSTATE) + sizeof(RMFI2_VIDEOREFINFO);
        if (pgAvDeviceInfo->Adi_get_watermark_switch())
        {
            pRm_frame_header->lExtendCount += 1;
            pRm_frame_header->lExtendLen += sizeof(RMFI2_H264INFO);
        }
    }
    else
    {
        _AV_FATAL_(1, "CAvEncoder::Ae_put_video_stream FATAL\n");
    }
    pRm_frame_header->lFrameLen = pAv_stream_data->frame_len;
    pRm_frame_header->lStreamExam = Ae_calculate_stream_CheckSum(pAv_stream_data);
    frame_header_len += sizeof(RMSTREAM_HEADER);

    /*extend data:RMFI2_VTIME*/
    pRm_vtime = (RMFI2_VTIME *)(av_video_stream->pFrame_header_buffer + frame_header_len);
    pRm_vtime->stuInfoType.lInfoType = RMS_INFOTYPE_VIRTUALTIME;
    pRm_vtime->stuInfoType.lInfoLength = sizeof(RMFI2_VTIME);
    pRm_vtime->llVirtualTime = pAv_stream_data->pts / 1000;
    frame_header_len += sizeof(RMFI2_VTIME);
    /*extend data:RMFI2_VIDEOINFO*/
    pRm_video_info = (RMFI2_VIDEOINFO *)(av_video_stream->pFrame_header_buffer + frame_header_len);
    pRm_video_info->stuInfoType.lInfoType = RMS_INFOTYPE_VIDEOINFO;
    pRm_video_info->stuInfoType.lInfoLength = sizeof(RMFI2_VIDEOINFO);
    if(av_video_stream->video_encoder_para.resolution != _AR_SIZE_)
    {
        int width = 0, height = 0;
        pgAvDeviceInfo->Adi_get_video_size(av_video_stream->video_encoder_para.resolution, av_video_stream->video_encoder_para.tv_system, &width, &height);
#if defined(_AV_PRODUCT_CLASS_IPC_)
        pgAvDeviceInfo->Adi_covert_vidoe_size_by_rotate( &width, &height);
#endif
        pRm_video_info->lWidth = width;
        pRm_video_info->lHeight = height;
    }
    else
    {
#if defined(_AV_PRODUCT_CLASS_IPC_)
        pgAvDeviceInfo->Adi_covert_vidoe_size_by_rotate( &av_video_stream->video_encoder_para.resolution_w, &av_video_stream->video_encoder_para.resolution_h);
#endif
        pRm_video_info->lWidth = av_video_stream->video_encoder_para.resolution_w;
        pRm_video_info->lHeight = av_video_stream->video_encoder_para.resolution_h;
    }
    pRm_video_info->lFPS = av_video_stream->video_encoder_para.frame_rate;
    frame_header_len += sizeof(RMFI2_VIDEOINFO);

    /*extend data:RMFI2_RTCTIME*/
    if(pAv_stream_data->frame_type == _AFT_IFRAME_)
    {
        pRtc_time = (RMFI2_RTCTIME *)(av_video_stream->pFrame_header_buffer + frame_header_len);
        pRtc_time->stuInfoType.lInfoType = RMS_INFOTYPE_RTCTIME;
        pRtc_time->stuInfoType.lInfoLength = sizeof(RMFI2_RTCTIME);
        pRtc_time->stuRtcTime.cYear = date_time.year;
        pRtc_time->stuRtcTime.cMonth = date_time.month;
        pRtc_time->stuRtcTime.cDay = date_time.day;
        pRtc_time->stuRtcTime.cHour = date_time.hour;
        pRtc_time->stuRtcTime.cMinute = date_time.minute;
        pRtc_time->stuRtcTime.cSecond = date_time.second;
        pRtc_time->stuRtcTime.usMilliSecond = 0;
        pRtc_time->stuRtcTime.usMilliValidate = 0;
        pRtc_time->stuRtcTime.usWeek = date_time.week;
        pRtc_time->stuRtcTime.usReserved = 0;
        frame_header_len += sizeof(RMFI2_RTCTIME);
        /*SShareMemFrameInfo*/
        share_mem_frame_info.year = date_time.year;
        share_mem_frame_info.month = date_time.month;
        share_mem_frame_info.day = date_time.day;
        share_mem_frame_info.hour = date_time.hour;
        share_mem_frame_info.minute = date_time.minute;
        share_mem_frame_info.second = date_time.second;
    }

    pVideo_state = (RMFI2_VIDEOSTATE *)(av_video_stream->pFrame_header_buffer + frame_header_len);
    pVideo_state->stuInfoType.lInfoType = RMS_INFOTYPE_VIDEOSTATE;
    pVideo_state->stuInfoType.lInfoLength = sizeof(RMFI2_VIDEOSTATE);
    pVideo_state->lReserve = 0;
    pVideo_state->lVideoLost = Ae_get_video_lost_flag(av_video_stream->phy_video_chn_num);
    pVideo_state->lRefType = av_video_stream->video_encoder_para.ref_para.ref_type;
    frame_header_len += sizeof(RMFI2_VIDEOSTATE);

#if defined(_AV_PRODUCT_CLASS_IPC_)
    switch(av_video_stream->video_encoder_para.video_type)
    {
            case _AVT_H264_:
            default:
                RMFI2_H264INFO stuH264Info;
                Ae_get_H264_info(av_video_stream->video_encoder_id.pVideo_encoder_info, &stuH264Info);

                pVideo_ref_info = (RMFI2_VIDEOREFINFO*)(av_video_stream->pFrame_header_buffer + frame_header_len);
                pVideo_ref_info->stuInfoType.lInfoType = RMS_INFOTYPE_VIDEOREF;
                pVideo_ref_info->stuInfoType.lInfoLength = sizeof(RMFI2_VIDEOREFINFO);
                pVideo_ref_info->lEnablePred = av_video_stream->video_encoder_para.ref_para.pred_enable;
                pVideo_ref_info->lRefBaseInterval = av_video_stream->video_encoder_para.ref_para.base_interval;
                pVideo_ref_info->lRefEhanceInterval = av_video_stream->video_encoder_para.ref_para.enhance_interval;
                pVideo_ref_info->lFrameId = av_video_stream->video_frame_id;
                pVideo_ref_info->lRefSliceType = stuH264Info.lRefSliceType;
                pVideo_ref_info->lRefType = stuH264Info.lRefType;
                pVideo_ref_info->lReserve = 0;
                break;
#if defined(_AV_PLATFORM_HI3516A_)
            case _AVT_H265_:
            {
                RMFI2_H265INFO stuH265Info;
                Ae_get_H265_info(av_video_stream->video_encoder_id.pVideo_encoder_info, &stuH265Info);

                pVideo_ref_info = (RMFI2_VIDEOREFINFO*)(av_video_stream->pFrame_header_buffer + frame_header_len);
                pVideo_ref_info->stuInfoType.lInfoType = RMS_INFOTYPE_VIDEOREF;
                pVideo_ref_info->stuInfoType.lInfoLength = sizeof(RMFI2_VIDEOREFINFO);
                pVideo_ref_info->lEnablePred = av_video_stream->video_encoder_para.ref_para.pred_enable;
                pVideo_ref_info->lRefBaseInterval = av_video_stream->video_encoder_para.ref_para.base_interval;
                pVideo_ref_info->lRefEhanceInterval = av_video_stream->video_encoder_para.ref_para.enhance_interval;
                pVideo_ref_info->lFrameId = av_video_stream->video_frame_id;
                pVideo_ref_info->lRefSliceType = 0;//H265 has no this filed,so we set it to zero.
                pVideo_ref_info->lRefType = stuH265Info.lRefType;
                pVideo_ref_info->lReserve = 0;    
            }
                break;
#endif
    }

#else
    RMFI2_H264INFO stuH264Info;
    Ae_get_H264_info(av_video_stream->video_encoder_id.pVideo_encoder_info, &stuH264Info);

    pVideo_ref_info = (RMFI2_VIDEOREFINFO*)(av_video_stream->pFrame_header_buffer + frame_header_len);
    pVideo_ref_info->stuInfoType.lInfoType = RMS_INFOTYPE_VIDEOREF;
    pVideo_ref_info->stuInfoType.lInfoLength = sizeof(RMFI2_VIDEOREFINFO);
    pVideo_ref_info->lEnablePred = av_video_stream->video_encoder_para.ref_para.pred_enable;
    pVideo_ref_info->lRefBaseInterval = av_video_stream->video_encoder_para.ref_para.base_interval;
    pVideo_ref_info->lRefEhanceInterval = av_video_stream->video_encoder_para.ref_para.enhance_interval;
    pVideo_ref_info->lFrameId = av_video_stream->video_frame_id;
    pVideo_ref_info->lRefSliceType = stuH264Info.lRefSliceType;
    pVideo_ref_info->lRefType = stuH264Info.lRefType;
    pVideo_ref_info->lReserve = 0;
#endif
    
    frame_header_len += sizeof(RMFI2_VIDEOREFINFO);
    
    share_mem_frame_info.refType = pVideo_ref_info->lRefType;
    share_mem_frame_info.frCount = pVideo_ref_info->lFrameId;
    /*extend data H264 info*/
#if !defined(_AV_PRODUCT_CLASS_IPC_)
    if (pgAvDeviceInfo->Adi_get_watermark_switch())
    {
        RMFI2_H264INFO *pH264_info = (RMFI2_H264INFO *)(av_video_stream->pFrame_header_buffer + frame_header_len);
        memcpy(pH264_info, &stuH264Info, sizeof(RMFI2_H264INFO));
        //Ae_get_H264_info(av_video_stream->video_encoder_id.pVideo_encoder_info, pH264_info);
        pH264_info->stuInfoType.lInfoType = RMS_INFOTYPE_H264INFO;
        pH264_info->stuInfoType.lInfoLength = sizeof(RMFI2_H264INFO);
        frame_header_len += sizeof(RMFI2_H264INFO);     
    }
#endif

    /*SShareMemFrameInfo*/
    share_mem_frame_info.standard = (av_video_stream->video_encoder_para.tv_system == _AT_PAL_)?0:1;

    if (pgAvDeviceInfo->Adi_get_video_resolution(&video_resolution, &av_video_stream->video_encoder_para.resolution, false) < 0)
    {     
        _AV_FATAL_(1, "CAvEncoder::Ae_put_video_stream You must give the implement\n");
    }
    share_mem_frame_info.resolution = video_resolution;

    goto _PUT_DATA_;

_PUT_DATA_:
    /*frame header*/
    share_mem_data.pstuPack[0].pAddr = (char *)av_video_stream->pFrame_header_buffer;
    share_mem_data.pstuPack[0].iLen = frame_header_len;
    share_mem_data.iPackCount = 1;
    u32DataLen += frame_header_len;

    /*frame data*/
    
    for(int index = 0; index < pAv_stream_data->pack_number; index ++)
    {
#ifdef AAC_6AI_3515_TEST
		//printf("11111111111111, type = %d, chn = %d\n", pAv_stream_data->frame_type == _AFT_AUDIO_FRAME_, av_video_stream->phy_video_chn_num);
		if(g_pAudioFile != NULL && pAv_stream_data->frame_type == _AFT_AUDIO_FRAME_ && av_video_stream->phy_video_chn_num == 1 && av_video_stream->type == _AST_MAIN_VIDEO_)
		{	//测试，写第2路纯音频通道的数据//
			int ret = fwrite(pAv_stream_data->pack_list[index].pAddr[0], 1, pAv_stream_data->pack_list[index].len[0], g_pAudioFile);
			if(pAv_stream_data->pack_list[index].len[1] != 0)
			{
				int ret = fwrite(pAv_stream_data->pack_list[index].pAddr[1], 1, pAv_stream_data->pack_list[index].len[1], g_pAudioFile);
			}
			DEBUG_DEBUG("Write audio data, chn = %d, index = %d, len = (%d--%d), type = %d, ret = %d\n", av_video_stream->phy_video_chn_num, index, pAv_stream_data->pack_list[index].len[0], \
				pAv_stream_data->pack_list[index].len[1], av_video_stream->type, ret);
		}
#endif
#if 0
		static FILE* g_video_file = fopen("video.h264", "w+");
		if(g_video_file != NULL && av_video_stream->phy_video_chn_num == 2 && av_video_stream->type == _AST_MAIN_VIDEO_ && (pAv_stream_data->frame_type == _AFT_IFRAME_ || pAv_stream_data->frame_type == _AFT_PFRAME_))
		{	//测试，写第2路纯音频通道的数据//
			int ret = fwrite(pAv_stream_data->pack_list[index].pAddr[0], 1, pAv_stream_data->pack_list[index].len[0], g_video_file);
			if(pAv_stream_data->pack_list[index].len[1] != 0)
			{
				int ret = fwrite(pAv_stream_data->pack_list[index].pAddr[1], 1, pAv_stream_data->pack_list[index].len[1], g_video_file);
			}
			DEBUG_DEBUG("Write video data, chn = %d, index = %d, len = (%d--%d), type = %d, frame = %d, ret = %d\n", av_video_stream->phy_video_chn_num, index, pAv_stream_data->pack_list[index].len[0], \
				pAv_stream_data->pack_list[index].len[1], av_video_stream->type, pAv_stream_data->frame_type, ret);
		}
#endif
		/*if(av_video_stream->phy_video_chn_num == 0 && av_video_stream->type == _AST_MAIN_VIDEO_ && (pAv_stream_data->frame_type == _AFT_IFRAME_ || pAv_stream_data->frame_type == _AFT_PFRAME_) && pRm_frame_header != NULL)
		{
			DEBUG_CRITICAL("Put data, chn = %d, index = %d, type = %d, dataLen = %d, pts = %llu, time = (%d:%d:%d), len = %d--%d, count = %d, frameLen = %d, extendLen = %d\n", av_video_stream->phy_video_chn_num, index,
				pAv_stream_data->frame_type, pAv_stream_data->frame_len, pAv_stream_data->pts, date_time.hour, date_time.minute, date_time.second, pAv_stream_data->pack_list[index].len[0], pAv_stream_data->pack_list[index].len[1],
				pRm_frame_header->lExtendCount, pRm_frame_header->lFrameLen, pRm_frame_header->lExtendLen);
		}*/
		//printf("55555555, index = %d, len = %d, type = %d\n", index, pAv_stream_data->pack_list[index].len[0], av_video_stream->type);
        share_mem_data.pstuPack[share_mem_data.iPackCount].pAddr = (char *)pAv_stream_data->pack_list[index].pAddr[0];
        share_mem_data.pstuPack[share_mem_data.iPackCount].iLen = pAv_stream_data->pack_list[index].len[0];
        share_mem_data.iPackCount ++;
        u32DataLen += pAv_stream_data->pack_list[index].len[0];
        if(av_video_stream->type == _AST_MAIN_VIDEO_ || av_video_stream->type == _AST_SUB_VIDEO_)
        {
            m_code_rate_count[av_video_stream->phy_video_chn_num][av_video_stream->type].currentrate += pAv_stream_data->pack_list[index].len[0];
        }

        if(pAv_stream_data->pack_list[index].len[1] != 0)
        {
            share_mem_data.pstuPack[share_mem_data.iPackCount].pAddr = (char *)pAv_stream_data->pack_list[index].pAddr[1];
            share_mem_data.pstuPack[share_mem_data.iPackCount].iLen = pAv_stream_data->pack_list[index].len[1];
            share_mem_data.iPackCount ++;
            u32DataLen += pAv_stream_data->pack_list[index].len[1];
            if(av_video_stream->type == _AST_MAIN_VIDEO_ || av_video_stream->type == _AST_SUB_VIDEO_)
            {
              m_code_rate_count[av_video_stream->phy_video_chn_num][av_video_stream->type].currentrate += pAv_stream_data->pack_list[index].len[1];
            }
        }
    }
#if 0//defined (_AV_PLATFORM_HI3515_)		//为了适应6a，将字节对齐放在回放的地方///
    int align_num = 4;
    int left = u32DataLen % align_num;
    if ( left !=0)
    {
        int add_len = align_num - left;
//        char * add_conten = new char[add_len];
	 char add_conten[add_len];
        memset(add_conten, 0, add_len);
        
        share_mem_data.pstuPack[share_mem_data.iPackCount].pAddr = (char *)add_conten;
        share_mem_data.pstuPack[share_mem_data.iPackCount].iLen = add_len;
        share_mem_data.iPackCount ++;
        u32DataLen += add_len;

        if(pgAvDeviceInfo->Adi_product_type() == PTD_6A_I)	//6A一代帧长度修改//
        {
			pRm_frame_header = (RMSTREAM_HEADER *)av_video_stream->pFrame_header_buffer;
			pRm_frame_header->lFrameLen += add_len;
        }
        
        if(av_video_stream->type == _AST_MAIN_VIDEO_ || av_video_stream->type == _AST_SUB_VIDEO_)
        {
          m_code_rate_count[av_video_stream->phy_video_chn_num][av_video_stream->type].currentrate += add_len;
        }
    }
#endif
#if 0
    if(av_video_stream->type == _AST_MAIN_VIDEO_)
    {
        static FILE* fp =NULL;
        if(NULL == fp)
        {
            fp = fopen("./stream/stream.h265", "wb");
        }

        if(NULL != fp)
        {
            for(unsigned int i=0;i< share_mem_data.iPackCount; ++i)
            {
                fwrite(share_mem_data.pstuPack[i].pAddr, share_mem_data.pstuPack[i].iLen, 1, fp);
                fflush(fp);
            }
        }
    }
#endif
    if(av_video_stream->type == _AST_MAIN_VIDEO_ || av_video_stream->type == _AST_SUB_VIDEO_)
    {
        if(pAv_stream_data->frame_type == _AFT_IFRAME_)
        {
            m_code_rate_count[av_video_stream->phy_video_chn_num][av_video_stream->type].beforesecondrate = m_code_rate_count[av_video_stream->phy_video_chn_num][av_video_stream->type].currentrate;
            m_code_rate_count[av_video_stream->phy_video_chn_num][av_video_stream->type].currentrate = 0;
        }
    }

    if((av_video_stream->type == _AST_SUB2_VIDEO_) && (av_video_stream->video_encoder_para.bitrate_mode == _ABM_VBR_))
    {
        if(share_mem_frame_info.flag == 1)
        {
            if(av_video_stream->count_total_frame != 0)
            {
                avagebitrate = av_video_stream->count_total_rate / av_video_stream->count_total_frame * av_video_stream->video_encoder_para.frame_rate;
                if(av_video_stream->video_encoder_para.bitrate_level == 1)
                {
                    realbitrate = av_video_stream->video_encoder_para.bitrate.hivbr.bitrate * 100 / 85;
                }
                else if(av_video_stream->video_encoder_para.bitrate_level == 2)
                {
                    realbitrate = av_video_stream->video_encoder_para.bitrate.hivbr.bitrate * 100 / 70;
                }
                else
                {
                    realbitrate = av_video_stream->video_encoder_para.bitrate.hivbr.bitrate;
                }

                if(((avagebitrate * 8) / 1024) > realbitrate)
                {
                    av_video_stream->high_count_number_percent++;
                }
                else
                {
                    int percent = ((avagebitrate * 8) / 1024) * 100 / realbitrate;
                    if(percent > 80)
                    {
                        av_video_stream->high_count_number_percent++;
                    }
                    else if(percent > 50)
                    {
                        av_video_stream->middle_count_number_percent++;
                    }
                    else
                    {
                        av_video_stream->low_count_number_percent++;
                    }
                }

                //printf("\n",av_video_stream->high_count_number_percent,av_video_stream->middle_count_number_percent,av_video_stream->low_count_number_percent,totalframe);
                totalframe = av_video_stream->high_count_number_percent + av_video_stream->middle_count_number_percent + av_video_stream->low_count_number_percent;

                //printf("chn:%d avagebitrate1:%d vbr.bitrate:%d level:%d high:%d middle:%d low:%d total:%d\n",av_video_stream->phy_video_chn_num,(avagebitrate * 8) / 1024,realbitrate,av_video_stream->vbr_wave_level,av_video_stream->high_count_number_percent,av_video_stream->middle_count_number_percent,av_video_stream->low_count_number_percent,totalframe);
                if(totalframe * av_video_stream->video_encoder_para.gop_size / av_video_stream->video_encoder_para.frame_rate >= _COUNT_VBR_RATE_TIME_)
                {

                    if(av_video_stream->high_count_number_percent * 100 / totalframe > 80)
                    {
                        av_video_stream->vbr_wave_level = 0;
                    }
                    if(av_video_stream->middle_count_number_percent * 100 / totalframe > 80)
                    {
                        av_video_stream->vbr_wave_level = 1;
                    }
                    if(av_video_stream->low_count_number_percent * 100 / totalframe > 80)
                    {
                        av_video_stream->vbr_wave_level = 2;
                    }
                    if(av_video_stream->phy_video_chn_num == 0)
                    {
                        printf("chn 0 cont levle..............:%d\n",av_video_stream->vbr_wave_level);
                    }
                    av_video_stream->high_count_number_percent = 0;
                    av_video_stream->middle_count_number_percent = 0;
                    av_video_stream->low_count_number_percent = 0;
                }

                av_video_stream->count_total_rate = u32DataLen;
                av_video_stream->count_total_frame = 1;
            }
        }
        else if(share_mem_frame_info.flag == 0)
        {
            av_video_stream->count_total_rate += u32DataLen;
            av_video_stream->count_total_frame++;
        }
    }
    /*put data*/
    N9M_SHPutOneFrame(av_video_stream->stream_buffer_handle, &share_mem_data, &share_mem_frame_info, u32DataLen);
        
    //add by hswang add i frame reocrd function
    if((av_video_stream->type == _AST_MAIN_VIDEO_) && (pAv_stream_data->frame_type == _AFT_IFRAME_))
    {
         if(av_video_stream->iframe_stream_buffer_handle != NULL)
         {
            pRm_video_info->lFPS = 1;
            N9M_SHPutOneFrame(av_video_stream->iframe_stream_buffer_handle, &share_mem_data, &share_mem_frame_info, u32DataLen);
         }    
    }
    
    return 0;
}

unsigned char CAvEncoder::Ae_calculate_stream_CheckSum(av_stream_data_t *pAv_stream_data)
{
    unsigned char check_sum = 0;
    int check_interval = 0;
    int check_count = 0;
    int check_index = 0;
    int pack_index = 0;
    int pack_start_position = 0;
    int pack_end_position = 0;
    int reserve_position = 0;
    int in_pack_index = 0;

    if(pAv_stream_data->frame_len > 128)
    {
        check_count = 128;
        check_interval = pAv_stream_data->frame_len / 128;
    }
    else
    {
        check_count = pAv_stream_data->frame_len;
        check_interval = 1;
    }

    pack_index = pAv_stream_data->pack_number - 1;
    pack_start_position = 0;
    if(pAv_stream_data->pack_list[pack_index].len[1] != 0)
    {
        pack_end_position = pAv_stream_data->pack_list[pack_index].len[1] - 1;
        in_pack_index = 1;
    }
    else
    {
        pack_end_position = pAv_stream_data->pack_list[pack_index].len[0] - 1;
        in_pack_index = 0;
    }

    for(check_index = 0; check_index < check_count; check_index ++)
    {
        reserve_position = check_index * check_interval;
        while(1)
        {
            /*[pack_start_position, pack_end_position]*/
            if((reserve_position >= pack_start_position) && (reserve_position <= pack_end_position))
            {
                check_sum += *(pAv_stream_data->pack_list[pack_index].pAddr[in_pack_index] + pAv_stream_data->pack_list[pack_index].len[in_pack_index] - (reserve_position - pack_start_position) - 1);
                break;
            }

            pack_start_position = pack_end_position + 1;
            if(in_pack_index == 0)
            {
                pack_index --;
                if(pAv_stream_data->pack_list[pack_index].len[1] != 0)
                {
                    in_pack_index = 1;
                }
                else
                {
                    in_pack_index = 0;
                }
            }
            else
            {
                in_pack_index = 0;
            }
            pack_end_position = pack_start_position + pAv_stream_data->pack_list[pack_index].len[in_pack_index] - 1;
        }
    }

    return check_sum;
}

int CAvEncoder::Ae_get_encoder_used_resource(int *pCif_count)
{   
    int cif_count = 0;
    
    for(int task_id = 0; task_id < _AE_ENCODER_MAX_RECV_THREAD_NUMBER_; task_id ++)
    {
        av_recv_thread_task_t *pAv_thread_task = m_pRecv_thread_task + task_id;
        std::list<av_audio_video_group_t>::iterator local_av_group_it;   
        std::list<av_video_stream_t>::iterator video_stream_it;
        if (pAv_thread_task == NULL)
        {
            continue;
        }
        local_av_group_it = pAv_thread_task->local_task_list.begin();
        while(local_av_group_it != pAv_thread_task->local_task_list.end())/*task list*/
        {
            /*video stream*/
            video_stream_it = local_av_group_it->video_stream.begin();
            while(video_stream_it != local_av_group_it->video_stream.end())
            {
                int factor = 1;
                pgAvDeviceInfo->Adi_get_factor_by_cif(video_stream_it->video_encoder_para.resolution, factor);
                cif_count += (video_stream_it->video_encoder_para.frame_rate*factor);
                video_stream_it ++;
            }
            local_av_group_it ++;
        }
    }

    if (pCif_count)
    {
        *pCif_count = cif_count;
    }
            
    return 0;
}

bool CAvEncoder::Ae_whether_encoder_have_this_resolution( av_resolution_t eResolution, int *pResolutionNum)
{
    bool bResExist = false;
    for(int task_id = 0; task_id < _AE_ENCODER_MAX_RECV_THREAD_NUMBER_; task_id ++)
    {
        av_recv_thread_task_t *pAv_thread_task = m_pRecv_thread_task + task_id;
        std::list<av_audio_video_group_t>::iterator local_av_group_it;   
        std::list<av_video_stream_t>::iterator video_stream_it;
        if( pAv_thread_task == NULL )
        {
            continue;
        }
        local_av_group_it = pAv_thread_task->local_task_list.begin();
        while(local_av_group_it != pAv_thread_task->local_task_list.end())/*task list*/
        {
            /*video stream*/
            video_stream_it = local_av_group_it->video_stream.begin();
            while(video_stream_it != local_av_group_it->video_stream.end())
            {
                if( video_stream_it->video_encoder_para.resolution == eResolution )
                {
                    bResExist = true;
                    if(NULL != pResolutionNum && _AST_MAIN_VIDEO_ == video_stream_it->type)
                    {
                        (*pResolutionNum)++;
                    }
                }

                video_stream_it ++;
            }
            local_av_group_it ++;
        }
    }
   
    return bResExist;
}
int CAvEncoder::Ae_get_net_stream_level(unsigned int *chnmask,unsigned char chnvalue[SUPPORT_MAX_VIDEO_CHANNEL_NUM])
{
    for(int task_id = 0; task_id < _AE_ENCODER_MAX_RECV_THREAD_NUMBER_; task_id ++)
    {
        av_recv_thread_task_t *pAv_thread_task = m_pRecv_thread_task + task_id;
        std::list<av_audio_video_group_t>::iterator local_av_group_it;   
        std::list<av_video_stream_t>::iterator video_stream_it;
        if( pAv_thread_task == NULL )
        {
            continue;
        }
        local_av_group_it = pAv_thread_task->local_task_list.begin();
        while(local_av_group_it != pAv_thread_task->local_task_list.end())/*task list*/
        {
            /*video stream*/
            video_stream_it = local_av_group_it->video_stream.begin();
            while(video_stream_it != local_av_group_it->video_stream.end())
            {
            	*chnmask = *chnmask | (0x1 << video_stream_it->phy_video_chn_num);
                chnvalue[video_stream_it->phy_video_chn_num] = video_stream_it->vbr_wave_level;
                video_stream_it ++;
            }
            local_av_group_it ++;
        }
    }
    return 0;
}
int CAvEncoder::Ae_start_talkback_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, av_audio_encoder_para_t *pAudio_para, bool thread_control)
{
    int recv_task_id = -1;
    int send_task_id = -1;
    int ret_val = -1;
    
    /*check parameter valid*/
    if((_AAST_TALKBACK_ != audio_stream_type) || (NULL == pAudio_para))
    {
        DEBUG_ERROR( "CAvEncode::Ae_start_talkback_encoder Invalid parameter(type:%d)\n", audio_stream_type);
        /*considered*/
        goto _OVER_;
    }

    if(Ae_get_task_id(audio_stream_type, phy_audio_chn_num, &recv_task_id, &send_task_id) != 0)
    {
        DEBUG_ERROR( "CAvEncoder::Ae_start_talkback_encoder Ae_get_task_id FAILED\n");
        goto _OVER_;
    }
    if(true == thread_control)
    {
        m_pThread_lock->lock();
        Ae_thread_control(_ETT_STREAM_REC_, recv_task_id, _ATC_REQUEST_PAUSE_);
    }

    /*start talkback*/
    if(0 != Ae_start_encoder_local(video_stream_type, phy_video_chn_num, pVideo_para, audio_stream_type, phy_audio_chn_num, pAudio_para))
    {
        DEBUG_ERROR( "CAvEncoder::Ae_start_encoder Ae_start_composite_encoder_local FAILED(%d)\n", phy_video_chn_num);
        goto _OVER_;
    }

    ret_val = 0;
    
_OVER_:
    if(true == thread_control)
    {
        Ae_thread_control(_ETT_STREAM_REC_, recv_task_id, _ATC_REQUEST_RUNNING_);
    
        m_pThread_lock->unlock();

    }
    return ret_val;
}

int CAvEncoder::Ae_stop_talkback_encoder(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_audio_stream_type_t audio_stream_type, int phy_audio_chn_num, bool thread_control)
{
    int recv_task_id = -1;
    int send_task_id = -1;
    int ret_val = -1;

    if(audio_stream_type != _AAST_TALKBACK_)
    {
        DEBUG_ERROR( "CAvEncoder::Ae_stop_encoder Invalid parameter(type:%d)\n", audio_stream_type);
        return -1;
    }


    if(Ae_get_task_id(audio_stream_type, phy_audio_chn_num, &recv_task_id, &send_task_id) != 0)
    {
        DEBUG_ERROR( "CAvEncoder::Ae_stop_encoder Ae_get_task_id FAILED\n");
        return -1;
    }
    
    if(true == thread_control)
    {
        m_pThread_lock->lock();
        
        Ae_thread_control(_ETT_STREAM_REC_, recv_task_id, _ATC_REQUEST_PAUSE_);
    }

    if(0 != Ae_stop_encoder_local(video_stream_type, phy_video_chn_num, audio_stream_type, phy_audio_chn_num))
    {
        DEBUG_ERROR( "CAvEncoder::Ae_stop_encoder Ae_start_composite_encoder_local FAILED(%d)\n", phy_video_chn_num);
        goto _OVER_;
    }
    ret_val = 0;

_OVER_:
    if(thread_control == true)
    {
        DEBUG_MESSAGE("*********local_task_list:%d**********!\n", m_pRecv_thread_task[recv_task_id].local_task_list.size());
        if(m_pRecv_thread_task[recv_task_id].local_task_list.size() == 0)
        {
            Ae_thread_control(_ETT_STREAM_REC_, recv_task_id, _ATC_REQUEST_EXIT_);
        }
        else
        {
            Ae_thread_control(_ETT_STREAM_REC_, recv_task_id, _ATC_REQUEST_RUNNING_);
        }

        m_pThread_lock->unlock();
    }

    return ret_val;
}

int CAvEncoder::Ae_encoder_recv_thread(int task_id)
{
    _AV_ASSERT_((task_id >= 0) && (task_id < _AE_ENCODER_MAX_RECV_THREAD_NUMBER_), "CAvEncoder::Ae_encoder_recv_thread Invalid task id(%d)(%d)\n", task_id, _AE_ENCODER_MAX_RECV_THREAD_NUMBER_);

    /*receive stream from local hardware encoder, Or receive stream from slave chip by dma*/
    av_recv_thread_task_t *pAv_thread_task = m_pRecv_thread_task + task_id;
    std::list<av_audio_video_group_t>::iterator local_av_group_it;
    std::list<av_video_stream_t>::iterator video_stream_it;
    std::list<av_audio_stream_t>::iterator audio_stream_it;
    av_stream_data_t av_stream_data;
    struct timeval time_out;
    int max_fd = -1;
    fd_set read_set;
    SShareMemData share_mem_data;

    share_mem_data.iPackCount = 0;
    share_mem_data.pstuPack = new SShareMemData::_framePack_[_AV_MAX_PACK_NUM_ONEFRAME_ + 1];
    while(1)
    {
        /******************************thread control*******************************/
        //printf("ffffffffffff, requeset = %d, task_id = %d\n", pAv_thread_task->thread_request, task_id);
_CHECK_THREAD_STATE_:
        switch(pAv_thread_task->thread_request)
        {
            case _ATC_REQUEST_RUNNING_:
                /*i will get stream*/
                break;
            case _ATC_REQUEST_PAUSE_:
                pAv_thread_task->thread_response = _ATC_RESPONSE_PAUSED_;
                usleep(1000 * 30);
                goto _CHECK_THREAD_STATE_;
                break;
            case _ATC_REQUEST_EXIT_:
                pAv_thread_task->thread_response = _ATC_RESPONSE_EXITED_;
                goto _EXIT_THREAD_;
                break;
            default:
                _AV_FATAL_(1, "CAvEncoder::Ae_encoder_recv_thread Invalid request(%d)\n", pAv_thread_task->thread_request);
                break;
        }

        /******************************collect fd*******************************/
        FD_ZERO(&read_set);
        max_fd = -1;

        ////////////////////////////////////local task////////////////////////////////////
        local_av_group_it = pAv_thread_task->local_task_list.begin();
        while(local_av_group_it != pAv_thread_task->local_task_list.end())/*task list*/
        {
            /*audio stream*/
            if(local_av_group_it->audio_stream.audio_encoder_id.audio_encoder_fd >= 0)
            {
                FD_SET(local_av_group_it->audio_stream.audio_encoder_id.audio_encoder_fd, &read_set);
                max_fd = (local_av_group_it->audio_stream.audio_encoder_id.audio_encoder_fd > max_fd)?local_av_group_it->audio_stream.audio_encoder_id.audio_encoder_fd:max_fd;
            }
            /*video stream*/
            video_stream_it = local_av_group_it->video_stream.begin();
            while(video_stream_it != local_av_group_it->video_stream.end())
            {
                if(video_stream_it->video_encoder_id.video_encoder_fd >= 0)
                {
                    FD_SET(video_stream_it->video_encoder_id.video_encoder_fd, &read_set);
                    max_fd = (video_stream_it->video_encoder_id.video_encoder_fd > max_fd)?video_stream_it->video_encoder_id.video_encoder_fd:max_fd;
                }
                video_stream_it ++;
            }
            local_av_group_it ++;
        }
        ////////////////////////////////////slave task////////////////////////////////////
        if(max_fd < 0)
        {
            //usleep(1000 * 30);
            usleep(1000 * 100);
            continue;
        }
        /******************************get stream*******************************/
        time_out.tv_sec = 1;
        time_out.tv_usec = 0;
        int ret_val = select(max_fd + 1, &read_set, NULL, NULL, &time_out);
        if(ret_val > 0)
        {
            //int audio_ret = 0, video_ret[5];
            //memset(video_ret, 0, 5 * sizeof(int));
            bool bHaveData;
            for (int times = 0; times < 5; ++times)
            {
                bHaveData = false;
                ////////////////////////////////////local task////////////////////////////////////
                local_av_group_it = pAv_thread_task->local_task_list.begin();
                while(local_av_group_it != pAv_thread_task->local_task_list.end())
                {
                    /*get audio stream*/
                    if(  (local_av_group_it->audio_stream.audio_encoder_id.audio_encoder_fd >= 0)
                        && (FD_ISSET(local_av_group_it->audio_stream.audio_encoder_id.audio_encoder_fd, &read_set)) )
                    {
                        /*read audio stream*/
#ifdef _AV_PLATFORM_HI3515_	//3515 no_block is true, opposite to other products///
						if(Ae_get_audio_stream(&local_av_group_it->audio_stream.audio_encoder_id, &av_stream_data, true) == 0)
#else
						if(Ae_get_audio_stream(&local_av_group_it->audio_stream.audio_encoder_id, &av_stream_data, false) == 0)
#endif
                        {
                            bHaveData = true;
                            /*put it into stream buffer*/
                            if(local_av_group_it->audio_stream.type == _AAST_NORMAL_)
                            {
                                /*put it into video stream buffer*/
                                video_stream_it = local_av_group_it->video_stream.begin();
                                while(video_stream_it != local_av_group_it->video_stream.end())
                                {
#if (!defined(_AV_PRODUCT_CLASS_IPC_)) && (!defined(_AV_PRODUCT_CLASS_NVR_)) && (!defined(_AV_HAVE_PURE_AUDIO_CHANNEL))
									/*2016-3-22, not put audio stream if chn is videoloss*/
									if(Ae_get_video_lost_flag(video_stream_it->phy_video_chn_num) == 0)
#endif
									{
										if((video_stream_it->video_encoder_para.have_audio == 1) || (video_stream_it->video_encoder_para.have_audio == 2 && video_stream_it->video_encoder_para.record_audio == 1))
										{
											if(true == m_locked_video_stream_buffer[video_stream_it->type][video_stream_it->phy_video_chn_num])
											{
												Ae_put_video_stream(video_stream_it, &local_av_group_it->audio_stream, &av_stream_data, share_mem_data);
											}
										}
									}
                                    video_stream_it ++;
                                }
                            }
                            else if(local_av_group_it->audio_stream.type == _AAST_TALKBACK_)
                            {
                                Ae_put_audio_stream(&local_av_group_it->audio_stream, &av_stream_data, share_mem_data);
                            }
                            else
                            {
                                _AV_FATAL_(1, "CAvEncoder::Ae_encoder_recv_thread You must give the implement\n");
                            }
                            Ae_release_audio_stream(&local_av_group_it->audio_stream.audio_encoder_id, &av_stream_data);
                        }
                        else
                        {
                            //DEBUG_MESSAGE("ch%d no audio so clear it\n", local_av_group_it->audio_stream.phy_audio_chn_num);
                            FD_CLR(local_av_group_it->audio_stream.audio_encoder_id.audio_encoder_fd, &read_set);
                        }
                    }
                    /*get video stream*/
                    video_stream_it = local_av_group_it->video_stream.begin();
                    while(video_stream_it != local_av_group_it->video_stream.end())
                    {
                        if( (video_stream_it->video_encoder_id.video_encoder_fd >= 0) && (FD_ISSET(video_stream_it->video_encoder_id.video_encoder_fd, &read_set) ))
                        {
                            if(false == m_locked_video_stream_buffer[video_stream_it->type][video_stream_it->phy_video_chn_num])
                            {
                                // DEBUG_MESSAGE("@@@++++++CAvEncoder::Ae_encoder_recv_thread() test and try to get shared buffer lock++++++++++++++\n");
                                if(0 == N9M_SHLockWriter(m_video_stream_buffer[video_stream_it->type][video_stream_it->phy_video_chn_num]))
                                {
                                    m_locked_video_stream_buffer[video_stream_it->type][video_stream_it->phy_video_chn_num] = true;
                                }
                                else
                                {
                                    DEBUG_ERROR( "\nCAvEncoder::Ae_encoder_recv_thread()get lock failed\n");
                                }
                                // DEBUG_MESSAGE("@@@@+++++++++m_locked_video_stream_buffer[%d][%d] = %d++++++++++++++\n",video_stream_it->type,video_stream_it->phy_video_chn_num,m_locked_video_stream_buffer[video_stream_it->type][video_stream_it->phy_video_chn_num]);
                            }
                            
#ifdef _AV_PLATFORM_HI3515_	//3515 no_block is true, opposite to other products///
                            if(Ae_get_video_stream(&video_stream_it->video_encoder_id, &av_stream_data, true) == 0)
#else
                            if(Ae_get_video_stream(&video_stream_it->video_encoder_id, &av_stream_data, false) == 0)
#endif
                            {
                                bHaveData = true;
                                if(true == m_locked_video_stream_buffer[video_stream_it->type][video_stream_it->phy_video_chn_num])
                                {
                                    Ae_put_video_stream(video_stream_it, NULL, &av_stream_data, share_mem_data);
                                }
                                Ae_release_video_stream(&video_stream_it->video_encoder_id, &av_stream_data);
                            }
                            else
                            {
                                FD_CLR(video_stream_it->video_encoder_id.video_encoder_fd, &read_set);
                                //DEBUG_MESSAGE("ch%d no video\n", video_stream_it->phy_video_chn_num);
                            }
                        }
                        video_stream_it ++;
                    }
                    local_av_group_it ++;
                }            

                ////////////////////////////////////slave task////////////////////////////////////
                if( bHaveData == false )
                {
                    // DEBUG_MESSAGE("don't have data , break\n");
                    break;
                }
            }
#ifdef _AV_PLATFORM_HI3515_
            mSleep(15); //! previous 20
#else
			mSleep(40);
#endif
        }
        else
        {
            //usleep(1000 * 30);
#ifdef _AV_PLATFORM_HI3515_
			mSleep(30);
#else
			mSleep(40);
#endif
        }
    }
_EXIT_THREAD_:
    if(NULL != share_mem_data.pstuPack)
    {
        delete []share_mem_data.pstuPack;
        share_mem_data.pstuPack = NULL;
    }
    
    return 0;
}

int CAvEncoder::CacheOsdFond( const char* pszStr, bool bosd_thread_control_flag)
{
    static char* pszLastOsd = NULL;
    if( pszLastOsd != NULL )
    {
        if( strcmp(pszLastOsd, pszStr) == 0 )
        {
            DEBUG_MESSAGE( "CAvEncoder::CacheOsdFond str(%s) is the same\n", pszStr);
            return 0;
        }

        delete [] pszLastOsd;
        pszLastOsd = NULL;
    }

    int font_len = strlen(pszStr);
    if( font_len > 0 )
    {
        pszLastOsd = new char[font_len+1];
        strcpy( pszLastOsd, pszStr);
        pszLastOsd[font_len] =  '\0';
    }
    
    if(bosd_thread_control_flag && m_pOsd_thread->thread_running)
    {
        m_pOsd_thread->thread_request = _ATC_REQUEST_PAUSE_;
        while( m_pOsd_thread->thread_response != _ATC_RESPONSE_PAUSED_ )
        {
            DEBUG_MESSAGE( "CAvEncoder::CacheOsdFond Wait Osd thread paused for change osd mem\n");
            usleep(100*1000);
        }
    }

    char* pszBuffer = NULL;
    //if( strlen(pszStr) < 32 )
    {
        pszBuffer = new char[strlen(pszStr)+4];
        memset( pszBuffer, 0, strlen(pszStr)+4 );
        strncpy( pszBuffer,  pszStr, strlen(pszStr)+3);
        DEBUG_MESSAGE( "Cache osd font, str=%s len:%d \n", pszBuffer, strlen(pszBuffer));
    }
    m_pAv_font_library->CacheFontsToShareMem( pszBuffer );

    delete [] pszBuffer;
    pszBuffer = NULL;
    if(bosd_thread_control_flag)
    {
        m_pOsd_thread->thread_request = _ATC_REQUEST_RUNNING_;
    }
    return 0;
}

#ifndef _AV_PRODUCT_CLASS_IPC_ 
int CAvEncoder::ChangeLanguageCacheOsdChar()
{
        this->CacheOsdFond(GuiTKGetText(NULL, NULL, "STR_GPS"));
        this->CacheOsdFond(GuiTKGetText(NULL, NULL, "STR_GPS INVALID"));
        this->CacheOsdFond(GuiTKGetText(NULL, NULL, "STR_GPS NOT_EXIST"));
        return 0;
}
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_)       
int CAvEncoder::Ae_update_osd_content(av_video_stream_type_t  stream_type, int phy_video_chn_num, av_osd_name_t osd_name, char osd_content[], unsigned int osd_len)
{
    std::list<av_osd_item_t>::iterator osd_item;
 
    m_pOsd_thread->thread_lock->lock();
    osd_item = m_pOsd_thread->osd_list.begin();
    while(osd_item != m_pOsd_thread->osd_list.end())
    {
        if((stream_type == osd_item->video_stream.type) &&(phy_video_chn_num == osd_item->video_stream.phy_video_chn_num) &&\
            osd_name == osd_item->osd_name)
        {
        
            switch(osd_name)
            {
                case _AON_SPEED_INFO_:
                {
                    if((_AON_SPEED_INFO_ == osd_item->osd_name) && (0 != osd_item->video_stream.video_encoder_para.have_speed_osd))
                    {
                        int len = (sizeof(osd_item->video_stream.video_encoder_para.speed)-1)>osd_len? \
                            osd_len:(sizeof(osd_item->video_stream.video_encoder_para.speed)-1);
                        memcpy(osd_item->video_stream.video_encoder_para.speed, osd_content, len);
                        osd_item->video_stream.video_encoder_para.speed[len] =  '\0';
                    }
                }
                    break;
                case _AON_GPS_INFO_:
                {
                    if((_AON_GPS_INFO_ == osd_item->osd_name) && (0 != osd_item->video_stream.video_encoder_para.have_gps_osd))
                    {
                        int len = (sizeof(osd_item->video_stream.video_encoder_para.gps)-1)>osd_len? \
                            osd_len:(sizeof(osd_item->video_stream.video_encoder_para.gps)-1);
                        memcpy(osd_item->video_stream.video_encoder_para.gps, osd_content, len);
                        osd_item->video_stream.video_encoder_para.gps[len] =  '\0';
                    }                        
                }
                    break;
                case _AON_BUS_NUMBER_:
                {
                    if((_AON_BUS_NUMBER_ == osd_item->osd_name) && (0 != osd_item->video_stream.video_encoder_para.have_bus_number_osd))
                    {
                        int len = (sizeof(osd_item->video_stream.video_encoder_para.bus_number)-1)>osd_len? \
                            osd_len:(sizeof(osd_item->video_stream.video_encoder_para.bus_number)-1);
                        memcpy(osd_item->video_stream.video_encoder_para.bus_number, osd_content, len);
                        osd_item->video_stream.video_encoder_para.bus_number[len] =  '\0';
                    }                         
                }
                    break;
               case _AON_SELFNUMBER_INFO_:
               {
                    if((_AON_SELFNUMBER_INFO_ == osd_item->osd_name) && (0 != osd_item->video_stream.video_encoder_para.have_bus_selfnumber_osd))
                    {
                        int len = (sizeof(osd_item->video_stream.video_encoder_para.bus_selfnumber)-1)>osd_len? \
                            osd_len:(sizeof(osd_item->video_stream.video_encoder_para.bus_selfnumber)-1);
                        memcpy(osd_item->video_stream.video_encoder_para.bus_selfnumber, osd_content, len);
                        osd_item->video_stream.video_encoder_para.bus_selfnumber[len] =  '\0';
                    }                         
                }
                    break;
               case _AON_COMMON_OSD1_:
               {
                    if((_AON_COMMON_OSD1_ == osd_item->osd_name) && (0 != osd_item->video_stream.video_encoder_para.have_common1_osd))
                    {
                        int len = (sizeof(osd_item->video_stream.video_encoder_para.osd1_content)-1)>osd_len? \
                            osd_len:(sizeof(osd_item->video_stream.video_encoder_para.osd1_content)-1);
                        memset(osd_item->video_stream.video_encoder_para.osd1_content, 0, sizeof(osd_item->video_stream.video_encoder_para.osd1_content));
                        memcpy(osd_item->video_stream.video_encoder_para.osd1_content, osd_content, len);
                        osd_item->video_stream.video_encoder_para.osd1_content[len] =  '\0';
                    }                         
                }
                    break;
               case _AON_COMMON_OSD2_:
               {
                    if((_AON_COMMON_OSD2_ == osd_item->osd_name) && (0 != osd_item->video_stream.video_encoder_para.have_common2_osd))
                    {
                        int len = (sizeof(osd_item->video_stream.video_encoder_para.osd2_content)-1)>osd_len? \
                            osd_len:(sizeof(osd_item->video_stream.video_encoder_para.osd2_content)-1);
                        memset(osd_item->video_stream.video_encoder_para.osd2_content, 0, sizeof(osd_item->video_stream.video_encoder_para.osd2_content));
                        memcpy(osd_item->video_stream.video_encoder_para.osd2_content, osd_content, len);
                        osd_item->video_stream.video_encoder_para.osd2_content[len] =  '\0';
                    }                         
                }
                    break;
                default:
                    _AV_KEY_INFO_(_AVD_ENC_, "the extra osd:%d is not defined! \n", osd_name);
                    m_pOsd_thread->thread_lock->unlock();
                    return -1;
                    break;
            }
            osd_item->is_update = true;
            m_pOsd_thread->thread_lock->unlock();
            return 0;
        }
        osd_item++;
    }
    #if 0
    _AV_KEY_INFO_(_AVD_ENC_, "[%s %d]the stream type(%d ) or phy_video_chn_num(%d) is not found! \n", \
        __FUNCTION__, __LINE__, stream_type, phy_video_chn_num);
    #endif
    m_pOsd_thread->thread_lock->unlock();
    return -1;

}

#endif

#if defined(_AV_PLATFORM_HI3515_)
int CAvEncoder::Ae_get_osd_string_width(const char* szOsdString, int fontName, int horScale, bool align /* = true */)
{
	if(NULL == szOsdString || strlen(szOsdString) <= 0)
	{
		return 0;
	}

	CAvUtf8String stuUt8String(szOsdString);
	av_ucs4_t char_unicode = 0;
	av_char_lattice_info_t lattice_char_info;
	int length = 0;
	while(stuUt8String.Next_char(&char_unicode) == true)
	{
		if(m_pAv_font_library->Afl_get_char_lattice(fontName, char_unicode,&lattice_char_info) == 0)
		{
			length += lattice_char_info.width;
		}
		else
		{
			DEBUG_ERROR( "EXTEND_OSD: Afl_get_char_lattice failed, unicode = %ld, font_name = %d\n", char_unicode, fontName);
			continue;
		}
	}

	if(align)	//对齐///
	{
		return ((length * horScale + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_);
	}
	else
	{
		return (length * horScale);
	}
}

#endif



#include <sys/mman.h>
#include <stdio.h>
#include "HiIve.h"
#include "StreamBuffer.h"
#include "CommonDebug.h"
#include "HiVi.h"
#include "HiVpss.h"
#include "hi_mem.h"

#if defined(_AV_IVE_FD_) || defined(_AV_IVE_HC_)
#include "object_detection.h"
#endif

#if defined(_AV_IVE_LD_)
#include "lane_departure.h"
#endif

#if defined(_AV_IVE_BLE_)
#include "bus_lane_enforcement.h"
#include "mpi_vb.h"
#include "mpi_vgs.h"

#endif

#if defined(_AV_IVE_BLE_) || defined(_AV_IVE_LD_)
#include <string.h>
#include <fstream>
#include <json/json.h>
#endif

#if 0
#define SAVE_LOCAL_PICTURE
#endif


using namespace Common;

typedef enum thread_state
{
    IVE_THREAD_READY = 0,
    IVE_THREAD_START,
    IVE_THREAD_STOP,
    IVE_THREAD_EXIT
}ive_thread_state_e;

#define PHY_CHN_NUM (0)

#if defined(_AV_IVE_FD_)
static pthread_t thread_face_recognition_id;
static ive_thread_state_e thread_face_recognition_state = IVE_THREAD_EXIT;
pthread_mutex_t fd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t fd_cond = PTHREAD_COND_INITIALIZER;
static void* ive_face_recognition_body(void * args);

#define IVE_CONFIG_PATH "./fd_config/face_detection_config.xml"
#elif defined(_AV_IVE_LD_)
static pthread_t thread_lane_detection_id;
static ive_thread_state_e thread_lane_detection_state = IVE_THREAD_STOP;
static void *ive_lane_detection_body(void *args);
#elif defined(_AV_IVE_BLE_)
static pthread_t thread_lane_enforcement_id;
static ive_thread_state_e thread_lane_enforcement_state = IVE_THREAD_STOP;
static void *ive_lane_enforcement_body(void *args);

#define IVE_CONFIG_PATH "./ble_config/"

//! for singapore bus snap
#define SINGAPORE_BUS_TIME "/usr/local/cfg/SINGAPORE_BUS_CFG"

#define IVE_BLE_FRAME_WIDTH (1280)
#define IVE_BLE_FRAME_HEIGHT (720)
#define IVE_BLE_OSD_HEIGHT (80)
#define IVE_BLE_OSD_WIDTH (IVE_BLE_FRAME_WIDTH)

#define IVE_BLE_SCALE_FRAME_WIDTH (1280)
#define IVE_BLE_SCALE_FRAME_HEIGHT (720)
#define IVE_BLE_SNAP_PICTURE_WIDTH (IVE_BLE_FRAME_WIDTH*2)
#define IVE_BLE_SNAP_PICTURE_HEIGHT (IVE_BLE_SCALE_FRAME_HEIGHT*2)

#define IVE_BLE_SNAP_OSD_NUM 8

//! end singapor bus snap
#elif defined(_AV_IVE_HC_)
static pthread_t thread_head_count_id;
static ive_thread_state_e thread_head_count_state = IVE_THREAD_EXIT;
pthread_mutex_t hc_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t hc_cond = PTHREAD_COND_INITIALIZER;
static void* ive_head_count_body(void * args);
#define IVE_CONFIG_PATH "./hc_config/face_detection_config.xml"
#endif


CHiIve::CHiIve(data_sorce_e data_source_type, data_type_e data_type, void* data_source, CHiAvEncoder* mpEncoder):
    m_pData_source(data_source),m_eDate_source_type(data_source_type), m_eData_type(data_type)
{
 #if defined(_AV_IVE_FD_)
     //get share memery info
    unsigned int stream_buffer_size = 0;
    unsigned int stream_buffer_frame = 0;
    char stream_buffer_name[32];
    if(pgAvDeviceInfo->Adi_get_stream_buffer_info(BUFFER_TYPE_SNAP_STREAM, 0, stream_buffer_name, stream_buffer_size, stream_buffer_frame) != 0)
    {
        DEBUG_ERROR( "get buffer failed!\n");
        return;
    }

    m_hSnapShareMemHandle = N9M_SHCreateShareBuffer( "avStreaming", stream_buffer_name, stream_buffer_size, stream_buffer_frame, SHAREMEM_WRITE_MODE);
    N9M_SHUnlockWriter(m_hSnapShareMemHandle);
    N9M_SHClearAllFrames(m_hSnapShareMemHandle);
 #elif defined(_AV_IVE_HC_)
    m_detection_result_notify_func = NULL;
 #else
    m_detection_result_notify_func = NULL;
    m_bCurrent_detection_state = 0;
 #endif
    m_pEncoder = mpEncoder;
    m_Ive_process_func = NULL;
#if defined(_AV_IVE_BLE_) 

    //! necessary for singapore snap
    m_ive_snap_thread = new CIveSnap(mpEncoder);

    m_u32Blk_handle = VB_INVALID_HANDLE;
    m_u32Pool = VB_INVALID_POOLID;
    m_u32Frame_phy_addr  = 0;
    m_u32Frame_vir_addr = NULL;
    m_vFrames.clear();
    m_vSnap_osd.clear();
    
    m_single_snapped = 0;
    memset(m_snap_osd, 0, sizeof(m_snap_osd));
    memset(&m_yellow_lane_time, 0, sizeof(m_yellow_lane_time));
    memset(&m_pink_lane_time, 0, sizeof(m_pink_lane_time));
    //! read time cfg from file
    #if 1
    char customer[32];
    memset(customer, 0, sizeof(customer));
    pgAvDeviceInfo->Adi_get_customer_name(customer, sizeof(customer));

    if(0 == strcmp(customer, "CUSTOMER_SG"))
    {
        std::ifstream ifs;
        ifs.open(SINGAPORE_BUS_TIME, ios::binary);
        
        Json::Value bus_time;
        Json::Reader reader;
        if(reader.parse(ifs, bus_time))
        {
            if(!bus_time["YELLOW"].empty())
            {
                m_yellow_lane_time.start_week = bus_time["YELLOW"]["SW1"].asInt();
                m_yellow_lane_time.end_week   = bus_time["YELLOW"]["EW1"].asInt();
                m_yellow_lane_time.start_hour1 = bus_time["YELLOW"]["SH1"].asInt();
                m_yellow_lane_time.end_hour1 = bus_time["YELLOW"]["EH1"].asInt();
                m_yellow_lane_time.start_hour2 = bus_time["YELLOW"]["SH2"].asInt();
                m_yellow_lane_time.end_hour2 = bus_time["YELLOW"]["EH2"].asInt();
                m_yellow_lane_time.start_min1 = bus_time["YELLOW"]["SM1"].asInt();
                m_yellow_lane_time.end_min1 = bus_time["YELLOW"]["EM1"].asInt();
                m_yellow_lane_time.start_min2 = bus_time["YELLOW"]["SM2"].asInt();
                m_yellow_lane_time.end_min2 = bus_time["YELLOW"]["EM2"].asInt();
            }
            if(!bus_time["PINK"].empty())
            {
                m_pink_lane_time.start_week = bus_time["PINK"]["SW1"].asInt();
                m_pink_lane_time.end_week   = bus_time["PINK"]["EW1"].asInt();
                m_pink_lane_time.start_hour1 = bus_time["PINK"]["SH1"].asInt();
                m_pink_lane_time.end_hour1 = bus_time["PINK"]["EH1"].asInt();

                m_pink_lane_time.start_min1 = bus_time["PINK"]["SM1"].asInt();
                m_pink_lane_time.end_min1 = bus_time["PINK"]["EM1"].asInt();      
            }
    }

    	DEBUG_CRITICAL("yellow sw:%d,ew:%d,sh1:%d,eh1:%d,sh2:%d,eh2:%d,sm1:%d,ew1:%d,sm2:%d,ew2:%d\n",
        m_yellow_lane_time.start_week,
        m_yellow_lane_time.end_week,
        m_yellow_lane_time.start_hour1,
        m_yellow_lane_time.end_hour1,
        m_yellow_lane_time.start_hour2,
        m_yellow_lane_time.end_hour2,
        m_yellow_lane_time.start_min1,
        m_yellow_lane_time.end_min1,
        m_yellow_lane_time.start_min2,
        m_yellow_lane_time.end_min2);
    	DEBUG_CRITICAL("pink sw:%d,ew:%d,sh1:%d,eh1:%d,sm1:%d,ew1:%d\n",
        m_pink_lane_time.start_week,
        m_pink_lane_time.end_week,
        m_pink_lane_time.start_hour1,
        m_pink_lane_time.end_hour1,
        m_pink_lane_time.start_min1,
        m_pink_lane_time.end_min1);
    }
    #endif
#endif

#if defined(_AV_IVE_HC_) || defined(_AV_IVE_FD_)
    m_32Detect_handle = 0;
#endif
    memset(m_osd,0,sizeof(m_osd));
}
    
CHiIve::~CHiIve()
{
    ive_stop_work();
#if defined(_AV_IVE_BLE_) || defined(_AV_IVE_LD_)
    _AV_SAFE_DELETE_(m_ive_snap_thread);
    m_detection_result_notify_func = NULL;
    m_bCurrent_detection_state = 0;
#endif
#if defined(_AV_IVE_HC_)
    m_detection_result_notify_func = NULL;
#endif
}

int CHiIve::ive_start_work()
{   
    if((m_eData_type == _DATA_TYPE_YUV_420SP ||m_eData_type == _DATA_TYPE_YUV_422SP ) && m_eDate_source_type== _DATA_SRC_VI)
    {
         m_Ive_process_func = boost::bind(&CHiIve::ive_process_yuv_data_from_vi, this, _1);
    }
    else if((m_eData_type == _DATA_TYPE_YUV_420SP ||m_eData_type == _DATA_TYPE_YUV_422SP ) && m_eDate_source_type  == _DATA_SRC_VPSS)
    {
         m_Ive_process_func = boost::bind(&CHiIve::ive_process_yuv_data_from_vpss, this, _1);
    }
    else
    {
         m_Ive_process_func = boost::bind(&CHiIve::ive_process_raw_data_from_vi, this, _1);
    }
    
    ive_init();

#if defined(_AV_IVE_FD_)
    ive_fd_ready();
#elif defined(_AV_IVE_BLE_)
    char customer[32];
    memset(customer, 0, sizeof(customer));
    pgAvDeviceInfo->Adi_get_customer_name(customer, sizeof(customer));
    

    if(0 == strcmp(customer, "CUSTOMER_CQJY"))
    {
        m_ive_snap_thread->StartIveSnapThreadEx(boost::bind(&CHiIve::ive_recycle_resource, this, _1));
    }
    else
    {
        m_ive_snap_thread->StartIveSnapThreadEx(boost::bind(&CHiIve::ive_recycle_resource2, this, _1));
    }

    ive_ble_ready();
    
    m_ive_snap_thread->StartIveSnapThreadEx(boost::bind(&CHiIve::ive_recycle_resource2, this, _1));
    //!end singapore
#elif defined(_AV_IVE_LD_)
    ive_ld_ready();
#elif defined(_AV_IVE_HC_)
    ive_hc_ready();
#endif
    return 0;
}

int CHiIve::ive_stop_work()
{
    m_Ive_process_func = NULL;
    
#if defined(_AV_IVE_FD_)
    ive_fd_stop();
    ive_fd_exit();
#elif defined(_AV_IVE_BLE_)
    ive_ble_exit();
    m_ive_snap_thread->StopIveSnapThread();
#elif defined(_AV_IVE_LD_)
    ive_ld_exit();
#elif defined(_AV_IVE_HC_)
    ive_hc_stop();
    ive_hc_exit();    
#endif

    ive_uninit();

    return 0;
}

int CHiIve::ive_analyze_data()
{
    if(NULL != m_Ive_process_func)
    {
        return m_Ive_process_func(PHY_CHN_NUM);
    }
    return 0;
}

int CHiIve::ive_set_osd_info(char * osd)
{
    if(NULL != osd)
    {
        strncpy(m_osd, osd, sizeof(m_osd));
    }
    return 0;
}



int CHiIve::ive_process_raw_data_from_vi(int phy_chn)
{
    DEBUG_MESSAGE("this interface not support now! \n");
    return 0;
}
int CHiIve::ive_process_yuv_data_from_vi(int phy_chn)
{
#if 0
    int ret = 0;
    CHiVi* hivi = static_cast<CHiVi *>(m_pData_source);
    VIDEO_FRAME_INFO_S  video_frame;
    IVE_IMAGE_S src_image;

    memset(&video_frame, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&src_image, 0, sizeof(IVE_IMAGE_S));
    
    if(0 != hivi->Hivi_get_vi_frame(phy_chn, &video_frame))
    {
        DEBUG_ERROR("vi get frame failed ! \n");
        return -1;
    }

    if(video_frame.stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_422)
    {
        src_image.enType = IVE_IMAGE_TYPE_YUV422SP;
    }
    else
    {
        src_image.enType = IVE_IMAGE_TYPE_YUV420SP;
    }
    for(int i=0; i<3; ++i)
    {
        src_image.u32PhyAddr[i] = video_frame.stVFrame.u32PhyAddr[i];
        src_image.u16Stride[i] = (HI_U16)video_frame.stVFrame.u32Stride[i];
        src_image.pu8VirAddr[i]= (HI_U8*)video_frame.stVFrame.pVirAddr[i];
    }
    src_image.u16Width = (HI_U16)video_frame.stVFrame.u32Width;
    src_image.u16Height = (HI_U16)video_frame.stVFrame.u32Height;

    if ( HI_SUCCESS !=  AlgLaneDectAndSnapshot(&src_image) )
    {
        DEBUG_ERROR("AlgLaneDectAndSnapshot failed ! \n");
        ret = -1;
    }
    
    if(0 != hivi->Hivi_release_vi_frame(phy_chn, &video_frame))
    {
        DEBUG_ERROR("vi release frame failed ! \n");
        return -1;
    }
    
    return ret;
#else
    return 0;
#endif
}
int CHiIve::ive_process_yuv_data_from_vpss(int phy_chn)
{
#if defined(_AV_IVE_FD_)
    return ive_fd_process_frame(phy_chn);
#elif defined(_AV_IVE_BLE_)
    char customer[32];
    memset(customer, 0, sizeof(customer));
    pgAvDeviceInfo->Adi_get_customer_name(customer, sizeof(customer));

    if(0 == strcmp(customer, "CUSTOMER_SZGJ"))
    {
        return ive_ble_process_frame_for_SZGJ(phy_chn);
    }
    else if(0 == strcmp(customer, "CUSTOMER_SG"))
    {
        return ive_ble_process_frame(phy_chn);
    }
    else if(0 == strcmp(customer, "CUSTOMER_CQJY"))
    {
        return ive_ble_process_frame_for_CQJY(phy_chn);
    }
    else
    {
        return ive_ble_process_frame(phy_chn);
    }
#elif defined(_AV_IVE_LD_)
    return ive_ld_process_frame(phy_chn);
#elif defined(_AV_IVE_HC_)
    return ive_hc_process_frame(phy_chn);
#endif
}

int CHiIve::ive_init()
{
#if defined(_AV_IVE_FD_)
    if(HI_SUCCESS !=  ObjectDetectionInit((char *)IVE_CONFIG_PATH, &m_32Detect_handle))
    {
        DEBUG_ERROR("ObjectDetectionInit failed!  \n");
        return -1;
    }    
#elif defined(_AV_IVE_LD_)
    if(HI_SUCCESS !=  IveAlgInit((char *)"./"))
    {
        DEBUG_ERROR("lane departure init  failed!  \n");
        return -1;
    }       
#elif defined(_AV_IVE_BLE_)
    if(HI_SUCCESS !=  BusLaneEnforcementInit((HI_CHAR *)IVE_CONFIG_PATH))
    {
        DEBUG_ERROR("bus lane enforcemement init  failed!  \n");
        return -1;
    }       
#elif defined(_AV_IVE_HC_)
    if(HI_SUCCESS !=  ObjectCountingInit((char *)IVE_CONFIG_PATH, &m_32Detect_handle))
    {
        DEBUG_ERROR("FaceDetectionInit failed!  \n");
        return -1;
    }     
#endif

    return 0;
}

int CHiIve::ive_uninit()
{
#if defined(_AV_IVE_FD_)
    if(HI_SUCCESS != ObjectDetectionRelease())
    {
        DEBUG_ERROR("FaceDetectionRelease failed!  \n");
        return -1;
    }
#elif defined(_AV_IVE_BLE_)
    if(HI_SUCCESS != BusLaneEnforcementDeInit())
    {
        DEBUG_ERROR("BLE IveAlgDeInit  failed!  \n");
        return -1;
    } 
#elif defined(_AV_IVE_LD_)
    if(HI_SUCCESS != IveAlgDeInit())
    {
        DEBUG_ERROR("LD IveAlgDeInit failed!  \n");
        return -1;
    }     
#elif defined(_AV_IVE_HC_)
    if(HI_SUCCESS != ObjectCountingRelease())
    {
        DEBUG_ERROR("ObjectCountingRelease failed!  \n");
        return -1;
    }    
#endif
    return 0;
}

int CHiIve::ive_recycle_resource(ive_snap_task_s* snap_info)
    {
        if(NULL != snap_info)
        {
            _AV_SAFE_DELETE_ARRAY_(snap_info->pOsd);
            if(m_eDate_source_type == _DATA_SRC_VPSS)
            {
                //ive_clear_snap();
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

#if defined(_AV_IVE_BLE_)
int CHiIve::ive_recycle_resource2(ive_snap_task_s* snap_info)
{
    if(NULL != snap_info)
    {
        _AV_SAFE_DELETE_ARRAY_(snap_info->pOsd);
        if(m_eDate_source_type == _DATA_SRC_VPSS)
        {
            ive_clear_snap();
             _AV_SAFE_DELETE_(snap_info->pFrame);       
        }
    }
    return 0;
}
#endif

#if defined(_AV_IVE_FD_)
int CHiIve::ive_fd_ready()
{
    if(thread_face_recognition_state == IVE_THREAD_EXIT)
    {
        pthread_create(&thread_face_recognition_id, 0, ive_face_recognition_body, this); 
    }

    return 0;
}

int CHiIve::ive_fd_start()
{
    if(thread_face_recognition_state != IVE_THREAD_START)
    {
        if(thread_face_recognition_state == IVE_THREAD_EXIT)
        {
            return 0;
        }
        pthread_mutex_lock(&fd_mutex);
        thread_face_recognition_state = IVE_THREAD_START;
        pthread_cond_signal(&fd_cond);
        pthread_mutex_unlock(&fd_mutex);
    }

    return 0;
}

int CHiIve::ive_fd_stop()
{
    if(thread_face_recognition_state == IVE_THREAD_START)
    {
        pthread_mutex_lock(&fd_mutex);
        thread_face_recognition_state = IVE_THREAD_STOP;
        pthread_mutex_unlock(&fd_mutex);
    }

    return 0;
}

int CHiIve::ive_fd_exit()
{
    if(thread_face_recognition_state != IVE_THREAD_EXIT)
    {
        pthread_mutex_lock(&fd_mutex);
        thread_face_recognition_state = IVE_THREAD_EXIT;
        pthread_cond_signal(&fd_cond);
        pthread_mutex_unlock(&fd_mutex);   
        
        pthread_join(thread_face_recognition_id, NULL);
    }
    
    return 0;
}

int CHiIve::ive_fd_face_snap(VIDEO_FRAME_INFO_S* frame, RECT_S * crop_rect)
{
    if(NULL == frame || NULL == crop_rect)
    {
        DEBUG_ERROR("ive_fd_face_snap parameter is invalid!\n");
        return -1;
    }
    VENC_STREAM_S venc_stream;
    memset(&venc_stream, 0, sizeof(VENC_STREAM_S));
    
    if(0 != m_pEncoder->Ae_create_ive_snap_encoder(crop_rect->u32Width, crop_rect->u32Height, 0x0))
    {
        DEBUG_ERROR("Ae_create_ive_snap_encoder failed!\n");
        return -1;        
    }
    
    if(0 != m_pEncoder->Ae_encode_frame_to_picture(frame, crop_rect, &venc_stream))
    {
        m_pEncoder->Ae_destroy_ive_snap_encoder();
        DEBUG_ERROR("Ae_encode_frame_to_picture failed!\n");
        return -1;         
    }

    ive_fd_put_snap_frame_to_sm(&venc_stream);
    
    m_pEncoder->Ae_free_ive_snap_stream(&venc_stream);
    m_pEncoder->Ae_destroy_ive_snap_encoder();

    return 0;
}

int CHiIve::ive_fd_put_snap_frame_to_sm(VENC_STREAM_S* stream)
{
    if(NULL == stream)
    {
        DEBUG_ERROR("ive_fd_put_snap_frame_to_sm parameter is invalid!\n");
        return -1;
    }

    SnapShareMemoryHead_t stuPicHead;
    SShareMemData stuShareData;
    SShareMemFrameInfo stuShareInfo;    

    memset(&stuPicHead, 0, sizeof(stuPicHead));
    memset(&stuShareInfo, 0, sizeof(stuShareInfo));
    memset(&stuShareData, 0, sizeof(SShareMemData));
    stuShareInfo.flag = SHAREMEM_FLAG_IFRAME;
    stuShareData.pstuPack = new SShareMemData::_framePack_[_AV_MAX_PACK_NUM_ONEFRAME_ + 1]; 
    
    datetime_t stuTime; 
    memset(&stuTime, 0, sizeof(datetime_t));
    pgAvDeviceInfo->Adi_get_date_time( &stuTime );
    stuPicHead.stuSnapAttachedFile.ucSnapType = 6;//Smart capture
    stuPicHead.stuSnapAttachedFile.ucSnapChn = 0;
    stuPicHead.stuSnapAttachedFile.uiFileSize = 0;//stuFrameInfo.stVFrame.u32Width*stuFrameInfo.stVFrame.u32Height*1.5;
    stuPicHead.stuSnapAttachedFile.stuDateTime = stuTime;
    stuPicHead.stuSnapAttachedFile.ullSubType = 4;//bit2 is:fd bit1:ld bit0:ls
    stuPicHead.stuSnapAttachedFile.ucDeleteFlag = 0;
    stuPicHead.stuSnapAttachedFile.uiUserFlag = 4;
    stuPicHead.stuSnapAttachedFile.ucLockFlag = 0;
    stuShareData.iPackCount = 0;
    stuShareData.pstuPack[stuShareData.iPackCount].pAddr = (char *)&stuPicHead;
    stuShareData.pstuPack[stuShareData.iPackCount].iLen = sizeof(stuPicHead);
    ++stuShareData.iPackCount;

#ifdef SAVE_LOCAL_PICTURE
    static unsigned int u32Picture_num = 0;
    char szFileName[20];
    snprintf( szFileName, sizeof(szFileName), "./pic/%d.jpg", u32Picture_num++ );
    FILE* pfile = fopen(szFileName, "wb");

#endif
    for(unsigned int i = 0; i < stream->u32PackCount; ++i)
    {
        VENC_PACK_S * pstData = &(stream->pstPack[i]);
        stuShareData.pstuPack[stuShareData.iPackCount].pAddr = (char *)(pstData->pu8Addr + pstData->u32Offset);
        stuShareData.pstuPack[stuShareData.iPackCount].iLen = pstData->u32Len - pstData->u32Offset;
        ++stuShareData.iPackCount;
        
        stuPicHead.stuSnapAttachedFile.uiFileSize += (pstData->u32Len - pstData->u32Offset);
#ifdef SAVE_LOCAL_PICTURE
        fwrite( (char *)(pstData->pu8Addr + pstData->u32Offset), (pstData->u32Len - pstData->u32Offset), 1, pfile );
#endif 
    }
#ifdef SAVE_LOCAL_PICTURE  
    
    fclose(pfile);
#endif 
    DEBUG_MESSAGE("############snap file success, file size:%d \n", stuPicHead.stuSnapAttachedFile.uiFileSize);
    N9M_SHPutOneFrame(m_hSnapShareMemHandle, &stuShareData, &stuShareInfo, stuPicHead.stuSnapAttachedFile.uiFileSize + sizeof(stuPicHead));
    _AV_SAFE_DELETE_ARRAY_(stuShareData.pstuPack);
    return 0;
}

int CHiIve::ive_fd_process_frame(int phy_chn)
{
    CHiVpss* hivpss = static_cast<CHiVpss *>(m_pData_source);
    VIDEO_FRAME_INFO_S video_frame_1080p;  
    IVE_IMAGE_S src_image_1080p;
    int face_num = 0;
    RECT_S* face_size = NULL;
    HI_S32 nRet = HI_FAILURE;
    RECT_S detect_roi;

    memset(&detect_roi, 0, sizeof(RECT_S));
    memset(&video_frame_1080p, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&src_image_1080p, 0, sizeof(IVE_IMAGE_S));

    if(0 != hivpss->HiVpss_get_frame( _VP_MAIN_STREAM_,phy_chn, &video_frame_1080p))
    {
        DEBUG_ERROR("vpss get frame failed ! \n");
        return -1;
    }

    if(video_frame_1080p.stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_422)
    {
        src_image_1080p.enType = IVE_IMAGE_TYPE_YUV422SP;
    }
    else
    {
        src_image_1080p.enType = IVE_IMAGE_TYPE_YUV420SP;
    }
    for(int i=0; i<3; ++i)
    {
        src_image_1080p.u32PhyAddr[i] = video_frame_1080p.stVFrame.u32PhyAddr[i];
        src_image_1080p.u16Stride[i] = (HI_U16)video_frame_1080p.stVFrame.u32Stride[i];
        src_image_1080p.pu8VirAddr[i]= (HI_U8*)video_frame_1080p.stVFrame.pVirAddr[i];
    }
    src_image_1080p.u16Width = (HI_U16)video_frame_1080p.stVFrame.u32Width;
    src_image_1080p.u16Height = (HI_U16)video_frame_1080p.stVFrame.u32Height;
    //unsigned long long time_start = N9M_TMGetSystemMicroSecond();
    detect_roi.s32X = 0;
    detect_roi.s32Y = 0;
    detect_roi.u32Width = src_image_1080p.u16Width;
    detect_roi.u32Height = src_image_1080p.u16Height;

    if(1 == pgAvDeviceInfo->Adi_get_ive_debug_mode()) 
    {
        nRet = ObjectDetectionStart(&src_image_1080p, detect_roi, 1, m_32Detect_handle, &face_num, &face_size, NULL, NULL);
    }
    else
    {
        nRet = ObjectDetectionStart(&src_image_1080p, detect_roi, 0, m_32Detect_handle, &face_num, &face_size, NULL, NULL);
    }
    if(face_num> 0)
    {
        DEBUG_MESSAGE("[ST00626]detected %d faces! \n", face_num);
    }
    if(1 == pgAvDeviceInfo->Adi_get_ive_debug_mode())
    {
        if(m_pEncoder != NULL)    
        {        
            m_pEncoder->Ae_send_frame_to_encoder(_AST_MAIN_VIDEO_, 0, (void *)&video_frame_1080p, -1); 
        }
     }


    if ( HI_SUCCESS !=  nRet)
    {
        DEBUG_ERROR("AlgLaneDectAndSnapshot failed ! \n");
        if(0 != hivpss->HiVpss_release_frame(_VP_MAIN_STREAM_,phy_chn, &video_frame_1080p))
        {
            DEBUG_ERROR("vpss release 540p frame failed ! \n");
        }
        return -1;
    }
    


    //unsigned long long time_end = N9M_TMGetSystemMicroSecond();  
    //printf("[alex]the face detetion speed :%llu ms \n", time_end-time_start);
    if(face_num > 0)
    {
        //printf("[alex]found a face,  pts different is:%lld \n", (video_frame_1080p.stVFrame.u64pts - video_frame_540p.stVFrame.u64pts));
        for(int i=0; i<face_num; ++i)
        {
            RECT_S rect;

            rect.s32X= face_size[i].s32X;
            rect.s32Y= face_size[i].s32Y;
            rect.u32Width =  (face_size[i].u32Width + 3)/4*4;
            rect.u32Height = (face_size[i].u32Height + 3)/4*4;

            //unsigned long long time_start1 = N9M_TMGetSystemMicroSecond();
            ive_fd_face_snap(&video_frame_1080p,  &rect);
            //unsigned long long time_end1 = N9M_TMGetSystemMicroSecond();  
            //printf("[alex]ive_fd_face_snap speed :%llu ms \n", time_end1 - time_start1);
        }    
    } 
    
     if(0 != hivpss->HiVpss_release_frame(_VP_MAIN_STREAM_, phy_chn, &video_frame_1080p))
    {
        DEBUG_ERROR("vpss release 1080p frame failed ! \n");
        return -1;
    }      
     
    return 0;   
}


static void *ive_face_recognition_body(void *args)
{
    DEBUG_DEBUG("face recognition thread run!\n");
    CHiIve* ive = (CHiIve *)args;
    thread_face_recognition_state = IVE_THREAD_READY;

    while(1)
    {
        if(thread_face_recognition_state == IVE_THREAD_READY || thread_face_recognition_state == IVE_THREAD_STOP)
        {
            pthread_mutex_lock(&fd_mutex);
            while(thread_face_recognition_state == IVE_THREAD_READY || thread_face_recognition_state == IVE_THREAD_STOP)
            {
                pthread_cond_wait(&fd_cond ,&fd_mutex);
            }
            pthread_mutex_unlock(&fd_mutex);
        }
        else if(thread_face_recognition_state == IVE_THREAD_START)
        {
            ive->ive_analyze_data();
            usleep(10*1000);
        }
        else
        {
            break;
        }
    }

    return NULL;    
}
#endif

#if defined(_AV_IVE_HC_)
int CHiIve::ive_hc_start()
{
    DEBUG_MESSAGE("ive_hc_start! \n");
    if(thread_head_count_state != IVE_THREAD_START)
    {
        if(thread_head_count_state == IVE_THREAD_EXIT)
        {
            return -1;
        }
        pthread_mutex_lock(&hc_mutex);
        thread_head_count_state = IVE_THREAD_START;
        pthread_cond_signal(&hc_cond);
        pthread_mutex_unlock(&hc_mutex);
    }

    return 0;
}

int CHiIve::ive_hc_stop()
{
    DEBUG_MESSAGE("ive_hc_stop! \n");
    if(thread_head_count_state == IVE_THREAD_START)
    {
        pthread_mutex_lock(&hc_mutex);
        thread_head_count_state = IVE_THREAD_STOP;
        pthread_mutex_unlock(&hc_mutex);
        ive_hc_get_result();
    }
    return 0;
}

int CHiIve::ive_hc_ready()
{
    if(thread_head_count_state == IVE_THREAD_EXIT)
    {
        pthread_create(&thread_head_count_id, 0, ive_head_count_body, this); 
    }

    return 0;

}

int CHiIve::ive_hc_exit()
{
    if(thread_head_count_state != IVE_THREAD_EXIT)
    {
        pthread_mutex_lock(&hc_mutex);
        thread_head_count_state = IVE_THREAD_EXIT;
        pthread_cond_signal(&hc_cond);
        pthread_mutex_unlock(&hc_mutex);   
        
        pthread_join(thread_head_count_id, NULL);
    }
    
    return 0;

}

int CHiIve::ive_hc_process_frame(int phy_chn)
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
        nRet = ObjectCountingStart(&src_image_540p, roi, 1, m_32Detect_handle, &face_num, NULL);
    }
    else
    {
        nRet = ObjectCountingStart(&src_image_540p, roi, 0, m_32Detect_handle, &face_num, NULL);
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

int CHiIve::ive_hc_get_result()
{
    int head_count = 0;
    if(HI_SUCCESS != ObjectCountingEnd(m_32Detect_handle, &head_count))
    {
        DEBUG_ERROR("ObjectCountingEnd failed! \n");
        return -1;
    }

    DEBUG_MESSAGE("detected %d heads this time! \n", head_count);
    if(NULL != m_detection_result_notify_func)
    {
        msgIPCIntelligentInfoNotice_t notice;
        memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
        notice.cmdtype = 2;
        notice.happen = 1;
        notice.headcount = head_count;
        
        notice.happentime = N9M_TMGetShareSecond();
        m_detection_result_notify_func(&notice);
    }

    return 0;
}


static void* ive_head_count_body(void * args)
{
    printf("head count thread run!\n");
    CHiIve* ive = (CHiIve *)args;
    thread_head_count_state = IVE_THREAD_READY;

    while(1)
    {
        if(thread_head_count_state == IVE_THREAD_READY || thread_head_count_state == IVE_THREAD_STOP)
        {
            pthread_mutex_lock(&hc_mutex);
            while(thread_head_count_state == IVE_THREAD_READY || thread_head_count_state == IVE_THREAD_STOP)
            {
                pthread_cond_wait(&hc_cond ,&hc_mutex);
            }
            pthread_mutex_unlock(&hc_mutex);
        }
        else if(thread_head_count_state == IVE_THREAD_START)
        {
            ive->ive_analyze_data();
            usleep(10*1000);
        }
        else
        {
            break;
        }
    }

    return NULL;  

}

#endif

#if defined(_AV_IVE_LD_)
int CHiIve::ive_ld_ready()
{
    if(thread_lane_detection_state == IVE_THREAD_STOP)
    {
        thread_lane_detection_state = IVE_THREAD_START;
        pthread_create(&thread_lane_detection_id, 0, ive_lane_detection_body, this); 
    }

    return 0;    
}

int CHiIve::ive_ld_exit()
{
    if(thread_lane_detection_state == IVE_THREAD_START)
    {
        thread_lane_detection_state = IVE_THREAD_STOP;
        pthread_join(thread_lane_detection_id, NULL);
    }

    return 0;
}

int CHiIve::ive_ld_process_frame(int phy_chn)
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
        if(m_bCurrent_detection_state == 0)
        {
            if(ret == 1)
            {
                m_bCurrent_detection_state = 1;
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
                    m_bCurrent_detection_state = 0;
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

static void *ive_lane_detection_body(void *args)
{
    DEBUG_DEBUG("face recognition thread run!\n");
    CHiIve* ive = (CHiIve *)args;

    while(thread_lane_detection_state == IVE_THREAD_START)
    {
        ive->ive_analyze_data();
        usleep(10*1000);
    }

    return NULL;
}

#endif

#if defined(_AV_IVE_BLE_)
int CHiIve::ive_ble_ready()
{
    //! Added for singapore bus snap
    HI_U32 pic_size = 0;
    pic_size = (IVE_BLE_SNAP_PICTURE_WIDTH*IVE_BLE_SNAP_PICTURE_HEIGHT*3)>>1;

    m_u32Pool   = HI_MPI_VB_CreatePool( pic_size, 1, NULL);
    if (m_u32Pool == VB_INVALID_POOLID)
    {
        DEBUG_ERROR("HI_MPI_VB_CreatePool failed! \n");
        return -1;
    }

    m_u32Blk_handle = HI_MPI_VB_GetBlock(m_u32Pool, pic_size, NULL);
    if(VB_INVALID_HANDLE == m_u32Blk_handle)
    {
        DEBUG_ERROR("HI_MPI_VB_GetBlock failed! \n");
        HI_MPI_VB_DestroyPool(m_u32Pool);
        return -1;
    }

    m_u32Frame_phy_addr = HI_MPI_VB_Handle2PhysAddr(m_u32Blk_handle);
    m_u32Frame_vir_addr = (HI_U8*) HI_MPI_SYS_Mmap(m_u32Frame_phy_addr, pic_size );
    if (NULL== m_u32Frame_vir_addr)
    {
        DEBUG_ERROR("HI_MPI_SYS_Mmap failed! \n");
        HI_MPI_VB_ReleaseBlock(m_u32Blk_handle);
        HI_MPI_VB_DestroyPool(m_u32Pool);
        return -1;
    }
    //! end of singapore bus
    if(thread_lane_enforcement_state == IVE_THREAD_STOP)
    {
        thread_lane_enforcement_state = IVE_THREAD_START;
        pthread_create(&thread_lane_enforcement_id, 0, ive_lane_enforcement_body, this); 
    }

    return 0;    
}

int CHiIve::ive_ble_exit()
{
    if(thread_lane_enforcement_state == IVE_THREAD_START)
    {
        thread_lane_enforcement_state = IVE_THREAD_STOP;
        pthread_join(thread_lane_enforcement_id, NULL);
    }
    //! for singapore bus snap
    //ive_recycle_resource2();
    HI_MPI_SYS_Munmap(m_u32Frame_vir_addr, (IVE_BLE_SNAP_PICTURE_WIDTH*IVE_BLE_SNAP_PICTURE_HEIGHT*3)/2);
    HI_MPI_VB_ReleaseBlock(m_u32Blk_handle);
    HI_MPI_VB_DestroyPool(m_u32Pool);
    //!end singapore bus
    return 0;
}

    

int CHiIve::ive_ble_process_frame(int phy_chn)
{
    int ret = 0;
    CHiVpss* hivpss = static_cast<CHiVpss *>(m_pData_source);
    VIDEO_FRAME_INFO_S frame_1080p;
    VIDEO_FRAME_INFO_S  frame_540p;
    IVE_IMAGE_S image_1080p;
    IVE_IMAGE_S image_540p;
    
    memset(&frame_1080p, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&frame_540p, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&image_1080p, 0, sizeof(IVE_IMAGE_S));
    memset(&image_540p, 0, sizeof(IVE_IMAGE_S));

    if(0 != hivpss->HiVpss_get_frame( _VP_SUB_STREAM_,phy_chn, &frame_1080p))  //! fix 720p
    {
        DEBUG_ERROR("vpss get frame failed ! \n");
        return -1;
    }
        
    if(0 != hivpss->HiVpss_get_frame( _VP_SPEC_USE_,phy_chn, &frame_540p))
    {
        DEBUG_ERROR("vpss get frame failed ! \n");
        if(0 != hivpss->HiVpss_release_frame(_VP_MAIN_STREAM_, phy_chn, &frame_1080p))
        {
            DEBUG_ERROR("vpss release frame failed ! \n");
        }
        return -1;
    }

    image_1080p.enType = (frame_1080p.stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_422)?IVE_IMAGE_TYPE_YUV422SP:IVE_IMAGE_TYPE_YUV420SP;
    for(int i=0; i<3; ++i)
    {
        image_1080p.u32PhyAddr[i] = frame_1080p.stVFrame.u32PhyAddr[i];
        image_1080p.u16Stride[i] = (HI_U16)frame_1080p.stVFrame.u32Stride[i];
        image_1080p.pu8VirAddr[i]= (HI_U8*)frame_1080p.stVFrame.pVirAddr[i];
    }

    image_1080p.u16Width = (HI_U16)frame_1080p.stVFrame.u32Width;
    image_1080p.u16Height = (HI_U16)frame_1080p.stVFrame.u32Height;

    image_540p.enType = (frame_540p.stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_422)?IVE_IMAGE_TYPE_YUV422SP:IVE_IMAGE_TYPE_YUV420SP;

    for(int i=0; i<3; ++i)
    {
        image_540p.u32PhyAddr[i] = frame_540p.stVFrame.u32PhyAddr[i];
        image_540p.u16Stride[i] = (HI_U16)frame_540p.stVFrame.u32Stride[i];
        image_540p.pu8VirAddr[i]= (HI_U8*)frame_540p.stVFrame.pVirAddr[i];
    }
    image_540p.u16Width = (HI_U16)frame_540p.stVFrame.u32Width;
    image_540p.u16Height = (HI_U16)frame_540p.stVFrame.u32Height;    

    do
    {
        LaneEnforcementInfo plate_infos;
        memset(&plate_infos, 0, sizeof(LaneEnforcementInfo));
        if(HI_SUCCESS != BusLaneEnforcementStart(&image_1080p, &image_540p, &plate_infos))
        {
            DEBUG_ERROR("IveAlgStart  failed! \n");
            BusLaneEnforcementParametersReset((HI_CHAR *)IVE_CONFIG_PATH);
            ret = -1;
            break;
        }

        if(1 == pgAvDeviceInfo->Adi_get_ive_debug_mode())
        {
            if(m_pEncoder != NULL)    
            {        
                m_pEncoder->Ae_send_frame_to_encoder(_AST_SUB_VIDEO_, 0, (void *)&frame_540p, -1); 
            }
        }

        //! for singapore bus snap
        
        unsigned long long int snap_subtype = 0;
        VIDEO_FRAME_INFO_S single_frame;
        memset(&single_frame,0,sizeof(single_frame));
        switch(plate_infos.snap_type)
        {
            case EN_SNAP:
                DEBUG_CRITICAL("single snap\n");
                
                //ive_single_snap(&multi_frame[0],0);
                ive_single_snap(&single_frame,0);
                m_vFrames.push_back(single_frame);
                    
                ive_obtain_osd(m_snap_osd,2);
                m_vSnap_osd.push_back(m_snap_osd[0]);
                m_vSnap_osd.push_back(m_snap_osd[1]);
                if ((m_single_snapped ==0)&&(plate_infos.roi_rect_info.u32Width!=0)&&(plate_infos.roi_rect_info.u32Height!=0))
                {
                    roi = plate_infos.roi_rect_info;    //!< obtain roi info from the first snap
                }

                m_single_snapped = 1;
                
                break;
                
            case EN_EMPTY_BUFFER:
                DEBUG_CRITICAL("EN_EMPTY_BUFFER\n");
                ive_clear_snap();
                m_vSnap_osd.clear();
                m_single_snapped = 0;
                break;
                
            case EN_SINGAPORE_BUS_STOP:
                DEBUG_CRITICAL("EN_SINGAPORE_BUS_STOP snap\n");
                if(( 0 == m_single_snapped)||(m_vFrames.size()<2))
                {
                    DEBUG_CRITICAL("Invalid cmd!Single snap not enough!\n");
                    break;
                }              
                //ive_single_snap(&multi_frame[1],0); //!< second snap
                ive_single_snap(&single_frame,0);
                m_vFrames.push_back(single_frame);
                
                snap_subtype = 0x08;
                //! obtain osd info
                ive_obtain_osd(&m_snap_osd[2],2);
             
                memcpy(m_snap_osd[4].szStr, "bus stop", sizeof("bus stop")) ;
                m_vSnap_osd.push_back(m_snap_osd[2]);
                m_vSnap_osd.push_back(m_snap_osd[3]);
                m_vSnap_osd.push_back(m_snap_osd[4]);
                //end of osd info
                
                if (ive_stich_snap_for_singapore_bus(2,m_snap_osd,7,roi,snap_subtype) != 0)
                {
                    DEBUG_ERROR("Stich failed!\n");
                    ive_clear_snap();
                    m_vSnap_osd.clear();
                    break;
                }
                //!notify network
                 if(NULL != m_detection_result_notify_func)
                {
                    msgIPCIntelligentInfoNotice_t notice;
                    memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                    notice.cmdtype = 1;
                    notice.happen = 1;  
                    notice.reserved[0] = 1;

                    notice.happentime = N9M_TMGetShareSecond();
                    m_detection_result_notify_func(&notice);
                    m_bCurrent_detection_state = 1;
                } 
                 
                 //!end of notify
                 memset(m_snap_osd, 0, sizeof(m_snap_osd));
                 m_vSnap_osd.clear();

                 m_single_snapped = 0;
                break;
                
            case EN_SINGAPORE_YELLOW_BUS_LANE:
                
                DEBUG_CRITICAL("EN_SINGAPORE_YELLOW_BUS_LANE snap\n");
                if(( 0 == m_single_snapped)||(m_vFrames.size()<2))
                {
                    DEBUG_CRITICAL("Invalid cmd!Single snap not enough!\n");
                    break;
                }
                if(ive_is_singapore_bus_valid_time(1))
                {    
                    ive_clear_snap();
                    m_vSnap_osd.clear();
                    DEBUG_CRITICAL("Not the bus time!\n");
                    break;
                }
                ive_single_snap(&single_frame,0);
                m_vFrames.push_back(single_frame); //!< second snap
                snap_subtype = 0x08;
                
                //! obtain osd info
                ive_obtain_osd(&m_snap_osd[2],2);
                memcpy(m_snap_osd[4].szStr, "yellow bus lane", sizeof("yellow bus lane")) ;

                m_vSnap_osd.push_back(m_snap_osd[2]);
                m_vSnap_osd.push_back(m_snap_osd[3]);
                m_vSnap_osd.push_back(m_snap_osd[4]);
                //end of osd info
                DEBUG_CRITICAL("size of osd:%d\n", m_vSnap_osd.size());
                if (ive_stich_snap_for_singapore_bus(2,m_snap_osd,7,roi,snap_subtype) != 0)
                {
                    DEBUG_ERROR("Stich failed!\n");
                    ive_clear_snap();
                    m_vSnap_osd.clear();
                    break;
                }

                //!notify network
                 if(NULL != m_detection_result_notify_func)
                {
                    msgIPCIntelligentInfoNotice_t notice;
                    memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                    notice.cmdtype = 1;
                    notice.happen = 1;  
                    notice.reserved[0] = 1;

                    notice.happentime = N9M_TMGetShareSecond();
                    m_detection_result_notify_func(&notice);
                    m_bCurrent_detection_state = 1;
                } 
                 //!end of notify
                memset(m_snap_osd, 0, sizeof(m_snap_osd));
                 m_vSnap_osd.clear();
                m_single_snapped = 0;

                break;
            case EN_SINGAPORE_PINK_BUS_LANE:
                
                DEBUG_CRITICAL("EN_SINGAPORE_PINK_BUS_LANE snap\n");
                if(( 0 == m_single_snapped)||(m_vFrames.size()<2))
                {
                    DEBUG_CRITICAL("Invalid cmd!Single snap not enough\n");
                    break;
                }  
                if(ive_is_singapore_bus_valid_time(2))
                {                   
                    DEBUG_CRITICAL("Not the bus time!\n");
                    ive_clear_snap(); 
                    m_vSnap_osd.clear();
                    break;
                }
                ive_single_snap(&single_frame,0);
                m_vFrames.push_back(single_frame); //!< second snap

                snap_subtype = 0x08;
                
                //! obtain osd info
                ive_obtain_osd(&m_snap_osd[2],2);
                memcpy(m_snap_osd[4].szStr, "pink bus lane", sizeof("pink bus lane")) ;
                m_vSnap_osd.push_back(m_snap_osd[2]);
                m_vSnap_osd.push_back(m_snap_osd[3]);
                m_vSnap_osd.push_back(m_snap_osd[4]);
                //end of osd info
                
                if (ive_stich_snap_for_singapore_bus(2,m_snap_osd,7,roi,snap_subtype) != 0)
                {
                    DEBUG_ERROR("Stich failed!\n");
                    ive_clear_snap();
                    m_vSnap_osd.clear();
                    break;
                }

                //!notify network
                 if(NULL != m_detection_result_notify_func)
                {
                    msgIPCIntelligentInfoNotice_t notice;
                    memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                    notice.cmdtype = 1;
                    notice.happen = 1;  
                    notice.reserved[0] = 1;

                    notice.happentime = N9M_TMGetShareSecond();
                    m_detection_result_notify_func(&notice);
                    m_bCurrent_detection_state = 1;
                } 
                 //!end of notify 
                memset(m_snap_osd, 0, sizeof(m_snap_osd));
                m_vSnap_osd.clear();
                m_single_snapped = 0;
                break;
            case EN_NONE:
                
                break;                    
            default:
                DEBUG_CRITICAL("other\n");
                //m_single_snapped = 0;
                break;
        }
        //!end of singapore snap
#if 0
        if(plate_infos.plate_num > 0 && plate_infos.capture_flag == HI_TRUE)
        {
            ive_snap_info_s snap_info;
            memset(&snap_info, 0, sizeof(ive_snap_info_s));  
            snap_info.pPicture_info = frame_1080p;
            snap_info.u8Snap_type = 2;   
            snap_info.u8Picture_nums =  plate_infos.plate_num;
            snap_info.pPicture_size = (RECT_S*)malloc(snap_info.u8Picture_nums*sizeof(RECT_S));
            if(NULL == snap_info.pPicture_size)
            {
                DEBUG_ERROR("malloc snap picture rect  failed! \n");
                ret = -1;
                break;
            }
            
            for(int i=0; i<plate_infos.plate_num; ++i)
            {
                snap_info.pPicture_size[i].s32X = plate_infos.plate_pos_info[i].s32X*3;
                snap_info.pPicture_size[i].s32Y = plate_infos.plate_pos_info[i].s32Y*3;
                snap_info.pPicture_size[i].u32Width = plate_infos.plate_pos_info[i].u32Width*3;
                snap_info.pPicture_size[i].u32Height = plate_infos.plate_pos_info[i].u32Height*3;
            }
            m_ive_snap_thread->AddIveSnapTask(snap_info);
        }
#else
#if 0
        if(m_bCurrent_detection_state == 0)
        {
            plate_infos.plate_info_flag = 0x07;
        }
        else
        {
            plate_infos.plate_info_flag = 0x01;
        }
#endif
/*        
        switch(plate_infos.plate_info_flag)
        {
            case 0x07:
            {
                if(NULL != m_detection_result_notify_func)
                {
                    msgIPCIntelligentInfoNotice_t notice;
                    memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                    notice.cmdtype = 1;
                    notice.happen = 1;  
                    notice.reserved[0] = 0;
#if 1
                    for(int i=0; i<plate_infos.plate_count;++i)
                    {
                        notice.datainfo.carrange[i].vilid = 1;
                        notice.datainfo.carrange[i].point.x = plate_infos.plate_pos_info[i].s32X * 3;
                        notice.datainfo.carrange[i].point.y = plate_infos.plate_pos_info[i].s32Y * 3;
                        notice.datainfo.carrange[i].point.Width = plate_infos.plate_pos_info[i].u32Width * 3;
                        notice.datainfo.carrange[i].point.Height = plate_infos.plate_pos_info[i].u32Height * 3;
                    }
#else
                    plate_infos.plate_count = 2;
                    
                    for(int i=0; i<plate_infos.plate_count;++i)
                    {
                        notice.datainfo.carrange[i].vilid = 1;
                        notice.datainfo.carrange[i].point.x =  (40 + i*16) * 3;
                        notice.datainfo.carrange[i].point.y = (40 + i*16  )* 3;
                        notice.datainfo.carrange[i].point.Width = 240;
                        notice.datainfo.carrange[i].point.Height = 480;
                    }
#endif
                    notice.happentime = N9M_TMGetShareSecond();
                    m_detection_result_notify_func(&notice);
                    m_bCurrent_detection_state = 1;
                } 
            }
                break;
            case 0x0D:
            {
                if(NULL != m_detection_result_notify_func)
                {
                    msgIPCIntelligentInfoNotice_t notice;
                    memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                    notice.cmdtype = 1;
                    notice.happen = 1;  
                    notice.reserved[0] = 1;
                    for(int i=0; i<plate_infos.plate_count;++i)
                    {
                        memset(notice.datainfo.carnum[i], 0, sizeof(notice.datainfo.carnum[i]));
                        strncpy((char *)notice.datainfo.carnum[i], plate_infos.plate_num[i], strlen(plate_infos.plate_num[i]));
                    }
                    notice.happentime = N9M_TMGetShareSecond();
                    m_detection_result_notify_func(&notice);
                    m_bCurrent_detection_state = 1;
                }                
            }
                break;
            case 0x03:
            case 0x1:
            default:
                if(NULL != m_detection_result_notify_func && 0 != m_bCurrent_detection_state )
                {
                    msgIPCIntelligentInfoNotice_t notice;
                    memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                    notice.cmdtype = 1;
                    notice.happen = 0;  
                    notice.happentime = N9M_TMGetShareSecond();
                    m_bCurrent_detection_state = 0;
                    m_detection_result_notify_func(&notice);
                }
                
                break;
        }*/

#endif
    }while(0);

    if(0 != hivpss->HiVpss_release_frame(_VP_SUB_STREAM_,phy_chn, &frame_1080p)) //!< for singapore bus obtain from sub stream
    {
        DEBUG_ERROR("vpss release 1080P frame failed ! \n");
        ret = -1;
    }
 
    if(0 != hivpss->HiVpss_release_frame(_VP_SPEC_USE_,phy_chn, &frame_540p))
    {
        DEBUG_ERROR("vpss release 540p frame failed ! \n");
        ret = -1;
    }
    
    return ret;
}

static void *ive_lane_enforcement_body(void *args)
{
    CHiIve* ive = (CHiIve *)args;

    while(thread_lane_enforcement_state == IVE_THREAD_START)
    {
        ive->ive_analyze_data();
        usleep(10*1000);
    }

    return NULL;
}

#endif

#if defined(_AV_IVE_BLE_) || defined(_AV_IVE_LD_) || defined(_AV_IVE_HC_)
void CHiIve::ive_register_result_notify_callback(boost::function< void (msgIPCIntelligentInfoNotice_t *)> notify_func)
{
    m_detection_result_notify_func = notify_func;
}

void CHiIve::ive_unregister_result_notify_callback()
{
    m_detection_result_notify_func = NULL;
}
#endif


//! check if the time is valid
#if defined(_AV_IVE_BLE_)
int CHiIve::ive_is_singapore_bus_valid_time(int time_type)
{
    int ret = -1;
    datetime_t date_time;
    pgAvDeviceInfo->Adi_get_date_time(&date_time);
    int week = date_time.week;
    int hour = date_time.hour;
    int min = date_time.minute;
    
    switch(time_type)
        {
            case 1://! yellow bus
                if((week <=m_yellow_lane_time.end_week) && (week >=m_yellow_lane_time.start_week) )
                {
                    if (((hour < m_yellow_lane_time.end_hour1) && (hour > m_yellow_lane_time.start_hour1) )||\
                        ((hour < m_yellow_lane_time.end_hour2) && (hour > m_yellow_lane_time.start_hour2) ))
                    {

                        ret = 0;

                    }//!start == end hour
                    else if((hour = m_yellow_lane_time.end_hour1) && (hour = m_yellow_lane_time.start_hour1) &&\
                        (min <=m_yellow_lane_time.end_min1)&& (min >=m_yellow_lane_time.start_min1))
                    {
                        ret = 0;
                    } 
                    else if((hour = m_yellow_lane_time.end_hour2) && (hour = m_yellow_lane_time.start_hour2) &&\
                        (min <=m_yellow_lane_time.end_min2)&& (min >=m_yellow_lane_time.start_min2))
                    {
                        ret = 0;
                    }//! start != end hour
                    else if((hour = m_yellow_lane_time.end_hour1) && (hour > m_yellow_lane_time.start_hour1) &&\
                        (min <=m_yellow_lane_time.end_min1))
                    {
                        ret = 0;
                    } 
                    else if((hour = m_yellow_lane_time.end_hour2) && (hour > m_yellow_lane_time.start_hour2) &&\
                        (min <=m_yellow_lane_time.end_min2))
                    {
                        ret = 0;
                    }
                    else if((hour < m_yellow_lane_time.end_hour1) && (hour = m_yellow_lane_time.start_hour1) &&\
                        (min >=m_yellow_lane_time.start_min1))
                    {
                        ret = 0;
                    } 
                    else if((hour < m_yellow_lane_time.end_hour2) && (hour = m_yellow_lane_time.start_hour2) &&\
                        (min >=m_yellow_lane_time.start_min2))
                    {
                        ret = 0;
                    }
                    
                }
                break;            
            case 2: //!pink
                if((week <=m_pink_lane_time.end_week) && (week >=m_pink_lane_time.start_week) )
                {
                    if (((hour < m_pink_lane_time.end_hour1) && (hour > m_pink_lane_time.start_hour1)))
                    {

                        ret = 0;

                    }//!start == end hour
                    else if((hour = m_yellow_lane_time.end_hour1) && (hour = m_yellow_lane_time.start_hour1) &&\
                        (min <=m_yellow_lane_time.end_min1)&& (min >=m_yellow_lane_time.start_min1))
                    {
                        ret = 0;
                    } //! start != end hour
                    else if((hour = m_yellow_lane_time.end_hour1) && (hour > m_yellow_lane_time.start_hour1) &&\
                        (min <=m_yellow_lane_time.end_min1))
                    {
                        ret = 0;
                    } 
                    else if((hour < m_yellow_lane_time.end_hour1) && (hour = m_yellow_lane_time.start_hour1) &&\
                        (min >=m_yellow_lane_time.start_min1))
                    {
                        ret = 0;
                    } 

                }
                break;
                ret = -1;
            default:
                break;
    }
    return ret;
}


//! get one frame from vpss
/*
* frame_info: output, the snap result
*/

int CHiIve::ive_obtain_osd(hi_snap_osd_t * snap_osd, int osd_num)
{
    if (osd_num < 2)
    {
        DEBUG_ERROR("osd num(%d) is invalid!\n", osd_num);
        return -1;
    }
    if(0 == m_osd[0])
    {
        strncpy(snap_osd[0].szStr, "000.000.0000E 000.000.0000N",32);
        //strncpy(snap_osd[0].szStr, "Location invalid",32);
        snap_osd[0].szStr[32-1] = '\0';  
    }
    else
    {
        strncpy(snap_osd[0].szStr, m_osd,32);
        snap_osd[0].szStr[32-1] = '\0';  
    }
    datetime_t date_time;
    pgAvDeviceInfo->Adi_get_date_time(&date_time);
    snprintf(snap_osd[1].szStr,32, "20%02d-%02d-%02d", date_time.year, date_time.month, date_time.day);    
    snprintf(snap_osd[1].szStr, 32,"%s %02d:%02d:%02d", snap_osd[1].szStr, date_time.hour, date_time.minute, date_time.second);

    return 0;    
}

int CHiIve::ive_single_snap(VIDEO_FRAME_INFO_S * frame_info, int phy_chn)
{
    int ret = -1;
    if(NULL == frame_info)
    {
        return -1;
    }
    else
    {
        CHiVpss* hivpss = static_cast<CHiVpss *>(m_pData_source);
        if(0 != hivpss->HiVpss_get_frame( _VP_MAIN_STREAM_,phy_chn, frame_info))
        {
            DEBUG_ERROR("snap frame failed ! \n");
            return -1;
        }       
        ret = 0;
    }
    return ret;
}
int CHiIve::ive_single_snap(unsigned int snap_num)
{
    if(snap_num > 5)
    {
        DEBUG_ERROR("The snap num surpasses the limit!\n");
        return -1;
    }
    int ret = -1;
    VIDEO_FRAME_INFO_S frame_info;
    int phy_chn = 0;
    CHiVpss* hivpss = static_cast<CHiVpss *>(m_pData_source);

    for (unsigned int num = 0; num < snap_num; num++)
    {
        ret= hivpss->HiVpss_get_frame( _VP_MAIN_STREAM_,phy_chn, &frame_info);
        if(ret != 0)
        {
            DEBUG_ERROR("snap frame failed ! \n");
            ret =  -1;
            break;
        } 
        else
        {        
            m_vFrames.push_back(frame_info);
            ret = 0;
        }
    }
    return ret;
}

int CHiIve::ive_clear_snap(VIDEO_FRAME_INFO_S * frame_info, int phy_chn)
{
    int ret = -1;
    if(NULL == frame_info)
    {
        DEBUG_ERROR("input is null\n");
        return -1;
    }
    else
    {       
        CHiVpss* hivpss = static_cast<CHiVpss *>(m_pData_source);

        if(0 != hivpss->HiVpss_release_frame(_VP_MAIN_STREAM_,phy_chn, frame_info))
        {
            DEBUG_ERROR("vpss release frame failed ! \n");
            ret = -1;
        }

    }
    return ret;
}

int CHiIve::ive_clear_snap()
{
    int ret = -1;

    int phy_chn = 0;
    
    CHiVpss* hivpss = static_cast<CHiVpss *>(m_pData_source);

    for(unsigned int frame_num = 0; frame_num< m_vFrames.size();frame_num++)
    {      
        if(0 != hivpss->HiVpss_release_frame(_VP_MAIN_STREAM_,phy_chn, &(m_vFrames[frame_num])))
        {
            DEBUG_ERROR("vpss release frame failed ! \n");
            ret = -1;
            continue;
        }
    }
    m_vFrames.clear();
    ret = 0;
    return ret;
}

//! stich frames 

/*
int CHiIve::ive_stich_frames()
{
    
}*/
int CHiIve::ive_ble_process_frame_for_SZGJ(int phy_chn)
{
#if 1
    int ret = 0;
    bool release_falg = true;
    CHiVpss* hivpss = static_cast<CHiVpss *>(m_pData_source);
    VIDEO_FRAME_INFO_S frame_1080p;
    VIDEO_FRAME_INFO_S  frame_540p;
    IVE_IMAGE_S image_1080p;
    IVE_IMAGE_S image_540p;
    
    memset(&frame_1080p, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&frame_540p, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&image_1080p, 0, sizeof(IVE_IMAGE_S));
    memset(&image_540p, 0, sizeof(IVE_IMAGE_S));

    if(0 != hivpss->HiVpss_get_frame( _VP_SUB_STREAM_,phy_chn, &frame_1080p))
    {
        DEBUG_ERROR("vpss get frame failed ! \n");
        return -1;
    }
        
    if(0 != hivpss->HiVpss_get_frame( _VP_SPEC_USE_,phy_chn, &frame_540p))
    {
        DEBUG_ERROR("vpss get frame failed ! \n");
        if(0 != hivpss->HiVpss_release_frame(_VP_SUB_STREAM_, phy_chn, &frame_1080p))
        {
            DEBUG_ERROR("vpss release frame failed ! \n");
        }
        return -1;
    }

    image_1080p.enType = (frame_1080p.stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_422)?IVE_IMAGE_TYPE_YUV422SP:IVE_IMAGE_TYPE_YUV420SP;
    for(int i=0; i<3; ++i)
    {
        image_1080p.u32PhyAddr[i] = frame_1080p.stVFrame.u32PhyAddr[i];
        image_1080p.u16Stride[i] = (HI_U16)frame_1080p.stVFrame.u32Stride[i];
        image_1080p.pu8VirAddr[i]= (HI_U8*)frame_1080p.stVFrame.pVirAddr[i];
    }

    image_1080p.u16Width = (HI_U16)frame_1080p.stVFrame.u32Width;
    image_1080p.u16Height = (HI_U16)frame_1080p.stVFrame.u32Height;

    image_540p.enType = (frame_540p.stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_422)?IVE_IMAGE_TYPE_YUV422SP:IVE_IMAGE_TYPE_YUV420SP;

    for(int i=0; i<3; ++i)
    {
        image_540p.u32PhyAddr[i] = frame_540p.stVFrame.u32PhyAddr[i];
        image_540p.u16Stride[i] = (HI_U16)frame_540p.stVFrame.u32Stride[i];
        image_540p.pu8VirAddr[i]= (HI_U8*)frame_540p.stVFrame.pVirAddr[i];
    }
    image_540p.u16Width = (HI_U16)frame_540p.stVFrame.u32Width;
    image_540p.u16Height = (HI_U16)frame_540p.stVFrame.u32Height;    

    do
    {
        //PlateInfo plate_infos;
        LaneEnforcementInfo plate_infos;
        memset(&plate_infos, 0, sizeof(LaneEnforcementInfo));
#if 1
        if(HI_SUCCESS != BusLaneEnforcementStart(&image_1080p, &image_540p, &plate_infos))
        {
            DEBUG_ERROR("IveAlgStart  failed! \n");
            BusLaneEnforcementParametersReset((char *)"./");
            ret = -1;
            break;
        }
#endif
        //if(1 == pgAvDeviceInfo->Adi_get_ive_debug_mode())
        {
            if(m_pEncoder != NULL)    
            {        
                m_pEncoder->Ae_send_frame_to_encoder(_AST_SUB_VIDEO_, 0, (void *)&frame_540p, -1); 
            }
        }
        //! osd display

        switch(plate_infos.plate_recognition_flag)
        {
            case EN_PLATE_UPDATE:
                if(NULL != m_detection_result_notify_func)
                {
                    msgIPCIntelligentInfoNotice_t notice;
                    memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                    notice.cmdtype = 1;
                    notice.happen = 0;
                    notice.reserved[0] = 0;
                    int i = 0;

                    memset(notice.datainfo.carnum[i], 0, sizeof(notice.datainfo.carnum[i]));

                    memcpy(m_snap_osd[4].szStr, plate_infos.plate_num, sizeof(m_snap_osd[4].szStr)) ;

                    strncpy((char *)notice.datainfo.carnum[i], plate_infos.plate_num, strlen(plate_infos.plate_num));

                    notice.happentime = N9M_TMGetShareSecond();
                    m_detection_result_notify_func(&notice);
                    m_bCurrent_detection_state = 1;
                }
                break;
            case EN_PLATE_EMPTY:
                if(NULL != m_detection_result_notify_func)
                {
                    msgIPCIntelligentInfoNotice_t notice;
                    memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                    notice.cmdtype = 1;
                    notice.happen = 0;
                    notice.reserved[0] = 0;
                    int i = 0;

                    memset(notice.datainfo.carnum[i], 0, sizeof(notice.datainfo.carnum[i]));
                    //strncpy((char *)notice.datainfo.carnum[i], plate_infos.plate_num, strlen(plate_infos.plate_num));
                    memset(m_snap_osd[4].szStr, 0, sizeof(m_snap_osd[4].szStr)) ;
                    notice.happentime = N9M_TMGetShareSecond();
                    m_detection_result_notify_func(&notice);
                    m_bCurrent_detection_state = 1;
                }
                break;
            default:
                break;
            }

//! snap operation
        unsigned long long int snap_subtype = 0;
        VIDEO_FRAME_INFO_S single_frame;
        memset(&single_frame,0,sizeof(single_frame));

        switch(plate_infos.snap_type)
        {
            case EN_SNAP:
                DEBUG_CRITICAL("single snap\n");
                ive_single_snap(&single_frame,0);
                m_vFrames.push_back(single_frame);

                ive_obtain_osd(m_snap_osd,2);
                
                m_vSnap_osd.push_back(m_snap_osd[0]);
                m_vSnap_osd.push_back(m_snap_osd[1]);
                if ((m_single_snapped ==0)&&(plate_infos.roi_rect_info.u32Width!=0)&&(plate_infos.roi_rect_info.u32Height!=0))
                {
                    roi = plate_infos.roi_rect_info;    //!< obtain roi info from the first snap
                }

                m_single_snapped = 1;
                break;
            case EN_SZ_BUS_STOP:
                DEBUG_CRITICAL("EN_SZ_BUS_STOP \n");
                //if (plate_infos.plate_recognition_flag)
                {
                    if(( 0 == m_single_snapped)||(m_vFrames.size()<2))
                    {
                        ive_clear_snap();
                        m_vSnap_osd.clear();
                        DEBUG_CRITICAL("Invalid cmd!Single snap times incorrect!\n");
                        break;
                    }              
                    //ive_single_snap(&multi_frame[1],0); //!< second snap
                    ive_single_snap(&single_frame,0);
                    m_vFrames.push_back(single_frame);

                    snap_subtype = 0x08;
                    //! obtain osd info
                    ive_obtain_osd(&m_snap_osd[2],2);
                    memset(m_snap_osd[5].szStr, 0, sizeof(m_snap_osd[5].szStr));

                    memcpy(m_snap_osd[5].szStr, "SZ_BUS_STOP:", sizeof("SZ_BUS_STOP:")) ;
                    memcpy(m_snap_osd[5].szStr + strlen("SZ_BUS_STOP:"), m_snap_osd[4].szStr, strlen(m_snap_osd[5].szStr) - strlen("SZ_BUS_STOP:")) ;

                    //memcpy(m_snap_osd[4].szStr, plate_infos.plate_num, sizeof(m_snap_osd[4].szStr)) ;

                    m_vSnap_osd.push_back(m_snap_osd[2]);
                    m_vSnap_osd.push_back(m_snap_osd[3]);
                    m_vSnap_osd.push_back(m_snap_osd[5]);
                    //end of osd info

                    if (ive_stich_snap_for_singapore_bus(2,m_snap_osd,7,roi,snap_subtype) != 0)
                    {
                        DEBUG_ERROR("Stich failed!\n");
                        ive_clear_snap();
                        m_vSnap_osd.clear();
                        //m_vFrames.clear(); //! already clear in ive_clear_snap
                        break;
                    }
                    if(NULL != m_detection_result_notify_func)
                    {
                        msgIPCIntelligentInfoNotice_t notice;
                        memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                        notice.cmdtype = 1;
                        notice.happen = 1;  
                        notice.reserved[0] = 0;
                    	int i = 0;

                        memset(notice.datainfo.carnum[i], 0, sizeof(notice.datainfo.carnum[i]));
                        strncpy((char *)notice.datainfo.carnum[i], plate_infos.plate_num, strlen(plate_infos.plate_num));

                            notice.happentime = N9M_TMGetShareSecond();
                            m_detection_result_notify_func(&notice);
                            m_bCurrent_detection_state = 1;
                        } 
                    }
                        break;
                    case EN_SZ_BUS_LANE:
                        DEBUG_CRITICAL("EN_SZ_BUS_LANE \n");
                        //if (plate_infos.plate_recognition_flag)
                        {
                            if(( 0 == m_single_snapped)||(m_vFrames.size()!= 2))
                            {
                                ive_clear_snap();
                                m_vSnap_osd.clear();                	
                                DEBUG_CRITICAL("Invalid cmd!Single snap times incorrect!\n");
                                break;
                            }

                            ive_single_snap(&single_frame,0);
                            m_vFrames.push_back(single_frame); //!< last snap, before which 2 snapped
                            snap_subtype = 0x08;

                            //! obtain osd info
                            ive_obtain_osd(&m_snap_osd[2],2);
                            int spec_len = strlen("SZ_BUS_LANE:");
                            if (spec_len > 30)
                            {
                                spec_len = 30;                
                            }
                            memset(m_snap_osd[5].szStr, 0, sizeof(m_snap_osd[5].szStr));

                            memcpy(m_snap_osd[5].szStr, "SZ_BUS_LANE:", spec_len) ;
                            memcpy(m_snap_osd[5].szStr + spec_len, m_snap_osd[4].szStr, sizeof(m_snap_osd[5].szStr) - spec_len) ;

                            m_vSnap_osd.push_back(m_snap_osd[2]);
                            m_vSnap_osd.push_back(m_snap_osd[3]);
                            m_vSnap_osd.push_back(m_snap_osd[5]);
                            //end of osd info
                            //DEBUG_CRITICAL("size of osd:%d, last osd:%s\n", m_vSnap_osd.size(),m_snap_osd[4].szStr);
                            if (ive_stich_snap_for_singapore_bus(2,m_snap_osd,7,roi,snap_subtype) != 0)
                            {
                                DEBUG_ERROR("Stich failed!\n");
                                ive_clear_snap();
                                m_vSnap_osd.clear();
                                //m_vFrames.clear();
                                break;
                            }
                            //! notify network
                            if(NULL != m_detection_result_notify_func)
                            {
                                msgIPCIntelligentInfoNotice_t notice;
                                memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                                notice.cmdtype = 1;
                                notice.happen = 1;  
                                notice.reserved[0] = 0;
                                int i = 0;

                                memset(notice.datainfo.carnum[i], 0, sizeof(notice.datainfo.carnum[i]));
                                strncpy((char *)notice.datainfo.carnum[i], plate_infos.plate_num, strlen(plate_infos.plate_num));

                                notice.happentime = N9M_TMGetShareSecond();
                                m_detection_result_notify_func(&notice);
                                m_bCurrent_detection_state = 1;
                        }
                    }
                    break;
                case EN_EMPTY_BUFFER:
                    DEBUG_CRITICAL("EN_EMPTY_BUFFER\n");
                    ive_clear_snap();
                    m_vSnap_osd.clear();
                    //m_vFrames.clear();
                    m_single_snapped = 0;
                    break;
                default:
                    break;
                }

    }while(0);

    if(release_falg)
    {
        if(0 != hivpss->HiVpss_release_frame(_VP_SUB_STREAM_,phy_chn, &frame_1080p))
        {
            DEBUG_ERROR("vpss release 540p frame failed ! \n");
            ret = -1;
        }
    }
    
    if(0 != hivpss->HiVpss_release_frame(_VP_SPEC_USE_,phy_chn, &frame_540p))
    {
        DEBUG_ERROR("vpss release 1080P frame failed ! \n");
        ret = -1;
    }

    return ret;
#else
     
#endif
}

#define OSD_VERTICAL_INTERVAL 50
#define DATA_TIME_OSD_X   (1200)
#define DATA_TIME_OSD_Y    (10)
#define GPS_OSD_X  (1200)
#define GPS_OSD_Y  (DATA_TIME_OSD_Y+OSD_VERTICAL_INTERVAL)
#define VEHICLE_OSD_SATRT_X  (40)
#define VEHICLE_OSD_START_Y   (10)

int CHiIve::ive_ble_process_frame_for_CQJY(int phy_chn)
{
    int ret = 0;
    CHiVpss* hivpss = static_cast<CHiVpss *>(m_pData_source);
    VIDEO_FRAME_INFO_S frame_720p;
    VIDEO_FRAME_INFO_S  frame_540p;
    IVE_IMAGE_S image_720p;
    IVE_IMAGE_S image_540p;
        
    memset(&frame_720p, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&frame_540p, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&image_720p, 0, sizeof(IVE_IMAGE_S));
    memset(&image_540p, 0, sizeof(IVE_IMAGE_S));
    
    if(0 != hivpss->HiVpss_get_frame( _VP_MAIN_STREAM_,phy_chn, &frame_720p))
    {
        DEBUG_ERROR("vpss get frame failed ! \n");
        return -1;
    }
            
        if(0 != hivpss->HiVpss_get_frame( _VP_SPEC_USE_,phy_chn, &frame_540p))
        {
            DEBUG_ERROR("vpss get frame failed ! \n");
            if(0 != hivpss->HiVpss_release_frame(_VP_MAIN_STREAM_, phy_chn, &frame_720p))
            {
                DEBUG_ERROR("vpss release frame failed ! \n");
            }
            return -1;
        }
    
        image_720p.enType = (frame_720p.stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_422)?IVE_IMAGE_TYPE_YUV422SP:IVE_IMAGE_TYPE_YUV420SP;
        for(int i=0; i<3; ++i)
        {
            image_720p.u32PhyAddr[i] = frame_720p.stVFrame.u32PhyAddr[i];
            image_720p.u16Stride[i] = (HI_U16)frame_720p.stVFrame.u32Stride[i];
            image_720p.pu8VirAddr[i]= (HI_U8*)frame_720p.stVFrame.pVirAddr[i];
        }
    
        image_720p.u16Width = (HI_U16)frame_720p.stVFrame.u32Width;
        image_720p.u16Height = (HI_U16)frame_720p.stVFrame.u32Height;
    
        image_540p.enType = (frame_540p.stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_422)?IVE_IMAGE_TYPE_YUV422SP:IVE_IMAGE_TYPE_YUV420SP;
        for(int i=0; i<3; ++i)
        {
            image_540p.u32PhyAddr[i] = frame_540p.stVFrame.u32PhyAddr[i];
            image_540p.u16Stride[i] = (HI_U16)frame_540p.stVFrame.u32Stride[i];
            image_540p.pu8VirAddr[i]= (HI_U8*)frame_540p.stVFrame.pVirAddr[i];
        }
        image_540p.u16Width = (HI_U16)frame_540p.stVFrame.u32Width;
        image_540p.u16Height = (HI_U16)frame_540p.stVFrame.u32Height;    
    
        do
        {
            //PlateInfo plate_infos;
            LaneEnforcementInfo plate_infos;
            memset(&plate_infos, 0, sizeof(LaneEnforcementInfo));
            if(HI_SUCCESS != BusLaneEnforcementStart(&image_720p, &image_540p, &plate_infos))
            {
                DEBUG_ERROR("IveAlgStart  failed! \n");
                BusLaneEnforcementParametersReset((char *)"./");
                ret = -1;
                break;
            }
            if(1 == pgAvDeviceInfo->Adi_get_ive_debug_mode())
            {
                if(m_pEncoder != NULL)    
                {        
                    m_pEncoder->Ae_send_frame_to_encoder(_AST_SUB_VIDEO_, 0, (void *)&frame_540p, -1); 
                }
            }           
    
            VIDEO_FRAME_INFO_S single_frame;
            memset(&single_frame,0,sizeof(single_frame));
#if 0
            plate_infos.license_plate_num = 3;
            plate_infos.snap_type = EN_SNAP;
            char vehicles[45] = {0xE6,0xB8,0x9D,0x41,0x30,0x34,0x36,0x35,0x32, \
                0xE6,0xB8,0x9D,0x42,0x30,0x34,0x36,0x35,0x32, 0xE6,0xB8,0x9D,0x43,0x30,0x34,0x36,0x35,0x32,0x0};
            memcpy(plate_infos.plate_num, vehicles, sizeof(plate_infos.plate_num));
#endif    
            switch(plate_infos.plate_recognition_flag)
            {
                case EN_PLATE_UPDATE:
                {
                    DEBUG_MESSAGE("EN_PLATE_UPDATE\n");
                    ive_single_snap(&single_frame,0);
                    
                    ive_obtain_osd(m_snap_osd,2);
                    m_snap_osd[0].x = GPS_OSD_X;
                    m_snap_osd[0].y = GPS_OSD_Y;
                    m_snap_osd[1].x = DATA_TIME_OSD_X;
                    m_snap_osd[1].y = DATA_TIME_OSD_Y;                   
                    int osd_nums = plate_infos.license_plate_num>PLATE_NUM_MAX? PLATE_NUM_MAX: plate_infos.license_plate_num;
                    for(int i=0; i<osd_nums;++i)
                    {
                        strncpy(m_snap_osd[i+2].szStr, plate_infos.plate_num+i*PLATE_CHAR_NUM_MAX, PLATE_CHAR_NUM_MAX);
                        m_snap_osd[i+2].szStr[PLATE_CHAR_NUM_MAX] = '\0';
                        m_snap_osd[i+2].x = VEHICLE_OSD_SATRT_X;
                        m_snap_osd[i+2].y = VEHICLE_OSD_START_Y + i*OSD_VERTICAL_INTERVAL;
                    }
                    ive_single_snap(&single_frame, m_snap_osd, osd_nums+2);
                    
                    if(NULL != m_detection_result_notify_func)
                    {
                        msgIPCIntelligentInfoNotice_t notice;
                        memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                        notice.cmdtype = 1;
                        notice.happen = 1;
                        notice.reserved[0] = 1;
                        int i = 0;
                    
                        memset(notice.datainfo.carnum[i], 0, sizeof(notice.datainfo.carnum[i]));      
                        strncpy((char *)notice.datainfo.carnum[i], plate_infos.plate_num, strlen(plate_infos.plate_num));
                    
                        notice.happentime = N9M_TMGetShareSecond();
                        m_detection_result_notify_func(&notice);
                    }
                }
                    break;
               
                case EN_PLATE_EMPTY:
                    DEBUG_MESSAGE("EN_EMPTY_BUFFER\n");
                    //clear the sub stream osd
                    if(NULL != m_detection_result_notify_func)
                    {
                        msgIPCIntelligentInfoNotice_t notice;
                        memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                        notice.cmdtype = 1;
                        notice.happen = 0;
                        notice.reserved[0] = 1;
                    
                        notice.happentime = N9M_TMGetShareSecond();
                        m_detection_result_notify_func(&notice);
                    }

                    break;
                default:
                    break;
                    }
    
        }while(0);
    
        if(0 != hivpss->HiVpss_release_frame(_VP_MAIN_STREAM_,phy_chn, &frame_720p))
        {
            DEBUG_ERROR("vpss release 720P frame failed ! \n");
            ret = -1;
        }
    
        if(0 != hivpss->HiVpss_release_frame(_VP_SPEC_USE_,phy_chn, &frame_540p))
        {
            DEBUG_ERROR("vpss release 540p frame failed ! \n");
            ret = -1;
        }
    
        return ret;
}



//! stich two fames with roi region, osd and other info
int CHiIve::ive_stich_snap_for_singapore_bus( int frame_num, //!< input frame num, default 2 
                                                        hi_snap_osd_t * snap_osd,int osd_num,             //!< input osd info
                                                        RECT_S roi,                                       //!< osd region 
                                                        unsigned long long int snap_subtype)             //!< 

{
    //! check the arguments
    DEBUG_ERROR("Roi info,x:%d, y:%d, width:%d,height:%d\n", roi.s32X, roi.s32Y, roi.u32Width, roi.u32Height);
    if((roi.s32X < 0)||(roi.s32Y < 0)||(roi.u32Height < 50)||(roi.u32Width<50))
    {
        roi.u32Height =  50;
        roi.u32Width =  50;
        DEBUG_ERROR("Invalid roi info,x:%d, y:%d, width:%d,height:%d\n", roi.s32X, roi.s32Y, roi.u32Width, roi.u32Height);
        //return -1;
    }

    if (( m_vFrames.size() != 3)||(m_vSnap_osd.size() != 7))
    {
    	DEBUG_ERROR("Invalid frames snapped!\n");
    	return -1;
    }
    int ret = 0; 
    //int osd_height = 40;
    
    int x = roi.s32X;
    int y = roi.s32Y;
    int crop_width = roi.u32Width;
    int crop_height = roi.u32Height;
        
    VGS_HANDLE vgs_handle;
    VGS_TASK_ATTR_S vgs_task;
   // FILE* pfile = NULL;
   // HI_CHAR file_name[128];

    VIDEO_FRAME_INFO_S snap_frame;
    VIDEO_FRAME_INFO_S  dest_frame;
    VIDEO_FRAME_INFO_S src_frame;
    
    HI_U32 luma_addr_offset = 0;
    HI_U32 chroma_addr_offset = 0;
    //! application related
    HI_U32 snap_width = m_vFrames[0].stVFrame.u32Width;  //!< final resulted frame size after stich
    HI_U32 snap_height = m_vFrames[0].stVFrame.u32Height;
    int roi_scale = snap_width/640;

    //x *=roi_scale;
    //y *=roi_scale;
    //crop_width *=roi_scale;
    //crop_height *=roi_scale;
    crop_width = snap_width/2; //! same as height or rectangle
    crop_height = snap_height/2; 
    
    crop_width = (crop_width+1)/4 *4;
    crop_height = (crop_height + 1)/4 *4;
    
    x -= crop_width / roi_scale /2;
    y -= crop_height / roi_scale /2;
    
    x *=roi_scale;
    y *=roi_scale;
    if (x<0)
    {
    	x=0;
	}
    if (y<0)
    {
    	y=0;
	}
	x = (x+1)/4 *4;
	y = (y + 1)/4 *4;

    
    DEBUG_CRITICAL("snap wid:%d, snap height:%d, crop wid:%d, crop height:%d, roi scale:%d, x:%d, y:%d\n", \
    				snap_width, snap_height,crop_width,crop_height,roi_scale,x,y);
    //! the single frame size before encoder
    HI_U32 scaled_width =  snap_width/2;  
    HI_U32 scaled_height =  snap_height/2;
    
    rm_uint64_t after_scale = 0;
    rm_uint64_t befor_scale = 0;

    memset(&snap_frame, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&vgs_task, 0, sizeof(VGS_TASK_ATTR_S));

    memset(&dest_frame, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&src_frame, 0, sizeof(VIDEO_FRAME_INFO_S));
    
    memset(m_u32Frame_vir_addr, 0, snap_width*snap_height);
    memset(m_u32Frame_vir_addr + snap_width*snap_height, 128, snap_width*snap_height/2); 
    
    befor_scale = N9M_TMGetSystemMicroSecond();
    
//! copy frames

    //copy 1st 2nd 3th frame
    for(unsigned int i=0; i<4; i++)
    {
        //printf("[alex]copy the data of frame:%d \n", i);
        //luma_addr_offset = (i/2)*IVE_BLE_SNAP_PICTURE_WIDTH*IVE_BLE_SCALE_FRAME_HEIGHT + (i%2)*IVE_BLE_SCALE_FRAME_WIDTH;
        //chroma_addr_offset = IVE_BLE_SNAP_PICTURE_WIDTH*IVE_BLE_SNAP_PICTURE_HEIGHT + (i/2)*IVE_BLE_SNAP_PICTURE_WIDTH*IVE_BLE_SCALE_FRAME_HEIGHT/2 + (i%2)*IVE_BLE_SCALE_FRAME_WIDTH;

        luma_addr_offset = (i/2)*snap_width*scaled_height + (i%2)*scaled_width;
        chroma_addr_offset = snap_width*snap_height + (i/2)*snap_width*scaled_height/2 + (i%2)*scaled_width;
    
        if(3 == i)
        {
            //! roi region is obtained from first frame
            memcpy(&src_frame, &m_vFrames[0], sizeof(VIDEO_FRAME_INFO_S));
            src_frame.stVFrame.u32PhyAddr[0] = m_vFrames[0].stVFrame.u32PhyAddr[0] +  m_vFrames[0].stVFrame.u32Stride[0]*y+x;
            src_frame.stVFrame.u32PhyAddr[1] = m_vFrames[0].stVFrame.u32PhyAddr[1] + m_vFrames[0].stVFrame.u32Stride[1]*y/2+x;
            src_frame.stVFrame.pVirAddr[0] = (HI_VOID *)((HI_U32)m_vFrames[0].stVFrame.pVirAddr[0] + m_vFrames[0].stVFrame.u32Stride[0]*y+x);
            src_frame.stVFrame.pVirAddr[1] = (HI_VOID *)((HI_U32)m_vFrames[0].stVFrame.pVirAddr[1] + m_vFrames[0].stVFrame.u32Stride[1]*y/2+x);
            src_frame.stVFrame.u32Width = crop_width;
            src_frame.stVFrame.u32Height = crop_height;
        }
        
        dest_frame.stVFrame.u32PhyAddr[0] = m_u32Frame_phy_addr + luma_addr_offset;        //luma component
        dest_frame.stVFrame.u32PhyAddr[1] = m_u32Frame_phy_addr + chroma_addr_offset;
        dest_frame.stVFrame.pVirAddr[0] = m_u32Frame_vir_addr + luma_addr_offset;
        dest_frame.stVFrame.pVirAddr[1] = m_u32Frame_vir_addr + chroma_addr_offset;
        if(3 == i)
        {
            dest_frame.stVFrame.u32Width = scaled_height*crop_width/crop_height;//1280;//scale_width;
            dest_frame.stVFrame.u32Height = scaled_height;

        }
        else
        {
            dest_frame.stVFrame.u32Width = scaled_width;//frame_snaped[i].stVFrame.u32Width;
            dest_frame.stVFrame.u32Height = scaled_height;//frame_snaped[i].stVFrame.u32Height;
        }
        dest_frame.stVFrame.u32Stride[0] =  snap_width; 
        dest_frame.stVFrame.u32Stride[1] = snap_width;
        dest_frame.stVFrame.enCompressMode = COMPRESS_MODE_NONE;
        dest_frame.stVFrame.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        dest_frame.stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
        dest_frame.u32PoolId = m_u32Pool;
        
//!  employ video graphic system
        ret = HI_MPI_VGS_BeginJob(&vgs_handle);
        if(HI_SUCCESS != ret)
        {
            DEBUG_ERROR("HI_MPI_VGS_BeginJob failed! ,errorcode:0x%x \n", ret);
            break;
        }

        if(3 == i)
        {
            memcpy(&vgs_task.stImgIn, &src_frame.stVFrame, sizeof(VIDEO_FRAME_INFO_S));
        }
        else
        {
            memcpy(&vgs_task.stImgIn, &m_vFrames[i].stVFrame, sizeof(VIDEO_FRAME_INFO_S));
        }
        
        memcpy(&vgs_task.stImgOut , &dest_frame.stVFrame, sizeof(VIDEO_FRAME_INFO_S));
        ret = HI_MPI_VGS_AddScaleTask(vgs_handle, &vgs_task);
        if(HI_SUCCESS != ret)
        {
            DEBUG_ERROR("HI_MPI_VGS_AddScaleTask failed! errcode:0x%x ,inw:%d, inh:%d,outw:%d,outh:%d\n", ret,\
            vgs_task.stImgIn.stVFrame.u32Width, vgs_task.stImgIn.stVFrame.u32Height,\
            vgs_task.stImgOut.stVFrame.u32Width, vgs_task.stImgOut.stVFrame.u32Height);
            HI_MPI_VGS_CancelJob(vgs_handle);
        }
        ret = HI_MPI_VGS_EndJob(vgs_handle);
        if (ret != HI_SUCCESS)
        {
            DEBUG_ERROR("HI_MPI_VGS_EndJob failed! errcode:0x%x \n", ret);
            HI_MPI_VGS_CancelJob(vgs_handle);
        } 
    }
    
    snap_frame.stVFrame.u32Width = snap_width;
    snap_frame.stVFrame.u32Height = snap_height;
    snap_frame.stVFrame.u32Field = m_vFrames[0].stVFrame.u32Field;
    snap_frame.stVFrame.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    snap_frame.stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
    snap_frame.stVFrame.enCompressMode = COMPRESS_MODE_NONE;
    snap_frame.stVFrame.u32PhyAddr[0] = m_u32Frame_phy_addr;
    snap_frame.stVFrame.u32PhyAddr[1] = m_u32Frame_phy_addr + snap_width*snap_height;
    snap_frame.stVFrame.pVirAddr[0] = m_u32Frame_vir_addr;
    snap_frame.stVFrame.pVirAddr[1] = m_u32Frame_vir_addr + snap_width*snap_height;
    snap_frame.stVFrame.u32Stride[0] = snap_width;
    snap_frame.stVFrame.u32Stride[1] = snap_width;
    snap_frame.stVFrame.u64pts = m_vFrames[0].stVFrame.u64pts;
    snap_frame.stVFrame.u32TimeRef = m_vFrames[0].stVFrame.u32TimeRef;
    snap_frame.u32PoolId = m_u32Pool;

#if 0
    snprintf(file_name, sizeof(file_name), "./pic/snap.yuv");
    pfile = fopen(file_name, "wb");
    if(NULL != pfile)
    {
        sample_yuv_dump(&(snap_frame.stVFrame), pfile);
        fclose(pfile);
    }
#else

    ive_snap_task_s task;
    memset(&task, 0, sizeof(ive_snap_task_s));
    task.u8Osd_num = osd_num;
    task.u8Snap_type = 1;
    task.pOsd = new ive_snap_osd_display_attr_s[osd_num];

    if( osd_num == 5)
    {
        for (int i=0;i < 5;i++)
        {
            strncpy(task.pOsd[i].pContent, snap_osd[i].szStr, sizeof(task.pOsd[i].pContent)); 
        }
    //! first frame gps and time       
        HI_U32 len = strlen(task.pOsd[0].pContent);        
        HI_U32 font_width = 36;
        HI_U32 ox = 0;
        int t= scaled_width - font_width*len/2;
        if(t>0)
        {
            ox = (unsigned int)t;
        }
        else
        {
            ox = 0;
        }
        task.pOsd[0].u16X = ox;
        task.pOsd[0].u16Y = 150;   

        len = strlen(task.pOsd[1].pContent); 

        task.pOsd[1].u16X = ox;
        task.pOsd[1].u16Y = 50 ;

        //! second frame gps and time
        len = strlen(task.pOsd[2].pContent);
        t = scaled_width*2 - font_width*len/2;
        if(t> (int)scaled_width)
        {
            ox = (unsigned int)t;
        }
        else
        {
            ox = scaled_width;
        }
        task.pOsd[2].u16X = ox;
        task.pOsd[2].u16Y = 150;

        len = strlen(task.pOsd[3].pContent);      
        task.pOsd[3].u16X = ox;
        task.pOsd[3].u16Y = 50;

        //!special osd
        len = strlen(task.pOsd[4].pContent);
        t = scaled_width*2 - (scaled_width*2 - scaled_height*crop_width/crop_height) / 2 - font_width*len/2;
        if(t > (int)scaled_height)
        {
           ox = (unsigned int)t;
        }
        else
        {
            ox = scaled_height;
        }
        task.pOsd[4].u16X = ox;
        task.pOsd[4].u16Y = scaled_height + scaled_height/2;

    }
    else if(osd_num == 7)
    {
        for (int i=0;i < 7;i++)
        {
            strncpy(task.pOsd[i].pContent, m_vSnap_osd[i].szStr, sizeof(task.pOsd[i].pContent)); 
        }
        //! first frame gps and time       
            HI_U32 len = strlen(task.pOsd[0].pContent);        
            HI_U32 font_width = 36;
            HI_U32 ox = 0;
            int t= scaled_width - font_width*len/2;
            if(t>0)
            {
                ox = (unsigned int)t;
            }
            else
            {
                ox = 0;
            }
            task.pOsd[0].u16X = ox;
            task.pOsd[0].u16Y = 60;   
        
            len = strlen(task.pOsd[1].pContent); 
        
            task.pOsd[1].u16X = ox;
            task.pOsd[1].u16Y = 10 ;
        
            //! second frame gps and time
            len = strlen(task.pOsd[2].pContent);
            t = scaled_width*2 - font_width*len/2;
            if(t> (int)scaled_width)
            {
                ox = (unsigned int)t;
            }
            else
            {
                ox = scaled_width;
            }
            task.pOsd[2].u16X = ox;
            task.pOsd[2].u16Y = 60;
        
            len = strlen(task.pOsd[3].pContent);      
            task.pOsd[3].u16X = ox;
            task.pOsd[3].u16Y = 10;

            
            //! third frame gps and time
            len = strlen(task.pOsd[4].pContent);
            t = scaled_width - font_width*len/2;

            if(t> 0)
            {
                ox = (unsigned int)t;
            }
            else
            {
                ox = 0;
            }
            
            task.pOsd[4].u16X = ox;
            task.pOsd[4].u16Y= scaled_height + 60;
            
            len = strlen(task.pOsd[5].pContent);

            task.pOsd[5].u16X = ox;
            task.pOsd[5].u16Y= scaled_height + 10;
        
            //!special osd
            len = strlen(task.pOsd[6].pContent);
            t = scaled_width*2 - (scaled_width*2 - scaled_height*crop_width/crop_height) / 2 - font_width*len/2;
            if(t > (int)scaled_height)
            {
               ox = (unsigned int)t;
            }
            else
            {
                ox = scaled_height;
            }
            task.pOsd[6].u16X = ox;
            task.pOsd[6].u16Y = scaled_height + 10;

        m_vSnap_osd.clear();
    }
   // m_vFrames.clear();
    ive_clear_snap();
    task.pFrame = new ive_snap_video_frame_info_s;
    memcpy(task.pFrame, &snap_frame, sizeof(VIDEO_FRAME_INFO_S));
    DEBUG_MESSAGE("Send snap task! \n");
    m_ive_snap_thread->AddIveSnapTask(task);

    after_scale = N9M_TMGetSystemMicroSecond();
    DEBUG_ERROR("Do scale speed time:%llu \n", after_scale-befor_scale);

    return ret;  
}
#endif

int CHiIve::ive_single_snap(VIDEO_FRAME_INFO_S * frame_info, hi_snap_osd_t * snap_osd,int osd_num)
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
         task.u8Snap_subtype = 1;//vehicle
         task.pOsd = new ive_snap_osd_display_attr_s[osd_num];   
         for(int i=0; i<osd_num;++i)
         {
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
#endif

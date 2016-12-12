#include "HiIveFD.h"
#include "StreamBuffer.h"

using namespace Common;

#if 0
#define SAVE_LOCAL_PICTURE
#endif

#define IVE_CONFIG_PATH "/usr/local/app/extend/ive/bin/fd_config/face_detection_config.xml"

CHiIveFd::CHiIveFd(data_sorce_e data_source_type, data_type_e data_type, void* data_source, CHiAvEncoder* mpEncoder):
    CHiIve(data_source_type, data_type, data_source, mpEncoder)
{
    m_32Detect_handle = -1;
    m_bFace_detection_state = false;
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
}
    
CHiIveFd::~CHiIveFd()
{
    m_32Detect_handle = -1;
    m_bFace_detection_state = false;
    N9M_SHDestroyShareBuffer(m_hSnapShareMemHandle);
}

int CHiIveFd::ive_control(unsigned int ive_type, ive_ctl_type_e ctl_type, void* args)
{
    if(ive_type  & IVE_TYPE_FD)
    {
        switch(ctl_type)
        {
            case IVE_CTL_SAVE_RESULT:
            default:
            {
                
                int start_flag = (int)args;
                DEBUG_MESSAGE("the start flag is:%d \n", start_flag);
                if(start_flag)
                {
                    return fd_start_dectection();
                }
                else
                {
                    return fd_stop_dectection();
                }
            }
                break;
        }
    }
    return -1;    
}

int CHiIveFd::fd_start_dectection()
{
    DEBUG_MESSAGE("fd_start_dectection");
    m_bFace_detection_state = true;
    return 0;
}

int CHiIveFd::fd_stop_dectection()
{
    DEBUG_MESSAGE("fd_stop_dectection");
    m_bFace_detection_state = false;
    return 0;
}

int CHiIveFd::init()
{
    if(HI_SUCCESS !=  ObjectDetectionInit((char *)IVE_CONFIG_PATH, &m_32Detect_handle))
    {
        DEBUG_ERROR("ObjectDetectionInit failed!  \n");
        return -1;
    }
    
    return 0;
}


int CHiIveFd::uninit()
{
    if(HI_SUCCESS != ObjectCountingRelease())
    {
        DEBUG_ERROR("ObjectCountingRelease failed!  \n");
        return -1;
    }  
    return 0;
}

int CHiIveFd::process_body()
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

int CHiIveFd::process_raw_data_from_vi(int phy_chn)
{
    return 0;
}

int CHiIveFd::process_yuv_data_from_vi(int phy_chn)
{
    return 0;
}

int CHiIveFd::process_yuv_data_from_vpss(int phy_chn)
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
        DEBUG_MESSAGE("[FD]detected %d faces! \n", face_num);
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
    
    if(!m_bFace_detection_state)
    {
        goto END;
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

END:     
    if(1 == pgAvDeviceInfo->Adi_get_ive_debug_mode())
    {
        if(m_pEncoder != NULL)    
        {        
            m_pEncoder->Ae_send_frame_to_encoder(_AST_MAIN_VIDEO_, 0, (void *)&video_frame_1080p, -1); 
        }
     }
    
     if(0 != hivpss->HiVpss_release_frame(_VP_MAIN_STREAM_, phy_chn, &video_frame_1080p))
    {
        DEBUG_ERROR("vpss release 1080p frame failed ! \n");
        return -1;
    }      
     
    return 0; 
}

int CHiIveFd::ive_fd_face_snap(VIDEO_FRAME_INFO_S* frame, RECT_S * crop_rect)
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

int CHiIveFd::ive_fd_put_snap_frame_to_sm(VENC_STREAM_S* stream)
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


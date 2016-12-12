#include "ive_snap.h"

using namespace Common;


static pthread_t ive_thread_id; 

#if 0
#define SAVE_LOCAL_PICTURE
#endif


CIveSnap::CIveSnap(CHiAvEncoder* pEnc)
{
    m_pEnc = pEnc;
    m_bIve_encoder_created = false;
    m_bIs_ive_encoder_thread_start = false;
    
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
}

CIveSnap::~CIveSnap()
{
    StopIveSnapThread();
}

static void *ive_snapex_thread_body(void *args)
{
    CIveSnap* task = (CIveSnap*)args;
    task->IveSnapThreadRunEx();
    return NULL;
}

int CIveSnap::IveSnapThreadRunEx()
{
    ive_snap_task_s snap_info;
    VENC_STREAM_S stuStream;
    SnapShareMemoryHead_t stuPicHead;
    SShareMemData stuShareData;
    SShareMemFrameInfo stuShareInfo;    
    char customer[32];

    memset(customer, 0, sizeof(customer));
    pgAvDeviceInfo->Adi_get_customer_name(customer, sizeof(customer));
    
    memset(&stuPicHead, 0, sizeof(stuPicHead));
    memset(&stuShareInfo, 0, sizeof(stuShareInfo));
    memset(&stuShareData, 0, sizeof(SShareMemData));
    stuShareInfo.flag = SHAREMEM_FLAG_IFRAME;
    stuShareData.pstuPack = new SShareMemData::_framePack_[_AV_MAX_PACK_NUM_ONEFRAME_ + 1];    
    
    while(m_bIs_ive_encoder_thread_start)
    {
        memset(&snap_info, 0, sizeof(ive_snap_task_s));
        if(SUCCESS == GetIveSnapTask(snap_info))
        {
            DEBUG_WARNING("Got snap task video width:%d.height:%d, stride:%d\n", snap_info.pFrame->stVFrame.u32Width, 
                snap_info.pFrame->stVFrame.u32Height, snap_info.pFrame->stVFrame.u32Stride[0]);
            
            if (CreateIveSnapEncoder(snap_info.pFrame->stVFrame.u32Width, snap_info.pFrame->stVFrame.u32Height, snap_info.pOsd, snap_info.u8Osd_num)!=0)
            {
                DEBUG_ERROR("CreateIveSnapEncoder failed!\n");
                continue;    
            }
            memset(&stuStream, 0, sizeof(VENC_STREAM_S));
                
            if(0 == m_pEnc->Ae_encode_frame_to_picture(snap_info.pFrame, snap_info.u8Osd_num, snap_info.pOsd, &stuStream))
            {
                datetime_t stuTime; 

                memset(&stuTime, 0, sizeof(datetime_t));
                pgAvDeviceInfo->Adi_get_date_time( &stuTime );
                stuPicHead.stuSnapAttachedFile.ucSnapType = 6;//Smart capture
                stuPicHead.stuSnapAttachedFile.ucSnapChn = 0;
                stuPicHead.stuSnapAttachedFile.uiFileSize = 0;//stuFrameInfo.stVFrame.u32Width*stuFrameInfo.stVFrame.u32Height*1.5;
                stuPicHead.stuSnapAttachedFile.stuDateTime = stuTime;
                stuPicHead.stuSnapAttachedFile.ullSubType = snap_info.u8Snap_subtype;
                stuPicHead.stuSnapAttachedFile.ucDeleteFlag = 0;
                stuPicHead.stuSnapAttachedFile.ucLockFlag = 0;
                stuPicHead.stuSnapAttachedFile.uiUserFlag = 4;
                if(0 == strcmp(customer, "CUSTOMER_CQJY"))
                {
                    stuPicHead.stuSnapAttachedFile.ucPrivateDataType = 6;
                    memset(stuPicHead.stuSnapAttachedFile.ucPrivateData, 0, sizeof(stuPicHead.stuSnapAttachedFile.ucPrivateData));
                    for(int k=0; k<snap_info.u8Osd_num;++k)
                    {
                        if(snap_info.pOsd[k].u8type == _AON_BUS_NUMBER_)
                        {
                            if(0 != strlen((char *)stuPicHead.stuSnapAttachedFile.ucPrivateData))
                            {
                                strncat((char *)stuPicHead.stuSnapAttachedFile.ucPrivateData, "_", 1);
                            }
                            strncat((char *)stuPicHead.stuSnapAttachedFile.ucPrivateData, snap_info.pOsd[k].pContent, strlen(snap_info.pOsd[k].pContent));
                        }  
                    }
                    DEBUG_MESSAGE("the private data is:%s \n", (char *)stuPicHead.stuSnapAttachedFile.ucPrivateData);
                }
                stuShareData.iPackCount = 0;
                stuShareData.pstuPack[stuShareData.iPackCount].pAddr = (char *)&stuPicHead;
                stuShareData.pstuPack[stuShareData.iPackCount].iLen = sizeof(stuPicHead);
                ++stuShareData.iPackCount;

#ifdef SAVE_LOCAL_PICTURE
                static unsigned int u32Picture_num = 0;
                char szFileName[20];
                snprintf( szFileName, sizeof(szFileName), "./pic/%d.jpg", u32Picture_num++ );
                if(u32Picture_num > 10)
                {
                    u32Picture_num = 0;
                }
                FILE* pfile = fopen(szFileName, "wb");

#endif
                for(unsigned int i = 0; i < stuStream.u32PackCount; ++i)
                {
                    VENC_PACK_S * pstData = &stuStream.pstPack[i];
                    stuShareData.pstuPack[stuShareData.iPackCount].pAddr = (char *)(pstData->pu8Addr + pstData->u32Offset);
                    stuShareData.pstuPack[stuShareData.iPackCount].iLen = pstData->u32Len - pstData->u32Offset;
                    ++stuShareData.iPackCount;
                    
                    stuPicHead.stuSnapAttachedFile.uiFileSize += (pstData->u32Len - pstData->u32Offset);
#ifdef SAVE_LOCAL_PICTURE
                    fwrite( (char *)(pstData->pu8Addr + pstData->u32Offset), (pstData->u32Len - pstData->u32Offset), 1, pfile );
#endif 
                }
#ifdef SAVE_LOCAL_PICTURE  
                DEBUG_CRITICAL("############save file success, file size:%d \n", stuPicHead.stuSnapAttachedFile.uiFileSize);
                fclose(pfile);
#endif 
                N9M_SHPutOneFrame(m_hSnapShareMemHandle, &stuShareData, &stuShareInfo, stuPicHead.stuSnapAttachedFile.uiFileSize + sizeof(stuPicHead));
                m_pEnc->Ae_free_ive_snap_stream(&stuStream);
            }

            m_pEnc->Ae_destroy_ive_snap_encoder();
            m_bIve_encoder_created = false;

            if(m_recycle_function_ble!= NULL)
            {
                m_recycle_function_ble(&snap_info);
            }
        }
        usleep(10*1000);
    }

    return 0;
}

int CIveSnap::StartIveSnapThreadEx(boost::function< int (ive_snap_task_s *)> recycle_func)
{
    if(m_bIs_ive_encoder_thread_start == false)
    {
        m_recycle_function_ble = recycle_func;
        m_bIs_ive_encoder_thread_start = true;
        pthread_create(&ive_thread_id, NULL, ive_snapex_thread_body, this);
    }
    return 0;
}

int CIveSnap::AddIveSnapTask(ive_snap_task_s& snap_info)
{
    m_ive_snap_queue_ble.push_back(snap_info);

    return 0;
}

int CIveSnap::GetIveSnapTask(ive_snap_task_s& snap_info)
{
    if(!m_ive_snap_queue_ble.empty())
    {
        snap_info = m_ive_snap_queue_ble.front();
        m_ive_snap_queue_ble.pop_front();

        return 0;
    }

    return -1;
}

int CIveSnap::StopIveSnapThread()
{
    if(m_bIve_encoder_created)
    {
        DestroyIveSnapEncoder();
    } 
    
    if(m_bIs_ive_encoder_thread_start)
    {
        m_bIs_ive_encoder_thread_start = false;
        pthread_join(ive_thread_id, NULL);
    }

    return 0;
}

int CIveSnap::CreateIveSnapEncoder(unsigned int encoder_width, unsigned int encoder_height, ive_snap_osd_display_attr_s* pOsd, unsigned char osd_num)
{
    if(!m_bIve_encoder_created)
    {
        OVERLAY_ATTR_S osds[8];
        memset(osds, 0, sizeof(OVERLAY_ATTR_S)*8);
        av_resolution_t resolution = _AR_720_;
        
        if(encoder_width == 1920 && encoder_height == 1080)
        {
            resolution = _AR_1080_;
        }
        
        for(int i=0;i<osd_num;++i)
        { 
            osds[i].u32BgColor = 0x00;

            m_pEnc->Ae_get_osd_size(resolution,strlen(pOsd[i].pContent),&(osds[i].stSize));
        }
        if(0 != m_pEnc->Ae_create_ive_snap_encoder(encoder_width, encoder_height, osd_num, osds))
        {
            DEBUG_ERROR("create ive snap encoder failed! width:%d height:%d \n",encoder_width,encoder_height);
            return -1;
        }
        m_bIve_encoder_created = true;
    }
    return 0;
}


int CIveSnap::DestroyIveSnapEncoder()
{
    if(m_bIve_encoder_created)
    {
        return m_pEnc->Ae_destroy_ive_snap_encoder();
    }

    return 0;
}


#include "AvSnapTask.h"
//#include "SnapManage.h"
#include "AvCommonFunc.h"
#include "AvCommonType.h"
#include "SystemTime.h"
#include "CommonMacro.h"
#include <assert.h>
#include <uuidP.h>

#if !defined (_AV_PRODUCT_CLASS_IPC_)
#include "remote_operate_file_api.h"
#endif

using namespace Security;
using namespace Common;
#if 0
#define SAVE_PICTURE
#endif


typedef struct
{
    uint64_t SnapTime;//鎶撴媿寮€濮嬫椂闂达紝鍗曚綅姣
    uint8_t Channel;
    uint8_t Reserve[3];
    uint32_t PicDataLen;
    char PicData[0];
}__attribute__((packed)) SnapHead_t;

void *SnapThreadEntry( void *pThreadParam )
{

    CAvSnapTask* pTask = (CAvSnapTask*)pThreadParam;

	prctl(PR_SET_NAME, "SnapThreadEntry");
    pTask->SignalSnapThreadBody();
    DEBUG_MESSAGE( "##########SnapThreadEntry\n");
    return NULL;
}
#if defined(_AV_PLATFORM_HI3515_)
CAvSnapTask::CAvSnapTask( CHiVi* pVi, CHiAvEncoder* pEnc)
#else
CAvSnapTask::CAvSnapTask( CHiVi* pVi, CHiAvEncoder* pEnc, CHiVpss* pVpss )
#endif
{
    int max_chnnum = 0;
    m_bNeedExitThread = false;
    m_bIsTaskRunning = false;
    m_bCreatedSnap = false;
    m_bParaChange = false;
    m_pVi = pVi;
    m_pEnc = pEnc;
#if !defined(_AV_PLATFORM_HI3515_)
    m_pVpss = pVpss;
#endif
    m_analog_digital_flag = 0;
    m_pThread_lock = new CAvThreadLock;
    _AV_FATAL_((m_pAv_font_library = CAvFontLibrary::Afl_get_instance()) == NULL, "OUT OF MEMORY\n");
    assert( m_pVi && m_pEnc );
#ifdef _AV_PRODUCT_CLASS_HDVR_
    max_chnnum = pgAvDeviceInfo->Adi_max_channel_number();
#else
    max_chnnum = 1;
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_)
    m_pSignalSnapPara = NULL;
    for(unsigned int i=0; i<sizeof(m_bCommSnapParamChange)/sizeof(m_bCommSnapParamChange[0]); ++i)
    {
        m_bCommSnapParamChange[i] = false;
    }
#endif

    //char * chnm = GetShareMenLockState();
    for (int chn = 0; chn < max_chnnum; chn++)
    {         
        //DEBUG_MESSAGE("#################GetShareMenLockState chn %s\n", chnm);
        unsigned int stream_buffer_size = 0;
        unsigned int stream_buffer_frame = 0;
        char stream_buffer_name[32];
        
#ifdef _AV_PRODUCT_CLASS_HDVR_
        if(pgAvDeviceInfo->Adi_get_stream_buffer_info(BUFFER_TYPE_SNAP_STREAM, chn, stream_buffer_name, stream_buffer_size, stream_buffer_frame) != 0)
        {
            DEBUG_ERROR( "get buffer failed!\n");
            return;
        }
#else
        if(pgAvDeviceInfo->Adi_get_stream_buffer_info(BUFFER_TYPE_SNAP_STREAM, 0, stream_buffer_name, stream_buffer_size, stream_buffer_frame) != 0)
        {
            DEBUG_ERROR( "get buffer failed!\n");
            return;
        }
#endif
        //按通道创建共享内存
        m_SnapShareMemHandle[chn] = N9M_SHCreateShareBuffer( "avStreaming", stream_buffer_name, stream_buffer_size, stream_buffer_frame, SHAREMEM_WRITE_MODE);
        /*
        if (pgAvDeviceInfo->Adi_get_video_source(chn) == _VS_ANALOG_)
        {
            m_ShareMemLockState[chn] = 0;
            //DEBUG_MESSAGE("#########analog chn =%d\n",chn);
            N9M_SHLockWriter(m_SnapShareMemHandle[chn]);
        } 
        else
        {
            m_ShareMemLockState[chn] = 1;
        }
        */
        m_ShareMemLockState[chn] = 1;
        N9M_SHUnlockWriter(m_SnapShareMemHandle[chn]);
        N9M_SHClearAllFrames(m_SnapShareMemHandle[chn]);
    }

    m_u8MaxChlOnThisChip = pgAvDeviceInfo->Adi_max_channel_number();

    for(unsigned int i=0; i<(sizeof(m_pstuOsdInfo)/sizeof(m_pstuOsdInfo[0]));  ++i )
    {
        m_pstuOsdInfo[i] = new hi_snap_osd_t[m_u8MaxChlOnThisChip];
        memset( m_pstuOsdInfo[i], 0, sizeof(m_pstuOsdInfo) * m_u8MaxChlOnThisChip );
    }

    m_eSnapTvSystem = _AT_UNKNOWN_;  
    m_eTvSystem = _AT_UNKNOWN_;
    m_eSnapRes = _AR_UNKNOWN_;
    for( int ch=0; ch<_AV_VIDEO_MAX_CHN_; ch++ )
    {
        m_eMainStreamRes[ch] = _AR_UNKNOWN_;
    }

    m_eTaskState = SNAP_THREAD_NONE;
    m_u32MaxLenChlName = 0;

/////////////////////////////////////
    //QICF:0, wqcif:1, QVGA:2, CIF:3, wcif:4, HD1:5, VGA:6, whd1:7, D1:8, wd1:9, 720P:10, 960p:11, 1080p:12  
    m_ResWeight[_AR_SIZE_] = -1;
    m_ResWeight[_AR_CIF_] = 0;
    m_ResWeight[_AR_960H_WCIF_] = 1;
    m_ResWeight[_AR_HD1_] = 2;
    m_ResWeight[_AR_VGA_] = 3;
    m_ResWeight[_AR_960H_WHD1_] = 4;
    m_ResWeight[_AR_D1_] = 5;
    m_ResWeight[_AR_960H_WD1_] = 6;
    m_ResWeight[_AR_SVGA_] = 7;
    m_ResWeight[_AR_XGA_] = 8;
    m_ResWeight[_AR_720_] = 9;
    m_ResWeight[_AR_960P_] = 10;
    m_ResWeight[_AR_1080_] = 11;
    m_ResWeight[_AR_3M_] = 12;
    m_ResWeight[_AR_5M_] = 13;
    m_ResWeight[_AR_QCIF_] = -1;
    m_ResWeight[_AR_960H_WQCIF_] = -1;
    m_ResWeight[_AR_QVGA_] = -1;
    m_ResWeight[_AR_Q1080P_] = -1;
    m_ResWeight[_AR_UNKNOWN_] = 100;
}

CAvSnapTask::~CAvSnapTask()
{
    this->StopSnapThread();
    for(unsigned int i=0; i<(sizeof(m_pstuOsdInfo)/sizeof(m_pstuOsdInfo[0]));++i)
    {
        if(NULL != m_pstuOsdInfo[i])
        {
            delete m_pstuOsdInfo[i];
            m_pstuOsdInfo[i] = NULL;
        }
    }
    _AV_SAFE_DELETE_(m_pThread_lock);
    //_AV_SAFE_DELETE_(m_pAv_font_library);
    //delete m_AppMaster;

#if defined(_AV_PRODUCT_CLASS_IPC_)
        _AV_SAFE_DELETE_(m_pSignalSnapPara);
#endif
    for(int i=0; i<pgAvDeviceInfo->Adi_max_channel_number();++i)
    {
        N9M_SHUnlockWriter(m_SnapShareMemHandle[i]);
        N9M_SHDestroyShareBuffer(m_SnapShareMemHandle[i]);
        m_SnapShareMemHandle[i] = NULL;
    }

}

//#define SAVE_PICTURE
//#define TEST_SNAP_PICTURE
#ifdef SAVE_PICTURE
//static unsigned int g_uiTestSaveNum = 0;
#endif

int CAvSnapTask::SignalSnapThreadBody()
{
    DEBUG_MESSAGE( "CAvSnapTask::SignalSnapThreadBody\n");
    m_bIsTaskRunning = true;

    unsigned short iPhyChannel = 0;
    VIDEO_FRAME_INFO_S stuFrameInfo;
    VENC_STREAM_S stuStream;
    SnapShareMemoryHead_t stuPicHead;
    SShareMemData stuShareData;
    SShareMemFrameInfo stuShareInfo;
    unsigned int u32PicHeadLen = sizeof(stuPicHead);
#if defined(_AV_PRODUCT_CLASS_IPC_)
    hi_snap_osd_t stuOsdInfo[8];
#elif defined(_AV_PLATFORM_HI3515_)
    hi_snap_osd_t stuOsdInfo[4];
    hi_snap_osd_t stuOsdTem[4];
    int myosdnum = 4;
    HI_U32 ret = HI_FALSE;
#else
    hi_snap_osd_t stuOsdInfo[8];
    hi_snap_osd_t stuOsdTem[8];
    int myosdnum = 8;
    HI_U32 ret = HI_FALSE;
#endif
    unsigned int i = 0;
    memset(&stuOsdInfo, 0x0, sizeof(stuOsdInfo));
    memset(&stuPicHead, 0, sizeof(stuPicHead));
    memset(&stuShareInfo, 0, sizeof(stuShareInfo));
    stuShareInfo.flag = SHAREMEM_FLAG_IFRAME;
    stuShareData.pstuPack = new SShareMemData::_framePack_[_AV_MAX_PACK_NUM_ONEFRAME_ + 1]; 
#if defined(_AV_PRODUCT_CLASS_IPC_)

        while(!m_bNeedExitThread)
        {
            AvSnapPara_t stuSignalSnap ;
            static int serNum = 1;
            if(GetSignalSnapTask(&stuSignalSnap))
            {
                int channalMask = stuSignalSnap.uiChannel;
                int frameNum = stuSignalSnap.usNumber;
                for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
                {
                    if(0 != ((channalMask) & (1ll<<chn)))
                    {
                        for(int fra=0; fra<frameNum; fra++)
                        {
                            if(0 != m_pVpss->HiVpss_get_frame( _VP_MAIN_STREAM_,iPhyChannel, &stuFrameInfo))
                            //if( m_pVi->Hivi_get_vi_frame( iPhyChannel, &stuFrameInfo) != 0 )
                            {
                                DEBUG_WARNING("Encode picture Hivi_get_vi_frame failed phyCh=%d\n", iPhyChannel );
                                mSleep(10);
                                continue;
                            }

                            bool snap_para_changed_flag = false;
                            snap_para_changed_flag = IsSnapParaChanged(&stuSignalSnap);
                            this->GetSnapEncodeOsd(iPhyChannel, stuOsdInfo, sizeof(stuOsdInfo)/sizeof(stuOsdInfo[0]), m_eSnapRes);

                            if(snap_para_changed_flag || m_bCommSnapParamChange[chn] || !m_bCreatedSnap)
                            {
                                this->DestroySnapEncoder();
                                this->CreateSnapEncoder(chn, m_eSnapRes, m_pSignalSnapPara->ucQuality, stuOsdInfo, sizeof(stuOsdInfo)/sizeof(stuOsdInfo[0]));
                                m_bCommSnapParamChange[chn] = false;
                            }    
                            
                            this->CoordinateTrans(m_eSnapRes, m_eTvSystem, stuOsdInfo, sizeof(stuOsdInfo)/sizeof(stuOsdInfo[0]));
                            
                            if( m_pEnc->Ae_encode_vi_to_picture( chn, &stuFrameInfo, stuOsdInfo, sizeof(stuOsdInfo)/sizeof(stuOsdInfo[0]), &stuStream ) != 0 )
                            {
                                DEBUG_ERROR("Encode picture Ae_encode_vpss_to_picture failed\n");
                                //m_pVi->Hivi_release_vi_frame( iPhyChannel, &stuFrameInfo );
                                m_pVpss->HiVpss_release_frame(_VP_MAIN_STREAM_, iPhyChannel, &stuFrameInfo );
                                continue;
                            }
                                                        
                            datetime_t stuTime;
                            static rm_uint16_t serialLow = 1;
                            unsigned short stuSignalChnNum;
                            unsigned short stuChnNum;
                            unsigned short stuRestartNum;   
                            stuSignalChnNum = serialLow++;
                            stuChnNum = iPhyChannel;
                            stuRestartNum = this->GetSnapSerialNumber();
                            /* stuRestartNum(15)  stuChnNum(5) stuSignalChnNum(12)*/
                            unsigned int serialNumber = (stuSignalChnNum)|(stuChnNum<<12)|(stuRestartNum<<17);
                            pgAvDeviceInfo->Adi_get_date_time( &stuTime );
                            stuPicHead.stuSnapAttachedFile.ucSnapType = stuSignalSnap.uiSnapType;
                            stuPicHead.stuSnapAttachedFile.ucSnapChn = iPhyChannel;
                            stuPicHead.stuSnapAttachedFile.uiFileSize = 0;//stuFrameInfo.stVFrame.u32Width*stuFrameInfo.stVFrame.u32Height*1.5;
                            stuPicHead.stuSnapAttachedFile.stuDateTime = stuTime;
                            stuPicHead.stuSnapAttachedFile.ullSubType = stuSignalSnap.ullSubType;
                            stuPicHead.stuSnapAttachedFile.ucDeleteFlag = 0;
                            stuPicHead.stuSnapAttachedFile.ucLockFlag = 0;
                            stuPicHead.stuSnapAttachedFile.uiSnapTaskNumber = stuSignalSnap.uiSnapTaskNumber;
                            stuPicHead.stuSnapAttachedFile.uiSnapSerialNumber = serialNumber;
                            stuPicHead.stuSnapAttachedFile.uiUserFlag = stuSignalSnap.uiUser;
                            stuPicHead.stuSnapAttachedFile.ucPrivateDataType = stuSignalSnap.ucPrivateDataType;
                            memcpy(stuPicHead.stuSnapAttachedFile.ucPrivateData, stuSignalSnap.stuSnapSharePara.ucPrivateData, sizeof(stuSignalSnap.stuSnapSharePara.ucPrivateData));
                            stuShareData.iPackCount = 0;
                            stuShareData.pstuPack[stuShareData.iPackCount].pAddr = (char *)&stuPicHead;
                            stuShareData.pstuPack[stuShareData.iPackCount].iLen = u32PicHeadLen;
                            stuShareData.iPackCount++;
#ifdef SAVE_PICTURE
                            char szFileName[20];
                            //g_uiTestSaveNum=2;
                            sprintf( szFileName, "./pic/%d.jpg", g_uiTestSaveNum++ );
                            FILE* pfile = fopen(szFileName, "wb");
                            assert( pfile != NULL );
#endif     
                            for( i = 0; i < stuStream.u32PackCount; i++ )
                            {
#if defined(_AV_PLATFORM_HI3516A_) || defined(_AV_PLATFORM_HI3518EV200_)
                                VENC_PACK_S * pstData = &stuStream.pstPack[i];
                                stuShareData.pstuPack[stuShareData.iPackCount].pAddr = (char *)(pstData->pu8Addr + pstData->u32Offset);
                                stuShareData.pstuPack[stuShareData.iPackCount].iLen = pstData->u32Len - pstData->u32Offset;
                                stuShareData.iPackCount++;
                                
                                stuPicHead.stuSnapAttachedFile.uiFileSize += (pstData->u32Len - pstData->u32Offset);
#ifdef SAVE_PICTURE
                                fwrite( (char *)(pstData->pu8Addr + pstData->u32Offset), (pstData->u32Len - pstData->u32Offset), 1, pfile );
#endif                                
#else
                                VENC_PACK_S * pstData = &stuStream.pstPack[i];
                                stuShareData.pstuPack[stuShareData.iPackCount].pAddr = (char *)pstData->pu8Addr[0];
                                stuShareData.pstuPack[stuShareData.iPackCount].iLen = pstData->u32Len[0];
                                stuShareData.iPackCount++;
								
                                stuPicHead.stuSnapAttachedFile.uiFileSize += pstData->u32Len[0];
#ifdef SAVE_PICTURE
                                fwrite( (char *)pstData->pu8Addr[0], 1, pstData->u32Len[0], pfile );
#endif
                                if(pstData->u32Len[1] > 0)
                                {
                                    stuShareData.pstuPack[stuShareData.iPackCount].pAddr = (char *)pstData->pu8Addr[1];
                                    stuShareData.pstuPack[stuShareData.iPackCount].iLen = pstData->u32Len[1];
                                    stuShareData.iPackCount++;

                                    stuPicHead.stuSnapAttachedFile.uiFileSize += pstData->u32Len[1];
                                    
#ifdef SAVE_PICTURE
                					fwrite( (char *)pstData->pu8Addr[1], 1, pstData->u32Len[1], pfile );
#endif
                                }
#endif
                            }
                            DEBUG_MESSAGE("######stuPicHead.uiPicLength %u iPhyChannel=%d\n", stuPicHead.stuSnapAttachedFile.uiFileSize,iPhyChannel);
#ifdef SAVE_PICTURE  
                            DEBUG_MESSAGE("############save file success\n");
                            fclose(pfile);
#endif                                                       
                            N9M_SHPutOneFrame( m_SnapShareMemHandle[0], &stuShareData, &stuShareInfo, stuPicHead.stuSnapAttachedFile.uiFileSize + u32PicHeadLen );
                            msgSnapResultBoardcast stuSnapResult;
                            memset(&stuSnapResult, 0, sizeof(msgSnapResultBoardcast));
                            
                            stuSnapResult.uiPictureSerialnumber = serialNumber;
                            stuSnapResult.uiSanpCurNumber = fra+1;
                            stuSnapResult.uiSnapTaskNumber = stuSignalSnap.uiSnapTaskNumber;
                            stuSnapResult.uiSnapTotalNumber = stuSignalSnap.usNumber;
                            stuSnapResult.uiUser = stuSignalSnap.uiUser;
                            stuSnapResult.usSnapResult = 1;
                            serNum++;
                                                        
                            listSnapResult.push_back(stuSnapResult);
                            m_pEnc->Ae_encoder_vi_to_picture_free( &stuStream );
                            
                            //m_pVi->Hivi_release_vi_frame( iPhyChannel, &stuFrameInfo );
                            m_pVpss->HiVpss_release_frame(_VP_MAIN_STREAM_, iPhyChannel, &stuFrameInfo );
                            //memset( &stuPicHead, 0, sizeof(stuPicHead) );
                            //memset( &stuShareInfo, 0, sizeof(stuShareInfo) );
                            m_listOsdItem.clear();
                        }
                    }
                }
            }
            else
            {
                mSleep(100);
            }
        }
    
#else
    static int snapTask = 0;
    char snapResult =0;
    static rm_uint16_t serialLow[32] = {1};
    while( !m_bNeedExitThread )
    {
        AvSnapPara_t pstuSignalSnap ;
        //static int serNum = 1;
        while(this->GetChnState() == 1)
        {
            m_analog_digital_flag = 2;
            mSleep(30);
        }
        //DEBUG_ERROR("*****************while**************\n");
        if(GetSignalSnapTask(&pstuSignalSnap))
        {        
            DEBUG_WARNING("*****************while*****uiSnapType =%d *********\n",pstuSignalSnap.uiSnapType);
            if(pstuSignalSnap.uiSnapType != 5)
            {
                snapTask++;//DEBUG_WARNING
                
                DEBUG_WARNING("uiSnapTaskNumber =%d uiSnapType =%d uiChannel =%d\n", pstuSignalSnap.uiSnapTaskNumber, pstuSignalSnap.uiSnapType, pstuSignalSnap.uiChannel);
                for(int k=0; k<myosdnum; k++)
                {
                    memset(&stuOsdInfo[k], 0, sizeof(hi_snap_osd_t));
                }
                int channalMask = pstuSignalSnap.uiChannel;
                int frameNum = pstuSignalSnap.usNumber;
                for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
                {
                    if(0 != ((channalMask) & (1ll<<chn)))
                    {
                        iPhyChannel = chn;
                        while(N9M_SHLockWriter(m_SnapShareMemHandle[iPhyChannel]) != 0)
                        {
                            DEBUG_ERROR("realtime snap N9M_SHLockWriter  failed...\n");
                            mSleep(30);
                        }
                        if( iPhyChannel < m_u8MaxChlOnThisChip )
                        {
                            /* judge chn's video loss state */
                            DEBUG_DEBUG("Ae_get_video_lost_flag chn =%d flag=%d\n", iPhyChannel,m_pEnc->Ae_get_video_lost_flag(iPhyChannel));
                            //6AII_AV12 product also can snap in video_loss
                            bool bSnap = false;
                            bSnap  = (m_pEnc->Ae_get_video_lost_flag(iPhyChannel) == 0);
							bSnap |= (OSD_TEST_FLAG == pgAvDeviceInfo->Adi_product_type());
							bSnap |= (PTD_6A_I == pgAvDeviceInfo->Adi_product_type());
                            if(bSnap)
                            {
#if defined(_AV_PLATFORM_HI3515_)
                                if(m_eMainStreamRes[iPhyChannel] == 1) //!D1
                                {
                                    if ((pstuSignalSnap.ucResolution == 0)||(pstuSignalSnap.ucResolution == 1)||(pstuSignalSnap.ucResolution == 2))
                                    {
                                        ;
                                    }
                                    else
                                    {
                                        pstuSignalSnap.ucResolution = 2; //! D1
                                    }
                                   //continue; 
                                }
                                else if(m_eMainStreamRes[iPhyChannel] == _AR_HD1_)
                                {
                                    if ((pstuSignalSnap.ucResolution == 0)||(pstuSignalSnap.ucResolution == 1))
                                    {
                                        ;
                                    }
                                    else
                                    {
                                        pstuSignalSnap.ucResolution = 1; //! HD1
                                    }     
                                }
                                else if (m_eMainStreamRes[iPhyChannel] == _AR_CIF_)
                                {
                                    pstuSignalSnap.ucResolution = 0;   //!CIF
                                }
                                else
                                {
                                    pstuSignalSnap.ucResolution = m_eMainStreamRes[iPhyChannel];
                                }
                                    
#else
                                if(((m_eMainStreamRes[iPhyChannel] == 1) || (m_eMainStreamRes[iPhyChannel] == 2) ||(m_eMainStreamRes[iPhyChannel] == 3)) && ((pstuSignalSnap.ucResolution == 12) || (pstuSignalSnap.ucResolution == 13)))
                                {
                                   pstuSignalSnap.ucResolution = 2;
                                   //continue; 
                                }
#endif
                                int mainWidth = 0;
                                int mainHeight = 0;
                                int snapWidth = 0;
                                int snapHeight = 0;
                                
                                pgAvDeviceInfo->Adi_get_video_size((av_resolution_t)m_eMainStreamRes[iPhyChannel], m_eTvSystem, &mainWidth, &mainHeight);
                                pgAvDeviceInfo->Adi_get_video_size((av_resolution_t)SnapResolutionToMainStream(pstuSignalSnap.ucResolution), m_eTvSystem, &snapWidth, &snapHeight);
                                
                                DEBUG_MESSAGE("0-CIF 1-HD1 2-D1 3-QCIF 4-QVGA 5-VGA 6-720P 7-1080P 8-3MP(2048*1536) 9-5MP(2592*1920) 10 WQCIF11 WCIF12 WHD113 WD1(960H) 14-960P 15-Q1080P\n");
                                DEBUG_MESSAGE("snap chn =%d ucResolution =%d snapWidth =%d snapHeight =%d \n", iPhyChannel, pstuSignalSnap.ucResolution, snapWidth, snapHeight);
                                DEBUG_MESSAGE("main chn =%d ucResolution =%d snapWidth =%d snapHeight =%d \n", iPhyChannel, MainStreamToSnapResolution(m_eMainStreamRes[iPhyChannel]), mainWidth, mainHeight);
                                if((mainWidth < snapWidth) || (mainHeight < snapHeight))
                                {
                                    DEBUG_CRITICAL("----snap resolution[%d] from m_eMainStreamRe[%d] \n", pstuSignalSnap.ucResolution, MainStreamToSnapResolution(m_eMainStreamRes[iPhyChannel]));
                                    pstuSignalSnap.ucResolution = MainStreamToSnapResolution(m_eMainStreamRes[iPhyChannel]);
                                }
                                /*
                                if(((m_eMainStreamRes[iPhyChannel] == 1) || (m_eMainStreamRes[iPhyChannel] == 2) ||(m_eMainStreamRes[iPhyChannel] == 3)) && ((pstuSignalSnap.ucResolution == 12) || (pstuSignalSnap.ucResolution == 13)))
                                {
                                   pstuSignalSnap.ucResolution = 2;
                                   //continue; 
                                }*/
                                for(int fra=0; fra<frameNum; fra++)
                                {
                                    //DEBUG_MESSAGE("lshlsh total frameNum =%d,curFrameNum = %d\n", pstuSignalSnap.usNumber, fra+1);
                                    if( m_pVi->Hivi_get_vi_frame(iPhyChannel, &stuFrameInfo) != 0 )
                                    {
                                        DEBUG_ERROR("Encode picture Hivi_get_vi_frame failed phyCh=%d\n", iPhyChannel );
                                        mSleep(10);
                                        N9M_SHUnlockWriter(m_SnapShareMemHandle[iPhyChannel] );
                                        continue;
                                    }
            					    if (pgAvDeviceInfo->Adi_get_video_source(iPhyChannel) == _VS_DIGITAL_)
            					    {
            					        DEBUG_ERROR( "CHiVi::Hivi_get_vi_frame chn %d is digital!!! \n", iPhyChannel);
                                        N9M_SHUnlockWriter(m_SnapShareMemHandle[iPhyChannel] );
                                        continue;
            					    }
            					    
									this->GetSnapEncodeOsd(iPhyChannel, stuOsdInfo, myosdnum, ConvertResolutionFromIntToEnum(pstuSignalSnap.ucResolution));
									
                                    if(!m_bCreatedSnap)
                                    {
                                    	//DestroySnapEncoder(stuOsdInfo);
                                        CreateSnapEncoder(iPhyChannel, pstuSignalSnap.ucResolution, pstuSignalSnap.ucQuality, stuOsdInfo, myosdnum);
                                    }

                                    
#if !defined(_AV_PLATFORM_HI3515_)	//3515 when create osd region 3515 need region X/Y, so coordinate befro create region
                                    this->CoordinateTrans(m_eSnapRes, m_eTvSystem, stuOsdInfo,sizeof(stuOsdInfo)/sizeof(stuOsdInfo[0]));
#endif
                                    if( m_pEnc->Ae_encode_vi_to_picture(iPhyChannel,&stuFrameInfo, stuOsdInfo,sizeof(stuOsdInfo)/sizeof(stuOsdInfo[0]),&stuStream) != 0 )
                                    {
                                    	DEBUG_ERROR("m_pEnc->Ae_encode_vi_to_picture failed,chn = %d\n", iPhyChannel);
                                        m_pVi->Hivi_release_vi_frame(iPhyChannel, &stuFrameInfo);
                                        DestroySnapEncoder(stuOsdInfo);
										m_listOsdItem.clear();
                                        N9M_SHUnlockWriter(m_SnapShareMemHandle[iPhyChannel] );
                                        continue;
                                    }
                                    datetime_t stuTime;
                                    
                                    unsigned short stuSignalChnNum;
                                    unsigned short stuChnNum;
                                    unsigned short stuRestartNum;	
                                    stuSignalChnNum = serialLow[iPhyChannel]++;
                                    if(stuSignalChnNum > 4096)
                                    {
                                        serialLow[iPhyChannel] = 1;
                                    }
                                    stuChnNum = iPhyChannel;
                                    stuRestartNum = this->GetSnapSerialNumber();
                                    //stuRestartNum(15)  stuChnNum(5) stuSignalChnNum(12)
                                    unsigned int serialNumber = (stuSignalChnNum)|(stuChnNum << 12)|(stuRestartNum << 17);

                                    pgAvDeviceInfo->Adi_get_date_time( &stuTime );

                                    stuPicHead.stuSnapAttachedFile.ucSnapType = pstuSignalSnap.uiSnapType;
                                    stuPicHead.stuSnapAttachedFile.ucSnapChn = iPhyChannel;
                                    stuPicHead.stuSnapAttachedFile.uiFileSize = 0;//stuFrameInfo.stVFrame.u32Width*stuFrameInfo.stVFrame.u32Height*1.5;
                                    stuPicHead.stuSnapAttachedFile.stuDateTime = stuTime;
                                    //stuPicHead.stuSnapAttachedFile.ulAlarmType = pstuSignalSnap.ulAlarmType;
                                    stuPicHead.stuSnapAttachedFile.ucDeleteFlag = 0;
                                    stuPicHead.stuSnapAttachedFile.ucLockFlag = 0;
                                    stuPicHead.stuSnapAttachedFile.uiSnapTaskNumber = pstuSignalSnap.uiSnapTaskNumber;
                                    stuPicHead.stuSnapAttachedFile.uiSnapSerialNumber = serialNumber;
                                    stuPicHead.stuSnapAttachedFile.uiUserFlag = pstuSignalSnap.uiUser;
                                    stuPicHead.stuSnapAttachedFile.ucPrivateDataType = pstuSignalSnap.ucPrivateDataType;
                                    memcpy(stuPicHead.stuSnapAttachedFile.ucPrivateData, pstuSignalSnap.stuSnapSharePara.ucPrivateData, sizeof(pstuSignalSnap.stuSnapSharePara.ucPrivateData));
                                    stuPicHead.stuSnapAttachedFile.ullSubType = pstuSignalSnap.ullSubType;
                                    stuShareData.iPackCount = 0;

                                    stuShareData.pstuPack[stuShareData.iPackCount].pAddr = (char *)&stuPicHead;									
                                    stuShareData.pstuPack[stuShareData.iPackCount].iLen = u32PicHeadLen;
                                    stuShareData.iPackCount++;
#ifdef _AV_PLATFORM_HI3515_
									char jpghead[2] = {0xFF, 0xD8};
									stuShareData.pstuPack[stuShareData.iPackCount].pAddr = (char *)&jpghead[0];
									stuShareData.pstuPack[stuShareData.iPackCount].iLen = 2;
									stuShareData.iPackCount++;

									stuPicHead.stuSnapAttachedFile.uiFileSize += 2;
#endif
                                    for(i = 0; i < stuStream.u32PackCount; i++)
                                    {
                                        VENC_PACK_S * pstData = &stuStream.pstPack[i];

#ifdef _AV_PLATFORM_HI3520D_V300_						
	                                    stuShareData.pstuPack[stuShareData.iPackCount].pAddr = (char *)(pstData->pu8Addr + pstData->u32Offset);
	                                    stuShareData.pstuPack[stuShareData.iPackCount].iLen = pstData->u32Len - pstData->u32Offset;                                
	                                    stuPicHead.stuSnapAttachedFile.uiFileSize += (pstData->u32Len - pstData->u32Offset);
	                                    stuShareData.iPackCount++;

#else
	                                    stuShareData.pstuPack[stuShareData.iPackCount].pAddr = (char *)pstData->pu8Addr[0];
	                                    stuShareData.pstuPack[stuShareData.iPackCount].iLen = pstData->u32Len[0];
	                                    stuShareData.iPackCount++;

	                                    stuPicHead.stuSnapAttachedFile.uiFileSize += pstData->u32Len[0];

	                                    if(pstData->u32Len[1] > 0)
	                                    {
	                                            stuShareData.pstuPack[stuShareData.iPackCount].pAddr = (char *)pstData->pu8Addr[1];
	                                            stuShareData.pstuPack[stuShareData.iPackCount].iLen = pstData->u32Len[1];
	                                            stuShareData.iPackCount++;

	                                            stuPicHead.stuSnapAttachedFile.uiFileSize += pstData->u32Len[1];


	                                    }
#endif
                                    }
									
                                    DEBUG_CRITICAL("######stuPicHead.uiPicLength %u iPhyChannel=%d\n", stuPicHead.stuSnapAttachedFile.uiFileSize,iPhyChannel);
#ifdef _AV_PLATFORM_HI3515_
									char jpegtail[2]={0xFF, 0xD9};
									stuShareData.pstuPack[stuShareData.iPackCount].pAddr = (char *)&jpegtail[0];									
                                    stuShareData.pstuPack[stuShareData.iPackCount].iLen = 2;
									stuShareData.iPackCount++;
									stuPicHead.stuSnapAttachedFile.uiFileSize += 2;

#endif
								
#ifdef SAVE_PICTURE					
									char szFileName[50] = {0};
                                    sprintf(szFileName, "chn%02d_%d%02d%02d-%02d%02d%02d_%d_%lld_%d.jpeg", 
                                                            stuPicHead.stuSnapAttachedFile.ucSnapChn + 1, 
                                                            stuPicHead.stuSnapAttachedFile.stuDateTime.year,
                                                            stuPicHead.stuSnapAttachedFile.stuDateTime.month,
                                                            stuPicHead.stuSnapAttachedFile.stuDateTime.day,
                                                            stuPicHead.stuSnapAttachedFile.stuDateTime.hour,
                                                            stuPicHead.stuSnapAttachedFile.stuDateTime.minute,
                                                            stuPicHead.stuSnapAttachedFile.stuDateTime.second,
                                                            stuPicHead.stuSnapAttachedFile.ucSnapType,
                                                            stuPicHead.stuSnapAttachedFile.ullSubType,
                                                            stuPicHead.stuSnapAttachedFile.uiSnapSerialNumber);

                                    FILE* pfile = fopen(szFileName, "wb");
									if (pfile)
									{ 
										/*skip stuPicHead*/
										for (i = 1 ; i < stuShareData.iPackCount; i++)
										{
											int iLen = stuShareData.pstuPack[i].iLen;
											char *data = (char *)stuShareData.pstuPack[i].pAddr;
											if (iLen > 0)
												fwrite(data, 1, iLen, pfile );
										}
									}
									if (pfile)
										fclose (pfile);
#endif
                                    
#ifdef _AV_PRODUCT_CLASS_HDVR_  
                                    if((ret = N9M_SHPutOneFrame(m_SnapShareMemHandle[iPhyChannel], &stuShareData, &stuShareInfo, stuPicHead.stuSnapAttachedFile.uiFileSize + u32PicHeadLen)) != 0)
                                    {   
                                        DEBUG_MESSAGE("N9M_SHPutOneFrame success...%d chn =%d uiSnapTotalNumber =%d uiSanpCurNumber =%d\n", snapTask, iPhyChannel+1, pstuSignalSnap.usNumber, fra+1);
                                        snapResult = 1;
                                    }
                                    else
                                    {
                                        DEBUG_MESSAGE("N9M_SHPutOneFrame failed...%d chn =%d uiSnapTotalNumber =%d uiSanpCurNumber =%d\n", snapTask, iPhyChannel+1, pstuSignalSnap.usNumber, fra+1);
                                        snapResult = 0;
                                    }
#else
                                    N9M_SHPutOneFrame(m_SnapShareMemHandle[0], &stuShareData, &stuShareInfo, stuPicHead.stuSnapAttachedFile.uiFileSize + u32PicHeadLen);
#endif

                                    AddBoardcastSnapResult(snapResult, serialNumber, fra, pstuSignalSnap);
                                    m_pEnc->Ae_encoder_vi_to_picture_free(&stuStream);


                                    m_pVi->Hivi_release_vi_frame(iPhyChannel, &stuFrameInfo);
									DestroySnapEncoder(stuOsdInfo);//if not destory, will snap pre picture every time ///
                                    m_listOsdItem.clear();
                                    }
                                }

                            }
                            
                            N9M_SHUnlockWriter(m_SnapShareMemHandle[iPhyChannel]);
                        }
                    }
            }
            else //history snap 
            {
                DEBUG_WARNING("time %d %d %d :%d %d %d\n",\
                    pstuSignalSnap.stuSnapTime.year,pstuSignalSnap.stuSnapTime.month,pstuSignalSnap.stuSnapTime.day,pstuSignalSnap.stuSnapTime.hour,pstuSignalSnap.stuSnapTime.minute,pstuSignalSnap.stuSnapTime.second);
                int fd = -1;
                int buffer_len = 1024*1024;
                int frame_header_len = 0;
                int readlength = 0;
                char *buffer = NULL;
                RMFI2_RTCTIME *pRtc_time = NULL;
                int result = 0;
                datetime_t stuTime;
                
                buffer = (char*)malloc(buffer_len);
                if(buffer == NULL)
                {
                    DEBUG_ERROR("buffer NULL...\n");
                    return -1;
                }
                file_operate_stream_open_para_t open_para;
                memset(&open_para, 0, sizeof(file_operate_stream_open_para_t));
                int channalMask = pstuSignalSnap.uiChannel;
                //int frameNum = pstuSignalSnap.usNumber;
                
                for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
                {
                    if(0 != ((channalMask) & (1ll<<chn)))
                    {

                        
                        unsigned short stuSignalChnNum;
                        unsigned short stuChnNum;
                        unsigned short stuRestartNum;	
                        stuSignalChnNum = 1;
                        stuChnNum = chn;
                        stuRestartNum = this->GetSnapSerialNumber();
                        //stuRestartNum(15)  stuChnNum(5) stuSignalChnNum(12)
                        unsigned int serialNumber = (stuSignalChnNum)|(stuChnNum << 12)|(stuRestartNum << 17);
                        
                        while(N9M_SHLockWriter(m_SnapShareMemHandle[chn]) != 0)
                        {
                            DEBUG_ERROR("history snap N9M_SHLockWriter  failed...\n");
                            mSleep(30);
                        }
                        // 1.募系统囟时尾//
                        DEBUG_WARNING("chn =%d \n", chn);
                        open_para.channel = chn;
                        open_para.group_bitmap = 0xffff;
                        open_para.record_type_bitmap = 0xff;
                        open_para.stream_type= 0;
                        open_para.open_time.year = pstuSignalSnap.stuSnapTime.year;
                        open_para.open_time.month = pstuSignalSnap.stuSnapTime.month;
                        open_para.open_time.day = pstuSignalSnap.stuSnapTime.day;
                        open_para.open_time.hour = pstuSignalSnap.stuSnapTime.hour;
                        open_para.open_time.minute = pstuSignalSnap.stuSnapTime.minute;
                        open_para.open_time.second = pstuSignalSnap.stuSnapTime.second;
                        //为帧取抓时
                        //memcpy(&stuTime, &pstuSignalSnap.stuSnapTime, sizeof(datetime_t));
                        
                        fd = api_rof_client_open_stream(&open_para);
                        if(fd == -1)
                        {
                            DEBUG_ERROR("open stream failed.... time =%d-%d-%d %d-%d-%d\n",open_para.open_time.year,open_para.open_time.month,open_para.open_time.day,open_para.open_time.hour,open_para.open_time.minute,open_para.open_time.second);
                            N9M_SHUnlockWriter(m_SnapShareMemHandle[chn] );
                            snapResult = 0;
                            AddBoardcastSnapResult(snapResult, serialNumber, 0, pstuSignalSnap);
                            continue;
                        }
                        result = api_rof_client_seek_stream(fd, &open_para.open_time);
                        if(result == -1)
                        {
                            DEBUG_ERROR("api_rof_client_seek_stream failed.... \n");
                            N9M_SHUnlockWriter(m_SnapShareMemHandle[chn] );
                            snapResult = 0;
                            AddBoardcastSnapResult(snapResult, serialNumber, 0, pstuSignalSnap);
                            continue;
                        }
                        
                        DEBUG_WARNING("---ucSnapMode =%d\n",pstuSignalSnap.ucSnapMode );
                        for(int fra =0; fra< 3; fra++)
                        {
                            if(pstuSignalSnap.ucSnapMode == 0) //T-1 T T+1
                            {
                                // 取前时前一I帧危源取I帧// 
                                
                                if(fra == 0)
                                {
                                    
                                    readlength = api_rof_client_read_stream(fd, FOT_READ_PRE_IFRAME, 0, buffer, buffer_len);
                                }
                                else
                                {
                                    if(fra == 1)
                                    {
                                        result = api_rof_client_seek_stream(fd, &open_para.open_time);
                                        if(result == -1)
                                        {
                                            printf("api_rof_client_seek_stream failed.... \n");
                                            N9M_SHUnlockWriter(m_SnapShareMemHandle[chn] );
                                            snapResult = 0;
                                            AddBoardcastSnapResult(snapResult, serialNumber, fra, pstuSignalSnap);
                                            continue;
                                        }
                                        
                                    }
                                   
                                    readlength = api_rof_client_read_stream(fd, FOT_READ_NEXT_IFRAME, 0, buffer, buffer_len);
                                }
                                
                                if(readlength == -1)
                                {
                                    DEBUG_ERROR("read stream failed.... \n");
                                    N9M_SHUnlockWriter(m_SnapShareMemHandle[chn] );
                                    snapResult = 0;
                                    AddBoardcastSnapResult(snapResult, serialNumber, fra, pstuSignalSnap);
                                    continue;
                                }
                            }
                            else if(pstuSignalSnap.ucSnapMode == 1)// T-2 T-1 T
                            {

                                if(fra == 0)
                                {
                                    readlength = api_rof_client_read_stream(fd, FOT_READ_PRE_IFRAME, 1, buffer, buffer_len);
                                }
                                else
                                {
                                    if(fra == 1)
                                    {
                                        result = api_rof_client_seek_stream(fd, &open_para.open_time);
                                        if(result == -1)
                                        {
                                            printf("api_rof_client_seek_stream failed.... \n");
                                            snapResult = 0;
                                            AddBoardcastSnapResult(snapResult, serialNumber, fra, pstuSignalSnap);
                                            continue;
                                        }
                                        readlength = api_rof_client_read_stream(fd, FOT_READ_PRE_IFRAME, 0,buffer, buffer_len);
                                    }
                                    else
                                    {
                                        result = api_rof_client_seek_stream(fd, &open_para.open_time);
                                        if(result == -1)
                                        {
                                            printf("api_rof_client_seek_stream failed.... \n");
                                            snapResult = 0;
                                            AddBoardcastSnapResult(snapResult, serialNumber, fra, pstuSignalSnap);
                                            continue;
                                        }
                                        readlength = api_rof_client_read_stream(fd, FOT_READ_NORMAL, 0, buffer, buffer_len);
                                    }
                                    
                                    if(readlength == -1)
                                    {
                                        DEBUG_ERROR("read stream failed.... \n");
                                        N9M_SHUnlockWriter(m_SnapShareMemHandle[chn] );
                                        snapResult = 0;
                                        AddBoardcastSnapResult(snapResult, serialNumber, fra, pstuSignalSnap);
                                        continue;
                                    }
                                }
                            
                            }
                            else if(pstuSignalSnap.ucSnapMode == 2)// T 一
                            {  
                                if(fra == 0)
                                {
                                    readlength = api_rof_client_read_stream(fd, FOT_READ_PRE_IFRAME, 0, buffer, buffer_len);
                                    continue;
                                }
                                else
                                {
                                    if(fra == 1)
                                    {
                                        result = api_rof_client_seek_stream(fd, &open_para.open_time);
                                        if(result == -1)
                                        {
                                            printf("api_rof_client_seek_stream failed.... \n");
                                            N9M_SHUnlockWriter(m_SnapShareMemHandle[chn] );
                                            snapResult = 0;
                                            AddBoardcastSnapResult(snapResult, serialNumber, 0, pstuSignalSnap);
                                            continue;
                                        }
                                        
                                    }
                                    else
                                    {
                                        continue;
                                    }
                                   
                                    readlength = api_rof_client_read_stream(fd, FOT_READ_NEXT_IFRAME, 0, buffer, buffer_len);
                                }
                                
                                if(readlength == -1)
                                {
                                    DEBUG_ERROR("read stream failed.... \n");
                                    N9M_SHUnlockWriter(m_SnapShareMemHandle[chn] );
                                    snapResult = 0;
                                    AddBoardcastSnapResult(snapResult, serialNumber, 0, pstuSignalSnap);
                                    continue;
                                }
                            }
                            else
                            {
                            }
                            DEBUG_WARNING("readlength =%d i = %d\n", readlength, fra);
                            // 2.直臃诺砘盒广播抓慕//
                            //踊取帧谢取录时洌琁帧时时
                            frame_header_len = sizeof(RMSTREAM_HEADER) + sizeof(RMFI2_VTIME) +sizeof(RMFI2_VIDEOINFO);
                            pRtc_time = (RMFI2_RTCTIME *)(buffer + frame_header_len);
                            memset(&stuTime, 0, sizeof(stuTime));
                            stuTime.year = pRtc_time->stuRtcTime.cYear;
                            stuTime.month = pRtc_time->stuRtcTime.cMonth;
                            stuTime.day = pRtc_time->stuRtcTime.cDay;
                            stuTime.hour = pRtc_time->stuRtcTime.cHour;
                            stuTime.minute= pRtc_time->stuRtcTime.cMinute;
                            stuTime.second = pRtc_time->stuRtcTime.cSecond;
                            DEBUG_WARNING("-----time =%d %d %d :%d %d %d\n",\
                        pRtc_time->stuRtcTime.cYear, pRtc_time->stuRtcTime.cMonth,pRtc_time->stuRtcTime.cDay,pRtc_time->stuRtcTime.cHour,pRtc_time->stuRtcTime.cMinute,pRtc_time->stuRtcTime.cSecond);
                            //泻
                            stuSignalChnNum = serialLow[chn]++;
                            if(stuSignalChnNum > 4096)
                            {
                                serialLow[chn] = 1;
                            }
                            stuChnNum = chn;
                            stuRestartNum = this->GetSnapSerialNumber();
                            //stuRestartNum(15)  stuChnNum(5) stuSignalChnNum(12)
                            serialNumber = (stuSignalChnNum)|(stuChnNum << 12)|(stuRestartNum << 17);
                            
                            stuPicHead.stuSnapAttachedFile.ucSnapType = pstuSignalSnap.uiSnapType;
                            stuPicHead.stuSnapAttachedFile.ucSnapChn = chn;
                            stuPicHead.stuSnapAttachedFile.uiFileSize = 0;//stuFrameInfo.stVFrame.u32Width*stuFrameInfo.stVFrame.u32Height*1.5;
                            stuPicHead.stuSnapAttachedFile.stuDateTime = stuTime;
                            //stuPicHead.stuSnapAttachedFile.ulAlarmType = pstuSignalSnap.ulAlarmType;
                            stuPicHead.stuSnapAttachedFile.ucDeleteFlag = 0;
                            stuPicHead.stuSnapAttachedFile.ucLockFlag = 0;
                            stuPicHead.stuSnapAttachedFile.uiSnapTaskNumber = pstuSignalSnap.uiSnapTaskNumber;
                            stuPicHead.stuSnapAttachedFile.uiSnapSerialNumber = serialNumber;
                            stuPicHead.stuSnapAttachedFile.uiUserFlag = pstuSignalSnap.uiUser;
                            stuPicHead.stuSnapAttachedFile.ucPrivateDataType = pstuSignalSnap.ucPrivateDataType;
                            memcpy(stuPicHead.stuSnapAttachedFile.ucPrivateData, pstuSignalSnap.stuSnapSharePara.ucPrivateData, sizeof(pstuSignalSnap.stuSnapSharePara.ucPrivateData));
                            stuPicHead.stuSnapAttachedFile.ullSubType = pstuSignalSnap.ullSubType;
                            stuShareData.iPackCount = 0;
                            stuShareData.pstuPack[stuShareData.iPackCount].pAddr = (char *)&stuPicHead;
                            stuShareData.pstuPack[stuShareData.iPackCount].iLen = u32PicHeadLen;
                            stuShareData.iPackCount++;
#ifdef SAVE_PICTURE
                            char szFileName[50];
                            sprintf(szFileName, "chn%02d_%d%02d%02d-%02d%02d%02d_%d_%lld_%d.jpeg", 
                                                    stuPicHead.stuSnapAttachedFile.ucSnapChn + 1, 
                                                    stuPicHead.stuSnapAttachedFile.stuDateTime.year,
                                                    stuPicHead.stuSnapAttachedFile.stuDateTime.month,
                                                    stuPicHead.stuSnapAttachedFile.stuDateTime.day,
                                                    stuPicHead.stuSnapAttachedFile.stuDateTime.hour,
                                                    stuPicHead.stuSnapAttachedFile.stuDateTime.minute,
                                                    stuPicHead.stuSnapAttachedFile.stuDateTime.second,
                                                    stuPicHead.stuSnapAttachedFile.ucSnapType,
                                                    stuPicHead.stuSnapAttachedFile.ullSubType,
                                                    stuPicHead.stuSnapAttachedFile.uiSnapSerialNumber);

                            FILE* pfile = fopen(szFileName, "wb");
                            assert( pfile != NULL );
#endif      
                            //for(i = 0; i < stuStream.u32PackCount; i++)
                            {

                                //VENC_PACK_S * pstData = &stuStream.pstPack[i];
                                stuShareData.pstuPack[stuShareData.iPackCount].pAddr = buffer;
                                stuShareData.pstuPack[stuShareData.iPackCount].iLen = readlength;
                                stuShareData.iPackCount++;

                                stuPicHead.stuSnapAttachedFile.uiFileSize += readlength;
#ifdef SAVE_PICTURE
                                fwrite( buffer, 1, readlength, pfile );
#endif
                                
                            }
                            //DEBUG_MESSAGE("######stuPicHead.uiPicLength %u iPhyChannel=%d\n", stuPicHead.stuSnapAttachedFile.uiFileSize,iPhyChannel);
#ifdef SAVE_PICTURE  
                            DEBUG_ERROR("############save file success\n");
                            fclose(pfile);
#endif

                            char snapResult =0;
#ifdef _AV_PRODUCT_CLASS_HDVR_  
                            if((ret = N9M_SHPutOneFrame(m_SnapShareMemHandle[chn], &stuShareData, &stuShareInfo, stuPicHead.stuSnapAttachedFile.uiFileSize + u32PicHeadLen)) != 0)
                            {
                                DEBUG_WARNING("N9M_SHPutOneFrame success...%d chn =%d uiSnapTotalNumber =%d uiSanpCurNumber =%d\n", snapTask, iPhyChannel+1, pstuSignalSnap.usNumber, fra+1);
                                snapResult = 1;
                            }
                            else
                            {
                                DEBUG_WARNING("N9M_SHPutOneFrame failed...%d chn =%d uiSnapTotalNumber =%d uiSanpCurNumber =%d\n", snapTask, iPhyChannel+1, pstuSignalSnap.usNumber, fra+1);
                                snapResult = 0;
                            }
#else
                            N9M_SHPutOneFrame(m_SnapShareMemHandle[0], &stuShareData, &stuShareInfo, stuPicHead.stuSnapAttachedFile.uiFileSize + u32PicHeadLen);
#endif

                            AddBoardcastSnapResult(snapResult, serialNumber, fra, pstuSignalSnap);
                            /*
                            msgSnapResultBoardcast stuSnapResult;
                            memset(&stuSnapResult, 0, sizeof(msgSnapResultBoardcast));

                            stuSnapResult.uiPictureSerialnumber = serialNumber;
                            stuSnapResult.uiSanpCurNumber = fra+1;
                            stuSnapResult.uiSnapTaskNumber = pstuSignalSnap.uiSnapTaskNumber;
                            stuSnapResult.uiSnapTotalNumber = pstuSignalSnap.usNumber;
                            stuSnapResult.uiUser = pstuSignalSnap.uiUser;
                            stuSnapResult.usSnapResult = snapResult;
                            listSnapResult.push_back(stuSnapResult);*/

                        }
                        result = api_rof_client_close_stream(fd);
                        if(result != 0)
                        {
                            DEBUG_CRITICAL("api_rof_client_close_stream failed...\n");
                            N9M_SHUnlockWriter(m_SnapShareMemHandle[chn] );
                        }
                        N9M_SHUnlockWriter(m_SnapShareMemHandle[chn] );
                    }
                }
                free(buffer);
            }
        }
        else
        {
			mSleep(100);
        }
        mSleep(15);
    }
#endif
    if(NULL != stuShareData.pstuPack)
    {
        delete []stuShareData.pstuPack;
    }
#if defined(_AV_PRODUCT_CLASS_IPC_)
    if(m_bCreatedSnap)
    {
        this->DestroySnapEncoder();
    }
#else
    DestroySnapEncoder(stuOsdTem);
#endif

    m_bIsTaskRunning = false;
    DEBUG_MESSAGE("========================snap thread stoped!!\n");
        
    return 0;
}

#if defined(_AV_PLATFORM_HI3515_)
int CAvSnapTask::Get6AISnapEncoderOsd( unsigned char u8ChipChn, hi_snap_osd_t* pstuOsd, int osdNum, av_resolution_t eResolution)
{
    int font_width = 0;
    int font_height = 0;
    int font_name = 0;
    int horizontal_scaler = 0;
    int vertical_scaler = 0;
    int video_width = {0};
    int video_height = {0};

	int n8OsdStrLen[8] = {0};
	int n8OsdStrByte[8] = {0};
	char str8OsdContent[8][MAX_EXTEND_OSD_LENGTH] = {0};
	char str4OsdContent[4][MAX_EXTEND_OSD_LENGTH * 2] = {0};

	int nSpaceNum = 0;
	int nPos = 0;

	m_pEnc->Ae_get_osd_parameter(_AON_EXTEND_OSD, eResolution, &font_name, &font_width, &font_height, &horizontal_scaler, &vertical_scaler);

	int nSpaceLen = m_pEnc->Ae_get_osd_string_width(" ", font_name, horizontal_scaler, false);
	
    pgAvDeviceInfo->Adi_get_video_size( eResolution, m_eSnapTvSystem, &video_width, &video_height );
    
	for(int item = 0 ; item < 8 ; ++item)
	{
		if(m_pstuOsdInfo[item][u8ChipChn].bShow == 1)
		{
			if(m_pstuOsdInfo[item][u8ChipChn].type == 0)
			{
				datetime_t datetime = {0};
				pgAvDeviceInfo->Adi_get_date_time(&datetime);
				snprintf(str8OsdContent[item], sizeof(str8OsdContent[item]), "%02d.%02d.%02d   %02d:%02d:%02d", datetime.year, datetime.month, datetime.day, datetime.hour, datetime.minute, datetime.second);
				n8OsdStrLen[item] = m_pEnc->Ae_get_osd_string_width("16.26.26  26:26:26  ", font_name, horizontal_scaler);
			}
			else
			{
				memcpy(str8OsdContent[item], m_pstuOsdInfo[item][u8ChipChn].szStr, sizeof(m_pstuOsdInfo[item][u8ChipChn].szStr));
				n8OsdStrLen[item] = m_pEnc->Ae_get_osd_string_width(str8OsdContent[item], font_name, horizontal_scaler);
			}
		}
		n8OsdStrByte[item] = strlen(str8OsdContent[item]);
	}
	//Merge osd region///
	//date-time and speed-mile is same region//
	if(n8OsdStrLen[OSD_DATEANDTIME] <= 0 && n8OsdStrLen[OSD_SPEEDANDMIL] <= 0)
	{
	}
	else if(n8OsdStrLen[OSD_DATEANDTIME] > 0 && n8OsdStrLen[OSD_SPEEDANDMIL] <= 0)
	{
		snprintf(str4OsdContent[0], MAX_EXTEND_OSD_LENGTH, "%s", str8OsdContent[OSD_DATEANDTIME]);
	}
	else
	{
		if(n8OsdStrLen[OSD_DATEANDTIME] <= 0 && n8OsdStrLen[OSD_SPEEDANDMIL] > 0)
		{
			memset(str8OsdContent[OSD_DATEANDTIME], 0x0, sizeof(str8OsdContent[OSD_DATEANDTIME]));
			str8OsdContent[OSD_DATEANDTIME][0] = ' ';
			n8OsdStrByte[OSD_DATEANDTIME] = strlen(str8OsdContent[OSD_DATEANDTIME]);
			n8OsdStrLen[OSD_DATEANDTIME] = m_pEnc->Ae_get_osd_string_width(str8OsdContent[OSD_DATEANDTIME], font_name, horizontal_scaler);
		}
		
		if(n8OsdStrLen[OSD_DATEANDTIME] + n8OsdStrLen[OSD_SPEEDANDMIL] > video_width)
		{
			snprintf(str4OsdContent[0], MAX_EXTEND_OSD_LENGTH, "%s  %s", str8OsdContent[OSD_DATEANDTIME], str8OsdContent[OSD_SPEEDANDMIL]);
		}
		else
		{
			nSpaceNum = (video_width - n8OsdStrLen[OSD_DATEANDTIME] - n8OsdStrLen[OSD_SPEEDANDMIL]) / nSpaceLen - 3;
			nSpaceNum = (nSpaceNum <= 2) ? 2 : nSpaceNum;
			nPos = n8OsdStrByte[OSD_DATEANDTIME];
			memcpy(str4OsdContent[0], str8OsdContent[OSD_DATEANDTIME], nPos);
			memset(str4OsdContent[0] + nPos, ' ', nSpaceNum);
			nPos += nSpaceNum;
			memcpy(str4OsdContent[0] + nPos, str8OsdContent[OSD_SPEEDANDMIL], n8OsdStrByte[OSD_SPEEDANDMIL]);
		}
	}
	pstuOsd[0].color = m_pstuOsdInfo[OSD_DATEANDTIME][u8ChipChn].color;
	pstuOsd[0].x = (m_pstuOsdInfo[OSD_DATEANDTIME][u8ChipChn].x + 3) / 4 * 4;
	pstuOsd[0].y = (m_pstuOsdInfo[OSD_DATEANDTIME][u8ChipChn].y + 3) / 4 * 4;
	pstuOsd[0].bShow = (n8OsdStrLen[OSD_DATEANDTIME] + n8OsdStrLen[OSD_SPEEDANDMIL] > 0) ? 1 : 0;
	memcpy(pstuOsd[0].szStr, str4OsdContent[0], MAX_EXTEND_OSD_LENGTH - 1);
	
	//REC status use one region///
	memcpy(str4OsdContent[1], str8OsdContent[OSD_RECSTATUS], n8OsdStrByte[OSD_RECSTATUS]);
	pstuOsd[1].color = m_pstuOsdInfo[OSD_RECSTATUS][u8ChipChn].color;
	pstuOsd[1].x = (m_pstuOsdInfo[OSD_RECSTATUS][u8ChipChn].x + 3) / 4 * 4;
	pstuOsd[1].y = (m_pstuOsdInfo[OSD_RECSTATUS][u8ChipChn].y + 3) / 4 * 4;
	pstuOsd[1].bShow = (n8OsdStrLen[OSD_RECSTATUS] > 0) ? 1 : 0;
	pstuOsd[1].align = m_pstuOsdInfo[OSD_RECSTATUS][u8ChipChn].align;
	memcpy(pstuOsd[1].szStr, str4OsdContent[1], MAX_EXTEND_OSD_LENGTH - 1);
	
	//USB status and fire-alarm is same region///
	if(n8OsdStrLen[OSD_USBSTATUS] <= 0 && n8OsdStrLen[OSD_FIREMONITOR] <= 0)
	{
	}
	else if(n8OsdStrLen[OSD_USBSTATUS] > 0 && n8OsdStrLen[OSD_FIREMONITOR] <= 0)
	{
		snprintf(str4OsdContent[2], MAX_EXTEND_OSD_LENGTH, "%s", str8OsdContent[OSD_USBSTATUS]);
	}
	else
	{
		if(n8OsdStrLen[OSD_USBSTATUS] <= 0 && n8OsdStrLen[OSD_FIREMONITOR] > 0)
		{
			memset(str8OsdContent[OSD_USBSTATUS], 0x0, sizeof(str8OsdContent[OSD_USBSTATUS]));
			str8OsdContent[OSD_USBSTATUS][0] = ' ';
			n8OsdStrByte[OSD_USBSTATUS] = strlen(str8OsdContent[OSD_USBSTATUS]);
			n8OsdStrLen[OSD_USBSTATUS] = m_pEnc->Ae_get_osd_string_width(str8OsdContent[OSD_USBSTATUS], font_name, horizontal_scaler);
		}
			
		if(n8OsdStrLen[OSD_USBSTATUS] + n8OsdStrLen[OSD_FIREMONITOR] > video_width)
		{
			snprintf(str4OsdContent[2], MAX_EXTEND_OSD_LENGTH, "%s  %s", str8OsdContent[OSD_USBSTATUS], str8OsdContent[OSD_FIREMONITOR]);
		}
		else
		{
			nSpaceNum = (video_width - n8OsdStrLen[OSD_USBSTATUS] - n8OsdStrLen[OSD_FIREMONITOR]) / nSpaceLen - 3;
			nSpaceNum = (nSpaceNum <= 2) ? 2 : nSpaceNum;
			nPos = n8OsdStrByte[OSD_USBSTATUS];
			memcpy(str4OsdContent[2], str8OsdContent[OSD_USBSTATUS], nPos);
			memset(str4OsdContent[2] + nPos, ' ', nSpaceNum);
			nPos += nSpaceNum;
			memcpy(str4OsdContent[2] + nPos, str8OsdContent[OSD_FIREMONITOR], n8OsdStrByte[OSD_FIREMONITOR]);
		}
	}
	pstuOsd[2].color = m_pstuOsdInfo[OSD_USBSTATUS][u8ChipChn].color;
	pstuOsd[2].x = (m_pstuOsdInfo[OSD_USBSTATUS][u8ChipChn].x + 3) / 4 * 4;
	pstuOsd[2].y = (m_pstuOsdInfo[OSD_USBSTATUS][u8ChipChn].y + 3) / 4 * 4;
	pstuOsd[2].bShow = (n8OsdStrLen[OSD_USBSTATUS] + n8OsdStrLen[OSD_FIREMONITOR] > 0) ? 1 : 0;
	memcpy(pstuOsd[2].szStr, str4OsdContent[2], MAX_EXTEND_OSD_LENGTH - 1);
	
	//train-num and chn-name is same region///
	if(n8OsdStrLen[OSD_TRAINANDNUM] <= 0 && n8OsdStrLen[OSD_CHNNAME] <= 0)
	{
	}
	else if(n8OsdStrLen[OSD_TRAINANDNUM] > 0 && n8OsdStrLen[OSD_CHNNAME] <= 0)
	{
		snprintf(str4OsdContent[3], MAX_EXTEND_OSD_LENGTH, "%s", str8OsdContent[OSD_TRAINANDNUM]);
	}
	else
	{
		if(n8OsdStrLen[OSD_TRAINANDNUM] <= 0 && n8OsdStrLen[OSD_CHNNAME] > 0)
		{
			memset(str8OsdContent[OSD_TRAINANDNUM], 0x0, sizeof(str8OsdContent[OSD_TRAINANDNUM]));
			str8OsdContent[OSD_TRAINANDNUM][0] = ' ';
			n8OsdStrByte[OSD_TRAINANDNUM] = strlen(str8OsdContent[OSD_TRAINANDNUM]);
			n8OsdStrLen[OSD_TRAINANDNUM] = m_pEnc->Ae_get_osd_string_width(str8OsdContent[OSD_TRAINANDNUM], font_name, horizontal_scaler);
		}
			
		if(n8OsdStrLen[OSD_TRAINANDNUM] + n8OsdStrLen[OSD_CHNNAME] > video_width)
		{
			snprintf(str4OsdContent[3], MAX_EXTEND_OSD_LENGTH, "%s  %s", str8OsdContent[OSD_TRAINANDNUM], str8OsdContent[OSD_CHNNAME]);
		}
		else
		{
			nSpaceNum = (video_width - n8OsdStrLen[OSD_TRAINANDNUM] - n8OsdStrLen[OSD_CHNNAME]) / nSpaceLen - 3;
			nSpaceNum = (nSpaceNum <= 2) ? 2 : nSpaceNum;
			nPos = n8OsdStrByte[OSD_TRAINANDNUM];
			memcpy(str4OsdContent[3], str8OsdContent[OSD_TRAINANDNUM], nPos);
			memset(str4OsdContent[3] + nPos, ' ', nSpaceNum);
			nPos += nSpaceNum;
			memcpy(str4OsdContent[3] + nPos, str8OsdContent[OSD_CHNNAME], n8OsdStrByte[OSD_CHNNAME]);
		}
	}
	pstuOsd[3].color = m_pstuOsdInfo[OSD_TRAINANDNUM][u8ChipChn].color;
	pstuOsd[3].x = (m_pstuOsdInfo[OSD_TRAINANDNUM][u8ChipChn].x + 3) / 4 * 4;
	pstuOsd[3].y = (m_pstuOsdInfo[OSD_TRAINANDNUM][u8ChipChn].y + 3) / 4 * 4;
	pstuOsd[3].bShow = (n8OsdStrLen[OSD_TRAINANDNUM] + n8OsdStrLen[OSD_CHNNAME] > 0) ? 1 : 0;
	memcpy(pstuOsd[3].szStr, str4OsdContent[3], MAX_EXTEND_OSD_LENGTH - 1);
    
    return 0;
}
#endif

//N9M
int CAvSnapTask::GetSnapEncodeOsd( unsigned char u8ChipChn, hi_snap_osd_t* pstuOsd, int osdNum, av_resolution_t eResolution )
{
#if defined(_AV_PLATFORM_HI3515_)
	if(PTD_6A_I == pgAvDeviceInfo->Adi_product_type())
	{
		return Get6AISnapEncoderOsd(u8ChipChn, pstuOsd, osdNum, eResolution);
	}
#endif

    for( int i=0; i<osdNum; i++ )
    {
        memcpy( &(pstuOsd[i]), &(m_pstuOsdInfo[i][u8ChipChn]), sizeof(hi_snap_osd_t) );
        pstuOsd[i].szStr[MAX_EXTEND_OSD_LENGTH - 1] = '\0';
    }
    
#if defined(_AV_PLATFORM_HI3520D_V300_)		//6AII_AV12 product snap osd is different///
	if(OSD_TEST_FLAG == pgAvDeviceInfo->Adi_product_type())
	{
		for( int i=0; i<osdNum; i++ )
		{
			if(pstuOsd[i].bShow == 1 && pstuOsd[i].type == 0) //date time id different///
			{
				datetime_t stuTime = {0};
				pgAvDeviceInfo->Adi_get_date_time( &stuTime );
				sprintf(pstuOsd[i].szStr, "%02d.%02d.%02d   %02d:%02d:%02d", stuTime.year, stuTime.month, stuTime.day, stuTime.hour, stuTime.minute, stuTime.second);
			}
			pstuOsd[i].x = (pstuOsd[i].x + 3 )/4*4;
			pstuOsd[i].y = (pstuOsd[i].y + 3 )/4*4;
		}
		return 0;
	}
#endif

    if( pstuOsd[0].bShow == 1 )
    {
        this->GetCurrentTime( pstuOsd[0].szStr );
    }
    if( pstuOsd[1].bShow == 1 )
    {
        m_pEnc->Ae_get_osd_content( (int)u8ChipChn, _AON_BUS_NUMBER_, pstuOsd[1].szStr, sizeof(pstuOsd[1].szStr) );
    }
    if( pstuOsd[2].bShow == 1 )
    {
        m_pEnc->Ae_get_osd_content( (int)u8ChipChn, _AON_SPEED_INFO_, pstuOsd[2].szStr, sizeof(pstuOsd[2].szStr) );
    }
    if( pstuOsd[3].bShow == 1 )
    {
        m_pEnc->Ae_get_osd_content( (int)u8ChipChn, _AON_GPS_INFO_, pstuOsd[3].szStr, sizeof(pstuOsd[3].szStr) );
    }

#if defined(_AV_PRODUCT_CLASS_IPC_)
    if( pstuOsd[5].bShow == 1 )
    {
        m_pEnc->Ae_get_osd_content( (int)u8ChipChn, _AON_SELFNUMBER_INFO_, pstuOsd[5].szStr, sizeof(pstuOsd[5].szStr) );
    }  

    if(PTD_712_VB == pgAvDeviceInfo->Adi_product_type())
    {
        /*add mac address to the end of channel name*/
        int len = strlen(pstuOsd[4].szStr);
        if(0 == len)
        {
            pgAvDeviceInfo->Adi_get_mac_address_as_string(pstuOsd[4].szStr, sizeof(pstuOsd[4].szStr));
        }
        else
        {
            pstuOsd[4].szStr[len] = '-';
            ++len;
            pgAvDeviceInfo->Adi_get_mac_address_as_string(pstuOsd[4].szStr+len, sizeof(pstuOsd[4].szStr)-len);
        }
        
        pstuOsd[4].szStr[sizeof(pstuOsd[4].szStr)-1] =  '\0';
    }
    
#endif

    for(int i=0; i<osdNum; ++i)
    {
        pstuOsd[i].x = (pstuOsd[i].x +3 )/4*4;
        pstuOsd[i].y = (pstuOsd[i].y +3 )/4*4;
    }
      
    return 0;
}

int CAvSnapTask::GetSnapEncodeOsd( unsigned char u8ChipChn, hi_snap_osd_t* pstuOsd )
{
    memcpy( &(pstuOsd[0]), &(m_pstuOsdInfo[0][u8ChipChn]), sizeof(hi_snap_osd_t) );
    memcpy( &(pstuOsd[1]), &(m_pstuOsdInfo[1][u8ChipChn]), sizeof(hi_snap_osd_t) );
    
    this->GetCurrentTime( pstuOsd[0].szStr );

    int nPicWidth = 0;  
    int nPicHeight = 0;

    pgAvDeviceInfo->Adi_get_video_size( m_eSnapRes, m_eSnapTvSystem, &nPicWidth, &nPicHeight );
#if defined(_AV_PRODUCT_CLASS_IPC_)
    pgAvDeviceInfo->Adi_covert_vidoe_size_by_rotate( &nPicWidth, &nPicHeight );
#endif
    
    pstuOsd[0].x = (pstuOsd[0].x * nPicWidth / 1024) / 4 * 4;
    pstuOsd[0].y = (pstuOsd[0].y * nPicHeight / 768) / 4 * 4;
    pstuOsd[1].x = (pstuOsd[1].x * nPicWidth / 1024) / 4 * 4;
    pstuOsd[1].y = (pstuOsd[1].y * nPicHeight / 768) / 4 * 4;
    
    return 0;
}

int CAvSnapTask::CheckSnapEncoderParamChanged()
{
    DEBUG_WARNING("CAvSnapTask::CheckSnapEncoderParamChanged m_eSnapRes=%d m_eSnapTvSystem=%d m_eTvSystem=%d\n"
        , m_eSnapRes, m_eSnapTvSystem, m_eTvSystem);

    bool bChanged = false;
 #if defined(_AV_PRODUCT_CLASS_HDVR_)
    bool bSnapEncoder = false;

    for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
    {
        if (pgAvDeviceInfo->Adi_get_video_source(chn) == _VS_ANALOG_)
        {
            bSnapEncoder = true;
            break;
        }
    }
    if (bSnapEncoder == false)
    {
        return 0;
    }
#endif  
/* �����ı�������жϣ��Դ��Ƿ���Ҫ���´���ץ�ı���*/
 /*   unsigned int len = CalcMaxLenChlName();
    if( len != m_u32MaxLenChlName )
    {
		m_pEnc->Ae_get_osd_content( (int)u8ChipChn, _AON_COMMON_OSD1_, pstuOsd[6].szStr, sizeof(pstuOsd[6].szStr) );
		DEBUG_MESSAGE("#### pstuOsd[3].szStr %s\n", pstuOsd[3].szStr);
    }
	if( pstuOsd[7].bShow == 1 )
    {
		m_pEnc->Ae_get_osd_content( (int)u8ChipChn, _AON_COMMON_OSD2_, pstuOsd[7].szStr, sizeof(pstuOsd[7].szStr) );
		DEBUG_MESSAGE("#### pstuOsd[3].szStr %s\n", pstuOsd[3].szStr);
    }
#endif
*/
    if( m_eSnapRes == _AR_UNKNOWN_ )
    {
        m_bCreatedSnap = false;

        av_resolution_t eRes = _AR_UNKNOWN_;
        int CurrentWait = m_ResWeight[_AR_UNKNOWN_];
        bool Encode_D1 = false;
        bool Encode_960H_WHD1 = false;
        for( int ch = 0; ch < m_u8MaxChlOnThisChip; ch++ )
        {
            if(m_ResWeight[m_eMainStreamRes[ch]] < 0)
            {
	            DEBUG_WARNING("#####Do not surport resolution %d######\n",m_eMainStreamRes[ch]);
                continue;
            }

            switch(m_eMainStreamRes[ch])
            {
                case _AR_D1_:
                    Encode_D1 = true;
                    break;
                case _AR_960H_WHD1_:
                    Encode_960H_WHD1 = true;
                    break;
                default:
                    break;
            }
            if( m_ResWeight[m_eMainStreamRes[ch]] < CurrentWait )
            {
                eRes = m_eMainStreamRes[ch];
                CurrentWait = m_ResWeight[m_eMainStreamRes[ch]];
            }
        }

        if( eRes == _AR_UNKNOWN_ || m_eTvSystem == _AT_UNKNOWN_ )
        {
	        DEBUG_WARNING( "CAvSnapTask::CreateSnapEncoder create snap encoder failed, Res=%d tvSys=%d\n", eRes, m_eTvSystem );
            return -1;
        }

        //D1 and 960H_WHD1 exist in mainstream at same time, then set eRes to _AR_HD1_
        if(true == Encode_D1 && true == Encode_960H_WHD1 )
        {
            eRes = _AR_HD1_;
            CurrentWait = 2;
        }

        m_eSnapRes = eRes;
        m_eSnapTvSystem = m_eTvSystem;
	    DEBUG_WARNING( "CAvSnapTask::CreateSnapEncoder create snap encoder ok, res=%d tvSys=%d\n", eRes, m_eTvSystem );
        return 0;
        //return this->CreateSnapEncoder();
    }
    
    if( (m_eSnapTvSystem == m_eTvSystem) && (!bChanged) )
    {
        av_resolution_t eRes = _AR_UNKNOWN_;
        int CurrentWait = m_ResWeight[_AR_UNKNOWN_];
        for( int ch=0; ch<m_u8MaxChlOnThisChip; ch++ )
        {
            if( m_ResWeight[m_eMainStreamRes[ch]] < CurrentWait )
            {
                eRes = m_eMainStreamRes[ch];
                CurrentWait = m_ResWeight[m_eMainStreamRes[ch]];
            }
        }

        if( eRes == m_eSnapRes )
        {
            if( eRes != _AR_UNKNOWN_ )
            {
                return 0;
            }
            
            return -1;
        }
    }

    //this->DestroySnapEncoder();
	_AV_DEBUG_INFO_(_AVD_MSG_, "CAvSnapTask::CheckSnapEncoderParamChanged()\n");
    //return this->CreateSnapEncoder();
    return 0;
}

#if defined(_AV_PRODUCT_CLASS_IPC_)
int CAvSnapTask::CreateSnapEncoder(int chn, av_resolution_t  ucResolution, rm_uint8_t  ucQuality, hi_snap_osd_t pSnapOsd[], int osd_num)
{ 
    unsigned int factor = 0;
    switch (ucQuality)
    {
        case 1 :
            factor = 90;
            break;
        case 2 :
            factor = 85;
            break;
        case 3 : 
            factor = 80;
            break;
        case 4 : 
            factor = 75;
            break;
        case 5 : 
            factor = 70;
            break;
        case 6 : 
            factor = 65;
            break;
        case 7 : 
            factor = 60;
            break;
        case 8 : 
            factor = 55;
            break;
        default :
            break;

    }
    
    if( m_pEnc->Ae_create_snap_encoder( ucResolution, m_eTvSystem, factor, pSnapOsd, osd_num) != 0 )
    {
        DEBUG_WARNING( "CAvSnapTask::CreateSnapEncoder m_pEnc->Ae_create_snap_encoder failed\n" );
        return -1;
    }
    m_eSnapTvSystem = m_eTvSystem;
    m_bCreatedSnap = true;
    DEBUG_WARNING( "CAvSnapTask::CreateSnapEncoder create snap encoder ok, tvSys=%d\n", m_eTvSystem );

    return 0;
}    
#else
int CAvSnapTask::CreateSnapEncoder(int chn, rm_uint8_t  ucResolution, rm_uint8_t  ucQuality, hi_snap_osd_t pSnapOsd[], int osd_num)
{
    av_resolution_t eRes = _AR_UNKNOWN_;
   
    switch (ucResolution)
    {
        case 0 :
            eRes = _AR_CIF_;
            break;
        case 1 :
            eRes = _AR_HD1_;
            break;
        case 2 : 
            eRes = _AR_D1_;
            break;
        case 3 : 
            eRes = _AR_QCIF_;
            break;
        case 4 : 
            eRes = _AR_QVGA_;
            break;
        case 5 : 
            eRes = _AR_VGA_;
            break;
        case 6 : 
            eRes = _AR_720_;
            break;
        case 7 : 
            eRes = _AR_1080_;
            break;
        case 8:
            eRes = _AR_3M_;
            break;
        case 9:
            eRes = _AR_5M_;
            break;
        case 10 : 
            eRes = _AR_960H_WQCIF_;
            break;
        case 11 : 
            eRes = _AR_960H_WCIF_;
            break;
        case 12 : 
            eRes = _AR_960H_WHD1_;
            break;
        case 13 : 
            eRes = _AR_960H_WD1_;
            break;
        case 14 : 
            eRes = _AR_960P_;
            break;
        case 15 : 
            eRes = _AR_Q1080P_;
            break;
        case 16 : 
            eRes = _AR_SVGA_;
            break;
        case 17 : 
            eRes = _AR_XGA_;
            break;

        default :
            eRes = _AR_D1_;
            break;

    }
    unsigned int factor = 0;
    switch (ucQuality)
    {
        case 1 :
            factor = 90;
            break;
        case 2 :
            factor = 85;
            break;
        case 3 : 
            factor = 80;
            break;
        case 4 : 
            factor = 75;
            break;
        case 5 : 
            factor = 70;
            break;
        case 6 : 
            factor = 65;
            break;
        case 7 : 
            factor = 60;
            break;
        case 8 : 
            factor = 55;
            break;
        default :
            break;

    }
	if(PTD_6A_II_AVX == pgAvDeviceInfo->Adi_product_type())	//6AII_AVX snap picture size is limit, not overpace 64K///
	{
		factor -= 30;
	}
#if defined(_AV_PLATFORM_HI3515_)
	//3515 hisi-chip must set the region X and Y position when create region, so calculate X and Y first of all//
	this->CoordinateTrans(eRes, m_eTvSystem, pSnapOsd, osd_num);
	
    if( m_pEnc->Ae_create_snap_encoder( chn, eRes, m_eTvSystem, factor, pSnapOsd, osd_num) != 0 )
#else
    if( m_pEnc->Ae_create_snap_encoder( eRes, m_eTvSystem, factor, pSnapOsd, osd_num) != 0 )
#endif
    {
        DEBUG_ERROR( "CAvSnapTask::CreateSnapEncoder m_pEnc->Ae_create_snap_encoder failed\n" );
        return -1;
    }
    m_eSnapRes = eRes;
    m_eSnapTvSystem = m_eTvSystem;
    m_bCreatedSnap = true;
    DEBUG_MESSAGE( "CAvSnapTask::CreateSnapEncoder create snap encoder ok, res=%d tvSys=%d\n", eRes, m_eTvSystem );

    return 0;
}
#endif


#if !defined(_AV_PRODUCT_CLASS_IPC_)
int CAvSnapTask::MainStreamToSnapResolution(int mainResolution)
{
    //main 
    //snap /**< ֱ 0-CIF 1-HD1 2-D1 3-QCIF 4-QVGA 5-VGA 6-720P 7-1080P 8-3MP(2048*1536) 9-5MP(2592*1920) 10 WQCIF11 WCIF12 WHD113 WD1(960H) 14-960P 15-Q1080P*/
    int res = 0;
    switch(mainResolution)
    {

        case 1:
            res = 2;
            break;
        case 2:
            res = 1;
            break;
        case 3:
            res = 0;
            break;
        case 4:
            res = 3;
            break;
        case 5:
            res = 13;
            break;
        case 6:
            res = 12;
            break;
        case 7:
            res = 11;
            break;
        case 8:
            res = 10;
            break;
        case 9:
            res = 7;
            break;
        case 10:
            res = 6;
            break;
        case 11:
            res = 5;
            break;
        case 12:
            res = 4;
            break;
        case 13:
            res = 14;
            break;
        case 14:
            res = 15;
            break;
        case 15:
            res = 8;
            break;
        case 16:
            res = 9;
            break;
        default :
            res =1;
            break;
            
    }
    return res;
    
}

int CAvSnapTask::SnapResolutionToMainStream(int snapResolution)
{
    //main 
    //snap /**< ֱ 0-CIF 1-HD1 2-D1 3-QCIF 4-QVGA 5-VGA 6-720P 7-1080P 8-3MP(2048*1536) 9-5MP(2592*1920) 10 WQCIF11 WCIF12 WHD113 WD1(960H) 14-960P 15-Q1080P*/
    int res = 0;
    switch(snapResolution)
    {
        case 0:
            res = 3;
            break;
        case 1:
            res = 2;
            break;
        case 2:
            res = 1;
            break;
        case 3:
            res = 4;
            break;
        case 4:
            res = 12;
            break;
        case 5:
            res = 11;
            break;
        case 6:
            res = 10;
            break;
        case 7:
            res = 9;
            break;
        case 8:
            res = 15;
            break;
        case 9:
            res = 16;
            break;
        case 10:
            res = 8;
            break;
        case 11:
            res = 7;
            break;
        case 12:
            res = 6;
            break;
        case 13:
            res = 5;
            break;
        case 14:
            res = 13;
            break;
        case 15:
            res = 9;
            break;
        
        default :
            res =1;
            break;
            
    }
    return res;
    
}
int CAvSnapTask::DestroySnapEncoder(hi_snap_osd_t *stuOsdInfo)
{
    if( m_pEnc->Ae_destroy_snap_encoder(stuOsdInfo) != 0 )
    {
        DEBUG_ERROR( "CAvSnapTask::DestroySnapEncoder m_pEnc->Ae_destroy_snap_encoder failed\n" );
        return -1;
    }
    m_bCreatedSnap = false;
    return 0;
}
int CAvSnapTask::AddBoardcastSnapResult(char flag, int serialNumber, int curSnapNumber, AvSnapPara_t stuSnapParam)
{
    msgSnapResultBoardcast stuSnapResult;
    memset(&stuSnapResult, 0, sizeof(msgSnapResultBoardcast));
    
    stuSnapResult.uiPictureSerialnumber = serialNumber;
    stuSnapResult.uiSanpCurNumber = curSnapNumber+1;
    stuSnapResult.uiSnapTaskNumber = stuSnapParam.uiSnapTaskNumber;
    stuSnapResult.uiSnapTotalNumber = stuSnapParam.usNumber;
    stuSnapResult.uiUser = stuSnapParam.uiUser;
    stuSnapResult.usSnapResult = flag;                       
    listSnapResult.push_back(stuSnapResult);
    return 0;
}
#endif

int CAvSnapTask::StartSnapThread()
{

    DEBUG_MESSAGE("#######start snap thread, is running=%d\n", m_bIsTaskRunning);
    if( m_bIsTaskRunning )
    {
        return 0;
    }

    m_bNeedExitThread = false;
    m_eTaskState = SNAP_THREAD_CHANGED_ENCODER;
    Av_create_normal_thread(SnapThreadEntry, (void*)this, NULL);

    while(m_bIsTaskRunning != true)
    {
        mSleep(10);
    }

    DEBUG_MESSAGE("CAvSnapTask::StartSnapThread create thread ok\n");

    return 0;
}

int CAvSnapTask::StopSnapThread()
{
    m_bNeedExitThread = true;
    
    while( m_bIsTaskRunning == true )
    {
        mSleep(10);
    }
    
    return 0;
}

int CAvSnapTask::GetCurrentTime( char* pszTimeStr )
{
    datetime_t stuTime = {0};
    pgAvDeviceInfo->Adi_get_date_time( &stuTime );
    char product_group[32] = {0};
    pgAvDeviceInfo->Adi_get_product_group(product_group, sizeof(product_group));

    if( m_u8DateMode == _AV_DATE_MODE_ddmmyy_ )
    {
        sprintf(pszTimeStr, "%02d/%02d/20%02d", stuTime.day, stuTime.month, stuTime.year);
    }
    else if( m_u8DateMode == _AV_DATE_MODE_mmddyy_ )
    {
        sprintf(pszTimeStr, "%02d/%02d/20%02d", stuTime.month, stuTime.day, stuTime.year);
    }
    else
    {
        sprintf(pszTimeStr, "20%02d-%02d-%02d", stuTime.year, stuTime.month, stuTime.day);
    }
    if(strcmp(product_group, "ITS") != 0)
    {
        if( m_u8TimeMode == _AV_TIME_MODE_12_ )
        {
            if (stuTime.hour > 12)
            {
                sprintf(pszTimeStr, "%s %02d:%02d:%02d PM", pszTimeStr, stuTime.hour - 12, stuTime.minute, stuTime.second);
            }
            else if(stuTime.hour == 12)
            {
                sprintf(pszTimeStr, "%s %02d:%02d:%02d PM", pszTimeStr, stuTime.hour, stuTime.minute, stuTime.second);
            }
            else if(stuTime.hour == 0)
            {
                sprintf(pszTimeStr, "%s 12:%02d:%02d AM", pszTimeStr,  stuTime.minute, stuTime.second);
            }
            else
            {
                sprintf(pszTimeStr, "%s %02d:%02d:%02d AM", pszTimeStr, stuTime.hour, stuTime.minute, stuTime.second);                  
            }     
        }
        else
        {
            sprintf( pszTimeStr, "%s %02d:%02d:%02d", pszTimeStr, stuTime.hour, stuTime.minute, stuTime.second);
        }
    }
    else//
    {
        if( m_u8TimeMode == _AV_TIME_MODE_12_ )
        {
            if (stuTime.hour > 12)
            {
                sprintf(pszTimeStr, "%s  %02d:%02d:%02d PM", pszTimeStr, stuTime.hour - 12, stuTime.minute, stuTime.second);
            }
            else if(stuTime.hour == 12)
            {
                sprintf(pszTimeStr, "%s  %02d:%02d:%02d PM", pszTimeStr, stuTime.hour, stuTime.minute, stuTime.second);
            }
            else if(stuTime.hour == 0)
            {
                sprintf(pszTimeStr, "%s  12:%02d:%02d AM", pszTimeStr,  stuTime.minute, stuTime.second);
            }
            else
            {
                sprintf(pszTimeStr, "%s  %02d:%02d:%02d AM", pszTimeStr, stuTime.hour, stuTime.minute, stuTime.second);                  
            }     
        }
        else
        {
            sprintf( pszTimeStr, "%s  %02d:%02d:%02d", pszTimeStr, stuTime.hour, stuTime.minute, stuTime.second);
        }
    }
    return 0;
}

bool CAvSnapTask::RgnIsPassed(snap_osd_item_t &regin, const std::vector<snap_osd_item_t> &passed_path) const
{
    if(passed_path.empty())
     {
        return false;
     }

    std::vector<snap_osd_item_t>::const_iterator iter = passed_path.begin();
    while(iter != passed_path.end())
     {
        if((regin.visited== iter->visited) && \
                (regin.osdname== iter->osdname) )
        {
            return true;
        }

        ++iter;
     }

    return false;
}

bool CAvSnapTask::SnapIsReginOverlapWithExistRegins(const snap_osd_item_t &regin, snap_osd_item_t *overlap_regin) const
{

    std::list<snap_osd_item_t>::const_iterator av_osd_merge_item_it;
    av_osd_merge_item_it = m_listOsdItem.begin();
    while(av_osd_merge_item_it != m_listOsdItem.end())
    {
        if((true == av_osd_merge_item_it->visited))
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
                }
                
                return true;
            }
     
        }
        ++av_osd_merge_item_it;
    }

    return false;
}

int CAvSnapTask::GetRGNAttr(av_resolution_t eResolution, av_tvsystem_t eTvSystem, int index, hi_snap_osd_t &pSnapOsd, int &width, int &height, int &video_width, int &video_height)
{
    int fontWidth, fontHeight;
    int horScaler, verScaler;
    int charactorNum = 0;
    int fontName = 0;

#ifdef _AV_PLATFORM_HI3515_
    REGION_ATTR_S stuRgnAttr;
    memset(&stuRgnAttr, 0, sizeof(REGION_ATTR_S));
#else
	RGN_ATTR_S stuRgnAttr;
	memset(&stuRgnAttr, 0, sizeof(RGN_ATTR_S));
#endif

#if defined(_AV_PLATFORM_HI3520D_V300_)
	if(OSD_TEST_FLAG == pgAvDeviceInfo->Adi_product_type())
	{
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
        m_pEnc->Ae_get_osd_parameter( _AON_EXTEND_OSD, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
        charactorNum = strlen(pSnapOsd.szStr);
		if(pSnapOsd.type == 0)	//date time is different///
		{
			switch(eResolution)
			{
				case _AR_1080_:
				case _AR_720_:
				case _AR_960P_:
					stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler) / 2 + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
					stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + 4 ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
					break;
				default:
					stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler)*2/3	+ fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
					stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + 4 ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
					break;
			}
		}
		else
		{
			CAvUtf8String channel_name(pSnapOsd.szStr);
			av_ucs4_t char_unicode;
			av_char_lattice_info_t lattice_char_info;
			while(channel_name.Next_char(&char_unicode) == true)
			{
				if(m_pAv_font_library->Afl_get_char_lattice(fontName, char_unicode, &lattice_char_info) == 0)
				{
					stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
				}
				else
				{
					//DEBUG_ERROR( "EXTEND_OSD: Afl_get_char_lattice failed, unicode = %ld, font_name = %d\n", char_unicode, fontName);
					continue;//return -1;
				}
			}
			if(stuRgnAttr.unAttr.stOverlay.stSize.u32Width == 0)
			{
				DEBUG_ERROR( "EXTEND_OSD: Ae_create_osd_region (stuRgnAttr.unAttr.stOverlay.stSize.u32Width == 0), index = %d\n", index);
				return -1;
			}
			stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
			stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
		}
        pgAvDeviceInfo->Adi_covert_coordinate(eResolution, eTvSystem, &(pSnapOsd.x), &(pSnapOsd.y), 4);
		pgAvDeviceInfo->Adi_get_video_size(eResolution, eTvSystem, &video_width, &video_height);
		
		if(stuRgnAttr.unAttr.stOverlay.stSize.u32Width > (unsigned int)video_width)
		{
			stuRgnAttr.unAttr.stOverlay.stSize.u32Width = video_width;
		}
		
		if(stuRgnAttr.unAttr.stOverlay.stSize.u32Height > (unsigned int)video_height)
		{
			stuRgnAttr.unAttr.stOverlay.stSize.u32Height = video_height;
		}
		stuRgnAttr.unAttr.stOverlay.stSize.u32Width = stuRgnAttr.unAttr.stOverlay.stSize.u32Width / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
		stuRgnAttr.unAttr.stOverlay.stSize.u32Height = stuRgnAttr.unAttr.stOverlay.stSize.u32Height / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;

		width = stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
		height = stuRgnAttr.unAttr.stOverlay.stSize.u32Height;
		
        if(pSnapOsd.x < 0)
        {
            pSnapOsd.x = 0;
        }
        if(pSnapOsd.align == 1)	//right align///
        {
			if((pSnapOsd.x + width) < (video_width - 5))
			{
				pSnapOsd.x = video_width - width;
			}
        }
        /*else
        {
			if((pSnapOsd.x + (int)stuRgnAttr.unAttr.stOverlay.stSize.u32Width) > video_width )
			{
				pSnapOsd.x = video_width - stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
			}
        }*/
		
        if(pSnapOsd.x < 0)
        {
            pSnapOsd.x = 0;
        }

        if(pSnapOsd.y < 0)
        {
            pSnapOsd.y = 0;
        }
        if(((pSnapOsd.y + (int)stuRgnAttr.unAttr.stOverlay.stSize.u32Height)>video_height))
        {
            pSnapOsd.y = video_height - stuRgnAttr.unAttr.stOverlay.stSize.u32Height;
        }
        if(pSnapOsd.y < 0)
        {
            pSnapOsd.y = 0;
        }
		return 0;
	}
#elif defined(_AV_PLATFORM_HI3515_)
		stuRgnAttr.unAttr.stOverlay.stRect.u32Width  = 0;
		m_pEnc->Ae_get_osd_parameter( _AON_EXTEND_OSD, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
		charactorNum = strlen(pSnapOsd.szStr);
		if(pSnapOsd.type == 0)	//date time is different///
		{
			switch(eResolution)
			{
				case _AR_1080_:
				case _AR_720_:
				case _AR_960P_:
					stuRgnAttr.unAttr.stOverlay.stRect.u32Width  = ((fontWidth * charactorNum * horScaler) / 2 + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
					stuRgnAttr.unAttr.stOverlay.stRect.u32Height = (fontHeight * verScaler + 4 ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
					break;
				default:
					stuRgnAttr.unAttr.stOverlay.stRect.u32Width  = ((fontWidth * charactorNum * horScaler)*2/3	+ fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
					stuRgnAttr.unAttr.stOverlay.stRect.u32Height = (fontHeight * verScaler + 4 ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
					break;
			}
		}
		else
		{
			CAvUtf8String channel_name(pSnapOsd.szStr);
			av_ucs4_t char_unicode;
			av_char_lattice_info_t lattice_char_info;
			while(channel_name.Next_char(&char_unicode) == true)
			{
				if(m_pAv_font_library->Afl_get_char_lattice(fontName, char_unicode, &lattice_char_info) == 0)
				{
					stuRgnAttr.unAttr.stOverlay.stRect.u32Width += lattice_char_info.width;
				}
				else
				{
					//DEBUG_ERROR( "EXTEND_OSD: Afl_get_char_lattice failed, unicode = %ld, font_name = %d\n", char_unicode, fontName);
					continue;//return -1;
				}
			}
			if(stuRgnAttr.unAttr.stOverlay.stRect.u32Width == 0)
			{
				DEBUG_ERROR( "EXTEND_OSD: Ae_create_osd_region (stuRgnAttr.unAttr.stOverlay.stSize.u32Width == 0), index = %d\n", index);
				return -1;
			}
			stuRgnAttr.unAttr.stOverlay.stRect.u32Width  = (stuRgnAttr.unAttr.stOverlay.stRect.u32Width * horScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
			stuRgnAttr.unAttr.stOverlay.stRect.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
		}
		
		pgAvDeviceInfo->Adi_covert_coordinate(eResolution, eTvSystem, &(pSnapOsd.x), &(pSnapOsd.y), 8);
		
		pgAvDeviceInfo->Adi_get_video_size(eResolution, eTvSystem, &video_width, &video_height);
		
		if(stuRgnAttr.unAttr.stOverlay.stRect.u32Width > (unsigned int)video_width)
		{
			stuRgnAttr.unAttr.stOverlay.stRect.u32Width = video_width;
		}
		
		if(stuRgnAttr.unAttr.stOverlay.stRect.u32Height > (unsigned int)video_height)
		{
			stuRgnAttr.unAttr.stOverlay.stRect.u32Height = video_height;
		}
		stuRgnAttr.unAttr.stOverlay.stRect.u32Width = stuRgnAttr.unAttr.stOverlay.stRect.u32Width / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
		stuRgnAttr.unAttr.stOverlay.stRect.u32Height = stuRgnAttr.unAttr.stOverlay.stRect.u32Height / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;

		width = stuRgnAttr.unAttr.stOverlay.stRect.u32Width;
		height = stuRgnAttr.unAttr.stOverlay.stRect.u32Height;
		
		if(pSnapOsd.x < 0)
		{
			pSnapOsd.x = 0;
		}
		if(pSnapOsd.align == 1) //right align///
		{
			if((pSnapOsd.x + width) < (video_width - 5))
			{
				pSnapOsd.x = video_width - width;
			}
		}
		/*else
		{
			if((pSnapOsd.x + (int)stuRgnAttr.unAttr.stOverlay.stSize.u32Width) > video_width )
			{
				pSnapOsd.x = video_width - stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
			}
		}*/
		
		if(pSnapOsd.x < 0)
		{
			pSnapOsd.x = 0;
		}

		if(pSnapOsd.y < 0)
		{
			pSnapOsd.y = 0;
		}
		if(((pSnapOsd.y + (int)stuRgnAttr.unAttr.stOverlay.stRect.u32Height)>video_height))
		{
			pSnapOsd.y = video_height - stuRgnAttr.unAttr.stOverlay.stRect.u32Height;
		}
		if(pSnapOsd.y < 0)
		{
			pSnapOsd.y = 0;
		}
		return 0;
#endif
    
#ifndef _AV_PLATFORM_HI3515_
    if( index == 0 )//date
    {
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
#if defined(_AV_PRODUCT_CLASS_IPC_)
        m_pEnc->Ae_get_osd_parameter( _AON_DATE_TIME_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
#else
        m_pEnc->Ae_get_osd_parameter( _AON_DATE_TIME_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
#endif
        charactorNum = strlen(pSnapOsd.szStr);
        //DEBUG_MESSAGE("pSnapOsd[%d].szStr %s\n", index, pSnapOsd.szStr);
        switch(eResolution)
        {
            case _AR_1080_:
            case _AR_720_:
            case _AR_960P_:
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler) / 2 + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +4 ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
            default:
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler)  + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +4 ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
        }
        width = stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
        height = stuRgnAttr.unAttr.stOverlay.stSize.u32Height;
        //DEBUG_MESSAGE("GetRGNAttr index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
        //DEBUG_MESSAGE("GetRGNAttr before Adi_covert_coordinate pSnapOsd[%d].x = %d, pSnapOsd[%d].y = %d\n", index, pSnapOsd.x, index, pSnapOsd.y);
        pgAvDeviceInfo->Adi_covert_coordinate(eResolution, eTvSystem, &(pSnapOsd.x), &(pSnapOsd.y), 4);
        //DEBUG_MESSAGE("GetRGNAttr after Adi_covert_coordinate pSnapOsd[%d].x = %d, pSnapOsd[%d].y = %d\n", index, pSnapOsd.x, index, pSnapOsd.y);
        pgAvDeviceInfo->Adi_get_video_size(eResolution, eTvSystem, &video_width, &video_height);
        //DEBUG_MESSAGE("GetRGNAttr index = %d video_width = %d video_height = %d\n",index, video_width, video_height);

        if(pSnapOsd.x < 0)
        {
            pSnapOsd.x = 0;
        }
        if((pSnapOsd.x + (int)stuRgnAttr.unAttr.stOverlay.stSize.u32Width) > video_width )
        {
            pSnapOsd.x = video_width - stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
        }
        if(pSnapOsd.x < 0)
        {
            pSnapOsd.x = 0;
        }

        if(pSnapOsd.y < 0)
        {
            pSnapOsd.y = 0;
        }
        if(((pSnapOsd.y + (int)stuRgnAttr.unAttr.stOverlay.stSize.u32Height)>video_height))
        {
            pSnapOsd.y = video_height - stuRgnAttr.unAttr.stOverlay.stSize.u32Height;
        }
        if(pSnapOsd.y < 0)
        {
            pSnapOsd.y = 0;
        }

        #if defined(_AV_PRODUCT_CLASS_IPC_)
    if(PTD_712_VB == pgAvDeviceInfo->Adi_product_type())
    {
        if((index== 0) || (index == 4))
        {
             int video_width = 0;
             int video_height = 0;
             pgAvDeviceInfo->Adi_get_video_size(eResolution, eTvSystem, &video_width, &video_height);
             pgAvDeviceInfo->Adi_covert_vidoe_size_by_rotate(&video_width, &video_height);    
            
             pSnapOsd.x= video_width - stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
             pSnapOsd.y = video_height - stuRgnAttr.unAttr.stOverlay.stSize.u32Height;
        }
    }
#endif
    }
    else if(index == 1)//bus_number
    {
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
#if defined(_AV_PRODUCT_CLASS_IPC_)
        m_pEnc->Ae_get_osd_parameter( _AON_BUS_NUMBER_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );        
#else
        m_pEnc->Ae_get_osd_parameter( _AON_BUS_NUMBER_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
#endif
#if defined(_AV_PRODUCT_CLASS_IPC_)
        char vehicle[16];
        memset(vehicle, 47, sizeof(vehicle));
        vehicle[sizeof(vehicle)-1]='\0';
        CAvUtf8String bus_number(vehicle);
#else
        char tempstring[128] = {0};
        strcat(tempstring,pSnapOsd.szStr);
        CAvUtf8String bus_number(tempstring);
#endif

        av_ucs4_t char_unicode;
        av_char_lattice_info_t lattice_char_info;
        while(bus_number.Next_char(&char_unicode) == true)
        {
#if defined(_AV_PRODUCT_CLASS_IPC_)
            if(m_pAv_font_library->Afl_get_char_lattice(fontName, char_unicode, &lattice_char_info) == 0)
#else
            if(m_pAv_font_library->Afl_get_char_lattice(fontName, char_unicode, &lattice_char_info) == 0)
#endif
            {
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
            }
            else
            {
                //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                continue;//return -1;
            }
        }
        if(stuRgnAttr.unAttr.stOverlay.stSize.u32Width == 0)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        width = stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
        height = stuRgnAttr.unAttr.stOverlay.stSize.u32Height;

        //DEBUG_MESSAGE("GetRGNAttr index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
        //DEBUG_MESSAGE("GetRGNAttr before Adi_covert_coordinate pSnapOsd[%d].x = %d, pSnapOsd[%d].y = %d\n", index, pSnapOsd.x, index, pSnapOsd.y);
        pgAvDeviceInfo->Adi_covert_coordinate(eResolution, eTvSystem, &(pSnapOsd.x), &(pSnapOsd.y), 4);
        //DEBUG_MESSAGE("GetRGNAttr after Adi_covert_coordinate pSnapOsd[%d].x = %d, pSnapOsd[%d].y = %d\n", index, pSnapOsd.x, index, pSnapOsd.y);
        pgAvDeviceInfo->Adi_get_video_size(eResolution, eTvSystem, &video_width, &video_height);
        //DEBUG_MESSAGE("GetRGNAttr index = %d video_width = %d video_height = %d\n",index, video_width, video_height);

        if(pSnapOsd.x < 0)
        {
            pSnapOsd.x = 0;
        }
        if((pSnapOsd.x + (int)stuRgnAttr.unAttr.stOverlay.stSize.u32Width) > video_width )
        {
            pSnapOsd.x = video_width - stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
        }
        if(pSnapOsd.x < 0)
        {
            pSnapOsd.x = 0;
        }

        if(pSnapOsd.y < 0)
        {
            pSnapOsd.y = 0;
        }
        if(((pSnapOsd.y + (int)stuRgnAttr.unAttr.stOverlay.stSize.u32Height)>video_height))
        {
            pSnapOsd.y = video_height - stuRgnAttr.unAttr.stOverlay.stSize.u32Height;
        }
        if(pSnapOsd.y < 0)
        {
            pSnapOsd.y = 0;
        }
    }
    else if(index == 2)//speed
    {
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;        
#if defined(_AV_PRODUCT_CLASS_IPC_)
        m_pEnc->Ae_get_osd_parameter( _AON_SPEED_INFO_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
        charactorNum = 16;
#else
        m_pEnc->Ae_get_osd_parameter( _AON_SPEED_INFO_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
        charactorNum = 8;
#endif
        switch(eResolution)
        {
            case _AR_1080_:
            case _AR_720_:
            case _AR_960P_:
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler) / 2 + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
            default:
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler)* 2 / 3  + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;

        }
        width = stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
        height = stuRgnAttr.unAttr.stOverlay.stSize.u32Height;

        //DEBUG_MESSAGE("GetRGNAttr index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);

        //DEBUG_MESSAGE("GetRGNAttr before Adi_covert_coordinate pSnapOsd[%d].x = %d, pSnapOsd[%d].y = %d\n", index, pSnapOsd.x, index, pSnapOsd.y);
        pgAvDeviceInfo->Adi_covert_coordinate(eResolution, eTvSystem, &(pSnapOsd.x), &(pSnapOsd.y), 4);
        //DEBUG_MESSAGE("GetRGNAttr after Adi_covert_coordinate pSnapOsd[%d].x = %d, pSnapOsd[%d].y = %d\n", index, pSnapOsd.x, index, pSnapOsd.y);
        pgAvDeviceInfo->Adi_get_video_size(eResolution, eTvSystem, &video_width, &video_height);
        //DEBUG_MESSAGE("GetRGNAttr index = %d video_width = %d video_height = %d\n",index, video_width, video_height);

        if(pSnapOsd.x < 0)
        {
            pSnapOsd.x = 0;
        }
        if((pSnapOsd.x + (int)stuRgnAttr.unAttr.stOverlay.stSize.u32Width) > video_width )
        {
            pSnapOsd.x = video_width - stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
        }
        if(pSnapOsd.x < 0)
        {
            pSnapOsd.x = 0;
        }

        if(pSnapOsd.y < 0)
        {
            pSnapOsd.y = 0;
        }
        if(((pSnapOsd.y + (int)stuRgnAttr.unAttr.stOverlay.stSize.u32Height)>video_height))
        {
            pSnapOsd.y = video_height - stuRgnAttr.unAttr.stOverlay.stSize.u32Height;
        }
        if(pSnapOsd.y < 0)
        {
            pSnapOsd.y = 0;
        }
    }
    else if(index == 3)//gps
    {
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
#if defined(_AV_PRODUCT_CLASS_IPC_)
       m_pEnc->Ae_get_osd_parameter( _AON_GPS_INFO_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
       charactorNum = 32; 
#else
        m_pEnc->Ae_get_osd_parameter( _AON_GPS_INFO_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
        charactorNum = 25;
#endif
        switch(eResolution)
        {
            case _AR_1080_:
            case _AR_720_:
            case _AR_960P_:
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler) / 2 + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
            default:
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler)* 2 / 3  + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;

        }
        width = stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
        height = stuRgnAttr.unAttr.stOverlay.stSize.u32Height;

        //DEBUG_MESSAGE("GetRGNAttr index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
        //DEBUG_MESSAGE("GetRGNAttr before Adi_covert_coordinate pSnapOsd[%d].x = %d, pSnapOsd[%d].y = %d\n", index, pSnapOsd.x, index, pSnapOsd.y);
        pgAvDeviceInfo->Adi_covert_coordinate(eResolution, eTvSystem, &(pSnapOsd.x), &(pSnapOsd.y), 4);
        //DEBUG_MESSAGE("GetRGNAttr after Adi_covert_coordinate pSnapOsd[%d].x = %d, pSnapOsd[%d].y = %d\n", index, pSnapOsd.x, index, pSnapOsd.y);
        pgAvDeviceInfo->Adi_get_video_size(eResolution, eTvSystem, &video_width, &video_height);
        //DEBUG_MESSAGE("GetRGNAttr index = %d video_width = %d video_height = %d\n",index, video_width, video_height);

        if(pSnapOsd.x < 0)
        {
            pSnapOsd.x = 0;
        }
        if((pSnapOsd.x + (int)stuRgnAttr.unAttr.stOverlay.stSize.u32Width) > video_width )
        {
            pSnapOsd.x = video_width - stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
        }
        if(pSnapOsd.x < 0)
        {
            pSnapOsd.x = 0;
        }

        if(pSnapOsd.y < 0)
        {
            pSnapOsd.y = 0;
        }
        if(((pSnapOsd.y + (int)stuRgnAttr.unAttr.stOverlay.stSize.u32Height)>video_height))
        {
            pSnapOsd.y = video_height - stuRgnAttr.unAttr.stOverlay.stSize.u32Height;
        }
        if(pSnapOsd.y < 0)
        {
            pSnapOsd.y = 0;
        }
    }
    else if(index == 4)//chn_name
    {

        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
#if defined(_AV_PRODUCT_CLASS_IPC_)
        m_pEnc->Ae_get_osd_parameter( _AON_CHN_NAME_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );        
#else
        m_pEnc->Ae_get_osd_parameter( _AON_CHN_NAME_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
#endif
        char* channelname = pSnapOsd.szStr;
        CAvUtf8String channel_name(channelname);
        av_ucs4_t char_unicode;
        av_char_lattice_info_t lattice_char_info;
        while(channel_name.Next_char(&char_unicode) == true)
        {
#if defined(_AV_PRODUCT_CLASS_IPC_)
            if(m_pAv_font_library->Afl_get_char_lattice(fontName, char_unicode, &lattice_char_info) == 0)
#else
            if(m_pAv_font_library->Afl_get_char_lattice(fontName, char_unicode, &lattice_char_info) == 0)
#endif
            {
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
            }
            else
            {
                //_AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                continue;//return -1;
            }
        }
        
        if(stuRgnAttr.unAttr.stOverlay.stSize.u32Width == 0)
        {
            //_AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        width = stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
        height = stuRgnAttr.unAttr.stOverlay.stSize.u32Height;
        //printf("GetRGNAttr index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
        //printf("GetRGNAttrbefore Adi_covert_coordinate pSnapOsd[%d].x = %d, pSnapOsd[%d].y = %d\n", index, pSnapOsd.x, index, pSnapOsd.y);
        pgAvDeviceInfo->Adi_covert_coordinate(eResolution, eTvSystem, &(pSnapOsd.x), &(pSnapOsd.y), 4);
        //printf("GetRGNAttr after Adi_covert_coordinate pSnapOsd[%d].x = %d, pSnapOsd[%d].y = %d\n", index, pSnapOsd.x, index, pSnapOsd.y);
        pgAvDeviceInfo->Adi_get_video_size(eResolution, eTvSystem, &video_width, &video_height);
        //printf("GetRGNAttr index = %d video_width = %d video_height = %d\n",index, video_width, video_height);

        if(pSnapOsd.x < 0)
        {
            pSnapOsd.x = 0;
        }
        if((pSnapOsd.x + (int)stuRgnAttr.unAttr.stOverlay.stSize.u32Width) > video_width )
        {
            pSnapOsd.x = video_width - stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
        }
        if(pSnapOsd.x < 0)
        {
            pSnapOsd.x = 0;
        }

        if(pSnapOsd.y < 0)
        {
            pSnapOsd.y = 0;
        }
        if(((pSnapOsd.y + (int)stuRgnAttr.unAttr.stOverlay.stSize.u32Height)>video_height))
        {
            pSnapOsd.y = video_height - stuRgnAttr.unAttr.stOverlay.stSize.u32Height;
        }
        if(pSnapOsd.y < 0)
        {
            pSnapOsd.y = 0;
        }
    }
    else if(index == 5) //selfnumber
    {
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
#if defined(_AV_PRODUCT_CLASS_IPC_)
        m_pEnc->Ae_get_osd_parameter( _AON_SELFNUMBER_INFO_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
        charactorNum = 32;
        switch(eResolution)
        {
            case _AR_1080_:
            case _AR_720_:
            case _AR_960P_:
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler) / 2 + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +_AE_OSD_ALIGNMENT_BYTE_ ) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;
            default:
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = ((fontWidth * charactorNum * horScaler)* 2 / 3  + fontWidth*horScaler) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler +_AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
                break;

        }

#else
        m_pEnc->Ae_get_osd_parameter( _AON_SELFNUMBER_INFO_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
        char tempstring[128] = {0};
        strcat(tempstring,pSnapOsd.szStr);
        CAvUtf8String bus_slefnumber(tempstring);
        av_ucs4_t char_unicode;
        av_char_lattice_info_t lattice_char_info;
        while(bus_slefnumber.Next_char(&char_unicode) == true)
        {
            if(m_pAv_font_library->Afl_get_char_lattice(fontName, char_unicode, &lattice_char_info) == 0)
            {
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
            }
            else
            {
                //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                continue;//return -1;
            }
        }
        if(stuRgnAttr.unAttr.stOverlay.stSize.u32Width == 0)
        {
            //DEBUG_ERROR( "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
#endif

        width = stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
        height = stuRgnAttr.unAttr.stOverlay.stSize.u32Height;
        //DEBUG_MESSAGE("GetRGNAttr index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
        //DEBUG_MESSAGE("GetRGNAttrbefore Adi_covert_coordinate pSnapOsd[%d].x = %d, pSnapOsd[%d].y = %d\n", index, pSnapOsd.x, index, pSnapOsd.y);
        pgAvDeviceInfo->Adi_covert_coordinate(eResolution, eTvSystem, &(pSnapOsd.x), &(pSnapOsd.y), 4);
        //DEBUG_MESSAGE("GetRGNAttr after Adi_covert_coordinate pSnapOsd[%d].x = %d, pSnapOsd[%d].y = %d\n", index, pSnapOsd.x, index, pSnapOsd.y);
        pgAvDeviceInfo->Adi_get_video_size(eResolution, eTvSystem, &video_width, &video_height);
        //DEBUG_MESSAGE("GetRGNAttr index = %d video_width = %d video_height = %d\n",index, video_width, video_height);

        if(pSnapOsd.x < 0)
        {
            pSnapOsd.x = 0;
        }
        if((pSnapOsd.x + (int)stuRgnAttr.unAttr.stOverlay.stSize.u32Width) > video_width )
        {
            pSnapOsd.x = video_width - stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
        }
        if(pSnapOsd.x < 0)
        {
            pSnapOsd.x = 0;
        }

        if(pSnapOsd.y < 0)
        {
            pSnapOsd.y = 0;
        }
        if(((pSnapOsd.y + (int)stuRgnAttr.unAttr.stOverlay.stSize.u32Height)>video_height))
        { 
            pSnapOsd.y = video_height - stuRgnAttr.unAttr.stOverlay.stSize.u32Height;
        }
        if(pSnapOsd.y < 0)
        {
            pSnapOsd.y = 0;
        }

    }
        else if(index == 6) //selfnumber
        {
            stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
#if defined(_AV_PRODUCT_CLASS_IPC_)
            m_pEnc->Ae_get_osd_parameter( _AON_COMMON_OSD1_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
#else
            m_pEnc->Ae_get_osd_parameter( _AON_COMMON_OSD1_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
#endif
            char *user_define_osd1;
            user_define_osd1 = pSnapOsd.szStr;
            CAvUtf8String osd1(user_define_osd1);
            av_ucs4_t char_unicode;
            av_char_lattice_info_t lattice_char_info;
            while(osd1.Next_char(&char_unicode) == true)
            {
#if defined(_AV_PRODUCT_CLASS_IPC_)   
                if(m_pAv_font_library->Afl_get_char_lattice(fontName, char_unicode, &lattice_char_info) == 0)
#else
                if(m_pAv_font_library->Afl_get_char_lattice(fontName, char_unicode, &lattice_char_info) == 0)
#endif
                {
                    stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
                }
                else
                {
                    //_AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                    continue;//return -1;
                }
            }
            if(stuRgnAttr.unAttr.stOverlay.stSize.u32Width == 0)
            {
            //_AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
                return -1;
            }

            stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
            stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;

            width = stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
            height = stuRgnAttr.unAttr.stOverlay.stSize.u32Height;

            //DEBUG_MESSAGE("GetRGNAttr index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
            //DEBUG_MESSAGE("GetRGNAttr before Adi_covert_coordinate pSnapOsd[%d].x = %d, pSnapOsd[%d].y = %d\n", index, pSnapOsd.x, index, pSnapOsd.y);
            pgAvDeviceInfo->Adi_covert_coordinate(eResolution, eTvSystem, &(pSnapOsd.x), &(pSnapOsd.y), 4);
            //DEBUG_MESSAGE("GetRGNAttr after Adi_covert_coordinate pSnapOsd[%d].x = %d, pSnapOsd[%d].y = %d\n", index, pSnapOsd.x, index, pSnapOsd.y);
            pgAvDeviceInfo->Adi_get_video_size(eResolution, eTvSystem, &video_width, &video_height);
            //DEBUG_MESSAGE("index = %d video_width = %d video_height = %d\n",index, video_width, video_height);

            if(pSnapOsd.x < 0)
            {
                pSnapOsd.x = 0;
            }
            if((pSnapOsd.x + (int)stuRgnAttr.unAttr.stOverlay.stSize.u32Width) > video_width )
            {
                pSnapOsd.x = video_width - stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
            }
            if(pSnapOsd.x < 0)
            {
                pSnapOsd.x = 0;
            }

            if(pSnapOsd.y < 0)
            {
                pSnapOsd.y = 0;
            }
            if(((pSnapOsd.y + (int)stuRgnAttr.unAttr.stOverlay.stSize.u32Height)>video_height))
            {
                pSnapOsd.y = video_height - stuRgnAttr.unAttr.stOverlay.stSize.u32Height;
            }
            if(pSnapOsd.y < 0)
            {
                pSnapOsd.y = 0;
            }
    }
    else if(index == 7) //selfnumber
    {
        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = 0;
#if defined(_AV_PRODUCT_CLASS_IPC_)
        m_pEnc->Ae_get_osd_parameter( _AON_COMMON_OSD2_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
#else
        m_pEnc->Ae_get_osd_parameter( _AON_COMMON_OSD2_, eResolution, &fontName, &fontWidth, &fontHeight, &horScaler, &verScaler );
#endif
        char *user_define_osd2;
        user_define_osd2 = pSnapOsd.szStr;
        CAvUtf8String osd2(user_define_osd2);
        av_ucs4_t char_unicode;
        av_char_lattice_info_t lattice_char_info;
        while(osd2.Next_char(&char_unicode) == true)
        {
#if defined(_AV_PRODUCT_CLASS_IPC_)
           if(m_pAv_font_library->Afl_get_char_lattice(fontName, char_unicode, &lattice_char_info) == 0) 
#else
            if(m_pAv_font_library->Afl_get_char_lattice(fontName, char_unicode, &lattice_char_info) == 0)
#endif
            {
                stuRgnAttr.unAttr.stOverlay.stSize.u32Width += lattice_char_info.width;
            }
            else
            {
                //_AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region m_pAv_font_library->Afl_get_char_lattice FAILED(%08lx)(%d)\n", char_unicode, NULL);
                continue;//return -1;
            }
        }
        if(stuRgnAttr.unAttr.stOverlay.stSize.u32Width == 0)
        {
        //_AV_KEY_INFO_(_AVD_ENC_, "CHiAvEncoder::Ae_create_osd_region (region_attr.unAttr.stOverlay.stSize.u32Width == 0)\n");
            return -1;
        }

        stuRgnAttr.unAttr.stOverlay.stSize.u32Width  = (stuRgnAttr.unAttr.stOverlay.stSize.u32Width * horScaler + _AE_OSD_ALIGNMENT_BYTE_-1) / _AE_OSD_ALIGNMENT_BYTE_* _AE_OSD_ALIGNMENT_BYTE_;
        stuRgnAttr.unAttr.stOverlay.stSize.u32Height = (fontHeight * verScaler + _AE_OSD_ALIGNMENT_BYTE_) / _AE_OSD_ALIGNMENT_BYTE_ * _AE_OSD_ALIGNMENT_BYTE_;
        
        width = stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
        height = stuRgnAttr.unAttr.stOverlay.stSize.u32Height;

        //printf("GetRGNAttr index = %d stSize.u32Width = %d, stSize.u32Height = %d\n", index, stuRgnAttr.unAttr.stOverlay.stSize.u32Width, stuRgnAttr.unAttr.stOverlay.stSize.u32Height);
        //printf("GetRGNAttr before Adi_covert_coordinate pSnapOsd[%d].x = %d, pSnapOsd[%d].y = %d\n", index, pSnapOsd.x, index, pSnapOsd.y);
        pgAvDeviceInfo->Adi_covert_coordinate(eResolution, eTvSystem, &(pSnapOsd.x), &(pSnapOsd.y), 4);
        //printf("GetRGNAttr after Adi_covert_coordinate pSnapOsd[%d].x = %d, pSnapOsd[%d].y = %d\n", index, pSnapOsd.x, index, pSnapOsd.y);
        pgAvDeviceInfo->Adi_get_video_size(eResolution, eTvSystem, &video_width, &video_height);
        //printf("index = %d video_width = %d video_height = %d\n",index, video_width, video_height);

        if(pSnapOsd.x < 0)
        {
            pSnapOsd.x = 0;
        }
        if((pSnapOsd.x + (int)stuRgnAttr.unAttr.stOverlay.stSize.u32Width) > video_width )
        {
            pSnapOsd.x = video_width - stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
        }
        if(pSnapOsd.x < 0)
        {
            pSnapOsd.x = 0;
        }

        if(pSnapOsd.y < 0)
        {
            pSnapOsd.y = 0;
        }
        if(((pSnapOsd.y + (int)stuRgnAttr.unAttr.stOverlay.stSize.u32Height)>video_height))
        { 
            pSnapOsd.y = video_height - stuRgnAttr.unAttr.stOverlay.stSize.u32Height;
        }
        if(pSnapOsd.y < 0)
        {
            pSnapOsd.y = 0;
        }
    }
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_)
    if(PTD_712_VB == pgAvDeviceInfo->Adi_product_type())
    {
        if((index== 0) || (index == 4))
        {
             int video_width = 0;
             int video_height = 0;
             pgAvDeviceInfo->Adi_get_video_size(eResolution, eTvSystem, &video_width, &video_height);
             pgAvDeviceInfo->Adi_covert_vidoe_size_by_rotate(&video_width, &video_height);    
            
             pSnapOsd.x= video_width - stuRgnAttr.unAttr.stOverlay.stSize.u32Width;
             pSnapOsd.y = video_height - stuRgnAttr.unAttr.stOverlay.stSize.u32Height;
        }
    }
#endif

    return 0;
}
int CAvSnapTask::CoordinateTrans(av_resolution_t eResolution, av_tvsystem_t eTvSystem, hi_snap_osd_t pSnapOsd[], int osd_num)
{
#if defined(_AV_PLATFORM_HI3520D_V300_)
	if(OSD_TEST_FLAG == pgAvDeviceInfo->Adi_product_type())	//6AII_AV12 snap osd is different for other product///
	{
		int video_width = -1;
		int video_height = -1;
		int width[8];
		int height[8];
		for(int index = 0 ; index < osd_num ; index++)
		{
			width[index] = -1;
			height[index] = -1;
			if((pSnapOsd[index].bShow == 1) && (0 != strlen(pSnapOsd[index].szStr)))
			{
				snap_osd_item_t osdTem;
				memset(&osdTem, 0x0, sizeof(snap_osd_item_t));
				this->GetRGNAttr(eResolution, eTvSystem, index, pSnapOsd[index], width[index], height[index], video_width, video_height);
				osdTem.display_x = pSnapOsd[index].x;
				osdTem.display_y = pSnapOsd[index].y;
				osdTem.bitmap_width = width[index];
				osdTem.bitmap_height = height[index];
				osdTem.osdname = (av_osd_name_t)index;//_AON_EXTEND_OSD;
				osdTem.visited = false;
				m_listOsdItem.push_back(osdTem);
			}
		}
		
		for(int index = 0 ; index < osd_num ; index++)
		{
			if((pSnapOsd[index].bShow == 1) && (0 != strlen(pSnapOsd[index].szStr)))
			{
				GetOverlayCoordinate((av_osd_name_t)index, pSnapOsd[index].x, pSnapOsd[index].y, width[index], height[index], video_width, video_height);
			}
		}
		return 0;
	}
#elif defined(_AV_PLATFORM_HI3515_)
	if(PTD_6A_I == pgAvDeviceInfo->Adi_product_type())
	{
		int video_width = -1;
		int video_height = -1;
		int width[8] = {0};
		int height[8] = {0};
		for(int index = 0 ; index < osd_num ; index++)
		{
			if((pSnapOsd[index].bShow == 1) && (0 < strlen(pSnapOsd[index].szStr)))
			{
				this->GetRGNAttr(eResolution, eTvSystem, index, pSnapOsd[index], width[index], height[index], video_width, video_height);
			}
		}
		return 0;
	}
#endif
        int width0 = -1;
        int height0 = -1;
        int width1 = -1;
        int height1 = -1;
        int width2 = -1;
        int height2 = -1;
        int width3 = -1;
        int height3 = -1;
        int width4 = -1;
        int height4 = -1;
        int width5 = -1;
        int height5 = -1;
#if defined(_AV_PRODUCT_CLASS_IPC_) 
        int width6 = -1;
        int height6 = -1;
        int width7 = -1;
        int height7 = -1;
#endif
        int video_width = -1;
        int video_height = -1;
        //step 1 put enabled osd into list
        if( (pSnapOsd[0].bShow== 1) && (0 != strlen(pSnapOsd[0].szStr)) )//date
        {
            int index = 0;
            snap_osd_item_t osdTem;
            memset(&osdTem, 0, sizeof(snap_osd_item_t));
            this->GetRGNAttr(eResolution, eTvSystem, index, pSnapOsd[index], width0, height0, video_width, video_height);
            osdTem.display_x = pSnapOsd[index].x;
            osdTem.display_y = pSnapOsd[index].y;
            osdTem.bitmap_width = width0;
            osdTem.bitmap_height = height0;
            osdTem.osdname = _AON_DATE_TIME_;
            osdTem.visited = false;
            m_listOsdItem.push_back(osdTem);
        }
        if((pSnapOsd[1].bShow== 1) && (0 != strlen(pSnapOsd[1].szStr)))//bus_number
        {
            int index = 1;
            snap_osd_item_t osdTem;
            memset(&osdTem, 0, sizeof(snap_osd_item_t));
            this->GetRGNAttr(eResolution, eTvSystem, index, pSnapOsd[index], width1, height1, video_width, video_height);
            osdTem.display_x = pSnapOsd[index].x;
            osdTem.display_y = pSnapOsd[index].y;
            osdTem.bitmap_width = width1;
            osdTem.bitmap_height = height1;
            osdTem.osdname = _AON_BUS_NUMBER_;
            osdTem.visited = false;
            m_listOsdItem.push_back(osdTem);

        }
        if((pSnapOsd[2].bShow== 1) && (0 != strlen(pSnapOsd[2].szStr)))//speed
        {
            int index = 2;
            snap_osd_item_t osdTem;
            memset(&osdTem, 0, sizeof(snap_osd_item_t));
            this->GetRGNAttr(eResolution, eTvSystem, index, pSnapOsd[index], width2, height2, video_width, video_height);
            osdTem.display_x = pSnapOsd[index].x;
            osdTem.display_y = pSnapOsd[index].y;
            osdTem.bitmap_width = width2;
            osdTem.bitmap_height = height2;
            osdTem.osdname = _AON_SPEED_INFO_;
            osdTem.visited = false;
            m_listOsdItem.push_back(osdTem);

        }
        if((pSnapOsd[3].bShow== 1) && (0 != strlen(pSnapOsd[3].szStr)))//gps
        {
            int index = 3;
            snap_osd_item_t osdTem;
            memset(&osdTem, 0, sizeof(snap_osd_item_t));
            this->GetRGNAttr(eResolution, eTvSystem, index, pSnapOsd[index], width3, height3, video_width, video_height);
            osdTem.display_x = pSnapOsd[index].x;
            osdTem.display_y = pSnapOsd[index].y;
            osdTem.bitmap_width = width3;
            osdTem.bitmap_height = height3;
            osdTem.osdname = _AON_GPS_INFO_;
            osdTem.visited = false;
            m_listOsdItem.push_back(osdTem);
        }
        
        if((pSnapOsd[4].bShow== 1) && (0 != strlen(pSnapOsd[4].szStr)))//chn_name
        {
            int index = 4;

            snap_osd_item_t osdTem;
            memset(&osdTem, 0, sizeof(snap_osd_item_t));
            this->GetRGNAttr(eResolution, eTvSystem, index, pSnapOsd[index], width4, height4, video_width, video_height);
            osdTem.display_x = pSnapOsd[index].x;
            osdTem.display_y = pSnapOsd[index].y;
            osdTem.bitmap_width = width4;
            osdTem.bitmap_height = height4;
            osdTem.osdname = _AON_CHN_NAME_;
            osdTem.visited = false;
            m_listOsdItem.push_back(osdTem);

        }
        
        if((pSnapOsd[5].bShow== 1) && (0 != strlen(pSnapOsd[5].szStr))) //selfnumber
        {
            int index = 5;
            snap_osd_item_t osdTem;
            memset(&osdTem, 0, sizeof(snap_osd_item_t));
            this->GetRGNAttr(eResolution, eTvSystem, index, pSnapOsd[index], width5, height5, video_width, video_height);
            osdTem.display_x = pSnapOsd[index].x;
            osdTem.display_y = pSnapOsd[index].y;
            osdTem.bitmap_width = width5;
            osdTem.bitmap_height = height5;
            osdTem.osdname = _AON_SELFNUMBER_INFO_;
            osdTem.visited = false;
            m_listOsdItem.push_back(osdTem);

        }

#if defined(_AV_PRODUCT_CLASS_IPC_)        
        if((pSnapOsd[6].bShow==1) && (0 != strlen(pSnapOsd[6].szStr))) 
        {
            int index = 6;
            snap_osd_item_t osdTem;
            memset(&osdTem, 0, sizeof(snap_osd_item_t));
            this->GetRGNAttr(eResolution, eTvSystem, index, pSnapOsd[index], width6, height6, video_width, video_height);
            osdTem.display_x = pSnapOsd[index].x;
            osdTem.display_y = pSnapOsd[index].y;
            osdTem.bitmap_width = width6;
            osdTem.bitmap_height = height6;
            osdTem.osdname = _AON_COMMON_OSD1_;
            osdTem.visited = false;
            m_listOsdItem.push_back(osdTem);
        }
         
        if((pSnapOsd[7].bShow== 1) && (0 != strlen(pSnapOsd[7].szStr))) 
        {
            int index = 7;
            snap_osd_item_t osdTem;
            memset(&osdTem, 0, sizeof(snap_osd_item_t));
            this->GetRGNAttr(eResolution, eTvSystem, index, pSnapOsd[index], width7, height7, video_width, video_height);
            osdTem.display_x = pSnapOsd[index].x;
            osdTem.display_y = pSnapOsd[index].y;
            osdTem.bitmap_width = width7;
            osdTem.bitmap_height = height7;
            osdTem.osdname = _AON_COMMON_OSD2_;
            osdTem.visited = false;
            m_listOsdItem.push_back(osdTem);
        }
#endif        
        //step 2 deal with situation of the overlapping osds 
        if( (pSnapOsd[0].bShow== 1) && (0 != strlen(pSnapOsd[0].szStr)))//date
        {
            int index = 0;
            GetOverlayCoordinate(_AON_DATE_TIME_, pSnapOsd[index].x, pSnapOsd[index].y, width0, height0, video_width, video_height);
        }
        if((pSnapOsd[1].bShow== 1) && (0 != strlen(pSnapOsd[1].szStr)))//bus_number
        {
            int index = 1;
            GetOverlayCoordinate(_AON_BUS_NUMBER_, pSnapOsd[index].x, pSnapOsd[index].y, width1, height1, video_width, video_height);
        }
        if((pSnapOsd[2].bShow== 1) && (0 != strlen(pSnapOsd[2].szStr)))//speed
        {
            int index = 2;
            GetOverlayCoordinate(_AON_SPEED_INFO_, pSnapOsd[index].x, pSnapOsd[index].y, width2, height2, video_width, video_height);
        }
        if((pSnapOsd[3].bShow== 1)&& (0 != strlen(pSnapOsd[3].szStr)))//gps
        {
            int index = 3;
            GetOverlayCoordinate(_AON_GPS_INFO_, pSnapOsd[index].x, pSnapOsd[index].y, width3, height3, video_width, video_height);
        }

        if((pSnapOsd[4].bShow== 1)&& (0 != strlen(pSnapOsd[4].szStr)))//chn_name
        {
            int index = 4;
            GetOverlayCoordinate(_AON_CHN_NAME_, pSnapOsd[index].x, pSnapOsd[index].y, width4, height4, video_width, video_height);
        }

        if((pSnapOsd[5].bShow== 1)&& (0 != strlen(pSnapOsd[5].szStr))) //selfnumber
        {
            int index = 5;
            GetOverlayCoordinate(_AON_SELFNUMBER_INFO_, pSnapOsd[index].x, pSnapOsd[index].y, width5, height5, video_width, video_height);
        }
#if defined(_AV_PRODUCT_CLASS_IPC_)
        if((pSnapOsd[6].bShow== 1)&& (0 != strlen(pSnapOsd[6].szStr))) //common osd1
        {
            int index = 6;
            GetOverlayCoordinate(_AON_COMMON_OSD1_, pSnapOsd[index].x, pSnapOsd[index].y, width6, height6, video_width, video_height);
        }

        if((pSnapOsd[7].bShow== 1) && (0 != strlen(pSnapOsd[7].szStr))) //common osd2
        {
            int index = 7;
            GetOverlayCoordinate(_AON_COMMON_OSD2_, pSnapOsd[index].x, pSnapOsd[index].y, width7, height7, video_width, video_height);
        }
#endif     
        return 0;  
    }


int CAvSnapTask::GetOverlayCoordinate(av_osd_name_t osdname, int &x, int &y, unsigned int u32Width, unsigned int u32Height,int video_width, int video_height)
{
	snap_osd_item_t temp_regin;
    memset(&temp_regin, 0, sizeof(snap_osd_item_t));
    temp_regin.display_x = x;
    temp_regin.display_y = y;
    temp_regin.bitmap_width = u32Width;
    temp_regin.bitmap_height = u32Height;
    temp_regin.osdname = osdname;
    /*  ��ʼ�Ƚ�ʱ��Ҫ��ȡm_listOsdItem��visited��״̬ */
    std::list<snap_osd_item_t>::iterator iter;
    iter = SearchIteratorByOsdName(osdname);
    temp_regin.visited = iter->visited;

    std::vector<snap_osd_item_t> passed_path;
    std::queue<snap_osd_item_t> possible_path;
    possible_path.push(temp_regin);
    //DEBUG_MESSAGE("####GetOverlayCoordinate temp_regin001 name= %d x= %d y= %d width =%d height =%d\n",temp_regin.osdname, temp_regin.display_x, temp_regin.display_y,temp_regin.bitmap_width,temp_regin.bitmap_height);
    SearchOverlayCoordinate(temp_regin, passed_path, possible_path, video_width, video_height);
    //DEBUG_MESSAGE("####GetOverlayCoordinate temp_regin002 name= %d x= %d y= %d width =%d height =%d\n",temp_regin.osdname, temp_regin.display_x, temp_regin.display_y, temp_regin.bitmap_width,temp_regin.bitmap_height);
    x = temp_regin.display_x ;
    y = temp_regin.display_y;
    return 0;
}

int CAvSnapTask::SearchOverlayCoordinate(snap_osd_item_t &osd_item, std::vector<snap_osd_item_t> &passed_path, std::queue<snap_osd_item_t> &possible_path, int video_width, int video_height)
{

    //DEBUG_MESSAGE("######SearchOverlayCoordinate\n");
    if(possible_path.empty())
    {
        DEBUG_ERROR( "the possible path is null\n");
        return -1;
    }
    
    //printf("[ddh] the overlay osd is:[x:%d y:%d] \n", osd_item.display_x, osd_item.display_y);
    while(!possible_path.empty())
     {
         
         snap_osd_item_t regin = possible_path.front();
         
         std::list<snap_osd_item_t>::iterator osd_item_it;
         osd_item_it = m_listOsdItem.begin();
         //PrintfList( "out while",m_listOsdItem);
         while(osd_item_it != m_listOsdItem.end())
         {
            if((regin.osdname != osd_item_it->osdname) && (true == osd_item_it->visited))
            {
                if(!((regin.display_x >= osd_item_it->display_x + osd_item_it->bitmap_width) ||\
                        (osd_item_it->display_x >= regin.display_x + regin.bitmap_width) ||\
                            (regin.display_y >= osd_item_it->display_y + osd_item_it->bitmap_height) || \
                                (osd_item_it->display_y >= regin.display_y + regin.bitmap_height)))
                {
                    if(!RgnIsPassed(*osd_item_it, passed_path))
                    {

                        if(osd_item_it->display_y + osd_item_it->bitmap_height + regin.bitmap_height <= video_height)
                        {
                            regin.display_y = osd_item_it->display_y + osd_item_it->bitmap_height;
                            regin.display_x = possible_path.front().display_x;
                            regin.bitmap_width = possible_path.front().bitmap_width;
                            regin.bitmap_height = possible_path.front().bitmap_height;
                            regin.osdname = possible_path.front().osdname;
                            if(SnapIsReginOverlapWithExistRegins(regin, NULL))
                            {
                                possible_path.push(regin);
                            }
                            else
                            {
                                std::list<snap_osd_item_t>::iterator osditer;
                                osditer = SearchIteratorByOsdName(osd_item.osdname);
                                osditer->display_y = regin.display_y;
                                osditer->visited = true;
                                osd_item.display_x = regin.display_x;
                                osd_item.display_y = regin.display_y;
                        
                                return 0;
                            }                                  
                        }
                        
                        if(osd_item_it->display_y - regin.bitmap_height >= 0)
                        {
                            regin.display_y = osd_item_it->display_y - regin.bitmap_height;
                            regin.display_x = possible_path.front().display_x;
                            regin.bitmap_width = possible_path.front().bitmap_width;
                            regin.bitmap_height = possible_path.front().bitmap_height;
                            regin.osdname = possible_path.front().osdname;
                            if(SnapIsReginOverlapWithExistRegins(regin, NULL))
                            {
                                possible_path.push(regin);
                            }
                            else
                            {
                                std::list<snap_osd_item_t>::iterator osditer;
                                osditer = SearchIteratorByOsdName(osd_item.osdname);
                                osditer->display_y = regin.display_y;
                                osditer->visited = true;
                                osd_item.display_x = regin.display_x;
                                osd_item.display_y = regin.display_y;
                        
                                return 0;
                            }                                 
                        }

                        if(osd_item_it->display_x + osd_item_it->bitmap_width + regin.bitmap_width <= video_width)
                        {
                            regin.display_x = osd_item_it->display_x + osd_item_it->bitmap_width;
                            regin.display_y = possible_path.front().display_y;
                            regin.bitmap_width = possible_path.front().bitmap_width;
                            regin.bitmap_height = possible_path.front().bitmap_height;
                            regin.osdname = possible_path.front().osdname;

                        if(SnapIsReginOverlapWithExistRegins(regin, NULL))
                        {
                            possible_path.push(regin);
                        }
                        else
                        {
                            std::list<snap_osd_item_t>::iterator osditer;
                            osditer = SearchIteratorByOsdName(osd_item.osdname);
                            osditer->display_x = regin.display_x;
                            osditer->visited = true;

                            osd_item.display_x = regin.display_x;
                            osd_item.display_y = regin.display_y;

                            return 0;
                        }
                    }

                    if(osd_item_it->display_x - regin.bitmap_width >= 0)
                    {
                        regin.display_x  = osd_item_it->display_x - regin.bitmap_width;
                        regin.display_y = possible_path.front().display_y;
                        regin.bitmap_width = possible_path.front().bitmap_width;
                        regin.bitmap_height = possible_path.front().bitmap_height;
                        regin.osdname = possible_path.front().osdname;
                        if(SnapIsReginOverlapWithExistRegins(regin, NULL))
                        {
                            possible_path.push(regin);
                        }
                        else
                        {
                            std::list<snap_osd_item_t>::iterator osditer;
                            osditer = SearchIteratorByOsdName(osd_item.osdname);
                            osditer->display_x = regin.display_x;
                            osditer->visited = true;
                            osd_item.display_x = regin.display_x;
                            osd_item.display_y = regin.display_y;
                    
                            return 0;
                        } 
                    }
                    
                    passed_path.push_back(*osd_item_it);
                    }
                }
            }
            ++osd_item_it;
          }
          possible_path.pop();
     }
    
    std::list<snap_osd_item_t>::iterator osditer;
    osditer = SearchIteratorByOsdName(osd_item.osdname);
    osditer->visited = true;

    return -1;
}
bool CAvSnapTask::IsAllNotVisited()
{
    std::list<snap_osd_item_t>::iterator osd_item_it;
    osd_item_it = m_listOsdItem.begin();
    while(osd_item_it != m_listOsdItem.end())
    {
        if(osd_item_it->visited == false)
        {
            ++osd_item_it;
        }
        else
        {
            return false;
        }
    }
    return true;
}
int CAvSnapTask::VisitedNum()
{
    std::list<snap_osd_item_t>::iterator osd_item_it;
    osd_item_it = m_listOsdItem.begin();
    int Num = 0;
    while(osd_item_it != m_listOsdItem.end())
    {
        if(osd_item_it->visited == true)
        {
            ++Num;
        }
        ++osd_item_it;
    }
    return Num;
}
std::list<snap_osd_item_t>::iterator CAvSnapTask::SearchIteratorByOsdName(av_osd_name_t osdName)
{
    std::list<snap_osd_item_t>::iterator osd_item_it;
    osd_item_it = m_listOsdItem.begin();
    while(osd_item_it != m_listOsdItem.end())
    {
        if(osd_item_it->osdname == osdName)
        {
            return osd_item_it;
        }
        else
        {
            ++osd_item_it ;
        }
    }
    return osd_item_it;
}

int CAvSnapTask::PrintfList(const char * flag, std::list<snap_osd_item_t> listsnap)
{
    std::list<snap_osd_item_t>::iterator osd_item_it;
    osd_item_it = listsnap.begin();
    while(osd_item_it != listsnap.end())
    {
        DEBUG_MESSAGE("####print list flag = %s name = %d visited =%d x = %d y = %d width = %d height = %d\n ",flag, osd_item_it->osdname, osd_item_it->visited, osd_item_it->display_x, osd_item_it->display_y, osd_item_it->bitmap_width, osd_item_it->bitmap_height);
        ++osd_item_it;
    }
    return 0;
}
int CAvSnapTask::SetDigitAnalSwitchMap(char chnmap[_AV_VIDEO_MAX_CHN_])
{
    //DEBUG_MESSAGE( "#######CHiAvDeviceMaster::Ad_UpdateDevMap\n");
    memcpy(m_ShareMemLockState, chnmap, sizeof(char)*_AV_VIDEO_MAX_CHN_);

    return 0;
}
int CAvSnapTask::GetShareMenLockState(int chn)
{
    return m_ShareMemLockState[chn];
}
int CAvSnapTask::SetShareMenLockState(int chn, char ana_or_dig)
{
    m_ShareMemLockState[chn] = ana_or_dig;
    return 0;
}

int CAvSnapTask::SetShareMenLock(int chn, char lockFlag)
{
    if(lockFlag == 0)/* lock share menory */
    {
        N9M_SHLockWriter(m_SnapShareMemHandle[chn]);
        N9M_SHClearAllFrames(m_SnapShareMemHandle[chn]);
        m_ShareMemLockState[chn] = lockFlag;
        
    }
    else /* unlock share menory */
    {
        N9M_SHUnlockWriter(m_SnapShareMemHandle[chn]);
        N9M_SHClearAllFrames(m_SnapShareMemHandle[chn]);
        m_ShareMemLockState[chn] = lockFlag;
    }
    return 0;
}
int CAvSnapTask::UpdateSnapParam( int phy_video_chn_num, const av_video_encoder_para_t *pVideo_para )
{
    if( phy_video_chn_num < 0 || !pVideo_para )
    {
        DEBUG_MESSAGE("CAvSnapTask::UpdateSnapParam param error chn=%d param=%p\n", phy_video_chn_num, pVideo_para);
        return -1;
    }

#if defined(_AV_PRODUCT_CLASS_IPC_)
    /*add by dhdong 2014-10-18*/
    if( phy_video_chn_num < m_u8MaxChlOnThisChip )
    {
        /*date time*/
        m_pstuOsdInfo[0][phy_video_chn_num].x = pVideo_para->date_time_osd_x;
        m_pstuOsdInfo[0][phy_video_chn_num].y = pVideo_para->date_time_osd_y;
        m_pstuOsdInfo[0][phy_video_chn_num].szStr[0] = '\0';      

        /*vehicle*/
        m_pstuOsdInfo[1][phy_video_chn_num].x = pVideo_para->bus_number_osd_x;
        m_pstuOsdInfo[1][phy_video_chn_num].y = pVideo_para->bus_number_osd_y;
        m_pstuOsdInfo[1][phy_video_chn_num].szStr[0] = '\0';   
        
        /*speed*/
        m_pstuOsdInfo[2][phy_video_chn_num].x = pVideo_para->speed_osd_x;
        m_pstuOsdInfo[2][phy_video_chn_num].y = pVideo_para->speed_osd_y;
        m_pstuOsdInfo[2][phy_video_chn_num].szStr[0] = '\0';   

        /*GPS*/
        m_pstuOsdInfo[3][phy_video_chn_num].x = pVideo_para->gps_osd_x;
        m_pstuOsdInfo[3][phy_video_chn_num].y = pVideo_para->gps_osd_y;
        m_pstuOsdInfo[3][phy_video_chn_num].szStr[0] = '\0';  

        /*channel name*/
        m_pstuOsdInfo[4][phy_video_chn_num].x = pVideo_para->channel_name_osd_x;
        m_pstuOsdInfo[4][phy_video_chn_num].y = pVideo_para->channel_name_osd_y;

        if(0 != memcmp(m_pstuOsdInfo[4][phy_video_chn_num].szStr, pVideo_para->channel_name, sizeof(m_pstuOsdInfo[4][phy_video_chn_num].szStr)))
        {
            strncpy(m_pstuOsdInfo[4][phy_video_chn_num].szStr, pVideo_para->channel_name, sizeof(m_pstuOsdInfo[4][phy_video_chn_num].szStr));
            m_pstuOsdInfo[4][phy_video_chn_num].szStr[sizeof(m_pstuOsdInfo[4][phy_video_chn_num].szStr) -1 ] = '\0'; 
            m_bCommSnapParamChange[phy_video_chn_num]= true;
        }
 
        /*device id*/
        m_pstuOsdInfo[5][phy_video_chn_num].x = pVideo_para->bus_selfnumber_osd_x;
        m_pstuOsdInfo[5][phy_video_chn_num].y = pVideo_para->bus_selfnumber_osd_y;
        m_pstuOsdInfo[5][phy_video_chn_num].szStr[0] = '\0';    

        if(m_eMainStreamRes[phy_video_chn_num] != pVideo_para->resolution)
        {
            m_eMainStreamRes[phy_video_chn_num] = pVideo_para->resolution;
            m_bCommSnapParamChange[phy_video_chn_num]= true;
        }
           
    }

    m_eTvSystem = pVideo_para->tv_system;
    m_u8DateMode = pVideo_para->date_mode;
    m_u8TimeMode = pVideo_para->time_mode;

    m_eTaskState = SNAP_THREAD_CHANGED_ENCODER;
#else
    if( phy_video_chn_num < m_u8MaxChlOnThisChip )
    {
#if (defined(_AV_PLATFORM_HI3520D_V300_) || (defined(_AV_PLATFORM_HI3515_)))
		if(OSD_TEST_FLAG == pgAvDeviceInfo->Adi_product_type() || PTD_6A_I == pgAvDeviceInfo->Adi_product_type())	//6AII_AV12����ץ��osd�������⴦��///
		{
			for(int index = 0 ; index < 8 ; index++)
			{
				memset(&(m_pstuOsdInfo[index][phy_video_chn_num]), 0x0, sizeof(hi_snap_osd_t));
				m_pstuOsdInfo[index][phy_video_chn_num].bShow = pVideo_para->stuExtendOsdInfo[index].ucEnable;
				m_pstuOsdInfo[index][phy_video_chn_num].x = pVideo_para->stuExtendOsdInfo[index].usOsdX;
				m_pstuOsdInfo[index][phy_video_chn_num].y = pVideo_para->stuExtendOsdInfo[index].usOsdY;
				m_pstuOsdInfo[index][phy_video_chn_num].type = pVideo_para->stuExtendOsdInfo[index].ucOsdType;
				m_pstuOsdInfo[index][phy_video_chn_num].color = pVideo_para->stuExtendOsdInfo[index].ucColor;
				m_pstuOsdInfo[index][phy_video_chn_num].align = pVideo_para->stuExtendOsdInfo[index].ucAlign;
				memcpy(m_pstuOsdInfo[index][phy_video_chn_num].szStr, pVideo_para->stuExtendOsdInfo[index].szContent, sizeof(m_pstuOsdInfo[index][phy_video_chn_num].szStr) - 1);
			}
		}
		else
#endif
		{
	        m_pstuOsdInfo[0][phy_video_chn_num].x = pVideo_para->date_time_osd_x;
	        m_pstuOsdInfo[0][phy_video_chn_num].y = pVideo_para->date_time_osd_y;
	        m_pstuOsdInfo[0][phy_video_chn_num].bShow = pVideo_para->have_date_time_osd;
	        m_pstuOsdInfo[0][phy_video_chn_num].szStr[0] = '\0';

	        m_pstuOsdInfo[1][phy_video_chn_num].x = pVideo_para->bus_number_osd_x;
	        m_pstuOsdInfo[1][phy_video_chn_num].y = pVideo_para->bus_number_osd_y;
	        m_pstuOsdInfo[1][phy_video_chn_num].bShow = pVideo_para->have_bus_number_osd;
	        strcpy(m_pstuOsdInfo[1][phy_video_chn_num].szStr, pVideo_para->bus_number);
	        DEBUG_MESSAGE("######UpdateSnapParam busnumber chn= %d x=%d y= %d\n", phy_video_chn_num, m_pstuOsdInfo[1][phy_video_chn_num].x, m_pstuOsdInfo[1][phy_video_chn_num].y);
	        m_pstuOsdInfo[2][phy_video_chn_num].x = pVideo_para->speed_osd_x;
	        m_pstuOsdInfo[2][phy_video_chn_num].y = pVideo_para->speed_osd_y;
	        m_pstuOsdInfo[2][phy_video_chn_num].bShow = pVideo_para->have_speed_osd;

	        m_pstuOsdInfo[2][phy_video_chn_num].szStr[0] = '\0';

	        m_pstuOsdInfo[3][phy_video_chn_num].x = pVideo_para->gps_osd_x;
	        m_pstuOsdInfo[3][phy_video_chn_num].y = pVideo_para->gps_osd_y;
	        m_pstuOsdInfo[3][phy_video_chn_num].bShow = pVideo_para->have_gps_osd;

	        m_pstuOsdInfo[3][phy_video_chn_num].szStr[0] = '\0';

	        m_pstuOsdInfo[4][phy_video_chn_num].x = pVideo_para->channel_name_osd_x;
	        m_pstuOsdInfo[4][phy_video_chn_num].y = pVideo_para->channel_name_osd_y;
	        m_pstuOsdInfo[4][phy_video_chn_num].bShow = pVideo_para->have_channel_name_osd;
	        strcpy(m_pstuOsdInfo[4][phy_video_chn_num].szStr, pVideo_para->channel_name );

	        m_pstuOsdInfo[5][phy_video_chn_num].x = pVideo_para->bus_selfnumber_osd_x;
	        m_pstuOsdInfo[5][phy_video_chn_num].y = pVideo_para->bus_selfnumber_osd_y;
	        m_pstuOsdInfo[5][phy_video_chn_num].bShow = pVideo_para->have_bus_selfnumber_osd;
	        strcpy(m_pstuOsdInfo[5][phy_video_chn_num].szStr, pVideo_para->bus_selfnumber);
		}

        m_eMainStreamRes[phy_video_chn_num] = pVideo_para->resolution;
        m_eTvSystem = pVideo_para->tv_system;
        m_u8DateMode = pVideo_para->date_mode;
        m_u8TimeMode = pVideo_para->time_mode;
    
        m_eTaskState = SNAP_THREAD_CHANGED_ENCODER;

        DEBUG_MESSAGE("CAvSnapTask::UpdateSnapParam ch=%d(%d) tvSys=%d res=%d datemode=%d timemod=%d maxChOnThischip=%d\n"
            , phy_video_chn_num, phy_video_chn_num, m_eTvSystem, m_eMainStreamRes[phy_video_chn_num], m_u8DateMode, m_u8TimeMode, m_u8MaxChlOnThisChip);
    }
#endif
    return 0;
}

int CAvSnapTask::AddSignalSnapCmd( AvSnapPara_t pstuSignalSnapCmd )
{    
    m_pThread_lock->lock();
#if 0	
    if(listSignalSnapParam.size() >= 2)
    {
    	listSignalSnapParam.clear();
    }
    listSignalSnapParam.push_back(pstuSignalSnapCmd);
#else
	if (listSignalSnapParam.size() > 5)
		listSignalSnapParam.pop_front(); /*delete list head*/
	listSignalSnapParam.push_back(pstuSignalSnapCmd);
#endif	
    m_pThread_lock->unlock();
    return 0;
}

bool CAvSnapTask::GetSignalSnapTask(AvSnapPara_t *snapparam)
{
	m_pThread_lock->lock();
    if(!listSignalSnapParam.empty())
    {
        *snapparam = listSignalSnapParam.front();
        listSignalSnapParam.pop_front();
        m_pThread_lock->unlock();

        return true;
    }
	m_pThread_lock->unlock();
    return false;
    
}
int CAvSnapTask::GetSnapResult(msgSnapResultBoardcast *snapresultitem)
{
	m_pThread_lock->lock();

    std::list<msgSnapResultBoardcast> ::iterator snapresult_it;

    if(!listSnapResult.empty() && snapresultitem)
    {
        snapresult_it = listSnapResult.begin();
        snapresultitem->uiPictureSerialnumber = snapresult_it->uiPictureSerialnumber;
        snapresultitem->uiSnapTaskNumber = snapresult_it->uiSnapTaskNumber;
        snapresultitem->usSnapResult = snapresult_it->usSnapResult;
        snapresultitem->usStorageState = snapresult_it->usStorageState;
        snapresultitem->uiSnapTotalNumber = snapresult_it->uiSnapTotalNumber;
        snapresultitem->uiSanpCurNumber = snapresult_it->uiSanpCurNumber;
        snapresultitem->uiUser = snapresult_it->uiUser;

        listSnapResult.erase(snapresult_it);
        
        m_pThread_lock->unlock();
        return 0;
    }
	m_pThread_lock->unlock();

    return -1;
}

int CAvSnapTask::SetOsdInfo(av_video_encoder_para_t *pVideo_para, int chn, rm_uint16_t usOverlayBitmap)
{
#if (defined(_AV_PLATFORM_HI3520D_V300_) ||(defined(_AV_PLATFORM_HI3515_)))
	if(OSD_TEST_FLAG == pgAvDeviceInfo->Adi_product_type() || PTD_6A_I == pgAvDeviceInfo->Adi_product_type())	//6AII_AV12����ץ��osd�������⴦��///
	{
		if(NULL != pVideo_para)
		{
			for(int index = 0 ; index < 8 ; index++)
			{
				if(NULL != m_pstuOsdInfo[index])
				{
					memset(&(m_pstuOsdInfo[index][chn]), 0x0, sizeof(hi_snap_osd_t));
					m_pstuOsdInfo[index][chn].bShow = pVideo_para->stuExtendOsdInfo[index].ucEnable;
					m_pstuOsdInfo[index][chn].x = pVideo_para->stuExtendOsdInfo[index].usOsdX;
					m_pstuOsdInfo[index][chn].y = pVideo_para->stuExtendOsdInfo[index].usOsdY;
					m_pstuOsdInfo[index][chn].type = pVideo_para->stuExtendOsdInfo[index].ucOsdType;
					m_pstuOsdInfo[index][chn].color = pVideo_para->stuExtendOsdInfo[index].ucColor;
					m_pstuOsdInfo[index][chn].align = pVideo_para->stuExtendOsdInfo[index].ucAlign;
					memcpy(m_pstuOsdInfo[index][chn].szStr, pVideo_para->stuExtendOsdInfo[index].szContent, sizeof(m_pstuOsdInfo[index][chn].szStr) - 1);
					DEBUG_MESSAGE("SetOsdInfo, chn = %d, index = %d, show = %d, str = %s;\n", chn, index, m_pstuOsdInfo[index][chn].bShow, m_pstuOsdInfo[index][chn].szStr);
				}
				else
				{
					DEBUG_ERROR("SetOsdInfo, osd ptr is NULL, chn = %d, index = %d\n", chn, index);
					return -1;
				}
			}
		}
		else
		{
			return -1;
		}
		return 0;
	}
#endif
    //DEBUG_MESSAGE( "CAvSnapTask::SetOsdInfo \n");
    for(int i =0; i<8; i++)
    {
        m_pstuOsdInfo[i][chn].bShow = 0;
    }
    if(pVideo_para != NULL)
    {
        /* get main stream resolution */
        m_eMainStreamRes[chn] = pVideo_para->resolution;
        m_eTvSystem = pVideo_para->tv_system;
        if(GCL_BIT_VAL_TEST(usOverlayBitmap, 0))
        {
            //DEBUG_MESSAGE("############# date enable\n");
            m_pstuOsdInfo[0][chn].bShow = 1;
            m_pstuOsdInfo[0][chn].x = pVideo_para->date_time_osd_x;
            m_pstuOsdInfo[0][chn].y = pVideo_para->date_time_osd_y;
        }
        if(GCL_BIT_VAL_TEST(usOverlayBitmap, 1))
        {
            //DEBUG_MESSAGE("############# bus_number enable\n");
            m_pstuOsdInfo[1][chn].bShow = 1;
            m_pstuOsdInfo[1][chn].x = pVideo_para->bus_number_osd_x;
            m_pstuOsdInfo[1][chn].y = pVideo_para->bus_number_osd_y;
        }
        if(GCL_BIT_VAL_TEST(usOverlayBitmap, 2))
        {
            //DEBUG_MESSAGE("############# speed enable\n");
            m_pstuOsdInfo[2][chn].bShow = 1;
            m_pstuOsdInfo[2][chn].x = pVideo_para->speed_osd_x;
            m_pstuOsdInfo[2][chn].y = pVideo_para->speed_osd_y;
        }

        if(GCL_BIT_VAL_TEST(usOverlayBitmap, 3))
        {
            //DEBUG_MESSAGE("############# gps enable\n");
            m_pstuOsdInfo[3][chn].bShow = 1;
            m_pstuOsdInfo[3][chn].x = pVideo_para->gps_osd_x;
            m_pstuOsdInfo[3][chn].y = pVideo_para->gps_osd_y;
        }

        if(GCL_BIT_VAL_TEST(usOverlayBitmap, 4))
        {
            //DEBUG_MESSAGE("############# channel_name enable\n");
            m_pstuOsdInfo[4][chn].bShow = 1;
                m_pstuOsdInfo[4][chn].x = pVideo_para->channel_name_osd_x;
                m_pstuOsdInfo[4][chn].y = pVideo_para->channel_name_osd_y;
                strcpy(m_pstuOsdInfo[4][chn].szStr, pVideo_para->channel_name);
        }

        if(GCL_BIT_VAL_TEST(usOverlayBitmap, 5))
        {
            //DEBUG_MESSAGE("############# bus_selfnumber enable\n");
            m_pstuOsdInfo[5][chn].bShow = 1;
            m_pstuOsdInfo[5][chn].x = pVideo_para->bus_selfnumber_osd_x;
            m_pstuOsdInfo[5][chn].y = pVideo_para->bus_selfnumber_osd_y;
            strcpy(m_pstuOsdInfo[5][chn].szStr, pVideo_para->bus_selfnumber);
        }
        /*
        if(pVideo_para->have_common1_osd==1)
        {
            m_pstuOsdInfo[6][chn].bShow = 1;
            m_pstuOsdInfo[6][chn].x = pVideo_para->gps_osd_x;
            m_pstuOsdInfo[6][chn].y = pVideo_para->gps_osd_y;
            //DEBUG_MESSAGE("####CAvSnapTask::SetOsdInfo m_pstuOsdInfo[3][%d].x %d m_pstuOsdInfo[0][%d].y %d\n", chn, m_pstuOsdInfo[3][chn].x, chn,m_pstuOsdInfo[3][chn].y);
        }
        if(pVideo_para->have_common2_osd==1)
        {
            m_pstuOsdInfo[7][chn].bShow = 1;
            m_pstuOsdInfo[7][chn].x = pVideo_para->gps_osd_x;
            m_pstuOsdInfo[7][chn].y = pVideo_para->gps_osd_y;
            //DEBUG_MESSAGE("####CAvSnapTask::SetOsdInfo m_pstuOsdInfo[3][%d].x %d m_pstuOsdInfo[0][%d].y %d\n", chn, m_pstuOsdInfo[3][chn].x, chn,m_pstuOsdInfo[3][chn].y);
        }
        */
        return 0;
    }
    else
    {
        return -1;
    }
}

int CAvSnapTask::SetSnapSerialNumber(msgMachineStatistics_t *pstuMachineStatistics)
{
    if(pstuMachineStatistics == NULL)
    {
        return -1;
    }
    else
    {
        m_uHighSerialNum = pstuMachineStatistics->u16RestartCnt;
        return 0;
    }
}
unsigned short CAvSnapTask::GetSnapSerialNumber()
{
    return m_uHighSerialNum;
}
int CAvSnapTask::SetChnState(int flag)
{
    m_analog_digital_flag = flag;
    return 0;
}
bool CAvSnapTask::IsParaChanged(unsigned char &oldResolution, unsigned char &newResolution, unsigned char &oldQuality, unsigned char &newQuality, bool bShow, char *oldstate, char *newstate, rm_uint16_t &oldBitmap, rm_uint16_t &newBitmap)
{
    if(((oldResolution != newResolution) ||(oldQuality != newQuality)|| (strlen(oldstate) != strlen(newstate)) ||(oldBitmap != newBitmap)))
    {
        //DEBUG_MESSAGE("CAvSnapTask::IsParaChanged Yes\n");
        m_bParaChange = true;
        //strcpy(oldstate, newstate);
        oldResolution = newResolution;
        oldQuality = newQuality;
        oldBitmap = newBitmap;
    }
    else
    {
        //DEBUG_MESSAGE("CAvSnapTask::IsParaChanged No\n");
        m_bParaChange = false;
    }
    return m_bParaChange;
}
int CAvSnapTask::SnapForEncoder( unsigned int phy_video_chn_num )
{
    return 0;
}

int CAvSnapTask::SnapForDecoder( unsigned int phy_video_chn_num )
{
    //if( m_pVpss->HiVpss_get_frame_for_snap( phy_video_chn_num, &stuFrameInfo) != 0 )
    {
        DEBUG_ERROR("Encode picture HiVpss_get_frame_for_snap failed\n");
        return -1;
    }
    
    return 0;
}

#ifdef _AV_PRODUCT_CLASS_IPC_
int CAvSnapTask::ClearAllSnapPictures()
{
    for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
    {
        N9M_SHClearAllFrames(m_SnapShareMemHandle[chn]);
    }

    return -1;
}
bool CAvSnapTask::IsSnapParaChanged(const AvSnapPara_t *pNew_snap_para)
{
    bool bRet = false;
    
    if(NULL == m_pSignalSnapPara)
    {
        m_pSignalSnapPara = new AvSnapPara_t;
        memset(m_pSignalSnapPara, 0, sizeof(AvSnapPara_t));
        bRet = true;
    }
    else
    {
        /*only if resolution ucQuality stuSnapSharePara changed we need recreate encoder*/
        av_resolution_t resolution = ConvertResolutionFromIntToEnum(pNew_snap_para->ucResolution);
        if(/*(m_pSignalSnapPara->ucResolution != pNew_snap_para->ucResolution)*/
            (m_eSnapRes != resolution)|| \
            (m_pSignalSnapPara->ucQuality != pNew_snap_para->ucQuality) || \
            (m_pSignalSnapPara->usOverlayBitmap != pNew_snap_para->usOverlayBitmap)|| \
            (0 != memcmp(m_pSignalSnapPara->stuSnapSharePara.stuOsdOverlay[0].ucOsdContent, pNew_snap_para->stuSnapSharePara.stuOsdOverlay[0].ucOsdContent, sizeof(m_pSignalSnapPara->stuSnapSharePara.stuOsdOverlay[0].ucOsdContent))) || \
            (0 != memcmp(m_pSignalSnapPara->stuSnapSharePara.stuOsdOverlay[1].ucOsdContent, pNew_snap_para->stuSnapSharePara.stuOsdOverlay[1].ucOsdContent, sizeof(m_pSignalSnapPara->stuSnapSharePara.stuOsdOverlay[1].ucOsdContent))))
        {
            printf("[%s %d]the snap param is changed! old[%d %d 0x%x  %s %s] new [%d %d 0x%x %s %s] \n", __FUNCTION__, __LINE__, \
                m_eSnapRes, m_pSignalSnapPara->ucQuality, m_pSignalSnapPara->usOverlayBitmap, \
                (char *)m_pSignalSnapPara->stuSnapSharePara.stuOsdOverlay[0].ucOsdContent, (char *)m_pSignalSnapPara->stuSnapSharePara.stuOsdOverlay[1].ucOsdContent, \
                resolution, pNew_snap_para->ucQuality, pNew_snap_para->usOverlayBitmap, \
                pNew_snap_para->stuSnapSharePara.stuOsdOverlay[0].ucOsdContent, pNew_snap_para->stuSnapSharePara.stuOsdOverlay[1].ucOsdContent);

            bRet = true;
        }
    }
    
    memcpy(m_pSignalSnapPara, pNew_snap_para, sizeof(AvSnapPara_t));
    if(true == bRet)
    {
        SyncSnapParaToSnapOsdInfo();
    }

    return bRet;
}

int CAvSnapTask::SyncSnapParaToSnapOsdInfo()
{

    if(NULL == m_pSignalSnapPara)
    {
        return 0;
    }

    for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); ++chn)
    {
        if((m_pSignalSnapPara->uiChannel & (1 << chn)))
        {
             //time is show or not
             if(m_pSignalSnapPara->usOverlayBitmap & 0x1)
             {
                 m_pstuOsdInfo[0][chn].bShow = 1;
             }
             else
             {
                m_pstuOsdInfo[0][chn].bShow = 0;
             }
             
             //vehicle is show or not
             if(m_pSignalSnapPara->usOverlayBitmap & 0x2)
             {
                m_pstuOsdInfo[1][chn].bShow = 1;
             }
             else
             {
                m_pstuOsdInfo[1][chn].bShow = 0;
             }
             
             //speed is show or not
             if(m_pSignalSnapPara->usOverlayBitmap & 0x4)
             {
                 m_pstuOsdInfo[2][chn].bShow = 1;
             }
             else
             {
                m_pstuOsdInfo[2][chn].bShow = 0;
             }
             
             //gps is show or not
             if(m_pSignalSnapPara->usOverlayBitmap & 0x8)
             {
                 m_pstuOsdInfo[3][chn].bShow = 1;
             }
             else
             {
                m_pstuOsdInfo[3][chn].bShow = 0;
             }
             
             //channel name is show or not
             if(m_pSignalSnapPara->usOverlayBitmap & 0x10)
             {
                 m_pstuOsdInfo[4][chn].bShow  = 1;
             } 
             else
             {
                m_pstuOsdInfo[4][chn].bShow = 0;
             }
             
             //device id is show or not
             if(m_pSignalSnapPara->usOverlayBitmap & 0x20)
             {
                 m_pstuOsdInfo[5][chn].bShow= 1;
             }
             else
             {
                m_pstuOsdInfo[5][chn].bShow = 0;
             }
            
             //user define osd1 is show or not
             if(m_pSignalSnapPara->usOverlayBitmap & 0x40)
             {
                 m_pstuOsdInfo[6][chn].bShow = 1;
                 m_pstuOsdInfo[6][chn].x = m_pSignalSnapPara->stuSnapSharePara.stuOsdOverlay[0].uixcoord;
                 m_pstuOsdInfo[6][chn].y = m_pSignalSnapPara->stuSnapSharePara.stuOsdOverlay[0].uiycoord;
                 strncpy(m_pstuOsdInfo[6][chn].szStr, (char *)m_pSignalSnapPara->stuSnapSharePara.stuOsdOverlay[0].ucOsdContent, sizeof(m_pstuOsdInfo[6][chn].szStr));
                 m_pstuOsdInfo[6][chn].szStr[sizeof(m_pstuOsdInfo[6][chn].szStr)-1] = '\0';
             }
             else
             {
                 m_pstuOsdInfo[6][chn].bShow = 0;
                 m_pstuOsdInfo[6][chn].x = m_pSignalSnapPara->stuSnapSharePara.stuOsdOverlay[0].uixcoord;
                 m_pstuOsdInfo[6][chn].y = m_pSignalSnapPara->stuSnapSharePara.stuOsdOverlay[0].uiycoord;
                 strncpy(m_pstuOsdInfo[6][chn].szStr, (char *)m_pSignalSnapPara->stuSnapSharePara.stuOsdOverlay[0].ucOsdContent, sizeof(m_pstuOsdInfo[6][chn].szStr));
                 m_pstuOsdInfo[6][chn].szStr[sizeof(m_pstuOsdInfo[6][chn].szStr)-1] = '\0';
             }
             
             //user define osd2 is show or not
             if(m_pSignalSnapPara->usOverlayBitmap & 0x80)
             {
                 m_pstuOsdInfo[7][chn].bShow = 1;
                 m_pstuOsdInfo[7][chn].x = m_pSignalSnapPara->stuSnapSharePara.stuOsdOverlay[1].uixcoord;
                 m_pstuOsdInfo[7][chn].y = m_pSignalSnapPara->stuSnapSharePara.stuOsdOverlay[1].uiycoord;
                 strncpy(m_pstuOsdInfo[7][chn].szStr, (char *)m_pSignalSnapPara->stuSnapSharePara.stuOsdOverlay[1].ucOsdContent, sizeof(m_pstuOsdInfo[7][chn].szStr));
                 m_pstuOsdInfo[7][chn].szStr[sizeof(m_pstuOsdInfo[7][chn].szStr)-1] = '\0';           
            }
            else
            {
                m_pstuOsdInfo[7][chn].bShow = 0;
                m_pstuOsdInfo[7][chn].x = m_pSignalSnapPara->stuSnapSharePara.stuOsdOverlay[1].uixcoord;
                m_pstuOsdInfo[7][chn].y = m_pSignalSnapPara->stuSnapSharePara.stuOsdOverlay[1].uiycoord;
                strncpy(m_pstuOsdInfo[7][chn].szStr, (char *)m_pSignalSnapPara->stuSnapSharePara.stuOsdOverlay[1].ucOsdContent, sizeof(m_pstuOsdInfo[7][chn].szStr));
                m_pstuOsdInfo[7][chn].szStr[sizeof(m_pstuOsdInfo[7][chn].szStr)-1] = '\0'; 

            }
        }
    } 

    m_eSnapRes = ConvertResolutionFromIntToEnum(m_pSignalSnapPara->ucResolution);
    
    if( m_eSnapRes == _AR_UNKNOWN_ )
   {
        m_bCreatedSnap = false;

        av_resolution_t eRes = _AR_UNKNOWN_;
        int CurrentWait = m_ResWeight[_AR_UNKNOWN_];
        bool Encode_D1 = false;
        bool Encode_960H_WHD1 = false;
        for( int ch = 0; ch < m_u8MaxChlOnThisChip; ++ch)
        {
            if(m_ResWeight[m_eMainStreamRes[ch]] < 0)
            {
                DEBUG_WARNING("#####Do not surport resolution %d######\n",m_eMainStreamRes[ch]);
                continue;
            }
        	
            switch(m_eMainStreamRes[ch])
            {
                case _AR_D1_:
                    Encode_D1 = true;
                    break;
                case _AR_960H_WHD1_:
                    Encode_960H_WHD1 = true;
                    break;
                default:
                    break;
            }
            if( m_ResWeight[m_eMainStreamRes[ch]] < CurrentWait )
            {
                eRes = m_eMainStreamRes[ch];
                CurrentWait = m_ResWeight[m_eMainStreamRes[ch]];
                //first found break;
                break;
            }
        }

        if( eRes == _AR_UNKNOWN_ || m_eTvSystem == _AT_UNKNOWN_ )
        {
            DEBUG_WARNING( "[%s %d]the resolution and tvSystem setted to 720p PAL \n", __FUNCTION__, __LINE__);
            eRes = _AR_720_;
            m_eTvSystem = _AT_PAL_;
        }

        //D1 and 960H_WHD1 exist in mainstream at same time, then set eRes to _AR_HD1_
        if(true == Encode_D1 && true == Encode_960H_WHD1 )
        {
            eRes = _AR_HD1_;
            CurrentWait = 2;
        }

        m_eSnapRes = eRes;
        m_eSnapTvSystem = m_eTvSystem;
        DEBUG_WARNING( "CAvSnapTask::CreateSnapEncoder create snap encoder ok, res=%d tvSys=%d\n", eRes, m_eTvSystem );
    }
    //IPC only has a video chnnel ,so we compare with mainstream channel 0
    if((m_ResWeight[m_eSnapRes] == m_ResWeight[_AR_UNKNOWN_]) || (-1 == m_ResWeight[m_eSnapRes]) || \
        (m_ResWeight[m_eSnapRes] > m_ResWeight[m_eMainStreamRes[0]]))
    {
        m_eSnapRes = m_eMainStreamRes[0];
    }

    return 0;
}


int CAvSnapTask::DestroySnapEncoder()
{
    if(false == m_bCreatedSnap)
    {
        return 0;
    }
    
    m_bCreatedSnap = false;
    return m_pEnc->Ae_destroy_snap_encoder();
}

#endif

av_resolution_t CAvSnapTask::ConvertResolutionFromIntToEnum(rm_uint8_t resulotion)
{
    av_resolution_t res = _AR_UNKNOWN_;
    if(0 == resulotion)
    {
        res = _AR_CIF_;
    }
    else if(1 == resulotion)
    {
        res = _AR_HD1_;
    }
    else if(2== resulotion)
    {
        res = _AR_D1_;
    }
    else if(3 == resulotion)
    {
        res = _AR_QCIF_;
    }
    else if(4 == resulotion)
    {
       res = _AR_QVGA_;
    }
    else if(5 == resulotion)
    {
       res = _AR_VGA_;
    }
    else if(6 == resulotion)
    {
        res = _AR_720_;
    }
    else if(7 == resulotion)
    {
        res = _AR_1080_;
    }
    else if(8 == resulotion)
    {
        res = _AR_3M_;
    }
    else if(9 == resulotion)
    {
        res = _AR_5M_;
    }
    else if(10 == resulotion)
    {
       res = _AR_960H_WQCIF_;
    }
    else if(11 == resulotion)
    {
        res = _AR_960H_WCIF_;
    }
    else if(12 == resulotion)
    {
        res = _AR_960H_WHD1_;
    }
    else if(13 == resulotion)
    {
        res = _AR_960H_WD1_;
    }
    else if(14 == resulotion)
    {
        res = _AR_960P_;
    }
    else if(15 == resulotion)
    {
        res = _AR_Q1080P_;
    }
    else if(16 == resulotion)
    {
        res = _AR_SVGA_;
    }
    else if(17 == resulotion)
    {
        res = _AR_XGA_;
    }
    else
    {
        printf("[%s %d]the resolution is not defined! \n", __FUNCTION__, __LINE__);
    }

    return res;
}

unsigned int CAvSnapTask::CalcMaxLenChlName()
{
    unsigned u32Len = 2;

    for( int i = 0; i < m_u8MaxChlOnThisChip; i++ )
    {
        if( strlen(m_pstuOsdInfo[1][i].szStr) > u32Len )
        {
            u32Len = strlen(m_pstuOsdInfo[1][i].szStr);
        }
    }
    
    return u32Len;
}



#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/time.h>

#include "AvPreviewDecoder.h"

#define MAX_VO_WAIT_FR 20
#if 1
#define APD_PRINT(fmt, args...)	do {printf("[%s, %d]: "fmt"\n", __FUNCTION__, __LINE__, ##args);}while(0);
#else
#define APD_PRINT(fmt, args...) do{}while(0);
#endif
#define APD_ERROR(fmt, args...)	do {printf("[%s, %d]: "fmt"\n", __FUNCTION__, __LINE__, ##args);}while(0);

void *AvDecodeEntry(void *para)
{
    AvPrevThreadParam_t *pPara = (AvPrevThreadParam_t *)para;
    CAVPreviewDecCtrl *pApdOperate = (CAVPreviewDecCtrl *)pPara->ptrObj;
	prctl(PR_SET_NAME, "AvDecodeEntry");

    pApdOperate->SignalDecDealBody( pPara->u8ThreadIndex, pPara->u8StartCh, pPara->u8EndCh );

    return NULL;
}

//index is frame rate
static unsigned char g_u8szAdjustFrameLeftMin[31] = {
 2, 2, 2, 2, 2, 2, // 1~5
    2, 2, 2, 2, 2, // 6~10
    3, 3, 3, 3, 3, // 11~15
    3, 3, 3, 3, 3, // 16~20
    3, 3, 3, 3, 3, // 21~25
    3, 3, 3, 3, 3  // 26~30
};
static unsigned char g_u8szAdjustFrameLeftMax[31] = {
 2, 2,  2,  3,  3,  3, // 1~5
    3,  3,  3,  3,  3, // 6~10
    4,  4,  4,  4,  4, // 11~15
    5,  5,  5,  5,  5, // 16~20
    5,  5,  5,  5,  5, // 21~25
    5,  5,  5,  5,  5 // 26~30
};

static unsigned char g_u8szAdjustConvergenceSpeed[31] = {
 1, 1,  2,  3,  4,  5, // 1~5
    6,  6,  6,  6,  6, // 6~10
    7,  7,  7,  7,  7, // 11~15
    8,  8,  8,  8,  8, // 16~20
    9,  9,  9,  10, 10, // 21~25
    10, 12, 13, 13, 13 //26~30
};

static unsigned char g_u8szAdjustFrameLeftJump[31] = {
 5, 6,  7,  8,  9,  10, // 1~5
    13, 13, 13, 14, 14, // 6~10
    15, 15, 15, 15, 15, // 11~15   3sec
    15, 16, 16, 17, 17, // 16~20   2.5sec
    18, 18, 19, 19, 20, // 21~25
    20, 21, 21, 22, 22 //26~30
};

CAVPreviewDecCtrl::CAVPreviewDecCtrl( unsigned int u32MaxPrewChnNum, unsigned int u32ChnNumPerThread, int purpose, APD_Operation_t *pOperation, av_tvsystem_t eTvSystem )
{
    assert( u32MaxPrewChnNum > 0 && u32ChnNumPerThread > 0 && pOperation != NULL );

    m_u8AudioChn = 0;
    m_bAudioMute = false;
    m_u8Purpose = purpose;
    m_ApdOperate = *pOperation;
    m_u8ChlNum = u32MaxPrewChnNum;
    m_eThreadReq = EPREVTD_PAUSE;
    m_eTvSystem = eTvSystem;

    m_u32CurDecChlNum = 0;
#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_HDVR_)
    memset( m_szChannelMap, -1, sizeof(m_szChannelMap) );
#else    
    for( int i = 0; i < pgAvDeviceInfo->Adi_max_channel_number(); i++ )
    {
        m_szChannelMap[i] = i;
    }
#endif
	
    memset( szRealVoChlLay, -1, sizeof(szRealVoChlLay) );
    memset( m_szChannelList, -1, sizeof(m_szChannelList) );
    memset( m_szDevOnLineState, 1, sizeof(m_szDevOnLineState) );
    memset( m_szFrameRateInit, 30, sizeof(m_szFrameRateInit) );
    m_u8CurStreamType = 0;
    
    m_u8ThreadNum = (m_u8ChlNum + u32ChnNumPerThread - 1) / u32ChnNumPerThread;
   // DEBUG_MESSAGE( "=======CAVPreviewDecCtrl ChnNum %d prethread %d threadNum=%d=========\n", u32MaxPrewChnNum, u32ChnNumPerThread, m_u8ThreadNum );

    for( int ch = 0; ch < m_u8ChlNum; ch++ )
    {
        m_pChlParam[ch] = new AvPrevChlDecParam_t;
        m_pChlParam[ch]->u8HaveTask = false;
        m_pChlParam[ch]->u8Index = ch;
        m_pChlParam[ch]->u8ThreadIndex = (ch / u32ChnNumPerThread);
        m_pChlParam[ch]->u8RealCh = 0xff;
        
        memset( &(m_pChlParam[ch]->stuCmdTmp), 0, sizeof(m_pChlParam[ch]->stuCmdTmp) );
        
        m_pChlParam[ch]->u64LastFramePts = 0;
        m_pChlParam[ch]->u8FrameRate = 30;
        m_pChlParam[ch]->u8RequestIFrame = 1;
        m_pChlParam[ch]->u8DecoderRun = false;
        
        m_pChlParam[ch]->ShareStreamHandle = NULL;
        m_pChlParam[ch]->pData = NULL;
        m_pChlParam[ch]->pDataTemp = NULL;
        m_pChlParam[ch]->u32TmpDataLen = 0;
        m_pChlParam[ch]->u64TmpPts = 0;

        //DEBUG_MESSAGE("************create share mem ch=%d\n", ch);
        char stream_buffer_id[32];
        char stream_buffer_name[32];
        unsigned int stream_buffer_size = 0;
        unsigned int stream_buffer_frame = 0;

        sprintf(stream_buffer_id, "avPrevDecMain");
        if(pgAvDeviceInfo->Adi_get_stream_buffer_info(BUFFER_TYPE_MAIN_STREAM, ch, stream_buffer_name, stream_buffer_size, stream_buffer_frame) != 0)
        {
            _AV_FATAL_(1, "get buffer failed!\n");
        }
        m_ShareStreamHandle[ch][0] = N9M_SHCreateShareBuffer( stream_buffer_id, stream_buffer_name, stream_buffer_size, stream_buffer_frame, SHAREMEM_READ_MODE );

        sprintf(stream_buffer_id, "avPrevDecSub");
        if(pgAvDeviceInfo->Adi_get_stream_buffer_info(BUFFER_TYPE_ASSIST_STREAM, ch, stream_buffer_name, stream_buffer_size, stream_buffer_frame) != 0)
        {
            _AV_FATAL_(1, "get buffer failed!\n");
        }
        m_ShareStreamHandle[ch][1] = N9M_SHCreateShareBuffer( stream_buffer_id, stream_buffer_name, stream_buffer_size, stream_buffer_frame, SHAREMEM_READ_MODE );
        m_u8HdvrChannelState[ch] = HDVR_STATE_INLID;
    }

    for( int t = 0; t < m_u8ThreadNum; t++ )
    {
        m_pThreadParam[t] = new AvPrevThreadParam_t;
        m_pThreadParam[t]->u8ThreadIndex = t;
        m_pThreadParam[t]->u8StartCh = t * u32ChnNumPerThread;
        m_pThreadParam[t]->u8EndCh = (t + 1) * u32ChnNumPerThread;
        m_pThreadParam[t]->u8PauseThread = false;
        m_pThreadParam[t]->ptrObj = this;
        m_pThreadParam[t]->u64ThreadTime = 0;

        Av_create_normal_thread(AvDecodeEntry, (void*)(m_pThreadParam[t]), NULL);
    }

    m_bFirstFrameCheckLeft = true;
}


CAVPreviewDecCtrl::~CAVPreviewDecCtrl()
{
    this->ExitAllThread();
    
    for( int t=0; t<m_u8ThreadNum; t++ )
    {
        if( m_pThreadParam[t] )
        {
            delete m_pThreadParam[t];
            m_pThreadParam[t] = NULL;
        }
    }

    for( int ch=0; ch<m_u8ChlNum; ch++ )
    {
        if( !m_pChlParam[ch] )
        {    
            continue;
        }

        if( m_pChlParam[ch]->pDataTemp )
        {
            delete [] m_pChlParam[ch]->pDataTemp;
            m_pChlParam[ch]->pDataTemp = NULL;
        }
        
        delete m_pChlParam[ch];
        m_pChlParam[ch] = NULL;

        if( m_ShareStreamHandle[ch][0] )
        {
            N9M_SHDestroyShareBuffer(m_ShareStreamHandle[ch][0]);
            m_ShareStreamHandle[ch][0] = NULL;
        }
        if( m_ShareStreamHandle[ch][1] )
        {
            N9M_SHDestroyShareBuffer(m_ShareStreamHandle[ch][1]);
            m_ShareStreamHandle[ch][1] = NULL;
        }
    }
}

int CAVPreviewDecCtrl::ExitAllThread()
{
    m_eThreadReq = EPREVTD_EXIT;
    bool bExit = false;
    while( !bExit )
    {
        bExit = true;
        for( int t=0; t<m_u8ThreadNum; t++ )
        {
            if( m_pThreadParam[t]->eThreadState != EPREVTD_EXIT )
            {
                bExit = false;
                break;
            }
        }
        mSleep(10);
    }

    return 0;
}

int CAVPreviewDecCtrl::PrapareFrameData( AvPrevChlDecParam_t* pChParam )
{
    SShareMemFrameInfo *pFrameInfo = NULL;
    char *pFirstBuffer = NULL;
    char *pSecondBuffer = NULL;
    rm_uint32_t FirstBufferLen = 0, SecondBufferLen = 0;
    int frameLen = 0;

    frameLen = N9M_SHGetOneFrame(pChParam->ShareStreamHandle, &pFirstBuffer, FirstBufferLen, &pSecondBuffer, SecondBufferLen, &pFrameInfo);
    if( frameLen < 34 )
    {
        return -1;
    }
    else if( frameLen > PREVIEW_MAX_ONE_FRAME_SIZE )
    {
        APD_ERROR("To big frame len=%d\n", frameLen);
        return -1;
    }
    if((FirstBufferLen == 0) || (pFirstBuffer == NULL) || (pFrameInfo == NULL))
    {
        APD_ERROR("get one frame success, but para invalid!\n");
        return -1;
    }
#if 0 // 考虑到MDVR资源较为充足，并且解码最多为4路子码流，特修改此处，避免外部对数据修正时对回放的影响
    if( (SecondBufferLen == 0) || (pSecondBuffer == NULL) )
    {
        pChParam->pData = pFirstBuffer;
        pChParam->u32TmpDataLen = FirstBufferLen;
    }
    else
    {
        memcpy( pChParam->pDataTemp + pChParam->u32TmpDataLen, pFirstBuffer, FirstBufferLen );
        pChParam->u32TmpDataLen += FirstBufferLen;
        memcpy( pChParam->pDataTemp + pChParam->u32TmpDataLen, pSecondBuffer, SecondBufferLen );
        pChParam->u32TmpDataLen += SecondBufferLen;
        pChParam->pData = pChParam->pDataTemp;
    }
#else
    memcpy( pChParam->pDataTemp + pChParam->u32TmpDataLen, pFirstBuffer, FirstBufferLen );
    pChParam->u32TmpDataLen += FirstBufferLen;
    if( (SecondBufferLen != 0) && (pSecondBuffer != NULL) )
    {
        memcpy( pChParam->pDataTemp + pChParam->u32TmpDataLen, pSecondBuffer, SecondBufferLen );
        pChParam->u32TmpDataLen += SecondBufferLen;
    }

    pChParam->pData = pChParam->pDataTemp;
#endif
    if( pChParam->pData[2] != 'd' || pChParam->pData[3] != 'c' )
    {
        DEBUG_ERROR("index %d Frmae data maybe error\n", pChParam->u8Index);
        pChParam->u32TmpDataLen = 0;
        return -1;
    }

    static unsigned int vtimeOff = sizeof(RMSTREAM_HEADER);
    
    RMSTREAM_HEADER* pHeader = (RMSTREAM_HEADER*)pChParam->pData;
    RMFI2_VTIME* pVTime = (RMFI2_VTIME*)(pChParam->pData + vtimeOff);

    if( pHeader->lExtendLen + pHeader->lFrameLen + vtimeOff != pChParam->u32TmpDataLen )
    {
        DEBUG_ERROR("index %d data error, getLen=%d exLen=%ld FrameLen=%ld\n", pChParam->u8Index
            , pChParam->u32TmpDataLen, pHeader->lExtendLen, pHeader->lFrameLen);
        pChParam->u32TmpDataLen = 0;
        return -1;
    }
    
    //DEBUG_MESSAGE("parse data, exlen=%ld hlen=%ld dataLen(%d=%d) type=%d\n", pHeader->lExtendLen, pHeader->lFrameLen
    //    , pChParam->u32TmpDataLen, frameLen, frameType);
    pChParam->u64TmpPts = pVTime->llVirtualTime;
    if( pFrameInfo->flag == SHAREMEM_FLAG_IFRAME )
    {
        pChParam->eFrameType = DECDATA_IFRAME;
    }
    else if( pFrameInfo->flag == SHAREMEM_FLAG_PFRAME )
    {
        pChParam->eFrameType = DECDATA_PFRAME;
    }
    else if( pFrameInfo->flag == SHAREMEM_FLAG_AFRAME )
    {
        pChParam->eFrameType = DECDATA_AFRAME;
    }
    else
    {
        DEBUG_ERROR("index %d unknown framtype=%d\n", pChParam->u8Index, pFrameInfo->flag);
        pChParam->u32TmpDataLen = 0;
        return -1;
    }

    return 0;
}
    
int CAVPreviewDecCtrl::ReleaseFrameData( AvPrevChlDecParam_t* pChParam )
{
    pChParam->u32TmpDataLen = 0;

    return 0;
}

int CAVPreviewDecCtrl::ResetAdjustInfo( AvPrevChlDecParam_t* pChParam )
{
    pChParam->u64BaseFramePts = 0;
    pChParam->u64LastFramePts = 0;
    pChParam->u32VideoSendCount = 0;

    //DEBUG_MESSAGE("*********CAVPreviewDecCtrl::ResetAdjustInfo ch=%d\n", pChParam->u8RealCh);
    
    return 0;
}

int CAVPreviewDecCtrl::SetBaseFrameRate( unsigned char u8Index, unsigned char u8BaseFrameRate )
{
    m_pChlParam[u8Index]->u8FrameRate = u8BaseFrameRate;
    if( m_pChlParam[u8Index]->u8FrameRate > 30 || m_pChlParam[u8Index]->u8FrameRate == 0 )
    {
        DEBUG_MESSAGE("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!height frame rate %d\n", m_pChlParam[u8Index]->u8FrameRate );
        m_pChlParam[u8Index]->u8FrameRate = 30;
    }

   // m_ApdOperate.SetDecPlayFrameRate( m_u8Purpose, u8Index, 32 );//((u8BaseFrameRate<30)?30:u8BaseFrameRate+2) );//u8BaseFrameRate + 2 );// 60 );

    m_pChlParam[u8Index]->i32AjustSpanBase = (1000 / m_pChlParam[u8Index]->u8FrameRate);
    m_pChlParam[u8Index]->i32AjustSpan = 0;

    m_pChlParam[u8Index]->u8AdjustCallCount = g_u8szAdjustConvergenceSpeed[m_pChlParam[u8Index]->u8FrameRate];

    //APD_PRINT("CAVPreviewDecCtrl::SetBaseFrameRate purpose=%d index=%d frameRate=%d spanbase=%d call speed=%d fr=%d, vo fr=%d\n"
    //    , m_u8Purpose, u8Index, m_pChlParam[u8Index]->u8FrameRate, m_pChlParam[u8Index]->i32AjustSpanBase
    //    , m_pChlParam[u8Index]->u8AdjustCallCount, u8BaseFrameRate, 30+2 );

    return 0;
}

int CAVPreviewDecCtrl::ProcessAddTask( AvPrevChlDecParam_t* pChParam, AvPrevTask_t* pstuTaskParam )
{
    //APD_PRINT( "CAVPreviewDecCtrl::ProcessAddTask index=%d realch=%d haveTask=%d\n", pChParam->u8Index
    //    , pstuTaskParam->u8RealChl, pChParam->u8HaveTask );
    if( pstuTaskParam->u8ChlIndex != pChParam->u8Index )
    {
        APD_ERROR("Add task error, need index=%d myindex=%d\n", pstuTaskParam->u8ChlIndex, pChParam->u8Index );
        return -1;
    }

    if( pstuTaskParam->u8RealChl == pChParam->u8RealCh && pstuTaskParam->u8StreamType == pChParam->u8StreamType)
    {
        APD_PRINT("index=%d hopCh=%d nowCh=%d stream hop=%d now=%d, so ingor cmd\n", pstuTaskParam->u8ChlIndex, pstuTaskParam->u8RealChl, pChParam->u8RealCh
            , pstuTaskParam->u8StreamType, pChParam->u8StreamType );
        return 0;
    }

    //DEBUG_MESSAGE( "CAVPreviewDecCtrl::ProcessAddTask index=%d ch=%d stream=%d", pChParam->u8Index, pstuTaskParam->u8RealChl, pstuTaskParam->u8StreamType );
    if( pChParam->u8HaveTask )
    {
        this->ProcessDelTask( pChParam );
    }

    pChParam->u8RealCh = pstuTaskParam->u8RealChl;
    pChParam->u8StreamType = pstuTaskParam->u8StreamType;
    
    this->ResetAdjustInfo( pChParam );
    this->ReleaseFrameData( pChParam );
    pChParam->eFrameType = DECDATA_INVALID;

    pChParam->u8RequestIFrame = 1;

    this->SetBaseFrameRate( pChParam->u8Index, pstuTaskParam->u32FrameRate );

    //m_ApdOperate.SetDecPlayFrameRate( m_u8Purpose, pChParam->u8Index, pstuTaskParam->u32FrameRate*2 );
    
    //if( pChParam->pShareStream )
    //{
    //    APD_ERROR("UN expected thing occured index=%d myindex=%d\n", pstuTaskParam->u8ChlIndex, pChParam->u8Index );
     //   delete pChParam->pShareStream;
    //}

    if( pstuTaskParam->u8StreamType == 0 )
    {
        pChParam->ShareStreamHandle = m_ShareStreamHandle[pstuTaskParam->u8RealChl][0];
    }
    else
    {
        pChParam->ShareStreamHandle = m_ShareStreamHandle[pstuTaskParam->u8RealChl][1];
    }
    N9M_SHGotoLatestIFrame(pChParam->ShareStreamHandle);

    pChParam->pData = NULL;
    pChParam->u32TmpDataLen = 0;
    if( !pChParam->pDataTemp )
    {
        pChParam->pDataTemp = new char[PREVIEW_MAX_ONE_FRAME_SIZE];
    }

    pChParam->u8HaveTask = 1;
    pChParam->u8DecoderRun = 1;

    return 0;
}

int CAVPreviewDecCtrl::ProcessDelTask( AvPrevChlDecParam_t* pChParam )
{
    //APD_PRINT( "CAVPreviewDecCtrl::ProcessDelTask index=%d realch=%d haveTask=%d decoderRun=%d", pChParam->u8Index
        //, pChParam->u8RealCh, pChParam->u8HaveTask, pChParam->u8DecoderRun);
    if( !pChParam->u8HaveTask )
    {
        return 0;
    }
    m_ApdOperate.CloseDecoder( m_u8Purpose, pChParam->u8Index );
    this->ReleaseFrameData(pChParam);
    pgAvDeviceInfo->Adi_set_created_preview_decoder(pChParam->u8RealCh, false);
    pChParam->u64BaseFramePts = 0;
    pChParam->u64LastFramePts= 0;

    pChParam->u32VideoSendCount = 0;
    
    pChParam->u8HaveTask = 0;
    pChParam->u8DecoderRun = 0;
    
    pChParam->u8FrameRate = 0;
    pChParam->u8RequestIFrame = 1;
    
    pChParam->u8RealCh = 0xff; //real ch for sharebuffer

    if( pChParam->pDataTemp)
    {
        delete [] pChParam->pDataTemp;
        pChParam->pDataTemp = NULL;
    }

    pChParam->u32TmpDataLen = 0;
    pChParam->u64TmpPts = 0;
    pChParam->eFrameType = DECDATA_INVALID;
    
    return 0;
}

int CAVPreviewDecCtrl::ProcessDecoder( unsigned int chn, AvPrevChlDecParam_t* pChParam )
{
   // char cRealCh = m_szChannelMap[(int)(m_szChannelList[chn])]; 
    if( pChParam->PlayCmd.PullCmd( &pChParam->stuCmdTmp.iCmd, pChParam->stuCmdTmp.szData) == 0 )
    {
        if( pChParam->stuCmdTmp.iCmd == PREVDEC_CMD_ADDTASK )
        {
            this->ProcessAddTask( pChParam, (AvPrevTask_t*)pChParam->stuCmdTmp.szData );
        }
        else if( pChParam->stuCmdTmp.iCmd == PREVDEC_CMD_DELTASK )
        {
            this->ProcessDelTask( pChParam );
        }
        else
        {
            DEBUG_MESSAGE("###########unknow cmd index=%d\n", chn);
        }
       /* else if( pChParam->stuCmdTmp.iCmd == PREVDEC_CMD_PAUSETASK )
        {
            APD_PRINT( "CAVPreviewDecCtrl::ProcessDecoder get pause channel cmd\n" );
            pChParam->u8DecoderRun = 0;
        }
        else if( pChParam->stuCmdTmp.iCmd == PREVDEC_CMD_CONTINUETASK )
        {
            APD_PRINT( "CAVPreviewDecCtrl::ProcessDecoder get continue channel cmd\n" );
            pChParam->u8DecoderRun = 1;
        }*/

        return 0;
    }

    if( !pChParam->u8HaveTask || !pChParam->u8DecoderRun )
    {
        return -1;
    }
    if( pChParam->u32TmpDataLen == 0 )
    {
        if( this->PrapareFrameData(pChParam) != 0 )
        {
            return -1;
        }
    }

    if( pChParam->u8RequestIFrame )
    {
        if( pChParam->eFrameType != DECDATA_IFRAME )
        {
            //DEBUG_ERROR("[%d] I need I frame\n", chn );
            this->ReleaseFrameData(pChParam);
            return 0;
        }

        //make sure that, this the first part of frame head is pts
        RMFI2_VTIME* pvTime = (RMFI2_VTIME*)(pChParam->pData+sizeof(RMSTREAM_HEADER));
        pvTime->llVirtualTime = 0;
        
        m_ApdOperate.SendDataToDec( m_u8Purpose, chn, (unsigned char*)pChParam->pData, pChParam->u32TmpDataLen, pChParam->eFrameType );

        this->ReleaseFrameData(pChParam);
        
        if( m_bFirstFrameCheckLeft )
        {
            int left = N9M_SHGetLeftFrameCount(pChParam->ShareStreamHandle, 1);
            //check, continue request I frame
            if( left > (g_u8szAdjustFrameLeftJump[pChParam->u8FrameRate] / 2) )
            {
                DEBUG_ERROR("####################### ch=%d continue jumto next iframe, bacause left=%d need11=%d continue request I frame\n", chn, left
                    , g_u8szAdjustFrameLeftJump[pChParam->u8FrameRate] / 2 );
                return 0;
            }
        }
        
        pChParam->u8RequestIFrame = 0;

        return 0;
    }
    
    int iCheckSend = 0;
    if( pChParam->eFrameType != DECDATA_AFRAME )
    {
        //check send
        // DEBUG_MESSAGE("ch=%d pts=%lld\n", pChParam->u8RealCh, pChParam->u64TmpPts);
        iCheckSend = this->CheckSend(pChParam);

        //DEBUG_MESSAGE("=====get data pts=%lld type=%d send=%d\n", pChParam->u64TmpPts, iCheckSend, pChParam->eFrameType);
        if( iCheckSend == -1 )
        {
            return -1;
        }
        if( iCheckSend == -2 )
        {
            pChParam->u8RequestIFrame = true;
            this->ResetAdjustInfo( pChParam );
            this->ReleaseFrameData(pChParam);
            return 0;
        }

        pChParam->u32VideoSendCount++;
       // reduce display delay
        if( pChParam->u32VideoSendCount % pChParam->u8AdjustCallCount == 0 )
        {
            if( this->SmoooothVideo( pChParam ) == -1 )
            {
                pChParam->u8RequestIFrame = true;
                this->ResetAdjustInfo( pChParam );
                this->ReleaseFrameData(pChParam);
                return 0;
            }
        }

        if( pChParam->eFrameType == DECDATA_IFRAME )
        {
            int iFrCount = 0;
            int pDataLen = 0;
            m_ApdOperate.GetDecWaitFrCnt(m_u8Purpose, chn, &iFrCount, &pDataLen, DECDATA_IFRAME);
            if( iFrCount > 20 )
            {
                //if( pChParam->bMarkClear )
               // {
                    DEBUG_MESSAGE("***********CAVPreviewDecCtrl::ProcessDecoder ch=%d realchn=%d wRr=%d reset decodcer \n", chn, pChParam->u8RealCh, iFrCount);
                    m_ApdOperate.ResetDecoderChannel(m_u8Purpose, chn);
                    if( m_u8AudioChn == pChParam->u8RealCh )
                    {
                        m_ApdOperate.ClearAoBuf(chn);
                    }

                    RMFI2_VTIME* pvTime = (RMFI2_VTIME*)(pChParam->pData+sizeof(RMSTREAM_HEADER));
                    pvTime->llVirtualTime = 0;

                    if( iFrCount > 30 )
                    {
                        pChParam->u8RequestIFrame = 1;
                    }
               //     pChParam->bMarkClear = false;
               // }
              //  else
              //  {
              //      pChParam->bMarkClear = true;
               // }
            }
            else
            {
                pChParam->u8FlipForVoFramerateCheck++;
                if( pChParam->u8FlipForVoFramerateCheck > 5 )
                {
                    pChParam->u8FlipForVoFramerateCheck = 0;
                    
                    int fr = 32;
                    if( m_ApdOperate.GetVoChannelFramerate(szRealVoChlLay[chn], fr) == 0 )
                    {
                        pChParam->u8LastVoFramerate = fr;
                    }
                    DEBUG_MESSAGE("get vo frame chn=%d pChParam->u8RealCh=%d fr=%d\n", chn,szRealVoChlLay[chn],fr);
                }
                if( iFrCount > 3 )
                {
                    if( pChParam->u8LastVoFramerate != 32 )
                    {
                        if( m_ApdOperate.SetVoChannelFramerate(szRealVoChlLay[chn], 32) == 0 )
                        {
                            pChParam->u8LastVoFramerate = 32;
                        }
                        DEBUG_MESSAGE("###ch%d realchn=%d set vo fr=32\n", chn,szRealVoChlLay[chn]);
                    }
                }
                if( iFrCount < 2 )
                {
                    unsigned char u8AimFr = (m_eTvSystem==_AT_PAL_?25:30);
                    //DEBUG_MESSAGE("check , fr=%d sytem=%d\n", u8AimFr, m_eTvSystem);
                    if( pChParam->u8LastVoFramerate != u8AimFr )
                    {
                        if( m_ApdOperate.SetVoChannelFramerate(szRealVoChlLay[chn], u8AimFr) == 0 )
                        {
                            pChParam->u8LastVoFramerate = u8AimFr;
                        }
                        DEBUG_MESSAGE("###ch%d realchn=%d set vo fr=%d\n", chn,szRealVoChlLay[chn] ,u8AimFr);
                    }
                }
            }
          //  else
          //  {
           //     pChParam->bMarkClear = false;
           // }
        }
    }
    else
    {
        //DEBUG_MESSAGE("switch=%d audioch=%d realch=%d mute=%d\n", pgAvDeviceInfo->Adi_get_audio_cvbs_ctrl_switch(), m_u8AudioChn, pChParam->u8RealCh, m_bAudioMute);
        //X5 X7:no need for preview audio
        if( !pgAvDeviceInfo->Adi_get_audio_cvbs_ctrl_switch() )
        {
            this->ReleaseFrameData(pChParam);
            return 0;
        }
        
        if( m_u8AudioChn != pChParam->u8RealCh || m_bAudioMute )
        {
            //drop unsuitable audio
            this->ReleaseFrameData(pChParam);
            return 0;
        }
    }
    
    int sendret = m_ApdOperate.SendDataToDec( m_u8Purpose, chn, (unsigned char*)pChParam->pData, pChParam->u32TmpDataLen, pChParam->eFrameType );

    if( sendret >= 0 )
    {
        this->ReleaseFrameData(pChParam);
    }
    else
    {
        if( pChParam->eFrameType == DECDATA_AFRAME )
        {
            this->ReleaseFrameData( pChParam );
        }
        else
        {
            N9M_SHGotoNextIFrame(pChParam->ShareStreamHandle);

            pChParam->u8RequestIFrame = true;
            this->ResetAdjustInfo( pChParam );
            this->ReleaseFrameData( pChParam );
        }
    }
    return 0;
}

void CAVPreviewDecCtrl::CheckVlAndReset(AvPrevChlDecParam_t* pChParam)
{
    if(pChParam && (pChParam->u8RealCh < pgAvDeviceInfo->Adi_max_channel_number()) && pChParam->ShareStreamHandle)
    {
#if !defined(_AV_PLATFORM_HI3535_)
        if(m_szDevOnLineState[pChParam->u8RealCh] == 0)
        {
		N9M_SHGotoPrevIFrameEx(pChParam->ShareStreamHandle, 1, NULL);
        }
#else
    	//这里的不在线其实就是视频丢失
        if(m_szDevOnLineState[pChParam->u8RealCh] == 0)
        {
            //cxliu,151118,避免未接IPC但通道使能时太多的打印
	     static int cnt=0;
	     if(cnt>3)
	     {
			return;
	     }
	     if(N9M_SHGotoPrevIFrameEx(pChParam->ShareStreamHandle, 1, NULL)==-1)
            {
            		sleep(1);
			cnt++;
	     }
        }
#endif
    }
}

void CAVPreviewDecCtrl::SignalDecDealBody( unsigned char u8ThreadIndex, unsigned char u8StartCh, unsigned char u8EndCh )
{
    // APD_PRINT("CAVPreviewDecCtrl::SignalDecDealBody Singnal dec start thread index=%d ch=(%d~%d) pid=%d\n"
    //     , u8ThreadIndex, u8StartCh, u8EndCh, getpid());
    AvPrevThreadParam_t* pThParam = m_pThreadParam[u8ThreadIndex];
    unsigned long long TimeInterval = 0;
    unsigned char u8Index = 0;
    bool bSleep = true;
    while( true )
    {
        if( this->m_eThreadReq != EPREVTD_RUNNING )
        {
            if( this->m_eThreadReq == EPREVTD_PAUSE )
            {
                pThParam->eThreadState = EPREVTD_PAUSE;
                mSleep(30);
                continue;
            }
            else if( this->m_eThreadReq == EPREVTD_CONTINUE )
            {
                mSleep(10);
                pThParam->eThreadState = EPREVTD_CONTINUE;
                continue;
            }
            else
            {
                break;
            }
        }
        
        if( pThParam->u8PauseThread )
        {
            mSleep(30);
            continue;
        }
#ifdef _AV_PLATFORM_HI3515_
		struct timeval stTime;
        gettimeofday(&stTime, NULL);
        pThParam->u64ThreadTime = (stTime.tv_sec * 1000000LLU) + stTime.tv_usec;
#else
        pThParam->u64ThreadTime = GetSysTimeByUsec();
#endif
        bSleep = true;
        for( u8Index = u8StartCh; u8Index < u8EndCh; u8Index++ )
        {
            if( this->ProcessDecoder( u8Index, m_pChlParam[u8Index] ) == 0 )
            {
                bSleep = false;
            }
            else
            {
                if(abs(pThParam->u64ThreadTime - TimeInterval) >= 200000)
                {
                    if(u8Index == (u8EndCh - 1))
                    {
                        TimeInterval = pThParam->u64ThreadTime;
                    }
                    CheckVlAndReset(m_pChlParam[u8Index]);
                }
            }
        }

        if( bSleep )
        {
            mSleep(10);
        }
    }

    pThParam->eThreadState = EPREVTD_EXIT;
        
    return;
}

void CAVPreviewDecCtrl::SwitchAudioChn( int index )
{
    m_u8AudioChn = index;

    return;
}

int CAVPreviewDecCtrl::ResetPreviewDecParam()
{
    if( m_eThreadReq != EPREVTD_RUNNING )
    {
        APD_PRINT("CAVPreviewDecCtrl::ResetPreviewDecParam m_eThreadReq=%d ignor cmd!!!\n", m_eThreadReq);
        return 0;
    }
    int i32MaxChNum = pgAvDeviceInfo->Adi_max_channel_number();

    for( int i = 0; i < _AV_VDEC_MAX_CHN_; i++ )
    {
        if( i >= m_u8ChlNum )
        {
            break;
        }

        if( m_u8HdvrChannelState[i] != HDVR_STATE_INLID )
        {
            APD_PRINT("CAVPreviewDecCtrl::ResetPreviewDecParam, but channel %d is lock, state=%d!!!\n", i, m_u8HdvrChannelState[i]);
            continue;
        }
        
        if( m_szChannelList[i] < 0 || m_szChannelList[i] >= i32MaxChNum || i >= (int)m_u32CurDecChlNum)
        {
            //APD_PRINT("CAVPreviewDecCtrl::ResetPreviewDecParam Channel preview channel index[%d]=%d stop it\n", i, m_szChannelList[i]);
            //m_pChlParam[i]->PlayCmd.PushCmd( (int)PREVDEC_CMD_DELTASK, NULL, 0 );
        }
        else
        {
            char cRealCh = m_szChannelMap[(int)(m_szChannelList[i])];  
            if( cRealCh < 0 || cRealCh > i32MaxChNum )
            {
                //APD_PRINT("CAVPreviewDecCtrl::ResetPreviewDecParam Channel priview index=%d realch=%d index=%d stop it\n", i, cRealCh, m_szChannelList[i]);
                m_pChlParam[i]->PlayCmd.PushCmd( (int)PREVDEC_CMD_DELTASK, NULL, 0 );
                continue;
            }
            AvPrevTask_t stuTask;
            stuTask.u32FrameRate = m_szFrameRateInit[ (int)m_szChannelList[i] ];
            stuTask.u8ChlIndex = i;
            stuTask.u8RealChl = cRealCh;
            stuTask.u8StreamType = m_u8CurStreamType;
            APD_PRINT("CAVPreviewDecCtrl::ResetPreviewDecParam Channel priview index=%d realch=%d index=%d stream=%d\n", i, cRealCh, m_szChannelList[i], stuTask.u8StreamType);
            
            m_pChlParam[i]->PlayCmd.PushCmd( (int)PREVDEC_CMD_ADDTASK, (char*)&stuTask, sizeof(stuTask) );
        }
    }

    return 0;
}

int CAVPreviewDecCtrl::ResetPreviewChnMap( char chnMap[SUPPORT_MAX_VIDEO_CHANNEL_NUM] )
{
#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_HDVR_)
    if( memcmp(m_szChannelMap, chnMap, SUPPORT_MAX_VIDEO_CHANNEL_NUM * sizeof(char)) == 0 )
    {
        APD_PRINT("CAVPreviewDecCtrl::ResetPreviewChnMap not parameter changed\n");
        return 0;
    }
#endif

    for( int ch = 0; ch < pgAvDeviceInfo->Adi_max_channel_number(); ch++ )
    {
        m_szChannelMap[ch] = chnMap[ch];
        APD_PRINT("CAVPreviewDecCtrl::ResetPreviewChnMap:: chn %d is %d!\n", ch, chnMap[ch]);
    }
   
    this->ResetPreviewDecParam();
    
    return 0;
}

int CAVPreviewDecCtrl::ResetPreviewOnlineState( unsigned char szOnlineState[SUPPORT_MAX_VIDEO_CHANNEL_NUM] )
{
    for(int ch = 0; ch < pgAvDeviceInfo->Adi_max_channel_number(); ch++)
    {
        m_szDevOnLineState[ch] = szOnlineState[ch];
    }

    return 0;
}

int CAVPreviewDecCtrl::ChangePreviewChannel( unsigned int u32DecChlNum, int chnlist[_AV_VDEC_MAX_CHN_]
    , char chnMap[SUPPORT_MAX_VIDEO_CHANNEL_NUM], unsigned char szOnlineState[SUPPORT_MAX_VIDEO_CHANNEL_NUM], unsigned char u8StreamType,int realvochn_layout[_AV_VDEC_MAX_CHN_])
{
    m_u32CurDecChlNum = u32DecChlNum;

    for( int i = 0; i < _AV_VDEC_MAX_CHN_; i++ )
    {
        m_szChannelList[i] = chnlist[i];
    }
    for( int i = 0; i < _AV_VDEC_MAX_CHN_; i++ )
    {
        szRealVoChlLay[i] = realvochn_layout[i];
    }	
    
    for( int ch = 0; ch < pgAvDeviceInfo->Adi_max_channel_number(); ch++ )
    {
        m_szChannelMap[ch] = chnMap[ch];
	 //DEBUG_MESSAGE("~~~~~~~~~~~~  chnMap[%d]:%d\n",ch,chnMap[ch]);	
        m_pChlParam[ch]->PlayCmd.ClearAllCmd();
        //m_szDevOnLineState[ch] = szOnlineState[ch];
    }

    m_u8CurStreamType = (u8StreamType ? 1 : 0);

    m_eThreadReq = EPREVTD_CONTINUE;
    for( int t = 0; t < m_u8ThreadNum; t++ )
    {
        while( m_pThreadParam[t]->eThreadState != EPREVTD_CONTINUE )
        {
            this->m_eThreadReq = EPREVTD_CONTINUE;
            mSleep(10);
        }
    }

    m_eThreadReq = EPREVTD_RUNNING;
    
    this->ResetPreviewDecParam();
    
    return 0;
}

int CAVPreviewDecCtrl::StopDecoder( bool bWait )
{
    APD_PRINT( "CAVPreviewDecCtrl::StopDecoder chNum=%d wait=%d\n", m_u8ChlNum, bWait );
    if( bWait )
    {
        this->m_eThreadReq = EPREVTD_PAUSE;
        
        for( int t = 0; t < m_u8ThreadNum; t++ )
        {
            while( m_pThreadParam[t]->eThreadState != EPREVTD_PAUSE )
            {
                this->m_eThreadReq = EPREVTD_PAUSE;
                mSleep(10);
            }
        }
        
        for( int i = 0; i < m_u8ChlNum; i++ )
        {
            this->ProcessDelTask( m_pChlParam[i] );
            m_pChlParam[i]->PlayCmd.ClearAllCmd();
        }
    }
    else
    {
        for( int i = 0; i < _AV_VDEC_MAX_CHN_; i++ )
        {
            if( i >= m_u8ChlNum )
            {
                break;
            }
            
            m_pChlParam[i]->PlayCmd.PushCmd( (int)PREVDEC_CMD_DELTASK, NULL, 0 );
        }
    }

    return 0;
}

int CAVPreviewDecCtrl::PauseDecoder()
{
    for( int i=0; i<_AV_VDEC_MAX_CHN_; i++ )
    {
        if( i >= m_u8ChlNum )
        {
            break;
        }
        
        m_pChlParam[i]->PlayCmd.PushCmd( (int)PREVDEC_CMD_DELTASK, NULL, 0 );
    }
    return 0;
}

int CAVPreviewDecCtrl::RestartDecoder()
{
    APD_PRINT( "CAVPreviewDecCtrl::RestartDecoder\n" );

    this->StopDecoder(false);

    this->ResetPreviewDecParam();
    
    return 0;
}

int CAVPreviewDecCtrl::MuteSound( bool bMute )
{
    //APD_PRINT("CAVPreviewDecCtrl::MuteSound nvr set mute, %d->%d\n", m_bAudioMute, bMute )
    m_bAudioMute = bMute;

    return 0;
}

int CAVPreviewDecCtrl::CheckSend( AvPrevChlDecParam_t* pChParam )
{
    unsigned long long u64ThreadTime = m_pThreadParam[pChParam->u8ThreadIndex]->u64ThreadTime;
    if( pChParam->u64BaseFramePts == 0 )
    {
        pChParam->u64BaseFramePts= pChParam->u64TmpPts;
        pChParam->u64LastFramePts = pChParam->u64TmpPts;
        pChParam->u64BaseSysTime = u64ThreadTime;
        pChParam->u32FailedCountTest = 0;
        pChParam->i32AjustSpan = 0;

        //DEBUG_MESSAGE("Check send reset pts index=%d realch=%d, time=%lld\n", pChParam->u8Index, pChParam->u8RealCh, u64ThreadTime);
        return 0;
    }

    int span = pChParam->u64TmpPts - pChParam->u64LastFramePts;
   // DEBUG_MESSAGE("span=%d tamp=%lld %lld\n", span, pChParam->u64TmpPts, pChParam->u64LastFramePts);
    if( span < 0 || span > 2000 )
    {
        DEBUG_ERROR("index %d PTS unexpect jumping span=%d\n", pChParam->u8Index, span );
        return -2;
    }
    
    //DEBUG_MESSAGE("%lld==%lld\n", (u64ThreadTime - pChParam->u64BaseSysTime)/1000, (pChParam->u64TmpPts - pChParam->u64BaseFramePts) );
    if( (u64ThreadTime - pChParam->u64BaseSysTime)/1000 < (pChParam->u64TmpPts - pChParam->u64BaseFramePts + pChParam->i32AjustSpan) )
    {
        if( (pChParam->u32FailedCountTest++) > 1000 )
        {
           // ########################index to much failed count=1002 threadtime=1371102963070678 timeBase=1371102950520789 ptsNow=1371108789192 basepts=1371108789159
            DEBUG_MESSAGE("########################index[%d] to much failed count=%d threadtime=%lld timeBase=%lld ptsNow=%lld basepts=%lld span=%d spanbase=%d\n"
                , pChParam->u8Index, pChParam->u32FailedCountTest, u64ThreadTime, pChParam->u64BaseSysTime
                , pChParam->u64TmpPts, pChParam->u64BaseFramePts, pChParam->i32AjustSpan, pChParam->i32AjustSpanBase );
            return -2;
        }
        return -1;
    }

    pChParam->u32FailedCountTest = 0;
    //DEBUG_MESSAGE("pts=%lld span=%d time=%lld base=%lld\n", pChParam->u64TmpPts, span, u64ThreadTime, pChParam->u64BaseFramePts);

    pChParam->u64LastFramePts = pChParam->u64TmpPts;

    return 0;
}

int CAVPreviewDecCtrl::SmoooothVideo( AvPrevChlDecParam_t* pChParam )
{
    //int left = (pChParam->pShareStream->GetLeftFrameCount()&0xffff);

    int left = N9M_SHGetLeftFrameCount(pChParam->ShareStreamHandle, 1);
   // DEBUG_MESSAGE("ch=%d left=%d tm=%lld\n", pChParam->u8RealCh, left, GetSysTimeByUsec());
    if( left < 0 )
    {
        return 0;
    }

    if( left < g_u8szAdjustFrameLeftMin[pChParam->u8FrameRate] )
    {
        pChParam->i32AjustSpan += pChParam->i32AjustSpanBase;
    }
    else if( left > g_u8szAdjustFrameLeftJump[pChParam->u8FrameRate] )
    {
        return -1;
    }
    else if( left > g_u8szAdjustFrameLeftMax[pChParam->u8FrameRate] )
    {
        pChParam->i32AjustSpan -= pChParam->i32AjustSpanBase;
    }

   // DEBUG_MESSAGE("index=%d left=%d ajust=%d adjust base=%d min=%d max=%d\n", pChParam->u8Index, left, pChParam->i32AjustSpan, pChParam->i32AjustSpanBase
   //     , g_u8szAdjustFrameLeftMin[pChParam->u8FrameRate], g_u8szAdjustFrameLeftMax[pChParam->u8FrameRate] );
    //static unsigned int printfalg = 0;
   // if( printfalg++ % 30 == 0 )
    //    DEBUG_MESSAGE("index[%d] realspan=%d aimspan=%d left=%d\n", pChParam->u8Index, pChParam->u32FrameSpanReal, pChParam->u32FrameSpanBase, left);
    
    return 0;
}
    
int CAVPreviewDecCtrl::SetFrameRate( int index, unsigned char u8FrameRate )
{
    return this->SetBaseFrameRate( index, u8FrameRate );
}

int CAVPreviewDecCtrl::SetFrameRateFromParam( unsigned char szFrameRate[SUPPORT_MAX_VIDEO_CHANNEL_NUM] )
{
    for( int ch = 0; ch < pgAvDeviceInfo->Adi_max_channel_number(); ch++ )
    {
        m_szFrameRateInit[ch] = szFrameRate[ch];
    }

    return 0;
}

int CAVPreviewDecCtrl::GetChannelIndex( int iRealChl )
{
    for( int i=0; i<m_u8ChlNum; i++ )
    {
        if( m_pChlParam[i]->u8RealCh == iRealChl )
        {
            return i;
        }
    }

    return -1;
}

int CAVPreviewDecCtrl::HdvrOperate( const HdvrOperate_t* pOperate )
{
    if( pOperate->u8Channel >= m_u8ChlNum )
    {
        return -1;
    }
    
    if( pOperate->u8Command == HDVR_CMD_CHANNEL_STOP )
    {
        m_u8HdvrChannelState[pOperate->u8Channel] = pOperate->u8Command;

        m_pChlParam[pOperate->u8Channel]->PlayCmd.PushCmd( (int)PREVDEC_CMD_DELTASK, NULL, 0 );
        while( m_pChlParam[pOperate->u8Channel]->u8HaveTask )
        {
            mSleep(10);
        }

        return 0;
    }
    else if( pOperate->u8Command == HDVR_CMD_CHANNEL_START )
    {
        char cRealCh = m_szChannelMap[(int)(m_szChannelList[pOperate->u8Channel])];  
        if( cRealCh < 0 || cRealCh > m_u8ChlNum )
        {
            m_pChlParam[pOperate->u8Channel]->PlayCmd.PushCmd( (int)PREVDEC_CMD_DELTASK, NULL, 0 );
            return 0;
        }
        
        AvPrevTask_t stuTask;
        stuTask.u32FrameRate = m_szFrameRateInit[ (int)m_szChannelList[pOperate->u8Channel] ];
        stuTask.u8ChlIndex = pOperate->u8Channel;
        stuTask.u8RealChl = pOperate->u8Channel;
        stuTask.u8StreamType = m_u8CurStreamType;

        m_pChlParam[pOperate->u8Channel]->PlayCmd.PushCmd( (int)PREVDEC_CMD_ADDTASK, (char*)&stuTask, sizeof(stuTask) );

        m_u8HdvrChannelState[pOperate->u8Channel] = HDVR_STATE_INLID;
    }
    else if( pOperate->u8Command == HDVR_CMD_CHANGE_STREAM )
    {
        m_u8CurStreamType = pOperate->stuSteamChangeParam.u8StreamType;

        if( m_u8HdvrChannelState[pOperate->u8Channel] == HDVR_STATE_INLID)
        {
            m_pChlParam[pOperate->u8Channel]->PlayCmd.PushCmd( (int)PREVDEC_CMD_DELTASK, NULL, 0 );
            
            char cRealCh = m_szChannelMap[(int)(m_szChannelList[pOperate->u8Channel])];  
            if( cRealCh < 0 || cRealCh > m_u8ChlNum )
            {
                return 0;
            }
            
            AvPrevTask_t stuTask;
            stuTask.u32FrameRate = m_szFrameRateInit[ (int)m_szChannelList[pOperate->u8Channel] ];
            stuTask.u8ChlIndex = pOperate->u8Channel;
            stuTask.u8RealChl = pOperate->u8Channel;
            stuTask.u8StreamType = m_u8CurStreamType;

            m_pChlParam[pOperate->u8Channel]->PlayCmd.PushCmd( (int)PREVDEC_CMD_ADDTASK, (char*)&stuTask, sizeof(stuTask) );

        }
    }
    
    return -1;
}
int CAVPreviewDecCtrl::SystemUpgrade()
{
    APD_PRINT("CAVPreviewDecCtrl::SystemUpgrade sytem upgrading, so stop decoder..\n");
    
    return this->StopDecoder(true);
}

int CAVPreviewDecCtrl::SystemUpgradeFailed()
{
    APD_PRINT("CAVPreviewDecCtrl::SystemUpgradeFailedsytem upgrade failed..\n");
    for( int ch = 0; ch < pgAvDeviceInfo->Adi_max_channel_number(); ch++ )
    {
        m_pChlParam[ch]->PlayCmd.ClearAllCmd();
    }

    m_eThreadReq = EPREVTD_CONTINUE;
    
    for( int t=0; t<m_u8ThreadNum; t++ )
    {
        while( m_pThreadParam[t]->eThreadState != EPREVTD_CONTINUE )
        {
            this->m_eThreadReq = EPREVTD_CONTINUE;
            mSleep(10);
        }
    }

    m_eThreadReq = EPREVTD_RUNNING;
    
    this->ResetPreviewDecParam();
    
    return 0;
}


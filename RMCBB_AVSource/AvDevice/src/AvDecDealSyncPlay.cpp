
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
#include <hi_math.h>

#include "AvDecDealSyncPlay.h"
#include "Message.h"
#include "AvDeviceInfo.h"
using namespace Common;

#define MAX_VO_WAIT_FR 20
#if 0
#define DDSP_PRINT(fmt, args...)    do {printf("[%s, %d]: "fmt"\n", __FUNCTION__, __LINE__, ##args);}while(0);
#else
#define DDSP_PRINT(fmt, args...) do{}while(0);
#endif
#define DDSP_ERROR(fmt, args...)    do {printf("[%s, %d]: "fmt"\n", __FUNCTION__, __LINE__, ##args);}while(0);

inline unsigned long long cal_llabs(unsigned long long first, unsigned long long second)
{
	if(first >= second)
	{
		return(first - second);
	}
	else
	{
		return(second - first);
	}
}

void *DecodeEntry(void *para)
{
    DecThreadPara_t *pPara = (DecThreadPara_t *)para;
    CDecDealSyncPlay *pDdspOperate = (CDecDealSyncPlay *)pPara->DecClass;

    DEBUG_MESSAGE("\nDecodeEntry Chn %d-%d thread pid:%d\n\n", pPara->StartChnId, pPara->StopChnId,getpid());
    pPara->Pid = getpid();

	char threadName[32];
	sprintf(threadName, "DecodeEntry[%d]", pPara->ThreadIndex);
	prctl(PR_SET_NAME, threadName);
    pDdspOperate->SignalDecDealBody(pPara->ThreadIndex, pPara->StartChnId, pPara->StopChnId);

    return NULL;
}

void *DecMonitorEntry(void *para)
{
    DecThreadPara_t *pPara = (DecThreadPara_t *)para;
    CDecDealSyncPlay *pDdspOperate = (CDecDealSyncPlay *)pPara->DecClass;

    pPara->Pid = getpid();
	
	prctl(PR_SET_NAME, "DecMonitorEntry");
    pDdspOperate->DecMonitorBody();

    return NULL;
}

CDecDealSyncPlay::CDecDealSyncPlay(int purpose, int ChnNum, int ChnNumPerThread, DDSP_Operation_t *pOperation )
{
    DEBUG_MESSAGE("\n\n=======DDSP ChnNum %d prethread %d Time:%s %s=========\n\n", ChnNum, ChnNumPerThread, __DATE__, __TIME__);
    m_AvPurpose = purpose;

    if (pOperation)
    {
        m_DDSPOperate = *pOperation;
    }
    else
    {
        m_DDSPOperate.GetDecWaitFrCnt = NULL;
        m_DDSPOperate.SendDataToDecEx = NULL;
    }

    if(ChnNumPerThread <= 0)
    {
        ChnNumPerThread = 1;
    }

    m_ThreadNum = ChnNum / ChnNumPerThread;
    if ((ChnNum % ChnNumPerThread) > 0)
    {
        m_ThreadNum += 1;
    }

    m_ChnNum = ChnNum;

    m_DecChnMask = 0;
    m_DecDataChnMask = 0;
    m_SyncPtsBmp = 0;
    m_ExpectAudioChn = -1;
    m_AudioChn = -1;
    m_GetBasePtsDirect = 1;
    m_NeedClearAudio = false;
    for (int index = 0; index < ChnNum; index++)
    {
        m_PlayFrameRate[index] = 30;
        m_BaseFrameRate[index] = 30;
        m_DecThreadStatus[index].DecThreadRun = false;
        m_DecThreadStatus[index].DecThreadExit = true;
        m_DecThreadStatus[index].DecThreadIdle = false;
        m_DecThreadStatus[index].DecThreadFlag = 0;
        GCL_BIT_VAL_SET(m_DecChnMask, index);
        GCL_BIT_VAL_SET(m_DecDataChnMask, index);

        memset(&m_DecSendPara[index], 0, sizeof(DecSendPara_t));
        m_DecSendPara[index].CurPlayState = PS_NORMAL;
        m_DecSendPara[index].ExpectPlayState = PS_NORMAL;
        m_DecSendPara[index].ReSyncPts = false;
        m_DecSendPara[index].RedundantFrCnt = 0;
        m_DecSendPara[index].BaseFrameRtcTime = 0;
        m_DecSendPara[index].BaseChnPts = 0;

        m_CheckValid[index] = false;
        m_DecSendPara[index].RequestIFrame = true;

        m_ThreadPara[index].StartChnId = index;
        m_ThreadPara[index].StopChnId = index+1;
        m_ThreadPara[index].DecClass = this;

        ClearPtsByChn(index);
    }

    for (int index = 0; index < m_ThreadNum; index++)
    {
        m_ThreadPara[index].StartChnId = index*ChnNumPerThread;
        m_ThreadPara[index].StopChnId = (index+1)*ChnNumPerThread;

        if (m_ThreadPara[index].StopChnId > ChnNum)
        {
            m_ThreadPara[index].StopChnId = ChnNum;
        }
        m_ThreadPara[index].ThreadIndex = index;
        m_ThreadPara[index].DecClass = this;
        Av_create_normal_thread(DecodeEntry, (void*)(&m_ThreadPara[index]), NULL);
        while (m_DecThreadStatus[index].DecThreadRun == false)
        {
            mSleep(100 );
            DEBUG_ERROR("wait dec thread index %d start\n", index);
        }
    }

    m_MonitorThreadStatus.DecThreadRun = false;
    m_MonitorThreadStatus.DecThreadExit = true;
    m_MonitorThreadStatus.DecThreadIdle = false;
    m_MonitorThreadStatus.DecThreadFlag = 0;

    m_MonitorThreadPara.StartChnId = 0;
    m_MonitorThreadPara.StopChnId = 0;
    m_MonitorThreadPara.ThreadIndex = 0xff;
    m_MonitorThreadPara.DecClass = this;

    Av_create_normal_thread(DecMonitorEntry, (void*)(&m_MonitorThreadPara), NULL);
    while (m_MonitorThreadStatus.DecThreadRun == false)
    {
        mSleep(100 );
        DEBUG_ERROR("wait dec thread monitor start\n");
    }

    m_resource_limited_informed = false;
}

CDecDealSyncPlay::~CDecDealSyncPlay()
{
    m_MonitorThreadStatus.DecThreadRun = false;
    WaitToSepcialPlay(m_DecChnMask, true);    
    for (int index = 0; index < m_ThreadNum; index++)
    {
        m_DecThreadStatus[index].DecThreadRun = false;
    }

    /*Exit Monitor Thread*/
    while(!m_MonitorThreadStatus.DecThreadExit)
    {
        if (m_MonitorThreadStatus.DecThreadRun)
        {
            m_MonitorThreadStatus.DecThreadRun = false;
        }
        mSleep(10);
    }

    /*Exit Play control Thread*/
    for (int index = 0; index < m_ThreadNum; index++)
    {
        while(!m_DecThreadStatus[index].DecThreadExit)
        {
            m_DecThreadStatus[index].DecThreadRun = false;
            mSleep(10);
            DEBUG_MESSAGE("wait decocer thread %d exit\n", index);
        }
    }

    DEBUG_MESSAGE("##########wait decocer thread exit over\n");
}

int CDecDealSyncPlay::WaitIdle(bool idle, int index)
{
    if(idle)
    {
        m_DecThreadStatus[index].DecThreadFlag= 0;
        m_DecThreadStatus[index].DecThreadIdle = true;
        while(m_DecThreadStatus[index].DecThreadFlag == 0)
        {
            if(!m_DecThreadStatus[index].DecThreadRun)
            {
                break;
            }
            mSleep(10);
	     m_DecThreadStatus[index].DecThreadIdle = true;		
        }
    }
    else
    {
        m_DecThreadStatus[index].DecThreadIdle = false;
        m_DecThreadStatus[index].DecThreadFlag = 0;
    }

    return 0;
}

void CDecDealSyncPlay::WaitToSepcialPlay(unsigned int ChnMask, bool wait)
{
    if(wait)
    {
        m_MonitorThreadStatus.DecThreadFlag = 0;
        m_MonitorThreadStatus.DecThreadIdle = true;      
        while(m_MonitorThreadStatus.DecThreadFlag == 0)
        {
            if(!m_MonitorThreadStatus.DecThreadRun)
            {
                break;
            }
            mSleep(10);
        }
        for (int index = 0; index < m_ThreadNum; index++)
        {
            m_DecThreadStatus[index].DecThreadFlag= 0;
            m_DecThreadStatus[index].DecThreadIdle = true;
        }
        for (int index = 0; index < m_ThreadNum; index++)
        {
            while(m_DecThreadStatus[index].DecThreadFlag == 0)
            {
                if(!m_DecThreadStatus[index].DecThreadRun)
                {
                    break;
                }
                mSleep(10);
            }
        }
    }
    else
    {
        for (int index = 0; index < m_ThreadNum; index++)
        {
            m_DecThreadStatus[index].DecThreadIdle = false;
            m_DecThreadStatus[index].DecThreadFlag = 0;
        }
        m_MonitorThreadStatus.DecThreadIdle = false;
        m_MonitorThreadStatus.DecThreadFlag = 0;
    }
}

void CDecDealSyncPlay::DecSepcialPlayOperate(unsigned int ChnMask, PlayOperate_t ope)
{
    for (int index = 0; index < m_ChnNum; index++)
    {
        if (!(GCL_BIT_VAL_TEST(ChnMask, index)) )
        {
            //continue;
        }

        switch(ope)
        {
            case PO_PLAY:/**/
            {
                m_DecSendPara[index].ExpectPlayState = PS_NORMAL;
                break;
            }
            case PO_PAUSE:/*停*/
            {
                m_DecSendPara[index].ExpectPlayState = PS_PAUSE;;
                break;
            }
            case PO_STEP:/*帧*/
            {
                m_DecSendPara[index].ExpectPlayState = PS_PRESTEPPLAY;
                break;
            }
            case PO_SLOWPLAY:/**/
            {
                if (PS_SLOWPLAY2X == m_DecSendPara[index].CurPlayState)
                {
                    m_DecSendPara[index].ExpectPlayState = PS_SLOWPLAY4X;
                }
                else if (PS_SLOWPLAY4X == m_DecSendPara[index].CurPlayState)
                {
                    m_DecSendPara[index].ExpectPlayState = PS_SLOWPLAY8X;
                }
                else if (PS_SLOWPLAY8X == m_DecSendPara[index].CurPlayState)
                {
                    m_DecSendPara[index].ExpectPlayState = PS_SLOWPLAY16X;
                }
                else 
                {
                    m_DecSendPara[index].ExpectPlayState = PS_SLOWPLAY2X;
                }
                break;
            }
            case PO_FASTFORWARD:/**/
            {
                if (PS_FASTFORWARD2X == m_DecSendPara[index].CurPlayState)
                {
                    m_DecSendPara[index].ExpectPlayState = PS_FASTFORWARD4X;
                }
                else if (PS_FASTFORWARD4X == m_DecSendPara[index].CurPlayState)
                {
                    m_DecSendPara[index].ExpectPlayState = PS_FASTFORWARD8X;
                }
                else if (PS_FASTFORWARD8X == m_DecSendPara[index].CurPlayState)
                {
                    m_DecSendPara[index].ExpectPlayState = PS_FASTFORWARD16X;
                }
                else if (PS_FASTFORWARD16X == m_DecSendPara[index].CurPlayState)
                {
                    m_DecSendPara[index].ExpectPlayState = PS_FASTFORWARD2X;
                }
                else
                {
                    m_DecSendPara[index].ExpectPlayState = PS_FASTFORWARD2X;
                }
                break;
            }
            case PO_FASTBACKWARD:/**/
            {
                if (PS_FASTBACKWARD2X == m_DecSendPara[index].CurPlayState)
                {
                    m_DecSendPara[index].ExpectPlayState = PS_FASTBACKWARD4X;
                }
                else if (PS_FASTBACKWARD4X == m_DecSendPara[index].CurPlayState)
                {
                    m_DecSendPara[index].ExpectPlayState = PS_FASTBACKWARD8X;
                }
                else if (PS_FASTBACKWARD8X == m_DecSendPara[index].CurPlayState)
                {
                    m_DecSendPara[index].ExpectPlayState = PS_FASTBACKWARD16X;
                }
                else if (PS_FASTBACKWARD16X == m_DecSendPara[index].CurPlayState)
                {
                    m_DecSendPara[index].ExpectPlayState = PS_FASTBACKWARD2X;
                }
                else
                {
                    m_DecSendPara[index].ExpectPlayState = PS_FASTBACKWARD2X;
                }
                break;
            }
            case PO_SEEK:
            {
                m_DecSendPara[index].ExpectPlayState = PS_SEEK;
                break;
            }
            default:
                break;
        }
    }

    if (PO_FASTBACKWARD == ope)
    {
        m_GetBasePtsDirect = -1;
    }
    else
    {
        m_GetBasePtsDirect = 1;
    }
}


void CDecDealSyncPlay::SignalDecDealBody(int ThreadIndex, int startindex, int stopindex)
{
    int ChnId = -1;
    unsigned char *pData = NULL;
    int DataLen = 0;
    int Ret = 0;
    int direction = 0;
    int retval = 0;
    bool SendFlag = true;
    bool InitThread[MAX_DEC_DEAL_CHN_NUM];
    int NoDataError[MAX_DEC_DEAL_CHN_NUM];
    int NoSendTimes[MAX_DEC_DEAL_CHN_NUM];
    int AudioFrCnt[MAX_DEC_DEAL_CHN_NUM];
    unsigned long long CurPts = 0;
    int index = 0;
    unsigned long long CurSysTime = 0;
    unsigned int CurChnMask = 0;
    unsigned int SleepChnMask = 0;
    unsigned int ChnNoDataMask = 0;
//    bool PrintFlag = false;
    int FrameNum = 0, datalen = 0;
    PlaybackFrameinfo_t stuFrameInfo;

    m_DecThreadStatus[ThreadIndex].DecThreadRun = true;
    m_DecThreadStatus[ThreadIndex].DecThreadExit = false;

    DEBUG_MESSAGE("ThreadIndex %d, chn (%d-%d\n)\n", ThreadIndex, startindex, stopindex);
    for (index = startindex; index < stopindex; index++)
    {
        GCL_BIT_VAL_SET(CurChnMask, index);
        InitThread[index] = true;
        NoDataError[index] = 0;
        AudioFrCnt[index] = 0;
        NoSendTimes[index] = 0;
    }
    //start run
    while(m_DecThreadStatus[ThreadIndex].DecThreadRun)
    {
        bool sleepflag = true;

        for (index = startindex; index < stopindex; index++)
        {
            CheckPlayState(index);
        }
        // move it at start of while loop?
        if(m_DecThreadStatus[ThreadIndex].DecThreadIdle)
        {
            m_DecThreadStatus[ThreadIndex].DecThreadFlag = 1;
            //DEBUG_ERROR("ThreadIndex %d AVDecode idle...RunStatus %d\n", ThreadIndex, m_DecThreadStatus[ThreadIndex].DecThreadRun);
            mSleep(20);
            continue;
        }

        CurSysTime = Get_system_ustime();

        SleepChnMask = 0;
        ChnNoDataMask = 0;

//        PrintFlag = false;
        for (index = startindex; index < stopindex; index++)
        {
            m_DecSendPara[index].CurSysTime = CurSysTime;
            retval = 0;
            SendFlag = true;

            direction = 1;
            if ((m_DecSendPara[index].CurPlayState >= PS_FASTBACKWARD2X) && (m_DecSendPara[index].CurPlayState <= PS_FASTBACKWARD16X))
            {
                direction = -1;
            }
            //check whether current chn has data!! 
            if (m_DDSPOperate.CheckIndexStatus(index) == 0)
            {
                //SetDataChnMask(index, false);
                //SetSyncPtsBmp(index, false);
                GCL_BIT_VAL_SET(SleepChnMask, index);
                continue;
            }

            FrameNum = 0;
            datalen = 0;
            if( false == m_CheckValid[index] )
            {
                //DEBUG_ERROR("Chn %d invalid, m_SyncPtsBmp %x, m_DecDataChnMask %x\n", index, m_SyncPtsBmp, m_DecDataChnMask);
                GCL_BIT_VAL_SET(SleepChnMask, index);
                //mSleep(10);
                continue;
            }
            else if (PS_PAUSE == m_DecSendPara[index].CurPlayState)
            {
                //get un-decoded frame numbers
                m_DDSPOperate.GetDecWaitFrCnt(m_AvPurpose, index, &FrameNum, &datalen, DECDATA_VIDEO);    
                if (FrameNum > 0)
                {
                    GCL_BIT_VAL_SET(SleepChnMask, index);
                    DEBUG_MESSAGE("Chn %d pause\n", index);
                    //mSleep(10);
                    continue;
                }
                else
                {
                    DEBUG_ERROR("Chn %d Send Video\n", index);
                }
            }
            else if ((PS_STEP == m_DecSendPara[index].CurPlayState) && (false == m_DecSendPara[index].StepPlay))
            {
                GCL_BIT_VAL_SET(SleepChnMask, index);
                DEBUG_MESSAGE("Chn %d Step\n", index);
                //mSleep(10);
                continue;
            }
            //get data from sharemem 
            Ret = m_DDSPOperate.GetFrameBufferInfo( index, &pData, &DataLen, &stuFrameInfo, direction );

//             if(( DECDATA_IFRAME == stuFrameInfo.u8FrameType )&&(index==0)) 
//             DEBUG_ERROR("index%d Get frame info, ret=%d, pts=%lld ch=%d type=%d flag=%d\n"
//             , index , Ret, stuFrameInfo.ullPts, stuFrameInfo.u8Channel, stuFrameInfo.u8FrameType, stuFrameInfo.u8Flag);
            if ((Ret < 0) || (NULL == pData) || (DataLen < 0))
            {
                GCL_BIT_VAL_SET(SleepChnMask, index);
                GCL_BIT_VAL_SET(ChnNoDataMask, index);
                NoDataError[index]++;
                if ((NoDataError[index]%5) == 0)
                {
                    DEBUG_ERROR("index %d ChnId %d no data Times %d\n", index, ChnId, NoDataError[index]);
                }
                continue;
            }

            NoDataError[index] = 0;

            if (stuFrameInfo.u8Flag & RECORD_SEGMENT_BEGIN)
            {
                // DEBUG_MESSAGE("aa New Segmet Chn %d SetSyncPtsBmp=====bb, InitThread %d\n", index, InitThread[index]);
                //DEBUG_MESSAGE("aa New Segmet Chn %d SetSyncPtsBmp=====bb, InitThread %d\n", index, InitThread[index]);
                if (false == InitThread[index])
                {
                    //DEBUG_MESSAGE("m_SyncPtsBmp=0x%x\n",m_SyncPtsBmp);
                    if (!GCL_BIT_VAL_TEST(m_SyncPtsBmp, index))
                    {
                        //DEBUG_ERROR("New Segmet Chn %d SetSyncPtsBmp=====1\n", index);
                        SetSyncPtsBmp(index, true);
                    }
                }
                else
                {
                    // DEBUG_MESSAGE("m_SyncPtsBmp=0x%x check=%d\n", m_SyncPtsBmp, (!GCL_BIT_VAL_TEST(m_SyncPtsBmp, index)) );
                    if ((m_SyncPtsBmp != 0) && (!GCL_BIT_VAL_TEST(m_SyncPtsBmp, index)))
                    {
                        //DEBUG_ERROR("New Segmet Chn %d SetSyncPtsBmp=====2\n", index);
                        SetSyncPtsBmp(index, true);
                    }
                }
            }

            SendFlag = true;
            if ((m_DecSendPara[index].CurPlayState >= PS_FASTFORWARD2X) && (m_DecSendPara[index].CurPlayState <= PS_FASTBACKWARD16X)
                && (stuFrameInfo.u8FrameType != DECDATA_IFRAME))
            {
                SendFlag = false;
            }

            if (true == m_DecSendPara[index].RequestIFrame)
            {
                SendFlag = false;
                if (DECDATA_IFRAME == stuFrameInfo.u8FrameType)
                {
                    SendFlag = true;
                    m_DecSendPara[index].RequestIFrame = false;
                }
            }

            if ((m_DecSendPara[index].CurPlayState != PS_INIT) && (m_DecSendPara[index].CurPlayState != PS_NORMAL)
                && (DECDATA_AFRAME == stuFrameInfo.u8FrameType))
            {
                SendFlag = false;
            }
#if defined(_AV_PRODUCT_CLASS_DVR_REI_)
		if((pgAvDeviceInfo->Aa_get_screenmode()!=0)&& (DECDATA_AFRAME == stuFrameInfo.u8FrameType))
		{
                 SendFlag = false;
		  		//printf("\nscreenmode=%d\n",pgAvDeviceInfo->Aa_get_screenmode());
		}
#endif		
            if( DECDATA_AFRAME == stuFrameInfo.u8FrameType )
            {
                if( index != m_AudioChn || m_DecSendPara[index].CurPlayState != PS_NORMAL 
                || m_BaseFrameRate[index] != m_PlayFrameRate[index] )
                {
                    SendFlag = false;
                    if( m_NeedClearAudio )
                    {
                        m_NeedClearAudio = false;
                        if( m_AudioChn > 0 )
                        {
                            m_DDSPOperate.ClearAoBuf(m_AudioChn);
                        }
                    }
                }

                if( m_BaseFrameRate[index] != m_PlayFrameRate[index] )
                {
                    SendFlag = false;
                }
            }
            
            if ((stuFrameInfo.u8FrameType != DECDATA_AFRAME) && (true == SendFlag))
            {
                CurPts = stuFrameInfo.ullPts * 1000ULL; //stuFrameInfo.pVTime->llVirtualTime*1000ULL;//pVirtualTime->llVirtualTime*1000ULL;
                retval = CheckSend(index, CurPts);
                if (NoSendTimes[index] > 5)
                {
                    DEBUG_DEBUG("index=%d check pts=%lld typeType=%d ret=%d\n", index, stuFrameInfo.ullPts, stuFrameInfo.u8FrameType, retval);
                    DEBUG_DEBUG("retval %d CheckSend index %d Cur Pts %llu, BasePts %llu, %02d:%02d:%02d\n", 
                    retval, index,CurPts,
                    m_DecSendPara[index].BaseFramePts, 
                    stuFrameInfo.hour, 
                    stuFrameInfo.minute, 
                    stuFrameInfo.second);
                }
                //if ((index == 2)&& (retval == 0))
                if (retval == 0)
                {
                    if (DECDATA_IFRAME == stuFrameInfo.u8FrameType)
                    {
                        DEBUG_DEBUG("retval %d CheckSend index %d Cur Pts %llu, BasePts %llu, %02d:%02d:%02d\n", retval, index,CurPts,
                            m_DecSendPara[index].BaseFramePts, stuFrameInfo.hour, stuFrameInfo.minute, stuFrameInfo.second);
                    }
                    else
                    {
                        DEBUG_DEBUG("retval %d CheckSend index %d Cur Pts %llu, BasePts %llu\n", retval, index,CurPts, m_DecSendPara[index].BaseFramePts);
                    }
                    DEBUG_DEBUG("index %d, retval %d ChnId %d, CurPts %llu, BasePts %llu\n", index, retval, ChnId, CurPts, m_DecSendPara[index].BaseFramePts);
                }
            }
            
            if ((true == SendFlag) && (0 == retval))
            {
                //!informate UI
                #if defined(_AV_PLATFORM_HI3521_) || defined(_AV_PLATFORM_HI3520D_)
                if ((m_ChnNum > 1)&&(stuFrameInfo.u8FrameType !=DECDATA_AFRAME))
                {
                    if(false == m_resource_limited_informed)
                    {
                        MSSP_GetFrameInfo_t stuInfo;
                        int ret = m_DDSPOperate.GetFrameInfo( index, &stuInfo);
                        if (ret == 0)
                        {
                            DEBUG_WARNING("chn num:%d, height:%d, width:%d, informed:%d\n", m_ChnNum,stuInfo.u16Height, stuInfo.u16Width, m_resource_limited_informed);
                            if ((stuInfo.u16Width >=1920) &&(stuInfo.u16Height >= 1080))
                            {
                                //! send message to informate the resource is limited
                                    HANDLE client_handle = NULL;
                                    client_handle = N9M_MSGCreateMessageClient(100);
                                    if( client_handle != NULL)
                                    {
                                        const char * content = "DIALOG_AV_PLAYBACK_RESOURCE_LIMIT";
                                        int conten_size = sizeof("DIALOG_AV_PLAYBACK_RESOURCE_LIMIT");
                                        int msg_len = sizeof(msgDialogData_t) + conten_size;
                                        msgDialogData_t * dialog_data = (msgDialogData_t *)new char[msg_len] ;
                                        dialog_data->uctime = 0;
                                        dialog_data->uctype = 1;
                                        dialog_data->ucsize = conten_size;
                                        memcpy(dialog_data->body, content, conten_size);
                                        N9M_MSGSendMessage(client_handle,0,MESSAGE_GUI_RECEIVE_DIALOG_DATA_SHOW,0,(const char *)dialog_data,msg_len,0);
                                        N9M_MSGDestroyMessageClient(client_handle);

                                        DEBUG_WARNING("resource limited information message sent !\n");

                                        m_resource_limited_informed = true;
                                    }
                                        
                            }
                        }
                    }
                }
                #endif
                retval = m_DDSPOperate.SendDataToDecEx(m_AvPurpose, index, pData, DataLen, &stuFrameInfo);
                if (retval == 0)
                {
                    if (stuFrameInfo.u8FrameType != DECDATA_AFRAME)
                    {
                        m_DecSendPara[index].FrameNum++;
                        InitThread[index] = false;
                        if (true == m_DecSendPara[index].StepPlay)
                        {
                            DEBUG_ERROR("index %d Step Play\n", index);
                            //m_DDSPOperate.StepPlayVideo(m_AvPurpose, index);
                            m_DecSendPara[index].StepPlay = false;
                        }
                    }
                    else
                    {
                        AudioFrCnt[index]++;
                    }
                }
                else
                {
                    if ((stuFrameInfo.u8FrameType == DECDATA_AFRAME) && (m_AudioChn >= 0))
                    {
                        m_DDSPOperate.ClearAoBuf(m_AudioChn);
                        DEBUG_ERROR("Clear Ao Buf.........m_AudioChn %d\n", m_AudioChn);

                        retval = 0;
                    }
                    
                    //DEBUG_MESSAGE("=================================================index=%d get wait count\n", index);
                    m_DDSPOperate.GetDecWaitFrCnt(m_AvPurpose, index, &FrameNum, &datalen, stuFrameInfo.u8FrameType);
                    DEBUG_ERROR("index %d DataType %d FrameNum %d Send Data To Decord Failed\n", index, stuFrameInfo.u8FrameType, FrameNum);
                }
            }
            else if ((true == SendFlag) && (retval < 0))
            {
                retval = -1;
            }

            if ((retval >= 0) || (false == SendFlag))
            {
                if (stuFrameInfo.u8FrameType == DECDATA_IFRAME)
                {
                    DEBUG_DEBUG("index %d Release Pts %llu, %02d:%02d:%02d\n", index,CurPts, stuFrameInfo.hour, stuFrameInfo.minute, stuFrameInfo.second);
                }

                m_DDSPOperate.ReleaseFrameData(index);
                sleepflag = false;
            }

        }

        if (true == sleepflag)
        {
            mSleep(30);
        }
        else if (SleepChnMask == CurChnMask)
        {
            mSleep(40);
        }
        // DEBUG_MESSAGE("Sleep mas=%x chMask=%x\n", SleepChnMask, CurChnMask );
    }

    m_DecThreadStatus[ThreadIndex].DecThreadExit = true;
    DEBUG_MESSAGE("ThreadIndex %d, chn (%d-%d\n) Exit\n\n", ThreadIndex, startindex, stopindex);
    return;
}

void CDecDealSyncPlay::DecMonitorBody()
{
    unsigned int CurSysTime = 0;
    //  unsigned int preSecCheck = 0;
    unsigned int preSecSyncAll = 0;
    //unsigned int Interval = 0;
    bool bInit = false;

    m_MonitorThreadStatus.DecThreadRun = true;
    m_MonitorThreadStatus.DecThreadExit = false;
    DEBUG_MESSAGE("Start Dec Monitor Thread Body.........\n");
    while(m_MonitorThreadStatus.DecThreadRun)
    {
        if(m_MonitorThreadStatus.DecThreadIdle)
        {
            m_MonitorThreadStatus.DecThreadFlag = 1;
            //DEBUG_ERROR("Dec Monitor Body idle...RunStatus %d\n", m_MonitorThreadStatus.DecThreadRun);
            mSleep(20);
            continue;
        }

        CurSysTime = N9M_TMGetRtcSecond();//Get_system_ustime();

        if (bInit == false)
        {
            bInit = true;
            //preSecCheck = CurSysTime;
            preSecSyncAll = CurSysTime;
            SyncPts(m_DecChnMask, GETBASEPTS_FRAME_DISCARD);
        }
        else
        {
            //if( ABS(CurSysTime -preSecCheck) > 0 )//speed up decoding
            {
                //preSecCheck = CurSysTime;
                SyncPts(m_DecChnMask, GETBASEPTS_FRAME_DISCARD);
            }

            if( cal_llabs(CurSysTime, preSecSyncAll) > 3 )
            {
                preSecSyncAll = CurSysTime;
                ReSyncAllChn();
            }
        }
        mSleep(50);
    }
    m_MonitorThreadStatus.DecThreadExit = true;
    DEBUG_MESSAGE("Exit Dec Monitor Thread Body.........\n");
    return;
}

bool CDecDealSyncPlay::SetFrameRate(unsigned int ChnMask, int PlayFrameRate, int BaseFrameRate,bool firstset)
{
    if(firstset != true)
    {
    	WaitToSepcialPlay(ChnMask, true);
    }
    DEBUG_ERROR("PlayFrameRate %d, BaseFrameRate %d\n", PlayFrameRate, BaseFrameRate);
    for (int index = 0; index < m_ChnNum; index++)
    {
        if (!(GCL_BIT_VAL_TEST(ChnMask, index)) )
        {
            continue;
        }

        m_BaseFrameRate[index] = BaseFrameRate;

        m_PlayFrameRate[index] = PlayFrameRate;
        if (PlayFrameRate > BaseFrameRate)
        {
            m_PlayFrameRate[index] = BaseFrameRate;
        }
	 if(firstset != true)
    	 {	
	        ClearPtsByChn(index);
	        m_DecSendPara[index].RequestIFrame = true;
	  }
        if( m_BaseFrameRate[index] != m_PlayFrameRate[index] )
        {
            m_NeedClearAudio = true;
        }
    }
    if(firstset != true)
    {	
    	WaitToSepcialPlay(ChnMask, false);
    }
    return true;
}

int CDecDealSyncPlay::RePlay(int index)
{
    bool SyncPts = false;
    bool ResetPts = false;
    bool SetIFrame = false;
    DecSendPara_t *pOperate = &m_DecSendPara[index];

    switch(pOperate->CurPlayState)
    {
        case PS_PAUSE:
           ResetPts = true;
           break;
        case PS_STEP:
           ResetPts = true;
           pOperate->StepPlay = false;
           break;
        case PS_SLOWPLAY2X:
        case PS_SLOWPLAY4X:
        case PS_SLOWPLAY8X:
        case PS_SLOWPLAY16X:
            m_CheckValid[index] = false;
            pOperate->ReSyncPts = true;
            pOperate->RedundantFrCnt = 0;
            break;
        case PS_FASTFORWARD2X:
        case PS_FASTFORWARD4X:
        case PS_FASTFORWARD8X:
        case PS_FASTFORWARD16X:
        case PS_FASTBACKWARD2X:
        case PS_FASTBACKWARD4X:
        case PS_FASTBACKWARD8X:
        case PS_FASTBACKWARD16X:
            SyncPts = true;
            SetIFrame = true;
            break;
        default:
            return -1;
            break;
    }
    if (true == SyncPts)
    {
        ClearPtsByChn(index);
    }
    if (true == ResetPts)
    {
        ResetPtsByChn(index);
    }
    pOperate->RequestIFrame = SetIFrame;
    if ((PS_PAUSE ==  pOperate->CurPlayState) || (PS_STEP ==  pOperate->CurPlayState))
    {
        m_DDSPOperate.RePlayVideo(m_AvPurpose, index);
    }
    return 0;
}

int CDecDealSyncPlay::Pause(int index)
{
    m_DDSPOperate.PauseVideo(m_AvPurpose, index);
    return 0;
}

int CDecDealSyncPlay::FastForwardPlay(int index)
{
    bool SyncPts = true;
    bool SetIFrame = true;

    if (true == SyncPts)
    {
        ClearPtsByChn(index);
    }
    m_DecSendPara[index].RequestIFrame = SetIFrame;

    m_DDSPOperate.RePlayVideo(m_AvPurpose, index);
    return 0;
} 

int CDecDealSyncPlay::FastBackwardPlay(int index)
{
    bool SyncPts = true;
    bool SetIFrame = true;

    if (true == SyncPts)
    {
        ClearPtsByChn(index);
    }
    m_DecSendPara[index].RequestIFrame = SetIFrame;

    m_DDSPOperate.RePlayVideo(m_AvPurpose, index);
    return 0;
} 

int CDecDealSyncPlay::SlowPlay(int index)
{
    bool SyncPts = false;
    bool ResetPts = false;
    bool SetIFrame = false;

    switch(m_DecSendPara[index].CurPlayState)
    {
        case PS_NORMAL:
        case PS_PAUSE:
        case PS_STEP:
        case PS_SLOWPLAY2X:
        case PS_SLOWPLAY4X:
        case PS_SLOWPLAY8X:
        case PS_SLOWPLAY16X:
            m_CheckValid[index] = false;
            m_DecSendPara[index].ReSyncPts = true;
            m_DecSendPara[index].RedundantFrCnt = 0;
            break;
        case PS_FASTFORWARD2X:
        case PS_FASTFORWARD4X:
        case PS_FASTFORWARD8X:
        case PS_FASTFORWARD16X:
        case PS_FASTBACKWARD2X:
        case PS_FASTBACKWARD4X:
        case PS_FASTBACKWARD8X:
        case PS_FASTBACKWARD16X:
            SyncPts = true;
            SetIFrame = true;
            break;
        default:
            return -1;
            break;
    }
    if (true == SyncPts)
    {
        ClearPtsByChn(index);
    }
    if (true == ResetPts)
    {
        ResetPtsByChn(index);
    }
    m_DecSendPara[index].RequestIFrame = SetIFrame;
    m_DDSPOperate.RePlayVideo(m_AvPurpose, index);
    return 0;
} 

int CDecDealSyncPlay::StepPlay(int index)
{
    bool SyncPts = false;
    bool SetIFrame = false;

    switch(m_DecSendPara[index].CurPlayState)
    {
        case PS_NORMAL:
        case PS_PAUSE:
        case PS_STEP:
            SyncPts = false;
            SetIFrame = false;
            break;
        case PS_SLOWPLAY2X:
        case PS_SLOWPLAY4X:
        case PS_SLOWPLAY8X:
        case PS_SLOWPLAY16X:
            m_CheckValid[index] = false;
            m_DecSendPara[index].ReSyncPts = true;
            m_DecSendPara[index].RedundantFrCnt = 0;
            break;
        case PS_FASTFORWARD2X:
        case PS_FASTFORWARD4X:
        case PS_FASTFORWARD8X:
        case PS_FASTFORWARD16X:
        case PS_FASTBACKWARD2X:
        case PS_FASTBACKWARD4X:
        case PS_FASTBACKWARD8X:
        case PS_FASTBACKWARD16X:
            SyncPts = true;
            SetIFrame = true;
            break;
        default:
            return -1;
            break;
    }
    if (true == SyncPts)
    {
        ClearPtsByChn(index);
    }
    if (PS_PAUSE ==  m_DecSendPara[index].CurPlayState)
    {
        m_DDSPOperate.RePlayVideo(m_AvPurpose, index);
    }
    m_DecSendPara[index].RequestIFrame = SetIFrame;
    m_DecSendPara[index].StepPlay = true;
    //m_DDSPOperate.StepPlayVideo(m_AvPurpose, index);
    return 0;
}

int CDecDealSyncPlay::Seek(int index)
{
    ClearPtsByChn(index);
    m_DecSendPara[index].RequestIFrame = true;
    m_DDSPOperate.PauseVideo(m_AvPurpose, index);
    m_DDSPOperate.RePlayVideo(m_AvPurpose, index);
    return 0;
}

void CDecDealSyncPlay::CheckPlayState(int index)
{
    DecSendPara_t *pOperate = &m_DecSendPara[index];

    if (pOperate->CurPlayState != pOperate->ExpectPlayState)
    {
        DEBUG_ERROR("CurPlayState %d, ExpectPlayState %d\n",pOperate->CurPlayState, pOperate->ExpectPlayState);
        switch(pOperate->ExpectPlayState)
        {
            case PS_NORMAL:
                RePlay(index);
                break;
            case PS_SLOWPLAY2X:
            case PS_SLOWPLAY4X:
            case PS_SLOWPLAY8X:
            case PS_SLOWPLAY16X:
                SlowPlay(index);
                break;
            case PS_FASTFORWARD2X:
            case PS_FASTFORWARD4X:
            case PS_FASTFORWARD8X:
            case PS_FASTFORWARD16X:
                FastForwardPlay(index);
                break;
            case PS_FASTBACKWARD2X:
            case PS_FASTBACKWARD4X:
            case PS_FASTBACKWARD8X:
            case PS_FASTBACKWARD16X:
                FastBackwardPlay(index);
                break;
            case PS_PAUSE:
                Pause(index);
                break;
            case PS_PRESTEPPLAY:
                StepPlay(index);
                pOperate->ExpectPlayState = PS_STEP;
                break;
            case PS_SEEK:
                Seek(index);
                DEBUG_MESSAGE("Seek Pause audio\n");
                m_DDSPOperate.PauseAudio();
                pOperate->ExpectPlayState = PS_NORMAL;
                break;
            default:
                break;
        }
        pOperate->CurPlayState = pOperate->ExpectPlayState;
        if (m_AudioChn == index)
        {
            if ((pOperate->CurPlayState == PS_INIT) || (pOperate->CurPlayState == PS_NORMAL))
            {
                DEBUG_ERROR("Replay audio, m_AudioChn %d\n", m_AudioChn);
                m_DDSPOperate.RePlayAudio();
            }
            else
            {
                DEBUG_ERROR("Pause audio, m_AudioChn %d\n", m_AudioChn);
                m_DDSPOperate.PauseAudio();
            }
        }
    }
    if ((m_ExpectAudioChn != m_AudioChn) && ((index == m_ExpectAudioChn) || (m_ExpectAudioChn < 0)))
    {
        DEBUG_ERROR("Audio Change chn %d to chn %d, Pause audio\n", m_AudioChn, m_ExpectAudioChn);
        m_DDSPOperate.PauseAudio();
        m_DDSPOperate.ClearAoBuf(m_AudioChn);
        m_AudioChn = m_ExpectAudioChn;
        if (index == m_ExpectAudioChn)
        {
            DEBUG_ERROR("Audio Change Replay audio\n");
            m_DDSPOperate.RePlayAudio();
        }

    }
}

int CDecDealSyncPlay::CheckSend(int Chn, unsigned long long PlayPts)
{/*check pts is arrived then send the data*/
    long long PtsDiffer = 0, SysTimeDiffer = 0;
    unsigned long long UlPtsDiffer = 0, UllSysTimeDiffer = 0;
    DecSendPara_t *pInfo = NULL;
    unsigned long long PtsInterval = 0;
    int Retval = -1;
//    unsigned long long CurSec = 0;
    int factor = 1, PlayFrameRate = 25;
    unsigned long long CurPts = PlayPts;
#ifdef _AV_PLATFORM_HI3515_
	unsigned long long abspts = 0;
#endif
    pInfo = &m_DecSendPara[Chn];
    CurPts = PlayPts - pInfo->BaseChnPts + pInfo->BaseFrameRtcTime*1000000ULL;
    if ((pInfo->LastFramePts == pInfo->BaseFramePts) && (pInfo->LastFramePts != CurPts))
    {
        pInfo->LastFramePts = CurPts;
    }
    
    switch(pInfo->CurPlayState)
    {
        case PS_FASTBACKWARD2X:
        case PS_FASTFORWARD2X:
        case PS_SLOWPLAY2X:
            factor = 2;
            break;
        case PS_FASTBACKWARD4X:
        case PS_FASTFORWARD4X:
        case PS_SLOWPLAY4X:
            factor = 4;
            break;
        case PS_FASTBACKWARD8X:
        case PS_FASTFORWARD8X:
        case PS_SLOWPLAY8X:
            factor = 8;
            break;
        case PS_FASTBACKWARD16X:
        case PS_FASTFORWARD16X:
        case PS_SLOWPLAY16X:
            factor = 16;
            break;
        default:
            if (CurPts < pInfo->BaseFramePts)
            {
                DEBUG_ERROR("Release Chn %d, BsPts %llu, PrePts  %llu, CurPts %llu, CurSysTime %llu, BaseSysTime %llu\n", 
                    Chn, pInfo->BaseFramePts, pInfo->LastFramePts, CurPts,  pInfo->CurSysTime, pInfo->BaseSysPts);
                return 1;
            }
            break;
    }
    /*计算PTS差值，特殊回放的时候乘以或在除以相对应的因子*/
    switch(pInfo->CurPlayState)
    {
        case PS_FASTBACKWARD2X:
        case PS_FASTBACKWARD4X:
        case PS_FASTBACKWARD8X:
        case PS_FASTBACKWARD16X:
            PtsDiffer = cal_llabs(pInfo->BaseFramePts, CurPts) / factor;

#ifdef _AV_PLATFORM_HI3515_
            PtsDiffer = abspts / factor;
#endif
            if (CurPts < pInfo->BaseFramePts)
            {
                UlPtsDiffer = (pInfo->BaseFramePts - CurPts) / factor;
            }
            else
            {
                UlPtsDiffer = (CurPts - pInfo->BaseFramePts) / factor;
            }
            break;
        case PS_FASTFORWARD2X:
        case PS_FASTFORWARD4X:
        case PS_FASTFORWARD8X:
        case PS_FASTFORWARD16X:
            PtsDiffer = cal_llabs(CurPts, pInfo->BaseFramePts) / factor;
#ifdef _AV_PLATFORM_HI3515_
            PtsDiffer = abspts / factor;
#endif
            UlPtsDiffer = PtsDiffer;
            if (CurPts > pInfo->BaseFramePts)
            {
                UlPtsDiffer = (CurPts-pInfo->BaseFramePts) / factor;
            }
            else
            {
                DEBUG_ERROR("Fast Play factor %d Release Chn %d, BsPts %llu, PrePts  %llu, CurPts %llu, CurSysTime %llu, BaseSysTime %llu\n", 
                    factor, Chn, pInfo->BaseFramePts, pInfo->LastFramePts, CurPts,  pInfo->CurSysTime, pInfo->BaseSysPts);
                return 1;
            }
            break;
        case PS_SLOWPLAY2X:
        case PS_SLOWPLAY4X:
        case PS_SLOWPLAY8X:
        case PS_SLOWPLAY16X:
            PtsDiffer = cal_llabs(CurPts, pInfo->BaseFramePts)*factor;

#ifdef _AV_PLATFORM_HI3515_
            PtsDiffer = abspts*factor;
#endif
            if (CurPts > pInfo->BaseFramePts)
            {
                UlPtsDiffer = (CurPts - pInfo->BaseFramePts)*factor;
            }
            else
            {
                UlPtsDiffer = (pInfo->BaseFramePts - CurPts)*factor;
            }
            break;
        default:
            PtsDiffer = CurPts - pInfo->BaseFramePts;
            UlPtsDiffer = CurPts - pInfo->BaseFramePts;
            //PtsOffset = 1000000ULL/m_PlayFrameRate[Chn]*4;
            break;
    }
    //DEBUG_MESSAGE("----------------check send %d\n", __LINE__);
    switch(pInfo->CurPlayState)
    {
            case PS_FASTBACKWARD2X:
            case PS_FASTBACKWARD4X:
            case PS_FASTBACKWARD8X:
            case PS_FASTBACKWARD16X:
            case PS_FASTFORWARD2X:
            case PS_FASTFORWARD4X:
            case PS_FASTFORWARD8X:
            case PS_FASTFORWARD16X:
                PlayFrameRate = m_BaseFrameRate[Chn];
                break;
            default:
                PtsInterval = cal_llabs(CurPts, pInfo->LastFramePts);
#ifdef _AV_PLATFORM_HI3515_
                PtsInterval = abspts;
#endif
                if(!GCL_BIT_VAL_TEST(m_SyncPtsBmp, Chn))
                {
                    if(PtsInterval > 2000000)
                    {
                        DEBUG_ERROR("Release Send Chn %d, BsPts %llu, PrePts  %llu, CurPts %llu, PtsInterval %llu, FrameRate(%d/%d)\n", 
                            Chn, pInfo->BaseFramePts, pInfo->LastFramePts, CurPts,  PtsInterval, m_PlayFrameRate[Chn], m_BaseFrameRate[Chn]);
                            pInfo->LastFramePts = CurPts;
                        return 0;
                    }
                }
                PlayFrameRate = m_PlayFrameRate[Chn];
                break;
    }

    pInfo->CurSysTime = Get_system_ustime();
    SysTimeDiffer = (pInfo->CurSysTime - pInfo->BaseSysPts) * PlayFrameRate / m_BaseFrameRate[Chn];
    UllSysTimeDiffer = (pInfo->CurSysTime - pInfo->BaseSysPts) * PlayFrameRate / m_BaseFrameRate[Chn];
    if (SysTimeDiffer < 0)
    {
        SysTimeDiffer = 0;
        UllSysTimeDiffer = 0;
    }
    if (PtsDiffer < 0)
    {
        //DEBUG_MESSAGE("Chn %d, BsPts %llu, PrePts  %llu, CurPts %llu, PtsDiffer %llu, CurSysTime %llu, BaseSysTime %llu,  SysDiffer %llu\n", 
            //Chn, pInfo->BaseFramePts, pInfo->LastFramePts, Pts, PtsDiffer, pInfo->CurSysTime, pInfo->BaseSysPts, SysTimeDiffer);        
    }
    //UllSysTimeDiffer += PtsOffset;
    if (UlPtsDiffer <= UllSysTimeDiffer)
    {
        Retval = 0;
        if (GCL_BIT_VAL_TEST(m_SyncPtsBmp, Chn))
        {
            //DEBUG_ERROR("aa Chn %d Clear m_SyncPtsBmp %x\n", Chn, m_SyncPtsBmp);
            //DEBUG_ERROR("Chn %d, BsPts %llu, PrePts  %llu, CurPts %llu, UlPtsDiffer %llu, CurSysTime %llu, BaseSysTime %llu,  UllSysTimeDiffer %llu, %lld m_SyncPtsBmp=%x\n", 
            //Chn, pInfo->BaseFramePts, pInfo->LastFramePts, CurPts, UlPtsDiffer, pInfo->CurSysTime, pInfo->BaseSysPts, UllSysTimeDiffer, SysTimeDiffer, m_SyncPtsBmp);
            //SetSyncPtsBmp(Chn, false);
            //DEBUG_ERROR("bb Chn %d Clear m_SyncPtsBmp %x\n", Chn, m_SyncPtsBmp);
        }
    }
    //DEBUG_MESSAGE("----------------check send %d\n", __LINE__);
//    CurSec = pInfo->CurSysTime / 1000000ULL;
    if ((m_DecSendPara[Chn].CurPlayState == PS_STEP) && (false == m_DecSendPara[Chn].StepPlay))
    {
        DEBUG_MESSAGE("setp mode\n");
        Retval = -1;
    }
    if ((pInfo->BaseFramePts == pInfo->LastFramePts ) && (SysTimeDiffer > 2000000))
    {
        //DEBUG_MESSAGE("no send Chn %d, BsPts %lld, PrePts  %lld, CurPts %lld, PtsDiffer %lld, CurSysTime %lld, BaseSysTime %lld,  SysDiffer %lld\n", 
            //Chn, pInfo->BaseFramePts, pInfo->LastFramePts, Pts, PtsDiffer, pInfo->CurSysTime, pInfo->BaseSysPts, SysTimeDiffer);
    }
    if( Retval == 0 )
    {
        int FrameNum = 0, DataLen = 0;
        DecDataType_t DataType = DECDATA_VIDEO;
        //DEBUG_MESSAGE("======ch=%d============================check send get wait count\n", Chn);
        m_DDSPOperate.GetDecWaitFrCnt(m_AvPurpose, Chn, &FrameNum, &DataLen, DataType);
        if (FrameNum > (MAX_VO_WAIT_FR * 3))
        {
            m_DecSendPara[Chn].RedundantFrCnt = FrameNum;
            DEBUG_ERROR("Chn %d Wait Frcnt %d\n", Chn, FrameNum);
            //DEBUG_MESSAGE("=================to much wait\n");
            Retval = -1;
        }
        else if (m_DecSendPara[Chn].RedundantFrCnt >= (MAX_VO_WAIT_FR*3))
        {
            if (FrameNum < MAX_VO_WAIT_FR)
            {
                m_CheckValid[Chn] = false;
                m_DecSendPara[Chn].ReSyncPts = true;
                m_DecSendPara[Chn].RedundantFrCnt = 0;
                DEBUG_ERROR("Chn %d ReSyncPts\n", Chn);
            }
            //DEBUG_ERROR("Chn %d RedundantFrCnt %d\n", Chn, m_DecSendPara[Chn].RedundantFrCnt);
            //DEBUG_MESSAGE("=================== to much redundant frcount\n");
            Retval = -1;
        }
    }
    if (0 == Retval)
    {
        //if ((factor > 1) && (m_DecSendPara[Chn].FrameNum > 6))//((m_PlayFrameRate[Chn] != m_BaseFrameRate[Chn]) && (Chn != 1))//
        if (m_DecSendPara[Chn].FrameNum > (m_BaseFrameRate[Chn] + 5))
        {
            DEBUG_DEBUG("Send Chn %d, factor %d, pInfo->CurPlayState %d,BsPts %llu, PrePts  %llu, CurPts %llu, CurSysTime %llu, BaseSysTime %llu, UlPtsDiffer %llu, UllSysTimeDiffer %llu, PtsDiffer %lld, SysTimeDiffer %lld, (%d/%d)\n", 
                Chn, factor, pInfo->CurPlayState,  pInfo->BaseFramePts, pInfo->LastFramePts, CurPts,  pInfo->CurSysTime, pInfo->BaseSysPts, UlPtsDiffer, UllSysTimeDiffer, PtsDiffer, SysTimeDiffer, m_PlayFrameRate[Chn], m_BaseFrameRate[Chn]);
        }
        pInfo->LastFramePts = CurPts;
    }
    return Retval;
}

bool CDecDealSyncPlay::SyncPtsByChn(int index, unsigned long long BaseLinePts, unsigned long long SysTime, long long BaseRtcTime, unsigned long long BaseChnPts)
{
    if( !GCL_BIT_VAL_TEST(m_SyncPtsBmp, index) )
    {
        return true;
    }
    m_DecSendPara[index].BaseSysPts = SysTime;
    m_DecSendPara[index].LastFramePts = BaseLinePts;
    m_DecSendPara[index].BaseFramePts = BaseLinePts;
    m_DecSendPara[index].BaseFrameRtcTime = BaseRtcTime;
    m_DecSendPara[index].BaseChnPts = BaseChnPts;
    m_CheckValid[index] = true;
    GCL_BIT_VAL_CLEAR(m_SyncPtsBmp, index);
    DEBUG_ERROR(" index %d, BasePts %llu, BaseSysPts (%llu, %llu)\n", index, BaseLinePts, SysTime,  m_DecSendPara[index].BaseSysPts);
    return true;
}

bool CDecDealSyncPlay::ClearPtsByChn(int index)
{
    m_CheckValid[index] = false;
    m_DecSendPara[index].BaseSysPts = 0;
    m_DecSendPara[index].LastFramePts = 0;
    m_DecSendPara[index].BaseFramePts = 0;
    m_DecSendPara[index].FrameNum = 0;
    GCL_BIT_VAL_SET(m_SyncPtsBmp, index);
    SetDataChnMask(index, true);
    DEBUG_ERROR("CDecDealSyncPlay::ClearPtsByChn index %d, m_SyncPtsBmp %x\n", index, m_SyncPtsBmp);
    if (!GCL_BIT_VAL_TEST(m_SyncPtsBmp, index))
    {
        DEBUG_ERROR(" index %d, m_SyncPtsBmp %x SetFailed\n", index, m_SyncPtsBmp);
        GCL_BIT_VAL_SET(m_SyncPtsBmp, index);
        DEBUG_ERROR(" index %d, m_SyncPtsBmp %x sencond\n", index, m_SyncPtsBmp);
    }

    return true;
}

void CDecDealSyncPlay::ResetPtsByChn(int index)
{
    m_DecSendPara[index].CurSysTime = Get_system_ustime();
    m_DecSendPara[index].BaseSysPts = m_DecSendPara[index].CurSysTime;
    m_DecSendPara[index].BaseFramePts = m_DecSendPara[index].LastFramePts;
    DEBUG_MESSAGE(" index %d, BasePts %lld, BaseSysPts (%lld, %lld)\n", index, m_DecSendPara[index].BaseFramePts,  m_DecSendPara[index].CurSysTime,  m_DecSendPara[index].BaseSysPts);
    return;
}

bool CDecDealSyncPlay::SetSyncPtsBmp(int index, bool enable)
{
    if (GCL_BIT_VAL_TEST(m_SyncPtsBmp, index))
    {
        if (false == enable)
        {
            GCL_BIT_VAL_CLEAR(m_SyncPtsBmp, index);
            m_DecSendPara[index].FrameNum = 0;
            //DEBUG_ERROR("Chn %d, Clear SyncPtsBmp %x\n", index, m_SyncPtsBmp);
        }
    }
    else if (true == enable)
    {
        GCL_BIT_VAL_SET(m_SyncPtsBmp, index);
        //DEBUG_ERROR("Chn %d, Set SyncPtsBmp %x\n", index, m_SyncPtsBmp);

        m_DecSendPara[index].RequestIFrame = true;
        m_DecSendPara[index].FrameNum = 0;
    }
    return true;
}

bool CDecDealSyncPlay::ClearPts(unsigned int ChnMask)
{
    for (int index = 0; index < m_ChnNum; index++)
    {
        if (GCL_BIT_VAL_TEST(ChnMask, index))
        {
            ClearPtsByChn(index);
            m_DecSendPara[index].RequestIFrame = true;
        }
    }
    return true;
}

bool CDecDealSyncPlay::ReSyncAllChn()
{
    unsigned long long MinPts = 0, MaxPts = 0, DiffPts = 0;
    unsigned int ChnMask = 0;
    bool force = false;

    for (int index = 0; index < m_ChnNum; index++)
    {/*强制同步PTS*/
        if (true == m_DecSendPara[index].ReSyncPts)
        {
            force = true;
            break;
        }
    }
    //DEBUG_ERROR("m_SyncPtsBmp %x, m_DecDataChnMask %x\n", m_SyncPtsBmp, m_DecDataChnMask);

    if (false == force)
    {
        if (m_SyncPtsBmp != 0)
        {
            DEBUG_ERROR("m_SyncPtsBmp %x, m_DecDataChnMask %x\n", m_SyncPtsBmp, m_DecDataChnMask);
            return false;
        }

        for (int index = 0; index < m_ChnNum; index++)
        {
            //if (m_DecSendPara[index].FrameNum == 0)
            //{
                //DEBUG_ERROR("[ReSyncAllChn]Chn %d, FrameNum %d, BsPts %llu, PrePts  %llu,  CurSysTime %llu, BaseSysTime %llu\n", 
                    //index, m_DecSendPara[index].FrameNum, m_DecSendPara[index].BaseFramePts, m_DecSendPara[index].LastFramePts, 
                    //m_DecSendPara[index].CurSysTime, m_DecSendPara[index].BaseSysPts);
            //}
            if (GCL_BIT_VAL_TEST(m_SyncPtsBmp, index))
            {
                continue;
            }
            if ((m_DecSendPara[index].CurPlayState != PS_NORMAL) && (m_DecSendPara[index].CurPlayState != PS_INIT))
            {
                continue;
            }
            if (m_DecSendPara[index].FrameNum > 0)
            {
                if ((0 == MinPts) || (MinPts > m_DecSendPara[index].LastFramePts))
                {
                    MinPts = m_DecSendPara[index].LastFramePts;
                }
                if ((0 == MaxPts) || (MaxPts < m_DecSendPara[index].LastFramePts))
                {
                    MaxPts = m_DecSendPara[index].LastFramePts;
                }
                GCL_BIT_VAL_SET(ChnMask, index);
            }
        }

        DiffPts = MaxPts - MinPts;
        //DEBUG_ERROR("ReSyncAllChn MinPts %llu, MaxPts %llu, DiffPts %llu ChnMask %x, force %d syncBmp=0x%x dataBmp=0x%x\n", MinPts, MaxPts, DiffPts, ChnMask, force, m_SyncPtsBmp, m_DecDataChnMask);
        if ((DiffPts < 4000000) || (ChnMask == 0))
        {
            DEBUG_MESSAGE("ReSyncAllChns MinPts %lld, MaxPts %lld, DiffPts %lld, ChnMask %x force %d syncBmp=0x%x dataBmp=0x%x\n", MinPts, MaxPts, DiffPts, ChnMask, force, m_SyncPtsBmp, m_DecDataChnMask);
            return false;
        }
    }
    DEBUG_ERROR("Act ReSyncAllChn MinPts %llu, MaxPts %llu, DiffPts %llu, ChnMask %x, force %d\n", MinPts, MaxPts, DiffPts, ChnMask, force);
    for (int index = 0; index < m_ChnNum; index++)
    {
        WaitIdle(true, index);
        ClearPtsByChn(index);
    }
    if (SyncPts(m_DecChnMask, GETBASEPTS_FRAME_HOLD) == true)
    {
        m_SyncPtsBmp = 0;
        for (int index = 0; index < m_ChnNum; index++)
        {
            m_DecSendPara[index].ReSyncPts = false;
        }
    }
    if ((m_AudioChn >= 0) && (false == force))
    {
        m_DDSPOperate.ClearAoBuf(m_AudioChn);
    }
    for (int index = 0; index < m_ChnNum; index++)
    {
        WaitIdle(false, index);
    }
    return true;
}

bool CDecDealSyncPlay::SetDataChnMask(int index, bool enable)
{
    if (GCL_BIT_VAL_TEST(m_DecDataChnMask, index) && (false == enable))
    {
        GCL_BIT_VAL_CLEAR(m_DecDataChnMask, index);
        DEBUG_MESSAGE("Clear %d m_DecDataChnMask %x\n", index, m_DecDataChnMask);
    }
    else if ((!GCL_BIT_VAL_TEST(m_DecDataChnMask, index)) && (true == enable))
    {
        GCL_BIT_VAL_SET(m_DecDataChnMask, index);
        DEBUG_MESSAGE("Set %d m_DecDataChnMask %x\n", index, m_DecDataChnMask);
    }
    return true;
}

bool CDecDealSyncPlay::GetBaselineInfoByPts(BaseLineInfo_t &BaseInfo, int SyncMethod, int direct)
{
    unsigned long long BasePts = 0;
    int retval = 0;
    static int GetBasePtsFailedCnt = 0;

    if ((GetBasePtsFailedCnt%30) == 0)
    {
        DEBUG_ERROR("aa m_SyncPtsBmp %x, m_DecDataChnMask %x, m_GetBasePtsDirect %d\n", m_SyncPtsBmp, m_DecDataChnMask, m_GetBasePtsDirect);
    }

    retval = m_DDSPOperate.GetBaseLinePts(&BasePts, SyncMethod, m_GetBasePtsDirect);
    //DEBUG_MESSAGE("===============get base pts=%d pts=%lld\n",retval, BasePts );
    if (retval < 0)
    {
        //DEBUG_ERROR("GetBaseLinePts error.........SyncMethod %d\n", SyncMethod);
        GetBasePtsFailedCnt++;
        return false;
    }

    BaseInfo.BaseFramePts = BasePts;
    for (int index = 0; index < m_ChnNum; index++)
    {
        BaseInfo.BaseChnFramePts[index] = 0;
        BaseInfo.BaseRtcTime[index] = 0;
    }
    GetBasePtsFailedCnt = 0;
    return true;
}

bool CDecDealSyncPlay::SyncPts(unsigned int ChnMask, int SyncMethod)
{
    if( m_SyncPtsBmp == 0 )
    {
        return false;
    }
    bool retval = false;
    BaseLineInfo_t BaseInfo;

    //DEBUG_ERROR("=ChnMask %x, m_SyncPtsBmp %x, m_DecDataChnMask %x\n", ChnMask, m_SyncPtsBmp, m_DecDataChnMask);
    for (int index = 0; index < m_ChnNum; index++)
    {
        if (m_DDSPOperate.CheckIndexStatus(index) == 0)
        {
            SetDataChnMask(index, false);
            SetSyncPtsBmp(index, false);
        }
        else
        {
            SetDataChnMask(index, true);
        }
    }
    //DEBUG_MESSAGE("########222######sync pts, m_SyncPtsBmp=0x%x dataChnMask=0x%x\n", m_SyncPtsBmp, m_DecDataChnMask);

    if (m_SyncPtsBmp != m_DecDataChnMask)
    {
        return false;
    }

    //SysTime = Get_system_ustime();
    if (m_GetBasePtsDirect == 0)
    {/*方向值非法重新校验*/
        for (int index = 0; index < m_ChnNum; index++)
        {
            if ((m_DecSendPara[index].CurPlayState == PS_FASTBACKWARD2X)
                || (m_DecSendPara[index].CurPlayState == PS_FASTBACKWARD4X)
                || (m_DecSendPara[index].CurPlayState == PS_FASTBACKWARD8X)
                || (m_DecSendPara[index].CurPlayState == PS_FASTBACKWARD16X))
            {
                m_GetBasePtsDirect = -1;
                //DEBUG_ERROR("aa ReSet Pts Direct SysTime %lld ChnMask %x, m_SyncPtsBmp %x, m_DecDataChnMask %x, m_GetBasePtsDirect %d\n", SysTime, ChnMask, m_SyncPtsBmp, m_DecDataChnMask, m_GetBasePtsDirect);
                break;
            }
            else
            {
                m_GetBasePtsDirect = 1;
                //DEBUG_ERROR("bb ReSet Pts Direct SysTime %lld ChnMask %x, m_SyncPtsBmp %x, m_DecDataChnMask %x, m_GetBasePtsDirect %d\n", SysTime, ChnMask, m_SyncPtsBmp, m_DecDataChnMask, m_GetBasePtsDirect);
                break;
            }
        }
    }

    retval = GetBaselineInfoByPts(BaseInfo, SyncMethod, m_GetBasePtsDirect);
    //DEBUG_MESSAGE("base pts ret=%d\n", retval);
    if (false == retval)
    {
        return false;
    }

    DEBUG_MESSAGE("@@@@@@@@@@@@@@@@@@@m_SyncPtsBmp=0x%x m_DecDataChnMask=0x%x\n", m_SyncPtsBmp, m_DecDataChnMask);
    unsigned long long SysTime = Get_system_ustime();
    if( m_SyncPtsBmp != m_DecDataChnMask )
    {
        // BaseInfo.BaseFramePts = CheckSinglePtsDiffer( BaseInfo.BaseFramePts, SysTime, nSyncBaseChannel );
    }
    //DEBUG_ERROR("2.SysTime %lld mask %x, m_SyncPtsBmp 0x%x\n", SysTime, ChnMask, m_SyncPtsBmp);
    for (int index = 0; index < m_ChnNum; index++)
    {
        if (GCL_BIT_VAL_TEST(m_SyncPtsBmp, index) || (!GCL_BIT_VAL_TEST(m_DecDataChnMask, index)))
        {
            SyncPtsByChn(index, BaseInfo.BaseFramePts, SysTime, BaseInfo.BaseRtcTime[index], BaseInfo.BaseChnFramePts[index]);
        }
    }
    return true;
}

void CDecDealSyncPlay::SwitchAudioChn(int index)
{
    m_ExpectAudioChn = index;
}

unsigned long long CDecDealSyncPlay::CheckSinglePtsDiffer( unsigned long long u64NewPts, unsigned long long u64NewSysMircroSec, int& nSyncBaseChannel )
{
    unsigned long long u64ReturnPts = u64NewPts;
    unsigned long long u64FindSysMicroSec = 0;
    unsigned long long u64FindBasePts = 0;
    nSyncBaseChannel = -1;
    
    int index = 0;
    for( index = 0; index < m_ChnNum; index++ )
    {
        //DEBUG_MESSAGE("###################find need pts index=%d 0x%x basefr=%llu time=%llu\n", index, m_SyncPtsBmp, m_DecSendPara[index].BaseFramePts, m_DecSendPara[index].BaseSysPts);
        if( !GCL_BIT_VAL_TEST(m_SyncPtsBmp, index) && m_DecSendPara[index].BaseFramePts != 0 )
        {
            u64FindSysMicroSec = m_DecSendPara[index].BaseSysPts;
            u64FindBasePts = m_DecSendPara[index].BaseFramePts;
            //DEBUG_MESSAGE("find index=%d\n", index);
            break;
        }
    }

    if( u64FindSysMicroSec != 0 && u64FindBasePts != 0 )
    {
        unsigned long long u64HopePts = u64FindBasePts + (u64NewSysMircroSec-u64FindSysMicroSec);
        long long ptsSpan = u64HopePts - u64NewPts;
        if( ABS(ptsSpan) < 900000LL ) //900ms
        {
            u64ReturnPts = u64HopePts;
            nSyncBaseChannel = index;
        }

        DEBUG_MESSAGE("######CDecDealSyncPlay::CheckSinglePtsDiffer findmicro=%llu, newmicro=%llu findpts=%llu new pts=%llu hopepts=%llu abs=%llu resturn=%lld index=%d\n"
            , u64FindSysMicroSec, u64NewSysMircroSec, u64FindBasePts, u64NewPts, u64HopePts, ABS(ptsSpan), u64ReturnPts, nSyncBaseChannel);
    }
    else
    {
        DEBUG_MESSAGE("#######################not found need info return=%lld\n", u64ReturnPts);
    }

    return u64ReturnPts;
}


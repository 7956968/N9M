/*
 * DecDealSyncPlay.h
 *
 *  Created on: 2012-07-24
 *      Author: czzhao
 */

#ifndef AVPREVIEWDECODER_H_
#define AVPREVIEWDECODER_H_

#include <pthread.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "AvCommonFunc.h"

#include "System.h"
#include "CommonMacro.h"
#include "AvDevCommType.h"
#include "CmdBuffer.h"
#include "StreamBuffer.h"
#include "AvDeviceInfo.h"
#include "CommonDebug.h"
using namespace Common;

#define PREVIEW_MAX_ONE_FRAME_SIZE 8000000 //ABOUT 800k
enum ePrevThreadReq { EPREVTD_RUNNING, EPREVTD_PAUSE, EPREVTD_CONTINUE, EPREVTD_EXIT };
typedef ePrevThreadReq ePrevThreadStatus;

typedef struct tag_AvPrevChlDecParam{
    CCmdBuffer PlayCmd; //command for each channel
    struct _cmd_{ int iCmd; char szData[1024]; } stuCmdTmp;

    unsigned long long u64LastFramePts;
    unsigned long long u64BaseFramePts;
    unsigned long long u64BaseSysTime;
    int i32AjustSpanBase;
    int i32AjustSpan;
    
    unsigned int u32VideoSendCount;

    unsigned int u32FailedCountTest;
    unsigned char u8AdjustCallCount;
    
    unsigned char u8HaveTask; //is task exist
    unsigned char u8Index;
    unsigned char u8ThreadIndex;
    
    unsigned char u8FrameRate;
 //   unsigned char u8RealFrameRate;
    
    unsigned char u8RequestIFrame;
    unsigned char u8DecoderRun;
    unsigned char u8RealCh; //real ch for sharebuffer
    unsigned char u8StreamType; // 0:mainstream; 1:substream

    HANDLE ShareStreamHandle;
    char* pData;
    char* pDataTemp;
    unsigned int u32TmpDataLen;
    unsigned long long u64TmpPts;
    DecDataType_t eFrameType;

    bool bMarkClear;

    unsigned char u8LastVoFramerate;
    unsigned char u8FlipForVoFramerateCheck;
} AvPrevChlDecParam_t;

//thread params,thread controls 
typedef struct tag_AvPrevThreadParam{
    ePrevThreadStatus eThreadState;
    void* ptrObj;
    unsigned char u8PauseThread;
    unsigned char u8ThreadIndex;
    unsigned char u8StartCh;
    unsigned char u8EndCh;
    unsigned long long u64ThreadTime;
} AvPrevThreadParam_t;

enum ePrevDecoderCmd{ PREVDEC_CMD_ADDTASK, PREVDEC_CMD_DELTASK, PREVDEC_CMD_PAUSETASK, PREVDEC_CMD_CONTINUETASK };

typedef struct tag_AvPrevTask{
    unsigned char u8ChlIndex;
    unsigned char u8RealChl;
    unsigned char u8StreamType;
    char reserved;
    unsigned int u32FrameRate;
} AvPrevTask_t;

enum eHDVR_COMMAND{ HDVR_CMD_CHANNEL_STOP, HDVR_CMD_CHANNEL_START, HDVR_CMD_CHANGE_STREAM, HDVR_STATE_INLID };
typedef struct tag_hdvrOperate{
    unsigned char u8Command; // eHDVR_COMMAND:
    unsigned char u8Channel;
    char reserved[2];
    union{
        struct{
            unsigned char u8StreamType; //for changne stream 0-main stream 1-substream
            char reserved[3];
        } stuSteamChangeParam;
    };
} HdvrOperate_t;

typedef struct
{
	/**
	 * @brief ��ȡ֡����
	 * @param-purpose, ͨ��������,ͨ����,��������(��Ƶor��Ƶ),����buffer, ���ݳ���,PTS,�ֶα��(begin or end), ����
	 * @return 0:�ɹ� -1:ʧ��
	 */
	boost::function<int(int, int, unsigned char**, int *, int *)>GetFrameBuffer;
	/**
	 * @brief �����Ƶ�������
	 * @param-ͨ��������
	 * @return 0:�ɹ� -1:ʧ��
	 */
	boost::function<int(int)>ClearAoBuf;
	/**
	 * @brief ������Ƶ����
	 * @param-purpose, ͨ��������
	 * @return 0:�ɹ� -1:ʧ��
	 */
	boost::function<int(int, int)>RePlayVideo;
	/**
	 * @brief��ͣ��Ƶ����
	 * @param-purpose, ͨ��������
	 * @return 0:�ɹ� -1:ʧ��
	 */
	boost::function<int(int, int)>PauseVideo;
	/**
	 * @brief ������Ƶ����
	 * @return 0:�ɹ� -1:ʧ��
	 */
	boost::function<int(void)>RePlayAudio;
	/**
	 * @brief ��ͣ��Ƶ����
	 * @return 0:�ɹ� -1:ʧ��
	 */
	boost::function<int(void)>PauseAudio;
	/**
	 * @brief ��ȡ��ͨ���Ƿ�������
	 * @param-ͨ��������
	 * @return 0:������;����:������
	 */
	boost::function<int(int)>CheckIndexStatus;
	/**
	 * @brief ��ȡ���֡��PTS
	 * @param-purpose, chn, pts
	 * @return 0:�ɹ�;����:ʧ��
	 */
	boost::function<int(int, int, unsigned long long *)>GetLatestPts;
	/**
	 * @brief��ȡao����vo����δ���������֡���ͳ���
	 * @param-purpose, ͨ�������ţ�֡�������ݳ��ȣ���������(��Ƶor��Ƶ)
	 * @return 0:�ɹ� -1:ʧ��
	 */
	boost::function<int(int, int, int *, int*, int)> GetDecWaitFrCnt;/*chn, framenum, type*/

	/**
	 * @brief��ȡ���ý���vo���֡��
	 * @param-purpose, ͨ�������ţ��ط�֡��
	 * @return 0:�ɹ� -1:ʧ��
	 */
	boost::function<int(int, int, int)> SetDecPlayFrameRate;/*purpose, chn, framerate*/

	/**
	 * @brief������һ��I֡
	 * @param-purpose, ͨ��������
	 * @return 0:�ɹ� -1:ʧ��
	 */
	boost::function<int(int, int)> GotoNextIFrame;

	/**
	 * @brief��ȡ���廹�ж���֡
	 * @param-purpose, ͨ��������
	 * @return :֡��
	 */
	boost::function<int(int, int)> GetLeftFrameNum;
	/**
	 * @brief �������ݵ�����������
	 * @param-purpose, ͨ�������ţ����ݻ��棬���ݳ��ȣ���������(��Ƶor��Ƶ)
	 * @return 0:�ɹ� -1:ʧ��
	 */
	boost::function<int(int, int, unsigned char *, int, int)> SendDataToDec;/*chn, buffer, datalen, type*/

    /**
     * @brief close coder channel when not using
     **/
    boost::function<int(int, int)> CloseDecoder;

    boost::function<int(int, int)> ResetDecoderChannel;

    boost::function<int(int, int)> SetVoChannelFramerate;

    boost::function<int(int, int&)> GetVoChannelFramerate;

} APD_Operation_t;

class CAVPreviewDecCtrl
{
public:
	/**
	 * @brief 
	 * @param u32MaxPrewChnNum -���Ԥ����ͨ����
	 * @param ChnNumPerThread-ÿ���߳�֧�ֵ����ͨ����
	 * @param pOperation-�طŵĻص�����
	 */
	CAVPreviewDecCtrl( unsigned int u32MaxPrewChnNum, unsigned int u32ChnNumPerThread, int purpose, APD_Operation_t *pOperation, av_tvsystem_t eTvSystem );

	~CAVPreviewDecCtrl();

	void SignalDecDealBody( unsigned char u8ThreadIndex, unsigned char u8StartCh, unsigned char u8EndCh );

public:
	/**
	 * @brief�л���Ƶͨ��
	 * @param index -ͨ���Ŵ�0��ʼ
	 * @return ��
	 */
	void SwitchAudioChn( int index );

	int ResetPreviewChnMap( char chnMap[SUPPORT_MAX_VIDEO_CHANNEL_NUM] );

    int ResetPreviewOnlineState( unsigned char szOnlineState[SUPPORT_MAX_VIDEO_CHANNEL_NUM] );

    /**
     * @brief 
     * @param u32DecChlNum ��Чͨ����
     * @param u8StreamType ��������  0-������ 1-������
     **/
    int ChangePreviewChannel( unsigned int u32DecChlNum, int chnlist[_AV_VDEC_MAX_CHN_]
        , char chnMap[SUPPORT_MAX_VIDEO_CHANNEL_NUM], unsigned char szOnlineState[SUPPORT_MAX_VIDEO_CHANNEL_NUM], unsigned char u8StreamType,int realvochn_layout[_AV_VDEC_MAX_CHN_]);

    int StopDecoder( bool bWait );

    int PauseDecoder();

    /**
     * @brief ����ͨ�������֡��
     * @param 
     **/
    int SetFrameRate( int index, unsigned char u8FrameRate );

    int SetFrameRateFromParam( unsigned char szFrameRate[SUPPORT_MAX_VIDEO_CHANNEL_NUM] );

    /**
    * @brief ����ʱ��ͨ����ȡ���ڽ����λ��
    **/
    int GetChannelIndex( int iRealChl );

    int RestartDecoder();

    int MuteSound( bool bMute );

    /**
     * @brief for 
     **/
    int HdvrOperate( const HdvrOperate_t* pOperate );
    int SystemUpgrade();
    
    int SystemUpgradeFailed();

private:
    int ExitAllThread();

    /**
     * @brief process preview decoder
     * @return 0-ok, other: no task exist
     **/
    int ProcessDecoder( unsigned int chn, AvPrevChlDecParam_t* pChParam );

    int PrapareFrameData( AvPrevChlDecParam_t* pChParam );
    
    int ReleaseFrameData( AvPrevChlDecParam_t* pChParam );

    int CheckSend( AvPrevChlDecParam_t* pChParam );

    int ResetAdjustInfo( AvPrevChlDecParam_t* pChParam );

    int ProcessAddTask( AvPrevChlDecParam_t* pChParam, AvPrevTask_t* pstuTaskParam );

    int ProcessDelTask( AvPrevChlDecParam_t* pChParam );

    int SmoooothVideo( AvPrevChlDecParam_t* pChParam );

    int ResetPreviewDecParam();

    int SetBaseFrameRate( unsigned char u8Index, unsigned char u8BaseFrameRate );

	void CheckVlAndReset(AvPrevChlDecParam_t* pChParam);
        
private:
    ePrevThreadReq m_eThreadReq; // 1-running 2-pause for playback 3-exit all thread
    
	APD_Operation_t m_ApdOperate; //preview decoder operate
    unsigned char m_u8AudioChn; // audio decoder channel
    bool m_bAudioMute; // if system audio is mute, set this ture, whereas false
    unsigned char m_u8Purpose;
    unsigned char m_u8ChlNum; //max support decoder number for this machine
    unsigned char m_u8ThreadNum; //thread number for m_u8ChlNum
    AvPrevChlDecParam_t* m_pChlParam[_AV_VDEC_MAX_CHN_]; // decoder parameter for each decoder channel
    AvPrevThreadParam_t* m_pThreadParam[_AV_VDEC_MAX_CHN_]; // decoder parameter for each thread

    unsigned int m_u32CurDecChlNum;
    char m_szChannelMap[SUPPORT_MAX_VIDEO_CHANNEL_NUM]; // -1:closed else is the channel
    char m_szChannelList[_AV_VDEC_MAX_CHN_];
    int szRealVoChlLay[_AV_VDEC_MAX_CHN_];
    unsigned char m_szDevOnLineState[SUPPORT_MAX_VIDEO_CHANNEL_NUM];
    unsigned char m_szFrameRateInit[SUPPORT_MAX_VIDEO_CHANNEL_NUM];
    
    unsigned char m_u8CurStreamType; //0-main stream 1-substream
    HANDLE m_ShareStreamHandle[SUPPORT_MAX_VIDEO_CHANNEL_NUM][2];

    unsigned char m_u8HdvrChannelState[32];
    unsigned char m_bFirstFrameCheckLeft;

    av_tvsystem_t m_eTvSystem;
};


#endif /* AVPREVIEWDECODER_H_ */

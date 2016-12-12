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
	 * @brief 获取帧数据
	 * @param-purpose, 通道索引号,通道号,数据类型(音频or视频),数据buffer, 数据长度,PTS,分段标记(begin or end), 方向
	 * @return 0:成功 -1:失败
	 */
	boost::function<int(int, int, unsigned char**, int *, int *)>GetFrameBuffer;
	/**
	 * @brief 清除音频输出缓冲
	 * @param-通道索引号
	 * @return 0:成功 -1:失败
	 */
	boost::function<int(int)>ClearAoBuf;
	/**
	 * @brief 重启视频解码
	 * @param-purpose, 通道索引号
	 * @return 0:成功 -1:失败
	 */
	boost::function<int(int, int)>RePlayVideo;
	/**
	 * @brief暂停视频解码
	 * @param-purpose, 通道索引号
	 * @return 0:成功 -1:失败
	 */
	boost::function<int(int, int)>PauseVideo;
	/**
	 * @brief 重启音频解码
	 * @return 0:成功 -1:失败
	 */
	boost::function<int(void)>RePlayAudio;
	/**
	 * @brief 暂停音频解码
	 * @return 0:成功 -1:失败
	 */
	boost::function<int(void)>PauseAudio;
	/**
	 * @brief 获取该通道是否有数据
	 * @param-通道索引号
	 * @return 0:无数据;其他:有数据
	 */
	boost::function<int(int)>CheckIndexStatus;
	/**
	 * @brief 获取最后帧的PTS
	 * @param-purpose, chn, pts
	 * @return 0:成功;其他:失败
	 */
	boost::function<int(int, int, unsigned long long *)>GetLatestPts;
	/**
	 * @brief获取ao或者vo里面未解码的数据帧数和长度
	 * @param-purpose, 通道索引号，帧数，数据长度，数据类型(音频or视频)
	 * @return 0:成功 -1:失败
	 */
	boost::function<int(int, int, int *, int*, int)> GetDecWaitFrCnt;/*chn, framenum, type*/

	/**
	 * @brief获取设置解码vo输出帧率
	 * @param-purpose, 通道索引号，回放帧率
	 * @return 0:成功 -1:失败
	 */
	boost::function<int(int, int, int)> SetDecPlayFrameRate;/*purpose, chn, framerate*/

	/**
	 * @brief跳到下一个I帧
	 * @param-purpose, 通道索引号
	 * @return 0:成功 -1:失败
	 */
	boost::function<int(int, int)> GotoNextIFrame;

	/**
	 * @brief获取缓冲还有多少帧
	 * @param-purpose, 通道索引号
	 * @return :帧数
	 */
	boost::function<int(int, int)> GetLeftFrameNum;
	/**
	 * @brief 发送数据到解码器解码
	 * @param-purpose, 通道索引号，数据缓存，数据长度，数据类型(音频or视频)
	 * @return 0:成功 -1:失败
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
	 * @param u32MaxPrewChnNum -最多预览的通道数
	 * @param ChnNumPerThread-每个线程支持的最大通道数
	 * @param pOperation-回放的回调函数
	 */
	CAVPreviewDecCtrl( unsigned int u32MaxPrewChnNum, unsigned int u32ChnNumPerThread, int purpose, APD_Operation_t *pOperation, av_tvsystem_t eTvSystem );

	~CAVPreviewDecCtrl();

	void SignalDecDealBody( unsigned char u8ThreadIndex, unsigned char u8StartCh, unsigned char u8EndCh );

public:
	/**
	 * @brief切换音频通道
	 * @param index -通道号从0开始
	 * @return 无
	 */
	void SwitchAudioChn( int index );

	int ResetPreviewChnMap( char chnMap[SUPPORT_MAX_VIDEO_CHANNEL_NUM] );

    int ResetPreviewOnlineState( unsigned char szOnlineState[SUPPORT_MAX_VIDEO_CHANNEL_NUM] );

    /**
     * @brief 
     * @param u32DecChlNum 有效通道数
     * @param u8StreamType 码流类型  0-主码流 1-子码流
     **/
    int ChangePreviewChannel( unsigned int u32DecChlNum, int chnlist[_AV_VDEC_MAX_CHN_]
        , char chnMap[SUPPORT_MAX_VIDEO_CHANNEL_NUM], unsigned char szOnlineState[SUPPORT_MAX_VIDEO_CHANNEL_NUM], unsigned char u8StreamType,int realvochn_layout[_AV_VDEC_MAX_CHN_]);

    int StopDecoder( bool bWait );

    int PauseDecoder();

    /**
     * @brief 设置通道解码的帧率
     * @param 
     **/
    int SetFrameRate( int index, unsigned char u8FrameRate );

    int SetFrameRateFromParam( unsigned char szFrameRate[SUPPORT_MAX_VIDEO_CHANNEL_NUM] );

    /**
    * @brief 根据时间通道获取所在解码的位置
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

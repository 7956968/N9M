/*
 * DecDealSyncPlay.h
 *
 *  Created on: 2012-07-24
 *      Author: czzhao
 */

#ifndef AVDECDEALSYNCPLAY_H_
#define AVDECDEALSYNCPLAY_H_

#include <pthread.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "AvCommonFunc.h"
#include "System.h"
#include "CommonMacro.h"
#include "AvDevCommType.h"
#include "MultiStreamSyncParser.h"
#include "AvConfig.h"
#include "CommonDebug.h"
#define MAX_DEC_DEAL_CHN_NUM 16

#define RECORD_SEGMENT_BEGIN   0x01
#define RECORD_SEGMENT_END     0x02

#define GETBASEPTS_FRAME_HOLD 0  /*获取最小pts值保留的帧数据*/
#define GETBASEPTS_FRAME_DISCARD 1/*获取最小pts值丢弃前面无效的帧数据*/

#define STRATEGY_BY_RTC_TIME 1 /*按照RTC做同步参数*/
#define STRATEGY_BY_PTS       0 /*按照PTS做同步参数*/

typedef struct
{
	unsigned long long BaseFramePts;
	unsigned long long LastFramePts;
	unsigned long long BaseSysPts;
	unsigned long long CurSysTime;
	unsigned long long PreSec;
    long long BaseFrameRtcTime;/*基准帧RTC时间*/
	unsigned long long BaseChnPts;
	int FrameNum;
	int RedundantFrCnt;/*Vo滞留未解的帧数*/
	unsigned long long CurTime;
	PlayState_t CurPlayState;
	PlayState_t ExpectPlayState;
	bool RequestIFrame;
	bool StepPlay;
	bool ReSyncPts;/*重新同步PTS*/
}DecSendPara_t;


typedef struct
{
    unsigned long long BaseFramePts;
	long long BaseRtcTime[MAX_DEC_DEAL_CHN_NUM];
	unsigned long long BaseChnFramePts[MAX_DEC_DEAL_CHN_NUM];
}BaseLineInfo_t;

typedef MSSP_GetFrameInfo_t PlaybackFrameinfo_t;

typedef struct
{
    /**
     * @brief 获取帧数据
     * @param-通道索引号,通道号,数据类型(音频or视频),数据buffer, 数据长度,PTS,分段标记(begin or end), 方向
     * @return 0:成功 -1:失败
     */
    boost::function<int(int, int *, DecDataType_t *, unsigned char**, int *, unsigned long long *, unsigned int *, int)>GetFrameBuffer;
    boost::function<int(int, unsigned char**, int *, PlaybackFrameinfo_t*, int)> GetFrameBufferInfo;
	boost::function<int(int,PlaybackFrameinfo_t*)> GetFrameInfo;
    /**
     * @brief 释放帧数据
     * @param-通道索引号
     * @return 0:成功 -1:失败
     */
    boost::function<int(int)>ReleaseFrameData;

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
     * @brief 视频帧放
     * @param-purpose, 通道索引号
     * @return 0:成功 -1:失败
     */
    boost::function<int(int, int)>StepPlayVideo;
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
     * @brief 获取最小的基准PTS
     * @param-PTS值,是否丢帧,获取数据的方向(-1向后 1向前)
     * @return 0:成功;其他:失败
     */
    boost::function<int(unsigned long long *, int, int)>GetBaseLinePts;
    /**
     * @brief获取ao或者vo里面未解码的数据帧数和长度
     * @param-purpose, 通道索引号，帧数，数据长度，数据类型(音频or视频)
     * @return 0:成功 -1:失败
     */
    boost::function<int(int, int, int *, int*, int)> GetDecWaitFrCnt;/*chn, framenum, type*/
    /**
     * @brief 发送数据到解码器解码
     * @param-purpose, 通道索引号，数据缓存，数据长度，数据类型(音频or视频)
     * @return 0:成功 -1:失败
     */
    boost::function<int(int, int, unsigned char *, int, const PlaybackFrameinfo_t*)> SendDataToDecEx;/*chn, buffer, datalen, type*/
} DDSP_Operation_t;

class CDecDealSyncPlay
{
public:
	/**
	 * @brief 
	 * @param ChnNum -播放的通道数
	 * @param ChnNumPerThread-每个线程支持的最大通道数
	 * @param pOperation-回放的回调函数
	 */
	CDecDealSyncPlay( int purpose, int ChnNum, int ChnNumPerThread, DDSP_Operation_t *pOperation );

	~CDecDealSyncPlay();

	void SignalDecDealBody(int ThreadIndex, int startindex, int stopindex);

	void DecMonitorBody();
	/**
	 * @brief 设置帧率
	 * @param Chn -通道掩码，一个bit表示一个通道
	 * @param 
	                PlayFrameRate-回放帧率,
	 		  BaseFrameRate:满帧帧率，根据当前回放的制式填充,NTSC为30,PAL为25
	 * @return true:成功 false:失败
	 */
	bool SetFrameRate(unsigned int ChnMask, int PlayFrameRate, int BaseFrameRate,bool firstset = false);

	/**
	 * @brief 清楚重置基准PTS
	 * @param ChnMask -通道掩码
	 * @return true:成功 false:失败
	 */
	bool ClearPts(unsigned int ChnMask);

	/**
	 * @brief 等待操作
	 * @param ChnMask -通道掩码
	 * @param wait -true:等待；false-执行
	 * @return true:成功 false:失败
	 */
	void WaitToSepcialPlay(unsigned int ChnMask, bool wait);

	/**
	 * @brief 特殊播放操作
	 * @param ChnMask -通道掩码
	 * @param ope -操作代码
	 * @return 无
	 */
	void DecSepcialPlayOperate(unsigned int ChnMask, PlayOperate_t ope);

	/**
	 * @brief切换音频通道
	 * @param index -通道号从0开始
	 * @return 无
	 */
	void SwitchAudioChn(int index);
	

private:
	bool ReSyncAllChn();

    bool GetBaselineInfoByPts(BaseLineInfo_t &BaseInfo, int SyncMethod, int direct);
    	
	/**
	 * @brief 设置基准PTS
	 * @param ChnMask -通道掩码
	 * @param SyncMethod-获取基准PTS值策略
	 * @return true:成功 false:失败
	 */
	bool SyncPts(unsigned int ChnMask, int SyncMethod);

	/**
	 * @brief设置同步标记
	 * @param index -通道号从0开始
	 * @param enable -true-设置同步pts标记;false-清除同步pts标记
	 * @return 无
	 */
	bool SetSyncPtsBmp(int index, bool enable);

	/**
	 * @brief检测回放状态
	 * @param index -通道号从0开始
	 * @return 无
	 */
	void CheckPlayState(int index);

	/**
	 * @brief等待空闲
	 * @param index -通道号从0开始
	 * @param enable -true:等待；false-执行
	 * @return 无
	 */
	int WaitIdle(bool idle, int index);

	/**
	 * @brief检测是否送数据到解码器
	 * @param Chn -通道号从0开始
	 * @param Pts -检测的PTS值
	 * @return 0-发送;1-丢弃;-1-等待
	 */
	int CheckSend(int Chn, unsigned long long PlayPts);

	/**
	 * @brief按通道设置基准PTS和系统时间
	 * @param index -通道号从0开始
	 * @param BaseLinePts -基准PTS值
	 * @param SysTime -基准系统时间?	 * @return true-成功;false-失败
	 */
	bool SyncPtsByChn(int index, unsigned long long BaseLinePts, unsigned long long SysTime, long long BaseRtcTime, unsigned long long BaseChnPts);

	/**
	 * @brief按通道清除基准PTS
	 * @param index -通道号从0开始
	 * @return true-成功;false-失败
	 */
	bool ClearPtsByChn(int index);

	/**
	 * @brief按通道重置基准PTS
	 * @param index -通道号从0开始
	 * @return true-成功;false-失败
	 */
	void ResetPtsByChn(int index);

	/**
	 * @brief重新回放
	 * @param index -通道号从0开始
	 * @return 0-成功;-1-失败
	 */
	int RePlay(int index);

	/**
	 * @brief暂停
	 * @param index -通道号从0开始
	 * @return 0-成功;-1-失败
	 */
	int Pause(int index);

	/**
	 * @brief帧放
	 * @param index -通道号从0开始
	 * @return 0-成功;-1-失败
	 */
	int StepPlay(int index);

	/**
	 * @brief慢放
	 * @param index -通道号从0开始
	 * @return 0-成功;-1-失败
	 */
	int SlowPlay(int index);

	/**
	 * @brief快退
	 * @param index -通道号从0开始
	 * @return 0-成功;-1-失败
	 */
	int FastBackwardPlay(int index);

	/**
	 * @brief快进
	 * @param index -通道号从0开始
	 * @return 0-成功;-1-失败
	 */
	int FastForwardPlay(int index);

	/**
	 * @brief选时
	 * @param index -通道号从0开始
	 * @return 0-成功;-1-失败
	 */
	int Seek(int index);

	/**
	 * @brief设置解码掩码
	 * @param index -通道号从0开始
	 * @return 0-成功;-1-失败
	 */
	bool SetDataChnMask(int index, bool enable);

    unsigned long long CheckSinglePtsDiffer( unsigned long long u64NewPts, unsigned long long u64NewSysMircroSec, int& nSyncBaseChannel );
	
private://read
	int m_PlayFrameRate[MAX_DEC_DEAL_CHN_NUM];
	int m_BaseFrameRate[MAX_DEC_DEAL_CHN_NUM];
	int m_PlayPhyChnList[MAX_DEC_DEAL_CHN_NUM];
	int m_CheckValid[MAX_DEC_DEAL_CHN_NUM];
	int m_ThreadNum;
	unsigned int m_SyncPtsBmp;
	unsigned int m_DecChnMask;
	unsigned int m_DecDataChnMask;/*0-no data;1-have data*/
	int m_ExpectAudioChn;
	int m_AudioChn;
    bool m_NeedClearAudio; /**< 由正常播放到抽帧播放时，需要清理音频缓冲 */

    int m_AvPurpose;

	DecSendPara_t m_DecSendPara[MAX_DEC_DEAL_CHN_NUM];
	DDSP_Operation_t m_DDSPOperate;

	DecThreadPara_t m_ThreadPara[MAX_DEC_DEAL_CHN_NUM];
	DecThreadStatus_t m_DecThreadStatus[MAX_DEC_DEAL_CHN_NUM];

	DecThreadPara_t m_MonitorThreadPara;
	DecThreadStatus_t m_MonitorThreadStatus;
	int m_GetBasePtsDirect;/*获取数据的方向(-1向后 1向前)*/
	int m_ChnNum;
    bool m_SyncRtcOperate;

	bool m_resource_limited_informed; //!< false mean not informed
};


#endif /* AVDECDEALSYNCPLAY_H_ */


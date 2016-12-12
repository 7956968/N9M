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

#define GETBASEPTS_FRAME_HOLD 0  /*��ȡ��Сptsֵ������֡����*/
#define GETBASEPTS_FRAME_DISCARD 1/*��ȡ��Сptsֵ����ǰ����Ч��֡����*/

#define STRATEGY_BY_RTC_TIME 1 /*����RTC��ͬ������*/
#define STRATEGY_BY_PTS       0 /*����PTS��ͬ������*/

typedef struct
{
	unsigned long long BaseFramePts;
	unsigned long long LastFramePts;
	unsigned long long BaseSysPts;
	unsigned long long CurSysTime;
	unsigned long long PreSec;
    long long BaseFrameRtcTime;/*��׼֡RTCʱ��*/
	unsigned long long BaseChnPts;
	int FrameNum;
	int RedundantFrCnt;/*Vo����δ���֡��*/
	unsigned long long CurTime;
	PlayState_t CurPlayState;
	PlayState_t ExpectPlayState;
	bool RequestIFrame;
	bool StepPlay;
	bool ReSyncPts;/*����ͬ��PTS*/
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
     * @brief ��ȡ֡����
     * @param-ͨ��������,ͨ����,��������(��Ƶor��Ƶ),����buffer, ���ݳ���,PTS,�ֶα��(begin or end), ����
     * @return 0:�ɹ� -1:ʧ��
     */
    boost::function<int(int, int *, DecDataType_t *, unsigned char**, int *, unsigned long long *, unsigned int *, int)>GetFrameBuffer;
    boost::function<int(int, unsigned char**, int *, PlaybackFrameinfo_t*, int)> GetFrameBufferInfo;
	boost::function<int(int,PlaybackFrameinfo_t*)> GetFrameInfo;
    /**
     * @brief �ͷ�֡����
     * @param-ͨ��������
     * @return 0:�ɹ� -1:ʧ��
     */
    boost::function<int(int)>ReleaseFrameData;

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
     * @brief ��Ƶ֡��
     * @param-purpose, ͨ��������
     * @return 0:�ɹ� -1:ʧ��
     */
    boost::function<int(int, int)>StepPlayVideo;
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
     * @brief ��ȡ��С�Ļ�׼PTS
     * @param-PTSֵ,�Ƿ�֡,��ȡ���ݵķ���(-1��� 1��ǰ)
     * @return 0:�ɹ�;����:ʧ��
     */
    boost::function<int(unsigned long long *, int, int)>GetBaseLinePts;
    /**
     * @brief��ȡao����vo����δ���������֡���ͳ���
     * @param-purpose, ͨ�������ţ�֡�������ݳ��ȣ���������(��Ƶor��Ƶ)
     * @return 0:�ɹ� -1:ʧ��
     */
    boost::function<int(int, int, int *, int*, int)> GetDecWaitFrCnt;/*chn, framenum, type*/
    /**
     * @brief �������ݵ�����������
     * @param-purpose, ͨ�������ţ����ݻ��棬���ݳ��ȣ���������(��Ƶor��Ƶ)
     * @return 0:�ɹ� -1:ʧ��
     */
    boost::function<int(int, int, unsigned char *, int, const PlaybackFrameinfo_t*)> SendDataToDecEx;/*chn, buffer, datalen, type*/
} DDSP_Operation_t;

class CDecDealSyncPlay
{
public:
	/**
	 * @brief 
	 * @param ChnNum -���ŵ�ͨ����
	 * @param ChnNumPerThread-ÿ���߳�֧�ֵ����ͨ����
	 * @param pOperation-�طŵĻص�����
	 */
	CDecDealSyncPlay( int purpose, int ChnNum, int ChnNumPerThread, DDSP_Operation_t *pOperation );

	~CDecDealSyncPlay();

	void SignalDecDealBody(int ThreadIndex, int startindex, int stopindex);

	void DecMonitorBody();
	/**
	 * @brief ����֡��
	 * @param Chn -ͨ�����룬һ��bit��ʾһ��ͨ��
	 * @param 
	                PlayFrameRate-�ط�֡��,
	 		  BaseFrameRate:��֡֡�ʣ����ݵ�ǰ�طŵ���ʽ���,NTSCΪ30,PALΪ25
	 * @return true:�ɹ� false:ʧ��
	 */
	bool SetFrameRate(unsigned int ChnMask, int PlayFrameRate, int BaseFrameRate,bool firstset = false);

	/**
	 * @brief ������û�׼PTS
	 * @param ChnMask -ͨ������
	 * @return true:�ɹ� false:ʧ��
	 */
	bool ClearPts(unsigned int ChnMask);

	/**
	 * @brief �ȴ�����
	 * @param ChnMask -ͨ������
	 * @param wait -true:�ȴ���false-ִ��
	 * @return true:�ɹ� false:ʧ��
	 */
	void WaitToSepcialPlay(unsigned int ChnMask, bool wait);

	/**
	 * @brief ���ⲥ�Ų���
	 * @param ChnMask -ͨ������
	 * @param ope -��������
	 * @return ��
	 */
	void DecSepcialPlayOperate(unsigned int ChnMask, PlayOperate_t ope);

	/**
	 * @brief�л���Ƶͨ��
	 * @param index -ͨ���Ŵ�0��ʼ
	 * @return ��
	 */
	void SwitchAudioChn(int index);
	

private:
	bool ReSyncAllChn();

    bool GetBaselineInfoByPts(BaseLineInfo_t &BaseInfo, int SyncMethod, int direct);
    	
	/**
	 * @brief ���û�׼PTS
	 * @param ChnMask -ͨ������
	 * @param SyncMethod-��ȡ��׼PTSֵ����
	 * @return true:�ɹ� false:ʧ��
	 */
	bool SyncPts(unsigned int ChnMask, int SyncMethod);

	/**
	 * @brief����ͬ�����
	 * @param index -ͨ���Ŵ�0��ʼ
	 * @param enable -true-����ͬ��pts���;false-���ͬ��pts���
	 * @return ��
	 */
	bool SetSyncPtsBmp(int index, bool enable);

	/**
	 * @brief���ط�״̬
	 * @param index -ͨ���Ŵ�0��ʼ
	 * @return ��
	 */
	void CheckPlayState(int index);

	/**
	 * @brief�ȴ�����
	 * @param index -ͨ���Ŵ�0��ʼ
	 * @param enable -true:�ȴ���false-ִ��
	 * @return ��
	 */
	int WaitIdle(bool idle, int index);

	/**
	 * @brief����Ƿ������ݵ�������
	 * @param Chn -ͨ���Ŵ�0��ʼ
	 * @param Pts -����PTSֵ
	 * @return 0-����;1-����;-1-�ȴ�
	 */
	int CheckSend(int Chn, unsigned long long PlayPts);

	/**
	 * @brief��ͨ�����û�׼PTS��ϵͳʱ��
	 * @param index -ͨ���Ŵ�0��ʼ
	 * @param BaseLinePts -��׼PTSֵ
	 * @param SysTime -��׼ϵͳʱ��?	 * @return true-�ɹ�;false-ʧ��
	 */
	bool SyncPtsByChn(int index, unsigned long long BaseLinePts, unsigned long long SysTime, long long BaseRtcTime, unsigned long long BaseChnPts);

	/**
	 * @brief��ͨ�������׼PTS
	 * @param index -ͨ���Ŵ�0��ʼ
	 * @return true-�ɹ�;false-ʧ��
	 */
	bool ClearPtsByChn(int index);

	/**
	 * @brief��ͨ�����û�׼PTS
	 * @param index -ͨ���Ŵ�0��ʼ
	 * @return true-�ɹ�;false-ʧ��
	 */
	void ResetPtsByChn(int index);

	/**
	 * @brief���»ط�
	 * @param index -ͨ���Ŵ�0��ʼ
	 * @return 0-�ɹ�;-1-ʧ��
	 */
	int RePlay(int index);

	/**
	 * @brief��ͣ
	 * @param index -ͨ���Ŵ�0��ʼ
	 * @return 0-�ɹ�;-1-ʧ��
	 */
	int Pause(int index);

	/**
	 * @brief֡��
	 * @param index -ͨ���Ŵ�0��ʼ
	 * @return 0-�ɹ�;-1-ʧ��
	 */
	int StepPlay(int index);

	/**
	 * @brief����
	 * @param index -ͨ���Ŵ�0��ʼ
	 * @return 0-�ɹ�;-1-ʧ��
	 */
	int SlowPlay(int index);

	/**
	 * @brief����
	 * @param index -ͨ���Ŵ�0��ʼ
	 * @return 0-�ɹ�;-1-ʧ��
	 */
	int FastBackwardPlay(int index);

	/**
	 * @brief���
	 * @param index -ͨ���Ŵ�0��ʼ
	 * @return 0-�ɹ�;-1-ʧ��
	 */
	int FastForwardPlay(int index);

	/**
	 * @briefѡʱ
	 * @param index -ͨ���Ŵ�0��ʼ
	 * @return 0-�ɹ�;-1-ʧ��
	 */
	int Seek(int index);

	/**
	 * @brief���ý�������
	 * @param index -ͨ���Ŵ�0��ʼ
	 * @return 0-�ɹ�;-1-ʧ��
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
    bool m_NeedClearAudio; /**< ���������ŵ���֡����ʱ����Ҫ������Ƶ���� */

    int m_AvPurpose;

	DecSendPara_t m_DecSendPara[MAX_DEC_DEAL_CHN_NUM];
	DDSP_Operation_t m_DDSPOperate;

	DecThreadPara_t m_ThreadPara[MAX_DEC_DEAL_CHN_NUM];
	DecThreadStatus_t m_DecThreadStatus[MAX_DEC_DEAL_CHN_NUM];

	DecThreadPara_t m_MonitorThreadPara;
	DecThreadStatus_t m_MonitorThreadStatus;
	int m_GetBasePtsDirect;/*��ȡ���ݵķ���(-1��� 1��ǰ)*/
	int m_ChnNum;
    bool m_SyncRtcOperate;

	bool m_resource_limited_informed; //!< false mean not informed
};


#endif /* AVDECDEALSYNCPLAY_H_ */


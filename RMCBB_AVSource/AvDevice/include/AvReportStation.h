/*!
 *********************************************************************
 *Copyright (C), 2015-2015, Streamax Tech. Co., Ltd.
 *All rights reserved.
 *
 * \file         AvReportStation.h
 * \brief       structs and class for MP3 function.
 *
 * \Created  Jan 15, 2015 by shliu
 */
#ifndef __AVREPORTSTATION_H__
#define __AVREPORTSTATION_H__

#include "Defines.h"
#include "Message.h"
#include <stdio.h>
#include <list.h>
#include "mpi_ao.h"
#include "AvAppMaster.h"
#include "mp3dec.h"
#if defined(_AV_PLATFORM_HI3515_)
#include "HiAo-3515.h"
#if defined(_AV_HAVE_VIDEO_DECODER_)
#include "HiAvDec-3515.h"
#endif
#else
#include "HiAo.h"
#endif
#include "AvThreadLock.h"
enum STATION_THREAD_RUN_STATE{ STATION_THREAD_STOP, STATION_THREAD_START, STATION_THREAD_EXIT };
//max length of MP3 stream by bytes 
#define MAX_MP3_MAINBUF_SIZE    2048*2
#define MP3_SAMPLES_PER_FRAME   1152

typedef enum hiMP3_BPS_E
{
    MP3_BPS_32K     = 32000,
    MP3_BPS_40K     = 40000,
    MP3_BPS_48K     = 48000,
    MP3_BPS_56K     = 56000,
    MP3_BPS_64K     = 64000,
    MP3_BPS_80K     = 80000,
    MP3_BPS_96K     = 96000,
    MP3_BPS_112K    = 112000,
    MP3_BPS_128K    = 128000,
    MP3_BPS_160K    = 160000,
    MP3_BPS_192K    = 192000,
    MP3_BPS_224K    = 224000,
    MP3_BPS_256K    = 256000,
    MP3_BPS_320K    = 320000,
} MP3_BPS_E;

typedef enum hiMP3_LAYER_E
{
    MP3_LAYER_1     = 1,
    MP3_LAYER_2     = 2,
    MP3_LAYER_3     = 3,
} MP3_LAYER_E;

typedef enum hiMP3_VERSION_E
{
    MPEG_1          =1,
    MPEG_2          =0,
    MPEG_25         =2,
} MP3_VERSION_E;

typedef struct hiMP3_FRAME_INFO_S 
{
    HI_S32 s32BitRate;
    HI_S32 s32Chans;             /* 1 or 2 */
    HI_S32 s32SampRate;
    HI_S32 s32BitsPerSample;      /* only support 16 bits */
    HI_S32 s32OutPutSamps;      /* nChans*SamplePerFrame */ 
    HI_S32 s32Layer;            /* layer,the infomation of stream's encode profile */
    HI_S32 s32Version;              /* version,the infomation of stream's encode profile */
} MP3_FRAME_INFO_S;

typedef struct hiAENC_ATTR_MP3_S
{
    MP3_BPS_E           enBitRate;     /* MP3 bitrate 32~320)*/
    AUDIO_SAMPLE_RATE_E enSmpRate;     /* MP3 sample rate 32,44.1,48*/
    AUDIO_BIT_WIDTH_E   enBitWidth;    /* MP3 bit width (only support 16bit)*/
    AUDIO_SOUND_MODE_E  enAiSoundMode;     /* sound mode of AI input,MP3 Encoder will convert mono data to stereo data*/
    MP3_LAYER_E         enLayer;   /* Layer of MP3 Encoder (only support 3)*/
    MP3_VERSION_E       enVersion; /* Version of MP3 Encoder (only support MPEG1)*/
} AENC_ATTR_MP3_S;

typedef struct hiADEC_ATTR_MP3_S
{
    HI_U32 resv;
} ADEC_ATTR_MP3_S;

typedef struct hiMP3_DECODER_S
{
    ADEC_ATTR_MP3_S     stMP3Attr;
    HMP3Decoder         pstMP3State;
} MP3_DECODER_S;

class CAvThreadLock;
class CAvReportStation
{

public:
    CAvReportStation();
    ~CAvReportStation();

public:
	int StartReportStationThread();
	int StopReportStationThread();
	int ReportStationThreadBody();
    int AddReportStationTask( char * addr, int RightLeftChn);
	inline int GetReportStationResult() const {return m_iLeftByte;}
    bool ClearListTask();
    bool SetTaskRuningState(bool brun);
	int SetAudioAoChn(const int rightleftchn);
	inline bool GetReportState()const {return m_bIsTaskRunning;}
    inline bool SetInteruptTask(bool bTask) {m_bInteruptTask = bTask;return true;}
    inline bool SetLoopTask(bool bTask) {m_bLoopTask = bTask;return true;} //!< 
private:
	int InitAudioOutput(AUDIO_DEV AudioDevId, AO_CHN AoChn, MP3FrameInfo *mp3FrameInfo);
	int UninitAudioOutput(AUDIO_DEV AudioDevId, AO_CHN AoChn);
	int CloseMP3Decoder(HMP3Decoder hMP3Decoder);
	HMP3Decoder OpenMP3Decoder();
	int StartMP3Decoder(HMP3Decoder hMP3Decoder, HI_U8 **ppInbufPtr, HI_S32 *pBytesLeft, HI_S16 *pOutPcm, MP3FrameInfo *mp3FrameInfo);
	int PlayRawData(AUDIO_DEV AudioDevId, AO_CHN AoChn, MP3FrameInfo *pInfo, HI_U16 *pBuf);
	int FillReadBuffer(unsigned char *pReadBuf, unsigned char *pReadPtr, int BufSize, int BytesLeft, FILE *pFile/*, bool bufflag*/ );
	FILE* OpenPlayFile(const std::string pFileName);
    bool GetReportStationTask(std::string *addr);
	inline int GetAudioAoChn() const {return m_RightLeftChn;}
	int SetReportStationResult(int leftbyte);
	static bool ListSortCompareByLevel (msgReportStation_t first, msgReportStation_t second);
	

private:
	bool m_bNeedExitThread;
    bool m_bIsTaskRunning;
	bool m_bInteruptTask;
	bool m_bLoopTask; //!< for phone call   0307
    std::list<std::string> listReportStation;
	STATION_THREAD_RUN_STATE m_ePlayState;
    MP3FrameInfo mp3LastFrameInfo;
	bool m_isFinish;
	int m_RightLeftChn;
	int m_iLeftByte;//²éÑ¯²¥·Å×´Ì¬
	bool m_binit;
	CAvThreadLock *m_pThread_lock;
	CHiAo* m_ptrHi_Ao;
#if defined(_AV_HAVE_VIDEO_DECODER_) && defined(_AV_PLATFORM_HI3515_)
	CHiAvDec * m_pHi_adec;
#endif
};//

#endif //

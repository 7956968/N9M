/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __AVCOMMONFUNC_H__
#define __AVCOMMONFUNC_H__
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include "pthread.h"
#include "System.h"


#define EXTERN_FRAMEINFO_VIDEO_HEADER 0x1
#define EXTERN_FRAMEINFO_VIDEO_VIRTUALTIME 0x2
#define EXTERN_FRAMEINFO_VIDEO_VIDEOINFO 0x4
#define EXTERN_FRAMEINFO_VIDEO_RTCTIME 0x8
#define EXTERN_FRAMEINFO_AUDIO_INFO 0x10
#define EXTERN_FRAMEINFO_VIDEO_STATE 0x20
#define EXTERN_FRAMEINFO_VIDEO_REF 0x40

typedef struct{
	unsigned int Mask;
	RMSTREAM_HEADER *pHeader;
	RMFI2_VTIME *pVTime;
	RMFI2_VIDEOINFO *pVideoInfo;
	RMFI2_RTCTIME *pRtcTime;
	RMFI2_AUDIOINFO *pAudioInfo;
    RMFI2_VIDEOSTATE *pVideoState;
    RMFI2_VIDEOREFINFO *pVideoRefInfo;
}Stm_ExternFrameInfo_t;

typedef void *(*thread_entry_ptr_t)(void *);
int Av_create_normal_thread(thread_entry_ptr_t entry, void *pPara, pthread_t *pPid);
/**
	 * @brief 基线获取扩展帧头信息
	 * @param pBuffer -数据
	 * @param DataLen -数据长度
	 * @param stuExternVFInfo -扩展帧信息
	 * @return 0:成功 -1:失败
	 */
int Get_extern_frameinfo(unsigned char * pBuffer, int DataLen, Stm_ExternFrameInfo_t & stuExternVFInfo);
/**
 * @brief 获取当前系统时间
 * @return 时间，单位为微妙
 */
unsigned long long Get_system_ustime(void);

void mSleep(unsigned int MilliSecond);

/**
 * @brief 根据宽高获取分辨率值
 * @return 分辨率值
 */
int Get_Resvalue_By_Size(int width, int height);

#endif/*__AVCOMMONFUNC_H__*/


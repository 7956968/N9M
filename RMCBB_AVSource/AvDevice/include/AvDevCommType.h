/*
 * AvDevCommType.h
 *
 *  Created on: 2013-03-05
 *      Author: czzhao
 */

#ifndef AVDEVCOMMTYPE_H_
#define AVDEVCOMMTYPE_H_

#include <pthread.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "AvCommonFunc.h"
#include "System.h"
#include "CommonMacro.h"

#define PTS_INIT_VALUE 0xffffffff
#define _VDEC_MAX_VDP_NUM_  3

#define _SV_VL_STATUS_ 0x1
#define _SV_MD_STATUS_ 0x2

#define _SV_OD_STATUS_ (0x4)



typedef enum
{
	PO_PLAY,
	PO_PAUSE,
	PO_SLOWPLAY,
	PO_STEP,
	PO_FASTFORWARD,
	PO_FASTBACKWARD,
	PO_SEEK,
	PO_FASTPLAY,
	PO_INVALID
} PlayOperate_t;

typedef enum
{
	PS_INIT,
	PS_NORMAL,
	PS_PAUSE,
	PS_PRESTEPPLAY,
	PS_STEP,
	PS_SLOWPLAY2X,
	PS_SLOWPLAY4X,
	PS_SLOWPLAY8X,
	PS_SLOWPLAY16X,
	PS_FASTFORWARD2X,
	PS_FASTFORWARD4X,
	PS_FASTFORWARD8X,
	PS_FASTFORWARD16X,
	PS_FASTBACKWARD2X,
	PS_FASTBACKWARD4X,
	PS_FASTBACKWARD8X,
	PS_FASTBACKWARD16X,
	PS_SEEK,
	PS_STOP,
	PS_INVALID,
} PlayState_t;

typedef struct{
	bool DecThreadRun;
	bool DecThreadExit;
	bool DecThreadIdle;
	bool DecThreadFlag;
    bool DecForceExit;
}DecThreadStatus_t;

typedef enum
{
	DECDATA_IFRAME = 0,
	DECDATA_PFRAME,
	DECDATA_AFRAME,
	DECDATA_VIDEO,
	DECDATA_INVALID
}DecDataType_t;

typedef struct
{
	int StartChnId;
	int StopChnId;
	int ThreadIndex;
	int Pid;
	void *DecClass;
}DecThreadPara_t;

typedef struct
{
	unsigned int motioned;
	unsigned int videoLost;
	unsigned int videoMask;
} Av_video_state_t;

enum enumAvStreamReadyDefines
{
	
	AVSTREAM_VIDEOSTAND_SET = 0x01,
		
	AVSTREAM_MAINSTREAM_ENCODER_START = 0x01<<1,

	AVSTREAM_SUBSTREAM_ENCODER_START = 0x01<<2,

	AVSTREAM_MOBILESTREAM_ENCODER_START = 0x01<<3,

	AVSTREAM_MOTIONDETECT_START = 0x01<<4,

	AVSTREAM_VIDEOMASK_START = 0x01<<5,

	AVSTREAM_OVERLAYNAME_START = 0x01<<6,

	AVSTREAM_OVERLAYTIME_START = 0x01<<7,

	AVSTREAM_VIDEODECODER_START = 0x01<<8,

	AVSTREAM_ALARMDETECT_START = 0x01<<9,
};

#endif /* AVDEVCOMMTYPE_H_ */

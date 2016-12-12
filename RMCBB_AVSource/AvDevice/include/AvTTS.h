/*!
 *********************************************************************
 *Copyright (C), 2015-2015, Streamax Tech. Co., Ltd.
 *All rights reserved.
 *
 * \file         AvTTS.h
 * \brief       structs and class for TTS function.
 *
 * \Created  Jan 21, 2015 by Devz
 */

#ifndef _AV_TTS_H
#define _AV_TTS_H

#ifndef _IN
#define _IN 
#endif

#ifndef _OUT
#define _OUT
#endif

#include <list>
#include "Message.h"
#if defined(_AV_PLATFORM_HI3515_)
#include "HiAo-3515.h"
#if defined(_AV_HAVE_VIDEO_DECODER_)
#include "HiAvDec-3515.h"
#endif
#else
#include "HiAo.h"
#include "AvThreadLock.h"
#endif

typedef struct _tts_info_
{
	char tts_inited;      //!< avoid repeated init
	char tts_completed;
	char tts_synth_running;
	char tts_send_running;
	enumAudioPlayState tts_state;  //if start failed , state set finish
}tts_info_t;

typedef struct _ao_info_
{
	char ao_dev;
	char ao_chn;
	char ao_on; // control the output of real-time tts result, 1 active,
}ao_info_t;

typedef struct _tts_result_
{
	int result_stat;  //! < indicate the status of synth result  1, complete, 0 not 
	int voice_size;    //!< synthed result size each time
	int voice_num;   //!< the number of synthed data by the unit of callback
	unsigned char * voice_data; //!< 
}tts_result_t;
class CAvTTS
{
public:
	CAvTTS        ();
	~CAvTTS      ();
	int Load_file(const char * file_name, int & len);
	int Unload(void);
	
	int Init_tts     ();
	int Uninit_tts   ();

	int TTS_SynthThread_body();

	int TTS_SendThread_body();
	
	int Start_tts   (msgTTSPara_t * tts_param);        //!< 
	int Stop_tts    ();
	int Obtain_tts_status(){return m_tts_info.tts_state;}

	int Start_ao    ();
	int Stop_ao    ();
#if defined(_AV_PLATFORM_HI3515_) && defined(_AV_HAVE_VIDEO_DECODER_)
	int Start_adec_ao(int adec_chn,eAudioPayloadType type);
	int Destroy_adec_ao(int adec_chn);
#endif
    inline bool SetTTSLoopState(bool state){m_bLoopState = state;return true;}
private:

	bool TTSSynth(const string &capkey, const char* pszText);


private:
	bool m_run_tts;
	bool m_halt_tts;                           //!< halt tts during its running imediately

	tts_info_t m_tts_info;
	ao_info_t m_ao_info;

	std::list<msgTTSPara_t> m_tts_param_it;   //!< list of messages to adapt to demandes that vary
	msgTTSPara_t *m_tts_param_cur;             //! <current message

	CHiAo * m_pHi_ao;
#if defined(_AV_PLATFORM_HI3515_) && defined(_AV_HAVE_VIDEO_DECODER_)
	CHiAvDec * m_pHi_adec;
#endif

	CAvThreadLock * m_pThread_lock;
    bool m_bLoopState;
	unsigned char * m_pBuf;
	std::list<tts_result_t> m_tts_result_it;
	
	tts_result_t m_tts_result; //!< store the result of each time

	bool m_send_thread_state; //!<  0, not running ;1, running 
	
};//end class CAvTTS
	
#endif //_AV_TTS_H
	
	
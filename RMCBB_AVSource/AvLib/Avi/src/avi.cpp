/*
 * avi.cpp
 *
 *  Created on: 2012-03-12
 *      Author: gami
 */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "avi.h"
#include "aviriff.h"

#include "g711.h"

/*********ADPCM**********/
#ifndef __STDC__
#define signed
#endif

/* Intel ADPCM step variation table */
static int indexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
};

static int stepsizeTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

#if 0
/*-------------adpcm_ecoder---------------------------------------------*/
void adpcm_coder(short indata[], char outdata[], int len, struct adpcm_state *state)
{
    short *inp;   /* Input buffer pointer */
    signed char *outp;  /* output buffer pointer */
    int val;   /* Current input sample value */
    int sign;   /* Current adpcm sign bit */
    int delta;   /* Current adpcm output value */
    int diff;   /* Difference between val and valprev */
    int step;   /* Stepsize */
    int valpred;  /* Predicted output value */
    int vpdiff;   /* Current change to valpred */
    int index;   /* Current step change index */
    int outputbuffer;  /* place to keep previous 4-bit value */
    int bufferstep;  /* toggle between outputbuffer/output */

    outp = (signed char *)outdata;
    inp = indata;

    valpred = state->valprev;
    index = state->index;
    step = stepsizeTable[index];

    bufferstep = 1;

    for ( ; len > 0 ; len-- )
    {
         val = *inp++;

         /* Step 1 - compute difference with previous value */
         diff = val - valpred;
         sign = (diff < 0) ? 8 : 0;
         if ( sign ) diff = (-diff);

         /* Step 2 - Divide and clamp */
         /* Note:
         ** This code *approximately* computes:
         **    delta = diff*4/step;
         **    vpdiff = (delta+0.5)*step/4;
         ** but in shift step bits are dropped. The net result of this is
         ** that even if you have fast mul/div hardware you cannot put it to
         ** good use since the fixup would be too expensive.
         */
         delta = 0;
         vpdiff = (step >> 3);
         
         if ( diff >= step ) {
             delta = 4;
             diff -= step;
             vpdiff += step;
         }
         step >>= 1;
         if ( diff >= step  ) {
             delta |= 2;
             diff -= step;
             vpdiff += step;
         }
         step >>= 1;
         if ( diff >= step ) {
             delta |= 1;
             vpdiff += step;
         }

         /* Step 3 - Update previous value */
         if ( sign )
           valpred -= vpdiff;
         else
           valpred += vpdiff;

         /* Step 4 - Clamp previous value to 16 bits */
         if ( valpred > 32767 )
           valpred = 32767;
         else if ( valpred < -32768 )
           valpred = -32768;

         /* Step 5 - Assemble value, update index and step values */
         delta |= sign;
         
         index += indexTable[delta];
         if ( index < 0 ) index = 0;
         if ( index > 88 ) index = 88;
         step = stepsizeTable[index];

         /* Step 6 - Output value */
         if ( bufferstep ) {
             outputbuffer = (delta << 4) & 0xf0;
         } else {
             *outp++ = (delta & 0x0f) | outputbuffer;
         }
         bufferstep = !bufferstep;
    }

    /* Output last step, if needed */
    if ( !bufferstep )
      *outp++ = outputbuffer;

    state->valprev = valpred;
    state->index = index;
}


/*-------------adpcm_decoder---------------------------------------------*/
int adpcm_decoder(char indata[], short outdata[], int len, struct adpcm_state *state)
{
    signed char *inp;  /* Input buffer pointer */
    short *outp;  /* output buffer pointer */
    int sign;   /* Current adpcm sign bit */
    int delta;   /* Current adpcm output value */
    int step;   /* Stepsize */
    int valpred;  /* Predicted value */
    int vpdiff;   /* Current change to valpred */
    int index;   /* Current step change index */
    int inputbuffer;  /* place to keep next 4-bit value */
    int bufferstep;  /* toggle between inputbuffer/input */
    int outdatalen;

    outp = outdata;
    inp = (signed char *)indata;

    valpred = state->valprev;
    index = state->index;
    step = stepsizeTable[index];

    bufferstep = 0;

    /*add by gami 201109-27*/
    len *= 2;
    outdatalen = len*2;
    ///
    for ( ; len > 0 ; len-- )
    {
         /* Step 1 - get the delta value */
         if ( bufferstep )
         {
             delta = inputbuffer & 0xf;
         }
         else 
         {
             inputbuffer = *inp++;
             delta = (inputbuffer >> 4) & 0xf;
         }
         bufferstep = !bufferstep;

         /* Step 2 - Find new index value (for later) */
         index += indexTable[delta];
         if ( index < 0 )
         {
            index = 0;
         }
         
         if ( index > 88 )
         {
            index = 88;
         }

         /* Step 3 - Separate sign and magnitude */
         sign = delta & 8;
         delta = delta & 7;

         /* Step 4 - Compute difference and new predicted value */
         /*
         ** Computes 'vpdiff = (delta+0.5)*step/4', but see comment
         ** in adpcm_coder.
         */
         vpdiff = step >> 3;
         if ( delta & 4 )
         {
             vpdiff += step;
         }
         
         if ( delta & 2 )
         {
             vpdiff += step>>1;
         }
         
         if ( delta & 1 )
         {
             vpdiff += step>>2;
         }

         if ( sign )
         {
             valpred -= vpdiff;
         }
         else
         {
             valpred += vpdiff;
         }

         /* Step 5 - clamp output value */
         if ( valpred > 32767 )
         {
             valpred = 32767;
         }
         else if ( valpred < -32768 )
         {
             valpred = -32768;
         }

         /* Step 6 - Update step value */
         step = stepsizeTable[index];

         /* Step 7 - Output value */
         *outp++ = valpred;
    }

    state->valprev = valpred;
    state->index = index;

    return outdatalen;
}
#endif

/*************************************************************/

/*���ļ��ĵ�λ*/
#define READ_FILE_UNIT   (64*1024)
//#define MIN_FRAME_SIZE        24
#define INDEX_BUF_LEN    (4*1024)

CAVI::CAVI()
{
	m_pPCMData = NULL;

	m_FileOperate.NtfsOpen = NULL;
	m_FileOperate.Open = open;
	m_FileOperate.Read = read;
	m_FileOperate.Write = write;
	m_FileOperate.Seek = lseek;
	m_FileOperate.Close = close;
	m_FileOperate.Remove = remove;

	m_FSIType = AVI_FSI_STANDARD;

    m_pPCMData = (short *)malloc(4*1024);
    if(m_pPCMData == NULL)
    {
        DEBUG_ERROR("CAVI failed:out of memory\n");
    }

    DEBUG_MESSAGE("\n\nRecordStream2AVI VERSION:V1.0  %s %s\n\n", __DATE__, __TIME__);
}

CAVI::~CAVI()
{
    if(m_pPCMData != NULL)
    {
    	free(m_pPCMData);
    	m_pPCMData = NULL;
    }
}

/*ADPCM*/
int CAVI::AdpcmDecoderInit()
{
    //m_AdpcmState.index = 0;
    //m_AdpcmState.valprev = 0;
    m_AdpcmState.index = 50;
    m_AdpcmState.valprev = 2167;
    /*
    m_pPCMData = (short *)malloc(4*1024);
    if(m_pPCMData == NULL)
    {
        DEBUG_ERROR("AdpcmDecoderInit failed:out of memory\n");

        return -1;
    }
    */

    return 0;
}

int CAVI::AdpcmDecoder(char indata [ ], short outdata [ ], int len)
{
    signed char *inp;  /* Input buffer pointer */
    short *outp;  /* output buffer pointer */
    int sign;   /* Current adpcm sign bit */
    int delta;   /* Current adpcm output value */
    int step;   /* Stepsize */
    int valpred;  /* Predicted value */
    int vpdiff;   /* Current change to valpred */
    int index;   /* Current step change index */
    int inputbuffer;  /* place to keep next 4-bit value */
    int bufferstep;  /* toggle between inputbuffer/input */
    int outdatalen;

    outp = outdata;
    inp = (signed char *)indata;

    valpred = m_AdpcmState.valprev;
    index = m_AdpcmState.index;
    step = stepsizeTable[index];

    bufferstep = 0;

    /*add by gami 201109-27*/
    len *= 2;
    outdatalen = len*2;
    ///
    for ( ; len > 0 ; len-- )
    {
		 /* Step 1 - get the delta value */
		 if ( bufferstep )
		 {
			 delta = inputbuffer & 0xf;
		 }
		 else
		 {
			 inputbuffer = *inp++;
			 delta = (inputbuffer >> 4) & 0xf;
		 }
		 bufferstep = !bufferstep;

		 /* Step 2 - Find new index value (for later) */
		 index += indexTable[delta];
		 if ( index < 0 )
		 {
			index = 0;
		 }
         
		 if ( index > 88 )
		 {
			index = 88;
		 }

		 /* Step 3 - Separate sign and magnitude */
		 sign = delta & 8;
		 delta = delta & 7;

         /* Step 4 - Compute difference and new predicted value */
         /*
         ** Computes 'vpdiff = (delta+0.5)*step/4', but see comment
         ** in adpcm_coder.
         */
		 vpdiff = step >> 3;
		 if ( delta & 4 )
		 {
			 vpdiff += step;
		 }
         
		 if ( delta & 2 )
		 {
			 vpdiff += step>>1;
		 }

		 if ( delta & 1 )
		 {
			 vpdiff += step>>2;
		 }

		 if ( sign )
		 {
			 valpred -= vpdiff;
		 }
		 else
		 {
			 valpred += vpdiff;
		 }

		 /* Step 5 - clamp output value */
		 if ( valpred > 32767 )
		 {
			 valpred = 32767;
		 }
		 else if ( valpred < -32768 )
		 {
			 valpred = -32768;
		 }

         /* Step 6 - Update step value */
         step = stepsizeTable[index];

         /* Step 7 - Output value */
         *outp++ = valpred;
    }

    m_AdpcmState.valprev = valpred;
    m_AdpcmState.index = index;

    return outdatalen;
}

int CAVI::AdpcmDecoderDestroy()
{
	/*
    if(m_pPCMData != NULL)
    {
        free(m_pPCMData);
    }

    m_pPCMData = NULL;
    */
    return 0;
}
/*******/


void CAVI::UpdateAVIFrameRate( float *pfVideoFPS )
{
	if ( m_rsinfo.videoinfo.norm == 1 )
	{
		// NTSC
		if ( *pfVideoFPS == 30.000f )
			*pfVideoFPS = 29.970f;
			//*pfVideoFPS = 29.975f;//fuhan同步问题修改
	}
	else
	{
		// PAL
		if ( 0 )
		{
		}
		else if ( *pfVideoFPS == 1.000f )
			*pfVideoFPS = 0.995f;				// 0.96386192017259978425026968716289
		//else if ( *pfVideoFPS == 9.000f )
		//	*pfVideoFPS = 9.000f;				// 9.0053908355795148247978436657682
	}
}

int CAVI::SetRecordStreamInfo(rs2avi_streaminfo_t *pRsInfo)
{
	if(pRsInfo == NULL)
	{
		DEBUG_ERROR("CRecordStream2AVI::SetRecordStreamInfo err:pRsInfo is null\n");
		return -1;
	}

    memcpy(&m_rsinfo, pRsInfo, sizeof(rs2avi_streaminfo_t));

    DEBUG_MESSAGE("video info: chno:%d  norm:%d  resolution:%d  fps:%f\n", m_rsinfo.videoinfo.chno, m_rsinfo.videoinfo.norm, m_rsinfo.videoinfo.resolution, m_rsinfo.videoinfo.framerate);
    DEBUG_MESSAGE("audio into: chno:%d  payloadtype:%d bitwidth:%d  samplerate:%d\n", m_rsinfo.audioinfo.chno, m_rsinfo.audioinfo.payloadtype, m_rsinfo.audioinfo.bitwidth, m_rsinfo.audioinfo.samplerate);

    m_videoframenum = 0;
    m_audioframenum = 0;

    return 0;
}

int CAVI::SetFSInterface(int type, rs2avi_FileOperation_t *pFileOperate)
{
	if(type == AVI_FSI_STANDARD)
	{
		m_FileOperate.NtfsOpen = NULL;
		m_FileOperate.Open = open;
		m_FileOperate.Read = read;
		m_FileOperate.Write = write;
		m_FileOperate.Seek = lseek;
		m_FileOperate.Close = close;
		m_FileOperate.Remove = remove;
	}
	else if(type == AVI_FSI_NTFS)
	{
		if(pFileOperate == NULL)
		{
			DEBUG_ERROR("CRecordStream2AVI::SetFSInterface err:pFSOperate is null\n");
			return -1;
		}
		else
		{
			memcpy(&m_FileOperate, pFileOperate, sizeof(m_FileOperate));
		}
	}

	if(type == AVI_FSI_STANDARD)
	{
		if(m_FileOperate.Open==NULL || m_FileOperate.Read==NULL || m_FileOperate.Write==NULL || m_FileOperate.Seek==NULL ||
				m_FileOperate.Close==NULL || m_FileOperate.Remove==NULL)
		{
			DEBUG_ERROR("CRecordStream2AVI::SetFSInterface err!!!!!!!!!!\n");
			fflush(stdout);
			exit(0);
			return -1;
		}
	}
	else
	{
		if(m_FileOperate.NtfsOpen==NULL || m_FileOperate.Read==NULL || m_FileOperate.Write==NULL || m_FileOperate.Seek==NULL ||
				m_FileOperate.Close==NULL || m_FileOperate.Remove==NULL)
		{
			DEBUG_ERROR("CRecordStream2AVI::SetFSInterface err!!!!!!!!!!\n");
			fflush(stdout);
			exit(0);
			return -1;
		}
	}

	m_FSIType = type;

	return 0;
}

int CAVI::InitAVIFileHeader(bool bStreamVideo, bool bStreamAudio, int FileHandle, unsigned long FrameCounter,
		                           const unsigned long VideoTypeForFcc)
{
	UpdateAVIFrameRate( &(m_rsinfo.videoinfo.framerate));

	RIFFLIST RiffList;
	RIFFLIST RiffInfoChunk;							// ��Ϣ���LIST
	RIFFLIST RiffStreamVideo;
	RIFFLIST RiffStreamAudio;
	AVIMAINHEADER	AviMainHdr;
	unsigned long	OffsetRiffInfo;							// List��Ϣ���ƫ��λ��?
	unsigned long	Temp;
	bool			bExtra4Bytes = true;

	AVISTREAMHEADER	StreamHdrVideo;
	AVISTREAMHEADER	StreamHdrAudio;

	BITMAPINFO		BmpInfo;
	WAVEFORMATEX	WavFmtEx;

	int VideoWidth, VideoHeight;

	switch(m_rsinfo.videoinfo.resolution)
	{
		case 0://CIF
			if(m_rsinfo.videoinfo.norm == 0)//PAL
			{
			   VideoWidth = 352;
			   VideoHeight = 288;
			}
			else
			{
			   VideoWidth = 352;
			   VideoHeight = 240;
			}
		   break;
		case 1://HD1
			if(m_rsinfo.videoinfo.norm == 0)//PAL
			{
			   VideoWidth = 704;
			   VideoHeight = 288;
			}
			else
			{
			   VideoWidth = 704;
			   VideoHeight = 240;
			}
		   break;
		case 2://D1
			if(m_rsinfo.videoinfo.norm == 0)//PAL
			{
			   VideoWidth = 704;
			   VideoHeight = 576;
			}
			else
			{
			   VideoWidth = 704;
			   VideoHeight = 480;
			}
		   break;
		case 6://720P
			VideoWidth = 1280;
			VideoHeight = 720;
			break;
		case 7://1080P
			VideoWidth = 1920;
			VideoHeight = 1080;
			break;

		case 10://10 WQCIF 
			VideoWidth = 232;
			VideoHeight = (m_rsinfo.videoinfo.norm == 0)?144:120;
			break;
		case 11://11 WCIF 
			VideoWidth = 464;
			VideoHeight = (m_rsinfo.videoinfo.norm == 0)?288:240;
			break;
		case 12://12 WHD1 
			VideoWidth = 928;
			VideoHeight = (m_rsinfo.videoinfo.norm == 0)?288:240;
			break;
		case 13://13 WD1(960H)
			VideoWidth = 928;
			VideoHeight = (m_rsinfo.videoinfo.norm == 0)?576:480;
			break;
		default:
		       DEBUG_ERROR("CRecordStream2AVI::InitAVIFileHeader err:not supported resolution:%d\n", m_rsinfo.videoinfo.resolution);
		       return -1;
		       break;
	}

	// 1. �ļ�ͷ
	RiffList.fcc			= FCC("RIFF");
	RiffList.cb			    = FCC("size");						// ���ļ�����֮���ٽ��и���
	RiffList.fccListType    = FCC("AVI ");						// ������?
	m_FileOperate.Write(FileHandle, &RiffList, sizeof(RiffList));

	// 2. ��Ϣ���List
	RiffInfoChunk.fcc						= FCC("LIST");
	RiffInfoChunk.cb						= 4;			// 'hdrl'
	RiffInfoChunk.fccListType				= FCC("hdrl");
	OffsetRiffInfo = sizeof(RiffList);
	m_FileOperate.Write(FileHandle, &RiffInfoChunk, sizeof(RiffInfoChunk));

	// 2.1 AVI����Ϣ�ṹ
	RiffInfoChunk.cb += sizeof(AviMainHdr);
	memset( &AviMainHdr, 0, sizeof(AviMainHdr) );
	AviMainHdr.fcc							= FCC("avih");						// FCC('avih');
	AviMainHdr.cb							    = sizeof(AviMainHdr) - 8;
	AviMainHdr.dwMicroSecPerFrame			= (DWORD)(1000000 /m_rsinfo.videoinfo.framerate);
	AviMainHdr.dwMaxBytesPerSec				= 0;										// �������?
	AviMainHdr.dwPaddingGranularity		   = 0;
	AviMainHdr.dwFlags						= AVIF_HASINDEX;
	AviMainHdr.dwTotalFrames				   = FrameCounter;							// �ܵ���Ƶ֡��
	AviMainHdr.dwInitialFrames				= 0;
	AviMainHdr.dwStreams					   = 0;										// �������?
	AviMainHdr.dwSuggestedBufferSize		= VideoWidth * VideoHeight * 3 / 100;		// ��RGB�Աȣ�ѹ���Ȼ�Ϊ275��
	AviMainHdr.dwWidth						= VideoWidth;
	AviMainHdr.dwHeight						= VideoHeight;

	if ( bStreamVideo )
	{
		// ����Ƶ
		AviMainHdr.dwMaxBytesPerSec			= (DWORD)(m_rsinfo.videoinfo.framerate * AviMainHdr.dwSuggestedBufferSize);
		AviMainHdr.dwStreams ++;

		// ��Ƶ�б�ͷ
	    RiffStreamVideo.fcc					= FCC("LIST");
		RiffStreamVideo.cb					= 4;
		RiffStreamVideo.fccListType			= FCC("strl");						// 'strl';

		// ��Ƶ�б���?
		StreamHdrVideo.fcc					= FCC("strh");						// FCC('strh');
		StreamHdrVideo.cb					= sizeof(StreamHdrVideo) - 8;
		StreamHdrVideo.fccType				= streamtypeVIDEO;						// 'vids';
		StreamHdrVideo.fccHandler			= VideoTypeForFcc;
		StreamHdrVideo.dwFlags				= 0;
		StreamHdrVideo.wPriority				= 0;
		StreamHdrVideo.wLanguage				= 0;
		StreamHdrVideo.dwInitialFrames		= 0;
		StreamHdrVideo.dwScale				= AviMainHdr.dwMicroSecPerFrame;		// ֡���?
		StreamHdrVideo.dwRate				= 1000000;								// ΢��
		StreamHdrVideo.dwStart				= 0;
		StreamHdrVideo.dwLength				= FrameCounter;						// ��֡�����ļ��������ʱ��?dwLength/(dwRate/dwScale)
		StreamHdrVideo.dwSuggestedBufferSize	= AviMainHdr.dwSuggestedBufferSize;
		StreamHdrVideo.dwQuality				= 0xffffffff;//0;////////////////////////////////-1;									// ȱʡ����
		StreamHdrVideo.dwSampleSize			= 0;
		StreamHdrVideo.rcFrame.top			= 0;
		StreamHdrVideo.rcFrame.left			= 0;
		StreamHdrVideo.rcFrame.right		= (short)VideoWidth;
		StreamHdrVideo.rcFrame.bottom		= (short)VideoHeight;
		RiffStreamVideo.cb				    += sizeof(StreamHdrVideo);

		// ��Ƶ���ԣ�ǰ���и�'strf'������Ҫ��ɫ��,���ԣ�ֻ��ҪstuBmpInfo.bmiHeader����ҪstuBmpInfo.bmiColors
		BmpInfo.bmiHeader.biSize			= sizeof(BmpInfo.bmiHeader);
		BmpInfo.bmiHeader.biWidth			= VideoWidth;
		BmpInfo.bmiHeader.biHeight			= VideoHeight;
		BmpInfo.bmiHeader.biPlanes			= 1;
		BmpInfo.bmiHeader.biBitCount	    = 24;
		BmpInfo.bmiHeader.biCompression		= VideoTypeForFcc;						// FCC('H264');
		BmpInfo.bmiHeader.biSizeImage		= 0;									// ������Ҫ��д
		BmpInfo.bmiHeader.biXPelsPerMeter	= 0;
		BmpInfo.bmiHeader.biYPelsPerMeter	= 0;
		BmpInfo.bmiHeader.biClrUsed			= 0;
		BmpInfo.bmiHeader.biClrImportant	= 0;
		RiffStreamVideo.cb					+= 4 + sizeof(BmpInfo.bmiHeader);

		if ( bExtra4Bytes )
		{
			// ��ʵ�ʵ�AVI�ļ��У�'strf'���治��ֱ�Ӹ�stuBmpInfo�ṹ�������ȸ�����ṹ�Ĵ�С��Ȼ���ٸ�����ṹ������
			// 'strf'������listһ������ĺ�������������ݵĴ�Сһ����������������?���ֽ�
			RiffStreamVideo.cb				+= 4;
		}

		RiffInfoChunk.cb += 8 + RiffStreamVideo.cb;
	}
#if 1
	if ( bStreamAudio )
	{
		// ����Ƶ������Ƶ��ֻ࣬�Ǻ���ʹ�õ�����Ƶ��ʽ
		//AviMainHdr.dwMaxBytesPerSec			+= pstuAudioInfo->cAudioChannels * pstuAudioInfo->cSampleBits / 8 * pstuAudioInfo->lSampleRate / 20;
        AviMainHdr.dwMaxBytesPerSec			+= 1 * m_rsinfo.audioinfo.bitwidth/ 8 * m_rsinfo.audioinfo.samplerate / 20;
		AviMainHdr.dwStreams ++;

		// ��Ƶ�б�ͷ
		RiffStreamAudio.fcc					= FCC("LIST");
		RiffStreamAudio.cb					= 4;
		RiffStreamAudio.fccListType			= FCC("strl");						// 'strl';

		// ��Ƶ�б���?
		StreamHdrAudio.fcc					= FCC("strh");						// 'strh';
		StreamHdrAudio.cb					= sizeof(StreamHdrAudio) - 8;
		StreamHdrAudio.fccType				= FCC("auds");						// 'auds';
		StreamHdrAudio.fccHandler			= 0;
		StreamHdrAudio.dwFlags				= 0;
		StreamHdrAudio.wPriority			= 0;
		StreamHdrAudio.wLanguage			= 0;
		StreamHdrAudio.dwInitialFrames		= 0;
		//StreamHdrAudio.dwScale				= 1000000 / pstuAudioInfo->lSampleRate;	// ��Ƶ�Ĳ�����
		StreamHdrAudio.dwScale				= 1000000 / m_rsinfo.audioinfo.samplerate;	// ��Ƶ�Ĳ�����
		StreamHdrAudio.dwRate				= 1000000;								// ΢��
		StreamHdrAudio.dwStart				= 0;
		StreamHdrAudio.dwLength				= StreamHdrVideo.dwLength;			// ʱ�䳤��(��)����ʱ��Ϊ�϶�����Ƶ�����������Ƶ��ʱ�䳤����һ���
		//StreamHdrAudio.dwSuggestedBufferSize	= pstuAudioInfo->cAudioChannels * pstuAudioInfo->cSampleBits / 8 * pstuAudioInfo->lSampleRate;
		StreamHdrAudio.dwSuggestedBufferSize	= 1 * m_rsinfo.audioinfo.bitwidth/ 8 * m_rsinfo.audioinfo.samplerate;
		StreamHdrAudio.dwQuality				= 0xffffffff;//0;//////////////////////////-1;									// ȱʡ����
		StreamHdrAudio.dwSampleSize			= 0;
		StreamHdrAudio.rcFrame.top			= 0;
		StreamHdrAudio.rcFrame.left			= 0;
		StreamHdrAudio.rcFrame.right		= 0;
		StreamHdrAudio.rcFrame.bottom		= 0;
		RiffStreamAudio.cb					+= sizeof(StreamHdrAudio);

		// ��Ƶ���ԣ�ǰ���и�'strf'
		WavFmtEx.wFormatTag					= WAVE_FORMAT_PCM;
		//WavFmtEx.nChannels					= pstuAudioInfo->cAudioChannels;
		WavFmtEx.nChannels					= 1;
		//WavFmtEx.nSamplesPerSec				= pstuAudioInfo->lSampleRate;
		WavFmtEx.nSamplesPerSec				= m_rsinfo.audioinfo.samplerate;
		//WavFmtEx.wBitsPerSample				= pstuAudioInfo->cSampleBits;			// ���ļ�����������16�������ļ�ͷ��ȴд����8����ʱ�̶�����2���Ժ������Ҫ����ļ�ͷ�е����ݽ����޸�
		WavFmtEx.wBitsPerSample				= m_rsinfo.audioinfo.bitwidth;
		WavFmtEx.nBlockAlign				= WavFmtEx.wBitsPerSample / 8 * WavFmtEx.nChannels;
		WavFmtEx.nAvgBytesPerSec			= WavFmtEx.nBlockAlign * WavFmtEx.nSamplesPerSec;
		WavFmtEx.cbSize						= sizeof(WavFmtEx);
		RiffStreamAudio.cb					+= 4 + sizeof(WavFmtEx);

		if ( bExtra4Bytes )
		{
			// ��ʵ�ʵ�AVI�ļ��У�'strf'���治��ֱ�Ӹ�stuBmpInfo�ṹ�������ȸ�����ṹ�Ĵ�С��Ȼ���ٸ�����ṹ������
			// 'strf'������listһ������ĺ�������������ݵĴ�Сһ����������������?���ֽ�
			RiffStreamAudio.cb				+= 4;
		}

		RiffInfoChunk.cb += 8 + RiffStreamAudio.cb;
	}
#endif
    m_framenumpos[0] = (unsigned long)m_FileOperate.Seek(FileHandle, 0, SEEK_CUR) + ((unsigned long)&(AviMainHdr.dwTotalFrames) - (unsigned long)&AviMainHdr);
    m_FileOperate.Write(FileHandle, &AviMainHdr, sizeof(AviMainHdr));

	// 2.2 д��Ƶ��Ϣ
	if ( bStreamVideo )
	{
        FOURCC FccStrf = FCC("strf");
		// д�б�ͷ
		m_FileOperate.Write(FileHandle, &RiffStreamVideo, sizeof(RiffStreamVideo));

		m_framenumpos[1] = (unsigned long)m_FileOperate.Seek(FileHandle, 0, SEEK_CUR) + ((unsigned long)&(StreamHdrVideo.dwLength) - (unsigned long)&StreamHdrVideo);
		//DEBUG_MESSAGE("\n2 offset:%ld\n\n", (unsigned long)&(StreamHdrVideo.dwLength) - (unsigned long)&StreamHdrVideo);

		// д��Ƶ�б���?
		m_FileOperate.Write(FileHandle, &StreamHdrVideo, sizeof(StreamHdrVideo));
		// д��Ƶ��ʽ�ṹͷ��ʾ
		m_FileOperate.Write(FileHandle, &FccStrf, sizeof(FccStrf));
		if ( bExtra4Bytes )
		{
			Temp = sizeof(BmpInfo.bmiHeader);
			m_FileOperate.Write(FileHandle, &Temp, sizeof(Temp));
		}
		// д��Ƶ����ϸ����
		m_FileOperate.Write(FileHandle, &BmpInfo.bmiHeader, sizeof(BmpInfo.bmiHeader));
	}

	// 2.3 д��Ƶ��Ϣ
	if ( bStreamAudio )
	{
        FOURCC FccStrf = FCC("strf");

		// д�б�ͷ
		m_FileOperate.Write(FileHandle, &RiffStreamAudio, sizeof(RiffStreamAudio) );			// д�б�ͷ

        m_framenumpos[2] = (unsigned long)m_FileOperate.Seek(FileHandle, 0, SEEK_CUR) + ((unsigned long)&(StreamHdrAudio.dwLength) - (unsigned long)&StreamHdrAudio);
        //DEBUG_MESSAGE("\n3 offset:%ld\n\n", (unsigned long)&(StreamHdrAudio.dwLength) - (unsigned long)&StreamHdrAudio);

		// д��Ƶ�б���?
		m_FileOperate.Write(FileHandle, &StreamHdrAudio, sizeof(StreamHdrAudio) );				// д��Ƶ�б���?
		// д��Ƶ��ʽ�ṹͷ��ʾ
		m_FileOperate.Write(FileHandle, &FccStrf, sizeof(FccStrf) );								// д��Ƶ��ʽ�ṹͷ��ʾ
		if ( bExtra4Bytes )
		{
			Temp = sizeof(WavFmtEx);
			//Temp = 18;
			m_FileOperate.Write(FileHandle, &Temp, sizeof(Temp) );
		}
		// д��Ƶ����ϸ����
		m_FileOperate.Write(FileHandle, &WavFmtEx, sizeof(WavFmtEx));
	}

	// 3. ������Ϣ�б�ĳ���?
	m_FileOperate.Seek(FileHandle, OffsetRiffInfo, SEEK_SET);
	m_FileOperate.Write(FileHandle, &RiffInfoChunk, sizeof(RiffInfoChunk));
	m_FileOperate.Seek(FileHandle, 0, SEEK_END);

	// 4. ���ļ�ͷ�ĳ��ȴ�����0x800�ֽ�
#if 0
	if ( handleFile.GetLength() < 0x7f4 )
	{
		lTemp = 0x7f4 - (unsigned long)handleFile.GetLength() - 8;
		handleFile.Write( "JUNK", 4 );
		handleFile.Write( &lTemp, 4 );
		handleFile.Seek( lTemp, CFile::current );
	}
#endif

	// 5. д��������б�ͷ������ĳ��ȣ���Ҫ���ļ���ɺ���и���
	RiffList.fcc			= FCC("LIST");
	RiffList.cb			    = FCC("size");						// ���ļ�����֮���ٽ��и���
	RiffList.fccListType    = FCC("movi");
	m_FileOperate.Write(FileHandle, &RiffList, sizeof(RiffList));
    m_streamlenoffset = (unsigned long)m_FileOperate.Seek(FileHandle, 0, SEEK_CUR) - 8;
    m_streamlen = 4;//sizeof("movi")

    return 0;
}

int CAVI::WriteStream2AVIFile(int FileHandle, unsigned char *pStreamData, unsigned int len, rs2avi_frametype_t type)
{
    unsigned char buf[8];
    unsigned char *ptr = buf;
    int ret;
    
    if(type == RS2AVI_IFRAME || type == RS2AVI_PFRAME)
    {
        *((FOURCC *)ptr) = FCC("00dc");
    }
    else if(type == RS2AVI_AFRAME)
    {
        *((FOURCC *)ptr) = FCC("01wb");
    }
    else
    {
        DEBUG_ERROR("error frame type\n");
        exit(1);
    }

    ptr += 4;

    *((unsigned int *)ptr) = len;
    ret = m_FileOperate.Write(FileHandle, buf, 8);
    if(ret != 8)
    {
    	DEBUG_ERROR("CRecordStream2AVI::WriteStream2AVIFile err:write err1\n");
    	return -1;
    }
    if(len%2!= 0)
    {
        len++;//¼���ļ���֡���Ȱ�8�ֽڶ��룬�ʴ˴�����Խ��
        pStreamData[len - 1] = 0;/*2�ֽڶ��룬��0���?/
    }
    ret = m_FileOperate.Write(FileHandle, pStreamData, len);
    if(ret != len)
    {
    	DEBUG_ERROR("CRecordStream2AVI::WriteStream2AVIFile err:write err2\n");
    	return -1;
    }

    m_streamlen += (len+8);

    return (len+8);
}


void CAVI::UpdateAVIFileHeader( int FileHandle)
{
    unsigned long len = (unsigned long)m_FileOperate.Seek(FileHandle, 0, SEEK_CUR) - 8;

	// �ļ�����
	m_FileOperate.Seek(FileHandle, 4, SEEK_SET);
	m_FileOperate.Write(FileHandle, &len, sizeof(len));

	// �����ĳ���
	m_FileOperate.Seek(FileHandle, m_streamlenoffset, SEEK_SET);
	m_FileOperate.Write(FileHandle, &m_streamlen, sizeof(m_streamlen));

    //��Ƶ֡��Ŀ
    for(int i=0; i<3; i++)
    {
        if(m_framenumpos[i] != 0)
        {
            m_FileOperate.Seek(FileHandle, m_framenumpos[i], SEEK_SET);
            m_FileOperate.Write(FileHandle, &m_videoframenum, 4);
        }
    }
}
int CAVI::AppendAVIIndex(int FileHandle)
{
    int ret;
    unsigned long indexlen = 0;
    char *pBuf = NULL;

    DEBUG_MESSAGE("videoframenum:%d  audioframenum:%d\n", m_videoframenum, m_audioframenum);

    DEBUG_MESSAGE("write index data...\n");

    indexlen = (m_videoframenum+m_audioframenum)*sizeof(AVIINDEXENTRY);

    m_FileOperate.Seek(m_tmpindexfilefd, 4, SEEK_SET);
    m_FileOperate.Write(m_tmpindexfilefd, &indexlen, 4);
    
    m_FileOperate.Seek(m_tmpindexfilefd, 0, SEEK_SET);

    pBuf = (char *)malloc(READ_FILE_UNIT);
    if(pBuf == NULL)
    {
        DEBUG_ERROR("AppendAVIIndex err:out of memory\n");
    }
    
    while(1)
    {
		ret = m_FileOperate.Read(m_tmpindexfilefd, pBuf, READ_FILE_UNIT);
		if(ret <= 0)
		{
			break;
		}
		ret = m_FileOperate.Write(FileHandle, pBuf, ret);
    }

    m_FileOperate.Close(m_tmpindexfilefd);

#if 1
    //unlink(m_tmpindexfile);
    m_FileOperate.Remove(m_tmpindexfile);
#else
    unlink(TMP_INDEX_FILE_PATH);
#endif

    free(pBuf);

    return 0;
}
/*********************************************************************
	Function:
	Description:����AVI�ļ�������ʼ���ļ�ͷ
	Calls:
  	Called By:
	parameter:
  	Return:
  	author:
  		����
********************************************************************/
int CAVI::AVI_Open(char *pFileName)
{
    int fd;
    bool bStreamVideo, bStreamAudio;
    int ret;
    
    m_videoframenum = 0;
    m_audioframenum = 0;

    if(m_FSIType == AVI_FSI_STANDARD)
    {
    	fd = m_FileOperate.Open(pFileName, O_RDWR|O_CREAT|O_TRUNC, 0777);
    }
    else
    {
    	fd = m_FileOperate.NtfsOpen(pFileName, m_rsinfo.streamsize);
    }
    if(fd < 0)
    {
        return -1;
    }

    if(m_rsinfo.videoinfo.chno >= 0)
    {
        bStreamVideo = true;
    }
    else
    {
        bStreamVideo = false;
    }

    if(m_rsinfo.audioinfo.chno >= 0)
    {
        bStreamAudio = true;

        if(m_rsinfo.audioinfo.payloadtype == 0)//adpcm
        {
			ret = AdpcmDecoderInit();
			if(ret < 0)
			{
				m_FileOperate.Close(fd);
				return -1;
			}
        }
    }
    else
    {
        bStreamAudio = false;
    }

    memset(m_framenumpos, 0, sizeof(m_framenumpos));
    InitAVIFileHeader(bStreamVideo, bStreamAudio, fd, 1000, FCC("H264"));

#if 1
    memset(m_tmpindexfile, 0, sizeof(m_tmpindexfile));
    memcpy(m_tmpindexfile, pFileName, strrchr(pFileName, '/')-pFileName);
    strcat(m_tmpindexfile, "/.avitmpindex");
    //DEBUG_MESSAGE("m_tmpindexfile:%s\n", m_tmpindexfile);
    if(m_FSIType == AVI_FSI_STANDARD)
    {
    	m_tmpindexfilefd = m_FileOperate.Open(m_tmpindexfile, O_RDWR|O_CREAT|O_TRUNC, 0777);
    }
    else
    {
    	m_tmpindexfilefd = m_FileOperate.NtfsOpen(m_tmpindexfile, 6*1024*1024);
    }
#else
    m_tmpindexfilefd = open(TMP_INDEX_FILE_PATH, O_RDWR|O_CREAT|O_TRUNC, 0777);
#endif
    if(m_tmpindexfilefd < 0)
    {
        DEBUG_ERROR("CRecordStream2AVI::AVI_Open err:open tmp file failed\n");
        m_FileOperate.Close(fd);
        if(m_rsinfo.audioinfo.payloadtype == 0)//adpcm
        {
           AdpcmDecoderDestroy();
        }
        return -1;
    }

    m_pIndexBuf = (char *)malloc(INDEX_BUF_LEN);
    if(m_pIndexBuf == NULL)
    {
		DEBUG_ERROR("CRecordStream2AVI::AVI_Open err:malloc for index failed\n");
		m_FileOperate.Close(fd);
		m_FileOperate.Close(m_tmpindexfilefd);
       if(m_rsinfo.audioinfo.payloadtype == 0)//adpcm
        {
		    AdpcmDecoderDestroy();
        }
		return -1;
    }
    m_IndexLen = 0;

    RIFFCHUNK IndexChunk;
	IndexChunk.fcc	= FCC("idx1");
	IndexChunk.cb   = FCC("size");//������ݵĳ���?m_indexlen

	m_FileOperate.Write(m_tmpindexfilefd, &IndexChunk, sizeof(IndexChunk));
	m_frameoffset = m_FileOperate.Seek(fd, 0, SEEK_CUR);/*����ļ�ͷ��ƫ����?/

    return fd;
}

/*********************************************************************
	Function:
	Description:д����Ƶ֡���?
	Calls:
  	Called By:
	parameter:
  	Return:
  	author:
  		����
********************************************************************/
int CAVI::AVI_Write(int FileHandle, unsigned char *pData, unsigned int len, rs2avi_frametype_t type)
{
    AVIINDEXENTRY index;
    int DataLen;

    if(type == RS2AVI_IFRAME)
    {
        index.ckid = FCC("00dc");
        index.dwFlags = AVIIF_KEYFRAME;
        m_videoframenum++;
    }
    else if(type == RS2AVI_PFRAME)
    {
        index.ckid = FCC("00dc");
        index.dwFlags = 0;
        m_videoframenum++;
    }
    else if(type == RS2AVI_AFRAME)
    {
        index.ckid = FCC("01wb");
        index.dwFlags = 0;
        m_audioframenum++;
        if(m_rsinfo.audioinfo.payloadtype == 0)//adpcm
        {
          len = AdpcmDecoder((char *)pData, m_pPCMData, len);
        }
        else if(m_rsinfo.audioinfo.payloadtype == 2)//g711 alaw
        {
        	len = G711AlawDecoder((char *)pData, m_pPCMData, len);
        }
        else if(m_rsinfo.audioinfo.payloadtype == 3)//g711 ulaw
        {
        	len = G711UlawDecoder((char *)pData, m_pPCMData, len);
        }
        else
        {
        	DEBUG_ERROR("CAVI:no supported audio format:%d\n", m_rsinfo.audioinfo.payloadtype);
        }
    }
    else
    {
        DEBUG_ERROR("error frame type\n");
        exit(1);
    }

    int ret;
    index.dwChunkLength = len;
    index.dwChunkOffset = m_frameoffset;
#if 0
    ret = write(m_tmpindexfilefd, &index, sizeof(index));
    if(ret != sizeof(index))
    {
        DEBUG_ERROR("write error!!!\n");
        fflush(stdout);
        exit(1);
    }
#endif
    memcpy(m_pIndexBuf+m_IndexLen, &index, sizeof(index));
    m_IndexLen += sizeof(index);
    if(m_IndexLen == INDEX_BUF_LEN)
    {
        ret = m_FileOperate.Write(m_tmpindexfilefd, m_pIndexBuf, m_IndexLen);
        if(ret != m_IndexLen)
        {
            DEBUG_ERROR("write error!!!\n");
            fflush(stdout);
            //exit(1);
            return -1;
        }
        m_IndexLen = 0;

    }
    else if(m_IndexLen > INDEX_BUF_LEN)
    {
    	DEBUG_ERROR("\n\nerrrrrrrrrrrrrrrrrrrrrrrr\n\n");
    }


    if(len%2!=0)// 2�ֽڶ���
        m_frameoffset += (len + 8 + 1);
    else
        m_frameoffset += (len + 8);

    if(type == RS2AVI_AFRAME)
    {
        DataLen = WriteStream2AVIFile(FileHandle, (unsigned char *)m_pPCMData, len, type);
    }
    else
    {
        DataLen = WriteStream2AVIFile(FileHandle, pData, len, type);
    }

    if(DataLen <= 0)
    {
    	return -1;
    }
    //DEBUG_MESSAGE("frame no:%d len:%d\n", m_framenum, len);
    return (DataLen+sizeof(AVIINDEXENTRY));
}
/*********************************************************************
	Function:
	Description:
	Calls:
  	Called By:
	parameter:
  	Return:
  	author:
  		����
********************************************************************/
int CAVI::AVI_Close(int FileHandle, bool force)
{
	int ret;

    if(!force && m_IndexLen > 0)
    {
        ret = m_FileOperate.Write(m_tmpindexfilefd, m_pIndexBuf, m_IndexLen);
        if(ret != m_IndexLen)
        {
            DEBUG_ERROR("write error!!!\n");
            fflush(stdout);
            //exit(1);

            free(m_pIndexBuf);
            m_IndexLen = 0;

			 m_FileOperate.Close(FileHandle);
			 //sync();

			 if(m_rsinfo.audioinfo.payloadtype == 0)//adpcm
			 {
				 AdpcmDecoderDestroy();
			 }

            m_FileOperate.Close(m_tmpindexfilefd);
            m_FileOperate.Remove(m_tmpindexfile);

            DEBUG_ERROR("AVI Closed:-1\n");
            return -1;
        }
    }
    free(m_pIndexBuf);
    m_IndexLen = 0;

    if(!force)
    {
    	AppendAVIIndex(FileHandle);
    	UpdateAVIFileHeader(FileHandle);
    }
    else
    {
        m_FileOperate.Close(m_tmpindexfilefd);
        m_FileOperate.Remove(m_tmpindexfile);
    }

    m_FileOperate.Close(FileHandle);
    if(!force)
    {
    	sync();
    }

	if(m_rsinfo.audioinfo.payloadtype == 0)//adpcm
	{
		AdpcmDecoderDestroy();
	}
    DEBUG_ERROR("AVI Closed\n");
    return 0;
}



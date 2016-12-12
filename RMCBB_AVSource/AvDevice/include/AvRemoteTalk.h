#ifndef __CAVREMOTETALK_H__
#define __CAVREMOTETALK_H__
#include "Message.h"
#include "CommonLibrary.h"
using namespace Common;

#include "AvConfig.h"

#if defined(_AV_PLATFORM_HI3515_)
#include "HiAo-3515.h"
#else
#include "HiAo.h"
#endif

class CAvRemoteTalk
{
public:
    CAvRemoteTalk( CHiAo* pHi_Ao, CHiAi* pHi_Ai );
    ~CAvRemoteTalk();

public:
    int ReciveTalkThreadBody();
    int SendTalkThreadBody();
    
    int StartTalk( const msgRequestAudioNetPara_t* pstuTalkParam );
    int StopTalk(const rm_int32_t talkback_type);

private:
    /////////////////////////encoder///////////////////////////////
    int InitAudioInput();
    int UnInitAudioInput();
    int CreateAudioEncoder();
    int DestroyAudioEncoder();

    AENC_CHN GetTalkAudioEncoderChl();


    //////////////////////////decoder/////////////////////////////
    // adecType :0-adpcm
    int InitAudioOutput( int eAudioType );
    int UnitAudioOutput();
	int SetTalkbackType( const rm_int32_t talkback_type);
	int GetTalkbackType() const;
    // adecType :0-adpcm
    int CreateAudioDecoder( int eAdecType );
    int DestroyAudioDecoder();

    ADEC_CHN GetTalkAudioDecoderChl();

    int DecodeAudioFrame( const char* pszData, unsigned int u32DataLen );
    
        
private:
    bool m_bNeedExitThread;
    bool m_bIsReciveTaskRunning;
    bool m_bIsSendTaskRunning;
	rm_int32_t m_talkback_type;
    HANDLE m_ShareMemReciveHandle;
    HANDLE m_ShareMemSendHandle;

    CHiAo* m_ptrHi_Ao;
    CHiAi* m_ptrHi_Ai;

    struct{
        RMSTREAM_HEADER stuHead;
        RMFI2_VTIME stuPts;
        RMFI2_AUDIOINFO stuInfo;
    } m_stuAudioFrameHead;
};

#endif


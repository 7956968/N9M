#include "AvRemoteTalk.h"
#include "AvCommonFunc.h"
#include "Defines.h"
#include "System.h"
#include "hi_comm_vdec.h"
#include "mpi_adec.h"
#include "mpi_sys.h"
#include "mpi_aenc.h"
#include "AvDeviceInfo.h"
#ifdef _AV_USE_AACENC_
#ifndef _AV_PLATFORM_HI3515_
#include "audio_aac_adp.h"
#endif
#endif

#if !defined(_AV_PLATFORM_HI3515_)
#include "audio_amr_adp.h"
#endif
using namespace Common;
#define _AV_MAX_PACK_NUM_ONEFRAME_   10


void *TalkSendThreadEntry( void *pThreadParam );
void *TalkReciveThreadEntry( void *pThreadParam );

//#define SAVE_TALKBACK 1

typedef struct
{
    struct
    {
        unsigned char *pAddr[2];
        unsigned int Len[2];
    } DataPack[20];
    int PackCount;
} TalkFrameData_t;

unsigned int CalculateCheckSum(TalkFrameData_t *pData, int len);

CAvRemoteTalk::CAvRemoteTalk( CHiAo* pHi_Ao, CHiAi* pHi_Ai )
{
    m_bNeedExitThread = true;
    m_bIsReciveTaskRunning = false;
    m_bIsSendTaskRunning = false;

    m_ShareMemReciveHandle = NULL;
    m_ShareMemSendHandle = NULL;

    m_ptrHi_Ao = pHi_Ao;
    m_ptrHi_Ai = pHi_Ai;
    if( !m_ptrHi_Ai )
    {
        m_ptrHi_Ai = new CHiAi();
    }

    memset( &m_stuAudioFrameHead, 0, sizeof(m_stuAudioFrameHead) );
    ((char*)&m_stuAudioFrameHead)[0] = 0;
    ((char*)&m_stuAudioFrameHead)[1] = '4';
    ((char*)&m_stuAudioFrameHead)[2] = 'd';
    ((char*)&m_stuAudioFrameHead)[3] = 'c';
    m_stuAudioFrameHead.stuHead.lFrameLen = 168;
    m_stuAudioFrameHead.stuHead.lStreamExam = 0; 
    m_stuAudioFrameHead.stuHead.lExtendLen = sizeof(m_stuAudioFrameHead) - sizeof(m_stuAudioFrameHead.stuHead); 
    m_stuAudioFrameHead.stuHead.lExtendCount = 2;
    
    m_stuAudioFrameHead.stuPts.stuInfoType.lInfoType = RMS_INFOTYPE_VIRTUALTIME;
    m_stuAudioFrameHead.stuPts.stuInfoType.lInfoLength = sizeof(RMFI2_VTIME);

    m_stuAudioFrameHead.stuInfo.stuInfoType.lInfoType = RMS_INFOTYPE_AUDIOINFO;
    m_stuAudioFrameHead.stuInfo.stuInfoType.lInfoLength = sizeof(RMFI2_AUDIOINFO);
#if defined(_AV_PRODUCT_CLASS_IPC_)
    m_stuAudioFrameHead.stuInfo.lPayloadType = 2; //g711a
#else
    m_stuAudioFrameHead.stuInfo.lPayloadType = 0; //adpcm
    //! for CB check criterion, G726 is only used
    char check_criterion[32]={0};
    pgAvDeviceInfo->Adi_get_check_criterion(check_criterion, sizeof(check_criterion));
    char customer_name[32] = {0};
    pgAvDeviceInfo->Adi_get_customer_name(customer_name,sizeof(customer_name));
    if ((!strcmp(check_criterion, "CHECK_CB")))
    {
        m_stuAudioFrameHead.stuInfo.lPayloadType = 1; //g726 
    }
    else if (!strcmp(customer_name, "CUSTOMER_04.1062"))
    {
        m_stuAudioFrameHead.stuInfo.lPayloadType = 3; //g711u
    }
#endif
    m_stuAudioFrameHead.stuInfo.lSoundMode = 0; //mono
    m_stuAudioFrameHead.stuInfo.lPlayAudio = 1;
    m_stuAudioFrameHead.stuInfo.lBitWidth = 16;
    m_stuAudioFrameHead.stuInfo.lSampleRate = 8000;
}

CAvRemoteTalk::~CAvRemoteTalk()
{
    int talktype = this->GetTalkbackType();
    this->StopTalk(talktype);
}

int CAvRemoteTalk::ReciveTalkThreadBody()
{
    m_bIsReciveTaskRunning = true;

    char* ptrData = NULL;
    int len = 0;
    
#ifdef SAVE_TALKBACK
    //保存编码数据，用于验证
    FILE* fp_stream= NULL;
    if(NULL ==  (fp_stream = fopen("/var/mnt/usbstate/front/Adectalk.dat", "wb+")))
    {
        DEBUG_ERROR("open file failed!\n");
    }
#endif

    while( !m_bNeedExitThread )
    {
        len = N9M_SHGetOneFrame(m_ShareMemReciveHandle, &ptrData, NULL);
        //DEBUG_MESSAGE("get one frame, len=%d flag=%d\n", len, flag);
        if( len <= 0 )
        {
            mSleep(10);
            continue;
        }
        //test
        // m_pShareMemSend->PutOneFrame( ptrData, len , flag);
        this->DecodeAudioFrame( ptrData, len );
        
#ifdef SAVE_TALKBACK           
            //保存编码数据，用于验证
        if(fp_stream != NULL)
        {
            fwrite( ptrData, 1, len, fp_stream );
        }
#endif
      
    }
    
#ifdef SAVE_TALKBACK
        //保存编码数据，用于验证
        fclose(fp_stream);
#endif

    m_bIsReciveTaskRunning = false;

    return 0;
}

int CAvRemoteTalk::SendTalkThreadBody()
{
    m_bIsSendTaskRunning = true;
    
    AENC_CHN aencChn = this->GetTalkAudioEncoderChl();
    HI_S32 sRet = HI_SUCCESS;
    AUDIO_STREAM_S stuStream;
	memset(&stuStream,0,sizeof(stuStream));
    SShareMemData stuShareData;
    SShareMemFrameInfo stuShareInfo;
    memset( &stuShareData, 0, sizeof(stuShareData) );
    memset( &stuShareInfo, 0, sizeof(stuShareInfo) );
    stuShareInfo.flag = SHAREMEM_FLAG_IFRAME;
    unsigned int u32FrameHeadLen = sizeof(m_stuAudioFrameHead);
    
#ifdef SAVE_TALKBACK
    //保存编码数据，用于验证
    FILE* fp_stream= NULL;
    if(NULL ==  (fp_stream = fopen("/var/mnt/usbstate/front/Aenctalk.dat", "wb+")))
    {
        DEBUG_ERROR("open file failed!\n");
    }
#endif

    stuShareData.pstuPack = new SShareMemData::_framePack_[_AV_MAX_PACK_NUM_ONEFRAME_ + 1];
    while( !m_bNeedExitThread )
    {
#ifdef _AV_PLATFORM_HI3535_
        if( (sRet=HI_MPI_AENC_GetStream(aencChn, &stuStream, -1)) != HI_SUCCESS )
#elif defined(_AV_PLATFORM_HI3520D_V300_)
        if( (sRet=HI_MPI_AENC_GetStream(aencChn, &stuStream, -1)) != HI_SUCCESS )
#elif defined(_AV_PLATFORM_HI3515_)
        if( (sRet=HI_MPI_AENC_GetStream(aencChn, &stuStream, HI_IO_BLOCK)) != HI_SUCCESS )//! HI_TRUE
#else
        if( (sRet=HI_MPI_AENC_GetStream(aencChn, &stuStream, HI_TRUE)) != HI_SUCCESS )
#endif
        {
#if defined(_AV_PLATFORM_HI3515_)        
            if ( (HI_U32)sRet == 0xA015800E)//! 0411 
            {
                DEBUG_MESSAGE( "HI_MPI_AENC_GetStream error width 0x%08x, aenchn=%d\n", sRet, aencChn);
                mSleep(25);
            }
            else
#endif            
            {
                DEBUG_ERROR( "HI_MPI_AENC_GetStream error width 0x%08x, aenchn=%d\n", sRet, aencChn);
            }
            
            mSleep(10);
            continue;
        }
        
        //DEBUG_MESSAGE("get audio encoder, len=%d seq=%d\n", stuStream.u32Len, stuStream.u32Seq );
        if( stuStream.u32Len > 0 && stuStream.u32Seq != 0 )
        {
            m_stuAudioFrameHead.stuPts.llVirtualTime = stuStream.u64TimeStamp / 1000;
            m_stuAudioFrameHead.stuHead.lFrameLen = stuStream.u32Len;
            m_stuAudioFrameHead.stuHead.lStreamExam = 0;

            stuShareData.iPackCount = 2;
            stuShareData.pstuPack[0].iLen = u32FrameHeadLen;
            stuShareData.pstuPack[0].pAddr = (char*)&m_stuAudioFrameHead;

            stuShareData.pstuPack[1].iLen = stuStream.u32Len;
            stuShareData.pstuPack[1].pAddr = (char*)stuStream.pStream;

            N9M_SHPutOneFrame(m_ShareMemSendHandle, &stuShareData, &stuShareInfo, u32FrameHeadLen + stuStream.u32Len);
            
#ifdef SAVE_TALKBACK           
            //保存编码数据，用于验证
            if (fp_stream != NULL)
            {
                fwrite( (char *)stuShareData.pstuPack[0].pAddr, 1, stuShareData.pstuPack[0].iLen, fp_stream );
                fwrite( (char *)stuShareData.pstuPack[1].pAddr, 1, stuShareData.pstuPack[1].iLen, fp_stream );
            }
#endif

        }
        else
        {
            mSleep(10);
        }

        HI_MPI_AENC_ReleaseStream(aencChn, &stuStream);
    }

#ifdef SAVE_TALKBACK
    //保存编码数据，用于验证
    fclose(fp_stream);
#endif

    if(NULL != stuShareData.pstuPack)
    {
        delete []stuShareData.pstuPack;
    }
    m_bIsSendTaskRunning = false;

    return 0;
}

int CAvRemoteTalk::DecodeAudioFrame( const char* pszData, unsigned int u32DataLen )
{    
    RMSTREAM_HEADER *pStreamHeader = (RMSTREAM_HEADER*)pszData;

    AUDIO_STREAM_S AudioStream;
    AudioStream.pStream = (HI_U8 *)pszData + sizeof(RMSTREAM_HEADER) + pStreamHeader->lExtendLen;
    AudioStream.u32Len = pStreamHeader->lFrameLen;
    AudioStream.u64TimeStamp = 0;
    AudioStream.u32Seq = 0;

    //////////////////test/////////////////////
   /* RMFI2_AUDIOINFO* pInfo = (RMFI2_AUDIOINFO*)pszData + sizeof(RMSTREAM_HEADER)+sizeof(RMFI2_VTIME);
    DEBUG_MESSAGE("info.lSoundMode=%ld bidw=%ld sample=%ld lPayloadType=%ld framelen=%ld exlen=%ld exam=%ld exount=%ld\n"
        , pInfo->lSoundMode
        , pInfo->lBitWidth
        , pInfo->lSampleRate
        , pInfo->lPayloadType, pStreamHeader->lFrameLen, pStreamHeader->lExtendLen
        , pStreamHeader->lStreamExam, pStreamHeader->lExtendCount);*/
    ///////////////////////////////////////////

    HI_S32 sRet = HI_MPI_ADEC_SendStream( this->GetTalkAudioDecoderChl(), &AudioStream, HI_IO_BLOCK);

    if( sRet != 0 )
    {
        DEBUG_ERROR("CAvRemoteTalk::DecodeAudioFrame()HI_MPI_ADEC_SendStream err:0x%08x, lFrameLen %ld decChn=%d\n", sRet, pStreamHeader->lFrameLen, this->GetTalkAudioDecoderChl());
        return -1;
    }

    return 0;
}

int CAvRemoteTalk::CreateAudioDecoder( int eAdecType )
{
    HI_S32 sRet = HI_SUCCESS;

#ifdef _AV_PLATFORM_HI3531_
    MPP_CHN_S stuMppChn;
    const char *pDdrName = NULL;
    
    pDdrName = "ddr1";
    stuMppChn.enModId = HI_ID_ADEC;
    stuMppChn.s32DevId = 0;
    stuMppChn.s32ChnId = this->GetTalkAudioDecoderChl();
    
    if( (sRet = HI_MPI_SYS_SetMemConf(&stuMppChn, pDdrName)) != 0 )
    {
        DEBUG_ERROR( "CHiAvDec::CreateAudioDecoder HI_MPI_SYS_SetMemConf (aChn:%d)(0x%08lx)\n", stuMppChn.s32ChnId, (unsigned long)sRet );
        return -1;
    }
#endif
    
    ADEC_ATTR_ADPCM_S AdpcmAttr;
    ADEC_ATTR_AMR_S AmrAttr;
    ADEC_ATTR_G726_S G726Attr;
    ADEC_CHN_ATTR_S AdecAttr;
#ifdef _AV_USE_AACENC_		
     ADEC_ATTR_AAC_S AacAttr;
#endif   
    ADEC_ATTR_G711_S G711Attr;
    
    AdecAttr.u32BufSize = 30;
    AdecAttr.enMode = ADEC_MODE_PACK;
    switch( eAdecType )
    {
        case 0:
            AdecAttr.enType = PT_ADPCMA;
            AdpcmAttr.enADPCMType = ADPCM_TYPE_DVI4;
            AdecAttr.pValue = &AdpcmAttr;
            break;
        case 1:
            AdecAttr.enType = PT_AMR;
            AdecAttr.pValue = &AmrAttr;
            AmrAttr.enFormat = AMR_FORMAT_MMS;
            //HI_MPI_ADEC_AmrInit(); 
        case 2:
            {
                AdecAttr.enType = PT_G726;
                char check_criterion[32]={0};
                pgAvDeviceInfo->Adi_get_check_criterion(check_criterion, sizeof(check_criterion));
                char customer_name[32] = {0};
                pgAvDeviceInfo->Adi_get_customer_name(customer_name,sizeof(customer_name));
                if ((!strcmp(check_criterion, "CHECK_CB"))||(!strcmp(customer_name, "CUSTOMER_04.1062")))
                {
                    G726Attr.enG726bps = G726_16K;//MEDIA_G726_16K;
                }
                else if (!strcmp(customer_name, "CUSTOMER_JIUTONG"))
                {
                    G726Attr.enG726bps = G726_32K;
                }
                else
                {
                   G726Attr.enG726bps = MEDIA_G726_16K; 
                }
                AdecAttr.pValue = &G726Attr;
                break;
            }  
#ifdef _AV_USE_AACENC_		
        case 3:
            AdecAttr.enType = PT_AAC;
            AdecAttr.pValue = &AacAttr;
#ifndef _AV_PLATFORM_HI3515_
            HI_MPI_ADEC_AacInit();
#endif            
            break;
#endif
        case 4:
            AdecAttr.enType = PT_G711U;
            AdecAttr.pValue = &G711Attr;
            break;
        default:
            DEBUG_ERROR( "invalid audio type:%d\n", eAdecType );
            return -1;
            break;
    }

    sRet = HI_MPI_ADEC_CreateChn( this->GetTalkAudioDecoderChl(), &AdecAttr );
    if( sRet != 0 )
    {
        DEBUG_ERROR("HI_MPI_ADEC_CreateChn %d err:0x%08x\n", sRet, this->GetTalkAudioDecoderChl());
        return -1;
    }

    return 0;
}

int CAvRemoteTalk::DestroyAudioDecoder()
{
    HI_S32 sRet = HI_MPI_ADEC_DestroyChn( this->GetTalkAudioDecoderChl() );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR("destroy talk decoder failed width:0x08%x\n", sRet);
        return -1;
    }

    return 0;
}

ADEC_CHN CAvRemoteTalk::GetTalkAudioDecoderChl()
{
    return 1;
}

//AudioType: 0 adpcm, 1 amr, 2 g726, 3 ACC 4 g711u 
int CAvRemoteTalk::InitAudioOutput( int eAudioType )
{
    if( this->CreateAudioDecoder(eAudioType) != 0 )
    {
        DEBUG_ERROR("InitAudio input failed, bacause of create talk decoder failed\n");
        return -1;
    }
    
    m_ptrHi_Ao->HiAo_unbind_cvbs_ao_for_talk( true );
    
    if( m_ptrHi_Ao->HiAo_adec_bind_ao(_AO_TALKBACK_, this->GetTalkAudioDecoderChl()) != 0 )
    {
        DEBUG_ERROR("InitAudio input failed, bind talkback ao failed\n");
        this->DestroyAudioDecoder();
        return -1;
    }

    return 0;
}

int CAvRemoteTalk::UnitAudioOutput()
{
    if( m_ptrHi_Ao->HiAo_adec_unbind_ao(_AO_TALKBACK_, this->GetTalkAudioDecoderChl()) != 0 )
    {
        DEBUG_ERROR("Ao unbind failed\n");
    }

    m_ptrHi_Ao->HiAo_unbind_cvbs_ao_for_talk( false );

    if( this->DestroyAudioDecoder() != 0 )
    {
        DEBUG_ERROR("Desotroy talk decoder failed\n");
    }

    return 0;
}

int CAvRemoteTalk::InitAudioInput()
{
    //for some AD, need set ad as slave mode, so maybe the AI was inited yet.
#if !defined(_AV_PLATFORM_HI3520D_)&&!defined(_AV_PLATFORM_HI3520D_V300_)

    m_ptrHi_Ai->HiAi_stop_ai(_AI_TALKBACK_);
    
    if( m_ptrHi_Ai->HiAi_start_ai(_AI_TALKBACK_) != 0 )
    {
        DEBUG_ERROR("Remote Talk m_ptrHi_Ai->HiAi_start_ai failed\n");
        return -1;
    }
#endif

    if( this->CreateAudioEncoder() != 0 )
    {
        DEBUG_ERROR( "Remote talk audio input init this->CreateAudioEncoder failed\n" );
#if !defined(_AV_PLATFORM_HI3520D_)&&!defined(_AV_PLATFORM_HI3520D_V300_)
        m_ptrHi_Ai->HiAi_stop_ai( _AI_TALKBACK_ );
#endif
        return -1;
    }

    if( m_ptrHi_Ai->HiAi_ai_bind_aenc( _AI_TALKBACK_, 0, this->GetTalkAudioEncoderChl() ) != 0 )
    {
        DEBUG_ERROR( "Remote talk audio bind m_ptrHi_Ai->HiAi_ai_bind_aenc failed\n" );
        this->DestroyAudioEncoder();
#if !defined(_AV_PLATFORM_HI3520D_)&&!defined(_AV_PLATFORM_HI3520D_V300_)
        m_ptrHi_Ai->HiAi_stop_ai( _AI_TALKBACK_ );
#endif
        return -1;
    }
    
    return 0;
}
int CAvRemoteTalk::SetTalkbackType(const rm_int32_t talkback_type)
{
    m_talkback_type = talkback_type;
	
	return 0;
}
int CAvRemoteTalk::GetTalkbackType() const
{
    return m_talkback_type;
}
int CAvRemoteTalk::CreateAudioEncoder()
{
    AENC_CHN_ATTR_S stuAencChnAttr;
    AENC_ATTR_ADPCM_S stuAdpcmParam;

    memset( &stuAencChnAttr, 0, sizeof(AENC_CHN_ATTR_S) );
    stuAencChnAttr.enType = PT_ADPCMA;
    stuAencChnAttr.u32BufSize = 40;
    stuAencChnAttr.pValue = &stuAdpcmParam;
#if defined(_AV_PLATFORM_HI3520D_V300_)
    stuAencChnAttr.u32PtNumPerFrm = 320;
#endif
    stuAdpcmParam.enADPCMType = ADPCM_TYPE_DVI4;

    //! for CB check criterion, G726 is only used
    char check_criterion[32]={0};
    pgAvDeviceInfo->Adi_get_check_criterion(check_criterion, sizeof(check_criterion));
    char customer_name[32] = {0};
    pgAvDeviceInfo->Adi_get_customer_name(customer_name,sizeof(customer_name));
    if ((!strcmp(check_criterion, "CHECK_CB")))
    {
        AENC_ATTR_G726_S stuG726Param;
        stuAencChnAttr.enType = PT_G726;
        stuAencChnAttr.u32BufSize = 40;
        stuAencChnAttr.pValue = &stuG726Param;
        stuG726Param.enG726bps = G726_16K;//MEDIA_G726_16K;//MEDIA_G726_40K;
    }
    else if (!strcmp(customer_name, "CUSTOMER_JIUTONG"))
    {
        AENC_ATTR_G726_S stuG726Param;
        stuAencChnAttr.enType = PT_G726;
        stuAencChnAttr.u32BufSize = 40;
        stuAencChnAttr.pValue = &stuG726Param;
        stuG726Param.enG726bps = G726_32K;

    }
    else if (!strcmp(customer_name, "CUSTOMER_04.1062"))
    {
        AENC_ATTR_G711_S stuG711Param;
        stuAencChnAttr.enType = PT_G711U;   //! 
        stuAencChnAttr.u32BufSize = 40;
        stuAencChnAttr.pValue = &stuG711Param;

    }

    HI_S32 sRet = HI_MPI_AENC_CreateChn( this->GetTalkAudioEncoderChl(), &stuAencChnAttr );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "HI_MPI_AENC_CreateChn FAILED width 0x%08x\n", sRet );
        return -1;
    }

    return 0;
}

int CAvRemoteTalk::UnInitAudioInput()
{
    if( m_ptrHi_Ai->HiAi_ai_unbind_aenc( _AI_TALKBACK_, 0, this->GetTalkAudioEncoderChl() ) != 0 )
    {
        DEBUG_ERROR( "Remote talk audio bind m_ptrHi_Ai->HiAi_ai_bind_aenc failed\n" );
    }

    if( this->DestroyAudioEncoder() != 0 )
    {
        DEBUG_ERROR( "Remote talk audio input init this->CreateAudioEncoder failed\n" );
    }
    
#if !defined(_AV_PLATFORM_HI3520D_)&&!defined(_AV_PLATFORM_HI3520D_V300_)
    if( m_ptrHi_Ai->HiAi_stop_ai(_AI_TALKBACK_) != 0 )
    {
        DEBUG_ERROR("Remote Talk m_ptrHi_Ai->HiAi_start_ai failed\n");
    }    
#endif
    

    
    return 0;
}

int CAvRemoteTalk::DestroyAudioEncoder()
{
    HI_S32 sRet = HI_MPI_AENC_DestroyChn( this->GetTalkAudioEncoderChl() );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR("HI_MPI_AENC_DestroyChn error with 0x%08x\n", sRet);
        return -1;
    }

    return 0;
}

AENC_CHN CAvRemoteTalk::GetTalkAudioEncoderChl()
{
    return pgAvDeviceInfo->Adi_max_channel_number();
}
    
int CAvRemoteTalk::StartTalk( const msgRequestAudioNetPara_t* pstuTalkParam )
{
    m_bNeedExitThread = false;
    char stream_buffer_name[32];
    unsigned int stream_buffer_size = 0;
    unsigned int stream_buffer_frame = 0;
    this->SetTalkbackType(pstuTalkParam->talkback_type);
    if(pstuTalkParam->talkback_type == 0 || pstuTalkParam->talkback_type == 1)
    {
        if( !m_bIsReciveTaskRunning )
        {
            DEBUG_MESSAGE("create remote talk reciving thread share=%s\n", pstuTalkParam->sendStreamBufferName);
            if( m_ShareMemReciveHandle )
            {
                N9M_SHDestroyShareBuffer(m_ShareMemReciveHandle);
                m_ShareMemReciveHandle = NULL;
            }
            if(pgAvDeviceInfo->Adi_get_stream_buffer_info(BUFFER_TYPE_TALKSET_STREAM, 0, stream_buffer_name, stream_buffer_size, stream_buffer_frame) != 0)
            {
                DEBUG_ERROR( "get buffer failed!\n");
                return -1;
            }
            m_ShareMemReciveHandle = N9M_SHCreateShareBuffer("avTalkReciver", stream_buffer_name, stream_buffer_size, stream_buffer_frame,  SHAREMEM_READ_MODE );
            if( !m_ShareMemReciveHandle )
            {
                return -1;
            }
            N9M_SHGotoLatestIFrame(m_ShareMemReciveHandle);

            if( pstuTalkParam->audioparam.audioFormat == AudioFormat_ADPCMA )
            {
                DEBUG_CRITICAL(" adpcm audio format\n");
                this->InitAudioOutput(0);
            }
            else if( pstuTalkParam->audioparam.audioFormat == AudioFormat_G726)
            {
                DEBUG_CRITICAL(" G726 audio format \n");
                this->InitAudioOutput(2);
            }
	        else if( pstuTalkParam->audioparam.audioFormat == AudioFormat_AACLC)
	        {
	            DEBUG_CRITICAL("AACLC audio format \n");
	            this->InitAudioOutput(3);
	        }
	        else if( pstuTalkParam->audioparam.audioFormat == AudioFormat_G711U)
	        {
	            DEBUG_CRITICAL(" G711U audio format \n");
	            this->InitAudioOutput(4);
	        }
            else
            {
                DEBUG_ERROR("Not recognise audio stream type\n");
            }
        	Av_create_normal_thread(TalkReciveThreadEntry, (void*)this, NULL);

			int tryTime = 0;
			while( tryTime < 5 && !m_bIsReciveTaskRunning )
			{
			    mSleep(10);
			}
        }
    }
    if(pstuTalkParam->talkback_type == 0 || pstuTalkParam->talkback_type == 2)
    {
        if( !m_bIsSendTaskRunning )
        {
            DEBUG_MESSAGE("create remote talk sending thread share=%s\n", pstuTalkParam->recvStreamBufferName);

            if( m_ShareMemSendHandle )
            {
                N9M_SHDestroyShareBuffer(m_ShareMemSendHandle);
                m_ShareMemSendHandle = NULL;
            }
            if(pgAvDeviceInfo->Adi_get_stream_buffer_info(BUFFER_TYPE_TALKGET_STREAM, 0, stream_buffer_name, stream_buffer_size, stream_buffer_frame) != 0)
            {
                DEBUG_ERROR( "get buffer failed!\n");
                return -1;
            }
            m_ShareMemSendHandle = N9M_SHCreateShareBuffer("avTalkSender", stream_buffer_name, stream_buffer_size, stream_buffer_frame,  SHAREMEM_WRITE_MODE );
            N9M_SHClearAllFrames(m_ShareMemSendHandle);
            this->InitAudioInput();

            Av_create_normal_thread(TalkSendThreadEntry, (void*)this, NULL);

            int tryTime = 0;
            while( tryTime < 5 && !m_bIsSendTaskRunning )
            {
                mSleep(10);
            }
        }
    }
    
    DEBUG_MESSAGE("Start remote talk over recivingTask=%d sendTask=%d\n", m_bIsReciveTaskRunning, m_bIsSendTaskRunning);

    return 0;
}

int CAvRemoteTalk::StopTalk(rm_int32_t talkback_type)
{
    DEBUG_MESSAGE("Stop remote talk\n");
    
    m_bNeedExitThread = true;
    if(talkback_type == 0 || talkback_type == 1)
    {
        while( m_bIsReciveTaskRunning )
        {
            mSleep(10);
        }
        if( m_ShareMemReciveHandle )
        {
            N9M_SHDestroyShareBuffer(m_ShareMemReciveHandle);
            m_ShareMemReciveHandle = NULL;
        }
        this->UnitAudioOutput();
    }
    if(talkback_type == 0 || talkback_type == 2)
    {
        while( m_bIsSendTaskRunning )
        {
            mSleep(10);
        }
        if( m_ShareMemSendHandle )
        {
            N9M_SHDestroyShareBuffer(m_ShareMemSendHandle);
            m_ShareMemSendHandle = NULL;
        }
        this->UnInitAudioInput();
    }

    DEBUG_MESSAGE("Stop remote talk over\n");
    return 0;
}

void *TalkSendThreadEntry( void *pThreadParam )
{
    CAvRemoteTalk* pTask = (CAvRemoteTalk*)pThreadParam;
	prctl(PR_SET_NAME, "TalkSend");

    pTask->SendTalkThreadBody();
     
    return NULL;
}

void *TalkReciveThreadEntry( void *pThreadParam )
{
    CAvRemoteTalk* pTask = (CAvRemoteTalk*)pThreadParam;
	prctl(PR_SET_NAME, "TalkRecive");

    pTask->ReciveTalkThreadBody();
    
    return NULL;
}

unsigned int CalculateCheckSum(TalkFrameData_t *pData, int len)
{
    unsigned int CheckSum = 0;
    int count;
    unsigned int interval;
    unsigned char *ptr;

    if(len < 128)
    {
        count = len;
        interval = 1;
    }
    else
    {
        interval = len/128;
        count = 128;
    }

    int PackIndex = pData->PackCount-1;
    unsigned int PackLen;
    char index;
    if(pData->DataPack[PackIndex].Len[1] > 0)
    {
        PackLen = pData->DataPack[PackIndex].Len[1];
        ptr = pData->DataPack[PackIndex].pAddr[1]+PackLen-1;
        index = 1;
    }
    else
    {
        PackLen = pData->DataPack[PackIndex].Len[0];
        ptr = pData->DataPack[PackIndex].pAddr[0]+PackLen-1;
        index = 0;
    }

    PackLen--;

    for(int i=0; i<count; i++)
    {
        CheckSum += ptr[0];
        if(i >= count-1)
        {
            break;
        }
AGAIN:
        if(PackLen >= interval)
        {
            PackLen -= interval;
            ptr -= interval;
        }
        else
        {
            if(index == 0)
            {
                PackIndex--;
                if(PackIndex<0)
                {
                    DEBUG_ERROR("\n\nCalculateCheckSum err0:PackIndex:%d i:%d count:%d\n\n", PackIndex,i,count);
                    fflush(stdout);
                }

                if(pData->DataPack[PackIndex].Len[1] > 0)
                {
                    if(pData->DataPack[PackIndex].Len[1] >= (interval-PackLen))
                    {
                        ptr = pData->DataPack[PackIndex].pAddr[1]+pData->DataPack[PackIndex].Len[1]-(interval-PackLen);
                        PackLen = pData->DataPack[PackIndex].Len[1]-(interval-PackLen);
                        index = 1;
                    }
                    else
                    {
                        PackLen += pData->DataPack[PackIndex].Len[1];
                        index = 1;
                        goto AGAIN;
                    }
                }
                else
                {
                    if(pData->DataPack[PackIndex].Len[0] >= (interval-PackLen))
                    {
                        ptr = pData->DataPack[PackIndex].pAddr[0]+pData->DataPack[PackIndex].Len[0]-(interval-PackLen);
                        PackLen = pData->DataPack[PackIndex].Len[0]-(interval-PackLen);
                        index = 0;
                    }
                    else
                    {
                        PackLen += pData->DataPack[PackIndex].Len[0];
                        index = 0;
                        goto AGAIN;
                    }
                }
            }
            else
            {
                if(pData->DataPack[PackIndex].Len[0] >= (interval-PackLen))
                {
                    ptr = pData->DataPack[PackIndex].pAddr[0]+pData->DataPack[PackIndex].Len[0]-(interval-PackLen);
                    PackLen = pData->DataPack[PackIndex].Len[0]-(interval-PackLen);
                    index = 0;
                }
                else
                {
                    PackLen += pData->DataPack[PackIndex].Len[0];
                    index = 0;
                    goto AGAIN;
                }
            }
        }
    }

    return (CheckSum & 0x00ff);
}


#include "AvReportStation.h"
#ifdef _AV_PLATFORM_HI3515_
#include "mpi_sys.h"
#define MP3_FRAME_LEN 1152*2 //! samplerate 44.1k
#endif
using namespace Common;

//#define _SAVE_PCM_
#ifdef _SAVE_PCM_
FILE *fp_mp3;
#endif

CAvReportStation::CAvReportStation()
{
    m_ePlayState = STATION_THREAD_STOP;
    m_bIsTaskRunning = false;
    m_bInteruptTask = false;
    m_bNeedExitThread = true;
    m_bLoopTask = false; //!< 0307
    //m_bNewAddTask = false;
    m_isFinish = false;
    m_binit = false;
    m_iLeftByte = 0;
    m_pThread_lock = new CAvThreadLock;
    m_ptrHi_Ao = new CHiAo();
#if defined(_AV_HAVE_VIDEO_DECODER_) && defined(_AV_PLATFORM_HI3515_)
    m_pHi_adec = new CHiAvDec;
#endif
    memset(&mp3LastFrameInfo, 0, sizeof(MP3FrameInfo));
    m_RightLeftChn = 0;
    
}
CAvReportStation::~CAvReportStation()
{
    m_bNeedExitThread = false;
    m_bIsTaskRunning = false;
    
    AUDIO_DEV AudioDevId = 2;
    AO_CHN AoChn = 0;
    m_ptrHi_Ao->HiAo_get_ao_info(_AO_PLAYBACK_CVBS_, &AudioDevId, &AoChn/*, &AoAttr*/);
    this->UninitAudioOutput(AudioDevId, AoChn);
    if(m_pThread_lock)
    {
        delete m_pThread_lock;
        m_pThread_lock = NULL;
    }
    if(m_ptrHi_Ao)
    {
        delete m_ptrHi_Ao;
        m_ptrHi_Ao = NULL;
    }
#if defined(_AV_HAVE_VIDEO_DECODER_) && defined(_AV_PLATFORM_HI3515_)
    if(m_pHi_adec)
    {
        delete m_pHi_adec;
        m_pHi_adec = NULL;
    }
#endif    
}

void *ReportStationThreadEntry( void *pThreadParam )
{
	DEBUG_WARNING("##########ReportStationThreadEntry\n");
	
	prctl(PR_SET_NAME, "ReportStation");
	CAvReportStation* pTask = (CAvReportStation*)pThreadParam;
    pTask->ReportStationThreadBody();
    
    return NULL;
}
int CAvReportStation::StartReportStationThread()
{
	
	DEBUG_WARNING("#######start station broadcast thread, is running=%d\n", m_bIsTaskRunning);
    if( m_bIsTaskRunning )
    {
        DEBUG_CRITICAL("####### thread, is running return \n");
        return 0;
    }
    //AUDIO_DEV AudioDevId;
    //AO_CHN AoChn;
    m_bNeedExitThread = false;
    m_bInteruptTask = false;
    m_ePlayState= STATION_THREAD_START;
    /*
    m_ptrHi_Ao->HiAo_stop_ao(_AO_PLAYBACK_CVBS_);
    //添加初始化ao
    m_ptrHi_Ao->HiAo_get_ao_info(_AO_PLAYBACK_CVBS_, &AudioDevId, &AoChn);
    if(AoChn != this->GetAudioAoChn())
    {
        AoChn = this->GetAudioAoChn();
    }
    this->InitAudioOutput(AudioDevId, AoChn);
    */
    Av_create_normal_thread(ReportStationThreadEntry, (void*)this, NULL);

    while(m_bIsTaskRunning != true)
    {
        mSleep(10);
    }

    DEBUG_WARNING("CAvReportStation::StartReportStationThread create thread ok\n");
	
    return 0;
}


int CAvReportStation::StopReportStationThread()
{
    DEBUG_WARNING("CAvReportStation::StopReportStationThread\n");
    m_bNeedExitThread = true;
    AUDIO_DEV AudioDevId;
    AO_CHN AoChn;
    m_ePlayState = STATION_THREAD_STOP;
    m_bInteruptTask = true;
    m_bLoopTask = false;
    m_iLeftByte = 0;
    
    while(m_bIsTaskRunning != false)//等待线程停止后再销毁A0
    {
        mSleep(10);
        printf("m_bIsTaskRunning = false\n");
    }
    if(!m_bIsTaskRunning && m_binit)
    {
        m_ptrHi_Ao->HiAo_get_ao_info(_AO_PLAYBACK_CVBS_, &AudioDevId, &AoChn/*, &AoAttr*/);
        if(AoChn != this->GetAudioAoChn())
        {
            AoChn = this->GetAudioAoChn();
        }
        this->UninitAudioOutput(AudioDevId, AoChn);
    }
    
    this->ClearListTask();//中断发生后立马清空任务
    
    return 0;
}

int CAvReportStation::ReportStationThreadBody()
{
#ifdef _AV_PLATFORM_HI3515_
    if(setpriority(PRIO_PROCESS, getpid(), -18) != 0) //!< setpriority(PRIO_PROCESS, 0,  -10)
    {
        printf("set priority failed!\n");
    }
#endif
    DEBUG_WARNING("CAvReportStation::ReportStationThreadBody\n");

    SetReportStationResult(4096);//MP3_MAINBUF_SIZE
    m_bIsTaskRunning = true;
    HI_S32 s32Ret = HI_SUCCESS;
    FILE *pfMP3 = NULL;
    HMP3Decoder hMP3Decoder =0;
    int bytesLeft = 0;
    int nRead = 0; 
    unsigned char readBuf[MP3_MAINBUF_SIZE], *readPtr;
    short outBuf[MP3_MAX_NCHANS * MP3_MAX_OUT_NSAMPS];
    MP3FrameInfo mp3FrameInfo;
    AUDIO_DEV AudioDevId = 2;
    AO_CHN AoChn = 0;
    m_ptrHi_Ao->HiAo_get_ao_info(_AO_PLAYBACK_CVBS_, &AudioDevId, &AoChn/*, &AoAttr*/);
    if(AoChn != this->GetAudioAoChn())
    {
        AoChn = this->GetAudioAoChn();
    }
    eProductType Type = PTD_INVALID; 
    N9M_GetProductType(Type);
    //InitAudioOutput(AudioDevId, AoChn, NULL);
    int nFrames = 0;
    while( !m_bNeedExitThread )
    {
        if (m_ePlayState == STATION_THREAD_STOP)
		{
			mSleep(10);
			continue;
		}       
        //msgReportStation_t pstuReportStation;
        std::string pstuReportStation;
        while( GetReportStationTask(&pstuReportStation) )
        {
            //struct timeval timer;
            //memset(&timer, 0, sizeof(timeval));
            //gettimeofday(&timer, NULL);
            //DEBUG_CRITICAL("\n open MP3 name =%s ms =%llu \n",pstuReportStation.c_str(), timer.tv_usec);//N9M_TMGetSystemMicroSecond
            //m_bInteruptTask = false;
            readPtr = NULL;
            memset(readBuf, 0, sizeof(unsigned char)*MP3_MAINBUF_SIZE);
            memset(outBuf, 0, sizeof(short)*MP3_MAX_NCHANS * MP3_MAX_OUT_NSAMPS);
            DEBUG_CRITICAL("----MP3 pstuReportStation.AudioAddr = %s size =%d\n", pstuReportStation.c_str(), listReportStation.size());
            do
            {
            if((pfMP3 = OpenPlayFile(pstuReportStation)) == NULL)
    		{
    			DEBUG_ERROR(" *** OpenPlayFile Failed ***\n");
    			continue;
    		}

    		SetReportStationResult(4096);//MP3_MAINBUF_SIZE
    		
    		//*> 根据MP3文件是否带帧头信息判断是否跳过帧头解码 2015.03.10
    		char  buffer[10];
    		fread(buffer, 1, 10, pfMP3);
    		// mp3 tag header mark is 73 68 51 or not tag header 
    		if((buffer[0] == 73 ) && (buffer[1] == 68 ) && (buffer[2] == 51 ))
    		{
                //计算标签大小
                int ID3V2_frame_size = ((int)( buffer[6] & 0x7F) << 21)
                 | ((int)(buffer[7] & 0x7F) << 14)
                  | ((int)(buffer[8] & 0x7F) << 7)
                  | ((int)(buffer[9]  & 0x7F) + 10);/*每一帧包括帧头和数据，各个帧相互独立，10 为帧头大小*/
                fseek(pfMP3, ID3V2_frame_size, SEEK_SET);/*此时指向的mp3实体数据位置*/
            }
            else
            {
                fseek(pfMP3, 0, SEEK_SET);
            }
            
            hMP3Decoder = this->OpenMP3Decoder();
            if(hMP3Decoder == 0)
            {
               DEBUG_ERROR(" *** HI_MP3InitDecoder Failed ***\n");
 			   return 0;//NULL; 
            }

            bytesLeft = 0;
    		readPtr = readBuf;
    		nRead = 0;
    		nFrames = 0;
            
            while((m_bInteruptTask == false) && (m_ePlayState == STATION_THREAD_START))
            {
                if (bytesLeft < MP3_MAINBUF_SIZE )
    			{

                    nRead = FillReadBuffer(readBuf, readPtr, MP3_MAINBUF_SIZE, bytesLeft, pfMP3);
    				bytesLeft += nRead;  
    				readPtr = readBuf;
    			}
                
                if (0 == nRead || bytesLeft == 0)
    			{
    				//DEBUG_ERROR("FillReadBuffer error \n");
                    //m_bInteruptTask = true;
    				break;
    			}

                while((m_bInteruptTask == false) && (m_ePlayState == STATION_THREAD_START))
                {
                    s32Ret = StartMP3Decoder(hMP3Decoder, &readPtr, &bytesLeft, outBuf, &mp3FrameInfo);
        			if( s32Ret != 0 )
        			{
        				//DEBUG_ERROR(" StartMP3Decoder failed \n");
        				break;
        			}
                    ///*
                    if(nFrames == 0)//如果是第一次,要初始化一下
            		{
            			//StopAllAudioOutput();
            			//InitAudioOutput(&mp3FrameInfo,taskInfo->outp, pAoChn, pAdecChn);
            			m_ptrHi_Ao->HiAo_stop_ao(_AO_PLAYBACK_CVBS_);
                        //添加初始化ao
                        m_ptrHi_Ao->HiAo_get_ao_info(_AO_PLAYBACK_CVBS_, &AudioDevId, &AoChn);
                        if(AoChn != this->GetAudioAoChn())
                        {
                            AoChn = this->GetAudioAoChn();
                        }
                        this->InitAudioOutput(AudioDevId, AoChn, &mp3FrameInfo);
            		}
            	    
            		if ((mp3FrameInfo.samprate != mp3LastFrameInfo.samprate)|| \
            			(mp3FrameInfo.bitsPerSample != mp3LastFrameInfo.bitsPerSample)|| \
            			(mp3FrameInfo.nChans != mp3LastFrameInfo.nChans)|| \
            			(mp3FrameInfo.outputSamps != mp3LastFrameInfo.outputSamps))
            		{
            			
            			m_ptrHi_Ao->HiAo_stop_ao(_AO_PLAYBACK_CVBS_);
            			///*添加初始化ao
                        m_ptrHi_Ao->HiAo_get_ao_info(_AO_PLAYBACK_CVBS_, &AudioDevId, &AoChn);
                        if(AoChn != this->GetAudioAoChn())
                        {
                            AoChn = this->GetAudioAoChn();
                        }
                        this->InitAudioOutput(AudioDevId, AoChn, &mp3FrameInfo);
            		}
            		
            		nFrames++;
                    if(((Type == PTD_D5) ||(Type == PTD_D5M)||(Type==PTD_D5_35))  && mp3FrameInfo.samprate == 44100)
                    {
                        continue;
                    }
                    else if(((Type == PTD_A5_II)||(Type == PTD_6A_I)) && (mp3FrameInfo.samprate != 44100))
                    {
                        continue;
                    }
                    else
                    {
                            if (PlayRawData(AudioDevId, AoChn, &mp3FrameInfo, (HI_U16 *)outBuf))
                            {
                                SetReportStationResult(0);
                                m_bIsTaskRunning = false; 
                                m_ePlayState = STATION_THREAD_STOP;
                                CloseMP3Decoder(hMP3Decoder);
                                fclose(pfMP3);
                                pfMP3 = NULL;
                            	DEBUG_CRITICAL("PlayRawData failed!\n");
#ifdef _SAVE_PCM_
                            if (fp_mp3 != NULL)
                            {
                                fclose(fp_mp3);
                            }
#endif
                                return 0;
                            }
                        }
                    //SetReportStationResult(bytesLeft);//查询播放状态 bytesLeft>0 播放中  bytesLeft =0 播放完成
                }
               
            }
            
            if((m_bInteruptTask == true) /*&& (bytesLeft > 0)*/)
            {
                SetReportStationResult(0);
                m_bIsTaskRunning = false; 
                m_ePlayState = STATION_THREAD_STOP;
                CloseMP3Decoder(hMP3Decoder);
                fclose(pfMP3);
                pfMP3 = NULL;
                    DEBUG_MESSAGE("Interrupted!\n");
#ifdef _SAVE_PCM_
                if (fp_mp3 != NULL)
                {
                    fclose(fp_mp3);
                }
#endif
                return 0;
            }
            
            CloseMP3Decoder(hMP3Decoder);
            fclose(pfMP3);
            pfMP3 = NULL;
#ifdef _SAVE_PCM_
            if (fp_mp3 != NULL)
            {
                fclose(fp_mp3);
            }
#endif
         	//memset(&timer, 0, sizeof(timeval));
            //gettimeofday(&timer, NULL);
            //DEBUG_CRITICAL("\n finish MP3 name =%s ms =%llu \n",pstuReportStation.c_str(), timer.tv_usec);
            if((Type == PTD_D5) ||(Type == PTD_D5M)||(Type==PTD_D5_35)||(Type==PTD_X1)||(Type==PTD_X1_AHD)||(Type == PTD_A5_II)||(Type == PTD_6A_I))
            {
                mSleep(300); //处理站名结尾少读现象
            }
            if ((Type == PTD_A5_II)||(Type == PTD_6A_I))
            {
                mSleep(700);
            }
	     if(Type==PTD_D5_35)
	     {
		  usleep(200*1000);
	     }
                if(m_bLoopTask)
                {
                    usleep(100 * 1000);
                }
            }while(m_bLoopTask);//!< 0307
        }
        //report's state is finished if all audio of current task are reported , or state is playing 
        
        SetReportStationResult(0);
        m_ePlayState = STATION_THREAD_STOP;
        
    }
    
    m_bIsTaskRunning = false;
   
    return 0;  
}


int CAvReportStation::InitAudioOutput(AUDIO_DEV AudioDevId, AO_CHN AoChn, MP3FrameInfo *mp3FrameInfo)
{
    int ret = -1;
	AIO_ATTR_S stAttr;
    memset(&stAttr, 0, sizeof(AIO_ATTR_S));
    AUDIO_DEV DevId;
    AO_CHN Chn;
    //0717
#ifdef _AV_PLATFORM_HI3521_
    if(HI_MPI_AO_ClrPubAttr(AudioDevId) != 0)
    {
        DEBUG_ERROR( "InitAudioOutput (ai_dev:%d) FAILED!\n", AudioDevId);
        return -1;
    }
#endif
    m_ptrHi_Ao->HiAo_get_ao_info(_AO_PLAYBACK_CVBS_, &DevId, &Chn , &stAttr);
	DevId = AudioDevId;
    Chn = AoChn;

    HI_U32 tmpFreq = 0;
    
    if(mp3FrameInfo != NULL)
    {
    	switch(mp3FrameInfo->samprate)
    	{
    		case 8000:    /* 8K samplerate*/
    			stAttr.enSamplerate =AUDIO_SAMPLE_RATE_8000;
    			break;
        	case 11025:  /* 11.025K samplerate*/
    			stAttr.enSamplerate =AUDIO_SAMPLE_RATE_11025;
    			break;
        	case 16000:   /* 16K samplerate*/
    			stAttr.enSamplerate =AUDIO_SAMPLE_RATE_16000;
    			break;
        	case 22050:   /* 22.050K samplerate*/
    			stAttr.enSamplerate =AUDIO_SAMPLE_RATE_22050;
    			break;
       		case 24000:   /* 24K samplerate*/
    			stAttr.enSamplerate =AUDIO_SAMPLE_RATE_24000;
    			break;
        	case 32000:   /* 32K samplerate*/
    			stAttr.enSamplerate =AUDIO_SAMPLE_RATE_32000;
                tmpFreq = 0x45218DEF;
    			break;
        	case 44100:   /* 44.1K samplerate*/
    			stAttr.enSamplerate =AUDIO_SAMPLE_RATE_44100;
                tmpFreq = 0x452E3E00;
    			break;
        	case 48000:  /* 48K samplerate*/
    			stAttr.enSamplerate =AUDIO_SAMPLE_RATE_48000;
    			break;
    		default:
    			break;
    	}
        stAttr.u32PtNumPerFrm = mp3FrameInfo->outputSamps / mp3FrameInfo->nChans;//160;//320;//160;//210;//160;//320 每帧的采样点数
    }
    else
    {
        stAttr.enSamplerate =AUDIO_SAMPLE_RATE_44100;
        stAttr.u32PtNumPerFrm = 1152;
    }
    DEBUG_WARNING("mp3 bitrate =%d bitsPerSample =%d layer =%d nChans =%d \n",\
        mp3FrameInfo->bitrate,\
        mp3FrameInfo->bitsPerSample,\
        mp3FrameInfo->layer, \
        mp3FrameInfo->nChans, \
        mp3FrameInfo->outputSamps, \
        mp3FrameInfo->samprate
        );
    DEBUG_WARNING("sample rate =%d u32PtNumPerFrm =%d mp3 nChans =%d \n",stAttr.enSamplerate, stAttr.u32PtNumPerFrm, mp3FrameInfo->nChans);
    ret = HI_MPI_AO_Disable(DevId);
    if(ret != 0)
	{
    	DEBUG_ERROR("HI_MPI_AO_Disable err 0x%x\n",ret);
	}  
#ifdef _AV_PLATFORM_HI3515_
    stAttr.u32FrmNum = 20;
#endif

	ret = HI_MPI_AO_SetPubAttr(DevId, &stAttr);
	if(ret != 0)
	{
    	DEBUG_ERROR("HI_MPI_AO_SetPubAttr err-01 0x%x\n",ret);
        ret = HI_MPI_AO_SetPubAttr(DevId, &stAttr);
        if(ret != 0)
    	{
        	DEBUG_ERROR("HI_MPI_AO_SetPubAttr err-02 0x%x\n",ret);
            HI_MPI_AO_Disable(DevId);
    		return -1;	
    	}
		
	}

#ifdef _AV_PLATFORM_HI3515_
    HI_MPI_SYS_SetReg(0x20050064,tmpFreq);
#else
	tmpFreq += 0;
#endif

	if(HI_MPI_AO_Enable(DevId) != 0)
	{
    	DEBUG_ERROR("HI_MPI_AO_Enable err\n");
		return -1;	
	}
	ret = HI_MPI_AO_EnableChn(DevId, Chn);
	if(ret != 0)
	{
    	DEBUG_ERROR("HI_MPI_AO_EnableChn err ret=%x AudioDevId=%d AoChn=%d\n",ret,AudioDevId,AoChn);
		return -1;	
	}
    //! clear ao 01.25.2016
    if ((ret =HI_MPI_AO_ClearChnBuf(DevId, Chn)) != 0)
    {
         DEBUG_ERROR( "HI_MPI_AO_ClearChnBuf FAILED\
                       (ao_dev:%d)(ao_chn:%d)(0x%08lx)\n", DevId, Chn, (unsigned long)ret);  
    }   
    if(mp3FrameInfo != NULL)
    {
        memset(&mp3LastFrameInfo, 0, sizeof(MP3FrameInfo));
        memcpy(&mp3LastFrameInfo, mp3FrameInfo, sizeof(MP3FrameInfo));
    }
#ifdef _AV_PLATFORM_HI3515_
#ifdef _AV_HAVE_VIDEO_DECODER_
//create lpcm audio decoder
    int adec_chn = ADEC_CHN_PLAYBACK;
    eAudioPayloadType type = APT_LPCM;
    
    if (m_pHi_adec->Ade_destroy_adec(adec_chn) != 0)
    {
        DEBUG_ERROR("Ade_destroy_adec FAILED! (adec_chn:%d) \n", adec_chn);
    }
    if (m_pHi_adec->Ade_create_adec(adec_chn, type) !=0)
    {
        DEBUG_ERROR("Ade_create_adec failed!(adec_chn:%d) \n", adec_chn);
        return -1;
    }
    //bind adec to ao
    
    int ret_val = -1;
    if ((ret_val = m_ptrHi_Ao->HiAo_adec_bind_ao(_AO_PLAYBACK_CVBS_, AoChn,adec_chn)) != 0)
    {
         DEBUG_ERROR( "CHiAo::HiAo_adec_bind_ao HiAo_adec_bind_ao FAILED (ao_type:%d) (adec_chn:%d) (0x%08lx)\n", _AO_PLAYBACK_CVBS_, adec_chn, (unsigned long)ret_val);
        return -1;
    } 
#endif
#endif

    m_binit = true;
	return 0;
}
int CAvReportStation::UninitAudioOutput(AUDIO_DEV AudioDevId, AO_CHN AoChn)
{
    /*if( m_ptrHi_Ao->HiAo_stop_ao(_AO_PLAYBACK_CVBS_) != 0 )
    {
        DEBUG_ERROR("Stop failed\n");
    }
    */
    int ret_val = -1;
   
#ifdef _AV_PLATFORM_HI3515_
#ifdef _AV_HAVE_VIDEO_DECODER_

    int adec_chn = ADEC_CHN_PLAYBACK;
    if ((ret_val = m_ptrHi_Ao->HiAo_adec_unbind_ao(_AO_PLAYBACK_CVBS_,AoChn, adec_chn)) != 0)
    {
        DEBUG_ERROR( "CHiAo::HiAo_adec_bind_ao HiAo_adec_unbind_ao FAILED (ao_type:%d) (adec_chn:%d)(0x%08lx)\n", _AO_PLAYBACK_CVBS_, adec_chn, (unsigned long)ret_val);
        return -1;
    }
    if (m_pHi_adec->Ade_destroy_adec(adec_chn) != 0)
    {
        DEBUG_ERROR("Ade_destroy_adec FAILED! (adec_chn:%d) \n", adec_chn);
        return -1;
    }
#endif
#endif

    if ((ret_val = HI_MPI_AO_DisableChn(AudioDevId, AoChn)) != 0)
    {
        DEBUG_ERROR( "HI_MPI_AO_DisableChn FAILED(ao_dev:%d)(ao_chn:%d)(0x%08lx)\n", AudioDevId, AoChn, (unsigned long)ret_val);
        return -1;
    }

    DEBUG_ERROR( "HI_MPI_AO_DisableChn FAILED(ao_dev:%d)(ao_chn:%d)(0x%08lx)\n", AudioDevId, AoChn, (unsigned long)ret_val);

    if ((ret_val = HI_MPI_AO_Disable(AudioDevId)) != 0)
    {
        DEBUG_ERROR( "HI_MPI_AO_Disable FAILED(ao_dev:%d)(0x%08lx)\n", AudioDevId, (unsigned long)ret_val);
        return -1;
    }
    //0717
#ifdef _AV_PLATFORM_HI3521_
    if(HI_MPI_AO_ClrPubAttr(AudioDevId) != 0)
    {
        DEBUG_ERROR( "UninitAudioOutput (ai_dev:%d) FAILED!\n", AudioDevId);
        return -1;
    }
#endif
    m_binit = false;
	return 0;
}

int CAvReportStation::SetAudioAoChn(const int rightleftchn)
{
    m_RightLeftChn = rightleftchn;
    return 0;
}

int CAvReportStation::PlayRawData(AUDIO_DEV AudioDevId, AO_CHN AoChn, MP3FrameInfo *pInfo, HI_U16 *pBuf)
{
    //printf("AudioDevId =%d AoChn =%d\n",AudioDevId, AoChn);
    int datalen = (pInfo->outputSamps/pInfo->nChans)*(pInfo->bitsPerSample/8); //2304;//

#ifdef _SAVE_PCM_
    if(fp_mp3 != NULL)
    {
        fwrite((HI_U8 *)pBuf, 1, datalen, fp_mp3);
    }
#endif
	
#if defined(_AV_PLATFORM_HI3515_)
    // pack is only supported
    AUDIO_STREAM_S audio_stream;
    audio_stream.u32Len = datalen;
    audio_stream.u32Seq = 0;
    audio_stream.u64TimeStamp = 0;
    audio_stream.pStream = (HI_U8 *)pBuf;
    
    //fwrite(audio_stream.pStream, 1, datalen, fp_mp3);
#else
	AUDIO_FRAME_S stAudioFrame;
    memset(&stAudioFrame, 0, sizeof(AUDIO_FRAME_S));
	stAudioFrame.enBitwidth = AUDIO_BIT_WIDTH_16;
	stAudioFrame.enSoundmode = AUDIO_SOUND_MODE_MONO;
    stAudioFrame.pVirAddr[0] = pBuf;
    stAudioFrame.pVirAddr[1] = pBuf+datalen;
    stAudioFrame.u32Seq = 0;
    stAudioFrame.u64TimeStamp = 0;
	stAudioFrame.u32Len = datalen;
 #endif   
    int Retval = -1;
    if(this->GetReportState())
    {
#if defined(_AV_PLATFORM_HI3535_)||defined(_AV_PLATFORM_HI3520D_V300_)
        Retval = HI_MPI_AO_SendFrame(AudioDevId, AoChn, &stAudioFrame, -1);
#elif defined(_AV_PLATFORM_HI3515_)
#if defined(_AV_HAVE_VIDEO_DECODER_)
        Retval = HI_MPI_ADEC_SendStream(ADEC_CHN_PLAYBACK, &audio_stream, HI_IO_BLOCK);
#endif
		if(Retval != 0)
		{
		   DEBUG_ERROR("HI_MPI_ADEC_SendStream failed....0x%x\n", Retval);
		   return -1;
		}
#else         
    	Retval = HI_MPI_AO_SendFrame(AudioDevId, AoChn, &stAudioFrame, HI_TRUE);
#endif
        if(Retval != 0)
        {
           DEBUG_ERROR("HI_MPI_AO_SendFrame failed....0x%x\n", Retval);
           if(Retval != (int)0xA016800F)  //0xA0168005
           {
                if ((int)0xA0168005 == Retval)
                {
                   return -1;// added to stop when ao is unavailable
                }
           }
        }
    }
    return 0;
}


int CAvReportStation::FillReadBuffer(unsigned char *pReadBuf, unsigned char *pReadPtr, int BufSize, int BytesLeft, FILE *pFile)
{
    int nRead = 0;
	int nRead2 = 0; // (L40186) 
    
	memmove(pReadBuf, pReadPtr, BytesLeft);
	nRead = fread(pReadBuf + BytesLeft, 1, BufSize - BytesLeft, pFile);
	
    if (nRead < BufSize - BytesLeft) 
    {
        memset(pReadBuf + BytesLeft + nRead, 0, BufSize - BytesLeft - nRead);
    }

    return (nRead + nRead2);
}

HMP3Decoder CAvReportStation::OpenMP3Decoder()
{
#if defined(_AV_PLATFORM_HI3515_)
	return HI_MP3InitDecoder();
#else
    return MP3InitDecoder();
#endif
}

int CAvReportStation::CloseMP3Decoder(HMP3Decoder hMP3Decoder)
{
#if defined(_AV_PLATFORM_HI3515_)
	HI_MP3FreeDecoder(hMP3Decoder);
#else
    MP3FreeDecoder(hMP3Decoder);
#endif
    hMP3Decoder = NULL;
    return 0;
}

int CAvReportStation::StartMP3Decoder(HMP3Decoder hMP3Decoder, HI_U8 **ppInbufPtr, HI_S32 *pBytesLeft, HI_S16 *pOutPcm, MP3FrameInfo *mp3FrameInfo)
{
    HI_S32 s32Ret = HI_SUCCESS;
    HI_S32 s32FrameLen;
#if defined(_AV_PLATFORM_HI3515_)	
    s32FrameLen = HI_MP3DecodeFindSyncHeader(hMP3Decoder, ppInbufPtr, pBytesLeft);
#else
	s32FrameLen = MP3DecodeFindSyncHeader(hMP3Decoder, ppInbufPtr, pBytesLeft);
#endif
                    
	if( s32FrameLen<0 )// do not find frame header exit while
	{
		DEBUG_ERROR(" ***( frameBytes<0 ) ***\n");
		return -1;
	}
#if defined(_AV_PLATFORM_HI3515_)    
    s32Ret = HI_MP3Decode(hMP3Decoder, ppInbufPtr, pBytesLeft, pOutPcm, 0);
#else
	s32Ret = MP3Decode(hMP3Decoder, ppInbufPtr, pBytesLeft, pOutPcm, 0);
#endif
	if (s32Ret)
	{
        //DEBUG_ERROR(" ***MP3Decode failed *** s32Ret =%d\n",s32Ret);
        return -1;
		
	}
	else/* no error */
	{ 
        //DEBUG_ERROR("decoder succcess\n");
#if defined(_AV_PLATFORM_HI3515_)         
		HI_MP3GetLastFrameInfo(hMP3Decoder, mp3FrameInfo);
#else
		MP3GetLastFrameInfo(hMP3Decoder, mp3FrameInfo);
#endif
        /*
		printf("bitrate: %d    bitsPerSample: %d    nChans: %d    outputSamps: %d     samprate:%d\n", 
			mp3FrameInfo->bitrate,
			mp3FrameInfo->bitsPerSample, 
			mp3FrameInfo->nChans, 
			mp3FrameInfo->outputSamps, 
			mp3FrameInfo->samprate);*/
    }
    return 0;
    
}

FILE* CAvReportStation::OpenPlayFile(const std::string pFileName)
{
    FILE * pFile = NULL;
	m_pThread_lock->lock();
	if(!pFileName.empty())
	{
		if((pFile = fopen(pFileName.c_str(),"rb")) != NULL)
		{
			m_ePlayState = STATION_THREAD_START;
		}
		else
		{
			fprintf(stderr, "Open MP3 File %s Failed\n", pFileName.c_str());
			perror(pFileName.c_str());
		}
		//bzero(pFileName,sizeof(pFileName));
#ifdef _SAVE_PCM_
        char fn[100] = {0};
        char fn2[100] = {0};
        memcpy(fn, pFileName.c_str(), 100);
        printf("fn:%s, filename:%s\n", fn, pFileName.c_str());
        char * p_slash;
        for(int i=0;i<50;i++)
        {
            if(fn[i] == '/')
            {
               p_slash = fn+i;
            }
        } 
        sprintf(fn2, "/mnt/usbstate/front/report_pcm/%s", (p_slash+1));
        DEBUG_CRITICAL("to open:%s !\n", fn2);
	    fp_mp3 = fopen(fn2, "wb+");
        if (fp_mp3 == NULL)
        {
            DEBUG_ERROR("open pcm file failed!\n");
        }
        else
        {
            DEBUG_CRITICAL("open pcm file success!\n");
        }
        DEBUG_CRITICAL("open complete!\n");
#endif
	}
	else 
	{
		fprintf(stderr, "Invalid MP3 File \n");
	}

	m_pThread_lock->unlock();
	return pFile;
}


int CAvReportStation::AddReportStationTask( char * addr, int RightLeftChn)
{
    DEBUG_WARNING("CAvReportStation::AddReportStationTask\n");
    m_pThread_lock->lock();
    std::list<msgReportStation_t>::iterator iter;

    if(!listReportStation.empty())
    {  
        /* 1. only clear auto type element and contain only one element, other types can coexist
        for(iter=listReportStation.begin(); iter!=listReportStation.end(); )
        {
           if(iter->ucMessageType == 0)
           {
                iter = listReportStation.erase(iter);
           }
           else
           {
                iter++;
           }
        }*/
        
       // 2. clear all elements, list only contain one element
       //listReportStation.clear();
       
       listReportStation.push_back(addr);  
    }
    else
    {
       listReportStation.push_back(addr); 
    }
    
    //printf("----list length =%d\n", listReportStation.size());
    //m_bInteruptTask = true;
    this->SetAudioAoChn(RightLeftChn);
	m_pThread_lock->unlock();
    return 0;
}
bool CAvReportStation::ClearListTask()
{
    if(!listReportStation.empty())
    {
        listReportStation.clear(); 
    }
    else
    {
        return false;
    }
    
    return true;
}

bool CAvReportStation::SetTaskRuningState(bool brun)
{
    m_bIsTaskRunning = brun;
    return true;
}

bool CAvReportStation::GetReportStationTask(std::string *addr)
{   
    //printf("listReportStation length =%d\n", listReportStation.size());
    if(!listReportStation.empty())
    {
        m_pThread_lock->lock();
        *addr = listReportStation.front();
        printf("GetReportStationTask addr =%s\n", (*addr).c_str());
        listReportStation.pop_front();
        m_pThread_lock->unlock();
        return true;
    }
    
    return false;
}

int CAvReportStation::SetReportStationResult(int leftbyte)
{
   m_iLeftByte = leftbyte;
   return 0;
}

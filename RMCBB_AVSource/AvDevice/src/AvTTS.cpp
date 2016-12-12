/*! \file   AvTTS.cpp
     \brief  a AvTTS function class
*/
#include <string>
using std::string;

#include "AvDebug.h"
#include "AvCommonFunc.h"
#include "AvCommonMacro.h"
#include "ConfigCommonDefine.h"
#include "AvDeviceMaster.h"

#include "AvTTS.h"
#include "SystemConfig.h"
//#include "CommonDebug.h"

#include <sys/sysinfo.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

//#include <sched.h> //!

#if !defined(_AV_PLATFORM_HI3520D_V300_)&&!defined(_AV_PLATFORM_HI3515_)
#include "hi_mem.h"
#endif

// TTS sdk related 
#include "hci_tts.h"

//Audio related
#include "mpi_ao.h"

using namespace Common;

//#define TEXT_FILE "./tts/tts.txt"
//#define RESULT_FILE "./tts/synth_result.pcm"
#define LOG_FILE_PATH "/var/mnt/usbstate/front"  //"/usr/local/bin/tts"//
#define AUTH_FILE_PATH "/tmp"
#define DATA_PATH "/usr/local/bin/tts"
//FILE * tts_result = NULL;

#ifdef _AV_PLATFORM_HI3515_
#define TTS_AUDIO_FRAME_LEN 320 //sample num *bit_width /8 = 320 * 16/8  g711a compression rate = 2
#else
#define TTS_AUDIO_FRAME_LEN 640 //sample num *bit_width /8 = 320 * 16/8
#endif

#define NEED_CHECK_AUTH 0

static int tts_on = 1;
static int tts_synth_pause = 0;

#ifdef _AV_PLATFORM_HI3515_
	int inline Construct_hisi_audio_frame(HI_U8 * audio_raw, int cnt, int payload_len, AUDIO_STREAM_S * audio_stream);
#else
	int inline Construct_hisi_audio_frame(HI_U16 * audio_raw, int cnt, int payload_len, AUDIO_FRAME_S * audio_stream);
#endif

int Create_prior_thread(thread_entry_ptr_t entry, void *pPara, pthread_t *pPid, int priority)
{
    pthread_t thread_id;
    pthread_attr_t thread_attr;

    pthread_attr_init(&thread_attr);
    pthread_attr_setscope(&thread_attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    if(pthread_create(&thread_id, &thread_attr, entry, pPara) == 0)
    {
        int policy;
        int rs;

        rs = pthread_attr_getschedpolicy( &thread_attr, &policy );
        assert(rs  == 0);
        
        /*printf("current schedule policy:%d[SCHED_FIFO:%d, SCHED_RR:%d, SCHED_OTHER:%d\n", \
       policy, SCHED_FIFO,SCHED_RR,SCHED_OTHER);*/
        
        policy = SCHED_FIFO;
        rs = pthread_attr_setschedpolicy (&thread_attr, policy);
        assert(rs  == 0);

        rs = pthread_attr_getschedpolicy( &thread_attr, &policy );
        assert(rs  == 0);

        printf("new policy:%d\n",policy);

        struct sched_param param;
        rs = pthread_attr_getschedparam (&thread_attr, &param);
        assert(rs  == 0);

        param.sched_priority = priority; 
        rs = pthread_attr_setschedparam(&thread_attr, &param);
        assert(rs  == 0);

        rs = pthread_attr_getschedparam (&thread_attr, &param);
        assert(rs  == 0);
        printf ("new priority = %d\n", param.__sched_priority);

        pthread_attr_destroy(&thread_attr);
        if(pPid != NULL)
        {
            *pPid = thread_id;
        }
        
        return 0;
    }
    
    pthread_attr_destroy(&thread_attr);
    return -1;
}

CAvTTS::CAvTTS()
{
    m_tts_info.tts_synth_running = false;
    m_tts_info.tts_send_running = false;
    
    m_tts_info.tts_inited = false;
    m_tts_info.tts_state = AUDIO_NO_PLAY;
    m_tts_info.tts_completed = false;

    m_ao_info.ao_chn = 0;
    m_ao_info.ao_dev = 0;
    m_ao_info.ao_on = 1;
    
    m_run_tts = false;   
    m_halt_tts = false;
    m_bLoopState = false;
    
    m_pBuf = NULL;
    m_tts_result.result_stat = 0;
    m_tts_result.voice_data = NULL;
    m_tts_result.voice_size = 0;
    m_tts_result.voice_num = 0;

    m_tts_param_cur = NULL;

    m_send_thread_state = 0; 
    
    _AV_FATAL_((m_pHi_ao = new CHiAo) == NULL, "OUT OF MEMORY\n");
#if defined(_AV_PLATFORM_HI3515_) && defined(_AV_HAVE_VIDEO_DECODER_)
    _AV_FATAL_((m_pHi_adec = new CHiAvDec) == NULL, "OUT OF MEMORY\n");
#endif
    _AV_FATAL_((m_pThread_lock = new CAvThreadLock) == NULL, "OUT OF MEMORY!\n");
    
}

CAvTTS::~CAvTTS()
{
    m_run_tts = false;

    _AV_SAFE_DELETE_( m_pHi_ao);
#if defined(_AV_PLATFORM_HI3515_) && defined(_AV_HAVE_VIDEO_DECODER_)
    _AV_SAFE_DELETE_( m_pHi_adec);
#endif
    _AV_SAFE_DELETE_( m_pThread_lock);
    _AV_SAFE_DELETE_(m_tts_param_cur);
    _AV_SAFE_DELETE_(m_pBuf);
    Uninit_tts();
}

bool IsCapkeyEnable(const string &capkey)
{
	//obtain all the available capbilities of authority
	CAPABILITY_LIST capbility_list;
	HCI_ERR_CODE errCode = hci_get_capability_list( NULL, &capbility_list);
	if (errCode != HCI_ERR_NONE)
	{
		DEBUG_ERROR("hci_get_capability_list failed return %d \n",errCode);
		return false;
	}

	//determine if the input capkey is among the list of available capbilities
	bool is_capkey_enable = false;

	for (size_t capbility_index = 0; capbility_index < capbility_list.uiItemCount; capbility_index++)
	{
		if (capkey == string(capbility_list.pItemList[capbility_index].pszCapKey))
		{
			is_capkey_enable = true;
			break;
		}
	}
	//release the list
	hci_free_capability_list(&capbility_list);
	return is_capkey_enable;
}

int CAvTTS::Init_tts()
{
    //! Init sys module of TTS
    HCI_ERR_CODE errCode = HCI_ERR_NONE;
    string err_info;
    //! copy auth file to tmp dir, which has writting right
    FILE * src_file = NULL;
    FILE *dst_file = NULL;

    if (NULL == (dst_file = fopen("/tmp/HCI_AUTH_FOREVER", "rb")))
    {
        if ((src_file = fopen("/usr/local/bin/tts/HCI_AUTH_FOREVER", "rb") )!= NULL)
        {
            if ((dst_file = fopen("/tmp/HCI_AUTH_FOREVER", "wb") )!= NULL)
            {
                //! copy the content to tmp
                int bytes_read, bytes_written; 
                char data[256];
                while ( (bytes_read = fread( data, 1, 256, src_file)) > 0)
                {
                    bytes_written = fwrite(data, 1, bytes_read, dst_file);
                    if (bytes_written != bytes_read)
                    {
                        DEBUG_ERROR("writing error!\n");
                        return -1;
                    }

                }
                if (chmod("/tmp/HCI_AUTH_FOREVER",  S_IRUSR|S_IRGRP|S_IROTH) !=0)
                {
                    DEBUG_ERROR("Chmod error\n");
                }
                       
            }
            else
            {
                DEBUG_ERROR(" create /tmp/HCI_AUTH_FOREVER failed!\n");
                return -1;
            }
            

        }
        else
        {
            DEBUG_ERROR(" read /usr/local/bin/tts/HCI_AUTH_FOREVER failed!\n");
            return -1;
        }

    }
    else
    {
        DEBUG_CRITICAL("/tmp/HCI_AUTH_FOREVER exits already!\n");
    }  
    if (src_file != NULL)
    {
        fclose(src_file);
    }

    if (dst_file != NULL)
    {
        fclose(dst_file);
    }
    
    //Authority info
    string strAccountInfo = "appKey=185d540b,\
                             developerKey=16e71c3dac74948536db8558d2870a24,\
                             cloudUrl=http://api.hcicloud.com:8888,";
    string capkey;                        
    char cutomername[32] = {0};
    pgAvDeviceInfo->Adi_get_customer_name(cutomername, sizeof(cutomername));
    if (strcmp(cutomername, "CUSTOMER_HUAYOU") == 0)
    {                           
        capkey = "tts.local.zhangnan";//"tts.local.zhangnan_zhangnan.n6";//!< local, female, en/cn
    }
    else
    {
        capkey = "tts.local.zhangnan_zhangnan";//"tts.local.zhangnan_zhangnan.n6";//!< local, female, en/cn
    }
 	string strConfig = strAccountInfo
		//+ string("logFileSize=500,logLevel=5,logFilePath=" LOG_FILE_PATH ",logFileCount=10,")
		+ string("authPath=" AUTH_FILE_PATH ",autoCloudAuth=no"",autoUpload=no"); //! auto check authority or not

	errCode = hci_init( strConfig.c_str() );
    
	if( errCode != HCI_ERR_NONE )
	{
        err_info = hci_get_error_info(errCode);
		DEBUG_ERROR("hci_init return %d: %s!!!\n", errCode, err_info.c_str() );
		return -1;
	}
	DEBUG_CRITICAL( "hci_init success\n" );

    //determine if the CapKey is available

    //IsCapkeyEnable(capkey);
#if 0//NEED_CHECK_AUTH
    for ( int i = 0; i< 2; i++)
    {
    
	    if (!IsCapkeyEnable(capkey))
	    {
		    //If the capkey is correctly filled, however still unavailable, hci_check_auth() operation may be tried
		    errCode = hci_check_auth();                     //!< check authority
		    DEBUG_MESSAGE("check auth return=%d\n",errCode);
		}
		else
		{
		    break;
		}
	}
    
	if (!IsCapkeyEnable(capkey))
	{
	    DEBUG_ERROR("capkey [%s] is not enable\n",capkey.c_str());

		hci_release();
		return -1;
    }
#else

#endif	
	

	//!Init TTS module
	//ultilize local capability, configured as following 
	string init_config = "dataPath=" DATA_PATH;
	init_config += ",initCapkeys=";
	init_config += capkey;
	//init_config += ",fileFlag=android_so";
	errCode = hci_tts_init(init_config.c_str());
	if (errCode != HCI_ERR_NONE)
	{
        err_info = hci_get_error_info(errCode);
		DEBUG_ERROR("hci_tts_init failed return %d: %s!!!\n", errCode, err_info.c_str() );
		hci_release();
		return -1;
	}
    m_tts_info.tts_inited = true;
    DEBUG_CRITICAL( "hci_tts_init success\n" );
    return 0;
}


int CAvTTS::Uninit_tts()
{
    //Uninit TTS
    int ret_val;
    ret_val = hci_tts_release();
  //  if (ret_val !=HCI_ERR_NONE)
    {
        DEBUG_ERROR("hci_tts_release return %d\n",ret_val);
    }
    //Uninit sys module
    
    ret_val = hci_release();
   // if (ret_val !=HCI_ERR_NONE)
    {
        DEBUG_ERROR("hci_release return %d\n",ret_val);
    }
    
    return 0;
}
tts_result_t tts_result;

//Synthesis callback function, in which the synthesis results are stored.
bool HCIAPI TtsSynthCallbackFunction(_OPT_ _IN_ void * pvUserParam,
									 _MUST_ _IN_ TTS_SYNTH_RESULT * psTtsSynthResult,
									 _MUST_ _IN_ HCI_ERR_CODE  hciErrCode)
{
	if(( hciErrCode != HCI_ERR_NONE )||(tts_on !=1))
	{
		return false;
	}
    
	//DEBUG_MESSAGE("voice data size %d\n",psTtsSynthResult->uiVoiceSize);
    
	if (psTtsSynthResult->pvVoiceData != NULL)
	{
		//tts_result_t* tts_result = (tts_result_t*)pvUserParam;
		std::list<tts_result_t> * tts_result_it = static_cast<std::list<tts_result_t> *>(pvUserParam);        
        
        if ( psTtsSynthResult->uiVoiceSize < TTS_AUDIO_FRAME_LEN)
	    {
	        //DEBUG_CRITICAL("*********  AUDIO LENGTH TOO SMALL***************\n");   
        }
        else
        {
            if(tts_on !=1)
            {
                DEBUG_CRITICAL(" TTS synth terminated!\n");
                return false;   //!< terminate tts output and synth
            }
            tts_result.result_stat = 1;
            tts_result.voice_size = psTtsSynthResult->uiVoiceSize;
            tts_result.voice_num++;

            _AV_FATAL_((tts_result.voice_data =  (unsigned char *)malloc(psTtsSynthResult->uiVoiceSize)) == NULL, "Out of memory\n");
                
            memcpy(tts_result.voice_data, psTtsSynthResult->pvVoiceData, psTtsSynthResult->uiVoiceSize);
  
            tts_result_it->push_back(tts_result);

            //mSleep(100);
           //mSleep(10); 
            if (tts_synth_pause)
            {
               //mSleep(30);
            }
        } 
        
	}


	return true;//true means continue, false break
}

bool CAvTTS::TTSSynth(const string &capkey, const char* pszText)
{
	HCI_ERR_CODE errCode = HCI_ERR_NONE;
    string err_info;   
	//
	string strSessionConfig = "capkey=";
	strSessionConfig += capkey;
	
	//Start TTS Session
	int nSessionId = -1;
	DEBUG_MESSAGE( "hci_tts_session_start config [%s]\n", strSessionConfig.c_str() );
	
	errCode = hci_tts_session_start( strSessionConfig.c_str(), &nSessionId );
	if( errCode != HCI_ERR_NONE )
	{
        err_info = hci_get_error_info(errCode);
		DEBUG_ERROR("hci_tts_session_start return %d: %s!!!\n", errCode, err_info.c_str() );
		return false;
	}
	DEBUG_MESSAGE("hci_tts_session_start success\n" );

	//Synth config
#ifdef _AV_PLATFORM_HI3515_
	string strSynthConfig = "audioFormat=alaw8k8bit";//!< default audioformat is pcm 16k16bit , 
	                                                //pcm8k16bit,pcm8k8bit, pcm11k8bit, pcm11k16bit
                                                    // pcm16k8bit, pcm16k16bit
                                                    // alaw8k8bit, ulaw8k8bit
                                                    // Verily, 8bit may not be supported
	strSynthConfig += ",pitch=5";					 //!< base frequency, 0~9.99
	//strSynthConfig += ",speedUp=3";				   //!< speedup and optimize 50%, default no speed
#else
	string strSynthConfig = "audioFormat=pcm8k16bit";//!< default audioformat is pcm 16k16bit , 
													//pcm8k16bit,pcm8k8bit, pcm11k8bit, pcm11k16bit
													// pcm16k8bit, pcm16k16bit
													// alaw8k8bit, ulaw8k8bit
													// Verily, 8bit may not be supported
	strSynthConfig += ",pitch=5";					 //!< base frequency, 0~9.99
	strSynthConfig += ",speedUp=3"; 				 //!< speedup and optimize 50%, default no speed
#endif
	strSynthConfig += ",voiceStyle=clear";          //!< clear, vivid, normal, plain, wth normal default
	strSynthConfig += ",voiceEffect=base";           //!< base, reverb, echo, chorus, nearfar, robot, default base
	strSynthConfig += ",mixSound=9";                 //!1~9 ,default 5
	strSynthConfig += ",gainFactor=2";               //!< 2~8, default 2
	strSynthConfig += ",symbolFilter=off";   
    strSynthConfig += ",namePolyphone=off"; 

	strSynthConfig += ",volume=5";                                 //!<0~9.99, default 5
	strSynthConfig += ",speed=5";                                  //!< as above

    strSynthConfig += ",puncMode=off";
    //strSynthConfig += ",engMode=english";//!< to read english number only
    
    char check_criterion[32]={0};
    pgAvDeviceInfo->Adi_get_check_criterion(check_criterion, sizeof(check_criterion));

    if (!strcmp(check_criterion, "JT_RULE"))
    {
        printf("Turn on punc read!\n");
	    strSynthConfig += ",puncMode=on";              //!< synth punc, auto determine rtn as seperation
	    //strSynthConfig += ",symbolFilter=on";  
	    strSynthConfig += ",engMode=letter";
    }
    else
    {
        if (m_tts_param_cur != NULL)
        {
            if(m_tts_param_cur->ucLanguage == 1)
            {
                DEBUG_CRITICAL("language is english!\n");
                strSynthConfig += ",engMode=english"; 
                strSynthConfig += ",digitMode=telegram"; 
            }
            else
            {
            	eProductType product_type = PTD_INVALID;
            	N9M_GetProductType(product_type);

                if ((PTD_D5 == product_type)||(PTD_A5_III == product_type))
                {
                    strSynthConfig += ",digitMode=auto_number"; 
                    
                }
                else
                {
                    strSynthConfig += ",digitMode=telegram";         //!<  telegram,number, auto_telegram, auto_number
                }
            } 
        }

    }
       
	char * text_utf8 = NULL;

	text_utf8 = (char*)pszText;
	
	//Text to be synthesed should be encoded in UTF-8 and ends with '\0'.

    //errCode = hci_tts_synth( nSessionId, text_utf8, strSynthConfig.c_str(), TtsSynthCallbackFunction, &m_tts_result);
	
	errCode = hci_tts_synth( nSessionId, text_utf8, strSynthConfig.c_str(), TtsSynthCallbackFunction, &m_tts_result_it);

    
	if( errCode != HCI_ERR_NONE )
	{
        err_info = hci_get_error_info(errCode);       
		DEBUG_ERROR("hci_tts_synth return %d: %s!!!\n", errCode, err_info.c_str());
		hci_tts_session_stop( nSessionId );
		return false;
	}
    DEBUG_CRITICAL(" TTS synth success\n");

	// stop TTS Session
	errCode = hci_tts_session_stop( nSessionId );
	if( errCode != HCI_ERR_NONE )
	{
        err_info = hci_get_error_info(errCode);
		DEBUG_ERROR("hci_tts_session_stop return %d: %s!!!\n", errCode, err_info.c_str() );
		return false;
	}
	DEBUG_MESSAGE( "hci_tts_session_stop success\n" );

    return true;

}

#ifdef _AV_PLATFORM_HI3515_
int Construct_hisi_audio_frame(HI_U8 * audio_raw, int cnt, int payload_len, AUDIO_STREAM_S * audio_stream)
{
    //HI_U64 pts;
    if ((NULL == audio_raw))
    {
        DEBUG_ERROR("Invalid arguments!\n");
        return -1;
    }
    audio_raw[0] = 0x00;   
    audio_raw[1] = 0x01;  //
    audio_raw[2] = 0xA0;  //320 bytes, ie 160 HI_S16
    audio_raw[3] = 0x00;
    
    audio_stream->pStream = audio_raw;
    audio_stream->u32Seq = 0;
    audio_stream->u32Len = payload_len;
  
    audio_stream->u64TimeStamp = 0;

    return 0;
}
#else
int Construct_hisi_audio_frame(HI_U16 * audio_raw, int cnt, int payload_len, AUDIO_FRAME_S * audio_stream)
{
    //HI_U64 pts;
    if ((NULL==audio_raw))// || (cnt < 0) || (payload_len <=0)|| (NULL == audio_stream))
    {
        DEBUG_ERROR("Invalid arguments!\n");
        return -1;
    }

    audio_stream->enBitwidth = AUDIO_BIT_WIDTH_16;
    audio_stream->enSoundmode = AUDIO_SOUND_MODE_MONO;
    audio_stream->pVirAddr[0] = audio_raw;
    audio_stream->u32Len = payload_len;
    audio_stream->u32Seq = 0;
  /*  
    if( HI_MPI_SYS_GetCurPts(&pts) != 0)
    {
        DEBUG_MESSAGE("Get pts failed!\n");
        //return -1;
    }*/
    audio_stream->u64TimeStamp = 0;

    return 0;
}

#endif
int CAvTTS::TTS_SynthThread_body()
{
#ifdef _AV_PLATFORM_HI3515_
    if(setpriority(PRIO_PROCESS, getpid(), -18) != 0) //!< setpriority(PRIO_PROCESS, 0,  -10)
    {
        printf("set priority failed!\n");
    }
#endif    
    string capkey;
    char cutomername[32] = {0};
    pgAvDeviceInfo->Adi_get_customer_name(cutomername, sizeof(cutomername));
    if (strcmp(cutomername, "CUSTOMER_HUAYOU") == 0)
    {                           
        capkey = "tts.local.zhangnan";//"tts.local.zhangnan_zhangnan.n6";//!< local, female, en/cn
    }
    else
    {
        capkey = "tts.local.zhangnan_zhangnan";//"tts.local.zhangnan_zhangnan.n6";//!< local, female, en/cn
    }

	m_tts_info.tts_synth_running = true;
    m_tts_info.tts_state = AUDIO_PLAY_PLAYING;
    
	while (m_run_tts)
	{
        char * pText = NULL;
        
	    //check if the received string ends with '\0'
        if ( m_tts_param_cur != NULL)
	    {
    	    DEBUG_MESSAGE("Obtained tts text length:%d\n",m_tts_param_cur->TextLength);
          
    	    if (m_tts_param_cur->TextContent[m_tts_param_cur->TextLength-1] != '\0')
    	    {
                _AV_FATAL_((pText = new char[m_tts_param_cur->TextLength + 1]) == NULL, "OUT OF MEMORY\n");
                memcpy(pText, m_tts_param_cur->TextContent, m_tts_param_cur->TextLength);
    	        pText[m_tts_param_cur->TextLength] = '\0';
    	    }
            else
            {
                _AV_FATAL_((pText = new char[m_tts_param_cur->TextLength]) == NULL, "OUT OF MEMORY\n");
                memcpy(pText, m_tts_param_cur->TextContent, m_tts_param_cur->TextLength);
            }          
        }
        else
        {
            DEBUG_ERROR(" m_tts_param_cur is null!\n");
            break;
        }
        
        DEBUG_CRITICAL("text to be synthesed=%s\n", pText);     
	    if ( !TTSSynth(capkey, pText))   //
	    {
	        DEBUG_ERROR( " TTS synthesis failed!\n");        
            _AV_SAFE_DELETE_ARRAY_(pText);
            
            m_run_tts = false; // prevent send thread awaiting
            m_tts_info.tts_state = AUDIO_PLAY_FINISH;
            //fclose(tts_result);
	        break;
	    }
        //gettimeofday(&time_complete, NULL);   

        //DEBUG_MESSAGE("TTS synth complete time=%d\n",(int)time_complete.tv_sec);

        //DEBUG_MESSAGE("time consumed =%d\n", (int)(time_complete.tv_sec - time_start.tv_sec));
       
        if( m_halt_tts == true)                  //!< interrupted with new task
        {
            //m_halt_tts = false;
            _AV_SAFE_DELETE_ARRAY_(pText);
            DEBUG_CRITICAL(" Halt tts !\n");
            break;
        }
        _AV_SAFE_DELETE_ARRAY_(pText);
        DEBUG_CRITICAL("TTS completed!\n");      
        break;                                  //!<  Task completed, quit the thread
        
	}
    m_tts_info.tts_completed = true;

	m_tts_info.tts_synth_running = false;
    return 0;
}

int CAvTTS::TTS_SendThread_body()
{    
    m_send_thread_state= 0;
    static unsigned int left_size = 0;
    m_tts_info.tts_send_running = true;
#ifdef _AV_PLATFORM_HI3515_
    if(setpriority(PRIO_PROCESS, getpid(), -18) != 0) //!< setpriority(PRIO_PROCESS, 0,  -10)
    {
        printf("set priority failed!\n");
    }
#endif

    static char left_data[TTS_AUDIO_FRAME_LEN];
    
    int first_send = 0; // indicate if the result is sent to ao first time
    int loop_time = 1;  //default, 

    struct sysinfo info;
    
    if (sysinfo(&info)) 
    {
        DEBUG_ERROR(" Obtain sys time failed!\n");
    }
    else
    {
        DEBUG_WARNING("uptime :%ld\n", info.uptime);
        if (info.uptime < 100)
        {
            DEBUG_MESSAGE(" Boot up shortly\n");
            mSleep(600); // for the interval of start up, increase delay to get rid of  ucontinuity
        }
        else
        {
            DEBUG_MESSAGE(" Boot up long \n");
            for (int i=0; i<25; i++)
            {
                if (tts_on != 1)
                {
                    break;
                }
                else
                {
                    mSleep(20); 
                }
            }
        }
    }

    std::list<tts_result_t> ::iterator tts_result_it;

    std::list<tts_result_t> tts_result_l;

	while (m_run_tts)
	{               
        if( m_halt_tts == true)                  //!< interrupted with new task
        {
            DEBUG_CRITICAL(" Send thread be halted!\n");
            //m_halt_tts = false;
            break;
        }

        if(tts_on !=1)
        {
            DEBUG_CRITICAL("Stop sending now!\n");
            break;
        }
        
        if ( m_tts_result_it.empty())     
        {
            m_send_thread_state= 0;
            DEBUG_WARNING("Awaiting the synthed result\n"); //turn off
            if (m_tts_info.tts_completed)
            {
                DEBUG_CRITICAL("Empty result when complete!\n");
                break;                   // complete and empty indicates the end of sending
            }
            else
            {
                mSleep(10);
                continue;                                                      // turn off
            }
        }

        DEBUG_MESSAGE("loop_time = %d\n", loop_time);

        DEBUG_CRITICAL(" Start sending to ao\n");
        //! loop the first time
        
        //tts_result_l = m_tts_result_it;
        
        while( loop_time > 0)
        {
            //DEBUG_MESSAGE("loop value:%d\n", m_bLoopState);
            if(tts_on !=1)
            {
                break;
            }
            
            if (0 == first_send)
            {
                while(!(m_tts_info.tts_completed && m_tts_result_it.empty())) // in case synth is slower than send
                {
                    if(tts_on !=1)
                    {
                        break;
                    }
                    if ( m_tts_result_it.empty())     //!< 
                    //if ( !m_tts_info.tts_completed)       //!< send when synth complete
                    {
                        mSleep(20);
                        m_send_thread_state= 0;
                        //DEBUG_MESSAGE("Awaiting the first synthed result\n"); //turn off
                        continue;                                                      // turn off
                    }

                    if (m_tts_result_it.size()> 12)
                    {
                        //DEBUG_CRITICAL("pause synth for a while\n");
                        tts_synth_pause = 1;
                    }
                    else
                    {
                        tts_synth_pause = 0;
                    }
                    m_tts_result = m_tts_result_it.front();
                    if ( (loop_time > 1) || ( true == m_bLoopState))
                    {
                        tts_result_l.push_back(m_tts_result);
                        DEBUG_MESSAGE("push result \n");
                    }
                    m_tts_result_it.pop_front();

                    m_send_thread_state = 1;

                    char * combined_data;

                    //! to avoid data loss, the remained data be ultilized in next call 
                    int n = (m_tts_result.voice_size+ left_size)/ TTS_AUDIO_FRAME_LEN;
                    _AV_FATAL_((combined_data = new char[TTS_AUDIO_FRAME_LEN * n]) == NULL, "OUT OF MEMORY\n");
                    DEBUG_MESSAGE("voice size:%d, left size:%d, n:%d\n",m_tts_result.voice_size, left_size, n);
                    if(1)// (left_size != 0)
                    {
                        memcpy(combined_data, left_data, left_size);
                        memcpy(combined_data + left_size, m_tts_result.voice_data, TTS_AUDIO_FRAME_LEN * n - left_size);  
                        left_size = m_tts_result.voice_size + left_size - TTS_AUDIO_FRAME_LEN * n;
                        memcpy(left_data, (char *)m_tts_result.voice_data + m_tts_result.voice_size - left_size, left_size);
                    }
                    else
                    {
                        memcpy(combined_data, left_data, left_size);
                        memcpy(combined_data + left_size, m_tts_result.voice_data, TTS_AUDIO_FRAME_LEN * n - left_size);

                        left_size = m_tts_result.voice_size + left_size - TTS_AUDIO_FRAME_LEN * n;
                        memcpy(left_data, (char *)m_tts_result.voice_data + m_tts_result.voice_size - left_size, left_size);
                    }

                    if ( (loop_time > 1) || ( true == m_bLoopState))
                    {

                    }
                    else
                    {
                        _AV_SAFE_DELETE_ARRAY_(m_tts_result.voice_data);  // added
                    }

#ifdef _AV_PLATFORM_HI3515_
                    HI_U8 * pCur_data;
                    _AV_FATAL_((pCur_data = new HI_U8[TTS_AUDIO_FRAME_LEN + 4]) == NULL, "OUT OF MEMORY\n"); // to construct hisi audio frame
                    
                    AUDIO_STREAM_S  audio_frame;
                    int frame_len = TTS_AUDIO_FRAME_LEN + 4;
#else
					HI_U16 * pCur_data;
					AUDIO_FRAME_S  audio_frame;
					int frame_len = TTS_AUDIO_FRAME_LEN;
#endif
                    for (int i=0; i<n; i++)
                    {
                        if(tts_on !=1)
                        {
                            break;
                        }
#ifdef _AV_PLATFORM_HI3515_
                        memcpy(pCur_data + 4, (HI_U8 *)(combined_data + i*TTS_AUDIO_FRAME_LEN), TTS_AUDIO_FRAME_LEN);
#else
						pCur_data = (HI_U16 *)(combined_data + i*TTS_AUDIO_FRAME_LEN);
#endif
                       //!send to ao
                         int ret_val = -1; 
                        
                        if (Construct_hisi_audio_frame(pCur_data, 0, frame_len, &audio_frame) !=0)
                        {   
                            DEBUG_ERROR("CAvTTS::Send_tts_ao_frame Construct_hisi_audio_frame failed!\n");
                            ret_val = -1;
                        }

#if defined(_AV_PLATFORM_HI3535_)||defined(_AV_PLATFORM_HI3520D_V300_)
                        if ((ret_val = HI_MPI_AO_SendFrame(m_ao_info.ao_dev, m_ao_info.ao_chn, &audio_frame, -1)) != 0)
#elif defined(_AV_PLATFORM_HI3515_)
                        if ((ret_val = HI_MPI_ADEC_SendStream(ADEC_CHN_PLAYBACK, &audio_frame, HI_IO_BLOCK)) != 0)
#else
                        if ((ret_val = HI_MPI_AO_SendFrame(m_ao_info.ao_dev, m_ao_info.ao_chn, &audio_frame, HI_TRUE)) != 0)
#endif
                        {
                            DEBUG_ERROR("CHiAo::HI_MPI_ADEC_SendStream  FAILED\
                            (ao_dev:%d)(ao_chn:%d)(0x%08lx)\n", m_ao_info.ao_dev, m_ao_info.ao_chn, (unsigned long)ret_val);
                        }
                        DEBUG_MESSAGE("Frame sent!\n");
                        if ( ret_val !=0)
                        {
                            DEBUG_ERROR("Send tts frame to ao failed!\n");
                            break;
                        }
                    }  

                    _AV_SAFE_DELETE_ARRAY_(combined_data);
                    
                    m_tts_result.result_stat = 0; 

                }
                if ( true == m_bLoopState)
                {
                    
                }
                else
                {
                    loop_time--;
                }
                first_send = 1;
            }
            
            //!loop for more  time
            DEBUG_MESSAGE("loop time for more: %d\n", loop_time);
            tts_result_it = tts_result_l.begin();
            while ((tts_result_it != tts_result_l.end())  &&      
                    ((1 == first_send) && (loop_time != 0)))
            {
                if(tts_on !=1)
                {
                    break;
                }
                
                m_send_thread_state = 1;
                char * combined_data = NULL;

                //! to avoid data loss, the remained data be ultilized in next call 
                int n = (tts_result_it->voice_size+ left_size)/ TTS_AUDIO_FRAME_LEN;
                _AV_FATAL_((combined_data = new char[TTS_AUDIO_FRAME_LEN * n]) == NULL, "OUT OF MEMORY\n");

                if(1)// (left_size != 0)
                {
                    memcpy(combined_data, left_data, left_size);
                    memcpy(combined_data + left_size, tts_result_it->voice_data, TTS_AUDIO_FRAME_LEN * n - left_size);  
                    left_size = tts_result_it->voice_size + left_size - TTS_AUDIO_FRAME_LEN * n;
                    memcpy(left_data, (char *)tts_result_it->voice_data + tts_result_it->voice_size - left_size, left_size);
                }
                else
                {
                    memcpy(combined_data, left_data, left_size);
                    memcpy(combined_data + left_size, tts_result_it->voice_data, TTS_AUDIO_FRAME_LEN * n - left_size);
                           
                    left_size = tts_result_it->voice_size + left_size - TTS_AUDIO_FRAME_LEN * n;
                    memcpy(left_data, (char *)tts_result_it->voice_data + tts_result_it->voice_size - left_size, left_size);
                }
                
                for (int i=0; i<n; i++)
                {
                    if(tts_on !=1)
                    {
                        break;
                    }
#ifdef _AV_PLATFORM_HI3515_
                    HI_U8 *pCur_data = (HI_U8 *)(combined_data + i*TTS_AUDIO_FRAME_LEN); 
                    AUDIO_STREAM_S  audio_frame;
#else
					HI_U16 *pCur_data = (HI_U16 *)(combined_data + i*TTS_AUDIO_FRAME_LEN); 
                    AUDIO_FRAME_S  audio_frame;
#endif
					int ret_val = -1;
                    //!send to ao
                    if (Construct_hisi_audio_frame(pCur_data, 0, TTS_AUDIO_FRAME_LEN, &audio_frame) !=0)
                    {   
                        DEBUG_ERROR("CAvTTS::Send_tts_ao_frame Construct_hisi_audio_frame failed!\n");
                        ret_val = -1;
                    }
#if defined(_AV_PLATFORM_HI3535_)||defined(_AV_PLATFORM_HI3520D_V300_)
                    if ((ret_val = HI_MPI_AO_SendFrame(m_ao_info.ao_dev, m_ao_info.ao_chn, &audio_frame, -1)) != 0)
#elif defined(_AV_PLATFORM_HI3515_)
                    if ((ret_val = HI_MPI_ADEC_SendStream( m_ao_info.ao_chn, &audio_frame, HI_TRUE)) != 0)
#else
                    if ((ret_val = HI_MPI_AO_SendFrame(m_ao_info.ao_dev, m_ao_info.ao_chn, &audio_frame, HI_TRUE)) != 0)
#endif
                    {
                        DEBUG_ERROR("CHiAo::HiAo_send_ao_frame HI_MPI_AO_SendFrame FAILED\
                                    (ao_dev:%d)(ao_chn:%d)(0x%08lx)\n", m_ao_info.ao_dev, m_ao_info.ao_chn, (unsigned long)ret_val);
                    }
                 
                    if ( ret_val !=0)
                    {
                        DEBUG_ERROR("Send tts frame to ao failed!\n");
                        break;
                    }
                } 

                _AV_SAFE_DELETE_ARRAY_(combined_data);
                
                if (tts_result_it != tts_result_l.end())
                {
                    tts_result_it++;
                }
                else
                {
                    DEBUG_MESSAGE(" reach the end\n");
                }       

            }

            //modify state
            if (m_bLoopState == true)
            {
            }
            else
            {
                loop_time--;
            }
        }          
        
        m_tts_result.result_stat = 0;  
        
        break;

	}

//! free the contens in case of halt 
    m_tts_info.tts_send_running = false;

    while( !m_tts_result_it.empty())
    {
        m_tts_result = m_tts_result_it.front();
        m_tts_result_it.pop_front();
        _AV_SAFE_DELETE_ARRAY_(m_tts_result.voice_data);  // added
    }

    tts_result_it = tts_result_l.begin();
    while ((tts_result_it != tts_result_l.end()))
    {
        DEBUG_CRITICAL(" free other list!\n");
        _AV_SAFE_DELETE_ARRAY_(tts_result_it->voice_data);
        tts_result_it++;
    }
    for (int i=0; i<25; i++)
    {
        if (tts_on != 1)
        {
            break;
        }
        else
        {
            mSleep(20); // wait for the empty of ao buffer
        }
    }
    //! 
	eProductType product_type = PTD_INVALID;
	N9M_GetProductType(product_type);
    if ((PTD_A5_II== product_type)||(product_type == PTD_6A_I))
    {
        mSleep(500);
    }
    //! end
    left_size = 0;
    memset(left_data, 0, sizeof(left_data));
    
    m_tts_result_it.clear(); 
    
    m_send_thread_state = 0;  // in case abort early
   // m_tts_info.tts_send_running = false;
    m_tts_info.tts_state = AUDIO_PLAY_FINISH;
    
    DEBUG_CRITICAL("TTS send completed!\n");

    return 0;
}

void * TTS_synth_thread_entry(void * thread_param)
{
	prctl(PR_SET_NAME, "TTS_Synth");
    CAvTTS * tts = (CAvTTS *) thread_param;
    tts->TTS_SynthThread_body();

    return NULL;
}

void * TTS_send_thread_entry(void * thread_param)
{
	prctl(PR_SET_NAME, "TTS_Send");
    CAvTTS * tts = (CAvTTS *) thread_param;
    tts->TTS_SendThread_body();

    return NULL;
}
int CAvTTS::Start_tts(msgTTSPara_t * pTTS_param)
{
    //! the rule is that latest has highest priority
    if ((NULL == pTTS_param) || (0 == pTTS_param->TextLength))
    {
        DEBUG_ERROR( "Invalid param!\n");
        m_tts_info.tts_state = AUDIO_PLAY_FINISH; 
        m_tts_info.tts_send_running = false;
        m_tts_info.tts_synth_running = false;
        m_tts_param_cur = NULL;
        return -1;
    }
        
    m_pThread_lock->lock(); 
//! for HUAYOU customer, add "soundq" before each context.
    char cutomername[32] = {0};
    pgAvDeviceInfo->Adi_get_customer_name(cutomername, sizeof(cutomername));
    if (strcmp(cutomername, "CUSTOMER_HUAYOU") == 0)
    {
        char sound_special[10] = {"soundu,"};
        m_tts_param_cur = (msgTTSPara_t *)malloc(sizeof(msgTTSPara_t) + pTTS_param->TextLength + strlen(sound_special));
        memset(m_tts_param_cur, 0, sizeof(msgTTSPara_t) + pTTS_param->TextLength + strlen(sound_special));
        memcpy(m_tts_param_cur, pTTS_param, sizeof(msgTTSPara_t) );
        strncpy(m_tts_param_cur->TextContent, sound_special, strlen(sound_special));
        
        memcpy(m_tts_param_cur->TextContent + strlen(sound_special), pTTS_param->TextContent, pTTS_param->TextLength);
        m_tts_param_cur->TextLength += strlen(sound_special);
    }
    else
    {
        m_tts_param_cur = (msgTTSPara_t *)malloc(sizeof(msgTTSPara_t) + pTTS_param->TextLength);
        memset(m_tts_param_cur, 0, sizeof(msgTTSPara_t) + pTTS_param->TextLength);
        memcpy(m_tts_param_cur, pTTS_param, sizeof(msgTTSPara_t) + pTTS_param->TextLength);
    }

    m_pThread_lock->unlock();
      
    m_tts_info.tts_completed = false;

    if ((m_tts_info.tts_synth_running == true)||(m_tts_info.tts_send_running == true))
    {
        m_halt_tts = true;                         //!< thread is running, halt it
        tts_on = 0; 
        DEBUG_ERROR("thread is still unfinished!\n");
        return 0;
    }
    
    m_tts_info.tts_state = AUDIO_NO_PLAY;

    //!repeated initing tts should be avoided,
    if (m_tts_info.tts_inited == false)
    {    
        if (this->Init_tts() != 0)
        {
            DEBUG_ERROR( "TTS init failed!\n");
            m_tts_info.tts_state = AUDIO_PLAY_FINISH;          
            return -1;
        }
    }
    m_tts_info.tts_inited = true;
    
    //! stop ao first, but sometimes unnecessary
    this->Stop_ao();

    if (this->Start_ao() !=0 )
    {
        DEBUG_ERROR( "AO set failed!\n");
        m_tts_info.tts_state = AUDIO_PLAY_FINISH; 
        return -1;
    }

#ifdef _AV_PLATFORM_HI3515_
    if ( Start_adec_ao(ADEC_CHN_PLAYBACK, APT_G711A))
    //if ( Start_adec_ao(ADEC_CHN_PLAYBACK, APT_ADPCM))
    {
        DEBUG_ERROR(" Start adec ao failed!\n");
        m_tts_info.tts_state = AUDIO_PLAY_FINISH; 
        return -1;
    }
#endif    
    m_run_tts = true;
    tts_on = 1;
    m_halt_tts = false; 

    Av_create_normal_thread(TTS_synth_thread_entry, (void *)this, NULL);
    Av_create_normal_thread(TTS_send_thread_entry, (void *)this, NULL);
    
   //Create_prior_thread(TTS_synth_thread_entry, (void *)this, NULL, 72);
   //Create_prior_thread(TTS_send_thread_entry, (void *)this, NULL, 72);
        
    if ( m_tts_info.tts_synth_running == false)
    {
        mSleep(1);
        DEBUG_MESSAGE("Waiting for tts thread to start!\n");
    }
    
    return 0;
}

int CAvTTS::Stop_tts()
{
    m_run_tts = false;
    tts_on = 0;
//! modified to disable tts in non-eng or non-cn language
    if ((NULL == m_tts_param_cur) || (0 == m_tts_param_cur->TextLength))
    {
        DEBUG_ERROR( "Invalid param!\n");
        m_tts_info.tts_state = AUDIO_NO_PLAY;
        return 0;
    }
    
    if (m_tts_info.tts_synth_running || m_tts_info.tts_send_running)
    {
        m_halt_tts = true;             //! it may be desired that the output stop right now
    }
    
    while (m_tts_info.tts_synth_running || m_tts_info.tts_send_running)// || m_send_thread_state || ((m_tts_info.tts_state != AUDIO_PLAY_FINISH) )
    {
#ifdef _AV_PLATFORM_HI3515_
        mSleep(100); //! previous 10  for A5-II
#else
		mSleep(10); //! previous 10  for A5-II
#endif
       /* DEBUG_CRITICAL("Waiting for the stop of tts thread!(synth run:%d, send run:%d\n",\
           m_tts_info.tts_synth_running, m_tts_info.tts_send_running);*/
    }
    
    _AV_SAFE_FREE_(m_tts_param_cur);
    if (! m_tts_result_it.empty())
    {
        m_tts_result_it.clear();
    }

    if ( m_tts_info.tts_inited)
    {
        //Uninit_tts();                   //may fall into stuck
    }

   // m_tts_info.tts_inited = false;
#ifdef _AV_PLATFORM_HI3515_
    this->Destroy_adec_ao(ADEC_CHN_PLAYBACK);
#endif    
    this->Stop_ao();

    m_tts_info.tts_state = AUDIO_NO_PLAY;

    DEBUG_CRITICAL("Stopping tts succeed!\n");
    
    m_halt_tts = false;  // 8.26
    return 0;
}

int CAvTTS::Start_ao()
{   
    AUDIO_DEV ao_dev = 0;
    AIO_ATTR_S ao_attr;

    ao_type_e ao_type = _AO_PLAYBACK_CVBS_;
    
    memset(&ao_attr, 0, sizeof(AIO_ATTR_S));  
    
    m_pHi_ao->HiAo_get_ao_info(ao_type, &ao_dev, NULL, &ao_attr); //!< Obtain default setting
    m_ao_info.ao_dev = ao_dev;
    //ao_attr.u32FrmNum = 50;
#ifdef _AV_PLATFORM_HI3515_
    ao_attr.u32FrmNum = 20;
#endif

    //!start ao dev, the corresponding functions in hi_ao is private, which may not be called

    int ret_val = -1;

    //AO should first be disabled when channles of dev disabled beforehand

    if ((ret_val = HI_MPI_AO_SetPubAttr(ao_dev, &ao_attr)) != 0)
    {
        DEBUG_ERROR( "CAvTTS::Set_ao HI_MPI_AO_SetPubAttr FAILED\
                      (ao_dev:%d)(0x%08lx)\n", ao_dev, (unsigned long)ret_val);
        return -1;
    }

    if ((ret_val = HI_MPI_AO_Enable(ao_dev)) != 0)
    {
        DEBUG_ERROR( "CAvTTS::Set_ao HI_MPI_AO_Enable FAILED\
                     (ao_dev:%d)(0x%08lx)\n", ao_dev, (unsigned long)ret_val);
        return -1;
    }
    int chn = 0;

    //start ao channel
    if (NULL == m_tts_param_cur)
    {
        DEBUG_ERROR("Current param is invalid!\n");
        return -1;
    }
    else
    {
        chn = m_tts_param_cur->AOChn;       //!< set the channel according to the message
        m_ao_info.ao_chn = m_tts_param_cur->AOChn;
    }
    
    if ((ret_val = HI_MPI_AO_EnableChn(ao_dev, chn)) != 0)
    {
         DEBUG_ERROR( "CAvTTS::Set_ao HI_MPI_AO_EnableChn FAILED\
                       (ao_dev:%d)(ao_chn:%d)(0x%08lx)\n", ao_dev, chn, (unsigned long)ret_val);
        return -1;
    }
    if ((ret_val =HI_MPI_AO_ClearChnBuf(ao_dev, chn)) != 0)
    {
         DEBUG_ERROR( "CAvTTS::Set_ao HI_MPI_AO_ClearChnBuf FAILED\
                       (ao_dev:%d)(ao_chn:%d)(0x%08lx)\n", ao_dev, chn, (unsigned long)ret_val);  
    }      

    return 0;

}

int CAvTTS::Stop_ao()
{
    
    if( m_pHi_ao->HiAo_stop_ao(_AO_PLAYBACK_CVBS_) != 0 )
    {
        DEBUG_ERROR("Stop failed\n");
    }

    return 0;
}

#if defined(_AV_PLATFORM_HI3515_) && defined(_AV_HAVE_VIDEO_DECODER_)
//! Added so as to send audio stream to adec, in that 3515 supports not sending to ao directly
int CAvTTS::Start_adec_ao(int adec_chn, eAudioPayloadType type)
{
    if (m_pHi_adec->Ade_destroy_adec(adec_chn) != 0)
    {
        DEBUG_ERROR("Ade_destroy_adec FAILED! (adec_chn:%d) \n", adec_chn);
    }
    if (m_pHi_adec->Ade_create_adec(adec_chn, type) !=0)
    {
        DEBUG_ERROR("Ade_create_adec failed!(adec_chn:%d) \n", adec_chn);
        return -1;
    }
    
    int ret_val = -1;
    if ((ret_val = m_pHi_ao->HiAo_adec_bind_ao(_AO_PLAYBACK_CVBS_, adec_chn)) != 0)
    {
         DEBUG_ERROR( "CHiAo::HiAo_adec_bind_ao HiAo_adec_bind_ao FAILED (ao_type:%d) (adec_chn:%d) (0x%08lx)\n", _AO_PLAYBACK_CVBS_, adec_chn, (unsigned long)ret_val);
        return -1;
    } 

 /*  
    MPP_CHN_S stSrcChn,stDestChn;
    stSrcChn.enModId = HI_ID_ADEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = adec_chn;
    stDestChn.enModId = HI_ID_AO;
    m_pHi_ao->HiAo_get_ao_info(_AO_PLAYBACK_CVBS_, &stDestChn.s32DevId, &stDestChn.s32ChnId);
    stDestChn.s32ChnId = m_ao_info.ao_chn;

    if ((ret_val = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn)) != 0)
    {
         DEBUG_ERROR( "CHiAo::HiAo_adec_bind_ao HI_MPI_SYS_Bind FAILED (ao_type:%d) (adec_chn:%d) (ao_dev:%d) (ao_chn:%d)(0x%08lx)\n", _AO_PLAYBACK_CVBS_, adec_chn, stDestChn.s32DevId, stDestChn.s32ChnId, (unsigned long)ret_val);
        return -1;
    } */

    return 0;
}

int CAvTTS::Destroy_adec_ao(int adec_chn)
{
    int ret_val = -1;
             
    if ((ret_val = m_pHi_ao->HiAo_adec_unbind_ao(_AO_PLAYBACK_CVBS_,adec_chn)) != 0)
    {
        DEBUG_ERROR( "CHiAo::HiAo_adec_bind_ao HiAo_adec_unbind_ao FAILED (ao_type:%d) (adec_chn:%d)(0x%08lx)\n", _AO_PLAYBACK_CVBS_, adec_chn, (unsigned long)ret_val);
        return -1;
    }
    if (m_pHi_adec->Ade_destroy_adec(adec_chn) != 0)
    {
        DEBUG_ERROR("Ade_destroy_adec FAILED! (adec_chn:%d) \n", adec_chn);
        return -1;
    }
    return 0;
}
#endif


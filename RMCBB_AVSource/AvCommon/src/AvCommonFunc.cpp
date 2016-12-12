#include "AvCommonFunc.h"
#include "CommonDebug.h"
#include "AvConfig.h"
#include <unistd.h>

int Av_create_normal_thread(thread_entry_ptr_t entry, void *pPara, pthread_t *pPid)
{
    pthread_t thread_id;
    pthread_attr_t thread_attr;

    pthread_attr_init(&thread_attr);
    pthread_attr_setscope(&thread_attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    if(pthread_create(&thread_id, &thread_attr, entry, pPara) == 0)
    {
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

int Get_extern_frameinfo(unsigned char * pBuffer, int DataLen, Stm_ExternFrameInfo_t & stuExternVFInfo)
{
    int pos = 0;
    RMS_INFOTYPE *pTypeInfo;
    bool FindHeader = false;
    int limitlen = 0;
    int externLen = 0;
    int externCount = 0;

    stuExternVFInfo.Mask = 0;
    if ((NULL == pBuffer) || (DataLen < 0))
    {
        DEBUG_ERROR("para is illegal, DataLen %d\n", DataLen);
        return -1;
    }
    while (pos < DataLen)
    {
        int headerpos = pos+1;
        if (headerpos >= DataLen)
        {
            break;
        }
        
        if(memcmp(pBuffer+headerpos, "2dc", 3) == 0)//I Frame
        {
            FindHeader = true;
            break;
        }
        else if(memcmp(pBuffer+headerpos, "3dc", 3) == 0)//P Frame
        {
            FindHeader = true;
            break;
        }
        else if(memcmp(pBuffer+headerpos, "4dc", 3) == 0)//Audio Frame
        {
            FindHeader = true;
            break;
        }
        
        pos++;
    }
    if (FindHeader == false)
    {
        return -1;
    }
    limitlen = pos+sizeof(RMSTREAM_HEADER);
    if (limitlen < DataLen)
    {
        stuExternVFInfo.pHeader = (RMSTREAM_HEADER *)(pBuffer+pos);
        stuExternVFInfo.Mask |= EXTERN_FRAMEINFO_VIDEO_HEADER;
        pos+= sizeof(RMSTREAM_HEADER);
        while(pos < DataLen)
        {
            limitlen = pos+sizeof(RMS_INFOTYPE);
            if (limitlen > DataLen)
            {
                break;
            }
            pTypeInfo = (RMS_INFOTYPE *)(pBuffer+pos);
            limitlen = pos+pTypeInfo->lInfoLength;
            if (limitlen > DataLen)
            {
                break;
            }
            if(pTypeInfo->lInfoLength == 0)
            {
                DEBUG_ERROR("[%s,%d]EXTERN INFO LENGTH IS 0!\n", __FUNCTION__, __LINE__);
                break;
            }


            if (pTypeInfo->lInfoType == RMS_INFOTYPE_VIRTUALTIME)
            {
                stuExternVFInfo.pVTime = (RMFI2_VTIME *)(pBuffer+pos);
                stuExternVFInfo.Mask |= EXTERN_FRAMEINFO_VIDEO_VIRTUALTIME;
                externCount++;
            }
            else if (pTypeInfo->lInfoType == RMS_INFOTYPE_VIDEOINFO)
            {
                stuExternVFInfo.pVideoInfo = (RMFI2_VIDEOINFO *)(pBuffer+pos);
                stuExternVFInfo.Mask |= EXTERN_FRAMEINFO_VIDEO_VIDEOINFO;
                externCount++;
            }
            else if (pTypeInfo->lInfoType == RMS_INFOTYPE_RTCTIME)
            {
                stuExternVFInfo.pRtcTime = (RMFI2_RTCTIME *)(pBuffer+pos);
                stuExternVFInfo.Mask |= EXTERN_FRAMEINFO_VIDEO_RTCTIME;
                externCount++;
            }
            else if (pTypeInfo->lInfoType == RMS_INFOTYPE_AUDIOINFO)
            {
                stuExternVFInfo.pAudioInfo = (RMFI2_AUDIOINFO*)(pBuffer+pos);
                stuExternVFInfo.Mask |= EXTERN_FRAMEINFO_AUDIO_INFO;
                externCount++;
            }
            else if (pTypeInfo->lInfoType == RMS_INFOTYPE_VIDEOSTATE)
            {
                stuExternVFInfo.pVideoState = (RMFI2_VIDEOSTATE*)(pBuffer+pos);
                stuExternVFInfo.Mask |= EXTERN_FRAMEINFO_VIDEO_STATE;
                externCount++;
            }
            else if (pTypeInfo->lInfoType == RMS_INFOTYPE_VIDEOREF)
            {
                stuExternVFInfo.pVideoRefInfo = (RMFI2_VIDEOREFINFO*)(pBuffer+pos);
                stuExternVFInfo.Mask |= EXTERN_FRAMEINFO_VIDEO_REF;
                externCount++;
            }
            pos += pTypeInfo->lInfoLength;
            externLen += pTypeInfo->lInfoLength;
            if (((unsigned int)externLen >= stuExternVFInfo.pHeader->lExtendLen) || ((unsigned int)externCount >= stuExternVFInfo.pHeader->lExtendCount))
            {
                break;
            }
        }
        return 0;
    }
    return -1;
}

unsigned long long Get_system_ustime(void)
{
    struct timeval  tv;
    unsigned long long  now;
    
    if (gettimeofday(&tv, NULL))
        return 0;

    now  = tv.tv_sec*1000000ULL;
    now += tv.tv_usec;
   // DEBUG_MESSAGE("Time %ld.%ld, now %lld\n", tv.tv_sec, tv.tv_usec, now);
    return now;
}

void mSleep(unsigned int  MilliSecond)
{
#if defined(_AV_PLATFORM_HI3515_)
	struct timeval time;
	
	time.tv_sec = MilliSecond / 1000;//!<seconds
	time.tv_usec = MilliSecond * 1000 % 1000000;//microsecond
	
	select(0, NULL, NULL, NULL, &time);
#else
	usleep(MilliSecond * 1000);
#endif
}

int Get_Resvalue_By_Size(int width, int height)
{/*0-cif;1-hd1;2-d1;3-Qcif;4-QVGA;5-VGA;6-720P;7-1080P, 10-wqcif, 11-wcif, 12-whd1, 13-wd1, 15-540p*/
    int res = -1;

    if (width==1920 && height==1080)
    {
        res = 7;
    }
    else if (width==1280 && height==720)
    {
        res = 6;
    }
    else if (width==640 && (height==360 || height==368))
    {
        res = 5;
    }
    else if ((width==704 || width==720) && (height==480 || height==576))
    {
        res = 2;
    }
    else if ((width==704 || width==720) && (height==240 || height==288))
    {
        res = 1;
    }
    else if ((width==352 || width==360) && (height==240 || height==288))
    {
        res = 0;
    }
    else if ((width==176 || width==180) && (height==120 || height==144))
    {
        res = 3;
    }
    else if ((width==928 || width==960 || width==944) && (height==576 || height==480))
    {
        res = 13;
    }
    else if ((width==928 || width==960 || width==944) && (height==288 || height==240))
    {
        res = 12;
    }
    else if ((width==464 || width==480 || width==472) && (height==288 || height==240))
    {
        res = 11;
    }
    else if ((width==232 || width==240) && (height==144 || height==120))
    {
        res = 10;
    }
    else if( width == 1280 && height == 960 )
    {
        res = 14;
    }
    else if( width == 320 && height <= 240 )
    {
        res = 4;
    }
    else if ((width==960) && (height==540))
    {
        res = 15;
    }
    else
    {
        DEBUG_ERROR("invalid resolution w=%d height=%d\n", width, height);
    }
    return res;
}


#ifndef __SHARE_PTS_H__
#define __SHARE_PTS_H__

#include "AvCommonFunc.h"
#include "mpi_sys.h"
#include <systemtime/SystemTime.h>
namespace AV_PTS
{
using namespace Common;

inline bool NeedInitPts()
{
    if( N9M_SHGetSystemSharePts() == 0 )
    {
        return true;
    }

    return false;
}

static bool g_bsystemptsinited = false;
inline int InitSystemPts( bool bCheck )
{
    if( !g_bsystemptsinited )
    {
        g_bsystemptsinited = true;

        if( bCheck )
        {
            HI_U64 u64Pts = N9M_SHGetSystemSharePts();
            if( u64Pts != 0ULL )
            {
                int ret_val = HI_MPI_SYS_InitPtsBase(u64Pts*1000ULL);
                DEBUG_MESSAGE("InitSystemPts GetShare Pts HI_MPI_SYS_InitPtsBase pts=%lld result=0x%x\n", u64Pts, ret_val);
                if(ret_val != HI_SUCCESS)
                {
                    DEBUG_ERROR("InitSystemPts HI_MPI_SYS_InitPtsBase FAILED(0x%08x)\n", ret_val);
                    return -1;
                }
                return 0;
            }
        }
        int time_count = 0;
        datetime_t stRtcTime;
        memset( &stRtcTime, 0, sizeof(stRtcTime) );
        while( stRtcTime.year==0 && stRtcTime.month==0 )
        {
            N9M_TMGetShareTime( &stRtcTime );

            time_count++;
            if (time_count > 3600)
            {
                DEBUG_ERROR("InitSystemPts()::GET SHARE SECOND FAILED\n");
                break;
            }
            usleep(50 * 1000);
        }

        HI_U64 pts = 1000ull * 1000ull * (time(NULL) - 3600 * 24 * 365 * 30);

        int ret_val = HI_MPI_SYS_InitPtsBase(pts);
        if(ret_val != HI_SUCCESS)
        {
            DEBUG_ERROR("InitSystemPts HI_MPI_SYS_InitPtsBase FAILED(0x%08x)\n", ret_val);
            return -1;
        }
    }

    return 0;
}

inline int SetPtsToShareMemory()
{
    HI_U64 u64CurPts = 0;
    if( HI_MPI_SYS_GetCurPts(&u64CurPts) == 0 )
    {
  //      printf("@@@N9M_SHSetSystemSharePts pts=%llu\n", u64CurPts);
        N9M_SHSetSystemSharePts(u64CurPts / 1000);
    }
    else
    {
        DEBUG_ERROR("avStreaming: get share pts failed\n");
    }
    
    return 0;
}

#if 0
void* SharePtsThreadBody(void *pPara)
{
    while( true )
    {
        SetPtsToShareMemory();
        DEBUG_MESSAGE("get data, type=x pts=%lld timesec=%d\n", u64CurPts/1000, CSystemTime::GetRtcSecond());
        sleep(5);
    }

    return NULL;
}

void StartSharePtsThread()
{
    int iResult = Av_create_normal_thread(SharePtsThreadBody, NULL, NULL);
    if( iResult != 0 )
    {
        DEBUG_ERROR("avStreaming @@@@@@@@@@@@StartSharePtsThread failed\n");
    }
}
#endif
};

#endif


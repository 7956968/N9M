/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __IVE_SNAP_H__
#define __IVE_SNAP_H__

#include "hi_common.h"
#include "hi_comm_video.h"
#include "HiAvEncoder.h"
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <deque>

using namespace std;
using namespace Common;

typedef struct _ive_snap_info_
{
    unsigned char u8Snap_type;//bit2 is:fd bit1:ld bit0:ls
    unsigned char u8Picture_nums;
    unsigned char u8Osd_bitmap;
    unsigned char reserve[5];
    RECT_S* pPicture_size;
    VIDEO_FRAME_INFO_S* pPicture_info;
}ive_snap_info_s;


typedef VIDEO_FRAME_INFO_S ive_snap_video_frame_info_s;

typedef struct _ive_snap_task_
{
    unsigned char u8Snap_type;//bit2 is:fd bit1:ld bit0:ls
    unsigned char u8Snap_subtype; //! for smart capture, 
    unsigned char u8Osd_num;
    unsigned char u8Reserved[2];
    ive_snap_osd_display_attr_s* pOsd;
    ive_snap_video_frame_info_s* pFrame;
}ive_snap_task_s;

class CIveSnap
{
    public:
        CIveSnap(CHiAvEncoder* pEnc);
        virtual ~CIveSnap();   
    public:
        int StopIveSnapThread();
        int IveSnapThreadRun();

    	int IveSnapThreadRunEx();
    	int StartIveSnapThreadEx(boost::function< int (ive_snap_task_s *)> recycle_func);
    	int AddIveSnapTask(ive_snap_task_s& snap_info);


    private:
        int DestroyIveSnapEncoder();
    
    public:
            int CreateIveSnapEncoder(unsigned int encoder_width, unsigned int encoder_height, ive_snap_osd_display_attr_s* pOsd, unsigned char osd_num);
            int GetIveSnapTask(ive_snap_task_s & snap_info);

    private:
        CHiAvEncoder* m_pEnc;
        bool m_bIve_encoder_created;
        bool m_bIs_ive_encoder_thread_start;
        boost::function<int (ive_snap_task_s*)> m_recycle_function_ble;
        std::deque<ive_snap_task_s> m_ive_snap_queue_ble;

        HANDLE m_hSnapShareMemHandle;

    };

#endif


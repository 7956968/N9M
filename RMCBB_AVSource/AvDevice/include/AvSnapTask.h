#ifndef __AVSNAPTASK_H__
#define __AVSNAPTASK_H__

#include "AvConfig.h"

#if defined(_AV_PLATFORM_HI3515_)
#include "HiVi-3515.h"
#include "HiAvEncoder-3515.h"
#else
#include "HiVi.h"
#include "HiAvEncoder.h"
#endif
#include "StreamBuffer.h"
#include "Defines.h"
#include "Message.h"
#include <list.h>
#include "Snap.h"
#include "SystemTime.h"
#include "rm_types.h"
#include "AvAppMaster.h"
#define _AV_PRODUCT_N9M_ 
using namespace Common;

enum SNAP_THREAD_RUN_STATE{ SNAP_THREAD_NONE, SNAP_THREAD_CHANGED_ENCODER };
typedef struct _snap_osd_item_{
	av_osd_name_t osdname;
    bool visited;
    /*coordinate*/
    int display_x;
    int display_y;
    /*bitmap*/
    int bitmap_width;
    int bitmap_height;
    
}snap_osd_item_t;

class CAvSnapTask
{
public:
#if defined(_AV_PLATFORM_HI3515_)	
    CAvSnapTask( CHiVi* pVi, CHiAvEncoder* pEnc);
#else
    CAvSnapTask( CHiVi* pVi, CHiAvEncoder* pEnc, CHiVpss* pVpss );
#endif
    ~CAvSnapTask();

public:
    int SignalSnapThreadBody();
    int StartSnapThread();
    int StopSnapThread();
    int UpdateSnapParam( int phy_video_chn_num, const av_video_encoder_para_t *pVideo_para );

    int AddSnapCmd(/* const MsgSnapPicture_t* pstuSnapCmd */);
	
	int AddSignalSnapCmd( AvSnapPara_t pstuSignalSnapCmd );
	int SetDigitAnalSwitchMap(char chnmap[_AV_VIDEO_MAX_CHN_]);
	int SetSnapSerialNumber(msgMachineStatistics_t *pstuMachineStatistics);
	unsigned short GetSnapSerialNumber();
	int GetShareMenLockState(int chn);
	int SetShareMenLockState(int chn, char ana_or_dig);
	int SetShareMenLock(int chn, char lockFlag);
	bool GetSignalSnapTask(AvSnapPara_t *snapparam);
	int GetSnapResult(msgSnapResultBoardcast *snapresultitem);
    inline int GetChnState()const {return m_analog_digital_flag; }
    int SetChnState(int flag);
	int SetOsdInfo(av_video_encoder_para_t  *pVideo_para, int chn, rm_uint16_t usOverlayBitmap);
	bool IsParaChanged(unsigned char &oldResolution, unsigned char &newResolution, unsigned char &oldQuality, unsigned char &newQuality, bool bShow, char *oldstate, char *newstate, rm_uint16_t &oldBitmap, rm_uint16_t  &newBitmap);
#ifdef _AV_PRODUCT_CLASS_IPC_
    int ClearAllSnapPictures();
 private:
    bool IsSnapParaChanged(const AvSnapPara_t *pnew_snap_para);
    int SyncSnapParaToSnapOsdInfo();
#if 0
    int CreateSnapEncoder(int phy_video_chn_num, av_resolution_t resolution, rm_uint8_t quality, hi_snap_osd_t *osd_info);
#endif
#endif
	av_resolution_t ConvertResolutionFromIntToEnum(rm_uint8_t resulotion);

private:

    int GetCurrentTime( char* pszTimeStr );
    int GetSnapEncodeOsd( unsigned char u8ChipChn, hi_snap_osd_t* pstuOsd );
    //定义单次抓拍OSD
    int GetSnapEncodeOsd(unsigned char u8ChipChn, hi_snap_osd_t* pstuOsd, int osdNum, av_resolution_t eResolution);
#if defined(_AV_PLATFORM_HI3515_)
	int Get6AISnapEncoderOsd( unsigned char u8ChipChn, hi_snap_osd_t* pstuOsd, int osdNum, av_resolution_t eResolution);
#endif
    int SnapForEncoder( unsigned int phy_video_chn_num );
    int SnapForDecoder( unsigned int phy_video_chn_num );

#if defined(_AV_PRODUCT_CLASS_IPC_)
    int CreateSnapEncoder(int chn, av_resolution_t  ucResolution, rm_uint8_t  ucQuality, hi_snap_osd_t pSnapOsd[], int osd_num);
#else
    int CreateSnapEncoder(int chn, rm_uint8_t  ucResolution, rm_uint8_t  ucQuality, hi_snap_osd_t pSnapOsd[], int osd_num);
#endif
    int GetRGNAttr(av_resolution_t eResolution, av_tvsystem_t eTvSystem, int index, hi_snap_osd_t &pSnapOsd, int &width, int &height, int &video_width, int &video_height);
    int CoordinateTrans(av_resolution_t eResolution, av_tvsystem_t eTvSystem, hi_snap_osd_t pSnapOsd[], int osd_num);
    bool RgnIsPassed(snap_osd_item_t &regin, const std::vector<snap_osd_item_t> &passed_path) const;
    bool SnapIsReginOverlapWithExistRegins(const snap_osd_item_t &regin, snap_osd_item_t *overlap_regin) const;
    int GetOverlayCoordinate(av_osd_name_t osdname, int &x, int &y, unsigned int u32Width, unsigned int u32Height,int video_width, int video_height);
    int SearchOverlayCoordinate(snap_osd_item_t &osd_item, std::vector<snap_osd_item_t> &passed_path, std::queue<snap_osd_item_t> &possible_path,int video_width, int video_height);
    bool IsAllNotVisited();
    int VisitedNum();
    int PrintfList(const char * flag, std::list<snap_osd_item_t> listsnap);
    std::list<snap_osd_item_t>::iterator SearchIteratorByOsdName(av_osd_name_t osdName);
    //int CreateSnapEncoder();
#if defined(_AV_PRODUCT_CLASS_IPC_)
        int DestroySnapEncoder();
#else
        int MainStreamToSnapResolution(int mainResolution);
        int SnapResolutionToMainStream(int snapResolution);
        int DestroySnapEncoder(hi_snap_osd_t *stuOsdInfo);
        int AddBoardcastSnapResult(char flag, int serialNumber, int curSnapNumber, AvSnapPara_t stuSnapParam);
#endif

    int CheckSnapEncoderParamChanged();
    unsigned int CalcMaxLenChlName();

private:
    bool m_bNeedExitThread;
    bool m_bIsTaskRunning;
    bool m_bParaChange;
    bool m_bCreatedSnap;
    unsigned char m_u8MaxChlOnThisChip;
    CHiVi* m_pVi;
    CHiAvEncoder* m_pEnc;
#if defined(_AV_HISILICON_VPSS_)
    CHiVpss* m_pVpss;
#endif
    CAvThreadLock *m_pThread_lock;
    CAvFontLibrary *m_pAv_font_library;
    std::list<AvSnapPara_t> listSignalSnapParam;
    std::list<msgSnapResultBoardcast> listSnapResult;
    std::list<snap_osd_item_t> m_listOsdItem;
    //CAvAppMaster *m_AppMaster;
    HANDLE m_SnapShareMemHandle[_AV_VIDEO_MAX_CHN_];
    //define chn lock states
    char m_ShareMemLockState[_AV_VIDEO_MAX_CHN_];//0 : analog ,1 : digital
    unsigned char m_u8DateMode;
    unsigned char m_u8TimeMode;

	hi_snap_osd_t* m_pstuOsdInfo[8];/*0:时间1:车牌2:车速 3:GPS信息 4 通道名称 5 车辆自编号 6 自定义 7自定义;6AII_AV12 有8个区域*/
#if defined(_AV_PRODUCT_CLASS_IPC_)
    AvSnapPara_t* m_pSignalSnapPara;
    bool m_bCommSnapParamChange[_AV_VIDEO_MAX_CHN_];
#endif
	
    av_resolution_t m_eMainStreamRes[_AV_VIDEO_MAX_CHN_]; // current snap resolution, the resolution is main stream min resolution    
    av_tvsystem_t m_eTvSystem;
	unsigned short m_uHighSerialNum;
    av_resolution_t m_eSnapRes;
    av_tvsystem_t m_eSnapTvSystem;
    int m_analog_digital_flag;
    SNAP_THREAD_RUN_STATE m_eTaskState; //0-no need, 1:need create snap encoder, 2:changed tv system or encoder resotion

    int m_ResWeight[_AR_UNKNOWN_+1];
    
    unsigned int m_u32MaxLenChlName;
};

#endif


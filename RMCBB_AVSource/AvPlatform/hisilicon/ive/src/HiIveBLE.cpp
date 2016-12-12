#include "HiIveBLE.h"
#include "bus_lane_enforcement.h"

#include <string.h>
#include <fstream>
#include <json/json.h>

#include "mpi_vb.h"
#include "mpi_vgs.h"


using namespace Common;

#if 0
#define  _IVE_TEST_
#endif

#if defined(_IVE_TEST_)
#define CHONGQING_BLACKLIST "./blacklist.txt"
#else
#define CHONGQING_BLACKLIST "/var/tmp/blacklist.txt" 
#endif


#define IVE_CONFIG_PATH "./ble_config/"

//! for singapore bus snap
#define SINGAPORE_BUS_TIME "/usr/local/cfg/SINGAPORE_BUS_CFG"

#define IVE_BLE_FRAME_WIDTH (1280)
#define IVE_BLE_FRAME_HEIGHT (720)
#define IVE_BLE_OSD_HEIGHT (80)
#define IVE_BLE_OSD_WIDTH (IVE_BLE_FRAME_WIDTH)

#define IVE_BLE_SCALE_FRAME_WIDTH (1280)
#define IVE_BLE_SCALE_FRAME_HEIGHT (720)
#define IVE_BLE_SNAP_PICTURE_WIDTH (IVE_BLE_FRAME_WIDTH*2)
#define IVE_BLE_SNAP_PICTURE_HEIGHT (IVE_BLE_SCALE_FRAME_HEIGHT*2)

#define IVE_BLE_SNAP_OSD_NUM 8

#define IVE_BLE_TIME_INT (30*1000)

CHiIveBle::CHiIveBle(data_sorce_e data_source_type, data_type_e data_type, void* data_source, CHiAvEncoder* mpEncoder):
    CHiIve(data_source_type, data_type, data_source, mpEncoder)
{
        //! necessary for singapore snap
    m_ive_snap_thread = new CIveSnap(mpEncoder);
    
    m_u32Blk_handle = VB_INVALID_HANDLE;
    m_u32Pool = VB_INVALID_POOLID;
    m_u32Frame_phy_addr  = 0;
    m_u32Frame_vir_addr = NULL;
    m_vFrames.clear();
    m_vSnap_osd.clear();
        
    m_single_snapped = 0;
    memset(m_snap_osd, 0, sizeof(m_snap_osd));
    memset(&m_yellow_lane_time, 0, sizeof(m_yellow_lane_time));
    memset(&m_pink_lane_time, 0, sizeof(m_pink_lane_time));
    memset(m_pOsd, 0, sizeof(m_pOsd));
    
    char customer[32];
    memset(customer, 0, sizeof(customer));
    pgAvDeviceInfo->Adi_get_customer_name(customer, sizeof(customer));
        
    if(0 == strcmp(customer, "CUSTOMER_SG"))
    {
        std::ifstream ifs;
        ifs.open(SINGAPORE_BUS_TIME, ios::binary);
        
        Json::Value bus_time;
        Json::Reader reader;
        if(reader.parse(ifs, bus_time))
        {
            if(!bus_time["YELLOW"].empty())
            {
                m_yellow_lane_time.start_week = bus_time["YELLOW"]["SW1"].asInt();
                m_yellow_lane_time.end_week   = bus_time["YELLOW"]["EW1"].asInt();
                m_yellow_lane_time.start_hour1 = bus_time["YELLOW"]["SH1"].asInt();
                m_yellow_lane_time.end_hour1 = bus_time["YELLOW"]["EH1"].asInt();
                m_yellow_lane_time.start_hour2 = bus_time["YELLOW"]["SH2"].asInt();
                m_yellow_lane_time.end_hour2 = bus_time["YELLOW"]["EH2"].asInt();
                m_yellow_lane_time.start_min1 = bus_time["YELLOW"]["SM1"].asInt();
                m_yellow_lane_time.end_min1 = bus_time["YELLOW"]["EM1"].asInt();
                m_yellow_lane_time.start_min2 = bus_time["YELLOW"]["SM2"].asInt();
                m_yellow_lane_time.end_min2 = bus_time["YELLOW"]["EM2"].asInt();
            }
            if(!bus_time["PINK"].empty())
            {
                m_pink_lane_time.start_week = bus_time["PINK"]["SW1"].asInt();
                m_pink_lane_time.end_week   = bus_time["PINK"]["EW1"].asInt();
                m_pink_lane_time.start_hour1 = bus_time["PINK"]["SH1"].asInt();
                m_pink_lane_time.end_hour1 = bus_time["PINK"]["EH1"].asInt();
    
                m_pink_lane_time.start_min1 = bus_time["PINK"]["SM1"].asInt();
                m_pink_lane_time.end_min1 = bus_time["PINK"]["EM1"].asInt();      
            }
        }
        
        DEBUG_CRITICAL("yellow sw:%d,ew:%d,sh1:%d,eh1:%d,sh2:%d,eh2:%d,sm1:%d,ew1:%d,sm2:%d,ew2:%d\n",
        m_yellow_lane_time.start_week,
        m_yellow_lane_time.end_week,
        m_yellow_lane_time.start_hour1,
        m_yellow_lane_time.end_hour1,
        m_yellow_lane_time.start_hour2,
        m_yellow_lane_time.end_hour2,
        m_yellow_lane_time.start_min1,
        m_yellow_lane_time.end_min1,
        m_yellow_lane_time.start_min2,
        m_yellow_lane_time.end_min2);
        DEBUG_CRITICAL("pink sw:%d,ew:%d,sh1:%d,eh1:%d,sm1:%d,ew1:%d\n",
        m_pink_lane_time.start_week,
        m_pink_lane_time.end_week,
        m_pink_lane_time.start_hour1,
        m_pink_lane_time.end_hour1,
        m_pink_lane_time.start_min1,
        m_pink_lane_time.end_min1);
    }

    m_u8Matching_degree = 80;
    m_bCurrent_detection_state = 0;
    m_blacklist.clear();
}

CHiIveBle::~CHiIveBle()
{
    _AV_SAFE_DELETE_(m_ive_snap_thread);
    m_detection_result_notify_func = NULL;
    m_blacklist.clear();
}

int CHiIveBle::ive_control(unsigned int ive_type, ive_ctl_type_e ctl_type, void* args)
{
    if(ive_type  & IVE_TYPE_BLE)
    {
        switch(ctl_type)
        {
            case IVE_CTL_OVERLAY_OSD:
            {
                snprintf(m_pOsd, sizeof(m_pOsd),"%s", (char *)args);
                m_pOsd[sizeof(m_pOsd)-1] = '\0';
            }
                break;
            case IVE_CTL_SET_MATCH_DEGREE:
            {
                m_u8Matching_degree = (int)args;
            }
                break;
            case IVE_CTL_UPDATE_BLACKLIST:
            {
                char customer[32];
                memset(customer, 0, sizeof(customer));
                pgAvDeviceInfo->Adi_get_customer_name(customer, sizeof(customer));
                if(0 == strcmp(customer, "CUSTOMER_CQJY"))
                {
                    update_blacklist_for_cqjy();
                }
            }
                break;
            default:
                DEBUG_ERROR("the command:%d is not supported! \n", ctl_type);
                return -1;
        }
    }
    else
    {
        DEBUG_ERROR("unkown ive type:%d or contrl type:%d !", ive_type, ctl_type);
    }

    return 0;
}

int CHiIveBle::init()
{

    //! Added for singapore bus snap
    HI_U32 pic_size = 0;
    pic_size = (IVE_BLE_SNAP_PICTURE_WIDTH*IVE_BLE_SNAP_PICTURE_HEIGHT*3)>>1;

    m_u32Pool   = HI_MPI_VB_CreatePool( pic_size, 1, NULL);
    if (m_u32Pool == VB_INVALID_POOLID)
    {
        DEBUG_ERROR("HI_MPI_VB_CreatePool failed! \n");
        return -1;
    }

    m_u32Blk_handle = HI_MPI_VB_GetBlock(m_u32Pool, pic_size, NULL);
    if(VB_INVALID_HANDLE == m_u32Blk_handle)
    {
        DEBUG_ERROR("HI_MPI_VB_GetBlock failed! \n");
        HI_MPI_VB_DestroyPool(m_u32Pool);
        return -1;
    }

    m_u32Frame_phy_addr = HI_MPI_VB_Handle2PhysAddr(m_u32Blk_handle);
    m_u32Frame_vir_addr = (HI_U8*) HI_MPI_SYS_Mmap(m_u32Frame_phy_addr, pic_size );
    if (NULL== m_u32Frame_vir_addr)
    {
        DEBUG_ERROR("HI_MPI_SYS_Mmap failed! \n");
        HI_MPI_VB_ReleaseBlock(m_u32Blk_handle);
        HI_MPI_VB_DestroyPool(m_u32Pool);
        return -1;
    }

    char customer[32];
    memset(customer, 0, sizeof(customer));
    pgAvDeviceInfo->Adi_get_customer_name(customer, sizeof(customer));
    

    if(0 == strcmp(customer, "CUSTOMER_CQJY"))
    {
        m_ive_snap_thread->StartIveSnapThreadEx(boost::bind(&CHiIveBle::ive_recycle_resource, this, _1));
    }
    else
    {
        m_ive_snap_thread->StartIveSnapThreadEx(boost::bind(&CHiIveBle::ive_recycle_resource2, this, _1));
    }

    if(HI_SUCCESS !=  BusLaneEnforcementInit((HI_CHAR *)IVE_CONFIG_PATH))
    {
        DEBUG_ERROR("bus lane enforcemement init  failed!  \n");
        return -1;
    }

    //add for test
#if defined(_IVE_TEST_)
    update_blacklist_for_cqjy();
#endif

    return 0;
}

int CHiIveBle::uninit()
{
    if(HI_SUCCESS != BusLaneEnforcementDeInit())
    {
        DEBUG_ERROR("BLE IveAlgDeInit  failed!  \n");
        return -1;
    } 
    m_ive_snap_thread->StopIveSnapThread();
    HI_MPI_SYS_Munmap(m_u32Frame_vir_addr, (IVE_BLE_SNAP_PICTURE_WIDTH*IVE_BLE_SNAP_PICTURE_HEIGHT*3)/2);
    HI_MPI_VB_ReleaseBlock(m_u32Blk_handle);
    HI_MPI_VB_DestroyPool(m_u32Pool);

    return 0;
}

int CHiIveBle::process_body()
{
    DEBUG_MESSAGE("head count thread run!\n");
    m_thread_state = IVE_THREAD_START;
    while(m_thread_state == IVE_THREAD_START)
    {
        m_Ive_process_func(0);
        usleep(10*1000);
    }

    return 0;
}

int CHiIveBle::process_raw_data_from_vi(int phy_chn)
{
    return 0;
}
int CHiIveBle::process_yuv_data_from_vi(int phy_chn)
{
    return 0;
}
int CHiIveBle::process_yuv_data_from_vpss(int phy_chn)
{
    char customer[32];
    memset(customer, 0, sizeof(customer));
    pgAvDeviceInfo->Adi_get_customer_name(customer, sizeof(customer));

    if(0 == strcmp(customer, "CUSTOMER_SZGJ"))
    {
        return ive_ble_process_frame_for_SZGJ(phy_chn);
    }
    else if(0 == strcmp(customer, "CUSTOMER_SG"))
    {
        return ive_ble_process_frame_SG(phy_chn);
    }
    else if(0 == strcmp(customer, "CUSTOMER_CQJY"))
    {
        return ive_ble_process_frame(phy_chn);
    }
    else
    {
        return ive_ble_process_frame(phy_chn);
    }
}


int CHiIveBle::ive_ble_process_frame_for_SZGJ(int phy_chn)
{
#if 1
    static RECT_S roi = {0};
    int ret = 0;
    bool release_falg = true;
    CHiVpss* hivpss = static_cast<CHiVpss *>(m_pData_source);
    VIDEO_FRAME_INFO_S frame_1080p;
    VIDEO_FRAME_INFO_S  frame_540p;
    IVE_IMAGE_S image_1080p;
    IVE_IMAGE_S image_540p;

    memset(&frame_1080p, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&frame_540p, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&image_1080p, 0, sizeof(IVE_IMAGE_S));
    memset(&image_540p, 0, sizeof(IVE_IMAGE_S));

    if(0 != hivpss->HiVpss_get_frame( _VP_MAIN_STREAM_,phy_chn, &frame_1080p))
    {
        DEBUG_ERROR("vpss get frame failed ! \n");
        return -1;
    }
        
    if(0 != hivpss->HiVpss_get_frame( _VP_SPEC_USE_,phy_chn, &frame_540p))
    {
        DEBUG_ERROR("vpss get frame failed ! \n");
        if(0 != hivpss->HiVpss_release_frame(_VP_MAIN_STREAM_, phy_chn, &frame_1080p))
        {
            DEBUG_ERROR("vpss release frame failed ! \n");
        }
        return -1;
    }

    image_1080p.enType = (frame_1080p.stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_422)?IVE_IMAGE_TYPE_YUV422SP:IVE_IMAGE_TYPE_YUV420SP;
    for(int i=0; i<3; ++i)
    {
        image_1080p.u32PhyAddr[i] = frame_1080p.stVFrame.u32PhyAddr[i];
        image_1080p.u16Stride[i] = (HI_U16)frame_1080p.stVFrame.u32Stride[i];
        image_1080p.pu8VirAddr[i]= (HI_U8*)frame_1080p.stVFrame.pVirAddr[i];
    }

    image_1080p.u16Width = (HI_U16)frame_1080p.stVFrame.u32Width;
    image_1080p.u16Height = (HI_U16)frame_1080p.stVFrame.u32Height;

    image_540p.enType = (frame_540p.stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_422)?IVE_IMAGE_TYPE_YUV422SP:IVE_IMAGE_TYPE_YUV420SP;

    for(int i=0; i<3; ++i)
    {
        image_540p.u32PhyAddr[i] = frame_540p.stVFrame.u32PhyAddr[i];
        image_540p.u16Stride[i] = (HI_U16)frame_540p.stVFrame.u32Stride[i];
        image_540p.pu8VirAddr[i]= (HI_U8*)frame_540p.stVFrame.pVirAddr[i];
    }
    image_540p.u16Width = (HI_U16)frame_540p.stVFrame.u32Width;
    image_540p.u16Height = (HI_U16)frame_540p.stVFrame.u32Height;    

    do
    {
        //PlateInfo plate_infos;
        LaneEnforcementInfo plate_infos;
        memset(&plate_infos, 0, sizeof(LaneEnforcementInfo));
#if 1
        if(HI_SUCCESS != BusLaneEnforcementStart(&image_1080p, &image_540p, &plate_infos))
        {
            DEBUG_ERROR("IveAlgStart  failed! \n");
            BusLaneEnforcementParametersReset((char *)"./");
            ret = -1;
            break;
        }
#endif
        //if(1 == pgAvDeviceInfo->Adi_get_ive_debug_mode())
        {
            if(m_pEncoder != NULL)    
            {        
                m_pEncoder->Ae_send_frame_to_encoder(_AST_SUB_VIDEO_, 0, (void *)&frame_540p, -1); 
            }
        }
        //! osd display

        switch(plate_infos.plate_recognition_flag)
        {
            case EN_PLATE_UPDATE:
                if(NULL != m_detection_result_notify_func)
                {
                    msgIPCIntelligentInfoNotice_t notice;
                    memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                    notice.cmdtype = 1;
                    notice.happen = 0;
                    notice.reserved[0] = 0;
                    int i = 0;

                    memset(notice.datainfo.carnum[i], 0, sizeof(notice.datainfo.carnum[i]));

                    memcpy(m_snap_osd[4].szStr, plate_infos.plate_num, sizeof(m_snap_osd[4].szStr)) ;

                    strncpy((char *)notice.datainfo.carnum[i], plate_infos.plate_num, strlen(plate_infos.plate_num));

                    notice.happentime = N9M_TMGetShareSecond();
                    m_detection_result_notify_func(&notice);
                    m_bCurrent_detection_state = 1;
                }
                break;
            case EN_PLATE_EMPTY:
                if(NULL != m_detection_result_notify_func)
                {
                    msgIPCIntelligentInfoNotice_t notice;
                    memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                    notice.cmdtype = 1;
                    notice.happen = 0;
                    notice.reserved[0] = 0;
                    int i = 0;

                    memset(notice.datainfo.carnum[i], 0, sizeof(notice.datainfo.carnum[i]));
                    //strncpy((char *)notice.datainfo.carnum[i], plate_infos.plate_num, strlen(plate_infos.plate_num));
                    memset(m_snap_osd[4].szStr, 0, sizeof(m_snap_osd[4].szStr)) ;
                    notice.happentime = N9M_TMGetShareSecond();
                    m_detection_result_notify_func(&notice);
                    m_bCurrent_detection_state = 1;
                }
                break;
            default:
                break;
            }

//! snap operation
        unsigned long long int snap_subtype = 0;
        VIDEO_FRAME_INFO_S single_frame;
        memset(&single_frame,0,sizeof(single_frame));

        switch(plate_infos.snap_type)
        {
            case EN_SNAP:
                DEBUG_CRITICAL("single snap\n");
                ive_single_snap(&single_frame,0);
                m_vFrames.push_back(single_frame);

                ive_obtain_osd(m_snap_osd,2);
                
                m_vSnap_osd.push_back(m_snap_osd[0]);
                m_vSnap_osd.push_back(m_snap_osd[1]);
                if ((m_single_snapped ==0)&&(plate_infos.roi_rect_info.u32Width!=0)&&(plate_infos.roi_rect_info.u32Height!=0))
                {
                    roi = plate_infos.roi_rect_info;    //!< obtain roi info from the first snap
                }

                m_single_snapped = 1;
                break;
            case EN_SZ_BUS_STOP:
                DEBUG_CRITICAL("EN_SZ_BUS_STOP \n");
                //if (plate_infos.plate_recognition_flag)
                {
                    if(( 0 == m_single_snapped)||(m_vFrames.size()<2))
                    {
                        ive_clear_snap();
                        m_vSnap_osd.clear();
                        DEBUG_CRITICAL("Invalid cmd!Single snap times incorrect!\n");
                        break;
                    }              
                    //ive_single_snap(&multi_frame[1],0); //!< second snap
                    ive_single_snap(&single_frame,0);
                    m_vFrames.push_back(single_frame);

                    snap_subtype = 0x08;
                    //! obtain osd info
                    ive_obtain_osd(&m_snap_osd[2],2);
                    memset(m_snap_osd[5].szStr, 0, sizeof(m_snap_osd[5].szStr));

                    memcpy(m_snap_osd[5].szStr, "SZ_BUS_STOP:", sizeof("SZ_BUS_STOP:")) ;
                    memcpy(m_snap_osd[5].szStr + strlen("SZ_BUS_STOP:"), m_snap_osd[4].szStr, strlen(m_snap_osd[5].szStr) - strlen("SZ_BUS_STOP:")) ;

                    //memcpy(m_snap_osd[4].szStr, plate_infos.plate_num, sizeof(m_snap_osd[4].szStr)) ;

                    m_vSnap_osd.push_back(m_snap_osd[2]);
                    m_vSnap_osd.push_back(m_snap_osd[3]);
                    m_vSnap_osd.push_back(m_snap_osd[5]);
                    //end of osd info

                    if (ive_stich_snap_for_singapore_bus(2,m_snap_osd,7,roi,snap_subtype) != 0)
                    {
                        DEBUG_ERROR("Stich failed!\n");
                        ive_clear_snap();
                        m_vSnap_osd.clear();
                        //m_vFrames.clear(); //! already clear in ive_clear_snap
                        break;
                    }
                    if(NULL != m_detection_result_notify_func)
                    {
                        msgIPCIntelligentInfoNotice_t notice;
                        memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                        notice.cmdtype = 1;
                        notice.happen = 1;  
                        notice.reserved[0] = 0;
                        int i = 0;

                        memset(notice.datainfo.carnum[i], 0, sizeof(notice.datainfo.carnum[i]));
                        strncpy((char *)notice.datainfo.carnum[i], plate_infos.plate_num, strlen(plate_infos.plate_num));

                            notice.happentime = N9M_TMGetShareSecond();
                            m_detection_result_notify_func(&notice);
                            m_bCurrent_detection_state = 1;
                        } 
                    }
                        break;
                    case EN_SZ_BUS_LANE:
                        DEBUG_CRITICAL("EN_SZ_BUS_LANE \n");
                        //if (plate_infos.plate_recognition_flag)
                        {
                            if(( 0 == m_single_snapped)||(m_vFrames.size()!= 2))
                            {
                                ive_clear_snap();
                                m_vSnap_osd.clear();                    
                                DEBUG_CRITICAL("Invalid cmd!Single snap times incorrect!\n");
                                break;
                            }

                            ive_single_snap(&single_frame,0);
                            m_vFrames.push_back(single_frame); //!< last snap, before which 2 snapped
                            snap_subtype = 0x08;

                            //! obtain osd info
                            ive_obtain_osd(&m_snap_osd[2],2);
                            int spec_len = strlen("SZ_BUS_LANE:");
                            if (spec_len > 30)
                            {
                                spec_len = 30;                
                            }
                            memset(m_snap_osd[5].szStr, 0, sizeof(m_snap_osd[5].szStr));

                            memcpy(m_snap_osd[5].szStr, "SZ_BUS_LANE:", spec_len) ;
                            memcpy(m_snap_osd[5].szStr + spec_len, m_snap_osd[4].szStr, sizeof(m_snap_osd[5].szStr) - spec_len) ;

                            m_vSnap_osd.push_back(m_snap_osd[2]);
                            m_vSnap_osd.push_back(m_snap_osd[3]);
                            m_vSnap_osd.push_back(m_snap_osd[5]);
                            //end of osd info
                            //DEBUG_CRITICAL("size of osd:%d, last osd:%s\n", m_vSnap_osd.size(),m_snap_osd[4].szStr);
                            if (ive_stich_snap_for_singapore_bus(2,m_snap_osd,7,roi,snap_subtype) != 0)
                            {
                                DEBUG_ERROR("Stich failed!\n");
                                ive_clear_snap();
                                m_vSnap_osd.clear();
                                //m_vFrames.clear();
                                break;
                            }
                            //! notify network
                            if(NULL != m_detection_result_notify_func)
                            {
                                msgIPCIntelligentInfoNotice_t notice;
                                memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                                notice.cmdtype = 1;
                                notice.happen = 1;  
                                notice.reserved[0] = 0;
                                int i = 0;

                                memset(notice.datainfo.carnum[i], 0, sizeof(notice.datainfo.carnum[i]));
                                strncpy((char *)notice.datainfo.carnum[i], plate_infos.plate_num, strlen(plate_infos.plate_num));

                                notice.happentime = N9M_TMGetShareSecond();
                                m_detection_result_notify_func(&notice);
                                m_bCurrent_detection_state = 1;
                        }
                    }
                    break;
                case EN_EMPTY_BUFFER:
                    DEBUG_CRITICAL("EN_EMPTY_BUFFER\n");
                    ive_clear_snap();
                    m_vSnap_osd.clear();
                    //m_vFrames.clear();
                    m_single_snapped = 0;
                    break;
                default:
                    break;
                }

    }while(0);

    if(release_falg)
    {
        if(0 != hivpss->HiVpss_release_frame(_VP_MAIN_STREAM_,phy_chn, &frame_1080p))
        {
            DEBUG_ERROR("vpss release 540p frame failed ! \n");
            ret = -1;
        }
    }
    
    if(0 != hivpss->HiVpss_release_frame(_VP_SPEC_USE_,phy_chn, &frame_540p))
    {
        DEBUG_ERROR("vpss release 1080P frame failed ! \n");
        ret = -1;
    }

    return ret;
#else
     
#endif
}

int CHiIveBle::ive_ble_process_frame_SG(int phy_chn)
{
    int ret = 0;
    static RECT_S roi = {0};
    CHiVpss* hivpss = static_cast<CHiVpss *>(m_pData_source);
    VIDEO_FRAME_INFO_S frame_1080p;
    VIDEO_FRAME_INFO_S  frame_540p;
    IVE_IMAGE_S image_1080p;
    IVE_IMAGE_S image_540p;

    //memset(&roi, 0, sizeof(RECT_S));
    memset(&frame_1080p, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&frame_540p, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&image_1080p, 0, sizeof(IVE_IMAGE_S));
    memset(&image_540p, 0, sizeof(IVE_IMAGE_S));

    if(0 != hivpss->HiVpss_get_frame( _VP_MAIN_STREAM_,phy_chn, &frame_1080p))  //! fix 720p
    {
        DEBUG_ERROR("vpss get frame failed ! \n");
        return -1;
    }
        
    if(0 != hivpss->HiVpss_get_frame( _VP_SPEC_USE_,phy_chn, &frame_540p))
    {
        DEBUG_ERROR("vpss get frame failed ! \n");
        if(0 != hivpss->HiVpss_release_frame(_VP_MAIN_STREAM_, phy_chn, &frame_1080p))
        {
            DEBUG_ERROR("vpss release frame failed ! \n");
        }
        return -1;
    }

    image_1080p.enType = (frame_1080p.stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_422)?IVE_IMAGE_TYPE_YUV422SP:IVE_IMAGE_TYPE_YUV420SP;
    for(int i=0; i<3; ++i)
    {
        image_1080p.u32PhyAddr[i] = frame_1080p.stVFrame.u32PhyAddr[i];
        image_1080p.u16Stride[i] = (HI_U16)frame_1080p.stVFrame.u32Stride[i];
        image_1080p.pu8VirAddr[i]= (HI_U8*)frame_1080p.stVFrame.pVirAddr[i];
    }

    image_1080p.u16Width = (HI_U16)frame_1080p.stVFrame.u32Width;
    image_1080p.u16Height = (HI_U16)frame_1080p.stVFrame.u32Height;

    image_540p.enType = (frame_540p.stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_422)?IVE_IMAGE_TYPE_YUV422SP:IVE_IMAGE_TYPE_YUV420SP;

    for(int i=0; i<3; ++i)
    {
        image_540p.u32PhyAddr[i] = frame_540p.stVFrame.u32PhyAddr[i];
        image_540p.u16Stride[i] = (HI_U16)frame_540p.stVFrame.u32Stride[i];
        image_540p.pu8VirAddr[i]= (HI_U8*)frame_540p.stVFrame.pVirAddr[i];
    }
    image_540p.u16Width = (HI_U16)frame_540p.stVFrame.u32Width;
    image_540p.u16Height = (HI_U16)frame_540p.stVFrame.u32Height;    

    do
    {
        LaneEnforcementInfo plate_infos;
        memset(&plate_infos, 0, sizeof(LaneEnforcementInfo));
        if(HI_SUCCESS != BusLaneEnforcementStart(&image_1080p, &image_540p, &plate_infos))
        {
            DEBUG_ERROR("IveAlgStart  failed! \n");
            BusLaneEnforcementParametersReset((HI_CHAR *)IVE_CONFIG_PATH);
            ret = -1;
            break;
        }

        if(1 == pgAvDeviceInfo->Adi_get_ive_debug_mode())
        {
            if(m_pEncoder != NULL)    
            {        
                m_pEncoder->Ae_send_frame_to_encoder(_AST_SUB_VIDEO_, 0, (void *)&frame_540p, -1); 
            }
        }

        //! for singapore bus snap
        
        unsigned long long int snap_subtype = 0;
        VIDEO_FRAME_INFO_S single_frame;
        memset(&single_frame,0,sizeof(single_frame));
        switch(plate_infos.snap_type)
        {
            case EN_SNAP:
                DEBUG_CRITICAL("single snap\n");
                
                //ive_single_snap(&multi_frame[0],0);
                ive_single_snap(&single_frame,0);
                m_vFrames.push_back(single_frame);
                    
                ive_obtain_osd(m_snap_osd,2);
                m_vSnap_osd.push_back(m_snap_osd[0]);
                m_vSnap_osd.push_back(m_snap_osd[1]);
                if ((m_single_snapped ==0)&&(plate_infos.roi_rect_info.u32Width!=0)&&(plate_infos.roi_rect_info.u32Height!=0))
                {
                    roi = plate_infos.roi_rect_info;    //!< obtain roi info from the first snap
                }

                m_single_snapped = 1;
                
                break;
                
            case EN_EMPTY_BUFFER:
                DEBUG_CRITICAL("EN_EMPTY_BUFFER\n");
                ive_clear_snap();
                m_vSnap_osd.clear();
                m_single_snapped = 0;
                break;
                
            case EN_SINGAPORE_BUS_STOP:
                DEBUG_CRITICAL("EN_SINGAPORE_BUS_STOP snap\n");
                if(( 0 == m_single_snapped)||(m_vFrames.size()<2))
                {
                    DEBUG_CRITICAL("Invalid cmd!Single snap not enough!\n");
                    break;
                }              
                //ive_single_snap(&multi_frame[1],0); //!< second snap
                ive_single_snap(&single_frame,0);
                m_vFrames.push_back(single_frame);
                
                snap_subtype = 0x08;
                //! obtain osd info
                ive_obtain_osd(&m_snap_osd[2],2);
             
                memcpy(m_snap_osd[4].szStr, "bus stop", sizeof("bus stop")) ;
                m_vSnap_osd.push_back(m_snap_osd[2]);
                m_vSnap_osd.push_back(m_snap_osd[3]);
                m_vSnap_osd.push_back(m_snap_osd[4]);
                //end of osd info
                
                if (ive_stich_snap_for_singapore_bus(2,m_snap_osd,7,roi,snap_subtype) != 0)
                {
                    DEBUG_ERROR("Stich failed!\n");
                    ive_clear_snap();
                    m_vSnap_osd.clear();
                    break;
                }
                //!notify network
                 if(NULL != m_detection_result_notify_func)
                {
                    msgIPCIntelligentInfoNotice_t notice;
                    memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                    notice.cmdtype = 1;
                    notice.happen = 1;  
                    notice.reserved[0] = 1;

                    notice.happentime = N9M_TMGetShareSecond();
                    m_detection_result_notify_func(&notice);
                    m_bCurrent_detection_state = 1;
                } 
                 
                 //!end of notify
                 memset(m_snap_osd, 0, sizeof(m_snap_osd));
                 m_vSnap_osd.clear();

                 m_single_snapped = 0;
                break;
                
            case EN_SINGAPORE_YELLOW_BUS_LANE:
                
                DEBUG_CRITICAL("EN_SINGAPORE_YELLOW_BUS_LANE snap\n");
                if(( 0 == m_single_snapped)||(m_vFrames.size()<2))
                {
                    DEBUG_CRITICAL("Invalid cmd!Single snap not enough!\n");
                    break;
                }
                if(ive_is_singapore_bus_valid_time(1))
                {    
                    ive_clear_snap();
                    m_vSnap_osd.clear();
                    DEBUG_CRITICAL("Not the bus time!\n");
                    break;
                }
                ive_single_snap(&single_frame,0);
                m_vFrames.push_back(single_frame); //!< second snap
                snap_subtype = 0x08;
                
                //! obtain osd info
                ive_obtain_osd(&m_snap_osd[2],2);
                memcpy(m_snap_osd[4].szStr, "yellow bus lane", sizeof("yellow bus lane")) ;

                m_vSnap_osd.push_back(m_snap_osd[2]);
                m_vSnap_osd.push_back(m_snap_osd[3]);
                m_vSnap_osd.push_back(m_snap_osd[4]);
                //end of osd info
                DEBUG_CRITICAL("size of osd:%d\n", m_vSnap_osd.size());
                if (ive_stich_snap_for_singapore_bus(2,m_snap_osd,7,roi,snap_subtype) != 0)
                {
                    DEBUG_ERROR("Stich failed!\n");
                    ive_clear_snap();
                    m_vSnap_osd.clear();
                    break;
                }

                //!notify network
                 if(NULL != m_detection_result_notify_func)
                {
                    msgIPCIntelligentInfoNotice_t notice;
                    memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                    notice.cmdtype = 1;
                    notice.happen = 1;  
                    notice.reserved[0] = 1;

                    notice.happentime = N9M_TMGetShareSecond();
                    m_detection_result_notify_func(&notice);
                    m_bCurrent_detection_state = 1;
                } 
                 //!end of notify
                memset(m_snap_osd, 0, sizeof(m_snap_osd));
                 m_vSnap_osd.clear();
                m_single_snapped = 0;

                break;
            case EN_SINGAPORE_PINK_BUS_LANE:
                
                DEBUG_CRITICAL("EN_SINGAPORE_PINK_BUS_LANE snap\n");
                if(( 0 == m_single_snapped)||(m_vFrames.size()<2))
                {
                    DEBUG_CRITICAL("Invalid cmd!Single snap not enough\n");
                    break;
                }  
                if(ive_is_singapore_bus_valid_time(2))
                {                   
                    DEBUG_CRITICAL("Not the bus time!\n");
                    ive_clear_snap(); 
                    m_vSnap_osd.clear();
                    break;
                }
                ive_single_snap(&single_frame,0);
                m_vFrames.push_back(single_frame); //!< second snap

                snap_subtype = 0x08;
                
                //! obtain osd info
                ive_obtain_osd(&m_snap_osd[2],2);
                memcpy(m_snap_osd[4].szStr, "pink bus lane", sizeof("pink bus lane")) ;
                m_vSnap_osd.push_back(m_snap_osd[2]);
                m_vSnap_osd.push_back(m_snap_osd[3]);
                m_vSnap_osd.push_back(m_snap_osd[4]);
                //end of osd info
                
                if (ive_stich_snap_for_singapore_bus(2,m_snap_osd,7,roi,snap_subtype) != 0)
                {
                    DEBUG_ERROR("Stich failed!\n");
                    ive_clear_snap();
                    m_vSnap_osd.clear();
                    break;
                }

                //!notify network
                 if(NULL != m_detection_result_notify_func)
                {
                    msgIPCIntelligentInfoNotice_t notice;
                    memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                    notice.cmdtype = 1;
                    notice.happen = 1;  
                    notice.reserved[0] = 1;

                    notice.happentime = N9M_TMGetShareSecond();
                    m_detection_result_notify_func(&notice);
                    m_bCurrent_detection_state = 1;
                } 
                 //!end of notify 
                memset(m_snap_osd, 0, sizeof(m_snap_osd));
                m_vSnap_osd.clear();
                m_single_snapped = 0;
                break;
            case EN_NONE:
                
                break;                    
            default:
                DEBUG_CRITICAL("other\n");
                //m_single_snapped = 0;
                break;
        }
        //!end of singapore snap
#if 0
        if(plate_infos.plate_num > 0 && plate_infos.capture_flag == HI_TRUE)
        {
            ive_snap_info_s snap_info;
            memset(&snap_info, 0, sizeof(ive_snap_info_s));  
            snap_info.pPicture_info = frame_1080p;
            snap_info.u8Snap_type = 2;   
            snap_info.u8Picture_nums =  plate_infos.plate_num;
            snap_info.pPicture_size = (RECT_S*)malloc(snap_info.u8Picture_nums*sizeof(RECT_S));
            if(NULL == snap_info.pPicture_size)
            {
                DEBUG_ERROR("malloc snap picture rect  failed! \n");
                ret = -1;
                break;
            }
            
            for(int i=0; i<plate_infos.plate_num; ++i)
            {
                snap_info.pPicture_size[i].s32X = plate_infos.plate_pos_info[i].s32X*3;
                snap_info.pPicture_size[i].s32Y = plate_infos.plate_pos_info[i].s32Y*3;
                snap_info.pPicture_size[i].u32Width = plate_infos.plate_pos_info[i].u32Width*3;
                snap_info.pPicture_size[i].u32Height = plate_infos.plate_pos_info[i].u32Height*3;
            }
            m_ive_snap_thread->AddIveSnapTask(snap_info);
        }
#else
#if 0
        if(m_bCurrent_detection_state == 0)
        {
            plate_infos.plate_info_flag = 0x07;
        }
        else
        {
            plate_infos.plate_info_flag = 0x01;
        }
#endif
/*        
        switch(plate_infos.plate_info_flag)
        {
            case 0x07:
            {
                if(NULL != m_detection_result_notify_func)
                {
                    msgIPCIntelligentInfoNotice_t notice;
                    memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                    notice.cmdtype = 1;
                    notice.happen = 1;  
                    notice.reserved[0] = 0;
#if 1
                    for(int i=0; i<plate_infos.plate_count;++i)
                    {
                        notice.datainfo.carrange[i].vilid = 1;
                        notice.datainfo.carrange[i].point.x = plate_infos.plate_pos_info[i].s32X * 3;
                        notice.datainfo.carrange[i].point.y = plate_infos.plate_pos_info[i].s32Y * 3;
                        notice.datainfo.carrange[i].point.Width = plate_infos.plate_pos_info[i].u32Width * 3;
                        notice.datainfo.carrange[i].point.Height = plate_infos.plate_pos_info[i].u32Height * 3;
                    }
#else
                    plate_infos.plate_count = 2;
                    
                    for(int i=0; i<plate_infos.plate_count;++i)
                    {
                        notice.datainfo.carrange[i].vilid = 1;
                        notice.datainfo.carrange[i].point.x =  (40 + i*16) * 3;
                        notice.datainfo.carrange[i].point.y = (40 + i*16  )* 3;
                        notice.datainfo.carrange[i].point.Width = 240;
                        notice.datainfo.carrange[i].point.Height = 480;
                    }
#endif
                    notice.happentime = N9M_TMGetShareSecond();
                    m_detection_result_notify_func(&notice);
                    m_bCurrent_detection_state = 1;
                } 
            }
                break;
            case 0x0D:
            {
                if(NULL != m_detection_result_notify_func)
                {
                    msgIPCIntelligentInfoNotice_t notice;
                    memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                    notice.cmdtype = 1;
                    notice.happen = 1;  
                    notice.reserved[0] = 1;
                    for(int i=0; i<plate_infos.plate_count;++i)
                    {
                        memset(notice.datainfo.carnum[i], 0, sizeof(notice.datainfo.carnum[i]));
                        strncpy((char *)notice.datainfo.carnum[i], plate_infos.plate_num[i], strlen(plate_infos.plate_num[i]));
                    }
                    notice.happentime = N9M_TMGetShareSecond();
                    m_detection_result_notify_func(&notice);
                    m_bCurrent_detection_state = 1;
                }                
            }
                break;
            case 0x03:
            case 0x1:
            default:
                if(NULL != m_detection_result_notify_func && 0 != m_bCurrent_detection_state )
                {
                    msgIPCIntelligentInfoNotice_t notice;
                    memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                    notice.cmdtype = 1;
                    notice.happen = 0;  
                    notice.happentime = N9M_TMGetShareSecond();
                    m_bCurrent_detection_state = 0;
                    m_detection_result_notify_func(&notice);
                }
                
                break;
        }*/

#endif
    }while(0);

    if(0 != hivpss->HiVpss_release_frame(_VP_MAIN_STREAM_,phy_chn, &frame_1080p)) //!< for singapore bus obtain from sub stream
    {
        DEBUG_ERROR("vpss release 1080P frame failed ! \n");
        ret = -1;
    }
 
    if(0 != hivpss->HiVpss_release_frame(_VP_SPEC_USE_,phy_chn, &frame_540p))
    {
        DEBUG_ERROR("vpss release 540p frame failed ! \n");
        ret = -1;
    }
    
    return ret;
}



#define OSD_VERTICAL_INTERVAL 50
#define DATA_TIME_OSD_X   (1200)
#define DATA_TIME_OSD_Y    (10)
#define GPS_OSD_X  (1200)
#define GPS_OSD_Y  (DATA_TIME_OSD_Y+OSD_VERTICAL_INTERVAL)
#define VEHICLE_OSD_SATRT_X  (40)
#define VEHICLE_OSD_START_Y   (10)

int CHiIveBle::ive_ble_process_frame_for_CQJY(int phy_chn)
{
    int ret = 0;
    CHiVpss* hivpss = static_cast<CHiVpss *>(m_pData_source);
    VIDEO_FRAME_INFO_S frame_720p;
    VIDEO_FRAME_INFO_S  frame_540p;
    IVE_IMAGE_S image_720p;
    IVE_IMAGE_S image_540p;
        
    memset(&frame_720p, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&frame_540p, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&image_720p, 0, sizeof(IVE_IMAGE_S));
    memset(&image_540p, 0, sizeof(IVE_IMAGE_S));
    
    if(0 != hivpss->HiVpss_get_frame( _VP_MAIN_STREAM_,phy_chn, &frame_720p))
    {
        DEBUG_ERROR("vpss get frame failed ! \n");
        return -1;
    }
            
        if(0 != hivpss->HiVpss_get_frame( _VP_SPEC_USE_,phy_chn, &frame_540p))
        {
            DEBUG_ERROR("vpss get frame failed ! \n");
            if(0 != hivpss->HiVpss_release_frame(_VP_MAIN_STREAM_, phy_chn, &frame_720p))
            {
                DEBUG_ERROR("vpss release frame failed ! \n");
            }
            return -1;
        }
    
        image_720p.enType = (frame_720p.stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_422)?IVE_IMAGE_TYPE_YUV422SP:IVE_IMAGE_TYPE_YUV420SP;
        for(int i=0; i<3; ++i)
        {
            image_720p.u32PhyAddr[i] = frame_720p.stVFrame.u32PhyAddr[i];
            image_720p.u16Stride[i] = (HI_U16)frame_720p.stVFrame.u32Stride[i];
            image_720p.pu8VirAddr[i]= (HI_U8*)frame_720p.stVFrame.pVirAddr[i];
        }
    
        image_720p.u16Width = (HI_U16)frame_720p.stVFrame.u32Width;
        image_720p.u16Height = (HI_U16)frame_720p.stVFrame.u32Height;
    
        image_540p.enType = (frame_540p.stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_422)?IVE_IMAGE_TYPE_YUV422SP:IVE_IMAGE_TYPE_YUV420SP;
        for(int i=0; i<3; ++i)
        {
            image_540p.u32PhyAddr[i] = frame_540p.stVFrame.u32PhyAddr[i];
            image_540p.u16Stride[i] = (HI_U16)frame_540p.stVFrame.u32Stride[i];
            image_540p.pu8VirAddr[i]= (HI_U8*)frame_540p.stVFrame.pVirAddr[i];
        }
        image_540p.u16Width = (HI_U16)frame_540p.stVFrame.u32Width;
        image_540p.u16Height = (HI_U16)frame_540p.stVFrame.u32Height;    

        do
        {
            //PlateInfo plate_infos;
            LaneEnforcementInfo plate_infos;
            memset(&plate_infos, 0, sizeof(LaneEnforcementInfo));
            if(HI_SUCCESS != BusLaneEnforcementStart(&image_720p, &image_540p, &plate_infos))
            {
                DEBUG_ERROR("IveAlgStart  failed! \n");
                BusLaneEnforcementParametersReset((char *)"./");
                ret = -1;
                break;
            }
            if(1 == pgAvDeviceInfo->Adi_get_ive_debug_mode())
            {
                if(m_pEncoder != NULL)    
                {        
                    m_pEncoder->Ae_send_frame_to_encoder(_AST_SUB_VIDEO_, 0, (void *)&frame_540p, -1); 
                }
            }           
    
            VIDEO_FRAME_INFO_S single_frame;
            memset(&single_frame,0,sizeof(single_frame));
#if 0
            plate_infos.plate_recognition_flag = EN_PLATE_UPDATE;
            plate_infos.license_plate_num = 3;
            plate_infos.snap_type = EN_SNAP;
            char vehicles[45] = {0xE6,0xB8,0x9D,0x51,0x38,0x38,0x37,0x39,0x30, \
                0xE6,0xB8,0x9D,0x41,0x39,0x38,0x30,0x39,0x43, 0xE6,0xB8,0x9D,0x43,0x30,0x34,0x36,0x35,0x32,0x0};
            memcpy(plate_infos.plate_num, vehicles, sizeof(plate_infos.plate_num));
#endif    
            switch(plate_infos.plate_recognition_flag)
            {
                case EN_PLATE_UPDATE:
                {
                    DEBUG_MESSAGE("EN_PLATE_UPDATE \n");
                    int detected_vehicle_num = 0;
                    int illegal_vehicle_num = 0;
                    unsigned char black_car_bitmap = 0;
                    char vehicle_number[PLATE_CHAR_NUM_MAX+1];
                    char vehicle_info[32]; 
                    msgIPCIntelligentInfoNotice_t notice;
                    
                    memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                    
                    detected_vehicle_num = plate_infos.license_plate_num>PLATE_NUM_MAX? PLATE_NUM_MAX: plate_infos.license_plate_num;
                    for(int idx=0; idx<detected_vehicle_num; ++idx)
                    {
                        memset(vehicle_number, 0, sizeof(vehicle_number));
                        memset(vehicle_info, 0, sizeof(vehicle_info));
                        strncpy(vehicle_number, plate_infos.plate_num+idx*PLATE_CHAR_NUM_MAX, PLATE_CHAR_NUM_MAX);
                        if(!is_vehicle_detected(vehicle_number, strlen(vehicle_number)) && is_vehicle_in_blacklist(vehicle_number, strlen(vehicle_number), vehicle_info, sizeof(vehicle_info)))
                        {
                            strncpy(m_snap_osd[illegal_vehicle_num+2].szStr, vehicle_number, PLATE_CHAR_NUM_MAX);
                            m_snap_osd[illegal_vehicle_num+2].osd_type = _AON_BUS_NUMBER_;
                            m_snap_osd[illegal_vehicle_num+2].szStr[PLATE_CHAR_NUM_MAX] = '\0';
                            m_snap_osd[illegal_vehicle_num+2].x = VEHICLE_OSD_SATRT_X;
                            m_snap_osd[illegal_vehicle_num+2].y = VEHICLE_OSD_START_Y + illegal_vehicle_num*OSD_VERTICAL_INTERVAL;
                            ++illegal_vehicle_num;
                            black_car_bitmap = black_car_bitmap | (1 << idx);
                            strncpy((char *)notice.datainfo.carnum[idx], vehicle_info, strlen(vehicle_info));
                        }
                        else
                        {
                            strncpy((char *)notice.datainfo.carnum[idx], plate_infos.plate_num+idx*PLATE_CHAR_NUM_MAX, PLATE_CHAR_NUM_MAX);
                        }
                    }
                    
                    if(illegal_vehicle_num > 0)
                    {
                        ive_single_snap(&single_frame,0);       
                        ive_obtain_osd(m_snap_osd,2);

                        m_snap_osd[0].osd_type = _AON_GPS_INFO_;
                        m_snap_osd[0].x = GPS_OSD_X;
                        m_snap_osd[0].y = GPS_OSD_Y;
                        m_snap_osd[1].osd_type = _AON_DATE_TIME_;
                        m_snap_osd[1].x = DATA_TIME_OSD_X;
                        m_snap_osd[1].y = DATA_TIME_OSD_Y;     
                        ive_single_snap(&single_frame, m_snap_osd, illegal_vehicle_num+2);
                    }

                    if(NULL != m_detection_result_notify_func)
                    {
                        notice.cmdtype = 3;
                        notice.happen = 1;
                        notice.reserved[0] = 1;
                        notice.reserved[1] = black_car_bitmap;
                        
                        notice.happentime = N9M_TMGetShareSecond();
                        m_detection_result_notify_func(&notice);
                    }
                }
                    break;
               
                case EN_PLATE_EMPTY:
                    DEBUG_MESSAGE("EN_EMPTY_BUFFER\n");
                    //clear the sub stream osd
                    if(NULL != m_detection_result_notify_func)
                    {
                        msgIPCIntelligentInfoNotice_t notice;
                        memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                        notice.cmdtype = 1;
                        notice.happen = 0;
                        notice.reserved[0] = 1;
                    
                        notice.happentime = N9M_TMGetShareSecond();
                        m_detection_result_notify_func(&notice);
                    }
                    break;
                default:
                    break;
            }
#if 0
            switch(plate_infos.snap_type)
            {
                case EN_SNAP:
                {
                    DEBUG_MESSAGE("EN_SNAP \n");
                    int detected_vehicle_num = 0;
                    int illegal_vehicle_num = 0;
                    char vehicle_number[PLATE_CHAR_NUM_MAX+1];

                    detected_vehicle_num = plate_infos.license_plate_num>PLATE_NUM_MAX? PLATE_NUM_MAX: plate_infos.license_plate_num;
                    for(int idx=0; idx<detected_vehicle_num; ++idx)
                    {
                        memset(vehicle_number, 0, sizeof(vehicle_number));
                        strncpy(vehicle_number, plate_infos.plate_num+idx*PLATE_CHAR_NUM_MAX, PLATE_CHAR_NUM_MAX);
                        if(!is_vehicle_detected(vehicle_number, strlen(vehicle_number)) && is_vehicle_in_blacklist(vehicle_number, strlen(vehicle_number)))
                        {
                            strncpy(m_snap_osd[illegal_vehicle_num+2].szStr, vehicle_number, PLATE_CHAR_NUM_MAX);
                            m_snap_osd[illegal_vehicle_num+2].szStr[PLATE_CHAR_NUM_MAX] = '\0';
                            m_snap_osd[illegal_vehicle_num+2].x = VEHICLE_OSD_SATRT_X;
                            m_snap_osd[illegal_vehicle_num+2].y = VEHICLE_OSD_START_Y + illegal_vehicle_num*OSD_VERTICAL_INTERVAL;
                            ++illegal_vehicle_num;
                        }
                    }

                    if(illegal_vehicle_num > 0)
                    {
                        ive_single_snap(&single_frame,0);       
                        ive_obtain_osd(m_snap_osd,2);
                        
                        m_snap_osd[0].x = GPS_OSD_X;
                        m_snap_osd[0].y = GPS_OSD_Y;
                        m_snap_osd[1].x = DATA_TIME_OSD_X;
                        m_snap_osd[1].y = DATA_TIME_OSD_Y;     
                        ive_single_snap(&single_frame, m_snap_osd, illegal_vehicle_num+2);
                    }
#if 1
                    int osd_nums = plate_infos.license_plate_num>PLATE_NUM_MAX? PLATE_NUM_MAX: plate_infos.license_plate_num;
                    for(int i=0; i<osd_nums;++i)
                    {
                        strncpy(m_snap_osd[i+2].szStr, plate_infos.plate_num+i*PLATE_CHAR_NUM_MAX, PLATE_CHAR_NUM_MAX);
                        m_snap_osd[i+2].szStr[PLATE_CHAR_NUM_MAX] = '\0';
                        m_snap_osd[i+2].x = VEHICLE_OSD_SATRT_X;
                        m_snap_osd[i+2].y = VEHICLE_OSD_START_Y + i*OSD_VERTICAL_INTERVAL;
                    }
                    ive_single_snap(&single_frame, m_snap_osd, osd_nums+2);
#endif  
                }
                    break;
                default:
                    break;
            }
#endif
        }while(0);
    
        if(0 != hivpss->HiVpss_release_frame(_VP_MAIN_STREAM_,phy_chn, &frame_720p))
        {
            DEBUG_ERROR("vpss release 720P frame failed ! \n");
            ret = -1;
        }
    
        if(0 != hivpss->HiVpss_release_frame(_VP_SPEC_USE_,phy_chn, &frame_540p))
        {
            DEBUG_ERROR("vpss release 540p frame failed ! \n");
            ret = -1;
        }
    
        return ret;
}


int CHiIveBle::ive_is_singapore_bus_valid_time(int time_type)
{
    int ret = -1;
    datetime_t date_time;
    pgAvDeviceInfo->Adi_get_date_time(&date_time);
    int week = date_time.week;
    int hour = date_time.hour;
    int min = date_time.minute;
    
    switch(time_type)
        {
            case 1://! yellow bus
                if((week <=m_yellow_lane_time.end_week) && (week >=m_yellow_lane_time.start_week) )
                {
                    if (((hour < m_yellow_lane_time.end_hour1) && (hour > m_yellow_lane_time.start_hour1) )||\
                        ((hour < m_yellow_lane_time.end_hour2) && (hour > m_yellow_lane_time.start_hour2) ))
                    {

                        ret = 0;

                    }//!start == end hour
                    else if((hour = m_yellow_lane_time.end_hour1) && (hour = m_yellow_lane_time.start_hour1) &&\
                        (min <=m_yellow_lane_time.end_min1)&& (min >=m_yellow_lane_time.start_min1))
                    {
                        ret = 0;
                    } 
                    else if((hour = m_yellow_lane_time.end_hour2) && (hour = m_yellow_lane_time.start_hour2) &&\
                        (min <=m_yellow_lane_time.end_min2)&& (min >=m_yellow_lane_time.start_min2))
                    {
                        ret = 0;
                    }//! start != end hour
                    else if((hour = m_yellow_lane_time.end_hour1) && (hour > m_yellow_lane_time.start_hour1) &&\
                        (min <=m_yellow_lane_time.end_min1))
                    {
                        ret = 0;
                    } 
                    else if((hour = m_yellow_lane_time.end_hour2) && (hour > m_yellow_lane_time.start_hour2) &&\
                        (min <=m_yellow_lane_time.end_min2))
                    {
                        ret = 0;
                    }
                    else if((hour < m_yellow_lane_time.end_hour1) && (hour = m_yellow_lane_time.start_hour1) &&\
                        (min >=m_yellow_lane_time.start_min1))
                    {
                        ret = 0;
                    } 
                    else if((hour < m_yellow_lane_time.end_hour2) && (hour = m_yellow_lane_time.start_hour2) &&\
                        (min >=m_yellow_lane_time.start_min2))
                    {
                        ret = 0;
                    }
                    
                }
                break;            
            case 2: //!pink
                if((week <=m_pink_lane_time.end_week) && (week >=m_pink_lane_time.start_week) )
                {
                    if (((hour < m_pink_lane_time.end_hour1) && (hour > m_pink_lane_time.start_hour1)))
                    {

                        ret = 0;

                    }//!start == end hour
                    else if((hour = m_yellow_lane_time.end_hour1) && (hour = m_yellow_lane_time.start_hour1) &&\
                        (min <=m_yellow_lane_time.end_min1)&& (min >=m_yellow_lane_time.start_min1))
                    {
                        ret = 0;
                    } //! start != end hour
                    else if((hour = m_yellow_lane_time.end_hour1) && (hour > m_yellow_lane_time.start_hour1) &&\
                        (min <=m_yellow_lane_time.end_min1))
                    {
                        ret = 0;
                    } 
                    else if((hour < m_yellow_lane_time.end_hour1) && (hour = m_yellow_lane_time.start_hour1) &&\
                        (min >=m_yellow_lane_time.start_min1))
                    {
                        ret = 0;
                    } 

                }
                break;
                ret = -1;
            default:
                break;
    }
    return ret;
}


int CHiIveBle::ive_obtain_osd(hi_snap_osd_t * snap_osd,int osd_num)
{
    if (osd_num < 2)
    {
        DEBUG_ERROR("osd num(%d) is invalid!\n", osd_num);
        return -1;
    }
    if(0 == m_pOsd[0])
    {
        strncpy(snap_osd[0].szStr, "000.000.0000E 000.000.0000N",32);
        //strncpy(snap_osd[0].szStr, "Location invalid",32);
        snap_osd[0].szStr[32-1] = '\0';  
    }
    else
    {
        strncpy(snap_osd[0].szStr, m_pOsd,32);
        snap_osd[0].szStr[32-1] = '\0';  
    }
    datetime_t date_time;
    pgAvDeviceInfo->Adi_get_date_time(&date_time);
    snprintf(snap_osd[1].szStr,32, "20%02d-%02d-%02d", date_time.year, date_time.month, date_time.day);    
    snprintf(snap_osd[1].szStr, 32,"%s %02d:%02d:%02d", snap_osd[1].szStr, date_time.hour, date_time.minute, date_time.second);

    return 0;    
}


int CHiIveBle::ive_stich_snap_for_singapore_bus(int frame_num,
                                                hi_snap_osd_t * snap_osd,int osd_num,RECT_S roi,
                                                unsigned long long int snap_subtype)
{
    //! check the arguments
    DEBUG_ERROR("Roi info,x:%d, y:%d, width:%d,height:%d\n", roi.s32X, roi.s32Y, roi.u32Width, roi.u32Height);
    if((roi.s32X < 0)||(roi.s32Y < 0)||(roi.u32Height < 50)||(roi.u32Width<50))
    {
        roi.u32Height =  50;
        roi.u32Width =  50;
        DEBUG_ERROR("Invalid roi info,x:%d, y:%d, width:%d,height:%d\n", roi.s32X, roi.s32Y, roi.u32Width, roi.u32Height);
        //return -1;
    }

    if (( m_vFrames.size() != 3)||(m_vSnap_osd.size() != 7))
    {
        DEBUG_ERROR("Invalid frames snapped!\n");
        return -1;
    }
    int ret = 0; 
    //int osd_height = 40;
    
    int x = roi.s32X;
    int y = roi.s32Y;
    int crop_width = roi.u32Width;
    int crop_height = roi.u32Height;
        
    VGS_HANDLE vgs_handle;
    VGS_TASK_ATTR_S vgs_task;
   // FILE* pfile = NULL;
   // HI_CHAR file_name[128];

    VIDEO_FRAME_INFO_S snap_frame;
    VIDEO_FRAME_INFO_S  dest_frame;
    VIDEO_FRAME_INFO_S src_frame;
    
    HI_U32 luma_addr_offset = 0;
    HI_U32 chroma_addr_offset = 0;
    //! application related
    HI_U32 snap_width = m_vFrames[0].stVFrame.u32Width;  //!< final resulted frame size after stich
    HI_U32 snap_height = m_vFrames[0].stVFrame.u32Height;
    int roi_scale = snap_width/640;

    //x *=roi_scale;
    //y *=roi_scale;
    //crop_width *=roi_scale;
    //crop_height *=roi_scale;
    crop_width = snap_width/2; //! same as height or rectangle
    crop_height = snap_height/2; 
    
    crop_width = (crop_width+1)/4 *4;
    crop_height = (crop_height + 1)/4 *4;
    
    x -= crop_width / roi_scale /2;
    y -= crop_height / roi_scale /2;
    
    x *=roi_scale;
    y *=roi_scale;
    if (x<0)
    {
        x=0;
    }
    if (y<0)
    {
        y=0;
    }
    x = (x+1)/4 *4;
    y = (y + 1)/4 *4;

    
    DEBUG_CRITICAL("snap wid:%d, snap height:%d, crop wid:%d, crop height:%d, roi scale:%d, x:%d, y:%d\n", \
                    snap_width, snap_height,crop_width,crop_height,roi_scale,x,y);
    //! the single frame size before encoder
    HI_U32 scaled_width =  snap_width/2;  
    HI_U32 scaled_height =  snap_height/2;
    
    rm_uint64_t after_scale = 0;
    rm_uint64_t befor_scale = 0;

    memset(&snap_frame, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&vgs_task, 0, sizeof(VGS_TASK_ATTR_S));

    memset(&dest_frame, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&src_frame, 0, sizeof(VIDEO_FRAME_INFO_S));
    
    memset(m_u32Frame_vir_addr, 0, snap_width*snap_height);
    memset(m_u32Frame_vir_addr + snap_width*snap_height, 128, snap_width*snap_height/2); 
    
    befor_scale = N9M_TMGetSystemMicroSecond();
    
//! copy frames

    //copy 1st 2nd 3th frame
    for(unsigned int i=0; i<4; i++)
    {
        //printf("[alex]copy the data of frame:%d \n", i);
        //luma_addr_offset = (i/2)*IVE_BLE_SNAP_PICTURE_WIDTH*IVE_BLE_SCALE_FRAME_HEIGHT + (i%2)*IVE_BLE_SCALE_FRAME_WIDTH;
        //chroma_addr_offset = IVE_BLE_SNAP_PICTURE_WIDTH*IVE_BLE_SNAP_PICTURE_HEIGHT + (i/2)*IVE_BLE_SNAP_PICTURE_WIDTH*IVE_BLE_SCALE_FRAME_HEIGHT/2 + (i%2)*IVE_BLE_SCALE_FRAME_WIDTH;

        luma_addr_offset = (i/2)*snap_width*scaled_height + (i%2)*scaled_width;
        chroma_addr_offset = snap_width*snap_height + (i/2)*snap_width*scaled_height/2 + (i%2)*scaled_width;
    
        if(3 == i)
        {
            //! roi region is obtained from first frame
            memcpy(&src_frame, &m_vFrames[0], sizeof(VIDEO_FRAME_INFO_S));
            src_frame.stVFrame.u32PhyAddr[0] = m_vFrames[0].stVFrame.u32PhyAddr[0] +  m_vFrames[0].stVFrame.u32Stride[0]*y+x;
            src_frame.stVFrame.u32PhyAddr[1] = m_vFrames[0].stVFrame.u32PhyAddr[1] + m_vFrames[0].stVFrame.u32Stride[1]*y/2+x;
            src_frame.stVFrame.pVirAddr[0] = (HI_VOID *)((HI_U32)m_vFrames[0].stVFrame.pVirAddr[0] + m_vFrames[0].stVFrame.u32Stride[0]*y+x);
            src_frame.stVFrame.pVirAddr[1] = (HI_VOID *)((HI_U32)m_vFrames[0].stVFrame.pVirAddr[1] + m_vFrames[0].stVFrame.u32Stride[1]*y/2+x);
            src_frame.stVFrame.u32Width = crop_width;
            src_frame.stVFrame.u32Height = crop_height;
        }
        
        dest_frame.stVFrame.u32PhyAddr[0] = m_u32Frame_phy_addr + luma_addr_offset;        //luma component
        dest_frame.stVFrame.u32PhyAddr[1] = m_u32Frame_phy_addr + chroma_addr_offset;
        dest_frame.stVFrame.pVirAddr[0] = m_u32Frame_vir_addr + luma_addr_offset;
        dest_frame.stVFrame.pVirAddr[1] = m_u32Frame_vir_addr + chroma_addr_offset;
        if(3 == i)
        {
            dest_frame.stVFrame.u32Width = scaled_height*crop_width/crop_height;//1280;//scale_width;
            dest_frame.stVFrame.u32Height = scaled_height;

        }
        else
        {
            dest_frame.stVFrame.u32Width = scaled_width;//frame_snaped[i].stVFrame.u32Width;
            dest_frame.stVFrame.u32Height = scaled_height;//frame_snaped[i].stVFrame.u32Height;
        }
        dest_frame.stVFrame.u32Stride[0] =  snap_width; 
        dest_frame.stVFrame.u32Stride[1] = snap_width;
        dest_frame.stVFrame.enCompressMode = COMPRESS_MODE_NONE;
        dest_frame.stVFrame.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        dest_frame.stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
        dest_frame.u32PoolId = m_u32Pool;
        
//!  employ video graphic system
        ret = HI_MPI_VGS_BeginJob(&vgs_handle);
        if(HI_SUCCESS != ret)
        {
            DEBUG_ERROR("HI_MPI_VGS_BeginJob failed! ,errorcode:0x%x \n", ret);
            break;
        }

        if(3 == i)
        {
            memcpy(&vgs_task.stImgIn, &src_frame.stVFrame, sizeof(VIDEO_FRAME_INFO_S));
        }
        else
        {
            memcpy(&vgs_task.stImgIn, &m_vFrames[i].stVFrame, sizeof(VIDEO_FRAME_INFO_S));
        }
        
        memcpy(&vgs_task.stImgOut , &dest_frame.stVFrame, sizeof(VIDEO_FRAME_INFO_S));
        ret = HI_MPI_VGS_AddScaleTask(vgs_handle, &vgs_task);
        if(HI_SUCCESS != ret)
        {
            DEBUG_ERROR("HI_MPI_VGS_AddScaleTask failed! errcode:0x%x ,inw:%d, inh:%d,outw:%d,outh:%d\n", ret,\
            vgs_task.stImgIn.stVFrame.u32Width, vgs_task.stImgIn.stVFrame.u32Height,\
            vgs_task.stImgOut.stVFrame.u32Width, vgs_task.stImgOut.stVFrame.u32Height);
            HI_MPI_VGS_CancelJob(vgs_handle);
        }
        ret = HI_MPI_VGS_EndJob(vgs_handle);
        if (ret != HI_SUCCESS)
        {
            DEBUG_ERROR("HI_MPI_VGS_EndJob failed! errcode:0x%x \n", ret);
            HI_MPI_VGS_CancelJob(vgs_handle);
        } 
    }
    
    snap_frame.stVFrame.u32Width = snap_width;
    snap_frame.stVFrame.u32Height = snap_height;
    snap_frame.stVFrame.u32Field = m_vFrames[0].stVFrame.u32Field;
    snap_frame.stVFrame.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    snap_frame.stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
    snap_frame.stVFrame.enCompressMode = COMPRESS_MODE_NONE;
    snap_frame.stVFrame.u32PhyAddr[0] = m_u32Frame_phy_addr;
    snap_frame.stVFrame.u32PhyAddr[1] = m_u32Frame_phy_addr + snap_width*snap_height;
    snap_frame.stVFrame.pVirAddr[0] = m_u32Frame_vir_addr;
    snap_frame.stVFrame.pVirAddr[1] = m_u32Frame_vir_addr + snap_width*snap_height;
    snap_frame.stVFrame.u32Stride[0] = snap_width;
    snap_frame.stVFrame.u32Stride[1] = snap_width;
    snap_frame.stVFrame.u64pts = m_vFrames[0].stVFrame.u64pts;
    snap_frame.stVFrame.u32TimeRef = m_vFrames[0].stVFrame.u32TimeRef;
    snap_frame.u32PoolId = m_u32Pool;

    ive_snap_task_s task;
    memset(&task, 0, sizeof(ive_snap_task_s));
    task.u8Osd_num = osd_num;
    task.u8Snap_type = 1;
    task.u8Snap_subtype = 1;
    task.pOsd = new ive_snap_osd_display_attr_s[osd_num];

    if( osd_num == 5)
    {
        for (int i=0;i < 5;i++)
        {
            strncpy(task.pOsd[i].pContent, snap_osd[i].szStr, sizeof(task.pOsd[i].pContent)); 
        }
    //! first frame gps and time       
        HI_U32 len = strlen(task.pOsd[0].pContent);        
        HI_U32 font_width = 36;
        HI_U32 ox = 0;
        int t= scaled_width - font_width*len/2;
        if(t>0)
        {
            ox = (unsigned int)t;
        }
        else
        {
            ox = 0;
        }
        task.pOsd[0].u16X = ox;
        task.pOsd[0].u16Y = 150;   

        len = strlen(task.pOsd[1].pContent); 

        task.pOsd[1].u16X = ox;
        task.pOsd[1].u16Y = 50 ;

        //! second frame gps and time
        len = strlen(task.pOsd[2].pContent);
        t = scaled_width*2 - font_width*len/2;
        if(t> (int)scaled_width)
        {
            ox = (unsigned int)t;
        }
        else
        {
            ox = scaled_width;
        }
        task.pOsd[2].u16X = ox;
        task.pOsd[2].u16Y = 150;

        len = strlen(task.pOsd[3].pContent);      
        task.pOsd[3].u16X = ox;
        task.pOsd[3].u16Y = 50;

        //!special osd
        len = strlen(task.pOsd[4].pContent);
        t = scaled_width*2 - (scaled_width*2 - scaled_height*crop_width/crop_height) / 2 - font_width*len/2;
        if(t > (int)scaled_height)
        {
           ox = (unsigned int)t;
        }
        else
        {
            ox = scaled_height;
        }
        task.pOsd[4].u16X = ox;
        task.pOsd[4].u16Y = scaled_height + scaled_height/2;

    }
    else if(osd_num == 7)
    {
        for (int i=0;i < 7;i++)
        {
            strncpy(task.pOsd[i].pContent, m_vSnap_osd[i].szStr, sizeof(task.pOsd[i].pContent)); 
        }
        //! first frame gps and time       
     
            task.pOsd[0].u16X = 10;//ox;
            task.pOsd[0].u16Y = 60;   
        
        
            task.pOsd[1].u16X = 10;//ox;
            task.pOsd[1].u16Y = 10 ;
        
            //! second frame gps and time

            task.pOsd[2].u16X = scaled_width + 10;//ox;
            task.pOsd[2].u16Y = 60;
           
            task.pOsd[3].u16X = scaled_width + 10;//ox;
            task.pOsd[3].u16Y = 10;

            
            //! third frame gps and time
            
            task.pOsd[4].u16X = 10;//ox;
            task.pOsd[4].u16Y= scaled_height + 60;
            
            task.pOsd[5].u16X = 10;//ox;
            task.pOsd[5].u16Y= scaled_height + 10;
        
            //!special osd

            task.pOsd[6].u16X = scaled_width + 10;//ox;
            task.pOsd[6].u16Y = scaled_height + 10;

        m_vSnap_osd.clear();
    }
   // m_vFrames.clear();
    ive_clear_snap();
    task.pFrame = new ive_snap_video_frame_info_s;
    memcpy(task.pFrame, &snap_frame, sizeof(VIDEO_FRAME_INFO_S));
    DEBUG_MESSAGE("Send snap task! \n");
    m_ive_snap_thread->AddIveSnapTask(task);

    after_scale = N9M_TMGetSystemMicroSecond();
    DEBUG_ERROR("Do scale speed time:%llu \n", after_scale-befor_scale);

    return ret;  
}

int  CHiIveBle::ive_single_snap(VIDEO_FRAME_INFO_S * frame_info,int phy_chn)
{
    int ret = -1;
    if(NULL == frame_info)
    {
        return -1;
    }
    else
    {
        CHiVpss* hivpss = static_cast<CHiVpss *>(m_pData_source);
        if(0 != hivpss->HiVpss_get_frame( _VP_MAIN_STREAM_,phy_chn, frame_info))
        {
            DEBUG_ERROR("snap frame failed ! \n");
            return -1;
        }       
        ret = 0;
    }
    return ret;
}

int CHiIveBle::ive_single_snap(VIDEO_FRAME_INFO_S * frame_info, hi_snap_osd_t * snap_osd,int osd_num) 
{
    if(NULL == frame_info)
    {
        DEBUG_ERROR("the frame data is invalid! \n");
        return -1;
    }
     ive_snap_task_s task;
     memset(&task, 0, sizeof(ive_snap_task_s));

     if(NULL != snap_osd && 0 != osd_num)
    {
         task.u8Osd_num = osd_num;
         task.u8Snap_type = 1;//0x08
         task.u8Snap_subtype = 8;//vehicle
         task.pOsd = new ive_snap_osd_display_attr_s[osd_num];   
         for(int i=0; i<osd_num;++i)
         {
            task.pOsd[i].u8type = snap_osd[i].osd_type;
            task.pOsd[i].u8Index = i;
            task.pOsd[i].u16X = snap_osd[i].x;
            task.pOsd[i].u16Y = snap_osd[i].y;
            strncpy(task.pOsd[i].pContent, snap_osd[i].szStr, sizeof(task.pOsd[i].pContent));
         }
    }
     
     task.pFrame = new ive_snap_video_frame_info_s;
     memcpy(task.pFrame, frame_info, sizeof(VIDEO_FRAME_INFO_S));
     DEBUG_MESSAGE("Send snap task! \n");
     m_ive_snap_thread->AddIveSnapTask(task);

    return 0;
}

int CHiIveBle::ive_recycle_resource(ive_snap_task_s* snap_info)
{
    if(NULL != snap_info)
    {
        _AV_SAFE_DELETE_ARRAY_(snap_info->pOsd);
        if(m_eDate_source_type == _DATA_SRC_VPSS)
        {
            CHiVpss* hivpss = static_cast<CHiVpss *>(m_pData_source);
            if(0 != hivpss->HiVpss_release_frame(_VP_MAIN_STREAM_,0, snap_info->pFrame))
            {
                DEBUG_ERROR("vpss release frame failed ! \n");
                
            }
             _AV_SAFE_DELETE_(snap_info->pFrame);       
        }
    }
    return 0;
}

int CHiIveBle::ive_recycle_resource2(ive_snap_task_s* snap_info)
{
    if(NULL != snap_info)
    {
        _AV_SAFE_DELETE_ARRAY_(snap_info->pOsd);
        if(m_eDate_source_type == _DATA_SRC_VPSS)
        {
            ive_clear_snap();
             _AV_SAFE_DELETE_(snap_info->pFrame);       
        }
    }
    return 0;
}

int  CHiIveBle::ive_clear_snap()
{
    int ret = -1;
    int phy_chn = 0;
    
    CHiVpss* hivpss = static_cast<CHiVpss *>(m_pData_source);

    for(unsigned int frame_num = 0; frame_num< m_vFrames.size();frame_num++)
    {      
        if(0 != hivpss->HiVpss_release_frame(_VP_MAIN_STREAM_,phy_chn, &(m_vFrames[frame_num])))
        {
            DEBUG_ERROR("vpss release frame failed ! \n");
            ret = -1;
            continue;
        }
    }
    m_vFrames.clear();
    ret = 0;
    return ret;
}

int CHiIveBle::update_blacklist_for_cqjy()
{
    rm_uint64_t time1;
    rm_uint64_t time2;
    time1   = N9M_TMGetSystemMicroSecond();
    
    char vehicle_number[32];
    pthread_mutex_lock(&m_mutex);
    if(!m_blacklist.empty())
    {
        m_blacklist.clear();
    }
    FILE *blacklist = NULL;
    blacklist = fopen(CHONGQING_BLACKLIST, "r");
    if(NULL == blacklist)
    {
        DEBUG_ERROR("open blacklist file failed! file path:%s \n", CHONGQING_BLACKLIST);
        pthread_mutex_unlock(&m_mutex);
        return -1;
    }
    while(!feof(blacklist))
    {
        memset(vehicle_number, 0, sizeof(vehicle_number));
        fgets(vehicle_number, sizeof(vehicle_number), blacklist);
        add_vehicle_number_to_blacklist(vehicle_number, strlen(vehicle_number));
    }
    fclose(blacklist);

    pthread_mutex_unlock(&m_mutex);
    time2   = N9M_TMGetSystemMicroSecond();
    DEBUG_MESSAGE("update_blacklist_for_cqjy speed time:%llu \n", time2-time1);
    DEBUG_MESSAGE("the vehicle number is:%d  \n", m_blacklist.size());
    return 0;
}

bool CHiIveBle::is_vehicle_in_blacklist(const char vehicle_number[], unsigned char vehicle_len, char vehicle_info[], unsigned char info_len)
{
    //rm_uint64_t time1;
    //rm_uint64_t time2;
    //time1   = N9M_TMGetSystemMicroSecond();
    pthread_mutex_lock(&m_mutex);
    if(m_blacklist.empty())
    {
        pthread_mutex_unlock(&m_mutex);
        return false;
    }
    for(unsigned int i=0;i<m_blacklist.size();++i)
    {
        //only compare the first 9 bytes
        //if(0 == strncmp(m_blacklist[i].c_str(), vehicle_number, 9))
        if(vehicle_strncmp(m_blacklist[i].c_str(), vehicle_number, 9))
        {
            memset(vehicle_info, 0, info_len);
            snprintf(vehicle_info, info_len, "%s%s", vehicle_number, m_blacklist[i].substr(9,-1).c_str());
            vehicle_info[info_len-1] = '\0';
            pthread_mutex_unlock(&m_mutex);
            //time2   = N9M_TMGetSystemMicroSecond();
            //DEBUG_MESSAGE("this car is illeage,take time:%llu \n", time2-time1);
            return true;
        }
    }       
    pthread_mutex_unlock(&m_mutex);
    //time2   = N9M_TMGetSystemMicroSecond();
    //DEBUG_MESSAGE("search vehicle number speed time:%llu \n", time2-time1);

    return false;
}


bool CHiIveBle::is_vehicle_detected(const char vehicle_number[], unsigned char vehicle_len)
{
    for(unsigned int i=2; i<IVE_SUPPORT_MAX_OSD_NUMBER;++i)
    {
        if(0 == strlen(m_snap_osd[i].szStr))
        {
            return false;
        }

        if(0 == strncmp(m_snap_osd[i].szStr, vehicle_number, 9))
        {
            return true;
        }
    }

    return false;
}


int CHiIveBle::add_vehicle_number_to_blacklist(char vehicle_number[], unsigned char vehicle_len)
{
    string vc_num;
    for(int i=0; i<vehicle_len; ++i)
    {
        if(!isspace(vehicle_number[i]))
        {
            if(islower(vehicle_number[i]))
            {
                vc_num.append(1, toupper(vehicle_number[i]));
            }
            else
            {
                vc_num.append(1, vehicle_number[i]);
            }
        }
        //the vehicle number has been analyzed,following is vehicle info, we add an "_"between them
        if((vc_num.size() == 9) && (i != vehicle_len -1))
        {
            vc_num.append(1,'_');
        }
    }

    //this is only valid in china
    if(vc_num.size() >= 9 )
    {
        DEBUG_MESSAGE("the vehicle is:%s \n", vc_num.c_str());
        m_blacklist.push_back(vc_num);
    }
    
    return 0;
}

int CHiIveBle::ive_ble_process_frame(int phy_chn)
{
    int ret = 0;
    CHiVpss* hivpss = static_cast<CHiVpss *>(m_pData_source);
    VIDEO_FRAME_INFO_S frame_720p;
    VIDEO_FRAME_INFO_S  frame_540p;
    IVE_IMAGE_S image_720p;
    IVE_IMAGE_S image_540p;
    static rm_uint64_t time_record = 0;
        
    memset(&frame_720p, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&frame_540p, 0, sizeof(VIDEO_FRAME_INFO_S));
    memset(&image_720p, 0, sizeof(IVE_IMAGE_S));
    memset(&image_540p, 0, sizeof(IVE_IMAGE_S));
    
    if(0 != hivpss->HiVpss_get_frame( _VP_MAIN_STREAM_,phy_chn, &frame_720p))
    {
        DEBUG_ERROR("vpss get frame failed ! \n");
        return -1;
    }
            
    if(0 != hivpss->HiVpss_get_frame( _VP_SPEC_USE_,phy_chn, &frame_540p))
    {
        DEBUG_ERROR("vpss get frame failed ! \n");
        if(0 != hivpss->HiVpss_release_frame(_VP_MAIN_STREAM_, phy_chn, &frame_720p))
        {
            DEBUG_ERROR("vpss release frame failed ! \n");
        }
        return -1;
    }
    
    image_720p.enType = (frame_720p.stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_422)?IVE_IMAGE_TYPE_YUV422SP:IVE_IMAGE_TYPE_YUV420SP;
    for(int i=0; i<3; ++i)
    {
        image_720p.u32PhyAddr[i] = frame_720p.stVFrame.u32PhyAddr[i];
        image_720p.u16Stride[i] = (HI_U16)frame_720p.stVFrame.u32Stride[i];
        image_720p.pu8VirAddr[i]= (HI_U8*)frame_720p.stVFrame.pVirAddr[i];
    }
    
    image_720p.u16Width = (HI_U16)frame_720p.stVFrame.u32Width;
    image_720p.u16Height = (HI_U16)frame_720p.stVFrame.u32Height;

    image_540p.enType = (frame_540p.stVFrame.enPixelFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_422)?IVE_IMAGE_TYPE_YUV422SP:IVE_IMAGE_TYPE_YUV420SP;
    for(int i=0; i<3; ++i)
    {
        image_540p.u32PhyAddr[i] = frame_540p.stVFrame.u32PhyAddr[i];
        image_540p.u16Stride[i] = (HI_U16)frame_540p.stVFrame.u32Stride[i];
        image_540p.pu8VirAddr[i]= (HI_U8*)frame_540p.stVFrame.pVirAddr[i];
    }
    image_540p.u16Width = (HI_U16)frame_540p.stVFrame.u32Width;
    image_540p.u16Height = (HI_U16)frame_540p.stVFrame.u32Height;    

    do
    {
        //PlateInfo plate_infos;
        LaneEnforcementInfo plate_infos;
        memset(&plate_infos, 0, sizeof(LaneEnforcementInfo));
        if(HI_SUCCESS != BusLaneEnforcementStart(&image_720p, &image_540p, &plate_infos))
        {
            DEBUG_ERROR("IveAlgStart  failed! \n");
            BusLaneEnforcementParametersReset((char *)"./");
            ret = -1;
            break;
        }
        if(1 == pgAvDeviceInfo->Adi_get_ive_debug_mode())
        {
            if(m_pEncoder != NULL)    
            {        
                m_pEncoder->Ae_send_frame_to_encoder(_AST_SUB_VIDEO_, 0, (void *)&frame_540p, -1); 
            }
        }           

        VIDEO_FRAME_INFO_S single_frame;
        memset(&single_frame,0,sizeof(single_frame));
#if defined(_IVE_TEST_)
        plate_infos.plate_recognition_flag = EN_PLATE_UPDATE;
        plate_infos.license_plate_num = 3;
        plate_infos.snap_type = EN_SNAP;
        char vehicles[45] = {0xE6,0xB8,0x9D,0x51,0x38,0x38,0x37,0x39,0x30, \
            0xE6,0xB8,0x9D,0x41,0x39,0x38,0x30,0x39,0x43, 0xE6,0xB8,0x9D,0x43,0x30,0x34,0x36,0x35,0x32,0x0};
        memcpy(plate_infos.plate_num, vehicles, sizeof(plate_infos.plate_num));
#endif    
        switch(plate_infos.plate_recognition_flag)
        {
            case EN_PLATE_UPDATE:
            {
                int detected_vehicle_num = 0;
                int illegal_vehicle_num = 0;
                char vehicle_number[PLATE_CHAR_NUM_MAX+1];
                char vehicle_info[32]; 
                msgIPCIntelligentInfoNotice_t notice;
                bool is_detected = false;
                bool is_target_vehicle = false;
                
                memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                
                detected_vehicle_num = plate_infos.license_plate_num>PLATE_NUM_MAX? PLATE_NUM_MAX: plate_infos.license_plate_num;
                for(int idx=0; idx<detected_vehicle_num; ++idx)
                {
                    memset(vehicle_number, 0, sizeof(vehicle_number));
                    memset(vehicle_info, 0, sizeof(vehicle_info));
                    strncpy(vehicle_number, plate_infos.plate_num+idx*PLATE_CHAR_NUM_MAX, PLATE_CHAR_NUM_MAX);
                    if(is_vehicle_detected(vehicle_number, strlen(vehicle_number)))
                    {  
                        is_detected = true;
                    }
                    else
                    {
                        if(is_vehicle_in_blacklist(vehicle_number, strlen(vehicle_number), vehicle_info, sizeof(vehicle_info)))
                        {
                            is_target_vehicle = true;
                        }
                    }
                    if(is_detected || is_target_vehicle)
                    {
                        //the headcount var used to indicate which vehicle number in balcklist 
                        notice.headcount = notice.headcount | (1 << idx);
                        if(is_target_vehicle)
                        {
                            notice.reserved[1] = notice.reserved[1] | (1 << idx);
                        }
                    }

                    if(is_target_vehicle)
                    {
                        strncpy(m_snap_osd[illegal_vehicle_num+2].szStr, vehicle_number, PLATE_CHAR_NUM_MAX);
                        m_snap_osd[illegal_vehicle_num+2].osd_type = _AON_BUS_NUMBER_;
                        m_snap_osd[illegal_vehicle_num+2].szStr[PLATE_CHAR_NUM_MAX] = '\0';
                        m_snap_osd[illegal_vehicle_num+2].x = VEHICLE_OSD_SATRT_X;
                        m_snap_osd[illegal_vehicle_num+2].y = VEHICLE_OSD_START_Y + illegal_vehicle_num*OSD_VERTICAL_INTERVAL;
                        ++illegal_vehicle_num;
                        strncpy((char *)notice.datainfo.carnum[idx], vehicle_info, strlen(vehicle_info));
                    }
                    else
                    {
                        strncpy((char *)notice.datainfo.carnum[idx], plate_infos.plate_num+idx*PLATE_CHAR_NUM_MAX, PLATE_CHAR_NUM_MAX);
                    }

                    is_target_vehicle = false;
                    is_detected = false;
                }
                
                if(illegal_vehicle_num > 0)
                {
                    ive_single_snap(&single_frame,0);       
                    ive_obtain_osd(m_snap_osd,2);

                    m_snap_osd[0].osd_type = _AON_GPS_INFO_;
                    m_snap_osd[0].x = GPS_OSD_X;
                    m_snap_osd[0].y = GPS_OSD_Y;
                    m_snap_osd[1].osd_type = _AON_DATE_TIME_;
                    m_snap_osd[1].x = DATA_TIME_OSD_X;
                    m_snap_osd[1].y = DATA_TIME_OSD_Y;     
                    ive_single_snap(&single_frame, m_snap_osd, illegal_vehicle_num+2);
                }

                //every 30s clear the history vehicle numbers
                if(0 == time_record)
                {
                    time_record = N9M_TMGetSystemMicroSecond();
                }
                else
                {
                    rm_uint64_t time_now = N9M_TMGetSystemMicroSecond();
                    if((time_now - time_record) >= IVE_BLE_TIME_INT)
                    {
                        clear_history_vehicle_numbers();
                        time_record = time_now;
                    }
                }

                if(NULL != m_detection_result_notify_func)
                {
                    notice.cmdtype = 3;
                    notice.happen = 1;
                    notice.reserved[0] = 1;
                    
                    notice.happentime = N9M_TMGetShareSecond();
                    m_detection_result_notify_func(&notice);
                }
            }
                break;
           
            case EN_PLATE_EMPTY:
                //clear the sub stream osd
                if(NULL != m_detection_result_notify_func)
                {
                    msgIPCIntelligentInfoNotice_t notice;
                    memset(&notice, 0, sizeof(msgIPCIntelligentInfoNotice_t));
                    notice.cmdtype = 1;
                    notice.happen = 0;
                    notice.reserved[0] = 1;
                
                    notice.happentime = N9M_TMGetShareSecond();
                    m_detection_result_notify_func(&notice);
                }
                break;
            default:
                break;
        }
    }while(0);
    
    if(0 != hivpss->HiVpss_release_frame(_VP_MAIN_STREAM_,phy_chn, &frame_720p))
    {
        DEBUG_ERROR("vpss release 720P frame failed ! \n");
        ret = -1;
    }

    if(0 != hivpss->HiVpss_release_frame(_VP_SPEC_USE_,phy_chn, &frame_540p))
    {
        DEBUG_ERROR("vpss release 540p frame failed ! \n");
        ret = -1;
    }

    return ret;
}

bool CHiIveBle::vehicle_strncmp(const char* blacklist_vehicle, const char* detected_vehicle, unsigned int size)
{
    if(NULL == blacklist_vehicle || NULL == detected_vehicle || 0 == size)
    {
        return false;
    }
    
    for(unsigned int i=0;i<size;++i)
    {
        if((blacklist_vehicle[i] != detected_vehicle[i]) && (blacklist_vehicle[i] != '?'))
        {
            return false;
        }
        
        if(blacklist_vehicle[i] == '\n')
        {
            break;
        }
    }

    return true;
}

void CHiIveBle::clear_history_vehicle_numbers()
{
    memset(m_snap_osd, 0, sizeof(m_snap_osd));
}


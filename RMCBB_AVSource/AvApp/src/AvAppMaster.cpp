#include "AvAppMaster.h"
//#include "ConfigJsonKeyword.h"
#if defined(_AV_PRODUCT_CLASS_IPC_) || defined(_AV_PLATFORM_HI3520A_) || defined(_AV_PLATFORM_HI3521_) || defined(_AV_PLATFORM_HI3531_) || defined(_AV_PLATFORM_HI3532_) || defined(_AV_PLATFORM_HI3520D_) || defined(_AV_PLATFORM_HI3535_) || defined(_AV_PLATFORM_HI3520D_V300_)
#include "HiAvDeviceMaster.h"
#endif

#if defined(_AV_PLATFORM_HI3515_)
#include "HiAvDeviceMaster-3515.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mpi_vb.h"
#include "hi_math.h"
#endif

#include "SharePts.h"
#include "SystemConfig.h"
#include "gb2312ToUft8.h"
#include <math.h>

using namespace Common;

#if defined(_AV_PRODUCT_CLASS_IPC_)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#if defined(_AV_PLATFORM_HI3518C_)
#include "hi_i2c.h"
#endif
#endif

#if !defined(_AV_PRODUCT_CLASS_IPC_)
#include "GbkUtf8Ucs2Conv.h"
#endif

CAvAppMaster::CAvAppMaster()
{
    m_pAv_device_master = NULL;
    m_message_client_handle = NULL;
    m_start_work_flag = 0;
    m_entityReady = 0;

    m_talkback_start = false;
    m_reportstation_start = false;
    m_watchdog.interval = 30000;
    strcpy(m_watchdog.name, "avStreaming.1");

    /*system config parameters*/
    m_av_tvsystem = _AT_UNKNOWN_;
    m_pSystem_setting = NULL;
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
    m_pScreen_Margin = NULL;
    m_pScreenMosaic  = NULL;
    m_pSpot_output_mosaic = NULL;
    memset(&m_av_pip_para, 0, sizeof(av_pip_para_t));
    m_spot_output_state = _SPOT_OUTPUT_STATE_uninit_;
#endif

#if defined(_AV_HAVE_VIDEO_DECODER_)
    m_playState = PS_INVALID;
    for (int i = 0; i < _VDEC_MAX_VDP_NUM_; i++)
    {
        memset(&m_vdec_image_res[i], 0, sizeof(msgVideoResolution_t));
    }
    memset(&m_av_dec_set, 0, sizeof(av_dec_set_t));
    m_play_source_id = 0;
    m_play_streamtype_isMirror = 0;	
    m_playback_zoom_flag =0;	
    memset(&m_last_playbcak_time, 0, sizeof(datetime_t));
#endif

#if defined(_AV_HAVE_VIDEO_ENCODER_)
    m_sub_record_type = 0;
    m_pMain_stream_encoder = NULL;
    m_pSub_stream_encoder = NULL;
    m_pTime_setting = NULL;
    m_pVideo_encoder_osd = NULL;
    m_net_transfe = 0;
#if (defined(_AV_PLATFORM_HI3520D_V300_) || defined(_AV_PLATFORM_HI3515_))
	memset(&m_stuExtendOsdParam, 0x0, sizeof(m_stuExtendOsdParam));
	memset(&m_bOsdChanged, 0x0, sizeof(m_bOsdChanged));
#endif
#if !defined(_AV_PRODUCT_CLASS_IPC_)
    memset(m_ext_osd_content, 0, sizeof(m_ext_osd_content));
#endif
    memset(m_bus_number,0,sizeof(m_bus_number));  

    memset(m_record_type,0,sizeof(m_record_type));
    memset(m_record_type_bak,0,sizeof(m_record_type));
    memset(m_bus_selfsequeue_number,0,sizeof(m_bus_selfsequeue_number));
    memset(m_chn_name,0,sizeof(m_chn_name));
    m_pSub2_stream_encoder = NULL;
    memset(m_net_start_substream_enable,0,sizeof(m_net_start_substream_enable));
#endif

#if defined(_AV_HAVE_VIDEO_INPUT_)
    m_pMotion_detect_para = NULL;
    m_pVideo_shield_para = NULL;
    //m_pCover_area_para = NULL;
    //m_pInput_video_format = NULL;
    memset(m_vi_config_set, 0, 32 * sizeof(vi_config_set_t));
    m_pVi_primary_config = new vi_config_set_t[32];
    m_pVi_minor_config = new vi_config_set_t[32];
    memset(&m_alarm_status, 0, sizeof(m_alarm_status));
#endif

    memset(&m_alarm_status, 0, sizeof(Av_video_state_t));

#if defined(_AV_HAVE_VIDEO_DECODER_)
	memset( &m_stuModuleEncoderInfo, 0, sizeof(m_stuModuleEncoderInfo) );
	memset(m_remote_vl, 1, sizeof(m_remote_vl));
	memset(m_last_chn_state, 0, sizeof(m_last_chn_state));
#endif

#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_HDVR_) || defined(_AV_HAVE_VIDEO_DECODER_)
	memset(m_ChnProtocolType, 0xff, sizeof(m_ChnProtocolType));
#endif


#if defined(_AV_PRODUCT_CLASS_IPC_)
    m_pstuCameraAttrParam = NULL;
    m_pstuCameraColorParam = NULL;
    m_u8SourceFrameRate = 30;
    m_u8IPCSnapWorkMode = 0;

    for(unsigned int i=0;i<sizeof(m_osd_content)/sizeof(m_osd_content[0]);++i)
    {
        memset(m_osd_content[i], 0, sizeof(m_osd_content[i]));
    }

    for(unsigned int i=0;i<sizeof(m_ext_osd_content)/sizeof(m_ext_osd_content[0]);++i)
    {
        memset(m_ext_osd_content[i], 0, sizeof(m_ext_osd_content[i]));
    }

    m_pIpAddress = NULL;
#endif
#if defined(_AV_HAVE_AUDIO_INPUT_)
#if defined(_AV_PRODUCT_CLASS_IPC_)
    m_pAudio_param = NULL;
#endif
#endif
    m_StartPlayCmd = false;
#if !defined(_AV_PRODUCT_CLASS_IPC_)
    m_report_method = -1;
    m_ext_osd_x[0] = 500; //!< 20160614
    m_ext_osd_y[0] = 500;
    
    m_ext_osd_x[1] = 500;
    m_ext_osd_y[1] = 500;

#endif
#if defined(_AV_HAVE_VIDEO_ENCODER_)
    memset(m_chn_state, 0, sizeof(m_chn_state)) ;
#endif
    m_upgrade_flag=0;
#if defined(_AV_PLATFORM_HI3515_)
	m_nPickupStat = 0x1;	//6a一代拾音器状态,默认有效///
#endif
}


CAvAppMaster::~CAvAppMaster()
{
    Aa_stop_work();
    Common::N9M_MSGDestroyMessageClient(m_message_client_handle);

    /*system config parameters*/
    _AV_SAFE_DELETE_(m_pSystem_setting);
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
    _AV_SAFE_DELETE_(m_pScreen_Margin);
    _AV_SAFE_DELETE_(m_pScreenMosaic);
    _AV_SAFE_DELETE_(m_pSpot_output_mosaic);
#endif
#if defined(_AV_HAVE_VIDEO_INPUT_)
    _AV_SAFE_DELETE_(m_pVideo_shield_para);
    _AV_SAFE_DELETE_ARRAY_(m_pVi_primary_config);
    _AV_SAFE_DELETE_ARRAY_(m_pVi_minor_config);
    
#endif
#if defined(_AV_HAVE_VIDEO_ENCODER_)
    _AV_SAFE_DELETE_(m_pMain_stream_encoder);
    _AV_SAFE_DELETE_(m_pSub_stream_encoder);
    _AV_SAFE_DELETE_(m_pTime_setting);
    _AV_SAFE_DELETE_(m_pVideo_encoder_osd);
    _AV_SAFE_DELETE_(m_pSub2_stream_encoder);
#endif
#if defined(_AV_HAVE_AUDIO_INPUT_)
#if defined(_AV_PRODUCT_CLASS_IPC_)
    _AV_SAFE_DELETE_(m_pAudio_param);
#endif
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_)
    _AV_SAFE_DELETE_(m_pIpAddress);
#endif
}


int CAvAppMaster::Aa_init()
{
    Common::N9M_MSGEventRegister(m_message_client_handle, EVENT_SERVER_SET_DEBUGLEVEL, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SetDebugLevel));
#if defined(_AV_PRODUCT_CLASS_IPC_) || defined(_AV_PLATFORM_HI3520A_) || defined(_AV_PLATFORM_HI3521_) || defined(_AV_PLATFORM_HI3531_) || defined(_AV_PLATFORM_HI3532_) || defined(_AV_PLATFORM_HI3520D_) || defined(_AV_PLATFORM_HI3535_) ||defined(_AV_PLATFORM_HI3520D_V300_) || defined(_AV_PLATFORM_HI3515_)
    m_pAv_device_master = new CHiAvDeviceMaster;
    m_pAv_device = m_pAv_device_master;
#else
    _AV_FATAL_(1, "You must give the implement\n");
#endif

    _AV_FATAL_(m_pAv_device == NULL, "OUT OF MEMORY\n");

    return CAvApp::Aa_init();
}

int CAvAppMaster::Aa_parse_vi_info_from_system_config()
{
    int retval = -1;
    int vi_num = 0, vi_max_width = 0, vi_max_height = 0;
    int resolution = 0;

    eProductType product_type = PTD_INVALID;
    eSystemType subproduct_type = SYSTEM_TYPE_MAX;
    N9M_GetProductType(product_type);
    subproduct_type = N9M_GetSystemType();
    vi_num = N9M_GetVichnNum();
    if((product_type == PTD_X1) && ((subproduct_type == SYSTEM_TYPE_X1_N0400) || (subproduct_type == SYSTEM_TYPE_X1_V2_X1N0400)))
    {
        //vi_num = 1;
        return 0;
    }

    if(vi_num <= 0)
    {
        _AV_FATAL_(1, "Get vi chn num failed!\n");
    }

    resolution = N9M_GetVichnSize();
    switch(resolution)
    {
        case RESOLUTION_D1:
            vi_max_width = 720;
            vi_max_height = 576;
            break;
        case RESOLUTION_960H:
            vi_max_width = 960;
            vi_max_height = 576;
            break;
        case RESOLUTION_720P:
            vi_max_width = 1280;
            vi_max_height = 720;
            break;
        case RESOLUTION_1080P:
            vi_max_width = 1920;
            vi_max_height = 1080;
            break;
        default:
            _AV_FATAL_(1, "invalid vi size!\n");
            break;
    }
    pgAvDeviceInfo->Adi_set_vi_info(vi_num, vi_max_width, vi_max_height);
    retval = 0;
    
    return retval;
}

int CAvAppMaster::Aa_parse_stream_buffer_info_from_system_config()
{
    int retval = -1;
    unsigned int index;
    char buf_name [32];
    unsigned int buf_size;
    unsigned int buf_cnt;

    for(index = 0; index < SUPPORT_MAX_VIDEO_CHANNEL_NUM; index++)
    {
        if(N9M_GetStreamBufferName(BUFFER_TYPE_MAIN_STREAM, index, buf_name, buf_size, buf_cnt) == 0)
        {
            pgAvDeviceInfo->Adi_set_stream_buffer_info(BUFFER_TYPE_MAIN_STREAM, index, buf_name, buf_size, buf_cnt);
        }
        if(N9M_GetStreamBufferName(BUFFER_TYPE_MAIN_STREAM_IFRAME, index, buf_name, buf_size, buf_cnt) == 0)
        {
            pgAvDeviceInfo->Adi_set_stream_buffer_info(BUFFER_TYPE_MAIN_STREAM_IFRAME, index, buf_name, buf_size, buf_cnt);
        }
        if(N9M_GetStreamBufferName(BUFFER_TYPE_ASSIST_STREAM, index, buf_name, buf_size, buf_cnt) == 0)
        {
            pgAvDeviceInfo->Adi_set_stream_buffer_info(BUFFER_TYPE_ASSIST_STREAM, index, buf_name, buf_size, buf_cnt);
        }

        if(N9M_GetStreamBufferName(BUFFER_TYPE_SNAP_STREAM, index, buf_name, buf_size, buf_cnt) == 0)
        {
            pgAvDeviceInfo->Adi_set_stream_buffer_info(BUFFER_TYPE_SNAP_STREAM, index, buf_name, buf_size, buf_cnt);
        }

        if(N9M_GetStreamBufferName(BUFFER_TYPE_NET_TRANSFER_STREAM, index, buf_name, buf_size, buf_cnt) == 0)
        {
            pgAvDeviceInfo->Adi_set_stream_buffer_info(BUFFER_TYPE_NET_TRANSFER_STREAM, index, buf_name, buf_size, buf_cnt);
        }
    }
    if(N9M_GetStreamBufferName(BUFFER_TYPE_BACKUP_STREAM, 0, buf_name, buf_size, buf_cnt) == 0)
    {
        pgAvDeviceInfo->Adi_set_stream_buffer_info(BUFFER_TYPE_BACKUP_STREAM, 0, buf_name, buf_size, buf_cnt);
    }

    if(N9M_GetStreamBufferName(BUFFER_TYPE_TALKSET_STREAM, 0, buf_name, buf_size, buf_cnt) == 0)
    {
        pgAvDeviceInfo->Adi_set_stream_buffer_info(BUFFER_TYPE_TALKSET_STREAM, 0, buf_name, buf_size, buf_cnt);
    }

    if(N9M_GetStreamBufferName(BUFFER_TYPE_TALKGET_STREAM, 0, buf_name, buf_size, buf_cnt) == 0)
    {
        pgAvDeviceInfo->Adi_set_stream_buffer_info(BUFFER_TYPE_TALKGET_STREAM, 0, buf_name, buf_size, buf_cnt);
    }

    if(N9M_GetStreamBufferName(BUFFER_TYPE_MSSP_STREAM, 0, buf_name, buf_size, buf_cnt) == 0)
    {
        pgAvDeviceInfo->Adi_set_stream_buffer_info(BUFFER_TYPE_MSSP_STREAM, 0, buf_name, buf_size, buf_cnt);
    }
    retval = 0;
    
    return retval;
}

int CAvAppMaster::Aa_load_system_config(int argc/* = 0*/, char *argv[]/* = NULL*/)
{
    eProductType product_type = PTD_INVALID;
    eSystemType sub_product_type = SYSTEM_TYPE_MAX;
    eAppType app_type = APP_TYPE_FORMAL;

    /*device type*/
    N9M_GetProductType(product_type);
    sub_product_type = N9M_GetSystemType();
    /*app type*/
#if defined(_AV_TEST_MAINBOARD_) || defined(_AV_PRODUCT_CLASS_IPC_)
    N9M_GetAppType(app_type);
#endif
    DEBUG_MESSAGE( "CAvAppMaster::Aa_load_system_config(product_type:%d)(app_type:%d)\n", product_type, app_type);
    _AV_FATAL_(( pgAvDeviceInfo = new CAvDeviceInfo(product_type,sub_product_type,app_type)) == NULL, "OUT OF MEMORY\n");
#if defined(_AV_PRODUCT_CLASS_IPC_)
    pgAvDeviceInfo->Adi_set_framerate(m_u8SourceFrameRate);
#endif
#ifndef _AV_PRODUCT_CLASS_NVR_
    if(Aa_parse_vi_info_from_system_config() != 0)
    {
        _AV_FATAL_(1, "Get vi info failed!\n");
    }
#endif
    if(Aa_parse_stream_buffer_info_from_system_config() != 0)
    {
        _AV_FATAL_(1, "Get vi info failed!\n");
    }

    if(Aa_load_special_info() != 0)
    {
        _AV_FATAL_(1, "load special info failed!\n");
    }

    return 0;
}


int CAvAppMaster::Aa_load_special_info()
{
    int ai_num = 0, digital_chn_num = 0;
    Json::Value special_info;

    ai_num = N9M_GetAichnNum();
    if(ai_num > 0)
    {
        pgAvDeviceInfo->Adi_set_audio_chn_number(ai_num);
    }

    digital_chn_num = N9M_GetDigitalchnNum();
    if(digital_chn_num > 0)
    {
        pgAvDeviceInfo->Adi_set_digital_chn_num(digital_chn_num);
    }
#if defined(_AV_PRODUCT_CLASS_IPC_)
    if(PTD_712_VD == pgAvDeviceInfo->Adi_product_type())
    {
        eSystemType sytem_type = N9M_GetSystemType();
        if(SYSTEM_TYPE_RAILWAYIPC_MASTER == sytem_type)
        {
            pgAvDeviceInfo->Adi_set_ipc_work_mode(0);
        }
        else
        {
             pgAvDeviceInfo->Adi_set_ipc_work_mode(1);
        }
    }
#endif

    N9M_GetSpecInfo(special_info);
    if (!special_info["VLOSS_LOGO"].empty() && special_info["VLOSS_LOGO"].isString())
    {
        DEBUG_MESSAGE( "CAvAppMaster::Aa_load_special_info (VLOSS_LOGO:%s)\n", special_info["VLOSS_LOGO"].asCString());
        pgAvDeviceInfo->Adi_set_sys_pic_path(_ASPT_VIDEO_LOST_,  special_info["VLOSS_LOGO"].asString());
    }

#if defined(_AV_PRODUCT_CLASS_IPC_)
    if (!special_info["HLC_SWITCH"].empty() && special_info["HLC_SWITCH"].isString() && \
    0 == strcmp(special_info["HLC_SWITCH"].asCString(), "true"))
    {
            _AV_KEY_INFO_(_AVD_APP_, "CAvAppMaster::Aa_load_special_info (HEIGHT_LIGHT_COMPS:true)\n");
        pgAvDeviceInfo->Adi_set_height_light_comps_switch(true);   
    }

    if (!special_info["WDR_SWITCH"].empty() \
    && strcmp(special_info["WDR_SWITCH"].asCString(), "true") == 0)
    {
        _AV_KEY_INFO_(_AVD_APP_, "CAvAppMaster::Aa_load_special_info (SCKW_WDR_SWITCH:true)\n");
        pgAvDeviceInfo->Adi_set_wdr_control_switch(true);
    }

#if 0
    //add by dhdong
    if((!special_info["DEVICE_AUDIO_NUM"].empty()) && special_info["DEVICE_AUDIO_NUM"].isNumeric())
    {
        _AV_KEY_INFO_(_AVD_APP_, "CAvAppMaster::Aa_load_special_info (audio device num:%d)\n", special_info["DEVICE_AUDIO_NUM"].asInt());
        pgAvDeviceInfo->Adi_set_audio_chn_number(special_info["DEVICE_AUDIO_NUM"].asInt()); 
    }
    //add by dhdong 
    if((!special_info["IRCUT_SWITCH"].empty()) && special_info["IRCUT_SWITCH"].isString())
    {
        _AV_KEY_INFO_(_AVD_APP_, "CAvAppMaster::Aa_load_special_info (has ircut:%s)\n", special_info["IRCUT_SWITCH"].asCString());

        if(0 == strcmp(special_info["IRCUT_SWITCH"].asCString(), "true"))
        {
            pgAvDeviceInfo->Adi_set_ircut_and_infrared_flag(true);
        }
        else
        {
            pgAvDeviceInfo->Adi_set_ircut_and_infrared_flag(false);
        }
    }
#endif

    //add by dhdong for user define bitrate
    if((!special_info["USE_CUSTOM_BITRATE"].empty()) && special_info["USE_CUSTOM_BITRATE"].isString())
    {
        _AV_KEY_INFO_(_AVD_APP_, "CAvAppMaster::Aa_load_special_info (USE_CUSTOM_BITRATE:%s)\n", special_info["USE_CUSTOM_BITRATE"].asCString());
        if(0 == strcmp(special_info["USE_CUSTOM_BITRATE"].asCString(), "true"))
        {
            pgAvDeviceInfo->Adi_set_custom_bitrate(true);
        }
        else
        {
            pgAvDeviceInfo->Adi_set_custom_bitrate(false);
        }
    }  
    //get default focus len
    if((!special_info["DEF_FOCUS_LEN"].empty()) && (special_info["DEF_FOCUS_LEN"].isInt()))
    {
        pgAvDeviceInfo->Adi_set_focus_length(special_info["DEF_FOCUS_LEN"].asInt());
    }
    
    //get vi config
    if(!special_info["VI_CFG"].empty())
    {
        sensor_e sensor_type;
        for(unsigned int i=0;i<special_info["VI_CFG"].size();++i)
        {
            if(0 == strcmp("IMX238", special_info["VI_CFG"][i]["SENSOR"].asCString()))
            {
                sensor_type = ST_SENSOR_IMX238;
            }
            else if(0 == strcmp("IMX225", special_info["VI_CFG"][i]["SENSOR"].asCString()))
            {
                sensor_type = ST_SENSOR_IMX225;
            }
            else if(0 == strcmp("IMX290", special_info["VI_CFG"][i]["SENSOR"].asCString()))
            {
                sensor_type = ST_SENSOR_IMX290;
            }
            else if(0 == strcmp("IMX222", special_info["VI_CFG"][i]["SENSOR"].asCString()))
            {
                sensor_type = ST_SENSOR_IMX222;
            }
            else if(0 == strcmp("OV9732", special_info["VI_CFG"][i]["SENSOR"].asCString()))
            {
                sensor_type = ST_SENSOR_OV9732;
            }
            else 
            {
                sensor_type = ST_SENSOR_DEFAULT;
            }

            for(unsigned int j=0; j<special_info["VI_CFG"][i]["CHN_CFG"].size();++j)
            {
                for(unsigned int k=0; k<special_info["VI_CFG"][i]["CHN_CFG"][j]["RES_INFO"].size();++k)
                {
                    vi_capture_rect_s rect;
                    memset(&rect, 0, sizeof(vi_capture_rect_s));
                    rect.m_res = special_info["VI_CFG"][i]["CHN_CFG"][j]["RES_INFO"][k]["VI_OUTPUT_RES"].asInt();
                    rect.m_vi_capture_width = special_info["VI_CFG"][i]["CHN_CFG"][j]["RES_INFO"][k]["VI_CAPTURE_WIDTH"].asInt();
                    rect.m_vi_capture_height = special_info["VI_CFG"][i]["CHN_CFG"][j]["RES_INFO"][k]["VI_CAPTURE_HEIGHT"].asInt();
                    rect.m_vi_capture_x = special_info["VI_CFG"][i]["CHN_CFG"][j]["RES_INFO"][k]["VI_CAPTURE_X"].asInt();
                    rect.m_vi_capture_y = special_info["VI_CFG"][i]["CHN_CFG"][j]["RES_INFO"][k]["VI_CAPTURE_Y"].asInt();
                    pgAvDeviceInfo->Adi_set_vi_config(sensor_type, j, rect);
                }
            }
        }
    }
#endif

    if(!special_info["CUSTOMER_NAME"].empty())
    {
        _AV_KEY_INFO_(_AVD_APP_, "CAvAppMaster::Aa_load_special_info (CUSTOMER_NAME:%s)\n", special_info["CUSTOMER_NAME"].asCString());
        pgAvDeviceInfo->Adi_set_customer_name(special_info["CUSTOMER_NAME"].asCString());
    }
    if(!special_info["PRODUCT_GROUP"].empty())
    {
        _AV_KEY_INFO_(_AVD_APP_, "CAvAppMaster::Aa_load_special_info (PRODUCT_GROUP:%s)\n", special_info["PRODUCT_GROUP"].asCString());
        pgAvDeviceInfo->Adi_set_product_group(special_info["PRODUCT_GROUP"].asCString());
    }
    if(!special_info["CHECK_CRITERION_TYPE"].empty())
    {
        _AV_KEY_INFO_(_AVD_APP_, "CAvAppMaster::Aa_load_special_info (CHECK_CRITERION_TYPE:%s)\n", special_info["CHECK_CRITERION_TYPE"].asCString());
        pgAvDeviceInfo->Adi_set_check_criterion(special_info["CHECK_CRITERION_TYPE"].asCString());
    }
#if defined(_AV_HAVE_VIDEO_OUTPUT_)

    if(!special_info["FUNCTIONS"]["VIDEO_MONITOR_SPOT"].empty() && strcmp(special_info["FUNCTIONS"]["VIDEO_MONITOR_SPOT"].asCString(),"true") == 0)
    {
        _AV_KEY_INFO_(_AVD_APP_, "CAvAppMaster::Aa_load_special_info (VIDEO_MONITOR_SPOT:%s)\n", special_info["FUNCTIONS"]["VIDEO_MONITOR_SPOT"].asCString());
        Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_SCREEN_MODE_EXTRA_PARAM_GET, 1, NULL, 0, 0);
    }
#endif

#if defined(_AV_PLATFORM_HI3516A_)
        /*GET VI_VPSS mode*/
        FILE *fd = NULL;

        fd = fopen("/sys/module/hi3516a_sys/parameters/vi_vpss_online", "r");
        if(NULL == fd)
        {
            _AV_KEY_INFO_(_AVD_APP_, "open /sys/module/hi3516a_sys/parameters failed! \n");
        }
        else
        {
            int vi_vpss_mode = 0;
            if(fscanf(fd, "%d", &vi_vpss_mode) != 1)
            {
                _AV_KEY_INFO_(_AVD_APP_, "fscanf /sys/module/hi3516a_sys/parameters failed! \n");
            }
            else
            {
                pgAvDeviceInfo->Adi_set_vi_vpss_mode(vi_vpss_mode);
                _AV_KEY_INFO_(_AVD_APP_, "the vi_vpss mode is:%d \n", vi_vpss_mode);

                HI_U32 u32RegValue = 0x0;
                HI_MPI_SYS_GetReg(0x20120004, &u32RegValue);
                
                if(vi_vpss_mode)
                {
                    HI_MPI_SYS_SetReg(0x20120004, u32RegValue|0x40000000);//online: set the 30th bit to 1
                }
                else
                {
                    HI_MPI_SYS_SetReg(0x20120004, u32RegValue & 0xBFFFFFFF);
                }
            }
            fclose(fd);
        }
#endif

#if defined(_AV_PLATFORM_HI3518EV200_)
    /*GET VI_VPSS mode*/
    FILE *fd = NULL;

    fd = fopen("/sys/module/hi3518e_sys/parameters/vi_vpss_online", "r");
    if(NULL == fd)
    {
        _AV_KEY_INFO_(_AVD_APP_, "open /sys/module/hi3518e_sys/parameters failed! \n");
    }
    else
    {
        int vi_vpss_mode = 0;
        if(fscanf(fd, "%d", &vi_vpss_mode) != 1)
        {
            _AV_KEY_INFO_(_AVD_APP_, "fscanf /sys/module/hi3518e_sys/parameters failed! \n");
        }
        else
        {
            pgAvDeviceInfo->Adi_set_vi_vpss_mode(vi_vpss_mode);
            _AV_KEY_INFO_(_AVD_APP_, "the vi_vpss mode is:%d \n", vi_vpss_mode);

            HI_U32 u32RegValue = 0x0;
            HI_MPI_SYS_GetReg(0x20120004, &u32RegValue);

            if(vi_vpss_mode)
            {
                HI_MPI_SYS_SetReg(0x20120004, u32RegValue|0x40000000);//online: set the 30th bit to 1
            }
            else
            {
                HI_MPI_SYS_SetReg(0x20120004, u32RegValue & 0xBFFFFFFF);
            }
        }
        fclose(fd);
    }
#endif
    return 0;
}

int CAvAppMaster::Aa_construct_message_client()
{
    m_message_client_handle = Common::N9M_MSGCreateMessageClient(100);
    return 0;
}

void CAvAppMaster::Aa_message_handle_GETSTATUS(const char *msg, int length)
{
    SMessageHead *pMessage_head = (SMessageHead*)msg;

    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        if(m_start_work_flag != 0)
        {
            msgStatus av_status;

            av_status.ucStatus = 1;
#if defined(_AV_HAVE_VIDEO_DECODER_)
            if(m_playState != PS_INVALID)
            {
                av_status.ucStatus = 2;
            }
#endif
            Common::N9M_MSGResponseMessage(m_message_client_handle, pMessage_head, (char *)&av_status, sizeof(msgStatus));
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_GETSTATUS invalid message length\n");
    }
}


int CAvAppMaster::Aa_register_message()
{
    /**/
    //Common::N9M_MSGEventRegister(m_message_client_handle, EVENT_SERVER_SET_DEBUGLEVEL, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SetDebugLevel));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_GET_STATUS, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_GETSTATUS));
   

#if defined(_AV_HAVE_VIDEO_OUTPUT_)
    /*screen split*/
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_SCREENMOSAIC_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SCREENMOSIAC));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_SCREEN_MODE_PARAM_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SCREENMOSIAC));
    m_start_work_env[MESSAGE_CONFIG_MANAGE_SCREEN_MODE_PARAM_GET] = false;

    /*drag channel*/
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_DRAGCHN, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SWAPCHN));

    /*margin*/
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_VIDEO_OUTPUT_PARAM_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_MARGIN));
    m_start_work_env[MESSAGE_CONFIG_MANAGE_VIDEO_OUTPUT_PARAM_GET] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_VIDEO_OUTPUT_PARAM_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_MARGIN));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_SCREENMARGIN_RESET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_MARGIN));
    //for X7 spot
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_SCREEN_MODE_EXTRA_PARAM_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SPOTSEQUENCE));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_SPOT_SEQUENCE, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SPOTSEQUENCE));
    /*set screen in*/
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_SCREEN_ZOOMIN_PARAM_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_ZOOMINSCREEN));
    /*set screen pip*/
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_SCREEN_PIP_PARAM_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_PIPSCREEN));
    /*screen display adjust*/
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_SCREEN_DISPLAY_ADJUST, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SCREEN_DISPLAY_ADJUST));
    
#endif

#if defined(_AV_HAVE_VIDEO_DECODER_)
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_DECODER_STREAMSTART_PARAM_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_DECODER_STREAMSTART));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_DECODER_STREAMSTOP_PARAM_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_DECODER_STREAMSTOP));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_DECODER_STREAM_OPERATE, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_DECODER_STREAM_OPERATE));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_DECODER_SWITCH_CHANNELS, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_DECODER_SWITCH_CHANNELS));

    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_STORAGE_PLAYBACK_START | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_PLAYBACK_START));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_STORAGE_PLAYBACK_STOP | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_PLAYBACK_STOP));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_STORAGE_PLAYBACK_OPERATE | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_PLAYBACK_OPERATE));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_STORAGE_PLAYBACK_SWITCH_CHANNELS | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_PLAYBACK_SWITCH_CHANNELS));
    //回放音频启停消息
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_START_PLAYBACKAUDIO, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_START_PLAYBACK_AUDIO));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_STOP_PLAYBACKAUDIO, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_STOP_PLAYBACK_AUDIO));
    
#endif

#if defined(_AV_HAVE_VIDEO_ENCODER_)
#if !defined(_AV_PRODUCT_CLASS_DVR_REI_)
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_MAIN_STREAM_PARAM_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_MAINSTREAMPARA));
    m_start_work_env[MESSAGE_CONFIG_MANAGE_MAIN_STREAM_PARAM_GET] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_MAIN_STREAM_PARAM_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_MAINSTREAMPARA));
#else
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_MAIN_STREAM_PARAM_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_MAINSTREAMPARA_REI));
    m_start_work_env[MESSAGE_CONFIG_MANAGE_MAIN_STREAM_PARAM_GET] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_MAIN_STREAM_PARAM_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_MAINSTREAMPARA_REI));
#endif
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_ASSIST_RECORD_PARAM_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SUBSTREAMPARA));
    m_start_work_env[MESSAGE_CONFIG_MANAGE_ASSIST_RECORD_PARAM_GET] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_ASSIST_RECORD_PARAM_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SUBSTREAMPARA));    

    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_NET_TRANSFER_PARAM_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_MOBSTREAMPARA));
    m_start_work_env[MESSAGE_CONFIG_MANAGE_NET_TRANSFER_PARAM_GET] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_NET_TRANSFER_PARAM_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_MOBSTREAMPARA));
    
#ifdef _AV_PRODUCT_CLASS_IPC_
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_NET_SNAP, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_NetSnap)); 
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_CONNECT_TYPE, Common::N9M_Bind(this, &CAvAppMaster::Ae_message_handle_IPCWorkMode));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_VIDEO_OSD_OVERLAY_UPDATE , Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_update_video_osd));
#if defined(_AV_SUPPORT_IVE_)
#if defined(_AV_IVE_FD_) || defined(_AV_IVE_HC_)
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_NETWORK_CLIENT_INTELLIGENT_CTRL , Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_IveFaceDectetion));
#endif

#if defined(_AV_IVE_BLE_)
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_NETWORK_CLIENT_GET_BLACKLIST | RESPONSE_FLAG , Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_chongqing_blacklist));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_NETWORK_CLIENT_SET_BLACKLIST , Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_chongqing_blacklist));
#endif
#endif
#endif

    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_ENCODE_OSD_PARAM_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_VIDEOOSD));
    m_start_work_env[MESSAGE_CONFIG_MANAGE_ENCODE_OSD_PARAM_GET] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_ENCODE_OSD_PARAM_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_VIDEOOSD));

#if defined(_AV_PRODUCT_CLASS_IPC_)
    if(((PTD_712_VD == pgAvDeviceInfo->Adi_product_type()) && (0 == pgAvDeviceInfo->Adi_get_ipc_work_mode())) || (PTD_712_VF == pgAvDeviceInfo->Adi_product_type()))
    {
        Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_REGISTER_PARAM_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_BusInfo));
        Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_REGISTER_PARAM_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_BusInfo));
    }
#else
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_REGISTER_PARAM_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_RegisterInfo));
    m_start_work_env[MESSAGE_CONFIG_MANAGE_REGISTER_PARAM_GET] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_REGISTER_PARAM_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_RegisterInfo));
#endif

    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_VIEW_OSD_PARAM_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_ViewOsdInfo));
    m_start_work_env[MESSAGE_CONFIG_MANAGE_VIEW_OSD_PARAM_GET] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_VIEW_OSD_PARAM_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_ViewOsdInfo));

#if !defined(_AV_PRODUCT_CLASS_IPC_)
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_SPEED_ALARM_PARAM_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SpeedSource));
    m_start_work_env[MESSAGE_CONFIG_MANAGE_SPEED_ALARM_PARAM_GET] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_SPEED_ALARM_PARAM_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SpeedSource));
#endif

    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_TIME_PARAM_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_TIMESETTING));
    m_start_work_env[MESSAGE_CONFIG_MANAGE_TIME_PARAM_GET] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_TIME_PARAM_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_TIMESETTING));

    //Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_SNAP_PICTURE_TO_ENCODE, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SNAPSHOT));
    /*单次抓拍任务*/
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_SNAP_PICTURE_TASK_FROM_EVENT_TO_AV, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SIGNALSNAP));

/*add by dhdong, the deviceManager of C6 responses this message only,other products not implemets this message.*/
#if defined(_AV_PRODUCT_CLASS_IPC_)
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_MACHINE_STATISTICS_GET | Common::RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SNAPSERIALNUMBER));    
#else
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_MACHINE_STATISTICS_GET | Common::RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SNAPSERIALNUMBER));
    m_start_work_env[MESSAGE_DEVICE_MANAGER_MACHINE_STATISTICS_GET] = false;
#endif

    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_NETWORK_AUTO_SET_SUBPARAM, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_auto_adapt_encoder));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_NETWORK_CLIENT_GET_ENCODE_SOURCE, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_GET_ENCODER_SOURCE));
#if defined(_AV_PRODUCT_CLASS_IPC_)
    if(((PTD_712_VD == pgAvDeviceInfo->Adi_product_type()) && (0 == pgAvDeviceInfo->Adi_get_ipc_work_mode())) || (PTD_712_VF == pgAvDeviceInfo->Adi_product_type()))
    {
        Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_GPS_INFO_UPDATE, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_update_gpsInfo));    
    }
#else
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_GPS_INFO_UPDATE, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_update_gps));
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_)
    if((PTD_712_VD == pgAvDeviceInfo->Adi_product_type()) && (0 == pgAvDeviceInfo->Adi_get_ipc_work_mode()))
    {
        Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_VEHICLE_INFO_UPDATE, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_update_speedInfo));    
    }
#else
    
#if !defined(_AV_PRODUCT_CLASS_DVR_REI_)
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_BASIC_SERVICE_SPEED_INFO_UPDATE, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_update_speed_info)); 
#endif
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_VEHICLE_INFO_UPDATE, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_update_pulse_speed));
#endif
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_ENCODER_REQUEST_IFRAME, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_request_iframe));
   
    // 启停预览音频
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_ENCODE_START, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_start_encoder));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_ENCODE_STOP, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_stop_encoder));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_GET_NET_STREAM_LEVEL, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_get_net_stream_level));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_STORAGE_START_RECORD, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_record_mode));
	Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_STORAGE_STOP_RECORD, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_record_mode));

#if !defined(_AV_PRODUCT_CLASS_IPC_)
    //0628 获取报站站名
    //Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_REPORT_ITS_DATA, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_GetStationName));
    //0730 CommonOSD 叠加
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAMING_ENABLE_COMMON_OSD, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_Enable_Common_OSD));
    //0703 GPS从黑匣子获取数据
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_TAX2_DATA_UPDATE, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_TAX2_GPS_SPEED));
#endif

#if (defined(_AV_PLATFORM_HI3520D_V300_) ||	defined(_AV_PLATFORM_HI3515_))/*6AII_AV12机型专用osd叠加*/
    if(OSD_TEST_FLAG == pgAvDeviceInfo->Adi_product_type() || PTD_6A_I == pgAvDeviceInfo->Adi_product_type())
    {
        Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_OSD_TEMP_EXTEND_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_EXTEND_OSD));
    }
#endif

#endif

#if defined(_AV_HAVE_VIDEO_INPUT_)
#if !defined(_AV_PLATFORM_HI3518EV200_)
    /*motion detect*/
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_MD_ALARM_PARAM_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_MOTIONDETECTPARA));
    m_start_work_env[MESSAGE_CONFIG_MANAGE_MD_ALARM_PARAM_GET] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_MD_ALARM_PARAM_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_MOTIONDETECTPARA));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_MD_BLOCKSTATE_GET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_GET_MD_BLOCKSTATE));
    // Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_EVENT_GET_MD_STATUS, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_GET_VIDEO_STATUS));
#if 1
    /*video shield*/
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_VS_ALARM_PARAM_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_VIDEOSHIELDPARA));
    m_start_work_env[MESSAGE_CONFIG_MANAGE_VS_ALARM_PARAM_GET] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_VS_ALARM_PARAM_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_VIDEOSHIELDPARA));
#endif
#endif
#if 0
    /*video cover area*/
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_COVERAREA_PARAM_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_COVERPARA));
    m_start_work_env[MESSAGE_AVSTREAM_COVERAREA_PARAM_GET] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_COVERAREA_PARAM_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_COVERPARA));
#endif

#if !defined(_AV_PRODUCT_CLASS_IPC_)
    /*get vi resolution*/
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_GET_VIDEO_RESOLUTION, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_VIRESOLUTION));
#endif

   /*device videoloss state*/
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_GET_VLOSS_STATE | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_DEAL_VIDEOLOSS_STATE));
#if !(defined(_AV_PRODUCT_CLASS_NVR_) && defined(_AV_PRODUCT_CLASS_DECODER_))
    m_start_work_env[MESSAGE_DEVICE_MANAGER_GET_VLOSS_STATE] = false;
#endif
#if defined(_AV_PRODUCT_CLASS_IPC_)
    m_start_work_env[MESSAGE_DEVICE_MANAGER_GET_VLOSS_STATE] = true;
#endif
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_SET_VLOSS_STATE, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_DEAL_VIDEOLOSS_STATE));
    //Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_EVENT_GET_VL_STATUS, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_GET_VIDEO_STATUS));
        
    /*sdi video format*/
#if 0
    if(0)
    {
        Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_VIDEO_INPUT_FORMAT_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_VIDEOINPUTFORMAT));
        m_start_work_env[MESSAGE_DEVICE_VIDEO_INPUT_FORMAT_GET] = false;
        Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_VIDEO_INPUT_FORMAT_CHANGED, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_VIDEOINPUTFORMAT));  
    }
#endif
//! set analog input video attributes, such as flip,mirror
#if defined(_AV_PRODUCT_CLASS_HDVR_)
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_CAMERA_ATTRIBUTE_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SET_VIDEO_ATTR) );

    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_CAMERA_ATTRIBUTE_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SET_VIDEO_ATTR));
    m_start_work_env[MESSAGE_CONFIG_MANAGE_CAMERA_ATTRIBUTE_SET] = false;

#if defined (_AV_SUPPORT_AHD_) //!

    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_AHD_VIDEO_FORMAT_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SET_VIDEO_FORMAT) );
    m_start_work_env[MESSAGE_DEVICE_MANAGER_AHD_VIDEO_FORMAT_GET] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_AHD_VIDEO_FORMAT_CHANGED, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SET_VIDEO_FORMAT));

#endif

#endif

#endif

#if defined(_AV_HAVE_AUDIO_OUTPUT_)
    /* set audio output*/
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_ADUDIO_OUTPUT, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_AUDIO_OUTPUT));

    /*set mute*/
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_AUDIO_MUTE, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_AUDIO_MUTE));

    /*set audio volumn*/
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_AUDIO_VOLUMN_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_AUDIO_VOLUMN_SET));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_START_PREVIEWAUDIO, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_STARTPREVIEWAUDIO));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_STOP_PREVIEWAUDIO, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_STOPPREVIEWAUDIO));
     //从GUI获取是否在配置页面
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_GUI_USER_INTERFACE_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_Set_PreviewAudio_State));
#endif

    /*talkback*/
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_AUDIO_START_TALK, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_START_TALKBACK));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_AUDIO_STOP_TALK, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_STOP_TALKBACK));
 
    /*reboot the av msg*/
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_SYSTEM_REBOOT, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_REBOOT));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_UPGRADE_IN_PROCESSING, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_REBOOT));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_UPGRADE_TRANSFER_STATUS, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_REBOOT));
    Common::N9M_MSGEventRegister(m_message_client_handle, EVENT_SERVER_WATCHDOG_POWERDOWN, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_REBOOT));

    /*system time setting*/
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_SYSTEM_TIME_WILL_CHANGE, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_CHANGE_SYSTEM_TIME));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_SYSTEM_TIME_CHANGED, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_CHANGE_SYSTEM_TIME));
    
    /*system parameter*/
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_GENERAL_PARAM_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SYSTEMPARA));
    m_start_work_env[MESSAGE_CONFIG_MANAGE_GENERAL_PARAM_GET] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_GENERAL_PARAM_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_SYSTEMPARA));

#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
    //device online state
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_ONVIF_DEVICE_ONLINE_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_DeviceOnlineState));
    //m_start_work_env[MESSAGE_ONVIF_DEVICE_ONLINE_GET] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_ONVIF_DEVICE_ONLINE_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_DeviceOnlineState));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_EVENT_GET_VL_STATUS | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_RemoteDeviceVideoLoss));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_EVENT_SET_VL_STATUS, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_RemoteDeviceVideoLoss));
#endif

#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_HDVR_)
    //nvr channel map
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_REMOTE_DEVICE_NODE_PARAM_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_ChannelMap));
    m_start_work_env[MESSAGE_CONFIG_MANAGE_REMOTE_DEVICE_NODE_PARAM_GET] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_REMOTE_DEVICE_NODE_PARAM_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_ChannelMap));
#endif

#ifdef _AV_PRODUCT_CLASS_IPC_
#if defined(_AV_HAVE_ISP_)
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_CAMERA_ATTRIBUTE_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_CameraAttr) );
    m_start_work_env[MESSAGE_CONFIG_MANAGE_CAMERA_ATTRIBUTE_GET] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_CAMERA_ATTRIBUTE_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_CameraAttr) );

    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_VIDEO_IMAGE_PARAM_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_CameraColor) );
    m_start_work_env[MESSAGE_CONFIG_MANAGE_VIDEO_IMAGE_PARAM_GET] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_VIDEO_IMAGE_PARAM_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_CameraColor) );

    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_ENVIRONMENT_LUMINANCE_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_LightLeval) );
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_ENVIRONMENT_LUMINANCE_SET, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_LightLeval) );
#endif
#endif

#if !defined(_AV_PRODUCT_CLASS_IPC_)
     // 报站
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_START_REPORT_STATION, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_STARTREPORTSTATION));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_STOP_REPORT_STATION, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_STOPREPORTSTATION));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_BASIC_SERVICE_INQUIRE_AUDIO_PLAY_STATE, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_INQUIRE_AUDIO_STATE));
    //!TTS
#if defined(_AV_HAVE_TTS_)
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_START_TTS, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_StartTTS));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_STOP_TTS, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_StopTTS));
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_GUI_PHONECALL_CURRENT_STATUS, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_CallPhoneTTS));
    //Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_AVSTREAM_START_TTS,Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_OperateTTS));
#endif

#endif
#if defined(_AV_HAVE_AUDIO_INPUT_)
#ifdef _AV_PRODUCT_CLASS_IPC_
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_AUDIO_PARAM_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_AUDIO_PARAM));
    m_start_work_env[MESSAGE_CONFIG_MANAGE_AUDIO_PARAM_GET] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_AUDIO_PARAM_SET , Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_AUDIO_PARAM));    
#endif
#endif

#ifdef _AV_PRODUCT_CLASS_IPC_
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_KEY_PARAM_GET | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_MacAddress));
    m_start_work_env[MESSAGE_CONFIG_MANAGE_KEY_PARAM_GET] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_KEY_PARAM_SET , Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_MacAddress));    
#if 1
    if(Aa_is_customer_cnr())
     {
        Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_NETWORK_SERVICE_GET_ETHERNET_INFO | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Ae_message_handle_GetIpAddress));
        Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_NETWORK_SERVICE_SET_ETHERNET_INFO, Common::N9M_Bind(this, &CAvAppMaster::Ae_message_handle_GetIpAddress));
     }
#endif
#if defined(_AV_PLATFORM_HI3515_)
	Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_GET_PICKUP_STATUS, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_Get_6a1_audio_fault));
#endif

#endif

#if defined(_AV_PRODUCT_CLASS_IPC_)
//added by dhdong to make sure our software is safety
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_GET_AUTH_SERIAL | RESPONSE_FLAG, Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_AuthSerial));
    m_start_work_env[MESSAGE_DEVICE_MANAGER_GET_AUTH_SERIAL] = false;
    Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_DEVICE_MANAGER_SET_AUTH_SERIAL , Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_AuthSerial));
#endif
    //北京公交客户，osd叠加站点信息，替换gps区域///

#if !defined(_AV_PRODUCT_CLASS_IPC_)&&!defined(_AV_PRODUCT_CLASS_DVR_REI_)
    char customer_name[32] = {0};
    pgAvDeviceInfo->Adi_get_customer_name(customer_name, sizeof(customer_name));
    if(!strcmp(customer_name, "CUSTOMER_BJGJ"))
    {
		Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_BASIC_SERVICE_BEIJING_BUS_STATION_INFO , Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_Beijing_bus_station));
    }

    if(!strcmp(customer_name, "CUSTOMER_04.1129"))
    {      
		Common::N9M_MSGEventRegister(m_message_client_handle, MESSAGE_BASIC_SERVICE_SEND_APC_NUM_TO_SERVER , Common::N9M_Bind(this, &CAvAppMaster::Aa_message_handle_APC_Number));
    }
#endif

    return 0;
}


int CAvAppMaster::Aa_construct_communication_component()
{
    if(CAvApp::Aa_construct_communication_component() != 0)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_construct_communication_component (CAvApp::Aa_construct_communication_component() != 0)\n");
        return -1;
    }
    if(Aa_register_message() != 0)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_construct_communication_component (Aa_register_message() != 0)\n");        
        return -1;
    }

    /*start watch dog*/
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, EVENT_SERVER_WATCHDOG_REGISTER, 0, (char *) &m_watchdog, sizeof(SWatchdogMsg), 0);

    /*get parameters*/
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_SCREEN_MODE_PARAM_GET, 1, NULL, 0, 0);
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_GENERAL_PARAM_GET, 1, NULL, 0, 0);
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_VIDEO_OUTPUT_PARAM_GET, 1, NULL, 0, 0);
    
#if defined(_AV_HAVE_VIDEO_ENCODER_)
    msgGetVideoEncodeSource_t get_video_encode_source;
#if defined(_AV_PRODUCT_CLASS_IPC_)
    //IPC get current encoder parameters
    get_video_encode_source.encodeParaSource = 1;
#else
    get_video_encode_source.encodeParaSource = 0;
#endif
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_MAIN_STREAM_PARAM_GET, 1, (char *)&get_video_encode_source, sizeof(msgGetVideoEncodeSource_t), 0);
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_ASSIST_RECORD_PARAM_GET, 1,(char *)&get_video_encode_source, sizeof(msgGetVideoEncodeSource_t), 0);
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_NET_TRANSFER_PARAM_GET, 1, (char *)&get_video_encode_source, sizeof(msgGetVideoEncodeSource_t), 0);
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_ENCODE_OSD_PARAM_GET, 1, NULL, 0, 0);

#if defined(_AV_PRODUCT_CLASS_IPC_)
    if(((PTD_712_VD == pgAvDeviceInfo->Adi_product_type()) && (0 == pgAvDeviceInfo->Adi_get_ipc_work_mode())) || (PTD_712_VF == pgAvDeviceInfo->Adi_product_type()))
    {
        Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_REGISTER_PARAM_GET, 1, NULL, 0, 0);   
    }
#else
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_REGISTER_PARAM_GET, 1, NULL, 0, 0);        
#endif
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_VIEW_OSD_PARAM_GET, 1, NULL, 0, 0); 
#if !defined(_AV_PRODUCT_CLASS_IPC_)
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_SPEED_ALARM_PARAM_GET, 1, NULL, 0, 0);        
#endif
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_TIME_PARAM_GET, 1, NULL, 0, 0);
    
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_DEVICE_MANAGER_MACHINE_STATISTICS_GET, 1, NULL, 0, 0);	

#ifdef _AV_PRODUCT_CLASS_IPC_
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_NET_TRANSFER_PARAM_GET, 1, NULL, 0, 0);
    // Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_IPC_DAY_NIGHT_CUT_LUM_GET, 1, NULL, 0, 0);
#endif
#endif

#if defined(_AV_HAVE_VIDEO_INPUT_) 
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_MD_ALARM_PARAM_GET, 1, NULL, 0, 0);
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_VS_ALARM_PARAM_GET, 1, NULL, 0, 0);
#if 0
    if(0)
    {
        Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_DEVICE_VIDEO_INPUT_FORMAT_GET, 1, NULL, 0, 0);
    }
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_AVSTREAM_VIDEO_SHIELD_PARAM_GET, 1, NULL, 0, 0);
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_AVSTREAM_COVERAREA_PARAM_GET, 1, NULL, 0, 0);
#endif
#if defined(_AV_PRODUCT_CLASS_HDVR_)
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_CAMERA_ATTRIBUTE_GET, 1, NULL, 0, 0);
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_DEVICE_MANAGER_AHD_VIDEO_FORMAT_GET, 1, NULL, 0, 0);
#endif


#endif

#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_ONVIF_DEVICE_ONLINE_GET, 1, NULL, 0, 0);
    // Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_ONVIF_VIDEO_GET_ENCODERINFO, 1, NULL, 0, 0);
#endif

#if !(defined(_AV_PRODUCT_CLASS_NVR_) && defined(_AV_PRODUCT_CLASS_DECODER_))   
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_DEVICE_MANAGER_GET_VLOSS_STATE, 1, NULL, 0, 0);
#endif

#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_HDVR_)
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_REMOTE_DEVICE_NODE_PARAM_GET, 1, NULL, 0, 0);
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_)
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_CAMERA_ATTRIBUTE_GET, 1, NULL, 0, 0);
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_VIDEO_IMAGE_PARAM_GET, 1, NULL, 0, 0);


#if defined(_AV_SUPPORT_IVE_) && defined(_AV_IVE_BLE_)
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_NETWORK_CLIENT_GET_BLACKLIST, 1, NULL, 0, 0);
#endif
#endif

#if defined(_AV_HAVE_AUDIO_INPUT_)
#if defined(_AV_PRODUCT_CLASS_IPC_) 
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_AUDIO_PARAM_GET, 1, NULL, 0, 0);
#endif
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_) 
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_KEY_PARAM_GET, 1, NULL, 0, 0);
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_NETWORK_SERVICE_GET_ETHERNET_INFO, 1, NULL, 0, 0);
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_DEVICE_MANAGER_GET_AUTH_SERIAL, 1, NULL, 0, 0);
#endif
    return 0;
}

void CAvAppMaster::Aa_check_status()
{
#if	defined(_AV_HAVE_VIDEO_DECODER_)
#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
    if(m_playState == PS_INVALID)
    {
        Aa_check_video_image_res(VDP_PREVIEW_VDEC);
    }
#endif
#endif
    return;
}

void CAvAppMaster::Aa_check_playback_status()
{
#if defined(_AV_HAVE_VIDEO_DECODER_)
    if (m_playState != PS_INVALID)
    {
        msgPlaybackTime_t time;
        datetime_t playtime;
        int idifference_time = 0;
        if(m_pAv_device_master->Ad_get_dec_playtime(VDP_PLAYBACK_VDEC, &playtime) == true)
        {
            idifference_time = (playtime.hour*3600+playtime.minute*60+playtime.second) - (m_last_playbcak_time.hour*3600+m_last_playbcak_time.minute*60+m_last_playbcak_time.second);
            if(ABS(idifference_time) > 0)
            {
                time.hour   = playtime.hour;
                time.minute = playtime.minute;
                time.second = playtime.second;
                Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_AVSTREAM_PLAYTIME_SEND, 0, (const char *)&time, sizeof(msgPlaybackTime_t), 0);
                // DEBUG_ERROR("\n++++++m_last_playbcak_time::(hour = %d, minute = %d, second = %d)+++++++\n", m_last_playbcak_time.hour, m_last_playbcak_time.minute,m_last_playbcak_time.second);
                m_last_playbcak_time.hour = playtime.hour;
                m_last_playbcak_time.minute = playtime.minute;
                m_last_playbcak_time.second = playtime.second;
                // DEBUG_ERROR("\n+++playtime::(hour = %d, minute = %d, second = %d)+++++\n", playtime.hour, playtime.minute, playtime.second);
                Aa_check_video_image_res(VDP_PLAYBACK_VDEC);
            }
        }
    }
#endif
}

#if !defined(_AV_PLATFORM_HI3518EV200_)
void CAvAppMaster::Aa_send_av_alarm()
{
#if defined(_AV_HAVE_VIDEO_INPUT_)  
    if(m_pMotion_detect_para->ucEnable)
    {
    unsigned int motioned = 0;
    if (Aa_get_vda_result(_VDA_MD_, &motioned) != 0)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_send_av_alarm Aa_get_vda_result _VDA_MD_ ERROR!!! \n");
    }
    if (m_alarm_status.motioned != motioned)
    {
            m_alarm_status.motioned = motioned;
            Aa_send_video_status(_SV_MD_STATUS_);
        }
    }
#if defined(_AV_PRODUCT_CLASS_IPC_)
    if(m_pVideo_shield_para->ucEnable)
    {
        if(Aa_is_customer_cnr())
        {
            Aa_send_od_alarm_info_for_cnr(30);
        }
        else if(Aa_is_customer_dc())
        {
            Aa_send_od_alarm_info(30);
        }
        else
        {
            Aa_send_od_alarm_info(1);
        }
    }
#else
    if(m_pVideo_shield_para->ucEnable)
    {  
        Aa_send_analog_od_alarm_info(1);
    }

#endif
#endif
}
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_)
void CAvAppMaster::Aa_IPC_check_adjust()
{
/*  if( pgAvDeviceInfo->Adi_app_type() == APP_TYPE_TEST )
    {
        return;
    }*/
    
    static unsigned char u8LastIrCutState = 0xff;
    static unsigned char u8LastLampState = 0xff;
    //static unsigned char u8LastIspFrameRate = 30;
    static unsigned int u32LastStateUpdate = 0; // rong cuo
    //static unsigned char u8LastIris = 0xff;

    unsigned char u8IrCut;
    unsigned char u8Lamp;
    unsigned char u8Iris;
    unsigned char u8IspFrameRate;
    bool bForceSend = false;
    unsigned int u32PwmHz = 30*1000;
    if( (u32LastStateUpdate++) % 120 == 0 )
    {
        bForceSend = true;
    }
    
    if( m_pAv_device_master->Ad_GetIspControl(u8IrCut, u8Lamp, u8Iris, u8IspFrameRate, u32PwmHz) == 0 )
    {
        if( u8IrCut != u8LastIrCutState || bForceSend )
        {
            msgIrCutSwitch_t stuParam;
            memset( &stuParam, 0, sizeof(stuParam) );
            stuParam.ucIrCut = u8IrCut;

            DEBUG_MESSAGE( "CAvAppMaster::Aa_IPC_check_adjust send cmd to open/close ircut=%d\n", u8IrCut);
            if( u8IrCut )
            {
                //first
                if( u8IrCut != u8LastIrCutState )
                {
                    m_pAv_device_master->Ad_SetVideoColorGrayMode( 1 );
                }
                //second
                mSleep(100);
                Common::N9M_MSGSendMessage(m_message_client_handle,  0, MESSAGE_DEVICE_MANAGER_IR_CUT_SWITCH, 0, (const char*)&stuParam, sizeof(stuParam), 0 );
            }
            else
            {
                //first
                if( u8Lamp != u8LastLampState || bForceSend )
                {
                    bForceSend = false;
                    u8LastLampState = u8Lamp;

                    msgInfraredLightLevel_t stuLevel;
                    memset( &stuLevel, 0, sizeof(stuLevel) );
                    stuLevel.ucInfraredLevel = u8LastLampState;
                    stuLevel.u32Frequncy = u32PwmHz;

                    DEBUG_MESSAGE( "CAvAppMaster::Aa_IPC_check_adjust send cmd to set infrared level2=%d pwmHz=%d\n", u8LastLampState, stuLevel.u32Frequncy);
                    Common::N9M_MSGSendMessage(m_message_client_handle,  0, MESSAGE_DEVICE_MANAGER_INFRARED_LIGHT_CTRL, 0, (const char*)&stuLevel, sizeof(stuLevel), 0 );
                }
                //second   
                if( u8LastIrCutState != u8IrCut )
                {
                    m_pAv_device_master->Ad_SetAEAttrStep(0);
                }
                Common::N9M_MSGSendMessage(m_message_client_handle,  0, MESSAGE_DEVICE_MANAGER_IR_CUT_SWITCH, 0, (const char*)&stuParam, sizeof(stuParam), 0 );
                mSleep(900);
                //sleep(1);
                //third
                if( u8LastIrCutState != u8IrCut )
                {
                    m_pAv_device_master->Ad_SetVideoColorGrayMode( 0 );
                    m_pAv_device_master->Ad_SetAEAttrStep(1);
                }
            }

            u8LastIrCutState = u8IrCut;
        }

        //third
        if( u8Lamp != u8LastLampState || bForceSend )
        {
            u8LastLampState = u8Lamp;

            msgInfraredLightLevel_t stuLevel;
            memset( &stuLevel, 0, sizeof(stuLevel) );
            stuLevel.ucInfraredLevel = u8LastLampState;
            stuLevel.u32Frequncy = u32PwmHz;

            DEBUG_MESSAGE( "CAvAppMaster::Aa_IPC_check_adjust send cmd to set infrared level=%d pwmHz=%d\n", u8LastLampState, stuLevel.u32Frequncy);
            Common::N9M_MSGSendMessage(m_message_client_handle,  0, MESSAGE_DEVICE_MANAGER_INFRARED_LIGHT_CTRL, 0, (const char*)&stuLevel, sizeof(stuLevel), 0 );
        }

        //if( u8IspFrameRate != u8LastIspFrameRate )

#if 0
        printf("[alex]the u8IspFrameRate is:%d m_u8SourceFrameRate:%d \n", u8IspFrameRate, m_u8SourceFrameRate);
        if( (u8IspFrameRate != m_u8SourceFrameRate) && (0 != u8IspFrameRate))//! modified
        {
            DEBUG_MESSAGE("CAvAppMaster::Aa_IPC_check_adjust ISP change frame rate:%d->%d\n", m_u8SourceFrameRate, u8IspFrameRate);
            //u8LastIspFrameRate = u8IspFrameRate;
            m_u8SourceFrameRate = u8IspFrameRate;
            for(int chn=0;chn<pgAvDeviceInfo->Adi_max_channel_number();++chn)
            {
                m_pAv_device->Ad_notify_framerate_changed(chn, m_u8SourceFrameRate);
            }
            this->Aa_adapt_encoder_param( ((1<<_AST_MAIN_VIDEO_) | (1<<_AST_SUB_VIDEO_) | (1<<_AST_SUB2_VIDEO_)), 0x1, -1, 0xff, 0xff, m_u8SourceFrameRate);
        }
#endif
    }

    if( bForceSend )
    {
        Common::N9M_MSGSendMessage(m_message_client_handle,  0, MESSAGE_DEVICE_MANAGER_ENVIRONMENT_LUMINANCE_GET, true, NULL, 0, 0 );
    }
}


//! Added for 712C6_VA, Nov 20, 2014.
/*!
*\brief Set framerate by ISP and update encoder and osd params.
*/
#if defined(_AV_PLATFORM_HI3518C_)
int CAvAppMaster::Aa_set_isp_framerate(av_tvsystem_t norm, int main_stream_framerate)
{
    if(NULL == m_pAv_device_master)
    {
        _AV_KEY_INFO_(_AVD_APP_, "the m_pAv_device_master is NULL \n");
        return -1;
    }
    

    if(0 == m_start_work_flag)
    {
        _AV_KEY_INFO_(_AVD_APP_, "the m_start_work_flag is 0 \n");
        return -1;
    }
    
    CHiAvDeviceMaster  *pHi_av_device_master = NULL;
    pHi_av_device_master = dynamic_cast<CHiAvDeviceMaster  *>(m_pAv_device_master);

    if(NULL == pHi_av_device_master)
    {
        _AV_KEY_INFO_(_AVD_APP_, "the m_pAv_device_master may be not initialized!");
        return -1;
    }

   unsigned char frameRate = 30;
    if((_AT_PAL_ == norm)&& (25 == main_stream_framerate))
    {
        frameRate = 25;
    }
    else
   {
        frameRate = 30;
   }

    if(frameRate == m_u8SourceFrameRate)
    {
        return 0;
    }
    else
    {
        pHi_av_device_master->Ad_SetIspFrameRate(frameRate);
        m_u8SourceFrameRate = frameRate;
        pgAvDeviceInfo->Adi_set_framerate(m_u8SourceFrameRate);
    }
    
    //after changed the isp frameRate,we must syncorize the streams param,otherise the stream framerate will be not exact.
     for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
     {
         m_pAv_device->Ad_notify_framerate_changed(chn, m_u8SourceFrameRate);
         if (m_main_encoder_state.test(chn))
         {
            av_video_encoder_para_t video_encoder_para;
            memset(&video_encoder_para, 0, sizeof(av_video_encoder_para_t));
            Aa_get_encoder_para(_AST_MAIN_VIDEO_, chn, &video_encoder_para, -1, NULL);
            m_pAv_device->Ad_modify_encoder(_AST_MAIN_VIDEO_, chn, &video_encoder_para, false);
         }
         
         /*update sub stream osd*/
         if (m_sub_encoder_state.test(chn))
         {
             av_video_encoder_para_t video_encoder_para;
             memset(&video_encoder_para, 0, sizeof(av_video_encoder_para_t));
             Aa_get_encoder_para(_AST_SUB_VIDEO_, chn, &video_encoder_para, -1, NULL);
             m_pAv_device->Ad_modify_encoder(_AST_SUB_VIDEO_, chn, &video_encoder_para, false);                                        
         }

#ifdef _AV_PRODUCT_CLASS_IPC_
         /*update sub2 stream osd*/
         if (m_sub2_encoder_state.test(chn))
         {
             av_video_encoder_para_t video_encoder_para;
             memset(&video_encoder_para, 0, sizeof(av_video_encoder_para_t));
             Aa_get_encoder_para(_AST_SUB2_VIDEO_, chn, &video_encoder_para, -1, NULL);
             m_pAv_device->Ad_modify_encoder(_AST_SUB2_VIDEO_, chn, &video_encoder_para, false);                                        
         }                                   
#endif

     }
    
    return 0;
}
#endif
#if defined(_AV_PLATFORM_HI3516A_)
bool CAvAppMaster::Aa_is_reset_isp(unsigned char resolution, unsigned char framerate, IspOutputMode_e &isp_output_mode)
{
    /*
    ISP_OUTPUT_MODE_5M30FPS,
    ISP_OUTPUT_MODE_3M30FPS,   
    ISP_OUTPUT_MODE_1080P60FPS,
    ISP_OUTPUT_MODE_1080P30FPS,
    ISP_OUTPUT_MODE_720P120FPS,
    ISP_OUTPUT_MODE_720P60FPS,
     */
    //int expected_resolution= -1;//0:720p 1:1080p 2:3m 3:5m
    //int curr_isp_resolution = -1;
    //int curr_isp_framerate = -1;
    IspOutputMode_e output_mode = ISP_OUTPUT_MODE_INVALID;
#if 0
    switch(resolution)
    {
        case 6://720P
        {
             //expected_resolution = 0;
             framerate = framerate>120?120:framerate;
             if(framerate > 60)
             {
                output_mode = ISP_OUTPUT_MODE_720P120FPS;
             }
             else
            {
                output_mode = ISP_OUTPUT_MODE_720P60FPS;
            }
        }
            break;
        case 7://1080P
        {
            //expected_resolution = 1;
            framerate = framerate>60?60:framerate;
            if(framerate > 30)
            {
                output_mode = ISP_OUTPUT_MODE_1080P60FPS;
            }
            else
            {
                output_mode = ISP_OUTPUT_MODE_1080P30FPS;
            }
        }
            break;
        case 9://5M
        {
            //expected_resolution = 3;
            framerate = framerate>30?30:framerate;
            output_mode = ISP_OUTPUT_MODE_5M30FPS;
        }
            break;
        case 8://3//3M
        {
            //expected_resolution = 2;
            framerate = framerate>30?30:framerate;
            output_mode = ISP_OUTPUT_MODE_3M30FPS;
        }
            break;
        default:
            DEBUG_MESSAGE("the resolution is not need changed isp output mode! \n", resolution);
            return false;
            break;
    }
#else
    output_mode = Aa_calculate_isp_out_put_mode(resolution, framerate);
#endif

    IspOutputMode_e curr_isp_output_mode;
    RM_ISP_API_GetOutputMode(curr_isp_output_mode);
#if 0
    switch(curr_isp_output_mode)
    {
        case ISP_OUTPUT_MODE_5M30FPS:
        default:
        {
            curr_isp_resolution = 3;
            curr_isp_framerate = 30;
        }
            break;
        case ISP_OUTPUT_MODE_3M30FPS:
        {
            curr_isp_resolution = 2;
            curr_isp_framerate = 30;
        }
            break;
        case ISP_OUTPUT_MODE_1080P60FPS:
        {
            curr_isp_resolution = 1;
            curr_isp_framerate = 60;
        }
            break;
        case ISP_OUTPUT_MODE_720P120FPS:
        {
            curr_isp_resolution = 0;
            curr_isp_framerate = 120;
        }
            break;
       case ISP_OUTPUT_MODE_720P60FPS:
        {
            curr_isp_resolution = 0;
            curr_isp_framerate = 60;            
        }
            break;
        case ISP_OUTPUT_MODE_1080P30FPS:
        {
            curr_isp_resolution = 1;
            curr_isp_framerate = 30;            
        }
            break;            
    }

    if((expected_resolution>curr_isp_resolution) || (framerate>curr_isp_framerate))
#else    
    if((curr_isp_output_mode != output_mode) && (output_mode != ISP_OUTPUT_MODE_INVALID))
#endif
    {
        isp_output_mode = output_mode;
        return true;
    }
    
    return false;
    
}
int CAvAppMaster::Aa_reset_isp(IspOutputMode_e isp_output_mode)
{
    /*first set output mode*/
    //pgAvDeviceInfo->Adi_set_isp_output_mode(isp_output_mode);
    if (m_start_work_flag != 0)
   {
        /*secnod isp exit*/
        RM_ISP_API_IspExit();
        mSleep(100);
        /*third API init*/
        ISPInitParam_t isp_param;

        memset(&isp_param, 0, sizeof(ISPInitParam_t));
        isp_param.sensor_type = pgAvDeviceInfo->Adi_get_sensor_type();
        isp_param.focus_len = pgAvDeviceInfo->Adi_get_focus_length();
        isp_param.u8WDR_Mode = WDR_MODE_NONE;
        isp_param.etOptResolution = isp_output_mode;
        isp_param.u8FstInit_flag = 1;

        RM_ISP_API_Init(&isp_param);

        /*fourth restart  sensor*/
        RM_ISP_API_InitSensor();
        /*fifth restart isp*/
        RM_ISP_API_IspStart();

        mSleep(100);
        m_pAv_device_master->Ad_SetCameraColor( &(m_pstuCameraColorParam->stuVideoImageSetting[0]) );
        m_pAv_device_master->Ad_SetCameraAttribute( 0xffffffff, &(m_pstuCameraAttrParam->stuViParam[0].image) );
   }
    return 0;
}
#endif

IspOutputMode_e CAvAppMaster::Aa_calculate_isp_out_put_mode(unsigned char resolution, unsigned char framerate)
{
    switch(resolution)
    {
     //3516A  not support 720P right now
#ifndef _AV_PLATFORM_HI3516A_
        case 6://720P
        default:
        {
             resolution = 0;
             if(framerate > 60)
             {
                return ISP_OUTPUT_MODE_720P120FPS;
             }
             else if(framerate > 30)
            {
                return ISP_OUTPUT_MODE_720P60FPS;
            }
             else
            {
                return ISP_OUTPUT_MODE_720P30FPS;
            }
        }
            break;
       case 14:
       case 17://1024*768
       {
            if(framerate > 30)
            {
                return ISP_OUTPUT_MODE_960P60FPS;
            }
            else
            {
                return ISP_OUTPUT_MODE_960P30FPS;
            }
        }
            break;       
#else
        case 6:
        case 14:
        default:
#endif
        case 7://1080P
        {
            resolution = 1;
            if(framerate > 30)
            {
                return ISP_OUTPUT_MODE_1080P60FPS;
            }
            else
            {
                return ISP_OUTPUT_MODE_1080P30FPS;
            }
        }
            break;
        case 9://5M
        {
            resolution = 3;
            return ISP_OUTPUT_MODE_5M30FPS;
        }
            break;
        case 8://3//3M
        {
            resolution = 2;
            return ISP_OUTPUT_MODE_3M30FPS;
        }
            break;
    }
}

unsigned short CAvAppMaster::Aa_convert_framerate_from_IOM(IspOutputMode_e isp_output_mode)
{
    unsigned short framerate = 0;
    switch(isp_output_mode)
    {
        case ISP_OUTPUT_MODE_5M30FPS:
        case ISP_OUTPUT_MODE_1080P30FPS:
        case ISP_OUTPUT_MODE_3M30FPS:
        default:
            framerate =  30;
            break;
        case ISP_OUTPUT_MODE_1080P60FPS:
        case ISP_OUTPUT_MODE_720P60FPS:
            framerate = 60;
            break;
        case ISP_OUTPUT_MODE_720P120FPS:
            framerate = 120;
            break;
    }

    return framerate;
}

#endif // end #if defined(_AV_PRODUCT_CLASS_IPC_)

#if defined(_AV_HAVE_VIDEO_DECODER_)
void CAvAppMaster::Aa_check_video_image_res(int purpose)
{
    bool sendFlag = false;
    if (VDP_PLAYBACK_VDEC == purpose)
    {
        m_vdec_image_res[purpose].u16Status = 1;
    }
    else
    {
        m_vdec_image_res[purpose].u16Status = 0;
    }

#if defined(_AV_HAVE_VIDEO_DECODER_) //&& defined(_AV_HAVE_VIDEO_INPUT_)
    if(m_entityReady & AVSTREAM_VIDEODECODER_START)
    {
        m_pAv_device_master->Ad_dynamic_modify_vdec_framerate(purpose);
    }
#endif

    for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
    {
        int res = -1;
        int width = 0, height = 0;
        int ret = 0;

#if defined(_AV_HAVE_VIDEO_DECODER_) && (defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_))
        ret = m_pAv_device_master->Ad_get_dec_image_size(purpose, chn, width, height);  
#elif defined(_AV_HAVE_VIDEO_DECODER_) && defined(_AV_HAVE_VIDEO_INPUT_)        
#if defined(_AV_PRODUCT_CLASS_HDVR_) 
        if ((VDP_PLAYBACK_VDEC == purpose) || (pgAvDeviceInfo->Adi_get_video_source(chn) == _VS_DIGITAL_))
#else
        if (VDP_PLAYBACK_VDEC == purpose)
#endif
        {
            ret = m_pAv_device_master->Ad_get_dec_image_size(purpose, chn, width, height);
        }
        else
        {
            ret = 0;
            width = m_vi_config_set[chn].w;
            height = m_vi_config_set[chn].h;
            /*add new*/
            av_resolution_t av_res = m_vi_config_set[chn].resolution;
            uint8_t u32ResValue;
            
            pgAvDeviceInfo->Adi_get_video_resolution((uint8_t *)&u32ResValue, &av_res, false);
            switch(u32ResValue)
            {
                case 0:/*CIF*/
                case 1:/*HD1*/    
                case 2:/*D1*/
                case 3:/*QCIF*/
                    width = pgAvDeviceInfo->Adi_get_D1_width();
                    height = (m_av_tvsystem == _AT_PAL_)?576:480;
                    break;
                case 6:/*720p*/
                    width = 1280;
                    height = 720;
                    break;
                case 7:/*1080p*/
                    width = 1920;
                    height = 1080;
                    break;
                case 10:/*WQCIF*/
                case 11:/*WCIF*/
                case 12:/*WHD1*/
                case 13:/*WD1*/
                    width = pgAvDeviceInfo->Adi_get_WD1_width();
                    height = (m_av_tvsystem == _AT_PAL_)?576:480;
                    break;
                case 14:
                    width = 1280;
                    height = 960;
                    break;
                default:
                    DEBUG_ERROR("@@@@CAvAppMaster::Aa_check_video_image_res invalid resolution(%d) return now\n", u32ResValue);
                    return;
                    break;
            }
            /*end*/
        }       
#endif
        if (ret >= 0)
        {
            res = Get_Resvalue_By_Size(width, height);

            if (!GCL_BIT_VAL_TEST(m_vdec_image_res[purpose].u32ChnMask, chn))
            {
                GCL_BIT_VAL_SET(m_vdec_image_res[purpose].u32ChnMask, chn);
                sendFlag = true;
            }
            else if (m_vdec_image_res[purpose].stuVideoRes[chn].u32ResValue != (unsigned int)res)
            {
                sendFlag = true;
            }

            if (true == sendFlag)
            {
                m_vdec_image_res[purpose].stuVideoRes[chn].u32ResValue = res;
                m_vdec_image_res[purpose].stuVideoRes[chn].u32Width = width;
                m_vdec_image_res[purpose].stuVideoRes[chn].u32Height = height;
                //DEBUG_ERROR( "CAvAppMaster::Aa_check_video_image_res chn %d res %d, w %d, h %d\n", chn, res, width, height);
            }
        }
        else if (GCL_BIT_VAL_TEST(m_vdec_image_res[purpose].u32ChnMask, chn))
        {
            GCL_BIT_VAL_CLEAR(m_vdec_image_res[purpose].u32ChnMask, chn);
            sendFlag = true;
        }
    }

    if (true == sendFlag)
    {
        Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_AVSTREAM_SET_VIDEO_RESOLUTION, 0, (char *)&m_vdec_image_res[purpose], sizeof(msgVideoResolution_t), 0);
    }

    return;
}
#endif

#if defined(_AV_HAVE_VIDEO_OUTPUT_)
int CAvAppMaster::Aa_start_output(av_output_type_t output, int index, av_tvsystem_t tv_system, av_vag_hdmi_resolution_t resolution)
{
    if(m_pAv_device_master->Ad_start_output(output, index, tv_system, resolution) != 0)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_start_output m_pAv_device_master->Ad_start_output FAILED\n");
        return -1;
    }

    if(m_pAv_device_master->Ad_start_video_layer(output, index) != 0)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_start_output m_pAv_device_master->Ad_start_output FAILED\n");
        return -1;
    }

    return 0;
}


int CAvAppMaster::Aa_stop_output(av_output_type_t output, int index)
{
    if(m_pAv_device_master->Ad_stop_video_layer(output, index) != 0)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_stop_output m_pAv_device_master->Ad_stop_video_layer FAILED\n");
        return -1;
    }

    if(m_pAv_device_master->Ad_stop_output(output, index) != 0)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_stop_output m_pAv_device_master->Ad_stop_output FAILED\n");
        return -1;
    }

    return 0;
}

av_split_t CAvAppMaster::Aa_get_split_mode(int iMode)
{
    av_split_t av_mode = _AS_UNKNOWN_;
    switch(iMode)
    {
        case 0:
            av_mode = _AS_SINGLE_;
            break;
        case 1:
            av_mode = _AS_4SPLIT_;
            break;
        case 2:
            av_mode = _AS_9SPLIT_;
            break;
        case 3:
            av_mode = _AS_16SPLIT_;
            break;
        case 4:
            av_mode = _AS_25SPLIT_;
            break;
        case 5:
            av_mode = _AS_36SPLIT_;
            break;
        case 6:
            av_mode = _AS_6SPLIT_;
            break;
        case 7:
            av_mode = _AS_8SPLIT_;
            break;
        case 8:
            av_mode = _AS_13SPLIT_;
            break;
        case 9:
            av_mode = _AS_10SPLIT_;
            break;
        case 10:
            av_mode = _AS_PIP_;
            break;
        case 11:
            av_mode = _AS_2SPLIT_;
            break;
        case 12:
            av_mode = _AS_3SPLIT_;
            break;			
        case 13:
            av_mode = _AS_12SPLIT_;
	     break;
        default:
            break;
    }

    return av_mode;
}

int CAvAppMaster::Ad_get_split_iMode(av_split_t slipmode)
{
    int imode = 0xff;
    switch(slipmode)
    {
        case _AS_SINGLE_:
            imode = 0;
            break;
        case _AS_4SPLIT_:
            imode = 1;
            break;
        case _AS_9SPLIT_:
            imode = 2;
            break;
        case _AS_16SPLIT_:
            imode = 3;
            break;
        case _AS_25SPLIT_:
            imode = 4;
            break;
        case _AS_36SPLIT_:
            imode = 5;
            break;
        case _AS_6SPLIT_:
            imode = 6;
            break;
        case _AS_8SPLIT_:
            imode = 7;
            break;
        case _AS_13SPLIT_:
            imode = 8;
            break;
        case _AS_10SPLIT_:
            imode = 9;
            break;
        case _AS_PIP_:
            imode = 10;
            break;
        case _AS_2SPLIT_:
            imode = 11;
            break;	
        case _AS_3SPLIT_:
            imode = 12;
            break;				
        default:
            imode = 0xff;
            break;
    }

    return imode;
}


av_vag_hdmi_resolution_t CAvAppMaster::Aa_get_vga_hdmi_resolution(int resolution)
{
    switch(resolution)
    {
        case RT_800x600_60:
            return _AVHR_800_600_60_;
        case RT_1024x768_60:
            return _AVHR_1024_768_60_;
        case RT_1280x1024_60:
            return _AVHR_1280_1024_60_;
        case RT_1366x768_60:
            return _AVHR_1366_768_60_;
        case RT_1440x900_60:
            return _AVHR_1440_900_60_;
        case RT_720P60:
            return _AVHR_720p60_;
        case RT_1080I60:
            return _AVHR_1080i60_;
        case RT_1080P60:
            return _AVHR_1080p60_;
        case RT_480P60:
            return _AVHR_480p60_;
        case RT_576P60:
            return _AVHR_576p50_;
        default:
            return _AVHR_UNKNOWN_;
    }

    return _AVHR_UNKNOWN_;
}

int CAvAppMaster::Aa_switch_output(msgScreenMosaic_t *pScreen_mosaic, bool bswitch_channel)
{
    int chn_layout[_AV_SPLIT_MAX_CHN_];
    int chnnum = pgAvDeviceInfo->Adi_get_split_chn_info(Aa_get_split_mode(pScreen_mosaic->iMode));
    chnnum = (chnnum > 32) ? 32 : chnnum;
   
    memset(chn_layout, -1, sizeof(int) * _AV_SPLIT_MAX_CHN_);
    for(int index = 0; index < chnnum; index ++)
    {
        if((pScreen_mosaic->chn[index] >= 0) && (pScreen_mosaic->chn[index] < _AV_SPLIT_MAX_CHN_))
        {
            chn_layout[index] = pScreen_mosaic->chn[index];
            DEBUG_MESSAGE("cxliu,chn_layout[%d] = %d\n",index, chn_layout[index]);
        }
    }
    

    switch(pScreen_mosaic->iFlag)/*0:预览输出 1:解码输出 2:Spot输出*/
    {
        case 0:
        case 1:
            m_pAv_device_master->Ad_switch_output(pgAvDeviceInfo->Adi_get_main_out_mode(), 0, Aa_get_split_mode(pScreen_mosaic->iMode), chn_layout, bswitch_channel);
            break;

        case 2:
            m_pAv_device_master->Ad_switch_output(_AOT_SPOT_, 0, Aa_get_split_mode(pScreen_mosaic->iMode), chn_layout, bswitch_channel);
            break;

        default:
            return -1;
            break;
    }

    // if current state is decoder audio output and it's single change the audio output channel
    // the preview audio channel change while recive message MESSAGE_DEVICE_ADUDIO_OUTPUT
    //if(pScreen_mosaic->iFlag == 0)
    {
        if (_AS_SINGLE_ == Aa_get_split_mode(pScreen_mosaic->iMode))
        {/*if change channel to single show, the audio must to be changed*/
            
            m_pAv_device_master->Ad_switch_audio_chn( pScreen_mosaic->iFlag, pScreen_mosaic->chn[0]);
        }
    }

    return 0;
}

int CAvAppMaster::Aa_GotoPreviewOutput(msgScreenMosaic_t* pScreen_mosaic)
{
    int chn_layout[_AV_SPLIT_MAX_CHN_];
    int margin[4];
    int max_chn_num = pgAvDeviceInfo->Adi_max_channel_number();
    av_split_t split_mode = Aa_get_split_mode(pScreen_mosaic->iMode);
   
    if (split_mode == _AS_UNKNOWN_)
    {
        DEBUG_ERROR( " CAvAppMaster::Aa_GotoPreviewOutput FAILED! (split_mode:%d)\n", split_mode);
        return -1;
    }

    if (max_chn_num > 32)
    {
        max_chn_num = 32;
    }

    memset(chn_layout, -1, _AV_SPLIT_MAX_CHN_*sizeof(int));
    DEBUG_MESSAGE( "CAvAppMaster::Aa_GotoPreviewOutput:");
    for(int index = 0; index < max_chn_num; index ++)
    {
        if((pScreen_mosaic->chn[index] >= 0) && (pScreen_mosaic->chn[index] < _AV_SPLIT_MAX_CHN_))
        {
            chn_layout[index] = pScreen_mosaic->chn[index];
            DEBUG_MESSAGE("%d ", chn_layout[index]);
        }
    }
    DEBUG_MESSAGE("\n\n");
    
    margin[_AV_MARGIN_BOTTOM_INDEX_] = m_pScreen_Margin->usBottom;
    margin[_AV_MARGIN_LEFT_INDEX_] = m_pScreen_Margin->usLeft;
    margin[_AV_MARGIN_RIGHT_INDEX_] = m_pScreen_Margin->usRight;
    margin[_AV_MARGIN_TOP_INDEX_] = m_pScreen_Margin->usTop;

    /*start preview*/
    if(m_pAv_device_master->Ad_start_preview_output(pgAvDeviceInfo->Adi_get_main_out_mode(), 0, split_mode, chn_layout, margin) != 0)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_GotoPreviewOutput m_pAv_device_master->Ad_start_preview_output FAIELD\n");
        return -1;
    }
    
    
    return 0;
}

int CAvAppMaster::Aa_GotoSelectedPreviewOutput(msgScreenMosaic_t* pScreen_mosaic, unsigned long long int chn_mask, bool brelate_decoder)
{
    int chn_layout[_AV_SPLIT_MAX_CHN_];
    int margin[4];
    int max_chn_num = pgAvDeviceInfo->Adi_max_channel_number();
    av_split_t split_mode = Aa_get_split_mode(pScreen_mosaic->iMode);
   
    if (split_mode == _AS_UNKNOWN_)
    {
        DEBUG_ERROR( " CAvAppMaster::Aa_GotoSelectedPreviewOutput FAILED! (split_mode:%d)\n", split_mode);
        return -1;
    }

    if (max_chn_num > 32)
    {
        max_chn_num = 32;
    }

    memset(chn_layout, -1, _AV_SPLIT_MAX_CHN_*sizeof(int));
    DEBUG_MESSAGE( "CAvAppMaster::Aa_GotoSelectedPreviewOutput:");
    for(int index = 0; index < max_chn_num; index ++)
    {
        if((pScreen_mosaic->chn[index] >= 0) && (pScreen_mosaic->chn[index] < _AV_SPLIT_MAX_CHN_))
        {
            chn_layout[index] = pScreen_mosaic->chn[index];
            DEBUG_MESSAGE("%d ", chn_layout[index]);
        }
    }
    DEBUG_MESSAGE("\n\n");
    
    margin[_AV_MARGIN_BOTTOM_INDEX_] = m_pScreen_Margin->usBottom;
    margin[_AV_MARGIN_LEFT_INDEX_] = m_pScreen_Margin->usLeft;
    margin[_AV_MARGIN_RIGHT_INDEX_] = m_pScreen_Margin->usRight;
    margin[_AV_MARGIN_TOP_INDEX_] = m_pScreen_Margin->usTop;

    /*start preview*/
    if(m_pAv_device_master->Ad_start_selected_preview_output(pgAvDeviceInfo->Adi_get_main_out_mode(), 0, split_mode, chn_layout, chn_mask, brelate_decoder, margin) != 0)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_GotoSelectedPreviewOutput m_pAv_device_master->Ad_start_preview_output FAIELD\n");
        return -1;
    }
    
    
    return 0;

}

int CAvAppMaster::Aa_Change_VoResolution_TvSystem( unsigned char u8TvSys, unsigned char u8VoRes )
{
    if( !m_pSystem_setting )
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_Change_VoResolution_TvSystem a thing not happen\n");
        return -1;
    }

    if( u8TvSys == m_pSystem_setting->stuGeneralSetting.ucVideoStandard && u8VoRes == m_pSystem_setting->stuGeneralSetting.ucVgaResolution)
    {
        return 0;
    }

    DEBUG_ERROR( "CAvAppMaster::Aa_Change_VoResolution_TvSystem change res=%d sys=%d m_av_tvsystem:%d\n", u8VoRes, u8TvSys ,m_av_tvsystem);

    m_av_tvsystem = u8TvSys ? _AT_NTSC_ : _AT_PAL_;
	pgAvDeviceInfo->Adi_set_system(m_av_tvsystem);
    av_vag_hdmi_resolution_t res = Aa_get_vga_hdmi_resolution( u8VoRes );
    
    if (_AS_PIP_ == Aa_get_split_mode(m_pScreenMosaic->iMode))
    {
        av_pip_para_t av_pip_para;
        memset(&av_pip_para, 0, sizeof(av_pip_para));
        av_pip_para.enable = false;
        m_pAv_device_master->Ad_set_pip_screen(av_pip_para);
    }
    m_pAv_device_master->Ad_stop_preview_output(pgAvDeviceInfo->Adi_get_main_out_mode(), 0);

    this->Aa_stop_output( pgAvDeviceInfo->Adi_get_main_out_mode(), 0 );
    this->Aa_start_output( pgAvDeviceInfo->Adi_get_main_out_mode(), 0, m_av_tvsystem, res );

    this->Aa_GotoPreviewOutput(m_pScreenMosaic);
    
    if( u8VoRes != m_pSystem_setting->stuGeneralSetting.ucVgaResolution)
    {
        msgVideoImageSize_t stuSize;

        stuSize.height = 768;
        stuSize.width = 1024;
        switch( u8VoRes )
        {
            case 0://800*600
                stuSize.width = 800;
                stuSize.height = 600;
                break;
            case 1://1024*768
                stuSize.width = 1024;
                stuSize.height = 768;
                break;
            case 2://1280*1024
                stuSize.width = 1280;
                stuSize.height = 1024;
                break;
            case 3://1366*768
                stuSize.width = 1366;
                stuSize.height = 768;
                break;
            case 4://1440*900
                stuSize.width = 1440;
                stuSize.height = 900;
                break;
            case 5://720P
                stuSize.width = 1280;
                stuSize.height = 720;
                break;
            case 6:
            case 7:
                stuSize.width = 1920;
                stuSize.height = 1080;
                break;
            default:
                DEBUG_ERROR("\n\nNot supported resolution:%d\n\n", u8VoRes);
                return -1;
                break;
        }
        //ui need this meesage to recalc position
        Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_AVSTREAM_SET_VIDEO_RESOLUTION, 0, (char *)&stuSize, sizeof(stuSize), 0);
    }
    m_pSystem_setting->stuGeneralSetting.ucVgaResolution = u8VoRes;

    return 0;
}

void CAvAppMaster::Aa_message_handle_ZOOMINSCREEN(const char *msg, int length)
{
    DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_ZOOMINSCREEN\n");
    
    SMessageHead *head = (SMessageHead*) msg;
    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        msgScreenZoon_t *option = (msgScreenZoon_t *) head->body;
        av_zoomin_para_t av_zoomin_para;    
        stMessageResponse response;

        response.result = SUCCESS;
        if(m_start_work_flag != 0)
        {
            memset(&av_zoomin_para, 0, sizeof(av_zoomin_para_t));
            if(option->ucEnable)
            {  
                av_zoomin_para.enable = true;
            }
            else
            {
                av_zoomin_para.enable = false;
            }

            av_zoomin_para.videophychn = option->ucChn;
            
            av_zoomin_para.stuMainPipArea.x = option->stuArea[2].x;
            av_zoomin_para.stuMainPipArea.y = option->stuArea[2].y;
            av_zoomin_para.stuMainPipArea.w = option->stuArea[2].w;
            av_zoomin_para.stuMainPipArea.h = option->stuArea[2].h;
            
            av_zoomin_para.stuSlavePipArea.x = option->stuArea[1].x;
            av_zoomin_para.stuSlavePipArea.y = option->stuArea[1].y;
            av_zoomin_para.stuSlavePipArea.w = option->stuArea[1].w;
            av_zoomin_para.stuSlavePipArea.h = option->stuArea[1].h;

            av_zoomin_para.stuZoomArea.x = option->stuArea[0].x;
            av_zoomin_para.stuZoomArea.y = option->stuArea[0].y;
            av_zoomin_para.stuZoomArea.w = option->stuArea[0].w;
            av_zoomin_para.stuZoomArea.h = option->stuArea[0].h;

            if (m_pAv_device_master->Ad_zoomin_screen(av_zoomin_para) < 0)
            {
                response.result = -1;
                DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_ZOOMINSCREEN failed\n");
            }
            else
            {
                response.result = SUCCESS;
            }
        }
        else
        {
             response.result = -1;
            DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_ZOOMINSCREEN not start work\n");
        }
        Common::N9M_MSGResponseMessage(m_message_client_handle, head, (char *)&response, sizeof(response));
    }
}

void CAvAppMaster::Aa_message_handle_PIPSCREEN(const char *msg, int length)
{
    DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_PIPSCREEN\n");
    
    SMessageHead *head = (SMessageHead*) msg;
    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        msgScreenPipSet_t *option = (msgScreenPipSet_t *) head->body;
        stMessageResponse response;
        memset(&m_av_pip_para, 0, sizeof(av_pip_para_t));

        response.result = SUCCESS;
        if(m_start_work_flag != 0)
        {
            if(option->ucEnable)
            {  
                m_av_pip_para.enable = true;
            }
            else
            {
                m_av_pip_para.enable = false;
            }

            m_av_pip_para.video_phychn[0] = option->ucChn[0] - 1;
            m_av_pip_para.video_phychn[1] = option->ucChn[1] - 1;

            DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_PIPSCREEN, main chn %d, sub chn %d\n", option->ucChn[0], option->ucChn[1]);

            m_av_pip_para.stuSlavePipArea.x = option->stuArea[0].x;
            m_av_pip_para.stuSlavePipArea.y = option->stuArea[0].y;
            m_av_pip_para.stuSlavePipArea.w = option->stuArea[0].w;
            m_av_pip_para.stuSlavePipArea.h = option->stuArea[0].h;

            m_av_pip_para.stuMainPipArea.x = option->stuArea[1].x;
            m_av_pip_para.stuMainPipArea.y = option->stuArea[1].y;
            m_av_pip_para.stuMainPipArea.w = option->stuArea[1].w;
            m_av_pip_para.stuMainPipArea.h = option->stuArea[1].h;


            if (m_pAv_device_master->Ad_set_pip_screen(m_av_pip_para) < 0)
            {
                response.result = -1;
                DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_PIPSCREEN failed\n");
            }
            else
            {
                response.result = SUCCESS;
            }
        }
        else
        {
            response.result = -1;
            DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_PIPSCREEN not start work\n");
        }
        if (m_pScreenMosaic)
        {
            m_pScreenMosaic->iFlag = 0;
            m_pScreenMosaic->iMode = Ad_get_split_iMode(_AS_PIP_);
            memset(m_pScreenMosaic->chn, -1, sizeof(m_pScreenMosaic->chn));
            m_pScreenMosaic->chn[0] = option->ucChn[0];
            m_pScreenMosaic->chn[1] = option->ucChn[1];
        }
        Common::N9M_MSGResponseMessage(m_message_client_handle, head, (char *)&response, sizeof(response));
    }
}

void CAvAppMaster::Aa_message_handle_SCREEN_DISPLAY_ADJUST(const char *msg, int length)
{
    SMessageHead *pMessage_head = (SMessageHead*)msg;

    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgScreenDisplayAdjust_t *pScreen_adjust = (msgScreenDisplayAdjust_t *)pMessage_head->body;
        pgAvDeviceInfo->Adi_screen_display_is_adjust(pScreen_adjust->bEnable);
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_SCREEN_DISPLAY_ADJUST invalid message length\n");
    }
}
void CAvAppMaster::Aa_message_handle_SCREENMOSIAC(const char *msg, int length)
{
#if defined(_AV_PRODUCT_CLASS_DVR_REI_)
    static int screen_chn = 0;
#endif
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgScreenMosaic_t ScreenMosaic;
        memset(&ScreenMosaic, 0, sizeof(msgScreenMosaic_t));
         m_start_work_env[MESSAGE_CONFIG_MANAGE_SCREEN_MODE_PARAM_GET] = true;
         /*deal with screen switch twice when reload avStreaming because one functin deal with two register messages*/
         if(pMessage_head->event == (MESSAGE_CONFIG_MANAGE_SCREEN_MODE_PARAM_GET|RESPONSE_FLAG))
         {
            if(m_pScreenMosaic != NULL)
            {
                DEBUG_ERROR("param inited already!\n");
                return;
            }
            msgScreenModeSetting_t *pScreen_Mode = (msgScreenModeSetting_t *)pMessage_head->body;
            ScreenMosaic.iFlag = 0;
            ScreenMosaic.iMode= pScreen_Mode->stuScreenModeSetting.eScreenMode;
            memcpy(ScreenMosaic.chn, pScreen_Mode->stuScreenModeSetting.szChannel, sizeof(ScreenMosaic.chn));    
         }
         else //gui send message
         {
            msgScreenMosaic_t *pScreen_mosaic = (msgScreenMosaic_t *)pMessage_head->body;
            ScreenMosaic.iFlag = pScreen_mosaic->iFlag;
            ScreenMosaic.iMode = pScreen_mosaic->iMode;
            memcpy(ScreenMosaic.chn, pScreen_mosaic->chn, sizeof(ScreenMosaic.chn));   
	     	pgAvDeviceInfo->Aa_set_screenmode(pScreen_mosaic->iMode);
         }
         DEBUG_CRITICAL( "Aa_message_handle_SCREENMOSIAC iFlag =%d iMode =%d,chn[%d,%d,%d,%d,%d,%d,%d,%d,%d] \n",
		 	ScreenMosaic.iFlag,ScreenMosaic.iMode,ScreenMosaic.chn[0],ScreenMosaic.chn[1],ScreenMosaic.chn[2],ScreenMosaic.chn[3],
		 	ScreenMosaic.chn[4],ScreenMosaic.chn[5],ScreenMosaic.chn[6],ScreenMosaic.chn[7],ScreenMosaic.chn[8]);
	  if((ScreenMosaic.iMode!=0)&&(ScreenMosaic.chn[0]==0)&&(ScreenMosaic.chn[1]==0))
	  {
		printf("\npScreen_mosaic send error\n");
		ScreenMosaic.chn[0]=0;
		ScreenMosaic.chn[1]=1;
		ScreenMosaic.chn[2]=2;
		ScreenMosaic.chn[3]=3;
		ScreenMosaic.chn[4]=4;
		ScreenMosaic.chn[5]=5;
		ScreenMosaic.chn[6]=6;
		ScreenMosaic.chn[7]=7;		
	  }
         if(m_pScreenMosaic != NULL)
         {
            if((memcmp(m_pScreenMosaic, &ScreenMosaic, sizeof(msgScreenMosaic_t)) != 0) || (ScreenMosaic.iFlag == 1)|| (ScreenMosaic.iFlag == 0))
            {
                //DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_SCREENMOSIAC...(Changed.)\n");
                memcpy(m_pScreenMosaic, &ScreenMosaic, sizeof(msgScreenMosaic_t));
                if(m_start_work_flag != 0)
                {
                    Aa_switch_output(m_pScreenMosaic);
#if defined(_AV_PRODUCT_CLASS_DVR_REI_)

		    if(m_play_streamtype_isMirror==1)
		    {
		    		printf("\nmirror rec,return \n");
				return;
		    }
				
                    //! for rei playback switch 
                    if (( m_playState != PS_INVALID ))  //&&( m_playState != PS_INIT)
                    {
                        
                        if (m_pScreenMosaic->iMode == 0)
                    	{
                            DEBUG_CRITICAL("\nStart Zoom large.......\n");
							m_playback_zoom_flag =1;
                            unsigned  int vi_mask_play = 0;
                            screen_chn = m_pScreenMosaic->chn[0];
                            GCL_BIT_VAL_SET(vi_mask_play, m_av_dec_set.chnlist[screen_chn]);	                

                    		datetime_t mydatetime={0};
                    		m_pAv_device_master->Ad_get_dec_playtime(VDP_PLAYBACK_VDEC,&mydatetime);                            

                    		msgPlaybackSwitchStream_t msg_para;
                    		msg_para.u32GroupId=0;
                    //		msg_para.ucChn=chn_layout[0];
                    		msg_para.uiChnMask=vi_mask_play;
                    		msg_para.ucStreamType=RECORD_MODE_MAIN;
                    		msg_para.year=mydatetime.year;
                    		msg_para.month=mydatetime.month;
                    		msg_para.day=mydatetime.day;
                    		msg_para.hour=mydatetime.hour;
                    		msg_para.minute=mydatetime.minute;
                    		msg_para.second=mydatetime.second;

                            N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_STORAGE_PLAYBACK_SWITCH_STREAM, 1, (const char *)&msg_para, sizeof(msg_para), 0);			

                    		DEBUG_WARNING("\n001,av sendmessage,%d-uiChnMask[%d]-%d;%d-%d-%d-%d-%d-%d\n",msg_para.u32GroupId,msg_para.uiChnMask,
                    		msg_para.ucStreamType,msg_para.year,msg_para.month,msg_para.day,msg_para.hour,msg_para.minute,msg_para.second);
                            
                    	}
                    	else
                    	{
                    		if(m_playback_zoom_flag==1)
	                    	{
	                            DEBUG_CRITICAL("\nStop Zoom large.......\n");
	                            unsigned int vi_mask_playback = 0;
	                            GCL_BIT_VAL_SET(vi_mask_playback, m_av_dec_set.chnlist[screen_chn]);	              
	                    		   
	                    		datetime_t mydatetime={0};
	                    		m_pAv_device_master->Ad_get_dec_playtime(VDP_PLAYBACK_VDEC,&mydatetime);

	                    		msgPlaybackSwitchStream_t msg_para;
	                    		msg_para.u32GroupId=0;
	                    //		msg_para.ucChn=chn_layout[0];
	                    		msg_para.uiChnMask=vi_mask_playback;
	                    		msg_para.ucStreamType=RECORD_MODE_SUB;
	                    		msg_para.year=mydatetime.year;
	                    		msg_para.month=mydatetime.month;
	                    		msg_para.day=mydatetime.day;
	                    		msg_para.hour=mydatetime.hour;
	                    		msg_para.minute=mydatetime.minute;
	                    		msg_para.second=mydatetime.second;
	                    		
	                            N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_STORAGE_PLAYBACK_SWITCH_STREAM, 1, (const char *)&msg_para, sizeof(msg_para), 0);			

	                    		DEBUG_WARNING("\n002,av sendmessage,%d-uiChnMask[%d]-%d;%d-%d-%d-%d-%d-%d\n",msg_para.u32GroupId,msg_para.uiChnMask,
	                    			msg_para.ucStreamType,msg_para.year,msg_para.month,msg_para.day,msg_para.hour,msg_para.minute,msg_para.second);
								m_playback_zoom_flag=0;
				}
                    	}
                    }
#endif
                }
            }
         }
         else
         {
            _AV_FATAL_((m_pScreenMosaic = new msgScreenMosaic_t) == NULL, "CAvAppMaster::Aa_message_handle_SCREENMOSIAC OUT OF MEMORY\n");
            memcpy(m_pScreenMosaic, &ScreenMosaic, sizeof(msgScreenMosaic_t));  
         }       
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_SCREEN_MOSIAC invalid message length\n");
    }
}
void CAvAppMaster::Aa_message_handle_SWAPCHN(const char *msg, int length)
{
    SMessageHead *pMessage_head = (SMessageHead*)msg;

    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgSwapChn_t *pSwap_chn = (msgSwapChn_t *)pMessage_head->body;

        if(m_start_work_flag != 0)
        {
            DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_SWAPCHN(%d)(%d)\n", pSwap_chn->uiChn1, pSwap_chn->uiChn2);
            int ret_val = m_pAv_device_master->Ad_output_swap_chn(pgAvDeviceInfo->Adi_get_main_out_mode(), 0, pSwap_chn->uiChn1, pSwap_chn->uiChn2);
            if (ret_val >= 0)
            {
                int chn_layout[_AV_SPLIT_MAX_CHN_];
                int chnnum = pgAvDeviceInfo->Adi_get_split_chn_info(Aa_get_split_mode(m_pScreenMosaic->iMode));
                int first_index = -1, second_index = -1;
           
                if (chnnum > 32)
                {
                    chnnum = 32;
                }    
                memset(chn_layout, -1, sizeof(int)*_AV_SPLIT_MAX_CHN_);
                for(int index = 0; index < chnnum; index ++)
                {
                    if((m_pScreenMosaic->chn[index] > 0) && (m_pScreenMosaic->chn[index] < _AV_SPLIT_MAX_CHN_))
                    {
                        chn_layout[index] = m_pScreenMosaic->chn[index];
                        if (m_pScreenMosaic->chn[index] == (pSwap_chn->uiChn1+1))
                        {
                            first_index = index;
                        }

                        if (m_pScreenMosaic->chn[index] == (pSwap_chn->uiChn2+1))
                        {
                            second_index = index;
                        }
                    }
                }

                if ((first_index >= 0) && (second_index >= 0))
                {
                    m_pScreenMosaic->chn[second_index] = chn_layout[first_index];
                    m_pScreenMosaic->chn[first_index] = chn_layout[second_index];
                }
            }
        }        
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_SWAPCHN invalid message length\n");
    }
}

void CAvAppMaster::Aa_message_handle_START_SWAPCHN( const char *msg, int length )
{
    DEBUG_MESSAGE("########################################start swapch\n");
}

void CAvAppMaster::Aa_message_handle_STOP_SWAPCHN( const char *msg, int length )
{
    DEBUG_MESSAGE("########################################stop swapch\n");
    m_pAv_device_master->Ad_output_swap_chn_reset( pgAvDeviceInfo->Adi_get_main_out_mode(), 0 );
}

void CAvAppMaster::Aa_message_handle_SPOTSEQUENCE(const char *msg, int length)
{
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_SPOTSEQUENCE \n");

    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgScreenMosaic_t ScreenMosaic;
        memset(&ScreenMosaic, 0, sizeof(msgScreenMosaic_t));

        if(pMessage_head->event == (MESSAGE_CONFIG_MANAGE_SCREEN_MODE_EXTRA_PARAM_GET|RESPONSE_FLAG))
        {
            msgScreenModeSetting_t *pScreen_Mode = (msgScreenModeSetting_t *)pMessage_head->body;
            ScreenMosaic.iFlag = 2;
            ScreenMosaic.iMode= pScreen_Mode->stuScreenModeSetting.eScreenMode;
            memcpy(ScreenMosaic.chn, pScreen_Mode->stuScreenModeSetting.szChannel, sizeof(ScreenMosaic.chn));  
            N9M_MSGEventUnRegister(m_message_client_handle, MESSAGE_CONFIG_MANAGE_SCREEN_MODE_EXTRA_PARAM_GET | RESPONSE_FLAG); 
        }
        else //gui send message
        {
            msgScreenMosaic_t *pScreen_mosaic = (msgScreenMosaic_t *)pMessage_head->body;
            ScreenMosaic.iFlag = pScreen_mosaic->iFlag;
            ScreenMosaic.iMode = pScreen_mosaic->iMode;
            memcpy(ScreenMosaic.chn, pScreen_mosaic->chn, sizeof(ScreenMosaic.chn));   
        }
        DEBUG_CRITICAL( "Aa_message_handle_SCREEN_MODE_EXTRA iFlag =%d iMode =%d \n",ScreenMosaic.iFlag,ScreenMosaic.iMode);
        
        if(m_pSpot_output_mosaic != NULL)
        {
            if (memcmp(m_pSpot_output_mosaic, &ScreenMosaic, sizeof(msgScreenMosaic_t)) != 0)
            {
                memcpy(m_pSpot_output_mosaic, &ScreenMosaic, sizeof(msgScreenMosaic_t));
                if(m_start_work_flag != 0)
                {
                    Aa_spot_output_control(_SPOT_OUTPUT_CONTROL_output_, m_pSpot_output_mosaic);
                }
            }
         
        }
        else            
        {
            _AV_FATAL_((m_pSpot_output_mosaic = new msgScreenMosaic_t) == NULL, "CAvAppMaster::Aa_message_handle_SPOTSEQUENCE OUT OF MEMORY\n");
            memcpy(m_pSpot_output_mosaic, &ScreenMosaic, sizeof(msgScreenMosaic_t));
        }
        

    }
    else
    {
        DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_SPOTSEQUENCE invalid message length\n");
    }
}

void CAvAppMaster::Aa_message_handle_MARGIN(const char *msg, int length)
{
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    //Json::Value special_info;
    //N9M_GetSpecInfo(special_info);
    char customer_name[32] = {0};
    pgAvDeviceInfo->Adi_get_customer_name(customer_name, sizeof(customer_name));
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgVideoOutputSetting_t *pParam = (msgVideoOutputSetting_t *)pMessage_head->body;
        paramScreenMargin_t ScreenMargin = pParam->stuVideoOutputSetting[0].stuScreenMargin;
        m_start_work_env[MESSAGE_CONFIG_MANAGE_VIDEO_OUTPUT_PARAM_GET] = true;
         /*
    	 //if (!special_info["CUSTOMER_NAME"].empty() && special_info["CUSTOMER_NAME"].isString())
    	 {
    		if(strcmp(special_info["CUSTOMER_NAME"].asCString(),"CUSTOMER_BJGJ") == 0)
    		{
    			ScreenMargin.usBottom = 19;
    			ScreenMargin.usTop = 19;
    			ScreenMargin.usRight = 30;
    			ScreenMargin.usLeft = 30;
    		}	
    	 } 
    	 */
    	 
		if(strcmp(customer_name,"CUSTOMER_BJGJ") == 0)
		{
			ScreenMargin.usBottom = 19;
			ScreenMargin.usTop = 19;
			ScreenMargin.usRight = 30;
			ScreenMargin.usLeft = 30;
		}	
    	 
        DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_MARGIN event 0x%x\n\n", pMessage_head->event);

        if(m_pScreen_Margin != NULL)
        {
            if (memcmp(&ScreenMargin, m_pScreen_Margin, sizeof(paramScreenMargin_t)) != 0)
            {
#if defined(_AV_HAVE_VIDEO_OUTPUT_)                 
                if(m_start_work_flag != 0)
                {
                    m_pAv_device_master->Ad_set_vo_margin( pgAvDeviceInfo->Adi_get_main_out_mode(), 0, ScreenMargin.usLeft, ScreenMargin.usRight, ScreenMargin.usTop, ScreenMargin.usBottom, false);
                }
#endif
                memcpy(m_pScreen_Margin, &ScreenMargin, sizeof(paramScreenMargin_t));
            }            
        }
        else
        {
            _AV_FATAL_((m_pScreen_Margin = new paramScreenMargin_t) == NULL, "CAvAppMaster::Aa_message_handle_MARGIN OUT OF MEMORY\n");
            memcpy(m_pScreen_Margin, &ScreenMargin, sizeof(paramScreenMargin_t));            
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_MARGIN invalid message length\n");
    }
}

int CAvAppMaster::Aa_start_spot(av_split_t split_mode, int chn_layout[_AV_SPLIT_MAX_CHN_])
{
    if(Aa_start_output(_AOT_SPOT_, 0, m_av_tvsystem, Aa_get_vga_hdmi_resolution(m_pSystem_setting->stuGeneralSetting.ucVgaResolution)) != 0)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_start_spot Aa_start_output FAIELD\n");
        return -1;
    }

    if(m_pAv_device_master->Ad_start_preview_output(_AOT_SPOT_, 0, split_mode, chn_layout, NULL) != 0)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_start_spot m_pAv_device_master->Ad_start_preview_output FAIELD\n");
        return -1;
    }

    return 0;
}


int CAvAppMaster::Aa_stop_spot()
{
    if(m_pAv_device_master->Ad_stop_preview_output(_AOT_SPOT_, 0) != 0)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_stop_spot m_pAv_device_master->Aa_stop_spot FAIELD\n");
        return -1;
    }

    if(Aa_stop_output(_AOT_SPOT_, 0) != 0)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_stop_spot Aa_stop_output FAIELD\n");
        return -1;
    }

    return 0;
}


int CAvAppMaster::Aa_spot_output_control(int control, msgScreenMosaic_t *pScreen_mosaic)
{    
    if (NULL == pScreen_mosaic)
    {
         DEBUG_MESSAGE( "CAvAppMaster::Aa_spot_output_control pScreen_mosaic is null\n");
        return 0;
    }
    DEBUG_MESSAGE( "CAvAppMaster::Aa_spot_output_control\n");

    if(pgAvDeviceInfo->Adi_have_spot_function() == 0)
    {
        av_split_t split_mode = Aa_get_split_mode(pScreen_mosaic->iMode);
        int max_chn_num = pgAvDeviceInfo->Adi_get_split_chn_info(split_mode);
        int chn_layout[_AV_SPLIT_MAX_CHN_];

        if (max_chn_num > 32)
        {
            max_chn_num = 32;
        }
        memset(chn_layout, -1, sizeof(int)*_AV_SPLIT_MAX_CHN_);
        for(int index = 0; index < max_chn_num; index ++)
        {
            if((pScreen_mosaic->chn[index] >= 0) && (pScreen_mosaic->chn[index] < _AV_SPLIT_MAX_CHN_))
            {
                chn_layout[index] = pScreen_mosaic->chn[index];
                //printf("----chn_layout[%d] = %d pScreen_mosaic->chn[%d] =%d \n", index, chn_layout[index],index,pScreen_mosaic->chn[index]);              
#if defined(_AV_PRODUCT_CLASS_HDVR_)
                if(pgAvDeviceInfo->Adi_get_video_source(chn_layout[index]) == _VS_DIGITAL_)
                {
                     //1102 支持数字通道spot
                    //return 0;
                }
#endif
            }
        }

        switch(control)
        {
            case _SPOT_OUTPUT_CONTROL_output_:
                switch(m_spot_output_state)
                {
                    case _SPOT_OUTPUT_STATE_uninit_:
                        Aa_start_spot(split_mode, chn_layout);
                        m_spot_output_state = _SPOT_OUTPUT_STATE_running_;
                        break;

                    case _SPOT_OUTPUT_STATE_running_:
                         m_pAv_device_master->Ad_switch_output(_AOT_SPOT_, 0, split_mode, chn_layout, false);
                        break;

                    default:
                        break;
                }
                break;

            case _SPOT_OUTPUT_CONTROL_stop_:
                switch(m_spot_output_state)
                {
                    case _SPOT_OUTPUT_STATE_running_:
                        Aa_stop_spot();
                        m_spot_output_state = _SPOT_OUTPUT_STATE_uninit_;
                        return 0;
                        break;

                    default:
                        break;
                }
                break;

            default:
                break;
        }
    }

    return 0;
}
#endif


#if defined(_AV_HAVE_VIDEO_DECODER_)
void CAvAppMaster::Aa_message_handle_PLAYBACK_START(const char *msg, int length)
{
    DEBUG_MESSAGE("\n\n Aa_message_handle_PLAYBACK_START \n\n");
    SMessageHead *head = (SMessageHead*) msg;
    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        stMessageResponse ResponseGui;
        int ret = 0;

        if(m_StartPlayCmd == false)
        {
            DEBUG_MESSAGE( "Playback has stopped!\n");
            return;
        }
        if(m_entityReady & AVSTREAM_VIDEODECODER_START)
        {
            DEBUG_MESSAGE( "Has started decoding!\n");
            return;
        }

        m_entityReady |= AVSTREAM_VIDEODECODER_START;

#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_) || defined(_AV_PRODUCT_CLASS_DVR_)
        if( true ) // for ui is in playing page, while av in view page
#else
        msgStartPlaybackAck_t *response = (msgStartPlaybackAck_t *) head->body;
        if( response->iResult == 0)
#endif
        {
            pgAvDeviceInfo->Adi_backup_cur_preview_res();
            ResponseGui.result = SUCCESS; 
            ret = m_pAv_device_master->Ad_start_playback_dec(&m_av_dec_set);      
            if(ret >= 0)
            {
                m_playState = PS_NORMAL;
            }
            else
            {
                DEBUG_MESSAGE("StartDecoder err, ret %d\n", ret);
                msgStopPlayback_t option;
                option.iGroupId = 0;
                Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_STORAGE_PLAYBACK_STOP, 1, (const char *)&option, sizeof(msgStopPlayback_t), head->session);
                           
                ResponseGui.result = -1;
            }
        }
        else 
        {
            ResponseGui.result = -1;
        }

        DEBUG_MESSAGE("\n\n[Aa_message_handle_PLAYBACK_START]Video Decoder Send ResponseGui.result %d\n\n", ResponseGui.result);
        Common::N9M_MSGSendMessage(m_message_client_handle, head->source, MESSAGE_AVSTREAM_DECODER_STREAMSTART_PARAM_SET | RESPONSE_FLAG, 0, (char *)&ResponseGui, sizeof(stMessageResponse), m_play_source_id);
    }
    else
    {
        DEBUG_MESSAGE("\n\n Aa_message_handle_PLAYBACK_START message err:head->length:%d length:%d\n\n", head->length, length);
    }
}

void CAvAppMaster::Aa_message_handle_PLAYBACK_STOP(const char *msg, int length)
{
    DEBUG_MESSAGE("\n\n Aa_message_handle_PLAYBACK_STOP\n\n");
    SMessageHead *head = (SMessageHead*) msg;
    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        //m_entityReady &= (~AVSTREAM_VIDEODECODER_START);
    }
    else
    {
        DEBUG_MESSAGE("\n\n Aa_message_handle_PLAYBACK_STOP message err:head->length:%d length:%d\n\n", head->length, length);
    }
}

void CAvAppMaster::Aa_message_handle_PLAYBACK_OPERATE(const char *msg, int length)
{
    DEBUG_MESSAGE("\n\n Aa_message_handle_PLAYBACK_OPERATE \n\n");
    SMessageHead *head = (SMessageHead*) msg;
    stMessageResponse ResponseGui;

    if (!(m_entityReady & AVSTREAM_VIDEODECODER_START))
    {
        DEBUG_ERROR("\n\n[Aa_message_handle_PLAYBACK_OPERATE]Video Decoder not started....\n\n");
        return;
    }
    if (m_playState == PS_INVALID)
    {
        DEBUG_ERROR("\n\n[OnPlaybackOperationPost]Video Decoder has stopped....\n\n");
        return;
    }
    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        msgPlaybackOperateAck_t *response = (msgPlaybackOperateAck_t *) head->body;
        if(response->iResult == 0)
        {
            ResponseGui.result = SUCCESS;
            if (m_pAv_device_master)
            {
                DEBUG_WARNING("[Aa_message_handle_PLAYBACK_OPERATE]============================true, iOperate %d\n", response->iOperate);
                m_pAv_device_master->Ad_operate_dec(VDP_PLAYBACK_VDEC, response->iOperate, true);
            }
        }
        else
        {
            ResponseGui.result = -1;
        }
        DEBUG_WARNING("\n\n[Aa_message_handle_PLAYBACK_OPERATE]ResponseToGUI:result:%d opcode:%d\n\n", ResponseGui.result, response->iOperate);
        Common::N9M_MSGSendMessage(m_message_client_handle, head->source, MESSAGE_AVSTREAM_DECODER_STREAM_OPERATE | RESPONSE_FLAG, 0, (char *)&ResponseGui, sizeof(stMessageResponse), m_play_source_id);
        m_playState = PS_NORMAL;
    }
    else
    {
        DEBUG_MESSAGE("\n\n Aa_message_handle_PLAYBACK_OPERATE message err:head->length:%d length:%d\n\n", head->length, length);
    }
}

void CAvAppMaster::Aa_message_handle_PLAYBACK_SWITCH_CHANNELS(const char *msg, int length)
{
    DEBUG_CRITICAL("\n\n Aa_message_handle_PLAYBACK_SWITCH_CHANNELS \n\n");
    
    SMessageHead *head = (SMessageHead*) msg;
    int ret_val = 0;

    if (!(m_entityReady & AVSTREAM_VIDEODECODER_START))
    {
        DEBUG_ERROR("\n\n[Aa_message_handle_PLAYBACK_SWITCH_CHANNELS]Video Decoder not started....\n\n");
        return;
    }
    
    if (m_playState == PS_INVALID)
    {
        DEBUG_ERROR("\n\n[OnPlaybackOperationPost]Video Decoder has stopped....\n\n");
        return;
    }
    
    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        msgPlaybackSwitchChannelsAck_t *option = (msgPlaybackSwitchChannelsAck_t *) head->body;
        DEBUG_CRITICAL("playback result =0x%x u32Mode =%d",option->result, option->u32Mode);
        if (m_pAv_device_master)
        {
            memcpy(m_pScreenMosaic->chn, option->szCurChnId, 32 * sizeof(char));
            m_pScreenMosaic->iMode = option->u32Mode;
			m_pScreenMosaic->iFlag = 1;
            ret_val= Aa_switch_output(m_pScreenMosaic, true);
        }

        stMessageResponse ResponseGui;
        ResponseGui.result = (0 == ret_val) ? 0 : -1;
        DEBUG_MESSAGE("\n\n[Aa_message_handle_PLAYBACK_SWITCH_CHANNELS]ResponseToGUI:result:%d\n\n", ResponseGui.result);
        Common::N9M_MSGSendMessage(m_message_client_handle, head->source, MESSAGE_AVSTREAM_DECODER_SWITCH_CHANNELS | RESPONSE_FLAG, 0, (char *)&ResponseGui, sizeof(stMessageResponse), m_play_source_id);
    }
    return;
}


void CAvAppMaster::Aa_message_handle_DECODER_STREAM_OPERATE(const char *msg, int length)
{
    DEBUG_MESSAGE("\n\n Aa_message_handle_DECODER_STREAM_OPERATE \n\n");
    SMessageHead *head = (SMessageHead*) msg;
    int purpose = 1;

    if (!(m_entityReady & AVSTREAM_VIDEODECODER_START))
    {
        DEBUG_ERROR("\n\n[OnVideoDecoderOperate]Video Decoder not started....\n\n");
        return;
    }

    if (m_playState == PS_INVALID)
    {
        DEBUG_ERROR("\n\n[OnVideoDecoderOperate]Video Decoder has stopped....\n\n");
        return;
    }

    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        msgPlaybackOperate_t  *option = (msgPlaybackOperate_t *)head->body;
        if(option != NULL)
        {
            DEBUG_WARNING("iGroupId =%d iOperate =%d \n", option->iGroupId, option->iOperate);
            option->iGroupId = 0;
            
            if (m_pAv_device_master)
            {
                DEBUG_MESSAGE("[Aa_message_handle_DECODER_STREAM_OPERATE]##############################false====%d\n", option->iOperate);
                m_pAv_device_master->Ad_operate_dec(purpose, option->iOperate, false);
            }

            if(option->iOperate == 1)//seek
            {
                m_playState = PS_SEEK;
                option->unOperateData.stuSeek.pts = 0;
            }
            else
            {
                datetime_t playtime;
                if(m_pAv_device_master->Ad_get_dec_playtime(purpose, &playtime) == true)
                {
                    option->unOperateData.stuPlay.hour   = playtime.hour;
                    option->unOperateData.stuPlay.minute = playtime.minute;
                    option->unOperateData.stuPlay.second = playtime.second;
                    DEBUG_MESSAGE("Aa_message_handle_DECODER_STREAM_OPERATE  %02d:%02d:%02d\n", playtime.hour, playtime.minute, playtime.second);
                }
            }
            m_play_source_id = head->source;
            DEBUG_MESSAGE("\n\n Aa_message_handle_DECODER_STREAM_OPERATE send MESSAGE_STORAGE_PLAYBACK_OPERATE\n\n");
            Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_STORAGE_PLAYBACK_OPERATE, 1, (char *)option, sizeof(msgPlaybackOperate_t), head->session);
        }
    }
    else
    {    
        DEBUG_MESSAGE("\n\n Aa_message_handle_DECODER_STREAM_OPERATE message err:head->length:%d length:%d\n\n", head->length, length);

        stMessageResponse ResponseGui;
        ResponseGui.result = -1;
        DEBUG_MESSAGE("\n\n[Aa_message_handle_DECODER_STREAM_OPERATE]ResponseToGUI:result:%d\n\n", ResponseGui.result);
        Common::N9M_MSGResponseMessage(m_message_client_handle, head, (char *)&ResponseGui, sizeof(stMessageResponse));
    }
}

void CAvAppMaster::Aa_message_handle_DECODER_SWITCH_CHANNELS(const char *msg, int length)
{
    DEBUG_MESSAGE("\n\n Aa_message_handle_DECODER_SWITCH_CHANNELS\n\n");
    
    if (!m_StartPlayCmd)
    {
        DEBUG_MESSAGE("\n do not playing now....\n\n");
        return;
    }
    
    SMessageHead *head = (SMessageHead*) msg;
    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        msgPlaybackSwitchChannels_t *option = (msgPlaybackSwitchChannels_t *) head->body;
        DEBUG_CRITICAL("decoder u32ChlMask =0x%x u32Mode =%d",option->u32ChlMask, option->u32Mode);
        m_play_source_id = head->source;
        Common::N9M_MSGSendMessage(m_message_client_handle, 0,  MESSAGE_STORAGE_PLAYBACK_SWITCH_CHANNELS, 1, (char *)option, sizeof(msgPlaybackSwitchChannels_t), head->session);
        
        m_pAv_device_master->Ad_operate_dec_for_switch_channel(VDP_PLAYBACK_VDEC);
    }
    
    return;
}


void CAvAppMaster::Aa_message_handle_DECODER_STREAMSTART(const char *msg, int length)
{
    DEBUG_MESSAGE("\n\n Aa_message_handle_DECODER_STREAMSTART\n\n");

    if(m_StartPlayCmd)
    {
        DEBUG_MESSAGE("\n\nBeing played....\n\n");
        return;
    }
#if defined(_AV_HAVE_AUDIO_INPUT_) && defined(_AV_HAVE_AUDIO_OUTPUT_) && (defined(_AV_PRODUCT_CLASS_DVR_) || defined(_AV_PRODUCT_CLASS_HDVR_))
#ifdef _AV_PLATFORM_HI3515_
    m_pAv_device_master->Ad_unbind_preview_ao();
#else
	m_pAv_device_master->Ad_preview_audio_ctrl(false);
#endif
#endif
    m_StartPlayCmd = true;

    SMessageHead *head = (SMessageHead*) msg;

    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        memset(&m_last_playbcak_time, 0, sizeof(datetime_t));
        //av_dec_set_t av_dec_set;
        int index = 0;
        //int ret = -1;
        msgStartPlayback_t *option = (msgStartPlayback_t *) head->body;

        // option->iGroupId = 0;
        //option->iStreamType = STREAM_TYPE_MAIN;
        if(option->iStreamType == RECORD_MODE_MIRROR)
       	{
		m_play_streamtype_isMirror =1;
	}
	else
	{
		m_play_streamtype_isMirror =0;
	}
	printf("\n###########option->iGroupId=%d,iStreamType=%d,iChlMask=%d,iRecordTypeMask=%d\n",option->iGroupId,option->iStreamType,option->iChlMask,option->iRecordTypeMask);
        memset(&m_av_dec_set, 0, sizeof(av_dec_set_t));
        m_av_dec_set.av_dec_purpose = 1; 
        for (int i = 0; i < _AV_SPLIT_MAX_CHN_; i++)
        {
            m_av_dec_set.chnlist[i] = -1;
        }

        m_av_dec_set.chnbmp = option->iChlMask;
        m_av_dec_set.chnnum = 0;
        index = 0;
        for (int i = 0; i < 32; i++)
        {
            if(GCL_BIT_VAL_TEST(option->iChlMask, i))
            {
                m_av_dec_set.chnlist[index] = i;
                index++;
                m_av_dec_set.chnnum++;
            }
        }
        DEBUG_CRITICAL("m_av_dec_set.chnnum=%d, chnbmp=%x \n",m_av_dec_set.chnnum, m_av_dec_set.chnbmp);

        eProductType product_type = PTD_INVALID;
        N9M_GetProductType(product_type);
        
        char cutomername[32] = {0};
        pgAvDeviceInfo->Adi_get_customer_name(cutomername, sizeof(cutomername));
        
        if ((PTD_T2 == product_type)||(strcmp(cutomername, "CUSTOMER_04.1062") == 0))
        {
            //! Added for T2 which has 14 channels
            if ((m_av_dec_set.chnnum > 1) &&(m_av_dec_set.chnnum < 4))
            {
                DEBUG_CRITICAL(" Probably to modify channel mask !\n");
                if (( option->iChlMask > 0x1000)&&( option->iChlMask < 0xf000))
                {
                    if (!((option->iChlMask)&0xffff0fff)) // there exit not two in different group
                    {
                       m_av_dec_set.chnbmp = 0xf000;
                   
                       index = 0;
                        for (int i = 0; i < 32; i++)
                        {
                            if(GCL_BIT_VAL_TEST(m_av_dec_set.chnbmp, i))
                            {
                                m_av_dec_set.chnlist[index] = i;
                                index++;
                            }
                        }
                       m_av_dec_set.chnnum = 4;
                       DEBUG_CRITICAL("Modified m_av_dec_set.chnnum=%d, chnbmp=%x \n",m_av_dec_set.chnnum, m_av_dec_set.chnbmp);
                       
                    }
                }
                
            }
        }
        
        m_av_dec_set.split_mode = pgAvDeviceInfo->Adi_get_split_mode(m_av_dec_set.chnnum);       
        m_av_dec_set.av_dec_purpose = VDP_PLAYBACK_VDEC;

        DEBUG_MESSAGE("playback before call, splitMode=%d purpose=%d\n",m_av_dec_set.split_mode, m_av_dec_set.av_dec_purpose );

        m_play_source_id = head->source;
        Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_STORAGE_PLAYBACK_START, 1, (char *)option, sizeof(msgStartPlayback_t), head->session);
        Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_AVSTREAM_START_PLAYBACK, 0, NULL, 0, 0);
        //
    }
    else
    {
        DEBUG_MESSAGE("\n\n Aa_message_handle_DECODER_STREAMSTART message err:head->length:%d length:%d\n\n", head->length, length);
    }
}

void CAvAppMaster::Aa_message_handle_DECODER_STREAMSTOP(const char *msg, int length)
{
    DEBUG_MESSAGE("\n\n Aa_message_handle_DECODER_STREAMSTOP\n\n");
    SMessageHead *head = (SMessageHead*) msg;

    m_StartPlayCmd = false;
    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        msgStopPlayback_t option;
        option.iGroupId = 0;

        m_play_source_id = head->source;
        Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_STORAGE_PLAYBACK_STOP, 1, (const char *)&option, sizeof(msgStopPlayback_t), head->session);
        Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_AVSTREAM_STOP_PLAYBACK, 0, NULL, 0, 0);

        if(!(m_entityReady & AVSTREAM_VIDEODECODER_START))
        {
            DEBUG_MESSAGE( "dec is not start,so return!\n");
            return;
        }
        m_entityReady &= (~AVSTREAM_VIDEODECODER_START);
        m_pAv_device_master->Ad_stop_playback_dec();
        m_playState = PS_INVALID;
        pgAvDeviceInfo->Adi_set_cur_preview_res_with_backup_res();
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
        this->Aa_GotoPreviewOutput(m_pScreenMosaic);
#endif
        /*start preview audio*/
#if defined(_AV_HAVE_AUDIO_INPUT_) && defined(_AV_HAVE_AUDIO_OUTPUT_) && (defined(_AV_PRODUCT_CLASS_DVR_) || defined(_AV_PRODUCT_CLASS_HDVR_))
        m_pAv_device_master->Ad_preview_audio_ctrl(true);
#endif
    }
    else
    {
        DEBUG_MESSAGE("\n\n Aa_message_handle_STARTPLAYBACK message err:head->length:%d length:%d\n\n", head->length, length);
    }
    DEBUG_MESSAGE("\n\nAa_message_handle_DECODER_STREAMSTOP Video Decoder stopped...\n\n");
}
#endif

#if defined(_AV_HAVE_VIDEO_ENCODER_)
int CAvAppMaster::Aa_calculate_bitrate( unsigned char u8Resolution, unsigned char u8Quality, unsigned char u8FrameRate, int n32ParaBitRate/*=-1*/ )
{
    int bitrate = 0;
    const float wD1_to_D1_factor = 1.3;
    switch( u8Resolution )
    {
        case 0:/*CIF*/
            switch( u8Quality )/*1-优 2-良 3-好 4-中*/
            {
                case 1:
                default:
                    bitrate = 1024;
                    break;
                case 2:
                    bitrate = 768;
                    break;
                case 3:
                    bitrate = 640;
                    break;
                case 4:
                    bitrate = 512;
                    break;          
                case 5:
                    bitrate = 440;
                    break;
                case 6:
                    bitrate = 350;
                    break;
                case 7:
                    bitrate = 312;
                    break;
                case 8:
                    bitrate = 280;
                    break;
            }
            break;
        case 1:/*HD1*/
            switch( u8Quality )/*1-优 2-良 3-好 4-中*/
            {
                case 1:
                    default:
                    bitrate = 1536;
                    break;
                case 2:
                    bitrate = 1280;
                    break;
                case 3:
                    bitrate = 1024;
                    break;
                case 4:
                    bitrate = 768;
                    break;       
                case 5:
                    bitrate = 640;
                    break;
                case 6:
                    bitrate = 560;
                    break;
                case 7:
                    bitrate = 500;
                    break;
                case 8:
                    bitrate = 450;
                    break;  
            }
            break;
        case 2:/*D1*/
        case 5:
        case 16://SVGA
            switch( u8Quality )/*1-优 2-良 3-好 4-中*/
            {
                case 1:
                default:
                    bitrate = 2048;
                    break;
                case 2:
                    bitrate = 1536;
                    break;
                case 3:
                    bitrate = 1280;
                    break;
                case 4:
                    bitrate = 1024;
                    break;
                case 5:
                    bitrate = 900;
                    break;
                case 6:
                    bitrate = 800;
                    break;
                case 7:
                    bitrate = 720;
                    break;
                case 8:
                    bitrate = 640;
                    break;
            }
            break;
        case 3:/*QCIF*/
        case 4:
            switch( u8Quality )/*1-优 2-良 3-好 4-中*/
            {
                case 1:
                default:
                    bitrate = 512;
                    break;
                case 2:
                    bitrate = 384;
                    break;
                case 3:
                    bitrate = 320;
                    break;
                case 4:
                    bitrate = 256;
                    break;
            }
            break;
        case 6:/*720p*/
        case 17://XGA
            switch( u8Quality )/*1-优 2-良 3-好 4-中*/
            {
                case 1:
                default:
                    bitrate = 6144;
                    break;
                case 2:
                    bitrate = 5120;
                    break;
                case 3:
                    bitrate = 4096;
                    break;
                case 4:
                    bitrate = 3072;
                    break;
                case 5:
                    bitrate = 2560;
                    break;
                case 6:
                    bitrate = 2048;
                    break;
                case 7:
                    bitrate = 1536;
                    break;
                case 8:
                    bitrate = 1024;
                    break;
            }
            break;
        case 7:/*1080p*/
            switch( u8Quality )/*1-优 2-良 3-好 4-中*/
            {
                case 1:
                default:
                    bitrate = 8192;
                    break;
                case 2:
                    bitrate = 6390;
                    break;
                case 3:
                    bitrate = 5505;
                    break;
                case 4:
                    bitrate = 4068;
                    break;
                case 5:
                    bitrate = 3712;
                    break;
                case 6:
                    bitrate = 2816;
                    break;
                case 7:
                    bitrate = 1919;
                    break;
                case 8:
                    bitrate = 1024;
                    break;
            }
            break;
        case 10:/*WQCIF*/
            switch( u8Quality )/*1-优 2-良 3-好 4-中*/
            {
                case 1:
                default:
                    bitrate = (int )(512 * wD1_to_D1_factor);
                    break;
                case 2:
                    bitrate = (int )(384 * wD1_to_D1_factor);
                    break;
                case 3:
                    bitrate = (int )(320 * wD1_to_D1_factor);
                    break;
                case 4:
                    bitrate = (int )(256 * wD1_to_D1_factor);
                    break;
            }
            break;

        case 11:/*WCIF*/
            switch( u8Quality )/*1-优 2-良 3-好 4-中*/
            {
                case 1:
                default:
                    bitrate = (int )(1024 * wD1_to_D1_factor);
                    break;
                case 2:
                    bitrate = (int )(768 * wD1_to_D1_factor);
                    break;
                case 3:
                    bitrate = (int )(640 * wD1_to_D1_factor);
                    break;
                case 4:
                    bitrate = (int )(512 * wD1_to_D1_factor);
                    break;
                case 5:
                    bitrate = (int )(440 * wD1_to_D1_factor);
                    break;
                case 6:
                    bitrate = (int )(350 * wD1_to_D1_factor);
                    break;
                case 7:
                    bitrate = (int )(312 * wD1_to_D1_factor);
                    break;
                case 8:
                    bitrate = (int )(280 * wD1_to_D1_factor);
                    break;

            }
            break;
        case 12:/*WHD1*/
            switch( u8Quality )/*1-优 2-良 3-好 4-中*/
            {
                case 1:
                default:
                    bitrate = (int )(1536 * wD1_to_D1_factor);
                    break;
                case 2:
                    bitrate = (int )(1280 * wD1_to_D1_factor);
                    break;
                case 3:
                    bitrate = (int )(1024 * wD1_to_D1_factor);
                    break;
                case 4:
                    bitrate = (int )(768 * wD1_to_D1_factor);
                    break;
                case 5:
                    bitrate = (int )(640 * wD1_to_D1_factor);
                    break;
                case 6:
                    bitrate = (int )(560 * wD1_to_D1_factor);
                    break;
                case 7:
                    bitrate = (int )(500 * wD1_to_D1_factor);
                    break;
                case 8:
                    bitrate = (int )(450 * wD1_to_D1_factor);
                    break;  
            }
            break;
        case 13:/*WD1*/
            switch( u8Quality )/*1-优 2-良 3-好 4-中*/
            {
                case 1:
                default:
                    bitrate = (int )(2048 * wD1_to_D1_factor);
                    break;
                case 2:
                    bitrate = (int )(1536 * wD1_to_D1_factor);
                    break;
                case 3:
                    bitrate = (int )(1280 * wD1_to_D1_factor);
                    break;
                case 4:
                    bitrate = (int )(1024 * wD1_to_D1_factor);
                    break;
                case 5:
                    bitrate = (int )(900 * wD1_to_D1_factor);
                    break;
                case 6:
                    bitrate = (int )(800 * wD1_to_D1_factor);
                    break;
                case 7:
                    bitrate = (int )(720 * wD1_to_D1_factor);
                    break;
                case 8:
                    bitrate = (int )(640 * wD1_to_D1_factor);
                    break;
            }
            break;
        case 14:/*960p*/
            switch( u8Quality )/*1-优 2-良 3-好 4-中*/
            {
                case 1:
                default:
                    bitrate = 3072;
                    break;
                case 2:
                    bitrate = 2560;
                    break;
                case 3:
                    bitrate = 2048;
                    break;
                case 4:
                    bitrate = 1536;
                    break;
            }
            break;
        case 15: // 540p
            switch( u8Quality )/*1-优 2-良 3-好 4-中*/
            {
                case 1:
                default:
                    bitrate = 2048;
                    break;
                case 2:
                    bitrate = 1536;
                    break;
                case 3:
                    bitrate = 1280;
                    break;
                case 4:
                    bitrate = 1024;
                    break;
            }
            break;
        case 9: // 5M
            switch( u8Quality )/*1-优 2-良 3-好 4-中*/
            {
                case 1:
                default:
                    bitrate = 8192;
                    break;
                case 2:
                    bitrate = 7680;
                    break;
                case 3:
                    bitrate = 7168;
                    break;
                case 4:
                    bitrate = 6656;
                    break;
            }
            break;       
        default:
            bitrate = 0;
            DEBUG_ERROR("Aa_calculate_bitrate You must give the implement(%d)\n", u8Resolution);
            //_AV_FATAL_(1, "CAvAppMaster::Aa_calculate_bitrate You must give the implement(%d)\n", u8Resolution);
            break;
    }
    if( n32ParaBitRate > 0 )
    {
        bitrate = n32ParaBitRate;

#ifndef _AV_PRODUCT_CLASS_IPC_        
        return bitrate;
#endif
    }

    double coefficient;
    if(m_av_tvsystem == _AT_PAL_)
    {
        if( u8FrameRate <= 5 )
        {
            coefficient = 1.4;
        }
        else if((u8FrameRate >= 6) && (u8FrameRate <= 11))
        {
            coefficient = 1.3;
        }
        else if((u8FrameRate >= 12) && (u8FrameRate <= 17))
        {
            coefficient = 1.2;
        }
        else if((u8FrameRate >= 18) && (u8FrameRate <= 22))
        {
            coefficient = 1.1;
        }
        else
        {
            coefficient = 1.0;
        }

        if(u8FrameRate > 25)
        {
            bitrate = (int)(bitrate * 25 * coefficient / 25);
        }
        else
        {
            bitrate = (int)(bitrate * u8FrameRate * coefficient / 25);            
        }

    }
    else
    {
        if(u8FrameRate <= 6)
        {
            coefficient = 1.4;
        }
        else if((u8FrameRate >= 7) && (u8FrameRate <= 14))
        {
            coefficient = 1.3;
        }
        else if((u8FrameRate >= 15) && (u8FrameRate <= 21))
        {
            coefficient = 1.2;
        }
        else if((u8FrameRate >= 22) && (u8FrameRate <= 27))
        {
            coefficient = 1.1;
        }
        else
        {
            coefficient = 1.0;
        }

        if(u8FrameRate > 30)
        {
            bitrate = (int)(bitrate * 30 * coefficient / 30);
        }
        else
        {
            bitrate = (int)(bitrate * u8FrameRate * coefficient / 30);
        }
    }

    return bitrate;
}



int CAvAppMaster::Aa_stop_selected_streamtype_encoders(unsigned long long int chn_mask, unsigned int streamtype_mask, bool bcheck_flag)
{
    if(0 == chn_mask || 0 == streamtype_mask)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_stop_selected_streamtype_encoders(chn_mask = %llu, stream_mask = %d)\n", chn_mask, streamtype_mask);   
        return 0;
    }

    int max_vi_chn_num = pgAvDeviceInfo->Adi_get_vi_chn_num();
    unsigned long long int ulllocal_encoder_chn_mask = 0;
#ifdef _AV_PLATFORM_HI3515_
	for (int video_stream_type_num = _AST_UNKNOWN_-1; video_stream_type_num >=_AST_MAIN_VIDEO_; video_stream_type_num--)
#else
	for(int video_stream_type_num = _AST_MAIN_VIDEO_; video_stream_type_num < _AST_UNKNOWN_; video_stream_type_num++)
#endif
    {
        if(GCL_BIT_VAL_TEST(streamtype_mask, video_stream_type_num))
        {
            ulllocal_encoder_chn_mask = chn_mask;
            if(bcheck_flag)
            {
                if(0 != Aa_get_local_encoder_chn_mask(static_cast<av_video_stream_type_t>(video_stream_type_num), &ulllocal_encoder_chn_mask))
                {
                    DEBUG_ERROR( "CAvAppMaster::Aa_get_local_encoder_chn_mask() failed\n");
                    return -1;
                }
            }
            if(0 != m_pAv_device->Ad_stop_selected_encoder(ulllocal_encoder_chn_mask, static_cast<av_video_stream_type_t>(video_stream_type_num), _AAST_NORMAL_))
            {
                DEBUG_ERROR( "CAvAppMaster::Ad_stop_selected_encoder failed\n");
            }

            Aa_set_encoders_state(ulllocal_encoder_chn_mask, max_vi_chn_num, false, static_cast<av_video_stream_type_t>(video_stream_type_num));
        }
    }

    return 0;
}

int CAvAppMaster::Aa_stop_encoders_for_parameters_change(unsigned long long int stop_encoder_chn_mask[_AST_UNKNOWN_], bool bcheck_flag)
{
    int max_vi_chn_num = pgAvDeviceInfo->Adi_get_vi_chn_num();
    
    unsigned long long int ulllocal_encoder_chn_mask = 0;
#ifdef _AV_PLATFORM_HI3515_
	for (int video_stream_type_num = _AST_UNKNOWN_-1; video_stream_type_num >=_AST_MAIN_VIDEO_; video_stream_type_num--)
#else
	for (int video_stream_type_num = _AST_MAIN_VIDEO_; video_stream_type_num < _AST_UNKNOWN_; video_stream_type_num++)
#endif
    {
       if (_AST_SNAP_VIDEO_ == video_stream_type_num)
       {
           continue;
       }
       
       if (0 != stop_encoder_chn_mask[video_stream_type_num])
       {
           ulllocal_encoder_chn_mask = stop_encoder_chn_mask[video_stream_type_num];
           
           if (bcheck_flag)
           {
               if (0 != Aa_get_local_encoder_chn_mask(static_cast<av_video_stream_type_t>(video_stream_type_num), &ulllocal_encoder_chn_mask))
               {
                   DEBUG_ERROR( "CAvAppMaster::Aa_get_local_encoder_chn_mask() failed\n");
                   return -1;
               }
           }
           if (0 != m_pAv_device->Ad_stop_selected_encoder(ulllocal_encoder_chn_mask, static_cast<av_video_stream_type_t>(video_stream_type_num), _AAST_NORMAL_))
           {
               DEBUG_ERROR( "CAvAppMaster::Ad_stop_selected_encoder failed\n");
           }
           
           Aa_set_encoders_state(ulllocal_encoder_chn_mask, max_vi_chn_num, false, static_cast<av_video_stream_type_t>(video_stream_type_num));
       } 
    }

    return 0;
}


int CAvAppMaster::Aa_set_encoder_param(av_video_stream_type_t video_stream_type, int pyh_chn_num, int n32BitRate/*=-1*/, unsigned char u8FrameRate/*=0xff*/, unsigned char u8Res/*=0xff*/, bool bEnable/*=true*/)
{  
    av_video_encoder_para_t video_para;
    av_audio_encoder_para_t audio_para;
    int phy_audio_chn_num = -1;
    bool encoder_start_mark = false;
    
    switch(video_stream_type)
    {
        case _AST_MAIN_VIDEO_:  
            encoder_start_mark = m_main_encoder_state.test(pyh_chn_num); 
            break;
        case _AST_SUB_VIDEO_:  
            encoder_start_mark = m_sub_encoder_state.test(pyh_chn_num); 
            break;
        case _AST_SUB2_VIDEO_:  
            encoder_start_mark = m_sub2_encoder_state.test(pyh_chn_num); 
            break;
        default:          
            DEBUG_ERROR( "CAvAppMaster::Aa_set_encoder_param invalid type\n");
            return -1;
    }
      
    pgAvDeviceInfo->Adi_get_audio_chn_id(pyh_chn_num , phy_audio_chn_num);      
    Aa_get_encoder_para(video_stream_type, pyh_chn_num, &video_para, phy_audio_chn_num, &audio_para);

    if( _AST_SUB2_VIDEO_ == video_stream_type && encoder_start_mark && !bEnable )
    {
        if(m_pAv_device->Ad_stop_encoder(video_stream_type, pyh_chn_num, _AAST_NORMAL_, phy_audio_chn_num) != 0)
        {
            DEBUG_ERROR( "CAvAppMaster::Aa_set_encoder_param m_pAv_device->Ad_stop_encoder FAIELD(chn:%d)\n", pyh_chn_num);
            return -1;
        }
         
        m_sub2_encoder_state[pyh_chn_num] = 0;
        return 0;
    }

    // DEBUG_ERROR( "CAvAppMaster::Aa_set_encoder_param (encoder_start_mark %d) (enable %d)(type:%d) (bitrate:%d) (framerate:%d) (res:%d)\n", encoder_start_mark, bEnable, video_stream_type, n32BitRate, u8FrameRate, u8Res);
               
    if( n32BitRate > 0 )
    {
        video_para.bitrate.hicbr.bitrate = n32BitRate;
    }
    
    if( u8FrameRate != 0xff && u8FrameRate > 0 )
    {
        video_para.frame_rate = u8FrameRate;

        if(video_stream_type == _AST_SUB_VIDEO_ || video_stream_type == _AST_SUB2_VIDEO_)
        {
#ifdef _AV_PRODUCT_CLASS_IPC_
            //the gop of N9M setted to 50 temporarily,it will be configurable in the future
            if(video_stream_type == _AST_SUB2_VIDEO_)
            {
                video_para.gop_size = u8FrameRate;
            }
            else
             {
                video_para.gop_size  = 50;
             }          
 #else
            if(u8FrameRate < 5)
            {
                video_para.gop_size = u8FrameRate * 4;
            }
            else if(u8FrameRate < 10)
            {
                video_para.gop_size = u8FrameRate * 3;
            }
            else
            {
                video_para.gop_size = u8FrameRate * 2;
            }
 #endif
        }
        else
        {
#if defined(_AV_PLATFORM_HI3518EV200_)
            video_para.gop_size = u8FrameRate*2;
#else
            if(PTD_712_VF == pgAvDeviceInfo->Adi_product_type() && video_para.bitrate_mode == _ABM_VBR_)
            {
                video_para.gop_size = u8FrameRate*4;
            }
            else
            {
                video_para.gop_size = u8FrameRate;
            }        
#endif
        }
    }

    if (false == encoder_start_mark && bEnable)
    {
        if(m_pAv_device->Ad_start_encoder(video_stream_type, pyh_chn_num, &video_para, _AAST_NORMAL_, phy_audio_chn_num, &audio_para) != 0)
        {
            DEBUG_ERROR( "CAvAppMaster::Aa_set_encoder_param m_pAv_device->Ad_start_encoder FAIELD(chn:%d)\n", pyh_chn_num);
            return -1;
        }
        
        switch(video_stream_type)
        {
            case _AST_MAIN_VIDEO_:
                m_main_encoder_state[pyh_chn_num] = 1;
                break;
            case _AST_SUB_VIDEO_:
                m_sub_encoder_state[pyh_chn_num] = 1;
                break;
            case _AST_SUB2_VIDEO_:
                m_sub2_encoder_state[pyh_chn_num] = 1;
                break;
            default:
                return -1;
        }
    }
    else if (u8Res != 0xff)
    {
        pgAvDeviceInfo->Adi_get_video_resolution(&u8Res, &video_para.resolution, true);
        
        if(m_pAv_device->Ad_stop_encoder(video_stream_type, pyh_chn_num, _AAST_NORMAL_, phy_audio_chn_num) != 0)
        {
            DEBUG_ERROR( "CAvAppMaster::Aa_set_encoder_param m_pAv_device->Ad_stop_encoder FAIELD(chn:%d)\n", pyh_chn_num);
            return -1;
        }

        if(m_pAv_device->Ad_start_encoder(video_stream_type, pyh_chn_num, &video_para, _AAST_NORMAL_, phy_audio_chn_num, &audio_para) != 0)
        {
            DEBUG_ERROR( "CAvAppMaster::Aa_set_encoder_param m_pAv_device->Ad_start_encoder FAIELD(chn:%d)\n", pyh_chn_num);
            return -1;
        }
    }
    else
    {
        if(m_pAv_device->Ad_modify_encoder(video_stream_type, pyh_chn_num, &video_para) != 0)
        {
            DEBUG_ERROR( "CAvAppMaster::Aa_set_encoder_param m_pAv_device->Ad_start_encoder FAIELD(chn:%d)\n", pyh_chn_num);
            return -1;
        }
    }
    
    return 0;
}

int CAvAppMaster::Aa_adapt_encoder_param( unsigned int u32StreamMask, unsigned int u32ChnMask, int n32BitRate/*=0xff*/, unsigned char u8FrameRate/*=0xff*/, unsigned char u8Res/*=0xff*/, unsigned char u8SrcFrameRate/*=0xff*/ )
{
#ifdef _AV_PRODUCT_CLASS_IPC_
    if( u8SrcFrameRate != 0xff && u8SrcFrameRate != 0 )
    {
        m_u8SourceFrameRate = u8SrcFrameRate;
    }
    if( u8FrameRate > m_u8SourceFrameRate && u8FrameRate != 0xff )
    {
        u8FrameRate = m_u8SourceFrameRate;
    }
#endif

    if( (u32StreamMask>>_AST_MAIN_VIDEO_) & 0x01 )
    {
        for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
        {
            if( ((u32ChnMask>>chn)&0x01) && m_main_encoder_state.test(chn))
            {
                Aa_set_encoder_param(_AST_MAIN_VIDEO_, chn, n32BitRate, u8FrameRate, u8Res);
            }
        }
    }


    if( (u32StreamMask>>_AST_SUB_VIDEO_) & 0x01 )
    {
        for( int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++ )
        {
            if( ((u32ChnMask>>chn)&0x01) && m_sub_encoder_state.test(chn) )
            {
                Aa_set_encoder_param(_AST_SUB_VIDEO_, chn, n32BitRate, u8FrameRate, u8Res);               
            }
        }
    }
    
    if( (u32StreamMask>>_AST_SUB2_VIDEO_) & 0x01 )
    {
        for( int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++ )
        {
            if( ((u32ChnMask>>chn)&0x01) && m_sub2_encoder_state.test(chn) )
            {
#ifdef _AV_PRODUCT_CLASS_IPC_
                if( u8FrameRate > m_u8SourceFrameRate && u8FrameRate != 0xff )
                {
                    u8FrameRate = m_u8SourceFrameRate;
                }
#endif
                Aa_set_encoder_param(_AST_SUB2_VIDEO_, chn, n32BitRate, u8FrameRate, u8Res, m_pSub2_stream_encoder ? m_pSub2_stream_encoder->stuVideoEncodeSetting[chn].ucChnEnable : true);
            }
        }
    }

    return 0;
}

int CAvAppMaster::Aa_check_and_reset_encoder_param(paramVideoEncode_t* pstuEncoderParam)
{
    if(pstuEncoderParam)
    {
        if( pstuEncoderParam->ucFrameRate == 0 )
        {
            pstuEncoderParam->ucFrameRate = 2;
        }

#if 0
        if( pstuEncoderParam->ucResolution > 15 )
        {
            pstuEncoderParam->ucResolution = 0;
        }
#endif
    }
    
    return 0;
}

int CAvAppMaster::Aa_init_video_audio_para(av_video_encoder_para_t *pVideo_para, av_audio_encoder_para_t *pAudio_para, unsigned long long int chn_mask, int max_vi_chn_num, av_video_stream_type_t video_stream_type)
{
    if(_AST_UNKNOWN_ == video_stream_type)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_init_video_audio_para() invalid video_stream_type = %d\n", video_stream_type);
        return -1;
    }
    
    memset(pVideo_para, 0, max_vi_chn_num * sizeof(av_video_encoder_para_t));
    memset(pAudio_para, 0, max_vi_chn_num * sizeof(av_audio_encoder_para_t));

    int phy_audio_chn_num = -1;
    for(int chn_num = 0; chn_num < max_vi_chn_num; chn_num++)
    {
        if(GCL_BIT_VAL_TEST(chn_mask, chn_num))
        {
            pgAvDeviceInfo->Adi_get_audio_chn_id(chn_num , phy_audio_chn_num);
            
            Aa_get_encoder_para(video_stream_type, chn_num, pVideo_para + chn_num, phy_audio_chn_num, pAudio_para + chn_num);
        }
    }

    return 0;
}

unsigned char CAvAppMaster::Aa_get_all_streamtype_mask()
{
    unsigned char streamtype_mask = 0;
    eProductType product_type = PTD_INVALID;
    eSystemType subproduct_type = SYSTEM_TYPE_MAX;

    /*device type*/
    N9M_GetProductType(product_type);
    subproduct_type = N9M_GetSystemType();
    
    for(int video_stream_type_num = _AST_MAIN_VIDEO_; video_stream_type_num < _AST_UNKNOWN_; video_stream_type_num++)
    {
        if(video_stream_type_num == _AST_SNAP_VIDEO_)
        {
            continue;
        }
        if(((product_type == PTD_X1) && ((subproduct_type == SYSTEM_TYPE_X1_M0401) ||\
                                        (subproduct_type == SYSTEM_TYPE_X1_V2_M1H0401)||\
                                        (subproduct_type == SYSTEM_TYPE_X1_V2_M1SH0401)))||\
             ((product_type == PTD_X1_AHD) && ((subproduct_type == SYSTEM_TYPE_X1_V2_M1H0401_AHD)||\
                                        (subproduct_type == SYSTEM_TYPE_X1_V2_M1SH0401_AHD)))||\
          (product_type == PTD_D5M)||\
          (product_type == PTD_A5_II)||(product_type == PTD_6A_I))
        {
            if(video_stream_type_num == _AST_SUB_VIDEO_)
            {
                continue;
            }
        }
        GCL_BIT_VAL_SET(streamtype_mask, video_stream_type_num);
    }
   
    return streamtype_mask;
}

#if  !defined(_AV_PRODUCT_CLASS_IPC_)
//获取语言参数对应的语言ID 
bool CAvAppMaster::GetLanguageSysConfigID(int language, int &languageId)
{
    Json::Value special_info;    
    N9M_GetSpecInfo(special_info);
    for(int index = 0; index < (int)special_info["UI_LANG_CFG"].size(); index++)
    {
        if(special_info["UI_LANG_CFG"][index]["VALUE"].isInt() && \
        (language == special_info["UI_LANG_CFG"][index]["VALUE"].asInt()))
        {
            languageId = index;
            break;
        }
    }
    return true;
}
//绑定语言
bool CAvAppMaster::LoadLanguage(Json::Value m_SystemConfig, int languageID)
{
    std::string ConfigResult;
    std::string Dir;

    int position = 0;
    std::string BaseDir;
    if(languageID < (int)m_SystemConfig["UI_LANG_CFG"].size())
    {
        if(!m_SystemConfig["UI_LANG_CFG"][languageID]["CFG_FILE"].isString() ||\
        !m_SystemConfig["UI_LANG_CFG"][languageID]["FONT_LIB"].isString())
        {
            return false;
        }
        DEBUG_MESSAGE("Load Language Start.........\n");
        GuiTKClearLanguage();

        if((position = m_SystemConfig["UI_LANG_CFG"][languageID]["CFG_FILE"].asString().find_last_of('/')) != (int)std::string::npos)
        {
            BaseDir = m_SystemConfig["UI_LANG_CFG"][languageID]["CFG_FILE"].asString().substr(0, position + 1);
        }

        DEBUG_MESSAGE("First Load English Standard dir[%s]\n", (BaseDir + "english.ini").c_str());
        GuiTKBindLanguage((BaseDir + "english.ini").c_str(), "NORMAL", NULL);
        std::string language_dir = m_SystemConfig["UI_LANG_CFG"][languageID]["CFG_FILE"].asCString();

        DEBUG_MESSAGE("Second Load Current Language dir[%s]\n", language_dir.c_str());

        //! start load font lib
        std::string load_language_dir = m_SystemConfig["UI_LANG_CFG"][languageID]["FONT_LIB_FILE"].asString();

         //!< absolute dir  
         CAvFontLibrary * pAvfl = NULL;
        _AV_FATAL_((pAvfl = CAvFontLibrary::Afl_get_instance()) == NULL, "OUT OF MEMORY\n");
        std::string ab_dir = "/usr/local/bin/";
        ab_dir += load_language_dir;
        pAvfl->FontInit((char*)ab_dir.c_str()); 
        std::cout<<"load lib"<<load_language_dir<<std::endl;
        //!< end load
        GuiTKBindLanguage((char*)language_dir.c_str(), NULL, NULL);

        /************SPECIAL CUSTOMER LANGUAGE FILE*****START************/
        if(m_SystemConfig["CUSTOMER_NAME"].isString())
        {
            if(!BaseDir.empty())
            {
                Dir = BaseDir + m_SystemConfig["CUSTOMER_NAME"].asString() + "_" + \
                m_SystemConfig["UI_LANG_CFG"][languageID]["CFG_FILE"].asString().substr(position + 1);
                DEBUG_MESSAGE("Third load [%s] special language file[%s]", m_SystemConfig["CUSTOMER_NAME"].asCString(), Dir.c_str());
                GuiTKBindLanguage(Dir.c_str(), NULL, NULL);
            }
        }
        /************SPECIAL CUSTOMER LANGUAGE FILE*****END************/

        /************SINGLE TRANSLATION SET*****START************/
        //SingleTranslationSet();
        /************SINGLE TRANSLATION SET*****END  ************/
        return true;
    }
    return false;
}
#endif

int CAvAppMaster::Aa_get_encoder_para(av_video_stream_type_t video_stream_type, int phy_video_chn_num, av_video_encoder_para_t *pVideo_para, int phy_audio_chn_num, av_audio_encoder_para_t *pAudio_para)
{
    if((phy_video_chn_num >= 0) && (pVideo_para != NULL))
    {
        int n32ParaBitRate = -1;
        int record_audio = 0;
        eProductType product_type = PTD_INVALID;
        N9M_GetProductType(product_type);
        unsigned char ucAudioEnable = 0, ucFrameRate = 0, ucQuality = 0, ucResolution = 0xff,bitratelevel = 0; 
        int ucBitrate=0;
        av_ref_para_t av_ref_para;
        av_bitrate_mode_t EncodeMode = _ABM_CBR_;
#if defined(_AV_PRODUCT_CLASS_IPC_)
        av_video_type_t video_type = _AVT_H264_;
#endif

        av_ref_para.ref_type = _ART_REF_FOR_1X;
        av_ref_para.pred_enable = 1;
        av_ref_para.base_interval = 1;
        av_ref_para.enhance_interval = 0;

        switch(video_stream_type)
        {
            case _AST_MAIN_VIDEO_:
            {
                pgAvDeviceInfo->Adi_get_ref_para(av_ref_para);
#if  defined(_AV_PRODUCT_CLASS_IPC_)
                if (pgAvDeviceInfo->Adi_get_custom_bitrate())
                {
                    n32ParaBitRate = m_pMain_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].uiBitrate;
                }
#endif
                if(m_record_type[phy_video_chn_num][0] == 0)
                {
                    DEBUG_DEBUG("normal quality phy_video_chn_num=%d\n",phy_video_chn_num);
                    ucQuality = m_pMain_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucNormalQuality;
                    ucBitrate = m_pMain_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].uiBitrate;
                }
                else
                {
                    DEBUG_DEBUG("alarm quality phy_video_chn_num=%d\n",phy_video_chn_num);
                    ucQuality = m_pMain_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucAlarmQuality;
                    ucBitrate = m_pMain_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].uiBitrate;
                }
#if !defined(_AV_PRODUCT_CLASS_DVR_REI_)
                ucFrameRate = m_pMain_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucFrameRate;
#else
                if(m_record_type[phy_video_chn_num][0] == 0)
                {
                    ucFrameRate = m_pMain_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucFrameRate;
                }
                else
                {
                    ucFrameRate = m_pMain_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucAlarmFrameRate;
                }
#endif
                ucResolution = m_pMain_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucResolution;
                ucAudioEnable = m_pMain_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucAudioEnable;
                if(m_pMain_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucBrMode)
                {
                    EncodeMode = _ABM_VBR_;
                }
                else
                {
                    EncodeMode = _ABM_CBR_;
                }
                if(ucAudioEnable == 2)
                {
                    if(m_record_type[phy_video_chn_num][0] == 0)
                    {
                        record_audio = 0;
                    }
                    else
                    {
                        record_audio = 1;
                    }
                }
                break;
            }
            case _AST_SUB_VIDEO_:
            {
                if(m_pSub_stream_encoder != NULL)
                {
                    pgAvDeviceInfo->Adi_get_ref_para(av_ref_para, _AST_SUB_VIDEO_);
#if defined(_AV_PRODUCT_CLASS_IPC_)
                    if (pgAvDeviceInfo->Adi_get_custom_bitrate())
                    {
                        n32ParaBitRate = m_pSub_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].uiBitrate;
                    }
#endif
#if defined(_AV_PLATFORM_HI3516A_)
#if defined(_AV_SUPPORT_IVE_)
                    if(1 == pgAvDeviceInfo->Adi_get_ive_debug_mode())
                    {
                        ucResolution = 5;
                    }
                    else
                    {
                        ucResolution = m_pSub_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucResolution;                  
                    }
#else
                    ucResolution = m_pSub_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucResolution;
#endif
#else
                    ucResolution = m_pSub_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucResolution;
#endif
                    if(m_record_type[phy_video_chn_num][1] == 0)
                    {
                        ucQuality = m_pSub_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucNormalQuality;
                        ucBitrate = m_pSub_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].uiBitrate;
                    }
                    else
                    {
                        ucQuality = m_pSub_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucAlarmQuality;
                        ucBitrate = m_pSub_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].uiBitrate;
                    }
                    ucFrameRate = m_pSub_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucFrameRate;
#ifdef _AV_PLATFORM_HI3515_
					ucAudioEnable = m_pMain_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucAudioEnable;
#else
					ucAudioEnable = m_pSub_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucAudioEnable;
#endif
                    if(m_pSub_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucBrMode)
                    {
                        EncodeMode = _ABM_VBR_;
                    }
                    else
                    {
                        EncodeMode = _ABM_CBR_;
                    }
                    if(ucAudioEnable == 2)
                    {
                        if(m_record_type[phy_video_chn_num][1] == 0)
                        {
                            record_audio = 0;
                        }
                        else
                        {
                            record_audio = 1;
                        }
                    }
                }
                break;
            }
            case _AST_SUB2_VIDEO_:
            {
                if (pgAvDeviceInfo->Adi_get_custom_bitrate())
                {
                    n32ParaBitRate = m_pSub2_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].uiBitrate;
                }
                ucBitrate = m_pSub2_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].uiBitrate;
                ucResolution = m_pSub2_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucResolution;
                ucQuality = m_pSub2_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucNormalQuality;
                ucFrameRate = m_pSub2_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucFrameRate;
#ifdef _AV_PLATFORM_HI3515_
				ucAudioEnable = m_pMain_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucAudioEnable;
#else
				ucAudioEnable = m_pSub2_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucAudioEnable;
#endif
                if(m_pSub2_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucBrMode)
                {
                    EncodeMode = _ABM_VBR_;
                }
                else
                {
                    EncodeMode = _ABM_CBR_;
                }
                if( ucFrameRate == 0 )
                {
                    ucFrameRate = 1;
                }
                bitratelevel = m_pSub2_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucReserved[0];
                av_ref_para.ref_type = _ART_REF_FOR_4X;
                if ((PTD_X1 == pgAvDeviceInfo->Adi_product_type())||(PTD_X1_AHD== pgAvDeviceInfo->Adi_product_type()))
                {
					if(ucAudioEnable == 2)
					{
						if(m_record_type[phy_video_chn_num][1] == 0)
						{
							record_audio = 0;
						}
						else
						{
							record_audio = 1;
						}
					}
                }
                break;
            }
            default:
                return 0;
                break;
        }
        /*video parameter*/
        memset(pVideo_para, 0, sizeof(av_video_encoder_para_t));
        memcpy(&pVideo_para->ref_para, &av_ref_para, sizeof(av_ref_para_t));
        pVideo_para->virtual_chn_num = phy_video_chn_num;
        pVideo_para->tv_system = m_av_tvsystem;
        pVideo_para->bitrate_mode = EncodeMode;
        pVideo_para->net_transfe_mode = m_net_transfe;
        pVideo_para->have_audio = ucAudioEnable;
        pVideo_para->record_audio = record_audio;
        pgAvDeviceInfo->Adi_get_video_resolution(&ucResolution, &pVideo_para->resolution, true);
        pVideo_para->frame_rate = ucFrameRate;
        pVideo_para->bitrate_level = bitratelevel;
#if defined(_AV_PRODUCT_CLASS_IPC_)
        pVideo_para->video_type = video_type;
#endif
#ifdef _AV_PRODUCT_CLASS_IPC_
        pVideo_para->source_frame_rate = m_u8SourceFrameRate;
        if( pVideo_para->frame_rate > pVideo_para->source_frame_rate )
        {
            pVideo_para->frame_rate = pVideo_para->source_frame_rate;
        }
#endif
        if(video_stream_type == _AST_SUB_VIDEO_ || video_stream_type == _AST_SUB2_VIDEO_)
        {
#if defined(_AV_PRODUCT_CLASS_DVR_REI_)
            pVideo_para->gop_size = pVideo_para->frame_rate;
#else
            if(pVideo_para->frame_rate < 5)
            {
                pVideo_para->gop_size = pVideo_para->frame_rate * 4;
            }
            else if(pVideo_para->frame_rate < 10)
            {
                pVideo_para->gop_size = pVideo_para->frame_rate * 3;
            }
            else
            {
                pVideo_para->gop_size = pVideo_para->frame_rate * 2;
            }
            if(PTD_6A_II_AVX == pgAvDeviceInfo->Adi_product_type() || PTD_6A_I == pgAvDeviceInfo->Adi_product_type())	//6AII项目，字码流每秒一个I帧///
            {
                pVideo_para->gop_size = pVideo_para->frame_rate;
            }
#endif
        }
        else
        {
#if defined(_AV_PLATFORM_HI3518EV200_)
            pVideo_para->gop_size = pVideo_para->frame_rate*2;
#else
            if(PTD_712_VF == pgAvDeviceInfo->Adi_product_type() && EncodeMode ==  _ABM_VBR_)
            {
                pVideo_para->gop_size = pVideo_para->frame_rate*4;
            }
            else
            {
                pVideo_para->gop_size = pVideo_para->frame_rate;
            }     
#endif
            pVideo_para->i_frame_record = m_pMain_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucFrameType ? 1 : 0;
        }
#if defined(_AV_PRODUCT_CLASS_IPC_)
        if(video_stream_type != _AST_SUB2_VIDEO_)
        {
            pVideo_para->bitrate.hicbr.bitrate = Aa_calculate_bitrate(ucResolution, ucQuality, pVideo_para->frame_rate, n32ParaBitRate);
            pVideo_para->bitrate.hivbr.bitrate = Aa_calculate_bitrate(ucResolution, ucQuality, pVideo_para->frame_rate, n32ParaBitRate);             
        }
        else
        {
            pVideo_para->bitrate.hicbr.bitrate = n32ParaBitRate;
            pVideo_para->bitrate.hivbr.bitrate = n32ParaBitRate;
        }
        ucBitrate =ucBitrate;
#else
#if defined(_AV_PRODUCT_CLASS_DVR_REI_)
printf("\nAa_get_encoder_para ***,ucBitrate=%d,ucQuality=%d\n",ucBitrate,ucQuality);

        pVideo_para->bitrate.hicbr.bitrate = ucBitrate;
        pVideo_para->bitrate.hivbr.bitrate = ucBitrate;
        ucQuality=ucQuality;
        n32ParaBitRate=n32ParaBitRate;
#else
        pVideo_para->bitrate.hicbr.bitrate = Aa_calculate_bitrate(ucResolution, ucQuality, pVideo_para->frame_rate, n32ParaBitRate);
        pVideo_para->bitrate.hivbr.bitrate = Aa_calculate_bitrate(ucResolution, ucQuality, pVideo_para->frame_rate, n32ParaBitRate);
        ucBitrate =ucBitrate;
#endif
#endif
        
        if( video_stream_type == _AST_MAIN_VIDEO_ )
        {
            int minBitRate = ((64 * 8) / 16); // 在共享内存中，录像写入64K为单位，预估最多20帧
            if( pVideo_para->bitrate.hicbr.bitrate > 0 && pVideo_para->bitrate.hicbr.bitrate < minBitRate )
            {
                pVideo_para->bitrate.hicbr.bitrate = minBitRate;
            }
        }

        /*******************get osd overlay para*********************/
#if defined(_AV_PRODUCT_CLASS_IPC_)
        if(NULL != m_pVideo_encoder_osd)
        {
#if 0
            printf("[alex]osd_show_flag:%d %d %d %d %d %d %d %d \n", m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].ucEnableChannal, \
                m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].ucEnableTime, m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].ucEnableVehicle, \
                m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].ucEnableDeviceId, m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].ucEnableGps,
                m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].ucEnableSpeed, m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[0].ucEnableOsd, \
                m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[1].ucEnableOsd);
#endif
        /*channel name osd*/
            pVideo_para->have_channel_name_osd = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].ucEnableChannal;
            pVideo_para->channel_name_osd_x = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usChannelX;
            pVideo_para->channel_name_osd_y = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usChannelY;
            memcpy(pVideo_para->channel_name, m_chn_name[phy_video_chn_num], sizeof(pVideo_para->channel_name));
            
            pVideo_para->channel_name[sizeof(pVideo_para->channel_name) - 1] = '\0';

            /*date time osd*/
            pVideo_para->have_date_time_osd = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].ucEnableTime;
            switch(m_pTime_setting->stuTimeSetting.ucDateMode)/**< 日期格式 0： MM/DD/YY 1： YY-MM-DD  2：DD-MM-YY */
            {
                case 0:
                    pVideo_para->date_mode = _AV_DATE_MODE_mmddyy_;
                    break;
                case 1:
                default:
                    pVideo_para->date_mode = _AV_DATE_MODE_yymmdd_;
                    break;
                case 2:
                    pVideo_para->date_mode = _AV_DATE_MODE_ddmmyy_;
                    break;
            }
            switch(m_pTime_setting->stuTimeSetting.ucTimeMode) /**< 时间格式 0：24小时制， 1：12小时制 */
            {
                case 0:
                default:
                    pVideo_para->time_mode = _AV_TIME_MODE_24_;
                    break;
                case 1:
                    pVideo_para->time_mode = _AV_TIME_MODE_12_;
                    break;
            }        
            pVideo_para->date_time_osd_x = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usTimeX;
            pVideo_para->date_time_osd_y = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usTimeY;

            /*the vehicle param setting*/
            pVideo_para->have_bus_number_osd = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].ucEnableVehicle; 
            pVideo_para->bus_number_osd_x = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usVehicleX;
            pVideo_para->bus_number_osd_y = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usVehicleY;
            strncpy(pVideo_para->bus_number, m_osd_content[0], sizeof(pVideo_para->bus_number));
            pVideo_para->bus_number[sizeof(pVideo_para->bus_number)-1]='\0';
            //自编号参数设置
            pVideo_para->have_bus_selfnumber_osd = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].ucEnableDeviceId; 
            pVideo_para->bus_selfnumber_osd_x = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usDeviceIdX;
            pVideo_para->bus_selfnumber_osd_y = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usDeviceIdY;
            strncpy(pVideo_para->bus_selfnumber, m_osd_content[3], sizeof(pVideo_para->bus_selfnumber));
            pVideo_para->bus_selfnumber[sizeof(pVideo_para->bus_selfnumber)-1]='\0';

            //GPS参数设置
            pVideo_para->have_gps_osd = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].ucEnableGps; 
            pVideo_para->gps_osd_x = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usGpsX;
            pVideo_para->gps_osd_y = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usGpsY;
            if(Aa_is_customer_cnr())
            {
                if(NULL != m_pIpAddress)
                {
                    snprintf(pVideo_para->gps, MAX_OSD_NAME_SIZE, "%s  0%d",  m_osd_content[1], (m_pIpAddress->stuAutoIp.ucIpAddr[3])%10);
                }
                else
                {
                    strncpy(pVideo_para->gps, m_osd_content[1], sizeof(pVideo_para->gps));
                }
            }
            else
            {
                strncpy(pVideo_para->gps, m_osd_content[1], sizeof(pVideo_para->gps));
            }
            
            pVideo_para->gps[sizeof(pVideo_para->gps)-1]='\0';


            //速度参数设置
            pVideo_para->have_speed_osd = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].ucEnableSpeed; 
            pVideo_para->speed_osd_x = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usSpeedX;
            pVideo_para->speed_osd_y = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usSpeedY;
            strncpy(pVideo_para->speed, m_osd_content[2], sizeof(pVideo_para->speed));
            pVideo_para->speed[sizeof(pVideo_para->speed)-1]='\0';

            //通用叠加1参数设置
            pVideo_para->have_common1_osd = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[0].ucEnableOsd;
            pVideo_para->osd1_contentfix = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[0].ucOsdContentFix;
            pVideo_para->osd1_content_byte = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[0].ucOsdContentLength;
            pVideo_para->common1_osd_x = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[0] .usOsdX;
            pVideo_para->common1_osd_y = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[0] .usOsdY;
            if(0 != pVideo_para->have_common1_osd)
            {
                strncpy(pVideo_para->osd1_content,m_ext_osd_content[0], sizeof(pVideo_para->osd1_content));
                pVideo_para->osd1_content[sizeof(pVideo_para->osd1_content) - 1] = '\0';
            } 
         
            //通用叠加2参数设置
            pVideo_para->have_common2_osd = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[1].ucEnableOsd;
            pVideo_para->osd2_contentfix = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[1].ucOsdContentFix;;
            pVideo_para->osd2_content_byte = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[1].ucOsdContentLength;
            pVideo_para->common2_osd_x = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[1] .usOsdX;
            pVideo_para->common2_osd_y = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[1] .usOsdY;
            if(0 != pVideo_para->have_common2_osd)
            {
                strncpy(pVideo_para->osd2_content,m_ext_osd_content[1], sizeof(pVideo_para->osd2_content));
                pVideo_para->osd2_content[sizeof(pVideo_para->osd2_content) -1] = '\0';
            }            

            /*CNR special requirement:  only overlay gps osd*/
            if((Aa_is_customer_cnr()))
            {
                pVideo_para->have_channel_name_osd = 0;
                pVideo_para->have_date_time_osd = 0;
                pVideo_para->have_gps_osd = 1;
                pVideo_para->have_speed_osd = 0;
                pVideo_para->have_bus_number_osd = 0;
                pVideo_para->have_water_mark_osd = 0;
                pVideo_para->have_bus_selfnumber_osd = 0;
                pVideo_para->have_common1_osd = 0;
                pVideo_para->have_common2_osd = 0;
            }  
#if 0
            char customer_name[32];
            memset(customer_name, 0 ,sizeof(customer_name));
            pgAvDeviceInfo->Adi_get_customer_name(customer_name, sizeof(customer_name));

             if(0 == strcmp(customer_name, "CUSTOMER_CQJY"))
             {
                pVideo_para->have_common1_osd = 1;
             }
#endif            
        }
#else
#if (defined(_AV_PLATFORM_HI3520D_V300_) || defined(_AV_PLATFORM_HI3515_))//6AII_AV12机型专用osd叠加
        if(OSD_TEST_FLAG == pgAvDeviceInfo->Adi_product_type() || PTD_6A_I == pgAvDeviceInfo->Adi_product_type())
        {
        	memcpy(&(pVideo_para->is_osd_change), m_bOsdChanged[phy_video_chn_num], sizeof(pVideo_para->is_osd_change));
            memcpy(&(pVideo_para->stuExtendOsdInfo), &(m_stuExtendOsdParam[phy_video_chn_num]), sizeof(pVideo_para->stuExtendOsdInfo));
        }
        else
#endif
        {
            if(NULL != m_pVideo_encoder_osd)
            {
                /*channel name osd*/
                pVideo_para->have_channel_name_osd = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].ucEnableChannal;
                pVideo_para->channel_name_osd_x = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usChannelX;
                pVideo_para->channel_name_osd_y = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usChannelY;
                memcpy(pVideo_para->channel_name, m_chn_name[phy_video_chn_num], MAX_OSD_NAME_SIZE);

                /*date time osd*/
                pVideo_para->have_date_time_osd = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].ucEnableTime;
                switch(m_pTime_setting->stuTimeSetting.ucDateMode)/**< 日期格式 0： MM/DD/YY 1： YY-MM-DD  2：DD-MM-YY */
                {
                    case 0:
                        pVideo_para->date_mode = _AV_DATE_MODE_mmddyy_;
                    break;
                    case 1:
                    default:
                        pVideo_para->date_mode = _AV_DATE_MODE_yymmdd_;
                    break;
                    case 2:
                        pVideo_para->date_mode = _AV_DATE_MODE_ddmmyy_;
                    break;
                }
                switch(m_pTime_setting->stuTimeSetting.ucTimeMode) /**< 时间格式 0：24小时制， 1：12小时制 */
                {
                    case 0:
                    default:
                        pVideo_para->time_mode = _AV_TIME_MODE_24_;
                    break;
                    case 1:
                        pVideo_para->time_mode = _AV_TIME_MODE_12_;
                    break;
                }        
                pVideo_para->date_time_osd_x = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usTimeX;
                pVideo_para->date_time_osd_y = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usTimeY;

                //车牌号参数设置
                pVideo_para->have_bus_number_osd = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].ucEnableVehicle; 
                pVideo_para->bus_number_osd_x = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usVehicleX;
                pVideo_para->bus_number_osd_y = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usVehicleY;
                memcpy(pVideo_para->bus_number, m_bus_number, MAX_OSD_NAME_SIZE);

                //自编号参数设置
                pVideo_para->have_bus_selfnumber_osd = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].ucEnableDeviceId; 
                pVideo_para->bus_selfnumber_osd_x = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usDeviceIdX;
                pVideo_para->bus_selfnumber_osd_y = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usDeviceIdY;
                memcpy(pVideo_para->bus_selfnumber, m_bus_selfsequeue_number, MAX_OSD_NAME_SIZE);

                //GPS参数设置
                pVideo_para->have_gps_osd = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].ucEnableGps; 
                pVideo_para->gps_osd_x = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usGpsX;
                pVideo_para->gps_osd_y = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usGpsY;

                //速度参数设置
                pVideo_para->have_speed_osd = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].ucEnableSpeed; 
                pVideo_para->speed_osd_x = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usSpeedX;
                pVideo_para->speed_osd_y = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].usSpeedY;

                //通用叠加1参数设置

                pVideo_para->have_common1_osd = 1;//m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[0].ucEnableOsd;
                pVideo_para->osd1_contentfix = 1;//m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[0].ucOsdContentFix;
                pVideo_para->osd1_content_byte = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[0].ucOsdContentLength;
                pVideo_para->common1_osd_x = m_ext_osd_x[0];//m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[0] .usOsdX;
                pVideo_para->common1_osd_y = m_ext_osd_y[0];// m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[0] .usOsdY;

                if(0 != pVideo_para->have_common1_osd)
                {
                    strncpy(pVideo_para->osd1_content,m_ext_osd_content[0], sizeof(pVideo_para->osd1_content));
                    pVideo_para->osd1_content[sizeof(pVideo_para->osd1_content) - 1] = '\0';
                } 

                //通用叠加2参数设置
                pVideo_para->have_common2_osd = 1;//m_bCommonOSD2Enable;//m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[0].ucEnableOsd;
                pVideo_para->osd2_contentfix = 1;//m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[0].ucOsdContentFix;
                pVideo_para->osd2_content_byte = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[0].ucOsdContentLength;
                pVideo_para->common2_osd_x = m_ext_osd_x[1];//m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[0] .usOsdX;
                pVideo_para->common2_osd_y = m_ext_osd_y[1];// m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[0] .usOsdY;

                if(0 != pVideo_para->have_common2_osd)
                {
                    strncpy(pVideo_para->osd2_content,m_ext_osd_content[1], sizeof(pVideo_para->osd2_content));
                    pVideo_para->osd2_content[sizeof(pVideo_para->osd2_content) - 1] = '\0';
                }  
#if !defined(_AV_PRODUCT_CLASS_DVR_REI_)

                //! for 04.471 to overlay "KenKart" watermark, here use that of common osd2 as watermark param
                char cutomername[32] = {0};
                pgAvDeviceInfo->Adi_get_customer_name(cutomername, sizeof(cutomername));
                if (strcmp(cutomername, "CUSTOMER_04.471") == 0)
                {
                    pVideo_para->have_common2_osd = 0;
                    pVideo_para->have_water_mark_osd = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[1].ucEnableOsd;
                    pVideo_para->water_mark_osd_x = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[1].usOsdX; //768;//
                    pVideo_para->water_mark_osd_y = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[1].usOsdY; //40;//
                    strncpy(pVideo_para->water_mark_name,
                           m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[1].szOsdContent, 
                           sizeof(pVideo_para->water_mark_name)); 

                    pVideo_para->osd_transparency = m_pVideo_encoder_osd->stuEncodeOsdSetting[phy_video_chn_num].stuCommonOsd[1].ucOsdTransparency;
                }
#endif	            
            }
    	}
#endif
    char customer_name[32] = {0};
    pgAvDeviceInfo->Adi_get_customer_name(customer_name, sizeof(customer_name));

    if (!strcmp(customer_name, "CUSTOMER_BJGJ")) //!< right alignment, 20160613
    {
        pVideo_para->date_time_osd_x = 1024;
        //pVideo_para->speed_osd_x = 1024;
        //pVideo_para->gps_osd_x = 1024;
    }


    }
    /*audio*/
    if((phy_audio_chn_num >= 0) && (pAudio_para != NULL))
    {
        memset(pAudio_para, 0, sizeof(av_audio_encoder_para_t));
#ifdef _AV_PRODUCT_CLASS_IPC_
        if(NULL != m_pAudio_param)
        {
            switch(m_pAudio_param->stuAudioSetting.u8AudioType)
            {
                case 0:
                default:
                {
                    pAudio_para->type = _AET_G711A_;
                }
                    break;
                case 1:
                {
                    pAudio_para->type = _AET_G711U_;
                }
                    break;
                case 2:
                {
                    pAudio_para->type = _AET_ADPCM_;
                }
                    break;
                case 3:
                {
                    pAudio_para->type = _AET_G726_;
                }
                    break;
            }
        }
        else
        {
            pAudio_para->type = _AET_G711A_; //IPC default AUDIO type is G711A
        }
#else// not ipc
#ifdef _AV_USE_AACENC_
        pAudio_para->type = _AET_AAC_;
#else
#ifdef _AV_HAVE_PURE_AUDIO_CHANNEL
		if(PTD_P1 == pgAvDeviceInfo->Adi_product_type())
		{
			pAudio_para->type = _AET_LPCM_;	//设置P1的编码方式///
		}
		else
#endif
		{
			pAudio_para->type = _AET_ADPCM_;
		}
#endif
//! for CB check criterion, G726 is only used
        char check_criterion[32]={0};
        pgAvDeviceInfo->Adi_get_check_criterion(check_criterion, sizeof(check_criterion));
        if (!strcmp(check_criterion, "CHECK_CB"))
        {
            pAudio_para->type = _AET_G726_;
        }

        char cutomername[32] = {0};
        pgAvDeviceInfo->Adi_get_customer_name(cutomername, sizeof(cutomername));
        if (strcmp(cutomername, "CUSTOMER_JIUTONG") == 0)
        {
            pAudio_para->type = _AET_G726_;
        }
        
#endif

    }

    return 0;
}

int CAvAppMaster::Aa_get_local_encoder_chn_mask(av_video_stream_type_t video_stream_type, unsigned long long int *pulllocal_encoder_chn_mask)
{
    if(_AST_UNKNOWN_ == video_stream_type || NULL == pulllocal_encoder_chn_mask)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_get_local_encoder_chn_mask() invalid video_stream_type = %d or pulllocal_encoder_chn_mask == NULL\n", video_stream_type);
        return -1;
    }

    if(0 == *pulllocal_encoder_chn_mask)
    {
        DEBUG_ERROR( "NO CHN NEEDS TO START ENCODER\n");
        return 0;
    }

    const int max_vi_chn_num = pgAvDeviceInfo->Adi_get_vi_chn_num();
    
    switch(video_stream_type)
    {
        case _AST_MAIN_VIDEO_:
            for(int chn_index = 0; chn_index < max_vi_chn_num; chn_index++)
            {
                if(GCL_BIT_VAL_TEST((*pulllocal_encoder_chn_mask), chn_index))
                {
                    if(!m_pMain_stream_encoder->stuVideoEncodeSetting[chn_index].ucChnEnable)
                    {
                        GCL_BIT_VAL_CLEAR((*pulllocal_encoder_chn_mask), chn_index);
                    }
                }
            }
            break;
        
        case _AST_SUB_VIDEO_:
            for(int chn_index = 0; chn_index < max_vi_chn_num; chn_index++)
            {
                if(GCL_BIT_VAL_TEST((*pulllocal_encoder_chn_mask), chn_index))
                {
                    if((m_pSub_stream_encoder != NULL) && !m_pSub_stream_encoder->stuVideoEncodeSetting[chn_index].ucChnEnable && !m_net_start_substream_enable[chn_index])
                    {
                        GCL_BIT_VAL_CLEAR((*pulllocal_encoder_chn_mask), chn_index);
                    }
                }
            }
            break;
        
        case _AST_SUB2_VIDEO_:
            for(int chn_index = 0; chn_index < max_vi_chn_num; chn_index++)
            {
                if(GCL_BIT_VAL_TEST((*pulllocal_encoder_chn_mask), chn_index))
                {
                    if(!m_pSub2_stream_encoder->stuVideoEncodeSetting[chn_index].ucChnEnable)
                    {
                        GCL_BIT_VAL_CLEAR((*pulllocal_encoder_chn_mask), chn_index);
                    }
                }
            }
            break;
            
        default:
            break;
    }
    
    return 0;
}


int CAvAppMaster::Aa_start_selected_streamtype_encoders(unsigned long long int chn_mask, unsigned char streamtype_mask, bool bcheck_flag)
{
    if(0 == chn_mask || 0 == streamtype_mask)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_start_selected_streamtype_encoders(chn_mask = %llu, streamtype_mask = %d)\n",chn_mask, streamtype_mask);   
        return 0;
    }
    const int max_vi_chn_num = pgAvDeviceInfo->Adi_get_vi_chn_num();
    unsigned long long int ulllocal_encoder_chn_mask = 0;
    av_video_encoder_para_t av_video_para[max_vi_chn_num];
    av_audio_encoder_para_t av_audio_para[max_vi_chn_num];
#if !defined(_AV_PRODUCT_CLASS_DVR_REI_)
    for(int video_stream_type_num = _AST_MAIN_VIDEO_; video_stream_type_num < _AST_UNKNOWN_; video_stream_type_num++)
#else
    for(int video_stream_type_num = _AST_MAIN_VIDEO_; video_stream_type_num < _AST_SUB2_VIDEO_; video_stream_type_num++)
#endif
    {
        if(GCL_BIT_VAL_TEST(streamtype_mask, video_stream_type_num))
        {
            ulllocal_encoder_chn_mask = chn_mask;
            if(bcheck_flag)
            {
                if(0 != Aa_get_local_encoder_chn_mask(static_cast<av_video_stream_type_t>(video_stream_type_num), &ulllocal_encoder_chn_mask))
                {
                    DEBUG_ERROR( "CAvAppMaster::Aa_get_local_encoder_chn_mask() failed\n");
                    return -1;
                }
            }
            if(0 != Aa_init_video_audio_para(av_video_para, av_audio_para, ulllocal_encoder_chn_mask, max_vi_chn_num, static_cast<av_video_stream_type_t>(video_stream_type_num)))
            {
                DEBUG_ERROR( "CAvAppMaster::Aa_init_video_audio_para() failed\n");
                return -1;
            }
            if(0 != m_pAv_device->Ad_start_selected_encoder(ulllocal_encoder_chn_mask, av_video_para, av_audio_para, static_cast<av_video_stream_type_t>(video_stream_type_num), _AAST_NORMAL_))
            {
                DEBUG_ERROR( "CAvAppMaster::Ad_start_selected_encoder() failed\n");
                return -1;
            }
            if(0 != Aa_set_encoders_state(ulllocal_encoder_chn_mask, max_vi_chn_num, true, static_cast<av_video_stream_type_t>(video_stream_type_num)))
            {
                DEBUG_ERROR( "CAvAppMaster::Aa_set_encoders_state() failed\n");
                return -1;
            }
        }
    }
    
    return 0;
}

int CAvAppMaster::Aa_start_encoders_for_parameter_change(unsigned long long int start_encoder_chn_mask[_AST_UNKNOWN_], bool bcheck_flag)
{  
    const int max_vi_chn_num = pgAvDeviceInfo->Adi_get_vi_chn_num();
    unsigned long long int ulllocal_encoder_chn_mask = 0;
    av_video_encoder_para_t av_video_para[max_vi_chn_num];
    av_audio_encoder_para_t av_audio_para[max_vi_chn_num];
    for(int video_stream_type_num = _AST_MAIN_VIDEO_; video_stream_type_num < _AST_UNKNOWN_; video_stream_type_num++)
    {
        if (_AST_SNAP_VIDEO_ == video_stream_type_num)
        {
            continue;
        }
        if(0 != start_encoder_chn_mask[video_stream_type_num])
        {

            ulllocal_encoder_chn_mask = start_encoder_chn_mask[video_stream_type_num];
            if(bcheck_flag)
            {
                if(0 != Aa_get_local_encoder_chn_mask(static_cast<av_video_stream_type_t>(video_stream_type_num), &ulllocal_encoder_chn_mask))
                {
                    DEBUG_ERROR( "CAvAppMaster::Aa_get_local_encoder_chn_mask() failed\n");
                    return -1;
                }
            }
            
            if(0 != Aa_init_video_audio_para(av_video_para, av_audio_para, ulllocal_encoder_chn_mask, max_vi_chn_num, static_cast<av_video_stream_type_t>(video_stream_type_num)))
            {
                DEBUG_ERROR( "CAvAppMaster::Aa_init_video_audio_para() failed\n");
                return -1;
            }
            
            if(0 != m_pAv_device->Ad_start_selected_encoder(ulllocal_encoder_chn_mask, av_video_para, av_audio_para, static_cast<av_video_stream_type_t>(video_stream_type_num), _AAST_NORMAL_))
            {
                DEBUG_ERROR( "CAvAppMaster::Ad_start_selected_encoder() failed\n");
                return -1;
            }

            if(0 != Aa_set_encoders_state(ulllocal_encoder_chn_mask, max_vi_chn_num, true, static_cast<av_video_stream_type_t>(video_stream_type_num)))
            {
                DEBUG_ERROR( "CAvAppMaster::Aa_set_encoders_state() failed\n");
                return -1;
            }
        }
    }

    return 0;
}

int CAvAppMaster::Aa_set_encoders_state(unsigned long long int chn_mask, int max_vi_chn_num, bool start_or_stop, av_video_stream_type_t video_stream_type, av_audio_stream_type_t audio_stream_type)
{
    if(_AST_UNKNOWN_ == video_stream_type)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_set_encoders_state() video_stream_type(%d) invalid\n", video_stream_type);
    }

    for(int chn_num = 0; chn_num < max_vi_chn_num; chn_num++)
    {
        if(GCL_BIT_VAL_TEST(chn_mask, chn_num))
        {
            switch(video_stream_type)
            {
                case _AST_MAIN_VIDEO_:
                     m_main_encoder_state[chn_num] = start_or_stop;
                     break;
                case _AST_SUB_VIDEO_:
                     m_sub_encoder_state[chn_num] = start_or_stop;
                     break;
                case _AST_SUB2_VIDEO_:
                     m_sub2_encoder_state[chn_num] = start_or_stop;
                     break;
                default:
                     break;
            }
        }
    }
    return 0;

}

#if !defined(_AV_PRODUCT_CLASS_DVR_REI_)
void CAvAppMaster::Aa_message_handle_MAINSTREAMPARA(const char *msg, int length)
{    
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    msgVideoEncodeSetting_t *pMain_stream = NULL;
    int chn_index;
    unsigned long long int vi_stop_mask = 0;
    unsigned long long int vi_start_mask = 0;

#if !defined(_AV_PRODUCT_CLASS_IPC_)    
    char ChannelMap[SUPPORT_MAX_VIDEO_CHANNEL_NUM];
    m_pAv_device_master->Ad_ObtainChannelMap(ChannelMap);
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_) && defined(_AV_PLATFORM_HI3518C_)
        char customer_name[32] = {0};

        memset(customer_name, 0 ,sizeof(customer_name));
        pgAvDeviceInfo->Adi_get_customer_name(customer_name, sizeof(customer_name));
#endif


    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        m_start_work_env[MESSAGE_CONFIG_MANAGE_MAIN_STREAM_PARAM_GET] = true;
        
        pMain_stream = (msgVideoEncodeSetting_t *)pMessage_head->body;
        DEBUG_CRITICAL( "Aa_message_handle_MAINSTREAMPARA called \n");  
        for (chn_index = 0; chn_index < pgAvDeviceInfo->Adi_get_vi_chn_num(); chn_index++)
        {
        
#if defined(_AV_PRODUCT_CLASS_IPC_) && defined(_AV_PLATFORM_HI3518C_)
            if((PTD_712_VA == pgAvDeviceInfo->Adi_product_type()) && (0 == strcmp(customer_name, "CUSTOMER_01.01")))
            {
                //712C3_VA of customer 01.01,the res 8 represents XGA in main stream para
                if(8 == pMain_stream->stuVideoEncodeSetting[chn_index].ucResolution)
                {
                    pMain_stream->stuVideoEncodeSetting[chn_index].ucResolution = 17;
                }
            }
#endif

            if (m_start_work_flag != 0) 
            {
                if (m_pMain_stream_encoder == NULL)
                {
                    goto jump ;
                }

                if(pMain_stream->stuVideoEncodeSetting[chn_index].ucChnEnable!=m_pMain_stream_encoder->stuVideoEncodeSetting[chn_index].ucChnEnable)
                {
                    if (pMain_stream->stuVideoEncodeSetting[chn_index].ucChnEnable == 0)
                    {

                        GCL_BIT_VAL_SET(vi_stop_mask, chn_index); //!
                        //GCL_BIT_VAL_SET(m_alarm_status.videoLost, chn_index);

                    }
                    else
                    {
                        GCL_BIT_VAL_SET(vi_start_mask, chn_index);
                        //GCL_BIT_VAL_CLEAR(m_alarm_status.videoLost, chn_index);
                    }
                }

                jump:
                ;
            }
            else
            {
                if (pMain_stream->stuVideoEncodeSetting[chn_index].ucChnEnable == 0)
                {

                    GCL_BIT_VAL_SET(vi_stop_mask, chn_index); //!
                    //GCL_BIT_VAL_SET(m_alarm_status.videoLost, chn_index);

                }
                else
                {
                    GCL_BIT_VAL_SET(vi_start_mask, chn_index);
                    //GCL_BIT_VAL_CLEAR(m_alarm_status.videoLost, chn_index);
                }

            }
            if (GCL_BIT_VAL_TEST(pMain_stream->mask, chn_index))
            {
                if (pgAvDeviceInfo->Adi_get_video_source(chn_index) == _VS_DIGITAL_)
                {
                    continue;
                }
                Aa_check_and_reset_encoder_param(&pMain_stream->stuVideoEncodeSetting[chn_index]);

            }

        }
        //!
        for (chn_index = 0; chn_index < pgAvDeviceInfo->Adi_max_channel_number(); chn_index++)    
        {
          if (GCL_BIT_VAL_TEST(pMain_stream->mask, chn_index))
            {
                //DEBUG_CRITICAL("All channel[%d] enable:%d \n",chn_index, pMain_stream->stuVideoEncodeSetting[chn_index].ucChnEnable);

                DEBUG_WARNING("Digital channel[%d] enable:%d \n",chn_index, pMain_stream->stuVideoEncodeSetting[chn_index].ucChnEnable);
                    if (pMain_stream->stuVideoEncodeSetting[chn_index].ucChnEnable == 0)
                    {
#if !defined(_AV_PRODUCT_CLASS_IPC_)
                        ChannelMap[chn_index] = (char)-1;
#endif
                        m_chn_state[chn_index] = 0;
                    }
                    else
                    {
 #if defined(_AV_PRODUCT_CLASS_IPC_)      
                        if(chn_index >= pgAvDeviceInfo->Adi_get_vi_chn_num())
 #else
                        if ((0 != m_start_work_flag) &&( chn_index >= pgAvDeviceInfo->Adi_get_vi_chn_num())&& (ChannelMap[chn_index] == 0xff))
#endif
                        {
 #if !defined(_AV_PRODUCT_CLASS_IPC_)                   
                            if (1 == m_chn_state[chn_index]) //only when there exists difference, signaling channelmap closed 
                            {
                                ChannelMap[chn_index] = (char)-1;  
                            }
                            else
                            {
                               ChannelMap[chn_index] = chn_index;
                            }
#endif
                            m_chn_state[chn_index] = 1; //this only indicate the mainstream state
                        }
                        else
                        {
#if !defined(_AV_PRODUCT_CLASS_IPC_)
                            ChannelMap[chn_index] = chn_index;
#endif
                            m_chn_state[chn_index] = 1;  
                        }

                }
    
            }
        }
      
 //! Added for IPC 
#if defined( _AV_PRODUCT_CLASS_IPC_)
#if defined(_AV_PLATFORM_HI3516A_)
        IspOutputMode_e isp_output_mode;
        if(0 == m_start_work_flag)
        {
            isp_output_mode = Aa_calculate_isp_out_put_mode(pMain_stream->stuVideoEncodeSetting[0].ucResolution, pMain_stream->stuVideoEncodeSetting[0].ucFrameRate);
            pgAvDeviceInfo->Adi_set_isp_output_mode(isp_output_mode);
            m_u8SourceFrameRate = Aa_convert_framerate_from_IOM(isp_output_mode);
        }
        else
        {
            if(Aa_is_reset_isp(pMain_stream->stuVideoEncodeSetting[0].ucResolution, pMain_stream->stuVideoEncodeSetting[0].ucFrameRate, isp_output_mode))
            {
#if defined(_AV_HAVE_ISP_)
                Aa_reset_isp(isp_output_mode);
#endif
            }             
        }
#else
        IspOutputMode_e isp_output_mode;
        isp_output_mode = Aa_calculate_isp_out_put_mode(pMain_stream->stuVideoEncodeSetting[0].ucResolution, pMain_stream->stuVideoEncodeSetting[0].ucFrameRate);
        pgAvDeviceInfo->Adi_set_isp_output_mode(isp_output_mode);
        m_u8SourceFrameRate = Aa_convert_framerate_from_IOM(isp_output_mode);

#if defined(_AV_PLATFORM_HI3518C_)
        if(PTD_913_VA != pgAvDeviceInfo->Adi_product_type())
        {
            Aa_set_isp_framerate(m_av_tvsystem,  pMain_stream->stuVideoEncodeSetting[0].ucFrameRate);
        }
#endif             
#endif
#endif
        if (m_pMain_stream_encoder != NULL)
        {
            if (m_start_work_flag != 0)   // already start
            {              
#if !defined(_AV_PRODUCT_CLASS_IPC_)
#ifndef _AV_PLATFORM_HI3515_
				unsigned long long start_encoder_chn_mask[_AST_UNKNOWN_] = {0};
				unsigned long long stop_encoder_chn_mask[_AST_UNKNOWN_] = {0};
				for(int i = 0 ; i < _AST_UNKNOWN_ ; ++i)	//first stop encoder, stop main vi is to stop all stream type vi ///
				{
					stop_encoder_chn_mask[i] = vi_stop_mask;
				}
				start_encoder_chn_mask[_AST_MAIN_VIDEO_] = vi_start_mask;	//only start main encoder//
				Aa_stop_encoders_for_parameters_change(stop_encoder_chn_mask, false);
                DEBUG_ERROR("CXLIU stop vi:%llx \n", vi_stop_mask);
                m_pAv_device_master->Ad_stop_selected_vi(vi_stop_mask);
              
                DEBUG_ERROR("CXLIU start vi:%llx \n", vi_start_mask);
                Aa_start_selected_vi(vi_start_mask);
				Aa_start_encoders_for_parameter_change(start_encoder_chn_mask, false);
#endif
                for (int ch=0; ch < pgAvDeviceInfo->Adi_get_vi_chn_num(); ch++)
                {
                    if (pgAvDeviceInfo->Adi_get_video_source(ch) == _VS_DIGITAL_ )
                    {
                        continue;
                    }
                    else if( vi_stop_mask&(1LL<<ch))
                    {
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
                        m_pAv_device_master->Ad_stop_preview_output_chn(_AOT_MAIN_,0, ch);
#endif
                    }
                }

                //! reset preview according to the playstate
#if defined(_AV_HAVE_VIDEO_DECODER_) || defined(_AV_HAVE_VIDEO_OUTPUT_)
                bool reset_preview = false;//true;
#endif
                                
#if defined(_AV_HAVE_VIDEO_DECODER_)
                if (m_playState != PS_INVALID)
                {/*处于回放状态不重置preview*/
                    reset_preview = false;
                }
#endif
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
                /*start vo preview*/   
                if (true == reset_preview)
                {

                m_pAv_device_master->Ad_stop_selected_preview_output( pgAvDeviceInfo->Adi_get_main_out_mode(), 0, vi_stop_mask, true);

                //! start vi
                int chn_layout[_AV_SPLIT_MAX_CHN_];
                av_split_t split_mode = Aa_get_split_mode(m_pScreenMosaic->iMode);
                int max_chn_num = pgAvDeviceInfo->Adi_max_channel_number();
                int margin[4];
                
                if (split_mode == _AS_UNKNOWN_)
                {
                    DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_MAINSTREAMPARA Aa_get_split_mode FAILED! (split_mode:%d)\n", split_mode);
                    split_mode = _AS_4SPLIT_;
                    memset(chn_layout, -1, sizeof(int) * _AV_SPLIT_MAX_CHN_);
                    for(int index = 0; index < 4; index ++)
                    {
                        chn_layout[index] = index;
                    }
                }
                else
                {
                    memset(chn_layout, -1, sizeof(int) * _AV_SPLIT_MAX_CHN_);
                    for(int index = 0; index < max_chn_num; index ++)
                    {
                        if((m_pScreenMosaic->chn[index] >= 0) && (m_pScreenMosaic->chn[index] <= pgAvDeviceInfo->Adi_max_channel_number()))
                        {
                            chn_layout[index] = m_pScreenMosaic->chn[index];
                        }
                    }
                }

                margin[_AV_MARGIN_BOTTOM_INDEX_] = m_pScreen_Margin->usBottom;
                margin[_AV_MARGIN_LEFT_INDEX_] = m_pScreen_Margin->usLeft;
                margin[_AV_MARGIN_RIGHT_INDEX_] = m_pScreen_Margin->usRight;
                margin[_AV_MARGIN_TOP_INDEX_] = m_pScreen_Margin->usTop;

                if(m_pAv_device_master->Ad_start_selected_preview_output(pgAvDeviceInfo->Adi_get_main_out_mode(), 0, split_mode, chn_layout, vi_start_mask, true, margin) != 0)
                {
                    DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_MAINSTREAMPARA m_pAv_device_master->Ad_start_preview_output FAIELD\n");

                }
                }
#endif
               // m_pAv_device_master->Ad_vi_insert_picture(m_alarm_status.videoLost);
               
                m_pAv_device_master->Ad_UpdateDevMap(ChannelMap);
#endif
                Aa_modify_encoder_para(_AST_MAIN_VIDEO_, pMain_stream);
            }
            else
            {
                m_pMain_stream_encoder->mask = pMain_stream->mask;
                for (int chn_index = 0; chn_index < pgAvDeviceInfo->Adi_max_channel_number(); chn_index++)
                {
                    if (GCL_BIT_VAL_TEST(m_pMain_stream_encoder->mask, chn_index))
                    {
                        if (_VS_ANALOG_ == pgAvDeviceInfo->Adi_get_video_source(chn_index) && 0 != memcmp(m_pMain_stream_encoder->stuVideoEncodeSetting + chn_index, pMain_stream->stuVideoEncodeSetting + chn_index, sizeof(paramVideoEncode_t)))
                        {
                            memcpy(m_pMain_stream_encoder->stuVideoEncodeSetting + chn_index, pMain_stream->stuVideoEncodeSetting + chn_index, sizeof(paramVideoEncode_t));
                        }
                    }
                }
                
            }
        }
        else
        {
            _AV_FATAL_((m_pMain_stream_encoder = new msgVideoEncodeSetting_t) == NULL, "CAvAppMaster::Aa_message_handle_MAINSTREAMPARA OUT OF MEMORY\n");            
            memcpy(m_pMain_stream_encoder, pMain_stream, sizeof(msgVideoEncodeSetting_t));
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_MAINSTREAMPARA invalid message length\n");
    }
}

#else
void CAvAppMaster::Aa_message_handle_MAINSTREAMPARA_REI(const char *msg, int length)
{    
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    msgVideoEncodeSetting_t *pMain_stream = NULL;
    int chn_index;
    unsigned long long int vi_stop_mask = 0;
    unsigned long long int vi_start_mask = 0;

    unsigned long long int vi_stop_mask_rei = 0;
    unsigned long long int vi_start_mask_rei = 0;

#if !defined(_AV_PRODUCT_CLASS_IPC_)    
    char ChannelMap[SUPPORT_MAX_VIDEO_CHANNEL_NUM];
    m_pAv_device_master->Ad_ObtainChannelMap(ChannelMap);
#endif
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        m_start_work_env[MESSAGE_CONFIG_MANAGE_MAIN_STREAM_PARAM_GET] = true;
        
        pMain_stream = (msgVideoEncodeSetting_t *)pMessage_head->body;
        DEBUG_CRITICAL( "Aa_message_handle_MAINSTREAMPARA_REI called \n");  
        for (chn_index = 0; chn_index < pgAvDeviceInfo->Adi_get_vi_chn_num(); chn_index++)
        {
                if (m_start_work_flag != 0) 
	        {
	               if (m_pMain_stream_encoder == NULL)
	            	{
					goto jump ;
	            	}
				   
	            if(pMain_stream->stuVideoEncodeSetting[chn_index].ucChnEnable!=m_pMain_stream_encoder->stuVideoEncodeSetting[chn_index].ucChnEnable)
		     {
		            if (pMain_stream->stuVideoEncodeSetting[chn_index].ucChnEnable == 0)
		            {

		                GCL_BIT_VAL_SET(vi_stop_mask, chn_index); //!
		                //GCL_BIT_VAL_SET(m_alarm_status.videoLost, chn_index);
		                                 
		            }
		            else
		            {
		                GCL_BIT_VAL_SET(vi_start_mask, chn_index);
		                //GCL_BIT_VAL_CLEAR(m_alarm_status.videoLost, chn_index);
		            }
		     }
								
jump:				
			;
		}
		else
		{
	            if (pMain_stream->stuVideoEncodeSetting[chn_index].ucChnEnable == 0)
	            {

	                GCL_BIT_VAL_SET(vi_stop_mask, chn_index); //!
	                //GCL_BIT_VAL_SET(m_alarm_status.videoLost, chn_index);
	                                 
	            }
	            else
	            {
	                GCL_BIT_VAL_SET(vi_start_mask, chn_index);
	                //GCL_BIT_VAL_CLEAR(m_alarm_status.videoLost, chn_index);
	            }
					     
		}
	            if (GCL_BIT_VAL_TEST(pMain_stream->mask, chn_index))
	            {
	                if (pgAvDeviceInfo->Adi_get_video_source(chn_index) == _VS_DIGITAL_)
	                {
	                    continue;
	                }
	                Aa_check_and_reset_encoder_param(&pMain_stream->stuVideoEncodeSetting[chn_index]);
	    
	            }
	        	
        }
        //!
        for (chn_index = 0; chn_index < pgAvDeviceInfo->Adi_max_channel_number(); chn_index++)    
        {
           if (pgAvDeviceInfo->Adi_get_video_source(chn_index) != _VS_DIGITAL_ )
           {
	            if (pMain_stream->stuVideoEncodeSetting[chn_index].ucLiveChnEnable == 0)
	            {
	              GCL_BIT_VAL_SET(vi_stop_mask_rei, chn_index); //!
			      m_pAv_device_master->Ad_set_preview_enable_switch(chn_index, 0);		  
		 	      printf("\nAd_stop_preview_output_chn,ch=%d\n",chn_index);
	            }
	            else
	            {
	              GCL_BIT_VAL_SET(vi_start_mask_rei, chn_index); //!
	              m_pAv_device_master->Ad_set_preview_enable_switch(chn_index, 1);	
		 	      printf("\nAd_start_preview_output_chn,ch=%d\n",chn_index);            					
	            }
          }

          if (GCL_BIT_VAL_TEST(pMain_stream->mask, chn_index))
            {
                //DEBUG_CRITICAL("All channel[%d] enable:%d \n",chn_index, pMain_stream->stuVideoEncodeSetting[chn_index].ucChnEnable);

                DEBUG_WARNING("Digital channel[%d] enable:%d \n",chn_index, pMain_stream->stuVideoEncodeSetting[chn_index].ucChnEnable);
                    if((pMain_stream->stuVideoEncodeSetting[chn_index].ucChnEnable == 0)||(pMain_stream->stuVideoEncodeSetting[chn_index].ucLiveChnEnable == 0))
                    {
#if !defined(_AV_PRODUCT_CLASS_IPC_)
                        ChannelMap[chn_index] = -1;
#endif
                        m_chn_state[chn_index] = 0;
                    }
                    else
                    {
 #if defined(_AV_PRODUCT_CLASS_IPC_)      
                        if(chn_index >= pgAvDeviceInfo->Adi_get_vi_chn_num())
 #else
                        if ((0 != m_start_work_flag) &&( chn_index >= pgAvDeviceInfo->Adi_get_vi_chn_num())&& (ChannelMap[chn_index] == 0xff))
#endif
                        {
 #if !defined(_AV_PRODUCT_CLASS_IPC_)                   
                            if (1 == m_chn_state[chn_index]) //only when there exists difference, signaling channelmap closed 
                            {
                                ChannelMap[chn_index] = -1;  
                            }
                            else
                            {
                               ChannelMap[chn_index] = chn_index;
                            }
#endif
                            m_chn_state[chn_index] = 1; //this only indicate the mainstream state
                        }
                        else
                        {
#if !defined(_AV_PRODUCT_CLASS_IPC_)
                            ChannelMap[chn_index] = chn_index;
#endif
                            m_chn_state[chn_index] = 1;  
                        }

                }
    
            }
        }
      
        
        
//! Added for IPC 
#if defined( _AV_PRODUCT_CLASS_IPC_)
#if defined(_AV_PLATFORM_HI3516A_)
        IspOutputMode_e isp_output_mode;
        if(0 != m_start_work_flag)
        {
            if(Aa_is_reset_isp(pMain_stream->stuVideoEncodeSetting[0].ucResolution, pMain_stream->stuVideoEncodeSetting[0].ucFrameRate, isp_output_mode))
            {
                Aa_reset_isp(isp_output_mode);
            }            
        }
        else
        {
            isp_output_mode = Aa_calculate_isp_out_put_mode(pMain_stream->stuVideoEncodeSetting[0].ucResolution, pMain_stream->stuVideoEncodeSetting[0].ucFrameRate);
            pgAvDeviceInfo->Adi_set_isp_output_mode(isp_output_mode);
            m_u8SourceFrameRate = Aa_convert_framerate_from_IOM(isp_output_mode);
        }
#elif defined(_AV_PLATFORM_HI3518EV200_)
        IspOutputMode_e isp_output_mode;
        isp_output_mode = Aa_calculate_isp_out_put_mode(pMain_stream->stuVideoEncodeSetting[0].ucResolution, pMain_stream->stuVideoEncodeSetting[0].ucFrameRate);
        pgAvDeviceInfo->Adi_set_isp_output_mode(isp_output_mode);
        m_u8SourceFrameRate = Aa_convert_framerate_from_IOM(isp_output_mode);
#elif defined(_AV_PLATFORM_HI3518C_) 
        if(PTD_913_VA == pgAvDeviceInfo->Adi_product_type())
        {
            IspOutputMode_e isp_output_mode;
            isp_output_mode = Aa_calculate_isp_out_put_mode(pMain_stream->stuVideoEncodeSetting[0].ucResolution, pMain_stream->stuVideoEncodeSetting[0].ucFrameRate);
            pgAvDeviceInfo->Adi_set_isp_output_mode(isp_output_mode);
            m_u8SourceFrameRate = Aa_convert_framerate_from_IOM(isp_output_mode);
        }
        else
        {
            if(0 != m_start_work_flag)
            {   
                Aa_set_isp_framerate(m_av_tvsystem,  pMain_stream->stuVideoEncodeSetting[0].ucFrameRate);
            }   
        }
#endif
#endif
        
        if (m_pMain_stream_encoder != NULL)
        {
            if (m_start_work_flag != 0)   // already start
            {              
#if !defined(_AV_PRODUCT_CLASS_IPC_)
                DEBUG_ERROR("CXLIU stop vi:%llx \n", vi_stop_mask);
                m_pAv_device_master->Ad_stop_selected_vi(vi_stop_mask);
              
                DEBUG_ERROR("CXLIU start vi:%llx \n", vi_start_mask);
                Aa_start_selected_vi(vi_start_mask);

                DEBUG_ERROR("vi_stop_mask_rei:%llx,vi_start_mask_rei:%llx\n", vi_stop_mask_rei,vi_start_mask_rei);
                if (m_playState == PS_INVALID)
                {
                for (int ch=0; ch < pgAvDeviceInfo->Adi_get_vi_chn_num(); ch++)
                {
                    if (pgAvDeviceInfo->Adi_get_video_source(ch) == _VS_DIGITAL_ )
                    {
                        continue;
                    }
                    else if( vi_stop_mask&(1LL<<ch))
                    {
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
                        m_pAv_device_master->Ad_stop_preview_output_chn(_AOT_MAIN_,0, ch);  //! 0428 
#endif
                    }
                    else if( vi_stop_mask_rei&(1LL<<ch))
                    {
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
                        m_pAv_device_master->Ad_stop_preview_output_chn(_AOT_MAIN_,0, ch);
#endif
                    }
                    else if( vi_start_mask_rei&(1LL<<ch))
                    {
                        m_pAv_device_master->Ad_start_preview_output_chn(_AOT_MAIN_,0, ch);
                        }					
                    }					
                }

                //! reset preview according to the playstate
#if defined(_AV_HAVE_VIDEO_DECODER_) || defined(_AV_HAVE_VIDEO_OUTPUT_)
                bool reset_preview = false;//true;
#endif
                                
#if defined(_AV_HAVE_VIDEO_DECODER_)
                if (m_playState != PS_INVALID)
                {/*处于回放状态不重置preview*/
                    reset_preview = false;
                }
#endif
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
                /*start vo preview*/   
                if (true == reset_preview)
                {

                m_pAv_device_master->Ad_stop_selected_preview_output( pgAvDeviceInfo->Adi_get_main_out_mode(), 0, vi_stop_mask, true);

                //! start vi
                int chn_layout[_AV_SPLIT_MAX_CHN_];
                av_split_t split_mode = Aa_get_split_mode(m_pScreenMosaic->iMode);
                int max_chn_num = pgAvDeviceInfo->Adi_max_channel_number();
                int margin[4];
                
                if (split_mode == _AS_UNKNOWN_)
                {
                    DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_MAINSTREAMPARA_REI Aa_get_split_mode FAILED! (split_mode:%d)\n", split_mode);
                    split_mode = _AS_4SPLIT_;
                    memset(chn_layout, -1, sizeof(int) * _AV_SPLIT_MAX_CHN_);
                    for(int index = 0; index < 4; index ++)
                    {
                        chn_layout[index] = index;
                    }
                }
                else
                {
                    memset(chn_layout, -1, sizeof(int) * _AV_SPLIT_MAX_CHN_);
                    for(int index = 0; index < max_chn_num; index ++)
                    {
                        if((m_pScreenMosaic->chn[index] >= 0) && (m_pScreenMosaic->chn[index] <= pgAvDeviceInfo->Adi_max_channel_number()))
                        {
                            chn_layout[index] = m_pScreenMosaic->chn[index];
                        }
                    }
                }

                margin[_AV_MARGIN_BOTTOM_INDEX_] = m_pScreen_Margin->usBottom;
                margin[_AV_MARGIN_LEFT_INDEX_] = m_pScreen_Margin->usLeft;
                margin[_AV_MARGIN_RIGHT_INDEX_] = m_pScreen_Margin->usRight;
                margin[_AV_MARGIN_TOP_INDEX_] = m_pScreen_Margin->usTop;

                if(m_pAv_device_master->Ad_start_selected_preview_output(pgAvDeviceInfo->Adi_get_main_out_mode(), 0, split_mode, chn_layout, vi_start_mask, true, margin) != 0)
                {
                    DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_MAINSTREAMPARA_REI m_pAv_device_master->Ad_start_preview_output FAIELD\n");

                }
                }
#endif
               // m_pAv_device_master->Ad_vi_insert_picture(m_alarm_status.videoLost);
               
                m_pAv_device_master->Ad_UpdateDevMap(ChannelMap);
#endif
                Aa_modify_encoder_para(_AST_MAIN_VIDEO_, pMain_stream);
            }
            else
            {
                m_pMain_stream_encoder->mask = pMain_stream->mask;
                for (int chn_index = 0; chn_index < pgAvDeviceInfo->Adi_max_channel_number(); chn_index++)
                {
                    if (GCL_BIT_VAL_TEST(m_pMain_stream_encoder->mask, chn_index))
                    {
                        if (_VS_ANALOG_ == pgAvDeviceInfo->Adi_get_video_source(chn_index) && 0 != memcmp(m_pMain_stream_encoder->stuVideoEncodeSetting + chn_index, pMain_stream->stuVideoEncodeSetting + chn_index, sizeof(paramVideoEncode_t)))
                        {
                            memcpy(m_pMain_stream_encoder->stuVideoEncodeSetting + chn_index, pMain_stream->stuVideoEncodeSetting + chn_index, sizeof(paramVideoEncode_t));
                        }
                    }
                }
                
            }
        }
        else
        {
            _AV_FATAL_((m_pMain_stream_encoder = new msgVideoEncodeSetting_t) == NULL, "CAvAppMaster::Aa_message_handle_MAINSTREAMPARA_REI OUT OF MEMORY\n");            
            memcpy(m_pMain_stream_encoder, pMain_stream, sizeof(msgVideoEncodeSetting_t));
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_MAINSTREAMPARA_REI invalid message length\n");
    }
}
#endif

void CAvAppMaster::Aa_message_handle_SUBSTREAMPARA(const char *msg, int length)
{
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    msgAssistRecordSetting_t *pParam = NULL;
    msgVideoEncodeSetting_t sub_stream;
    int chn_index;

    DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_SUBSTREAMPARA\n");
    eProductType product_type = PTD_INVALID;
    eSystemType subproduct_type = SYSTEM_TYPE_MAX;

    /*device type*/
    N9M_GetProductType(product_type);
    subproduct_type = N9M_GetSystemType();
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_SUBSTREAMPARA\n");
        m_start_work_env[MESSAGE_CONFIG_MANAGE_ASSIST_RECORD_PARAM_GET] = true;

        pParam = (msgAssistRecordSetting_t *)pMessage_head->body;
        memset(&sub_stream, 0, sizeof(msgVideoEncodeSetting_t));
        memcpy(sub_stream.stuVideoEncodeSetting, pParam->stuAssistRecordSetting.stChlParam, sizeof(paramVideoEncode_t) * SUPPORT_MAX_VIDEO_CHANNEL_NUM);
        sub_stream.mask = 0xffffffff;

#if defined(_AV_PRODUCT_CLASS_IPC_)
#if defined(_AV_PLATFORM_HI3518C_)
        char customer_name[32] = {0};

        memset(customer_name, 0 ,sizeof(customer_name));
        pgAvDeviceInfo->Adi_get_customer_name(customer_name, sizeof(customer_name));
#endif

        for(int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); ++chn)
        {
            int width = 0;
            int height = 0;
            av_resolution_t res ;
            pgAvDeviceInfo->Adi_get_video_resolution(&sub_stream.stuVideoEncodeSetting[chn].ucResolution, &res, true);
            pgAvDeviceInfo->Adi_get_video_size(res, m_av_tvsystem, &width, &height);
            pgAvDeviceInfo->Adi_set_stream_size(chn, _AST_SUB_VIDEO_, width,height);

#if defined(_AV_PLATFORM_HI3518C_)
            if((PTD_712_VA == pgAvDeviceInfo->Adi_product_type()) && (0 == strcmp(customer_name, "CUSTOMER_01.01")))
            {
                //712C3_VA of customer 01.01,the res 9 represents SVGA in sub stream para
                if(9 == sub_stream.stuVideoEncodeSetting[chn].ucResolution)
                {
                    sub_stream.stuVideoEncodeSetting[chn].ucResolution = 16;
                }
            }
#endif            
        }
#endif

        for(chn_index = 0; chn_index < pgAvDeviceInfo->Adi_get_vi_chn_num(); chn_index++)
        {
            if(m_pMain_stream_encoder != NULL)
            {
                //sub_stream.stuVideoEncodeSetting[chn_index].ucAudioEnable = m_pMain_stream_encoder->stuVideoEncodeSetting[chn_index].ucAudioEnable;
            }
            if (pgAvDeviceInfo->Adi_get_video_source(chn_index) == _VS_DIGITAL_)
            {
                continue;
            }
            Aa_check_and_reset_encoder_param(&sub_stream.stuVideoEncodeSetting[chn_index]);
        }
        
        if(((product_type == PTD_X1) && ((subproduct_type == SYSTEM_TYPE_X1_M0401) ||\
                                        (subproduct_type == SYSTEM_TYPE_X1_V2_M1H0401)||\
                                       (subproduct_type == SYSTEM_TYPE_X1_V2_M1SH0401)))||\
            ((product_type == PTD_X1_AHD) && ((subproduct_type == SYSTEM_TYPE_X1_V2_M1H0401_AHD)||\
                                       (subproduct_type == SYSTEM_TYPE_X1_V2_M1SH0401_AHD)))||\
        (product_type == PTD_D5M)||\
        (product_type == PTD_A5_II)||(product_type == PTD_6A_I))

        {
            if(m_pSub2_stream_encoder != NULL)
            {
                m_sub_record_type = pParam->stuAssistRecordSetting.u8RecordMode;
                if(m_start_work_flag != 0)
                {  
                   Aa_modify_encoder_para(_AST_SUB2_VIDEO_, &sub_stream);
                }
                else
                {
                    m_pSub2_stream_encoder->mask = sub_stream.mask;
                    for (int chn_index = 0; chn_index < pgAvDeviceInfo->Adi_max_channel_number(); chn_index++)
                    {
                        if (GCL_BIT_VAL_TEST(m_pSub2_stream_encoder->mask, chn_index))
                        {

                            if (_VS_ANALOG_ == pgAvDeviceInfo->Adi_get_video_source(chn_index) && 0 != memcmp(m_pSub2_stream_encoder->stuVideoEncodeSetting + chn_index, sub_stream.stuVideoEncodeSetting + chn_index, sizeof(paramVideoEncode_t)))
                            {
                                memcpy(m_pSub2_stream_encoder->stuVideoEncodeSetting + chn_index, sub_stream.stuVideoEncodeSetting + chn_index, sizeof(paramVideoEncode_t));
                            }
                        }
                    }
                }
            }
            else
            {
                //_AV_FATAL_((m_pSub_stream_encoder = new msgVideoEncodeSetting_t) == NULL, "CAvAppMaster::Aa_message_handle_SUBSTREAMPARA OUT OF MEMORY\n");
                
                m_sub_record_type = pParam->stuAssistRecordSetting.u8RecordMode;
                DEBUG_MESSAGE("122222222222222222\n");
                _AV_FATAL_((m_pSub2_stream_encoder = new msgVideoEncodeSetting_t) == NULL, "CAvAppMaster::Aa_message_handle_SUBSTREAMPARA OUT OF MEMORY\n");
                memcpy(m_pSub2_stream_encoder, &sub_stream, sizeof(msgVideoEncodeSetting_t));
                DEBUG_MESSAGE("333222222222222222\n");
                m_sub_record_type = pParam->stuAssistRecordSetting.u8RecordMode;
            }
#if defined(_AV_PLATFORM_HI3515_)
            if ((m_pMain_stream_encoder != NULL)&&(m_pSub2_stream_encoder != NULL))
            {
                int main_wid = 0;
                int main_hei = 0;
                int sub_wid = 0;
                int sub_hei = 0;
                for( int index = 0; index < pgAvDeviceInfo->Adi_max_channel_number();index++)
                {
                    av_resolution_t eResolution;
                    pgAvDeviceInfo->Adi_get_video_resolution(&m_pMain_stream_encoder->stuVideoEncodeSetting[index].ucResolution, &eResolution, true);
                    pgAvDeviceInfo->Adi_get_video_size(eResolution, m_av_tvsystem,&main_wid, &main_hei);
        
                    unsigned char res = m_pSub2_stream_encoder->stuVideoEncodeSetting[index].ucResolution;
        
                    pgAvDeviceInfo->Adi_get_video_resolution(&res, &eResolution, true);
                    pgAvDeviceInfo->Adi_get_video_size(eResolution,m_av_tvsystem,&sub_wid, &sub_hei);  
        
                    //! check the sub stream param for they can never be exceed the main ones
                    if((main_wid < sub_wid)||(main_hei < sub_hei))
                    {
                        m_pSub2_stream_encoder->stuVideoEncodeSetting[index].ucResolution = m_pMain_stream_encoder->stuVideoEncodeSetting[index].ucResolution;
                        DEBUG_CRITICAL("substream resoulution invalid! modified:%d\n", m_pSub2_stream_encoder->stuVideoEncodeSetting[index].ucResolution);
                    }
                    if(m_pMain_stream_encoder->stuVideoEncodeSetting[index].ucFrameRate < m_pSub2_stream_encoder->stuVideoEncodeSetting[index].ucFrameRate)
                    {
                        m_pSub2_stream_encoder->stuVideoEncodeSetting[index].ucFrameRate = m_pMain_stream_encoder->stuVideoEncodeSetting[index].ucFrameRate;
        
                    }
                }
                
            }
            
#endif

        }
        else
        {
            if(m_pSub_stream_encoder != NULL)
            {
                m_sub_record_type = pParam->stuAssistRecordSetting.u8RecordMode;
                if(m_start_work_flag != 0)
                {
                    Aa_modify_encoder_para(_AST_SUB_VIDEO_, &sub_stream);
                }
                else
                {
                    m_pSub_stream_encoder->mask = sub_stream.mask;
                    for (int chn_index = 0; chn_index < pgAvDeviceInfo->Adi_max_channel_number(); chn_index++)
                    {
                        if (GCL_BIT_VAL_TEST(m_pSub_stream_encoder->mask, chn_index))
                        {
                            if (_VS_ANALOG_ == pgAvDeviceInfo->Adi_get_video_source(chn_index) && 0 != memcmp(m_pSub_stream_encoder->stuVideoEncodeSetting + chn_index, sub_stream.stuVideoEncodeSetting + chn_index, sizeof(paramVideoEncode_t)))
                            {
                                memcpy(m_pSub_stream_encoder->stuVideoEncodeSetting + chn_index, sub_stream.stuVideoEncodeSetting + chn_index, sizeof(paramVideoEncode_t));
                            }
                        }
                    }
                }
            }
            else
            {
                _AV_FATAL_((m_pSub_stream_encoder = new msgVideoEncodeSetting_t) == NULL, "CAvAppMaster::Aa_message_handle_SUBSTREAMPARA OUT OF MEMORY\n");
                memcpy(m_pSub_stream_encoder, &sub_stream, sizeof(msgVideoEncodeSetting_t));
                m_sub_record_type = pParam->stuAssistRecordSetting.u8RecordMode;
            }
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_SUBSTREAMPARA invalid message length\n");
    }
}

void CAvAppMaster::Aa_message_handle_MOBSTREAMPARA(const char *msg, int length)
{
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    int chn_index;

    eProductType product_type = PTD_INVALID;
    eSystemType subproduct_type = SYSTEM_TYPE_MAX;
    DEBUG_CRITICAL( "CAvAppMaster::Aa_message_handle_MOBSTREAMPARA\n");

    /*device type*/
    N9M_GetProductType(product_type);
    subproduct_type = N9M_GetSystemType();
    msgNetTransferSetting_t *pnet_para = NULL;
    msgVideoEncodeSetting_t sub2_stream;
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_MOBSTREAMPARA\n");
        m_start_work_env[MESSAGE_CONFIG_MANAGE_NET_TRANSFER_PARAM_GET] = true;
        
        pnet_para = (msgNetTransferSetting_t *)pMessage_head->body;
        memset(&sub2_stream, 0, sizeof(msgVideoEncodeSetting_t));
        m_net_transfe = pnet_para->stuNetTransferSetting.ucSubStreamMode;

#if defined(_AV_PRODUCT_CLASS_IPC_)
        for(int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); ++chn)
        {
            int width = 0;
            int height = 0;
            av_resolution_t res ;
            pgAvDeviceInfo->Adi_get_video_resolution(&pnet_para->stuNetTransferSetting.stuSubStreamSet[chn].ucResolution, &res, true);
            pgAvDeviceInfo->Adi_get_video_size(res, m_av_tvsystem, &width, &height);
            pgAvDeviceInfo->Adi_set_stream_size(chn, _AST_SUB2_VIDEO_, width,height);
        }
#endif

        for(chn_index = 0; chn_index < pgAvDeviceInfo->Adi_get_vi_chn_num(); chn_index++)
        {
            if (pgAvDeviceInfo->Adi_get_video_source(chn_index) == _VS_DIGITAL_)
            {
                continue;
            }
            else
            {
                GCL_BIT_VAL_SET(sub2_stream.mask, chn_index);
                if(m_pMain_stream_encoder != NULL)
                {
                    //sub2_stream.stuVideoEncodeSetting[chn_index].ucAudioEnable = m_pMain_stream_encoder->stuVideoEncodeSetting[chn_index].ucAudioEnable;
                }
#if defined(_AV_PLATFORM_HI3518EV200_)
                if(PTD_718_VA == pgAvDeviceInfo->Adi_product_type())
                {
                    sub2_stream.stuVideoEncodeSetting[chn_index].ucChnEnable = 0;
                }
                else
                {
                    sub2_stream.stuVideoEncodeSetting[chn_index].ucChnEnable = pnet_para->stuNetTransferSetting.stuSubStreamSet[chn_index].ucChnEnable;    
                }
#else
                if(PTD_712_VF == pgAvDeviceInfo->Adi_product_type())
                {
                    sub2_stream.stuVideoEncodeSetting[chn_index].ucChnEnable = 0;
                }
                else
                {
                    sub2_stream.stuVideoEncodeSetting[chn_index].ucChnEnable = pnet_para->stuNetTransferSetting.stuSubStreamSet[chn_index].ucChnEnable;                
                }
#endif
                sub2_stream.stuVideoEncodeSetting[chn_index].ucFrameType = pnet_para->stuNetTransferSetting.stuSubStreamSet[chn_index].ucFrameType;
                sub2_stream.stuVideoEncodeSetting[chn_index].ucFrameRate = pnet_para->stuNetTransferSetting.stuSubStreamSet[chn_index].ucFrameRate;
                sub2_stream.stuVideoEncodeSetting[chn_index].ucNormalQuality = pnet_para->stuNetTransferSetting.stuSubStreamSet[chn_index].ucQuality;
                sub2_stream.stuVideoEncodeSetting[chn_index].ucResolution = pnet_para->stuNetTransferSetting.stuSubStreamSet[chn_index].ucResolution;
                sub2_stream.stuVideoEncodeSetting[chn_index].ucBrMode = pnet_para->stuNetTransferSetting.stuSubStreamSet[chn_index].ucBrMode;
                sub2_stream.stuVideoEncodeSetting[chn_index].uiBitrate = pnet_para->stuNetTransferSetting.stuSubStreamSet[chn_index].u32Bitrate;
                Aa_check_and_reset_encoder_param(&sub2_stream.stuVideoEncodeSetting[chn_index]);
            }
        }

        if(m_pSub2_stream_encoder != NULL)
        {
            if(((product_type == PTD_X1) && ((subproduct_type == SYSTEM_TYPE_X1_M0401) ||\
                    (subproduct_type == SYSTEM_TYPE_X1_V2_M1H0401)||\
                   (subproduct_type == SYSTEM_TYPE_X1_V2_M1SH0401)))||\
                ((product_type == PTD_X1_AHD) && ((subproduct_type == SYSTEM_TYPE_X1_V2_M1H0401_AHD)||\
                   (subproduct_type == SYSTEM_TYPE_X1_V2_M1SH0401_AHD)))||\
            (product_type == PTD_D5M)||\
            (product_type == PTD_A5_II)||(product_type == PTD_6A_I))
            {
                return ;
            }
            if(m_start_work_flag != 0)
            {
                Aa_modify_encoder_para(_AST_SUB2_VIDEO_, &sub2_stream);
            }
            else
            { 
                m_pSub2_stream_encoder->mask = sub2_stream.mask;
                for (int chn_index = 0; chn_index < pgAvDeviceInfo->Adi_max_channel_number(); chn_index++)
                {
                    if (GCL_BIT_VAL_TEST(m_pSub2_stream_encoder->mask, chn_index))
                    {
                        if (_VS_ANALOG_ == pgAvDeviceInfo->Adi_get_video_source(chn_index) && 0 != memcmp(m_pSub2_stream_encoder->stuVideoEncodeSetting + chn_index, sub2_stream.stuVideoEncodeSetting + chn_index, sizeof(paramVideoEncode_t)))
                        {
                            memcpy(m_pSub2_stream_encoder->stuVideoEncodeSetting + chn_index, sub2_stream.stuVideoEncodeSetting + chn_index, sizeof(paramVideoEncode_t));
                        }
                    }
                }
            }
        }
        else
        {
            _AV_FATAL_((m_pSub2_stream_encoder = new msgVideoEncodeSetting_t) == NULL, "CAvAppMaster::Aa_message_handle_MOBSTREAMPARA OUT OF MEMORY\n");
            if(((product_type == PTD_X1) && ((subproduct_type == SYSTEM_TYPE_X1_M0401) ||\
                                            (subproduct_type == SYSTEM_TYPE_X1_V2_M1H0401)||\
                                           (subproduct_type == SYSTEM_TYPE_X1_V2_M1SH0401)))||\
                ((product_type == PTD_X1_AHD) && ((subproduct_type == SYSTEM_TYPE_X1_V2_M1H0401_AHD)||\
                                           (subproduct_type == SYSTEM_TYPE_X1_V2_M1SH0401_AHD)))||\
            (product_type == PTD_D5M)||\
            (product_type == PTD_A5_II)||(product_type == PTD_6A_I))
			{
				return ;
			}
            memcpy(m_pSub2_stream_encoder, &sub2_stream, sizeof(msgVideoEncodeSetting_t)); 
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_MOBSTREAMPARA invalid message length\n");
    }
}

#ifdef _AV_PRODUCT_CLASS_IPC_
void CAvAppMaster::Aa_message_handle_NetSnap(const char* msg, int length )
{
    if(m_start_work_flag == 0)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_NetSnap system not ok\n");
        return;
    }
#if 0
    SMessageHead *head = (SMessageHead*) msg;
    if( head->length == sizeof(msgNetSnapPicture_t) )
    {
        msgNetSnapPicture_t* pNetParam = (msgNetSnapPicture_t*)head->body;

        DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_NetSnap net snap\n");
        MsgSnapPicture_t stuSnapParam;
        stuSnapParam.Channel = pNetParam->Channel;
        stuSnapParam.Number = pNetParam->Number;
        stuSnapParam.PreTime = pNetParam->PreTime;
        stuSnapParam.Interval = pNetParam->Interval;
        stuSnapParam.SnapPriority = pNetParam->SnapPriority;
        stuSnapParam.DelayDeviation = pNetParam->DelayDeviation; 
        m_pAv_device->Ad_cmd_snap_picutres( &stuSnapParam );
    }   
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_NetSnap invalid message length\n");
    }
#endif

}

void CAvAppMaster::Ae_message_handle_IPCWorkMode( const char* msg, int length )
{
    SMessageHead *head = (SMessageHead*) msg;
    if( head && head->length == sizeof(msgDevConType_t) )
    {
        msgDevConType_t* pstuParam = (msgDevConType_t*)head->body;
        if( pstuParam->contype == 0 )
        {
     //       m_u8IPCSnapWorkMode = 0;
        }
        else
        {
     //       m_u8IPCSnapWorkMode = 1;
            m_pAv_device->Ad_snap_clear_all_pictures();
        }
        DEBUG_MESSAGE( "CAvAppMaster::Ae_message_handle_IPCWorkMode workMode=%d\n", m_u8IPCSnapWorkMode);
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Ae_message_handle_IPCWorkMode invalid message length\n");
    }
}

void CAvAppMaster::Aa_message_handle_update_video_osd(const char* msg, int length) 
{
    SMessageHead *head = (SMessageHead*) msg;
    if(head)
    {
        msgOsdOverlay_t* pOsdParam = (msgOsdOverlay_t*)head->body;

        if(NULL == m_pVideo_encoder_osd)
        {
            //sleep 3S to ensure the regin has been created successfully when the first time the osd update
            mSleep(3*1000);
        }
        DEBUG_MESSAGE("update video osd [v:%s s:%s g:%s i:%s] \n", (char *)pOsdParam->vehicle, (char *)pOsdParam->speed, (char *)pOsdParam->gps, (char *)pOsdParam->id);
        if((0 != m_start_work_flag) && (NULL != m_pVideo_encoder_osd))
        {
             for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); ++chn)
             {
                 if(0 != ((pOsdParam->uiChannelMask) & (1ll<<chn)))
                 {
                    if(Aa_is_customer_cnr())
                    {
                        //unsigned char  carriage_num = 0;
                        unsigned char camera_num = 0;
                        //char carriage_str[6] ;
                        char ci_chn_str[4];
                        char osd_info[32];
                        
                        memset(osd_info, 0, sizeof(osd_info));
                        //memset(carriage_str, 0, sizeof(carriage_str));
                        memset(ci_chn_str, 0, sizeof(ci_chn_str));
                        
                        if( NULL != m_pIpAddress)
                        {
                            //carriage_num = m_pIpAddress->stuAutoIp.szIpAddr[2];
                            camera_num = m_pIpAddress->stuAutoIp.ucIpAddr[3];
                        }
#if 0
                        if((carriage_num>=0) && (carriage_num<10))
                        {
                            snprintf(carriage_str, sizeof(carriage_str), "0%d", carriage_num);
                        }
                        else if((carriage_num>=10) && (carriage_num<20))
                        {
                            snprintf(carriage_str, sizeof(carriage_str), "%d", carriage_num);
                        }
                        else if((carriage_num>=70) && (carriage_num<80))
                        {
                            snprintf(carriage_str, sizeof(carriage_str), "+%d", carriage_num-70);
                        }
                        else
                        {
                            snprintf(carriage_str, sizeof(carriage_str), "00");
                        }
#endif   
                        /*when the vehicle is null ,we should retain the place for vehicle*/
                        if(0 == strlen((char *)pOsdParam->vehicle))
                        {
                            strncpy((char *)m_osd_content[0], "        ", MAX_OSD_NAME_SIZE);
                            m_osd_content[0][8] = '\0';
                        }
                        else
                        {
                            strncpy((char *)m_osd_content[0], (char *)pOsdParam->vehicle, MAX_OSD_NAME_SIZE);
                            m_osd_content[0][MAX_OSD_NAME_SIZE - 1] = '\0';
                        }
                        snprintf(osd_info, sizeof(osd_info), "%s  0%d",(char *)m_osd_content[0], camera_num%10);
                        osd_info[sizeof(osd_info)-1] = '\0';
                        if(m_main_encoder_state.test(chn))
                        {
                            m_pAv_device->Ad_update_extra_osd_content(_AST_MAIN_VIDEO_, chn, _AON_GPS_INFO_, osd_info, strlen(osd_info));
                        }

                        if(m_sub_encoder_state.test(chn))
                        {
                            m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, chn, _AON_GPS_INFO_, osd_info, strlen(osd_info));
                        }


                        if(m_sub2_encoder_state.test(chn))
                        {
                            m_pAv_device->Ad_update_extra_osd_content(_AST_SUB2_VIDEO_, chn, _AON_GPS_INFO_, osd_info, strlen(osd_info));                                
                        }

                    }
                    else
                    {
                        bool bVehicle_changed = false;
                        bool bDevice_id_changed = false;
                        bool bSpeed_changed = false;
                        bool bGps_changed = false;
                        bool bExt_osd1_changed = false;
                        bool bExt_osd2_changed = false;

                        if(0 != strcmp((char *)pOsdParam->vehicle, m_osd_content[0]))
                        {
                            bVehicle_changed = true;
                            strncpy(m_osd_content[0], (char *)(pOsdParam->vehicle), sizeof(m_osd_content[0]));
                            m_osd_content[0][sizeof(m_osd_content[0])-1] = '\0';
                        }

                        if(0 != strcmp((char *)pOsdParam->id, m_osd_content[3]))
                        {
                            bDevice_id_changed = true;
                            strncpy(m_osd_content[3],  (char *)pOsdParam->id, sizeof(m_osd_content[3]));
                            m_osd_content[3][sizeof(m_osd_content[3])-1] = '\0';
                        }

                        if(0 != strcmp((char *)pOsdParam->gps, m_osd_content[1]))
                        {
                            bGps_changed = true;
                            strncpy(m_osd_content[1], (char *)(pOsdParam->gps), sizeof(m_osd_content[1]));
                            m_osd_content[1][sizeof(m_osd_content[1])-1] = '\0';
                            //! update to ive for snap
#if defined(_AV_SUPPORT_IVE_)
                            m_pAv_device_master->Ad_set_ive_osd(m_osd_content[1]);
#endif
                        }

                        if(0 != strcmp((char *)pOsdParam->speed, m_osd_content[2]))
                        {
                            bSpeed_changed = true;
                            strncpy(m_osd_content[2], (char *)(pOsdParam->speed), sizeof(m_osd_content[2]));
                            m_osd_content[2][sizeof(m_osd_content[2])-1] = '\0';
                        }
                        if(pOsdParam->extcount >= 1)
                        {
                            msgExtOsdInfoChn_t* ext_osd_info = (msgExtOsdInfoChn_t*)pOsdParam->extbody;
                            if(NULL != ext_osd_info)
                            {
                            	DEBUG_MESSAGE("common osd1 is:%s \n", (char *)ext_osd_info[0].osdinfo[0].str);
                                if(0 != strcmp((char *)ext_osd_info[0].osdinfo[0].str, m_ext_osd_content[0]))
                                {
                                    bExt_osd1_changed = true;
                                    memset(m_ext_osd_content[0], 0, sizeof(m_ext_osd_content[0]));
                                    strncpy(m_ext_osd_content[0], (char *)(ext_osd_info[0].osdinfo[0].str), sizeof(m_ext_osd_content[0]));
                                    m_ext_osd_content[0][sizeof(m_ext_osd_content[0])-1] = '\0';
                                }

                                if(pOsdParam->extcount >= 2)
                                {
                                    DEBUG_MESSAGE("common osd2 is:%s \n", (char *)ext_osd_info[1].osdinfo[0].str);
                                    if(0 != strcmp((char *)ext_osd_info[1].osdinfo[0].str, m_ext_osd_content[1]))
                                    {
                                        bExt_osd2_changed = true;
                                        memset(m_ext_osd_content[1], 0, sizeof(m_ext_osd_content[1]));
                                        strncpy(m_ext_osd_content[1], (char *)(ext_osd_info[1].osdinfo[0].str), sizeof(m_ext_osd_content[1]));
                                        m_ext_osd_content[1][sizeof(m_ext_osd_content[1])-1] = '\0';
                                    }
                                }
                            }
                        }
                        else
                        {
                            if(0 != strlen(m_ext_osd_content[0]))
                            {   
                                bExt_osd1_changed = true;
                                strncpy(m_ext_osd_content[0], "", sizeof(m_ext_osd_content[0]));
                            }
                            if(0 != strlen(m_ext_osd_content[1]))
                            {   
                                bExt_osd2_changed = true;
                                strncpy(m_ext_osd_content[1], "", sizeof(m_ext_osd_content[1]));
                            }
                        }
                    
                        //cash osd font
                        av_video_encoder_para_t video_encoder;
                        Aa_get_encoder_para(_AST_MAIN_VIDEO_, chn, &video_encoder, -1, NULL);

                        if(m_main_encoder_state.test(chn)) 
                         {
                            if(bVehicle_changed && video_encoder.have_bus_number_osd)
                            {
                                m_pAv_device->Ad_update_extra_osd_content(_AST_MAIN_VIDEO_, chn, _AON_BUS_NUMBER_, (char *)pOsdParam->vehicle, strlen((char *)pOsdParam->vehicle));                           
                            }

                            if(bGps_changed && video_encoder.have_gps_osd)
                            {
                                m_pAv_device->Ad_update_extra_osd_content(_AST_MAIN_VIDEO_, chn, _AON_GPS_INFO_, (char *)pOsdParam->gps, strlen((char *)pOsdParam->gps));
                            }

                            if(bSpeed_changed && video_encoder.have_speed_osd)
                            {
                                m_pAv_device->Ad_update_extra_osd_content(_AST_MAIN_VIDEO_, chn, _AON_SPEED_INFO_, (char *)pOsdParam->speed, strlen((char *)pOsdParam->speed));                              
                            }
                            
                            if(bDevice_id_changed && video_encoder.have_bus_selfnumber_osd)
                            {
                                m_pAv_device->Ad_update_extra_osd_content(_AST_MAIN_VIDEO_, chn, _AON_SELFNUMBER_INFO_, (char *)pOsdParam->id, strlen((char *)pOsdParam->id));                                       
                            }

                            if(bExt_osd1_changed && video_encoder.have_common1_osd)
                            {
                                m_pAv_device->Ad_update_extra_osd_content(_AST_MAIN_VIDEO_, chn, _AON_COMMON_OSD1_, m_ext_osd_content[0], strlen(m_ext_osd_content[0]));                                       
                            }

                            if(bExt_osd2_changed && video_encoder.have_common2_osd)
                            {
                                m_pAv_device->Ad_update_extra_osd_content(_AST_MAIN_VIDEO_, chn, _AON_COMMON_OSD2_, m_ext_osd_content[1], strlen(m_ext_osd_content[1]));                                       
                            }
                         }
                     
                         if(m_sub_encoder_state.test(chn))
                         {
                            if(bVehicle_changed && video_encoder.have_bus_number_osd)
                            {
                                m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, chn, _AON_BUS_NUMBER_, (char *)pOsdParam->vehicle, strlen((char *)pOsdParam->vehicle));  
                            }
                            if(bGps_changed && video_encoder.have_gps_osd)
                            {
                                m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, chn, _AON_GPS_INFO_, (char *)pOsdParam->gps, strlen((char *)pOsdParam->gps));
                            }

                            if(bSpeed_changed && video_encoder.have_speed_osd)
                            {
                                m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, chn, _AON_SPEED_INFO_, (char *)pOsdParam->speed, strlen((char *)pOsdParam->speed));     
                            }   

                            if(bDevice_id_changed && video_encoder.have_bus_selfnumber_osd)
                            {
                                m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, chn, _AON_SELFNUMBER_INFO_,(char *)pOsdParam->id, strlen((char *)pOsdParam->id)); 
                            }

                            if(bExt_osd1_changed && video_encoder.have_common1_osd)
                            {
                                m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, chn, _AON_COMMON_OSD1_, m_ext_osd_content[0], strlen(m_ext_osd_content[0]));                                       
                            }

                            if(bExt_osd2_changed && video_encoder.have_common2_osd)
                            {
                                m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, chn, _AON_COMMON_OSD2_, m_ext_osd_content[1], strlen( m_ext_osd_content[1]));                                       
                            }
                         }
                         if(m_sub2_encoder_state.test(chn))
                         {
                            if(bVehicle_changed && video_encoder.have_bus_number_osd)
                            {
                                m_pAv_device->Ad_update_extra_osd_content(_AST_SUB2_VIDEO_, chn, _AON_BUS_NUMBER_, (char *)pOsdParam->vehicle, strlen((char *)pOsdParam->vehicle));                               
                            }
                            if(bGps_changed && video_encoder.have_gps_osd)
                            {
                                m_pAv_device->Ad_update_extra_osd_content(_AST_SUB2_VIDEO_, chn, _AON_GPS_INFO_, (char *)pOsdParam->gps, strlen((char *)pOsdParam->gps));
                            }

                            if(bSpeed_changed && video_encoder.have_speed_osd)
                            {
                                m_pAv_device->Ad_update_extra_osd_content(_AST_SUB2_VIDEO_, chn, _AON_SPEED_INFO_, (char *)pOsdParam->speed, strlen((char *)pOsdParam->speed));   
                            }  
                            if(bDevice_id_changed && video_encoder.have_bus_selfnumber_osd)
                            {
                                m_pAv_device->Ad_update_extra_osd_content(_AST_SUB2_VIDEO_, chn, _AON_SELFNUMBER_INFO_, (char *)pOsdParam->id, strlen((char *)pOsdParam->id)); 
                            }
                            if(bExt_osd1_changed && video_encoder.have_common1_osd)
                            {
                                m_pAv_device->Ad_update_extra_osd_content(_AST_SUB2_VIDEO_, chn, _AON_COMMON_OSD1_, m_ext_osd_content[0], strlen(m_ext_osd_content[0]));                                       
                            }

                            if(bExt_osd2_changed && video_encoder.have_common2_osd)
                            {
                                m_pAv_device->Ad_update_extra_osd_content(_AST_SUB2_VIDEO_, chn, _AON_COMMON_OSD2_, m_ext_osd_content[1], strlen(m_ext_osd_content[1]));                                       
                            }
                         } 
                 }
             }
        }
    }
    }
    else
    {
        DEBUG_ERROR("CAvAppMaster::Aa_message_handle_update_video_osd invalid message length\n");
    }  
}
void CAvAppMaster::Ae_message_handle_GetIpAddress( const char* msg, int length )
{
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgAutoIpInfo_t *pIp_address = (msgAutoIpInfo_t *)pMessage_head->body;
        if(NULL == m_pIpAddress)
        {
            m_pIpAddress = new msgAutoIpInfo_t();
        }

        if(0 != memcmp(pIp_address, m_pIpAddress, sizeof(msgAutoIpInfo_t)))
        {
            memcpy(m_pIpAddress, pIp_address, sizeof(msgAutoIpInfo_t));
        }
        printf("[%s %d]the device ip is:\n", __FUNCTION__, __LINE__);
        for(unsigned int i=0;i<4;i++)
        {
            printf("%d", pIp_address->stuAutoIp.ucIpAddr[i]);
        }
        printf("\n");
        if((Aa_is_customer_cnr()))
        {
            //unsigned char  carriage_num = 0;
            unsigned char camera_num = 0;
            //char carriage_str[6] ;
            char ci_chn_str[4];
            char osd_info[32];

            //memset(carriage_str, 0, sizeof(carriage_str));
            memset(ci_chn_str, 0, sizeof(ci_chn_str));
            memset(osd_info, 0, sizeof(osd_info));
            
            if( NULL != m_pIpAddress)
            {
                //carriage_num = m_pIpAddress->stuAutoIp.szIpAddr[2];
                camera_num = m_pIpAddress->stuAutoIp.ucIpAddr[3];
            }
            #if 0
            if((carriage_num>=0) && (carriage_num<10))
            {
                snprintf(carriage_str, sizeof(carriage_str), "0%d", carriage_num);
            }
            else if((carriage_num>=10) && (carriage_num<20))
            {
                snprintf(carriage_str, sizeof(carriage_str), "%d", carriage_num);
            }
            else if((carriage_num>=70) && (carriage_num<80))
            {
                snprintf(carriage_str, sizeof(carriage_str), "+%d", carriage_num-70);
            }
            else
            {
                snprintf(carriage_str, sizeof(carriage_str), "00");
            }
            #endif
            if(0 == strlen(m_osd_content[0]))
            {
                snprintf(osd_info, sizeof(osd_info), "       0%d", camera_num%10);
            }
            else
            {
                snprintf(osd_info, sizeof(osd_info), "%s  0%d", (char *)m_osd_content[0], camera_num%10);    
            }

            osd_info[sizeof(osd_info)-1] = '\0';
             for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); ++chn)
             {
                 if(m_main_encoder_state.test(chn))
                 {
                     m_pAv_device->Ad_update_extra_osd_content(_AST_MAIN_VIDEO_, chn, _AON_GPS_INFO_, osd_info, strlen(osd_info));
                 }

                 if(m_sub_encoder_state.test(chn))
                 {
                     m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, chn, _AON_GPS_INFO_, osd_info, strlen(osd_info));
                 }

                 if(m_sub2_encoder_state.test(chn))
                 {
                     m_pAv_device->Ad_update_extra_osd_content(_AST_SUB2_VIDEO_, chn, _AON_GPS_INFO_, osd_info, strlen(osd_info));                                
                 }

             }
        }
    }
    else
    {
        _AV_KEY_INFO_(_AVD_APP_,"Ae_message_handle_GetIpAddress the message head is invalid! \n");
    }
}
#endif

void CAvAppMaster::Aa_message_handle_VIDEOOSD(const char *msg, int length)
{
    SMessageHead *pMessage_head = (SMessageHead*)msg;

    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgEncodeOsdSetting_t *pVideo_osd = (msgEncodeOsdSetting_t *)pMessage_head->body;
        m_start_work_env[MESSAGE_CONFIG_MANAGE_ENCODE_OSD_PARAM_GET] = true;
        
        if(m_pVideo_encoder_osd != NULL)
        {
#if (defined(_AV_PLATFORM_HI3520D_V300_)||defined(_AV_PLATFORM_HI3515_))		//6AII_AV12机型不需要标准的osd叠加
            if((pgAvDeviceInfo->Adi_product_type() == OSD_TEST_FLAG)||(pgAvDeviceInfo->Adi_product_type() == PTD_6A_I))
            {
                memcpy(m_pVideo_encoder_osd, pVideo_osd, sizeof(msgEncodeOsdSetting_t));
                return;
            }
#endif    
            if ((0 != memcmp(pVideo_osd, m_pVideo_encoder_osd, sizeof(msgEncodeOsdSetting_t))) && (0 != m_start_work_flag))
            {
                memcpy(m_pVideo_encoder_osd, pVideo_osd, sizeof(msgEncodeOsdSetting_t));
                if ((m_start_work_flag != 0) && (Aa_everything_is_ready() == 0))
                {
                    av_video_encoder_para_t video_encoder_para;
                    for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
                    {
                        bool bosd_need_change = false;
                        if(0 != ((pVideo_osd->mask) & (1ll<<chn)))
                        {
                            bosd_need_change = true;
                            int phy_audio_chn_num = chn;
                            pgAvDeviceInfo->Adi_get_audio_chn_id(chn , phy_audio_chn_num);
                            /*main stream osd*/
                            if (m_main_encoder_state.test(chn))
                            {
                                Aa_get_encoder_para(_AST_MAIN_VIDEO_, chn, &video_encoder_para, phy_audio_chn_num, NULL);		
                                m_pAv_device->Ad_modify_encoder(_AST_MAIN_VIDEO_, chn, &video_encoder_para, bosd_need_change);
                            }

                            /*sub stream osd*/
                            if (m_sub_encoder_state.test(chn))
                            {
                                Aa_get_encoder_para(_AST_SUB_VIDEO_, chn, &video_encoder_para, phy_audio_chn_num, NULL);
                                m_pAv_device->Ad_modify_encoder(_AST_SUB_VIDEO_, chn, &video_encoder_para, bosd_need_change);
                            }
                            
                            /*sub stream osd*/
                            if (m_sub2_encoder_state.test(chn))
                            {
                                Aa_get_encoder_para(_AST_SUB2_VIDEO_, chn, &video_encoder_para, phy_audio_chn_num, NULL);
#ifndef _AV_PRODUCT_CLASS_IPC_ 
                                char product_group[32] = {0};
                                pgAvDeviceInfo->Adi_get_product_group(product_group, sizeof(product_group));
                                char cutomername[32] = {0};
                                pgAvDeviceInfo->Adi_get_customer_name(cutomername, sizeof(cutomername));
                                if((strcmp(product_group, "ITS") != 0) && (strcmp(cutomername, "CUSTOMER_04.862") != 0))
                                {
                                    video_encoder_para.have_channel_name_osd = 0;
                                    video_encoder_para.have_bus_number_osd = 0;
                                    video_encoder_para.have_bus_selfnumber_osd= 0;
                                    video_encoder_para.have_gps_osd = 0;
                                    video_encoder_para.have_speed_osd = 0; 
                                    video_encoder_para.have_common1_osd= 0;
                                    video_encoder_para.have_common2_osd= 0;
                                    /*时间*/               
                                    video_encoder_para.have_date_time_osd = pVideo_osd->stuEncodeOsdSetting[chn].ucEnableTime;
                                    video_encoder_para.date_time_osd_x = pVideo_osd->stuEncodeOsdSetting[chn].usTimeX;
                                    video_encoder_para.date_time_osd_y = pVideo_osd->stuEncodeOsdSetting[chn].usTimeY;
                                }
#endif
                                m_pAv_device->Ad_modify_encoder(_AST_SUB2_VIDEO_, chn, &video_encoder_para);
                            }


                        }
                    }
                }
            }
        }
        else
        {
            _AV_FATAL_((m_pVideo_encoder_osd = new msgEncodeOsdSetting_t) == NULL, "CAvAppMaster::Aa_message_handle_VIDEOOSD OUT OF MEMORY\n");
            memcpy(m_pVideo_encoder_osd, pVideo_osd, sizeof(msgEncodeOsdSetting_t));
        }
#if defined(_AV_PRODUCT_CLASS_IPC_)
#if 0
        printf("common osd1:%s[x:%d y:%d] osd2:%s  [x:%d y:%d]\n", pVideo_osd->stuEncodeOsdSetting[0].stuCommonOsd[0].szOsdContent,\
        pVideo_osd->stuEncodeOsdSetting[0].stuCommonOsd[0].usOsdX, pVideo_osd->stuEncodeOsdSetting[0].stuCommonOsd[0].usOsdY, \
        pVideo_osd->stuEncodeOsdSetting[0].stuCommonOsd[1].szOsdContent,  pVideo_osd->stuEncodeOsdSetting[0].stuCommonOsd[1].usOsdX, \
        pVideo_osd->stuEncodeOsdSetting[0].stuCommonOsd[1].usOsdY);
#endif
        if(0 != strcmp(m_ext_osd_content[0], pVideo_osd->stuEncodeOsdSetting[0].stuCommonOsd[0].szOsdContent))
        {
            strncpy(m_ext_osd_content[0], pVideo_osd->stuEncodeOsdSetting[0].stuCommonOsd[0].szOsdContent, sizeof(m_ext_osd_content[0]));
            if(0 != m_start_work_flag)
            {
                if(pVideo_osd->stuEncodeOsdSetting[0].stuCommonOsd[0].ucEnableOsd)
                {
                    m_pAv_device->Ad_update_extra_osd_content(_AST_MAIN_VIDEO_, 0, _AON_COMMON_OSD1_, m_ext_osd_content[0], strlen(m_ext_osd_content[0])); 
                    m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, 0, _AON_COMMON_OSD1_, m_ext_osd_content[0], strlen(m_ext_osd_content[0])); 
                    m_pAv_device->Ad_update_extra_osd_content(_AST_SUB2_VIDEO_, 0, _AON_COMMON_OSD1_, m_ext_osd_content[0], strlen(m_ext_osd_content[0])); 
                }
            }
        }
        
        if(0 != strcmp(m_ext_osd_content[1], pVideo_osd->stuEncodeOsdSetting[0].stuCommonOsd[1].szOsdContent))
        {
            strncpy(m_ext_osd_content[1], pVideo_osd->stuEncodeOsdSetting[0].stuCommonOsd[1].szOsdContent, sizeof(m_ext_osd_content[1]));
            if(0 != m_start_work_flag)
            {
                if(pVideo_osd->stuEncodeOsdSetting[0].stuCommonOsd[1].ucEnableOsd)
                {
                    m_pAv_device->Ad_update_extra_osd_content(_AST_MAIN_VIDEO_, 0, _AON_COMMON_OSD2_, m_ext_osd_content[1], strlen(m_ext_osd_content[1])); 
                    m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, 0, _AON_COMMON_OSD2_, m_ext_osd_content[1], strlen(m_ext_osd_content[1])); 
                    m_pAv_device->Ad_update_extra_osd_content(_AST_SUB2_VIDEO_, 0, _AON_COMMON_OSD2_, m_ext_osd_content[1], strlen(m_ext_osd_content[1])); 
                }
            }
        }
#endif
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_VIDEOOSD invalid message length\n");
    }
}

void CAvAppMaster::Aa_message_handle_ViewOsdInfo(const char *msg, int length)
{
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        paramDeviceViewOsd_t *pViewOsdInfo = (paramDeviceViewOsd_t *)pMessage_head->body;
        m_start_work_env[MESSAGE_CONFIG_MANAGE_VIEW_OSD_PARAM_GET] = true;

#if (defined(_AV_PLATFORM_HI3520D_V300_)||defined(_AV_PLATFORM_HI3515_))		//6AII_AV12机型不需要标准的osd叠加
		if((pgAvDeviceInfo->Adi_product_type() == OSD_TEST_FLAG)||(pgAvDeviceInfo->Adi_product_type() == PTD_6A_I))
		{
			return;
		}
		else
#endif
		{
	        av_video_encoder_para_t video_encoder_para;
	        memset(&video_encoder_para, 0 ,sizeof(av_video_encoder_para_t));
	        if(memcmp(m_chn_name,pViewOsdInfo->szChnName,sizeof(m_chn_name)) != 0)
	        {
	            for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
	            {
	                memset(m_chn_name[chn],0 , sizeof(m_chn_name[chn]));
	                strncpy(m_chn_name[chn],pViewOsdInfo->szChnName[chn], 16);
	                int phy_audio_chn_num = chn;
	                pgAvDeviceInfo->Adi_get_audio_chn_id(chn , phy_audio_chn_num);

	                /*main stream osd*/
	                if (m_main_encoder_state.test(chn))
	                {
	                    Aa_get_encoder_para(_AST_MAIN_VIDEO_, chn, &video_encoder_para, phy_audio_chn_num, NULL);
	                    strcpy(video_encoder_para.channel_name,pViewOsdInfo->szChnName[chn]);
	                    m_pAv_device->Ad_modify_encoder(_AST_MAIN_VIDEO_, chn, &video_encoder_para);
	                }

	                /*sub stream osd*/
	                if (m_sub_encoder_state.test(chn))
	                {
	                    Aa_get_encoder_para(_AST_SUB_VIDEO_, chn, &video_encoder_para, phy_audio_chn_num, NULL);
	                    strcpy(video_encoder_para.channel_name,pViewOsdInfo->szChnName[chn]);
	                    m_pAv_device->Ad_modify_encoder(_AST_SUB_VIDEO_, chn, &video_encoder_para);
	                }

	                /*sub stream osd*/
	                if (m_sub2_encoder_state.test(chn))
	                {
	                    Aa_get_encoder_para(_AST_SUB2_VIDEO_, chn, &video_encoder_para, phy_audio_chn_num, NULL);
	                    strcpy(video_encoder_para.channel_name,pViewOsdInfo->szChnName[chn]);
	                    m_pAv_device->Ad_modify_encoder(_AST_SUB2_VIDEO_, chn, &video_encoder_para);
	                }
	            }
	        }
        }
    }
    else
    {
    DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_RegisterInfo invalid message length\n");
    }
}

#if defined(_AV_PRODUCT_CLASS_IPC_)
void CAvAppMaster::Aa_message_handle_BusInfo(const char *msg, int length)
{
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgRegisterSetting_t *pRegusterInfo = (msgRegisterSetting_t *)pMessage_head->body;
        DEBUG_MESSAGE("the bus number is:%s  device id is:%s bus self number is:%s \n", pRegusterInfo->stuRegisterSetting.szVehicleNumber, \
            pRegusterInfo->stuRegisterSetting.szDeviceId, pRegusterInfo->stuRegisterSetting.szVechicleId);
        if((0 != strlen(pRegusterInfo->stuRegisterSetting.szVehicleNumber)) && (0 != strcmp(m_osd_content[0], pRegusterInfo->stuRegisterSetting.szVehicleNumber)))
        {
            memset(m_osd_content[0], 0, sizeof(m_osd_content[0]));
            strncpy(m_osd_content[0], pRegusterInfo->stuRegisterSetting.szVehicleNumber, sizeof(m_osd_content[0]));
            if(0 != m_start_work_flag)
            {
                for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); ++chn)
                {
                    av_video_encoder_para_t video_encoder;
                    Aa_get_encoder_para(_AST_MAIN_VIDEO_, chn, &video_encoder, -1, NULL);
                    if(m_main_encoder_state.test(chn)) 
                     {
                        if(video_encoder.have_bus_number_osd)
                        {
                            m_pAv_device->Ad_update_extra_osd_content(_AST_MAIN_VIDEO_, chn, _AON_BUS_NUMBER_, m_osd_content[0], strlen(m_osd_content[0]));                           
                        }
                     }
                    
                     if(m_sub_encoder_state.test(chn))
                     {
                        if(video_encoder.have_bus_number_osd)
                        {
                            m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, chn, _AON_BUS_NUMBER_, m_osd_content[0], strlen(m_osd_content[0]));  
                        } 
                     }
                }
            }
        }

        if((0 != strlen(pRegusterInfo->stuRegisterSetting.szVechicleId)) && (0 != strcmp(m_osd_content[3], pRegusterInfo->stuRegisterSetting.szVechicleId)))
        {
            memset(m_osd_content[3], 0, sizeof(m_osd_content[3]));
            strncpy(m_osd_content[3], pRegusterInfo->stuRegisterSetting.szVechicleId, sizeof(m_osd_content[3]));
            if(0 != m_start_work_flag)
            {
                for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); ++chn)
                {
                    av_video_encoder_para_t video_encoder;
                    Aa_get_encoder_para(_AST_MAIN_VIDEO_, chn, &video_encoder, -1, NULL);
                    if(m_main_encoder_state.test(chn)) 
                     {
                        if(video_encoder.have_bus_selfnumber_osd)
                        {
                            m_pAv_device->Ad_update_extra_osd_content(_AST_MAIN_VIDEO_, chn, _AON_SELFNUMBER_INFO_, m_osd_content[3], strlen(m_osd_content[3]));                           
                        }
                     }
                    
                     if(m_sub_encoder_state.test(chn))
                     {
                        if(video_encoder.have_bus_selfnumber_osd)
                        {
                            m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, chn, _AON_SELFNUMBER_INFO_, m_osd_content[3], strlen(m_osd_content[3]));  
                        } 
                     }
                }
            }
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_RegisterInfo invalid message length\n");
    }    
}
#else
void CAvAppMaster::Aa_message_handle_RegisterInfo(const char *msg, int length)
{
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgRegisterSetting_t *pRegusterInfo = (msgRegisterSetting_t *)pMessage_head->body;
        m_start_work_env[MESSAGE_CONFIG_MANAGE_REGISTER_PARAM_GET] = true;
        
#if (defined(_AV_PLATFORM_HI3520D_V300_)||defined(_AV_PLATFORM_HI3515_))		//6AII_AV12机型不需要标准的osd叠加
		if((pgAvDeviceInfo->Adi_product_type() == OSD_TEST_FLAG)||(pgAvDeviceInfo->Adi_product_type() == PTD_6A_I))
		{
			return;
		}
		else
#endif
		{
	        av_video_encoder_para_t video_encoder_para;
	        if((strcmp(m_bus_number,pRegusterInfo->stuRegisterSetting.szVehicleNumber) != 0) || (strcmp(m_bus_selfsequeue_number,pRegusterInfo->stuRegisterSetting.szVechicleId) != 0))
	        {
	            strcpy(m_bus_number,pRegusterInfo->stuRegisterSetting.szVehicleNumber);
	            strcpy(m_bus_selfsequeue_number,pRegusterInfo->stuRegisterSetting.szVechicleId);
	            for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
	            {
	                int phy_audio_chn_num = chn;
	                pgAvDeviceInfo->Adi_get_audio_chn_id(chn , phy_audio_chn_num);
	                
	                /*main stream osd*/
	                if (m_main_encoder_state.test(chn))
	                {
	                    Aa_get_encoder_para(_AST_MAIN_VIDEO_, chn, &video_encoder_para, phy_audio_chn_num, NULL);
	                    strcpy(video_encoder_para.bus_number,pRegusterInfo->stuRegisterSetting.szVehicleNumber);
	                    strcpy(video_encoder_para.bus_selfnumber,pRegusterInfo->stuRegisterSetting.szVechicleId);
	                    m_pAv_device->Ad_modify_encoder(_AST_MAIN_VIDEO_, chn, &video_encoder_para);
	                }

	                /*sub stream osd*/
	                if (m_sub_encoder_state.test(chn))
	                {
	                    Aa_get_encoder_para(_AST_SUB_VIDEO_, chn, &video_encoder_para, phy_audio_chn_num, NULL);
	                    strcpy(video_encoder_para.bus_number,pRegusterInfo->stuRegisterSetting.szVehicleNumber);
	                    strcpy(video_encoder_para.bus_selfnumber,pRegusterInfo->stuRegisterSetting.szVechicleId);
	                    m_pAv_device->Ad_modify_encoder(_AST_SUB_VIDEO_, chn, &video_encoder_para);
	                }
	                        
	                /*sub stream osd*/
	                if (m_sub2_encoder_state.test(chn))
	                {
	                    Aa_get_encoder_para(_AST_SUB2_VIDEO_, chn, &video_encoder_para, phy_audio_chn_num, NULL);                            
	                    strcpy(video_encoder_para.bus_number,pRegusterInfo->stuRegisterSetting.szVehicleNumber);
	                    strcpy(video_encoder_para.bus_selfnumber,pRegusterInfo->stuRegisterSetting.szVechicleId);
	                    m_pAv_device->Ad_modify_encoder(_AST_SUB2_VIDEO_, chn, &video_encoder_para);
	                }
	            }
	        }    
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_RegisterInfo invalid message length\n");
    }    
}
#endif
//0628 
#if !defined(_AV_PRODUCT_CLASS_IPC_)
void CAvAppMaster::Aa_message_handle_GetStationName(const char *msg, int length)
{
    DEBUG_CRITICAL("----Aa_message_handle_GetStationName----\n");
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgReportStationMsg_t *pStationName= (msgReportStationMsg_t *)pMessage_head->body;
        LeaveStation * pleaveName;
        av_video_encoder_para_t video_encoder_para;
        memset(&video_encoder_para, 0, sizeof(av_video_encoder_para_t));
        if(pStationName->type == ReportMsg_Leave)
        {
            pleaveName = (LeaveStation * )pStationName->body;
            char station_name[MAX_EXT_OSD_NAME_SIZE];
            memset(station_name, 0, sizeof(char)*MAX_EXT_OSD_NAME_SIZE);
            char src[MAX_EXT_OSD_NAME_SIZE] = "下一站:";
            char dst[MAX_EXT_OSD_NAME_SIZE] = "";
            int len = 0;
            if(0 == Gb2312ToUTF((unsigned char*)src , (unsigned char*)dst))
            {
                len = strlen(dst);
                dst[len] = '\0';
                len ++;
            }
            
            int length = sprintf(station_name, "%s%s", dst, pleaveName->nextStation.stationName[0]);
            
            station_name[length]='\0';
            DEBUG_CRITICAL("---lsh lentth =%d name =%s \n",length,station_name);
            
            if(strcmp(m_station_name, station_name) != 0)
            {
                
                strcpy(m_station_name, station_name);
                m_pAv_device->Ad_get_AvEncoder()->CacheOsdFond(station_name, true);
                for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
                {
                    int phy_audio_chn_num = chn;
                    pgAvDeviceInfo->Adi_get_audio_chn_id(chn , phy_audio_chn_num);
                    
                    if (m_main_encoder_state.test(chn))
                    {
                        strcpy(video_encoder_para.osd1_content, station_name);
                        m_pAv_device->Ad_update_common_osd_content(_AST_MAIN_VIDEO_, phy_audio_chn_num, _AON_COMMON_OSD1_, station_name ,strlen(station_name));
                    }
                    if(m_sub_encoder_state.test(chn))
                    {
                        strcpy(video_encoder_para.osd1_content, station_name);
                        m_pAv_device->Ad_update_common_osd_content(_AST_SUB_VIDEO_, phy_audio_chn_num, _AON_COMMON_OSD1_, station_name ,strlen(station_name));
                    }
                }
            }
        }
           
     
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_GetStationName invalid message length\n");
    }   
}
//0730

void CAvAppMaster::Aa_message_handle_Enable_Common_OSD(const char *msg, int length)
{
#if (defined(_AV_PLATFORM_HI3520D_V300_)||defined(_AV_PLATFORM_HI3515_))		//6AII_AV12机型不需要标准的osd叠加
	if((pgAvDeviceInfo->Adi_product_type() == OSD_TEST_FLAG)||(pgAvDeviceInfo->Adi_product_type() == PTD_6A_I))
	{
		return;
	}
#endif
    #if 1
    DEBUG_CRITICAL("[OSD] Aa_message_handle_Enable_Common_OSD\n");
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    char product_group[32] = {0};
    pgAvDeviceInfo->Adi_get_product_group(product_group, sizeof(product_group));
    char customer_name[32] = {0};
    pgAvDeviceInfo->Adi_get_customer_name(customer_name,sizeof(customer_name));
    
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgCommonOSD_t *pCommonOSD = (msgCommonOSD_t *)pMessage_head->body;
        if(pCommonOSD != NULL)
        {
            unsigned int chn_mask = 0;
            av_video_encoder_para_t video_encoder_para;
            memset(&video_encoder_para, 0, sizeof(av_video_encoder_para_t));
            
            msgCommonOSDBody_t *pCommonBody = (msgCommonOSDBody_t *)pCommonOSD->ucosdbody;
            //msgCommonOSDBody_t *pCommonBody2 = NULL;
            
            DEBUG_CRITICAL("[OSD] ucEnableOSD =0x%x ucosd1_Pack_len =%d ucosd2_Pack_len =%d \n ",
                pCommonOSD->ucEnableOSD, pCommonOSD->ucosd1_Pack_len, pCommonOSD->ucosd2_Pack_len);
            
            if(pCommonOSD->ucEnableOSD == 0x01 || pCommonOSD->ucEnableOSD == 0x02)
            {
                DEBUG_CRITICAL("[OSD] uiosd_x =%d uiosd_y=%d uccontent_len =%d uccontentbody =%s \n",
                pCommonBody->uiosd_x, pCommonBody->uiosd_y, pCommonBody->uccontent_len, pCommonBody->uccontentbody);
            }
            if(pCommonOSD->ucEnableOSD == 0x03)
            {
                DEBUG_CRITICAL("[OSD]   osd1 uiosd_x =%d uiosd_y=%d uccontent_len =%d uccontentbody =%s \n",
                pCommonBody->uiosd_x, pCommonBody->uiosd_y, pCommonBody->uccontent_len, pCommonBody->uccontentbody);
                msgCommonOSDBody_t *pCommonBody2  = (msgCommonOSDBody_t *)(pCommonOSD->ucosdbody+pCommonOSD->ucosd1_Pack_len);
                DEBUG_CRITICAL("[OSD]   osd2 uiosd_x =%d uiosd_y=%d uccontent_len =%d uccontentbody =%s \n",
                pCommonBody2->uiosd_x, pCommonBody2->uiosd_y, pCommonBody2->uccontent_len, pCommonBody2->uccontentbody);
            }
            
            if(pCommonOSD->ucEnableOSD == 0x01)
            {

                DEBUG_WARNING("[OSD] Common OSD1 enable...\n");
                
                if(NULL != pCommonBody)
                {
                    if(0 != strcmp(pCommonBody->uccontentbody, m_ext_osd_content[0]))
                    {                        
                        strncpy(m_ext_osd_content[0], pCommonBody->uccontentbody, sizeof(m_ext_osd_content[0]));
                        m_ext_osd_content[0][sizeof(m_ext_osd_content[0])-1] = '\0';
                    }
                    //! for 04.480
                    chn_mask = pCommonBody->ucChnMask;
                    
                    if((m_ext_osd_x[0] != pCommonBody->uiosd_x) || (m_ext_osd_y[0] != pCommonBody->uiosd_y))
                    {
                        DEBUG_WARNING("[OSD] Common OSD1 rebuilt...\n");
                        
                        for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
                        {
                            if (GCL_BIT_VAL_TEST(chn_mask,chn))
                            {
                                continue;   
                            }
                            if (m_main_encoder_state.test(chn))
                            {
                                Aa_get_encoder_para(_AST_MAIN_VIDEO_, chn, &video_encoder_para, -1, NULL);
                                //video_encoder_para.have_common1_osd = 1;
                                video_encoder_para.common1_osd_x = pCommonBody->uiosd_x;
                                video_encoder_para.common1_osd_y = pCommonBody->uiosd_y;
                                memcpy(video_encoder_para.osd1_content, pCommonBody->uccontentbody, pCommonBody->uccontent_len);
                                m_pAv_device->Ad_modify_encoder(_AST_MAIN_VIDEO_, chn, &video_encoder_para, true);
                            }

                            //sub stream osd
                            if (m_sub_encoder_state.test(chn))
                            {   
                                Aa_get_encoder_para(_AST_SUB_VIDEO_, chn, &video_encoder_para, -1, NULL);
                                //video_encoder_para.have_common1_osd = 1;
                                video_encoder_para.common1_osd_x = pCommonBody->uiosd_x;
                                video_encoder_para.common1_osd_y = pCommonBody->uiosd_y;
                                memcpy(video_encoder_para.osd1_content, pCommonBody->uccontentbody, pCommonBody->uccontent_len);
                                m_pAv_device->Ad_modify_encoder(_AST_SUB_VIDEO_, chn, &video_encoder_para, true);
                            }
                                if ((strcmp(product_group, "ITS") == 0)||(strcmp(customer_name, "CUSTOMER_04.471") == 0)||\
                                    (((PTD_X1 == pgAvDeviceInfo->Adi_product_type())||(PTD_X1_AHD == pgAvDeviceInfo->Adi_product_type())) &&(strcmp(customer_name, "CUSTOMER_04.718") == 0)))
                            {
                                if (m_sub2_encoder_state.test(chn))
                                {   
                                    Aa_get_encoder_para(_AST_SUB2_VIDEO_, chn, &video_encoder_para, -1, NULL);
                                    //video_encoder_para.have_common1_osd = 1;
                                    video_encoder_para.common1_osd_x = pCommonBody->uiosd_x;
                                    video_encoder_para.common1_osd_y = pCommonBody->uiosd_y;
                                    memcpy(video_encoder_para.osd1_content, pCommonBody->uccontentbody, pCommonBody->uccontent_len);
                                    m_pAv_device->Ad_modify_encoder(_AST_SUB2_VIDEO_, chn, &video_encoder_para, true);
                                }
                            }
                       
                        }
                        
                    }
                    else
                    {
                        
                        DEBUG_WARNING("[OSD] Common OSD1 update...\n");
                        
                        for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
                        {
                            //int phy_audio_chn_num = chn;
                            //pgAvDeviceInfo->Adi_get_audio_chn_id(chn , phy_audio_chn_num);
                            
                            if (GCL_BIT_VAL_TEST(chn_mask,chn))
                            {
                                continue;
                            }
                           
                            if (m_main_encoder_state.test(chn))
                            {
                                //DEBUG_CRITICAL("chn =%d uccontentbody =%d uccontent_len =%d \n",chn, strlen(pCommonBody->uccontentbody)+1,pCommonBody->uccontent_len);
                                m_pAv_device->Ad_update_common_osd_content(_AST_MAIN_VIDEO_, chn, _AON_COMMON_OSD1_, m_ext_osd_content[0], strlen(m_ext_osd_content[0]));
                            }
                            if(m_sub_encoder_state.test(chn))
                            {
                                m_pAv_device->Ad_update_common_osd_content(_AST_SUB_VIDEO_, chn, _AON_COMMON_OSD1_, m_ext_osd_content[0], strlen(m_ext_osd_content[0]));
                            }
                            if((strcmp(product_group, "ITS") == 0)||(strcmp(customer_name, "CUSTOMER_04.471") == 0)||\
                                ((PTD_X1 == pgAvDeviceInfo->Adi_product_type() || PTD_X1_AHD== pgAvDeviceInfo->Adi_product_type()) &&(strcmp(customer_name, "CUSTOMER_04.718") == 0)))
                            {
                                if(m_sub2_encoder_state.test(chn))
                                {
                                    m_pAv_device->Ad_update_common_osd_content(_AST_SUB2_VIDEO_, chn, _AON_COMMON_OSD1_, m_ext_osd_content[0], strlen(m_ext_osd_content[0]));
                                }
                            }
                        }
                    }

                    m_ext_osd_x[0] = pCommonBody->uiosd_x;
                    m_ext_osd_y[0] = pCommonBody->uiosd_y;
                }
                 

            }
            else if(pCommonOSD->ucEnableOSD == 0x02)
            {
             
                DEBUG_WARNING("[OSD] Common OSD2 enable...\n");
                
                if(NULL != pCommonBody)
                {
                    if(0 != strcmp(pCommonBody->uccontentbody, m_ext_osd_content[1]))
                    {                        
                        strncpy(m_ext_osd_content[1], pCommonBody->uccontentbody, sizeof(m_ext_osd_content[1]));
                        m_ext_osd_content[1][sizeof(m_ext_osd_content[1])-1] = '\0';
                    }
                    //!
                    chn_mask = pCommonBody->ucChnMask;
                    
                    if((m_ext_osd_x[1] != pCommonBody->uiosd_x) || (m_ext_osd_y[1] != pCommonBody->uiosd_y))
                    {
                        DEBUG_WARNING("[OSD] Common OSD2 rebuilt...\n");
                        
                        for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
                        {
                            if (GCL_BIT_VAL_TEST(chn_mask,chn))
                            {
                                continue;
                            }
                            
                            if (m_main_encoder_state.test(chn))
                            {
                                Aa_get_encoder_para(_AST_MAIN_VIDEO_, chn, &video_encoder_para, -1, NULL);
                                //video_encoder_para.have_common1_osd = 1;
                                video_encoder_para.common2_osd_x = pCommonBody->uiosd_x;
                                video_encoder_para.common2_osd_y = pCommonBody->uiosd_y;
                                memcpy(video_encoder_para.osd2_content, pCommonBody->uccontentbody, pCommonBody->uccontent_len);
                                m_pAv_device->Ad_modify_encoder(_AST_MAIN_VIDEO_, chn, &video_encoder_para, true);
                            }

                            //sub stream osd
                            if (m_sub_encoder_state.test(chn))
                            {   
                                Aa_get_encoder_para(_AST_SUB_VIDEO_, chn, &video_encoder_para, -1, NULL);
                                //video_encoder_para.have_common1_osd = 1;
                                video_encoder_para.common2_osd_x = pCommonBody->uiosd_x;
                                video_encoder_para.common2_osd_y = pCommonBody->uiosd_y;
                                memcpy(video_encoder_para.osd2_content, pCommonBody->uccontentbody, pCommonBody->uccontent_len);
                                m_pAv_device->Ad_modify_encoder(_AST_SUB_VIDEO_, chn, &video_encoder_para, true);
                            }
                            if((strcmp(product_group, "ITS") == 0)||(strcmp(customer_name, "CUSTOMER_04.471") == 0)||\
                                ((PTD_X1 == pgAvDeviceInfo->Adi_product_type() || PTD_X1_AHD== pgAvDeviceInfo->Adi_product_type()) &&(strcmp(customer_name, "CUSTOMER_04.718") == 0)))
                            {
                                if (m_sub2_encoder_state.test(chn))
                                {   
                                    Aa_get_encoder_para(_AST_SUB2_VIDEO_, chn, &video_encoder_para, -1, NULL);
                                    //video_encoder_para.have_common1_osd = 1;
                                    video_encoder_para.common2_osd_x = pCommonBody->uiosd_x;
                                    video_encoder_para.common2_osd_y = pCommonBody->uiosd_y;
                                    memcpy(video_encoder_para.osd2_content, pCommonBody->uccontentbody, pCommonBody->uccontent_len);
                                    m_pAv_device->Ad_modify_encoder(_AST_SUB2_VIDEO_, chn, &video_encoder_para, true);
                                }
                            }
                       
                        }
                        
                    }
                    else
                    {
                        
                        DEBUG_WARNING("[OSD] Common OSD2 update...\n");
                        
                        for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
                        {
                            if (GCL_BIT_VAL_TEST(chn_mask,chn))
                            {
                                continue;
                            }
                            
                            if (m_main_encoder_state.test(chn))
                            {
                                //DEBUG_CRITICAL("chn =%d uccontentbody =%d uccontent_len =%d \n",chn, strlen(pCommonBody->uccontentbody)+1,pCommonBody->uccontent_len);
                                m_pAv_device->Ad_update_common_osd_content(_AST_MAIN_VIDEO_, chn, _AON_COMMON_OSD2_, m_ext_osd_content[1], strlen(m_ext_osd_content[1]));
                            }
                            if(m_sub_encoder_state.test(chn))
                            {
                                m_pAv_device->Ad_update_common_osd_content(_AST_SUB_VIDEO_, chn, _AON_COMMON_OSD2_, m_ext_osd_content[1], strlen(m_ext_osd_content[1]));
                            }
                            if(strcmp(product_group, "ITS") == 0)
                            {
                                if(m_sub2_encoder_state.test(chn))
                                {
                                    m_pAv_device->Ad_update_common_osd_content(_AST_SUB2_VIDEO_, chn, _AON_COMMON_OSD2_, m_ext_osd_content[1], strlen(m_ext_osd_content[1]));
                                }
                            }
                        }
                    }

                    m_ext_osd_x[1] = pCommonBody->uiosd_x;
                    m_ext_osd_y[1] = pCommonBody->uiosd_y;
                }
                 

            }
            else if(pCommonOSD->ucEnableOSD == 0x03)
            {
                DEBUG_WARNING("[OSD] Common OSD1 enable...\n");
                
                if(NULL != pCommonBody)
                {
                    if(0 != strcmp(pCommonBody->uccontentbody, m_ext_osd_content[0]))
                    {
                        
                        strncpy(m_ext_osd_content[0], pCommonBody->uccontentbody, sizeof(m_ext_osd_content[0]));
                        m_ext_osd_content[0][sizeof(m_ext_osd_content[0])-1] = '\0';
                    }
                    chn_mask = pCommonBody->ucChnMask;
                    
                    if((m_ext_osd_x[0] != pCommonBody->uiosd_x) || (m_ext_osd_y[0] != pCommonBody->uiosd_y))
                    {
                        DEBUG_WARNING("[OSD] Common OSD1 rebuilt...\n");
                        
                        for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
                        {
                            if (GCL_BIT_VAL_TEST(chn_mask,chn))
                            {
                                continue;
                            }     
                            if (m_main_encoder_state.test(chn))
                            {
                                Aa_get_encoder_para(_AST_MAIN_VIDEO_, chn, &video_encoder_para, -1, NULL);
                                //video_encoder_para.have_common1_osd = 1;
                                video_encoder_para.common1_osd_x = pCommonBody->uiosd_x;
                                video_encoder_para.common1_osd_y = pCommonBody->uiosd_y;
                                memcpy(video_encoder_para.osd1_content, pCommonBody->uccontentbody, pCommonBody->uccontent_len);
                                m_pAv_device->Ad_modify_encoder(_AST_MAIN_VIDEO_, chn, &video_encoder_para, true);
                            }

                            //sub stream osd
                            if (m_sub_encoder_state.test(chn))
                            {   
                                Aa_get_encoder_para(_AST_SUB_VIDEO_, chn, &video_encoder_para, -1, NULL);
                                //video_encoder_para.have_common1_osd = 1;
                                video_encoder_para.common1_osd_x = pCommonBody->uiosd_x;
                                video_encoder_para.common1_osd_y = pCommonBody->uiosd_y;
                                memcpy(video_encoder_para.osd1_content, pCommonBody->uccontentbody, pCommonBody->uccontent_len);
                                m_pAv_device->Ad_modify_encoder(_AST_SUB_VIDEO_, chn, &video_encoder_para, true);
                            }
                            if((strcmp(product_group, "ITS") == 0)||(strcmp(customer_name, "CUSTOMER_04.471") == 0)||\
                                ((PTD_X1 == pgAvDeviceInfo->Adi_product_type() || PTD_X1_AHD== pgAvDeviceInfo->Adi_product_type()) &&(strcmp(customer_name, "CUSTOMER_04.718") == 0)))
                            {
                                if (m_sub2_encoder_state.test(chn))
                                {   
                                    Aa_get_encoder_para(_AST_SUB2_VIDEO_, chn, &video_encoder_para, -1, NULL);
                                    //video_encoder_para.have_common1_osd = 1;
                                    video_encoder_para.common1_osd_x = pCommonBody->uiosd_x;
                                    video_encoder_para.common1_osd_y = pCommonBody->uiosd_y;
                                    memcpy(video_encoder_para.osd1_content, pCommonBody->uccontentbody, pCommonBody->uccontent_len);
                                    m_pAv_device->Ad_modify_encoder(_AST_SUB2_VIDEO_, chn, &video_encoder_para, true);
                                }
                            }
                       
                        }
                        
                    }
                    else
                    {
                        
                        DEBUG_WARNING("[OSD] Common OSD1 update...\n");
                        
                        for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
                        {
                            //int phy_audio_chn_num = chn;
                            //pgAvDeviceInfo->Adi_get_audio_chn_id(chn , phy_audio_chn_num);
                            if (GCL_BIT_VAL_TEST(chn_mask,chn))
                            {
                                continue;
                            }
                            if (m_main_encoder_state.test(chn))
                            {
                                //DEBUG_CRITICAL("chn =%d uccontentbody =%d uccontent_len =%d \n",chn, strlen(pCommonBody->uccontentbody)+1,pCommonBody->uccontent_len);
                                m_pAv_device->Ad_update_common_osd_content(_AST_MAIN_VIDEO_, chn, _AON_COMMON_OSD1_, m_ext_osd_content[0], strlen(m_ext_osd_content[0]));
                            }
                            if(m_sub_encoder_state.test(chn))
                            {
                                m_pAv_device->Ad_update_common_osd_content(_AST_SUB_VIDEO_, chn, _AON_COMMON_OSD1_, m_ext_osd_content[0], strlen(m_ext_osd_content[0]));
                            }
                            if((strcmp(product_group, "ITS") == 0)||(strcmp(customer_name, "CUSTOMER_04.471") == 0)||\
                                ((PTD_X1 == pgAvDeviceInfo->Adi_product_type() || PTD_X1_AHD== pgAvDeviceInfo->Adi_product_type()) &&(strcmp(customer_name, "CUSTOMER_04.718") == 0)))
                            {
                                if(m_sub2_encoder_state.test(chn))
                                {
                                    m_pAv_device->Ad_update_common_osd_content(_AST_SUB2_VIDEO_, chn, _AON_COMMON_OSD1_, m_ext_osd_content[0], strlen(m_ext_osd_content[0]));
                                }
                            }
                        }
                    }

                    m_ext_osd_x[0] = pCommonBody->uiosd_x;
                    m_ext_osd_y[0] = pCommonBody->uiosd_y;
                }
                
                DEBUG_WARNING("[OSD] Common OSD2 enable...\n");
                msgCommonOSDBody_t *pCommonBody2  = (msgCommonOSDBody_t *)(pCommonOSD->ucosdbody+pCommonOSD->ucosd1_Pack_len);

                if(NULL != pCommonBody2)
                {
                    if(0 != strcmp(pCommonBody2->uccontentbody, m_ext_osd_content[1]))
                    {                        
                        strncpy(m_ext_osd_content[1], pCommonBody2->uccontentbody, sizeof(m_ext_osd_content[1]));
                        m_ext_osd_content[1][sizeof(m_ext_osd_content[1])-1] = '\0';
                    }
                    chn_mask = pCommonBody2->ucChnMask;
                  
                    if((m_ext_osd_x[1] != pCommonBody2->uiosd_x) || (m_ext_osd_y[1] != pCommonBody2->uiosd_y))
                    {
                        DEBUG_WARNING("[OSD] Common OSD2 rebuilt...\n");
                        
                        for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
                        {
                            
                            if (GCL_BIT_VAL_TEST(chn_mask,chn))
                            {
                                continue;
                            }
                            if (m_main_encoder_state.test(chn))
                            {
                                Aa_get_encoder_para(_AST_MAIN_VIDEO_, chn, &video_encoder_para, -1, NULL);
                                //video_encoder_para.have_common1_osd = 1;
                                video_encoder_para.common2_osd_x = pCommonBody2->uiosd_x;
                                video_encoder_para.common2_osd_y = pCommonBody2->uiosd_y;
                                memcpy(video_encoder_para.osd2_content, pCommonBody2->uccontentbody, pCommonBody2->uccontent_len);
                                m_pAv_device->Ad_modify_encoder(_AST_MAIN_VIDEO_, chn, &video_encoder_para, true);
                            }

                            //sub stream osd
                            if (m_sub_encoder_state.test(chn))
                            {   
                                Aa_get_encoder_para(_AST_SUB_VIDEO_, chn, &video_encoder_para, -1, NULL);
                                //video_encoder_para.have_common1_osd = 1;
                                video_encoder_para.common2_osd_x = pCommonBody2->uiosd_x;
                                video_encoder_para.common2_osd_y = pCommonBody2->uiosd_y;
                                memcpy(video_encoder_para.osd2_content, pCommonBody2->uccontentbody, pCommonBody2->uccontent_len);
                                m_pAv_device->Ad_modify_encoder(_AST_SUB_VIDEO_, chn, &video_encoder_para, true);
                            }
                            if((strcmp(product_group, "ITS") == 0)||(strcmp(customer_name, "CUSTOMER_04.471") == 0)||\
                                ((PTD_X1 == pgAvDeviceInfo->Adi_product_type() || PTD_X1_AHD== pgAvDeviceInfo->Adi_product_type()) &&(strcmp(customer_name, "CUSTOMER_04.718") == 0)))
                            {
                                if (m_sub2_encoder_state.test(chn))
                                {   
                                    Aa_get_encoder_para(_AST_SUB2_VIDEO_, chn, &video_encoder_para, -1, NULL);
                                    //video_encoder_para.have_common1_osd = 1;
                                    video_encoder_para.common2_osd_x = pCommonBody2->uiosd_x;
                                    video_encoder_para.common2_osd_y = pCommonBody2->uiosd_y;
                                    memcpy(video_encoder_para.osd2_content, pCommonBody2->uccontentbody, pCommonBody2->uccontent_len);
                                    m_pAv_device->Ad_modify_encoder(_AST_SUB2_VIDEO_, chn, &video_encoder_para, true);
                                }
                            }
                       
                        }
                        
                    }
                    else
                    {
                        
                        DEBUG_WARNING("[OSD] Common OSD2 update...\n");
                        
                        for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
                        {
                            
                            if (GCL_BIT_VAL_TEST(chn_mask,chn))
                            {
                                continue;
                            }

                            if (m_main_encoder_state.test(chn))
                            {
                                //DEBUG_CRITICAL("chn =%d uccontentbody =%d uccontent_len =%d \n",chn, strlen(pCommonBody->uccontentbody)+1,pCommonBody->uccontent_len);
                                m_pAv_device->Ad_update_common_osd_content(_AST_MAIN_VIDEO_, chn, _AON_COMMON_OSD2_, m_ext_osd_content[1], strlen(m_ext_osd_content[1]));
                            }
                            if(m_sub_encoder_state.test(chn))
                            {
                                m_pAv_device->Ad_update_common_osd_content(_AST_SUB_VIDEO_, chn, _AON_COMMON_OSD2_, m_ext_osd_content[1], strlen(m_ext_osd_content[1]));
                            }
                            if((strcmp(product_group, "ITS") == 0)||(strcmp(customer_name, "CUSTOMER_04.471") == 0)||\
                                ((PTD_X1 == pgAvDeviceInfo->Adi_product_type() || PTD_X1_AHD == pgAvDeviceInfo->Adi_product_type()) &&(strcmp(customer_name, "CUSTOMER_04.718") == 0)))
                            {
                                if(m_sub2_encoder_state.test(chn))
                                {
                                    m_pAv_device->Ad_update_common_osd_content(_AST_SUB2_VIDEO_, chn, _AON_COMMON_OSD2_, m_ext_osd_content[1], strlen(m_ext_osd_content[1]));
                                }
                            }
                        }
                    }

                    m_ext_osd_x[1] = pCommonBody2->uiosd_x;
                    m_ext_osd_y[1] = pCommonBody2->uiosd_y;
                }
                
            }
            
        }
        else
        {
            DEBUG_ERROR( "Aa_message_handle_Enable_Common_OSD param null\n");
        }
    }
    else
    {
        DEBUG_ERROR( "Aa_message_handle_Enable_Common_OSD invalid message length\n");
    }
    #endif
}
#endif

#if !defined(_AV_PRODUCT_CLASS_IPC_)
void CAvAppMaster::Aa_message_handle_SpeedSource(const char *msg, int length)
{
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgSpeedAlarmSetting_t *pSpeedInfo = (msgSpeedAlarmSetting_t *)pMessage_head->body;
        pgAvDeviceInfo->Adi_Set_SpeedSource_Info(&(pSpeedInfo->stuSpeedAlarmSetting));
        m_start_work_env[MESSAGE_CONFIG_MANAGE_SPEED_ALARM_PARAM_GET] = true;
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_SpeedSource invalid message length\n");
    }    
}
#endif

#if (defined(_AV_PLATFORM_HI3520D_V300_) || defined(_AV_PLATFORM_HI3515_))
void CAvAppMaster::Aa_message_handle_EXTEND_OSD(const char *msg, int length)
{
	if(PTD_6A_I != pgAvDeviceInfo->Adi_product_type() && PTD_6A_II_AVX != pgAvDeviceInfo->Adi_product_type())
	{
		return ;
	}
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgExtendOsd_t *pMsgExtendOsd = (msgExtendOsd_t *)pMessage_head->body;
        if(pMsgExtendOsd->ucOsdNum <= 0)
        {
			DEBUG_CRITICAL("EXTEND_OSD: Recv osd overlay task num error, num = %d\n", pMsgExtendOsd->ucOsdNum);
			return;
        }
		if ((m_start_work_flag != 0) && (Aa_everything_is_ready() == 0))
		{
			rm_uint8_t maxChnNum = pgAvDeviceInfo->Adi_get_vi_chn_num();
			av_video_encoder_para_t video_encoder_para;
			memset(&video_encoder_para, 0, sizeof(av_video_encoder_para_t));
			rm_uint32_t chnChangeMask = 0x0;
			
			memset(&m_bOsdChanged, 0x0, sizeof(m_bOsdChanged));
	        for(rm_uint8_t index = 0 ; index < pMsgExtendOsd->ucOsdNum ; ++index)
	        {
	        	OsdItem_t* pOsdItem = (OsdItem_t*)pMsgExtendOsd->body + index;
	        	if(pOsdItem->ucAlign <= 2 && pOsdItem->ucOsdChn < maxChnNum && pOsdItem->ucOsdId < OVERLAY_MAX_NUM_VENC)
	        	{
					//叠加参数保存///
					pOsdItem->szContent[MAX_EXTEND_OSD_LENGTH - 1] = '\0';
					m_bOsdChanged[pOsdItem->ucOsdChn][pOsdItem->ucOsdId] = 1;
					if(memcmp(&(m_stuExtendOsdParam[pOsdItem->ucOsdChn][pOsdItem->ucOsdId]), pOsdItem, sizeof(OsdItem_t)) == 0)
					{
						m_bOsdChanged[pOsdItem->ucOsdChn][pOsdItem->ucOsdId] = 0;
						continue;
					}
					memcpy(&(m_stuExtendOsdParam[pOsdItem->ucOsdChn][pOsdItem->ucOsdId]), pOsdItem, sizeof(OsdItem_t));
					GCL_BIT_VAL_SET(chnChangeMask, pOsdItem->ucOsdChn);
					
					DEBUG_WARNING("EXTEND_OSD: Recv osd task, index = %d, id = %d, type = %d, chn = %d, enable = %d, align = %d, color = %d, x = %d, y = %d, len = %d, content = %s\n", \
						index, pOsdItem->ucOsdId, pOsdItem->ucOsdType, pOsdItem->ucOsdChn, pOsdItem->ucEnable, pOsdItem->ucAlign, pOsdItem->ucColor, pOsdItem->usOsdX, pOsdItem->usOsdY, strlen(pOsdItem->szContent), pOsdItem->szContent);
	        	}
	        	else
	        	{
					DEBUG_ERROR("EXTEND_OSD: Recv osd overlay task, param illegel\n");
	        	}
			}
			DEBUG_CRITICAL("EXTEND_OSD: Recv osd overlay task, num = %d, changeMAsk = 0x%x\n", pMsgExtendOsd->ucOsdNum, chnChangeMask);

			for(int chn = 0 ;chn < maxChnNum ; ++chn)
			{
				if(GCL_BIT_VAL_TEST(chnChangeMask, chn))
				{
					int phy_audio_chn_num = chn;
					pgAvDeviceInfo->Adi_get_audio_chn_id(chn , phy_audio_chn_num);

					if(m_main_encoder_state.test(chn))
					{
						Aa_get_encoder_para(_AST_MAIN_VIDEO_, chn, &video_encoder_para, phy_audio_chn_num, NULL);
						m_pAv_device->Ad_modify_encoder(_AST_MAIN_VIDEO_, chn, &video_encoder_para, true);
					}
					
					if(m_sub_encoder_state.test(chn))
					{
						Aa_get_encoder_para(_AST_SUB_VIDEO_, chn, &video_encoder_para, phy_audio_chn_num, NULL);
						m_pAv_device->Ad_modify_encoder(_AST_SUB_VIDEO_, chn, &video_encoder_para, true);
					}
					
					if(m_sub2_encoder_state.test(chn))
					{
						Aa_get_encoder_para(_AST_SUB2_VIDEO_, chn, &video_encoder_para, phy_audio_chn_num, NULL);
						m_pAv_device->Ad_modify_encoder(_AST_SUB2_VIDEO_, chn, &video_encoder_para, true);
					}
				}
			}
			
        }
    }
    else
    {
        DEBUG_ERROR("EXTEND_OSD: Recv osd overlay task failed, invalid message length(%d)\n", length);
    }
}

#endif

void CAvAppMaster::Aa_message_handle_TIMESETTING(const char *msg, int length)
{
    SMessageHead *pMessage_head = (SMessageHead*)msg;

    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgTimeSetting_t *pTime_setting = (msgTimeSetting_t *)pMessage_head->body;
        if( !m_pTime_setting )
        {
            _AV_FATAL_((m_pTime_setting = new msgTimeSetting_t) == NULL, "CAvAppMaster::Aa_message_handle_TIMESETTING OUT OF MEMORY\n");
        }
        
        memcpy(m_pTime_setting, pTime_setting, sizeof(msgTimeSetting_t));
        m_start_work_env[MESSAGE_CONFIG_MANAGE_TIME_PARAM_GET] = true;
#if (defined(_AV_PLATFORM_HI3520D_V300_)||defined(_AV_PLATFORM_HI3515_))		//6AII_AV12机型不需要标准的osd叠加
		if((pgAvDeviceInfo->Adi_product_type() == OSD_TEST_FLAG)||(pgAvDeviceInfo->Adi_product_type() == PTD_6A_I))
		{
			return;
		}
#endif

        if(m_start_work_flag != 0)
        {
            av_video_encoder_para_t video_encoder_para;
            int maxChannelNUm = pgAvDeviceInfo->Adi_max_channel_number();
            
            for (int chn = 0; chn < maxChannelNUm; chn++)
            {
                int phy_audio_chn_num = chn;
                pgAvDeviceInfo->Adi_get_audio_chn_id(chn , phy_audio_chn_num);
                    
                if (m_main_encoder_state.test(chn))
                {
                    Aa_get_encoder_para(_AST_MAIN_VIDEO_, chn, &video_encoder_para, phy_audio_chn_num, NULL);
                    m_pAv_device->Ad_modify_encoder(_AST_MAIN_VIDEO_, chn, &video_encoder_para);
                }

                if (m_sub_encoder_state.test(chn))
                {
                    Aa_get_encoder_para(_AST_SUB_VIDEO_, chn, &video_encoder_para, phy_audio_chn_num, NULL);
                    m_pAv_device->Ad_modify_encoder(_AST_SUB_VIDEO_, chn, &video_encoder_para);
                }

                if (m_sub2_encoder_state.test(chn))
                {
                    Aa_get_encoder_para(_AST_SUB2_VIDEO_, chn, &video_encoder_para, phy_audio_chn_num, NULL);
                    m_pAv_device->Ad_modify_encoder(_AST_SUB2_VIDEO_, chn, &video_encoder_para);
                }
            }
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_TIMESETTING invalid message length\n");
    }
}

void CAvAppMaster::Aa_message_handle_auto_adapt_encoder( const char* msg, int length )
{
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    msgNetParamSubEncode_t *pnet_para = NULL;
    msgVideoEncodeSetting_t encode_para;
    
    if ((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        // DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_auto_adapt_encoder\n");
        
        pnet_para = (msgNetParamSubEncode_t *)pMessage_head->body;
        if(pnet_para->chn < pgAvDeviceInfo->Adi_get_vi_chn_num())
        {
            memset(&encode_para, 0, sizeof(msgVideoEncodeSetting_t));
            if(pgAvDeviceInfo->Adi_get_video_source(pnet_para->chn) != _VS_DIGITAL_)
            {
                DEBUG_WARNING("Chn:%d, enable:%d, bitrate:%d,framerate:%d, resoulution:%d, brmode:%d \n",pnet_para->chn, pnet_para->ucEnable,\
                    pnet_para->ulBitrate, pnet_para->ucFrameRate, pnet_para->ucResolution, pnet_para->ucBrMode);
                GCL_BIT_VAL_SET(encode_para.mask, pnet_para->chn);
                encode_para.stuVideoEncodeSetting[pnet_para->chn].ucChnEnable = pnet_para->ucEnable;
                encode_para.stuVideoEncodeSetting[pnet_para->chn].uiBitrate = pnet_para->ulBitrate;
                encode_para.stuVideoEncodeSetting[pnet_para->chn].ucFrameType = pnet_para->ucFrameType;
                encode_para.stuVideoEncodeSetting[pnet_para->chn].ucFrameRate = pnet_para->ucFrameRate;
                encode_para.stuVideoEncodeSetting[pnet_para->chn].ucNormalQuality = pnet_para->ucQuality;
                encode_para.stuVideoEncodeSetting[pnet_para->chn].ucAlarmQuality = pnet_para->ucQuality;
                encode_para.stuVideoEncodeSetting[pnet_para->chn].ucResolution = pnet_para->ucResolution;
                encode_para.stuVideoEncodeSetting[pnet_para->chn].ucBrMode = pnet_para->ucBrMode;
                if (encode_para.stuVideoEncodeSetting[pnet_para->chn].uiBitrate < 2)
                {
                    encode_para.stuVideoEncodeSetting[pnet_para->chn].uiBitrate = 2;
                }
                if(pnet_para->ucFrameRate <= 3)
                {
                    encode_para.stuVideoEncodeSetting[pnet_para->chn].ucBrMode = 0;
                }
                else
                {
                    encode_para.stuVideoEncodeSetting[pnet_para->chn].ucBrMode = 1;
                }
                encode_para.stuVideoEncodeSetting[pnet_para->chn].ucReserved[0] = pnet_para->bandlevel;
                if (0 == encode_para.stuVideoEncodeSetting[pnet_para->chn].ucFrameRate)
                {
                    encode_para.stuVideoEncodeSetting[pnet_para->chn].ucFrameRate = 2;
                }
                if(m_pMain_stream_encoder != NULL)
                {
                    encode_para.stuVideoEncodeSetting[pnet_para->chn].ucAudioEnable = m_pMain_stream_encoder->stuVideoEncodeSetting[pnet_para->chn].ucAudioEnable;
                }
                
                Aa_modify_encoder_para(_AST_SUB2_VIDEO_, &encode_para);
            }
        }

        msgNetParamSubEncodeAck_t net_treansfer_respones;
        memset(&net_treansfer_respones, 0, sizeof(msgNetParamSubEncodeAck_t));
        for (int chn_index = 0; chn_index < pgAvDeviceInfo->Adi_get_vi_chn_num(); chn_index++)
        {
            if (m_sub2_encoder_state.test(chn_index))
            {
                GCL_BIT_VAL_SET(net_treansfer_respones.uiChannelMask, chn_index);
            }
        }
            
        Common::N9M_MSGResponseMessage(m_message_client_handle, pMessage_head, (char *)&net_treansfer_respones, sizeof(msgNetParamSubEncodeAck_t));

    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_auto_adapt_encoder invalid message length\n");
    }
    
    return;
}

void CAvAppMaster::Aa_message_handle_request_iframe( const char* msg, int length )
{
    SMessageHead *head = (SMessageHead*) msg;

    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        msgRequestIFrame_t *p_para = (msgRequestIFrame_t *)head->body;
        av_video_stream_type_t stream_type;

        // DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_request_iframe chn=0x%x, streamtype=%d\n", p_para->iChlMask, p_para->iStreamType );
        if (m_start_work_flag != 0)
        {
            switch(p_para->iStreamType)
            {
                case 0:
                    stream_type = _AST_SUB2_VIDEO_;
                    break;
                case 1:
                    stream_type = _AST_MAIN_VIDEO_;
                    break;
                case 2:
                    stream_type = _AST_SUB_VIDEO_;
                    break;
                default:
                    stream_type = _AST_UNKNOWN_;
                    break;
            }
            if((p_para->iChlMask != 0) && (stream_type != _AST_UNKNOWN_))
            {
                for(int index = 0; index < pgAvDeviceInfo->Adi_get_vi_chn_num(); index++)
                {
                    if(GCL_BIT_VAL_TEST(p_para->iChlMask, index))
                    {
                        m_pAv_device->Ad_request_iframe(stream_type, index);
                    }
                }
            }
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_request_iframe invalid message length %d=%d?\n", head->length, sizeof(msgRequestIFrame_t) );
    }
}


void CAvAppMaster::Aa_message_handle_start_encoder( const char* msg, int length )
{
    SMessageHead *head = (SMessageHead*) msg;
    unsigned long long int ullencode_local_chn_mask = 0;
    unsigned char streamtype_mask = 0;
    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        msgStartEncoder_t *p_para = (msgStartEncoder_t *)head->body;
        DEBUG_MESSAGE("sb ullencode_local_chn_mask=%u streamtype_mask=%d\n",p_para->ucChlMask,p_para->ucEncoderModeMask);

        if (m_start_work_flag != 0)
        {
            for(int chn_num = 0; chn_num < pgAvDeviceInfo->Adi_get_vi_chn_num(); chn_num++)
            {
                if(_VS_DIGITAL_ == pgAvDeviceInfo->Adi_get_video_source(chn_num))
                {
                    DEBUG_MESSAGE("_VS_DIGITAL_....\n");
                    continue;
                }

                if((p_para->ucEncoderModeMask & 0x2) && (m_sub_record_type == 0) && (m_pSub_stream_encoder != NULL) && (m_pSub_stream_encoder->stuVideoEncodeSetting[chn_num].ucChnEnable == 1))
                {
                    DEBUG_MESSAGE("already exit....\n");
                    continue;
                }
                if((m_pSub_stream_encoder != NULL) && m_pSub_stream_encoder->stuVideoEncodeSetting[chn_num].ucChnEnable == 0)
                {
                    m_net_start_substream_enable[chn_num] = 1;
                }
                ullencode_local_chn_mask = ullencode_local_chn_mask | ((p_para->ucChlMask) & (1ll << (chn_num)));
            }

            if(p_para->ucEncoderModeMask & 0x1)
            {
                streamtype_mask = streamtype_mask | 0x1;
            }
            if(p_para->ucEncoderModeMask & 0x2)
            {
                streamtype_mask = streamtype_mask | 0x2;
            }
            if(p_para->ucEncoderModeMask & 0x4)
            {
                streamtype_mask = streamtype_mask | 0x8;
            }

            DEBUG_MESSAGE("s ullencode_local_chn_mask=%llu streamtype_mask=%d\n",ullencode_local_chn_mask,streamtype_mask);
            Aa_start_selected_streamtype_encoders(ullencode_local_chn_mask,streamtype_mask);
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_start_encoder invalid message length %d=%d?\n", head->length, sizeof(msgStartEncoder_t) );
    }
}

void CAvAppMaster::Aa_message_handle_stop_encoder( const char* msg, int length )
{
    SMessageHead *head = (SMessageHead*) msg;
    unsigned long long int ullencode_local_chn_mask = 0;
    unsigned char streamtype_mask = 0;
    unsigned long long stop_encoder_chn_mask[_AST_UNKNOWN_] = {0};
    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        msgStopEncoder_t *p_para = (msgStopEncoder_t *)head->body;
        DEBUG_MESSAGE("b ullencode_local_chn_mask=%u streamtype_mask=%d\n",p_para->ucChlMask,p_para->ucEncoderModeMask);
        if (m_start_work_flag != 0)
        {
            for(int chn_num = 0; chn_num < pgAvDeviceInfo->Adi_get_vi_chn_num(); chn_num++)
            {
                if(_VS_DIGITAL_ == pgAvDeviceInfo->Adi_get_video_source(chn_num))
                {
                    continue;
                }

                if((p_para->ucEncoderModeMask & 0x2) && (m_sub_record_type == 0) && (m_pSub_stream_encoder != NULL) && (m_pSub_stream_encoder->stuVideoEncodeSetting[chn_num].ucChnEnable == 1))
                {
                    continue;
                }
                ullencode_local_chn_mask = ullencode_local_chn_mask | ((p_para->ucChlMask) & (1ll << (chn_num)));
            }

            if(p_para->ucEncoderModeMask & 0x1)
            {
                stop_encoder_chn_mask[0] = ullencode_local_chn_mask;
                stop_encoder_chn_mask[1] = ullencode_local_chn_mask;
                stop_encoder_chn_mask[3] = ullencode_local_chn_mask;
                streamtype_mask = streamtype_mask | 0x1;
            }
            if(p_para->ucEncoderModeMask & 0x2)
            {
                stop_encoder_chn_mask[1] = ullencode_local_chn_mask;
                streamtype_mask = streamtype_mask | 0x2;
            }
            if(p_para->ucEncoderModeMask & 0x4)
            {
                stop_encoder_chn_mask[3] = ullencode_local_chn_mask;
                streamtype_mask = streamtype_mask | 0x8;
            }
            DEBUG_MESSAGE("ullencode_local_chn_mask=%llu streamtype_mask=%d\n",ullencode_local_chn_mask,streamtype_mask);
#ifdef _AV_PLATFORM_HI3515_
			Aa_stop_encoders_for_parameters_change(stop_encoder_chn_mask,false);
#else
			stop_encoder_chn_mask[_AST_MAIN_VIDEO_] += 0;
			Aa_stop_selected_streamtype_encoders(ullencode_local_chn_mask,streamtype_mask);
#endif
            for(int chn_num = 0; chn_num < pgAvDeviceInfo->Adi_get_vi_chn_num(); chn_num++)
            {
                if((p_para->ucChlMask) & (1ll << (chn_num)))
                {
                    m_net_start_substream_enable[chn_num] = 0;
                }
            }
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_start_encoder invalid message length %d=%d?\n", head->length, sizeof(msgStartEncoder_t) );
    }
}
void CAvAppMaster::Aa_message_handle_get_net_stream_level(const char* msg, int length )
{
    SMessageHead *head = (SMessageHead*) msg;    
    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        unsigned int realchnmask = 0;
        unsigned char realchnvalue[SUPPORT_MAX_VIDEO_CHANNEL_NUM] = {0};
        msgStreamLevel_t msgStreamLevel;
        memset(&msgStreamLevel,0,sizeof(msgStreamLevel));
        Aa_get_net_stream_level(&realchnmask,realchnvalue);
        msgStreamLevel.uiChannelMask = realchnmask;
        memcpy(msgStreamLevel.ChnStreamLevel,realchnvalue,sizeof(realchnvalue));
        Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_AVSTREAM_SET_NET_STREAM_LEVEL, 0, (const char *) &msgStreamLevel, sizeof(msgStreamLevel_t), 0);
    }
}

void CAvAppMaster::Aa_message_handle_record_mode(const char* msg, int length )
{
    DEBUG_WARNING("Aa_message_handle_record_mode \n");
    if (PTD_X1 == pgAvDeviceInfo->Adi_product_type() || PTD_X1_AHD == pgAvDeviceInfo->Adi_product_type() )
    {
    	if((m_pMain_stream_encoder == NULL) || (m_pSub2_stream_encoder == NULL))
    	{
            DEBUG_ERROR("NUll encoder param!\n");
    		return ;
    	}        
    }
    else
    {
    	if((m_pMain_stream_encoder == NULL) || (m_pSub_stream_encoder == NULL))
    	{
            DEBUG_ERROR("NUll encoder param!\n");
    		return ;
    	}
    }
	unsigned char startorstop = 0;/*0:启动录像1:停止录像*/
    SMessageHead *head = (SMessageHead*) msg;   
	msgVideoEncodeSetting_t encode_para_main;
	msgVideoEncodeSetting_t encode_para_sub;
	memset(&encode_para_main,0,sizeof(encode_para_main));
	memcpy(&encode_para_main,m_pMain_stream_encoder,sizeof(encode_para_main));
	memset(&encode_para_sub,0,sizeof(encode_para_sub));
     if (PTD_X1 == pgAvDeviceInfo->Adi_product_type() || PTD_X1_AHD == pgAvDeviceInfo->Adi_product_type())
     {
        memcpy(&encode_para_sub,m_pSub2_stream_encoder,sizeof(encode_para_sub));
     }
     else
     {
    	memcpy(&encode_para_sub,m_pSub_stream_encoder,sizeof(encode_para_sub));
     }
	encode_para_main.mask = 0x0;
	encode_para_sub.mask = 0x0;
	msgStartRecordPara_t *StartPara = NULL;
	msgStopRecord_t *StopPara = NULL;
	eRecordMode_t streamtype = RECORD_MODE_MAIN;
    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
    	if(head->event == MESSAGE_STORAGE_START_RECORD)
    	{
    		startorstop = 0;
    	}
		else
		{
			startorstop = 1;
		}
		
		if(startorstop == 0)
    	{
			StartPara = (msgStartRecordPara_t *)head->body;
    	}
		else
		{
			StopPara = (msgStopRecord_t *)head->body;
		}
		
		for(int chn_num = 0; chn_num < pgAvDeviceInfo->Adi_get_vi_chn_num(); chn_num++)
		{
			if((_VS_ANALOG_ == pgAvDeviceInfo->Adi_get_video_source(chn_num)))
			{
				if(startorstop == 0)
				{					
					if(StartPara->eRecordMode == RECORD_MODE_MAIN && (StartPara->iChlMask & (0x1 << chn_num)))
					{
						if(StartPara->stuStartRecParam[chn_num].ucRecordType == 1)
						{
							 m_record_type[chn_num][0] = 1;
						}
						else
						{
							m_record_type[chn_num][0] = 0;
						}
						encode_para_main.mask = encode_para_main.mask | (0x1 << chn_num);
						streamtype = RECORD_MODE_MAIN;
					}

					if(StartPara->eRecordMode == RECORD_MODE_SUB && (StartPara->iChlMask & (0x1 << chn_num)))
					{
						if(StartPara->stuStartRecParam[chn_num].ucRecordType == 1)
						{
							 m_record_type[chn_num][1] = 1;
						}
						else
						{
							m_record_type[chn_num][1] = 0;
						}
						encode_para_sub.mask = encode_para_sub.mask | (0x1 << chn_num);
						streamtype = RECORD_MODE_SUB;
					}
				}
				else
				{
					if(StopPara->eRecordMode == RECORD_MODE_MAIN && (StopPara->iChlMask & (0x1 << chn_num)))
					{
						m_record_type[chn_num][0] = 0;
						encode_para_main.mask = encode_para_main.mask | (0x1 << chn_num);
						streamtype = RECORD_MODE_MAIN;
					}
					else if(StopPara->eRecordMode == RECORD_MODE_SUB && (StopPara->iChlMask & (0x1 << chn_num)))
					{
						m_record_type[chn_num][1] = 0;
						encode_para_sub.mask = encode_para_sub.mask | (0x1 << chn_num);
						streamtype = RECORD_MODE_SUB;
					}
				}
			}
			

		}
		if(streamtype == RECORD_MODE_MAIN)
		{
			Aa_modify_encoder_para(_AST_MAIN_VIDEO_, &encode_para_main);
		}

		if(streamtype == RECORD_MODE_SUB)
		{
            if (PTD_X1 == pgAvDeviceInfo->Adi_product_type()|| PTD_X1_AHD == pgAvDeviceInfo->Adi_product_type())
            {
                Aa_modify_encoder_para(_AST_SUB2_VIDEO_, &encode_para_sub);
            }
            else
            {
    			Aa_modify_encoder_para(_AST_SUB_VIDEO_, &encode_para_sub);
            }
		}
    }
    else
    {
        DEBUG_ERROR("invalid message length %d=%d (stop:%d)?\n", head->length, sizeof(msgStartRecordPara_t),sizeof(msgStopRecord_t) );
    }
}



void CAvAppMaster::Aa_message_handle_GET_ENCODER_SOURCE(const char* msg, int length )
{
    SMessageHead *head = (SMessageHead*) msg;    
    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {        
        DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_GET_ENCODER_SOURCE \n");
        msgNetLastEncodeSource_t encodeRes;

        pgAvDeviceInfo->Adi_get_sub_encoder_resource(encodeRes.encodesource, m_av_tvsystem);
        
        Common::N9M_MSGResponseMessage(m_message_client_handle, head, (const char *)&encodeRes, sizeof(msgNetLastEncodeSource_t));
        
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_GET_ENCODER_SOURCE invalid message length %d=%d?\n", head->length, sizeof(msgNetParamSubEncode_t) );
    }
    return;
}

#if defined(_AV_PRODUCT_CLASS_IPC_)
static char  gps_invalid_chinese[32] = {0xE5, 0xAE, 0x9A, 0xE4, 0xBD, 0x8D, 0xE5, 0x95, 0xB0, 0xE6, 0x8D, 0xAE, 0xE6, 0x97, 0xA0, 0xE6, 0x95, 0x88, 0};
static char gps_invalid_english[32] = "Location invalid";
static char  gps_none_chinese[32] = {0xE6, 0x97, 0xA0, 0xE5, 0xAE, 0x9A, 0xE4, 0xBD, 0x8D, 0xE6, 0xA8, 0xA1, 0xE5, 0x9D, 0x97, 0};
static char gps_none_english[32] = "No location module";
void CAvAppMaster::Aa_message_handle_update_gpsInfo(const char* msg, int length)
{
    SMessageHead *head = (SMessageHead*) msg;
    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        msgGpsInfo_t* pstuGpsInfo = (msgGpsInfo_t*)head->body;
        char gps[32] = {0};
        char speed[16];

        memset(gps, 0, sizeof(gps));
        memset(speed, 0, sizeof(speed));
        
        if(1 == pstuGpsInfo->ucGpsStatus)
        {
            if(0 == m_pSystem_setting->stuGeneralSetting.ucLanguageType)
            {//chinese
                strncpy(gps, gps_invalid_chinese, sizeof(gps));
            }
            else
            {
                strncpy(gps, gps_invalid_english, sizeof(gps));
            }
        }
        else if(2 == pstuGpsInfo->ucGpsStatus)
        {
            if(0 == m_pSystem_setting->stuGeneralSetting.ucLanguageType)
            {//chinese
                strncpy(gps, gps_none_chinese, sizeof(gps));
            }
            else
            {
                strncpy(gps, gps_none_english, sizeof(gps));
            }
        }
        else
        {
            snprintf(gps,sizeof(gps),"%d.%d.%04u%c %d.%d.%04u%c",pstuGpsInfo->ucLatitudeDegree,pstuGpsInfo->ucLatitudeCent,\
                pstuGpsInfo->ulLatitudeSec,pstuGpsInfo->ucDirectionLatitude ? 'S':'N',pstuGpsInfo->ucLongitudeDegree, \
                pstuGpsInfo->ucLongitudeCent,pstuGpsInfo->ulLongitudeSec,pstuGpsInfo->ucDirectionLongitude ? 'W':'E');            
        }

        if(0 == pstuGpsInfo->ucSpeedUnit)
        {
            if(0 != pstuGpsInfo->ucGpsStatus)
            {
                snprintf(speed, sizeof(speed), "0KM/H");
            }
            else
            {
                snprintf(speed, sizeof(speed), "%dKM/H", pstuGpsInfo->usSpeedKMH);
            }
        }
        else
        {
            if(0 != pstuGpsInfo->ucGpsStatus)
            {
                snprintf(speed, sizeof(speed), "0MPH");
            }
            else
            {
                snprintf(speed, sizeof(speed), "%dMPH", pstuGpsInfo->usSpeedMPH);
            }
        }
        
        
        DEBUG_MESSAGE("gps update, new value:%s \n", gps);
        if(0 != strcmp(m_osd_content[1], gps))
        {
            memset(m_osd_content[1], 0, sizeof(m_osd_content[1]));
            strncpy(m_osd_content[1], gps, sizeof(m_osd_content[1]));
            if(0 != m_start_work_flag)
            {
                for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); ++chn)
                {
                    av_video_encoder_para_t video_encoder;
                    Aa_get_encoder_para(_AST_MAIN_VIDEO_, chn, &video_encoder, -1, NULL);
                    if(m_main_encoder_state.test(chn)) 
                     {
                        if(video_encoder.have_gps_osd)
                        {
                            m_pAv_device->Ad_update_extra_osd_content(_AST_MAIN_VIDEO_, chn, _AON_GPS_INFO_, m_osd_content[1], strlen(m_osd_content[1]));                           
                        }
                     }
                    
                     if(m_sub_encoder_state.test(chn))
                     {
                        if(video_encoder.have_gps_osd)
                        {
                            m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, chn, _AON_GPS_INFO_, m_osd_content[1], strlen(m_osd_content[1]));    
                        } 
                     }
                }
            }
        }

        DEBUG_MESSAGE("speed update, new value:%s \n", speed);
        if(0 != strcmp(m_osd_content[2], speed))
        {
            memset(m_osd_content[2], 0, sizeof(m_osd_content[2]));
            strncpy(m_osd_content[2], speed, sizeof(m_osd_content[2]));
            if(0 != m_start_work_flag)
            {
                for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); ++chn)
                {
                    av_video_encoder_para_t video_encoder;
                    Aa_get_encoder_para(_AST_MAIN_VIDEO_, chn, &video_encoder, -1, NULL);
                    if(m_main_encoder_state.test(chn)) 
                     {
                        if(video_encoder.have_speed_osd)
                        {
                            m_pAv_device->Ad_update_extra_osd_content(_AST_MAIN_VIDEO_, chn, _AON_SPEED_INFO_, m_osd_content[2], strlen(m_osd_content[2]));                           
                        }
                     }
                    
                     if(m_sub_encoder_state.test(chn))
                     {
                        if(video_encoder.have_speed_osd)
                        {
                            m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, chn, _AON_SPEED_INFO_, m_osd_content[2], strlen(m_osd_content[2]));    
                        } 
                     }
                }
            }
        }

    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_update_pulse_speed invalid message length %d=%d?\n", head->length, sizeof(msgDeviceVehicleInfo_t) );
    }
    
    return;
}
#else
//! for customer 04.935

#define VALID_YEAR(year) ((year) <= 37)
#define VALID_MONTH(month) (((month) >= 1) && ((month) <= 12))
#define VALID_DATE(date) (((date) >= 1) && ((date) <= 31))
#define VALID_WEEK(week) ((week) <= 6)
#define VALID_HOUR(hour) ((hour) <= 23)
#define VALID_MINUTE(minute) ((minute) <= 59)
#define VALID_SECOND(second) ((second) <= 59)

#define _WEEK_INDEX_(week_info) (((week_info) >> 4) & 0x0f)
#define _WEEK_DAY_(week_info) ((week_info) & 0x0f)

#define _DST_STATE_ERROR_		0
#define _DST_STATE_INDST_ 		1
#define _DST_STATE_OUTDST_		2

#define DST_NO_OUT_IN	0
#define DST_OUT_TO_IN	1
#define DST_IN_TO_OUT	2


/*获取给定年月的最大的天数*/
inline int Get_max_day(unsigned char year, unsigned char month, unsigned char *pDay)
{	 
	if(VALID_YEAR(year) && VALID_MONTH(month))
	{
		unsigned char Max_day_table[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

		*pDay = Max_day_table[month - 1];
		if((month == 2) && (year % 4 == 0))/*闰年，注意:在2000到2099年，下面的闰年计算方式没有问题*/
		{
			*pDay = 29;
		}

		return 0;
	}

	return -1;
}

int Dt_realtime_to_calendar(const datetime_t *pReal_dt, time_t *pCal_dt)
{    
	tm tm_tmp;/*结构体tm表示分解时间(broken-down time)*/

	memset(&tm_tmp, 0, sizeof(tm));

	/*The number of years since 1900*/
	tm_tmp.tm_year = pReal_dt->year + 100;/*pReal_dt->year is the number of years since 2000*/
	/*The number of months since January, in the range 0 to 11*/
	tm_tmp.tm_mon = pReal_dt->month - 1;/**/
	/*The day of the month, in the range 1 to 31*/
	tm_tmp.tm_mday = pReal_dt->day;
	/*The number of hours past midnight, in the range 0 to 23*/
	tm_tmp.tm_hour = pReal_dt->hour;
	/*The number of minutes after the hour, in the range 0 to 59*/
	tm_tmp.tm_min = pReal_dt->minute;
	/*The  number of seconds after the minute, normally in the range 
	0 to 59, but can be up to 61 to allow for leap seconds.*/
	tm_tmp.tm_sec = pReal_dt->second;
	/*A flag that indicates whether daylight saving time is in effect 
	at the time described. The value is  positive  if daylight saving time is 
	in effect, zero if it is not, and negative if the information is not available*/
	tm_tmp.tm_isdst = 0;

	/*The mktime() function converts a broken-down time structure, expressed as local time, to 
	calendar time representation.The function ignores the specified contents of the structure 
	members tm_wday and tm_yday and recomputes them from the other information in the broken-down 
	time structure*/

	*pCal_dt = mktime(&tm_tmp);

	return 0;
}

/*得到某年某月某日是星期几*/
int Dt_get_week(unsigned char year, unsigned char month, unsigned char day, unsigned char *pWeek)
{
	unsigned char max_day = 0;

	if((Get_max_day(year, month, &max_day) != 0) || (!VALID_DATE(day)) || (day > max_day))
	{
		DEBUG_ERROR("Dt_get_week Get_max_day FAILED(year:%d)(month:%d)(day:%d)(max_day:%d)\n", year, month, day, max_day);
		return -1;
	}
    
    	unsigned long month_max[]={
		0,
		0,
		31,
		31 + 28,
		31 + 28 + 31,
		31 + 28 + 31 + 30,
		31 + 28 + 31 + 30 + 31,
		31 + 28 + 31 + 30 + 31 + 30,
		31 + 28 + 31 + 30 + 31 + 30 + 31,
		31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
		31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
		31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
		31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
		31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31};

	unsigned long tmp = 6 + year + (3 + year) / 4 + month_max[month] + day - 1;

	if(((year % 4 == 0)) && (month > 2)) 
	{
		tmp = tmp + 1;
	}

	*pWeek = tmp % 7;

	//  DM_DEBUG_INFO("Dt_get_week 20%02d-%02d-%02d: %d\n", year, month, day, *pWeek);

	return 0;
}


/*得到year年的month月的第几个星期几是那一天*/
int Dt_get_day_by_week(unsigned char year, unsigned char month, unsigned char week_info, unsigned char *pDay)
{ 
	if(!(VALID_YEAR(year) && VALID_MONTH(month) && VALID_WEEK(_WEEK_DAY_(week_info))))
	{
		DEBUG_ERROR("Dt_get_day_by_week (!(VALID_YEAR(year) && VALID_MONTH(month) && VALID_WEEK(_WEEK_DAY_(week_info))))\n");
		return -1;
	}

	unsigned char max_day = 0;

	if(Get_max_day(year, month, &max_day) != 0)
	{
		DEBUG_ERROR("Dt_get_day_by_week Get_max_day FAILED(%d)(%d)\n", year, month);
		return -1;
	}

	unsigned char day_week = 0;
	if(Dt_get_week(year, month, 1, &day_week) != 0)/*获取这个月的1号是星期几*/
	{
		DEBUG_ERROR("Dt_get_day_by_week Dt_get_week FAILED\n");
		return -1;
	}

	if(day_week < _WEEK_DAY_(week_info))
	{
		day_week = (_WEEK_DAY_(week_info) - day_week) + 1;
	}
	else if(day_week > _WEEK_DAY_(week_info))
	{
		day_week = 8 - (day_week - _WEEK_DAY_(week_info));
	}
	else
	{
		day_week = 1;
	}

	if(_WEEK_INDEX_(week_info) < 4)/*0:first, 1:second, 2:third, 3:fourth, 4:last*/
	{
		day_week += ((_WEEK_INDEX_(week_info) * 7));
	}
	else
	{
		for(unsigned char week_index = 4; week_index > 0; week_index --)
		{
			if((day_week + (week_index * 7)) <= max_day)
			{
				/*lint -e734*/
				day_week += (week_index * 7);
				/*lint +e734*/
				break;
			}
		}
	}

	if(day_week > max_day)
	{
		return -1;
	}

	*pDay = day_week;

	return 0;
}

int Dm_dst_compare(const paramTimeSetting_t *pTime_settings)
{
	if(pTime_settings->stuDst.stuDefaultMode.ucStartMonth == pTime_settings->stuDst.stuDefaultMode.ucEndMonth)
	{
		if(_WEEK_INDEX_(pTime_settings->stuDst.stuDefaultMode.ucStartWeek) == _WEEK_INDEX_(pTime_settings->stuDst.stuDefaultMode.ucEndWeek))
		{
			if(_WEEK_DAY_(pTime_settings->stuDst.stuDefaultMode.ucStartWeek) == _WEEK_DAY_(pTime_settings->stuDst.stuDefaultMode.ucEndWeek))
			{
				unsigned long start_time = pTime_settings->stuDst.stuDefaultMode.ucStartHour * 3600 + pTime_settings->stuDst.stuDefaultMode.ucStartMinute * 60 + pTime_settings->stuDst.stuDefaultMode.ucStartSecond;
				unsigned long end_time = pTime_settings->stuDst.stuDefaultMode.ucEndHour * 3600 + pTime_settings->stuDst.stuDefaultMode.ucEndMinute * 60 + pTime_settings->stuDst.stuDefaultMode.ucEndSecond;

				if(start_time == end_time)
				{
					return 0;
				}
				else if(start_time > end_time)
				{
					return -1;
				}
				else
				{
					return 1;
				}
			}
			else
			{
				/*这个的夏令时设置着实有些变态，夏令时开始和结束于同一个月的第x个星期几，仅仅是星期几的不同，我们很难判断
				出谁大谁小，比如12月的第1个星期6不见得比12月的第一个星期1大，因为12月1号可能就是星期六，很是难于判断，
				如此，直接返回0算了，反正这种设置现实中是绝对不存在的*/
				DEBUG_ERROR("Dm_dst_compare bian tai de she zhi\n");
				return 0;
			}
		}
		else if(_WEEK_INDEX_(pTime_settings->stuDst.stuDefaultMode.ucStartWeek) > _WEEK_INDEX_(pTime_settings->stuDst.stuDefaultMode.ucEndWeek))
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}
	else if(pTime_settings->stuDst.stuDefaultMode.ucStartMonth > pTime_settings->stuDst.stuDefaultMode.ucEndMonth)
	{
		return -1;
	}

	return 1;
}


/*获取指定年的夏令时的开始时间点，结束时间点，以及上一个结束时间点*/
int Dm_get_dst_info(const paramTimeSetting_t *pTime_settings, const datetime_t *pLinux_local_utc, time_t *pStart_local_utc/* = NULL*/, time_t *pEnd_local_utc_last/* = NULL*/, time_t *pEnd_local_utc_next/* = NULL*/)
{
	if(pTime_settings == NULL)
	{
		DEBUG_ERROR("CDeviceManage::Dm_get_dst_info (pTime_settings == NULL)\n");
		return -1;
	}

	if(pTime_settings->stuDst.ucSwitch == 0)
	{
		DEBUG_ERROR("CDeviceManage::Dm_get_dst_info (pTime_settings->stuDst.ucSwitch == 0)\n");
		return -1;
	}

	if(pTime_settings->stuDst.ucDstMode == 2)/*date mode*/
	{
		*pStart_local_utc = pTime_settings->stuDst.StuSelfDefinedMode.uiStartTime;
		if(pEnd_local_utc_next != NULL)
		{
			/*Note:pTime_settings->stuDst.StuSelfDefinedMode.uiEndTime is the dst time，so need - the offset*/
			*pEnd_local_utc_next = pTime_settings->stuDst.StuSelfDefinedMode.uiEndTime - pTime_settings->stuDst.ucDstTimeOffset * 3600;
		}        
		return 0;
	}
	else if(pTime_settings->stuDst.ucDstMode != 1)/*week mode*/
	{
		DEBUG_ERROR("CDeviceManage::Dm_get_dst_info invalid ucDstMode:%d\n", pTime_settings->stuDst.ucDstMode);
		return -1;
	}

	/*为了更高效，我们将上次计算的结果保存下来*/
	static paramTimeSetting_t *pTime_setting_backup = NULL;
	static time_t start_local_utc_backup = 0;
	static time_t end_local_utc_last_backup = 0;
	static time_t end_local_utc_next_backup = 0;
	static unsigned char year_backup = 0xff;

	if(pTime_setting_backup == NULL)
	{
		pTime_setting_backup = new paramTimeSetting_t;
		memset(pTime_setting_backup, 0, sizeof(paramTimeSetting_t));
	}

	if(!((memcmp(pTime_setting_backup, pTime_settings, sizeof(paramTimeSetting_t)) != 0) || (year_backup != pLinux_local_utc->year) || 
		(start_local_utc_backup == 0) || (end_local_utc_last_backup == 0) || (end_local_utc_next_backup == 0)))
	{
		if(pStart_local_utc != NULL)
		{
			*pStart_local_utc = start_local_utc_backup;
		}
		if(pEnd_local_utc_last != NULL)
		{
			*pEnd_local_utc_last = end_local_utc_last_backup;
		}
		if(pEnd_local_utc_next != NULL)
		{
			*pEnd_local_utc_next = end_local_utc_next_backup;
		}
		return 0;
	}

	start_local_utc_backup = 0;
	end_local_utc_last_backup = 0;
	end_local_utc_next_backup = 0;
	year_backup = 0xff;
	memset(pTime_setting_backup, 0, sizeof(paramTimeSetting_t));

	datetime_t start_dt;
	start_dt.year = pLinux_local_utc->year;
	start_dt.month = pTime_settings->stuDst.stuDefaultMode.ucStartMonth + 1;
	if(Dt_get_day_by_week(start_dt.year, start_dt.month, pTime_settings->stuDst.stuDefaultMode.ucStartWeek, &start_dt.day) != 0)
	{
		DEBUG_ERROR("CDeviceManage::Dm_get_dst_info Dt_get_day_by_week FAILED(%d)(%d)(%02x)\n", pLinux_local_utc->year, pTime_settings->stuDst.stuDefaultMode.ucStartMonth, pTime_settings->stuDst.stuDefaultMode.ucStartWeek);
		return -1;
	}
	start_dt.hour = pTime_settings->stuDst.stuDefaultMode.ucStartHour;
	start_dt.minute = pTime_settings->stuDst.stuDefaultMode.ucStartMinute;
	start_dt.second = pTime_settings->stuDst.stuDefaultMode.ucStartSecond;

	if(Dt_realtime_to_calendar(&start_dt, &start_local_utc_backup) != 0)
	{
		DEBUG_ERROR("CDeviceManage::Dm_get_dst_info Dt_realtime_to_calendar FAILED\n");
		return -1;
	}
	if(pStart_local_utc != NULL)
	{
		*pStart_local_utc = start_local_utc_backup;
	}

	DEBUG_ERROR("CDeviceManage::Dm_get_dst_info start:20%02d-%02d-%02d\n", start_dt.year, start_dt.month, start_dt.day);

	datetime_t end_dt;
	switch(Dm_dst_compare(pTime_settings))
	{
		case 0:
		default:/*start == end*/
			DEBUG_ERROR("CDeviceManage::Dm_get_dst_info Dm_dst_compare == 0\n");
			return -1;
			//break;

		case 1:/*start < end*/
			if(pLinux_local_utc->year > 0)
			{
				end_dt.year = pLinux_local_utc->year - 1;  
			}
			else
			{
				end_dt.year = pLinux_local_utc->year;  
			}
			break;
		case -1:/*start > end*/
			end_dt.year = pLinux_local_utc->year;
			break;
	}
	end_dt.month = pTime_settings->stuDst.stuDefaultMode.ucEndMonth + 1;
	DEBUG_WARNING("##---%d, %d, %d\n", end_dt.year, end_dt.month, pTime_settings->stuDst.stuDefaultMode.ucEndWeek);
	if(Dt_get_day_by_week(end_dt.year, end_dt.month, pTime_settings->stuDst.stuDefaultMode.ucEndWeek, &end_dt.day) != 0)
	{
		DEBUG_ERROR("CDeviceManage::Dm_get_dst_info Dt_get_day_by_week FAILED\n");
		return -1;
	}
	end_dt.hour = pTime_settings->stuDst.stuDefaultMode.ucEndHour;
	end_dt.minute = pTime_settings->stuDst.stuDefaultMode.ucEndMinute;
	end_dt.second = pTime_settings->stuDst.stuDefaultMode.ucEndSecond;
	if(Dt_realtime_to_calendar(&end_dt, &end_local_utc_last_backup) != 0)
	{
		DEBUG_ERROR("CDeviceManage::Dm_get_dst_info Dt_realtime_to_calendar FAILED\n");
		return -1;
	}
	end_local_utc_last_backup -= (pTime_settings->stuDst.ucDstTimeOffset * 3600);

	DEBUG_ERROR("CDeviceManage::Dm_get_dst_info last end:20%02d-%02d-%02d\n", end_dt.year, end_dt.month, end_dt.day);    

	if(pEnd_local_utc_last != NULL)
	{
		*pEnd_local_utc_last = end_local_utc_last_backup;
	}

	/*下一个结束时间点*/
	switch(Dm_dst_compare(pTime_settings))
	{
		case 0:
		default:/*start == end*/
			DEBUG_ERROR("CDeviceManage::Dm_get_dst_info Dm_dst_compare == 0\n");
			return -1;
			//break;

		case 1:/*start < end*/
			end_dt.year = pLinux_local_utc->year;                
			break;

		case -1:/*start > end*/
			end_dt.year = pLinux_local_utc->year + 1;
			break;
	}
	end_dt.month = pTime_settings->stuDst.stuDefaultMode.ucEndMonth + 1;
	if(Dt_get_day_by_week(end_dt.year, end_dt.month, pTime_settings->stuDst.stuDefaultMode.ucEndWeek, &end_dt.day) != 0)
	{
		DEBUG_ERROR("CDeviceManage::Dm_get_dst_info Dt_get_day_by_week FAILED\n");
		return -1;
	}
	end_dt.hour = pTime_settings->stuDst.stuDefaultMode.ucEndHour;
	end_dt.minute = pTime_settings->stuDst.stuDefaultMode.ucEndMinute;
	end_dt.second = pTime_settings->stuDst.stuDefaultMode.ucEndSecond;
	if(Dt_realtime_to_calendar(&end_dt, &end_local_utc_next_backup) != 0)
	{
		DEBUG_ERROR("CDeviceManage::Dm_get_dst_info Dt_realtime_to_calendar FAILED\n");
		return -1;
	}
	DEBUG_ERROR("CDeviceManage::Dm_get_dst_info next end:20%02d-%02d-%02d\n", end_dt.year, end_dt.month, end_dt.day);
	end_local_utc_next_backup -= (pTime_settings->stuDst.ucDstTimeOffset * 3600);

	if(pEnd_local_utc_next != NULL)
	{
		*pEnd_local_utc_next = end_local_utc_next_backup;
	}

	year_backup = pLinux_local_utc->year;
	memcpy(pTime_setting_backup, pTime_settings, sizeof(paramTimeSetting_t));

	return 0;
}


int Dm_is_in_dst(const paramTimeSetting_t *pTime_settings, const datetime_t *pLinux_local_utc)
{
	time_t dst_start_local_utc;
	time_t dst_end_local_utc_last;
	time_t dst_end_local_utc_next;
	time_t linux_local_utc_cal;

	if(Dm_get_dst_info(pTime_settings, pLinux_local_utc, &dst_start_local_utc, &dst_end_local_utc_last, &dst_end_local_utc_next) != 0)
	{
		DEBUG_ERROR("CDeviceManage::Dm_is_in_dst Dm_get_dst_info FAILED\n");
		return _DST_STATE_ERROR_;
	}

	if(Dt_realtime_to_calendar(pLinux_local_utc, &linux_local_utc_cal) != 0)
	{
		DEBUG_ERROR("CDeviceManage::Dm_is_in_dst Dt_realtime_to_calendar FAILED\n");
		return _DST_STATE_ERROR_;
	}

	if(pTime_settings->stuDst.ucDstMode == 2)
	{
		if((linux_local_utc_cal >= dst_start_local_utc) && (linux_local_utc_cal < dst_end_local_utc_next))
		{
			return _DST_STATE_INDST_;
		}
		else
		{
			return _DST_STATE_OUTDST_;
		}
	}
	else
	{
		//if((linux_local_utc_cal >= dst_end_local_utc_last) && (linux_local_utc_cal < dst_start_local_utc))
		//{
		// 	return _DST_STATE_OUTDST_;
		//}

		if((linux_local_utc_cal >= dst_start_local_utc) && (linux_local_utc_cal < dst_end_local_utc_next))
		{
			return _DST_STATE_INDST_;
		}
		else
		{
			return _DST_STATE_OUTDST_;
		}
	}

	return _DST_STATE_OUTDST_;
}



int CheckDaylightTime(datetime_t *b_time, datetime_t *a_time, msgTimeSetting_t m_pTime)
{
	int result_1 = Dm_is_in_dst(&m_pTime.stuTimeSetting, b_time);
	int result_2 = Dm_is_in_dst(&m_pTime.stuTimeSetting, a_time);
	printf("result_1 = %d\n", result_1);
	printf("result_2 = %d\n", result_2);
	if(result_1 == _DST_STATE_OUTDST_ && result_2 == _DST_STATE_INDST_)
	{
		return DST_OUT_TO_IN;
	}
	else if(result_1 == _DST_STATE_INDST_ && result_2 == _DST_STATE_OUTDST_)
	{
		return DST_IN_TO_OUT;
	}

	return DST_NO_OUT_IN;
}

void CAvAppMaster::Gps_time_to_local_time(msgTime_t gps_time_m, msgTime_t *local_time)
{
	DEBUG_MESSAGE("START GPS sync time...\n");
	//读取UTC时间
	time_t cur_utc_time;
	cur_utc_time = N9M_TMGetShareUTCSecond();

	//读取GPS时间
	datetime_t gps_time;
	memset(&gps_time, 0, sizeof(datetime_t));
	gps_time.year   = gps_time_m.year;
	gps_time.month  = gps_time_m.month;
	gps_time.day    = gps_time_m.day;
	gps_time.hour   = gps_time_m.hour;
	gps_time.minute = gps_time_m.minute;
	gps_time.second = gps_time_m.second;
    
	time_t gps_utc_time = 0;
	N9M_TMDateTimeToUtcTime(&gps_time, &gps_utc_time);

	//获取当前系统时间
	datetime_t systime;
	N9M_TMGetShareTime(&systime);

	//转换为UTC时间
	time_t utctime;
	N9M_TMDateTimeToUtcTime(&systime, &utctime);
	utctime = utctime + (gps_utc_time - cur_utc_time);

	datetime_t settime;
	N9M_TMUtcTimeToDateTime(&utctime, &settime);
	
	int result = CheckDaylightTime(&systime, &settime,*m_pTime_setting);
	if(result == DST_OUT_TO_IN)
	{
		settime.hour = settime.hour + m_pTime_setting->stuTimeSetting.stuDst.ucDstTimeOffset;
	}
	else if(result == DST_IN_TO_OUT)
	{
		settime.hour = settime.hour - m_pTime_setting->stuTimeSetting.stuDst.ucDstTimeOffset;
	}

	local_time->year   = settime.year;
	local_time->month  = settime.month;
	local_time->day    = settime.day;
	local_time->hour   = settime.hour;
	local_time->minute = settime.minute;
	local_time->second = settime.second;

    return;
}
void CAvAppMaster::Aa_message_handle_update_gps(const char* msg, int length )
{
    SMessageHead *head = (SMessageHead*) msg;

    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        msgGpsInfo_t* pstuGpsInfo = (msgGpsInfo_t*)head->body;
        //! for customer 04.935 only
        //! translate the gps time to local time
        char cutomername[32] = {0};
        pgAvDeviceInfo->Adi_get_customer_name(cutomername, sizeof(cutomername));
        if (strcmp(cutomername, "CUSTOMER_04.935") == 0)
        {
            DEBUG_CRITICAL("04.935 gps convert\n");
            msgTime_t origin_time = pstuGpsInfo->GpsTime;
            Gps_time_to_local_time(origin_time, &(pstuGpsInfo->GpsTime));
        }
        //!
        pgAvDeviceInfo->Adi_Set_Gps_Info(pstuGpsInfo);
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_update_pulse_speed invalid message length %d=%d?\n", head->length, sizeof(msgDeviceVehicleInfo_t) );
    }
    
    return;
}
#endif
void CAvAppMaster::Aa_message_handle_TAX2_GPS_SPEED(const char* msg, int length )
{
    SMessageHead *head = (SMessageHead*) msg;
    DEBUG_WARNING("[GPS] Aa_message_handle_TAX2_GPS_SPEED \n");
    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        Tax2DataList* pstuTax2 = (Tax2DataList*)head->body;
        DEBUG_WARNING("[GPS] usRealSpeed =%d \n",pstuTax2->usRealSpeed);
        pgAvDeviceInfo->Adi_Set_Speed_Info_from_Tax2(pstuTax2);
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_TAX2_GPS_SPEED invalid message length %d=%d?\n", head->length, sizeof(msgDeviceVehicleInfo_t) );
    }
    
    return;
}

#if defined(_AV_PRODUCT_CLASS_IPC_)
void CAvAppMaster::Aa_message_handle_update_speedInfo(const char* msg, int length)
{
    SMessageHead *head = (SMessageHead*) msg;

    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        msgDeviceVehicleInfo_t* pstuVehicleInfo = (msgDeviceVehicleInfo_t*)head->body;
        char speed[16] = {0};
        if(1 == pstuVehicleInfo->speed.usSpeedUnit)
        {
            snprintf(speed, sizeof(speed), "%dMPH", pstuVehicleInfo->speed.uiMiliSpeed);
        }
        else
        {
            snprintf(speed, sizeof(speed), "%dKM/H", pstuVehicleInfo->speed.uiKiloSpeed);
        }

        DEBUG_MESSAGE("speed update, new value:%s \n", speed);
        if(0 != strcmp(m_osd_content[2], speed))
        {
            memset(m_osd_content[2], 0, sizeof(m_osd_content[2]));
            strncpy(m_osd_content[2], speed, sizeof(m_osd_content[2]));
            if(0 != m_start_work_flag)
            {
                for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); ++chn)
                {
                    av_video_encoder_para_t video_encoder;
                    Aa_get_encoder_para(_AST_MAIN_VIDEO_, chn, &video_encoder, -1, NULL);
                    if(m_main_encoder_state.test(chn)) 
                     {
                        if(video_encoder.have_speed_osd)
                        {
                            m_pAv_device->Ad_update_extra_osd_content(_AST_MAIN_VIDEO_, chn, _AON_SPEED_INFO_, m_osd_content[2], strlen(m_osd_content[2]));                           
                        }
                     }
                    
                     if(m_sub_encoder_state.test(chn))
                     {
                        if(video_encoder.have_speed_osd)
                        {
                            m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, chn, _AON_SPEED_INFO_, m_osd_content[2], strlen(m_osd_content[2]));    
                        } 
                     }
                }
            }
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_update_pulse_speed invalid message length %d=%d \n", head->length, sizeof(msgDeviceVehicleInfo_t) );
    }
    
    return;
}
#else

#if !defined(_AV_PRODUCT_CLASS_DVR_REI_)

void CAvAppMaster::Aa_message_handle_update_speed_info(const char* msg, int length )
{
    SMessageHead *head = (SMessageHead*) msg;
    DEBUG_WARNING("Aa_message_handle_update_speed_info \n");
    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        msgSpeedInfo_t * pstuSpeedInfo = (msgSpeedInfo_t*)head->body;
        pgAvDeviceInfo->Adi_Set_Speed_Info(pstuSpeedInfo);
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_update_speed_info invalid message length %d=%d?\n", head->length, sizeof(msgDeviceVehicleInfo_t) );
    }
    
    return;
}

#endif

void CAvAppMaster::Aa_message_handle_update_pulse_speed(const char* msg, int length )
{
    SMessageHead *head = (SMessageHead*) msg;

    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        msgDeviceVehicleInfo_t* pstuVehicleInfo = (msgDeviceVehicleInfo_t*)head->body;
        pgAvDeviceInfo->Adi_Set_Speed_Info(&(pstuVehicleInfo->speed));
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_update_pulse_speed invalid message length %d=%d?\n", head->length, sizeof(msgDeviceVehicleInfo_t) );
    }
    
    return;
}
#endif

int CAvAppMaster::Aa_modify_encoder_para(av_video_stream_type_t stream_type, msgVideoEncodeSetting_t *p_encode_para)
{
    int index = 0;
    msgVideoEncodeSetting_t *p_local_encode_para = NULL;
    unsigned long long start_encoder_chn_mask[_AST_UNKNOWN_] = {0};
    unsigned long long stop_encoder_chn_mask[_AST_UNKNOWN_] = {0};
    unsigned long long modify_mask = 0; 
    unsigned long long reset_vi_mask = 0;
    
    unsigned long long int vi_stop_mask = 0;
    unsigned long long int vi_start_mask = 0;
    
    av_video_encoder_para_t encoder_para;
    bool recordtypechange =false;
    if(p_encode_para == NULL)
    {
        DEBUG_WARNING("encde para is null!\n");
        return -1;
    }
    switch(stream_type)
    {
        case _AST_MAIN_VIDEO_:
            p_local_encode_para = m_pMain_stream_encoder;
            break;
        case _AST_SNAP_VIDEO_:
            //p_local_encode_para = m_pMain_stream_encoder;
            DEBUG_WARNING("modify snap para invalid!\n");
            return -1;
            break;
        case _AST_SUB_VIDEO_:
            if(m_pSub_stream_encoder != NULL)
            {
                p_local_encode_para = m_pSub_stream_encoder;
            }
            break;
        case _AST_SUB2_VIDEO_:
            p_local_encode_para = m_pSub2_stream_encoder;
            break;
        default:
            DEBUG_WARNING("stream type is invalid!\n");
            return -1;
            break;
    }

    if(p_local_encode_para == NULL)
    {
        DEBUG_WARNING("local encde para is null!\n");
        return -1;
    }
    
    for(index = 0; index < pgAvDeviceInfo->Adi_get_vi_chn_num(); index++)
    {
        DEBUG_DEBUG("p_encode_para->mask=%llx\n",p_encode_para->mask);
        if(GCL_BIT_VAL_TEST(p_encode_para->mask, index))
        {
            if(_VS_DIGITAL_ == pgAvDeviceInfo->Adi_get_video_source(index))
            {
                continue;
            }
            else
            {
            	if(stream_type == _AST_MAIN_VIDEO_)
            	{
                    if(m_record_type[index][0] != m_record_type_bak[index][0])
                    {
                        recordtypechange = true;
                        m_record_type_bak[index][0] = m_record_type[index][0];  
                        DEBUG_DEBUG("index:%d true\n",index);
                    }
                    else
                    {
                        recordtypechange = false;
                        DEBUG_DEBUG("index:%d false\n",index);
                    }
                }
                else if(stream_type == _AST_SUB_VIDEO_)
                {
                    if (PTD_X1 != pgAvDeviceInfo->Adi_product_type() && PTD_X1_AHD != pgAvDeviceInfo->Adi_product_type())
                    {
                        if(m_record_type[index][1] != m_record_type_bak[index][1])
                        {
                            recordtypechange = true;
                            m_record_type_bak[index][1] = m_record_type[index][1];  
                        }
                        else
                        {
                            recordtypechange = false;
                        }
                    }
                }
                else if (stream_type == _AST_SUB2_VIDEO_)
                {
                    if (PTD_X1 == pgAvDeviceInfo->Adi_product_type() || PTD_X1_AHD == pgAvDeviceInfo->Adi_product_type())
                    {
                        if(m_record_type[index][1] != m_record_type_bak[index][1])
                        {
                            recordtypechange = true;
                            m_record_type_bak[index][1] = m_record_type[index][1];  
                        }
                        else
                        {
                            recordtypechange = false;
                        }
                        DEBUG_WARNING("X1 sub2 video index:%d , change:%d\n",index, recordtypechange);
                    }
                }

#if defined(_AV_PRODUCT_CLASS_IPC_) && defined(_AV_PLATFORM_HI3516A_) && defined(_AV_HAVE_ISP_)
                if(stream_type == _AST_MAIN_VIDEO_)
                {
                    IspOutputMode_e isp_output_mode;
                    RM_ISP_API_GetOutputMode(isp_output_mode);
                    /*if isp output mode changed, all encoders and vi need to be resetted*/
                    if(isp_output_mode != pgAvDeviceInfo->Adi_get_isp_output_mode())
                    {
                        if(m_pMain_stream_encoder!= NULL && m_pMain_stream_encoder->stuVideoEncodeSetting[index].ucChnEnable)
                        {
                            GCL_BIT_VAL_SET(stop_encoder_chn_mask[_AST_MAIN_VIDEO_], index);
                            GCL_BIT_VAL_SET(start_encoder_chn_mask[_AST_MAIN_VIDEO_], index);                             
                        }

                        if (m_pSub_stream_encoder != NULL && m_pSub_stream_encoder->stuVideoEncodeSetting[index].ucChnEnable)
                        {
                            GCL_BIT_VAL_SET(stop_encoder_chn_mask[_AST_SUB_VIDEO_], index);
                            GCL_BIT_VAL_SET(start_encoder_chn_mask[_AST_SUB_VIDEO_], index);                            
                        }

                        if (m_pSub2_stream_encoder != NULL && m_pSub2_stream_encoder->stuVideoEncodeSetting[index].ucChnEnable)
                        {
                            GCL_BIT_VAL_SET(stop_encoder_chn_mask[_AST_SUB2_VIDEO_], index);
                            GCL_BIT_VAL_SET(start_encoder_chn_mask[_AST_SUB2_VIDEO_], index);                           
                        }

                        GCL_BIT_VAL_SET(reset_vi_mask, index); 

                        pgAvDeviceInfo->Adi_set_isp_output_mode(isp_output_mode);
                        m_u8SourceFrameRate = Aa_convert_framerate_from_IOM(isp_output_mode);
                        continue;
                    }
                }
#endif
                if ((memcmp(&p_encode_para->stuVideoEncodeSetting[index], &p_local_encode_para->stuVideoEncodeSetting[index], sizeof(paramVideoEncode_t)) != 0) || (recordtypechange == true))
                {
                    if ((recordtypechange == true))
                    {
                        //GCL_BIT_VAL_SET(modify_mask, index);
                        modify_mask = 0x0f;
                    }
                    if ((stream_type == _AST_MAIN_VIDEO_) && (p_encode_para->stuVideoEncodeSetting[index].ucAudioEnable != p_local_encode_para->stuVideoEncodeSetting[index].ucAudioEnable))
                    {
                       if (p_encode_para->stuVideoEncodeSetting[index].ucAudioEnable == 0) // stop audio encoder
                        {
                            //printf("--------stop audio encoder channl:%d\n", index);
                            if(m_pAv_device->Ad_stop_encoder(stream_type, index, _AAST_NORMAL_, index, 1) != 0)
                            {
                                DEBUG_ERROR( "CAvAppMaster::Aa_set_encoder_param m_pAv_device->Ad_stop_encoder FAIELD(chn:%d)\n", index);
                                return -1;
                            }
                            //GCL_BIT_VAL_SET(stop_encoder_chn_mask[stream_type], index);
                        }
                        else //start audio encoder
                        {
                            //printf("--------start audio encoder channl:%d\n", index);
                           av_video_encoder_para_t video_para;
                            av_audio_encoder_para_t audio_para;
                            int phy_audio_chn_num = -1;
                            unsigned long long int stop_mask = 0;

                            pgAvDeviceInfo->Adi_get_audio_chn_id(index , phy_audio_chn_num);      
                            Aa_get_encoder_para(stream_type, index, &video_para, phy_audio_chn_num, &audio_para);
                            
                            video_para.have_audio = p_encode_para->stuVideoEncodeSetting[index].ucAudioEnable;
                            GCL_BIT_VAL_SET(stop_mask, index);
#if defined(_AV_PLATFORM_HI3515_)
							m_pAv_device->Ad_stop_selected_encoder(stop_mask ,_AST_SUB2_VIDEO_,_AAST_NORMAL_);
#endif
                            m_pAv_device->Ad_stop_selected_encoder(stop_mask ,stream_type,_AAST_NORMAL_);
                            if(m_pAv_device->Ad_start_encoder(stream_type, index, &video_para, _AAST_NORMAL_,
                                phy_audio_chn_num, &audio_para, 1) != 0)
                            {
                                DEBUG_ERROR( "CAvAppMaster::Aa_set_encoder_param m_pAv_device->Ad_start_encoder FAIELD(chn:%d)\n", phy_audio_chn_num);
                                return -1;
                            }
#if defined(_AV_PLATFORM_HI3515_)
							if(m_pAv_device->Ad_start_encoder(_AST_SUB2_VIDEO_, index, &video_para, _AAST_NORMAL_,
								phy_audio_chn_num, &audio_para, 1) != 0)
							{
								DEBUG_ERROR( "CAvAppMaster::Aa_set_encoder_param m_pAv_device->Ad_start_encoder FAIELD(chn:%d)\n", phy_audio_chn_num);
								return -1;
							}
#endif
                            //GCL_BIT_VAL_SET(start_encoder_chn_mask[stream_type], index);
                        }

                    }
                    //
                    if (p_encode_para->stuVideoEncodeSetting[index].ucChnEnable != p_local_encode_para->stuVideoEncodeSetting[index].ucChnEnable)
                    {     
                        if (p_encode_para->stuVideoEncodeSetting[index].ucChnEnable == 0) //! channel is disabled
                        {
                            GCL_BIT_VAL_SET(stop_encoder_chn_mask[stream_type], index);
                            
                            if ((stream_type == _AST_MAIN_VIDEO_))//&& (p_encode_para->stuVideoEncodeSetting[index].ucResolution != p_local_encode_para->stuVideoEncodeSetting[index].ucResolution))
                            {
                                //if (m_pSub_stream_encoder->stuVideoEncodeSetting[index].ucChnEnable)
                                {
                                    //printf(" stop sub encoder index: %d\n", index);
                                    GCL_BIT_VAL_SET(stop_encoder_chn_mask[_AST_SUB_VIDEO_], index);
                                   // GCL_BIT_VAL_SET(start_encoder_chn_mask[_AST_SUB_VIDEO_], index);
                                }
                                
                               // if (m_pSub2_stream_encoder->stuVideoEncodeSetting[index].ucChnEnable)
                                {
                                    GCL_BIT_VAL_SET(stop_encoder_chn_mask[_AST_SUB2_VIDEO_], index);
                                   // GCL_BIT_VAL_SET(start_encoder_chn_mask[_AST_SUB2_VIDEO_], index);
                                }
                               GCL_BIT_VAL_SET(vi_stop_mask, index); // 2.20 
                            }
                        }
                        else   //! channel is set to be enabled
                        {
                            if ( (stream_type == _AST_SUB_VIDEO_) || (stream_type == _AST_SUB2_VIDEO_)  )
                            {
                                if (m_pMain_stream_encoder->stuVideoEncodeSetting[index].ucChnEnable != 0)
                                {
                                    GCL_BIT_VAL_SET(start_encoder_chn_mask[stream_type], index); 
                                }
                                else
                                {
                                    DEBUG_CRITICAL(" Channel is closed!\n");
                                }
                            }
                            else if (stream_type == _AST_MAIN_VIDEO_)
                            {
                                GCL_BIT_VAL_SET(start_encoder_chn_mask[stream_type], index); 
                                GCL_BIT_VAL_SET(vi_start_mask, index); // 2.20
                            }
                            
                            if ((stream_type == _AST_MAIN_VIDEO_) )//&& (p_encode_para->stuVideoEncodeSetting[index].ucResolution != p_local_encode_para->stuVideoEncodeSetting[index].ucResolution))
                            {
                                
                                if (m_pSub_stream_encoder != NULL && m_pSub_stream_encoder->stuVideoEncodeSetting[index].ucChnEnable)
                                {
                                    GCL_BIT_VAL_SET(stop_encoder_chn_mask[_AST_SUB_VIDEO_], index);
                                    GCL_BIT_VAL_SET(start_encoder_chn_mask[_AST_SUB_VIDEO_], index);
                                }
                                
                                if (m_pSub2_stream_encoder->stuVideoEncodeSetting[index].ucChnEnable)
                                {
                                    GCL_BIT_VAL_SET(stop_encoder_chn_mask[_AST_SUB2_VIDEO_], index);
                                    GCL_BIT_VAL_SET(start_encoder_chn_mask[_AST_SUB2_VIDEO_], index);
                                }
                                GCL_BIT_VAL_SET(vi_stop_mask, index); // 2.20
                                GCL_BIT_VAL_SET(vi_start_mask, index); // 2.20
                                GCL_BIT_VAL_SET(reset_vi_mask, index);    
                            }
                        }
                    }
                    else if (p_encode_para->stuVideoEncodeSetting[index].ucChnEnable != 0) //!<  enable para changed reset
                    {
#if defined(_AV_PLATFORM_HI3515_)
                        if ((stream_type == _AST_MAIN_VIDEO_)&&(m_pSub2_stream_encoder != NULL))
                        {
                            int main_wid = 0;
                            int main_hei = 0;
                            int sub_wid = 0;
                            int sub_hei = 0;

                            av_resolution_t eResolution;
                            pgAvDeviceInfo->Adi_get_video_resolution(&p_encode_para->stuVideoEncodeSetting[index].ucResolution, &eResolution, true);
                            pgAvDeviceInfo->Adi_get_video_size(eResolution, m_av_tvsystem,&main_wid, &main_hei);

                            unsigned char res = m_pSub2_stream_encoder->stuVideoEncodeSetting[index].ucResolution;

                            pgAvDeviceInfo->Adi_get_video_resolution(&res, &eResolution, true);
                            pgAvDeviceInfo->Adi_get_video_size(eResolution,m_av_tvsystem,&sub_wid, &sub_hei);  

                            //! check the sub stream param for they can never be exceed the main ones
                            if((main_wid < sub_wid)||(main_hei < sub_hei))
                            {
                                m_pSub2_stream_encoder->stuVideoEncodeSetting[index].ucResolution = p_encode_para->stuVideoEncodeSetting[index].ucResolution;
                                GCL_BIT_VAL_SET(stop_encoder_chn_mask[_AST_SUB2_VIDEO_], index);
                                GCL_BIT_VAL_SET(start_encoder_chn_mask[_AST_SUB2_VIDEO_], index); 

                            }
                            if(p_encode_para->stuVideoEncodeSetting[index].ucFrameRate < m_pSub2_stream_encoder->stuVideoEncodeSetting[index].ucFrameRate)
                            {
                                m_pSub2_stream_encoder->stuVideoEncodeSetting[index].ucFrameRate = p_encode_para->stuVideoEncodeSetting[index].ucFrameRate;
                                GCL_BIT_VAL_SET(stop_encoder_chn_mask[_AST_SUB2_VIDEO_], index);
                                GCL_BIT_VAL_SET(start_encoder_chn_mask[_AST_SUB2_VIDEO_], index); 
                            }
                        }
                        else if(m_pMain_stream_encoder != NULL) //! sub stream
                        {
                            int main_wid = 0;
                            int main_hei = 0;
                            int sub_wid = 0;
                            int sub_hei = 0;

                            av_resolution_t eResolution;
                            pgAvDeviceInfo->Adi_get_video_resolution(&m_pMain_stream_encoder->stuVideoEncodeSetting[index].ucResolution, &eResolution, true);
                            pgAvDeviceInfo->Adi_get_video_size(eResolution, m_av_tvsystem,&main_wid, &main_hei);

                            pgAvDeviceInfo->Adi_get_video_resolution(&p_encode_para->stuVideoEncodeSetting[index].ucResolution, &eResolution, true);
                            pgAvDeviceInfo->Adi_get_video_size(eResolution,m_av_tvsystem,&sub_wid, &sub_hei); 
                             
                            if((main_wid < sub_wid)||(main_hei < sub_hei))
                            {
                                p_encode_para->stuVideoEncodeSetting[index].ucResolution = m_pMain_stream_encoder->stuVideoEncodeSetting[index].ucResolution;
                            }
                            if(p_encode_para->stuVideoEncodeSetting[index].ucFrameRate > m_pMain_stream_encoder->stuVideoEncodeSetting[index].ucFrameRate)
                            {
                                p_encode_para->stuVideoEncodeSetting[index].ucFrameRate = m_pMain_stream_encoder->stuVideoEncodeSetting[index].ucFrameRate;
                            }
                        }
#endif
                        if ((p_encode_para->stuVideoEncodeSetting[index].ucResolution != p_local_encode_para->stuVideoEncodeSetting[index].ucResolution)
                            || (p_encode_para->stuVideoEncodeSetting[index].ucBrMode != p_local_encode_para->stuVideoEncodeSetting[index].ucBrMode))
                        {
                            GCL_BIT_VAL_SET(start_encoder_chn_mask[stream_type], index); 
                            GCL_BIT_VAL_SET(stop_encoder_chn_mask[stream_type], index);
                        }
                        else
                        {
                            GCL_BIT_VAL_SET(modify_mask, index);
                        }
                        
                        if ((p_encode_para->stuVideoEncodeSetting[index].ucResolution != p_local_encode_para->stuVideoEncodeSetting[index].ucResolution)
                            && (stream_type == _AST_MAIN_VIDEO_))
                        {
                            if (m_pSub_stream_encoder != NULL && m_pSub_stream_encoder->stuVideoEncodeSetting[index].ucChnEnable)
                            {
                                GCL_BIT_VAL_SET(stop_encoder_chn_mask[_AST_SUB_VIDEO_], index);
                                GCL_BIT_VAL_SET(start_encoder_chn_mask[_AST_SUB_VIDEO_], index);
                            }
                            
                            if (m_pSub2_stream_encoder != NULL &&m_pSub2_stream_encoder->stuVideoEncodeSetting[index].ucChnEnable)
                            {
                                GCL_BIT_VAL_SET(stop_encoder_chn_mask[_AST_SUB2_VIDEO_], index);
                                GCL_BIT_VAL_SET(start_encoder_chn_mask[_AST_SUB2_VIDEO_], index);       
                            }
                            GCL_BIT_VAL_SET(vi_stop_mask, index); // 2.20
                            GCL_BIT_VAL_SET(vi_start_mask, index); // 2.20
                            GCL_BIT_VAL_SET(reset_vi_mask, index);    
                        }

                    }
                    else //disable  still 
                    {
                        /*
                        if ((stream_type == _AST_MAIN_VIDEO_) && (p_encode_para->stuVideoEncodeSetting[index].ucResolution != p_local_encode_para->stuVideoEncodeSetting[index].ucResolution))
                        {
                            if (m_pSub_stream_encoder != NULL && m_pSub_stream_encoder->stuVideoEncodeSetting[index].ucChnEnable)
                            {
                                GCL_BIT_VAL_SET(stop_encoder_chn_mask[_AST_SUB_VIDEO_], index);
                                GCL_BIT_VAL_SET(start_encoder_chn_mask[_AST_SUB_VIDEO_], index);
                            }
                            
                            if (m_pSub2_stream_encoder->stuVideoEncodeSetting[index].ucChnEnable)
                            {
                                GCL_BIT_VAL_SET(stop_encoder_chn_mask[_AST_SUB2_VIDEO_], index);
                                GCL_BIT_VAL_SET(start_encoder_chn_mask[_AST_SUB2_VIDEO_], index);
                            }
                            
                            GCL_BIT_VAL_SET(reset_vi_mask, index);    
                        }*/
                    }
                }

            }
        }
    }

    /*avoid copy digital encode parameters to local parameters*/
    p_local_encode_para->mask = p_encode_para->mask;
    for (int chn_index = 0; chn_index < pgAvDeviceInfo->Adi_get_vi_chn_num(); chn_index++)
    {
        if (GCL_BIT_VAL_TEST(p_encode_para->mask, chn_index))
        {
            if (_VS_ANALOG_ == pgAvDeviceInfo->Adi_get_video_source(chn_index))
            {
                memcpy(p_local_encode_para->stuVideoEncodeSetting + chn_index, p_encode_para->stuVideoEncodeSetting + chn_index, sizeof(paramVideoEncode_t));
            }
            else
            {
                GCL_BIT_VAL_CLEAR(p_local_encode_para->mask, chn_index);
            }
        }
    }

    if(m_start_work_flag != 0)/*av not start,so only set encode para*/
    {
#ifdef _AV_PLATFORM_HI3515_
        if (stream_type == _AST_MAIN_VIDEO_)
        {
            stop_encoder_chn_mask[_AST_SUB2_VIDEO_] = stop_encoder_chn_mask[_AST_MAIN_VIDEO_];
        }
#endif        
        Aa_stop_encoders_for_parameters_change(stop_encoder_chn_mask, false);
        if (reset_vi_mask != 0)
        {
#if defined(_AV_PLATFORM_HI3518EV200_)
            Aa_reset_selected_vi(reset_vi_mask, false);
#else
            Aa_stop_selected_vda(_VDA_MD_, reset_vi_mask);
//#ifndef _AV_PLATFORM_HI3515_
            Aa_reset_selected_vi(reset_vi_mask, false); //! cause vi restart
//#endif            
            Aa_start_selected_vda(_VDA_MD_, reset_vi_mask);
#endif
        } 
/*        
#ifdef _AV_PLATFORM_HI3515_
        m_pAv_device_master->Ad_stop_selected_vi(vi_stop_mask);        
        Aa_start_selected_vi(vi_start_mask);
        if (stream_type == _AST_MAIN_VIDEO_)
        {
            start_encoder_chn_mask[_AST_SUB2_VIDEO_] = start_encoder_chn_mask[_AST_MAIN_VIDEO_];
        }
#endif
*/
        Aa_start_encoders_for_parameter_change(start_encoder_chn_mask, false);

#if defined(_AV_PRODUCT_CLASS_IPC_) && defined(_AV_HAVE_ISP_)
        if( m_pstuCameraColorParam )
        {
            m_pAv_device_master->Ad_SetCameraColor( &(m_pstuCameraColorParam->stuVideoImageSetting[0]) );
        }
        if( m_pstuCameraAttrParam )
        {
            m_pAv_device_master->Ad_SetCameraAttribute( 0xffffffff, &(m_pstuCameraAttrParam->stuViParam[0].image) );
        }
#endif
        DEBUG_WARNING("Final modify mask:0x%llx\n", modify_mask);
        if (modify_mask != 0)
        {
            for(index = 0; index < pgAvDeviceInfo->Adi_get_vi_chn_num(); index++)
            {
                if (GCL_BIT_VAL_TEST(modify_mask, index))
                {
                    memset(&encoder_para, 0, sizeof(av_video_encoder_para_t));
                    Aa_get_encoder_para(stream_type, index, &encoder_para, -1, NULL);
                    m_pAv_device->Ad_modify_encoder(stream_type, index, &encoder_para);
                }
            }
        }
    }

    return 0;
}

void CAvAppMaster::Aa_get_snap_result_message()
{
    int result = -1;
    do
    {
		msgSnapResultBoardcast listSnapResult;
		memset(&listSnapResult,0,sizeof(msgSnapResultBoardcast));
		result = m_pAv_device_master->Ad_get_snap_result(&listSnapResult);
        //DEBUG_MESSAGE("listSnapResult.uiSnapTaskNumber=%d\n",listSnapResult.uiSnapTaskNumber);
        //DEBUG_MESSAGE("listSnapResult.uiPictureSerialnumber=%d\n",listSnapResult.uiPictureSerialnumber);
        //DEBUG_MESSAGE("listSnapResult.usSnapResult=%d\n",listSnapResult.usSnapResult);
        //DEBUG_MESSAGE("listSnapResult.usStorageState=%d\n",listSnapResult.usStorageState);
        //DEBUG_MESSAGE("listSnapResult.uiSnapTotalNumber=%d\n",listSnapResult.uiSnapTotalNumber);
        //DEBUG_MESSAGE("listSnapResult.uiSanpCurNumber=%d\n",listSnapResult.uiSanpCurNumber);
        //DEBUG_MESSAGE("listSnapResult.uiUser=%d\n",listSnapResult.uiUser);
        if(result >= 0)
        {
			Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_SNAP_PICTURE_AV_BROADCAST_RESULT, 0, (char *)&listSnapResult, sizeof(msgSnapResultBoardcast), 0);
        }
    }while(result > 0);
}

void CAvAppMaster::Aa_set_osd_info(const AvSnapPara_t &pSignalSnapParam )
{
    //DEBUG_MESSAGE( "######CAvAppMaster::Aa_set_osd_info()\n");
    av_video_encoder_para_t pVideo_para  ;
    memset(&pVideo_para, 0,sizeof(av_video_encoder_para_t));
    int iPhyChannel = 0;
    for(int chn=0; chn<pgAvDeviceInfo->Adi_max_channel_number(); chn++)
    {
        if(0 != ((pSignalSnapParam.uiChannel) & (1ll<<chn)))
        {
            iPhyChannel = chn;
            Aa_get_encoder_para(_AST_MAIN_VIDEO_, iPhyChannel, &pVideo_para, iPhyChannel, NULL);
            //DEBUG_MESSAGE(" date = %d busnumber = %d speed = %d gps = %d chn = %d selfnum = %d\n",pVideo_para.have_date_time_osd, pVideo_para.have_bus_number_osd, pVideo_para.have_speed_osd, pVideo_para.have_gps_osd, pVideo_para.have_channel_name_osd, pVideo_para.have_bus_selfnumber_osd);
            //DEBUG_MESSAGE("######pVideo_para.bus_number %s bus_number_osd_x %d bus_number_osd_y %d\n",pVideo_para.bus_number, pVideo_para.bus_number_osd_x, pVideo_para.bus_number_osd_y);
            //DEBUG_MESSAGE("######pVideo_para.date_mode %d date_time_osd_x %d date_time_osd_y %d\n",pVideo_para.date_mode, pVideo_para.date_time_osd_x, pVideo_para.date_time_osd_y);
            //DEBUG_MESSAGE("######pVideo_para.speed_unit %c speed_osd_x %d speed_osd_y %d\n",pVideo_para.speed_unit, pVideo_para.speed_osd_x, pVideo_para.speed_osd_y);
            //DEBUG_MESSAGE("######gps_osd_x %d gps_osd_y %d\n", pVideo_para.gps_osd_x, pVideo_para.gps_osd_y);
            //DEBUG_MESSAGE("###### channel_name %s channel_name_osd_x %d channel_name_osd_y %d\n", pVideo_para.channel_name, pVideo_para.channel_name_osd_x, pVideo_para.channel_name_osd_y);
            //DEBUG_MESSAGE("###### bus_selfnumber %s bus_selfnumber_osd_x %d bus_selfnumber_osd_y %d\n", pVideo_para.bus_selfnumber, pVideo_para.bus_selfnumber_osd_x, pVideo_para.bus_selfnumber_osd_y);
            //pVideo_para.have_bus_number_osd = 0;
            //pVideo_para.have_bus_selfnumber_osd = 0;
            //pVideo_para.have_channel_name_osd = 0;
            //pVideo_para.have_gps_osd = 0;
            //pVideo_para.have_speed_osd = 0;
            m_pAv_device_master->Ad_set_osd_info(&pVideo_para, iPhyChannel, pSignalSnapParam.usOverlayBitmap);
        }

    }

}

int CAvAppMaster::Aa_get_net_stream_level(unsigned int *chnmask,unsigned char chnvalue[SUPPORT_MAX_VIDEO_CHANNEL_NUM])
{
    if(m_pAv_device && chnmask)
    {
        m_pAv_device->Ad_get_net_stream_level(chnmask,chnvalue);
        return 0;
    }
    return -1;
}

void CAvAppMaster::Aa_check_net_stream_level()
{
    static unsigned int chnmask = 0;
    static unsigned char chnvalue[SUPPORT_MAX_VIDEO_CHANNEL_NUM] = {0};
    unsigned int realchnmask = 0;
    unsigned char realchnvalue[SUPPORT_MAX_VIDEO_CHANNEL_NUM] = {0};
    msgStreamLevel_t msgStreamLevel;
    memset(&msgStreamLevel,0,sizeof(msgStreamLevel));
    Aa_get_net_stream_level(&realchnmask,realchnvalue);

    if((chnmask != realchnmask) || (memcmp(chnvalue,realchnvalue,sizeof(chnvalue)) != 0))
    {
        msgStreamLevel.uiChannelMask = realchnmask;
        memcpy(msgStreamLevel.ChnStreamLevel,realchnvalue,sizeof(chnvalue));
        Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_AVSTREAM_SET_NET_STREAM_LEVEL, 0, (const char *) &msgStreamLevel, sizeof(msgStreamLevel_t), 0);
        for(int chn = 0;chn < SUPPORT_MAX_VIDEO_CHANNEL_NUM;chn++)
        {
            if(realchnmask & (0x1 << chn))
            DEBUG_WARNING("chn:%d val:%d\n",chn,msgStreamLevel.ChnStreamLevel[chn]);
        }
        chnmask = realchnmask;
        memcpy(chnvalue,realchnvalue,sizeof(chnvalue));
    }
}

#endif


#if defined(_AV_HAVE_VIDEO_INPUT_)
void CAvAppMaster::Aa_message_handle_GET_VIDEO_STATUS(const char *msg, int length)
{
#if 0
    SMessageHead *pMessage_head = (SMessageHead*) msg;
    int chn;

    DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_GET_VIDEO_STATUS\n");
    if ((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        msgVideoStatus_t VideoStatus;
        memset(&VideoStatus, 0, sizeof(msgVideoStatus_t));
        if (m_start_work_env[MESSAGE_DEVICE_GET_VLOSS_STATE] == false)
        {
            return;
        }
        memset(&VideoStatus, 0, sizeof(msgVideoStatus_t));
        for (chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
        {
            if (_VS_DIGITAL_ == pgAvDeviceInfo->Adi_get_video_source(chn))
            {
                VideoStatus.u32Mask[chn] = 0xff;
            }
            else
            {
                VideoStatus.u32Mask[chn] = 0;
                if(pMessage_head->event == MESSAGE_EVENT_GET_VL_STATUS)
                {
                    if(GCL_BIT_VAL_TEST(m_alarm_status.videoLost, chn))
                    {
                        VideoStatus.u32Status[chn] = 1;
                    }
                }
                else if(pMessage_head->event == MESSAGE_EVENT_GET_MD_STATUS)
                {
                    if(!GCL_BIT_VAL_TEST(m_alarm_status.videoLost, chn) && GCL_BIT_VAL_TEST(m_alarm_status.motioned, chn))
                    {
                        VideoStatus.u32Status[chn] = 1;
                    }
                }
            }
        }
        Common::N9M_MSGResponseMessage(m_message_client_handle, pMessage_head, (const char*)&VideoStatus, sizeof(msgVideoStatus_t));
    }
    return;
#endif
}

#if !defined(_AV_PLATFORM_HI3518EV200_)
void CAvAppMaster::Aa_message_handle_MOTIONDETECTPARA(const char *msg, int length)
{
#if 1
    SMessageHead *head = (SMessageHead*) msg;

    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        paramMDAlarmSetting_t *pMotionDectPara = (paramMDAlarmSetting_t *)(head->body);

        if (m_pMotion_detect_para != NULL)
        {
            if(memcmp(m_pMotion_detect_para, pMotionDectPara, sizeof(paramMDAlarmSetting_t)) != 0)
            {
                memcpy(m_pMotion_detect_para, pMotionDectPara, sizeof(paramMDAlarmSetting_t));               
                if (m_start_work_flag != 0)
                {
                    Aa_stop_vda(_VDA_MD_);
                    Aa_start_vda(_VDA_MD_);
                }
            }
        }
        else
        {
            _AV_FATAL_((m_pMotion_detect_para = new paramMDAlarmSetting_t) == NULL, "CAvAppMaster::Aa_message_handle_MOTIONDETECTPARA OUT OF MEMORY\n");
            memcpy(m_pMotion_detect_para, pMotionDectPara, sizeof(paramMDAlarmSetting_t));
            m_start_work_env[MESSAGE_CONFIG_MANAGE_MD_ALARM_PARAM_GET] = true;
        }
    }    
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_MOTIONDETECTPARA invalid message length\n");
    }
#endif
}

void CAvAppMaster::Aa_message_handle_GET_MD_BLOCKSTATE(const char *msg, int length)
{
#if 1
    SMessageHead *pHead = reinterpret_cast<SMessageHead *>(const_cast<char *>(msg));

    if (pHead->length + sizeof(SMessageHead) == static_cast<unsigned int>(length))
    {
        msgMDBlockState_t *pMD_block_state = reinterpret_cast<msgMDBlockState_t *>(pHead->body);

        if (pMD_block_state != NULL)
        {
             static bool bVda_start = false;
             unsigned int vda_result = 0;
             msgMDBlockState_t msg_MD_block_state;
             memset(&msg_MD_block_state, 0, sizeof(msgMDBlockState_t));
             
             vda_chn_param_t vda_chn_param;
             memset(&vda_chn_param, 0, sizeof(vda_chn_param_t));
             vda_chn_param.enable = pMD_block_state->ucEnable;
             vda_chn_param.sensitivity = pMD_block_state->ucSensitivity;
             memset(vda_chn_param.region, 0xff, sizeof(vda_chn_param.region));
             if (vda_chn_param.enable != 0)
             {
                if (!bVda_start)
                {  
                    Aa_start_vda(_VDA_MD_, pMD_block_state->ucChn, &vda_chn_param);
                    bVda_start = true;
                }
                
                Aa_get_vda_result(_VDA_MD_, &vda_result, pMD_block_state->ucChn, msg_MD_block_state.ucBlockState);
                
                Common::N9M_MSGResponseMessage(m_message_client_handle, pHead, reinterpret_cast<const char *>(&msg_MD_block_state), sizeof(msgMDBlockState_t));
             }
             else
             {
                if (bVda_start)
                {
                    Aa_stop_vda(_VDA_MD_, pMD_block_state->ucChn);
                    bVda_start = false;
                }
             } 
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_GET_MD_BLOCKSTATE invalid message length!!! \n");
    }
#endif
}


void CAvAppMaster::Aa_message_handle_VIDEOSHIELDPARA(const char *msg, int length)
{
#if 1
    SMessageHead *head = (SMessageHead*) msg;
    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        paramVideoShieldAlarmSetting_t *pVideoShieldPara= (paramVideoShieldAlarmSetting_t *) head->body;
        _AV_KEY_INFO_(_AVD_APP_, "Aa_message_handle_VIDEOSHIELDPARA called! vs enable:%d \n", pVideoShieldPara->ucEnable);
        if (NULL != m_pVideo_shield_para)
        {
            if(0 != memcmp(m_pVideo_shield_para, pVideoShieldPara, sizeof(paramVideoShieldAlarmSetting_t)))
            {
                memcpy(m_pVideo_shield_para, pVideoShieldPara, sizeof(paramVideoShieldAlarmSetting_t));
            }
        }
        else
        {
            m_pVideo_shield_para = new paramVideoShieldAlarmSetting_t;
            m_start_work_env[MESSAGE_CONFIG_MANAGE_VS_ALARM_PARAM_GET] = true;
            memcpy(m_pVideo_shield_para, pVideoShieldPara, sizeof(paramVideoShieldAlarmSetting_t));
        } 

        if (0 != m_start_work_flag)
        {
            if (0 != m_pVideo_shield_para->ucEnable)
            {
                Aa_stop_vda(_VDA_OD_);
                Aa_start_vda(_VDA_OD_);               
            }
            else
            {
                Aa_stop_vda(_VDA_OD_);
            }
        }
    }    
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_VIDEOSHIELDPARA invalid message length\n");
    }
#endif
}
#endif
#if 0
int CAvAppMaster::Aa_cover_area(msgVideoCover_t *pNew_cover, msgVideoCover_t *pOld_cover)
{
    int phy_chn_num = -1;

    for(int chn_index = 0; chn_index < pgAvDeviceInfo->Adi_max_channel_number(); chn_index ++)
    {
        phy_chn_num = chn_index;

        for(int cover_index = 0; cover_index < 4; cover_index ++)
        {
            //DEBUG_MESSAGE( "CAvAppMaster::Aa_cover_area(chn:%d)(enable:%d)(index:%d)(x:%d, y:%d, w:%d, h:%d)\n", phy_chn_num, pNew_cover->cover[chn_index].ucEnable, cover_index, pNew_cover->cover[chn_index].stuArea[cover_index].x, pNew_cover->cover[chn_index].stuArea[cover_index].y, pNew_cover->cover[chn_index].stuArea[cover_index].w, pNew_cover->cover[chn_index].stuArea[cover_index].h);
            m_pAv_device->Ad_vi_uncover(phy_chn_num, cover_index);
            if(pNew_cover != NULL)
            {
                if((pNew_cover->cover[chn_index].ucEnable != 0) && (pNew_cover->cover[chn_index].stuArea[cover_index].w != 0) && 
                                        (pNew_cover->cover[chn_index].stuArea[cover_index].h != 0))
                {
                    int x = pNew_cover->cover[chn_index].stuArea[cover_index].x;
                    int y = pNew_cover->cover[chn_index].stuArea[cover_index].y;
                    int w = pNew_cover->cover[chn_index].stuArea[cover_index].w;
                    int h = pNew_cover->cover[chn_index].stuArea[cover_index].h;
                    if( x + w > 1024 )
                    {
                        w = 1024 - x;
                        if( w < 0 )
                        {
                            w = 0;
                        }
                    }
                    if( y + h > 768 )
                    {
                        h = 768 - y;
                        if( h < 0 )
                        {
                            h = 0;
                        }
                    }
                    
                    m_pAv_device->Ad_vi_cover(phy_chn_num, x, y, w, h, cover_index);
                }
            }
        }
    }

    return 0;
}
#endif

void CAvAppMaster::Aa_message_handle_COVERPARA(const char *msg, int length)
{
#if 0
    SMessageHead *head = (SMessageHead*) msg;

    DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_COVERPARA\n");

    if((head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgVideoCover_t *pCover_area_para= (msgVideoCover_t *) head->body;
        if (m_pCover_area_para != NULL)
        {
            if(memcmp(m_pCover_area_para, pCover_area_para, sizeof(msgVideoCover_t)) == 0)
            {
                DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_COVERPARA SAME CONFIG\n");
                return;
            }
            if(m_start_work_flag != 0)
            {
                Aa_cover_area(pCover_area_para, m_pCover_area_para);
                memcpy(m_pCover_area_para, pCover_area_para, sizeof(msgVideoCover_t));
            }
        }
        else
        {
            _AV_FATAL_((m_pCover_area_para = new msgVideoCover_t) == NULL, "CAvAppMaster::Aa_message_handle_COVERPARA OUT OF MEMORY\n");
            m_start_work_env[MESSAGE_AVSTREAM_COVERAREA_PARAM_GET] = true;
            memcpy(m_pCover_area_para, pCover_area_para, sizeof(msgVideoCover_t));
        }
    }   
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_COVERPARA invalid message length\n");
    }
#endif
}

#if !defined(_AV_PRODUCT_CLASS_IPC_)
void CAvAppMaster::Aa_message_handle_VIRESOLUTION(const char *msg, int length)
{
    if(m_start_work_flag == 0)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_VIRESOLUTION (m_start_work_flag == 0)\n");
        return;
    }

    SMessageHead *head = (SMessageHead*) msg;

    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        msgVideoResolution_t video_resolution;

        memset(&video_resolution, 0, sizeof(msgVideoResolution_t));
#if (defined(_AV_HAVE_VIDEO_DECODER_) && (defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)))
		memcpy(&video_resolution, &m_vdec_image_res[VDP_PREVIEW_VDEC], sizeof(msgVideoResolution_t));
#else
        video_resolution.u16Status = 0;
        video_resolution.u32ChnMask = 0;
        for(int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn ++)
        {
            video_resolution.u32ChnMask |= (0x01ull << chn);
            switch(m_pMain_stream_encoder->stuVideoEncodeSetting[chn].ucResolution)
            {
                case 0:/*CIF*/
                case 1:/*HD1*/    
                case 2:/*D1*/
                case 3:/*QCIF*/
                    video_resolution.stuVideoRes[chn].u32ResValue = 2;
                    video_resolution.stuVideoRes[chn].u32Width = pgAvDeviceInfo->Adi_get_D1_width();
                    video_resolution.stuVideoRes[chn].u32Height = (m_av_tvsystem == _AT_PAL_)?576:480;
                    break;
                case 6:/*720p*/
                    video_resolution.stuVideoRes[chn].u32ResValue = 6;
                    video_resolution.stuVideoRes[chn].u32Width = 1280;
                    video_resolution.stuVideoRes[chn].u32Height = 720;
                    break;
                case 7:/*1080p*/
                    video_resolution.stuVideoRes[chn].u32ResValue = 7;
                    video_resolution.stuVideoRes[chn].u32Width = 1920;
                    video_resolution.stuVideoRes[chn].u32Height = 1080;
                    break;
                case 10:/*WQCIF*/
                case 11:/*WCIF*/
                case 12:/*WHD1*/
                case 13:/*WD1*/
                    video_resolution.stuVideoRes[chn].u32ResValue = 13;
                    video_resolution.stuVideoRes[chn].u32Width = pgAvDeviceInfo->Adi_get_WD1_width();
                    video_resolution.stuVideoRes[chn].u32Height = (m_av_tvsystem == _AT_PAL_)?576:480;
                    break;
                case 14:
                    video_resolution.stuVideoRes[chn].u32ResValue = 14;
                    video_resolution.stuVideoRes[chn].u32Width = 1280;
                    video_resolution.stuVideoRes[chn].u32Height = 960;
                    break;
                case 16:
                    video_resolution.stuVideoRes[chn].u32ResValue = 16;
                    video_resolution.stuVideoRes[chn].u32Width = 800;
                    video_resolution.stuVideoRes[chn].u32Height = 600;
                    break;
                case 17:
                    video_resolution.stuVideoRes[chn].u32ResValue = 17;
                    video_resolution.stuVideoRes[chn].u32Width = 1024;
                    video_resolution.stuVideoRes[chn].u32Height = 768;
                    break;

                default:
                    //_AV_FATAL_(1, "CAvAppMaster::Aa_boardcast_vi_resolution invalid resolution(%d)\n", m_pMain_stream_encoder->stuVideoEncodeSetting[chn].ucResolution);
                    DEBUG_CRITICAL("CAvAppMaster::Aa_boardcast_vi_resolution invalid resolution(%d)\n",m_pMain_stream_encoder->stuVideoEncodeSetting[chn].ucResolution);
                    break;
            }
        }
#endif
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_VIRESOLUTION OK!!!!!!!!\n");
        Common::N9M_MSGResponseMessage(m_message_client_handle, head, (const char*)&video_resolution, sizeof(msgVideoResolution_t));
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_VIRESOLUTION invalid message length\n");
    }
}
#endif

void CAvAppMaster::Aa_message_handle_SNAPSHOT(const char *msg, int length)
{
    if(m_start_work_flag == 0)
    {
        return;
    }

    SMessageHead *head = (SMessageHead*) msg;
    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
#ifdef _AV_PRODUCT_CLASS_IPC_      
        if( m_u8IPCSnapWorkMode != 0 )
        {
            DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_SNAPSHOT Eveng snap current Mode NVR mode,ignor msg\n");
            return;
        }
#endif
        //m_pAv_device->Ad_cmd_snap_picutres( (MsgSnapPicture_t*) head->body );
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_SNAPSHOT invalid message length\n");
    }

    return;
}

/* 单次抓拍任务处理函数 */
void CAvAppMaster::Aa_message_handle_SIGNALSNAP(const char* msg, int length )
{
    DEBUG_WARNING( "######### CAvAppMaster::Aa_message_handle_SIGNALSNAP\n");
    if(m_start_work_flag == 0)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_SIGNALSNAP system not ok\n");
        return;
    }
    //static int taskNum = 0;
    SMessageHead *head = (SMessageHead*) msg;
    AvSnapPara_t stuSnapPara;
    memset(&stuSnapPara, 0, sizeof(AvSnapPara_t));
    
    if( (head->length + sizeof(SMessageHead)) == (unsigned int) length )
    {
        msgSignalSnapPara_t* pSignalSnapParam = (msgSignalSnapPara_t*)head->body;
        DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_SIGNALSNAP, mask:0x%x res:%d\n", pSignalSnapParam->usOverlayBitmap, pSignalSnapParam->ucResolution);
        //DEBUG_CRITICAL("uiSnapType =%d uiChannel =%d ucHistorySnapItemCount =%d\n",pSignalSnapParam->uiSnapType,pSignalSnapParam->uiChannel,pSignalSnapParam->ucHistorySnapItemCount);
        
#if defined(_AV_PRODUCT_CLASS_IPC_)
        memcpy(&stuSnapPara, pSignalSnapParam, sizeof(AvSnapPara_t));
        m_pAv_device->Ad_cmd_signal_snap_picutres(stuSnapPara);
        if(0 != m_start_work_flag)
        {
            for(int chn=0; chn<pgAvDeviceInfo->Adi_max_channel_number(); chn++)
            {
                if(0 != ((pSignalSnapParam->uiChannel) & (1ll<<chn)))
                {
                    av_video_encoder_para_t pVideo_para  ;
                    memset(&pVideo_para, 0,sizeof(av_video_encoder_para_t));
                    
                    Aa_get_encoder_para(_AST_MAIN_VIDEO_, chn, &pVideo_para, -1, NULL);
                    m_pAv_device_master->Ad_modify_snap_param(_AST_MAIN_VIDEO_, chn, &pVideo_para);
                }
            }
        }            
#else
        //get osd coordinate info from main stream
        if(pSignalSnapParam->uiSnapType != 5)
        {
            memcpy(&stuSnapPara, pSignalSnapParam, sizeof(msgSignalSnapPara_t));
            m_pAv_device->Ad_cmd_signal_snap_picutres(stuSnapPara);
            Aa_set_osd_info(stuSnapPara);
        }
        else
        {
            for(int i = 0; i< pSignalSnapParam->ucHistorySnapItemCount; i++)
            {
                memset(&stuSnapPara, 0, sizeof(AvSnapPara_t));
                HistorySnapItem_t *pSnapBody = (HistorySnapItem_t*)(pSignalSnapParam->body + i*sizeof(HistorySnapItem_t));
                DEBUG_WARNING("uiChannel =%d ucResolution =%d ucQuality =%d usNumber =%d uiUser =%d \n",\
                    pSnapBody->uiChannel,pSnapBody->ucResolution,pSnapBody->ucQuality,pSnapBody->usNumber,pSnapBody->uiUser);
                DEBUG_WARNING("year =%d month =%d day =%d hour =%d mintue =%d second =%d\n",
                    pSnapBody->stuSnapTime.year,pSnapBody->stuSnapTime.month,pSnapBody->stuSnapTime.day,pSnapBody->stuSnapTime.hour,pSnapBody->stuSnapTime.minute,pSnapBody->stuSnapTime.second);
                stuSnapPara.uiSnapType          = pSignalSnapParam->uiSnapType;
                stuSnapPara.uiSnapTaskNumber    = pSignalSnapParam->uiSnapTaskNumber;
                stuSnapPara.ullSubType          = pSignalSnapParam->ullSubType;
                stuSnapPara.ucPrivateDataType   = pSignalSnapParam->ucPrivateDataType;
                stuSnapPara.usOverlayBitmap     = pSignalSnapParam->usOverlayBitmap;
          
                stuSnapPara.uiChannel           = pSnapBody->uiChannel;
                stuSnapPara.ucResolution        = pSnapBody->ucResolution;
                stuSnapPara.ucQuality           = pSnapBody->ucQuality;
                stuSnapPara.usNumber            = pSnapBody->usNumber;
                stuSnapPara.uiUser              = pSnapBody->uiUser;
                stuSnapPara.ucSnapMode          = pSnapBody->ucSnapMode;
                memcpy(&stuSnapPara.stuSnapSharePara, (char*)&(pSignalSnapParam->stuSnapSharePara), sizeof(SnapSharePara_t));
                memcpy(&stuSnapPara.stuSnapTime, (char*)&(pSnapBody->stuSnapTime), sizeof(msgTime_t));
                //一天历史抓拍只打开一次流段，然后根据时间段seek流段I帧
                m_pAv_device->Ad_cmd_signal_snap_picutres(stuSnapPara);
            }
        }
        
        
#endif
    }   
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_SIGNALSNAP invalid message length\n");
    }
}

/* 抓拍序列号高两个字节获取 */
void CAvAppMaster::Aa_message_handle_SNAPSERIALNUMBER(const char* msg, int length )
{
    //DEBUG_MESSAGE( "######### CAvAppMaster::Aa_message_handle_SNAPSERIALNUMBER\n");

    SMessageHead *head = (SMessageHead*) msg;
	if( (head->length + sizeof(SMessageHead)) == (unsigned int) length  )
    {
        m_start_work_env[MESSAGE_DEVICE_MANAGER_MACHINE_STATISTICS_GET] = true;
        msgMachineStatistics_t* pMachineStatistics = (msgMachineStatistics_t*)head->body;
        //DEBUG_MESSAGE("##########serial number %d\n", pMachineStatistics->u16RestartCnt);
        m_pAv_device->Ad_get_snap_serial_number(pMachineStatistics);

    }   
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_SIGNALSNAP invalid message length\n");
    }
}

void CAvAppMaster::Aa_message_handle_DEAL_VIDEOLOSS_STATE(const char *msg, int length)
{
    DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_DEAL_VIDEOLOSS_STATE\n");

    SMessageHead *head = (SMessageHead*) msg;
    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
        msgVideolossState_t *option = (msgVideolossState_t *) head->body;
        m_alarm_status.videoLost = option->lVideolossState;
        //! check if the channel has been diabled
       /* for(int index = 0; index < pgAvDeviceInfo->Adi_get_vi_chn_num(); index++)
        {
            if (!m_pMain_stream_encoder->stuVideoEncodeSetting[index].ucChnEnable)
            {
                
                GCL_BIT_VAL_CLEAR(m_alarm_status.videoLost, index);
            }
        }*/
        
        DEBUG_CRITICAL( "CAvAppMaster::Aa_message_handle_DEAL_VIDEOLOSS_STATE videoLost:0x%08x\n", m_alarm_status.videoLost);
        if(m_start_work_flag != 0)
        {
            m_pAv_device_master->Ad_vi_insert_picture(m_alarm_status.videoLost); //videoLost is channel mask
        }
        Aa_send_video_status(_SV_VL_STATUS_);
        m_start_work_env[MESSAGE_DEVICE_MANAGER_GET_VLOSS_STATE] = true;
    }
}

void CAvAppMaster::Aa_message_handle_VIDEOINPUTFORMAT(const char *msg, int length)
{
}

void CAvAppMaster::Aa_message_handle_VIDEOINPUTIMG(const char *msg, int length)
{
#if 0
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgVideoInputImage_t *option = (msgVideoInputImage_t *)pMessage_head->body;
        av_video_image_para_t image;

        DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_VIDEOINPUTIMG\n");

        image.luminance = option->Luminance;
        image.contrast = option->Contrast;
        image.dark_enhance = option->DarkEnhance;
        image.bright_enhance = option->BrightEnhance;
        image.ie_strength = option->IeStrength;
        image.ie_sharp = option->IeSharp;
        image.sf_strength = option->SfStrength;
        image.tf_Strength = option->TfStrength;
        image.motion_thresh = option->MotionThresh;
        image.di_strength = option->DiStrength;
        image.chromaRange = option->ChromaRange;
        image.nr_wf_or_tsr = option->NrWforTsr;
        image.sf_window = option->SfWindow;
        image.dis_mode = option->DisMode;        

        m_pAv_device_master->Ad_set_videoinput_image(image);

        if(m_start_work_flag != 0)
        {
            /*stop all encoder*/
            Aa_stop_all_encoder();
            m_start_work_env[MESSAGE_AVSTREAM_ENCODER_MAINSTREAM_PARAM_GET] = false;
            m_start_work_env[MESSAGE_AVSTREAM_ENCODER_SUBSTREAM_PARAM_GET] = false;

            /*reset vi*/
            Aa_reset_vi();

            /*start all encoder*/
            Aa_start_all_encoder();
        }
    }
    else
    {        
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_VIDEOINPUTIMG invalid message length\n");
    }
    return;
#endif
}

void CAvAppMaster::Aa_message_handle_GET_MAINBOARD_VERSION(const char *msg, int length)
{
    
}
#if !defined(_AV_PRODUCT_CLASS_IPC_)  
void CAvAppMaster::Aa_send_analog_od_alarm_info( int  alarm_interval)
{
    static time_t  last_time[16] = {0};
    static time_t firsttime = 0;
    static unsigned int alarm_threshold = 0;
    static unsigned int alarm_reset_threhold = 0;
    static std::vector<unsigned int> alarm_nums;
    alarm_reset_threhold =alarm_reset_threhold;    
	
    if(0 == firsttime)//!< Initialize for the first time
    {

        alarm_threshold = alarm_interval;
        //printf("alarm_threshold = %d\n",alarm_threshold);
        
        alarm_reset_threhold = alarm_interval - alarm_threshold;

        alarm_nums.clear();
        for(int i=0;i<pgAvDeviceInfo->Adi_max_channel_number();++i)
        {
            m_pAv_device_master->Ad_reset_vda_od_counter(i);
            alarm_nums.push_back(0);
            last_time[i] = time(NULL);
        }
   
        firsttime = time(NULL);
    }

    for(int i=0;i<pgAvDeviceInfo->Adi_max_channel_number();++i)
    { 
        
        DEBUG_MESSAGE("[VS]u32VSChannel=0x%x videoLost =0x%x\n",  m_pVideo_shield_para->u32VSChannel,m_alarm_status.videoLost);
        if(GCL_BIT_VAL_TEST(m_pVideo_shield_para->u32VSChannel, i) && !GCL_BIT_VAL_TEST(m_alarm_status.videoLost, i))
        {
            alarm_nums[i] = m_pAv_device_master->Ad_get_vda_od_alarm_num(i);
            
            //DEBUG_MESSAGE("[VS]chn:%d alarm_num is:%d u32VSChannel=0x%x \n", i, alarm_nums[i], m_pVideo_shield_para->u32VSChannel);
            
            time_t curr_time = time(NULL);
            
            if(GCL_ABS(curr_time - last_time[i]) >= alarm_interval)
            {
                DEBUG_MESSAGE("[VS]alarm_nums[%d] =%d alarm_threshold =%d \n", i,alarm_nums[i],alarm_threshold);
                if(alarm_nums[i] >= alarm_threshold)
                {                
                    m_alarm_status.videoMask = (m_alarm_status.videoMask | (1<<i));
                    DEBUG_MESSAGE("[VS][%s %d]send alarm info! videoMsk:0x%x chn = %d \n", __FUNCTION__, __LINE__,m_alarm_status.videoMask,i);
                    Aa_send_video_status(_SV_OD_STATUS_);
                }
                else
                {
                    if(m_alarm_status.videoMask & (1 << i) )
                    {
                        m_alarm_status.videoMask =(m_alarm_status.videoMask & (~(1 << i)));
                        //printf("[debug][%s %d]send alarm info! videoMsk:0x%x \n", __FUNCTION__, __LINE__,m_alarm_status.videoMask);
                        Aa_send_video_status(_SV_OD_STATUS_);                    
                    }
                }
                
                last_time[i] = time(NULL);
                alarm_nums[i] = 0;  
                ///DEBUG_MESSAGE("[debug]reset counter! chn = %d \n",i);
                m_pAv_device_master->Ad_reset_vda_od_counter(i);
            }
            else
            {   //printf(" < 4s curr_time - last_time =%ld  alarm_nums =%d\n",curr_time - last_time, alarm_nums[i]);
                /*
                if((curr_time -last_time - alarm_nums[i]) >  1)
                {
                    if(m_alarm_status.videoMask & (1 << i) )
                    {
                        m_alarm_status.videoMask =(m_alarm_status.videoMask & (~(1 << i)));
                        printf("[debug][%s %d]send alarm info! videoMsk:0x%x \n", __FUNCTION__, __LINE__,m_alarm_status.videoMask);
                        Aa_send_video_status(_SV_OD_STATUS_);                    
                    }
      
                    //last_time = time(NULL);
                    alarm_nums[i] = 0;  
                    printf("[debug]video shelied,reset counter to count! \n");
                    m_pAv_device_master->Ad_reset_vda_od_counter(i);                
                }*/
            }
        }
    }
}

#endif

#if defined(_AV_PRODUCT_CLASS_IPC_) 
#if !defined(_AV_PLATFORM_HI3518EV200_)
void CAvAppMaster::Aa_send_od_alarm_info( int  alarm_interval)
{
    static time_t  last_time = 0;
    static unsigned int alarm_threshold = 0;
    static unsigned int alarm_reset_threhold = 0;
    static std::vector<unsigned int> alarm_nums;
    
    if(0 == last_time)//!< Initialize for the first time
    {
        alarm_threshold = round(static_cast<float>(alarm_interval*9)/10);        
        alarm_reset_threhold = alarm_interval - alarm_threshold;

        alarm_nums.clear();
        for(int i=0;i<pgAvDeviceInfo->Adi_max_channel_number();++i)
        {
            m_pAv_device_master->Ad_reset_vda_od_counter(i);
            alarm_nums.push_back(0);
        }
   
        last_time = Common::N9M_TMGetShareUTCSecond();
    }

    for(int i=0;i<pgAvDeviceInfo->Adi_max_channel_number();++i)
    { 
        alarm_nums[i] = m_pAv_device_master->Ad_get_vda_od_alarm_num(i);
        //DEBUG_MESSAGE("chn:%d alarm_num is:%d \n", i, alarm_nums[i]);
        
        time_t curr_time = Common::N9M_TMGetShareUTCSecond();
        if((curr_time - last_time) >= alarm_interval)
        {
            if(alarm_nums[i] >= alarm_threshold)
            {                
                m_alarm_status.videoMask = (m_alarm_status.videoMask | (1<<i));
                Aa_send_video_status(_SV_OD_STATUS_);
            }
            else
            {
                if(m_alarm_status.videoMask & (1 << i) )
                {
                    m_alarm_status.videoMask =(m_alarm_status.videoMask & (~(1 << i)));
                    Aa_send_video_status(_SV_OD_STATUS_);                    
                }
            }
            
            last_time = Common::N9M_TMGetShareUTCSecond();
            alarm_nums[i] = 0;  
            m_pAv_device_master->Ad_reset_vda_od_counter(i);
        }
        else
        {
            if((curr_time -last_time - alarm_nums[i]) >  alarm_reset_threhold)
            {
                if(m_alarm_status.videoMask & (1 << i) )
                {
                    m_alarm_status.videoMask =(m_alarm_status.videoMask & (~(1 << i)));
                    Aa_send_video_status(_SV_OD_STATUS_);                    
                }
  
                last_time = Common::N9M_TMGetShareUTCSecond();
                alarm_nums[i] = 0;  
                m_pAv_device_master->Ad_reset_vda_od_counter(i);
            }
        }
    }
}

void CAvAppMaster::Aa_send_od_alarm_info_for_cnr(int  alarm_interval)
{
    static time_t  last_time = 0;
    static unsigned int alarm_threshold = 0;
    static unsigned int alarm_reset_threhold = 0;
    static std::vector<unsigned int> alarm_nums;
    
    if(0 == last_time)//!< Initialize for the first time
    {
        alarm_threshold = round(static_cast<float>(alarm_interval*8)/10);        
        alarm_reset_threhold = alarm_interval - alarm_threshold;

        alarm_nums.clear();
        for(int i=0;i<pgAvDeviceInfo->Adi_max_channel_number();++i)
        {

            m_pAv_device_master->Ad_reset_ive_od_counter(i);
            alarm_nums.push_back(0);
        }
   
        last_time = Common::N9M_TMGetShareUTCSecond();
    }

    for(int i=0;i<pgAvDeviceInfo->Adi_max_channel_number();++i)
    { 

        alarm_nums[i] = m_pAv_device_master->Ad_get_ive_od_counter(i);
        //DEBUG_MESSAGE("chn:%d alarm_num is:%d \n", i, alarm_nums[i]);
        
        time_t curr_time = Common::N9M_TMGetShareUTCSecond();
        if((curr_time - last_time) >= alarm_interval)
        {
            if(alarm_nums[i] >= alarm_threshold)
            {                
                m_alarm_status.videoMask = (m_alarm_status.videoMask | (1<<i));
                Aa_send_video_status(_SV_OD_STATUS_);
            }
            else
            {
                if(m_alarm_status.videoMask & (1 << i) )
                {
                    m_alarm_status.videoMask =(m_alarm_status.videoMask & (~(1 << i)));
                    Aa_send_video_status(_SV_OD_STATUS_);                    
                }
            }
            
            last_time = Common::N9M_TMGetShareUTCSecond();
            alarm_nums[i] = 0;  

            m_pAv_device_master->Ad_reset_ive_od_counter(i);
        }
        else
        {
            if((curr_time -last_time - alarm_nums[i]) >  alarm_reset_threhold)
            {
                if(m_alarm_status.videoMask & (1 << i) )
                {
                    m_alarm_status.videoMask =(m_alarm_status.videoMask & (~(1 << i)));
                    Aa_send_video_status(_SV_OD_STATUS_);                    
                }
  
                last_time = Common::N9M_TMGetShareUTCSecond();
                alarm_nums[i] = 0;  

                m_pAv_device_master->Ad_reset_ive_od_counter(i);
            }
        }
    }    
}
#endif

bool CAvAppMaster::Aa_is_customer_cnr()
{
    static char customer_name[32] = {0};

    if(0 == strlen(customer_name))
    {
        memset(customer_name, 0 ,sizeof(customer_name));
        pgAvDeviceInfo->Adi_get_customer_name(customer_name, sizeof(customer_name));
    }

    if(0 == strcmp(customer_name, "CUSTOMER_CNR"))
    {
        return true;
    }

    return false;
}

bool CAvAppMaster::Aa_is_customer_dc()
{
    static char customer_name[32] = {0};

    if(0 == strlen(customer_name))
    {
        memset(customer_name, 0 ,sizeof(customer_name));
        pgAvDeviceInfo->Adi_get_customer_name(customer_name, sizeof(customer_name));
    }

    if(0 == strcmp(customer_name, "CUSTOMER_DC"))
    {
        return true;
    }

    return false;    
}


#if defined(_AV_PLATFORM_HI3518C_)
int CAvAppMaster::Aa_sensor_read_reg_value(int reg_addr)
{
    int fd = -1;
    int ret = -1;

    I2C_DATA_S i2c_data;
    
    fd = open("/dev/hi_i2c", 0);
    if(fd<0)
    {
        _AV_KEY_INFO_(_AVD_APP_, "Open hi_i2c error!\n");
        return -1;
    }
    
    i2c_data.dev_addr = 0x34;
    i2c_data.reg_addr = reg_addr;
    i2c_data.addr_byte_num = 2;
    i2c_data.data_byte_num = 1;

    ret = ioctl(fd, CMD_I2C_READ, &i2c_data);

    if (ret)
    {
        _AV_KEY_INFO_(_AVD_APP_, "hi_i2c read faild!\n");
        close(fd);
        return -1;
    }

    close(fd);

    return i2c_data.data;
}
sensor_e CAvAppMaster::Aa_sensor_get_type()
{
    int addr = 0x3008;
    int value = -1;
    value = Aa_sensor_read_reg_value(addr);
     _AV_KEY_INFO_(_AVD_APP_, "the register 0x3008 value is:%#x \n", value);
    if(value == 0x0)
    {
        return ST_SENSOR_IMX225;
    }
    else
    {
        return ST_SENSOR_IMX238;
    }
}
#endif

static pthread_t  vi_monitor_thread_id;
static bool vi_monitor_thread_start = false;

void *vi_interrupt_detect_thread_body(void *args);

void *vi_interrupt_detect_thread_body(void *args)
{
    CAvAppMaster* av_app_master = (CAvAppMaster*)args;
    
    DEBUG_MESSAGE("hivi_interrupt_detect_thread_body start work! \n");
    //sleep 5s to make sure the isp init done over completly!!
    sleep(5);
    while(vi_monitor_thread_start)
    {
        av_app_master->Aa_vi_interrput_detect_proc();
        sleep(3);
    }

    return NULL;
}

int CAvAppMaster::Aa_start_vi_interrput_monitor()
{
    if(vi_monitor_thread_start == false)
    {
        vi_monitor_thread_start = true;
        pthread_create(&vi_monitor_thread_id, 0, vi_interrupt_detect_thread_body, this);
    }

    return 0;
}
int CAvAppMaster::Aa_stop_vi_interrput_monitor()
{
    if(vi_monitor_thread_start == true)
    {
        vi_monitor_thread_start = false;
        pthread_join(vi_monitor_thread_id, NULL);
    }

    return 0;
}

int CAvAppMaster::Aa_vi_interrput_detect_proc()
{
    bool bRet = false;
    unsigned int interrupt_cnt = 0;
    static unsigned long long interrupt = 0;
    
    
    bRet = m_pAv_device_master->Ad_get_vi_chn_interrut_cnt(0, interrupt_cnt);
    if(false == bRet)
    {
        //the vi chn is disable
        interrupt = 0;
    }
    else
    {
        if((interrupt_cnt >0) && (interrupt == interrupt_cnt))
        {
            Aa_reset_sensor();
            interrupt = 0;
        }
        else
        {
            interrupt = interrupt_cnt;
        }
    }

    return 0;
}

int CAvAppMaster::Aa_reset_sensor()
{
    DEBUG_MESSAGE( "the vi interrupt stopped, the isp will restart! \n");
#if defined(_AV_PLATFORM_HI3516A_)
    RM_ISP_API_IspExit();
    usleep(100*1000);
    HI_MPI_SYS_SetReg(0x20140004, 0x0);//sensor reset
    usleep(100*1000);
    HI_MPI_SYS_SetReg(0x20140004, 0xFF);//sensor power on
    usleep(100*1000);
    
    ISPInitParam_t isp_param;
    
    memset(&isp_param, 0, sizeof(ISPInitParam_t));
    isp_param.sensor_type = pgAvDeviceInfo->Adi_get_sensor_type();
    isp_param.focus_len = pgAvDeviceInfo->Adi_get_focus_length();
    isp_param.u8WDR_Mode = WDR_MODE_NONE;
    isp_param.etOptResolution = pgAvDeviceInfo->Adi_get_isp_output_mode();
    isp_param.u8FstInit_flag = 1;
    DEBUG_MESSAGE("isp init, sensor type:%d focus len:%d wdr mode:%d resolution:%d \n", isp_param.sensor_type, isp_param.focus_len, isp_param.u8WDR_Mode, isp_param.etOptResolution);
    RM_ISP_API_Init(&isp_param);
    RM_ISP_API_InitSensor();
    RM_ISP_API_IspStart();
    usleep(100*1000);
#elif defined(_AV_PLATFORM_HI3518C_)
    RM_ISP_API_IspExit();
    usleep(100*1000);
    HI_MPI_SYS_SetReg(0x20140040, 0X0);//sensor reset
    usleep(100*1000);
    HI_MPI_SYS_SetReg(0x20140040, 0XFF);;//sensor power on
    usleep(100*1000);
    RM_ISP_API_Init(pgAvDeviceInfo->Adi_get_sensor_type(), pgAvDeviceInfo->Adi_get_focus_length());
    RM_ISP_API_InitSensor();
    RM_ISP_API_IspStart();
    usleep(100*1000);
#elif defined(_AV_PLATFORM_HI3518EV200_)
    //do something
    RM_ISP_API_IspExit();
    usleep(100*1000);
    HI_MPI_SYS_SetReg(0x20140080, 0x0);//sensor reset
    usleep(100*1000);
    HI_MPI_SYS_SetReg(0x20140080, 0xFF);//sensor power on
    usleep(100*1000);
    
    ISPInitParam_t isp_param;
    
    memset(&isp_param, 0, sizeof(ISPInitParam_t));
    isp_param.sensor_type = pgAvDeviceInfo->Adi_get_sensor_type();
    isp_param.focus_len = pgAvDeviceInfo->Adi_get_focus_length();
    isp_param.u8WDR_Mode = WDR_MODE_NONE;
    isp_param.etOptResolution = pgAvDeviceInfo->Adi_get_isp_output_mode();
    DEBUG_MESSAGE("isp init, sensor type:%d focus len:%d wdr mode:%d resolution:%d \n", isp_param.sensor_type, isp_param.focus_len, isp_param.u8WDR_Mode, isp_param.etOptResolution);
    RM_ISP_API_Init(&isp_param);
    RM_ISP_API_InitSensor();
    RM_ISP_API_IspStart();
    usleep(100*1000);    
#endif
    /*set camera attribute*/
    m_pAv_device_master->Ad_SetCameraColor( &(m_pstuCameraColorParam->stuVideoImageSetting[0]) );
    m_pAv_device_master->Ad_SetCameraAttribute( 0xffffffff, &(m_pstuCameraAttrParam->stuViParam[0].image) );

    return 0;
}
#endif   
#endif

void CAvAppMaster::Aa_message_handle_SYSTEMPARA(const char *msg, int length)
{
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {

#if !defined(_AV_PRODUCT_CLASS_IPC_)
        bool changlanguage = false;
#endif
        msgGeneralSetting_t *pSystem_setting = (msgGeneralSetting_t *)pMessage_head->body;
        av_tvsystem_t av_tvsystem = m_av_tvsystem;/* 临时变量与系统参数比较 如果不同重启编码*/
        bool breset_encoder = false;

        m_av_tvsystem = pSystem_setting->stuGeneralSetting.ucVideoStandard ? _AT_NTSC_ : _AT_PAL_;

//! Added for IPC, Nov 2014
#ifdef _AV_PRODUCT_CLASS_IPC_
#if defined(_AV_PLATFORM_HI3518C_)
    if(PTD_913_VA != pgAvDeviceInfo->Adi_product_type())
    {
        if (NULL != m_pMain_stream_encoder)
        {
            av_tvsystem_t norm = (0 == pSystem_setting->stuGeneralSetting.ucVideoStandard)?_AT_PAL_:_AT_NTSC_;
            //Hi3516A 
            Aa_set_isp_framerate(norm, m_pMain_stream_encoder->stuVideoEncodeSetting[0].ucFrameRate);
        }
    }
#endif
    if(av_tvsystem != m_av_tvsystem)
    {
        for(int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); ++chn)
        {
            int width = 0;
            int height = 0;
            av_resolution_t res;
            if(NULL != m_pSub_stream_encoder)
            {
                pgAvDeviceInfo->Adi_get_video_resolution(&m_pSub_stream_encoder->stuVideoEncodeSetting[chn].ucResolution, &res, true);
                pgAvDeviceInfo->Adi_get_video_size(res, m_av_tvsystem, &width, &height);
                pgAvDeviceInfo->Adi_set_stream_size(chn, _AST_SUB_VIDEO_, width,height);                
            }
            if(NULL != m_pSub2_stream_encoder)
            {
                pgAvDeviceInfo->Adi_get_video_resolution(&m_pSub2_stream_encoder->stuVideoEncodeSetting[chn].ucResolution, &res, true);
                pgAvDeviceInfo->Adi_get_video_size(res, m_av_tvsystem, &width, &height);
                pgAvDeviceInfo->Adi_set_stream_size(chn, _AST_SUB2_VIDEO_, width,height);                
            }
        }
    }
#endif
        if ((av_tvsystem != m_av_tvsystem) && (m_start_work_flag != 0) && (Aa_everything_is_ready() == 0))
        {
            breset_encoder = true;
        }
        m_start_work_env[MESSAGE_CONFIG_MANAGE_GENERAL_PARAM_GET] = true;

        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_SYSTEMPARA(%d)(%d)\n", pSystem_setting->stuGeneralSetting.ucVideoStandard, pSystem_setting->stuGeneralSetting.ucVgaResolution);

        if(m_pSystem_setting != NULL)
        {
            if(memcmp(m_pSystem_setting, pSystem_setting, sizeof(msgGeneralSetting_t)) != 0)
            {
                if( m_start_work_flag != 0 )
                {
#if defined(_AV_HAVE_VIDEO_OUTPUT_)                    
                    this->Aa_Change_VoResolution_TvSystem( pSystem_setting->stuGeneralSetting.ucVideoStandard, pSystem_setting->stuGeneralSetting.ucVgaResolution );
#endif
                }
                else
                {
                    if(pgAvDeviceInfo)
                    {
                        pgAvDeviceInfo->Adi_set_system(m_av_tvsystem);
                    }
                }
#if !defined(_AV_PRODUCT_CLASS_IPC_)
                if(m_pSystem_setting->stuGeneralSetting.ucLanguageType != pSystem_setting->stuGeneralSetting.ucLanguageType)
                {
                    changlanguage = true;
                }
#endif
                memcpy(m_pSystem_setting, pSystem_setting, sizeof(msgGeneralSetting_t));
            }
        }
        else
        {
            _AV_FATAL_((m_pSystem_setting = new msgGeneralSetting_t) == NULL, "CAvAppMaster::Aa_message_handle_SYSTEMPARA OUT OF MEMORY\n");
            memcpy(m_pSystem_setting, pSystem_setting, sizeof(msgGeneralSetting_t));
#if !defined(_AV_PRODUCT_CLASS_IPC_)
            changlanguage = true;
#endif
            if(pgAvDeviceInfo)
            {
                pgAvDeviceInfo->Adi_set_system(m_av_tvsystem);
            }
        }  
#if defined(_AV_HAVE_VIDEO_ENCODER_)
        if ((true == breset_encoder) && (m_start_work_flag != 0))
        {
            unsigned char streamtype_mask = Aa_get_all_streamtype_mask();
            Aa_stop_selected_streamtype_encoders(pgAvDeviceInfo->Aa_get_all_analog_chn_mask(), streamtype_mask);
            Aa_start_selected_streamtype_encoders(pgAvDeviceInfo->Aa_get_all_analog_chn_mask(), streamtype_mask);
        }
#endif
        pgAvDeviceInfo->Adi_set_audio_cvbs_ctrl_switch(m_pSystem_setting->stuGeneralSetting.u8PreviewAudioSwitch?true:false);
#ifdef _AV_HAVE_VIDEO_ENCODER_
#if !defined(_AV_PRODUCT_CLASS_IPC_)
        if(changlanguage == true)
        {         
            //设置语言
            Json::Value special_info;    
            N9M_GetSpecInfo(special_info);
            int languageID = 0;
            
            GetLanguageSysConfigID(pSystem_setting->stuGeneralSetting.ucLanguageType, languageID);
            
            DEBUG_CRITICAL("----ucLanguageType =%d languageID =%d UI_LANG_CFG =%s\n",pSystem_setting->stuGeneralSetting.ucLanguageType, languageID, special_info["UI_LANG_CFG"][pSystem_setting->stuGeneralSetting.ucLanguageType]["CFG_FILE"].asCString());
            LoadLanguage(special_info, languageID);
            m_pAv_device->Ad_ChangeLanguageCacheChar();
            /*
            DEBUG_WARNING("----ucLanguageType =%d UI_LANG_CFG =%s\n",pSystem_setting->stuGeneralSetting.ucLanguageType, special_info["UI_LANG_CFG"][pSystem_setting->stuGeneralSetting.ucLanguageType]["CFG_FILE"].asCString());
            if (special_info["UI_LANG_CFG"][pSystem_setting->stuGeneralSetting.ucLanguageType]["CFG_FILE"].isString())
            {
                std::string language_dir = special_info["UI_LANG_CFG"][pSystem_setting->stuGeneralSetting.ucLanguageType]["CFG_FILE"].asCString();
                std::string ab_dir = "/usr/local/bin/";
                ab_dir += language_dir;
                
                GuiTKClearLanguage();
                GuiTKBindLanguage
                GuiTKBindLanguage((char*)ab_dir.c_str(), NULL, NULL);
                m_pAv_device->Ad_ChangeLanguageCacheChar();
            }*/
        }
#endif
#endif
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_SYSTEMPARA invalid message length\n");
    }
}

void CAvAppMaster::Aa_message_handle_REBOOT(const char *msg, int length)
{
    DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_REBOOT\n");

    SMessageHead *head = (SMessageHead*) msg;
    if ((head->length + sizeof(SMessageHead)) == (unsigned int) length)
    {
#if defined(_AV_HAVE_VIDEO_ENCODER_)
        //unsigned char streamtype_mask = Aa_get_all_streamtype_mask();
        if (head->event == MESSAGE_DEVICE_MANAGER_UPGRADE_TRANSFER_STATUS)
        {
            //Aa_start_selected_streamtype_encoders(pgAvDeviceInfo->Aa_get_all_analog_chn_mask(), streamtype_mask);
            Aa_restart_work();
            //printf("cxliu 111ddd\n");
        }
        else
        {
            //Aa_stop_selected_streamtype_encoders(pgAvDeviceInfo->Aa_get_all_analog_chn_mask(), streamtype_mask);
            Aa_stop_work();
            //printf("cxliu 222ddd\n");
        }
#endif
        if ((EVENT_SERVER_WATCHDOG_POWERDOWN == head->event) || (MESSAGE_DEVICE_MANAGER_SYSTEM_REBOOT == head->event))
        {
            Common::N9M_MSGSendMessage(m_message_client_handle, 0, EVENT_SERVER_WATCHDOG_UNREGISTER, 0, (char *) &m_watchdog, sizeof(SWatchdogMsg), 0);
            DEBUG_MESSAGE( "Wait reboot...\n");
            sleep(1);
#if defined(_AV_HAVE_VIDEO_DECODER_)
            DEBUG_MESSAGE( "Close Decoder...\n");
            m_pAv_device_master->Ad_stop_playback_dec();
#endif

#if defined(_AV_HAVE_AUDIO_OUTPUT_)
            DEBUG_MESSAGE( "Close AO...\n");
            this->Aa_stop_ao();
#endif

#if defined(_AV_HAVE_VIDEO_OUTPUT_)
            DEBUG_MESSAGE( "Close VO...\n");
            this->Aa_stop_spot();
            m_pAv_device_master->Ad_stop_preview_output( pgAvDeviceInfo->Adi_get_main_out_mode(), 0 );
            this->Aa_stop_output( pgAvDeviceInfo->Adi_get_main_out_mode(), 0 );
#endif
            while(1)
            {
                DEBUG_MESSAGE("avStreaming, wait for reboot or standby\n");
                sleep(3);
            }
        }
#ifdef _AV_PRODUCT_CLASS_NVR_
        else if( MESSAGE_DEVICE_MANAGER_UPGRADE_IN_PROCESSING == head->event )
        {
            m_pAv_device_master->Ad_SetSystemUpgradeState(1);
            Common::N9M_MSGResponseMessage(m_message_client_handle, head, NULL, 0);
        }
        else if( MESSAGE_DEVICE_MANAGER_UPGRADE_TRANSFER_STATUS == head->event )
        {
            m_pAv_device_master->Ad_SetSystemUpgradeState(2);
            Common::N9M_MSGResponseMessage(m_message_client_handle, head, NULL, 0);
        }
#endif
        else
        {
            Common::N9M_MSGResponseMessage(m_message_client_handle, head, NULL, 0);
        }
    }
    else
    {
        DEBUG_ERROR( "the param len isn't correct\n");
    }
    return;
}

void CAvAppMaster::Aa_message_handle_CHANGE_SYSTEM_TIME( const char* msg, int length )
{
#if defined(_AV_PRODUCT_CLASS_IPC_)    
    DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_CHANGE_SYSTEM_TIME IPC none of my buness\n" );
    return;
#endif
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_CHANGE_SYSTEM_TIME %x\n\n", pMessage_head->event);

        return;
#if defined(_AV_HAVE_VIDEO_ENCODER_)
        unsigned char streamtype_mask = Aa_get_all_streamtype_mask();
        if(pMessage_head->event == MESSAGE_DEVICE_MANAGER_SYSTEM_TIME_WILL_CHANGE)
        {
            DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_CHANGE_SYSTEM_TIME MESSAGE_DEVICE_SYSTEM_TIME_WILL_CHANGE\n\n");
            Aa_stop_selected_streamtype_encoders(pgAvDeviceInfo->Aa_get_all_analog_chn_mask(), streamtype_mask);
        }
        else if(pMessage_head->event == MESSAGE_DEVICE_MANAGER_SYSTEM_TIME_CHANGED)
        {
            DEBUG_MESSAGE("CAvAppMaster::Aa_message_handle_CHANGE_SYSTEM_TIME MESSAGE_DEVICE_SYSTEM_TIME_CHANGED\n\n");
            Aa_start_selected_streamtype_encoders(pgAvDeviceInfo->Aa_get_all_analog_chn_mask(), streamtype_mask);
        }
#endif
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_CHANGE_SYSTEM_TIME invalid message length\n");
    }
    return;
}

#if defined(_AV_HAVE_AUDIO_INPUT_)
#if defined(_AV_PRODUCT_CLASS_IPC_)
void CAvAppMaster::Aa_message_handle_AUDIO_PARAM(const char *msg, int length)
{
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        m_start_work_env[MESSAGE_CONFIG_MANAGE_AUDIO_PARAM_GET] = true;
        
        msgAudioSetting_t *audio_param =  (msgAudioSetting_t *)pMessage_head->body;
        if(NULL != audio_param)
        {
            _AV_KEY_INFO_(_AVD_APP_, "Aa_message_handle_AUDIO_PARAM audio type:%d volume:%d \n", audio_param->stuAudioSetting.u8AudioType, audio_param->stuAudioSetting.u8AudioVoice);

            bool bRestart_encoder = false;
            bool bReset_volume = false;
            if(NULL != m_pAudio_param)
            {
                if(0 != memcmp(m_pAudio_param, audio_param, sizeof(msgAudioSetting_t)))
                {
                    if(m_pAudio_param->stuAudioSetting.u8AudioType != audio_param->stuAudioSetting.u8AudioType)
                    {
                        bRestart_encoder = true;
                    }
                    if(m_pAudio_param->stuAudioSetting.u8AudioVoice != audio_param->stuAudioSetting.u8AudioVoice)
                    {
                        bReset_volume = true;
                    }
                    memcpy(m_pAudio_param, audio_param, sizeof(msgAudioSetting_t));
                }
                else
                {
                    return;
                }
            }
            else
            {
                m_pAudio_param = new msgAudioSetting_t;
                bRestart_encoder = true;
                bReset_volume = true;
                memset(m_pAudio_param, 0, sizeof(msgAudioSetting_t));
                memcpy(m_pAudio_param, audio_param, sizeof(msgAudioSetting_t));
            }
            
            if ((0 != m_start_work_flag ) && (true == bRestart_encoder))
            {
#if defined(_AV_HAVE_VIDEO_ENCODER_)
                unsigned char streamtype_mask = Aa_get_all_streamtype_mask();
                Aa_stop_selected_streamtype_encoders(pgAvDeviceInfo->Aa_get_all_analog_chn_mask(), streamtype_mask);
                Aa_start_selected_streamtype_encoders(pgAvDeviceInfo->Aa_get_all_analog_chn_mask(), streamtype_mask);
#endif
            }

            if((0 != m_start_work_flag )  && (true == bReset_volume))
            {
                m_pAv_device_master->Ad_set_macin_volum(audio_param->stuAudioSetting.u8AudioVoice);
            }
        }
    }
    else
    {
        _AV_KEY_INFO_(_AVD_APP_, "CAvAppMaster::Aa_message_handle_AUDIO_PARAM invalid message length\n");
    }

    return;
}

#define IO_CMD(type, nr) (0x5a0000a5 | (((type) & 0xff) << 16) | (((nr) & 0xff) << 8))
#define I2C_IOC_INIT_FM1288_SW IO_CMD('I', 0x0D)
#define I2C_IOC_INIT_FM1288_HW IO_CMD('H', 0x01)

int CAvAppMaster::Aa_outer_codec_fm1288_init(eProductType product)
{
    enum
    {
        FM1288_REG_ARRAY_BUS = 0,
        FM1288_REG_ARRAY_TAXI = 1,
        FM1288_REG_ARRAY_RAILWAY = 2,
        FM1288_REG_ARRAY_MAX,
    };
      
    unsigned long init_cmd_array = FM1288_REG_ARRAY_BUS;
    unsigned long ioctl_cmd;
    char *i2c_dev_name;
       
    switch (product)
    {
        case PTD_916_VA:
            i2c_dev_name = (char *)"/dev/Gpioi2c";
            ioctl_cmd = I2C_IOC_INIT_FM1288_SW;
            init_cmd_array = FM1288_REG_ARRAY_BUS;

            /*withdrawl reset GPIO13_7*/
            HI_MPI_SYS_SetReg(0x20210200, 0xFF);
            /*to close /dev/mem*/
            HI_MPI_SYS_CloseFd();
            break;
    
        case PTD_718_VA:
            i2c_dev_name = (char *)"/dev/Gpioi2c";
            ioctl_cmd = I2C_IOC_INIT_FM1288_SW;
            init_cmd_array = FM1288_REG_ARRAY_BUS;

            /*withdrawl reset GPIO6_7*/
            HI_MPI_SYS_SetReg(0x201A0200, 0xFF);
            /*to close /dev/mem*/
            HI_MPI_SYS_CloseFd();
            break;
        case PTD_913_VB:
            i2c_dev_name = (char *)"/dev/Gpioi2c";
            ioctl_cmd = I2C_IOC_INIT_FM1288_SW;
            init_cmd_array = FM1288_REG_ARRAY_BUS;

            /*withdrawl reset GPIO6_1*/
            HI_MPI_SYS_SetReg(0x201A0008, 0xFF);
            /*to close /dev/mem*/
            HI_MPI_SYS_CloseFd();
            break;
    
#if 0
        case PTD_712_VA:
        case PTD_712_VE:
        case PTD_913_VA:
        case PTD_714_VA:
            i2c_dev_name = (char *)"/dev/hi_i2c";
            ioctl_cmd = I2C_IOC_INIT_FM1288_HW;
            init_cmd_array = FM1288_REG_ARRAY_RAILWAY;
            break;
    
        case PTD_712_VD:
            if (SYSTEM_TYPE_RAILWAYIPC_SLAVE == N9M_GetSystemType())
            {
                /*only slave chip get fm1288*/
                i2c_dev_name = (char *)"/dev/hi_i2c";
                ioctl_cmd = I2C_IOC_INIT_FM1288_HW;
                init_cmd_array = FM1288_REG_ARRAY_TAXI;
            }
            else
            {
                return;
            }
            break;

        case PTD_712_VF:
            i2c_dev_name = (char *)"/dev/hi_i2c";
            ioctl_cmd = I2C_IOC_INIT_FM1288_HW;
            init_cmd_array = FM1288_REG_ARRAY_TAXI;
            break;
#endif
    
        default:
            Common::DEBUG_MESSAGE("nothing to be done\n");
            return 0;
        }
    
    int i2c_fd = open(i2c_dev_name, O_RDONLY);

    if (i2c_fd < 0)
    {
        Common::DEBUG_MESSAGE("fail to open %s: %s\n", i2c_dev_name, strerror(errno));
        return -1;
    }
    
        if (0 != ioctl(i2c_fd, ioctl_cmd, &init_cmd_array))
        {
            Common::DEBUG_MESSAGE("fail to ioctl 0x%X: %s\n", ioctl_cmd, strerror(errno));
            close(i2c_fd);
            return -1;
        }
    
        switch (product)
        {
            case PTD_718_VA:
                /*reset gpio to I2C0*/
                /*0: GPIO3_3 1: SPI0_CLK; 2: I2C0_SCL*/
                HI_MPI_SYS_SetReg(0x200F0040, 0x2);
                /*0: GPIO3_4 1: SPI0_CLK; 2: I2C0_SDA*/
                HI_MPI_SYS_SetReg(0x200F0044, 0x2);
                /*to close /dev/mem*/
                HI_MPI_SYS_CloseFd();
                break;

            case PTD_913_VB:
                /*reset gpio to I2C2*/
                /*0: GPIO4_4 1: I2C2_SCL*/
                HI_MPI_SYS_SetReg(0x200F0064, 0x1);
                /*0: GPIO4_3 1: I2C2_SCA*/
                HI_MPI_SYS_SetReg(0x200F0060, 0x1);
                /*to close /dev/mem*/
                HI_MPI_SYS_CloseFd();
                break;

    
            default:
                break;
        }
    
        close(i2c_fd);
        return 0;
}
#endif
#endif

#if defined(_AV_HAVE_VIDEO_INPUT_)
void CAvAppMaster::Aa_send_video_status(int type)
{
    msgVideoStatus_t VideoStatus;
    memset(&VideoStatus, 0, sizeof(msgVideoStatus_t));

    for(int chn = 0; chn < pgAvDeviceInfo->Adi_get_vi_chn_num(); chn++)
    {
        if (_VS_DIGITAL_ == pgAvDeviceInfo->Adi_get_video_source(chn))
        {
            VideoStatus.u32Src[chn] = 0xff;
        }
        else
        {
            VideoStatus.u32Src[chn] = 0;
            if (type & _SV_VL_STATUS_)
            {
                if(GCL_BIT_VAL_TEST(m_alarm_status.videoLost, chn))
                {
                    VideoStatus.u32Status[chn] = 1;
                }
            }
            else if (type & _SV_MD_STATUS_)
            {
                if(!GCL_BIT_VAL_TEST(m_alarm_status.videoLost, chn) && GCL_BIT_VAL_TEST(m_alarm_status.motioned, chn))
                {
                    VideoStatus.u32Status[chn] = 1;
                }
            }
#if 1//defined(_AV_PRODUCT_CLASS_IPC_)
            else if(type&_SV_OD_STATUS_)
             {
                if(GCL_BIT_VAL_TEST(m_alarm_status.videoMask, chn))
                {
                    VideoStatus.u32Status[chn] = 1;
                }
             }
#endif
        }
    }
    if (type & _SV_VL_STATUS_)
    {
        //Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_EVENT_SET_VL_STATUS, 0, (const char *) &VideoStatus, sizeof(msgVideoStatus_t), 0);
    }
    else if (type & _SV_MD_STATUS_)
    {
        Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_EVENT_SET_MD_STATUS, 0, (const char *) &VideoStatus, sizeof(msgVideoStatus_t), 0);
    }
#if 1//defined(_AV_PRODUCT_CLASS_IPC_)
    else if(type&_SV_OD_STATUS_)
    {
#if defined(_AV_PRODUCT_CLASS_IPC_)
        DEBUG_MESSAGE("send VS state, the state is:%d \n", VideoStatus.u32Status[0]);
#endif
        Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_EVENT_SET_VS_STATUS, 0, (const char *) &VideoStatus, sizeof(msgVideoStatus_t), 0);
    }
#endif
    return;
}

int CAvAppMaster::Aa_stop_vi()
{
    m_pAv_device->Ad_stop_vi();

    return 0;
}

int CAvAppMaster::Aa_stop_selected_vi(unsigned long long int chn_mask)
{
    m_pAv_device->Ad_stop_selected_vi(chn_mask);
    return 0;
}


int CAvAppMaster::Aa_get_video_input_format(int phy_video_chn_num, av_resolution_t *pResolution, int *pFrame_rate, bool *pProgress)
{
#if 0
    switch(m_pInput_video_format->InputFormat[phy_video_chn_num])
    {/*0: Invalid; 1:1080P60; 2:1080P50; 3:1080P30; 4: 1080P25; 5: 1080P24; 6: 1080I60
 * 7: 1080I50; 8:720P60; 9: 720P50; A: 720P30; B: 720P25; C: 720P24; D: 1920X1035I60*/
        case 1:/*1080P60*/
            _AV_POINTER_ASSIGNMENT_(pResolution, _AR_1080_);
            _AV_POINTER_ASSIGNMENT_(pFrame_rate, 60);
            _AV_POINTER_ASSIGNMENT_(pProgress, true);
            break;

        case 2:/*1080P50*/
            _AV_POINTER_ASSIGNMENT_(pResolution, _AR_1080_);
            _AV_POINTER_ASSIGNMENT_(pFrame_rate, 50);
            _AV_POINTER_ASSIGNMENT_(pProgress, true);
            break;

        case 3:/*1080P30*/
            _AV_POINTER_ASSIGNMENT_(pResolution, _AR_1080_);
            _AV_POINTER_ASSIGNMENT_(pFrame_rate, 30);
            _AV_POINTER_ASSIGNMENT_(pProgress, true);
            break;

        case 4:/*1080P25*/
        default:
            _AV_POINTER_ASSIGNMENT_(pResolution, _AR_1080_);
            _AV_POINTER_ASSIGNMENT_(pFrame_rate, 25);
            _AV_POINTER_ASSIGNMENT_(pProgress, true);
            break;

        case 5:/*1080P24*/
            _AV_POINTER_ASSIGNMENT_(pResolution, _AR_1080_);
            _AV_POINTER_ASSIGNMENT_(pFrame_rate, 24);
            _AV_POINTER_ASSIGNMENT_(pProgress, true);
            break;

        case 6:/*1080I60*/
            _AV_POINTER_ASSIGNMENT_(pResolution, _AR_1080_);
            _AV_POINTER_ASSIGNMENT_(pFrame_rate, 30);
            _AV_POINTER_ASSIGNMENT_(pProgress, false);
            break;

        case 7:/*1080I50*/
            _AV_POINTER_ASSIGNMENT_(pResolution, _AR_1080_);
            _AV_POINTER_ASSIGNMENT_(pFrame_rate, 25);
            _AV_POINTER_ASSIGNMENT_(pProgress, false);
            break;

        case 8:/*720P60*/
            _AV_POINTER_ASSIGNMENT_(pResolution, _AR_720_);
            _AV_POINTER_ASSIGNMENT_(pFrame_rate, 60);
            _AV_POINTER_ASSIGNMENT_(pProgress, true);
            break;

        case 9:/*720P50*/
            _AV_POINTER_ASSIGNMENT_(pResolution, _AR_720_);
            _AV_POINTER_ASSIGNMENT_(pFrame_rate, 50);
            _AV_POINTER_ASSIGNMENT_(pProgress, true);
            break;

        case 10:/*720P30*/
            _AV_POINTER_ASSIGNMENT_(pResolution, _AR_720_);
            _AV_POINTER_ASSIGNMENT_(pFrame_rate, 30);
            _AV_POINTER_ASSIGNMENT_(pProgress, true);
            break;

        case 11:/*720P25*/
            _AV_POINTER_ASSIGNMENT_(pResolution, _AR_720_);
            _AV_POINTER_ASSIGNMENT_(pFrame_rate, 25);
            _AV_POINTER_ASSIGNMENT_(pProgress, true);
            break;

        case 12:/*720P24*/
            _AV_POINTER_ASSIGNMENT_(pResolution, _AR_720_);
            _AV_POINTER_ASSIGNMENT_(pFrame_rate, 24);
            _AV_POINTER_ASSIGNMENT_(pProgress, true);
            break;
    }
#endif
    return 0;
}

int CAvAppMaster::Aa_get_vi_config_set(int phy_video_chn_num, vi_config_set_t &vi_config, void *pVi_Para/*=NULL*/, vi_config_set_t *pPrimary_attr/*=NULL*/, vi_config_set_t *pMinor_attr/*=NULL*/)
{
#if defined(_AV_PRODUCT_CLASS_HDVR_)
    if (pgAvDeviceInfo->Adi_get_video_source(phy_video_chn_num) == _VS_DIGITAL_)
    {
        return 0;
    }
#endif

    if (true == pgAvDeviceInfo->Adi_weather_phychn_sdi(phy_video_chn_num))
    {
        Aa_get_video_input_format(phy_video_chn_num, &vi_config.resolution, &vi_config.frame_rate, &vi_config.progress);
    }
    else /*_ANALOG_*/
    {
        unsigned char ucResolution = 0xff;
        if (pVi_Para != NULL)
        {          
            msgVideoEncodeSetting_t *pMain_stream_encoder = (msgVideoEncodeSetting_t *)pVi_Para;
            ucResolution = pMain_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucResolution;
        }
        else
        {
            ucResolution = m_pMain_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucResolution;
        }

        /*vi source config*/
        switch(ucResolution)
        {
            case 0:/*CIF*/
                vi_config.resolution = _AR_CIF_;
                break;
            case 1:/*HD1*/
                vi_config.resolution = _AR_HD1_;
                break;
            case 2:/*D1*/
                vi_config.resolution = _AR_D1_;
                break;
            case 3:/*QCIF*/
                vi_config.resolution = _AR_QCIF_;              
                break;
            case 6:/*720p*/
                vi_config.resolution = _AR_720_;
                break;
            case 7:/*1080p*/
                vi_config.resolution = _AR_1080_;
                break;
            case 8:/*3M*/
                vi_config.resolution = _AR_3M_;
                break;
            case 9:/*5M*/
                vi_config.resolution = _AR_5M_;
                break;
            case 10:/*WQCIF*/
                vi_config.resolution = _AR_960H_WQCIF_;              
                break;
            case 11:/*WCIF*/
                vi_config.resolution = _AR_960H_WCIF_;              
                break;
            case 12:/*WHD1*/
                vi_config.resolution = _AR_960H_WHD1_;              
                break;
            case 13:/*WD1*/
                vi_config.resolution = _AR_960H_WD1_;              
                break;
            case 14:
                vi_config.resolution = _AR_960P_;
                break;
            case 15://FOR NVR
                vi_config.resolution = _AR_Q1080P_;
                break;
            case 16:
                vi_config.resolution = _AR_SVGA_;
                break;
            case 17:
                vi_config.resolution = _AR_XGA_;
                break;

            default:
                vi_config.resolution = _AR_D1_;
                DEBUG_ERROR( "CAvAppMaster::Aa_get_vi_config_set invalid resolution(%d)\n", m_pMain_stream_encoder->stuVideoEncodeSetting[phy_video_chn_num].ucResolution);
                //_AV_FATAL_(1, "CAvAppMaster::Aa_get_vi_config_set invalid resolution(%d)\n", m_pMain_stream_encoder->encode[phy_video_chn_num].ucResolution);
                break;
        } 
#if defined(_AV_PLATFORM_HI3516A_)
        IspOutputMode_e iom;
        //RM_ISP_API_GetOutputMode(iom);
        iom = pgAvDeviceInfo->Adi_get_isp_output_mode();
        vi_config.frame_rate = Aa_convert_framerate_from_IOM(iom);
#else
        if(m_av_tvsystem == _AT_PAL_)
        {
            vi_config.frame_rate = 25;
        }
        else
        {
            vi_config.frame_rate = 30;
        }
#endif
    }
    pgAvDeviceInfo->Adi_get_video_size(vi_config.resolution, m_av_tvsystem, &vi_config.w, &vi_config.h);
#if  defined(_AV_PRODUCT_CLASS_IPC_) 
    vi_config.u8Rotate = (m_pstuCameraAttrParam ? m_pstuCameraAttrParam->stuViParam[0].image.u8Rotate : 0);
    vi_config.u8Flip = (m_pstuCameraAttrParam ? m_pstuCameraAttrParam->stuViParam[0].image.u8Flip: 0);
    vi_config.u8Mirror= (m_pstuCameraAttrParam ? m_pstuCameraAttrParam->stuViParam[0].image.u8Mirror : 0);
#endif
    return 0;
}

int CAvAppMaster::Aa_start_vi()
{
    
    DEBUG_MESSAGE( "CAvAppMaster::Aa_start_vi...m_start_work_flag %d\n", m_start_work_flag);
    for(int i = 0; i < pgAvDeviceInfo->Adi_get_vi_chn_num(); i ++)
    {
        Aa_get_vi_config_set(i, m_vi_config_set[i], NULL);
    }

    if(m_pAv_device_master->Ad_start_vi(m_av_tvsystem, m_vi_config_set) != 0)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_start_work m_pAv_device_master->Ad_start_vi FAIELD\n");
        return -1;
    }

#if !defined(_AV_PRODUCT_CLASS_IPC_)
    Aa_boardcast_vi_resolution();
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_DEVICE_MANAGER_GET_VLOSS_STATE, 1, NULL, 0, 0);
#endif
    DEBUG_MESSAGE( "CAvAppMaster::Aa_start_vi...m_start_work_flag %d end\n", m_start_work_flag);

    return 0;
}

int CAvAppMaster::Aa_start_selected_vi(unsigned long long int chn_mask)
{
	if(chn_mask != 0x0)
	{
	    DEBUG_ERROR( "CAvAppMaster::Aa_start_selected_vi...m_start_work_flag = %d, chn_mask = 0x%llx\n", m_start_work_flag, chn_mask);
	    for(int i = 0; i < pgAvDeviceInfo->Adi_get_vi_chn_num(); i ++)
	    {
	        if(0 != ((chn_mask) & (1ll<<i)))
	        {
	            Aa_get_vi_config_set(i, m_vi_config_set[i], NULL);
	        }
	    }

	    if(m_pAv_device_master->Ad_start_selected_vi(m_av_tvsystem, chn_mask, m_vi_config_set) != 0)
	    {
	        DEBUG_ERROR( "CAvAppMaster::Aa_start_work m_pAv_device_master->Aa_start_selected_vi FAIELD\n");
	        return -1;
	    }

#if !defined(_AV_PRODUCT_CLASS_IPC_)
		Aa_boardcast_vi_resolution();
		Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_DEVICE_MANAGER_GET_VLOSS_STATE, 1, NULL, 0, 0);
#endif
	    DEBUG_MESSAGE( "CAvAppMaster::Aa_start_selected_vi...m_start_work_flag %d end\n", m_start_work_flag);
	}

    return 0;
}

void CAvAppMaster::Aa_release_vi()
{
#if defined(_AV_HAVE_VIDEO_DECODER_) || defined(_AV_HAVE_VIDEO_OUTPUT_)
    bool reset_preview = true;
#endif

#if defined(_AV_HAVE_VIDEO_DECODER_)
    if (m_playState != PS_INVALID)
    {/*处于回放状态不重置preview*/
        reset_preview = false;
    }
#endif
    DEBUG_MESSAGE( "CAvAppMaster::Aa_release_vi_resource\n");
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
    if (_AS_PIP_ == Aa_get_split_mode(m_pScreenMosaic->iMode))
    {
        av_pip_para_t av_pip_para;
        memset(&av_pip_para, 0, sizeof(av_pip_para));
        av_pip_para.enable = false;
        m_pAv_device_master->Ad_set_pip_screen(av_pip_para);
    }
    
    /*stop vo preview*/
    if (true == reset_preview)
    {
        m_pAv_device_master->Ad_stop_preview_output( pgAvDeviceInfo->Adi_get_main_out_mode(), 0 );
    }
    
    if (m_pSpot_output_mosaic != NULL)
    {
        Aa_spot_output_control(_SPOT_OUTPUT_CONTROL_stop_, m_pSpot_output_mosaic);
    }
#endif
    /*stop vi*/
    DEBUG_MESSAGE( "CAvAppMaster::Aa_release_vi_resource===aa stop vi\n");
    Aa_stop_vi();
    DEBUG_MESSAGE( "CAvAppMaster::Aa_release_vi_resource===bb stop vi\n");
    return;
}

void CAvAppMaster::Aa_release_selected_vi(unsigned long long int chn_mask, bool brelate_decoder)
{
#if defined(_AV_HAVE_VIDEO_DECODER_) || defined(_AV_HAVE_VIDEO_OUTPUT_)
    bool reset_preview = true;
#endif

#if defined(_AV_HAVE_VIDEO_DECODER_)
    if (m_playState != PS_INVALID)
    {/*处于回放状态不重置preview*/
        reset_preview = false;
    }
#endif
    DEBUG_MESSAGE( "CAvAppMaster::Aa_release_selected_vi_resource\n");
#if defined(_AV_HAVE_VIDEO_OUTPUT_)

    if (_AS_PIP_ == Aa_get_split_mode(m_pScreenMosaic->iMode))
    {
        av_pip_para_t av_pip_para;
        memset(&av_pip_para, 0, sizeof(av_pip_para));
        av_pip_para.enable = false;
        m_pAv_device_master->Ad_set_pip_screen(av_pip_para);
    }
    
    /*stop vo preview*/
    if (true == reset_preview)
    {
        m_pAv_device_master->Ad_stop_selected_preview_output( pgAvDeviceInfo->Adi_get_main_out_mode(), 0, chn_mask, brelate_decoder);
    }
    
    if (m_pSpot_output_mosaic != NULL)
    {
        Aa_spot_output_control(_SPOT_OUTPUT_CONTROL_stop_, m_pSpot_output_mosaic);
    }
    
#endif
    /*stop vi*/
    DEBUG_MESSAGE( "CAvAppMaster::Aa_release_vi_resource===aa stop vi\n");

    Aa_stop_selected_vi(chn_mask);
    
    DEBUG_MESSAGE( "CAvAppMaster::Aa_release_vi_resource===bb stop vi\n");
    
    return;
}

void CAvAppMaster::Aa_restart_vi()
{
#if defined(_AV_HAVE_VIDEO_DECODER_) || defined(_AV_HAVE_VIDEO_OUTPUT_)
    bool reset_preview = true;
#endif

#if defined(_AV_HAVE_VIDEO_DECODER_)
    if (m_playState != PS_INVALID)
    {/*处于回放状态不重置preview*/
        reset_preview = false;
    }
#endif
    DEBUG_MESSAGE( "CAvAppMaster::Aa_restart_vi\n");
    /*start vi*/
    Aa_start_vi();

    //this->Aa_cover_area( m_pCover_area_para, m_pCover_area_para );
    
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
    /*start vo preview*/   
    if (true == reset_preview)
    {
        this->Aa_GotoPreviewOutput(m_pScreenMosaic);
        if (_AS_PIP_ == Aa_get_split_mode(m_pScreenMosaic->iMode))
        {
            m_av_pip_para.enable = true;
            m_pAv_device_master->Ad_set_pip_screen(m_av_pip_para);
        }
    }

    if (m_pSpot_output_mosaic != NULL)
    {
        Aa_spot_output_control(_SPOT_OUTPUT_CONTROL_output_, m_pSpot_output_mosaic);
    }
#endif
    return;
}

void CAvAppMaster::Aa_restart_selected_vi(unsigned long long int chn_mask, bool brelate_decoder)
{
#if defined(_AV_HAVE_VIDEO_DECODER_) || defined(_AV_HAVE_VIDEO_OUTPUT_)
    bool reset_preview = true;
#endif

#if defined(_AV_HAVE_VIDEO_DECODER_)
    if (m_playState != PS_INVALID)
    {/*处于回放状态不重置preview*/
        reset_preview = false;
    }
#endif
    DEBUG_MESSAGE( "CAvAppMaster::Aa_restart_selected_vi\n");
    /*start vi*/
    Aa_start_selected_vi(chn_mask);
        
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
    /*start vo preview*/   
    if (true == reset_preview)
    {
        this->Aa_GotoSelectedPreviewOutput(m_pScreenMosaic, chn_mask, brelate_decoder);
        if (_AS_PIP_ == Aa_get_split_mode(m_pScreenMosaic->iMode))
        {
            m_av_pip_para.enable = true;
            m_pAv_device_master->Ad_set_pip_screen(m_av_pip_para);
        }
    }
    
    if (m_pSpot_output_mosaic != NULL)
    {
        Aa_spot_output_control(_SPOT_OUTPUT_CONTROL_output_, m_pSpot_output_mosaic);
    }   
#endif
    return;
}

int CAvAppMaster::Aa_stop_selected_vi_for_analog_to_digital(unsigned long long int chn_mask)
{
    if(0 == chn_mask)
    {
        DEBUG_MESSAGE( "CAvAppMaster::Aa_stop_selected_vi_for_analog_to_digital(chn_mask == 0)\n");
        return 0;
    }
#if defined(_AV_HAVE_VIDEO_DECODER_) || defined(_AV_HAVE_VIDEO_OUTPUT_)
    bool reset_preview = true;
#endif

#if defined(_AV_HAVE_VIDEO_DECODER_)
    if (m_playState != PS_INVALID)
    {
         reset_preview = false;
    }
#endif

    Aa_release_selected_vi(chn_mask);

    for(int chn_index = 0; chn_index < pgAvDeviceInfo->Adi_get_vi_chn_num(); chn_index++)
    {
        if(GCL_BIT_VAL_TEST(chn_mask, chn_index))
        {
            pgAvDeviceInfo->Adi_set_video_source(chn_index, _VS_DIGITAL_);
            DEBUG_MESSAGE("++chn_index = %d is digital+++\n",chn_index );
        }
    }


#if defined(_AV_HAVE_VIDEO_OUTPUT_)
    if (true == reset_preview)
    {
        this->Aa_GotoSelectedPreviewOutput(m_pScreenMosaic, chn_mask, true);
        if (_AS_PIP_ == Aa_get_split_mode(m_pScreenMosaic->iMode))
        {
            m_av_pip_para.enable = true;
            m_pAv_device_master->Ad_set_pip_screen(m_av_pip_para);
        }
    }
    
    if (m_pSpot_output_mosaic != NULL)
    {
        Aa_spot_output_control(_SPOT_OUTPUT_CONTROL_output_, m_pSpot_output_mosaic);
    }
#endif

    return 0;
}

int CAvAppMaster::Aa_start_selected_vi_for_digital_to_analog(unsigned long long int chn_mask)
{
    if (0 == chn_mask)
    {
        DEBUG_MESSAGE( "CAvAppMaster::Aa_start_selected_vi_for_digital_to_analog(chn_mask == 0)\n");
        return 0;
    }
#if defined(_AV_HAVE_VIDEO_DECODER_) || defined(_AV_HAVE_VIDEO_OUTPUT_)
    bool reset_preview = true;
#endif
#if defined(_AV_HAVE_VIDEO_DECODER_)
    if (m_playState != PS_INVALID)
    {
         reset_preview = false;
    }
#endif
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
    if (_AS_PIP_ == Aa_get_split_mode(m_pScreenMosaic->iMode))
    {
        av_pip_para_t av_pip_para;
        memset(&av_pip_para, 0, sizeof(av_pip_para));
        av_pip_para.enable = false;
        m_pAv_device_master->Ad_set_pip_screen(av_pip_para);
    }
    
    if (true == reset_preview)
    {
         m_pAv_device_master->Ad_stop_selected_preview_output( pgAvDeviceInfo->Adi_get_main_out_mode(), 0, chn_mask, true);
    }

    if (m_pSpot_output_mosaic != NULL)
    {
        Aa_spot_output_control(_SPOT_OUTPUT_CONTROL_stop_, m_pSpot_output_mosaic);
    }
#endif

    for(int chn_index = 0; chn_index < pgAvDeviceInfo->Adi_get_vi_chn_num(); chn_index++)
    {
        if(GCL_BIT_VAL_TEST(chn_mask, chn_index))
        {
            pgAvDeviceInfo->Adi_set_video_source(chn_index, _VS_ANALOG_);
        }
    }

    Aa_restart_selected_vi(chn_mask);

    return 0;
}


int CAvAppMaster::Aa_reset_vi()
{
#if defined(_AV_PRODUCT_CLASS_IPC_) && defined(_AV_HAVE_ISP_)
    unsigned char u8IrCut, u8Lamp, u8Iris, u8IspFrameRate;
    unsigned int u32PwmHz;
    m_pAv_device_master->Ad_GetIspControl(u8IrCut, u8Lamp, u8Iris, u8IspFrameRate, u32PwmHz);
#endif

    Aa_release_vi();

    Aa_restart_vi();
    
#if defined(_AV_PRODUCT_CLASS_IPC_) && defined(_AV_HAVE_ISP_)
    m_pAv_device_master->Ad_SetVideoColorGrayMode(u8IrCut);
#endif
    DEBUG_MESSAGE( "CAvAppMaster::Aa_reset_vi\n");
    return 0;
}

int CAvAppMaster::Aa_reset_selected_vi(unsigned long long int chn_mask, bool brelate_decoder)
{
    if(0 == chn_mask)
    {
        DEBUG_MESSAGE( "CAvAppMaster::Aa_reset_selected_vi(chn_mask == 0)\n");   
        return 0;
    }

#if defined(_AV_PRODUCT_CLASS_IPC_) && defined(_AV_HAVE_ISP_)
    unsigned char u8IrCut, u8Lamp, u8Iris, u8IspFrameRate;
    unsigned int u32PwmHz;
    m_pAv_device_master->Ad_GetIspControl(u8IrCut, u8Lamp, u8Iris, u8IspFrameRate, u32PwmHz);
#endif

    Aa_release_selected_vi(chn_mask, brelate_decoder);

    Aa_restart_selected_vi(chn_mask, brelate_decoder);

#if defined(_AV_PRODUCT_CLASS_IPC_) && defined(_AV_HAVE_ISP_)
    m_pAv_device_master->Ad_SetVideoColorGrayMode(u8IrCut);
#endif
    DEBUG_MESSAGE( "CAvAppMaster::Aa_reset_selected_vi end\n");

    return 0;
}

bool CAvAppMaster::Aa_weather_reset_vi(void *pVi_para /*= NULL*/)
{
    bool reset_vi = false;
    for(int i = 0; i < pgAvDeviceInfo->Adi_max_channel_number(); i ++)
    {
#if defined(_AV_PRODUCT_CLASS_HDVR_)
        if(pgAvDeviceInfo->Adi_get_video_source(i) == _VS_DIGITAL_)
        {
            DEBUG_MESSAGE( "CAvDevice::Aa_weather_reset_vi(chnnum = %d) is from digital\n", i);
            continue;
        }
#endif
        vi_config_set_t vi_config, vi_primary_config;
        Aa_get_vi_config_set(i, vi_config, pVi_para, &vi_primary_config);
        if(m_vi_config_set[i].resolution != vi_config.resolution)
        {
            reset_vi = true;
            break;
        }
    }
    
    return reset_vi;
}

#if !defined(_AV_PRODUCT_CLASS_IPC_)
int CAvAppMaster::Aa_boardcast_vi_resolution()
{
    msgVideoResolution_t video_resolution;

    memset(&video_resolution, 0, sizeof(msgVideoResolution_t));
    video_resolution.u16Status = 0;
    for(int chn = 0; chn < pgAvDeviceInfo->Adi_get_vi_chn_num(); chn ++)
    {
        if (pgAvDeviceInfo->Adi_get_video_source(chn) == _VS_DIGITAL_)
        {
            continue;
        }

        av_resolution_t av_res = m_vi_config_set[chn].resolution;
        
        video_resolution.u32ChnMask |= (0x01ull << chn);

        pgAvDeviceInfo->Adi_get_video_resolution((uint8_t *)&video_resolution.stuVideoRes[chn].u32ResValue, &av_res, false);
        switch(video_resolution.stuVideoRes[chn].u32ResValue)
        {
            case 0:/*CIF*/
            case 1:/*HD1*/    
            case 2:/*D1*/
            case 3:/*QCIF*/
                video_resolution.stuVideoRes[chn].u32ResValue = 2;
                video_resolution.stuVideoRes[chn].u32Width = pgAvDeviceInfo->Adi_get_D1_width();
                video_resolution.stuVideoRes[chn].u32Height = (m_av_tvsystem == _AT_PAL_)?576:480;
                break;
            case 6:/*720p*/
                video_resolution.stuVideoRes[chn].u32ResValue = 6;
                video_resolution.stuVideoRes[chn].u32Width = 1280;
                video_resolution.stuVideoRes[chn].u32Height = 720;
                break;
            case 7:/*1080p*/
                video_resolution.stuVideoRes[chn].u32ResValue = 7;
                video_resolution.stuVideoRes[chn].u32Width = 1920;
                video_resolution.stuVideoRes[chn].u32Height = 1080;
                break;
            case 9:/*5M*/
                video_resolution.stuVideoRes[chn].u32ResValue = 9;
                video_resolution.stuVideoRes[chn].u32Width = 2592;
                video_resolution.stuVideoRes[chn].u32Height = 1920;
                break;                
            case 10:/*WQCIF*/
            case 11:/*WCIF*/
            case 12:/*WHD1*/
            case 13:/*WD1*/
                video_resolution.stuVideoRes[chn].u32ResValue = 13;
                video_resolution.stuVideoRes[chn].u32Width = pgAvDeviceInfo->Adi_get_WD1_width();
                video_resolution.stuVideoRes[chn].u32Height = (m_av_tvsystem == _AT_PAL_)?576:480;
                break;
            case 14:
                video_resolution.stuVideoRes[chn].u32ResValue = 14;
                video_resolution.stuVideoRes[chn].u32Width = 1280;
                video_resolution.stuVideoRes[chn].u32Height = 960;
                break;
            case 16:
                video_resolution.stuVideoRes[chn].u32ResValue = 16;
                video_resolution.stuVideoRes[chn].u32Width = 800;
                video_resolution.stuVideoRes[chn].u32Height = 600;
                break;                
            case 17:
                video_resolution.stuVideoRes[chn].u32ResValue = 17;
                video_resolution.stuVideoRes[chn].u32Width = 1024;
                video_resolution.stuVideoRes[chn].u32Height = 768;
                break;

            default:
                _AV_FATAL_(1, "CAvAppMaster::Aa_boardcast_vi_resolution invalid resolution(%d)\n", m_pMain_stream_encoder->stuVideoEncodeSetting[chn].ucResolution);
                break;
        }
    }

    return Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_AVSTREAM_SET_VIDEO_RESOLUTION, 0, (char *)&video_resolution, sizeof(msgVideoResolution_t), 0);
}
#endif

#if !defined(_AV_PLATFORM_HI3518EV200_)
int CAvAppMaster::Aa_start_vda(vda_type_e vda_type, int phy_video_chn/*=-1*/, const vda_chn_param_t* pVda_param/*=NULL*/)
{
    
    
    if(!(((_VDA_MD_ == vda_type) &&(0 != m_pMotion_detect_para->ucEnable)) || \
        ((_VDA_OD_ == vda_type)&&(0 != m_pVideo_shield_para->ucEnable))))
    {
        return 0;
    }

#if defined(_AV_PRODUCT_CLASS_IPC_)
    if(Aa_is_customer_cnr())
    {
        if(vda_type == _VDA_OD_)
        {
            for(int i=0;i<pgAvDeviceInfo->Adi_get_vi_chn_num();++i)
            {
                m_pAv_device_master->Ad_start_ive_od(i, (unsigned char)m_pVideo_shield_para->stuVideoShield[i].ucSensitivity);
            }

            return 0;
        }
    }
#endif
    
    int ret_val = 0;
    vda_chn_param_t *pVda_chn_param = NULL;
    
    if (phy_video_chn != -1 && pVda_param != NULL)
    {
        _AV_FATAL_((pVda_chn_param = new vda_chn_param_t) == NULL, "OUT of memory!!\n");
        memcpy(pVda_chn_param, pVda_param, sizeof(vda_chn_param_t));
    }
    else
    {
        const int max_vi_chn_num = pgAvDeviceInfo->Adi_get_vi_chn_num();
        _AV_FATAL_((pVda_chn_param = new vda_chn_param_t[max_vi_chn_num]) == NULL, "OUT of memory!!\n");
        
        for (int chn_index = 0; chn_index != max_vi_chn_num; chn_index++)
        {
            if(_VDA_MD_ == vda_type)
            {
                if (GCL_BIT_VAL_TEST(m_pMotion_detect_para->u32MDChannel, chn_index))
                {
                    pVda_chn_param[chn_index].enable = 1;
                }
                else
                {
                    pVda_chn_param[chn_index].enable = 0;
                }
                pVda_chn_param[chn_index].sensitivity = m_pMotion_detect_para->stuMotionDetect[chn_index].ucSensitivity;
                memcpy(pVda_chn_param[chn_index].region, m_pMotion_detect_para->stuMotionDetect[chn_index].ucRegion, sizeof(pVda_chn_param[chn_index].region));            
            }
            else
            {
                if (GCL_BIT_VAL_TEST(m_pVideo_shield_para->u32VSChannel, chn_index))
                {
                    pVda_chn_param[chn_index].enable = 1;
                }
                else
                {
                    pVda_chn_param[chn_index].enable = 0;
                }   
                pVda_chn_param[chn_index].sensitivity = m_pVideo_shield_para->stuVideoShield[chn_index].ucSensitivity;
            }
        }
    }
    
    if (m_pAv_device_master->Ad_start_vda(vda_type, m_av_tvsystem, phy_video_chn, pVda_chn_param) != 0)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_start_vda  Ad_start_vda (phy_video_chn:%d) FAILED!!! \n", phy_video_chn);
        ret_val = -1;
    }

    if (phy_video_chn != -1 && pVda_param != NULL)
    {
        _AV_SAFE_DELETE_(pVda_chn_param);
    }
    else
    {
        _AV_SAFE_DELETE_ARRAY_(pVda_chn_param);
    }

    return ret_val;
}

int CAvAppMaster::Aa_start_selected_vda(vda_type_e vda_type, unsigned long long int start_chn_mask)
{
    if (0 == m_pMotion_detect_para->ucEnable)
    {
        return 0;
    }

    vda_chn_param_t Vda_chn_param;
    int max_vi_chn = pgAvDeviceInfo->Adi_get_vi_chn_num();
    for (int chn_index = 0; chn_index < max_vi_chn; chn_index++)
    {
        if (GCL_BIT_VAL_TEST(start_chn_mask, chn_index))
        {
            if (GCL_BIT_VAL_TEST(m_pMotion_detect_para->u32MDChannel, chn_index))
            {
                Vda_chn_param.enable = 1;
                Vda_chn_param.sensitivity = m_pMotion_detect_para->stuMotionDetect[chn_index].ucSensitivity;
                memcpy(Vda_chn_param.region, m_pMotion_detect_para->stuMotionDetect[chn_index].ucRegion, sizeof(Vda_chn_param.region));
                if (m_pAv_device_master->Ad_start_selected_vda(vda_type, m_av_tvsystem, chn_index, &Vda_chn_param) != 0)
                {
                    DEBUG_ERROR( "CAvAppMaster::Aa_start_vda  Ad_start_selected_vda (phy_video_chn:%d) FAILED!!! \n", chn_index);
                    return -1;
                }
            }
        }
    }
    
    return 0;  
}


int CAvAppMaster::Aa_stop_vda(vda_type_e vda_type, int phy_video_chn/*=-1*/)
{
#if defined(_AV_PRODUCT_CLASS_IPC_)
    if(Aa_is_customer_cnr())
    {
        if(vda_type == _VDA_OD_)
        {
            for(int i=0;i<pgAvDeviceInfo->Adi_get_vi_chn_num();++i)
            {
                m_pAv_device_master->Ad_stop_ive_od(i);
            }

            return 0;
        }
    }
#endif


    if (m_pAv_device_master->Ad_stop_vda(vda_type, phy_video_chn) != 0)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_stop_vda  Ad_stop_vda (phy_video_chn:%d) FAILED!!! \n", phy_video_chn);
        return -1;
    }

    return 0;
}

int CAvAppMaster::Aa_stop_selected_vda(vda_type_e vda_type, unsigned long long int stop_chn_mask)
{
    int max_vi_chn = pgAvDeviceInfo->Adi_get_vi_chn_num();
    for (int chn_index = 0; chn_index != max_vi_chn; chn_index++)
    {
        if (GCL_BIT_VAL_TEST(stop_chn_mask, chn_index))
        {
            if (m_pAv_device_master->Ad_stop_selected_vda(vda_type, chn_index) != 0)
            {
                DEBUG_ERROR( "CAvAppMaster::Ad_stop_selected_vda (chn_index:%d) FAILED!!! \n", chn_index);
                return -1;
            }
        }
    }
    
    return 0;
}


int CAvAppMaster::Aa_get_vda_result(vda_type_e vda_type, unsigned int *pVda_result, int phy_video_chn/*=-1*/, unsigned char *pVda_region/*=NULL*/)
{
    if (vda_type == _VDA_MD_)
    {
        m_pAv_device_master->Ad_get_vda_result(vda_type, pVda_result, phy_video_chn, pVda_region);
    }
    else if (vda_type == _VDA_OD_)
    {
    }

    return 0;
}
#endif
#endif

#if defined(_AV_TEST_MAINBOARD_)
int CAvAppMaster::Aa_test_audio_IO()
{
    if (pgAvDeviceInfo->Adi_app_type() == APP_TYPE_TEST)
    {
        //return m_pAv_device_master->Ad_test_audio_IO();
    }
    
    return 0;
}
#endif

int CAvAppMaster::Aa_everything_is_ready()
{
    std::map<enumSystemMessage, bool>::iterator env_it = m_start_work_env.begin();

    while(env_it != m_start_work_env.end())
    {
        if(env_it->second == false)
        {
            DEBUG_ERROR("not recivecmd=0x%x\n", env_it->first);
        
            Common::N9M_MSGSendMessage(m_message_client_handle, 0, env_it->first, 1, NULL, 0, 111);
            return 1;
        }

        env_it ++;
    }

    return 0;
}


int CAvAppMaster::Aa_start_work()
{
    eProductType product_type = PTD_INVALID;
    /*device type*/
    N9M_GetProductType(product_type);
    /*Everything is ready???*/
    if(Aa_everything_is_ready() != 0)
    {
        //DEBUG_ERROR( "CAvAppMaster::Aa_start_work NOT READY!!!!\n");
        return -1;
    }
    
    /******************all ready, now start work******************/
    /*get sensor type*/
    if(product_type != PTD_T5 && product_type != PTD_6A_II_AV3 && product_type != PTD_T6)
    {
    /*start audio input*/
#if defined(_AV_HAVE_AUDIO_INPUT_)
        Aa_start_ai();
#endif
    }
#if defined(_AV_PLATFORM_HI3516A_) || defined(_AV_PLATFORM_HI3518EV200_)
    Aa_outer_codec_fm1288_init(pgAvDeviceInfo->Adi_product_type());
    usleep(200*1000);
#endif
    /*get sensor type*/
#if defined(_AV_PRODUCT_CLASS_IPC_)
#if defined(_AV_PLATFORM_HI3518C_) 
    if(PTD_913_VA == pgAvDeviceInfo->Adi_product_type())
    {
        pgAvDeviceInfo->Adi_set_sensor_type(ST_SENSOR_IMX222);
    }
    else
    {
        sensor_e sensor_type = Aa_sensor_get_type();
        pgAvDeviceInfo->Adi_set_sensor_type(sensor_type);            
    }
#elif defined(_AV_PLATFORM_HI3518EV200_)
    if(PTD_718_VA == pgAvDeviceInfo->Adi_product_type())
    {
        pgAvDeviceInfo->Adi_set_sensor_type(ST_SENSOR_OV9732);
    }
    else if(PTD_913_VB == pgAvDeviceInfo->Adi_product_type())
    {
        pgAvDeviceInfo->Adi_set_sensor_type(ST_SENSOR_IMX222);
    }
#else
    if(PTD_916_VA == pgAvDeviceInfo->Adi_product_type())
    {
        pgAvDeviceInfo->Adi_set_sensor_type(ST_SENSOR_IMX290);
    }
    else if(PTD_920_VA == pgAvDeviceInfo->Adi_product_type())
    {
        pgAvDeviceInfo->Adi_set_sensor_type(ST_SENSOR_IMX222);
    }
#endif
#endif
    DEBUG_MESSAGE( "CAvAppMaster::Aa_start_work...\n");

    /*start video input*/
#if defined(_AV_HAVE_VIDEO_INPUT_) 
    DEBUG_MESSAGE( "CAvAppMaster::Aa_start_vi...\n");   

#ifdef _AV_HAVE_ISP_
#if defined(_AV_PLATFORM_HI3518C_)
    RM_ISP_API_Init(pgAvDeviceInfo->Adi_get_sensor_type(), pgAvDeviceInfo->Adi_get_focus_length());
#else
    ISPInitParam_t isp_param;

    memset(&isp_param, 0, sizeof(ISPInitParam_t));
    isp_param.sensor_type = pgAvDeviceInfo->Adi_get_sensor_type();
    isp_param.focus_len = pgAvDeviceInfo->Adi_get_focus_length();
    isp_param.u8WDR_Mode = WDR_MODE_NONE;
    isp_param.etOptResolution = pgAvDeviceInfo->Adi_get_isp_output_mode();
    isp_param.u8FstInit_flag = 0;
    _AV_KEY_INFO_(_AVD_APP_, "isp init, sensor type:%d focus len:%d wdr mode:%d resolution:%d \n", isp_param.sensor_type, isp_param.focus_len, isp_param.u8WDR_Mode, isp_param.etOptResolution);
    RM_ISP_API_Init(&isp_param);
#endif
#if defined(_AV_PRODUCT_CLASS_IPC_)
    if(PTD_712_VD == pgAvDeviceInfo->Adi_product_type())
    {
        _AV_KEY_INFO_(_AVD_APP_, "IPC work mode:%d \n", pgAvDeviceInfo->Adi_get_ipc_work_mode());
        RM_ISP_API_SetIpcWorkMode(pgAvDeviceInfo->Adi_get_ipc_work_mode());
    }
#endif
    if( RM_ISP_API_InitSensor() != 0 )
    {
        DEBUG_MESSAGE( "CAvAppMaster::Aa_start_work RM_ISP_API_InitSensor FAIELD\n");
    }
    if( RM_ISP_API_IspStart() != 0 )
    {
        DEBUG_MESSAGE( "CAvAppMaster::Aa_start_work RM_ISP_API_IspStart FAIELD\n");
    }
#endif
#if 0
    //!3515 first set resolution to D1, second recover the resolution to param_setting
    unsigned char res_back[SUPPORT_MAX_VIDEO_CHANNEL_NUM] = {0};
    if((PTD_A5_II == product_type)||(product_type == PTD_6A_I))
    {   
        for (int chn_index = 0; chn_index < pgAvDeviceInfo->Adi_max_channel_number(); chn_index++)
        {
            res_back[chn_index] = m_pMain_stream_encoder->stuVideoEncodeSetting[chn_index].ucResolution;
            m_pMain_stream_encoder->stuVideoEncodeSetting[chn_index].ucResolution = 2;
        }
    }
#endif    
    Aa_start_vi();

    int chn_index;
    unsigned long long int vi_mask = 0;
    unsigned long long int vi_mask_stop = 0;
    unsigned long long int vi_digit_mask = 0;
    for (chn_index = 0; chn_index < pgAvDeviceInfo->Adi_get_vi_chn_num(); chn_index++)
    {
        //if (GCL_BIT_VAL_TEST(m_pMain_stream_encoder->mask, chn_index))
        {
            if (pgAvDeviceInfo->Adi_get_video_source(chn_index) == _VS_DIGITAL_)
            {
                if (m_pMain_stream_encoder->stuVideoEncodeSetting[chn_index].ucChnEnable == 0)
                { 
                    GCL_BIT_VAL_SET(vi_digit_mask, chn_index);
                }
                continue;
            }
            if (m_pMain_stream_encoder->stuVideoEncodeSetting[chn_index].ucChnEnable == 0)
            {
                GCL_BIT_VAL_SET(vi_mask_stop, chn_index);
                //GCL_BIT_VAL_SET(m_alarm_status.videoLost, chn_index);
                
            }
            else
            {
                GCL_BIT_VAL_SET(vi_mask, chn_index);
            }
        }
    }
   // m_pAv_device_master->Ad_vi_insert_picture(m_alarm_status.videoLost);

    m_pAv_device_master->Ad_stop_selected_vi(vi_mask_stop);
    // Aa_cover_area(m_pCover_area_para, NULL);
#if !defined(_AV_PLATFORM_HI3518EV200_)
    Aa_start_vda(_VDA_MD_);
#if defined(_AV_PRODUCT_CLASS_IPC_)
    Aa_start_vda(_VDA_OD_);

#else
    Aa_start_vda(_VDA_OD_, -1);
#endif
#endif

#endif

    /*start audio output*/
#if defined(_AV_HAVE_AUDIO_OUTPUT_)
    //Aa_start_ao();
    
#endif

    /*start preview HDMI audio (ai->ao)*/
#if defined(_AV_HAVE_AUDIO_INPUT_) && defined(_AV_HAVE_AUDIO_OUTPUT_) && (defined(_AV_PRODUCT_CLASS_DVR_) || defined(_AV_PRODUCT_CLASS_HDVR_))
    Aa_start_preview_audio();
#endif

    /*start output and preview*/
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
    int chn_layout[_AV_SPLIT_MAX_CHN_];
    int max_chn_num = pgAvDeviceInfo->Adi_max_channel_number();
    av_split_t split_mode = Aa_get_split_mode(m_pScreenMosaic->iMode);

    if (split_mode == _AS_UNKNOWN_)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_start_work Aa_get_split_mode FAILED! (split_mode:%d)\n", split_mode);
        split_mode = _AS_4SPLIT_;
        memset(chn_layout, -1, sizeof(int) * _AV_SPLIT_MAX_CHN_);
        for(int index = 0; index < 4; index ++)
        {
            chn_layout[index] = index;
        }
    }
    else
    {
        memset(chn_layout, -1, sizeof(int) * _AV_SPLIT_MAX_CHN_);
        for(int index = 0; index < max_chn_num; index ++)
        {
            if((m_pScreenMosaic->chn[index] >= 0) && (m_pScreenMosaic->chn[index] <= pgAvDeviceInfo->Adi_max_channel_number()))
            {
                chn_layout[index] = m_pScreenMosaic->chn[index];
            }
        }
    }
    /*clear logo*/
    m_pAv_device_master->Ad_clear_init_logo();

    /*Main output*/
    /*vga&hdmi, cvbs(write-back)*/
    /*start output*/
    if(Aa_start_output(pgAvDeviceInfo->Adi_get_main_out_mode(), 0, m_av_tvsystem, Aa_get_vga_hdmi_resolution(m_pSystem_setting->stuGeneralSetting.ucVgaResolution)) != 0)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_start_work m_pAv_device_master->Ad_start_output FAIELD\n");
        return -1;
    }
    
    /*start preview*/
    int margin[4];
    margin[_AV_MARGIN_BOTTOM_INDEX_] = m_pScreen_Margin->usBottom;
    margin[_AV_MARGIN_LEFT_INDEX_] = m_pScreen_Margin->usLeft;
    margin[_AV_MARGIN_RIGHT_INDEX_] = m_pScreen_Margin->usRight;
    margin[_AV_MARGIN_TOP_INDEX_] = m_pScreen_Margin->usTop;
    if(m_pAv_device_master->Ad_start_preview_output(pgAvDeviceInfo->Adi_get_main_out_mode(), 0, split_mode, chn_layout, margin) != 0)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_start_work m_pAv_device_master->Ad_start_preview_output FAIELD\n");
        return -1;
    }
#endif

#if 0
#if defined(_AV_HAVE_VIDEO_INPUT_)
//! for 3515 to avoid field missing
    if((PTD_A5_II == product_type)||(product_type == PTD_6A_I))
    {
        for (int chn_index = 0; chn_index < pgAvDeviceInfo->Adi_max_channel_number(); chn_index++)
        {
            m_pMain_stream_encoder->stuVideoEncodeSetting[chn_index].ucResolution = res_back[chn_index];
        }
        Aa_stop_vi();
        Aa_start_vi();  
    }
#endif
#endif

#if defined(_AV_HAVE_VIDEO_OUTPUT_)
#if !defined(_AV_PRODUCT_CLASS_IPC_)&& (!defined(_AV_PRODUCT_CLASS_NVR_))
    char ChannelMap[SUPPORT_MAX_VIDEO_CHANNEL_NUM];
    m_pAv_device_master->Ad_ObtainChannelMap(ChannelMap);

    for (int chn_index = 0; chn_index < pgAvDeviceInfo->Adi_max_channel_number(); chn_index++)    
    {

        {
            if (pgAvDeviceInfo->Adi_get_video_source(chn_index) == _VS_DIGITAL_)
            {
                if (m_chn_state[chn_index] == 0)
                {
                    ChannelMap[chn_index] = (char)-1;
                }
                else
                {
                    if (( chn_index >= pgAvDeviceInfo->Adi_get_vi_chn_num())&& (ChannelMap[chn_index] == 0xff))
                    {
                        
                        ChannelMap[chn_index] = (char)-1; //!
                    }
                    else
                    {
                        ChannelMap[chn_index] = chn_index;
                    }
                }
                continue;
            }

        }
    }
    
    m_pAv_device_master->Ad_UpdateDevMap(ChannelMap);
#endif

    /*Spot output*/
    if(m_pSpot_output_mosaic != NULL)
    {
        Aa_spot_output_control(_SPOT_OUTPUT_CONTROL_output_, m_pSpot_output_mosaic);
    }
#endif

    /*start encoder*/
#if defined(_AV_HAVE_VIDEO_ENCODER_)
    unsigned char streamtype_mask = Aa_get_all_streamtype_mask();
    //Aa_start_selected_streamtype_encoders(pgAvDeviceInfo->Aa_get_all_analog_chn_mask(), streamtype_mask);
    Aa_start_selected_streamtype_encoders(vi_mask, streamtype_mask);
    m_pAv_device->Ad_start_snap();
#endif

#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
#if defined(_AV_HAVE_VIDEO_OUTPUT_)
    if( m_pScreen_Margin )
    {
        m_pAv_device_master->Ad_set_vo_margin( pgAvDeviceInfo->Adi_get_main_out_mode(), 0, m_pScreen_Margin->usLeft, m_pScreen_Margin->usRight, m_pScreen_Margin->usTop, m_pScreen_Margin->usBottom);
    }
#endif
#endif

#if defined(_AV_TEST_MAINBOARD_)
    Aa_test_audio_IO(); // for test software talkback
#endif

#ifdef _AV_PRODUCT_CLASS_IPC_
#if defined(_AV_HAVE_ISP_)
    if( m_pstuCameraColorParam )
    {
        m_pAv_device_master->Ad_SetCameraColor( &(m_pstuCameraColorParam->stuVideoImageSetting[0]) );
    }
    if( m_pstuCameraAttrParam )
    {
        m_pAv_device_master->Ad_SetCameraAttribute( m_pstuCameraAttrParam->stuViParam[0].uiParamMask, &(m_pstuCameraAttrParam->stuViParam[0].image) );
    }
#endif

#if defined(_AV_PLATFORM_HI3516A_) && defined(_AV_SUPPORT_IVE_)
    m_pAv_device_master->Ad_register_ive_detection_result_notify_callback(boost::bind(&CAvAppMaster::Aa_ive_detection_result_notify_func, this, _1));
    m_pAv_device_master->Ad_start_ive();
#endif
#endif

#if !defined(_AV_PRODUCT_CLASS_IPC_)
#if defined(_AV_HAVE_TTS_)
    m_pAv_device_master->Ad_init_tts();
#endif
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_)
#if defined(_AV_HAVE_ISP_)
    Aa_start_vi_interrput_monitor();
#endif
#endif
    /*running*/
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_AVSTREAM_START_COMPLETE, 0, NULL, 0, 0);
    m_start_work_flag = 1;
#if defined(_AV_PRODUCT_CLASS_IPC_)
        /*avStreaming start to get luma value */
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_DEVICE_MANAGER_ENVIRONMENT_LUMINANCE_GET, true, NULL, 0, 0);
#endif

    DEBUG_MESSAGE( "CAvAppMaster::Aa_start_work OKOKOK!!!\n");

    return 0;
}

int CAvAppMaster::Aa_stop_work()
{
    m_upgrade_flag = 1;

#if defined(_AV_PLATFORM_HI3516A_) && defined(_AV_SUPPORT_IVE_)
    m_pAv_device_master->Ad_stop_ive();
    m_pAv_device_master->Ad_unregister_ive_detection_result_notify_callback();
#endif

#if defined(_AV_HAVE_VIDEO_INPUT_)
#if !defined(_AV_PLATFORM_HI3518EV200_)
    Aa_stop_vda(_VDA_MD_);
    Aa_stop_vda(_VDA_OD_);
#endif
#endif

#if defined(_AV_HAVE_VIDEO_ENCODER_)
    m_pAv_device->Ad_stop_snap();
    unsigned char streamtype_mask = Aa_get_all_streamtype_mask();
    Aa_stop_selected_streamtype_encoders(pgAvDeviceInfo->Aa_get_all_analog_chn_mask(), streamtype_mask);
#endif

#if 0//defined(_AV_HAVE_VIDEO_DECODER_)
            DEBUG_MESSAGE( "Close Decoder...\n");
		m_pAv_device_master->Ad_stop_preview_Dec(_AOT_MAIN_,0);
 		m_pAv_device_master->Ad_stop_preview_Dec(_AOT_SPOT_,0);
#endif
//! Reget screenmode may cause  the inconsistency with UI
        //Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_SCREEN_MODE_PARAM_GET, 1, NULL, 0, 0);

#if defined(_AV_HAVE_VIDEO_OUTPUT_)
	m_pAv_device_master->Ad_stop_preview_output(pgAvDeviceInfo->Adi_get_main_out_mode(),0);

    /*Spot output*/
    if(m_pSpot_output_mosaic != NULL)
    {
        Aa_spot_output_control(_SPOT_OUTPUT_CONTROL_stop_, m_pSpot_output_mosaic);
    }
#endif

#if defined(_AV_HAVE_AUDIO_INPUT_) && defined(_AV_HAVE_AUDIO_OUTPUT_) && (defined(_AV_PRODUCT_CLASS_DVR_) || defined(_AV_PRODUCT_CLASS_HDVR_))
       m_pAv_device_master->Ad_preview_audio_ctrl(false);
#endif

#if 1 /*这个开关开放虽然不会死机，但是可能会影响其他功能*/
    eProductType product_type = PTD_INVALID;
    /*device type*/
    N9M_GetProductType(product_type);

/*stop audio output*/
#if defined(_AV_HAVE_AUDIO_OUTPUT_)
          DEBUG_MESSAGE( "Close AO...\n");
          this->Aa_stop_ao();    
#endif

     if(product_type != PTD_T5 && product_type != PTD_6A_II_AV3 && product_type != PTD_T6)	//都不需要对讲///
     {
    /*stop audio input*/
#if defined(_AV_HAVE_AUDIO_INPUT_)
            Aa_stop_ai();
#endif
    }

#if defined(_AV_HAVE_VIDEO_INPUT_)
#ifdef _AV_PLATFORM_HI3520D_V300_
    Aa_stop_selected_vi(~m_alarm_status.videoLost);
#endif
#if defined(_AV_PRODUCT_CLASS_IPC_)
#if defined(_AV_HAVE_ISP_)
    Aa_stop_vi_interrput_monitor();
#endif
#endif
    Aa_stop_vi();
#endif
#endif

#if 0
    printf("cxliu==========,time=%ld\n",time((time_t *)NULL));
    sleep(10);
    Aa_require_mem();
    printf("cxliu-----------,time=%ld\n",time((time_t *)NULL));
#endif
    return 0;

}

int CAvAppMaster::Aa_restart_work()
{
if(m_upgrade_flag==1)
{ 
    /******************all ready, now restart work******************/
    /*get sensor type*/
    DEBUG_MESSAGE( "CAvAppMaster::Aa_restart_work...\n");

    eProductType product_type = PTD_INVALID;
    /*device type*/
    N9M_GetProductType(product_type);
	
#if defined(_AV_HAVE_AUDIO_OUTPUT_)
            DEBUG_MESSAGE( "start AO...\n");
          this->Aa_start_ao();    
#endif

     if(product_type != PTD_T5 && product_type != PTD_6A_II_AV3 && product_type != PTD_T6)
     {
    /*stop audio input*/
#if defined(_AV_HAVE_AUDIO_INPUT_)
		Aa_start_ai();
#endif
	}
#if defined(_AV_HAVE_VIDEO_INPUT_)		
		Aa_start_vi();
#endif		
    /*start encoder*/
#if defined(_AV_HAVE_VIDEO_ENCODER_)
    unsigned char streamtype_mask = Aa_get_all_streamtype_mask();
    Aa_start_selected_streamtype_encoders(pgAvDeviceInfo->Aa_get_all_analog_chn_mask(), streamtype_mask);
#endif
    //! reget screenmode may cause  the inconsistency with UI
    //Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_CONFIG_MANAGE_SCREEN_MODE_PARAM_GET, 1, NULL, 0, 0);
//	m_pAv_device_master->Ad_RestartDecoder();

#if defined(_AV_HAVE_VIDEO_OUTPUT_)
    int chn_layout[_AV_SPLIT_MAX_CHN_];
    av_split_t split_mode = Aa_get_split_mode(m_pScreenMosaic->iMode);
    int max_chn_num = pgAvDeviceInfo->Adi_max_channel_number();
    int margin[4];

    if (split_mode == _AS_UNKNOWN_)
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_start_work Aa_get_split_mode FAILED! (split_mode:%d)\n", split_mode);
        split_mode = _AS_4SPLIT_;
        memset(chn_layout, -1, sizeof(int) * _AV_SPLIT_MAX_CHN_);
        for(int index = 0; index < 4; index ++)
        {
            chn_layout[index] = index;
        }
    }
    else
    {
        memset(chn_layout, -1, sizeof(int) * _AV_SPLIT_MAX_CHN_);
        for(int index = 0; index < max_chn_num; index ++)
        {
            if((m_pScreenMosaic->chn[index] >= 0) && (m_pScreenMosaic->chn[index] <= pgAvDeviceInfo->Adi_max_channel_number()))
            {
                chn_layout[index] = m_pScreenMosaic->chn[index];
            }
        }
    }

    margin[_AV_MARGIN_BOTTOM_INDEX_] = m_pScreen_Margin->usBottom;
    margin[_AV_MARGIN_LEFT_INDEX_] = m_pScreen_Margin->usLeft;
    margin[_AV_MARGIN_RIGHT_INDEX_] = m_pScreen_Margin->usRight;
    margin[_AV_MARGIN_TOP_INDEX_] = m_pScreen_Margin->usTop;
	
    m_pAv_device_master->Ad_start_preview_output( pgAvDeviceInfo->Adi_get_main_out_mode(), 0,split_mode,chn_layout,margin );

	/*Spot output*/
    if(m_pSpot_output_mosaic != NULL)
    {
        Aa_spot_output_control(_SPOT_OUTPUT_CONTROL_output_, m_pSpot_output_mosaic);
    }
#endif

#if defined(_AV_HAVE_AUDIO_INPUT_) && defined(_AV_HAVE_AUDIO_OUTPUT_) && (defined(_AV_PRODUCT_CLASS_DVR_) || defined(_AV_PRODUCT_CLASS_HDVR_))
		m_pAv_device_master->Ad_preview_audio_ctrl(true);
#endif

#if !defined(_AV_PLATFORM_HI3518EV200_)
#if defined(_AV_HAVE_VIDEO_INPUT_)
    Aa_start_vda(_VDA_MD_);
#if defined(_AV_PRODUCT_CLASS_IPC_)
    Aa_start_vda(_VDA_OD_);
#else
    Aa_start_vda(_VDA_OD_, -1);
#endif
#endif
#endif

#if defined(_AV_HAVE_VIDEO_ENCODER_)

    	 m_pAv_device->Ad_start_snap();
#endif
#if defined(_AV_PLATFORM_HI3516A_) && defined(_AV_SUPPORT_IVE_)
        m_pAv_device_master->Ad_start_ive();
#if defined(_AV_IVE_BLE_) || defined(_AV_IVE_LD_) || defined(_AV_IVE_HC_)
        m_pAv_device_master->Ad_register_ive_detection_result_notify_callback(boost::bind(&CAvAppMaster::Aa_ive_detection_result_notify_func, this, _1));
#endif
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_)
#if defined(_AV_HAVE_ISP_)
    Aa_start_vi_interrput_monitor();
#endif
#endif
    m_upgrade_flag = 0;

    DEBUG_MESSAGE( "CAvAppMaster::Aa_restart_work OKOKOK!!!\n");
}

    return 0;
}
int CAvAppMaster::Aa_message_thread()
{
    struct timeval tmLastSec;
    struct timeval tmNowSec;
    memset(&tmLastSec, 0, sizeof(tmLastSec));
    memset(&tmNowSec, 0, sizeof(tmNowSec));
#if defined(_AV_PRODUCT_CLASS_HDVR_) || defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_)
    struct timeval tmLastSyncSec;
    memset(&tmLastSyncSec, 0, sizeof(tmLastSyncSec));
#endif
#if defined(_AV_PLATFORM_HI3515_)
	struct timeval tmLastCheckAudioFaultSec;
	memset(&tmLastCheckAudioFaultSec, 0, sizeof(tmLastCheckAudioFaultSec));
#endif

    while(1)
    {
        Common::N9M_MSGWaitForEvent(m_message_client_handle, 400);
        if(m_start_work_flag == 0)
        {
            Aa_start_work();
//! Added for IPC transplant, Nov 2014
#ifdef _AV_PRODUCT_CLASS_IPC_
#if defined(_AV_HAVE_ISP_)
#if defined(_AV_PLATFORM_HI3518C_)
            if(PTD_913_VA != pgAvDeviceInfo->Adi_product_type())
            {
                if(NULL != m_pMain_stream_encoder)
                {
                    Aa_set_isp_framerate(m_av_tvsystem, m_pMain_stream_encoder->stuVideoEncodeSetting[0].ucFrameRate);
                } 
            }
#endif
#endif
#endif
            Common::N9M_MSGSendMessage(m_message_client_handle, 0, EVENT_SERVER_WATCHDOG_CLEAR, 0, (char *) &m_watchdog, sizeof(SWatchdogMsg), 0);
        }
        else
        {
            gettimeofday(&tmNowSec, NULL);
            if( (tmNowSec.tv_sec + tmNowSec.tv_usec / 1000000ULL) - (tmLastSec.tv_sec +  tmLastSec.tv_usec / 1000000ULL) > 0 )
            {
                Aa_check_playback_status();
                tmLastSec = tmNowSec;

                Common::N9M_MSGSendMessage(m_message_client_handle, 0, EVENT_SERVER_WATCHDOG_CLEAR, 0, (char *) &m_watchdog, sizeof(SWatchdogMsg), 0);
                
                Aa_check_status();
#if defined(_AV_HAVE_VIDEO_ENCODER_)
                //Aa_get_snap_result_message();
                Aa_check_net_stream_level();
#endif
#if !defined(_AV_PLATFORM_HI3518EV200_)
                Aa_send_av_alarm();
#endif
#ifdef _AV_PRODUCT_CLASS_IPC_
#if defined(_AV_HAVE_ISP_)
                Aa_IPC_check_adjust();
#endif
#endif

#if defined(_AV_PRODUCT_CLASS_HDVR_) || defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_)            
                if( (tmNowSec.tv_sec + tmNowSec.tv_usec / 1000000ULL) - (tmLastSyncSec.tv_sec +  tmLastSyncSec.tv_usec / 1000000ULL)  > 4ULL )
                {
                    tmLastSyncSec = tmNowSec;
                    AV_PTS::SetPtsToShareMemory();
                }
#endif
            }
#if defined(_AV_PLATFORM_HI3515_)
			if(PTD_6A_I == pgAvDeviceInfo->Adi_product_type())	//6AI检测音频故障///
			{
				if(GCL_ABS((tmNowSec.tv_sec + tmNowSec.tv_usec / 1000000ULL) - (tmLastCheckAudioFaultSec.tv_sec +  tmLastCheckAudioFaultSec.tv_usec / 1000000ULL)) >= 5)
				{
					tmLastCheckAudioFaultSec = tmNowSec;
					unsigned int loss = 0;
					Aa_6a1_check_audio_fault(1, &loss);
					Aa_6a1_broadcast_audio_fault(loss);
				}
			}
#endif
#if defined(_AV_HAVE_VIDEO_ENCODER_)
            Aa_get_snap_result_message();
#endif
#if !defined(_AV_PRODUCT_CLASS_IPC_)
			if(PTD_6A_I != pgAvDeviceInfo->Adi_product_type())	//6AI不需要报站///
			{
				Aa_SendReportEndResultMessage();
			}
#endif
#ifdef _AV_PLATFORM_HI3515_
            mSleep(50); //!< 2.23
#endif            
        }
    }

    return 0;
}

#if defined(_AV_HAVE_AUDIO_INPUT_)
int CAvAppMaster::Aa_start_ai()
{
#if ( defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) )
    return m_pAv_device->Ad_start_ai(_AI_TALKBACK_);
#else
#if defined(_AV_PRODUCT_CLASS_IPC_)
    if(pgAvDeviceInfo->Adi_get_audio_chn_number() > 0)
    {
        m_pAv_device->Ad_start_ai(_AI_NORMAL_);
        CHiAvDeviceMaster* av_device = dynamic_cast<CHiAvDeviceMaster* >(m_pAv_device);
        if(NULL != av_device)
        {
            av_device->Ad_set_macin_volum(m_pAudio_param->stuAudioSetting.u8AudioVoice);
        }
    }

    return 0;
#else
    return m_pAv_device->Ad_start_ai(_AI_NORMAL_);
#endif
#endif
}

int CAvAppMaster::Aa_stop_ai()
{
#if ( defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) )
    return m_pAv_device->Ad_stop_ai(_AI_TALKBACK_);
#else
    return m_pAv_device->Ad_stop_ai(_AI_NORMAL_);
#endif
}
#endif


#if defined(_AV_HAVE_AUDIO_OUTPUT_)
int CAvAppMaster::Aa_start_ao()
{
#if defined(_AV_PRODUCT_CLASS_IPC_)
    return m_pAv_device_master->Ad_start_ao(_AO_TALKBACK_);
#else
    return (m_pAv_device_master->Ad_start_ao(_AO_PLAYBACK_CVBS_) /*+ m_pAv_device_master->Ad_start_ao(_AO_HDMI_)*/);
#endif
}

int CAvAppMaster::Aa_stop_ao()
{
    return (m_pAv_device_master->Ad_stop_ao(_AO_PLAYBACK_CVBS_) /*+ m_pAv_device_master->Ad_stop_ao(_AO_HDMI_)*/);
}

#endif

#if defined(_AV_HAVE_VIDEO_DECODER_)
void CAvAppMaster::Aa_message_handle_START_PLAYBACK_AUDIO(const char* msg, int length )
{
	DEBUG_MESSAGE("\nmsg z39 start\n");
    DEBUG_CRITICAL("######### CAvAppMaster::Aa_message_handle_START_PLAYBACK_AUDIO\n");

    SMessageHead *head = (SMessageHead*) msg;
    if( (head->length + sizeof(SMessageHead)) == (unsigned int)length )
    {
        msgAudioPlayPara_t* pStationParam = (msgAudioPlayPara_t*)head->body;
        msgPlaybackAudio_t* pStationBody = (msgPlaybackAudio_t*)pStationParam->body;
        DEBUG_CRITICAL("----start playback uiAudioType =%d ucOperateType=%d\n", pStationParam->uiAudioType, pStationParam->ucOperateType);
        DEBUG_CRITICAL("----lababit =0x%x\n", pStationBody->lababit);
        m_pAv_device_master->Ad_start_ao(_AO_PLAYBACK_CVBS_);
		DEBUG_MESSAGE("\nmsg z39 001\n");		

        m_pAv_device_master->Ad_set_audio_mute(0);
		DEBUG_MESSAGE("\nmsg z39 002\n");		

    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_START_PLAYBACK_AUDIO invalid message length\n");
    }
		DEBUG_MESSAGE("\nmsg z39 end\n");
    return;
}
//停止回放音频
void CAvAppMaster::Aa_message_handle_STOP_PLAYBACK_AUDIO(const char* msg, int length )
{
	DEBUG_MESSAGE("\nmsg z40 start\n");
    DEBUG_CRITICAL("######### CAvAppMaster::Aa_message_handle_STOP_PLAYBACK_AUDIO\n");
	
    SMessageHead *head = (SMessageHead*) msg;
    if( (head->length + sizeof(SMessageHead)) == (unsigned int)length )
    {
        msgAudioPlayPara_t* pStationParam = (msgAudioPlayPara_t*)head->body;
        msgPlaybackAudio_t* pStationBody = (msgPlaybackAudio_t*)pStationParam->body;
        DEBUG_CRITICAL("----stop playback uiAudioType =%d ucOperateType=%d\n", pStationParam->uiAudioType, pStationParam->ucOperateType);
        DEBUG_CRITICAL("----lababit =0x%x\n", pStationBody->lababit);
        m_pAv_device_master->Ad_stop_ao(_AO_PLAYBACK_CVBS_);
		DEBUG_MESSAGE("\nmsg z40 001\n");		
        m_pAv_device_master->Ad_set_audio_mute(1);
		DEBUG_MESSAGE("\nmsg z40 002\n");
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_STOP_PLAYBACK_AUDIO invalid message length\n");
    }
		DEBUG_MESSAGE("\nmsg z40 end\n");
    return;
}

#endif

void CAvAppMaster::Aa_message_handle_AUDIO_OUTPUT(const char *msg, int length)
{
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgADAudioOutput_t * pAudioOutput = (msgADAudioOutput_t *)pMessage_head->body;
        DEBUG_CRITICAL( "CAvAppMaster::Aa_message_handle_AUDIO_OUTPUT(%d)(%d)\n", pAudioOutput->audioOutput, pAudioOutput->ucChn);
#if defined(_AV_HAVE_VIDEO_DECODER_)
        if (m_playState != PS_INVALID)
        {
            DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_AUDIO_OUTPUT(%d)(%d) is in playback,quit this message\n", pAudioOutput->audioOutput, pAudioOutput->ucChn);
            return;
        }
#endif

        if (pAudioOutput != NULL)
        {
            if (pAudioOutput->audioOutput > 1 || pAudioOutput->ucChn > (rm_uint8_t)pgAvDeviceInfo->Adi_max_channel_number())
            {
                DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_AUDIO_OUTPUT invalid message parameter (audioOupt:%d) (ucChn:%d)\n", pAudioOutput->audioOutput, pAudioOutput->ucChn);
                return;
            }
#if defined(_AV_HAVE_VIDEO_ENCODER_)

            if (m_pMain_stream_encoder != NULL)
            {
                
                //if ((m_pMain_stream_encoder->stuVideoEncodeSetting[pAudioOutput->ucChn].ucChnEnable == 0)
                if ( (m_chn_state[pAudioOutput->ucChn] == 0)
                    &&pAudioOutput->audioOutput == 0)//预览通道使能关闭了
                {
                    printf(" Set channel[%d]  preview audio closed\n", pAudioOutput->ucChn);
                    pAudioOutput->ucChn = (char)-1;
                     
                }
            }
#endif
                
            m_pAv_device_master->Ad_switch_audio_chn(pAudioOutput->audioOutput, pAudioOutput->ucChn);
        }
        else
        {
            DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_AUDIO_OUTPUT invalid message\n");
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_AUDIO_OUTPUT invalid message length\n");
    }

    return;
}

void CAvAppMaster::Aa_message_handle_AUDIO_MUTE(const char *msg, int length)
{
    SMessageHead *pMessage_head = (SMessageHead*)msg;

    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgADAudioMute_t * pAudioMute = (msgADAudioMute_t *)pMessage_head->body;
        DEBUG_CRITICAL( "CAvAppMaster::Aa_message_handle_AUDIO_MUTE(%d)\n", pAudioMute->isAudioMute);

        if (pAudioMute != NULL && !m_talkback_start)
        {
            m_pAv_device_master->Ad_set_audio_mute(pAudioMute->isAudioMute);
        }
        else
        {
            DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_AUDIO_MUTE invalid message\n");
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_AUDIO_MUTE invalid message length\n");
    }

    return;
}

void CAvAppMaster::Aa_message_handle_AUDIO_VOLUMN_SET(const char *msg, int length)
{
    SMessageHead *pMessage_head = (SMessageHead*)msg;

    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgADVolumn_t * pAudioVolume = (msgADVolumn_t *)pMessage_head->body;
        printf( "CAvAppMaster::Aa_message_handle_AUDIO_VOLUMN_SET(%d)\n", pAudioVolume->AudioVolumn);
        if (pAudioVolume != NULL)
        {
            m_pAv_device_master->Ad_set_audio_volume(pAudioVolume->AudioVolumn);
        }
        else
        {
            DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_AUDIO_VOLUMN_SET invalid message\n");
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_AUDIO_VOLUMN_SET invalid message length\n");
    }

    return;  
}

#if defined(_AV_HAVE_AUDIO_INPUT_) && defined(_AV_HAVE_AUDIO_OUTPUT_) && (defined(_AV_PRODUCT_CLASS_DVR_) || defined(_AV_PRODUCT_CLASS_HDVR_))
int CAvAppMaster::Aa_start_preview_audio()
{
    return m_pAv_device_master->Ad_start_preview_audio();
}
#endif

/*talkback*/
void CAvAppMaster::Aa_message_handle_START_TALKBACK(const char *msg, int length)
{
    if (m_talkback_start)
    {
        DEBUG_CRITICAL("talk back started already!\n");
        return;
    }
    //this->Aa_stop_ao();
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgAudioPlayPara_t* pAudioPlayParam = (msgAudioPlayPara_t*)pMessage_head->body;
        msgTalkbackAduio_t* pTalkbackBody = (msgTalkbackAduio_t*)pAudioPlayParam->body;
        DEBUG_CRITICAL("start talkback  uiAudioType =%d ucOperateType=%d\n", pAudioPlayParam->uiAudioType, pAudioPlayParam->ucOperateType);
        DEBUG_CRITICAL("volumn=%d lababit =%d iTalkHandle =%d\n", pTalkbackBody->volumn, pTalkbackBody->lababit, pTalkbackBody->iTalkHandle);
        if (pTalkbackBody != NULL)
        {
            msgReponseAudioNetPara_t response;
            memset(&response, 0, sizeof(msgReponseAudioNetPara_t));
#if defined(_AV_HAVE_AUDIO_OUTPUT_)
            m_pAv_device_master->Ad_start_ao(_AO_TALKBACK_);
#endif
            /* start talkback */
            if(m_talkback_start == false)
            {
                if (Aa_talkback_ctrl(true, pTalkbackBody) == 0)//此函数要修改
                {
                    m_pAv_device_master->Ad_set_audio_mute(0);
                    m_talkback_start = true;
                    
                    response.result = 0;
                    response.audioparam.channelTotal = 1;
                    response.audioparam.samplingFigure = AudioSamplingBitWidth_16;
                    response.audioparam.samplingRate = AudioSamplingRate_8000;
                    response.audioparam.audioFormat = AudioFormat_ADPCMA;
                    response.audioparam.audioSource = pTalkbackBody->stuAudioNetPara.audioparam.audioSource;
                    response.audioparam.soundMode = SoundMode_MOMO;
                    response.audioparam.audioFrameLen = 168;

                    //! for CB check criterion, G726 is only used
                    char check_criterion[32]={0};
                    pgAvDeviceInfo->Adi_get_check_criterion(check_criterion, sizeof(check_criterion));
                    char customer_name[32] = {0};
                    pgAvDeviceInfo->Adi_get_customer_name(customer_name,sizeof(customer_name));
                    if ((!strcmp(check_criterion, "CHECK_CB")))
                    {
                       response.audioparam.audioFormat = AudioFormat_G726;
                       response.audioparam.audioFrameLen = 84;
                    }
                    else if (!strcmp(customer_name, "CUSTOMER_04.1062")) //! 20160524
                    {
                        response.audioparam.audioFormat = AudioFormat_G711U;
                        response.audioparam.audioFrameLen = 160;                       
                    }
                    if ((!strcmp(customer_name, "CUSTOMER_JIUTONG")))
                    {
                       response.audioparam.audioFormat = AudioFormat_G726;
                       response.audioparam.audioFrameLen = 84;
                    }
                }
                else
                {
                    response.result = -1;
                    DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_START_TALKBACK start talkback FAILED!\n");
                }
            }
            else
            {
                response.result = -3;
                DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_START_TALKBACK other user talking!\n");
            }
            
            if(pTalkbackBody->stuAudioNetPara.talkback_type == 2)//监听
            {
                Common::N9M_MSGResponseMessage(m_message_client_handle, pMessage_head, (const char *)&response, sizeof(msgReponseAudioNetPara_t));
            }
            else
            {
                //add 2015.3.23 发消息给网络和UI模块
                DEBUG_WARNING("talkback----audioSource =%d, result =%d\n", response.audioparam.audioSource, response.result);
                Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_AVSTREAM_TALKBACK_START_STATUS, 0, (char *)&response, sizeof(msgReponseAudioNetPara_t), pTalkbackBody->iTalkHandle);
            }
        }
        else
        {
            DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_START_TALKBACK invalid message\n");
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_START_TALKBACK invalid message length\n");
    }

    return;
}

void CAvAppMaster::Aa_message_handle_STOP_TALKBACK(const char *msg, int length)
{
    DEBUG_WARNING("CAvAppMaster::Aa_message_handle_STOP_TALKBACK\n");
    if (!m_talkback_start)
    {
        return;
    }
   
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgAudioPlayPara_t* pAudioPlayParam = (msgAudioPlayPara_t*)pMessage_head->body;
        msgTalkbackAduio_t* pTalkbackBody = (msgTalkbackAduio_t*)pAudioPlayParam->body;
        DEBUG_CRITICAL("stop talkback  uiAudioType =%d ucOperateType=%d\n", pAudioPlayParam->uiAudioType, pAudioPlayParam->ucOperateType);
        DEBUG_CRITICAL("volumn=%d lababit =%d iTalkHandle =%d\n", pTalkbackBody->volumn, pTalkbackBody->lababit, pTalkbackBody->iTalkHandle);
        /* stop talkback */
        if(m_talkback_start == true)
        {
            if (Aa_talkback_ctrl(false, pTalkbackBody) != 0)
            {
                DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_STOP_TALKBACK stop talkback FAILED!\n");
            }
            m_talkback_start = false;
            m_pAv_device_master->Ad_set_audio_mute(1);
#if defined(_AV_HAVE_AUDIO_OUTPUT_)
            m_pAv_device_master->Ad_stop_ao(_AO_TALKBACK_);
#endif
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_STOP_TALKBACK invalid message length\n");
    }

    return;  
}
int CAvAppMaster::Aa_talkback_ctrl(bool start_flag, msgTalkbackAduio_t* pTalkbackPara)
{
    DEBUG_MESSAGE("CAvAppMaster::Aa_talkback_ctrl\n");
    //bool isStartEncoder = true; //根据这个判断先前是否该通道启动编码，以至于先前启动了，但停止对讲时把它停止了。
#ifdef _AV_SUPPORT_REMOTE_TALK_ //for nvr and ipc

    if( start_flag )
    {
        return m_pAv_device_master->Ad_Start_Remote_Talk( &(pTalkbackPara->stuAudioNetPara));
    }

    return m_pAv_device_master->Ad_Stop_Remote_Talk(&(pTalkbackPara->stuAudioNetPara));

#else

#if defined(_AV_HAVE_AUDIO_INPUT_) && defined(_AV_HAVE_VIDEO_DECODER_)
     if (start_flag)
     {
        if (pTalkbackPara != NULL)
        {
            if(pTalkbackPara->stuAudioNetPara.talkback_type == 0 /*|| pAudioNetPara->talkback_type == 2*/)//对讲 设备端
            {
                DEBUG_MESSAGE("########### start listen\n");
            /*start talkback ai*/
#ifndef _AV_PLATFORM_HI3520D_
            m_pAv_device->Ad_start_ai(_AI_TALKBACK_);
#endif
            /*start talkback aenc*/
            av_audio_encoder_para_t audio_para;
            audio_para.type = _AET_ADPCM_;
            m_pAv_device->Ad_get_AvEncoder()->Ae_start_talkback_encoder(_AST_UNKNOWN_, -1, NULL, _AAST_TALKBACK_, pgAvDeviceInfo->Adi_max_channel_number(), &audio_para, true);
            }
            if(pTalkbackPara->stuAudioNetPara.talkback_type == 0 || pTalkbackPara->stuAudioNetPara.talkback_type == 1)//对讲 广播
            {
                DEBUG_MESSAGE("########### start broadcast\n");
                /*start talkback adec*/
                //note 2015.2.13
                m_pAv_device_master->Ad_start_ao(_AO_TALKBACK_);
                m_pAv_device_master->Ad_start_talkback_adec();
            }
            if(pTalkbackPara->stuAudioNetPara.talkback_type == 2)//监听
            {
                DEBUG_MESSAGE("########### start listen\n");
                if(pTalkbackPara->stuAudioNetPara.audioparam.audioSource == 2)//mic
                {
                    DEBUG_MESSAGE("########### start listen mic \n");
#ifndef _AV_PLATFORM_HI3520D_
                m_pAv_device->Ad_start_ai(_AI_TALKBACK_);
#endif
                    /*start talkback aenc*/
                    av_audio_encoder_para_t audio_para;
                    audio_para.type = _AET_ADPCM_;
                    m_pAv_device->Ad_get_AvEncoder()->Ae_start_talkback_encoder(_AST_UNKNOWN_, -1, NULL, _AAST_TALKBACK_, pgAvDeviceInfo->Adi_max_channel_number(), &audio_para, true);
                }
                else
                {
                   DEBUG_MESSAGE("########### start listen camera \n");
                   for(int chn =0; chn<pgAvDeviceInfo->Adi_get_audio_chn_number(); chn++)
                   {
                        
                        if((pTalkbackPara->stuAudioNetPara.audioparam.channel) & (1ll<<chn) )
                        {
                            if(!m_main_encoder_state.test(chn))//没有启动编码,则启动
                            { 
          
                                return -1;
                                //Aa_start_selected_streamtype_encoders(pAudioNetPara->audioparam.channel, _AST_MAIN_VIDEO_, true);
                            }
                            else
                            {
                                if(m_pAv_device->Ad_get_AvEncoder()->Ae_get_video_lost_flag(chn) != 0)//no video
                                {
                                    DEBUG_WARNING("----listen chn = %d is no video----\n", chn);
                                    return -1;
                                }
                            }
                          
                        }
                   }
                    
                }
            }
        }
        else
        {
            DEBUG_ERROR( "CAvAppMaster::Aa_talkback_ctrl pAudioNetPara is NULL\n");
        }
     }
     else
     {
        if(pTalkbackPara->stuAudioNetPara.talkback_type == 0 /*|| pAudioNetPara->talkback_type == 2*/)
        {
            DEBUG_MESSAGE("########### stop listen\n");
            /*stop talkback adec*/
            m_pAv_device_master->Ad_stop_talkback_adec();
        }
        if(pTalkbackPara->stuAudioNetPara.talkback_type == 0 || pTalkbackPara->stuAudioNetPara.talkback_type == 1)
        {
             DEBUG_MESSAGE("########### stop broadcast\n");
             /*stop talkback aenc*/
             m_pAv_device->Ad_get_AvEncoder()->Ae_stop_talkback_encoder(_AST_UNKNOWN_, -1, _AAST_TALKBACK_, pgAvDeviceInfo->Adi_max_channel_number(), true);
             //note 2015.2.13
             m_pAv_device_master->Ad_stop_ao(_AO_TALKBACK_);
             /*stop talkback ai*/
#ifndef _AV_PLATFORM_HI3520D_
             m_pAv_device->Ad_stop_ai(_AI_TALKBACK_);
#endif
        }

        if(pTalkbackPara->stuAudioNetPara.talkback_type == 2)//监听
        {
            DEBUG_MESSAGE("########### stop listen\n");
            if(pTalkbackPara->stuAudioNetPara.audioparam.audioSource == 2)//mic
            {
                /*stop talkback aenc*/
                 m_pAv_device->Ad_get_AvEncoder()->Ae_stop_talkback_encoder(_AST_UNKNOWN_, -1, _AAST_TALKBACK_, pgAvDeviceInfo->Adi_max_channel_number(), true);

                 /*stop talkback ai*/
#ifndef _AV_PLATFORM_HI3520D_
                 m_pAv_device->Ad_stop_ai(_AI_TALKBACK_);
#endif
            }
            else //模拟摄像头音频
            {
               for(int chn =0; chn<pgAvDeviceInfo->Adi_get_audio_chn_number(); chn++)
               {
                    if((pTalkbackPara->stuAudioNetPara.audioparam.channel) & (1ll<<chn) )
                    {
                        if(m_main_encoder_state.test(chn)/* && (isStartEncoder == false)*/)
                        {
                            return -1;
                            //Aa_stop_selected_streamtype_encoders(pAudioNetPara->audioparam.channel, _AST_MAIN_VIDEO_ , true);
                        }
                    }
               }
                
            }
        }
     }
#endif

#endif
     return 0;
}

#if !defined(_AV_PRODUCT_CLASS_IPC_)

/*报站开始消息处理函数*/
void CAvAppMaster::Aa_message_handle_STARTREPORTSTATION(const char* msg, int length )
{
    DEBUG_WARNING("CAvAppMaster::Aa_message_handle_STARTREPORTSTATION\n");
    
    if(m_start_work_flag == 0)
    {
        DEBUG_ERROR("CAvAppMaster::Aa_message_handle_STARTREPORTSTATION system not ok\n");
        return;
    }
    SMessageHead *head = (SMessageHead*) msg;
    
    eSystemType subProduct_type = SYSTEM_TYPE_MAX;
    subProduct_type = N9M_GetSystemType();
    
    if( (head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgAudioPlayPara_t* pStationParam = (msgAudioPlayPara_t*)head->body;
        msgReportStation_t* pStationBody = (msgReportStation_t*)pStationParam->body;
        m_report_method = 0;
        Aa_Set_ReportStation_AudioType(pStationParam->uiAudioType);
        m_source = head->source;
        DEBUG_CRITICAL("----start report uiAudioType =%d ucOperateType=%d\n", pStationParam->uiAudioType, pStationParam->ucOperateType);
        DEBUG_CRITICAL("----AOChn =%d volumn=%d lababit =%d\n", pStationBody->RightLeftChn, pStationBody->volumn, pStationBody->lababit);
        if ((SYSTEM_TYPE_A5_AHD_V2 == subProduct_type)||(pgAvDeviceInfo->Adi_product_type() == PTD_M3))
        {
            pStationBody->RightLeftChn = ((pStationBody->RightLeftChn==0)?1:0); //! A5-ahd ak4951
        }
        if(!pStationParam->ucOperateType)//start report
        {
            if(!m_pAv_device_master->Ad_get_audio_mute())
            {
                m_pAv_device_master->Ad_set_audio_mute(1);
            }
            m_pAv_device_master->Ad_add_report_station_task(pStationBody);
            m_pAv_device_master->Ad_start_report_station();
        }
    }   
    else
    {
        DEBUG_ERROR("CAvAppMaster::Aa_message_handle_STARTREPORTSTATION invalid message length\n");
    }

    return;
}
/*报站停止消息处理函数*/
void CAvAppMaster::Aa_message_handle_STOPREPORTSTATION(const char* msg, int length )
{
    DEBUG_WARNING("CAvAppMaster::Aa_message_handle_STOPREPORTSTATION\n");
    if(m_start_work_flag == 0)
    {
        DEBUG_ERROR("CAvAppMaster::Aa_message_handle_STOPREPORTSTATION system not ok\n");
        return;
    }
    SMessageHead *head = (SMessageHead*) msg;
    if( (head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgAudioPlayPara_t* pStationParam = (msgAudioPlayPara_t*)head->body;
        DEBUG_CRITICAL("----stop report uiAudioType =%d ucOperateType=%d\n", pStationParam->uiAudioType, pStationParam->ucOperateType);
        if(pStationParam->ucOperateType)// stop report
        {
             m_pAv_device_master->Ad_stop_report_station();
             m_reportstation_start = false;
             m_pAv_device_master->Ad_set_audio_mute(1);
        }
    }   
    else
    {
        DEBUG_ERROR("CAvAppMaster::Aa_message_handle_STOPREPORTSTATION invalid message length\n");
    }
    return;
}
//音频播放状态查询
void CAvAppMaster::Aa_message_handle_INQUIRE_AUDIO_STATE(const char* msg, int length )
{
    //DEBUG_WARNING("\n\nCAvAppMaster::Aa_message_handle_INQUIRE_AUDIO_STATE\n\n");
    int result = 0;
    SMessageHead *head = (SMessageHead*) msg;
    if( (head->length + sizeof(SMessageHead)) == (unsigned int)length )
    {
        msgInquireAudioPlayState_t* pStationParam = (msgInquireAudioPlayState_t*)head->body;
        msgInquireAudioPlayState_t response;
        memset(&response, 0, sizeof(msgInquireAudioPlayState_t));
        response.uiAudioType = pStationParam->uiAudioType;
        if(/*pStationParam->uiAudioType == VOICE_REPORTINSIDE || pStationParam->uiAudioType == VOICE_REPORTOUTSIDE ||*/ pStationParam->uiAudioType == VOICE_SPEEDALARM 
        || pStationParam->uiAudioType == VOICE_ABNOPENDOORALARM ||pStationParam->uiAudioType == VOICE_MESSAGE || pStationParam->uiAudioType == VOICE_RF_CARD)
        {
            if ( m_report_method == 0)
            {
            //!report station           
                result = m_pAv_device_master->Ad_get_report_station_result();
                
            }
#if defined(_AV_HAVE_TTS_)
            else if (m_report_method == 1)
            {
                result = m_pAv_device_master->Ad_query_tts();
            }
#endif
            else
            {
                result = 0;
            }
            response.uiState = result;
            
            DEBUG_WARNING("report | TTS method =[%d] result =[%d] \n",m_report_method, response.uiState);
        }
        else
        {
            DEBUG_WARNING(" \ninquire uiAudioType =%d invalid\n", pStationParam->uiAudioType);
            return ;
        }
        Common::N9M_MSGResponseMessage(m_message_client_handle, head, (const char *)&response, sizeof(msgInquireAudioPlayState_t));        
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_INQUIRE_AUDIO_STATE invalid message length\n");
    }
    return;
}
void CAvAppMaster::Aa_SendReportEndResultMessage()
{
    
    msgInquireAudioPlayState_t response;
    memset(&response, 0, sizeof(msgInquireAudioPlayState_t));
    response.uiAudioType = Aa_Get_ReportStation_AudioType();
    response.uiState = m_pAv_device_master->Ad_get_report_station_result();
    
    if(response.uiState == AUDIO_PLAY_FINISH)
    {
        Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_BASIC_SERVICE_SET_AUDIO_PLAY_TASK_STATE, 0, (char *)&response, sizeof(msgInquireAudioPlayState_t), m_source);
    }
    return;
}

#endif
//开启预览
#if defined (_AV_HAVE_AUDIO_OUTPUT_)

void CAvAppMaster::Aa_message_handle_STARTPREVIEWAUDIO(const char* msg, int length )
{
    printf("######### CAvAppMaster::Aa_message_handle_STARTPREVIEWAUDIO\n");

    SMessageHead *head = (SMessageHead*) msg;
    if( (head->length + sizeof(SMessageHead)) == (unsigned int)length )
    {
        msgAudioPlayPara_t* pStationParam = (msgAudioPlayPara_t*)head->body;
        
        msgPreviewAudio_t* pStationBody = (msgPreviewAudio_t*)pStationParam->body;
        DEBUG_CRITICAL("----start   preview uiAudioType =%d ucOperateType=%d\n", pStationParam->uiAudioType, pStationParam->ucOperateType);
        DEBUG_CRITICAL("----lababit =0x%x\n", pStationBody->lababit);
        //this->Aa_start_ai();
#if defined(_AV_HAVE_AUDIO_INPUT_) && (defined(_AV_PRODUCT_CLASS_DVR_) || defined(_AV_PRODUCT_CLASS_HDVR_))  

#if !defined(_AV_PLATFORM_HI3515_)
        m_pAv_device_master->Ad_set_preview_state(false);
        m_pAv_device_master->Ad_set_preview_start(true);
#else
        m_pAv_device_master->Ad_unbind_preview_ao();
        m_pAv_device_master->Ad_bind_preview_ao(0); 
#endif

#else
        Aa_start_ao(); //for nvr
#endif       
#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
     //this->Aa_start_preview_dec();
     //m_pAv_device_master->Ad_set_preview_dec_start_flag(true);
#endif
        m_pAv_device_master->Ad_set_audio_mute(0);
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_STARTPREVIEWAUDIO invalid message length\n");
    }
        return;
    }
//停止预览
void CAvAppMaster::Aa_message_handle_STOPPREVIEWAUDIO(const char* msg, int length )
{
    DEBUG_WARNING("######### CAvAppMaster::Aa_message_handle_STOPPREVIEWAUDIO\n");
	
    SMessageHead *head = (SMessageHead*) msg;
    if( (head->length + sizeof(SMessageHead)) == (unsigned int)length )
    {   
        msgAudioPlayPara_t* pStationParam = (msgAudioPlayPara_t*)head->body;
        msgPreviewAudio_t* pStationBody = (msgPreviewAudio_t*)pStationParam->body;
        DEBUG_CRITICAL("----stop preview uiAudioType =%d ucOperateType=%d\n", pStationParam->uiAudioType, pStationParam->ucOperateType);
        DEBUG_CRITICAL("----lababit =0x%x\n", pStationBody->lababit);
#if defined(_AV_HAVE_AUDIO_INPUT_) && (defined(_AV_PRODUCT_CLASS_DVR_) || defined(_AV_PRODUCT_CLASS_HDVR_))         
    #if !defined(_AV_PLATFORM_HI3515_)
        m_pAv_device_master->Ad_set_preview_start(false);
        m_pAv_device_master->Ad_set_preview_state(true);
    #else
        m_pAv_device_master->Ad_unbind_preview_ao();
    #endif
#else
        Aa_stop_ao();
#endif
#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
     //this->Aa_stop_preview_dec();
     //m_pAv_device_master->Ad_set_preview_dec_start_flag(false);
#endif
        m_pAv_device_master->Ad_set_audio_mute(1);
        //this->Aa_stop_ai();
        
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_STOPPREVIEWAUDIO invalid message length\n");
    }
    return;
}
void CAvAppMaster::Aa_message_handle_Set_PreviewAudio_State(const char* msg, int length )
{
    DEBUG_WARNING("######### CAvAppMaster::Aa_message_handle_Set_PreviewAudio_State \n");
	
    SMessageHead *head = (SMessageHead*) msg;
    if( (head->length + sizeof(SMessageHead)) == (unsigned int)length )
    {   
        msgGuiUserInterface_t* pPreAudioState = (msgGuiUserInterface_t*)head->body;
        DEBUG_WARNING("----PreviewAudio_State =%d\n", pPreAudioState->ucGuiStatus);
        //m_pAv_device_master->Ad_set_preview_state(pPreAudioState->ucGuiStatus);//false preview  true not preview
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_STOPPREVIEWAUDIO invalid message length\n");
    }
    return;
}
#endif
//开启回放音频


#if defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_) || defined(_AV_PRODUCT_CLASS_HDVR_)
void CAvAppMaster::Aa_message_handle_DeviceOnlineState( const char* msg, int length )
{
    SMessageHead *pHead = (SMessageHead*)msg;
    if( pHead->length != sizeof(msgAllDeviceState_t) )
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_DeviceOnlineState invalid message %d=%d?\n"
            , pHead->length, sizeof(msgAllDeviceState_t));
        return;
    }

    msgAllDeviceState_t* pstuParam = (msgAllDeviceState_t *)pHead->body;

    //DEBUG_MESSAGE( "CAvAppMaster::Aa_message_handle_DeviceOnlineState==mask=0x%x\n", pstuParam->mask);

    m_start_work_env[MESSAGE_ONVIF_DEVICE_ONLINE_GET] = true; 
    //DEBUG_MESSAGE("NVR online status:");
    unsigned char szState[_AV_VIDEO_MAX_CHN_];
    memset(szState, 0, sizeof(szState));
    for( int ch=0; ch <pgAvDeviceInfo->Adi_max_channel_number(); ch++ )
    {
        if( (pstuParam->mask>>ch) & 0x1 )
        {
            if( pstuParam->uistate[ch] )
            {
                szState[ch] = 1;
                m_stuModuleEncoderInfo.u8OnlineState[ch] = 1;
            }
            else
            {
                szState[ch] = 0;
                m_stuModuleEncoderInfo.u8OnlineState[ch] = 0;
            }
        }
        else
        {
            szState[ch] = 3;
        }
        //DEBUG_MESSAGE("%d ", szState[ch]);
    }
    //DEBUG_MESSAGE("\n");
    /*
    if( m_pAv_device_master )
    {
        m_pAv_device_master->Ad_UpdateDevOnlineState( szState );
    }*/
    return;
}

void CAvAppMaster::Aa_message_handle_RemoteDeviceVideoLoss( const char* msg, int length )
{
    SMessageHead *pHead = (SMessageHead*)msg;
    msgVideoStatus_t* pstuParam = NULL;
    unsigned char szState[SUPPORT_MAX_VIDEO_CHANNEL_NUM];
    int index = 0;

    if(pHead->length == sizeof(msgVideoStatus_t))
    {
        pstuParam = (msgVideoStatus_t *)pHead->body;
        for(index = 0; index < pgAvDeviceInfo->Adi_max_channel_number(); index++)
        {
            if(m_ChnProtocolType[index] != 0xff)
            {
                if(pstuParam->u32Src[index] == m_ChnProtocolType[index])
                {
                    m_remote_vl[index] = pstuParam->u32Status[index];
                }
            }
            else
            {
                m_remote_vl[index] = 1;
            }

            if(m_remote_vl[index])
            {
                szState[index] = 0;
            }
            else
            {
                szState[index] = 1;
            }
            //DEBUG_MESSAGE("[%s,%d]chn:%d,protocol:%d(%d),status:%d!\n", __FUNCTION__, __LINE__, index, pstuParam->u32Mask[index], m_ChnProtocolType[index], pstuParam->u32Status[index]);
        }
        if( m_pAv_device_master )
        {
            m_pAv_device_master->Ad_UpdateDevOnlineState(szState);
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_DeviceOnlineState invalid message %d=%d?\n", pHead->length, sizeof(msgVideoStatus_t));
    }
}

void CAvAppMaster::Aa_message_handle_ChannelMap( const char* msg, int length )
{
    char szChlMap[_AV_VIDEO_MAX_CHN_];
    int icurrent_chn_state[_AV_VIDEO_MAX_CHN_] = {0};
    unsigned long long int ullanalog_to_digital_mask = 0;
    unsigned long long int ulldigital_to_analog_mask = 0;
    unsigned int uistream_type_mask = 0;
    //! Added for X1
    eProductType product_type = PTD_INVALID;
    eSystemType subproduct_type = SYSTEM_TYPE_MAX;

    /*device type*/
    N9M_GetProductType(product_type);
    subproduct_type = N9M_GetSystemType();
    //end add
    SMessageHead* phead = (SMessageHead*)msg;

    DEBUG_CRITICAL( "CAvAppMaster::Aa_message_handle_ChannelMap \n");
    if( phead->length != sizeof(msgRemoteDevNodeSetting_t) )
    {
        DEBUG_ERROR("Param length error while get exdev info, hope=%d recv=%d\n", sizeof(msgRemoteDevNodeSetting_t), phead->length );
        return;
    }
    
    msgRemoteDevNodeSetting_t* pstuParam = (msgRemoteDevNodeSetting_t*)phead->body;
    memset( szChlMap, -1, sizeof(szChlMap));
    
    for( int index = 0; index < pgAvDeviceInfo->Adi_max_channel_number(); index++ )
    {
        if (GCL_BIT_VAL_TEST(pstuParam->mask, index))
        {
            if (index < pgAvDeviceInfo->Adi_get_vi_chn_num())
            {
                if (m_start_work_flag != 0)
                {
                    if (pstuParam->stuRemoteDevNodeSetting[index].enable)
                    {
                        icurrent_chn_state[index] = 1;
                    }
                    else
                    {
                        icurrent_chn_state[index] = 0;
                    }
                    
                    if (m_last_chn_state[index] - icurrent_chn_state[index] > 0)
                    {
                        m_last_chn_state[index] = 0;
                        GCL_BIT_VAL_SET(ulldigital_to_analog_mask, index);
                    }
                    else if (m_last_chn_state[index] - icurrent_chn_state[index] < 0)
                    {
                        m_last_chn_state[index] = 1;
                        GCL_BIT_VAL_SET(ullanalog_to_digital_mask, index);
                    }
                    else
                    {
                        DEBUG_MESSAGE("same chn state:  m_last_chn_state[%d] = %d, icurrent_chn_state[%d] = %d\n", index, m_last_chn_state[index], index, icurrent_chn_state[index]);
                    }
                    
                }
                else
                {
                    //DEBUG_MESSAGE( "chn %d is %s!\n", index, pstuParam->stuRemoteDevNodeSetting[index].enable ? "digital" : "analog");
                    pgAvDeviceInfo->Adi_set_video_source(index, pstuParam->stuRemoteDevNodeSetting[index].enable);
                    m_last_chn_state[index] = ((pstuParam->stuRemoteDevNodeSetting[index].enable == 1) ? 1 : 0);
                }
            }
            else
            {
                pgAvDeviceInfo->Adi_set_video_source(index, _VS_DIGITAL_);
                m_last_chn_state[index] = 1;
            }
            
            if (pstuParam->stuRemoteDevNodeSetting[index].enable && (pstuParam->stuRemoteDevNodeSetting[index].lock != 0xff))
            {
                //! added for channel 
#if defined(_AV_HAVE_VIDEO_ENCODER_)
                if ((m_start_work_flag != 0))
                {  
                    if (m_chn_state[index] == 0)
                    {
                        szChlMap[index] = (char)-1;
                        m_ChnProtocolType[index] = pstuParam->stuRemoteDevNodeSetting[index].protocoltype;
                        DEBUG_MESSAGE(" Set channel[%d] disable\n", index);  
                    }
                    else
                    {   
                        szChlMap[index] = index;
                        m_ChnProtocolType[index] = pstuParam->stuRemoteDevNodeSetting[index].protocoltype;

                    }
                }
                else
                {
                    szChlMap[index] = index;
                    m_ChnProtocolType[index] = pstuParam->stuRemoteDevNodeSetting[index].protocoltype;
                }
#else
                szChlMap[index] = index;
                m_ChnProtocolType[index] = pstuParam->stuRemoteDevNodeSetting[index].protocoltype;
#endif

            }
            else
            {
                szChlMap[index] = (char)-1;
                m_ChnProtocolType[index] = 0xff;
            }
        }
    }
    
    if (0 != m_start_work_flag && true == m_start_work_env[MESSAGE_CONFIG_MANAGE_REMOTE_DEVICE_NODE_PARAM_GET])
    {
        GCL_BIT_VAL_SET(uistream_type_mask, _AST_MAIN_VIDEO_);
        
        if(!(((product_type == PTD_X1) && ((subproduct_type == SYSTEM_TYPE_X1_M0401) ||\
                                        (subproduct_type == SYSTEM_TYPE_X1_V2_M1H0401)||\
                                       (subproduct_type == SYSTEM_TYPE_X1_V2_M1SH0401)))||\
            ((product_type == PTD_X1_AHD) && ((subproduct_type == SYSTEM_TYPE_X1_V2_M1H0401_AHD)||\
                                       (subproduct_type == SYSTEM_TYPE_X1_V2_M1SH0401_AHD)))||\
        (product_type == PTD_D5M)||(product_type == PTD_A5_II)||(product_type == PTD_6A_I)))

        {
            GCL_BIT_VAL_SET(uistream_type_mask, _AST_SUB_VIDEO_);
        }
        GCL_BIT_VAL_SET(uistream_type_mask, _AST_SUB2_VIDEO_);
        DEBUG_MESSAGE("\n+++ullanalog_to_digital_mask = %llu  =%llu+++\n", ullanalog_to_digital_mask,ulldigital_to_analog_mask);
        if (0 != ullanalog_to_digital_mask || 0 != ulldigital_to_analog_mask)
        {
#if defined(_AV_HAVE_VIDEO_ENCODER_)
            m_pAv_device->Ad_Set_Chn_state(1);
             while(m_pAv_device->Ad_Get_Chn_state() == 1)
            {
                mSleep(30);
            }
#endif
           
        }
        if (0 != ullanalog_to_digital_mask)
        {
#ifdef _AV_HAVE_VIDEO_ENCODER_        
            Aa_stop_selected_streamtype_encoders(ullanalog_to_digital_mask, uistream_type_mask);
#endif
#ifdef _AV_HAVE_VIDEO_INPUT_
            Aa_stop_selected_vda(_VDA_MD_, ullanalog_to_digital_mask);
            Aa_stop_selected_vi_for_analog_to_digital(ullanalog_to_digital_mask);
#endif
            if( m_pAv_device_master )
            {
                m_pAv_device_master->Ad_UpdateDevMap(szChlMap);
            }
        } 
        
        if (0 != ulldigital_to_analog_mask)
        {
            if( m_pAv_device_master )
            {
                m_pAv_device_master->Ad_UpdateDevMap(szChlMap);
            }
            
            for(int chn_index = 0; chn_index < pgAvDeviceInfo->Adi_get_vi_chn_num(); chn_index++)
            {
                if(GCL_BIT_VAL_TEST(ulldigital_to_analog_mask, chn_index))
                {
                    while(pgAvDeviceInfo->Adi_get_created_preview_decoder(chn_index))
                    {
                        mSleep(30 );
                    }
                }
            }
#ifdef _AV_HAVE_VIDEO_INPUT_
            Aa_start_selected_vi_for_digital_to_analog(ulldigital_to_analog_mask);
            Aa_start_selected_vda(_VDA_MD_, ulldigital_to_analog_mask);
#endif
#ifdef _AV_HAVE_VIDEO_ENCODER_
            Aa_start_selected_streamtype_encoders(ulldigital_to_analog_mask, uistream_type_mask);
#endif
        }

        if (0 == ulldigital_to_analog_mask && 0 == ullanalog_to_digital_mask)
        {
            if ( m_pAv_device_master )
            {
                m_pAv_device_master->Ad_UpdateDevMap(szChlMap);
            }
        }
    }
    else
    {
        if ( m_pAv_device_master )
        {
            m_pAv_device_master->Ad_UpdateDevMap(szChlMap);
        }
    }
#if defined(_AV_HAVE_VIDEO_ENCODER_)
    m_pAv_device->Ad_Set_Chn_state(3);
#endif

    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_EVENT_GET_VL_STATUS, 1, NULL, 0, 0);
    m_start_work_env[MESSAGE_CONFIG_MANAGE_REMOTE_DEVICE_NODE_PARAM_GET] = true;
}
#endif // endif #if defined(_AV_PRODUCT_CLASS_NVR_)

#if defined(_AV_PRODUCT_CLASS_IPC_)
void CAvAppMaster::Aa_message_handle_CameraAttr( const char* msg, int length )
{
    SMessageHead* phead = (SMessageHead*)msg;
    if( phead->length != sizeof(msgCameraAttributeQuickSet_t) )
    {
        DEBUG_ERROR("Param length error while get camera attribute info, hope=%d recv=%d\n", sizeof(msgVideoImageSetting_t), phead->length );
        return;
    }

    msgCameraAttributeQuickSet_t* pstuParam = (msgCameraAttributeQuickSet_t*)phead->body;

    if( !m_pstuCameraAttrParam )
    {
        m_pstuCameraAttrParam = new msgCameraAttributeQuickSet_t;
        memset( m_pstuCameraAttrParam, 0, sizeof(msgCameraAttributeQuickSet_t) );
    }
    memcpy( m_pstuCameraAttrParam, pstuParam, sizeof(msgCameraAttributeQuickSet_t) );
        
    if( m_start_work_flag != 0 )
    {
        m_pAv_device_master->Ad_SetCameraAttribute( pstuParam->stuViParam[0].uiParamMask, &(pstuParam->stuViParam[0].image) );

        if( ((pstuParam->stuViParam[0].uiParamMask>>11) & 0x1) || ((pstuParam->stuViParam[0].uiParamMask>>12) & 0x1) )
        {
          //  this->Aa_cover_area( m_pCover_area_para, m_pCover_area_para );
        }
    }
    else
    {
        pgAvDeviceInfo->Adi_set_system_rotate( pstuParam->stuViParam[0].image.u8Rotate );
    }

    m_start_work_env[MESSAGE_CONFIG_MANAGE_CAMERA_ATTRIBUTE_GET] = true;
}

void CAvAppMaster::Aa_message_handle_CameraColor( const char* msg, int length )
{
    SMessageHead* phead = (SMessageHead*)msg;
    if( phead->length != sizeof(msgVideoImageSetting_t) )
    {
        DEBUG_ERROR("Param length error while get camera color param, hope=%d recv=%d\n", sizeof(msgVideoImageSetting_t), phead->length );
        return;
    }

    m_start_work_env[MESSAGE_CONFIG_MANAGE_VIDEO_IMAGE_PARAM_GET] = true;

    msgVideoImageSetting_t* pstuParam = (msgVideoImageSetting_t*)phead->body;
    if( !m_pstuCameraColorParam )
    {
        m_pstuCameraColorParam = new msgVideoImageSetting_t;
    }
    if( m_pstuCameraColorParam )
    {
        memcpy( m_pstuCameraColorParam, pstuParam, sizeof(msgVideoImageSetting_t) );
    }
    
    if( m_start_work_flag != 0 )
    {
        /*add by dhdong for test*/
#if 0
        char door_state[32];

        memset(door_state, 0, sizeof(door_state));
        if(pstuParam->stuVideoImageSetting[0].ucChromaticity >= 32)
        {
            m_pAv_device_master->Ad_notify_bus_front_door_state(1);
            strncpy(door_state, "Door State:open", sizeof(door_state));
        }
        else
        {
            m_pAv_device_master->Ad_notify_bus_front_door_state(0);
            strncpy(door_state, "Door State:close", sizeof(door_state));
        }
        if(1 == pgAvDeviceInfo->Adi_get_ive_debug_mode())
        {
            m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, 0, _AON_COMMON_OSD1_, door_state, strlen(door_state));                
        }

#endif
        m_pAv_device_master->Ad_SetCameraColor( &(pstuParam->stuVideoImageSetting[0]) );
    }
}

void CAvAppMaster::Aa_message_handle_LightLeval( const char* msg, int lenght )
{
    SMessageHead* phead = (SMessageHead*)msg;
    if( phead->length != sizeof(msgEnvLuminance_t) )
    {
        DEBUG_ERROR("CAvAppMaster::Aa_message_handle_LightLeval Param length error , hope=%d recv=%d\n", sizeof(msgVideoImageSetting_t), phead->length );
        return;
    }
    
    if(m_start_work_flag  != 0)
    {
        msgEnvLuminance_t* pstuParam = (msgEnvLuminance_t*)phead->body;
        m_pAv_device_master->Ad_SetEnvironmentLuma( pstuParam->u32EnvLuminance );       
    }
}

void CAvAppMaster::Ae_message_handle_GetDayNightCutLuma( const char* msg, int length )
{
#if 0
    SMessageHead* phead = (SMessageHead*)msg;
    if( !phead || phead->length != sizeof(msgSaveDayNightCutLumaValue_t) )
    {
        DEBUG_ERROR("CAvAppMaster::Aa_message_handle_LightLeval Param length error , hope=%d recv=%d\n", sizeof(msgVideoImage_t), phead->length );
        return;
    }

    m_start_work_env[MESSAGE_CONFIG_IPC_DAY_NIGHT_CUT_LUM_GET] = true;

    msgSaveDayNightCutLumaValue_t* pstuCutValue = (msgSaveDayNightCutLumaValue_t*)phead->body;
    DEBUG_MESSAGE("mask=0x%x day->night=%d night->day=%d", pstuCutValue->u32Mask, pstuCutValue->u32Day2NightValue, pstuCutValue->u32Night2DayValue);
    m_pAv_device_master->Ad_SetDayNightCutLuma( pstuCutValue->u32Mask, pstuCutValue->u32Day2NightValue, pstuCutValue->u32Night2DayValue );
#endif
}

void CAvAppMaster::Aa_message_handle_MacAddress(const char* msg, int length)
{
    SMessageHead* phead = (SMessageHead*)msg;
    if( phead->length  == sizeof(msgKeySetting_t))
    {
        msgKeySetting_t *pMac_address = (msgKeySetting_t*)phead->body;
        _AV_KEY_INFO_(_AVD_APP_, "focus len is:%d \n", pMac_address->stuKeySetting.ucFocusLen);
        DEBUG_MESSAGE("the mac address is:%#x-%#x-%#x-%#x-%#x-%#x \n", pMac_address->stuKeySetting.ucMacAddr[0], \
            pMac_address->stuKeySetting.ucMacAddr[1], pMac_address->stuKeySetting.ucMacAddr[2], pMac_address->stuKeySetting.ucMacAddr[3], \
            pMac_address->stuKeySetting.ucMacAddr[4], pMac_address->stuKeySetting.ucMacAddr[5]);
        if(!m_start_work_env[MESSAGE_CONFIG_MANAGE_KEY_PARAM_GET] )
        {
            m_start_work_env[MESSAGE_CONFIG_MANAGE_KEY_PARAM_GET] = true;
        }

#if defined(_AV_HAVE_ISP_)
        if(0 != pMac_address->stuKeySetting.ucFocusLen)
        {
            if(m_start_work_flag == 0)
            {
                pgAvDeviceInfo->Adi_set_focus_length(pMac_address->stuKeySetting.ucFocusLen);
            }
            else
            {
                if(pgAvDeviceInfo->Adi_get_focus_length() != pMac_address->stuKeySetting.ucFocusLen)
                {
                     _AV_KEY_INFO_(_AVD_APP_, "focus len changed from:%d -> %d \n", pgAvDeviceInfo->Adi_get_focus_length(), pMac_address->stuKeySetting.ucFocusLen);
                     pgAvDeviceInfo->Adi_set_focus_length(pMac_address->stuKeySetting.ucFocusLen);
                    RM_ISP_API_IspExit();
                    usleep(100*1000);
#if defined(_AV_PLATFORM_HI3518C_)
                    RM_ISP_API_Init(pgAvDeviceInfo->Adi_get_sensor_type(),pgAvDeviceInfo->Adi_get_focus_length());
#else
                    ISPInitParam_t isp_param;

                    memset(&isp_param, 0, sizeof(ISPInitParam_t));
                    isp_param.sensor_type = pgAvDeviceInfo->Adi_get_sensor_type();
                    isp_param.focus_len = pgAvDeviceInfo->Adi_get_focus_length();
                    isp_param.u8WDR_Mode = WDR_MODE_NONE;
                    isp_param.etOptResolution = pgAvDeviceInfo->Adi_get_isp_output_mode();
                    _AV_KEY_INFO_(_AVD_APP_, "isp init, sensor type:%d focus len:%d wdr mode:%d resolution:%d \n", isp_param.sensor_type, isp_param.focus_len, isp_param.u8WDR_Mode, isp_param.etOptResolution);
                    RM_ISP_API_Init(&isp_param);
#endif
                    RM_ISP_API_InitSensor();
                    RM_ISP_API_IspStart();
                    usleep(100*1000);
                    m_pAv_device_master->Ad_SetCameraColor( &(m_pstuCameraColorParam->stuVideoImageSetting[0]) );
                    m_pAv_device_master->Ad_SetCameraAttribute( 0xffffffff, &(m_pstuCameraAttrParam->stuViParam[0].image) );
                }
            }
        }
#endif

#if defined(_AV_PLATFORM_HI3518C_)
        if(PTD_712_VD == pgAvDeviceInfo->Adi_product_type() &&  (0 != pgAvDeviceInfo->Adi_get_ipc_work_mode()))
        {
            if((pMac_address->stuKeySetting.n8ImageOffset>=1) &&(pMac_address->stuKeySetting.n8ImageOffset<=21))
            {
                int y_offset = (11-pMac_address->stuKeySetting.n8ImageOffset)*15;
                pgAvDeviceInfo->Adi_set_vi_chn_capture_y_offset(y_offset);
                if(m_start_work_flag != 0)
                {
                     for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); ++chn)
                     {
                        m_pAv_device_master->Ad_set_vi_chn_capture_y_offset(chn, y_offset);
                     }
                }
            }
        }
        
        if(PTD_712_VB == pgAvDeviceInfo->Adi_product_type())
        {
            pgAvDeviceInfo->Adi_set_mac_address(pMac_address->stuKeySetting.ucMacAddr, 6);

            for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); ++chn)
            {
                if(m_main_encoder_state.test(chn))
                {
                    m_pAv_device->Ad_update_extra_osd_content(_AST_MAIN_VIDEO_, chn, _AON_CHN_NAME_, m_chn_name[chn], sizeof(m_chn_name[chn]));                           
                }
                if(m_sub_encoder_state.test(chn))
                {
                    m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, chn, _AON_CHN_NAME_, m_chn_name[chn], sizeof(m_chn_name[chn])); 
                }
                if(m_sub2_encoder_state.test(chn))
                {
                    m_pAv_device->Ad_update_extra_osd_content(_AST_SUB2_VIDEO_, chn, _AON_CHN_NAME_, m_chn_name[chn], sizeof(m_chn_name[chn])); 
                }
            }
        }
#endif
        _AV_KEY_INFO_(_AVD_APP_, "the ircut flag is:%d the audio flag is:%d \n", pMac_address->stuKeySetting.ucIrcutFlag, pMac_address->stuKeySetting.ucAudioFlag);
        if(pMac_address->stuKeySetting.ucIrcutFlag == 2)
        {
            pgAvDeviceInfo->Adi_set_ircut_and_infrared_flag(false);
        }
        else
        {
            pgAvDeviceInfo->Adi_set_ircut_and_infrared_flag(true);
        }
        
        if(pMac_address->stuKeySetting.ucAudioFlag == 2)
        {
            if(pgAvDeviceInfo->Adi_get_audio_chn_number() > 0)
            {
                if(m_start_work_flag != 0)
                {
                    unsigned char streamtype_mask = Aa_get_all_streamtype_mask();
                    Aa_stop_selected_streamtype_encoders(pgAvDeviceInfo->Aa_get_all_analog_chn_mask(), streamtype_mask);
                    Aa_stop_ai();
                    pgAvDeviceInfo->Adi_set_audio_chn_number(0);
                    Aa_start_selected_streamtype_encoders(pgAvDeviceInfo->Aa_get_all_analog_chn_mask(), streamtype_mask);

                }
                else
                {
                    pgAvDeviceInfo->Adi_set_audio_chn_number(0);
                }
                
            }
        }
        else
        {
            if(pgAvDeviceInfo->Adi_get_audio_chn_number() <= 0)
            {
                if(m_start_work_flag != 0)
                {
                    unsigned char streamtype_mask = Aa_get_all_streamtype_mask();
                    Aa_stop_selected_streamtype_encoders(pgAvDeviceInfo->Aa_get_all_analog_chn_mask(), streamtype_mask);
                    pgAvDeviceInfo->Adi_set_audio_chn_number(1);
                    Aa_start_ai();
                    Aa_start_selected_streamtype_encoders(pgAvDeviceInfo->Aa_get_all_analog_chn_mask(), streamtype_mask);
                }
                else
                {
                    pgAvDeviceInfo->Adi_set_audio_chn_number(1);
                }
            }
        }
#if defined(_AV_PLATFORM_HI3516A_) && defined(_AV_SUPPORT_IVE_)
        if(pMac_address->stuKeySetting.ucOperationMode != pgAvDeviceInfo->Adi_get_ive_debug_mode())
        {
            _AV_KEY_INFO_(_AVD_APP_, "the software mode is:%d \n", pMac_address->stuKeySetting.ucOperationMode);
            if(m_start_work_flag != 0)
            {
                if(pMac_address->stuKeySetting.ucOperationMode == 1)
                {
                    unsigned char streamtype_mask = Aa_get_all_streamtype_mask();
                    Aa_stop_selected_streamtype_encoders(pgAvDeviceInfo->Aa_get_all_analog_chn_mask(), streamtype_mask);
                    Aa_clear_common_osd();
                    pgAvDeviceInfo->Adi_set_ive_debug_mode(1);
                    Aa_start_selected_streamtype_encoders(pgAvDeviceInfo->Aa_get_all_analog_chn_mask(), streamtype_mask);
                }
                else
                {
                    unsigned char streamtype_mask = Aa_get_all_streamtype_mask();
                    Aa_stop_selected_streamtype_encoders(pgAvDeviceInfo->Aa_get_all_analog_chn_mask(), streamtype_mask);
                    Aa_clear_common_osd();
                    pgAvDeviceInfo->Adi_set_ive_debug_mode(0);
                    Aa_start_selected_streamtype_encoders(pgAvDeviceInfo->Aa_get_all_analog_chn_mask(), streamtype_mask);
                }
            }
            else
            {
                if(pMac_address->stuKeySetting.ucOperationMode == 1)
                {
                     pgAvDeviceInfo->Adi_set_ive_debug_mode(1);
                }
                else
                {
                    pgAvDeviceInfo->Adi_set_ive_debug_mode(0);
                }
            }
        }
#endif
    }
    else
    {
        DEBUG_ERROR("Aa_message_handle_MacAddress parameter length error, length:%d expect:%d \n",phead->length ,sizeof(msgKeySetting_t));
    }

}

void CAvAppMaster::Aa_message_handle_AuthSerial(const char* msg, int length)
{
    SMessageHead* phead = (SMessageHead*)msg;
    if( phead->length  == sizeof(msgAuthSerial_t))
    {
        msgAuthSerial_t *pAuthSerial = (msgAuthSerial_t*)phead->body; 

        if(0 != m_start_work_flag)
        {
            if(0 != pAuthSerial->lResult)
            {
                _AV_KEY_INFO_(_AVD_APP_, "the auth serial is error! software will stop work! \n");
                Aa_stop_work();
            }
        }
        else
        {
            if(0 == pAuthSerial->lResult)
            {
                m_start_work_env[MESSAGE_DEVICE_MANAGER_GET_AUTH_SERIAL] = true;
            }
            else
            {
                _AV_KEY_INFO_(_AVD_APP_, "the auth serial is error! software will not work! \n");
            }
        }
    }
    else
    {
        DEBUG_ERROR("Aa_message_handle_AuthSerial parameter length error, length:%d expect:%d \n",phead->length ,sizeof(msgAuthSerial_t));        
    }
}


#if defined(_AV_SUPPORT_IVE_)
#if defined(_AV_IVE_FD_) || defined(_AV_IVE_HC_)
void CAvAppMaster::Aa_message_handle_IveFaceDectetion(const char* msg, int length)
{
    SMessageHead* phead = (SMessageHead*)msg;
    if( phead->length  == sizeof(msgIntelligentCtrl_t))
    {
        msgIntelligentCtrl_t* intelligent_ctrl = (msgIntelligentCtrl_t*)phead->body;
        DEBUG_MESSAGE("the bus door state is:%d \n", intelligent_ctrl->ucCtrlCmd);
        if(NULL != intelligent_ctrl && 0 != m_start_work_flag)
        {
            char door_state[32];

            memset(door_state, 0, sizeof(door_state));
            if(intelligent_ctrl->ucCtrlCmd == 1)
            {
                m_pAv_device_master->Ad_notify_bus_front_door_state(1);
                strncpy(door_state, "Door State: open", sizeof(door_state));
            }
            else
            {
                 m_pAv_device_master->Ad_notify_bus_front_door_state(0);
                 strncpy(door_state, "Door State: close", sizeof(door_state));
            }
            if(1 == pgAvDeviceInfo->Adi_get_ive_debug_mode())
            {
                m_pAv_device->Ad_update_extra_osd_content(_AST_MAIN_VIDEO_, 0, _AON_COMMON_OSD1_, door_state, strlen(door_state));
                m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, 0, _AON_COMMON_OSD1_, door_state, strlen(door_state)); 
                m_pAv_device->Ad_update_extra_osd_content(_AST_SUB2_VIDEO_, 0, _AON_COMMON_OSD1_, door_state, strlen(door_state)); 
            }
        }
    }
}
#endif

#if defined(_AV_IVE_BLE_)
void CAvAppMaster::Aa_message_handle_chongqing_blacklist(const char* msg, int length)
{
    SMessageHead* phead = (SMessageHead*)msg;
    if( phead->length  == sizeof(stMessageResponse))
    {
        stMessageResponse* result = (stMessageResponse*)phead->body;
        
        if(NULL != result && 0 != m_start_work_flag)
        {
            if(0 == result->result)
            {
                m_pAv_device_master->Ad_update_blacklist();
            }
            else
            {
                DEBUG_ERROR("the blacklist is not ready! \n");
            }
        }
    }
    else
    {
        DEBUG_ERROR("the blacklist update message len is invalid! \n");
    }

}
#endif

void CAvAppMaster::Aa_clear_common_osd()
{
    memset(m_ext_osd_content[0], 0, sizeof(m_ext_osd_content[0]));
    memset(m_ext_osd_content[1], 0, sizeof(m_ext_osd_content[1]));
}


void CAvAppMaster::Aa_ive_detection_result_notify_func(msgIPCIntelligentInfoNotice_t* pIve_detection_result)
{
    if(NULL == pIve_detection_result)
    {
        DEBUG_ERROR("the ive detection result is NULL ! \n");
        return;
    }
    
#if defined(_AV_IVE_BLE_)  // bus lane
    if(1 == pgAvDeviceInfo->Adi_get_ive_debug_mode())
    {
        char customer[32];
        memset(customer, 0, sizeof(customer));
        pgAvDeviceInfo->Adi_get_customer_name(customer, sizeof(customer));

        if(0 == strcmp(customer, "CUSTOMER_SZGJ"))
        {
            if( pIve_detection_result->cmdtype == 1)
            {
                //if( strlen((char *)pIve_detection_result->datainfo.carnum[0]) != 0)
                {          
                    int len = strlen((char *)pIve_detection_result->datainfo.carnum[0]);
                    m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, 0, _AON_BUS_NUMBER_, \
                    (char *)pIve_detection_result->datainfo.carnum[0], len); 
                    DEBUG_MESSAGE("Bus number updated!\n");
                    for (int i=0;i<len; i++)
                    {
                        DEBUG_MESSAGE("carnum[%d]:0x%x\n", i, pIve_detection_result->datainfo.carnum[0][i]);
                    }
                }
                
                if ( pIve_detection_result->happen == 0)
                {
                    return;
                }
            }
        }        
        else
        {
            if( pIve_detection_result->cmdtype == 3)
            {  
                if(pIve_detection_result->happen == 0)
                {
                    m_pAv_device->Ad_update_extra_osd_content(_AST_SUB2_VIDEO_, 0, _AON_COMMON_OSD1_, (char *)"", 0); 
                    m_pAv_device->Ad_update_extra_osd_content(_AST_SUB2_VIDEO_, 0, _AON_COMMON_OSD2_, (char *)"", 0); 
                    m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, 0, _AON_COMMON_OSD1_, (char *)"", 0); 
                    m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, 0, _AON_COMMON_OSD2_, (char *)"", 0); 
                    return;
                }
                else
                { 
                    char target_vehicle[32];
                    char other_vehicle[32];

                    memset(target_vehicle, 0, sizeof(target_vehicle));
                    memset(other_vehicle, 0, sizeof(other_vehicle));

                    strncat(target_vehicle, "B:", 2);
                    strncat(other_vehicle,  "W:", 2);
                    
                    for(int i=0; i<5; ++i)
                    {
                        if(0 != strlen((char *)pIve_detection_result->datainfo.carnum[i]))
                        {
                            if(pIve_detection_result->headcount & (1 << i ))
                            {
                                strncat(target_vehicle, (char *)pIve_detection_result->datainfo.carnum[i], 9);
                                strncat(target_vehicle, " ", 1);
                            }
                            else
                            {
                                strncat(other_vehicle, (char *)pIve_detection_result->datainfo.carnum[i], 9);
                                strncat(other_vehicle, " ", 1);
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                    
                    //if(0 != strcmp(m_ext_osd_content[0], target_vehicle))
                    {
                        m_pAv_device->Ad_update_extra_osd_content(_AST_SUB2_VIDEO_, 0, _AON_COMMON_OSD1_, target_vehicle, strlen(target_vehicle));                             
                        m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, 0, _AON_COMMON_OSD1_, target_vehicle, strlen(target_vehicle)); 
                    }
                    //if(0 != strcmp(m_ext_osd_content[1], other_vehicle))
                    {
                        m_pAv_device->Ad_update_extra_osd_content(_AST_SUB2_VIDEO_, 0, _AON_COMMON_OSD2_, other_vehicle, strlen(other_vehicle));                          
                        m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, 0, _AON_COMMON_OSD2_, other_vehicle, strlen(other_vehicle)); 
                    }
                }
            }
        }
    }
#endif
#if defined(_AV_IVE_HC_)
    //add for test
    if((pIve_detection_result->cmdtype == 2) && (1 == pgAvDeviceInfo->Adi_get_ive_debug_mode()))
    {
        char head_count[8];
        memset(head_count, 0 ,sizeof(head_count));
        snprintf(head_count, sizeof(head_count), "NO.%d", pIve_detection_result->headcount);
        m_pAv_device->Ad_update_extra_osd_content(_AST_MAIN_VIDEO_, 0, _AON_COMMON_OSD2_, head_count, strlen(head_count));
        m_pAv_device->Ad_update_extra_osd_content(_AST_SUB_VIDEO_, 0, _AON_COMMON_OSD2_, head_count, strlen(head_count)); 
        m_pAv_device->Ad_update_extra_osd_content(_AST_SUB2_VIDEO_, 0, _AON_COMMON_OSD2_, head_count, strlen(head_count)); 
    }
#endif
    //DEBUG_MESSAGE("the ive detection result is:ive type:%d detection result:%d \n", pIve_detection_result->cmdtype, pIve_detection_result->happen);
#if defined(_AV_IVE_BLE_)
    if(pIve_detection_result->reserved[1] != 0)
    {
        Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_NETWORK_CLIENT_INTELLIGENT_CHECK, 0, (char *) pIve_detection_result, sizeof(msgIPCIntelligentInfoNotice_t), 0);
    }
#else
    Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_NETWORK_CLIENT_INTELLIGENT_CHECK, 0, (char *) pIve_detection_result, sizeof(msgIPCIntelligentInfoNotice_t), 0);
#endif
}
#endif

#endif //end defined(_AV_PRODUCT_CLASS_IPC_)

void CAvAppMaster::Aa_message_handle_SetDebugLevel(const char *msg, int length)
{
    stMessageResponse response;
    response.result = MESSAGE_SET_PARAMETER_ERROR;
    SMessageHead *head = (SMessageHead *) msg;
    DEBUG_MESSAGE("length:%d\n", head->length);
    if (head->length == sizeof(SMsgSetDebugLevel))
    {
        SMsgSetDebugLevel *debug = (SMsgSetDebugLevel *) head->body;
        if (debug->module == AVSTREAM_MODULE)
        {
            DEBUG_SETLEVEL(debug->level, debug->output);
        }
        response.result = SUCCESS;
    }
    Common::N9M_MSGResponseMessage(m_message_client_handle, head, (char *) &response, sizeof(stMessageResponse));
}

//! TTS, Jan 26, 2015
#if !defined(_AV_PRODUCT_CLASS_IPC_)

#if defined(_AV_HAVE_TTS_)
void CAvAppMaster::Aa_message_handle_StartTTS(const char *msg, int length)
{
    DEBUG_WARNING("CAvAppMaster::Aa_message_handle_StartTTS\n");
    
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgAudioPlayPara_t* pAudio_param = (msgAudioPlayPara_t *)pMessage_head->body;
        if (NULL == pAudio_param)
        {
            DEBUG_ERROR("CAvAppMaster::Aa_message_handle_Start_TTS invalid msgAudioPlayPara_t message\n");
            return;
        }
        msgTTSPara_t * pTTS_para = (msgTTSPara_t *)pAudio_param->body;
        m_report_method = 1;
        if (pTTS_para != NULL)
        {    
            
            m_pAv_device_master->Ad_set_audio_mute(1);
            DEBUG_CRITICAL("start TTS type =%d chn=%d, volumn =%d tts_len=%d\n",pAudio_param->uiAudioType, pTTS_para->AOChn, pTTS_para->volumn, pTTS_para->TextLength);

            //validate the input parameter
            if (pTTS_para->AOChn < 0 || pTTS_para->TextLength <=0)
            {
                DEBUG_ERROR("Invalid input arguments!!! TTS chn=%d, tts_len=%d\n",\
                    pTTS_para->AOChn, pTTS_para->TextLength);
                return;
            }
            
            eSystemType subProduct_type = SYSTEM_TYPE_MAX;
            subProduct_type = N9M_GetSystemType();
            if ((SYSTEM_TYPE_A5_AHD_V2 == subProduct_type)||(pgAvDeviceInfo->Adi_product_type() == PTD_M3))
            {
                pTTS_para->AOChn = ((pTTS_para->AOChn==0)?1:0); //! A5-ahd ak4951
            }            
            //FILE * fp = fopen("./recv_tts.txt","wb");
            //fwrite(pTTS_para->TextContent, pTTS_para->TextLength, 1, fp);
            //fclose(fp);
            
            //! for language not supported, do no process
            if (m_pSystem_setting != NULL)
            {
                if (( m_pSystem_setting->stuGeneralSetting.ucLanguageType != 0)\
                    &&( m_pSystem_setting->stuGeneralSetting.ucLanguageType != 1))
                {
                    DEBUG_CRITICAL("Language is not supported!\n");   
                    pTTS_para = NULL;
                }
                else
                {
                    pTTS_para->ucLanguage = m_pSystem_setting->stuGeneralSetting.ucLanguageType;
                }
            }
            
            m_pAv_device_master->Ad_SET_TTS_LOOP_STATE(false);
            if (m_pAv_device_master->Ad_start_tts(pTTS_para) == 0)
            {
                
            }
            else
            {
                DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_START_TTS start tts failed!\n");
            }
           
        }
        else
        {
            DEBUG_ERROR("CAvAppMaster::Aa_message_handle_START_TTS invalid msgTTSPara_t message\n");
        }
    }
    else
    {
        DEBUG_ERROR("CAvAppMaster::Aa_message_handle_START_TTS invalid message length\n");
    }

    return;
}

void CAvAppMaster::Aa_message_handle_StopTTS(const char *msg, int length)
{
    DEBUG_MESSAGE("CAvAppMaster::Aa_message_handle_StopTTS\n");
    
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {   
        msgAudioPlayPara_t* pAudio_param = (msgAudioPlayPara_t *)pMessage_head->body;
        if (NULL == pAudio_param)
        {
            DEBUG_ERROR("CAvAppMaster::Aa_message_handle_Stop_TTS invalid msgAudioPlayPara_t message\n");
            return;
        }
        msgTTSPara_t * pTTS_para = (msgTTSPara_t *)pAudio_param->body;
	if(NULL == pTTS_para)
	{
            DEBUG_ERROR("CAvAppMaster::Aa_message_handle_Stop_TTS pTTS_para is NULL\n");
            return;		
	}
        
        DEBUG_CRITICAL("stop TTS type = %d chn=%d, volumn =%d\n",pAudio_param->uiAudioType, pTTS_para->AOChn, pTTS_para->volumn);

        m_pAv_device_master->Ad_SET_TTS_LOOP_STATE(false);
        if(m_pAv_device_master->Ad_stop_tts() == 0)
        {
            
        }
        else
        {
            DEBUG_ERROR("CAvAppMaster::Aa_message_handle_StopTTS stop tts FAILED!\n");
        }
    }
    else
    {
        DEBUG_ERROR("CAvAppMaster::Aa_message_handle_StopTTS invalid message length\n");
    }

    return;
}
void CAvAppMaster::Aa_message_handle_CallPhoneTTS(const char *msg, int length)
{
#if 1
    //printf("\n\n----CAvAppMaster::Aa_message_handle_CallPhoneTTS\n");
      
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {       
        msgPhoneStatus_t* pPhoneStatus = (msgPhoneStatus_t *)pMessage_head->body;
        if(pPhoneStatus != NULL)
        {
            DEBUG_WARNING("\n\n----CallPhoneTTS body =%s TextLength =%d ucPhoneType =%d ucPhoneStatus=%d\n", pPhoneStatus->body, pPhoneStatus->TextLength, pPhoneStatus->ucPhoneType, pPhoneStatus->ucPhoneStatus);
            if((pPhoneStatus->ucPhoneStatus == 1)||(pPhoneStatus->ucPhoneStatus == 2))
            {
                //! for language not supported, do no process
                if ((m_pSystem_setting!=NULL)&&( m_pSystem_setting->stuGeneralSetting.ucLanguageType != 0)\
                    &&( m_pSystem_setting->stuGeneralSetting.ucLanguageType != 1))
                {
                    DEBUG_CRITICAL("Language is not supported!\n");   
                    return ;
                }
            }

            if(pPhoneStatus->ucPhoneStatus == 1)
            {
                if(pPhoneStatus->TextLength <= 0 )
                {
                    DEBUG_ERROR("TTS Content length is 0\n");
                    return;
                }
            }
            eSystemType subProduct_type = SYSTEM_TYPE_MAX;
            subProduct_type = N9M_GetSystemType();
            
            //! tts type
            msgTTSPara_t * pTTS_para;
            int tts_len = sizeof(msgTTSPara_t) + sizeof(char) * pPhoneStatus->TextLength;
            pTTS_para = (msgTTSPara_t*)malloc(tts_len);
            pTTS_para->TextLength = pPhoneStatus->TextLength;
            memcpy(pTTS_para->TextContent, pPhoneStatus->body, pPhoneStatus->TextLength);              
            pTTS_para->AOChn= 0;
            pTTS_para->volumn = 60;
            pTTS_para->lababit = 4;
            if ((SYSTEM_TYPE_A5_AHD_V2 == subProduct_type)||(pgAvDeviceInfo->Adi_product_type() == PTD_M3))
            {
                pTTS_para->AOChn = ((pTTS_para->AOChn==0)?1:0); //! A5-ahd ak4951
            }    
            
            if (m_pSystem_setting!=NULL)
            {
                pTTS_para->ucLanguage = m_pSystem_setting->stuGeneralSetting.ucLanguageType;
            }

            //! mp3 type
            msgReportStation_t *pMp3_para;
            pMp3_para = (msgReportStation_t *)malloc(sizeof(msgReportStation_t) + 256);
            pMp3_para->audioCount = 1;
            pMp3_para->lababit = 0x04;
            pMp3_para->volumn = 60;
            pMp3_para->RightLeftChn = 0;
            if ((SYSTEM_TYPE_A5_AHD_V2 == subProduct_type)||(pgAvDeviceInfo->Adi_product_type() == PTD_M3))
            {
                pMp3_para->RightLeftChn = ((pMp3_para->RightLeftChn==0)?1:0); //! A5-ahd ak4951
            }    
            memcpy(pMp3_para->AudioAddr[0], pPhoneStatus->body, sizeof(pMp3_para->AudioAddr[0]));
            
            if(pPhoneStatus->ucPhoneType == 2 && pPhoneStatus->ucPhoneStatus == 1)//循环开启tts直到接通电话
            {
                if (pTTS_para != NULL)
                {    
                    DEBUG_WARNING("\n ----start TTS----\n");

                    //validate the input parameter
                    if (pTTS_para->AOChn < 0 || pTTS_para->TextLength <=0)
                    {
                        DEBUG_ERROR("Invalid input arguments!!! TTS chn=%d, tts_len=%d,text=%s\n",\
                            pTTS_para->AOChn, pTTS_para->TextLength,pTTS_para->TextContent);
                        return;
                    }
                    m_pAv_device_master->Ad_SET_TTS_LOOP_STATE(true);//设置循环播放状态
                    if (m_pAv_device_master->Ad_start_tts(pTTS_para) == 0)
                    {
                        
                    }
                    else
                    {
                        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_CallPhoneTTS start tts failed!\n");
                    }
                   
                }
            }
            else if(pPhoneStatus->ucPhoneType == 2&& pPhoneStatus->ucPhoneStatus == 2)// 停止tts
            {   
                {
                    DEBUG_CRITICAL("\n ----stop TTS----\n");
                    m_pAv_device_master->Ad_SET_TTS_LOOP_STATE(false);
                    m_pAv_device_master->Ad_stop_tts();
                }
            }
            else if(pPhoneStatus->ucPhoneType == 2 && pPhoneStatus->ucPhoneStatus == 4) //! mp3铃声
            {
                if (pMp3_para != NULL)
                {    
                    DEBUG_CRITICAL("\n ----start Mp3----\n");

                    //validate the input parameter
                    /*
                    if (pMp3_para->RightLeftChn < 0 || pMp3_para-> <=0)
                    {
                        DEBUG_ERROR("Invalid input arguments!!! TTS chn=%d, tts_len=%d,text=%s\n",\
                            pTTS_para->AOChn, pTTS_para->TextLength,pTTS_para->TextContent);
                        return;
                    }*/
                    m_pAv_device_master->Ad_loop_report_station(true);//设置循环播放状态
                    m_pAv_device_master->Ad_add_report_station_task(pMp3_para);
                    if (m_pAv_device_master->Ad_start_report_station() == 0)
                    {
                        
                    }
                    else
                    {
                        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_CallPhoneTTS start tts failed!\n");
                    }
                   
                }
            }
            else if(pPhoneStatus->ucPhoneType == 2&& pPhoneStatus->ucPhoneStatus == 5)// 停止mp3
            {   
                {
                    DEBUG_CRITICAL("\n ----stop mp3----\n");
                    m_pAv_device_master->Ad_loop_report_station(false);
                    m_pAv_device_master->Ad_stop_report_station();
                }
            }
            free(pTTS_para);
            free(pMp3_para);
        }
    }
    else
    {
        DEBUG_ERROR("CAvAppMaster::Aa_message_handle_CallPhoneTTS invalid message length\n");
    }
#endif
    return;
}

#endif

#endif

#if defined(_AV_HAVE_VIDEO_INPUT_)

#if defined(_AV_PRODUCT_CLASS_HDVR_)

void CAvAppMaster::Aa_message_handle_SET_VIDEO_ATTR(const char *msg, int length)
{
    DEBUG_CRITICAL("Aa_message_handle_SET_VIDEO_ATTR set flip mirror\n");
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgCameraAttributeQuickSet_t * pVideoAttr = (msgCameraAttributeQuickSet_t *)pMessage_head->body;

        if (pVideoAttr != NULL)
        {
            if (m_start_work_flag !=0)
            {               
                printf("channel mask is=%lld\n", pVideoAttr->uiChannelMask);
                for (int i=0;i<SUPPORT_MAX_VIDEO_CHANNEL_NUM; i++) //SUPPORT_MAX_VIDEO_CHANNEL_NUM
                {                    
                    if (pVideoAttr->uiChannelMask & (1LL<<i))
                    {
                        if (((pVideoAttr->stuViParam[i].uiParamMask >>11) & 0x1) || ((pVideoAttr->stuViParam[i].uiParamMask>>12) & 0x1))
                        {
                            printf("set video attr %d\n", i);
                            m_pAv_device_master->Ad_set_video_attr(i, pVideoAttr->stuViParam[i].image.u8Mirror, pVideoAttr->stuViParam[i].image.u8Flip);
                           printf("flip=%d, mirror=%d\n", pVideoAttr->stuViParam[i].image.u8Flip, pVideoAttr->stuViParam[i].image.u8Mirror);
                           m_vi_config_set[i].u8Flip = pVideoAttr->stuViParam[i].image.u8Flip;
                           m_vi_config_set[i].u8Mirror = pVideoAttr->stuViParam[i].image.u8Mirror;
                        }
                    }
                } 
            }
            else //! save the parameters
            {
                for (int i=0;i<SUPPORT_MAX_VIDEO_CHANNEL_NUM; i++) //SUPPORT_MAX_VIDEO_CHANNEL_NUM
                {                    
                    if (pVideoAttr->uiChannelMask & (1LL<<i))
                    {
                        if (((pVideoAttr->stuViParam[i].uiParamMask >>11) & 0x1) || ((pVideoAttr->stuViParam[i].uiParamMask>>12) & 0x1))
                        {
                           //printf("flip=%d, mirror=%d\n", pVideoAttr->stuViParam[i].image.u8Flip, pVideoAttr->stuViParam[i].image.u8Mirror);
                           m_vi_config_set[i].u8Flip = pVideoAttr->stuViParam[i].image.u8Flip;
                           m_vi_config_set[i].u8Mirror = pVideoAttr->stuViParam[i].image.u8Mirror;
                        }
                    }
                } 
            }
                        
        }
        else
        {
            DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_START_TALKBACK invalid message\n");
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_START_TALKBACK invalid message length\n");
    }
    m_start_work_env[MESSAGE_CONFIG_MANAGE_CAMERA_ATTRIBUTE_SET] = true;

    return;

}


void CAvAppMaster::Aa_message_handle_SET_VIDEO_FORMAT(const char *msg, int length)
{
    DEBUG_CRITICAL("Aa_message_handle_SET_VIDEO_FORMAT\n");
    SMessageHead *pMessage_head = (SMessageHead*)msg;
    if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
    {
        msgInputVideoFormat_t * pVideoFormat = (msgInputVideoFormat_t *)pMessage_head->body;

        if (pVideoFormat != NULL)
        {
  	     int max_vi_chn_num = pgAvDeviceInfo->Adi_get_vi_chn_num();
	     int invalid_chn_num;		
            for (invalid_chn_num=0;invalid_chn_num<max_vi_chn_num; invalid_chn_num++) 
            {                    
                if(pVideoFormat->InputFormat[invalid_chn_num]!=0)
                {
			break;
                }
            } 
			
            if((pVideoFormat->InputFormatValidMask==0)||(invalid_chn_num==max_vi_chn_num))
            {
            		printf("InputFormatValidMask=%d,invalid_chn_num=%d\n",pVideoFormat->InputFormatValidMask,invalid_chn_num);
			m_start_work_env[MESSAGE_DEVICE_MANAGER_AHD_VIDEO_FORMAT_GET] = true;
			return;
	     }
				
            if (m_start_work_flag !=0)
            {               
                printf("InputFormatValidMask 111 is=%x,[%d][%d][%d][%d][%d][%d][%d][%d]\n", pVideoFormat->InputFormatValidMask,
					pVideoFormat->InputFormat[0],pVideoFormat->InputFormat[1],pVideoFormat->InputFormat[2],
					pVideoFormat->InputFormat[3],pVideoFormat->InputFormat[4],pVideoFormat->InputFormat[5],
					pVideoFormat->InputFormat[6],pVideoFormat->InputFormat[7]);
				  
                for (int i=0;i<max_vi_chn_num; i++) 
                {                    
                   // if (pVideoFormat->InputFormatValidMask & (1LL<<i))
                    {
                            printf("set video format 111 %d\n", i);
                            pgAvDeviceInfo->Adi_set_ahd_video_format(i, pVideoFormat->InputFormatValidMask,pVideoFormat->InputFormat[i]);
                    }
                } 
		if(pgAvDeviceInfo->Adi_is_ahd_video_format_changed()==true)
		{
#if 1
                 unsigned char streamtype_mask = Aa_get_all_streamtype_mask();
			     unsigned char chn_mask = pgAvDeviceInfo->Adi_get_ahd_chn_mask()&0xff;
			     printf("\nliutest,streamtype_mask=%x,~videoLost=%x,chn_mask=%x\n",streamtype_mask,~m_alarm_status.videoLost,chn_mask);

			    for(int chn_num=0;chn_num<max_vi_chn_num;chn_num++)
			    {
			        if(_VS_DIGITAL_ == pgAvDeviceInfo->Adi_get_video_source(chn_num))
			        {
			            continue;
			        }
					
				if(m_pMain_stream_encoder->stuVideoEncodeSetting[chn_num].ucChnEnable ==0)
				{
				        chn_mask = ((chn_mask) & (~(1ll << (chn_num))));	
				}
			    }
			     printf("\nliutest111,videoLost=%x,chn_mask=%x\n",~m_alarm_status.videoLost,chn_mask);
				 
		            Aa_stop_selected_streamtype_encoders(chn_mask, streamtype_mask);
#ifdef _AV_PLATFORM_HI3520D_V300_
			     Aa_stop_selected_vi(chn_mask);
			     Aa_start_selected_vi(chn_mask);
#else
                 Aa_stop_vi();
			     Aa_start_vi();		
#endif
			    printf("111,restart vi and encoder\n");			   

		            Aa_start_selected_streamtype_encoders(chn_mask, streamtype_mask);		 
#endif
		}
            }
            else //! save the parameters
            {
                printf("InputFormatValidMask 222 is=%x,[%d][%d][%d][%d][%d][%d][%d][%d]\n", pVideoFormat->InputFormatValidMask,
					pVideoFormat->InputFormat[0],pVideoFormat->InputFormat[1],pVideoFormat->InputFormat[2],
					pVideoFormat->InputFormat[3],pVideoFormat->InputFormat[4],pVideoFormat->InputFormat[5],
					pVideoFormat->InputFormat[6],pVideoFormat->InputFormat[7]);
#if 1
                for (int i=0;i<max_vi_chn_num; i++) //SUPPORT_MAX_VIDEO_CHANNEL_NUM
                {                    
                    //if (pVideoFormat->InputFormatValidMask & (1LL<<i))
                    {
                            //printf("set video format 222, %d\n", i);
                            pgAvDeviceInfo->Adi_set_ahd_video_format(i, pVideoFormat->InputFormatValidMask,pVideoFormat->InputFormat[i]);
                    }
                } 
		if(pgAvDeviceInfo->Adi_is_ahd_video_format_changed()==true)
		{
			pgAvDeviceInfo->Adi_get_ahd_chn_mask();
#if 0
                          unsigned char streamtype_mask = Aa_get_all_streamtype_mask();
		            Aa_stop_selected_streamtype_encoders(pgAvDeviceInfo->Aa_get_all_analog_chn_mask(), streamtype_mask);
                          Aa_stop_vi();
			    printf("222,restart vi and encoder\n");			   
			    //mSleep(50);
			     Aa_start_vi();		
		            Aa_start_selected_streamtype_encoders(pgAvDeviceInfo->Aa_get_all_analog_chn_mask(), streamtype_mask);		 
#else
			printf("222,m_start_work_flag=%d, wait\n",m_start_work_flag);
#endif
		}				
#else
		 printf("m_start_work_flag=%d, wait\n",m_start_work_flag);
#endif
            }
                        
        }
        else
        {
            DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_SET_VIDEO_FORMAT invalid message\n");
        }
    }
    else
    {
        DEBUG_ERROR( "CAvAppMaster::Aa_message_handle_SET_VIDEO_FORMAT invalid message length\n");
    }
    m_start_work_env[MESSAGE_DEVICE_MANAGER_AHD_VIDEO_FORMAT_GET] = true;

    return;

}

#endif //_AV_PRODUCT_CLASS_HDVR_
#if defined(_AV_PLATFORM_HI3515_)
int CAvAppMaster::Aa_6a1_check_audio_fault(int chn, unsigned int* pval)
{
	#define AI_DATABUF_ADDR 0x2000200
	#define TMP_BUFSIZE 1024
	int fp_ai;
	char * ptr=NULL,*p=NULL;
	char		buf[TMP_BUFSIZE];
	char aibuf[TMP_BUFSIZE];
	unsigned char AiDev,AiChn,Read,Write,BufFul,AecFail,i;
	char State[16],AecAo[16];
	unsigned int u32Data0,u32Data1;
	int ailen,skiplen;
	static unsigned int dataAddr=AI_DATABUF_ADDR;

	fp_ai=open("/proc/umap/ai",O_RDONLY);
	if(fp_ai<0)
	{
		printf("open /mnt/tmpfs/ai file error\n");
		return 0;
	}
	ailen=read(fp_ai, aibuf, TMP_BUFSIZE);
	if(ailen<=0)
	{
		printf("read error\n");
		return 0;
	}
	close(fp_ai);

	*pval=0;
	p=aibuf;
	
	while(ailen>0)
	{
		skiplen=Get_alinedata(p,buf,TMP_BUFSIZE);
		if(skiplen<=0)
		{
			break;
		}
		ailen-=skiplen;
		p+=skiplen;
		if(strstr(buf, "AI CHN STATUS")!=NULL)
		{
			skiplen=Get_alinedata(p,buf,TMP_BUFSIZE);
			if(skiplen<=0)
			{
				break;
			}
			ailen-=skiplen;
			p+=skiplen;

			while(ailen>0)
			{
				skiplen=Get_alinedata(p,buf,TMP_BUFSIZE);
				if(skiplen<=0)
				{
					break;
				}
				ailen-=skiplen;
				p+=skiplen;
				ptr=buf;
				AiDev=AiChn=Read=Write=BufFul=AecFail=0;
				u32Data0=u32Data1=0;

				//get AiDev
				while(' ' == *ptr)
				{
					ptr++;
				}
				while((' ' != *ptr)&&(*ptr!='\0'))
				{
					if(((*ptr>='0')&&(*ptr<='9'))||((*ptr>='a')&&(*ptr<='f'))||((*ptr>='A')&&(*ptr<='F')))
						AiDev = AiDev*10+(*ptr-0x30);
					ptr++;
				}

				//get AiChn
				while(' ' == *ptr)
				{
					ptr++;
				}
				while((' ' != *ptr)&&(*ptr!='\0'))
				{
					if(((*ptr>='0')&&(*ptr<='9'))||((*ptr>='a')&&(*ptr<='f'))||((*ptr>='A')&&(*ptr<='F')))
						AiChn = AiChn*10+(*ptr-0x30);
					ptr++;
				}

				if(AiChn != chn)
				{
					continue;
				}
				
				//get State
				while(' ' == *ptr)
				{
					ptr++;
				}
				i=0;
				while((' ' != *ptr)&&(*ptr!='\0'))
				{
					State[i++]=*ptr;
					ptr++;
				}
				State[i]='\0';

				//get Read
				while(' ' == *ptr)
				{
					ptr++;
				}
				while((' ' != *ptr)&&(*ptr!='\0'))
				{
					if(((*ptr>='0')&&(*ptr<='9'))||((*ptr>='a')&&(*ptr<='f'))||((*ptr>='A')&&(*ptr<='F')))
						Read = Read*10+(*ptr-0x30);
					ptr++;
				}

				//get Write
				while(' ' == *ptr)
				{
					ptr++;
				}
				while((' ' != *ptr)&&(*ptr!='\0'))
				{
					if(((*ptr>='0')&&(*ptr<='9'))||((*ptr>='a')&&(*ptr<='f'))||((*ptr>='A')&&(*ptr<='F')))
						Write = Write*10+(*ptr-0x30);
					ptr++;
				}

				//get BufFul
				while(' ' == *ptr)
				{
					ptr++;
				}
				while((' ' != *ptr)&&(*ptr!='\0'))
				{
					if(((*ptr>='0')&&(*ptr<='9'))||((*ptr>='a')&&(*ptr<='f'))||((*ptr>='A')&&(*ptr<='F')))
						BufFul = BufFul*10+(*ptr-0x30);
					ptr++;
				}

				//get AecAo
				while(' ' == *ptr)
				{
					ptr++;
				}
				i=0;
				while((' ' != *ptr)&&(*ptr!='\0'))
				{
					AecAo[i++]=*ptr;
					ptr++;
				}
				AecAo[i]='\0';
				
				//get AecFail
				while(' ' == *ptr)
				{
					ptr++;
				}
				while((' ' != *ptr)&&(*ptr!='\0'))
				{
					if(((*ptr>='0')&&(*ptr<='9'))||((*ptr>='a')&&(*ptr<='f'))||((*ptr>='A')&&(*ptr<='F')))
						AecFail = AecFail*10+(*ptr-0x30);
					ptr++;
				}
				
				//get u32Data0
				while(' ' == *ptr)
				{
					ptr++;
				}
				while((' ' != *ptr)&&(*ptr!='\0'))
				{
					if(((*ptr>='0')&&(*ptr<='9')))
					{
						u32Data0 = (u32Data0<<4)|(*ptr-0x30);
					}
					else if(((*ptr>='a')&&(*ptr<='f'))||((*ptr>='A')&&(*ptr<='F')))
					{
						u32Data0 = (u32Data0<<4)|(*ptr);
					}
					ptr++;
				}

				//get u32Data1
				while(' ' == *ptr)
				{
					ptr++;
				}
				while((' ' != *ptr)&&(*ptr!='\0'))
				{
					if(((*ptr>='0')&&(*ptr<='9')))
					{
						u32Data1 = (u32Data1<<4)|(*ptr-0x30);
					}
					else if(((*ptr>='a')&&(*ptr<='f'))||((*ptr>='A')&&(*ptr<='F')))
					{
						u32Data1 = (u32Data1<<4)|(*ptr);
					}
						//u32Data1 = u32Data1*10+(*ptr-0x30);
					ptr++;
				}
				//printf("===AiDev:%d,AiChn:%d,State:%s,Read:%d,Write:%d,u32Data0:%x,u32Data1:%x\n",AiDev,AiChn,State,Read,Write,u32Data0,u32Data1);

				//audio is encoder in chn2 ,but actula audio is come from chn8 
				if(AiChn==chn)
				{
					if(strcmp(State,"enable")==0)
					{
						if(u32Data0==u32Data1)
							dataAddr=u32Data0;
						
						if((u32Data0==u32Data1)||(u32Data0==dataAddr)||(u32Data1==dataAddr)||(u32Data0==AI_DATABUF_ADDR)||(u32Data1==AI_DATABUF_ADDR))
						//if((u32Data0==u32Data1))//||(u32Data0==dataAddr)&&(u32Data1==dataAddr))//||(u32Data0==AI_DATABUF_ADDR)||(u32Data1==AI_DATABUF_ADDR))
						{
							//audio lost
							*pval |= (1<<7); 
							//printf("lost audio\n");
						}

						return 1;
					}

					break;
				}
				
			}
			
			break;
		}
	}

	return 0;
}

void CAvAppMaster::Aa_message_handle_Get_6a1_audio_fault(const char *msg, int length)
{
    SMessageHead *pMessage_head = (SMessageHead*)msg;

	msgPickupStat_t msgState;
	memset(&msgState, 0x0, sizeof(msgPickupStat_t));
	msgState.PickupStat = m_nPickupStat;
	Common::N9M_MSGResponseMessage(m_message_client_handle, pMessage_head, (char*)&msgState, sizeof(msgPickupStat_t));
}

int CAvAppMaster::Aa_6a1_broadcast_audio_fault(int fault)
{
	fault = (fault == 0) ? 1 : 0;
	if(fault != m_nPickupStat)
	{
		msgPickupStat_t msgFault;
		memset(&msgFault, 0x0, sizeof(msgPickupStat_t));
		msgFault.PickupStat = fault;

		DEBUG_CRITICAL("Aa_6a1_broadcast_audio_fault, lastFault = %d, new = %d\n", m_nPickupStat, fault);
		m_nPickupStat = fault;

		Common::N9M_MSGSendMessage(m_message_client_handle, 0, MESSAGE_DEVICE_MANAGER_SET_PICKUP_STATUS, 0, (char*)&msgFault, sizeof(msgPickupStat_t), 0);
	}
	return 0;
}

int CAvAppMaster::Get_alinedata(char *socbuf, char *disbuf,int size)
{
	int i=0;
	for(i=0;i<size;i++)
	{
		if((socbuf[i]=='\n')||(socbuf[i]=='\0'))
			break;
		
		disbuf[i]=socbuf[i];
	}
	disbuf[i]='\0';
	i++;
	return i;
}
#endif
#endif //_AV_HAVE_VIDEO_INPUT_

#if !defined(_AV_PRODUCT_CLASS_IPC_)&& !defined(_AV_PRODUCT_CLASS_DVR_REI_)
void CAvAppMaster::Aa_message_handle_Beijing_bus_station(const char* msg, int length )
{
	DEBUG_CRITICAL("Aa_message_handle_Beijing_bus_station\n");
	SMessageHead *pMessage_head = (SMessageHead*)msg;
	if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
	{
		msgBeijingBusStationData_t * pStationInfo = (msgBeijingBusStationData_t *)pMessage_head->body;

		if (pStationInfo != NULL)
		{
			char string_temp[1024] = {0};
			memcpy(string_temp, pStationInfo->body, (pStationInfo->uiDataLen >= sizeof(string_temp)) ? (sizeof(string_temp) - 2) : pStationInfo->uiDataLen);
			DEBUG_CRITICAL("BEIJING_BUS: Recv station info, dataState[%d], stnState[%d], datalen[%d], data[%s]\n",\
				pStationInfo->ucDataState, pStationInfo->ucStnState, pStationInfo->uiDataLen, string_temp);

			if(pStationInfo->uiDataLen <= 0 || pStationInfo->ucDataState != 1)	//清空字符串///
			{
				memset(string_temp, 0x0, sizeof(string_temp));
			}

			char utfString[1024] = {0};
			if(GBKToUTF8(string_temp, utfString) <= 0)
			{
				DEBUG_ERROR("BEIJING_BUS: GBKToUTF8 failed\n");
			}
			else
			{
				DEBUG_CRITICAL("BEIJING_BUS: newStr = %s, oldStr = %s\n", utfString, pgAvDeviceInfo->Adi_get_beijing_station_info().c_str());
				if(strcmp(utfString, pgAvDeviceInfo->Adi_get_beijing_station_info().c_str()) != 0)
				{
					pgAvDeviceInfo->Adi_set_beijing_station_info(utfString);
					
					av_video_encoder_para_t video_encoder_para;
					for (int chn = 0; chn < pgAvDeviceInfo->Adi_max_channel_number(); chn++)
					{
						int phy_audio_chn_num = chn;
						pgAvDeviceInfo->Adi_get_audio_chn_id(chn , phy_audio_chn_num);
					
						/*main stream osd*/
						if (m_main_encoder_state.test(chn))
						{
							Aa_get_encoder_para(_AST_MAIN_VIDEO_, chn, &video_encoder_para, phy_audio_chn_num, NULL);
							m_pAv_device->Ad_modify_encoder(_AST_MAIN_VIDEO_, chn, &video_encoder_para);
						}
					
						/*sub stream osd*/
						if (m_sub_encoder_state.test(chn))
						{
							Aa_get_encoder_para(_AST_SUB_VIDEO_, chn, &video_encoder_para, phy_audio_chn_num, NULL);
							m_pAv_device->Ad_modify_encoder(_AST_SUB_VIDEO_, chn, &video_encoder_para);
						}
					
						/*sub stream osd*/
						if (m_sub2_encoder_state.test(chn))
						{
							Aa_get_encoder_para(_AST_SUB2_VIDEO_, chn, &video_encoder_para, phy_audio_chn_num, NULL);							 
							m_pAv_device->Ad_modify_encoder(_AST_SUB2_VIDEO_, chn, &video_encoder_para);
						}
					}
				}
			}
		}
	}
	
}

void CAvAppMaster::Aa_message_handle_APC_Number(const char* msg, int length )
{
	DEBUG_CRITICAL("Aa_message_handle_APC_Number\n");
	SMessageHead *pMessage_head = (SMessageHead*)msg;
	if((pMessage_head->length + sizeof(SMessageHead)) == (unsigned int)length)
	{
        msgEthernetDevApcNumToServer_t * pApcNum = (msgEthernetDevApcNumToServer_t *)pMessage_head->body;
        if (pApcNum != NULL)
        {
            DEBUG_CRITICAL("Onboard:%d\n", pApcNum->onboardingnum);
            pgAvDeviceInfo->Adi_set_apc_num(* pApcNum);
        }
	}
    else
    {
        
    }
    return;
}
#endif


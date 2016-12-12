#include "AvStreamingMasterVersion.h"
#include "AvAppMaster.h"
#include "AvSignal.h"
#include "SharePts.h"

#if !defined(_AV_PRODUCT_CLASS_IPC_)
#include "remote_operate_file_api.h"
#endif

using namespace Common;
int main(int argc, char *argv[])
{
    CAvAppMaster *pAv_app_master = NULL;

    printf("\n========================================================\n");
    printf("  avStreaming-Master Built time:%s %s\n", __DATE__, __TIME__);
    printf("========================================================\n");

    /*debug level*/
    Av_debug_level(0xffffffff, 0xffffffff);
    DEBUG_SETLEVEL( LEVEL_CRITICAL, DEBUG_OUTPUT_STD );

    /*version string*/
    _AV_KEY_INFO_(_AVD_APP_, "%s", _avStreaming_VERSION_STR_);

    /*register signal handle*/
    Register_av_signal_handle();
#if !defined(_AV_PRODUCT_CLASS_IPC_)
    api_rof_init_client();
#endif
    /*new object*/
    pAv_app_master = new CAvAppMaster;
    _AV_FATAL_(pAv_app_master == NULL, "OUT OF MEMORY\n");
    pAv_app_master->Aa_construct_message_client();
    /*load config*/
    _AV_FATAL_(pAv_app_master->Aa_load_system_config(argc, argv) != 0, "(pAv_app_master->Aa_load_system_config(argc, argv) != 0)\n");

    /*init*/
    if(pAv_app_master->Aa_init() != 0)
    {
        DEBUG_ERROR( "AvStreamingMaster (pAv_app_master->Aa_init() != 0)\n");
        _AV_SAFE_DELETE_(pAv_app_master);
        return -1;
    }

#if defined(_AV_PRODUCT_CLASS_HDVR_) || defined(_AV_PRODUCT_CLASS_NVR_) || defined(_AV_PRODUCT_CLASS_DECODER_)    
    //init system pts, make sure that linux time has been setted
    AV_PTS::InitSystemPts(true); //true for avStreaming was collepsed and restart 
    AV_PTS::SetPtsToShareMemory();
#else
    //init system pts, make sure that linux time has been setted  
    if(((PTD_712_VD == pgAvDeviceInfo->Adi_product_type()) && (SYSTEM_TYPE_RAILWAYIPC_MASTER == N9M_GetSystemType())))
    {       
        AV_PTS::InitSystemPts(true); //true for avStreaming was collepsed and restart 
        AV_PTS::SetPtsToShareMemory();
    }
    else if((PTD_712_VF == pgAvDeviceInfo->Adi_product_type()))
    {
        AV_PTS::InitSystemPts(false); //true for avStreaming was collepsed and restart 
        AV_PTS::SetPtsToShareMemory();        
    }
    else
    {
        AV_PTS::InitSystemPts(false);
    }
#endif
    
    printf("init success\n");

    /*wait system ready*/
    if(pAv_app_master->Aa_wait_system_ready() != 0)
    {
        DEBUG_ERROR("AvStreamingMaster (pAv_app_master->Aa_wait_system_ready() != 0)\n");
        _AV_SAFE_DELETE_(pAv_app_master);
        return -1;
    }

    printf("wait system success\n");

    /*construct communication component*/
    if(pAv_app_master->Aa_construct_communication_component() != 0)
    {
        DEBUG_ERROR("AvStreamingMaster (pAv_app_master->Aa_construct_communication_component() != 0)\n");
        _AV_SAFE_DELETE_(pAv_app_master);
        return -1;
    }    
    printf("conmmunitcation system success\n");

    /*message*/
    pAv_app_master->Aa_message_thread();

    _AV_SAFE_DELETE_(pAv_app_master);

    return 0;
}


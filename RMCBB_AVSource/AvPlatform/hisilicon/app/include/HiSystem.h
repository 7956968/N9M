/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __HISYSTEM_H__
#define __HISYSTEM_H__
#include "AvDeviceInfo.h"
#include "mpi_sys.h"
#include "mpi_vb.h"
#include "CommonDebug.h"


#define _HI_SYS_ALIGN_BYTE_(align_num, align_byte) ((align_num + align_byte - 1) / align_byte * align_byte)
class CHiSystem{
    public:
        CHiSystem();
        
        ~CHiSystem();
        
        int HiSys_init();
        
        int HiSys_uninit();

    private:
        int HiSys_get_vb_config_on_single_ddr(unsigned int vi_num_sd, unsigned int vi_width_sd, unsigned int vi_hight_sd,unsigned int vi_num_hd, unsigned int vi_width_hd, unsigned int vi_hight_hd, VB_CONF_S *pvb_config);

        int HiSys_get_vb_config_on_two_ddr(unsigned int vi_num, unsigned int vi_width, unsigned int vi_hight, VB_CONF_S *pvb_config);

	 int HiSys_get_vb_config_BC_NVR(VB_CONF_S *pVb_config);

#if defined(_AV_PRODUCT_CLASS_IPC_)
        int HiSys_get_vb_config_IPC_720P(VB_CONF_S &pVb_config);
        int HiSys_get_vb_config_IPC_1080P(VB_CONF_S &pVb_config);
        int HiSys_get_vb_config_IPC_5M(VB_CONF_S &pVb_config);
        int HiSys_get_vb_config_IPC_720P_FOR_S1(VB_CONF_S &pVb_config);
#if defined(_AV_PLATFORM_HI3518EV200_)
        int HiSys_get_vb_config_IPC_for_3518EV200(VB_CONF_S &pVb_config);
#endif
#endif

};

#endif/*__HISYSTEM_H__*/


#include "HiSystem-3515.h"
#include "mpi_vi.h"
#if defined(_AV_PLATFORM_HI3515_)
#include "hi_math.h"
#endif
using namespace Common;
CHiSystem::CHiSystem()
{

}

CHiSystem::~CHiSystem()
{
    HiSys_uninit();
}

int CHiSystem::HiSys_get_vb_config_BC_NVR(VB_CONF_S *pVb_config)
{
    pVb_config->u32MaxPoolCnt = 128;
    DEBUG_MESSAGE("pgAvDeviceInfo->Adi_get_WD1_width()=%d\n",pgAvDeviceInfo->Adi_get_WD1_width());
    pVb_config->astCommPool[0].u32BlkSize = (HI_U32)(CEILING_2_POWER( pgAvDeviceInfo->Adi_get_WD1_width(), 64) * 576 * 1.5);
    pVb_config->astCommPool[0].u32BlkCnt = 12 * 8;/*8 for each channel*/

    pVb_config->astCommPool[1].u32BlkSize = (HI_U32)(CEILING_2_POWER( pgAvDeviceInfo->Adi_get_WD1_width()/2, 64) * 288 * 1.5);
    pVb_config->astCommPool[1].u32BlkCnt = 2 * 8;/*8 for each channel*/

    return 0;
}

int CHiSystem::HiSys_get_vb_config_on_single_ddr(unsigned int vi_num_sd, unsigned int vi_width_sd, unsigned int vi_hight_sd,unsigned int vi_num_hd, unsigned int vi_width_hd, unsigned int vi_hight_hd, VB_CONF_S *pvb_config)
{
    if((pvb_config == NULL) || ((vi_num_sd + vi_num_hd) < 1))
    {
        _AV_FATAL_(1, "para is invalid(vi_num_sd=%d, vi_num_hd=%d, pvb_config=%p)!\n", vi_num_sd, vi_width_sd, pvb_config);
        return -1;
    }
    int vb_index = 0;
    memset(pvb_config, 0, sizeof(VB_CONF_S));
    pvb_config->u32MaxPoolCnt = 128;
    
    /*vi(vi_num * 6)*/
	if(vi_num_sd > 0)
	{
    	pvb_config->astCommPool[vb_index].u32BlkSize = (HI_U32)(_HI_SYS_ALIGN_BYTE_(vi_width_sd, 64) * _HI_SYS_ALIGN_BYTE_(vi_hight_sd, 64) * pgAvDeviceInfo->Adi_get_pixel_size());
    	pvb_config->astCommPool[vb_index].u32BlkCnt = vi_num_sd * 6;
    	vb_index ++;
	}

	if(vi_num_hd > 0)
	{
	    pvb_config->astCommPool[vb_index].u32BlkSize = (HI_U32)(_HI_SYS_ALIGN_BYTE_(vi_width_hd, 64) * _HI_SYS_ALIGN_BYTE_(vi_hight_hd, 64) * pgAvDeviceInfo->Adi_get_pixel_size());
	    pvb_config->astCommPool[vb_index].u32BlkCnt = vi_num_hd * 6;
	    vb_index ++;
	}
	
    /*wbc(4)、user pic(3)*/
    pvb_config->astCommPool[vb_index].u32BlkSize = (HI_U32)(_HI_SYS_ALIGN_BYTE_(720, 64) * _HI_SYS_ALIGN_BYTE_(576, 64) * pgAvDeviceInfo->Adi_get_pixel_size());
    pvb_config->astCommPool[vb_index].u32BlkCnt = 4 + 3;
    vb_index ++;

    /*vda(vi_num * 4)*/
    pvb_config->astCommPool[vb_index].u32BlkSize = (HI_U32)(_HI_SYS_ALIGN_BYTE_(360, 64) * _HI_SYS_ALIGN_BYTE_(288, 64) * pgAvDeviceInfo->Adi_get_pixel_size());
    pvb_config->astCommPool[vb_index].u32BlkCnt = (vi_num_sd + vi_num_hd) * 4;
    vb_index ++;

    return 0;
}

int CHiSystem::HiSys_get_vb_config_on_two_ddr(unsigned int vi_num, unsigned int vi_width, unsigned int vi_hight, VB_CONF_S *pvb_config)
{
    if((vi_num < 1) || (vi_width < 0) || (vi_hight < 0) || (pvb_config == NULL))
    {
        _AV_FATAL_(1, "para is invalid(vi_num=%d, vi_width=%d, vi_hight=%d, pvb_config=%p)!\n", vi_num, vi_width, vi_hight, pvb_config);
        return -1;
    }
    int vb_index = 0;
    int ddr0_chn_num = 0;
    int ddr1_chn_num = 0;

    ddr0_chn_num = vi_num / 3;
    ddr1_chn_num = vi_num - ddr0_chn_num;
    memset(pvb_config, 0, sizeof(VB_CONF_S));
    pvb_config->u32MaxPoolCnt = 128;
    
    /*ddr0:vi*/
    pvb_config->astCommPool[vb_index].u32BlkSize = (HI_U32)(_HI_SYS_ALIGN_BYTE_(vi_width, 64) * _HI_SYS_ALIGN_BYTE_(vi_hight, 64) * pgAvDeviceInfo->Adi_get_pixel_size());
    pvb_config->astCommPool[vb_index].u32BlkCnt = ddr0_chn_num * 6;
    vb_index ++;

    pvb_config->astCommPool[vb_index].u32BlkSize = (HI_U32)(_HI_SYS_ALIGN_BYTE_(vi_width, 64) * _HI_SYS_ALIGN_BYTE_(vi_hight, 64) * pgAvDeviceInfo->Adi_get_pixel_size());
    pvb_config->astCommPool[vb_index].u32BlkCnt = ddr1_chn_num * 6;
#if !defined(_AV_PLATFORM_HI3515_)
    strcpy(pvb_config->astCommPool[vb_index].acMmzName, "ddr1");
#endif
    vb_index ++;

    /*wbc(4)、user pic(3)*/
    pvb_config->astCommPool[vb_index].u32BlkSize = (HI_U32)(_HI_SYS_ALIGN_BYTE_(720, 64) * _HI_SYS_ALIGN_BYTE_(576, 64) * pgAvDeviceInfo->Adi_get_pixel_size());
    pvb_config->astCommPool[vb_index].u32BlkCnt = 4 + 3;
    vb_index ++;

    /*vda*/
    pvb_config->astCommPool[vb_index].u32BlkSize = (HI_U32)(_HI_SYS_ALIGN_BYTE_(360, 64) * _HI_SYS_ALIGN_BYTE_(288, 64) * pgAvDeviceInfo->Adi_get_pixel_size());
    pvb_config->astCommPool[vb_index].u32BlkCnt = ddr0_chn_num * 4;
    vb_index ++;

    pvb_config->astCommPool[vb_index].u32BlkSize = (HI_U32)(_HI_SYS_ALIGN_BYTE_(360, 64) * _HI_SYS_ALIGN_BYTE_(288, 64) * pgAvDeviceInfo->Adi_get_pixel_size());
    pvb_config->astCommPool[vb_index].u32BlkCnt = ddr1_chn_num * 4;
#if !defined(_AV_PLATFORM_HI3515_)
    strcpy(pvb_config->astCommPool[vb_index].acMmzName, "ddr1");
#endif
    vb_index ++;

    return 0;
}

int CHiSystem::HiSys_get_vb_config_A5_II(unsigned int vi_num_sd, unsigned int vi_width_sd, unsigned int vi_hight_sd,VB_CONF_S *pvb_config)
{
    if((pvb_config == NULL) || (vi_num_sd < 1))
    {
        _AV_FATAL_(1, "para is invalid(vi_num_sd=%d, vi_num_hd=%d, pvb_config=%p)!\n", vi_num_sd, vi_width_sd, pvb_config);
        return -1;
    }
    int vb_index = 0;
    memset(pvb_config, 0, sizeof(VB_CONF_S));
    pvb_config->u32MaxPoolCnt = 128;

	printf("vi_num_sd:%d vi_width_sd:%d vi_hight_sd:%d\n",vi_num_sd,vi_width_sd,vi_hight_sd);
#if 1
    if (vi_num_sd >=4)
    {
        pvb_config->astCommPool[0].u32BlkSize = _HI_SYS_ALIGN_BYTE_(720, 64) * _HI_SYS_ALIGN_BYTE_(576, 64) * 2;/*D1*/   //!<2
        pvb_config->astCommPool[0].u32BlkCnt  = vi_num_sd * 6;

        pvb_config->astCommPool[1].u32BlkSize = _HI_SYS_ALIGN_BYTE_(720, 64) * _HI_SYS_ALIGN_BYTE_(288, 64) * 2;/*HD1*/   //!< 2
        pvb_config->astCommPool[1].u32BlkCnt  = 8;

        pvb_config->astCommPool[2].u32BlkSize = _HI_SYS_ALIGN_BYTE_(360, 64) * _HI_SYS_ALIGN_BYTE_(288, 64) * 2;/*CIF*/
        pvb_config->astCommPool[2].u32BlkCnt  = 8;
    }
    else
    {
        pvb_config->astCommPool[0].u32BlkSize = _HI_SYS_ALIGN_BYTE_(720, 64) * _HI_SYS_ALIGN_BYTE_(576, 64) * 2;/*D1*/
        pvb_config->astCommPool[0].u32BlkCnt  = 48;
    }
    return 0;
#endif

    /*vi(vi_num * 6)*/
	if(vi_num_sd > 0)
	{
    	pvb_config->astCommPool[vb_index].u32BlkSize = _HI_SYS_ALIGN_BYTE_(vi_width_sd, 64) * _HI_SYS_ALIGN_BYTE_(vi_hight_sd, 64) * 2;
    	pvb_config->astCommPool[vb_index].u32BlkCnt = vi_num_sd * 6;
    	vb_index ++;
	}
	
    /*wbc(4)、user pic(3)*/
    pvb_config->astCommPool[vb_index].u32BlkSize = _HI_SYS_ALIGN_BYTE_(720, 64) * _HI_SYS_ALIGN_BYTE_(576, 64) * 2;
    pvb_config->astCommPool[vb_index].u32BlkCnt = 3;
    vb_index ++;

    /*vda(vi_num * 4)*/
    pvb_config->astCommPool[vb_index].u32BlkSize = _HI_SYS_ALIGN_BYTE_(360, 64) * _HI_SYS_ALIGN_BYTE_(288, 64) * 2;
    pvb_config->astCommPool[vb_index].u32BlkCnt = vi_num_sd  * 4;
    vb_index ++;

    return 0;
}
int CHiSystem::HiSys_init()
{
    MPP_VERSION_S mpp_version;
    MPP_SYS_CONF_S mpp_sys_config;
    VB_CONF_S vb_config;
    HI_S32 ret_val = -1;
    int i=0;
    i=i;
    memset(&vb_config, 0, sizeof(VB_CONF_S));

#if !defined(_AV_PRODUCT_CLASS_IPC_)
    unsigned int vi_num_sd, vi_max_width_sd, vi_max_height_sd; 
    unsigned int vi_num_hd = 0, vi_max_width_hd = 0, vi_max_height_hd = 0; /*根据机型来判断*/

    eProductType product_type = PTD_INVALID;
    eSystemType subproduct_type = SYSTEM_TYPE_MAX;
    product_type = pgAvDeviceInfo->Adi_product_type();
    subproduct_type = pgAvDeviceInfo->Adi_sub_product_type();
    pgAvDeviceInfo->Adi_get_vi_info(vi_num_sd, vi_max_width_sd, vi_max_height_sd);
    DEBUG_CRITICAL("cxliu,product_type=%d,subproduct_type=%d vi_num_sd:%d,vi_num_hd:%d,vi_max_width_sd:%d,vi_max_height_sd:%d\n",
		product_type,subproduct_type,vi_num_sd,vi_num_hd,vi_max_width_sd,vi_max_height_sd);

    if((product_type == PTD_X3) && (subproduct_type == SYSTEM_TYPE_X3_AHD))
    {
        vi_num_hd = 2;
        vi_max_height_hd = 1280;
        vi_max_width_hd = 720;
        vi_num_sd = 0;
    }

    if(((product_type == PTD_X1_AHD) && ((subproduct_type ==SYSTEM_TYPE_X1_V2_X1H0401_AHD)
        ||(subproduct_type ==SYSTEM_TYPE_X1_V2_M1H0401_AHD)
        ||(subproduct_type ==SYSTEM_TYPE_X1_V2_M1SH0401_AHD)))
        ||((product_type == PTD_D5) && (subproduct_type == SYSTEM_TYPE_D5_AHD)))
    {
        vi_num_hd = 2;
        vi_max_height_hd = 1280;
        vi_max_width_hd = 720;
        vi_num_sd = 2;
    }

	if(((product_type == PTD_X1_AHD)&&(subproduct_type ==SYSTEM_TYPE_X1_V2_X1N0400_AHD))
		||(product_type == PTD_XMD3104))
    {
        vi_num_hd = 4;
#ifdef _AV_PLATFORM_HI3520D_V300_
		vi_max_height_hd = 720;
		vi_max_width_hd = 1280;	
#else
		vi_max_height_hd = 1280;
		vi_max_width_hd = 720;	
#endif
        vi_num_sd = 0;
    }
    /**2015-11-26改进型没有模拟通道，在此做个判断申请vb大小 */
    if((product_type == PTD_X1) && ((subproduct_type == SYSTEM_TYPE_X1_V2_X1N0400) || (subproduct_type == SYSTEM_TYPE_X1_N0400)))
    {
        vi_num_hd = 1;
        vi_max_height_hd = 1280;
        vi_max_width_hd = 720;
        vi_num_sd = 0;
    }
	if(product_type == PTD_D5_35)
	{
	       vi_num_hd = 2;
		vi_max_height_hd = 720;
		vi_max_width_hd = 1280;	
		vi_num_sd = 4;				
	}
#if defined(_AV_PLATFORM_HI3520D_V300_)&&(product_type == PTD_XMD3104)
	for(int i = 0;i < 8;i++)
	{	
       if ((ret_val = HI_MPI_VI_DisableUserPic(i)) != 0)
       {
            DEBUG_ERROR( "CHiVi::HiSys_init HI_MPI_VI_DisableUserPic FAILED! (0x%08x)\n", ret_val);
            return -1; 
       }
	}	
#endif

#endif
    /*uninit*/
    HI_MPI_SYS_Exit();
#ifdef _AV_PLATFORM_HI3520D_V300_
    for(int i = 0;i < VB_MAX_USER;i++)
    {
         HI_MPI_VB_ExitModCommPool((VB_UID_E)i);
    }
    for(int i = 0; i < VB_MAX_POOLS; i++)
    {
         HI_MPI_VB_DestroyPool(i);
    }
#endif
    HI_MPI_VB_Exit();

    memset(&vb_config, 0, sizeof(VB_CONF_S));
    
    /*video buffer config*/
#if defined(_AV_PLATFORM_HI3531_)
    ret_val = HiSys_get_vb_config_on_two_ddr(vi_num_sd, vi_max_width_sd, vi_max_height_sd, &vb_config);
#elif defined(_AV_PLATFORM_HI3521_)
    ret_val = HiSys_get_vb_config_on_single_ddr(vi_num_sd, vi_max_width_sd, vi_max_height_sd,vi_num_sd, vi_max_width_sd, vi_max_height_sd, &vb_config);
#elif defined(_AV_PLATFORM_HI3520D_)
    ret_val = HiSys_get_vb_config_on_single_ddr(vi_num_sd, vi_max_width_sd, vi_max_height_sd,vi_num_hd, vi_max_width_hd, vi_max_height_hd, &vb_config);
#elif defined(_AV_PLATFORM_HI3520D_V300_)
    ret_val = HiSys_get_vb_config_on_single_ddr(vi_num_sd, vi_max_width_sd, vi_max_height_sd,vi_num_hd, vi_max_width_hd, vi_max_height_hd, &vb_config);
#elif defined(_AV_PLATFORM_HI3535_)
    ret_val = HiSys_get_vb_config_BC_NVR(&vb_config);
#elif defined(_AV_PLATFORM_HI3515_)
    ret_val = HiSys_get_vb_config_A5_II(vi_num_sd, vi_max_width_sd, vi_max_height_sd, &vb_config);
#elif defined(_AV_PLATFORM_HI3518C_) || defined(_AV_PLATFORM_HI3516A_)
#if defined(_AV_PRODUCT_CLASS_IPC_) 
    switch(pgAvDeviceInfo->Adi_product_type())
    {
        case PTD_712_VD:
        case PTD_712_VA:
        case PTD_712_VB:
        case PTD_712_VC:
        case PTD_714_VA:
        case PTD_712_VE:
        default:
        {
            ret_val = HiSys_get_vb_config_IPC_720P(vb_config);
        }
            break;    
        case PTD_913_VA:
        case PTD_916_VA:
        {
            ret_val = HiSys_get_vb_config_IPC_1080P(vb_config);
        }
            break;
        case PTD_920_VA:
        {
             ret_val = HiSys_get_vb_config_IPC_5M(vb_config);
        }
            break;
    }
#endif
#else
    _AV_FATAL_(1, "you must select a platform!\n");
#endif
    if(ret_val != 0)
    {
        DEBUG_ERROR( "CHiSystem::HiSys_init HiSys_get_vb_config_xx FAILED\n");
        return -1;
    }
    if((ret_val = HI_MPI_VB_SetConf(&vb_config)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiSystem::HiSys_init HI_MPI_VB_SetConf FAILED(0x%08lx)\n", (unsigned long)ret_val);
        return -1;
    }
    if((ret_val = HI_MPI_VB_Init()) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiSystem::HiSys_init HI_MPI_VB_Init FAILED(0x%08lx)\n", (unsigned long)ret_val);
        return -1;
    }

    /*system config*/
    memset(&mpp_sys_config, 0, sizeof(MPP_SYS_CONF_S));
#if defined(_AV_PLATFORM_HI3520D_V300_)//||defined(_AV_PLATFORM_HI3515_)
    mpp_sys_config.u32AlignWidth = 16;
#else
    mpp_sys_config.u32AlignWidth = 64;
#endif
    if((ret_val = HI_MPI_SYS_SetConf(&mpp_sys_config)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiSystem::HiSys_init HI_MPI_SYS_SetConf FAILED(0x%08lx)\n", (unsigned long)ret_val);
        return -1;
    }

    if((ret_val = HI_MPI_SYS_Init()) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiSystem::HiSys_init HI_MPI_SYS_Init FAILED(0x%08lx)\n", (unsigned long)ret_val);
        return -1;
    }

    /*get mpp version*/
    mpp_version.aVersion[0] = '\0';
    if((ret_val = HI_MPI_SYS_GetVersion(&mpp_version)) == HI_SUCCESS)
    {
        DEBUG_MESSAGE( "MPP LIBRARY VERSION:%s\n", mpp_version.aVersion);
    }
    else
    {
        DEBUG_ERROR( "CHiSystem::HiSys_init HI_MPI_SYS_GetVersion FAILED(0x%08lx)\n", (unsigned long)ret_val);
    }

    return 0;
}


int CHiSystem::HiSys_uninit()
{
#ifdef _AV_PLATFORM_HI3520D_V300_
    int i;
    HI_S32 ret_val = -1;

for(i=0;i<8;i++)
{
    if ((ret_val = HI_MPI_VI_DisableUserPic(i)) != 0)
    {
        DEBUG_ERROR( "CHiVi::HiSys_uninit HI_MPI_VI_DisableUserPic FAILED! (0x%08x)\n", ret_val);
        return -1; 
    }
}	
#endif
    HI_MPI_SYS_Exit();
#ifdef _AV_PLATFORM_HI3520D_V300_
    for(i=0;i<VB_MAX_USER;i++)
    {
         HI_MPI_VB_ExitModCommPool((VB_UID_E)i);
    }
    for(i=0; i<VB_MAX_POOLS; i++)
    {
         HI_MPI_VB_DestroyPool(i);
    }	
#endif
    HI_MPI_VB_Exit();

    return 0;
}

#if defined(_AV_PRODUCT_CLASS_IPC_)
int CHiSystem::HiSys_get_vb_config_IPC_720P(VB_CONF_S &pVb_config)
{
    pVb_config.u32MaxPoolCnt = 128;
 
    pVb_config.astCommPool[0].u32BlkSize = _HI_SYS_ALIGN_BYTE_(1280, 64) * _HI_SYS_ALIGN_BYTE_(960, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
    pVb_config.astCommPool[0].u32BlkCnt = 10; 
 
    pVb_config.astCommPool[1].u32BlkSize = _HI_SYS_ALIGN_BYTE_(640, 64) * _HI_SYS_ALIGN_BYTE_(360, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
    pVb_config.astCommPool[1].u32BlkCnt = 6; 
    
    pVb_config.astCommPool[2].u32BlkSize = _HI_SYS_ALIGN_BYTE_(1280, 64) * _HI_SYS_ALIGN_BYTE_(720, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
    pVb_config.astCommPool[2].u32BlkCnt = 5; 
 
    pVb_config.astCommPool[3].u32BlkSize = (196*4);
    pVb_config.astCommPool[3].u32BlkCnt = 6; 
    
    return 0;    
}

int CHiSystem::HiSys_get_vb_config_IPC_1080P(VB_CONF_S &pVb_config)
{
    pVb_config.u32MaxPoolCnt = 128;
 
    pVb_config.astCommPool[0].u32BlkSize = _HI_SYS_ALIGN_BYTE_(1920, 64) * _HI_SYS_ALIGN_BYTE_(1080, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
    //pVb_config->astCommPool[0].u32BlkSize = CEILING_2_POWER(1280, 64) * CEILING_2_POWER(960, 64) * 1.5;
#if defined(_AV_PLATFORM_HI3516A_)
   pVb_config.astCommPool[0].u32BlkCnt = 40;//the ive need more blk
#else
    pVb_config.astCommPool[0].u32BlkCnt = 30; //LDC will take more vb blk
 #endif
    pVb_config.astCommPool[1].u32BlkSize = _HI_SYS_ALIGN_BYTE_(960, 64) * _HI_SYS_ALIGN_BYTE_(540, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
    pVb_config.astCommPool[1].u32BlkCnt = 12; 
    
    pVb_config.astCommPool[2].u32BlkSize = _HI_SYS_ALIGN_BYTE_(960, 64) * _HI_SYS_ALIGN_BYTE_(540, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
    pVb_config.astCommPool[2].u32BlkCnt = 12; 
 
    pVb_config.astCommPool[3].u32BlkSize = (196*4);
    pVb_config.astCommPool[3].u32BlkCnt = 10; 
    
    return 0; 
}

int CHiSystem::HiSys_get_vb_config_IPC_5M(VB_CONF_S &pVb_config)
{
    pVb_config.u32MaxPoolCnt = 128;
 
    pVb_config.astCommPool[0].u32BlkSize = _HI_SYS_ALIGN_BYTE_(2592, 64) * _HI_SYS_ALIGN_BYTE_(1944, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
    //pVb_config->astCommPool[0].u32BlkSize = CEILING_2_POWER(1280, 64) * CEILING_2_POWER(960, 64) * 1.5;
    pVb_config.astCommPool[0].u32BlkCnt = 20; 
 
    pVb_config.astCommPool[1].u32BlkSize = _HI_SYS_ALIGN_BYTE_(960, 64) * _HI_SYS_ALIGN_BYTE_(540, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
    pVb_config.astCommPool[1].u32BlkCnt = 10; 
    
    pVb_config.astCommPool[2].u32BlkSize = _HI_SYS_ALIGN_BYTE_(960, 64) * _HI_SYS_ALIGN_BYTE_(540, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
    pVb_config.astCommPool[2].u32BlkCnt = 10; 
 
    pVb_config.astCommPool[3].u32BlkSize = (196*4);
    pVb_config.astCommPool[3].u32BlkCnt = 10; 
    
    return 0; 
}

#endif


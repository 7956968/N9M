#include "HiSystem.h"
#include "mpi_vi.h"
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
    pVb_config->astCommPool[0].u32BlkSize = CEILING_2_POWER( pgAvDeviceInfo->Adi_get_WD1_width(), 64) * 576 * 1.5;
    pVb_config->astCommPool[0].u32BlkCnt = 12 * 8;/*8 for each channel*/

    pVb_config->astCommPool[1].u32BlkSize = CEILING_2_POWER( pgAvDeviceInfo->Adi_get_WD1_width()/2, 64) * 288 * 1.5;
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

	if(pgAvDeviceInfo->Adi_ad_type_isahd()==true) //#if defined(_AV_PLATFORM_HI3520D_)
	{
	    /*vi(vi_num * 6)*/
		if(vi_num_sd > 0)
		{
	    	pvb_config->astCommPool[vb_index].u32BlkSize = _HI_SYS_ALIGN_BYTE_(vi_width_sd, 64) * _HI_SYS_ALIGN_BYTE_(vi_hight_sd, 64) * 2;
	    	pvb_config->astCommPool[vb_index].u32BlkCnt = vi_num_sd * 6;
	    	vb_index ++;
		}

		if(vi_num_hd > 0)
		{
		    pvb_config->astCommPool[vb_index].u32BlkSize = _HI_SYS_ALIGN_BYTE_(vi_width_hd, 64) * _HI_SYS_ALIGN_BYTE_(vi_hight_hd, 64) * 2;
		    pvb_config->astCommPool[vb_index].u32BlkCnt = vi_num_hd * 6;
		    vb_index ++;
		}
		
	    /*wbc(4)、user pic(3)*/
	    pvb_config->astCommPool[vb_index].u32BlkSize = _HI_SYS_ALIGN_BYTE_(720, 64) * _HI_SYS_ALIGN_BYTE_(576, 64) * 2;
	    pvb_config->astCommPool[vb_index].u32BlkCnt = 4 + 3;
	    vb_index ++;

	    /*vda(vi_num * 4)*/
	    pvb_config->astCommPool[vb_index].u32BlkSize = _HI_SYS_ALIGN_BYTE_(360, 64) * _HI_SYS_ALIGN_BYTE_(288, 64) * 2;
	    pvb_config->astCommPool[vb_index].u32BlkCnt = (vi_num_sd + vi_num_hd) * 4;
	    vb_index ++;
	}
	else
	{

	    /*vi(vi_num * 6)*/
		if(vi_num_sd > 0)
		{
	    	pvb_config->astCommPool[vb_index].u32BlkSize = _HI_SYS_ALIGN_BYTE_(vi_width_sd, 64) * _HI_SYS_ALIGN_BYTE_(vi_hight_sd, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
	    	pvb_config->astCommPool[vb_index].u32BlkCnt = vi_num_sd * 6;
	    	vb_index ++;
		}

		if(vi_num_hd > 0)
		{
		    pvb_config->astCommPool[vb_index].u32BlkSize = _HI_SYS_ALIGN_BYTE_(vi_width_hd, 64) * _HI_SYS_ALIGN_BYTE_(vi_hight_hd, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
		    pvb_config->astCommPool[vb_index].u32BlkCnt = vi_num_hd * 6;
		    vb_index ++;
		}
		
	    /*wbc(4)、user pic(3)*/
	    pvb_config->astCommPool[vb_index].u32BlkSize = _HI_SYS_ALIGN_BYTE_(720, 64) * _HI_SYS_ALIGN_BYTE_(576, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
	    pvb_config->astCommPool[vb_index].u32BlkCnt = 4 + 3;
	    vb_index ++;

	    /*vda(vi_num * 4)*/
	    pvb_config->astCommPool[vb_index].u32BlkSize = _HI_SYS_ALIGN_BYTE_(360, 64) * _HI_SYS_ALIGN_BYTE_(288, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
	    pvb_config->astCommPool[vb_index].u32BlkCnt = (vi_num_sd + vi_num_hd) * 4;
	    vb_index ++;
	}
	
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
    pvb_config->astCommPool[vb_index].u32BlkSize = _HI_SYS_ALIGN_BYTE_(vi_width, 64) * _HI_SYS_ALIGN_BYTE_(vi_hight, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
    pvb_config->astCommPool[vb_index].u32BlkCnt = ddr0_chn_num * 6;
    vb_index ++;

    pvb_config->astCommPool[vb_index].u32BlkSize = _HI_SYS_ALIGN_BYTE_(vi_width, 64) * _HI_SYS_ALIGN_BYTE_(vi_hight, 64) * pgAvDeviceInfo->Adi_get_pixel_size();;
    pvb_config->astCommPool[vb_index].u32BlkCnt = ddr1_chn_num * 6;
    strcpy(pvb_config->astCommPool[vb_index].acMmzName, "ddr1");
    vb_index ++;

    /*wbc(4)、user pic(3)*/
    pvb_config->astCommPool[vb_index].u32BlkSize = _HI_SYS_ALIGN_BYTE_(720, 64) * _HI_SYS_ALIGN_BYTE_(576, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
    pvb_config->astCommPool[vb_index].u32BlkCnt = 4 + 3;
    vb_index ++;

    /*vda*/
    pvb_config->astCommPool[vb_index].u32BlkSize = _HI_SYS_ALIGN_BYTE_(360, 64) * _HI_SYS_ALIGN_BYTE_(288, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
    pvb_config->astCommPool[vb_index].u32BlkCnt = ddr0_chn_num * 4;
    vb_index ++;

    pvb_config->astCommPool[vb_index].u32BlkSize = _HI_SYS_ALIGN_BYTE_(360, 64) * _HI_SYS_ALIGN_BYTE_(288, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
    pvb_config->astCommPool[vb_index].u32BlkCnt = ddr1_chn_num * 4;
    strcpy(pvb_config->astCommPool[vb_index].acMmzName, "ddr1");
    vb_index ++;

    return 0;
}

int CHiSystem::HiSys_init()
{
    MPP_VERSION_S mpp_version;
    MPP_SYS_CONF_S mpp_sys_config;
    VB_CONF_S vb_config;
    HI_S32 ret_val = -1;
    
    memset(&vb_config, 0, sizeof(VB_CONF_S));

#if !defined(_AV_PRODUCT_CLASS_IPC_)
    unsigned int vi_num_sd, vi_max_width_sd, vi_max_height_sd; 
    unsigned int vi_num_hd = 0, vi_max_width_hd = 0, vi_max_height_hd = 0; /*根据机型来判断*/

    eProductType product_type = PTD_INVALID;
    eSystemType subproduct_type = SYSTEM_TYPE_MAX;
    product_type = pgAvDeviceInfo->Adi_product_type();
    subproduct_type = pgAvDeviceInfo->Adi_sub_product_type();
    pgAvDeviceInfo->Adi_get_vi_info(vi_num_sd, vi_max_width_sd, vi_max_height_sd);
    DEBUG_CRITICAL("product_type=%d,subproduct_type=%d,sdNum=%d,hdNum=%d\n",product_type,subproduct_type,vi_num_sd,vi_num_hd);

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
        vi_num_hd = 4;
        vi_max_height_hd = 1280;
        vi_max_width_hd = 720;
        vi_num_sd = 0;
    }

	if(((product_type == PTD_X1_AHD)&&(subproduct_type ==SYSTEM_TYPE_X1_V2_X1N0400_AHD))
		||(product_type == PTD_XMD3104)||(product_type == PTD_ES4206)||(product_type == PTD_M0026))
    {
        vi_num_hd = 4;
	vi_max_height_hd = 720;
	vi_max_width_hd = 1280;	
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
	if((product_type == PTD_D5_35)||(product_type == PTD_P1))
	{
	       vi_num_hd = 2;
		vi_max_height_hd = 720;
		vi_max_width_hd = 1280;	
		vi_num_sd = 4;				
	}
	if((product_type == PTD_6A_II_AVX)||(product_type == PTD_ES8412))
	{
	    vi_num_hd = 8;
		vi_max_height_hd = 720;
		vi_max_width_hd = 1280;	
		vi_num_sd = 0;
	}
	if(product_type == PTD_A5_AHD)
	{
	    vi_num_hd = 8;
		vi_max_height_hd = 1080;
		vi_max_width_hd = 1920;	
		vi_num_sd = 0;
	}
	if(product_type == PTD_M3)
	{
	    vi_num_hd = 4;
		vi_max_height_hd = 1080;
		vi_max_width_hd = 1920;	
		vi_num_sd = 0;
	}

	if(product_type == PTD_ES4206)
	{
	    vi_num_hd = 4;
		vi_max_height_hd = 1080;
		vi_max_width_hd = 1920;	
		vi_num_sd = 0;
	}
	if(product_type == PTD_ES8412)
	{
	    vi_num_hd = 8;
		vi_max_height_hd = 1080;
		vi_max_width_hd = 1920;	
		vi_num_sd = 0;
	}
#ifdef _AV_PLATFORM_HI3520D_V300_	
	for(int i = 0;i < 16;i++)
	{
		if ((ret_val = HI_MPI_VI_DisableUserPic(i)) != 0)
		{
			DEBUG_ERROR( "CHiSystem::HiSys_init HI_MPI_VI_DisableUserPic FAILED! (0x%08x)\n", ret_val);
			continue;
			//return -1;
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
//! change sd to hd 0324
    ret_val = HiSys_get_vb_config_on_single_ddr(vi_num_sd, vi_max_width_sd, vi_max_height_sd,vi_num_hd, vi_max_width_hd, vi_max_height_hd, &vb_config);
#elif defined(_AV_PLATFORM_HI3520D_)
    ret_val = HiSys_get_vb_config_on_single_ddr(vi_num_sd, vi_max_width_sd, vi_max_height_sd,vi_num_hd, vi_max_width_hd, vi_max_height_hd, &vb_config);
#elif defined(_AV_PLATFORM_HI3520D_V300_)
    ret_val = HiSys_get_vb_config_on_single_ddr(vi_num_sd, vi_max_width_sd, vi_max_height_sd,vi_num_hd, vi_max_width_hd, vi_max_height_hd, &vb_config);
#elif defined(_AV_PLATFORM_HI3535_)
    ret_val = HiSys_get_vb_config_BC_NVR(&vb_config);
#elif defined(_AV_PRODUCT_CLASS_IPC_)
    switch(pgAvDeviceInfo->Adi_product_type())
    {
        case PTD_712_VD:
        case PTD_712_VA:
        case PTD_712_VB:
        case PTD_712_VC:
        case PTD_714_VA:
        case PTD_712_VE:
        case PTD_714_VB:
        default:
        {
            ret_val = HiSys_get_vb_config_IPC_720P(vb_config);
        }
            break;    
        case PTD_913_VA:
        case PTD_913_VB:
        case PTD_916_VA:
        {
            ret_val = HiSys_get_vb_config_IPC_1080P(vb_config);
        }
            break;
        case PTD_920_VA:
        {
             ret_val = HiSys_get_vb_config_IPC_1080P(vb_config);
        }
            break;
        case PTD_712_VF:
        {
            ret_val = HiSys_get_vb_config_IPC_720P_FOR_S1(vb_config);
        }
            break;
#if defined(_AV_PLATFORM_HI3518EV200_)
        case PTD_718_VA:
        {
            ret_val = HiSys_get_vb_config_IPC_for_3518EV200(vb_config);
        }
            break;
#endif
    }

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
#if defined(_AV_PLATFORM_HI3520D_V300_)
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
 #ifdef _AV_PLATFORM_HI3520D_V300_

        VB_CONF_S stModVbConf;	 
        memset(&stModVbConf, 0, sizeof(VB_CONF_S));
#if 1
	 int maxchnnum = pgAvDeviceInfo->Adi_max_channel_number();
	 HI_S32 PicSize=0;
        VB_PIC_BLK_SIZE(928, 576, PT_H264, PicSize);	
        stModVbConf.u32MaxPoolCnt = 2;
        stModVbConf.astCommPool[0].u32BlkSize = PicSize;
        stModVbConf.astCommPool[0].u32BlkCnt  = 4*maxchnnum;
		
        VB_PIC_BLK_SIZE(1920, 1080, PT_H264, PicSize);	
        stModVbConf.astCommPool[1].u32BlkSize = 3133440;//PicSize*2;
        stModVbConf.astCommPool[1].u32BlkCnt  = 4*2;
#else
	    stModVbConf.u32MaxPoolCnt               = 3;
	    stModVbConf.astCommPool[0].u32BlkSize   = 720*576*2;
	    stModVbConf.astCommPool[0].u32BlkCnt    = 32;

	    stModVbConf.astCommPool[1].u32BlkSize   = 720*576/4;
	    stModVbConf.astCommPool[1].u32BlkCnt    = 8;

	    stModVbConf.astCommPool[2].u32BlkSize   = 1920*1080*2;
	    stModVbConf.astCommPool[2].u32BlkCnt    = 4;
#endif    		
	 HI_MPI_VB_ExitModCommPool(VB_UID_VDEC);
        CHECK_RET_INT(HI_MPI_VB_SetModPoolConf(VB_UID_VDEC, &stModVbConf), "HI_MPI_VB_SetModPoolConf");
        CHECK_RET_INT(HI_MPI_VB_InitModCommPool(VB_UID_VDEC), "HI_MPI_VB_InitModCommPool");
#endif

    return 0;
}


int CHiSystem::HiSys_uninit()
{
#ifdef _AV_PLATFORM_HI3520D_V300_
    int i;
    HI_S32 ret_val = -1;

	for(i = 0 ; i < 16 ; i++)
	{
		if ((ret_val = HI_MPI_VI_DisableUserPic(i)) != 0)
		{
			DEBUG_ERROR( "CHiSystem::HiSys_uninit HI_MPI_VI_DisableUserPic FAILED! (0x%08x)\n", ret_val);
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

int CHiSystem::HiSys_get_vb_config_IPC_720P_FOR_S1(VB_CONF_S &pVb_config)
{
    pVb_config.u32MaxPoolCnt = 128;
 
    pVb_config.astCommPool[0].u32BlkSize = _HI_SYS_ALIGN_BYTE_(1280, 64) * _HI_SYS_ALIGN_BYTE_(720, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
    pVb_config.astCommPool[0].u32BlkCnt = 13; 
 
    pVb_config.astCommPool[1].u32BlkSize = _HI_SYS_ALIGN_BYTE_(640, 64) * _HI_SYS_ALIGN_BYTE_(360, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
    pVb_config.astCommPool[1].u32BlkCnt = 6; 
 
    pVb_config.astCommPool[3].u32BlkSize = (196*4);
    pVb_config.astCommPool[3].u32BlkCnt = 6; 
    
    return 0;  
}


int CHiSystem::HiSys_get_vb_config_IPC_1080P(VB_CONF_S &pVb_config)
{
    pVb_config.u32MaxPoolCnt = 128;

    pVb_config.astCommPool[0].u32BlkSize = _HI_SYS_ALIGN_BYTE_(1920, 64) * _HI_SYS_ALIGN_BYTE_(1080, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
    //pVb_config->astCommPool[0].u32BlkSize = CEILING_2_POWER(1280, 64) * CEILING_2_POWER(960, 64) * 1.5;

#if defined(_AV_SUPPORT_IVE_)
    pVb_config.astCommPool[0].u32BlkCnt = 30;
#else
    pVb_config.astCommPool[0].u32BlkCnt = 20;
#endif
    pVb_config.astCommPool[1].u32BlkSize = _HI_SYS_ALIGN_BYTE_(960, 64) * _HI_SYS_ALIGN_BYTE_(540, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
    pVb_config.astCommPool[1].u32BlkCnt = 12; 

    pVb_config.astCommPool[2].u32BlkSize = _HI_SYS_ALIGN_BYTE_(640, 64) * _HI_SYS_ALIGN_BYTE_(480, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
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
#if defined(_AV_PLATFORM_HI3518EV200_)
int CHiSystem::HiSys_get_vb_config_IPC_for_3518EV200(VB_CONF_S &pVb_config)
{
    pVb_config.u32MaxPoolCnt = 128;
 
    pVb_config.astCommPool[0].u32BlkSize = _HI_SYS_ALIGN_BYTE_(1280, 64) * _HI_SYS_ALIGN_BYTE_(720, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
    pVb_config.astCommPool[0].u32BlkCnt = 8; 
 
    pVb_config.astCommPool[1].u32BlkSize = _HI_SYS_ALIGN_BYTE_(640, 64) * _HI_SYS_ALIGN_BYTE_(480, 64) * pgAvDeviceInfo->Adi_get_pixel_size();
    pVb_config.astCommPool[1].u32BlkCnt = 5; 
 
    pVb_config.astCommPool[3].u32BlkSize = (196*4);
    pVb_config.astCommPool[3].u32BlkCnt = 6; 
    
    return 0;    
}
#endif
#endif


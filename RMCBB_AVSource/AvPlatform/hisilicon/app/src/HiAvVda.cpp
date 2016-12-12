#include "HiAvVda.h"
#include "AvDebug.h"
#include "mpi_sys.h"

#if defined(_AV_PLATFORM_HI3516A_)
CHiAvVda::CHiAvVda(CHiVpss *pHiVpss): m_pHiVpss(pHiVpss)
{
}
#else
CHiAvVda::CHiAvVda(CHiVi *pHiVi) : m_pHiVi(pHiVi)
{
}
#endif
CHiAvVda::~CHiAvVda()
{
#if defined(_AV_PLATFORM_HI3516A_)
    m_pHiVpss = NULL;
#else
    m_pHiVi = NULL;
#endif
}

int CHiAvVda::Av_create_detector(vda_type_e vda_type, int detector_handle, const vda_chn_param_t* pVda_chn_param)
{
    int ret = 0;
    const int squares_per_line = 22;
    const int squares_per_column = (CAvVda::m_vda_tv_system == _AT_PAL_) ? 18 : 15;

    if ((vda_type != _VDA_MD_ && vda_type != _VDA_OD_) || pVda_chn_param == NULL)
    {
        DEBUG_ERROR( "CHiAvVda::Av_create_detector parameter ERROR !!!! (vda_type:%d) \n", vda_type);
        return -1;
    }
#ifdef _AV_PLATFORM_HI3531_
    MPP_CHN_S stuMppChn;
    const char *pDdrName = NULL;
    HI_S32 sRet = HI_SUCCESS;

    pDdrName = "ddr1";
    stuMppChn.enModId = HI_ID_VDA;
    stuMppChn.s32DevId = 0;
    stuMppChn.s32ChnId = detector_handle;

    if( (sRet = HI_MPI_SYS_SetMemConf(&stuMppChn, pDdrName)) != 0 )
    {
        DEBUG_ERROR( "CHiAvDec::CreateAudioDecoder HI_MPI_SYS_SetMemConf (aChn:%d)(0x%08lx)\n", stuMppChn.s32ChnId, (unsigned long)sRet );
        return -1;
    }
#endif

    VDA_CHN_ATTR_S vda_chn_attr;
    memset(&vda_chn_attr, 0, sizeof(VDA_CHN_ATTR_S));
    vda_chn_attr.u32Width = 16 * squares_per_line;
    vda_chn_attr.u32Height = 16 * squares_per_column;
    
    if (vda_type == _VDA_MD_)
    {
        vda_chn_attr.enWorkMode = VDA_WORK_MODE_MD;
        vda_chn_attr.unAttr.stMdAttr.enMbSadBits = VDA_MB_SAD_8BIT;
        vda_chn_attr.unAttr.stMdAttr.enMbSize = VDA_MB_16PIXEL;
        vda_chn_attr.unAttr.stMdAttr.enRefMode = VDA_REF_MODE_DYNAMIC;
        vda_chn_attr.unAttr.stMdAttr.enVdaAlg = VDA_ALG_REF;
        vda_chn_attr.unAttr.stMdAttr.u32BgUpSrcWgt = 128;
        vda_chn_attr.unAttr.stMdAttr.u32MdBufNum = 8;
        vda_chn_attr.unAttr.stMdAttr.u32ObjNumMax = 128;
        vda_chn_attr.unAttr.stMdAttr.u32SadTh = 100;
        vda_chn_attr.unAttr.stMdAttr.u32VdaIntvl = 5;
    }
#if defined(_AV_PRODUCT_CLASS_IPC_)
    else if (vda_type == _VDA_OD_)
    {
        vda_chn_attr.enWorkMode = VDA_WORK_MODE_OD;
        vda_chn_attr.unAttr.stOdAttr.enVdaAlg      = VDA_ALG_BG;
        vda_chn_attr.unAttr.stOdAttr.enMbSize      = VDA_MB_16PIXEL;
        vda_chn_attr.unAttr.stOdAttr.enMbSadBits   = VDA_MB_SAD_8BIT;
        vda_chn_attr.unAttr.stOdAttr.enRefMode     = VDA_REF_MODE_DYNAMIC;
        if(25 == pgAvDeviceInfo->Adi_get_framerate())
        {
            vda_chn_attr.unAttr.stOdAttr.u32VdaIntvl   = 4;
        }
        else
        {
            vda_chn_attr.unAttr.stOdAttr.u32VdaIntvl   = 5;
        }
        vda_chn_attr.unAttr.stOdAttr.u32BgUpSrcWgt = 128;
        
        vda_chn_attr.unAttr.stOdAttr.u32RgnNum = 1;
        
        /*This value according to the specific environment is different*/
        vda_chn_attr.u32Width = VDA_MAX_WIDTH;
        vda_chn_attr.u32Height = VDA_MAX_HEIGHT;
        vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].stRect.s32X = 0;
        vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].stRect.s32Y =  0;
        vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].stRect.u32Width  = VDA_MAX_WIDTH;
        vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].stRect.u32Height = VDA_MAX_HEIGHT ;
        vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].u32SadTh      = 448;
        if(0 == pVda_chn_param->sensitivity)
        {
            /*The smaller the value the higher the degree of sensitivity, contrary to hisi document description*/
            vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].u32AreaTh     = 70;
        }
        else if(1 == pVda_chn_param->sensitivity)
        {
            vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].u32AreaTh     = 80;
        }
        else
        {
            vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].u32AreaTh     = 90;
        }
        DEBUG_MESSAGE("[debug]the area threshold  is:%d \n", vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].u32AreaTh);
        
        vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].u32OccCntTh   = 5;
        vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].u32UnOccCntTh = 1;

    }  
#else
    else if (vda_type == _VDA_OD_)
    {
        vda_chn_attr.enWorkMode = VDA_WORK_MODE_OD;
        vda_chn_attr.unAttr.stOdAttr.enVdaAlg      = VDA_ALG_BG;
        vda_chn_attr.unAttr.stOdAttr.enMbSize      = VDA_MB_16PIXEL;
        vda_chn_attr.unAttr.stOdAttr.enMbSadBits   = VDA_MB_SAD_8BIT;
        vda_chn_attr.unAttr.stOdAttr.enRefMode     = VDA_REF_MODE_DYNAMIC;
       
        vda_chn_attr.unAttr.stOdAttr.u32VdaIntvl   = 4;
       
        vda_chn_attr.unAttr.stOdAttr.u32BgUpSrcWgt = 128;
        
        vda_chn_attr.unAttr.stOdAttr.u32RgnNum = 1;
        
        /*This value according to the specific environment is different*/
        //vda_chn_attr.u32Width = 704;
        //vda_chn_attr.u32Height = 576;
        vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].stRect.s32X = 0;//176;
        vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].stRect.s32Y =  0;//144;
        vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].stRect.u32Width  = 16 * squares_per_line;
        vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].stRect.u32Height = 16 * squares_per_column ;
        vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].u32SadTh      = 250;
        if(0 == pVda_chn_param->sensitivity)
        {
            /*The smaller the value the higher the degree of sensitivity, contrary to hisi document description*/
            vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].u32AreaTh     = 75;
        }
        else if(1 == pVda_chn_param->sensitivity)
        {
            vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].u32AreaTh     = 85;
        }
        else
        {
            vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].u32AreaTh     = 95;
        }
        DEBUG_MESSAGE("[debug]the area threshold  is:%d \n", vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].u32AreaTh);
        
        vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].u32OccCntTh   = 5;
        vda_chn_attr.unAttr.stOdAttr.astOdRgnAttr[0].u32UnOccCntTh = 1;

        

    }
#endif	
    if ((ret = HI_MPI_VDA_CreateChn(detector_handle, &vda_chn_attr)) != 0)
    {
        DEBUG_ERROR( "CHiAvVda::Av_create_detector HI_MPI_VDA_CreateChn FAILED!!! (detector_handle:%d) (0x%08x) \n", detector_handle, ret);
        return -1;
    }

    return 0;
}

int CHiAvVda::Av_destroy_detector(vda_type_e vda_type, int detector_handle)
{
    int ret = 0;
    vda_type = vda_type; /*avoid warning*/
    
    if ((ret = HI_MPI_VDA_DestroyChn(detector_handle)) != 0)
    {
        DEBUG_ERROR( "CHiAvVda::Av_destroy_detector HI_MPI_VDA_DestroyChn FAILED!!! (0x%08X) (detector_handle:%d)\n", ret, detector_handle);
        return -1;
    }
    
    return 0;
}

int CHiAvVda::Av_detector_control(int phy_video_chn, int detector_handle, bool start_or_stop)
{
    int ret = 0;
    MPP_CHN_S src_chn, dest_chn;

#if defined(_AV_PLATFORM_HI3516A_)
    src_chn.enModId = HI_ID_VPSS;
    src_chn.s32DevId = 0;
    m_pHiVpss->HiVpss_find_vpss(_VP_MAIN_STREAM_, phy_video_chn, NULL, &src_chn.s32ChnId, NULL);
#else
    src_chn.enModId = HI_ID_VIU;
    m_pHiVi->HiVi_get_primary_vi_info(phy_video_chn, &src_chn.s32DevId, &src_chn.s32ChnId);
#endif
    dest_chn.enModId = HI_ID_VDA;
    dest_chn.s32DevId = 0;
    dest_chn.s32ChnId = detector_handle;

    if (start_or_stop)
    {
        if ((ret = HI_MPI_SYS_Bind(&src_chn, &dest_chn)) != 0)
        {
            DEBUG_ERROR( "CHiAvVda::Av_detector_control HI_MPI_SYS_Bind FAILED!!! (0x%08x) src:(dev:%d, chn:%d) dest:(dev:%d, chn:%d)\n", ret, src_chn.s32DevId, src_chn.s32ChnId, dest_chn.s32DevId, dest_chn.s32ChnId);
            return -1;
        }

        if ((ret = HI_MPI_VDA_StartRecvPic(detector_handle)) != 0)
        {
            DEBUG_ERROR( "CHiAvVda::Av_detector_control HI_MPI_VDA_StartRecvPic FAILED!!! (0x%08x) (detector_handle:%d)\n", ret, detector_handle);
            return -1;
        }
    }
    else
    {
        if ((ret = HI_MPI_VDA_StopRecvPic(detector_handle)) != 0)
        {
            DEBUG_ERROR( "CHiAvVda::Av_detector_control HI_MPI_VDA_StopRecvPic FAILED!!! (0x%08x) (detector_handle:%d)\n", ret, detector_handle);
            return -1;
        }

        if ((ret = HI_MPI_SYS_UnBind(&src_chn, &dest_chn)) != 0)
        {
            DEBUG_ERROR( "CHiAvVda::Av_detector_control HI_MPI_SYS_UnBind FAILED!!! (0x%08x) src:(dev:%d, chn:%d) dest:(dev:%d, chn:%d)\n", ret, src_chn.s32DevId, src_chn.s32ChnId, dest_chn.s32DevId, dest_chn.s32ChnId);
            return -1;
        }
    }

    return 0;
}

int CHiAvVda::Av_get_detector_fd(int detector_handle)
{
    int fd = -1;
    
    if ((fd = HI_MPI_VDA_GetFd(detector_handle)) < 0)
    {
        DEBUG_ERROR( "CHiAvVda::Av_get_detector_fd HI_MPI_VDA_GetFd FAILED!!! (detector_handle:%d)\n", detector_handle);
        return -1;
    }

    return fd;
}

int CHiAvVda::Av_get_vda_data(vda_type_e vda_type, int detector_handle, vda_data_t *pVda_data)
{
    int ret = 0;
    void *pAddr = NULL;
    unsigned char *p8Addr = NULL;
    unsigned short *p16Addr = NULL;

    _AV_ASSERT_(pVda_data != NULL, "CHiAvVda::Av_get_vda_data pVda_data is NULL !!!\n");

    /*get vda data*/
    VDA_DATA_S vda_data;
    memset(&vda_data, 0, sizeof(VDA_DATA_S));
    
    if ((ret = HI_MPI_VDA_GetData(detector_handle, &vda_data, HI_TRUE)) != 0)
    {
        DEBUG_ERROR( "CHiAvVda::Av_get_vda_data HI_MPI_VDA_GetData FAILED!!! (0x%08x) (detector_handle:%d)\n", ret, detector_handle);
        return -1;
    }

    /*get usefull result*/
    if (vda_type == _VDA_MD_)
    {
        if (vda_data.unData.stMdData.bMbSadValid)
        {
            for (unsigned int row = 0; row <18; ++row)
            {
                pAddr = reinterpret_cast<void *>(reinterpret_cast<unsigned int>(vda_data.unData.stMdData.stMbSadData.pAddr) + row * vda_data.unData.stMdData.stMbSadData.u32Stride);
                for (unsigned int line = 0; line <22; ++line)
                {
                    if (vda_data.unData.stMdData.stMbSadData.enMbSadBits == VDA_MB_SAD_8BIT)
                    {
                        p8Addr = reinterpret_cast<unsigned char *>(pAddr) + line;
                        pVda_data->sad_value[row][line] = *p8Addr;
                        //DEBUG_MESSAGE("[%s:%d] row %d, line %d, value %d\n", __FUNCTION__, __LINE__, row, line, pVda_data->sad_value[row][line]);
                    }
                    else if (vda_data.unData.stMdData.stMbSadData.enMbSadBits == VDA_MB_SAD_16BIT)
                    {
                        p16Addr = reinterpret_cast<unsigned short *>(pAddr) + line;
                        pVda_data->sad_value[row][line] = *p16Addr;
                        //DEBUG_MESSAGE("[%s:%d] row %d, line %d, value %d\n", __FUNCTION__, __LINE__, row, line, pVda_data->sad_value[row][line]);
                    }
                }
            }
        }

    }
    else if (vda_type == _VDA_OD_)
    {
        pVda_data->result = vda_data.unData.stOdData.abRgnAlarm[0];
        
        if(pVda_data->result)
        {
            HI_S32 nRet = -1;
            nRet = HI_MPI_VDA_ResetOdRegion(detector_handle, 0);
            if(0 != nRet)
            {
                _AV_KEY_INFO_(_AVD_VDA_, "HI_MPI_VDA_ResetOdRegion failed! error code:0x%x \n", nRet);
            }
        }
    }

    /*release vda data*/
    if ((ret = HI_MPI_VDA_ReleaseData(detector_handle, &vda_data)) != 0)
    {
        DEBUG_ERROR( "CHiAvVda::Av_release_vda_data HI_MPI_VDA_ReleaseData FAILED!!! (0x%08x) (detector_handle:%d)\n", ret, detector_handle);
        return -1;
    }
    
    return 0;
}


int CHiAvVda::Av_set_vda_attribut(int vda_type, int detector_handle, unsigned char attr_type, void* attribute)
{
    HI_S32 nRet = HI_FAILURE;
    VDA_CHN_ATTR_S chn_attr;

    memset(&chn_attr, 0, sizeof(VDA_CHN_ATTR_S));

    nRet = HI_MPI_VDA_GetChnAttr(detector_handle, &chn_attr);
    if(HI_SUCCESS != nRet)
    {
        _AV_KEY_INFO_(_AVD_VDA_, "HI_MPI_VDA_GetChnAttr failed, errcode:0x%x \n", nRet);
        return -1;
    }

    if(0 == attr_type)
    {
    //set vdaintval
        unsigned int frame_rate = (unsigned int)attribute;
        unsigned short vda_intvl = 0;
        
        if(25 == frame_rate)
        {
            vda_intvl = 4;
        }
        else
        {
            vda_intvl = 5;
        }

        _AV_KEY_INFO_(_AVD_VDA_, "set vda intvl to :%d \n", vda_intvl);

        if(0 == vda_type)
        {
            chn_attr.unAttr.stMdAttr.u32VdaIntvl = vda_intvl;
        }
        else
        {
            chn_attr.unAttr.stOdAttr.u32VdaIntvl = vda_intvl;
        }

        nRet = HI_MPI_VDA_SetChnAttr(detector_handle, &chn_attr);
        if(HI_SUCCESS != nRet)
        {
            _AV_KEY_INFO_(_AVD_VDA_, "HI_MPI_VDA_SetChnAttr failed, errcode:0x%x \n", nRet);
            return -1;
        }
    }

    return -1;
}



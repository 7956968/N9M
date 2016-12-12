#include "HiVpss.h"

#if defined(_AV_PLATFORM_HI3518EV200_)
#define VPSS_BSTR_CHN (0)
#define VPSS_LSTR_CHN (1)
#endif

#if defined(_AV_PRODUCT_CLASS_IPC_)
#define VPSS_PRE0_CHN   2
#define VPSS_PRE1_CHN   3
#endif


CHiVpss::CHiVpss()
{
    m_pVpss_state = new vpss_state_t[VPSS_MAX_GRP_NUM];
    _AV_FATAL_(m_pVpss_state == NULL, "OUT OF MEMORY\n");

    for(int index = 0; index < VPSS_MAX_GRP_NUM; index ++)
    {
        m_pVpss_state[index].using_bitmap = 0;
    }

    m_pVideo_image = NULL;

    _AV_FATAL_((m_pThread_lock = new CAvThreadLock) == NULL, "OUT OF MEMORY\n");

#if defined(_AV_HAVE_VIDEO_INPUT_)
    m_pHi_vi = NULL;
#endif

#if defined(_AV_HAVE_VIDEO_DECODER_)
    m_pHi_av_dec = NULL;
#endif
}


CHiVpss::~CHiVpss()
{
    _AV_SAFE_DELETE_ARRAY_(m_pVpss_state);
    _AV_SAFE_DELETE_(m_pThread_lock);    
}


int CHiVpss::HiVpss_memory_balance(vpss_purpose_t purpose, int chn_num, void *pPurpose_para, VPSS_GRP vpss_grp)
{
    return 0;
}

int CHiVpss::HiVpss_start_vpss(vpss_purpose_t purpose, int chn_num, VPSS_GRP vpss_grp, unsigned long grp_chn_bitmap, void *pPurpose_para)
{
    VPSS_GRP_ATTR_S vpss_grp_attr;
    VPSS_CHN_ATTR_S vpss_chn_attr;
#if !defined(_AV_PLATFORM_HI3518EV200_)    
    VPSS_GRP_PARAM_S vpss_grp_para;
#endif
    VPSS_CHN_MODE_S vpss_chn_mode;
    MPP_CHN_S chn_source;
    MPP_CHN_S chn_dest;
    HI_S32 ret_val = HI_SUCCESS;
    int max_width = 0;
    int max_height = 0;
    int vpss_grp_chn = 0;
#if defined(_AV_HAVE_VIDEO_INPUT_)
    VI_DEV vi_dev = 0;
    VI_CHN vi_chn = 0;
#endif

    /*collect information from source*/
    memset(&vpss_grp_attr, 0, sizeof(VPSS_GRP_ATTR_S));
    switch(purpose)
    {
#if defined(_AV_HAVE_VIDEO_INPUT_)
        case _VP_MAIN_STREAM_:
        case _VP_SUB_STREAM_:
        case _VP_SUB2_STREAM_:
#if defined(_AV_SUPPORT_IVE_)
        case _VP_SPEC_USE_:
#endif
            _AV_ASSERT_(m_pHi_vi != NULL, "You must call the function[HiVpss_set_vi] to set vi module pointer\n");
            if(m_pHi_vi->HiVi_get_vi_max_size(chn_num, &max_width, &max_height) != 0)
            {
                DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HiVi_get_vi_max_size(%d)\n", chn_num);
                return -1;
            }
            if(m_pHi_vi->HiVi_get_primary_vi_info(chn_num, &vi_dev, &vi_chn) != 0)
            {
                DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss m_pHi_vi->HiVi_get_primary_vi_info(%d)\n", chn_num);
                return -1;
            }
            /*source*/
            chn_source.enModId = HI_ID_VIU;
            chn_source.s32DevId = 0;
            chn_source.s32ChnId = vi_chn;
            /*vpss group attribute*/
            vpss_grp_attr.enPixFmt = pgAvDeviceInfo->Adi_get_vi_pixel_format();
            vpss_grp_attr.bNrEn = HI_TRUE;
            vpss_grp_attr.enDieMode = VPSS_DIE_MODE_NODIE;
            break;
#endif
        default:
            DEBUG_ERROR("CHiVpss::HiVpss_start_vpss You must give the implement(purpose:%d)\n", purpose);
            break;
    }

    if(!pgAvDeviceInfo->Adi_get_vi_vpss_mode())
    {
        pgAvDeviceInfo->Adi_covert_vidoe_size_by_rotate( &max_width, &max_height );
    }
    
    /*create vpss group*/
    vpss_grp_attr.u32MaxW = max_width;
    vpss_grp_attr.u32MaxH = max_height;
    vpss_grp_attr.bHistEn = HI_FALSE;
    vpss_grp_attr.bIeEn = HI_FALSE;
    vpss_grp_attr.bDciEn = HI_FALSE;

    if((ret_val = HI_MPI_VPSS_CreateGrp(vpss_grp, &vpss_grp_attr)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HI_MPI_VPSS_CreateGrp FAILED(group:%d)(ret_val:0x%08lx)(w=%d,h=%d)\n", vpss_grp, (unsigned long)ret_val, max_width, max_height);
        return -1;
    }

#if !defined(_AV_PLATFORM_HI3518EV200_)
    if (HiVpss_get_grp_param(vpss_grp, vpss_grp_para, purpose) < 0)
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HiVpss_get_grp_param FAILED(group:%d)\n", vpss_grp);
        return -1;
    }

    if((ret_val = HI_MPI_VPSS_SetGrpParam(vpss_grp, &vpss_grp_para)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HI_MPI_VPSS_SetGrpParam FAILED(group:%d)(ret_val:0x%08lx)\n", vpss_grp,(unsigned long)ret_val);
        return -1;
    }
#endif
    
    for(vpss_grp_chn = 0; vpss_grp_chn < VPSS_MAX_CHN_NUM; vpss_grp_chn ++)
    {
        if(grp_chn_bitmap & (0x01ul << vpss_grp_chn))
        {
            //online mode set roate in vpss
            if(pgAvDeviceInfo->Adi_get_vi_vpss_mode())
            {
                int rotate = pgAvDeviceInfo->Adi_get_system_rotate();

                if((rotate>=1) && (rotate <=3))
                {
                    ret_val = HI_MPI_VPSS_SetRotate(vpss_grp, vpss_grp_chn, (ROTATE_E)rotate);
                    if(HI_SUCCESS != ret_val)
                    {
                        DEBUG_ERROR( "HI_MPI_VPSS_SetRotate FAILED(group:%d)(chn:%d)(rotate:%d )(ret_val:0x%08lx)\n", vpss_grp, vpss_grp_chn, rotate, (unsigned long)ret_val);
                        return -1;
                    }
                }
            }
            
            memset(&vpss_chn_attr, 0, sizeof(VPSS_CHN_ATTR_S));
            vpss_chn_attr.bSpEn = HI_FALSE;

            vpss_chn_attr.s32SrcFrameRate = -1;
            vpss_chn_attr.s32DstFrameRate = -1;

            if((ret_val = HI_MPI_VPSS_SetChnAttr(vpss_grp, vpss_grp_chn, &vpss_chn_attr)) != HI_SUCCESS)
            {
                DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HI_MPI_VPSS_SetChnAttr FAILED(group:%d)(chn:%d)(ret_val:0x%08lx)\n", vpss_grp, vpss_grp_chn, (unsigned long)ret_val);
                return -1;
            }

            if((ret_val = HI_MPI_VPSS_GetChnMode(vpss_grp, vpss_grp_chn, &vpss_chn_mode)) != HI_SUCCESS)
            {
                DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HI_MPI_VPSS_GetChnMode FAILED(group:%d)(chn:%d)(ret_val:0x%08lx)\n", vpss_grp, vpss_grp_chn, (unsigned long)ret_val);
                return -1;
            }
            /*Hi3516A only support USER mode*/
            vpss_chn_mode.enChnMode = VPSS_CHN_MODE_USER;
#if defined(_AV_SUPPORT_IVE_)
            if(VPSS_PRE1_CHN == vpss_grp_chn)
            {
                vpss_chn_mode.u32Width = 640;
                vpss_chn_mode.u32Height = 360;                
            }
            else if(1 == vpss_grp_chn)  //! for singpore bus
            {
                vpss_chn_mode.u32Width = 1280;
                vpss_chn_mode.u32Height = 720; 
            }
            else
            {
                unsigned int width;
                unsigned int height;
                av_video_stream_type_t stream;
                switch(vpss_grp_chn)
                {
                    case VPSS_BSTR_CHN:
                    default:
                        stream = _AST_MAIN_VIDEO_;
                        break;
                    case VPSS_LSTR_CHN:
                        stream = _AST_SUB_VIDEO_;
                        break;
                    case VPSS_PRE0_CHN:
                        stream = _AST_SUB2_VIDEO_;
                        break;
                }
                pgAvDeviceInfo->Adi_get_stream_size(chn_num,stream,width,height);
                vpss_chn_mode.u32Width = width;
                vpss_chn_mode.u32Height = height;                              
            }
#else
            unsigned int width;
            unsigned int height;
            av_video_stream_type_t stream;
            switch(vpss_grp_chn)
            {
                case VPSS_BSTR_CHN:
                default:
                    stream = _AST_MAIN_VIDEO_;
                    break;
                case VPSS_LSTR_CHN:
                    stream = _AST_SUB_VIDEO_;
                    break;
                case VPSS_PRE0_CHN:
                    stream = _AST_SUB2_VIDEO_;
                    break;
            }
            pgAvDeviceInfo->Adi_get_stream_size(chn_num,stream,width,height);
            vpss_chn_mode.u32Width = width;
            vpss_chn_mode.u32Height = height;
#endif
            vpss_chn_mode.enPixelFormat = pgAvDeviceInfo->Adi_get_vi_pixel_format();
            vpss_chn_mode.bDouble = HI_FALSE;
            vpss_chn_mode.enCompressMode = COMPRESS_MODE_NONE;
            if((ret_val = HI_MPI_VPSS_SetChnMode(vpss_grp, vpss_grp_chn, &vpss_chn_mode)) != HI_SUCCESS)
            {
                DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HI_MPI_VPSS_SetChnMode FAILED(group:%d)(chn:%d)(ret_val:0x%08lx)\n", vpss_grp, vpss_grp_chn, (unsigned long)ret_val);
                return -1;
            }

#if defined(_AV_SUPPORT_IVE_)
            //vpss chn 3 needs opening nr function manualy
            if(vpss_grp_chn == VPSS_PRE1_CHN)
            {
                if(0 != (HI_MPI_VPSS_SetChnNR(vpss_grp, vpss_grp_chn, HI_TRUE)))
                {
                    DEBUG_ERROR( "HI_MPI_VPSS_SetChnNR FAILED(group:%d)(chn:%d)(ret_val:0x%08lx)\n", vpss_grp, vpss_grp_chn, (unsigned long)ret_val);
                }
            }
            
#endif
            
            if((ret_val = HI_MPI_VPSS_EnableChn(vpss_grp, vpss_grp_chn)) != 0)
            {
                DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HI_MPI_VPSS_EnableChn FAILED(group:%d)(chn:%d)(ret_val:0x%08lx)\n", vpss_grp, vpss_grp_chn, (unsigned long)ret_val);
                return -1;
            }
        }
    }

    /*start group*/
    if((ret_val = HI_MPI_VPSS_StartGrp(vpss_grp)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HI_MPI_VPSS_StartGrp FAILED(group:%d)(ret_val:0x%08lx)\n", vpss_grp, (unsigned long)ret_val);
        return -1;
    }
    
    if ((purpose == _VP_CUSTOM_LOGO_) || (purpose == _VP_ACCESS_LOGO_))
    {
        return 0;
    }
    
    /*bind source*/
    chn_dest.enModId = HI_ID_VPSS;
    chn_dest.s32DevId = vpss_grp;
    chn_dest.s32ChnId = 0;
    if((ret_val = HI_MPI_SYS_Bind(&chn_source, &chn_dest)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_start_vpss HI_MPI_SYS_Bind FAILED(group:%d)(ret_val:0x%08lx)\n", vpss_grp, (unsigned long)ret_val);
        return -1;
    }

    return 0;
}


int CHiVpss::HiVpss_stop_vpss(vpss_purpose_t purpose, int chn_num, VPSS_GRP vpss_grp, unsigned long grp_chn_bitmap, void *pPurpose_para)
{
    MPP_CHN_S chn_source;
    MPP_CHN_S chn_dest;
    HI_S32 ret_val = HI_SUCCESS;
    int vpss_grp_chn = 0;
#if defined(_AV_HAVE_VIDEO_INPUT_)
    VI_DEV vi_dev = 0;
    VI_CHN vi_chn = 0;
#endif

    /*confirm source*/
    switch(purpose)
    {
#if defined(_AV_HAVE_VIDEO_INPUT_)
        case _VP_PREVIEW_VI_:
        case _VP_SPOT_VI_:
        case _VP_MAIN_STREAM_:
        case _VP_SUB_STREAM_:
        case _VP_VI_DIGITAL_ZOOM_:
        case _VP_SUB2_STREAM_:
#if defined(_AV_SUPPORT_IVE_)
        case _VP_SPEC_USE_:
#endif
            _AV_ASSERT_(m_pHi_vi != NULL, "You must call the function[HiVpss_set_vi] to set vi module pointer\n");
            if(m_pHi_vi->HiVi_get_primary_vi_info(chn_num, &vi_dev, &vi_chn) != 0)
            {
                DEBUG_ERROR( "CHiVpss::HiVpss_stop_vpss m_pHi_vi->HiVi_get_primary_vi_info(%d)\n", chn_num);
                return -1;
            }
            /*source*/
            chn_source.enModId = HI_ID_VIU;
            chn_source.s32DevId = 0;
            chn_source.s32ChnId = vi_chn;
            break;
#endif
        case _VP_CUSTOM_LOGO_:
        case _VP_ACCESS_LOGO_:
            break;
        default:
            _AV_FATAL_(1, "CHiVpss::HiVpss_stop_vpss You must give the implement(purpose:%d)\n", purpose);
            break;
    }

    /*unbind*/
    if((purpose != _VP_CUSTOM_LOGO_) && (purpose != _VP_ACCESS_LOGO_))
    {
        chn_dest.enModId = HI_ID_VPSS;
        chn_dest.s32DevId = vpss_grp;
        chn_dest.s32ChnId = 0;
        if((ret_val = HI_MPI_SYS_UnBind(&chn_source, &chn_dest)) != HI_SUCCESS)
        {
            DEBUG_ERROR( "CHiVpss::HiVpss_stop_vpss HI_MPI_SYS_UnBind FAILED(group:%d)(ret_val:0x%08lx)\n", vpss_grp, (unsigned long)ret_val);
            return -1;
        }
    }

    /*disable vpss grp channel*/
    for(vpss_grp_chn = 0; vpss_grp_chn < VPSS_MAX_CHN_NUM; vpss_grp_chn ++)
    {
        if(grp_chn_bitmap & (0x01ul << vpss_grp_chn))
        {
            if((ret_val = HI_MPI_VPSS_DisableChn(vpss_grp, vpss_grp_chn)) != 0)
            {
                DEBUG_ERROR( "CHiVpss::HiVpss_stop_vpss HI_MPI_VPSS_DisableChn FAILED(group:%d)(chn:%d)(ret_val:0x%08lx)\n", vpss_grp, vpss_grp_chn, (unsigned long)ret_val);
                return -1;
            }
        }
    }

    /*stop group*/
    if((ret_val = HI_MPI_VPSS_StopGrp(vpss_grp)) != 0)
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_stop_vpss HI_MPI_VPSS_StopGrp FAILED(group:%d)(ret_val:0x%08lx)\n", vpss_grp, (unsigned long)ret_val);
        return -1;
    }

    /*destory group*/
    if((ret_val = HI_MPI_VPSS_DestroyGrp(vpss_grp)) != 0)
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_stop_vpss HI_MPI_VPSS_DestroyGrp FAILED(group:%d)(ret_val:0x%08lx)\n", vpss_grp, (unsigned long)ret_val);
        return -1;
    }

    return 0;
}

int CHiVpss::HiVpss_allotter_IPC_REGULAR(vpss_purpose_t purpose, int chn_num, VPSS_GRP *pVpss_grp, VPSS_CHN *pVpss_chn, unsigned long *pGrp_chn_bitmap, void *pPurpose_para)
{
    int max_chn_num = pgAvDeviceInfo->Adi_max_channel_number();
    _AV_ASSERT_(chn_num < max_chn_num, "CHiVpss::HiVpss_allotter_IPC_REGULAR INVALID CHANNEL NUMBER(%d, %d)\n", max_chn_num, chn_num);


    switch(purpose)
    {
#if defined(_AV_PLATFORM_HI3518EV200_)
        case _VP_MAIN_STREAM_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num);
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_BSTR_CHN);
            if(PTD_718_VA == pgAvDeviceInfo->Adi_product_type())
            {
                _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_LSTR_CHN));
            }
            else
            {
                _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_LSTR_CHN));
            }
            break;
        case _VP_SUB_STREAM_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num);
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_LSTR_CHN);
            if(PTD_718_VA == pgAvDeviceInfo->Adi_product_type())
            {
                _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_LSTR_CHN));
            }
            else
            {
                _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_LSTR_CHN));
            }

            break;
        case _VP_SUB2_STREAM_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num);
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_PRE0_CHN);
            if(PTD_718_VA == pgAvDeviceInfo->Adi_product_type())
            {
                _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_LSTR_CHN));
            }
            else
            {
                _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_LSTR_CHN));
            }

            break;
        default:
            DEBUG_ERROR("CHiVpss::HiVpss_allotter_DVR_REGULAR You must give the implement\n");
            break;    
#else
#if defined(_AV_SUPPORT_IVE_)
        case _VP_MAIN_STREAM_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num);
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_BSTR_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_LSTR_CHN) | (0x01ul << VPSS_PRE1_CHN) ); 
            break;

        case _VP_SUB_STREAM_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num);
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_LSTR_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_LSTR_CHN) | (0x01ul << VPSS_PRE1_CHN) ); 
            break;
        case _VP_SUB2_STREAM_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num);
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_PRE0_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_LSTR_CHN) | (0x01ul << VPSS_PRE1_CHN) );         
            break;
        case _VP_SPEC_USE_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num);
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_PRE1_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_LSTR_CHN) | (0x01ul << VPSS_PRE1_CHN) ); 
            break;
        default:
            _AV_FATAL_(1, "CHiVpss::HiVpss_allotter_DVR_REGULAR You must give the implement\n");
            break;
#else
        case _VP_MAIN_STREAM_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num);
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_BSTR_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_LSTR_CHN));
            break;

        case _VP_SUB_STREAM_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num);
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_LSTR_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_LSTR_CHN));
            break;
        case _VP_SUB2_STREAM_:
            _AV_POINTER_ASSIGNMENT_(pVpss_grp, chn_num);
            _AV_POINTER_ASSIGNMENT_(pVpss_chn, VPSS_PRE0_CHN);
            _AV_POINTER_ASSIGNMENT_(pGrp_chn_bitmap, (0x01ul << VPSS_PRE0_CHN) | (0x01ul << VPSS_BSTR_CHN) | (0x01ul << VPSS_LSTR_CHN));
            break;
        default:
            _AV_FATAL_(1, "CHiVpss::HiVpss_allotter_DVR_REGULAR You must give the implement\n");
            break;
#endif
#endif
    }

    if(pgAvDeviceInfo->Adi_get_vi_vpss_mode())
    {
        _AV_POINTER_ASSIGNMENT_(pVpss_grp, 0);
    }
    
    return 0;
}

int CHiVpss::HiVpss_allotter(vpss_purpose_t purpose, int chn_num, VPSS_GRP *pVpss_grp, VPSS_CHN *pVpss_chn, unsigned long *pGrp_chn_bitmap, void *pPurpose_para)
{
    HiVpss_allotter_IPC_REGULAR(purpose, chn_num, pVpss_grp, pVpss_chn, pGrp_chn_bitmap, pPurpose_para);
    return 0;
}

int CHiVpss::HiVpss_vpss_manager(vpss_purpose_t purpose, int chn_num, VPSS_GRP *pVpss_grp, VPSS_CHN *pVpss_chn, void *pPurpose_para, int flag)
{
    unsigned long grp_chn_bitmap = 0;
    VPSS_GRP vpss_grp = 0;
    VPSS_CHN vpss_chn = 0; 
    // ret_val = 0; -1?
    int ret_val = 0;
    /*alloc vpss group and channel*/
    if(HiVpss_allotter(purpose, chn_num, &vpss_grp, &vpss_chn, &grp_chn_bitmap, pPurpose_para) != 0)
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_vpss_manager HiVpss_allotter FAILED(%d, %d)\n", purpose, chn_num);
        return -1;
    }

    _AV_ASSERT_(vpss_grp < VPSS_MAX_GRP_NUM, "CHiVpss::HiVpss_vpss_manager INVALID VPSS GROUP(%d)(%d)(%d)(%d)\n", purpose, chn_num, vpss_grp, VPSS_MAX_GRP_NUM);
    _AV_ASSERT_(vpss_chn < VPSS_MAX_CHN_NUM, "CHiVpss::HiVpss_vpss_manager INVALID VPSS CHN(%d)(%d)(%d)(%d)\n", purpose, chn_num, vpss_chn, VPSS_MAX_CHN_NUM);

    /*control*/
    switch(flag)
    {
        case 0:/*get*/
            if(m_pVpss_state[vpss_grp].using_bitmap== 0)
            {
                ret_val = HiVpss_start_vpss(purpose, chn_num, vpss_grp, grp_chn_bitmap, pPurpose_para);
                if(ret_val != 0)
                {
                    DEBUG_ERROR( "CHiVpss::HiVpss_vpss_manager HiVpss_start_vpss FAILED %d purpose=%d chnum=%d grp=%d bit=0x%lx\n", ret_val, purpose, chn_num, vpss_grp, grp_chn_bitmap);
                    HiVpss_stop_vpss(purpose, chn_num, vpss_grp, grp_chn_bitmap, pPurpose_para);
                }
            }
            if(ret_val == 0)
            {
                m_pVpss_state[vpss_grp].using_bitmap = grp_chn_bitmap;
            }
            break;
        case 1:/*release*/
            //using_bitmap = m_pVpss_state[vpss_grp].using_bitmap & (~(0x01ul << vpss_chn));
            if(m_pVpss_state[vpss_grp].using_bitmap != 0)
            {
                ret_val = HiVpss_stop_vpss(purpose, chn_num, vpss_grp, grp_chn_bitmap, pPurpose_para);
            }
            if(ret_val == 0)
            {
                m_pVpss_state[vpss_grp].using_bitmap = 0;
            }
            break;
        default:
            _AV_FATAL_(1, "CHiVpss::HiVpss_vpss_manager You must give the implement\n");
            break;
    }

    _AV_POINTER_ASSIGNMENT_(pVpss_grp, vpss_grp);
    _AV_POINTER_ASSIGNMENT_(pVpss_chn, vpss_chn);

    return ret_val;
}

int CHiVpss::HiVpss_find_vpss(vpss_purpose_t purpose, int chn_num, VPSS_GRP *pVpss_grp, VPSS_CHN *pVpss_chn, void *pPurpose_para/* = NULL*/)
{
    int ret_val = -1;

    m_pThread_lock->lock();
    
    ret_val = HiVpss_allotter(purpose, chn_num, pVpss_grp, pVpss_chn, NULL, pPurpose_para);

    m_pThread_lock->unlock();

    return ret_val;
}


int CHiVpss::HiVpss_get_vpss(vpss_purpose_t purpose, int chn_num, VPSS_GRP *pVpss_grp, VPSS_CHN *pVpss_chn, void *pPurpose_para/* = NULL*/)
{
    int ret_val = -1;

    m_pThread_lock->lock();
    
    ret_val = HiVpss_vpss_manager(purpose, chn_num, pVpss_grp, pVpss_chn, pPurpose_para, 0);

    m_pThread_lock->unlock();

    //DEBUG_MESSAGE( "CHiVpss::HiVpss_get_vpss(purpose:%d)(chn_num:%d)(vpss_grp:%d)(vpss_chn:%d)\n", purpose, chn_num, (pVpss_grp != NULL)?*pVpss_grp:0xff, (pVpss_chn != NULL)?*pVpss_chn:0xff);

    return ret_val;
}

bool CHiVpss::Hivpss_whether_create_vpssgroup(VPSS_GRP vpss_grp)
{
    return m_pVpss_state[vpss_grp].using_bitmap != 0;
}

int CHiVpss::HiVpss_release_vpss(vpss_purpose_t purpose, int chn_num, void *pPurpose_para/* = NULL*/)
{
    int ret_val = -1;

    m_pThread_lock->lock();
    
    ret_val = HiVpss_vpss_manager(purpose, chn_num, NULL, NULL, pPurpose_para, 1);

    m_pThread_lock->unlock();

    //DEBUG_MESSAGE( "CHiVpss::HiVpss_release_vpss(purpose:%d)(chn_num:%d)\n", purpose, chn_num);

    return ret_val;
}


int CHiVpss::HiVpss_set_vpssgrp_cropcfg(bool enable, VPSS_GRP vpss_grp, av_rect_t *pRect, unsigned int maxw, unsigned int maxh)
{
    int ret = 0;
    VPSS_CROP_INFO_S VpssClipInfo;

    if(pgAvDeviceInfo->Adi_get_vi_vpss_mode())
    {
        DEBUG_MESSAGE( "online mode, not support group crop! \n");
        return -1;
    }
    
    if((pRect->w != 0) && (pRect->h != 0))
    {
        VpssClipInfo.bEnable = HI_TRUE;
        VpssClipInfo.enCropCoordinate = VPSS_CROP_ABS_COOR;
        VpssClipInfo.stCropRect.s32X = ALIGN_BACK(pRect->x, 4);
        VpssClipInfo.stCropRect.s32Y = ALIGN_BACK(pRect->y, 4);
        VpssClipInfo.stCropRect.u32Width = ALIGN_BACK(pRect->w, 16);
        VpssClipInfo.stCropRect.u32Height = ALIGN_BACK(pRect->h, 16);

        if (VpssClipInfo.stCropRect.u32Width > maxw)
        {
            VpssClipInfo.stCropRect.u32Width = maxw;
        }

        if (VpssClipInfo.stCropRect.u32Height > maxh)
        {
            VpssClipInfo.stCropRect.u32Height = maxh;
        }

        if((VpssClipInfo.stCropRect.s32X+VpssClipInfo.stCropRect.u32Width) > maxw)
        {
            VpssClipInfo.stCropRect.s32X = maxw - VpssClipInfo.stCropRect.u32Width + 4;
            VpssClipInfo.stCropRect.s32X = ALIGN_BACK(VpssClipInfo.stCropRect.s32X, 4);
        }

        if((VpssClipInfo.stCropRect.s32Y+VpssClipInfo.stCropRect.u32Height) > maxh)
        {
            VpssClipInfo.stCropRect.s32Y = maxh -VpssClipInfo.stCropRect.u32Height + 4;
            VpssClipInfo.stCropRect.s32Y = ALIGN_BACK(VpssClipInfo.stCropRect.s32Y, 4);
        }
    }
    else
    {
        ret = HI_MPI_VPSS_GetGrpCrop(vpss_grp, &VpssClipInfo);
        if(ret != 0)
        {
            DEBUG_MESSAGE( "HI_MPI_VPSS_GetCropCfg err:0x%08x, VpssGrp %d\n", ret, vpss_grp);
        }
    }

    if (true == enable)
    {
        VpssClipInfo.bEnable = HI_TRUE;
    }
    else
    {
        VpssClipInfo.bEnable = HI_FALSE;
    }
    DEBUG_MESSAGE("\n\nSetCropCfg [%d, %d, %d, %d] width:%d height:%d\n\n", VpssClipInfo.stCropRect.s32X, VpssClipInfo.stCropRect.s32Y,
                              VpssClipInfo.stCropRect.u32Width, VpssClipInfo.stCropRect.u32Height, maxw, maxh);
    
    ret = HI_MPI_VPSS_SetGrpCrop(vpss_grp, &VpssClipInfo);
    if(ret != 0)
    {
        DEBUG_MESSAGE( "HI_MPI_VPSS_SetClipCfg err:0x%08x, VpssGrp %d\n", ret, vpss_grp);
        DEBUG_MESSAGE("HI_MPI_VPSS_SetClipCfg err:0x%08x, VpssGrp %d\n", ret, vpss_grp);
        return -1;
    }
    return 0;
}

int CHiVpss::HiVpss_send_user_frame(VPSS_GRP vpss_grp, VIDEO_FRAME_INFO_S *pstVideoFrame)
{

    if(pgAvDeviceInfo->Adi_get_vi_vpss_mode())
    {
        DEBUG_ERROR("vi_vpss online mode not support HiVpss_send_user_frame \n");
        return -1;
    }
    
    int ret_val = HI_MPI_VPSS_SendFrame(vpss_grp, pstVideoFrame, -1);
    if (ret_val != 0)
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_send_user_frame HI_MPI_VPSS_UserSendFrame failed with 0x%x,group=%d\n", ret_val, vpss_grp);
        //return -1;
    }
    
    return 0;
}


int CHiVpss::HiVpss_ResetVpss( vpss_purpose_t purpose, int chn_num )
{
    VPSS_GRP vpss_grp;
    VPSS_CHN vpss_chn;
    unsigned long grp_chn_bitmap;

    m_pThread_lock->lock();
    if ( HiVpss_allotter( purpose, chn_num, &vpss_grp, &vpss_chn, &grp_chn_bitmap, NULL) != 0 )
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_vpss_manager HiVpss_allotter FAILED(%d, %d)\n", purpose, chn_num);
        m_pThread_lock->unlock();
        return -1;
    }

  //  DEBUG_MESSAGE("Reset vpss group=%d vpsschn=%d purpose=%d chn=%d\n", vpss_grp, vpss_chn, purpose, chn_num );
    HI_S32 sRet = HI_MPI_VPSS_ResetGrp(vpss_grp);

    if ( sRet != HI_SUCCESS )
    {
        m_pThread_lock->unlock();
        return -1;
    }
    m_pThread_lock->unlock();
    
    return 0;
}

int CHiVpss::HiVpss_set_vpssgrp_param(av_video_image_para_t video_image)
{
    if (m_pVideo_image == NULL)
    {
        _AV_FATAL_((m_pVideo_image = new av_video_image_para_t) == NULL, "OUT OF MEMORY\n");
    }
    memcpy(m_pVideo_image, &video_image, sizeof(av_video_image_para_t));
    return 0;
}

int CHiVpss::HiVpss_get_vpssgrp_param(av_video_image_para_t &video_image)
{
    if (m_pVideo_image)
    {
        memcpy(&video_image, m_pVideo_image, sizeof(av_video_image_para_t));
    }
    return 0;
}


int CHiVpss::HiVpss_get_grp_param(VPSS_GRP vpss_grp, VPSS_GRP_PARAM_S &vpss_grp_para, vpss_purpose_t purpose)
{
    HI_S32 ret_val = HI_SUCCESS;
    
    if((ret_val = HI_MPI_VPSS_GetGrpParam(vpss_grp, &vpss_grp_para)) != HI_SUCCESS)
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_get_grp_param HI_MPI_VPSS_GetGrpParam FAILED(group:%d)(ret_val:0x%08lx)\n", vpss_grp, (unsigned long)ret_val);
        return -1;
    }

    vpss_grp_para.s32GlobalStrength = 128;
    vpss_grp_para.u32Contrast = 0;
    vpss_grp_para.s32CSFStrength = -1; //!default 
    vpss_grp_para.s32CTFStrength = -1;
    vpss_grp_para.s32YSFStrength = -1;
    vpss_grp_para.s32YTFStrength = -1;
    vpss_grp_para.s32IeStrength = -1;
    vpss_grp_para.s32MotionLimen = -1;
    
    return 0;
}

#ifdef _AV_PRODUCT_CLASS_IPC_
#if defined(_AV_PLATFORM_HI3516A_)
int CHiVpss::HiVpss_3DNRFilter(vpss_purpose_t purpose, int chn_num, const vpss3DNRParam_t &stu3DNRParam)
{
     VPSS_GRP vpssGrp;
    if( HiVpss_find_vpss(purpose, chn_num, &vpssGrp, NULL, NULL) != 0 )
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_3DNoiseFilter HiVpss_find_vpss failed\n");
        return -1;
    }

    tVppNRsEx stuVpssGrpParam;
    HI_S32 sRet = HI_SUCCESS;

    memset(&stuVpssGrpParam, 0, sizeof(tVppNRsEx));
    
    stuVpssGrpParam.Chroma_SF_Strength = stu3DNRParam.u8ChromaSFStrength;
    stuVpssGrpParam.Chroma_TF_Strength = stu3DNRParam.u8ChromaTFStrength>32?32:stu3DNRParam.u8ChromaTFStrength;
    stuVpssGrpParam.IE_PostFlag = stu3DNRParam.u8IEPostFlag>1?1:stu3DNRParam.u8IEPostFlag;
    stuVpssGrpParam.IE_Strength = stu3DNRParam.u16IEStrength>63?63:stu3DNRParam.u16IEStrength;
    stuVpssGrpParam.Luma_MotionThresh = stu3DNRParam.u16LumaMotionThresh>511?511:stu3DNRParam.u16LumaMotionThresh;
    stuVpssGrpParam.Luma_SF_MoveArea = stu3DNRParam.u8LumaSFMoveArea;
    stuVpssGrpParam.Luma_SF_StillArea = stu3DNRParam.u8LumaSFStillArea>64?64:stu3DNRParam.u8LumaSFStillArea;
    stuVpssGrpParam.Luma_TF_Strength = stu3DNRParam.u8LumaTFStrength>15?15:stu3DNRParam.u8LumaTFStrength;
    stuVpssGrpParam.DeSand_Strength = stu3DNRParam.u8DeSandStrength>8?8:stu3DNRParam.u8DeSandStrength;
#if 0
    printf("[alex]vpss group:%d CSS:%d CTS:%d IEPF:%d IES:%d LMT:%d LSM:%d LSS:%d LTS:%d DSS:%d \n", vpssGrp, stuVpssGrpParam.Chroma_SF_Strength, \
        stuVpssGrpParam.Chroma_TF_Strength, stuVpssGrpParam.IE_PostFlag, stuVpssGrpParam.IE_Strength, stuVpssGrpParam.Luma_MotionThresh, 
        stuVpssGrpParam.Luma_SF_MoveArea, stuVpssGrpParam.Luma_SF_StillArea, stuVpssGrpParam.Luma_TF_Strength, stuVpssGrpParam.DeSand_Strength);
#endif
    sRet = HI_MPI_VPSS_SetGrpParamV2( vpssGrp, &stuVpssGrpParam );
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_3DNoiseFilter HI_MPI_VPSS_SetGrpParam failed width 0x%08x\n", sRet);
        return -1;
    }

    return 0;   
}
#endif

#if defined(_AV_PLATFORM_HI3518EV200_)
int CHiVpss::HiVpss_3DNRFilter(vpss_purpose_t purpose, int chn_num, const VpssGlb3DNRParamV1_t &stu3DNRParam)
{
    VPSS_GRP vpss_grp;
    VPSS_NR_PARAM_U vpss_nr_param;
    HI_S32 sRet = HI_SUCCESS;
    
    if( HiVpss_find_vpss(purpose, chn_num, &vpss_grp, NULL, NULL) != 0 )
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_3DNoiseFilter HiVpss_find_vpss failed\n");
        return -1;
    }

    memset(&vpss_nr_param, 0, sizeof(VPSS_NR_PARAM_U));

    vpss_nr_param.stNRParam_V1.s32YPKStr = stu3DNRParam.s32YPKStr;
    vpss_nr_param.stNRParam_V1.s32YSFStr = stu3DNRParam.s32YSFStr;
    vpss_nr_param.stNRParam_V1.s32YTFStr = stu3DNRParam.s32YTFStr;
    vpss_nr_param.stNRParam_V1.s32TFStrMax = stu3DNRParam.s32TFStrMax;
    vpss_nr_param.stNRParam_V1.s32YSmthStr = stu3DNRParam.s32YSmthStr;
    vpss_nr_param.stNRParam_V1.s32YSmthRat = stu3DNRParam.s32YSmthRat;
    vpss_nr_param.stNRParam_V1.s32YSFStrDlt = stu3DNRParam.s32YSFStrDlt;
    vpss_nr_param.stNRParam_V1.s32YTFStrDlt = stu3DNRParam.s32YTFStrDlt;
    vpss_nr_param.stNRParam_V1.s32YTFStrDl = stu3DNRParam.s32YTFStrDl;
    vpss_nr_param.stNRParam_V1.s32YSFBriRat = stu3DNRParam.s32YSFBriRat;
    vpss_nr_param.stNRParam_V1.s32CSFStr = stu3DNRParam.s32CSFStr;
    vpss_nr_param.stNRParam_V1.s32CTFstr = stu3DNRParam.s32CTFstr;
        
    sRet = HI_MPI_VPSS_SetNRParam( vpss_grp, &vpss_nr_param);
    if( sRet != HI_SUCCESS )
    {
        DEBUG_ERROR( "CHiVpss::HiVpss_3DNoiseFilter HI_MPI_VPSS_SetNRParam failed width 0x%08x\n", sRet);
        return -1;
    }

    return 0;  
}
#endif

int CHiVpss::HiVpss_get_frame(vpss_purpose_t purpose, int chn_num, VIDEO_FRAME_INFO_S *pstVideoFrame)
{
    VPSS_GRP vpssGrp;
    VPSS_CHN vpssChn;
    signed char u8GetFrameTime = 3;

    if( HiVpss_find_vpss( purpose, chn_num, &vpssGrp, &vpssChn ) != 0 )
    {
        _AV_KEY_INFO_(_AVD_VPSS_, "CHiVpss::HiVpss_get_frame HiVpss_find_vpss failed\n");
        return -1;
    }

    unsigned int depth = 0;
    HI_S32 eRet = HI_FAILURE;
    HI_MPI_VPSS_GetDepth(vpssGrp, vpssChn, &depth);
    if(depth <= 0)
    {
#if defined(_AV_IVE_BLE_) || defined(_AV_IVE_LD_)
        if (0 != (eRet = HI_MPI_VPSS_SetDepth(vpssGrp,vpssChn,8)))
#else
        if (0 != HI_MPI_VPSS_SetDepth(vpssGrp,vpssChn,2))
#endif
        {
            _AV_KEY_INFO_(_AVD_VPSS_, "HI_MPI_VPSS_SetDepth failed! errcode:0x%x \n", eRet);
            return -1;
        }        
    }

    while((u8GetFrameTime > 0) && (HI_SUCCESS != HI_MPI_VPSS_GetChnFrame( vpssGrp, vpssChn, pstVideoFrame, 300)) )
    {
        --u8GetFrameTime;
        usleep(100*1000);
    }

    if(u8GetFrameTime > 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int CHiVpss::HiVpss_release_frame(vpss_purpose_t purpose,int chn_num, VIDEO_FRAME_INFO_S *pstVideoFrame)
{
    VPSS_GRP vpssGrp;
    VPSS_CHN vpssChn;
    int ret = 0;

    if( HiVpss_find_vpss( purpose, chn_num, &vpssGrp, &vpssChn ) != 0 )
    {
        _AV_KEY_INFO_(_AVD_VPSS_, "CHiVpss::HiVpss_release_frame HiVpss_find_vpss failed\n");
        ret = -1;
    }
    
    HI_S32 sRet = HI_MPI_VPSS_ReleaseChnFrame( vpssGrp, vpssChn, pstVideoFrame );
    if( sRet != HI_SUCCESS )
    {
        if(HI_SUCCESS != (sRet = HI_MPI_VPSS_ReleaseChnFrame( vpssGrp, vpssChn, pstVideoFrame )))
        {
            _AV_KEY_INFO_(_AVD_VPSS_, " HI_MPI_VPSS_ReleaseChnFrame failed,[group:%d chn:%d]errorcode: 0x%08x\n", vpssGrp, vpssChn,sRet);
            ret = -1;            
        }
    }
    return ret;
}
#endif


#if defined(_AV_PLATFORM_HI3516A_) || defined(_AV_PLATFORM_HI3518EV200_)
int CHiVpss::HiVpss_set_ldc_attr( int phy_chn_num, const VPSS_LDC_ATTR_S* pstLDCAttr )
{
#if defined(_AV_PLATFORM_HI3518EV200_)
    return 0;
#endif

    VPSS_GRP vpssGrp;
    VPSS_CHN vpssChn;
    HI_S32 nRet = HI_SUCCESS;
    do
    {
        if( HiVpss_find_vpss( _VP_MAIN_STREAM_, phy_chn_num, &vpssGrp, &vpssChn ) != 0 )
        {
            _AV_KEY_INFO_(_AVD_VPSS_, "CHiVpss::HiVpss_get_frame HiVpss_find_vpss failed\n");
            nRet = HI_FAILURE;
            break;
        }

        nRet = HI_MPI_VPSS_SetLDCAttr(vpssGrp , vpssChn, pstLDCAttr);
        if(HI_SUCCESS != nRet)
        {
             _AV_KEY_INFO_(_AVD_VPSS_, " HI_MPI_VPSS_SetLDCAttr failed! errorCode:0x%#x \n", nRet);
             nRet = HI_FAILURE;
             break;
        }
     }while(0);

     do
    {
        if( HiVpss_find_vpss( _VP_SUB_STREAM_, phy_chn_num, &vpssGrp, &vpssChn ) != 0 )
        {
            _AV_KEY_INFO_(_AVD_VPSS_, "CHiVpss::HiVpss_get_frame HiVpss_find_vpss failed\n");
            nRet = HI_FAILURE;
            break;
        }

        nRet = HI_MPI_VPSS_SetLDCAttr(vpssGrp , vpssChn, pstLDCAttr);
        if(HI_SUCCESS != nRet)
        {
             _AV_KEY_INFO_(_AVD_VPSS_, " HI_MPI_VPSS_SetLDCAttr failed! errorCode:0x%#x \n", nRet);
             nRet = HI_FAILURE;
             break;
        }
     }while(0);

    do
    {
        if( HiVpss_find_vpss( _VP_SUB2_STREAM_, phy_chn_num, &vpssGrp, &vpssChn ) != 0 )
        {
            _AV_KEY_INFO_(_AVD_VPSS_, "CHiVpss::HiVpss_get_frame HiVpss_find_vpss failed\n");
            nRet = HI_FAILURE;
            break;
        }

        nRet = HI_MPI_VPSS_SetLDCAttr(vpssGrp , vpssChn, pstLDCAttr);
        if(HI_SUCCESS != nRet)
        {
             _AV_KEY_INFO_(_AVD_VPSS_, " HI_MPI_VPSS_SetLDCAttr failed! errorCode:0x%#x \n", nRet);
             nRet = HI_FAILURE;
             break;
        }
     }while(0);

#if defined(_AV_SUPPORT_IVE_)
    do
    {
        if( HiVpss_find_vpss( _VP_SPEC_USE_, phy_chn_num, &vpssGrp, &vpssChn ) != 0 )
        {
            _AV_KEY_INFO_(_AVD_VPSS_, "CHiVpss::HiVpss_get_frame HiVpss_find_vpss failed\n");
            nRet = HI_FAILURE;
            break;
        }

        nRet = HI_MPI_VPSS_SetLDCAttr(vpssGrp , vpssChn, pstLDCAttr);
        if(HI_SUCCESS != nRet)
        {
             _AV_KEY_INFO_(_AVD_VPSS_, " HI_MPI_VPSS_SetLDCAttr failed! errorCode:0x%#x \n", nRet);
             nRet = HI_FAILURE;
             break;
        }
     }while(0);

#endif
    
    return nRet;
}

int CHiVpss::HiVpss_set_Rotate(int phy_chn_num, unsigned char rotate)
{
    VPSS_GRP vpssGrp;
    VPSS_CHN vpssChn;
    HI_S32 nRet = HI_SUCCESS;

    if((rotate<=0) || (rotate > 3))
    {
         return 0;
    }

    do
    {
        if( HiVpss_find_vpss( _VP_MAIN_STREAM_, phy_chn_num, &vpssGrp, &vpssChn ) != 0 )
        {
            _AV_KEY_INFO_(_AVD_VPSS_, "CHiVpss::HiVpss_get_frame HiVpss_find_vpss failed\n");
            nRet = HI_FAILURE;
            break;
        }
        
        nRet = HI_MPI_VPSS_SetRotate(vpssGrp , vpssChn, (ROTATE_E)rotate);
        if(HI_SUCCESS != nRet)
        {
             _AV_KEY_INFO_(_AVD_VPSS_, " HI_MPI_VPSS_SetLDCAttr failed! errorCode:0x%#x \n", nRet);
             nRet = HI_FAILURE;
             break;
        }
    }while(0);

    do
    {
        if( HiVpss_find_vpss( _VP_SUB_STREAM_, phy_chn_num, &vpssGrp, &vpssChn ) != 0 )
        {
            _AV_KEY_INFO_(_AVD_VPSS_, "CHiVpss::HiVpss_get_frame HiVpss_find_vpss failed\n");
            nRet = HI_FAILURE;
            break;
        }
        
        nRet = HI_MPI_VPSS_SetRotate(vpssGrp , vpssChn, (ROTATE_E)rotate);
        if(HI_SUCCESS != nRet)
        {
             _AV_KEY_INFO_(_AVD_VPSS_, " HI_MPI_VPSS_SetLDCAttr failed! errorCode:0x%#x \n", nRet);
             nRet = HI_FAILURE;
             break;
        }
    }while(0);

    do
    {
        if( HiVpss_find_vpss( _VP_SUB2_STREAM_, phy_chn_num, &vpssGrp, &vpssChn ) != 0 )
        {
            _AV_KEY_INFO_(_AVD_VPSS_, "CHiVpss::HiVpss_get_frame HiVpss_find_vpss failed\n");
            nRet = HI_FAILURE;
            break;
        }
        
        nRet = HI_MPI_VPSS_SetRotate(vpssGrp , vpssChn, (ROTATE_E)rotate);
        if(HI_SUCCESS != nRet)
        {
             _AV_KEY_INFO_(_AVD_VPSS_, " HI_MPI_VPSS_SetLDCAttr failed! errorCode:0x%#x \n", nRet);
             nRet = HI_FAILURE;
             break;
        }
    }while(0);

#if defined(_AV_SUPPORT_IVE_)
    do
    {
        if( HiVpss_find_vpss( _VP_SPEC_USE_, phy_chn_num, &vpssGrp, &vpssChn ) != 0 )
        {
            _AV_KEY_INFO_(_AVD_VPSS_, "CHiVpss::HiVpss_get_frame HiVpss_find_vpss failed\n");
            nRet = HI_FAILURE;
            break;
        }
        
        nRet = HI_MPI_VPSS_SetRotate(vpssGrp , vpssChn, (ROTATE_E)rotate);
        if(HI_SUCCESS != nRet)
        {
             _AV_KEY_INFO_(_AVD_VPSS_, " HI_MPI_VPSS_SetLDCAttr failed! errorCode:0x%#x \n", nRet);
             nRet = HI_FAILURE;
             break;
        }
    }while(0);
#endif
    
    return nRet;    
}

int CHiVpss::HiVpss_set_mirror_flip(int phy_chn_num, bool is_mirror, bool is_flip)
{
    VPSS_GRP vpssGrp;
    VPSS_CHN vpssChn;
    VPSS_CHN_ATTR_S vpssChnAttr;
    HI_S32 nRet = HI_SUCCESS;

    memset(&vpssChnAttr, 0 , sizeof(VPSS_CHN_ATTR_S));

    do
    {
        if( HiVpss_find_vpss( _VP_MAIN_STREAM_, phy_chn_num, &vpssGrp, &vpssChn ) != 0 )
        {
            _AV_KEY_INFO_(_AVD_VPSS_, "CHiVpss::HiVpss_set_mirror_flip HiVpss_find_vpss failed\n");
            nRet = HI_FAILURE;
            break;
        }

        if(HI_SUCCESS != HI_MPI_VPSS_GetChnAttr(vpssGrp, vpssChn, &vpssChnAttr))
        {
            _AV_KEY_INFO_(_AVD_VPSS_, " HI_MPI_VPSS_SetChnAttr failed! errorCode:0x%#x \n", nRet);
            nRet = HI_FAILURE;
            break;
        }

        vpssChnAttr.bMirror = is_mirror?HI_TRUE:HI_FALSE;
        vpssChnAttr.bFlip = is_flip?HI_TRUE:HI_FALSE;
        
        nRet = HI_MPI_VPSS_SetChnAttr(vpssGrp , vpssChn,  &vpssChnAttr);
        if(HI_SUCCESS != nRet)
        {
             _AV_KEY_INFO_(_AVD_VPSS_, " HI_MPI_VPSS_SetChnAttr failed! errorCode:0x%#x \n", nRet);
             nRet = HI_FAILURE;
             break;
        }
    }while(0);

    do
    {
        if( HiVpss_find_vpss( _VP_SUB_STREAM_, phy_chn_num, &vpssGrp, &vpssChn ) != 0 )
        {
            _AV_KEY_INFO_(_AVD_VPSS_, "CHiVpss::HiVpss_set_mirror_flip HiVpss_find_vpss failed\n");
            nRet = HI_FAILURE;
            break;
        }

        if(HI_SUCCESS != HI_MPI_VPSS_GetChnAttr(vpssGrp, vpssChn, &vpssChnAttr))
        {
            _AV_KEY_INFO_(_AVD_VPSS_, " HI_MPI_VPSS_SetChnAttr failed! errorCode:0x%#x \n", nRet);
            nRet = HI_FAILURE;
            break;
        }

        vpssChnAttr.bMirror = is_mirror?HI_TRUE:HI_FALSE;
        vpssChnAttr.bFlip = is_flip?HI_TRUE:HI_FALSE;
        
        nRet = HI_MPI_VPSS_SetChnAttr(vpssGrp , vpssChn,  &vpssChnAttr);
        if(HI_SUCCESS != nRet)
        {
             _AV_KEY_INFO_(_AVD_VPSS_, " HI_MPI_VPSS_SetChnAttr failed! errorCode:0x%#x \n", nRet);
             nRet = HI_FAILURE;
             break;
        }
    }while(0);

    do
    {
        if( HiVpss_find_vpss( _VP_SUB2_STREAM_, phy_chn_num, &vpssGrp, &vpssChn ) != 0 )
        {
            _AV_KEY_INFO_(_AVD_VPSS_, "CHiVpss::HiVpss_set_mirror_flip HiVpss_find_vpss failed\n");
            nRet = HI_FAILURE;
            break;
        }

        if(HI_SUCCESS != HI_MPI_VPSS_GetChnAttr(vpssGrp, vpssChn, &vpssChnAttr))
        {
            _AV_KEY_INFO_(_AVD_VPSS_, " HI_MPI_VPSS_SetChnAttr failed! errorCode:0x%#x \n", nRet);
            nRet = HI_FAILURE;
            break;
        }

        vpssChnAttr.bMirror = is_mirror?HI_TRUE:HI_FALSE;
        vpssChnAttr.bFlip = is_flip?HI_TRUE:HI_FALSE;
        
        nRet = HI_MPI_VPSS_SetChnAttr(vpssGrp , vpssChn,  &vpssChnAttr);
        if(HI_SUCCESS != nRet)
        {
             _AV_KEY_INFO_(_AVD_VPSS_, " HI_MPI_VPSS_SetChnAttr failed! errorCode:0x%#x \n", nRet);
             nRet = HI_FAILURE;
             break;
        }
    }while(0);

#if defined(_AV_SUPPORT_IVE_)
    do
    {
        if( HiVpss_find_vpss( _VP_SPEC_USE_, phy_chn_num, &vpssGrp, &vpssChn ) != 0 )
        {
            _AV_KEY_INFO_(_AVD_VPSS_, "CHiVpss::HiVpss_set_mirror_flip HiVpss_find_vpss failed\n");
            nRet = HI_FAILURE;
            break;
        }

        if(HI_SUCCESS != HI_MPI_VPSS_GetChnAttr(vpssGrp, vpssChn, &vpssChnAttr))
        {
            _AV_KEY_INFO_(_AVD_VPSS_, " HI_MPI_VPSS_SetChnAttr failed! errorCode:0x%#x \n", nRet);
            nRet = HI_FAILURE;
            break;
        }

        vpssChnAttr.bMirror = is_mirror?HI_TRUE:HI_FALSE;
        vpssChnAttr.bFlip = is_flip?HI_TRUE:HI_FALSE;
        
        nRet = HI_MPI_VPSS_SetChnAttr(vpssGrp , vpssChn,  &vpssChnAttr);
        if(HI_SUCCESS != nRet)
        {
             _AV_KEY_INFO_(_AVD_VPSS_, " HI_MPI_VPSS_SetChnAttr failed! errorCode:0x%#x \n", nRet);
             nRet = HI_FAILURE;
             break;
        }
    }while(0);

#endif
    
    return nRet;    
}
#endif

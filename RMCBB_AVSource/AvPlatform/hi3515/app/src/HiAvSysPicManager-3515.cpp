#include "HiAvSysPicManager-3515.h"
#include "AvDebug.h"
#include "AvCommonMacro.h"
#include "AvCommonType.h"
#include "hi_comm_video.h"
#include "mpi_vb.h"
#include "hi_comm_vb.h"
#include "mpi_sys.h"

CHiAvSysPicManager::CHiAvSysPicManager()
{
    m_vb_info.clear();
    m_pic_info.clear();
}

CHiAvSysPicManager::~CHiAvSysPicManager()
{
}

int CHiAvSysPicManager::Aspm_get_picture(av_sys_pic_type_e pic_type, void * pPicture)
{
    _AV_FATAL_(pPicture == NULL, "CHiAvSysPicManager::Aspm_get_picture pPicture is NULL! (pic_type:%d) \n", pic_type);
    int retval = -1;
    VIDEO_FRAME_INFO_S *pFrame = (VIDEO_FRAME_INFO_S *)pPicture;

    if(m_vb_info.find(pic_type) != m_vb_info.end())
    {
        pFrame->u32PoolId = m_vb_info[pic_type].vb_pool_id;
        pFrame->stVFrame.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        pFrame->stVFrame.u32Width = m_pic_info[pic_type].pic_width;
        pFrame->stVFrame.u32Height = m_pic_info[pic_type].pic_height;
        pFrame->stVFrame.u32Field = VIDEO_FIELD_INTERLACED;
        pFrame->stVFrame.u32PhyAddr[0] = m_vb_info[pic_type].vb_phy_Addr;
        pFrame->stVFrame.u32PhyAddr[1] = m_vb_info[pic_type].vb_phy_Addr + m_vb_info[pic_type].luma_size;
        pFrame->stVFrame.u32PhyAddr[2] = m_vb_info[pic_type].vb_phy_Addr + m_vb_info[pic_type].luma_size + m_vb_info[pic_type].chrm_size;
        pFrame->stVFrame.pVirAddr[0] = m_vb_info[pic_type].pVb_vir_addr;
        pFrame->stVFrame.pVirAddr[1] = (void *)((unsigned int)m_vb_info[pic_type].pVb_vir_addr + m_vb_info[pic_type].luma_size);
        pFrame->stVFrame.pVirAddr[2] = (void *)((unsigned int)m_vb_info[pic_type].pVb_vir_addr + m_vb_info[pic_type].luma_size + m_vb_info[pic_type].chrm_size);
        pFrame->stVFrame.u32Stride[0] = m_pic_info[pic_type].pic_stride;
        pFrame->stVFrame.u32Stride[1] = m_pic_info[pic_type].pic_stride;
        pFrame->stVFrame.u32Stride[2] = m_pic_info[pic_type].pic_stride;
//add by cxliu,151119
#if defined(_AV_PLATFORM_HI3520D_V300_)
        pFrame->stVFrame.enCompressMode = COMPRESS_MODE_NONE;
        pFrame->stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
#endif		
        retval = 0;
    }

    return retval;
}

int CHiAvSysPicManager::Aspm_get_pic_buffer(av_sys_pic_type_e pic_type, av_sys_pic_info_t pic_info, av_sys_buffer_info_t &buf_info)
{
    buf_info.luma_size  = pic_info.pic_stride * pic_info.pic_height;
    buf_info.chrm_size  = (pic_info.pic_stride * pic_info.pic_height) >> 2;
    buf_info.total_size = buf_info.luma_size + (buf_info.chrm_size << 1);

    m_vb_info[pic_type].luma_size  = buf_info.luma_size;
    m_vb_info[pic_type].chrm_size = buf_info.chrm_size;
    m_vb_info[pic_type].total_size = buf_info.total_size;

    m_pic_info[pic_type] = pic_info;
#define   _AV_PLATFORM_HI3515_
#if defined(_AV_PLATFORM_HI3515_)
    if ((m_vb_info[pic_type].vb_block = HI_MPI_VB_GetBlock(VB_INVALID_POOLID, m_vb_info[pic_type].total_size)) == VB_INVALID_HANDLE)
#else
    if ((m_vb_info[pic_type].vb_block = HI_MPI_VB_GetBlock(VB_INVALID_POOLID, m_vb_info[pic_type].total_size, NULL)) == VB_INVALID_HANDLE)
#endif
    {
        DEBUG_ERROR( "CHiAvSysPicManager::Aspm_get_pic_buffer HI_MPI_VB_GetBlock FAILED! \n");
        return -1;
    }

    if ((m_vb_info[pic_type].vb_phy_Addr = HI_MPI_VB_Handle2PhysAddr(m_vb_info[pic_type].vb_block)) == 0)
    {
        DEBUG_ERROR( "CHiAvSysPicManager::Aspm_get_pic_buffer HI_MPI_VB_Handle2PhysAddr FAILED! \n");
        return -1;
    }

    if ((m_vb_info[pic_type].pVb_vir_addr = (unsigned char *)HI_MPI_SYS_Mmap(m_vb_info[pic_type].vb_phy_Addr, m_vb_info[pic_type].total_size)) == NULL)
    {
        DEBUG_ERROR( "CHiAvSysPicManager::Aspm_get_pic_buffer HI_MPI_SYS_Mmap FAILED! \n");
        return -1;
    }

    if ((m_vb_info[pic_type].vb_pool_id = HI_MPI_VB_Handle2PoolId(m_vb_info[pic_type].vb_block)) == VB_INVALID_POOLID)
    {
        DEBUG_ERROR( "CHiAvSysPicManager::Aspm_get_pic_buffer HI_MPI_VB_Handle2PoolId FAILED! \n");
        return -1;
    }

    buf_info.pY = m_vb_info[pic_type].pVb_vir_addr;
    buf_info.pU = (unsigned char *)((unsigned int)buf_info.pY + buf_info.luma_size);
    buf_info.pV = (unsigned char *)((unsigned int)buf_info.pU + buf_info.chrm_size);
    
    return 0;
}

int CHiAvSysPicManager::Aspm_release_pic_buffer(av_sys_pic_type_e pic_type)
{
    int ret_val = 0;
    if ((ret_val = HI_MPI_VB_ReleaseBlock(m_vb_info[pic_type].vb_block)) != 0)
    {
        DEBUG_ERROR( "CHiAvSysPicManager::Aspm_release_pic_buffer HI_MPI_VB_ReleaseBlock FAILED (pic_type:%d) (0x%08lx)\n", pic_type, (unsigned long)ret_val);
        return -1;
    }
    
    return 0;
}


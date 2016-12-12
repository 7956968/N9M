#include "HiAvOd.h"

#include "mpi_ive.h"

#include <math.h>

using namespace Common;

CHiAvIveOd::CHiAvIveOd():
    m_od_block_num_row(8),m_od_block_num_line(8)
{
#if defined(_AV_PLATFORM_HI3518C_)
    m_pu64VirData = NULL;
#endif
}

CHiAvIveOd::~CHiAvIveOd()
{
}

int CHiAvIveOd::av_od_init(HI_S32 occlusion_thresh_percent)
{
#if defined(_AV_PLATFORM_HI3518C_)
    memset(&m_dst_mem_info, 0, sizeof(IVE_MEM_INFO_S));
#else  
    memset(&m_src_image, 0, sizeof(IVE_SRC_IMAGE_S));
    memset(&m_dest_image, 0, sizeof(IVE_DST_IMAGE_S));
#endif
    memset(&m_linear_data, 0, sizeof(OD_LINEAR_DATA_S));

    m_linear_data.s32ThreshNum = occlusion_thresh_percent*m_od_block_num_row*m_od_block_num_line/100;
    m_linear_data.s32LinearNum = 2;
    m_linear_data.pstLinearPoint = new POINT_S[m_linear_data.s32LinearNum];
    m_linear_data.pstLinearPoint[0].s32X = 0;
    m_linear_data.pstLinearPoint[0].s32Y = 15;
    m_linear_data.pstLinearPoint[1].s32X = 100;
    m_linear_data.pstLinearPoint[1].s32Y = 20; 

    return 0;
}

int CHiAvIveOd::av_od_uninit()
{
    if(NULL != m_linear_data.pstLinearPoint)
    {
        delete []m_linear_data.pstLinearPoint;
    }
#if defined(_AV_PLATFORM_HI3518C_)
    if(0 != m_dst_mem_info.u32PhyAddr)
    {
        HI_MPI_SYS_MmzFree(m_dst_mem_info.u32PhyAddr, (HI_VOID *)m_pu64VirData);
    }
#else
    if(m_src_image.u32PhyAddr[0] != 0 && m_src_image.pu8VirAddr[0] != NULL)
    {
        HI_MPI_SYS_MmzFree(m_src_image.u32PhyAddr[0],  (HI_VOID *)m_src_image.pu8VirAddr[0]);
        m_src_image.u32PhyAddr[0] = 0;
        m_src_image.pu8VirAddr[0] = NULL;
    }
    
    if(m_dest_image.u32PhyAddr[0] != 0 && m_dest_image.pu8VirAddr[0] != NULL)
    {
        HI_MPI_SYS_MmzFree(m_dest_image.u32PhyAddr[0],  (HI_VOID *)m_dest_image.pu8VirAddr[0]);
        m_dest_image.u32PhyAddr[0] = 0;
        m_dest_image.pu8VirAddr[0] = NULL;
    }    
#endif
    return 0;    
}

int CHiAvIveOd::av_od_change_linear_data(HI_S32 occlusion_thresh_percent)
{
    m_linear_data.s32ThreshNum = occlusion_thresh_percent*m_od_block_num_row*m_od_block_num_line/100;
    return 0;
}

int CHiAvIveOd::av_od_analyse_frame(const VIDEO_FRAME_INFO_S& frame_info)
{
#if defined(_AV_PLATFORM_HI3516A_)
    HI_S32 s32Ret = HI_FAILURE;
    IVE_HANDLE hIveHandle;
    IVE_INTEG_CTRL_S integ_ctrl; 
    HI_S32 w,h,od_block_num;
    HI_U32 block_num = m_od_block_num_line*m_od_block_num_row;
    POINT_S stChar[block_num];
    IVE_DATA_S src_data;
    IVE_DST_DATA_S dest_data;
    IVE_DMA_CTRL_S dma_ctrl = {IVE_DMA_MODE_DIRECT_COPY, 0, 0, 0, 0};
    HI_BOOL bFinish = HI_FALSE;
    HI_U64 * pu64VirData = NULL;
    
    if((0 == m_src_image.u32PhyAddr[0]) || (NULL == m_src_image.pu8VirAddr[0]) )
    {
        HI_U32 u32Size = 0;
        
        m_src_image.enType = IVE_IMAGE_TYPE_U8C1;//olny copy luma info
        m_src_image.u16Width = frame_info.stVFrame.u32Width;
        m_src_image.u16Height = frame_info.stVFrame.u32Height;
        m_src_image.u16Stride[0] = frame_info.stVFrame.u32Stride[0];

        u32Size = m_src_image.u16Stride[0]*m_src_image.u16Height;
        s32Ret = HI_MPI_SYS_MmzAlloc(&m_src_image.u32PhyAddr[0], (HI_VOID **)&m_src_image.pu8VirAddr[0], "user_od", HI_NULL, u32Size);
    	if(s32Ret != HI_SUCCESS)
        {
            DEBUG_ERROR("malloc src image memory failed!errorcode:0x%x\n",s32Ret);
            return -1;
        }
    
        m_dest_image.enType = IVE_IMAGE_TYPE_U64C1;//olny copy luma info
        m_dest_image.u16Width = frame_info.stVFrame.u32Width;
        m_dest_image.u16Height = frame_info.stVFrame.u32Height;
        m_dest_image.u16Stride[0] = frame_info.stVFrame.u32Stride[0];
        
        u32Size = m_dest_image.u16Stride[0]*m_dest_image.u16Height*8;
        s32Ret = HI_MPI_SYS_MmzAlloc(&m_dest_image.u32PhyAddr[0], (HI_VOID **)&m_dest_image.pu8VirAddr[0], "user_od", HI_NULL, u32Size);
        if(s32Ret != HI_SUCCESS)
        {
            HI_MPI_SYS_MmzFree(m_src_image.u32PhyAddr[0],  (HI_VOID *)m_src_image.pu8VirAddr[0]);
            m_src_image.u32PhyAddr[0] = 0;
            m_src_image.pu8VirAddr[0] = NULL;
            DEBUG_ERROR("malloc dest image memory failed!errorcode:0x%x\n",s32Ret);
            return -1;
        }
    }
    else if(m_src_image.u16Width != frame_info.stVFrame.u32Width)
    {
        HI_MPI_SYS_MmzFree(m_src_image.u32PhyAddr[0],  (HI_VOID *)m_src_image.pu8VirAddr[0]);
        m_src_image.u32PhyAddr[0] = 0;
        m_src_image.pu8VirAddr[0] = NULL;

        HI_MPI_SYS_MmzFree(m_dest_image.u32PhyAddr[0],  (HI_VOID *)m_dest_image.pu8VirAddr[0]);
        m_dest_image.u32PhyAddr[0] = 0;
        m_dest_image.pu8VirAddr[0] = NULL;

        HI_U32 u32Size = 0;
        
        m_src_image.enType = IVE_IMAGE_TYPE_U8C1;//olny copy luma info
        m_src_image.u16Width = frame_info.stVFrame.u32Width;
        m_src_image.u16Height = frame_info.stVFrame.u32Height;
        m_src_image.u16Stride[0] = frame_info.stVFrame.u32Stride[0];
        
        u32Size = m_src_image.u16Stride[0]*m_src_image.u16Height;
        s32Ret = HI_MPI_SYS_MmzAlloc(&m_src_image.u32PhyAddr[0], (HI_VOID **)&m_src_image.pu8VirAddr[0], "user_od", HI_NULL, u32Size);
        if(s32Ret != HI_SUCCESS)
        {
            DEBUG_ERROR("malloc src image memory failed!errorcode:0x%x\n",s32Ret);
            return -1;
        }
        
        m_dest_image.enType = IVE_IMAGE_TYPE_U64C1;//olny copy luma info
        m_dest_image.u16Width = frame_info.stVFrame.u32Width;
        m_dest_image.u16Height = frame_info.stVFrame.u32Height;
        m_dest_image.u16Stride[0] = frame_info.stVFrame.u32Stride[0];
        
        u32Size = m_dest_image.u16Stride[0]*m_dest_image.u16Height*8;
        s32Ret = HI_MPI_SYS_MmzAlloc(&m_dest_image.u32PhyAddr[0], (HI_VOID **)&m_dest_image.pu8VirAddr[0], "user_od", HI_NULL, u32Size);
        if(s32Ret != HI_SUCCESS)
        {
            HI_MPI_SYS_MmzFree(m_src_image.u32PhyAddr[0],  (HI_VOID *)m_src_image.pu8VirAddr[0]);
            m_src_image.u32PhyAddr[0] = 0;
            m_src_image.pu8VirAddr[0] = NULL;
            DEBUG_ERROR("malloc dest image memory failed!errorcode:0x%x\n",s32Ret);
            return -1;
        }
    }

    src_data.pu8VirAddr = (HI_U8*)frame_info.stVFrame.pVirAddr[0];
    src_data.u32PhyAddr = frame_info.stVFrame.u32PhyAddr[0];
    src_data.u16Stride = (HI_U16)frame_info.stVFrame.u32Stride[0];
    src_data.u16Width = (HI_U16)frame_info.stVFrame.u32Width;
    src_data.u16Height = frame_info.stVFrame.u32Height;
    
    dest_data.pu8VirAddr = m_src_image.pu8VirAddr[0];
    dest_data.u32PhyAddr = m_src_image.u32PhyAddr[0];
    dest_data.u16Stride = (HI_U16)frame_info.stVFrame.u32Stride[0];
    dest_data.u16Width = (HI_U16)frame_info.stVFrame.u32Width;
    dest_data.u16Height = frame_info.stVFrame.u32Height;

    s32Ret = HI_MPI_IVE_DMA(&hIveHandle, &src_data, &dest_data, &dma_ctrl, HI_FALSE);
    if(s32Ret != HI_SUCCESS)
    {
        DEBUG_ERROR("HI_MPI_IVE_DMA failed! errorcode:0x%x \n", s32Ret);
        return -1;
    }

    integ_ctrl.enOutCtrl = IVE_INTEG_OUT_CTRL_COMBINE;
    w = m_src_image.u16Width/m_od_block_num_row;
    h = m_src_image.u16Height/m_od_block_num_line;

    s32Ret = HI_MPI_IVE_Integ(&hIveHandle, &m_src_image, &m_dest_image, &integ_ctrl, HI_TRUE);
    if(s32Ret != HI_SUCCESS)
    {
        DEBUG_ERROR(" HI_MPI_IVE_Integ failed ! errorcode:0x%x\n",s32Ret);
        return -1;
    } 
    
    s32Ret = HI_MPI_IVE_Query(hIveHandle, &bFinish, HI_TRUE);
    while (HI_ERR_IVE_QUERY_TIMEOUT == s32Ret)
    {
        usleep(100);
        s32Ret = HI_MPI_IVE_Query(hIveHandle, &bFinish, HI_TRUE);
    }

    pu64VirData = (HI_U64*)m_dest_image.pu8VirAddr[0];
    for(unsigned short i=0; i<m_od_block_num_row; ++i)
    {
        HI_U64 u64TopLeft, u64TopRight, u64BtmLeft, u64BtmRight;
        HI_U64 *u64TopRow, *u64BtmRow;

        u64TopRow = (0 == i) ? (pu64VirData) : ( pu64VirData + (i * h -1) * m_dest_image.u16Stride[0]);
        u64BtmRow = pu64VirData + ((i + 1) * h - 1) * m_dest_image.u16Stride[0];
        
        for(unsigned short j=0;j<m_od_block_num_line;j++)
        {                
            HI_U64 u64BlockSum,u64BlockSq;
            HI_U32 index = i * m_od_block_num_row+ j;

            u64TopLeft  = (0 == i) ? (0) : ((0 == j) ? (0) : (u64TopRow[j * w-1]));
            u64TopRight = (0 == i) ? (0) : (u64TopRow[(j + 1) * w - 1]);
            u64BtmLeft  = (0 == j) ? (0) : (u64BtmRow[j * w - 1]);
            u64BtmRight = u64BtmRow[(j + 1) * w -1];
                          
            u64BlockSum = (u64TopLeft & 0xfffffffLL) + (u64BtmRight & 0xfffffffLL)
                        - (u64BtmLeft & 0xfffffffLL) - (u64TopRight & 0xfffffffLL);

            u64BlockSq  = (u64TopLeft >> 28) + (u64BtmRight >> 28)
                        - (u64BtmLeft >> 28) - (u64TopRight >> 28);
           // mean
            stChar[index].s32X = u64BlockSum/(w*h);
           // sigma=sqrt(1/(w*h)*sum((x(i,j)-mean)^2)= sqrt(sum(x(i,j)^2)/(w*h)-mean^2)
            stChar[index].s32Y = sqrt(u64BlockSq/(w*h) - stChar[index].s32X * stChar[index].s32X);

            //DEBUG_MESSAGE("mean=%d, var=%d; \n", stChar[index].s32X, stChar[index].s32Y);
            }
        }

        od_block_num=av_od_linear2D_classifer(&stChar[0], block_num, m_linear_data.pstLinearPoint, m_linear_data.s32LinearNum);

        DEBUG_MESSAGE("the od block num is:%d the threshNum is:%d \n", od_block_num, m_linear_data.s32ThreshNum);
#else
    HI_S32 s32Ret = HI_FAILURE;
    IVE_HANDLE hIveHandle;
    IVE_SRC_INFO_S stSrc;
    HI_S32 w,h,od_block_num;
    HI_U32 block_num = m_od_block_num_line*m_od_block_num_row;
    POINT_S stChar[block_num];

    if(0 == m_dst_mem_info.u32PhyAddr)
    {
        s32Ret = HI_MPI_SYS_MmzAlloc_Cached(&m_dst_mem_info.u32PhyAddr, (HI_VOID **)&m_pu64VirData, 
        "user_od", HI_NULL, frame_info.stVFrame.u32Height * frame_info.stVFrame.u32Width*8);
        if(s32Ret != HI_SUCCESS)
        {
            DEBUG_ERROR("can't alloc intergal memory for %x\n",s32Ret);
            return -1;
        }
        m_dst_mem_info.u32Stride = frame_info.stVFrame.u32Width;
    }
    else if(m_dst_mem_info.u32Stride != frame_info.stVFrame.u32Width)
    {
        HI_MPI_SYS_MmzFree(m_dst_mem_info.u32PhyAddr, (HI_VOID *)m_pu64VirData);
        s32Ret = HI_MPI_SYS_MmzAlloc_Cached(&m_dst_mem_info.u32PhyAddr, (HI_VOID **)&m_pu64VirData, 
        "user_od", HI_NULL, frame_info.stVFrame.u32Height * frame_info.stVFrame.u32Width*8);

        if(s32Ret != HI_SUCCESS)
        {
            DEBUG_ERROR("can't alloc intergal memory for %x\n",s32Ret);
            return -1;
        }
        m_dst_mem_info.u32Stride = frame_info.stVFrame.u32Width;
    }

    memset(&stSrc, 0, sizeof(IVE_SRC_INFO_S));
    stSrc.u32Width = frame_info.stVFrame.u32Width;
    stSrc.u32Height = frame_info.stVFrame.u32Height;
    stSrc.stSrcMem.u32PhyAddr = frame_info.stVFrame.u32PhyAddr[0];
    stSrc.stSrcMem.u32Stride = frame_info.stVFrame.u32Stride[0];
    stSrc.enSrcFmt = IVE_SRC_FMT_SINGLE;
    
    w = stSrc.u32Width/m_od_block_num_row;
    h = stSrc.u32Height/m_od_block_num_line;

    s32Ret = HI_MPI_IVE_INTEG(&hIveHandle, &stSrc, &m_dst_mem_info, HI_TRUE);

    if(s32Ret != HI_SUCCESS)
    {
        HI_MPI_SYS_MmzFree(m_dst_mem_info.u32PhyAddr, (HI_VOID *)m_pu64VirData);
        memset(&m_dst_mem_info, 0, sizeof(IVE_MEM_INFO_S));
        DEBUG_ERROR(" ive integal function can't submmit for %x\n",s32Ret);
        return -1;
    } 

    for(unsigned short i=0; i<m_od_block_num_row; ++i)
    {
        HI_U64 u64TopLeft, u64TopRight, u64BtmLeft, u64BtmRight;
        HI_U64 *u64TopRow, *u64BtmRow;

        u64TopRow = (0 == i) ? (m_pu64VirData) : ( m_pu64VirData + (i * h -1) * m_dst_mem_info.u32Stride);
        u64BtmRow = m_pu64VirData + ((i + 1) * h - 1) * m_dst_mem_info.u32Stride;
        
        for(unsigned short j=0;j<m_od_block_num_line;j++)
        {                
            HI_U64 u64BlockSum,u64BlockSq;
            HI_U32 index = i * m_od_block_num_row+ j;

            u64TopLeft  = (0 == i) ? (0) : ((0 == j) ? (0) : (u64TopRow[j * w-1]));
            u64TopRight = (0 == i) ? (0) : (u64TopRow[(j + 1) * w - 1]);
            u64BtmLeft  = (0 == j) ? (0) : (u64BtmRow[j * w - 1]);
            u64BtmRight = u64BtmRow[(j + 1) * w -1];
                          
            u64BlockSum = (u64TopLeft & 0xfffffffLL) + (u64BtmRight & 0xfffffffLL)
                        - (u64BtmLeft & 0xfffffffLL) - (u64TopRight & 0xfffffffLL);

            u64BlockSq  = (u64TopLeft >> 28) + (u64BtmRight >> 28)
                        - (u64BtmLeft >> 28) - (u64TopRight >> 28);
           // mean
            stChar[index].s32X = u64BlockSum/(w*h);
           // sigma=sqrt(1/(w*h)*sum((x(i,j)-mean)^2)= sqrt(sum(x(i,j)^2)/(w*h)-mean^2)
            stChar[index].s32Y = sqrt(u64BlockSq/(w*h) - stChar[index].s32X * stChar[index].s32X);

           //DEBUG_MESSAGE("mean=%d, var=%d; \n", stChar[index].s32X, stChar[index].s32Y);
            }
        }

        od_block_num=av_od_linear2D_classifer(&stChar[0], block_num, m_linear_data.pstLinearPoint, m_linear_data.s32LinearNum);
        DEBUG_MESSAGE("the od block num is:%d the threshNum is:%d \n", od_block_num, m_linear_data.s32ThreshNum);

        s32Ret=HI_MPI_SYS_MmzFlushCache(m_dst_mem_info.u32PhyAddr , (HI_VOID *)m_pu64VirData , stSrc.u32Height * stSrc.u32Width*8);
        if(s32Ret!=HI_SUCCESS)
        {
            HI_MPI_SYS_MmzFree(m_dst_mem_info.u32PhyAddr, (HI_VOID *)m_pu64VirData);
            memset(&m_dst_mem_info, 0, sizeof(IVE_MEM_INFO_S));
            DEBUG_ERROR(" ive integal function can't flush cache for %x\n",s32Ret);
            return -1;
        }
#endif 
    if(od_block_num>= m_linear_data.s32ThreshNum)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int CHiAvIveOd::av_od_linear2D_classifer(POINT_S *pstChar, HI_S32 s32CharNum, POINT_S *pstLinearPoint, HI_S32 s32Linearnum)
{
    HI_S32 s32ResultNum;
    HI_S32 i,j;
    HI_BOOL bTestFlag;
    POINT_S *pstNextLinearPoint;
    
    s32ResultNum=0;
    pstNextLinearPoint=&pstLinearPoint[1];
    
    for(i=0;i<s32CharNum;i++)
    {
        bTestFlag=HI_TRUE;
        for(j=0;j<(s32Linearnum-1);j++)
        {
            if(   ( (pstChar[i].s32Y-pstLinearPoint[j].s32Y)*(pstNextLinearPoint[j].s32X-pstLinearPoint[j].s32X)>
            (pstChar[i].s32X-pstLinearPoint[j].s32X)*(pstNextLinearPoint[j].s32Y-pstLinearPoint[j].s32Y) 
            && (pstNextLinearPoint[j].s32X!=pstLinearPoint[j].s32X))
            || ( (pstChar[i].s32X>pstLinearPoint[j].s32X) && (pstNextLinearPoint[j].s32X==pstLinearPoint[j].s32X) ))	
                {
                    bTestFlag=HI_FALSE;
                    break;
                }
            }
            if(bTestFlag==HI_TRUE)
            {
                s32ResultNum++;
            }
    }
    return s32ResultNum;

}


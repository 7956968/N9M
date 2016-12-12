/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : aenc_amr_adp.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2011/02/26
  Description   : 
  History       :
  1.Date        : 2011/02/26
    Author      : n00168968
    Modification: Created file

******************************************************************************/


#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
//#incldue "AvDegbug.h"

#include "audio_amr_adp.h"

HI_S32 OpenAMREncoder(HI_VOID *pEncoderAttr, HI_VOID **ppEncoder)
{
    AMR_ENCODER_S *pstEncoder = NULL;
    AENC_ATTR_AMR_S *pstAttr = NULL;

    HI_ASSERT(pEncoderAttr != NULL); 
    HI_ASSERT(ppEncoder != NULL);

    /* 检查编码器属性 */
    pstAttr = (AENC_ATTR_AMR_S *)pEncoderAttr;
    if ((pstAttr->enMode < 0)
        ||(pstAttr->enMode >= AMR_MODE_BUTT))
	{
        printf("[Func]:%s [Line]:%d [Info]:invalid AMR mode%d\n", 
            __FUNCTION__, __LINE__, (HI_S32)pstAttr->enMode);
        return HI_ERR_AENC_ILLEGAL_PARAM;;
	}
    if ((pstAttr->enFormat < 0)
        ||(pstAttr->enFormat >= AMR_FORMAT_BUTT))
	{
        
        printf("[Func]:%s [Line]:%d [Info]:invalid AMR format%d\n", 
            __FUNCTION__, __LINE__, (HI_S32)pstAttr->enFormat);
        return HI_ERR_AENC_ILLEGAL_PARAM;;
	}
    if ((pstAttr->s32Dtx != 0) && (pstAttr->s32Dtx != 1))
	{
        printf("[Func]:%s [Line]:%d [Info]:%s\n", 
            __FUNCTION__, __LINE__, "s32Dtx must be 0 or 1");
        return HI_ERR_AENC_ILLEGAL_PARAM;;
	}

    /* 为编码器状态空间分配内存 */
    pstEncoder = (AMR_ENCODER_S *)malloc(sizeof(AMR_ENCODER_S));
    if(NULL == pstEncoder)
    {
        printf("[Func]:%s [Line]:%d [Info]:%s\n", 
            __FUNCTION__, __LINE__, "no memory");
        return HI_ERR_AENC_NOMEM;
    }
    memset(pstEncoder, 0, sizeof(AMR_ENCODER_S));
    *ppEncoder = (HI_VOID *)pstEncoder;

    /* 初始化编码器 */
    memcpy(&pstEncoder->stAMRAttr, pstAttr, sizeof(AENC_ATTR_AMR_S));
    
    return AMR_Encode_Init(&pstEncoder->pstAMRState, pstAttr->s32Dtx);
}

HI_S32 EncodeAMRFrm(HI_VOID *pEncoder, const AUDIO_FRAME_S *pstData,
                    HI_U8 *pu8Outbuf,HI_U32 *pu32OutLen)
{
    HI_S32 s32Ret = HI_SUCCESS;
    AMR_ENCODER_S *pstEncoder = NULL;
    HI_U32 u32PtNums;
    
    HI_ASSERT(pEncoder != NULL);
    HI_ASSERT(pstData != NULL);
    HI_ASSERT(pu8Outbuf != NULL); 
    HI_ASSERT(pu32OutLen != NULL);

    pstEncoder = (AMR_ENCODER_S *)pEncoder;
    
    /* 计算采样点数目(单声道的) */
    u32PtNums = pstData->u32Len/(pstData->enBitwidth + 1);
    if (u32PtNums != 160)
	{
        printf("[Func]:%s [Line]:%d [Info]:the point num of a frame for AMR is %d not 160\n", 
            __FUNCTION__, __LINE__, u32PtNums);
    	return HI_ERR_AENC_ILLEGAL_PARAM;;
	}
    
    *pu32OutLen = AMR_Encode_Frame(pstEncoder->pstAMRState, (HI_S16 *)pstData->pVirAddr[0],
        pu8Outbuf, (enum Mode)pstEncoder->stAMRAttr.enMode, (enum Format)pstEncoder->stAMRAttr.enFormat);
    
    s32Ret = (*pu32OutLen >0 )? HI_SUCCESS : HI_FAILURE;
    return s32Ret;
}

HI_S32 CloseAMREncoder(HI_VOID *pEncoder)
{
    AMR_ENCODER_S *pstEncoder = NULL;

    HI_ASSERT(pEncoder != NULL);
    pstEncoder = (AMR_ENCODER_S *)pEncoder;

    AMR_Encode_Exit(&pstEncoder->pstAMRState);

    free(pstEncoder);
    return HI_SUCCESS;
}

HI_S32 OpenAMRDecoder(HI_VOID *pDecoderAttr, HI_VOID **ppDecoder)
{
    AMR_DECODER_S *pstDecoder = NULL;
    ADEC_ATTR_AMR_S *pstAttr = NULL;

    HI_ASSERT(pDecoderAttr != NULL); 
    HI_ASSERT(ppDecoder != NULL);

    /* 检查编码器属性 */
    pstAttr = (ADEC_ATTR_AMR_S *)pDecoderAttr;
    if ((pstAttr->enFormat < 0)
        ||(pstAttr->enFormat >= AMR_FORMAT_BUTT))
	{
        printf("[Func]:%s [Line]:%d [Info]:invalid AMR format%d\n", 
            __FUNCTION__, __LINE__, (HI_S32)pstAttr->enFormat);
        return HI_ERR_ADEC_ILLEGAL_PARAM;;
	}

    /* 为编码器状态空间分配内存 */
    pstDecoder = (AMR_DECODER_S *)malloc(sizeof(AMR_DECODER_S));
    if(NULL == pstDecoder)
    {
        printf("[Func]:%s [Line]:%d [Info]:no memory\n", 
            __FUNCTION__, __LINE__);
        return HI_ERR_ADEC_NOMEM;
    }
    memset(pstDecoder, 0, sizeof(AMR_DECODER_S));
    *ppDecoder = (HI_VOID *)pstDecoder;

    /* 初始化编码器 */
    memcpy(&pstDecoder->stAMRAttr, pstAttr, sizeof(ADEC_ATTR_AMR_S));
    
    return AMR_Decode_Init(&pstDecoder->pstAMRState);
}

HI_S32 DecodeAMRFrm(HI_VOID *pDecoder, HI_U8 **pu8Inbuf,HI_S32 *ps32LeftByte,
                        HI_U16 *pu16Outbuf,HI_U32 *pu32OutLen,HI_U32 *pu32Chns)
{
    HI_S32 s32Ret = HI_SUCCESS, s32Size;
    AMR_DECODER_S *pstDecoder = NULL;
    
    HI_ASSERT(pDecoder != NULL);
    HI_ASSERT(pu8Inbuf != NULL);
    HI_ASSERT(ps32LeftByte != NULL); 
    HI_ASSERT(pu16Outbuf != NULL);  
    HI_ASSERT(pu32OutLen != NULL);  
    HI_ASSERT(pu32Chns != NULL);

    pstDecoder = (AMR_DECODER_S *)pDecoder;

    s32Size = AMR_Get_Length((enum Format)pstDecoder->stAMRAttr.enFormat, (*pu8Inbuf)[0]);
    if (s32Size<0)
    {
        printf("[Func]:%s [Line]:%d [Info]:AMR_Get_Length failed\n", 
            __FUNCTION__, __LINE__);
        return s32Size;
    }
    
    if (*ps32LeftByte < s32Size)
    {
        printf("[Func]:%s [Line]:%d [Info]:input buffer not enough to decode one frame\n", 
            __FUNCTION__, __LINE__);
        return HI_ERR_ADEC_BUF_LACK;
    }
    
    s32Ret = AMR_Decode_Frame(pstDecoder->pstAMRState, (*pu8Inbuf),
        (HI_S16*)pu16Outbuf, (enum Format)pstDecoder->stAMRAttr.enFormat); 
    if (0 != s32Ret)
    {
        printf("[Func]:%s [Line]:%d [Info]:AMR_Decode_Frame fail --- %s\n", 
            __FUNCTION__, __LINE__, strerror(errno));
        return HI_ERR_ADEC_DECODER_ERR;
    }

    *ps32LeftByte -= (s32Size+1);
    (*pu8Inbuf)  += (s32Size+1);
    *pu32OutLen = 160*sizeof(HI_U16);
    
    return s32Ret;
}

HI_S32 GetAmrFrmInfo(HI_VOID *pDecoder, HI_VOID *pInfo)
{
    return HI_SUCCESS;
}

HI_S32 CloseAMRDecoder(HI_VOID *pDecoder)
{
    AMR_DECODER_S *pstDecoder = NULL;

    HI_ASSERT(pDecoder != NULL);
    pstDecoder = (AMR_DECODER_S *)pDecoder;

    AMR_Decode_Exit(&pstDecoder->pstAMRState);

    free(pstDecoder);
    return HI_SUCCESS;
}

HI_S32 HI_MPI_AENC_AmrInit(HI_VOID)
{
    HI_S32 s32Handle;
    AENC_ENCODER_S stAmr;

    stAmr.enType = PT_AMR;
    sprintf(stAmr.aszName, "Amr");
    stAmr.u32MaxFrmLen = 160 * MAX_AUDIO_POINT_BYTES / 10;
    stAmr.pfnOpenEncoder = OpenAMREncoder;
    stAmr.pfnEncodeFrm = EncodeAMRFrm;
    stAmr.pfnCloseEncoder = CloseAMREncoder;
    return HI_MPI_AENC_RegeisterEncoder(&s32Handle, &stAmr);
}

HI_S32 HI_MPI_ADEC_AmrInit(HI_VOID)
{
    HI_S32 s32Handle;
    ADEC_DECODER_S stAmr;

    stAmr.enType = PT_AMR;
    sprintf(stAmr.aszName, "Amr");
    stAmr.pfnOpenDecoder = OpenAMRDecoder;
    stAmr.pfnDecodeFrm = DecodeAMRFrm;
    stAmr.pfnGetFrmInfo = GetAmrFrmInfo;
    stAmr.pfnCloseDecoder = CloseAMRDecoder;
    return HI_MPI_ADEC_RegeisterDecoder(&s32Handle, &stAmr);
}


#ifndef __AUDIO_AMR_ADP_H__
#define __AUDIO_AMR_ADP_H__

#include <stdio.h>

#include "mpi_aenc.h"
#include "hi_comm_aenc.h"
#include "hi_comm_aio.h"
#if !defined(_AV_PLATFORM_HI3518EV200_)
#include "amr_enc.h"
#include "amr_dec.h"
#endif

#include "mpi_adec.h"
#include "hi_comm_adec.h"


typedef enum hiAMR_MODE_E
{
    AMR_MODE_MR475 = 0,     /* AMR bit rate as  4.75k */
    AMR_MODE_MR515,         /* AMR bit rate as  5.15k */              
    AMR_MODE_MR59,          /* AMR bit rate as  5.90k */
    AMR_MODE_MR67,          /* AMR bit rate as  6.70k (PDC-EFR)*/
    AMR_MODE_MR74,          /* AMR bit rate as  7.40k (IS-641)*/
    AMR_MODE_MR795,         /* AMR bit rate as  7.95k */    
    AMR_MODE_MR102,         /* AMR bit rate as 10.20k */    
    AMR_MODE_MR122,         /* AMR bit rate as 12.20k (GSM-EFR)*/            
    AMR_MODE_BUTT
} AMR_MODE_E;

typedef enum hiAMR_FORMAT_E
{
    AMR_FORMAT_MMS,     /* Using for file saving        */
    AMR_FORMAT_IF1,     /* Using for wireless translate */
    AMR_FORMAT_IF2,     /* Using for wireless translate */
    AMR_FORMAT_BUTT
} AMR_FORMAT_E;

typedef struct hiAENC_ATTR_AMR_S
{
    AMR_MODE_E      enMode;
    AMR_FORMAT_E    enFormat; 
    HI_S32          s32Dtx;     /*Discontinuous Transmission(1:enable, 0:disable)*/
}AENC_ATTR_AMR_S;

typedef struct hiAMR_ENCODER_S
{
    HI_VOID             *pstAMRState;
    AENC_ATTR_AMR_S     stAMRAttr;
} AMR_ENCODER_S;

typedef struct hiADEC_ATTR_AMR_S
{
    AMR_FORMAT_E enFormat; 
}ADEC_ATTR_AMR_S;

typedef struct hiAMR_DECODER_S
{
    HI_VOID             *pstAMRState;
    ADEC_ATTR_AMR_S     stAMRAttr;
} AMR_DECODER_S;

HI_S32 HI_MPI_AENC_AmrInit(HI_VOID);

HI_S32 HI_MPI_ADEC_AmrInit(HI_VOID);

#endif


#ifndef _HI_AV_VDA_H_
#define _HI_AV_VDA_H_

#include "AvVda.h"
#include "hi_comm_vda.h"
#include "mpi_vda.h"
#if defined(_AV_PLATFORM_HI3516A_)
#include "HiVpss.h"
#else
#include "HiVi.h"
#endif

class CHiAvVda : public CAvVda
{
public:
#if defined(_AV_PLATFORM_HI3516A_)
    CHiAvVda(CHiVpss *pHiVpss);
#else
    CHiAvVda(CHiVi *pHiVi);
#endif
    ~CHiAvVda();

private:
    int Av_create_detector(vda_type_e vda_type, int detector_handle, const vda_chn_param_t* pVda_chn_param);
    int Av_destroy_detector(vda_type_e vda_type, int detector_handle);
    int Av_detector_control(int phy_video_chn, int detector_handle, bool start_or_stop);
    int Av_get_detector_fd(int detector_handle);
    int Av_get_vda_data(vda_type_e vda_type, int detector_handle, vda_data_t *pVda_data);
    int Av_set_vda_attribut(int vda_type, int detector_handle, unsigned char attr_type, void* attribute);

private:
#if defined(_AV_PLATFORM_HI3516A_)
    CHiVpss *m_pHiVpss;
#else
    CHiVi *m_pHiVi;
#endif
};

#endif

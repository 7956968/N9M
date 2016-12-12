#ifndef __HI_AV_SYS_PIC_MANAGER3515__
#define __HI_AV_SYS_PIC_MANAGER3515__

#include "AvSysPicManager.h"
#include <map>
#include "CommonDebug.h"
typedef struct _hi_vb_info_
{
    unsigned int vb_block;
    unsigned int vb_phy_Addr;
    unsigned char * pVb_vir_addr;
    unsigned int vb_pool_id;
    
    unsigned int luma_size;
    unsigned int chrm_size;
    unsigned int total_size;
} hi_vb_info_t;


class CHiAvSysPicManager : public CAvSysPicManager
{
public:
    CHiAvSysPicManager();
    ~CHiAvSysPicManager();

public:
    int Aspm_get_picture(av_sys_pic_type_e pic_type, void * pPicture = NULL);

private:
    int Aspm_get_pic_buffer(av_sys_pic_type_e pic_type, av_sys_pic_info_t pic_info, av_sys_buffer_info_t &buf_info);
    int Aspm_release_pic_buffer(av_sys_pic_type_e pic_type);

private:
    std::map<av_sys_pic_type_e, hi_vb_info_t> m_vb_info;
    std::map<av_sys_pic_type_e, av_sys_pic_info_t> m_pic_info;
};

#endif /*__HI_AV_SYS_PIC_MANAGER__*/


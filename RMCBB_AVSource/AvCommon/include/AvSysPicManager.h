#ifndef __AV_SYS_PIC_MANAGER__
#define __AV_SYS_PIC_MANAGER__
#include "AvCommonType.h"
#include <bitset>
#include "CommonDebug.h"

typedef struct _av_sys_pic_info_
{
    unsigned int pic_width;
    unsigned int pic_height;
    unsigned int pic_stride;
} av_sys_pic_info_t;

typedef struct _av_sys_buffer_info_
{
    unsigned char *pY;
    unsigned char *pU;
    unsigned char *pV;
    unsigned int luma_size;
    unsigned int chrm_size;
    unsigned int total_size;
} av_sys_buffer_info_t;

class CAvSysPicManager
{
public:
    CAvSysPicManager();
    virtual ~CAvSysPicManager();

public:
    int Aspm_load_picture(av_sys_pic_type_e pic_type);
    int Aspm_unload_picture(av_sys_pic_type_e pic_type);

    virtual int Aspm_get_picture(av_sys_pic_type_e pic_type, void * pPicture = NULL) = 0;

private:
    virtual int Aspm_get_pic_buffer(av_sys_pic_type_e pic_type, av_sys_pic_info_t pic_info, av_sys_buffer_info_t &buf_info) = 0;
    virtual int Aspm_release_pic_buffer(av_sys_pic_type_e pic_type) = 0;

    int Aspm_parse_yuv420_picture(int pic_fd, av_sys_pic_info_t pic_info, av_sys_buffer_info_t &buf_info);
    
private:
    std::bitset<_ASPT_MAX_PIC_TYPE_> m_load_pic_type;
};

#endif /*__AV_SYS_PIC_MANAGER__*/

#include "AvSysPicManager.h"
#include "AvDebug.h"
#include "AvCommonMacro.h"
#include "AvCommonType.h"
#include "AvDeviceInfo.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
using namespace Common;

CAvSysPicManager::CAvSysPicManager()
{
    m_load_pic_type.reset();
}

CAvSysPicManager::~CAvSysPicManager()
{
    for (unsigned int pic_type = 0; pic_type < _ASPT_MAX_PIC_TYPE_; ++pic_type)
    {
        if (m_load_pic_type.test(pic_type))
        {
            Aspm_unload_picture(static_cast<av_sys_pic_type_e>(pic_type));
        }
    }
}

int CAvSysPicManager::Aspm_load_picture(av_sys_pic_type_e pic_type)
{
    if (pgAvDeviceInfo->Adi_max_channel_number() != 8 && _ASPT_CUSTOMER_LOGO_ == pic_type)
    {
        return 0;
    }
    
    if (m_load_pic_type.test(pic_type) == 1)
    {
        DEBUG_ERROR( "CAvSysPicManager::Aspm_load_picture pic_type:%d has already been loaded! \n", pic_type);
        return 0;
    }

    std::string pic_path = pgAvDeviceInfo->Adi_get_sys_pic_path(pic_type);
    int fd = open(pic_path.c_str(), O_RDONLY);
    if (fd == -1)
    {
        DEBUG_ERROR( "CAvSysPicManager::Aspm_load_picture open %s FAILED!\n", pic_path.c_str());
        return -1;
    }
    
    av_sys_pic_info_t pic_info;
    av_sys_buffer_info_t buf_info;
    memset(&pic_info, 0, sizeof(av_sys_pic_info_t));
    memset(&buf_info, 0, sizeof(av_sys_buffer_info_t));
    
    switch (pic_type)
    {
        case _ASPT_VIDEO_LOST_:
        case _ASPT_CUSTOMER_LOGO_:
		case _ASPT_ACCESS_LOGO_:
            pic_info.pic_width  = 704;
            pic_info.pic_height = 576;
            pic_info.pic_stride = 704;
            break;
        default:
            DEBUG_ERROR( "You must give the pictue info!!!\n");
            return -1;
    }
    
    if (Aspm_get_pic_buffer(pic_type, pic_info, buf_info) == -1)
    {
        DEBUG_ERROR( "CAvSysPicManager::Aspm_load_picture  Aspm_get_pic_buffer FAILED!!! \n");
        return -1;
    }
    
    switch (pic_type)
    {
        case _ASPT_VIDEO_LOST_:
        case _ASPT_CUSTOMER_LOGO_:
		case _ASPT_ACCESS_LOGO_:
            Aspm_parse_yuv420_picture(fd, pic_info, buf_info);
            break;
        default:
            DEBUG_ERROR( "CAvSysPicManager::Aspm_load_picture You must give the implement of picture parser!!!\n");
            return -1;
    }
    m_load_pic_type.set(pic_type);
    
    _AV_SAFE_CLOSE_(fd);
    
    return 0;
}

int CAvSysPicManager::Aspm_unload_picture(av_sys_pic_type_e pic_type)
{
    if (pgAvDeviceInfo->Adi_max_channel_number() != 8 && _ASPT_CUSTOMER_LOGO_ == pic_type)
    {
        return 0;
    }
    
    if (m_load_pic_type.test(pic_type) == 0)
    {
        DEBUG_ERROR( "CAvSysPicManager::Aspm_unload_picture pic_type:%d has already been unloaded!! \n", pic_type);
        return 0;
    }
    
    if (Aspm_release_pic_buffer(pic_type) == -1)
    {
        DEBUG_ERROR( "CAvSysPicManager::Aspm_unload_picture Aspm_release_pic_buffer FAILED! (pic_type:%d) \n", pic_type);
        return -1;
    }
    
    m_load_pic_type.reset(pic_type);
    
    return 0;
}

int CAvSysPicManager::Aspm_parse_yuv420_picture(int pic_fd, av_sys_pic_info_t pic_info, av_sys_buffer_info_t &buf_info)
{
    unsigned char *pDst = NULL;
    unsigned int row    = 0;

    if (buf_info.pY == NULL || buf_info.pU == NULL || buf_info.pV == NULL)
    {
        DEBUG_ERROR( "CAvSysPicManager::Aspm_parse_yuv420_picture Parameter error!!\n");
        return -1;
    }

    /*read Y U V data from file to addr*/
    pDst = buf_info.pY;
    for (row = 0; row != pic_info.pic_height; ++row)
    {
        read(pic_fd, pDst, pic_info.pic_width);
        pDst += pic_info.pic_stride;
    }

    pDst = buf_info.pU;
    for (row = 0; row != pic_info.pic_height / 2; ++row)
    {
        read(pic_fd, pDst, pic_info.pic_width / 2);
        pDst += pic_info.pic_stride / 2;
    }

    pDst = buf_info.pV;
    for (row = 0; row != pic_info.pic_height / 2; ++row)
    {
        read(pic_fd, pDst, pic_info.pic_width / 2);
        pDst += pic_info.pic_stride / 2;
    }

    /*convert planar YUV420 to sem-planar YUV420*/
    unsigned int size = (pic_info.pic_width * pic_info.pic_height) >> 2;
    unsigned char * pTempU = NULL;
    unsigned char * pTempV = NULL;
    unsigned char * ptU    = NULL;
    unsigned char * ptV    = NULL;
    pTempU = (unsigned char *)malloc(size); ptU = pTempU;
    pTempV = (unsigned char *)malloc(size); ptV = pTempV;

    memcpy(pTempU, buf_info.pU, size);
    memcpy(pTempV, buf_info.pV, size);

    for (unsigned int u = 0; u != (size>>1); ++u)
    {
        *buf_info.pU++ = *pTempV++;
        *buf_info.pU++ = *pTempU++;
    }

    for (unsigned int v = 0; v != (size>>1); ++v)
    {
        *buf_info.pV++ = *pTempV++;
        *buf_info.pV++ = *pTempU++;
    }

    _AV_SAFE_FREE_(ptU); pTempU = NULL;
    _AV_SAFE_FREE_(ptV); pTempV = NULL;
    
    return 0;
}


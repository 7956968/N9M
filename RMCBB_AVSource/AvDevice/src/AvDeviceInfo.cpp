#include "AvDeviceInfo.h"
#include "SystemConfig.h"


/*used for source insight*/
using namespace Common;

CAvDeviceInfo *pgAvDeviceInfo = NULL;

av_split_t CAvDeviceInfo::Adi_get_split_mode(int chn_num)
{
    if (chn_num == 1)
    {
        return _AS_SINGLE_;
    }
    else if ((chn_num > 1) && (chn_num <= 4))
    {
        return _AS_4SPLIT_;
    }
    else if ((chn_num > 4) && (chn_num <= 9))
    {
        return _AS_9SPLIT_;
    }
    else if ((chn_num > 9) && (chn_num <= 16))
    {
#if defined(_AV_PRODUCT_CLASS_DVR_REI_)
        if (12 == chn_num)          //! for rei playback 0414
        {
            return _AS_12SPLIT_;
        }
        else
        {
            return _AS_16SPLIT_;
        }
#else  
        return _AS_16SPLIT_;
#endif
    }
    else if ((chn_num > 16) && (chn_num <= 36))
    {
        return _AS_36SPLIT_;
    }

    return _AS_UNKNOWN_;
}

int CAvDeviceInfo::Adi_get_split_chn_info(av_split_t split_mode)
{
    int chn_num = 0;
    
    switch(split_mode)
    {
        case _AS_SINGLE_:
            chn_num = 1;
            break;
        case _AS_PIP_:
            chn_num = 2;
            break;
        case _AS_DIGITAL_PIP_:
            chn_num = 1;
            break;
        case _AS_2SPLIT_:
            chn_num = 2;
            break;
        case _AS_3SPLIT_:
            chn_num = 3;
            break;			
        case _AS_4SPLIT_:
            chn_num = 4;
            break;
        case _AS_6SPLIT_:
            chn_num = 6;
            break;
        case _AS_8SPLIT_:
            chn_num = 8;
            break;
        case _AS_9SPLIT_:
            chn_num = 9;
            break;
        case _AS_10SPLIT_:
            chn_num = 10;
            break;
        case _AS_12SPLIT_:
            chn_num = 12;
            break;
        case _AS_13SPLIT_:
            chn_num = 13;
            break;
        case _AS_16SPLIT_:
            chn_num = 16;
            break;
        case _AS_25SPLIT_:
            chn_num = 25;
            break;
        case _AS_36SPLIT_:
            chn_num = 36;
            break;
        default:
            DEBUG_ERROR( "CAvDeviceInfo::Adi_get_split_chn_info You must give the implement(%d)\n", split_mode);
            break;
    }

    return chn_num;
}

int CAvDeviceInfo::Adi_get_split_chn_rect_REGULAR(av_rect_t *pVideo_rect, av_split_t split_mode, int chn_num, av_point_t *pLeft_top_point, av_point_t *pRight_bottom_point)
{
    int split_ratio = 0;
    if (pLeft_top_point == NULL || pRight_bottom_point == NULL)
    {
        DEBUG_ERROR( "CAvDeviceInfo::Adi_get_split_chn_rect_REGULAR parameter error!!!\n");
        return -1;
    }

    switch(split_mode)
    {
        case _AS_SINGLE_:
            split_ratio = 1;
            chn_num = 0;
            break;
        case _AS_4SPLIT_:
            split_ratio = 2;
            break;
        case _AS_9SPLIT_:
            split_ratio = 3;
            break;
        case _AS_16SPLIT_:
            split_ratio = 4;
            break;
        case _AS_25SPLIT_:
            split_ratio = 5;
            break;
        case _AS_36SPLIT_:
            split_ratio = 6;
            break;
        default:
            _AV_FATAL_(1, "Adi_get_split_chn_rect_REGULAR You must give the implement(split_mode:%d)\n", split_mode);
            break;
    }

    
    pLeft_top_point->x = pVideo_rect->x + (chn_num % split_ratio) * pVideo_rect->w / split_ratio;
    pLeft_top_point->y = pVideo_rect->y + (chn_num / split_ratio) * pVideo_rect->h / split_ratio;
    pRight_bottom_point->x = pVideo_rect->x + (chn_num % split_ratio + 1) * pVideo_rect->w / split_ratio;
    pRight_bottom_point->y = pVideo_rect->y + (chn_num / split_ratio + 1) * pVideo_rect->h / split_ratio;

    return 0;
}

/*
________________
|_______|______|
|   0   |  1   |
|_______|______|
|_______|______|

*/
int CAvDeviceInfo::Adi_get_split_chn_rect_2SPLIT(av_rect_t *pVideo_rect, int chn_num, av_point_t *pLeft_top_point, av_point_t *pRight_bottom_point)
{
    if (pLeft_top_point == NULL || pRight_bottom_point == NULL)
    {
        DEBUG_ERROR( "CAvDeviceInfo::Adi_get_split_chn_rect_2SPLIT parameter error!!!\n");
        return -1;
    }
	//DEBUG_MESSAGE("2split start!\n");

    switch (chn_num)
    {
        case 0:
            pLeft_top_point->x = pVideo_rect->x;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h / 4;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w / 2;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            break;
        case 1:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w / 2;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h / 4;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            break;
		case 2:
			pLeft_top_point->x = pVideo_rect->x;
			pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
			pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w / 2;
			pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;
			break;
		case 3:
			pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w * 2 / 3;
			pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 4 / 5;
			pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w;
			pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;

        default:
            break;
    }
    
    return 0;
}


/*
________________
|______|  1   |
|  0   |______|
|______|  2   |
|______|______|



*/
int CAvDeviceInfo::Adi_get_split_chn_rect_3SPLIT(av_rect_t *pVideo_rect, int chn_num, av_point_t *pLeft_top_point, av_point_t *pRight_bottom_point)
{
    if (pLeft_top_point == NULL || pRight_bottom_point == NULL)
    {
        DEBUG_ERROR( "CAvDeviceInfo::Adi_get_split_chn_rect_3SPLIT parameter error!!!\n");
        return -1;
    }
    //DEBUG_MESSAGE("3split start!\n");

    switch (chn_num)
    {
        case 0: //(0,h/4),(w/2, h*3/4)
            pLeft_top_point->x = pVideo_rect->x;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h / 4;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w  / 2;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h  * 3 / 4;
            break;
        case 1:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w / 2;
            pLeft_top_point->y = pVideo_rect->y;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h * 2 / 4;
            break;
        case 2:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w / 2;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h / 2;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;
            break;
		case 3:
            pLeft_top_point->x = pVideo_rect->x;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 4 / 5;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w/2;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;			
        default:
            break;
    }
    
    return 0;
}

int CAvDeviceInfo::Adi_screen_display_is_adjust(bool adjust)
{
    m_screen_display_adjust = adjust;
    return 0;
}


int CAvDeviceInfo::Adi_adjust_split_chn_rect(int chn_num, av_point_t *pLeft_top_point, av_point_t *pRight_bottom_point, VO_DEV vo_dev)
{
    int video_original_width;
    int video_original_height;
    int phy_chn_num;
    int split_width;
    int split_height;
    int adjust_split_width;
    int adjust_split_height;

    std::map<int, std::pair<int,int> >::iterator map_it;
    
    if(-1 == m_cur_display_chn[chn_num])
    {   
        return -1;
    }
    else
    {
        phy_chn_num = m_cur_display_chn[chn_num];
    }
    
    if((map_it =  m_cur_preview_res.find(phy_chn_num)) != m_cur_preview_res.end())
    {
        video_original_width = map_it->second.first;
        video_original_height = map_it->second.second;        
    }
    else
    {
        return -1;
    }
    split_width = pRight_bottom_point->x - pLeft_top_point->x;
    split_height = pRight_bottom_point->y - pLeft_top_point->y;
    
    /*以显示屏幕当前通道高位基准*/
    adjust_split_width = (int )(split_width * video_original_width * ((double) m_screen_height / m_screen_width) / video_original_height);
    if(adjust_split_width <= split_width)
    {
        pLeft_top_point->x += (split_width - adjust_split_width) / 2;
        pRight_bottom_point->x -= (split_width - adjust_split_width) / 2;
    }
    else
    {
        /*以显示屏幕当前通道宽为基准*/
        adjust_split_height = (int )(split_height * video_original_height * ((double) m_screen_width / m_screen_height) / video_original_width);
        if(adjust_split_height <= split_height)
        {
            pLeft_top_point->y += (split_height - adjust_split_height) / 2;
            pRight_bottom_point->y -= (split_height - adjust_split_height) / 2;
        }
        else
        {
            return -1;
        }
    }
    
    return 0;
}

int CAvDeviceInfo::Adi_get_split_chn_rect_PIP(av_rect_t *pVideo_rect, av_split_t split_mode, int chn_num, av_rect_t *pChn_rect, VO_DEV vo_dev)
{
    if (pChn_rect)
    {
        if ((split_mode == _AS_PIP_) && (chn_num == 0))
        {
            Adi_get_split_chn_rect(pVideo_rect, _AS_SINGLE_, chn_num, pChn_rect, vo_dev);
        }
        else
        {
            memcpy(pChn_rect, &m_pip_size, sizeof(av_rect_t));
        }
    }
    //_AV_FATAL_(1, "CAvDeviceInfo::Adi_get_split_chn_rect_PIP You must give the implement\n");
    return 0;
}

int CAvDeviceInfo::Adi_set_split_chn_rect_PIP(av_rect_t Video_rect, int chn_num, av_rect_t Chn_rect)
{
    memcpy(&m_pip_size, &Chn_rect, sizeof(av_rect_t));
    return 0;
}



/*
________________
|         |  1 |
|    0    |____|
|         |  2 |
|_________|____|
|  3 |  4 |  5 |
|____|____|____|
*/
int CAvDeviceInfo::Adi_get_split_chn_rect_6SPLIT(av_rect_t *pVideo_rect, int chn_num, av_point_t *pLeft_top_point, av_point_t *pRight_bottom_point)
{
    if (pLeft_top_point == NULL || pRight_bottom_point == NULL)
    {
        DEBUG_ERROR( "CAvDeviceInfo::Adi_get_split_chn_rect_6SPLIT parameter error!!!\n");
        return -1;
    }

    switch (chn_num)
    {
        case 0:
            pLeft_top_point->x = pVideo_rect->x;
            pLeft_top_point->y = pVideo_rect->y;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w * 2 / 3;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h * 2 / 3;
            break;
        case 1:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w * 2 / 3;
            pLeft_top_point->y = pVideo_rect->y;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h / 3;
            break;
        case 2:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w * 2 / 3;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h / 3;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h * 2 / 3;
            break;
        case 3:
            pLeft_top_point->x = pVideo_rect->x;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 2 / 3;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w / 3;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;
            break;
        case 4:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w / 3;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 2 / 3;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w * 2 / 3;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;
            break;
        case 5:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w * 2 / 3;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 2 / 3;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;
            break;
        default:
            break;
    }
    
    return 0;
}


/*
_____________________
| 0  | 1  |  2 |  3 |
|____|____|____|____|
| 4  |         |  6 |
|____|   5     |____|
| 7  |         |  8 |
|____|____ ____|____|
| 9  | 10 | 11 | 12 |
|____|____|____|____|
*/
int CAvDeviceInfo::Adi_get_split_chn_rect_13SPLIT(av_rect_t *pVideo_rect, int chn_num, av_point_t *pLeft_top_point, av_point_t *pRight_bottom_point)
{
    if (pLeft_top_point == NULL || pRight_bottom_point == NULL)
    {
        DEBUG_ERROR( "CAvDeviceInfo::Adi_get_split_chn_rect_13SPLIT parameter error!!!\n");
        return -1;
    }

    switch(chn_num)
    {
        case 0:
            pLeft_top_point->x = pVideo_rect->x;
            pLeft_top_point->y = pVideo_rect->y;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w / 4;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h / 4;
            break;
        case 1:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w / 4;
            pLeft_top_point->y = pVideo_rect->y;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w / 2;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h / 4;
            break;
        case 2:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w / 2;
            pLeft_top_point->y = pVideo_rect->y;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w * 3 / 4;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h / 4;
            break;
        case 3:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w * 3 / 4;
            pLeft_top_point->y = pVideo_rect->y;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h / 4;
            break;
        case 4:
            pLeft_top_point->x = pVideo_rect->x;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h / 4;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w / 4; 
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h / 2;
            break;
        case 5:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w / 4;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h / 4;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w * 3 / 4;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            break;
        case 6:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w * 3 / 4;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h / 4;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h / 2;
            break;
        case 7:
            pLeft_top_point->x = pVideo_rect->x;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h / 2;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w / 4; 
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            break;
        case 8:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w * 3 / 4;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h / 2;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            break;
        case 9:
            pLeft_top_point->x = pVideo_rect->x;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w / 4; 
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;
            break;
        case 10:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w / 4;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w / 2;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;
            break;
        case 11:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w / 2;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w * 3 / 4;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;
            break;
        case 12:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w * 3 / 4;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;
            break;
        default:
            break;
    }
    
    return 0;
}


/*
_____________________
|              |  1 |
|              |____|
|       0      |  2 |
|              |____|
|              |  3 |
|____ ____ ____|____|
|  4 |  5 | 6  |  7 |
|____|____|____|____|
*/
int CAvDeviceInfo::Adi_get_split_chn_rect_8SPLIT(av_rect_t *pVideo_rect, int chn_num, av_point_t *pLeft_top_point, av_point_t *pRight_bottom_point)
{
    if (pLeft_top_point == NULL || pRight_bottom_point == NULL)
    {
        DEBUG_ERROR( "CAvDeviceInfo::Adi_get_split_chn_rect_8SPLIT parameter error!!!\n");
        return -1;
    }

    switch(chn_num)
    {
        case 0:
            pLeft_top_point->x = pVideo_rect->x;
            pLeft_top_point->y = pVideo_rect->y;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w * 3 / 4;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            break;
        case 1:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w * 3 / 4;
            pLeft_top_point->y = pVideo_rect->y;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h / 4;
            break;
        case 2:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w * 3 / 4;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h / 4;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h / 2;
            break;
        case 3:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w * 3 / 4;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h / 2;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            break;
        case 4:
            pLeft_top_point->x = pVideo_rect->x;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w / 4;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;
            break;
        case 5:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w / 4;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w / 2;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;
            break;
        case 6:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w / 2;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w * 3 / 4;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;
            break;
        case 7:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w * 3 / 4;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;
            break;
        default:
            break;
    }
    
    return 0;
}


/*
_____________________
|         |         |
|    0    |    1    |
|         |         |
|____ ____|____ ____|
| 2  |  3 |  4 |  5 |
|____|____|____|____|
| 6  | 7  | 8  |  9 |
|____|____|____|____|
*/
int CAvDeviceInfo::Adi_get_split_chn_rect_10SPLIT(av_rect_t *pVideo_rect, int chn_num, av_point_t *pLeft_top_point, av_point_t *pRight_bottom_point)
{
    if (pLeft_top_point == NULL || pRight_bottom_point == NULL)
    {
        DEBUG_ERROR( "CAvDeviceInfo::Adi_get_split_chn_rect_10SPLIT parameter error!!!\n");
        return -1;
    }

    switch(chn_num)
    {
        case 0:
            pLeft_top_point->x = pVideo_rect->x;
            pLeft_top_point->y = pVideo_rect->y;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w / 2;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h / 2;
            break;
        case 1:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w / 2;
            pLeft_top_point->y = pVideo_rect->y;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h / 2;
            break;
        case 2:
            pLeft_top_point->x = pVideo_rect->x;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h / 2;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w / 4;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            break;
        case 3:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w / 4;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h / 2;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w / 2;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            break;
        case 4:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w / 2;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h / 2;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w * 3 / 4;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            break;
        case 5:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w * 3 / 4;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h / 2;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            break;
        case 6:
            pLeft_top_point->x = pVideo_rect->x;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w / 4;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;
            break;
        case 7:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w / 4;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w / 2;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;
            break;
        case 8:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w / 2;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w * 3 / 4;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;
            break;
        case 9:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w * 3 / 4;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 3 / 4;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;
            break;
        default:
            break;
    }
    
    return 0;
}

int CAvDeviceInfo::Adi_get_split_chn_rect_12SPLIT(av_rect_t *pVideo_rect, int chn_num, av_point_t *pLeft_top_point, av_point_t *pRight_bottom_point)
{
    if (pLeft_top_point == NULL || pRight_bottom_point == NULL)
    {
        DEBUG_ERROR( "CAvDeviceInfo::Adi_get_split_chn_rect_10SPLIT parameter error!!!\n");
        return -1;
    }

    switch(chn_num)
    {
        case 0:
            pLeft_top_point->x = pVideo_rect->x;
            pLeft_top_point->y = pVideo_rect->y ;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w / 4;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h / 3;
            break;
        case 1:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w / 4;
            pLeft_top_point->y = pVideo_rect->y ;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w / 2;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h / 3;
            break;
        case 2:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w / 2;
            pLeft_top_point->y = pVideo_rect->y ;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w *3 / 4;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h / 3;
            break;
        case 3:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w *3 / 4;
            pLeft_top_point->y = pVideo_rect->y;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w ;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h / 3;
            break;
        case 4:
            pLeft_top_point->x = pVideo_rect->x;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h / 3;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w / 4;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h * 2 / 3;
            break;
        case 5:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w / 4;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h / 3;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w *2/4;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h * 2 / 3;
            break;
        case 6:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w *2/ 4;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h / 3;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w *3/ 4;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h * 2 / 3;
            break;
        case 7:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w *3 / 4;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h / 3;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h * 2 / 3;
            break;
        case 8:
            pLeft_top_point->x = pVideo_rect->x;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 2 / 3;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w * 1 / 4;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;
            break;
        case 9:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w * 1 / 4;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 2 / 3;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w * 2 / 4;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;
            break;
        case 10:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w * 2 / 4;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 2 / 3;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w*3/4;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;
            break;
        case 11:
            pLeft_top_point->x = pVideo_rect->x + pVideo_rect->w * 3 / 4;
            pLeft_top_point->y = pVideo_rect->y + pVideo_rect->h * 2 / 3;
            pRight_bottom_point->x = pVideo_rect->x + pVideo_rect->w;
            pRight_bottom_point->y = pVideo_rect->y + pVideo_rect->h;
            break;
        default:
            break;
    }
    
    return 0;
}

int CAvDeviceInfo::Adi_get_split_chn_rect(av_rect_t *pVideo_rect, av_split_t split_mode, int chn_num, av_rect_t *pChn_rect, VO_DEV vo_dev)
{
    int ret_val = -1;

    av_point_t left_top_point, right_bottom_point;
    
	
    switch(split_mode)
    {
        case _AS_SINGLE_:
        case _AS_4SPLIT_:
        case _AS_9SPLIT_:
        case _AS_16SPLIT_:
        case _AS_25SPLIT_:
        case _AS_36SPLIT_:
            ret_val = Adi_get_split_chn_rect_REGULAR(pVideo_rect,split_mode,chn_num, &left_top_point, &right_bottom_point);
            break;
		case _AS_2SPLIT_ :
			ret_val = Adi_get_split_chn_rect_2SPLIT(pVideo_rect, chn_num, &left_top_point, &right_bottom_point);
			break;
		case _AS_3SPLIT_ :
			ret_val = Adi_get_split_chn_rect_3SPLIT(pVideo_rect, chn_num, &left_top_point, &right_bottom_point);
			break;			

        case _AS_6SPLIT_:
            ret_val = Adi_get_split_chn_rect_6SPLIT(pVideo_rect, chn_num, &left_top_point, &right_bottom_point);
            break;

        case _AS_8SPLIT_:
            ret_val = Adi_get_split_chn_rect_8SPLIT(pVideo_rect, chn_num, &left_top_point, &right_bottom_point);
            break;

        case _AS_10SPLIT_:
            ret_val = Adi_get_split_chn_rect_10SPLIT(pVideo_rect, chn_num, &left_top_point, &right_bottom_point);
            break;            
        case _AS_12SPLIT_:
            ret_val = Adi_get_split_chn_rect_12SPLIT(pVideo_rect, chn_num, &left_top_point, &right_bottom_point);
            break;            

        case _AS_PIP_:
        case _AS_DIGITAL_PIP_:
            return Adi_get_split_chn_rect_PIP(pVideo_rect, split_mode, chn_num, pChn_rect, vo_dev);
            break;

        case _AS_13SPLIT_:
            ret_val = Adi_get_split_chn_rect_13SPLIT(pVideo_rect, chn_num, &left_top_point, &right_bottom_point);
            break;

        default:
            _AV_FATAL_(1, "CAvDeviceInfo::Adi_get_split_chn_rect You must give the implement(%d)\n", split_mode);
            break;
    }
#if defined(_AV_PLATFORM_HI3521_)
    if (0 == vo_dev)
#else
    if (1 == vo_dev)
#endif
    {
#if 0    
    	 if (!special_info["CUSTOMER_NAME"].empty() && special_info["CUSTOMER_NAME"].isString())
    	 {
    		if(strcmp(special_info["CUSTOMER_NAME"].asCString(),"CUSTOMER_BJGJ") != 0)
    		{
                if(!m_screen_display_adjust)
                {
        			if(split_mode == _AS_SINGLE_ )
        			{
        				chn_num = 0;
        			}
                    if(0 != Adi_adjust_split_chn_rect(chn_num, &left_top_point, &right_bottom_point))
                    {
                        DEBUG_ERROR("CAvDeviceInfo::Adi_adjust_split_chn_rect() transform rect failed\n");
                    }
                }
    		}	
    	 }   
#endif		 
    }
    
    if(ret_val == 0)
    {
        left_top_point.x = (left_top_point.x + 1) / 2 * 2;
        left_top_point.y = (left_top_point.y + 1) / 2 * 2;
        right_bottom_point.x = (right_bottom_point.x + 1) / 2 * 2;
        right_bottom_point.y = (right_bottom_point.y + 1) / 2 * 2;
        
        pChn_rect->x = left_top_point.x;
        pChn_rect->y = left_top_point.y;
        pChn_rect->w = right_bottom_point.x - left_top_point.x;
        pChn_rect->h = right_bottom_point.y - left_top_point.y;
    }

    return ret_val;
}


int CAvDeviceInfo::Adi_have_spot_function()
{
    return 0;
}

int CAvDeviceInfo::Adi_max_channel_number()
{
#if defined(_AV_PRODUCT_CLASS_IPC_)
    /*IPC will only deal analog channel*/
    return Adi_get_vi_chn_num();
#else
    return Adi_get_vi_chn_num() + Adi_get_digital_chn_num();
#endif
}

bool CAvDeviceInfo::Adi_weather_phychn_sdi(int phy_chn_num)
{
    return false;
}

bool CAvDeviceInfo::Adi_is_dynamic_playback_by_split()
{
    bool mark = true;

    return mark;
}

int CAvDeviceInfo::Adi_get_max_resource(av_tvsystem_t tvsystem, int  &cif_total_count, int itotal_analog_num, int *iplayback_res_num, int *iencode_analog_res_num)
{    
    int palyback_cif_total_count = 0;
    int encode_cif_total_count = 0;
    int iplayback_frame_rate;
    cif_total_count = 0;
    
#if 1//_AV_PLATFORM_HI3521_
    iplayback_frame_rate = (tvsystem == _AT_PAL_) ? 25 : 30;
#else
    iplayback_frame_rate = (tvsystem == _AT_PAL_) ? 15 : 16;
#endif

    encode_cif_total_count = Adi_calculate_encode_source(tvsystem, itotal_analog_num, iencode_analog_res_num);
    
    palyback_cif_total_count = Adi_calculate_playback_source(tvsystem, iplayback_frame_rate, iplayback_res_num);
    
    cif_total_count = encode_cif_total_count + palyback_cif_total_count;

    return 0;
}

int CAvDeviceInfo::Adi_calculate_encode_source(av_tvsystem_t tvsystem, int itotal_analog_num, int *iencode_analog_res_num)
{
    int factor = 1;
    int frame_rate = (tvsystem==_AT_PAL_) ? 25 : 30;
    int encode_cif_total_count = 0;
    
    if (0 != itotal_analog_num && NULL != iencode_analog_res_num)
    {
        
        if (0 != iencode_analog_res_num[1])
        {
            /*main encode*/
            Adi_get_factor_by_cif(_AR_D1_, factor);
            encode_cif_total_count += iencode_analog_res_num[1] * factor * frame_rate;
            /*sub and mobile encode*/
            Adi_get_factor_by_cif(_AR_CIF_, factor);
            encode_cif_total_count += iencode_analog_res_num[1] * factor * frame_rate * 2;
        }
        
        if (0 != iencode_analog_res_num[2])
        {
            /*main encode*/
            Adi_get_factor_by_cif(_AR_HD1_, factor);
            encode_cif_total_count += iencode_analog_res_num[2] * factor * frame_rate;
            /*sub and mobile encode*/
            Adi_get_factor_by_cif(_AR_CIF_, factor);
            encode_cif_total_count += iencode_analog_res_num[2] * factor * frame_rate * 2;
        }
        
        if (0 != iencode_analog_res_num[3])
        {
            /*main encode*/
            Adi_get_factor_by_cif(_AR_CIF_, factor);
            encode_cif_total_count += iencode_analog_res_num[3] * factor * frame_rate;
            /*sub and mobile encode*/
            Adi_get_factor_by_cif(_AR_CIF_, factor);
            encode_cif_total_count += iencode_analog_res_num[3] * factor * frame_rate * 2;
        }
        
        if (0 != iencode_analog_res_num[5])
        {
            /*main encode*/
            Adi_get_factor_by_cif(_AR_960H_WD1_, factor);
            encode_cif_total_count += iencode_analog_res_num[5] * factor * frame_rate;
            /*sub and mobile encode*/
            Adi_get_factor_by_cif(_AR_CIF_, factor);
            encode_cif_total_count += iencode_analog_res_num[5] * factor * frame_rate * 2;
        }
        
        if (0 != iencode_analog_res_num[6])
        {
            /*main encode*/
            Adi_get_factor_by_cif(_AR_960H_WHD1_, factor);
            encode_cif_total_count += iencode_analog_res_num[6] * factor * frame_rate;
            /*sub and mobile encode*/
            Adi_get_factor_by_cif(_AR_CIF_, factor);
            encode_cif_total_count += iencode_analog_res_num[6] * factor * frame_rate * 2;            
        }
        
        if (0 != iencode_analog_res_num[7])
        {
            /*main encode*/
            Adi_get_factor_by_cif(_AR_960H_WCIF_, factor);
            encode_cif_total_count += iencode_analog_res_num[7] * factor * frame_rate;
            /*sub and mobile encode*/
            Adi_get_factor_by_cif(_AR_CIF_, factor);
            encode_cif_total_count += iencode_analog_res_num[7] * factor * frame_rate * 2;            
        }
    }
    
    return encode_cif_total_count;
}

int CAvDeviceInfo::Adi_calculate_playback_source(av_tvsystem_t tvsystem, int iplayback_frame_rate, int *iplayback_res_num)
{
    int factor = 1;
    int playback_cif_total_count = 0;

    if (NULL != iplayback_res_num)
    {
        if (0 != iplayback_res_num[0])
        {
            Adi_get_factor_by_cif(_AR_CIF_, factor);
            playback_cif_total_count += iplayback_res_num[0] * factor * iplayback_frame_rate;
        }

        if (0 != iplayback_res_num[1])
        {
            Adi_get_factor_by_cif(_AR_HD1_, factor);
            playback_cif_total_count += iplayback_res_num[1] * factor * iplayback_frame_rate;
        }

        if (0 != iplayback_res_num[2])
        {      
            Adi_get_factor_by_cif(_AR_D1_, factor);
            playback_cif_total_count += iplayback_res_num[2] * factor * iplayback_frame_rate;
        }

        if(0 != iplayback_res_num[11])
        {             
             Adi_get_factor_by_cif(_AR_960H_WCIF_, factor);
             playback_cif_total_count += iplayback_res_num[11] * factor * iplayback_frame_rate;
        }

        if (0 != iplayback_res_num[12])
        {
            Adi_get_factor_by_cif(_AR_960H_WHD1_, factor);
            playback_cif_total_count += iplayback_res_num[12] * factor * iplayback_frame_rate;
        }

        if(0 != iplayback_res_num[13])
        {
            Adi_get_factor_by_cif(_AR_960H_WD1_, factor);
            playback_cif_total_count += iplayback_res_num[13] * factor * iplayback_frame_rate;
        }
        
        if (0 != iplayback_res_num[6])
        {
            Adi_get_factor_by_cif(_AR_720_, factor);
            playback_cif_total_count += iplayback_res_num[6] * factor * iplayback_frame_rate;
        }

        if (0 != iplayback_res_num[7])
        {
            Adi_get_factor_by_cif(_AR_1080_, factor);
            playback_cif_total_count += iplayback_res_num[7] * factor * iplayback_frame_rate;
        }
    }

    return playback_cif_total_count;
} 


unsigned long long int CAvDeviceInfo::Adi_get_all_chn_mask()
{
    int imax_chn_num = pgAvDeviceInfo->Adi_max_channel_number();
    unsigned long long int  u64all_chn_mask = 0;

    for(int chn_num = 0; chn_num < imax_chn_num; chn_num++)
    {
        u64all_chn_mask = ((u64all_chn_mask) | (1ll << (chn_num)));
    }

    return u64all_chn_mask;
}

int CAvDeviceInfo::Adi_get_video_size(av_resolution_t resolution, av_tvsystem_t tv_system, int *pWidth, int *pHeight)
{
    switch(resolution)
    {
        case _AR_D1_:
            _AV_POINTER_ASSIGNMENT_(pWidth,  pgAvDeviceInfo->Adi_get_D1_width());
            _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 576 : 480);
            break;

        case _AR_HD1_:
            _AV_POINTER_ASSIGNMENT_(pWidth,  pgAvDeviceInfo->Adi_get_D1_width());
            _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 288 : 240);
            break;

        case _AR_CIF_:
            _AV_POINTER_ASSIGNMENT_(pWidth,  pgAvDeviceInfo->Adi_get_D1_width() / 2);
            _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 288 : 240);
            break;

        case _AR_QCIF_:
            _AV_POINTER_ASSIGNMENT_(pWidth,  pgAvDeviceInfo->Adi_get_D1_width() / 4);
            _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 144 : 120);
            break;

        case _AR_960H_WD1_:
            _AV_POINTER_ASSIGNMENT_(pWidth, pgAvDeviceInfo->Adi_get_WD1_width());
            _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 576 : 480);
            break;

        case _AR_960H_WHD1_:
            _AV_POINTER_ASSIGNMENT_(pWidth, pgAvDeviceInfo->Adi_get_WD1_width());
            _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 288 : 240);
            break;

        case _AR_960H_WCIF_:
            _AV_POINTER_ASSIGNMENT_(pWidth, pgAvDeviceInfo->Adi_get_WD1_width() / 2);
            _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 288 : 240);
            break;

        case _AR_960H_WQCIF_:
            //_AV_POINTER_ASSIGNMENT_(pWidth, pgAvDeviceInfo->Adi_get_WD1_width() / 4);
            _AV_POINTER_ASSIGNMENT_(pWidth, 240);
            _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 144 : 120);
            break;

        case _AR_1080_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 1920);
            _AV_POINTER_ASSIGNMENT_(pHeight, 1080);
            break;

        case _AR_720_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 1280);
            _AV_POINTER_ASSIGNMENT_(pHeight, 720);
            break;

        case _AR_VGA_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 640);
            _AV_POINTER_ASSIGNMENT_(pHeight, 360);
            break;
        case  _AR_QVGA_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 320);
            _AV_POINTER_ASSIGNMENT_(pHeight, 180);
            break;
        case  _AR_960P_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 1280);
            _AV_POINTER_ASSIGNMENT_(pHeight, 960);
            break;
        case _AR_Q1080P_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 960);
            _AV_POINTER_ASSIGNMENT_(pHeight, 540);
            break;
        case _AR_3M_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 2048);
            _AV_POINTER_ASSIGNMENT_(pHeight, 1536);
            break;
        case _AR_5M_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 2592);
            _AV_POINTER_ASSIGNMENT_(pHeight, 1920);
            break;
        case _AR_SVGA_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 800);
            _AV_POINTER_ASSIGNMENT_(pHeight, 600);
            break;
        case _AR_XGA_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 1024);
            _AV_POINTER_ASSIGNMENT_(pHeight, 768);
            break;


        default:
            //_AV_FATAL_(1, "CAvDeviceInfo::Adi_get_video_size You must give the implement res=%d\n", resolution);
            break;
    }

    return 0;
}

#if defined(_AV_PRODUCT_CLASS_IPC_)
int CAvDeviceInfo::Adi_get_video_size(IspOutputMode_e iom, int *pWidth, int *pHeight)
{
    if((pWidth == NULL) || (pHeight == NULL))
    {
        return -1;
    }

    switch(iom)
    {
        case ISP_OUTPUT_MODE_720P30FPS:
        case ISP_OUTPUT_MODE_720P60FPS:
        case ISP_OUTPUT_MODE_720P120FPS:
        default:
            *pWidth = 1280;
            *pHeight = 720;
            break;
        case ISP_OUTPUT_MODE_1080P30FPS:
        case ISP_OUTPUT_MODE_1080P60FPS:
            *pWidth = 1920;
            *pHeight = 1080;            
            break;
        case ISP_OUTPUT_MODE_3M30FPS:
            *pWidth = 2048;
            *pHeight = 1536;   
            break;
        case ISP_OUTPUT_MODE_5M30FPS:
            *pWidth = 2592;
            *pHeight = 1920;                
            break;
        case ISP_OUTPUT_MODE_960P60FPS:
        case ISP_OUTPUT_MODE_960P30FPS:
            *pWidth = 1280;
            *pHeight = 960;   
            break;
    }

    return 0;
}
#else
int CAvDeviceInfo::Adi_get_video_size_input_mix(unsigned char chn,av_resolution_t resolution, av_tvsystem_t tv_system, int *pWidth, int *pHeight)
{
    int retval = 0;
    switch(resolution)
    {
        case _AR_D1_:
            _AV_POINTER_ASSIGNMENT_(pWidth,  3840);
            _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 576 : 480);
            break;

        case _AR_960H_WD1_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 3712);
            _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 576 : 480);
            break;
        case _AR_720_:
            retval = Adi_get_ahd_video_format(chn);
            if(retval == 1)
            {
                _AV_POINTER_ASSIGNMENT_(pWidth, 2560);
                _AV_POINTER_ASSIGNMENT_(pHeight, 720);
            }
            else
            {
                _AV_POINTER_ASSIGNMENT_(pWidth, 1280);
                _AV_POINTER_ASSIGNMENT_(pHeight, 720);
            }
            break;
        default:
            //_AV_FATAL_(1, "CAvDeviceInfo::Adi_get_video_size You must give the implement res=%d\n", resolution);
            break;
    }
    printf("chn:%d resolution:%d width:%d height:%d\n",chn,resolution,*pWidth,*pHeight);
    return 0;
}
#endif

#if defined(_AV_PLATFORM_HI3515_)
int CAvDeviceInfo::Adi_get_vi_video_size(av_resolution_t resolution, av_tvsystem_t tv_system, int *pWidth, int *pHeight)
{
    switch(resolution)
    {
        case _AR_D1_:
            _AV_POINTER_ASSIGNMENT_(pWidth,  pgAvDeviceInfo->Adi_get_D1_width());
            _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 288 : 240);
            break;

        case _AR_HD1_:
            _AV_POINTER_ASSIGNMENT_(pWidth,  pgAvDeviceInfo->Adi_get_D1_width());
            _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 288 : 240);
            break;

        case _AR_CIF_:
            _AV_POINTER_ASSIGNMENT_(pWidth,  pgAvDeviceInfo->Adi_get_D1_width());
            _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 288 : 240);
            break;

        case _AR_QCIF_:
            _AV_POINTER_ASSIGNMENT_(pWidth,  pgAvDeviceInfo->Adi_get_D1_width() / 4);
#if defined(_AV_PLATFORM_HI3515_)
            //_AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 288 : 240);
            _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 72 : 60);
#else			
            _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 144 : 120);
#endif
            break;

        case _AR_960H_WD1_:
#if defined(_AV_PLATFORM_HI3515_)
            _AV_POINTER_ASSIGNMENT_(pWidth,  pgAvDeviceInfo->Adi_get_D1_width());
            _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 288 : 240);
#else
            _AV_POINTER_ASSIGNMENT_(pWidth, pgAvDeviceInfo->Adi_get_WD1_width());
	    _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 576 : 480);
#endif
            break;

        case _AR_960H_WHD1_:
#if defined(_AV_PLATFORM_HI3515_)
            _AV_POINTER_ASSIGNMENT_(pWidth,  pgAvDeviceInfo->Adi_get_D1_width());
            _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 288 : 240);
#else			
            _AV_POINTER_ASSIGNMENT_(pWidth, pgAvDeviceInfo->Adi_get_WD1_width());
            _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 288 : 240);
#endif
            break;

        case _AR_960H_WCIF_:

#if defined(_AV_PLATFORM_HI3515_)
            _AV_POINTER_ASSIGNMENT_(pWidth, pgAvDeviceInfo->Adi_get_WD1_width() / 2);
	     _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 288 : 240);
#else
            _AV_POINTER_ASSIGNMENT_(pWidth, pgAvDeviceInfo->Adi_get_WD1_width() / 2);
	     _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 288 : 240);
#endif
            break;

        case _AR_960H_WQCIF_:
           // _AV_POINTER_ASSIGNMENT_(pWidth, 240);
#if defined(_AV_PLATFORM_HI3515_)
            _AV_POINTER_ASSIGNMENT_(pWidth, pgAvDeviceInfo->Adi_get_WD1_width() / 2);
	    _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 144 : 120);
#else
            _AV_POINTER_ASSIGNMENT_(pWidth, pgAvDeviceInfo->Adi_get_WD1_width() / 4);
	    _AV_POINTER_ASSIGNMENT_(pHeight, (tv_system == _AT_PAL_) ? 144 : 120);
#endif
            break;

        case _AR_1080_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 1920);
            _AV_POINTER_ASSIGNMENT_(pHeight, 540);
            break;

        case _AR_720_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 1280);
            _AV_POINTER_ASSIGNMENT_(pHeight, 720);
            break;

        case _AR_VGA_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 640);
            _AV_POINTER_ASSIGNMENT_(pHeight, 480);
            break;
        case  _AR_QVGA_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 320);
            _AV_POINTER_ASSIGNMENT_(pHeight, 240);
            break;
        case  _AR_960P_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 1280);
            _AV_POINTER_ASSIGNMENT_(pHeight, 960);
            break;
        case _AR_Q1080P_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 960);
            _AV_POINTER_ASSIGNMENT_(pHeight, 540);
            break;
        case _AR_3M_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 2048);
            _AV_POINTER_ASSIGNMENT_(pHeight, 1536);
            break;
        case _AR_5M_:
            _AV_POINTER_ASSIGNMENT_(pWidth, 2592);
            _AV_POINTER_ASSIGNMENT_(pHeight, 1920);
            break;
        default:
            _AV_FATAL_(1, "CAvDeviceInfo::Adi_get_video_size You must give the implement res=%d\n", resolution);
            break;
    }

    return 0;
}
#endif

int CAvDeviceInfo::Adi_get_factor_by_cif(av_resolution_t av_resolution, int &factor)
{
    factor = 10;
    switch(av_resolution)
    {
        case _AR_D1_:
            factor = 40;
            break;
        case _AR_960H_WD1_:
            factor = 53;
            break;
        case _AR_HD1_:
            factor = 20;
            break;
        case _AR_960H_WHD1_:
            factor = 27;
            break;
        case _AR_1080_:
            factor = 205;
            break;
        case _AR_720_:
            factor = 91;
            break;
        case _AR_VGA_:
            factor = 23;
            break;
        case _AR_Q1080P_:
            factor = 51; //1080=20/4
            break;
        case _AR_960H_WCIF_:
            factor = 13;
            break;
        case _AR_960P_:
            factor = 122;
            break;
        case _AR_CIF_:
            break;
        case _AR_QCIF_:
            factor = 3;
            break;
        case _AR_3M_:
            factor = 310;//(2048*1536)/(352*288)*10
            break;
        case _AR_5M_:
            factor = 491;
            break;
        case _AR_SVGA_:
            factor = 47;
            break;
        case _AR_XGA_:
            factor = 78;
            break;

        default:
            break;
    }
    return 0;
}

int CAvDeviceInfo::Adi_get_video_resolution(uint8_t *piResolution, av_resolution_t *paResolution, bool from_i_to_a)
{
    /* 0-CIF 1-HD1 2-D1 3-QCIF 4-QVGA 5-VGA 6-720P 7-1080P 8-3MP(2048*1536) 9-5MP(2592*1920) 10 WQCIF,11 WCIF,12 WHD1,13 WD1(960H) 14(960p) 15(540P)*/
    struct _res_map_{
        int iR;
        av_resolution_t aR;
    }res_map[] = {
        {0, _AR_CIF_}, 
        {1, _AR_HD1_}, 
        {2, _AR_D1_}, 
        {3, _AR_QCIF_}, 
        {4, _AR_QVGA_},
        {5, _AR_VGA_},
        {6, _AR_720_}, 
        {7, _AR_1080_}, 
        {8, _AR_3M_},
        {9, _AR_5M_},
        {10, _AR_960H_WQCIF_}, 
        {11, _AR_960H_WCIF_}, 
        {12, _AR_960H_WHD1_}, 
        {13, _AR_960H_WD1_}, 
        {14, _AR_960P_},
        {15, _AR_Q1080P_},
        {16, _AR_SVGA_},
        {17, _AR_XGA_},
        {-1, _AR_UNKNOWN_}, 
    };

    int index = 0;
    while(res_map[index].iR != -1)
    {
        if(from_i_to_a == true)
        {
            if(*piResolution == res_map[index].iR)
            {
                *paResolution = res_map[index].aR;
                return 0;
            }
        }
        else
        {
            if(*paResolution == res_map[index].aR)
            {
                *piResolution = res_map[index].iR;
                return 0;
            }
        }
        index ++;
    }

    return -1;
}

unsigned int CAvDeviceInfo::Adi_get_vo_background_color( av_output_type_t output, int index )
{
    return 0x0;
}

#define OSD_COORDINATE_width 1024
#define OSD_COORDINATE_height 768
int CAvDeviceInfo::Adi_covert_coordinate(av_resolution_t resolution, av_tvsystem_t tv_system, int *pDirX, int *pDirY, int align/*=16*/)
{
    int u32ResWidth = 0;
    int u32ResHeight = 0;
    this->Adi_get_video_size( resolution, tv_system, &u32ResWidth, &u32ResHeight );

#if defined(_AV_PRODUCT_CLASS_IPC_)
    this->Adi_covert_vidoe_size_by_rotate( &u32ResWidth, &u32ResHeight );
#endif


    (*pDirX) = ((*pDirX) * u32ResWidth / OSD_COORDINATE_width) / align * align;
    (*pDirY) = ((*pDirY) * u32ResHeight / OSD_COORDINATE_height) / align * align;


    return 0;
}

int CAvDeviceInfo::Adi_covert_coordinateEx( unsigned short u16PicWidth, unsigned short u16PicHeight, int *pDirX, int *pDirY, int align/*=16*/ )
{
    (*pDirX) = ((*pDirX) * u16PicWidth / OSD_COORDINATE_width) / align * align;
    (*pDirY) = ((*pDirY) * u16PicHeight / OSD_COORDINATE_height) / align * align;

    return 0;
}

int CAvDeviceInfo::Adi_get_audio_chn_id(int phy_audio_chn_num, int &phy_audio_chn_id)
{
    phy_audio_chn_id = phy_audio_chn_num;
    if(phy_audio_chn_id >= Adi_get_audio_chn_number())
    {
        phy_audio_chn_id = -1;
    }
    return 0;
}

unsigned long long int CAvDeviceInfo::Aa_get_all_analog_chn_mask()
{
    int max_vi_chn_num = pgAvDeviceInfo->Adi_get_vi_chn_num();
    unsigned long long int ullencode_local_chn_mask = 0;
    for(int chn_num = 0; chn_num < max_vi_chn_num; chn_num++)
    {
        if(_VS_DIGITAL_ == pgAvDeviceInfo->Adi_get_video_source(chn_num))
        {
            continue;
        }
        ullencode_local_chn_mask = ((ullencode_local_chn_mask) | (1ll << (chn_num)));
    }
    
    return ullencode_local_chn_mask;
}

int CAvDeviceInfo::Aa_set_screenmode(unsigned int screenmode)
{
	m_screenmode = screenmode;
	return 0;
}

unsigned int CAvDeviceInfo::Aa_get_screenmode()
{
	return m_screenmode;
}

int CAvDeviceInfo::Adi_get_ref_para(av_ref_para_t &av_ref_para, av_video_stream_type_t eStreamType/*=_AST_MAIN_VIDEO_*/)
{
#if defined(_AV_PLATFORM_HI3516A_)
    // depend on N9,the 1080p product,the sub stream ref is ref for 4x,other stream is ref for 2x
    if(_AST_SUB_VIDEO_ == eStreamType)
    {
        //ref for 4x
        av_ref_para.ref_type = _ART_REF_FOR_4X;
        av_ref_para.pred_enable = 1;
        av_ref_para.base_interval = 2;
        av_ref_para.enhance_interval = 1;
    }
    else
    {
        //ref for 2x
        av_ref_para.ref_type = _ART_REF_FOR_2X;
        av_ref_para.pred_enable = 1;
        av_ref_para.base_interval = 1;
        av_ref_para.enhance_interval = 1;    
    }
#elif defined(_AV_PLATFORM_HI3518EV200_)
    av_ref_para.ref_type = _ART_REF_FOR_1X;
    av_ref_para.pred_enable = 1;
    av_ref_para.base_interval = 1;
    av_ref_para.enhance_interval = 0;
#else
    av_ref_para.ref_type = _ART_REF_FOR_2X;
    av_ref_para.pred_enable = 1;
    av_ref_para.base_interval = 1;
    av_ref_para.enhance_interval = 1;

    if( PTD_913_VA == pgAvDeviceInfo->Adi_product_type())
    {
        if(_AST_SUB_VIDEO_ == eStreamType)
        {
            av_ref_para.ref_type = _ART_REF_FOR_4X;
            av_ref_para.pred_enable = 1;
            av_ref_para.base_interval = 2;
            av_ref_para.enhance_interval = 1;
        }
    }
#endif
    
    return 0;
}
int CAvDeviceInfo::Adi_get_sub_encoder_resource(int &encode_resoure, av_tvsystem_t av_tvsystem)
{
    int chnnum = Adi_max_channel_number();
    
    encode_resoure = chnnum * ((av_tvsystem == _AT_PAL_) ? 25 : 30);
    return 0;
}

#if defined(_AV_PRODUCT_CLASS_IPC_)
int CAvDeviceInfo::Adi_set_system_rotate( unsigned char u8Rorate )
{   
    if( m_u8Rotate == u8Rorate )
    {
        return -1;
    }
    
    m_u8Rotate = u8Rorate; 

    return 0; 
}
int CAvDeviceInfo::Adi_covert_vidoe_size_by_rotate( int* pWidth, int* pHeight )
{
    if( m_u8Rotate == 1 || m_u8Rotate == 3 ) //rotate 90° 270°
    {
        int mediaValue = (*pWidth);
        *pWidth = (*pHeight);
        *pHeight = mediaValue;
    }

    return 0;
}
#endif

int CAvDeviceInfo::Adi_get_date_time(datetime_t *pDate_time)
{
    unsigned char u8ChangedCount = 0;
    N9M_TMGetShareTime(pDate_time, u8ChangedCount);
    if (u8ChangedCount != m_system_change_count)
    {
        for (int type = 0; type < _AV_VIDEO_STREAM_MAX_NUM_; type++)
        {
            m_channel_record_cut[type].set();
        }
        m_system_change_count = u8ChangedCount;
    }
    return 0;
}

int CAvDeviceInfo::Adi_get_record_cut(int type, int phy_chn)
{
    int ret_val = 0;
    if ((type < 0) || (type >= _AV_VIDEO_STREAM_MAX_NUM_)
        || (phy_chn < 0) || (phy_chn >= Adi_max_channel_number()))
    {
        return -1;
    }
    
    ret_val = m_channel_record_cut[type].test(phy_chn);
    m_channel_record_cut[type].reset(phy_chn);

    return ret_val;
}

int CAvDeviceInfo::Adi_set_vi_info(unsigned int vi_num, unsigned int vi_max_width, unsigned int vi_max_height)
{
    m_vi_num = vi_num;
    m_vi_max_width = vi_max_width;
    m_vi_max_height = vi_max_height;
    return 0;
}

int CAvDeviceInfo::Adi_get_vi_info(unsigned int &vi_num, unsigned int &vi_max_width, unsigned int &vi_max_height)
{
    vi_num = m_vi_num;
    vi_max_width = m_vi_max_width;
    vi_max_height = m_vi_max_height;
    return 0;
}

int CAvDeviceInfo::Adi_get_vi_chn_num()
{
    return m_vi_num;
}

int CAvDeviceInfo::Adi_set_cur_chn_res(int phy_chn_num, unsigned int width, unsigned int height)
{   
    // it should double height when displaying HD1 or WHD1
    if((704 == width || 720 == width || width==928 || width==960 || width==944) && (240 == height || 288 == height))
    {
        height = 2 * height;
    }

    if(width==928 || width==960 || width==944)
    {
    	 width = 704;
    }

    if(width==464 || width==480 || width==472)
    {
    	 width = 352;
    }

    std::map<int, std::pair<int,int> >::iterator map_it;
    std::pair<int,int> temp_pair(width, height);
    if( (map_it = m_cur_preview_res.find(phy_chn_num)) ==  m_cur_preview_res.end())
    {
        m_cur_preview_res[phy_chn_num] = temp_pair;
    }
    else
    {
        if(map_it->second == temp_pair)
        {
            DEBUG_MESSAGE("CAvDeviceInfo::Adi_set_cur_chn_res(phy_chn_num = %d has same width = %d and height = %d)\n",phy_chn_num, width, height);
        }
        else
        {
           m_cur_preview_res.erase(phy_chn_num);
           m_cur_preview_res[phy_chn_num] = temp_pair;
        }
    }
    return 0;
}

int CAvDeviceInfo::Adi_get_cur_display_chn(int cur_display_chn[36])
{
    if(NULL == cur_display_chn)
    {
        DEBUG_ERROR("CAvDeviceInfo::Adi_get_cur_display_chn() parameter invalid\n");
        return -1;
    }
    memset(m_cur_display_chn, -1, sizeof(int) * 36);
    memcpy(m_cur_display_chn, cur_display_chn, sizeof(av_resolution_t) * 32);

    return 0;
}


void CAvDeviceInfo::Adi_backup_cur_preview_res()
{
    m_cur_preview_res_backup.erase(m_cur_preview_res_backup.begin(), m_cur_preview_res_backup.end());
    
    m_cur_preview_res_backup = m_cur_preview_res;

    return;
}

void CAvDeviceInfo::Adi_set_cur_preview_res_with_backup_res()
{
    m_cur_preview_res.erase(m_cur_preview_res.begin(), m_cur_preview_res.end());
    
    m_cur_preview_res = m_cur_preview_res_backup;

    m_cur_preview_res_backup.erase(m_cur_preview_res_backup.begin(), m_cur_preview_res_backup.end());

    return;
}


int CAvDeviceInfo::Adi_set_stream_buffer_info(eStreamBufferType stream_type, unsigned int chn_id, char buf_name[32], unsigned int buf_size, unsigned int buf_cnt)
{
    snprintf(m_stream_buffer_info[stream_type][chn_id].buf_name, 32, "%s", buf_name);
    m_stream_buffer_info[stream_type][chn_id].buf_size = buf_size;
    m_stream_buffer_info[stream_type][chn_id].buf_cnt = buf_cnt;

    return 0;
}

int CAvDeviceInfo::Adi_get_stream_buffer_info(eStreamBufferType stream_type, unsigned int chn_id, char buf_name[32], unsigned int &buf_size, unsigned int &buf_cnt)
{
    if(m_stream_buffer_info.find(stream_type) != m_stream_buffer_info.end())
    {
        if(m_stream_buffer_info[stream_type].find(chn_id) != m_stream_buffer_info[stream_type].end())
        {
            snprintf(buf_name, 32, "%s", m_stream_buffer_info[stream_type][chn_id].buf_name);
            buf_size = m_stream_buffer_info[stream_type][chn_id].buf_size;
            buf_cnt = m_stream_buffer_info[stream_type][chn_id].buf_cnt;
            return 0;
        }
    }
    return -1;
}

int CAvDeviceInfo::Adi_set_digital_chn_num(unsigned int digital_chn_num)
{
    m_digital_num = digital_chn_num;
    return 0;
}

int CAvDeviceInfo::Adi_get_digital_chn_num()
{
    return m_digital_num;
}
//! novel speed
#if !defined(_AV_PRODUCT_CLASS_DVR_REI_)
int CAvDeviceInfo::Adi_Set_Speed_Info(msgSpeedInfo_t *speedinfo)
{
    if(speedinfo)
    {
        memcpy(&m_speed_uni,speedinfo,sizeof(msgSpeedInfo_t));
    }
    return -1;
}

int CAvDeviceInfo::Adi_Get_Speed_Info(msgSpeedInfo_t *speedinfo)
{
    if(speedinfo)
    {
        memcpy(speedinfo,&m_speed_uni,sizeof(msgSpeedInfo_t));
    }
    return -1;
}

#endif
int CAvDeviceInfo::Adi_Set_Speed_Info(msgSpeedState_t *speedinfo)
{
    if(speedinfo)
    {
        memcpy(&m_speed,speedinfo,sizeof(msgSpeedState_t));
    }
    return -1;
}

int CAvDeviceInfo::Adi_Get_Speed_Info(msgSpeedState_t *speedinfo)
{
    if(speedinfo)
    {
        memcpy(speedinfo,&m_speed,sizeof(msgSpeedState_t));
    }
    return -1;
}
//0703
int CAvDeviceInfo::Adi_Set_Speed_Info_from_Tax2(Tax2DataList *speedinfo)
{
    if(speedinfo)
    {
        memcpy(&m_tax2_speed,speedinfo,sizeof(Tax2DataList));
    }
    return -1;
}

int CAvDeviceInfo::Adi_Get_Speed_Info_from_Tax2(Tax2DataList *speedinfo)
{
    if(speedinfo)
    {
        memcpy(speedinfo,&m_tax2_speed,sizeof(Tax2DataList));
    }
    return -1;
}
int CAvDeviceInfo::Adi_Set_Gps_Info(msgGpsInfo_t *gpsinfo)
{
    if(gpsinfo)
    {
        memcpy(&m_gpsinfo,gpsinfo,sizeof(msgGpsInfo_t));
    }
    return -1;
}

int CAvDeviceInfo::Adi_Get_Gps_Info(msgGpsInfo_t *gpsinfo)
{
    if(gpsinfo)
    {
        memcpy(gpsinfo,&m_gpsinfo,sizeof(msgGpsInfo_t));
    }
    return -1;
}

int CAvDeviceInfo::Adi_Set_SpeedSource_Info(paramSpeedAlarmSetting_t *speedsource)
{
    if(speedsource)
    {
        memcpy(&m_Speed_Info,speedsource,sizeof(paramSpeedAlarmSetting_t));
    }
    return -1;
}

int CAvDeviceInfo::Adi_Get_SpeedSource_Info(paramSpeedAlarmSetting_t *speedsource)
{
    if(speedsource)
    {
        memcpy(speedsource,&m_Speed_Info,sizeof(paramSpeedAlarmSetting_t));
    }
    return -1;
}

#if defined(_AV_PRODUCT_CLASS_IPC_)
void CAvDeviceInfo::Adi_set_mac_address(const rm_uint8_t mac_address[], unsigned char array_len)
{
    int index = 0;
    if(0 == memcmp(mac_address, m_u8MacAddress, array_len))
    {
        return;
    }
    
    memcpy(m_u8MacAddress, mac_address, sizeof(m_u8MacAddress));
    for(int i=0; i<6;++i)
    {
        snprintf(m_u8MacAddressStr+index, sizeof(m_u8MacAddressStr)-index, "%02X", m_u8MacAddress[i]);
        index += 2;
        
        if(i != 5)
        {
            snprintf(m_u8MacAddressStr+index, sizeof(m_u8MacAddressStr)-index, "-");
            index += 1;
        }
    }

    _AV_KEY_INFO_(_AVD_APP_, "MAC address is:%s \n", m_u8MacAddressStr);
}
void CAvDeviceInfo::Adi_get_mac_address (rm_uint8_t mac_address[], unsigned char array_len) const
{
    int len = array_len>sizeof(m_u8MacAddress)?sizeof(m_u8MacAddress):array_len;
    memcpy(mac_address, m_u8MacAddress, len);
}
void CAvDeviceInfo::Adi_get_mac_address_as_string (char mac_address[], unsigned char array_len) const
{
    int len = array_len>sizeof(m_u8MacAddressStr)?sizeof(m_u8MacAddressStr):array_len;
    memcpy(mac_address, m_u8MacAddressStr, len);
}

void CAvDeviceInfo::Adi_set_vi_config(sensor_e sensor_type, unsigned char phy_chn_num, vi_capture_rect_s& vi_cap_rect)
{
    if(m_sVi_cfg.empty())
    {
        vi_cfg_s vi_cfg;
        vi_cfg.m_eSensor_type = sensor_type;
        std::vector<vi_capture_rect_s> rects;
        rects.push_back(vi_cap_rect);
        vi_cfg.vi_capture_chn_cfg.push_back(rects);
        m_sVi_cfg.push_back(vi_cfg);
    }
    else
    {
        unsigned int i;
        for(i=0; i<m_sVi_cfg.size();++i)
        {
            if(m_sVi_cfg[i].m_eSensor_type == sensor_type)
            {
                if(m_sVi_cfg[i].vi_capture_chn_cfg.size()<=phy_chn_num)
                {
                    std::vector<vi_capture_rect_s> rects;
                    rects.push_back(vi_cap_rect);
                    m_sVi_cfg[i].vi_capture_chn_cfg.push_back(rects);
                }
                else
                {
                    m_sVi_cfg[i].vi_capture_chn_cfg[phy_chn_num].push_back(vi_cap_rect);
                }
                break;
            }
        }

        if(i >= m_sVi_cfg.size())
        {
            vi_cfg_s vi_cfg;
            vi_cfg.m_eSensor_type = sensor_type;
            std::vector<vi_capture_rect_s> rects;
            rects.push_back(vi_cap_rect);
            vi_cfg.vi_capture_chn_cfg.push_back(rects);
            m_sVi_cfg.push_back(vi_cfg);
        }
    }
}
void CAvDeviceInfo::Adi_get_vi_config(sensor_e sensor_type, unsigned char phy_chn_num, unsigned char resolution, vi_capture_rect_s& vi_cap_rect)
{
#if 0
    for(unsigned int m=0;m<m_sVi_cfg.size();++m)
    {
        for(unsigned int n=0;n<m_sVi_cfg[m].vi_capture_chn_cfg.size();++n)
        {
            for(unsigned int l=0;l<m_sVi_cfg[m].vi_capture_chn_cfg[n].size();++l)
            {
                printf("[alex]sensor type:%d phy_chn_num:%d res:%d [w:%d h:%d x:%d y:%d] \n", m_sVi_cfg[m].m_eSensor_type, n, \
                    m_sVi_cfg[m].vi_capture_chn_cfg[n][l].m_res, m_sVi_cfg[m].vi_capture_chn_cfg[n][l].m_vi_capture_width, \
                    m_sVi_cfg[m].vi_capture_chn_cfg[n][l].m_vi_capture_height, m_sVi_cfg[m].vi_capture_chn_cfg[n][l].m_vi_capture_x, \
                    m_sVi_cfg[m].vi_capture_chn_cfg[n][l].m_vi_capture_y);
            }
        }
    }
#endif
    do
    {
        if(m_sVi_cfg.empty())
        {
            break;
        }
        for(unsigned int i=0;i<m_sVi_cfg.size();++i)
        {
            if(m_sVi_cfg[i].m_eSensor_type == sensor_type)
            {
                if(m_sVi_cfg[i].vi_capture_chn_cfg.size() > phy_chn_num)
                {
                    if(!m_sVi_cfg[i].vi_capture_chn_cfg[phy_chn_num].empty())
                    {
                        for(unsigned int j=0;j<m_sVi_cfg[i].vi_capture_chn_cfg[phy_chn_num].size();++j)
                        {
                            if(m_sVi_cfg[i].vi_capture_chn_cfg[phy_chn_num][j].m_res == resolution)
                            {
                                vi_cap_rect.m_vi_capture_width = m_sVi_cfg[i].vi_capture_chn_cfg[phy_chn_num][j].m_vi_capture_width;
                                vi_cap_rect.m_vi_capture_height = m_sVi_cfg[i].vi_capture_chn_cfg[phy_chn_num][j].m_vi_capture_height;
                                vi_cap_rect.m_vi_capture_x = m_sVi_cfg[i].vi_capture_chn_cfg[phy_chn_num][j].m_vi_capture_x;
                                vi_cap_rect.m_vi_capture_y = m_sVi_cfg[i].vi_capture_chn_cfg[phy_chn_num][j].m_vi_capture_y;
                                return;
                            }
                        }
                    }
                }
            }
        }
    }while(0);
    
    if(m_sVi_cfg.empty())
    {
        switch(resolution)
        {
            //720P
            case 6:
            default:
            {
                vi_cap_rect.m_res = 6;
                vi_cap_rect.m_vi_capture_width = 1280;
                vi_cap_rect.m_vi_capture_height = 720;
                vi_cap_rect.m_vi_capture_x = 0;
                vi_cap_rect.m_vi_capture_y = 0;
            }
                break;
            //1280
            case 7:
            {
                vi_cap_rect.m_res = 7;
                vi_cap_rect.m_vi_capture_width = 1920;
                vi_cap_rect.m_vi_capture_height = 1080;
                vi_cap_rect.m_vi_capture_x = 0;
                vi_cap_rect.m_vi_capture_y = 0;
            }
                break;
            //3M
            case 8:
            {
                vi_cap_rect.m_res = 8;
                vi_cap_rect.m_vi_capture_width = 2048;
                vi_cap_rect.m_vi_capture_height = 1536;
                vi_cap_rect.m_vi_capture_x = 0;
                vi_cap_rect.m_vi_capture_y = 0;
            }
                break;
            //5M
            case 9:
            {
                vi_cap_rect.m_res = 9;
                vi_cap_rect.m_vi_capture_width = 2592;
                vi_cap_rect.m_vi_capture_height = 1920;
                vi_cap_rect.m_vi_capture_x = 0;
                vi_cap_rect.m_vi_capture_y = 0;
            }
                break;
        }
    }
}

unsigned char CAvDeviceInfo::Adi_get_vi_max_resolution()
{
    switch(m_eIspOutputMode)
    {
        case    ISP_OUTPUT_MODE_5M30FPS:
            return 9;
        case    ISP_OUTPUT_MODE_3M30FPS:
            return 8;
        case    ISP_OUTPUT_MODE_1080P60FPS:
        case   ISP_OUTPUT_MODE_1080P30FPS:
#if defined(_AV_PLATFORM_HI3516A_)
        default:
 #endif
            return 7;
        case    ISP_OUTPUT_MODE_720P120FPS:
        case    ISP_OUTPUT_MODE_720P60FPS:
        case    ISP_OUTPUT_MODE_720P30FPS:
#if defined(_AV_PLATFORM_HI3518C_) || defined(_AV_PLATFORM_HI3518EV200_)
        default:
 #endif
            return 6;
        case ISP_OUTPUT_MODE_960P60FPS:
        case ISP_OUTPUT_MODE_960P30FPS:
            return 14;
    }
}

void CAvDeviceInfo::Adi_get_stream_size(unsigned char phy_chn, av_video_stream_type_t streamType, unsigned int& width, unsigned int &height)
{
    if(phy_chn >= SUPPORT_MAX_VIDEO_CHANNEL_NUM)
    {
        DEBUG_ERROR("the phy_chn:%d is invalid, the max chn number is:%d \n", phy_chn, SUPPORT_MAX_VIDEO_CHANNEL_NUM);
        return;
    }

    if(streamType == _AST_MAIN_VIDEO_)
    {
        width = m_stuEncoder_size[phy_chn].stuSize[0].width;
        height = m_stuEncoder_size[phy_chn].stuSize[0].height;
    }
    else if(streamType == _AST_SUB_VIDEO_)
    {
        width = m_stuEncoder_size[phy_chn].stuSize[1].width;
        height = m_stuEncoder_size[phy_chn].stuSize[1].height;
    }
    else
    {
        width = m_stuEncoder_size[phy_chn].stuSize[2].width;
        height = m_stuEncoder_size[phy_chn].stuSize[2].height;
    }
}
void CAvDeviceInfo::Adi_set_stream_size(unsigned char phy_chn, av_video_stream_type_t streamType, unsigned int width, unsigned int height)
{
    if(phy_chn >= SUPPORT_MAX_VIDEO_CHANNEL_NUM)
    {
        DEBUG_ERROR("the phy_chn:%d is invalid, the max chn number is:%d \n", phy_chn, SUPPORT_MAX_VIDEO_CHANNEL_NUM);
        return;
    }
    
    if(streamType == _AST_MAIN_VIDEO_)
    {
        m_stuEncoder_size[phy_chn].stuSize[0].width = width;
        m_stuEncoder_size[phy_chn].stuSize[0].height = height;
    }
    else if(streamType == _AST_SUB_VIDEO_)
    {
        m_stuEncoder_size[phy_chn].stuSize[1].width = width;
        m_stuEncoder_size[phy_chn].stuSize[1].height = height;
    }
    else
    {
        m_stuEncoder_size[phy_chn].stuSize[2].width = width;
        m_stuEncoder_size[phy_chn].stuSize[2].height = height;
    }
}
#endif

av_output_type_t CAvDeviceInfo::Adi_get_main_out_mode()
{
	return m_main_out_mode;
}

rm_int32_t CAvDeviceInfo::Adi_set_system(av_tvsystem_t system)
{
	m_tv_system = system;
	return 0;
}
av_tvsystem_t CAvDeviceInfo::Adi_get_system()
{
	return m_tv_system;
}

int CAvDeviceInfo::Adi_ad_type_isahd()
{
    	eProductType product_type = PTD_INVALID;
    	eSystemType subproduct_type = SYSTEM_TYPE_MAX;
    	product_type = pgAvDeviceInfo->Adi_product_type();
    	subproduct_type = pgAvDeviceInfo->Adi_sub_product_type();

    	if(((product_type == PTD_X1_AHD) && ((subproduct_type ==SYSTEM_TYPE_X1_V2_X1H0401_AHD)
        	||(subproduct_type ==SYSTEM_TYPE_X1_V2_X1N0400_AHD)
        	||(subproduct_type ==SYSTEM_TYPE_X1_V2_M1H0401_AHD)
        	||(subproduct_type ==SYSTEM_TYPE_X1_V2_M1SH0401_AHD))))
    	{
		return true;
	}
       else if(((product_type == PTD_D5) && (subproduct_type == SYSTEM_TYPE_D5_AHD))
	   	||((product_type == PTD_X3) && (subproduct_type == SYSTEM_TYPE_X3_AHD))
		||(product_type == PTD_XMD3104)||(product_type == PTD_ES8412)||(product_type == PTD_ES4206)
		||(product_type == PTD_6A_II_AVX)||(product_type == PTD_A5_AHD)||(product_type == PTD_M0026)
		||(product_type == PTD_M3))
       {
		return true;
	}
	else
	{
		return false;
	}
}

#if !defined(_AV_PRODUCT_CLASS_IPC_)
int CAvDeviceInfo::Adi_set_ahd_video_format( int phy_chn_num, unsigned int chn_mask,unsigned char av_video_format)
{

    if (pgAvDeviceInfo->Adi_get_video_source(phy_chn_num) == _VS_DIGITAL_)
    {
        DEBUG_CRITICAL( "CHiVi::HiVi_set_ahd_video_format chn %d is digital!!! \n", phy_chn_num);
        return 0;
    }
	if(phy_chn_num == 0)
	{
    	m_inputFormatValidMask = chn_mask;
	}
//    if(((av_video_format>=_VIDEO_INPUT_AHD_25P_)&&(av_video_format<=_VIDEO_INPUT_AHD_30P_))
//		||(av_video_format==_VIDEO_INPUT_INVALID_))
    {
	    m_videoinputformat[phy_chn_num] = (av_videoinputformat_t)av_video_format;
	    DEBUG_CRITICAL( "CHiVi::HiVi_set_ahd_video_format chn %d, m_videoinputformat[%d]=%d\n",
		   phy_chn_num,phy_chn_num,m_videoinputformat[phy_chn_num]);
           if(m_videoinputformat_bak[phy_chn_num]!=m_videoinputformat[phy_chn_num])
           {
				m_isvideoinputformatchanged=true;
                DEBUG_CRITICAL("m_videoinputformat_bak=%d,m_videoinputformat=%d\n",
				m_videoinputformat_bak[phy_chn_num],m_videoinputformat[phy_chn_num]);
	        	m_videoinputformat_bak[phy_chn_num] = m_videoinputformat[phy_chn_num];
				/*device 检测时有特殊情况 :没有视频丢失情况,摄像头制式变化*/
				printf("00 m_inputFormatValidMask:%x,m_inputFormatValidMask_bak:%x\n",m_inputFormatValidMask,m_inputFormatValidMask_bak);
				if((m_inputFormatValidMask & (0x1 << phy_chn_num)) && (m_inputFormatValidMask_bak & (0x1 << phy_chn_num)))
				{
					GCL_BIT_VAL_CLEAR(m_inputFormatValidMask,phy_chn_num);
				}
				printf("11 m_inputFormatValidMask:%x,m_inputFormatValidMask_bak:%x\n",m_inputFormatValidMask,m_inputFormatValidMask_bak);
           }
           if(m_inputFormatValidMask_bak!=m_inputFormatValidMask)
           {
				m_isvideoinputformatchanged=true;
                    DEBUG_CRITICAL("m_inputFormatValidMask_bak=%x,m_inputFormatValidMask=%x\n",
				m_inputFormatValidMask_bak,m_inputFormatValidMask);
			
				//m_inputFormatValidMask_bak = m_inputFormatValidMask;
	    	}

    }
//    else
    {
#if 0
       #ifdef _AV_PLATFORM_HI3520D_V300_
	    m_videoinputformat[phy_chn_num]=_VIDEO_INPUT_INVALID_;
	    DEBUG_CRITICAL( "INVALID PARAM, we make HiVi_set_ahd_video_format chn %d, m_videoinputformat[%d]=_VIDEO_INPUT_SD_PAL_\n",
			phy_chn_num,phy_chn_num);
	    m_videoinputformat_bak[phy_chn_num]=_VIDEO_INPUT_INVALID_;
	#else
	    m_videoinputformat[phy_chn_num]=_VIDEO_INPUT_SD_PAL_;
	    DEBUG_CRITICAL( "INVALID PARAM, we make HiVi_set_ahd_video_format chn %d, m_videoinputformat[%d]=_VIDEO_INPUT_SD_PAL_\n",
			phy_chn_num,phy_chn_num);
	    m_videoinputformat_bak[phy_chn_num]=_VIDEO_INPUT_SD_PAL_;
	#endif
#endif
    }

    return 0;
}

int CAvDeviceInfo::Adi_get_ahd_video_format(int chn)
{
	return Adi_videoformat_isahd(chn);
}

int CAvDeviceInfo::Adi_is_ahd_video_format_changed()
{
	printf("m_isvideoinputformatchanged=%d\n",m_isvideoinputformatchanged);
	if(m_isvideoinputformatchanged==true)
	{
		m_isvideoinputformatchanged=false;
		return true;
	}
	else
	{
		return false;
	}
}

int CAvDeviceInfo::Adi_videoformat_isahd(int chnno)
{


	 if(Adi_get_camera_insert_mode() == 1)
	 {
	 	switch(m_videoinputformat[chnno])
	 	{
	 		case _VIDEO_INPUT_SD_PAL_:
			case _VIDEO_INPUT_SD_NTSC_:
				return 0;
			case _VIDEO_INPUT_AHD_25P_:		
			case _VIDEO_INPUT_AHD_30P_:	
				return 1;
			case _VIDEO_INPUT_AFHD_25P_:		
			case _VIDEO_INPUT_AFHD_30P_:	
				return 2;
			case _VIDEO_INPUT_AHD_50P_:		
			case _VIDEO_INPUT_AHD_60P_:	
				return 3;
			default:
				return 0;
				break;
	 	}
	 }
	 else
	 {
		 if(((m_videoinputformat[chnno] >= _VIDEO_INPUT_AHD_25P_) && (m_videoinputformat[chnno] <= _VIDEO_INPUT_AHD_60P_)) && (m_inputFormatValidMask & (1LL << chnno)))
		 {
		 	if((m_videoinputformat[chnno] >= _VIDEO_INPUT_AHD_25P_) && (m_videoinputformat[chnno] <= _VIDEO_INPUT_AHD_30P_))
		 	{
				return 1;
		 	}
			else
			{
				return 3;
			}
		 }
		 else if(((m_videoinputformat[chnno] >= _VIDEO_INPUT_AFHD_25P_) && (m_videoinputformat[chnno] <= _VIDEO_INPUT_AFHD_30P_)) && (m_inputFormatValidMask & (1LL << chnno)))
		 {
		 	return 2;
		 }
		 else
		 {
		    if(chnno % 2 == 0)
		    {
				if(((m_videoinputformat[chnno + 1] >= _VIDEO_INPUT_AHD_25P_)&& (m_videoinputformat[chnno + 1] <= _VIDEO_INPUT_AHD_60P_)) && (m_inputFormatValidMask & (1LL << (chnno + 1))))
				{
					if((m_videoinputformat[chnno + 1] >= _VIDEO_INPUT_AHD_25P_) && (m_videoinputformat[chnno + 1] <= _VIDEO_INPUT_AHD_30P_))
				 	{
						return 1;
				 	}
					else
					{
						return 3;
					}
				}
				else if(((m_videoinputformat[chnno + 1] >= _VIDEO_INPUT_AFHD_25P_)&& (m_videoinputformat[chnno] <= _VIDEO_INPUT_AFHD_30P_)) && (m_inputFormatValidMask & (1LL << (chnno + 1))))
				{
					return 2;
				}
				else
				{
					return 0;
				}
		    }
		    else
		    {
				if(((m_videoinputformat[chnno-1] >= _VIDEO_INPUT_AHD_25P_) && (m_videoinputformat[chnno-1] <= _VIDEO_INPUT_AHD_60P_))&&(m_inputFormatValidMask & (1LL<<(chnno-1))))	
				{
					if((m_videoinputformat[chnno - 1] >= _VIDEO_INPUT_AHD_25P_) && (m_videoinputformat[chnno - 1] <= _VIDEO_INPUT_AHD_30P_))
				 	{
						return 1;
				 	}
					else
					{
						return 3;
					}
				}
				else if(((m_videoinputformat[chnno-1] >= _VIDEO_INPUT_AFHD_25P_) && (m_videoinputformat[chnno-1] <= _VIDEO_INPUT_AFHD_30P_))&&(m_inputFormatValidMask & (1LL<<(chnno-1))))
				{
					return 2;
				}
				else
				{
					return 0;
				}
	 	    }
		 }
	 }
}

int CAvDeviceInfo::Adi_get_ahd_chn_mask()
{
#if 0
    int inputFormatValidMask=0;

    DEBUG_CRITICAL("\nkkk,inputFormatValidMask=%d\n",inputFormatValidMask);

    int max_vi_chn_num = pgAvDeviceInfo->Adi_get_vi_chn_num();
    for(int chn_num=0;chn_num<max_vi_chn_num;chn_num++)
    {
        if(_VS_DIGITAL_ == pgAvDeviceInfo->Adi_get_video_source(chn_num))
        {
            continue;
        }
		
       if((m_inputFormatValidMask_bak &(1ll<<chn_num))!=(m_inputFormatValidMask &(1ll<<chn_num)))
       {
	        inputFormatValidMask = ((inputFormatValidMask) | (1ll << (chn_num)));	
       }
#if 0		   
	if((m_videoinputformat_bak[chn_num]==m_videoinputformat[chn_num])
		&&((m_inputFormatValidMask_bak &(1ll<<chn_num))==(m_inputFormatValidMask &(1ll<<chn_num)))
		&&(pMain_stream->stuVideoEncodeSetting[chn_index].ucChnEnable!=m_pMain_stream_encoder->stuVideoEncodeSetting[chn_index].ucChnEnable)

	{
	        m_inputFormatValidMask = ((m_inputFormatValidMask) & (~(1ll << (chn_num))));	
	}
#endif
    }
#endif
	m_ahdinputformatvalidmask = m_inputFormatValidMask_bak^m_inputFormatValidMask;

	DEBUG_CRITICAL("\nppp,m_inputFormatValidMask_bak=%x,m_inputFormatValidMask=%x,m_ahdinputformatvalidmask=%x\n",
		m_inputFormatValidMask_bak,m_inputFormatValidMask,m_ahdinputformatvalidmask);
	
       m_inputFormatValidMask_bak = m_inputFormatValidMask;

	return m_ahdinputformatvalidmask;
}

#endif



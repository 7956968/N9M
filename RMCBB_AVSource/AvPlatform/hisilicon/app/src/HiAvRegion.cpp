#include "HiAvRegion.h"


int Har_get_region_handle_encoder(av_video_stream_type_t stream_type, int phy_video_num, av_osd_name_t osd_name, RGN_HANDLE *pHandle, rm_uint8_t extend_osd_id /* = 0 */)
{
    int max_channel_num = pgAvDeviceInfo->Adi_max_channel_number();

#if defined(_AV_PRODUCT_CLASS_IPC_)
    switch(osd_name)
    {
        case _AON_DATE_TIME_:
        {
            switch(stream_type)
            {
                case _AST_MAIN_VIDEO_:
                    return max_channel_num * 4 + phy_video_num;
                    break;            
                case _AST_SUB_VIDEO_:
                    return max_channel_num * 5 + phy_video_num;
                    break;
                case _AST_SUB2_VIDEO_:
                    return max_channel_num * 6 + phy_video_num;
                    break;
                case _AST_SNAP_VIDEO_:
                    return max_channel_num * 7 + phy_video_num;
                        break;
                default:
                    _AV_FATAL_(1, "Har_get_region_handle_encoder _AON_CHN_NAME_(streamtype = %d) You must give the implement\n", stream_type);
                    break;
            }
        }
            break;
        case _AON_CHN_NAME_:
        {
            switch(stream_type)
            {
                case _AST_MAIN_VIDEO_:
                    return max_channel_num * 8 + phy_video_num;
                    break;            
                case _AST_SUB_VIDEO_:
                    return max_channel_num * 9 + phy_video_num;
                    break;
                case _AST_SUB2_VIDEO_:
                    return max_channel_num * 10 + phy_video_num;
                    break;
                case _AST_SNAP_VIDEO_:
                    return max_channel_num * 11 + phy_video_num;
                        break;
                default:
                    _AV_FATAL_(1, "Har_get_region_handle_encoder _AON_CHN_NAME_(streamtype = %d) You must give the implement\n", stream_type);
                    break;
            }
        }
            break;
        case _AON_BUS_NUMBER_:
        {
            switch(stream_type)
            {
                case _AST_MAIN_VIDEO_:
                    return max_channel_num * 12 + phy_video_num;
                    break;            
                case _AST_SUB_VIDEO_:
                    return max_channel_num * 13 + phy_video_num;
                    break;
                case _AST_SUB2_VIDEO_:
                    return max_channel_num * 14 + phy_video_num;
                    break;
                case _AST_SNAP_VIDEO_:
                    return max_channel_num * 15 + phy_video_num;
                        break;
                default:
                    _AV_FATAL_(1, "Har_get_region_handle_encoder _AON_CHN_NAME_(streamtype = %d) You must give the implement\n", stream_type);
                    break;
            }
        }
            break;
        case _AON_GPS_INFO_:
        {
            switch(stream_type)
            {        
                case _AST_MAIN_VIDEO_:
                    return max_channel_num * 16 + phy_video_num;
                    break;            
                case _AST_SUB_VIDEO_:
                    return max_channel_num * 17 + phy_video_num;
                    break;
                case _AST_SUB2_VIDEO_:
                    return max_channel_num * 18 + phy_video_num;
                    break;
                case _AST_SNAP_VIDEO_:
                    return max_channel_num * 19 + phy_video_num;
                        break;
                default:
                    _AV_FATAL_(1, "Har_get_region_handle_encoder _AON_CHN_NAME_(streamtype = %d) You must give the implement\n", stream_type);
                    break;
            }
        }
            break;
        case _AON_SPEED_INFO_:
        {
            switch(stream_type)
            {
                case _AST_MAIN_VIDEO_:
                    return max_channel_num * 20 + phy_video_num;
                    break;            
                case _AST_SUB_VIDEO_:
                    return max_channel_num * 21 + phy_video_num;
                    break;
                case _AST_SUB2_VIDEO_:
                    return max_channel_num * 22 + phy_video_num;
                    break;
                case _AST_SNAP_VIDEO_:
                    return max_channel_num * 23 + phy_video_num;
                        break;
                default:
                    _AV_FATAL_(1, "Har_get_region_handle_encoder _AON_CHN_NAME_(streamtype = %d) You must give the implement\n", stream_type);
                    break;
            }
        }
            break;
        case _AON_SELFNUMBER_INFO_:
        {
            switch(stream_type)
            {
                case _AST_MAIN_VIDEO_:
                    return max_channel_num * 24 + phy_video_num;
                    break;            
                case _AST_SUB_VIDEO_:
                    return max_channel_num * 25 + phy_video_num;
                    break;
                case _AST_SUB2_VIDEO_:
                    return max_channel_num * 26 + phy_video_num;
                    break;
                case _AST_SNAP_VIDEO_:
                    return max_channel_num * 27 + phy_video_num;
                        break;
                default:
                    _AV_FATAL_(1, "Har_get_region_handle_encoder _AON_CHN_NAME_(streamtype = %d) You must give the implement\n", stream_type);
                    break;
            }
        }
            break;
        case _AON_COMMON_OSD1_:
         {
             switch(stream_type)
             {
                 case _AST_MAIN_VIDEO_:
                     return max_channel_num * 28 + phy_video_num;
                     break;            
                 case _AST_SUB_VIDEO_:
                     return max_channel_num * 29 + phy_video_num;
                     break;
                 case _AST_SUB2_VIDEO_:
                     return max_channel_num * 30 + phy_video_num;
                     break;
                 case _AST_SNAP_VIDEO_:
                     return max_channel_num * 31 + phy_video_num;
                         break;
                 default:
                     _AV_FATAL_(1, "Har_get_region_handle_encoder _AON_CHN_NAME_(streamtype = %d) You must give the implement\n", stream_type);
                     break;
            }
         }
            break;
        case _AON_COMMON_OSD2_:
         {
             switch(stream_type)
             {
                 case _AST_MAIN_VIDEO_:
                     return max_channel_num * 32 + phy_video_num;
                     break;            
                 case _AST_SUB_VIDEO_:
                     return max_channel_num * 33 + phy_video_num;
                     break;
                 case _AST_SUB2_VIDEO_:
                     return max_channel_num * 34 + phy_video_num;
                     break;
                 case _AST_SNAP_VIDEO_:
                     return max_channel_num * 35 + phy_video_num;
                         break;
                 default:
                     _AV_FATAL_(1, "Har_get_region_handle_encoder _AON_CHN_NAME_(streamtype = %d) You must give the implement\n", stream_type);
                     break;
            }
         }
            break;
        default:
        {
        }
            break;
    }
#else
    switch(osd_name)
    {
        case _AON_CHN_NAME_:
            switch(stream_type)
            {
                case _AST_MAIN_VIDEO_:
                    return max_channel_num * 4 + phy_video_num;
                    break;

                case _AST_SUB_VIDEO_:
                    return max_channel_num * 12 + phy_video_num;
                    break;
                case _AST_SUB2_VIDEO_:
                    return max_channel_num * 20 + phy_video_num;
                    break;
                default:
                    _AV_FATAL_(1, "Har_get_region_handle_encoder _AON_CHN_NAME_(streamtype = %d) You must give the implement\n", stream_type);
                    break;
            }
            break;

        case _AON_DATE_TIME_:
            switch(stream_type)
            {
                case _AST_MAIN_VIDEO_:
                    return max_channel_num * 5 + phy_video_num;
                    break;

                case _AST_SUB_VIDEO_:
                    return max_channel_num * 13 + phy_video_num;
                    break;
                    
                case _AST_SUB2_VIDEO_:
                    return max_channel_num * 21 + phy_video_num;
                    break;
    
                default:
                    _AV_FATAL_(1, "Har_get_region_handle_encoder _AON_DATE_TIME_ (streamtype = %d)You must give the implement\n", stream_type);
                    break;
            }
            break;
    case _AON_BUS_NUMBER_:
            switch(stream_type)
            {
                case _AST_MAIN_VIDEO_:
                    return max_channel_num * 6 + phy_video_num;
                    break;

                case _AST_SUB_VIDEO_:
                    return max_channel_num * 14 + phy_video_num;
                    break;
                    
                case _AST_SUB2_VIDEO_:
                    return max_channel_num * 22 + phy_video_num;
                    break;
                    
                default:
                    _AV_FATAL_(1, "Har_get_region_handle_encoder _AON_BUS_NUMBER_(streamtype = %d) You must give the implement\n",stream_type);
                    break;
            }
            break;        
    case _AON_GPS_INFO_:
            switch(stream_type)
            {
                case _AST_MAIN_VIDEO_:
                    return max_channel_num * 7 + phy_video_num;
                    break;

                case _AST_SUB_VIDEO_:
                    return max_channel_num * 15 + phy_video_num;
                    break;
                    
                case _AST_SUB2_VIDEO_:
                    return max_channel_num * 23 + phy_video_num;
                    break;
                    
                default:
                    _AV_FATAL_(1, "Har_get_region_handle_encoder _AON_GPS_INFO_(streamtype = %d) You must give the implement\n",stream_type);
                    break;
            }
            break;
    case _AON_SPEED_INFO_:
            switch(stream_type)
            {
                case _AST_MAIN_VIDEO_:
                    return max_channel_num * 8 + phy_video_num;
                    break;

                case _AST_SUB_VIDEO_:
                    return max_channel_num * 16 + phy_video_num;
                    break;
                    
                case _AST_SUB2_VIDEO_:
                    return max_channel_num * 24 + phy_video_num;
                    break;
                    
                default:
                    _AV_FATAL_(1, "Har_get_region_handle_encoder _AON_SPEED_INFO_(streamtype = %d) You must give the implement\n",stream_type);
                    break;
            }
            break;        
    case _AON_COMMON_OSD1_:
            switch(stream_type)
            {
                case _AST_MAIN_VIDEO_:
                    return max_channel_num * 9 + phy_video_num;
                    break;

                case _AST_SUB_VIDEO_:
                    return max_channel_num * 17 + phy_video_num;
                    break;
                    
                case _AST_SUB2_VIDEO_:
                    return max_channel_num * 25 + phy_video_num;
                    break;
                    
                default:
                    _AV_FATAL_(1, "Har_get_region_handle_encoder _AON_COMMON_OSD1_(streamtype = %d) You must give the implement\n",stream_type);
                    break;
            }
            break;

    case _AON_COMMON_OSD2_:
            switch(stream_type)
            {
                case _AST_MAIN_VIDEO_:
                    return max_channel_num * 10 + phy_video_num;
                    break;

                case _AST_SUB_VIDEO_:
                    return max_channel_num * 18 + phy_video_num;
                    break;
                    
                case _AST_SUB2_VIDEO_:
                    return max_channel_num * 26 + phy_video_num;
                    break;
                    
                default:
                    _AV_FATAL_(1, "Har_get_region_handle_encoder _AON_COMMON_OSD2_(streamtype = %d) You must give the implement\n",stream_type);
                    break;
            }
            break;
    case _AON_SELFNUMBER_INFO_:
            switch(stream_type)
            {
                case _AST_MAIN_VIDEO_:
                    return max_channel_num * 11 + phy_video_num;
                    break;

                case _AST_SUB_VIDEO_:
                    return max_channel_num * 19 + phy_video_num;
                    break;
                    
                case _AST_SUB2_VIDEO_:
                    return max_channel_num * 27 + phy_video_num;
                    break;
                    
                default:
                    _AV_FATAL_(1, "Har_get_region_handle_encoder _AON_COMMON_OSD2_(streamtype = %d) You must give the implement\n",stream_type);
                    break;
            }
            break;	
        case _AON_WATER_MARK:
            switch(stream_type)
            {
                case _AST_MAIN_VIDEO_:
                    return max_channel_num * 28 + phy_video_num;
                    break;

                case _AST_SUB_VIDEO_:
                    return max_channel_num * 29 + phy_video_num;
                    break;
                    
                case _AST_SUB2_VIDEO_:
                    return max_channel_num * 30 + phy_video_num;
                    break;
                    
                default:
                    _AV_FATAL_(1, "Har_get_region_handle_encoder _AON_COMMON_OSD2_(streamtype = %d) You must give the implement\n",stream_type);
                    break;
            }
            break;	
	case _AON_EXTEND_OSD:
			switch(stream_type)
			{
				case _AST_MAIN_VIDEO_:
					return max_channel_num * 50 + phy_video_num * 10 + extend_osd_id;
					break;
	
				case _AST_SUB_VIDEO_:
					return max_channel_num * 70 + phy_video_num * 10 + extend_osd_id;
					break;
					
				case _AST_SUB2_VIDEO_:
					return max_channel_num * 90 + phy_video_num * 10 + extend_osd_id;
					break;
					
				default:
					_AV_FATAL_(1, "Har_get_region_handle_encoder _AON_COMMON_OSD2_(streamtype = %d) You must give the implement\n",stream_type);
					break;
			}
			break;
      
        default:
            _AV_FATAL_(1, "Har_get_region_handle_encoder You must give the implement\n");
            break;
    }
#endif
    return -1;
}

int Har_get_region_handle_vi(int phy_video_num, int area_index, RGN_HANDLE *pHandle)
{
    *pHandle = phy_video_num * 4 + area_index;
    return 0;
}


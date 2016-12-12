/*************************************************************************
    > Copyright (C), 2014, Streamax
  	> Author: XuHeng Niu
    > File Name: bus_lane_enforcement.h
  	> Mail: xhniu@streamax.com 
  	> Created Time: Mon 17 Aug 2015 02:18:35 PM CST
    > Descroption: 
    > Others: 
    > Function List: 
    > History: 
    			1.shenzhen bus lane enforcement
    			2.singapore bus lane enforcement
    			3.chongqing plate snapshot
 ************************************************************************/

#ifndef _IVE_BUS_LANE_ENFORCEMENT_H_
#define _IVE_BUS_LANE_ENFORCEMENT_H_

#include <stdio.h>
#include <stdlib.h>

#include <hi_type.h>
#include <hi_comm_ive.h>
#include <hi_common.h>

#include <unistd.h>
/* the char num max length is 9 bit, the plate num max is 5 */
#define PLATE_CHAR_NUM_MAX 9
#define PLATE_NUM_MAX 5

typedef enum _SNAP_TYPE_EN
{
	EN_NONE                        = 0,
	EN_SNAP                        = 1,
	EN_SZ_BUS_STOP                = 2, 
	EN_SZ_BUS_LANE                = 3,
	EN_EMPTY_BUFFER               = 4,
	EN_SINGAPORE_BUS_STOP         = 5,
	EN_SINGAPORE_YELLOW_BUS_LANE = 6,
	EN_SINGAPORE_PINK_BUS_LANE   = 7
}enSnapType;

typedef enum _PLATE_RECONGNITION_EN
{
	EN_PLATE_NONE   = 0,     //无需任何操作 
	EN_PLATE_UPDATE = 1,     //读取车牌并跟新
	EN_PLATE_EMPTY  = 2      //清空车牌内容
}enPlateRecognitionFlag;

typedef struct _LANE_ENFORCEMENT_INFO_S
{
	/*
	 *  HI_U8 snap_type = 0;               //没有任何操作
	 *	HI_U8 snap_type = 1;               //抓拍一张
	 *	HI_U8 snap_type = 2;               //车道占道(SZ)
	 *	HI_U8 snap_type = 3;               //车站占道(SZ)
	 *  HI_U8 snap_type = 4;               //清空缓存
	 *  HI_U8 snap_type = 5;               //车站占道合成(SINGAPORE)
	 *  HI_U8 snap_type = 6;               //黄色车道占道合成(SINGAPORE)	 
	 *  HI_U8 snap_type = 7;               //粉色车道占道合成(SINGAPORE)	 
	 */
    enSnapType snap_type;                 //
    /* chongqing police plate snapshot */    
    HI_U8        license_plate_num;
    /* 最后一张将车尾含车牌位置区域放大 */
    RECT_S  roi_rect_info;             
    enPlateRecognitionFlag plate_recognition_flag;   

    /* HI_TRUE:识别到车牌,叠加到子码流HI_FALSE:没有识别到车牌 */
    HI_CHAR plate_num[PLATE_CHAR_NUM_MAX * PLATE_NUM_MAX];   //只有一个车牌最多只有8个字符
}LaneEnforcementInfo;
/**************************************
 * Function: BusLaneEnforcementInit
 * Description: 所有的ive_alg模块的配置文件 
 			   训练器文件都放在这个目录,由其他模块定 直接传过来
 * Calls: main
 * Called By: none
 * Input:   file_path
 * Output: none
 * Return: none
 * Author:
 * History: <author> <date>  <desc>
			
 * Others: none
**************************************/
HI_S32 BusLaneEnforcementInit(HI_CHAR *file_path);
/**************************************
 * Function: BusLaneEnforcementStart
 * Description: 运行算法模块
 * Calls: main
 * Called By: none
 * Input:   src_image -> 1080P, resized_image ->540P;
 * Output:  HI_FAILURE/HI_SUCCESS
 * Return: 
 * Author:
 * History: <author> <date>  <desc>
		    xhniu	
 * Others: 
**************************************/
HI_S32 BusLaneEnforcementStart(IVE_IMAGE_S * src_image, 
	IVE_IMAGE_S * resized_image, LaneEnforcementInfo *lane_enforcement_info);
/**************************************
 * Function: BusLaneEnforcementDeInit
 * Description: 释放所有的算法资源
 
 * Calls: 
 * Called By: none
 * Input:   
 * Output: none
 * Return: none
 * Author:
 * History: <author> <date>  <desc>
			
 * Others: none
**************************************/
HI_S32 BusLaneEnforcementDeInit();
/**************************************
 * Function: BusLaneEnforcementParametersReset
 * Description: 如果一旦运行中出现错误,这个时候需要重新配置算法参数
 
 * Calls: 
 * Called By: none
 * Input:   file_path
 * Output: none
 * Return: none
 * Author:
 * History: <author> <date>  <desc>
			
 * Others: none
**************************************/
HI_S32 BusLaneEnforcementParametersReset(HI_CHAR *file_path);
#endif


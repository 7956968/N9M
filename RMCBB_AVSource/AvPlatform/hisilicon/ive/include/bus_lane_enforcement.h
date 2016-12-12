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
	EN_PLATE_NONE   = 0,     //�����κβ��� 
	EN_PLATE_UPDATE = 1,     //��ȡ���Ʋ�����
	EN_PLATE_EMPTY  = 2      //��ճ�������
}enPlateRecognitionFlag;

typedef struct _LANE_ENFORCEMENT_INFO_S
{
	/*
	 *  HI_U8 snap_type = 0;               //û���κβ���
	 *	HI_U8 snap_type = 1;               //ץ��һ��
	 *	HI_U8 snap_type = 2;               //����ռ��(SZ)
	 *	HI_U8 snap_type = 3;               //��վռ��(SZ)
	 *  HI_U8 snap_type = 4;               //��ջ���
	 *  HI_U8 snap_type = 5;               //��վռ���ϳ�(SINGAPORE)
	 *  HI_U8 snap_type = 6;               //��ɫ����ռ���ϳ�(SINGAPORE)	 
	 *  HI_U8 snap_type = 7;               //��ɫ����ռ���ϳ�(SINGAPORE)	 
	 */
    enSnapType snap_type;                 //
    /* chongqing police plate snapshot */    
    HI_U8        license_plate_num;
    /* ���һ�Ž���β������λ������Ŵ� */
    RECT_S  roi_rect_info;             
    enPlateRecognitionFlag plate_recognition_flag;   

    /* HI_TRUE:ʶ�𵽳���,���ӵ�������HI_FALSE:û��ʶ�𵽳��� */
    HI_CHAR plate_num[PLATE_CHAR_NUM_MAX * PLATE_NUM_MAX];   //ֻ��һ���������ֻ��8���ַ�
}LaneEnforcementInfo;
/**************************************
 * Function: BusLaneEnforcementInit
 * Description: ���е�ive_algģ��������ļ� 
 			   ѵ�����ļ����������Ŀ¼,������ģ�鶨 ֱ�Ӵ�����
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
 * Description: �����㷨ģ��
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
 * Description: �ͷ����е��㷨��Դ
 
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
 * Description: ���һ�������г��ִ���,���ʱ����Ҫ���������㷨����
 
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


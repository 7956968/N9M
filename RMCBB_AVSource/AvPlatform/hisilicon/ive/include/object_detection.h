/*************************************************************************
    > Copyright (C), 2015, Streamax
  	> Author: ZhiWei MA
    > File Name: object_detection.h
  	> Mail: zwma@streamax.com 
  	> Created Time: Mon 11 Jan 2016 11:00:00
    > Descroption: object detection interface.
    > Others: 
    > Function List: 
    > History: 
 ************************************************************************/
#ifndef _OBJECT_DETECTION_H_
#define _OBJECT_DETECTION_H_

#include <stdio.h>
#include <stdlib.h>

#include <hi_type.h>
#include <hi_comm_ive.h>
#include <hi_common.h>


/************************************************
 * 功能描述: 加载配置文件，初始化检测参数
 * 输入参数:       
        config_file_path:  配置文件路径. 
 * 输出参数:  
        pIndexID: 返回与输入配置文件相对应的目标检测应用实例标识(可同时运行多个目标检测任务)
 * 返回值:      
        HI_SUCCESS:  成功.
        HI_FAILURE:  失败.
 * 作者:  zwma        2016.03.04
************************************************/
HI_S32 ObjectDetectionInit(const char* config_file_path, int* pIndexID);


/************************************************
 * 功能描述: 对给定灰度图像执行目标检测过程
 * 输入参数:       
        psrc_image:  待检测的单通道灰度图像.
        search_roi: 待检测图像中所指定的搜索区域.
        draw_rect:  指定是否在原图中绘制所检测到的目标，
                    其值为1则进行绘制，否则不绘制.
        indexID:  待执行的目标检测应用实例标识
 * 输出参数:  
        psrc_image:  若draw_rect==1，则改变原图像内容.
        pnum_objs:  记录当前待测图像中所检测到的目标数量(若启动去重,则已跟踪目标不被返回).
        pprect_objs: 算法库内部供外访问的依次记录检测到的num个目标矩形框定位信息, 
                    单个定位信息存放方式为(左上角x坐标, 左上角y坐标, 矩形框width, 矩形框height). 
        ppserialNum: 算法库内部按时空关系标识各个目标的序列号. 
        pphits: 算法库内部返回的各个目标当前已跟踪帧数. 
 * 返回值:      
        HI_SUCCESS:  成功.
        HI_FAILURE:  失败.
 * 作者:  zwma        2016.06.24
************************************************/
HI_S32 ObjectDetectionStart(IVE_IMAGE_S* psrc_image, RECT_S search_roi, int draw_rect, int indexID, 
    int* pnum_objs, RECT_S** pprect_objs, unsigned int** ppserialNum, unsigned int** pphits);


/************************************************
 * 功能描述: 释放目标检测过程加载的资源
 * 输入参数:    无      
 * 输出参数:    无
 * 返回值:      
        HI_SUCCESS:  成功.
        HI_FAILURE:  失败.
 * 作者:  zwma        2016.01.11
************************************************/
HI_S32 ObjectDetectionRelease();


#endif


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
 * ��������: ���������ļ�����ʼ��������
 * �������:       
        config_file_path:  �����ļ�·��. 
 * �������:  
        pIndexID: ���������������ļ����Ӧ��Ŀ����Ӧ��ʵ����ʶ(��ͬʱ���ж��Ŀ��������)
 * ����ֵ:      
        HI_SUCCESS:  �ɹ�.
        HI_FAILURE:  ʧ��.
 * ����:  zwma        2016.03.04
************************************************/
HI_S32 ObjectDetectionInit(const char* config_file_path, int* pIndexID);


/************************************************
 * ��������: �Ը����Ҷ�ͼ��ִ��Ŀ�������
 * �������:       
        psrc_image:  �����ĵ�ͨ���Ҷ�ͼ��.
        search_roi: �����ͼ������ָ������������.
        draw_rect:  ָ���Ƿ���ԭͼ�л�������⵽��Ŀ�꣬
                    ��ֵΪ1����л��ƣ����򲻻���.
        indexID:  ��ִ�е�Ŀ����Ӧ��ʵ����ʶ
 * �������:  
        psrc_image:  ��draw_rect==1����ı�ԭͼ������.
        pnum_objs:  ��¼��ǰ����ͼ��������⵽��Ŀ������(������ȥ��,���Ѹ���Ŀ�겻������).
        pprect_objs: �㷨���ڲ�������ʵ����μ�¼��⵽��num��Ŀ����ο�λ��Ϣ, 
                    ������λ��Ϣ��ŷ�ʽΪ(���Ͻ�x����, ���Ͻ�y����, ���ο�width, ���ο�height). 
        ppserialNum: �㷨���ڲ���ʱ�չ�ϵ��ʶ����Ŀ������к�. 
        pphits: �㷨���ڲ����صĸ���Ŀ�굱ǰ�Ѹ���֡��. 
 * ����ֵ:      
        HI_SUCCESS:  �ɹ�.
        HI_FAILURE:  ʧ��.
 * ����:  zwma        2016.06.24
************************************************/
HI_S32 ObjectDetectionStart(IVE_IMAGE_S* psrc_image, RECT_S search_roi, int draw_rect, int indexID, 
    int* pnum_objs, RECT_S** pprect_objs, unsigned int** ppserialNum, unsigned int** pphits);


/************************************************
 * ��������: �ͷ�Ŀ������̼��ص���Դ
 * �������:    ��      
 * �������:    ��
 * ����ֵ:      
        HI_SUCCESS:  �ɹ�.
        HI_FAILURE:  ʧ��.
 * ����:  zwma        2016.01.11
************************************************/
HI_S32 ObjectDetectionRelease();


#endif


#ifndef _IVE_ALG_H__
#define _IVE_ALG_H__
#include <stdio.h>
#include <stdlib.h>

#include <hi_type.h>
#include <hi_comm_ive.h>
#include <hi_common.h>
/**************************************
 * Function: IveAlgInit
 * Description: ���е�ive_algģ��������ļ� 
 			   ѵ�����ļ����������Ŀ¼,������ģ�鶨 ֱ�Ӵ�����
 * Calls: main
 * Called By: none
 * Input:   file_path
 * Output: none
 * Return: 0:success, other:failure;
 * Others: none
**************************************/
HI_S32 IveAlgInit(HI_CHAR *file_path);
/**************************************
 * Function: IveAlgStart
 * Description: �����㷨ģ��
 * Calls: main
 * Called By: none
 * Input:   pSub_stream ->540P;
 	    draw_line:  1, ��Ҫ����
 	    	 	  other, ����Ҫ����
 * Output:  HI_FAILURE/HI_SUCCESS
 * Return: 0:success, 1:depart, other :failure;
 * Author: 
 * History: <author> <date>  <desc>
 
 * Others: 
**************************************/
HI_S32 IveAlgStart(IVE_IMAGE_S *pSub_stream, int draw_line);
/**************************************
 * Function: IveAlgDeInit
 * Description: �ͷ����е��㷨��Դ
 
 * Calls: 
 * Called By: none
 * Input:   
 * Output: none
 * Return: 0:success, other:failure;
 * Author:
 * History: <author> <date>  <desc>
		   
 * Others: none
**************************************/
HI_S32 IveAlgDeInit();
/**************************************
 * Function: IveAlgParametersReset
 * Description: ���һ�������г��ִ���,
 			   ���ʱ����Ҫ���������㷨����
 
 * Calls: 
 * Called By: none
 * Input:   file_path
 * Output: none
 * Return: 0:success, other: failure;
 * Author:
 * History: <author> <date>  <desc>
			
 * Others: none
**************************************/
HI_S32 IveAlgParametersReset(HI_CHAR *file_path);

#endif

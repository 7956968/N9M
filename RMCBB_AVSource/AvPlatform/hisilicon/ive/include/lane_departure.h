#ifndef _IVE_ALG_H__
#define _IVE_ALG_H__
#include <stdio.h>
#include <stdlib.h>

#include <hi_type.h>
#include <hi_comm_ive.h>
#include <hi_common.h>
/**************************************
 * Function: IveAlgInit
 * Description: 所有的ive_alg模块的配置文件 
 			   训练器文件都放在这个目录,由其他模块定 直接传过来
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
 * Description: 运行算法模块
 * Calls: main
 * Called By: none
 * Input:   pSub_stream ->540P;
 	    draw_line:  1, 需要划线
 	    	 	  other, 不需要划线
 * Output:  HI_FAILURE/HI_SUCCESS
 * Return: 0:success, 1:depart, other :failure;
 * Author: 
 * History: <author> <date>  <desc>
 
 * Others: 
**************************************/
HI_S32 IveAlgStart(IVE_IMAGE_S *pSub_stream, int draw_line);
/**************************************
 * Function: IveAlgDeInit
 * Description: 释放所有的算法资源
 
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
 * Description: 如果一旦运行中出现错误,
 			   这个时候需要重新配置算法参数
 
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

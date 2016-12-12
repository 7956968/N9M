#!/bin/bash

RUN_PATH=$(pwd)

##导入编译环境变量
source AvEnv/AvEnv_yuanyuan

##AV模块编译配置函数，传入2个参数
##param1：编译的机型
##param2：是否make clean
function AVSTREAMING_MODULE_CONFIG()
{	
	AvProductType=$1

	if [ $AvProductType == "1" ];then
		echo "开始编译机型：X5-III"
		protype=X5_HDVR
	elif [ $AvProductType == "2" ];then
		echo "开始编译机型：X7"
		protype=X7_HDVR
	elif [ $AvProductType == "3" ];then
		echo "开始编译机型：T-3535"
		protype=BC_NVR
	elif [ $AvProductType == "4" ];then
		echo "开始编译机型：IPC712_VD [C6]"
		protype=712_VD
	elif [ $AvProductType == "5" ];then
		echo "开始编译机型：X3"
		protype=X3_HDVR
	elif [ $AvProductType == "6" ];then
		echo "开始编译机型：X1"
		protype=X1_HDVR
	elif [ $AvProductType == "7" ];then
		echo "开始编译机型：IPC712_VA [C1 C2 C3]"
		protype=712_VA
	elif [ $AvProductType == "8" ];then
		echo "开始编译机型：IPC712_VB [C4]"
		protype=712_VB
	elif [ $AvProductType == "9" ];then
		echo "开始编译机型：IPC712_VC [C5]"
		protype=712_VC
	elif [ $AvProductType == "10" ];then
		echo "开始编译机型：IPC913_VA"
		protype=913_VA
	elif [ $AvProductType == "11" ];then
		echo "开始编译机型：IPC920_VA"
		protype=920_VA
	elif [ $AvProductType == "12" ];then
		echo "开始编译机型：IPC714_VA"
		protype=714_VA
	elif [ $AvProductType == "13" ];then
		echo "开始编译机型：D5"
		protype=D5
	elif [ $AvProductType == "14" ];then
		echo "开始编译机型：IPC916_VA"
		protype=916_VA
	elif [ $AvProductType == "15" ];then
		echo "开始编译机型：D5M-3.5"
		protype=D5M_3.5
	elif [ $AvProductType == "16" ];then
		echo "开始编译机型：XMD3104"
		protype=XMD3104
	elif [ $AvProductType == "17" ];then
		echo "开始编译机型：D5-3.5"
		protype=D5_3.5
	elif [ $AvProductType == "18" ];then
		echo "开始编译机型：A5_II"
		protype=A5_II
	elif [ $AvProductType == "19" ];then
		echo "开始编译：X1_AHD"
		protype=X1_AHD
	elif [ $AvProductType == "20" ];then
		echo "开始编译：IPC718_VA 【$(pwd)】"
		protype=718_VA
	elif [ $AvProductType == "21" ];then
		echo "开始编译：IPC712_VF 【$(pwd)】"
		protype=712_VF
	elif [ $AvProductType == "22" ];then
		echo "开始编译：ES4206--X16 【$(pwd)】"
		protype=X16
	elif [ $AvProductType == "23" ];then
		echo "开始编译：ES8412--X17 【$(pwd)】"
		protype=X17
	elif [ $AvProductType == "24" ];then
		echo "开始编译：X17-21A 【$(pwd)】"
		protype=X17
	elif [ $AvProductType == "25" ];then
		echo "开始编译：IPC714_VB 【$(pwd)】"
		protype=714_VB
	elif [ $AvProductType == "26" ];then
		echo "开始编译：6AII_AV12 【$(pwd)】"
		protype=6AII_AV12
	elif [ $AvProductType == "27" ];then
		echo "开始编译：6AII_AV3 【$(pwd)】"
		protype=6AII_AV3
	elif [ $AvProductType == "28" ];then
		echo "开始编译：A5_AHD 【$(pwd)】"
		protype=A5_AHD
	elif [ $AvProductType == "29" ];then
		echo "开始编译：P1 【$(pwd)】"
		protype=P1_HDVR	
	elif [ $AvProductType == "30" ];then
		echo "开始编译：913_VB 【$(pwd)】"
		protype=913_VB
	elif [ $AvProductType == "31" ];then
		echo "开始编译：M0026 【$(pwd)】"
		protype=M0026_HDVR
	elif [ $AvProductType == "32" ];then
		echo "开始编译：6AI_AV12 【$(pwd)】"
		protype=6AI_AV12

	else
		echo "Choose error product, and exit!!!"
		exit 1
	fi
	
	##开始编译模块指定机型的模块####
	if [ -n "$2" ] && [ $2 == 1 ];then
		make clean
	fi
	make _PRODUCT_=$protype master 
	echo -e "-----------------------------------完成编译 $dir-----------------------------------\n"
	echo
}

###makeav.sh传入一个参数，标识是否make clean
echo "Usage: ./makeav [clean]"
echo "clean: 1-make clean, others(default)-not make clean"
echo "当前编译目录为：" $(pwd)

###列出当前所有的机型
echo "0.ALL"
echo "1.X5-III ( 3521 )"
echo "2.X7 ( 3531 )"
echo "3.T-3535 ( 3535 )"
echo "4.IPC712_VD [C6] ( 3518 )"
echo "5.X3 ( 3520D )"
echo "6.X1 ( 3515C )"
echo "7.IPC712_VA [C1 C2 C3]( 3518C )"
echo "8.IPC712_VB [C4]( 3518C )"
echo "9.IPC712_VC [C5] ( 3518C )"
echo "10.IPC913_VA ( 3516 )"
echo "11.IPC920_VA ( 3516A )"
echo "12.PTD_714_VA ( 3518C )"
echo "13.D5 ( 3520D )"
echo "14.IPC916_VA ( 3516A )"
echo "15.D5M-3.5 ( 3515C )"
echo "16.XMD3104 ( 3520D -- v300 -- 3521a )"
echo "17.D5-3.5 ( 3521 -- v300 -- 3521a )"
echo "18.A5_II ( 3515 -- hismall -- 3515 )"
echo "19.X1_AHD ( 3515C -- v100 -- 3520D )"
echo "20.IPC718_VA ( 3518e -- v200)"
echo "21.IPC712_VF [C15]( 3518C )"
echo "22.REI--ES4206 ( 3521 -- v300 -- 3521a 原X16)"
echo "23.REI--ES8412 ( 3521 -- v100 -- 3521 原X17)"
echo "24.REI--X17-21A ( 3521A -- v300 -- 3521a 原X17-21A)"
echo "25.PTD_714_VB ( 3518C -- v100 -- 3518 )"
echo "26.6AII_AV12 ( 3521A -- v300)"
echo "27.6AII_AV3 ( 3535 -- v100)"
echo "28.A5_AHD ( 3521A -- v300)"
echo "29.P1 ( 3521A -- v300)"
echo "30.913_VB ( 3518E -- v200)"
echo "31.M0026 ( 3521A -- v300)"
echo "32.6AI_AV12( 3515 -- hismall -- 3515 )"
echo -e "***** 请选择编译的机型 *****"
read MACHINE_TYPE

##编译的所有的机型列表
AVSTREAMING_PRODUCT_LIST=(1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31)


if [ $MACHINE_TYPE == "0" ];then
	echo "开始编译AvStreaming模块的所有机型"
	for((id=0;id<${#AVSTREAMING_PRODUCT_LIST[*]};id))
	do
		##编译指定机型，初始化编译环境，此时必须 make clean##
		AVSTREAMING_MODULE_CONFIG ${AVSTREAMING_PRODUCT_LIST[id]} 1
	done
else
	##开始编译机型为MACHINE_TYPE的项####
	AVSTREAMING_MODULE_CONFIG ${MACHINE_TYPE} $1
fi



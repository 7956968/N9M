#Makefile for avStreaming by xzyang[2012-12-27]


#环境变量
###############################################################
ifndef _AV_CBB_ROOT_DIR_
$(error "You must give the cbb root dir[_AV_CBB_ROOT_DIR_], it is a environment variable")
endif

ifndef _AV_PRODUCT_HEADER_ROOT_DIR_
$(error "You must give product header root dir[_AV_PRODUCT_HEADER_ROOT_DIR_], it is a environment variable")
endif

ifndef _AV_APP_ROOT_DIR_
$(error "You must give product header root dir[_AV_APP_ROOT_DIR_], it is a environment variable")
endif




#包含配置文件
###############################################################
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/config

ifeq (${_PRODUCT_}, X1_HDVR)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/HDVR_3520D
endif

ifeq (${_PRODUCT_}, X1_AHD)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/HDVR_3520D
endif

ifeq (${_PRODUCT_}, X3_HDVR)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/HDVR_3520D
endif

ifeq (${_PRODUCT_}, X5)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/DVR_3521
endif
ifeq (${_PRODUCT_}, X5_HDVR)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/HDVR_3521
endif
ifeq (${_PRODUCT_}, A5_II)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/HDVR_3515
endif
ifeq (${_PRODUCT_}, X7)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/DVR_3531
endif
ifeq (${_PRODUCT_}, X7_HDVR)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/HDVR_3531
endif

ifeq (${_PRODUCT_}, BC_NVR)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/NVR_3535
endif

ifeq (${_PRODUCT_}, D5)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/HDVR_3520D
endif

ifeq (${_PRODUCT_}, D5M_3.5)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/HDVR_3520D
endif

ifeq (${_PRODUCT_}, D5_3.5)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/HDVR_3520D_V300
endif

ifeq (${_PRODUCT_}, XMD3104)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/HDVR_3520D_V300
endif

ifeq (${_PRODUCT_}, 6AI_AV12)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/HDVR_3515
endif

ifeq (${_PRODUCT_}, 712_VD)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/IPC_3518C
endif

ifeq (${_PRODUCT_}, 712_VA)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/IPC_3518C
endif

ifeq (${_PRODUCT_}, 712_VC)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/IPC_3518C
endif

ifeq (${_PRODUCT_}, 712_VB)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/IPC_3518C
endif

ifeq (${_PRODUCT_}, 913_VA)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/IPC_3516C
endif

ifeq (${_PRODUCT_}, 714_VA)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/IPC_3518C
endif

ifeq (${_PRODUCT_}, 920_VA)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/IPC_3516A
endif

ifeq (${_PRODUCT_}, 916_VA)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/IPC_3516A
endif

ifeq (${_PRODUCT_}, 712_VE)
_AV_PRODUCT_CONFIG_PATH_:=./AvConfig/IPC_3518C
endif

include ${_AV_PRODUCT_CONFIG_PATH_}

_PWD_:=$(shell pwd)



#平台
###############################################################
ifeq (${_CONFIG_PLATFORM_HI3535_}, y)
_PLATFORM_CFLAGS_:=-D_AV_PLATFORM_HI3535_MK_
_CROSS_COMPILER_:=arm-hisiv100nptl-linux-
_PLATFORM_NAME_:=HI3535
_CBB_PLATFORM_PREFIX_:=arm-hisiv100
_PLATFORM_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_AVStreaming/RMCBB_PlatformSdk/hi3535
_PLATFORM_HISILICON_:=y
_PLATFORM_HISILICON_PCIV_:=n
_PLATFORM_HISILICON_VPSS_:=y
endif

ifeq (${_CONFIG_PLATFORM_HI3531_}, y)
_PLATFORM_CFLAGS_:=-D_AV_PLATFORM_HI3531_MK_
_CROSS_COMPILER_:=arm-hisiv100nptl-linux-
_PLATFORM_NAME_:=HI3531
_CBB_PLATFORM_PREFIX_:=arm-hisiv100
_PLATFORM_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_AVStreaming/RMCBB_PlatformSdk/hi3531
_PLATFORM_HISILICON_:=y
_PLATFORM_HISILICON_PCIV_:=n
_PLATFORM_HISILICON_VPSS_:=y
endif

ifeq (${_CONFIG_PLATFORM_HI3532_}, y)
_PLATFORM_CFLAGS_:=-D_AV_PLATFORM_HI3532_MK_
_CROSS_COMPILER_:=arm-hisiv100nptl-linux-
_PLATFORM_NAME_:=HI3532
_CBB_PLATFORM_PREFIX_:=arm-hisiv100
_PLATFORM_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_AVStreaming/RMCBB_PlatformSdk/hi3532
_PLATFORM_HISILICON_:=y
_PLATFORM_HISILICON_PCIV_:=y
_PLATFORM_HISILICON_VPSS_:=y
endif

ifeq (${_CONFIG_PLATFORM_HI3521_}, y)
_PLATFORM_CFLAGS_:=-D_AV_PLATFORM_HI3521_MK_
_CROSS_COMPILER_:=arm-hisiv100nptl-linux-
_PLATFORM_NAME_:=HI3521
_CBB_PLATFORM_PREFIX_:=arm-hisiv100
_PLATFORM_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_AVStreaming/RMCBB_PlatformSdk/hi3521
_PLATFORM_HISILICON_:=y
_PLATFORM_HISILICON_PCIV_:=n
_PLATFORM_HISILICON_VPSS_:=y
endif

ifeq (${_CONFIG_PLATFORM_HI3515_}, y)
_PLATFORM_CFLAGS_:=-D_AV_PLATFORM_HI3515_MK_
_CROSS_COMPILER_:=arm-hismall-linux-
_PLATFORM_NAME_:=HI3515
_CBB_PLATFORM_PREFIX_:=arm-hismall
_PLATFORM_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_AVStreaming/RMCBB_PlatformSdk/hi3515
_PLATFORM_HISILICON_:=y
_PLATFORM_HISILICON_PCIV_:=n
_PLATFORM_HISILICON_VPSS_:=n
endif
ifeq (${_CONFIG_PLATFORM_HI3520A_}, y)
_PLATFORM_CFLAGS_:=-D_AV_PLATFORM_HI3520A_MK_
_CROSS_COMPILER_:=arm-hisiv100nptl-linux-
_PLATFORM_NAME_:=HI3520A
_CBB_PLATFORM_PREFIX_:=arm-hisiv100
_PLATFORM_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_AVStreaming/RMCBB_PlatformSdk/hi3520a
_PLATFORM_HISILICON_:=y
_PLATFORM_HISILICON_PCIV_:=n
_PLATFORM_HISILICON_VPSS_:=y
endif

ifeq (${_CONFIG_PLATFORM_HI3518C_}, y)
_PLATFORM_CFLAGS_:=-D_AV_PLATFORM_HI3518C_MK_
_CROSS_COMPILER_:=arm-hisiv100nptl-linux-
_PLATFORM_NAME_:=HI3518C
_CBB_PLATFORM_PREFIX_:=arm-hisiv100
_PLATFORM_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_AVStreaming/RMCBB_PlatformSdk/hi3518c
_PLATFORM_HISILICON_:=y
_PLATFORM_HISILICON_PCIV_:=n
_PLATFORM_HISILICON_VPSS_:=y
endif

ifeq (${_CONFIG_PLATFORM_HI3520D_}, y)
_PLATFORM_CFLAGS_:=-D_AV_PLATFORM_HI3520D_MK_
_CROSS_COMPILER_:=arm-hisiv100nptl-linux-
_PLATFORM_NAME_:=HI3520D
_CBB_PLATFORM_PREFIX_:=arm-hisiv100
_PLATFORM_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_AVStreaming/RMCBB_PlatformSdk/hi3520d
_PLATFORM_HISILICON_:=y
_PLATFORM_HISILICON_PCIV_:=n
_PLATFORM_HISILICON_VPSS_:=y
endif

ifeq (${_CONFIG_PLATFORM_HI3520D_V300_}, y)
_PLATFORM_CFLAGS_+=-D_AV_PLATFORM_HI3520D_V300_MK_
_CROSS_COMPILER_:=arm-hisiv300-linux-uclibcgnueabi-
_PLATFORM_NAME_:=HI3520D_V300
_CBB_PLATFORM_PREFIX_:=arm-hisiv300
_PLATFORM_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_AVStreaming/RMCBB_PlatformSdk/hi3521a
_PLATFORM_HISILICON_:=y
_PLATFORM_HISILICON_PCIV_:=n
_PLATFORM_HISILICON_VPSS_:=y
endif

ifeq (${_CONFIG_PLATFORM_HI3516C_}, y)
_PLATFORM_CFLAGS_:=-D_AV_PLATFORM_HI3516C_MK_
_CROSS_COMPILER_:=arm-hisiv100nptl-linux-
_PLATFORM_NAME_:=HI3516C
_CBB_PLATFORM_PREFIX_:=arm-hisiv100
_PLATFORM_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_AVStreaming/RMCBB_PlatformSdk/hi3518c
_PLATFORM_HISILICON_:=y
_PLATFORM_HISILICON_PCIV_:=n
_PLATFORM_HISILICON_VPSS_:=y
endif

ifeq (${_CONFIG_PLATFORM_HI3516A_}, y)
_PLATFORM_CFLAGS_:=-D_AV_PLATFORM_HI3516A_MK_
_CROSS_COMPILER_:=arm-hisiv300-linux-
_PLATFORM_NAME_:=HI3516A
_CBB_PLATFORM_PREFIX_:=arm-hisiv300
_PLATFORM_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_AVStreaming/RMCBB_PlatformSdk/hi3516a
_PLATFORM_HISILICON_:=y
_PLATFORM_HISILICON_PCIV_:=n
_PLATFORM_HISILICON_VPSS_:=y
endif

###############################################################
ifndef _CROSS_COMPILER_
$(error "You must give the platform define")
endif


#产品分类
###############################################################
ifeq (${_CONFIG_PRODUCT_CLASS_DVR_}, y)
_PRODUCT_CLASS_CFLAGS_:=-D_AV_PRODUCT_CLASS_DVR_MK_
_PRODUCT_NAME_:=DVR
endif

ifeq (${_CONFIG_PRODUCT_CLASS_IPC_}, y)
_PRODUCT_CLASS_CFLAGS_:=-D_AV_PRODUCT_CLASS_IPC_MK_
_PRODUCT_NAME_:=IPC
endif

ifeq (${_CONFIG_PRODUCT_CLASS_NVR_}, y)
_PRODUCT_CLASS_CFLAGS_:=-D_AV_PRODUCT_CLASS_NVR_MK_
_PRODUCT_NAME_:=NVR
endif

ifeq (${_CONFIG_PRODUCT_CLASS_HDVR_}, y)
_PRODUCT_CLASS_CFLAGS_:=-D_AV_PRODUCT_CLASS_HDVR_MK_
_PRODUCT_NAME_:=HDVR
endif

ifeq (${_CONFIG_PRODUCT_CLASS_DECODER_}, y)
_PRODUCT_CLASS_CFLAGS_:=-D_AV_PRODUCT_CLASS_DECODER_MK_
_PRODUCT_NAME_:=DECODER
endif
###############################################################
ifndef _PRODUCT_CLASS_CFLAGS_
$(error "You must give the product class")
endif



#产品相关的头文件及可执行程序具体目录
###############################################################
ifeq (${_PRODUCT_}, X7)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3531
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/X7
endif

ifeq (${_PRODUCT_}, X7_HDVR)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3531
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/X7
endif

ifeq (${_PRODUCT_}, X5)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3521
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/X5-III
endif

ifeq (${_PRODUCT_}, X5_HDVR)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3521
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/X5-III
endif

ifeq (${_PRODUCT_}, A5_II)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3515
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/A5-II
endif

ifeq (${_PRODUCT_}, X3_HDVR)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3520D
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/X3
endif

ifeq (${_PRODUCT_}, XMD3104)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3520D
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/XMD3104
endif

ifeq (${_PRODUCT_}, D5)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3520D
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/D5
endif

ifeq (${_PRODUCT_}, D5M_3.5)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3515C
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/D5M-3.5
endif

ifeq (${_PRODUCT_}, D5_3.5)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3520D
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/D5-3.5
endif

ifeq (${_PRODUCT_}, X1_HDVR)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3515C
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/X1
endif

ifeq (${_PRODUCT_}, X1_AHD)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3515C
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/X1_AHD
endif

ifeq (${_PRODUCT_}, BC_NVR)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3535
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/T-3535
endif


ifeq (${_PRODUCT_}, 6AI_AV12)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3515
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/6AI_AV12
_SUPPORT_AACENC_:=y
_SUPPORT_NO_VIDEO_OUTPUT_AND_DECODER:=y
_SUPPORT_TTS_:=n
endif

ifeq (${_PRODUCT_}, 712_VD)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3518
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/IPC712_VD
endif

ifeq (${_PRODUCT_}, 712_VA)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3518
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/IPC712_VA
endif

ifeq (${_PRODUCT_}, 712_VC)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3518
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/IPC712_VC
endif

ifeq (${_PRODUCT_}, 712_VB)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3518
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/IPC712_VB
endif

ifeq (${_PRODUCT_}, 712_VE)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3518
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/IPC712_VE
endif

ifeq (${_PRODUCT_}, 913_VA)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3516
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/IPC913_VA
endif

ifeq (${_PRODUCT_}, 714_VA)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3518
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/IPC714_VA
endif

ifeq (${_PRODUCT_}, 920_VA)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3516
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/IPC920_VA
endif

ifeq (${_PRODUCT_}, 916_VA)
_PRODUCT_HEADER_FILE_DIR_:=${_AV_PRODUCT_HEADER_ROOT_DIR_}/3516
_PRODUCT_APP_DIR_:=${_AV_APP_ROOT_DIR_}/IPC916_VA
endif

###############################################################

#IPC相关功能支持
###############################################################
ifeq (${_CONFIG_PRODUCT_CLASS_IPC_}, y)
ifdef _SUPPORT_IVE_

ifeq (${_SUPPORT_IVE_}, FD)
_PRODUCT_FUNCTION_CFLAGS_:=-D_AV_PRODUCT_FUNCTION_IVE_MK_
_PRODUCT_FUNCTION_CFLAGS_+=-D_AV_PRODUCT_FUNCTION_IVE_FD_MK_
endif

ifeq (${_SUPPORT_IVE_}, LD)
_PRODUCT_FUNCTION_CFLAGS_:=-D_AV_PRODUCT_FUNCTION_IVE_MK_
_PRODUCT_FUNCTION_CFLAGS_+=-D_AV_PRODUCT_FUNCTION_IVE_LD_MK_
endif

ifeq (${_SUPPORT_IVE_}, BLE)
_PRODUCT_FUNCTION_CFLAGS_:=-D_AV_PRODUCT_FUNCTION_IVE_MK_
_PRODUCT_FUNCTION_CFLAGS_+=-D_AV_PRODUCT_FUNCTION_IVE_BLE_MK_
endif

endif
endif
###############################################################

#TTS 功能支持
ifeq (${_SUPPORT_TTS_}, y)
_PRODUCT_FUNCTION_CFLAGS_+=-D_AV_PRODUCT_FUNCTION_TTS_MK_
endif

ifeq (${_SUPPORT_AACENC_}, y)
_PRODUCT_FUNCTION_CFLAGS_+=-D_AV_AUDIO_USE_AACENC_MK_
endif


#IPC是否带ISP功能
ifeq (${_CONFIG_PRODUCT_CLASS_IPC_}, y)
ifneq (${_PRODUCT_}, 920_VA)
_PRODUCT_FUNCTION_CFLAGS_+=-D_AV_PRODUCT_FUNCTION_ISP_MK_
endif
endif

ifeq (${_SUPPORT_AHD_}, y)
_PRODUCT_FUNCTION_CFLAGS_+=-D_AV_PRODUCT_FUNCTION_AHD_MK_
endif

ifeq (${_SUPPORT_PURE_AUDIO_CHANNEL}, y)
_PRODUCT_FUNCTION_CFLAGS_+=-D_AV_PRODUCT_FUNCTION_PURE_AUDIO_MK_
endif

ifeq (${_SUPPORT_NO_VIDEO_OUTPUT_AND_DECODER}, y)
_PRODUCT_FUNCTION_CFLAGS_+=-D_AV_PRODUCT_FUNCTION_NO_VIDEO_OUTPUT_AND_DECODER
endif

#通用定义
###############################################################
#编译器
CC=${_CROSS_COMPILER_}gcc
CXX=${_CROSS_COMPILER_}g++
STRIP=${_CROSS_COMPILER_}strip
#编译选项
ifeq (${_CONFIG_PLATFORM_HI3515_}, y)
_APP_CFLAGS_:=-Wall  -Wno-deprecated -DBOOST_NO_INTRINSIC_WCHAR_T -fno-omit-frame-pointer -I ${_PLATFORM_DIR_}/include -I ${_AV_PRODUCT_HEADER_ROOT_DIR_} -I${_PRODUCT_HEADER_FILE_DIR_}
else
_APP_CFLAGS_:=-Wall -Werror -Os -DBOOST_NO_INTRINSIC_WCHAR_T -fno-omit-frame-pointer -I ${_PLATFORM_DIR_}/include -I ${_AV_PRODUCT_HEADER_ROOT_DIR_} -I${_PRODUCT_HEADER_FILE_DIR_}
endif

ifeq (${_CONFIG_PLATFORM_HI3516A_}, y)
_APP_CFLAGS_+= -mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4 -mno-unaligned-access -fno-aggressive-loop-optimizations -Wl,-gc-sections
endif

ifeq (${_CORE_DOWN_}, y)
_APP_CFLAGS_+=-D_CORE_DOWN_FILE_
_APP_CFLAGS_+=-g
endif

ifeq (${_CONFIG_PLATFORM_HI3515_}, y)
_APP_CFLAGS_+=-g -o
endif

ifeq (${_CONFIG_PRODUCT_CLASS_HDVR_}, y)
ifeq (${_CONFIG_PLATFORM_HI3515_}, y)
_APP_CFLAGS_+=-ffunction-sections
else
_APP_CFLAGS_+=-ffunction-sections -Wl,-gc-sections
endif
endif

ifeq (${_CONFIG_PLATFORM_HI3518C_}, y)
_APP_CFLAGS_+=-Wl,-gc-sections
endif
#链接选项
_APP_LDFLAGS_:=-lpthread
#发布目录
_RELEASE_DIR_:=${_PWD_}/release/${_PRODUCT_}
#avStreaming master
_AVSTREAMING_MASTER_EXE_:=${_RELEASE_DIR_}/avStreaming
#目标文件路径
_OBJ_DIR_:=${_PWD_}/.obj
#依赖文件
_DEP_FILE_:=${_OBJ_DIR_}/dep
_DEP_FILE_MASTER_:=${_OBJ_DIR_}/dep_master
#脚本路径
_SCRIPT_DIR_:=${_PWD_}/script
###############################################################


#boost library
###############################################################
ifdef _AV_BOOST_ROOT_DIR_
_INCLUDE_LIST_+=-I${_AV_BOOST_ROOT_DIR_}
endif
###############################################################


#av config
###############################################################
_AVCONFIG_DIR_:=${_PWD_}/AvConfig
_INCLUDE_LIST_+=-I ${_AVCONFIG_DIR_}/
###############################################################


#av application
###############################################################
_AVAPP_DIR_:=${_PWD_}/AvApp
_INCLUDE_LIST_+=-I ${_AVAPP_DIR_}/include
_SRC_LIST_+=${_AVAPP_DIR_}/src/AvApp.cpp
_SRC_LIST_MASTER_+=${_AVAPP_DIR_}/src/AvAppMaster.cpp
###############################################################


#av common
###############################################################
_AVCOMMON_DIR_:=${_PWD_}/AvCommon
_INCLUDE_LIST_+=-I ${_AVCOMMON_DIR_}/include
_SRC_LIST_+=${_AVCOMMON_DIR_}/src/AvSignal.cpp
_SRC_LIST_+=${_AVCOMMON_DIR_}/src/AvDebug.cpp
_SRC_LIST_+=${_AVCOMMON_DIR_}/src/AvThreadLock.cpp
_SRC_LIST_+=${_AVCOMMON_DIR_}/src/AvCommonFunc.cpp
_SRC_LIST_+=${_AVCOMMON_DIR_}/src/AvSysPicManager.cpp
_SRC_LIST_+=${_AVCOMMON_DIR_}/src/CmdBuffer.cpp
_SRC_LIST_+=${_AVCOMMON_DIR_}/src/gb2312ToUft8.cpp
###############################################################


#av device
###############################################################
_AVDEVICE_DIR_:=${_PWD_}/AvDevice
_INCLUDE_LIST_+=-I ${_AVDEVICE_DIR_}/include
_SRC_LIST_+=${_AVDEVICE_DIR_}/src/AvDevice.cpp
_SRC_LIST_MASTER_+=${_AVDEVICE_DIR_}/src/AvDeviceMaster.cpp
_SRC_LIST_+=${_AVDEVICE_DIR_}/src/AvDeviceInfo.cpp
ifneq (${_CONFIG_PRODUCT_CLASS_IPC_}, y)
_SRC_LIST_+=${_AVDEVICE_DIR_}/src/AvReportStation.cpp

ifeq (${_SUPPORT_TTS_}, y)
_SRC_LIST_+=${_AVDEVICE_DIR_}/src/AvTTS.cpp
endif

endif

#_SRC_LIST_+=${_AVDEVICE_DIR_}/src/audio_amr_adp.cpp
ifeq (${_SUPPORT_AACENC_}, y)
ifeq (${_CONFIG_PLATFORM_HI3515_}, n)
_SRC_LIST_+=${_AVDEVICE_DIR_}/src/audio_aac_adp.c
endif
endif
######av decoder
_HISILICON_HAVE_VDEC_:=y
ifeq (${_CONFIG_PRODUCT_CLASS_IPC_}, y)
_HISILICON_HAVE_VDEC_:=n
endif

#6AI_AV12不需要解码
ifeq (${_SUPPORT_NO_VIDEO_OUTPUT_AND_DECODER}, y)
_HISILICON_HAVE_VDEC_:=n
endif

ifeq (${_HISILICON_HAVE_VDEC_}, y)
_SRC_LIST_MASTER_+=${_AVDEVICE_DIR_}/src/AvDecoder.cpp
_SRC_LIST_MASTER_+=${_AVDEVICE_DIR_}/src/AvPreviewDecoder.cpp
_SRC_LIST_MASTER_+=${_AVDEVICE_DIR_}/src/AvDecDealSyncPlay.cpp
endif

######vda
_AV_HAVE_VDA_:=n
ifeq (${_CONFIG_PRODUCT_CLASS_NVR_}, y)
_AV_HAVE_VDA_:=n
endif
ifeq (${_CONFIG_PRODUCT_CLASS_DECODER_}, y)
_AV_HAVE_VDA_:=n
endif
ifeq (${_CONFIG_PLATFORM_HI3515_}, y)
_AV_HAVE_VDA_:=n
endif

ifeq (${_AV_HAVE_VDA_}, y)
_SRC_LIST_+=${_AVDEVICE_DIR_}/src/AvVda.cpp
endif

#####remote talk
_AV_REMOTE_TALK_:=n
ifeq (${_CONFIG_PRODUCT_CLASS_NVR_}, y)
_AV_REMOTE_TALK_:=y
endif
ifeq (${_CONFIG_PRODUCT_CLASS_IPC_}, y)
_AV_REMOTE_TALK_:=y
 ifeq (${_SUPPORT_960P_}, y)
  _APP_CFLAGS_+=-D_AV_IPC_SUPPORT_960P_MK_
 endif
endif

ifeq (${_CONFIG_PRODUCT_CLASS_HDVR_}, y)
_AV_REMOTE_TALK_:=y
endif

ifeq (${_CONFIG_PRODUCT_CLASS_DECODER_}, y)
_AV_REMOTE_TALK_:=y
endif

ifeq (${_CONFIG_PRODUCT_CLASS_HDVR_}, y)
_AV_REMOTE_TALK_:=y
endif

ifeq (${_AV_REMOTE_TALK_}, y)
_SRC_LIST_+=${_AVDEVICE_DIR_}/src/AvRemoteTalk.cpp
endif

#####av encoder
_AV_HAVE_ENCODER_:=y
ifeq (${_CONFIG_PRODUCT_CLASS_NVR_}, y)
_AV_HAVE_ENCODER_:=n
endif
ifeq (${_CONFIG_PRODUCT_CLASS_DECODER_}, y)
_AV_HAVE_ENCODER_:=n
endif
ifeq (${_AV_HAVE_ENCODER_}, y)
_SRC_LIST_+=${_AVDEVICE_DIR_}/src/AvEncoder.cpp
_SRC_LIST_+=${_AVDEVICE_DIR_}/src/AvFontLibrary.cpp
_SRC_LIST_+=${_AVDEVICE_DIR_}/src/AvSnapTask.cpp
ifneq (${_CONFIG_PRODUCT_CLASS_IPC_}, y)
_CBB_LANG_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_GUILibrary/RMCBB_Language/Release/Language-${_CBB_PLATFORM_PREFIX_}
_INCLUDE_LIST_+=-I ${_CBB_LANG_DIR_}/include
_DLL_DIR_LIST_+=-L${_CBB_LANG_DIR_}
_DLL_LIST_+=-lLanguage
endif
_CBB_FONT_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_GUILibrary/RMCBB_FontLibShare/Release/FontLibShare-${_CBB_PLATFORM_PREFIX_}
_INCLUDE_LIST_+=-I ${_CBB_FONT_DIR_}/include
_DLL_DIR_LIST_+=-L${_CBB_FONT_DIR_}
_DLL_LIST_+=-lFontLibShare
endif

###############################################################


#av streaimg
###############################################################
_AVSTREAMING_DIR_:=${_PWD_}/AvStreaming
_INCLUDE_LIST_+=-I ${_AVSTREAMING_DIR_}/include
_SRC_LIST_MASTER_+=${_AVSTREAMING_DIR_}/src/AvStreamingMaster.cpp
master:_APP_VERSION_FILE_NAME_:=${_AVSTREAMING_DIR_}/include/AvStreamingMasterVersion.h
###############################################################

_HISILICON_HAVE_VO_:=y
ifeq (${_CONFIG_PRODUCT_CLASS_IPC_}, y)
_HISILICON_HAVE_VO_:=n
endif

#6AI_AV12不需要视频输出
ifeq (${_SUPPORT_NO_VIDEO_OUTPUT_AND_DECODER}, y)
_HISILICON_HAVE_VO_:=n
endif

####是否需要AO####
ifeq (${_CONFIG_PRODUCT_CLASS_IPC_}, y)
_HISILICON_HAVE_AO_:=n
else ifeq (${_SUPPORT_NO_VIDEO_OUTPUT_AND_DECODER}, y)
_HISILICON_HAVE_AO_:=n
else
_HISILICON_HAVE_AO_:=y
endif

#hisilicon
###############################################################
ifeq (${_PLATFORM_HISILICON_}, y)
ifeq (${_CONFIG_PLATFORM_HI3515_}, y)
_AVHISILICONAPP_DIR_:=${_PWD_}/AvPlatform/hi3515/app
_INCLUDE_LIST_+=-I ${_AVHISILICONAPP_DIR_}/include
else
_AVHISILICONAPP_DIR_:=${_PWD_}/AvPlatform/hisilicon/app
_INCLUDE_LIST_+=-I ${_AVHISILICONAPP_DIR_}/include
endif
ifeq (${_CONFIG_PLATFORM_HI3516A_}, y)
ifdef _SUPPORT_IVE_
_AVHISILICONIVE_DIR_:=${_PWD_}/AvPlatform/hisilicon/ive
_INCLUDE_LIST_+=-I ${_AVHISILICONIVE_DIR_}/include
endif
endif
ifeq (${_CONFIG_PLATFORM_HI3515_}, y)
####HiAvDevice
_SRC_LIST_+=${_AVHISILICONAPP_DIR_}/src/HiAvDevice-3515.cpp
_SRC_LIST_MASTER_+=${_AVHISILICONAPP_DIR_}/src/HiAvDeviceMaster-3515.cpp
####HiSystem
_SRC_LIST_+=${_AVHISILICONAPP_DIR_}/src/HiSystem-3515.cpp
####HiAvSysPicManager
_SRC_LIST_+=${_AVHISILICONAPP_DIR_}/src/HiAvSysPicManager-3515.cpp
####HiRegion
_SRC_LIST_+=${_AVHISILICONAPP_DIR_}/src/HiAvRegion-3515.cpp
####VI
_SRC_LIST_+=${_AVHISILICONAPP_DIR_}/src/HiVi-3515.cpp
####VO
ifeq (${_HISILICON_HAVE_VO_}, y)
_SRC_LIST_MASTER_+=${_AVHISILICONAPP_DIR_}/src/HiVo-3515.cpp
endif
####VDEC
ifeq (${_HISILICON_HAVE_VDEC_}, y)
_SRC_LIST_MASTER_+=${_AVHISILICONAPP_DIR_}/src/HiAvDec-3515.cpp
endif
###VENC
ifeq (${_AV_HAVE_ENCODER_}, y)
_SRC_LIST_+=${_AVHISILICONAPP_DIR_}/src/HiAvEncoder-3515.cpp
endif
####AI
_SRC_LIST_+=${_AVHISILICONAPP_DIR_}/src/HiAi-3515.cpp
####AO
_SRC_LIST_MASTER_+=${_AVHISILICONAPP_DIR_}/src/HiAo-3515.cpp
else

####HiAvDevice
_SRC_LIST_+=${_AVHISILICONAPP_DIR_}/src/HiAvDevice.cpp
_SRC_LIST_MASTER_+=${_AVHISILICONAPP_DIR_}/src/HiAvDeviceMaster.cpp
####HiSystem
_SRC_LIST_+=${_AVHISILICONAPP_DIR_}/src/HiSystem.cpp
####HiAvSysPicManager
_SRC_LIST_+=${_AVHISILICONAPP_DIR_}/src/HiAvSysPicManager.cpp
####HiRegion
_SRC_LIST_+=${_AVHISILICONAPP_DIR_}/src/HiAvRegion.cpp
####VI
_HISILICON_HAVE_VI_:=y
ifeq (${_CONFIG_PRODUCT_CLASS_NVR_}, y)
_HISILICON_HAVE_VI_:=n
endif
ifeq (${_CONFIG_PRODUCT_CLASS_DECODER_}, y)
_HISILICON_HAVE_VI_:=n
endif
ifeq (${_HISILICON_HAVE_VI_}, y)
ifeq (${_CONFIG_PLATFORM_HI3516A_}, y)
_SRC_LIST_+=${_AVHISILICONAPP_DIR_}/src/HiViV1.cpp	
else
_SRC_LIST_+=${_AVHISILICONAPP_DIR_}/src/HiVi.cpp
endif
#_SRC_LIST_+=${_AVHISILICONAPP_DIR_}/src/HiAvVda.cpp
endif
####VO

ifeq (${_HISILICON_HAVE_VO_}, y)
_SRC_LIST_MASTER_+=${_AVHISILICONAPP_DIR_}/src/HiVo.cpp
#_SRC_LIST_MASTER_+=${_AVHISILICONAPP_DIR_}/src/HiHdmi.cpp
endif
####VDEC
_HISILICON_HAVE_VDEC_:=y
ifeq (${_CONFIG_PRODUCT_CLASS_IPC_}, y)
_HISILICON_HAVE_VDEC_:=n
endif

#6AI_AV12不需要解码
ifeq (${_SUPPORT_NO_VIDEO_OUTPUT_AND_DECODER}, y)
_HISILICON_HAVE_VDEC_:=n
endif

ifeq (${_HISILICON_HAVE_VDEC_}, y)
_SRC_LIST_MASTER_+=${_AVHISILICONAPP_DIR_}/src/HiAvDec.cpp
endif
###VENC
ifeq (${_AV_HAVE_ENCODER_}, y)
ifeq (${_CONFIG_PLATFORM_HI3516A_}, y)
_SRC_LIST_+=${_AVHISILICONAPP_DIR_}/src/HiAvEncoderV1.cpp
else
_SRC_LIST_+=${_AVHISILICONAPP_DIR_}/src/HiAvEncoder.cpp
endif
endif
####VPSS
ifeq (${_PLATFORM_HISILICON_VPSS_}, y)
_APP_CFLAGS_+=-D_AV_HISILICON_VPSS_MK_
ifeq (${_CONFIG_PLATFORM_HI3516A_}, y)
_SRC_LIST_+=${_AVHISILICONAPP_DIR_}/src/HiVpssV1.cpp
ifdef _SUPPORT_IVE_
_SRC_LIST_+=${_AVHISILICONIVE_DIR_}/src/HiIve.cpp
##_SRC_LIST_+=${_AVHISILICONIVE_DIR_}/src/IveSnap.cpp
endif
else
_SRC_LIST_+=${_AVHISILICONAPP_DIR_}/src/HiVpss.cpp
endif
endif
####AI
_HISILICON_HAVE_AI_:=y
##nvr ai for remote talk
#ifeq (${_CONFIG_PRODUCT_CLASS_NVR_}, y) 
#_HISILICON_HAVE_AI_:=n
#endif
ifeq (${_HISILICON_HAVE_AI_}, y)
_SRC_LIST_+=${_AVHISILICONAPP_DIR_}/src/HiAi.cpp
endif
####AO
ifeq (${_HISILICON_HAVE_AO_}, y)
_SRC_LIST_MASTER_+=${_AVHISILICONAPP_DIR_}/src/HiAo.cpp
_SRC_LIST_MASTER_+=${_AVDEVICE_DIR_}/src/wave_decoder.cpp
endif
#_SRC_LIST_MASTER_+=${_AVHISILICONAPP_DIR_}/src/HiAudioControl.cpp
endif
endif

#海思平台库
###############################################################
ifeq (${_PLATFORM_HISILICON_}, y)
_INCLUDE_LIST_+=-I${_PLATFORM_DIR_}/include
_DLL_DIR_LIST_+=-L${_PLATFORM_DIR_}/lib

_DLL_LIST_+=-lmpi
ifeq (${_CONFIG_PLATFORM_HI3515_}, y)

_DLL_LIST_+=-l_VoiceEngine
_DLL_LIST_+=-l_amr_spc
_DLL_LIST_+=-l_amr_fipop
_DLL_LIST_+=-l_aec
_DLL_LIST_+=-l_aacdec
_DLL_LIST_+=-l_aacenc
_DLL_LIST_+=-lresampler
_DLL_LIST_+=-lloadbmp
endif
ifneq (${_CONFIG_PLATFORM_HI3520D_V300_}, y)
ifneq (${_CONFIG_PLATFORM_HI3515_}, y)
_DLL_LIST_+=-lmem
endif
endif

ifeq (${_CONFIG_PLATFORM_HI3521_}, y)
_DLL_LIST_+=-lvqev2
endif

ifeq (${_CONFIG_PLATFORM_HI3520D_}, y)
_DLL_LIST_+=-lvqev2
endif

ifneq (${_CONFIG_PLATFORM_HI3535_}, y)
ifneq (${_CONFIG_PLATFORM_HI3516A_}, y)
ifneq (${_CONFIG_PLATFORM_HI3520D_V300_}, y)
ifneq (${_CONFIG_PLATFORM_HI3515_}, y)
_DLL_LIST_+=-laec
_DLL_LIST_+=-lanr
endif
endif
endif
endif
ifeq (${_HISILICON_HAVE_VO_}, y)
ifneq (${_CONFIG_PLATFORM_HI3515_}, y)
_DLL_LIST_+=-lhdmi
endif
endif
ifeq (${_CONFIG_PRODUCT_CLASS_IPC_}, y)
#_DLL_LIST_+=-lisp
#_DLL_LIST_+=-lrmss
_DLL_LIST_+=-lrmisp
ifeq (${_CONFIG_PLATFORM_HI3516A_}, y)
_DLL_LIST_+=-l_staewb
endif
ifneq (${_CONFIG_PLATFORM_HI3516A_}, y)
_DLL_LIST_+=-lvqev2
endif
else
ifneq (${_CONFIG_PLATFORM_HI3515_}, y)
_DLL_LIST_+=-ljpeg
endif
endif

ifeq (${_CONFIG_PLATFORM_HI3516A_}, y)
_DLL_LIST_+=-lupvqe
_DLL_LIST_+=-ldnvqe
_DLL_LIST_+=-l_iniparser
_DLL_LIST_+=-l_cmoscfg
_DLL_LIST_+=-lhive_AEC
_DLL_LIST_+=-lhive_HPF
_DLL_LIST_+=-lhive_ANR
ifdef _SUPPORT_IVE_
ifeq (${_SUPPORT_IVE_}, FD)
_DLL_LIST_+=-lface_detection
endif

ifeq (${_SUPPORT_IVE_}, LD)
_DLL_LIST_+=-live_alg
endif

ifeq (${_SUPPORT_IVE_}, BLE)
_DLL_LIST_+=-lbus_lane_enforcement
endif

endif
endif

_DLL_LIST_+=-lfreetype


ifeq (${_SUPPORT_AACENC_}, y)
ifeq (${_CONFIG_PLATFORM_HI3515_}, n)
_DLL_LIST_+=-laacenc
_DLL_LIST_+=-laacdec
endif
endif

ifneq (${_CONFIG_PLATFORM_HI3516A_}, y)
ifneq (${_CONFIG_PLATFORM_HI3520D_V300_}, y)
_DLL_LIST_+=-lresampler
endif
endif
#报站用mp3解码库
ifneq (${_CONFIG_PRODUCT_CLASS_IPC_}, y)
_DLL_LIST_+=-lmp3dec
endif
_DLL_LIST_+=-ltde
ifneq (${_CONFIG_PLATFORM_HI3515_}, y)
_DLL_LIST_+=-lVoiceEngine
endif

ifeq (${_PLATFORM_HISILICON_PCIV_}, y)
_DLL_LIST_+=-lpciv
endif

ifeq (${_CONFIG_PLATFORM_HI3535_}, y)
_DLL_LIST_+=-lvqe
endif

ifeq (${_CONFIG_PLATFORM_HI3520D_V300_}, y)
_DLL_LIST_+=-ldnvqe
_DLL_LIST_+=-lupvqe
endif

endif
###############################################################


#各种公共库文件
###############################################################
#系统配置
_CBB_SYSTEM_CONFIG_:=${_AV_CBB_ROOT_DIR_}/RMCBB_SysConfig/Release/SysConfig-${_CBB_PLATFORM_PREFIX_}
_INCLUDE_LIST_+=-I${_CBB_SYSTEM_CONFIG_}/include
master:_DLL_DIR_LIST_+=-L${_CBB_SYSTEM_CONFIG_}
master:_DLL_LIST_+=-lSysConfig

#UTF8转码库
ifneq (${_CONFIG_PRODUCT_CLASS_IPC_}, y)
ifeq (${_CROSS_COMPILER_},arm-hisiv300-linux-uclibcgnueabi-)
_CBB_GUI_UTF8_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_GUILibrary/RMCBB_GbkUtf8Ucs2Conv/release/GbkUtf8Ucs2Conv-arm-hisiv300
else
ifeq (${_CROSS_COMPILER_},arm-hismall-linux-)
_CBB_GUI_UTF8_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_GUILibrary/RMCBB_GbkUtf8Ucs2Conv/release/GbkUtf8Ucs2Conv-arm-hismall
else
_CBB_GUI_UTF8_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_GUILibrary/RMCBB_GbkUtf8Ucs2Conv/release/GbkUtf8Ucs2Conv-arm-hisiv100
endif
endif
_INCLUDE_LIST_+=-I${_CBB_GUI_UTF8_DIR_}/include
_DLL_DIR_LIST_+=-L${_CBB_GUI_UTF8_DIR_}
_DLL_LIST_+=-lGbkUtf8Ucs2Conv
endif
#文件系统库
ifneq (${_CONFIG_PRODUCT_CLASS_IPC_}, y)
ifeq (${_CROSS_COMPILER_},arm-hisiv300-linux-uclibcgnueabi-)
_CBB_REMOTE_FILE_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_Filesystem/RMCBB_RemoteOperateFile/rmfs4.0-arm-hisiv300
else
ifeq (${_CROSS_COMPILER_},arm-hismall-linux-)
_CBB_REMOTE_FILE_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_Filesystem/RMCBB_RemoteOperateFile/rmfs4.0-arm-hismall
else
_CBB_REMOTE_FILE_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_Filesystem/RMCBB_RemoteOperateFile/rmfs4.0-arm-hisiv100
endif
endif
_INCLUDE_LIST_+=-I${_CBB_REMOTE_FILE_DIR_}/include
_DLL_DIR_LIST_+=-L${_CBB_REMOTE_FILE_DIR_}
_DLL_LIST_+=-lremote_operate_file
endif

#COMMON库
_CBB_COMMON_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_Common/Release/Common-${_CBB_PLATFORM_PREFIX_}

ifeq (${_CROSS_COMPILER_},arm-hisiv300-linux-uclibcgnueabi-)
_INCLUDE_LIST_+=-I${_AV_CBB_ROOT_DIR_}/RMCBB_GUILibrary/RMCBB_freetype/Release/freetype-arm-hisiv300/include
_DLL_DIR_LIST_+=-L${_AV_CBB_ROOT_DIR_}/RMCBB_GUILibrary/RMCBB_freetype/Release/freetype-arm-hisiv300
else
_INCLUDE_LIST_+=-I/opt/hisi-linux-nptl/arm-hisiv100-linux/target/usr/include/freetype2
#_INCLUDE_LIST_+=-I${_AV_CBB_ROOT_DIR_}/RMCBB_GUILibrary/RMCBB_freetype/Release/freetype-arm-hisimall/include
#_DLL_DIR_LIST_+=-L${_AV_CBB_ROOT_DIR_}/RMCBB_GUILibrary/RMCBB_freetype/Release/freetype-arm-hisimall

endif
_INCLUDE_LIST_+=-I${_CBB_COMMON_DIR_}/include
_INCLUDE_LIST_+=-I${_CBB_COMMON_DIR_}/include/general
_INCLUDE_LIST_+=-I${_CBB_COMMON_DIR_}/include/message
_INCLUDE_LIST_+=-I${_CBB_COMMON_DIR_}/include/systemtime
_INCLUDE_LIST_+=-I${_CBB_COMMON_DIR_}/include/streambuffer
_DLL_DIR_LIST_+=-L${_CBB_COMMON_DIR_}
_DLL_LIST_+=-lCommon

#Security uuid库
_CBB_SECURITY_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_Security/Release/Security-${_CBB_PLATFORM_PREFIX_}

_INCLUDE_LIST_+=-I${_CBB_SECURITY_DIR_}/include
_INCLUDE_LIST_+=-I${_CBB_SECURITY_DIR_}/include/base64
_INCLUDE_LIST_+=-I${_CBB_SECURITY_DIR_}/include/des2
_INCLUDE_LIST_+=-I${_CBB_SECURITY_DIR_}/include/md5
_INCLUDE_LIST_+=-I${_CBB_SECURITY_DIR_}/include/uuid
_DLL_DIR_LIST_+=-L${_CBB_SECURITY_DIR_}
_DLL_LIST_+=-lSecurity
ifeq (${_HISILICON_HAVE_VDEC_}, y)
_APP_CFLAGS_+=-D_PLATFORM_LINX_MK_
#Mssp库
_CBB_MSSP_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_Storage/RMCBB_MultiStreamSyncParser/Release/MultiStreamSyncParser-${_CBB_PLATFORM_PREFIX_}
_INCLUDE_LIST_+=-I${_CBB_MSSP_DIR_}/include
_INCLUDE_LIST_+=-I${_AV_CBB_ROOT_DIR_}/RMCBB_Storage/RMCBB_StoragePlug/Release/StoragePlug-${_CBB_PLATFORM_PREFIX_}/include
master:_DLL_DIR_LIST_+=-L${_CBB_MSSP_DIR_}
master:_DLL_LIST_+=-lmssp

#rmfs库
_CBB_RMFS_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_Storage/RMCBB_StoragePlug/Release/StoragePlug-${_CBB_PLATFORM_PREFIX_}
_INCLUDE_LIST_+=-I${_CBB_RMFS_DIR_}/include
_CBB_RMFS_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_Public
_INCLUDE_LIST_+=-I${_CBB_RMFS_DIR_}/inc
endif

#TTS库
ifneq (${_CONFIG_PRODUCT_CLASS_IPC_}, y)

ifeq (${_SUPPORT_TTS_}, y)
_CBB_TTS_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_AVStreaming/TTS/Release/TTS-${_CBB_PLATFORM_PREFIX_}
_INCLUDE_LIST_+=-I${_CBB_TTS_DIR_}/include
_DLL_DIR_LIST_+=-L${_CBB_TTS_DIR_}/lib
_DLL_LIST_+=-lcurl
_DLL_LIST_+=-lhci_sys
_DLL_LIST_+=-lhci_tts
_DLL_LIST_+=-lhci_tts_local_synth
endif

endif
###VENC
ifeq (${_AV_HAVE_ENCODER_}, y)
master:_DLL_DIR_LIST_+=-L${_PRODUCT_APP_DIR_}/lib
#master:_DLL_LIST_+=-lSnapManage
endif

###用于NVR,IPC的对讲
ifeq (${_AV_REMOTE_TALK_}, y)
_APP_CFLAGS_+=-D_AV_REMOTE_TALK_MK_
endif

#多片级联
ifeq (${_CONFIG_MULTICHIP_CASCADE_}, y)

_APP_CFLAGS_+=-D_AV_MULTICHIP_CASCADE_MK_

####MCC and PCIV
ifeq (${_PLATFORM_HISILICON_PCIV_}, y)
_SRC_LIST_+=${_AVHISILICONAPP_DIR_}/src/HiAvMcc.cpp
_SRC_LIST_+=${_AVHISILICONAPP_DIR_}/src/HiPciv.cpp
#####多片级联通讯库
_HI35XX_MULTICHIPCASCADE_CBB_DIR_:=${_AV_CBB_ROOT_DIR_}/RMCBB_Hi35xxMultiChipCascade/Release/Hi35xxMultiChipCascade-${_CBB_PLATFORM_PREFIX_}
_INCLUDE_LIST_+=-I${_HI35XX_MULTICHIPCASCADE_CBB_DIR_}/include
_DLL_DIR_LIST_+=-L${_HI35XX_MULTICHIPCASCADE_CBB_DIR_}
_DLL_LIST_+=-lHi35xxMultiChipCascade
endif

endif
###############################################################

#老化软件
ifeq (${_CONFIG_TEST_MAINBOARD_}, y)
_APP_CFLAGS_+=-D_AV_TEST_MAINBOARD_MK_
endif

###############################################################
.PHONY:help master app_version release_app_master release_app_slave error_judge


help:
	@echo "#####################################"
	@echo "[HELP INFO]"
	@echo "	make [_CORE_DOWN_=y] _PRODUCT_=3521 master"
	@echo "	make clean:"
	@echo "#####################################"


error_judge:
ifndef _PRODUCT_APP_DIR_
	@echo "	Invalid or unknown product name[_PRODUCT_]"
	@exit 1
else
	@echo -n ""
endif

master:error_judge app_version ${_AVSTREAMING_MASTER_EXE_} release_app_master
slave:error_judge app_version ${_AVSTREAMING_SLAVE_EXE_} release_app_slave
clean:
	@-rm -rf ${_PWD_}/release
	@-rm -rf ${_OBJ_DIR_}




#生成版本头文件
master:_APP_VERSION_MODULE_NAME_:=$(shell basename ${_AVSTREAMING_MASTER_EXE_})
master:_APP_VERSION_COMPILER_FLAG:=__AVSTREAMINGMASTERVERSION_H__
app_version:
	@${_SCRIPT_DIR_}/auto_version.sh ${_APP_VERSION_FILE_NAME_} ${_APP_VERSION_MODULE_NAME_} ${_PLATFORM_NAME_} ${_APP_VERSION_COMPILER_FLAG}




#发布可执行程序
release_app_master:
	@if [ -d ${_PRODUCT_APP_DIR_} -a -f ${_AVSTREAMING_MASTER_EXE_} ];then cp -v ${_AVSTREAMING_MASTER_EXE_} ${_PRODUCT_APP_DIR_}/bin/;fi

#公共依赖依赖文件
ifdef _SRC_LIST_
_OBJECT_LIST_+=$(addprefix $(_OBJ_DIR_)/, $(addsuffix .o, $(basename $(notdir $(_SRC_LIST_)))))

${_DEP_FILE_}:	
	@-mkdir -p ${_OBJ_DIR_}
	@-rm -rf ${_DEP_FILE_}
	@for f in ${_SRC_LIST_}; do \
	OBJ=${_OBJ_DIR_}/`basename $$f | sed -e 's/\.cpp/\.o/' -e 's/\.c/\.o/'`; \
          echo $$OBJ:$$f >> ${_DEP_FILE_}; \
          echo '	${CXX} -g ${_INCLUDE_LIST_} ${_PRODUCT_CLASS_CFLAGS_} ${_PRODUCT_FUNCTION_CFLAGS_} ${_PLATFORM_CFLAGS_} ${_APP_CFLAGS_} -c -o $$@ $$^' >> ${_DEP_FILE_}; \
        done

-include ${_DEP_FILE_}
endif




#主片依赖文件
ifdef _SRC_LIST_MASTER_
_OBJECT_LIST_MASTER_+=$(addprefix $(_OBJ_DIR_)/, $(addsuffix .o, $(basename $(notdir $(_SRC_LIST_MASTER_)))))

${_DEP_FILE_MASTER_}:	
	@-mkdir -p ${_OBJ_DIR_}
	@-rm -rf ${_DEP_FILE_MASTER_}
	@for f in ${_SRC_LIST_MASTER_}; do \
	OBJ=${_OBJ_DIR_}/`basename $$f | sed -e 's/\.cpp/\.o/' -e 's/\.c/\.o/'`; \
          echo $$OBJ:$$f >> ${_DEP_FILE_MASTER_}; \
          echo '	${CXX} -g ${_INCLUDE_LIST_} ${_PRODUCT_CLASS_CFLAGS_} ${_PRODUCT_FUNCTION_CFLAGS_} ${_PLATFORM_CFLAGS_} ${_APP_CFLAGS_} -c -o $$@ $$^' >> ${_DEP_FILE_MASTER_}; \
        done

-include ${_DEP_FILE_MASTER_}
endif

#生成主片可执行程序
ifdef _AVSTREAMING_MASTER_EXE_
${_AVSTREAMING_MASTER_EXE_}:${_OBJECT_LIST_} ${_OBJECT_LIST_MASTER_}
	@-mkdir -p ${_RELEASE_DIR_}
	${CXX} -g ${_INCLUDE_LIST_} ${_APP_LDFLAGS_} ${_PRODUCT_CLASS_CFLAGS_} ${_PRODUCT_FUNCTION_CFLAGS_} ${_PLATFORM_CFLAGS_} ${_APP_CFLAGS_} -o ${_AVSTREAMING_MASTER_EXE_} ${_OBJECT_LIST_} ${_OBJECT_LIST_MASTER_} ${_DLL_DIR_LIST_} ${_DLL_LIST_}
	$(STRIP) ${_AVSTREAMING_MASTER_EXE_}
	@echo "SUCCESS!!!!!!!!!!!!!!!!!"
endif

###############################################################


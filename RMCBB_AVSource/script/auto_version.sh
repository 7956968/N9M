#!/bin/bash

set -e

if [ $# -ne 4 ];then
	echo "usage auto_version.sh ver_file_name ver_module ver_platform compile_flag"
	exit 1
fi

ver_file_name=$1
ver_module=$2
ver_platform=$3
compile_flag=$4


echo "/********************************************************" > ${ver_file_name}
echo "	Copyright (C), 2007-2099,STREAMAXTECH Tech. Co., Ltd." >> ${ver_file_name}
echo "	Author:" >> ${ver_file_name}
echo "	Date:" >> ${ver_file_name}
echo "	Description:" >> ${ver_file_name}
echo "********************************************************/" >> ${ver_file_name}
echo "#ifndef ${compile_flag}" >> ${ver_file_name}
echo "#define ${compile_flag}" >> ${ver_file_name}
echo "" >> ${ver_file_name}
echo "#define _${ver_module}_VERSION_STR_	\"${ver_module} Version[${ver_platform}]:$(date +%Y)-$(date +%m)-$(date +%d) $(date +%H):$(date +%M):$(date +%S)\"" >> ${ver_file_name}
echo "" >> ${ver_file_name}
echo "#endif/*${compile_flag}*/" >> ${ver_file_name}
echo "" >> ${ver_file_name}

exit 0

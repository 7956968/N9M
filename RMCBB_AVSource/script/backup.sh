#!/bin/sh
#backup project script by xzyang

cd ..
DOCUMENT_NAME=$(basename $(pwd))

set $(who am i)
USER_NAME=$1


set $(date)
YEAR=$6
#MONTH=$(echo $2 | gawk '{if(length($1) == 4) print "0"substr($1, 1, 1);if(length($1) == 5) printf substr($1, 1, 2)}')
#echo $MONTH

case $2 in
	Jan | 1*)
		MONTH="01"
		;;
	Feb | 2*)
		MONTH="02"
		;;
	Mar | 3*)
		MONTH="03"
		;;
	Apr | 4*)
		MONTH="04"
		;;
	May | 5*)
		MONTH="05"
		;;
	Jun | 6*)
		MONTH="06"
		;;
	Jul | 7*)
		MONTH="07"
		;;
	Aug | 8*)
		MONTH="08"
		;;
	Sep | 9*)
		MONTH="09"
		;;
	Oct | 10*)
		MONTH="10"
		;;
	Nov | 11*)
		MONTH="11"
		;;
	Dec | 12*)
		MONTH="12"
		;;
	*)	
		echo "Invalid month"
		exit 1
		;;
esac

DAY=$(echo $3 | gawk '{if(length($1) == 1) print "0"$1;if(length($1) == 2) printf $1}')
HOUR=$(echo $4 | gawk '{print substr($1,1,2)}')
MINUTE=$(echo $4 | gawk '{print substr($1,4,2)}')
SECOND=$(echo $4 | gawk '{print substr($1,7,2)}')


DATE_TIME_STRING=${YEAR}${MONTH}${DAY}_${HOUR}${MINUTE}${SECOND}
TAR_NAME=${USER_NAME}_${DOCUMENT_NAME}_${DATE_TIME_STRING}.tar.gz
 

echo -e -n "backup $DOCUMENT_NAME..."
CUR_DIR=$(pwd)
cd ${CUR_DIR}/../
if tar -zcvf ${TAR_NAME} ./${DOCUMENT_NAME} ;then
	echo "($TAR_NAME) ok!"
	cd ${CUR_DIR}
else
	echo "tar $TAR_NAME failed!"
	cd ${CUR_DIR}
	exit 1
fi

echo "backup over!"







#!/bin/sh
product="6AI_AV12"
#product=$1
source ./AvEnv/AvEnv_yuanyuan
make clean; 
make _PRODUCT_=$product master


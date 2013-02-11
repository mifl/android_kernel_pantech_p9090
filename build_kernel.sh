#!/bin/bash
###############################################################################
#
#                           Kernel Build Script 
#
###############################################################################
# 2012-01-15 quixotism    : modified
# 2010-12-29 allydrop     : created
###############################################################################
##############################################################################
# set toolchain
##############################################################################
#export ARCH=arm
#export CROSS_COMPILE=$PWD/../../prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi- 

##############################################################################
# make zImage
##############################################################################
export PANTECH_ANDROID_FLAGS+="-DT_MAGNUS -I$PWD/include/pantech -include$PWD/include/pantech/CUST_PANTECH.h -DFIRM_VER=\"KAUSS28\" -DSYS_MODEL_NAME=\"MAGNUS\" -DPANTECH_MODEL_NAME=\"PantechP9090\" -DSYS_PROJECT_NAME=\"MAGNUS\"	-DFS_USER_DATA_VER=7 -DPANTECH_STORAGE_INTERNAL_EMUL"
mkdir -p ./obj/KERNEL_OBJ/
make O=./obj/KERNEL_OBJ/ msm8960_magnus_tp20_defconfig
make -j4 O=./obj/KERNEL_OBJ/

##############################################################################
# Copy Kernel Image
##############################################################################
cp -f ./obj/KERNEL_OBJ/arch/arm/boot/zImage .


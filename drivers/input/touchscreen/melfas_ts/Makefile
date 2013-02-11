#
# Makefile for the MSM8960 melfas MMS-144 touch drivers.
#

obj-$(CONFIG_TOUCHSCREEN_MELFAS_TS) += melfas_mms.o
melfas_mms-objs += melfas_ts.o
melfas_mms-objs += mms100_ISC_download.o
melfas_mms-objs += mms100_ISP_download.o
melfas_mms-objs += mms100_download_porting.o

#obj-m := melfas_mms.o
#melfas_mms-objs += melfas_ts.o
#melfas_mms-objs += mms100_ISC_download.o
#melfas_mms-objs += mms100_ISP_download.o
#melfas_mms-objs += mms100_download_porting.o


KDIR := ../../../../../out/target/product/msm8960/obj/KERNEL_OBJ/
PWD := $(shell pwd)

EXTRA_CFLAGS += -include ../../../../../pantech/include/BOARD_REV.h

all: 
	$(MAKE) -C $(KDIR) M=$(PWD) modules	

clean:
	rm -f -r *.o *.ko .*cmd *.mod.c .tmp* core *.i modules.order Module.symvers modules.builtin

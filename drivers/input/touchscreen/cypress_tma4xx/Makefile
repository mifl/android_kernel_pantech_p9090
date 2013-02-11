#
# Makefile for the MSM8960 CYPRESS TMA400 touch drivers.
#

# obj-$(CONFIG_TOUCHSCREEN_CYTTSP_GEN4) += cyttsp4_core.o cyttsp4_i2c.o

obj-m := cypress_tma4xx.o
cypress_tma4xx-objs += cyttsp4_core.o
cypress_tma4xx-objs += cyttsp4_i2c.o

#cypress_tma4xx-objs += issp_driver_routines.o
#cypress_tma4xx-objs += issp_routines.o

KDIR := ../../../../../out/target/product/msm8960/obj/KERNEL_OBJ/
PWD := $(shell pwd)

EXTRA_CFLAGS += -include ../../../../../pantech/include/BOARD_REV.h

all: 
	$(MAKE) -C $(KDIR) M=$(PWD) modules	

clean:
	rm -f -r *.o *.ko .*cmd *.mod.c .tmp* core *.i modules.order Module.symvers modules.builtin

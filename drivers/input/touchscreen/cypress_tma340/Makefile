#
# Makefile for the MSM8960 CYPRESS TMA340 touch drivers.
#

# obj-$(CONFIG_TOUCHSCREEN_CYTTSP_I2C) += cyttsp-i2c.o

obj-m := cypress_tma340.o
cypress_tma340-objs += cyttsp-i2c.o

#cypress_tma340-objs += issp_main.o
#cypress_tma340-objs += issp_driver_routines.o
#cypress_tma340-objs += issp_routines.o

KDIR := ../../../../../out/target/product/msm8960/obj/KERNEL_OBJ/
PWD := $(shell pwd)

EXTRA_CFLAGS += -include ../../../../../pantech/include/BOARD_REV.h

all: 
	$(MAKE) -C $(KDIR) M=$(PWD) modules	

clean:
	rm -f -r *.o *.ko .*cmd *.mod.c .tmp* core *.i modules.order Module.symvers modules.builtin

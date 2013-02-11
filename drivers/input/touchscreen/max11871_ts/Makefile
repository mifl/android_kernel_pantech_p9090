#
# Makefile for the MAX11871 drivers.
#


obj-$(CONFIG_TOUCHSCREEN_MAX11871) += max11871.o
#obj-m := max11871_module.o
#max11871_module-objs := max11871.o

KDIR := ../../../../../out/target/product/msm8960/obj/KERNEL_OBJ/
PWD := $(shell pwd)

#EXTRA_CFLAGS += -include ../../../../../../../BOARD_REV.h

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	rm -f -r *.o *.ko .*cmd *.mod.c .tmp* core *.i modules.order Module.symvers modules.builtin

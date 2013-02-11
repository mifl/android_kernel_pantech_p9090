
# Makefile for wlan sdio if driver

ifndef CONFIG_WIFI_CONTROL_FUNC
librasdioif-objs += libra_sdioif.o ../wcnss/qcomwlan_secif.o 
else
librasdioif-objs += libra_sdioif.o
endif

ifdef CONFIG_ARCH_MSM8X60
ifndef CONFIG_WIFI_CONTROL_FUNC
 	librasdioif-objs += qcomwlan_pwrif.o
endif	
endif

ifdef CONFIG_ARCH_MSM7X27A
	librasdioif-objs += qcomwlan7x27a_pwrif.o
endif

obj-$(CONFIG_LIBRA_SDIOIF) += librasdioif.o

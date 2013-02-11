/*
 * Copyright (C) 2011 S7760A DRIVER
 */
 
#ifndef _FELICA_H
#define _FELICA_H
#ifdef CONFIG_CXD2235AGC_NFC_FELICA
struct felica_platform_data {
	int gpio_pon;
	int gpio_rfs;
	int gpio_int;
	int gpio_intu;
	int gpio_hsel;
	int gpio_temp;
};
#endif
#endif

/*
 * Copyright (C) 2011 S7760A DRIVER
 */

/*
 * PN544 power control via ioctl
 * PN544_SET_PWR(0): power off
 * PN544_SET_PWR(1): power on
 * PN544_SET_PWR(2): reset and power on with firmware download enabled
 */

struct s7760a_i2c_platform_data {
	unsigned int wp_gpio;
	
};

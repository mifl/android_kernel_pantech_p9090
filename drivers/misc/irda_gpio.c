/* misc/felica/felica_uart.c
 *
 * Copyright (C) 2012 Pantech.
 *
 * Author: Cho HyunDon <cho.hyundon@pantech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/stddef.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/compat.h>
#include <linux/io.h>
#include <asm/ioctls.h>

#define PRT_NAME "irda_gpio"
#define DRV_VERSION "0.1"



#define GPIO_IRDA_HIGH       1
#define GPIO_IRDA_LOW        0

#define GPIO_IRDA_EN    7

#define MSM_GSBI4_PHYS		0x16300000

#if (BOARD_VER<WS10 && defined(CONFIG_MACH_MSM8960_SIRIUSLTE))

#define GPIO_IRDA_TX    18
#define GPIO_IRDA_RX    19
#define UART_DM_IRDA_OFFSET  0x38
#define GSBI_UART_DM_BASE   0x16340000	//GSBI4     
#define GSBI_MODE_IRDA       0x03
#else
#define GPIO_IRDA_TX    22
#define GPIO_IRDA_RX    23
#define UART_DM_IRDA_OFFSET  0x38
#define GSBI_UART_DM_BASE   0x16440000	//GSBI5
#define GSBI_MODE_IRDA       0x03

#endif
static	unsigned char __iomem *mapped_gsbi;

/**
 * @brief   Open file operation of irda gpio controller
 * @param   inode : (unused)
 * @param   file  : (unused)
 * @retval  0     : Success
 * @note
 */
static int irda_gpio_open(struct inode *inode, struct file *file)
{
	int ret;
	pr_err(PRT_NAME ": %s\n", __func__);

	ret = gpio_request(GPIO_IRDA_TX, "IRDA_TX");
	if (ret) {
		pr_err(PRT_NAME ": Error. IRDA TX GPIO request failed.\n");
		ret = -ENODEV;
		goto err_request_gpio_tx;
	}

	ret = gpio_request(GPIO_IRDA_RX, "IRDA_RX");
	if (ret) {
		pr_err(PRT_NAME ": Error. IRDA RX GPIO request failed.\n");
		ret = -ENODEV;
		goto err_request_gpio_rx;
	}

	gpio_set_value(GPIO_IRDA_EN, GPIO_IRDA_LOW);

	if ((ioread32(mapped_gsbi + UART_DM_IRDA_OFFSET) &
		GSBI_MODE_IRDA) != GSBI_MODE_IRDA)
		iowrite32(GSBI_MODE_IRDA, mapped_gsbi + UART_DM_IRDA_OFFSET);

	return 0;

err_request_gpio_tx:
	gpio_free(GPIO_IRDA_TX);
err_request_gpio_rx:
	return ret;
	
}

/**
 * @brief   Close file operation of irda gpio controller
 * @param   inode : (unused)
 * @param   file  : (unused)
 * @retval  0     : Success
 * @note
 */
static int irda_gpio_release(struct inode *inode, struct file *file)
{
	pr_err(PRT_NAME ": %s\n", __func__);

	gpio_free(GPIO_IRDA_TX);
	gpio_free(GPIO_IRDA_RX);

	gpio_set_value(GPIO_IRDA_EN, GPIO_IRDA_HIGH);
	return 0;
}


/***************** 	IRDA FOPS	****************************/
static const struct file_operations irda_gpio_fops = {
	.owner			= THIS_MODULE,
	.open			= irda_gpio_open,
	.release		= irda_gpio_release,
};

static struct miscdevice irda_gpio_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "irda_gpio",
	.fops = &irda_gpio_fops,
};

/**
 * @brief   Exit function of irda gpio driver
 * @details This function executes;\n
 *            # platform driver unregistration of irda gpio driver
 * @param   N/A
 * @retval  N/A
 * @note
 */

static int __init irda_gpio_init(void)
{
	int ret =0;

	pr_info(PRT_NAME ": irda gpio driver ver %s being loaded.\n",DRV_VERSION);

	/* Request PON GPIO */
	ret = gpio_request(GPIO_IRDA_EN, "IRDA_EN");
	if (ret) {
		pr_err(PRT_NAME ": Error. IRDA EN GPIO request failed.\n");
		ret = -ENODEV;
		goto err_request_gpio;
	}
	ret = gpio_direction_output(GPIO_IRDA_EN,GPIO_IRDA_HIGH);
	if (ret) {
		pr_err(PRT_NAME ": Error. TEMP GPIO direction failed.\n");
		ret = -ENODEV;
		goto err_direction_gpio;
	}

	mapped_gsbi = ioremap(GSBI_UART_DM_BASE, 0x100);
	if (!mapped_gsbi) {
		pr_err(PRT_NAME ":Error. Cannot ioremap\n");
		return -EBUSY;
	}
	
	iowrite32(GSBI_MODE_IRDA, mapped_gsbi + UART_DM_IRDA_OFFSET);
			
	/* Create irda_gpio character device (/dev/irda_gpio) */
	if (misc_register(&irda_gpio_miscdev)) {
		pr_err(PRT_NAME ": Error. Cannot register SNFC.\n");
		ret = -ENOMEM;
		goto err_misc;
	}
	return ret;
	
err_misc:
	iounmap(mapped_gsbi);
err_direction_gpio:
	gpio_free(GPIO_IRDA_EN);
err_request_gpio:
	return ret;
}

/**
 * @brief   Exit function of irda gpio driver
 * @details This function executes;\n
 *            # platform driver unregistration of irda gpio driver
 * @param   N/A
 * @retval  N/A
 * @note
 */
static void __exit irda_gpio_exit(void)
{
	pr_info(PRT_NAME ": %s\n", __func__);

	gpio_free(GPIO_IRDA_EN);
	gpio_free(GPIO_IRDA_TX);
	gpio_free(GPIO_IRDA_RX);
	
	iounmap(mapped_gsbi);
	misc_deregister(&irda_gpio_miscdev);
}

module_init(irda_gpio_init);
module_exit(irda_gpio_exit);

MODULE_VERSION(DRV_VERSION);
MODULE_AUTHOR("PANTECH");
MODULE_DESCRIPTION("IrDA driver");
MODULE_LICENSE("GPL");

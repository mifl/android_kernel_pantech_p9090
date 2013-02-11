/* vendor/semc/hardware/felica/felica_pon.c
 *
 * Copyright (C) 2010 Sony Ericsson Mobile Communications AB.
 *
 * Author: Hiroaki Kuriyama <Hiroaki.Kuriyama@sonyericsson.com>
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
#include <linux/err.h>
#include <linux/miscdevice.h>
#include <linux/mfd/pmic8058.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/felica.h>
#include "felica_master.h"

#define PRT_NAME "felica pon"

/**
 * @brief   Open file operation of FeliCa PON controller
 * @param   inode : (unused)
 * @param   file  : (unused)
 * @retval  0     : Success
 * @note
 */
static int felica_pon_open(struct inode *inode, struct file *file)
{
	pr_debug(PRT_NAME ": %s\n", __func__);

	return 0;
}

/**
 * @brief   Close file operation of FeliCa PON controller
 * @details This module is responsible for the following roles:\n
 *            # Forcedly, write Low to PON GPIO\n
 *            # Forcedly, turn off TVDD
 * @param   inode    : (unused)
 * @param   file     : (unused)
 * @retval  0        : Success
 * @note
 */
static int felica_pon_release(struct inode *inode, struct file *file)
{
	struct felica_platform_data *pfdata= felica_drv->pdev->dev.platform_data;
	pr_debug(PRT_NAME ": %s\n", __func__);

	/* Forcedly, write Low to PON GPIO */
	gpio_set_value(pfdata->gpio_pon, FELICA_NFC_LOW);

	return 0;
}

/**
 * @brief   Write file operation of FeliCa PON controller
 * @details This function executes;\n
 *            # Copy value from user space\n
 *            # [When writing High,]\n
 *            #   | Write High to PON GPIO\n
 *            #   | [Params meet the condition,] turn on TVDD.\n
 *            # [When writing Low,]\n
 *            #   | Write Low to PON GPIO\n
 *            #   | Forcedly, turn off TVDD.\n
 * @param   file     : (unused)
 * @param   buf      : Source of the written data
 * @param   count    : Data length must be 1 Byte.
 * @param   offset   : (unused)
 * @retval  1        : Success
 * @retval  Negative : Failure\n
 *            -EINVAL = Invalid argument\n
 *            -EFAULT = Cannot copy data from user space\n
 *            -EIO    = Cannot control VREG
 * @note
 */
static ssize_t felica_pon_write(struct file *file, const char __user *buf,
					size_t count, loff_t *offset)
{
	char kbuf;
	struct felica_platform_data *pfdata= felica_drv->pdev->dev.platform_data;


	if (1 != count  || !buf) {
		pr_err(PRT_NAME ": Error. Invalid arg @PON write.\n");
		return -EINVAL;
	}

	/* Copy value from user space */
	if (copy_from_user(&kbuf, buf, 1)) {
		pr_err(PRT_NAME ": Error. copy_from_user failure.\n");
		return -EFAULT;
	}

	if (FELICA_NFC_HIGH == kbuf) {
		/* Write High to PON GPIO */
		gpio_set_value(pfdata->gpio_pon, FELICA_NFC_HIGH);
	} else if (FELICA_NFC_LOW == kbuf) {
		/* Write LOW to PON GPIO */
		gpio_set_value(pfdata->gpio_pon, FELICA_NFC_LOW);
	} else {
		pr_err(PRT_NAME ": Error. Invalid val @PON write.\n");
		return -EINVAL;
	}

	/* 1 byte write */
	return 1;
}

/***************** PON FOPS ****************************/
static const struct file_operations felica_pon_fops = {
	.owner		= THIS_MODULE,
	.write		= felica_pon_write,
	.open		= felica_pon_open,
	.release	= felica_pon_release,
};

static struct miscdevice felica_pon_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "felica_pon",
	.fops = &felica_pon_fops,
};

static struct miscdevice snfc_pon_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "snfc_pon",
	.fops = &felica_pon_fops,
};

/**
 * @brief  Initilialize FeliCa PON controller
 * @details This function executes;\n
 *            # Check PON platform data\n
 *            # Alloc and init PON controller's data\n
 *            # Request PON GPIO\n
 *            # Disable TVDD vreg device\n
 *            # Create PON character device (/dev/felica_pon)
 * @param   pfdata   : Pointer to PON platform data
 * @retval  0        : Success
 * @retval  Negative : Initialization failed.\n
 *            -EINVAL = No platform data\n
 *            -ENODEV = Requesting GPIO failed / Getting TVDD dev failed\n
 *            -ENOMEM = No enough memory / Cannot create char dev
 *            -EIO    = Cannot control VREG
 * @note
 */
int felica_pon_probe_func(struct felica_device *pdevice)
{
	int ret;
	struct felica_platform_data *pfdata= pdevice->pdev->dev.platform_data;
	
	pr_debug(PRT_NAME ": %s\n", __func__);

	/* Check PON platform data */
	if (!pfdata) {
		pr_err(PRT_NAME ": Error. No platform data for PON.\n");
		return -EINVAL;
	}

	/* Request PON GPIO */
	ret = gpio_request(pfdata->gpio_pon, "felica_pon");
	if (ret) {
		pr_err(PRT_NAME ": Error. PON GPIO request failed.\n");
		ret = -ENODEV;
		goto err_request_pon_gpio;
	}
	ret = gpio_direction_output(pfdata->gpio_pon, FELICA_NFC_LOW);
	if (ret) {
		pr_err(PRT_NAME ": Error. PON GPIO direction failed.\n");
		ret = -ENODEV;
		goto err_direction_pon_gpio;
	}

	/* Create PON character device (/dev/felica_pon) */
	if (misc_register(&felica_pon_device)) {
		pr_err(PRT_NAME ": Error. Cannot register PON.\n");
		ret = -ENOMEM;
		goto err_create_pon_dev;
	}
	/* Create PON character device (/dev/snfc_pon) */
	if (misc_register(&snfc_pon_device)) {
		pr_err(PRT_NAME ": Error. Cannot register PON.\n");
		ret = -ENOMEM;
		goto err_create_pon_dev;
	}

	return 0;

err_create_pon_dev:
err_direction_pon_gpio:
	gpio_free(pfdata->gpio_pon);
err_request_pon_gpio:
	return ret;
}

/**
 * @brief   Terminate FeliCa PON controller
 * @details This function executes;\n
 *            # Deregister PON character device (/dev/felica_pon)\n
 *            # Release PON GPIO resource\n
 *            # Release PON controller's data
 * @param   N/A
 * @retval  N/A
 * @note
 */

void felica_pon_remove_func(void)
{
	struct felica_platform_data *pfdata= felica_drv->pdev->dev.platform_data;

	pr_debug(PRT_NAME ": %s\n", __func__);

	misc_deregister(&felica_pon_device);
	gpio_free(pfdata->gpio_pon);
}

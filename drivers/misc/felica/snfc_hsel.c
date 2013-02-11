/* vendor/semc/hardware/felica/felica_int.c
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
#include <linux/mfd/pmic8058.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/workqueue.h>
#include <linux/miscdevice.h>
#include <linux/felica.h>
#include "felica_master.h"

#define PRT_NAME "snfc_hsel"
#define WRITE_LEN 1

static struct felica_sysfs_dev snfc_hsel_dev;

static ssize_t snfc_hsel_value_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n",atomic_read(&felica_drv->hsel_value));
}

static ssize_t snfc_hsel_value_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	int value;
	struct felica_platform_data *pfdata= felica_drv->pdev->dev.platform_data;

	value = simple_strtoul(buf, NULL, 10);
	
	mutex_lock(&felica_drv->felica_uart_mutex);
	atomic_set(&felica_drv->hsel_value, value);
	mutex_unlock(&felica_drv->felica_uart_mutex);

	
	gpio_set_value(pfdata->gpio_hsel, value);
	return size;
}

static struct device_attribute snfc_hsel_sysfs_entries[] = 
{
	{
		.attr.name = "snfc_hsel_value",
		.attr.mode = S_IRUSR|S_IRGRP|S_IWUSR|S_IWGRP,
		.show = snfc_hsel_value_show,
		.store = snfc_hsel_value_store,
	}
};

static int snfc_hsel_sysfs_create(void)
{
	snfc_hsel_dev.attr = snfc_hsel_sysfs_entries;
	snfc_hsel_dev.attr_size = sizeof(snfc_hsel_sysfs_entries)/sizeof(struct device_attribute);
	snfc_hsel_dev.name = "snfc_hsel";
	snfc_hsel_dev.minor = 0;

	return felica_sysfs_register(&snfc_hsel_dev);
}

static void snfc_hsel_sysfs_remove(void)
{
	felica_sysfs_unregister(&snfc_hsel_dev);
}

/**
 * @brief   Open file operation of Snfc HSEL controller
 * @param   inode : (unused)
 * @param   file  : (unused)
 * @retval  0     : Success
 * @note
 */
static int snfc_hsel_open(struct inode *inode, struct file *file)
{
	pr_debug(PRT_NAME ": %s\n", __func__);

	return 0;
}

/**
 * @brief   Close file operation of Snfc HSEL controller
 * @details This module is responsible for the following roles:\n
 *            # Forcedly, write Low to PON GPIO\n
 *            # Forcedly, turn off TVDD
 * @param   inode    : (unused)
 * @param   file     : (unused)
 * @retval  0        : Success
 * @note
 */
static int snfc_hsel_release(struct inode *inode, struct file *file)
{
	struct felica_platform_data *pfdata= felica_drv->pdev->dev.platform_data;
	pr_debug(PRT_NAME ": %s\n", __func__);

	/* Forcedly, write Low to HSEL GPIO */
	gpio_set_value(pfdata->gpio_hsel, FELICA_NFC_LOW);

	return 0;
}

/**
 * @brief   Write file operation of Snfc HSEL controller
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
static ssize_t snfc_hsel_write(struct file *file, const char __user *buf,
					size_t count, loff_t *offset)
{
	char kbuf;
	struct felica_platform_data *pfdata= felica_drv->pdev->dev.platform_data;

	pr_debug(PRT_NAME ": %s\n", __func__);

	if (1 != count  || !buf) {
		pr_err(PRT_NAME ": Error. Invalid arg @hsel write.\n");
		return -EINVAL;
	}

	/* Copy value from user space */
	if (copy_from_user(&kbuf, buf, 1)) {
		pr_err(PRT_NAME ": Error. copy_from_user failure.\n");
		return -EFAULT;
	}
	
	wait_event_interruptible(felica_drv->felica_uart_close_queue ,
							((atomic_read(&felica_drv->felica_uart_cnt) == 0)
							|| (kbuf == FELICA_NFC_LOW)));
	mutex_lock(&felica_drv->felica_uart_mutex);
	if (FELICA_NFC_HIGH == kbuf) {
		/* Write High to HSEL GPIO */
		gpio_set_value(pfdata->gpio_hsel, FELICA_NFC_HIGH);
		atomic_set(&felica_drv->hsel_value,FELICA_NFC_HIGH);
	} else if (FELICA_NFC_LOW == kbuf) {
		/* Write LOW to HSEL GPIO */
		gpio_set_value(pfdata->gpio_hsel, FELICA_NFC_LOW);
		atomic_set(&felica_drv->hsel_value,FELICA_NFC_LOW);
		wake_up_interruptible(&felica_drv->felica_uart_open_queue);
	}
	mutex_unlock(&felica_drv->felica_uart_mutex);

	/* 1 byte write */
	return 1;
}
/**
 * @brief   Read file operation of Snfc HSEL controller
 * @details This function executes;\n
 *            # Copy value from user space\n
 *            # [When reading High,]\n
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
static ssize_t snfc_hsel_read(struct file *file, char __user *buf,
					size_t count, loff_t *offset)
{
	int ret;
	unsigned char kbuf;
	//unsigned long flags;
	struct felica_platform_data *pfdata= felica_drv->pdev->dev.platform_data;
		
	pr_debug(PRT_NAME ": %s\n", __func__);
	
	if (WRITE_LEN != count || !buf) {
		pr_err(PRT_NAME ": Error. Invalid arg @CEN read.\n");
		return -EINVAL;
	}
		
	kbuf =gpio_get_value(pfdata->gpio_hsel);
	
	/* Copy PM_NFC_EN value to user space */
	ret = copy_to_user(buf, &kbuf, 1);
	if (ret) {
		pr_err(PRT_NAME ": Error. copy_to_user failure.\n");
		return -EFAULT;
	}

	/* 1 byte read */
	return 1;
}
#if 0

static unsigned int snfc_hsel_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	pr_debug(PRT_NAME ": %s\n", __func__);

	poll_wait(file, &felica_drv->snfc_uart_close_queue, wait);
	if (felica_drv->hsel_status == CEN_STATUS_WRITE)
		mask |= (POLLOUT | POLLWRNORM);

	felica_drv->hsel_status = CEN_STATUS_NONE;
	return mask;
}
#endif	

/***************** HSEL FOPS ****************************/
static const struct file_operations snfc_hsel_fops = {
	.owner		= THIS_MODULE,
	.write		= snfc_hsel_write,
	.read		= snfc_hsel_read,
//	.poll		= snfc_hsel_poll,
	.open		= snfc_hsel_open,
	.release	= snfc_hsel_release,
};

static struct miscdevice snfc_hsel_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "snfc_hsel",
	.fops = &snfc_hsel_fops,
};

/**
 * @brief  Initilialize Snfc HSEL controller
 * @details This function executes;\n
 *            # Check HSEL platform data\n
 *            # Alloc and init HSEL controller's data\n
 *            # Request PON GPIO\n
 *            # Disable TVDD vreg device\n
 *            # Create PON character device (/dev/snfc_hsel)
 * @param   pfdata   : Pointer to PON platform data
 * @retval  0        : Success
 * @retval  Negative : Initialization failed.\n
 *            -EINVAL = No platform data\n
 *            -ENODEV = Requesting GPIO failed / Getting TVDD dev failed\n
 *            -ENOMEM = No enough memory / Cannot create char dev
 *            -EIO    = Cannot control VREG
 * @note
 */
int snfc_hsel_probe_func(struct felica_device *pdevice)
{
	int ret;
	struct felica_platform_data *pfdata= pdevice->pdev->dev.platform_data;
	
	pr_debug(PRT_NAME ": %s\n", __func__);

	/* Check PON platform data */
	if (!pfdata) {
		pr_err(PRT_NAME ": Error. No platform data for PON.\n");
		return -EINVAL;
	}

	spin_lock_init(&pdevice->snfc_hsel_lock);
	
	/* Request PON GPIO */
	ret = gpio_request(pfdata->gpio_hsel, "snfc_hsel");
	if (ret) {
		pr_err(PRT_NAME ": Error. PON GPIO request failed.\n");
		ret = -ENODEV;
		goto err_request_hsel_gpio;
	}
	ret = gpio_direction_output(pfdata->gpio_hsel, FELICA_NFC_LOW); // temp
	//ret = gpio_direction_output(pfdata->gpio_hsel, FELICA_NFC_HIGH);
	atomic_set(&pdevice->hsel_value, FELICA_NFC_LOW);
	//pdevice->hsel_value = FELICA_NFC_HIGH;
	if (ret) {
		pr_err(PRT_NAME ": Error. HSEL GPIO direction failed.\n");
		ret = -ENODEV;
		goto err_direction_hsel_gpio;
	}

	ret = snfc_hsel_sysfs_create();
	if (ret) {
		pr_err(PRT_NAME ": Error. Cannot SNFC HSEL SYS.\n");
		ret = -ENODEV;
		goto err_hsel_sysfs;
	}

	/* Create HSEL character device (/dev/snfc_hsel) */
	if (misc_register(&snfc_hsel_device)) {
		pr_err(PRT_NAME ": Error. Cannot register PON.\n");
		ret = -ENOMEM;
		goto err_create_hsel_dev;
	}

	return 0;

err_create_hsel_dev:
err_hsel_sysfs:
err_direction_hsel_gpio:
	gpio_free(pfdata->gpio_hsel);
err_request_hsel_gpio:
	return ret;
}

/**
 * @brief   Terminate Snfc HSEL controller
 * @details This function executes;\n
 *            # Deregister HSEL character device (/dev/snfc_hsel)\n
 *            # Release HSEL GPIO resource\n
 *            # Release HSEL controller's data
 * @param   N/A
 * @retval  N/A
 * @note
 */

void snfc_hsel_remove_func(void)
{
	struct felica_platform_data *pfdata= felica_drv->pdev->dev.platform_data;

	pr_debug(PRT_NAME ": %s\n", __func__);

	misc_deregister(&snfc_hsel_device);
	gpio_free(pfdata->gpio_hsel);
	snfc_hsel_sysfs_remove();
}


/* 
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
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/irq.h>
#include <linux/felica.h>
#include "felica_master.h"

#define PRT_NAME "felica rfs"

static struct felica_sysfs_dev felica_rfs_dev;

static ssize_t felica_rfs_value_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n",atomic_read(&felica_drv->rfs_value));
}

static ssize_t felica_rfs_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n",(felica_drv->rfs_status));
}

static struct device_attribute felica_rfs_sysfs_entries[] = 
{
	{
		.attr.name = "felica_rfs_value",
		.attr.mode = S_IRUSR|S_IRGRP|S_IWUSR|S_IWGRP,
		.show = felica_rfs_value_show,
	},
	{
		.attr.name = "felica_rfs_status",
		.attr.mode = S_IRUSR|S_IRGRP|S_IWUSR|S_IWGRP,
		.show = felica_rfs_status_show,
	}
};

static int felica_rfs_sysfs_create(void)
{
	felica_rfs_dev.attr = felica_rfs_sysfs_entries;
	felica_rfs_dev.attr_size = sizeof(felica_rfs_sysfs_entries)/sizeof(struct device_attribute);
	felica_rfs_dev.name = "felica_rfs";
	felica_rfs_dev.minor = 0;

	return felica_sysfs_register(&felica_rfs_dev);
}

static void felica_rfs_sysfs_remove(void)
{
	felica_sysfs_unregister(&felica_rfs_dev);
}

/**
 * @brief   Open file operation of FeliCa RFS controller
 * @param   inode : (unused)
 * @param   file  : (unused)
 * @retval  0     : Success
 * @note
 */
static int felica_rfs_open(struct inode *inode, struct file *file)
{
	pr_debug(PRT_NAME ": %s\n", __func__);

	return 0;
}

/**
 * @brief   Close file operation of FeliCa RFS controller
 * @param   inode : (unused)
 * @param   file  : (unused)
 * @retval  0     : Success
 * @note
 */
static int felica_rfs_release(struct inode *inode, struct file *file)
{
	pr_debug(PRT_NAME ": %s\n", __func__);

	return 0;
}

/**
 * @brief   Read file operation of FeliCa RFS controller
 * @details This function executes;\n
 *            # Read RFS GPIO value\n
 *            # Copy the value to user space
 * @param   inode    : (unused)
 * @param   file     : (unused)
 * @retval  1        : Success
 * @retval  Negative : Failure\n
 *            -EINVAL = Invalid argument\n
 *            -EIO    = GPIO read error\n
 *            -EFAULT = Cannot copy data to user space
 * @note
 */
static ssize_t felica_rfs_read(struct file *file, char __user *buf,
					size_t count, loff_t *offset)
{
	char kbuf;
	int gpio_val;
	unsigned long flags;
	struct felica_platform_data *pfdata= felica_drv->pdev->dev.platform_data;

	pr_debug(PRT_NAME ": %s\n", __func__);

	if (1 != count  || !buf) {
		pr_err(PRT_NAME ": Error. Invalid arg @RFS read.\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&felica_drv->felica_rfs_lock, flags);

#if 1
	/* Read RFS GPIO value */
	gpio_val = gpio_get_value(pfdata->gpio_rfs);
	if (FELICA_NFC_LOW == gpio_val)
		kbuf = FELICA_NFC_HIGH;
	else if (FELICA_NFC_HIGH == gpio_val)
		kbuf = FELICA_NFC_LOW;
	else {
		pr_err(PRT_NAME ": Error. Invalid GPIO value @RFS read.\n");
		return -EIO;
	}
#else
	kbuf = felica_drv->rfs_value;
#endif
	atomic_set(&felica_drv->rfs_value, kbuf);
	spin_unlock_irqrestore(&felica_drv->felica_rfs_lock, flags);

	/* Copy the value to user space */
	if (copy_to_user(buf, &kbuf, 1)) {
		pr_err(PRT_NAME ": Error. copy_to_user failure.\n");
		return -EFAULT;
	}

	/* 1 byte read */
	return 1;
}

/***************** RFS FOPS ****************************/
static const struct file_operations felica_rfs_fops = {
	.owner		= THIS_MODULE,
	.read		= felica_rfs_read,
	.open		= felica_rfs_open,
	.release	= felica_rfs_release,
};

static struct miscdevice felica_rfs_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "felica_rfs",
	.fops = &felica_rfs_fops,
};

static struct miscdevice snfc_rfs_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "snfc_rfs",
	.fops = &felica_rfs_fops,
};

/**
 * @brief  Initilialize FeliCa RFS controller
 * @details This function executes;\n
 *            # Check RFS platform data\n
 *            # Alloc and init RFS controller's data\n
 *            # Request RFS GPIO\n
 *            # Create RFS character device (/dev/felica_rfs)
 * @param   pfdata   : Pointer to RFS platform data
 * @retval  0        : Success
 * @retval  Negative : Initialization failed.\n
 *            -EINVAL = No platform data\n
 *            -ENODEV = Requesting GPIO failed\n
 *            -ENOMEM = No enough memory / Cannot create char dev
 * @note
 */
int felica_rfs_probe_func(struct felica_device *pdevice)
{
	int ret;
	struct felica_platform_data *pfdata= pdevice->pdev->dev.platform_data;

	pr_debug(PRT_NAME ": %s\n", __func__);

	/* Check RFS platform data */
	if (!pfdata) {
		pr_err(PRT_NAME ": Error. No platform data for RFS.\n");
		return -EINVAL;
	}

	spin_lock_init(&pdevice->felica_rfs_lock);
	atomic_set(&pdevice->rfs_value, FELICA_NFC_HIGH);
	pdevice->rfs_status = FELICA_NFC_POLL_STATUS_INIT;
	
	/* Request RFS GPIO */
	ret = gpio_request(pfdata->gpio_rfs, "felica_rfs");
	if (ret) {
		pr_err(PRT_NAME ": Error. RFS GPIO request failed.\n");
		ret = -ENODEV;
		goto err_request_rfs_gpio;
	}
	ret = gpio_direction_input(pfdata->gpio_rfs);
	if (ret) {
		pr_err(PRT_NAME ": Error. RFS GPIO direction failed.\n");
		ret = -ENODEV;
		goto err_direction_rfs_gpio;
	}

	ret = felica_rfs_sysfs_create();
	if (ret) {
		pr_err(PRT_NAME ": Error. Request IRQ failed.\n");
		ret = -EIO;
		goto err_rfs_sys;

	}
	/* Create RFS character device (/dev/felica_rfs) */
	if (misc_register(&felica_rfs_device)) {
		pr_err(PRT_NAME ": Error. Cannot register RFS.\n");
		ret = -ENOMEM;
		goto err_create_rfs_dev;
	}

	/* Create RFS character device (/dev/snfc_rfs) */
	if (misc_register(&snfc_rfs_device)) {
		pr_err(PRT_NAME ": Error. Cannot register RFS.\n");
		ret = -ENOMEM;
		goto err_create_rfs_dev;
	}

	return 0;

err_create_rfs_dev:
err_rfs_sys:
err_direction_rfs_gpio:
	gpio_free(pfdata->gpio_rfs);
err_request_rfs_gpio:
	return ret;
}

/**
 * @brief   Terminate FeliCa RFS controller
 * @details This function executes;\n
 *            # Deregister RFS character device (/dev/felica_rfs)\n
 *            # Release RFS GPIO resource\n
 *            # Release RFS controller's data
 * @param   N/A
 * @retval  N/A
 * @note
 */
void felica_rfs_remove_func(void)
{
	struct felica_platform_data *pfdata= felica_drv->pdev->dev.platform_data;

	pr_debug(PRT_NAME ": %s\n", __func__);

	misc_deregister(&felica_rfs_device);
	gpio_free(pfdata->gpio_rfs);
	felica_rfs_sysfs_remove();
}

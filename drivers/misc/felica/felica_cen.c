/* vendor/semc/hardware/felica/msm_felica_cen.c
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
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <crypto/hash.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/felica.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/syscalls.h>

#ifdef CONFIG_S7760A
#include <linux/s7760a.h>
#endif
#include "felica_master.h"


#define PRT_NAME "felica cen"
#define WRITE_LEN 1

static struct felica_sysfs_dev felica_cen_dev;

static ssize_t felica_cen_value_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n",atomic_read(&felica_drv->cen_value));
}

static ssize_t felica_cen_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n",(felica_drv->cen_status));
}

static struct device_attribute felica_cen_sysfs_entries[] = 
{
	{
		.attr.name = "felica_cen_value",
		.attr.mode = S_IRUSR|S_IRGRP|S_IWUSR|S_IWGRP,
		.show = felica_cen_value_show,
	},
	{
		.attr.name = "felica_cen_status",
		.attr.mode = S_IRUSR|S_IRGRP|S_IWUSR|S_IWGRP,
		.show = felica_cen_status_show,
	}
};

static int felica_cen_sysfs_create(void)
{
	felica_cen_dev.attr = felica_cen_sysfs_entries;
	felica_cen_dev.attr_size = sizeof(felica_cen_sysfs_entries)/sizeof(struct device_attribute);
	felica_cen_dev.name = "felica_cen";
	felica_cen_dev.minor = 0;

	return felica_sysfs_register(&felica_cen_dev);
}

static void felica_cen_sysfs_remove(void)
{
	felica_sysfs_unregister(&felica_cen_dev);
}

/**
 * @brief   Open file operation of FeliCa CEN controller
 * @details This function executes;\n
 *            # Get PMIC8058 NFC device\n
 *            # Read NFC support register\n
 *            # [If NFC support is not enabled,] write initial value.
 * @param   inode    : (unused)
 * @param   file     : (unused)
 * @retval  0        : Success
 * @retval  Negative : Failure\n
 *            -ENODEV = Cannot find PMIC NFC device\n
 *            -EIO    = Cannot access PMIC NFC device
 * @note
 */
static int felica_cen_open(struct inode *inode, struct file *file)
{
	pr_debug(PRT_NAME ": %s\n", __func__);

	return 0;
}

/**
 * @brief  Close file operation of FeliCa CEN controller
 * @param  inode : (unused)
 * @param  file  : (unused)
 * @retval 0     : Success
 * @note
 */
static int felica_cen_release(struct inode *inode, struct file *file)
{
	pr_debug(PRT_NAME ": %s\n", __func__);

	return 0;
}

/**
 * @brief   poll file operation of FeliCa CEN controller
 * @details This function executes;\n
 *            # Read PMIC8058 NFC support register\n
 *            # Copy PM_NFC_EN value to user space
 * @param   file     : (unused)
 * @param   buf      : Destination of the read data
 * @param   count    : Data length must be 1
 * @param   offset   : (unused)
 * @retval  1        : Success
 * @retval  Negative : Failure\n
 *            -EINVAL = Invalid argment\n
 *            -EIO    = Cannot access PMIC NFC device\n
 *            -EFAULT = Cannot copy data to user space
 * @note
 */

static unsigned int felica_cen_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
	unsigned long flags;

	pr_debug(PRT_NAME ": %s\n", __func__);

	poll_wait(file, &felica_drv->felica_cen_write_queue, wait);

	spin_lock_irqsave(&felica_drv->felica_cen_lock, flags);

	if (felica_drv->cen_status == FELICA_NFC_POLL_STATUS_ACTIVE)
		mask |= (POLLIN | POLLWRNORM);
	felica_drv->cen_status = FELICA_NFC_POLL_STATUS_INACTIVE;
	
	spin_unlock_irqrestore(&felica_drv->felica_cen_lock, flags);
	return mask;
}

/**
 * @brief   Read file operation of FeliCa CEN controller
 * @details This function executes;\n
 *            # Read PMIC8058 NFC support register\n
 *            # Copy PM_NFC_EN value to user space
 * @param   file     : (unused)
 * @param   buf      : Destination of the read data
 * @param   count    : Data length must be 1
 * @param   offset   : (unused)
 * @retval  1        : Success
 * @retval  Negative : Failure\n
 *            -EINVAL = Invalid argment\n
 *            -EIO    = Cannot access PMIC NFC device\n
 *            -EFAULT = Cannot copy data to user space
 * @note
 */
static ssize_t felica_cen_read(struct file *file, char __user *buf,
					size_t count, loff_t *offset)
{
#if(BOARD_VER > WS10)
	int ret;
	unsigned char kbuf;
	unsigned long flags;
	int old_fs;
	int fd;
	
	pr_debug(PRT_NAME ": %s\n", __func__);

	if (WRITE_LEN != count || !buf) {
		pr_err(PRT_NAME ": Error. Invalid arg @CEN read.\n");
		return -EINVAL;
	}
	
	mutex_lock(&felica_drv->felica_cen_mutex);

	old_fs =get_fs();
	set_fs(KERNEL_DS);

	fd = sys_open("/dev/s7760a", O_RDWR, 0);
	if (fd < 0) {
		pr_err(PRT_NAME ":Error s7760a open. \n");
		mutex_unlock(&felica_drv->felica_cen_mutex);
		return -EFAULT;
	}
	kbuf = (char)sys_ioctl(fd,12, 0);
	
	spin_lock_irqsave(&felica_drv->felica_cen_lock, flags);
	atomic_set(&felica_drv->cen_value, kbuf);
	spin_unlock_irqrestore(&felica_drv->felica_cen_lock, flags);

	sys_close(fd);
	set_fs(old_fs);

	mutex_unlock(&felica_drv->felica_cen_mutex);

	/* Copy PM_NFC_EN value to user space */
	ret = copy_to_user(buf, &kbuf, 1);
	if (ret) {
		pr_err(PRT_NAME ": Error. copy_to_user failure.\n");
		return -EFAULT;
	}
#else
	int ret;
	unsigned char kbuf;
	unsigned long flags;
	struct felica_platform_data *pfdata = felica_drv->pdev->dev.platform_data;

	pr_debug(PRT_NAME ": %s\n", __func__);

	if (WRITE_LEN != count || !buf) {
		pr_err(PRT_NAME ": Error. Invalid arg @CEN read.\n");
		return -EINVAL;
	}
	
	spin_lock_irqsave(&felica_drv->felica_cen_lock, flags);
	kbuf =gpio_get_value(pfdata->gpio_temp);
	atomic_set(&felica_drv->cen_value, kbuf);
	spin_unlock_irqrestore(&felica_drv->felica_cen_lock, flags);

	/* Copy PM_NFC_EN value to user space */
	ret = copy_to_user(buf, &kbuf, 1);
	if (ret) {
		pr_err(PRT_NAME ": Error. copy_to_user failure.\n");
		return -EFAULT;
	}
#endif
	/* 1 byte read */
	return 1;
}

/**
 * @brief   Write file operation of FeliCa CEN controller
 * @details This function executes;\n
 *            # Carry out user authentication\n
 *            # Copy value from user space\n
 *            # Write PMIC8058 NFC support register\n
 *            # usec delay
 * @param   file     : (unused)
 * @param   buf      : Source of the written data
 * @param   count    : Data length must be WRITE_LEN.
 * @param   offset   : (unused)
 * @retval  1        : Success
 * @retval  Negative : Failure\n
 *            -EINVAL = Invalid argument\n
 *            -ENOMEM = No enough memory\n
 *            -EFAULT = Cannot copy data from user space\n
 *            -EIO    = Cannot access PMIC NFC device\n
 *            -EACCES = Permission denied
 */
static ssize_t felica_cen_write(struct file *file, const char __user *buf,
					size_t count, loff_t *offset)
{
#if(BOARD_VER > WS10)
	int ret =1;
	char kbuf;
	unsigned long flags;
	int old_fs;
	int fd;

	pr_debug(PRT_NAME ": %s\n", __func__);

	if (WRITE_LEN != count  || !buf) {
		pr_err(PRT_NAME ": Error. Invalid arg @CEN write.\n");
		return -EINVAL;
	}

	/* Copy value from user space */
	ret = copy_from_user(&kbuf, buf, 1);
	if (ret) {
		pr_err(PRT_NAME ": Error. copy_from_user failure.\n");
		return -EFAULT;
	}
	
	if (atomic_read(&felica_drv->cen_value) == kbuf)
		return 1;

	mutex_lock(&felica_drv->felica_cen_mutex);
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fd = sys_open("/dev/s7760a", O_RDWR, 0);

	if (fd <0 ) {
		pr_err(PRT_NAME ": Error. Cannot write CEN Value.\n");
		mutex_unlock(&felica_drv->felica_cen_mutex);
		return -EFAULT;
	}

	sys_ioctl(fd, 13 ,(unsigned long)kbuf);
	
	sys_close(fd);
	set_fs(old_fs);
	
	spin_lock_irqsave(&felica_drv->felica_cen_lock, flags);
	atomic_set(&felica_drv->cen_value, kbuf);
	felica_drv->cen_status = FELICA_NFC_POLL_STATUS_ACTIVE;

	spin_unlock_irqrestore(&felica_drv->felica_cen_lock, flags);
	
	mutex_unlock(&felica_drv->felica_cen_mutex);
#else
	int ret =1;
	char kbuf;
	unsigned long flags;
	struct felica_platform_data *pfdata = felica_drv->pdev->dev.platform_data;

	pr_debug(PRT_NAME ": %s\n", __func__);

	if (WRITE_LEN != count  || !buf) {
		pr_err(PRT_NAME ": Error. Invalid arg @CEN write.\n");
		return -EINVAL;
	}

	/* Copy value from user space */
	ret = copy_from_user(&kbuf, buf, 1);
	if (ret) {
		pr_err(PRT_NAME ": Error. copy_from_user failure.\n");
		return -EFAULT;
	}
	
	if (atomic_read(&felica_drv->cen_value) == kbuf)
		return 1;
	
	spin_lock_irqsave(&felica_drv->felica_cen_lock, flags);
	if (FELICA_NFC_HIGH == kbuf) {
		/* Write High to PON GPIO */
		gpio_set_value(pfdata->gpio_temp, FELICA_NFC_HIGH);
		atomic_set(&felica_drv->cen_value, FELICA_NFC_HIGH);
	} else if (FELICA_NFC_LOW == kbuf) {
		/* Write LOW to PON GPIO */
		gpio_set_value(pfdata->gpio_temp, FELICA_NFC_LOW);
		atomic_set(&felica_drv->cen_value, FELICA_NFC_LOW);
	} else {
		pr_err(PRT_NAME ": Error. Invalid val @PON write.\n");
		ret = -EINVAL;
	}
	felica_drv->cen_status = FELICA_NFC_POLL_STATUS_ACTIVE;

	spin_unlock_irqrestore(&felica_drv->felica_cen_lock, flags);
#endif
	wake_up(&felica_drv->felica_cen_write_queue);

	/* 1 byte write */
	return 1;
}

/***************** CEN FOPS ****************************/
static const struct file_operations felica_cen_fops = {
	.owner		= THIS_MODULE,
	.read		= felica_cen_read,
	.write		= felica_cen_write,
	.open		= felica_cen_open,
	.release	= felica_cen_release,
};

static struct miscdevice felica_cen_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "felica_cen",
	.fops = &felica_cen_fops,
};

/***************** CEN FOPS ****************************/
static const struct file_operations snfc_cen_fops = {
	.owner		= THIS_MODULE,
	.read		= felica_cen_read,
	.open		= felica_cen_open,
	.release	= felica_cen_release,
	.poll		= felica_cen_poll,
};

static struct miscdevice snfc_cen_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "snfc_cen",
	.fops = &snfc_cen_fops,
};


/**
 * @brief   Initialize FeliCa CEN controller
 * @details This function executes;\n
 *            # Check CEN platform data\n
 *            # Alloc and Initialize CEN controller's data\n
 *            # Create CEN character device (/dev/felica_cen)
 * @param   pfdata   : Pointer to CEN platform data
 * @retval  0        : Success
 * @retval  Negative : Initialization failed.\n
 *            -EINVAL = No platform data\n
 *            -ENOMEM = No enough memory / Cannot create char dev
 * @note
 */
int felica_cen_probe_func(struct felica_device *pdevice)
{
	int ret;
	struct felica_platform_data *pfdata= pdevice->pdev->dev.platform_data;

	pr_debug(PRT_NAME ": %s\n", __func__);

	/* Check CEN platform data */
	if (!pfdata) {
		pr_err(PRT_NAME ": Error. No platform data for CEN.\n");
		return -EINVAL;
	}
	
	
	spin_lock_init(&pdevice->felica_cen_lock);
	init_waitqueue_head(&pdevice->felica_cen_write_queue);
	mutex_init(&pdevice->felica_cen_mutex);
	atomic_set(&pdevice->cen_value, -1);
	pdevice->cen_status = FELICA_NFC_POLL_STATUS_INIT;
	
	/* Request PON GPIO */
	ret = gpio_request(pfdata->gpio_temp, "felica_temp");
	if (ret) {
		pr_err(PRT_NAME ": Error. TEMP GPIO request failed.\n");
		ret = -ENODEV;
		goto err_request_temp_gpio;
	}

	
#if(BOARD_VER > WS10)
	ret = gpio_direction_output(pfdata->gpio_temp, FELICA_NFC_LOW);
#else
	ret = gpio_direction_output(pfdata->gpio_temp, FELICA_NFC_HIGH);
#endif
	if (ret) {
		pr_err(PRT_NAME ": Error. TEMP GPIO direction failed.\n");
		ret = -ENODEV;
		goto err_direction_temp_gpio;
	}

	ret =felica_cen_sysfs_create();
	if (ret) {
		pr_err(PRT_NAME ": Error. TEMP GPIO direction failed.\n");
		ret = -ENODEV;
		goto err_cen_sys;
	}
	/* Create CEN character device (/dev/felica_cen) */
	if (misc_register(&felica_cen_device)) {
		pr_err(PRT_NAME ": Error. Cannot register CEN.\n");
		ret = -ENOMEM;
		goto err_create_cen_dev;
	}

	/* Create CEN character device (/dev/snfc_cen) */
	if (misc_register(&snfc_cen_device)) {
		pr_err(PRT_NAME ": Error. Cannot register CEN.\n");
		ret = -ENOMEM;
		goto err_create_cen_dev;
	}

	return 0;
	
err_create_cen_dev:
err_direction_temp_gpio:
err_cen_sys:
	gpio_free(pfdata->gpio_temp);
err_request_temp_gpio:
	return ret;
}

/**
 * @brief   Terminate FeliCa cen controller
 * @details This function executes;\n
 *            # Deregister CEN character device (/dev/felica_cen)\n
 *            # Release CEN controller's data
 * @param   N/A
 * @retval  N/A
 * @note
 */
void felica_cen_remove_func(void)
{
	struct felica_platform_data *pfdata= felica_drv->pdev->dev.platform_data;

	pr_debug(PRT_NAME ": %s\n", __func__);

	misc_deregister(&felica_cen_device);
	gpio_free(pfdata->gpio_temp);
	felica_cen_sysfs_remove();
}

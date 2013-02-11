/* vendor/semc/hardware/felica/felica_rws.c
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
#include <linux/felica.h>
#include "felica_master.h"

#define PRT_NAME "felica rws"
#define ONEBYTE  1

/**
 * @brief   Open file operation of FeliCa RWS controller
 * @param   inode : (unused)
 * @param   file  : (unused)
 * @retval  0     : Success
 * @note
 */
static int felica_rws_open(struct inode *inode, struct file *file)
{
	pr_debug(PRT_NAME ": %s\n", __func__);

	return 0;
}

/**
 * @brief   Close file operation of FeliCa RWS controller
 * @param   inode : (unused)
 * @param   file  : (unused)
 * @retval  0     : Success
 * @note
 */
static int felica_rws_release(struct inode *inode, struct file *file)
{
	pr_debug(PRT_NAME ": %s\n", __func__);

	return 0;
}

/**
 * @brief   Write file operation of FeliCa RWS controller
 * @details This function executes;\n
 *            # Check R/W module parameters\n
 *            # Determine RWS value\n
 *            # Copy the value to user space
 * @param   file      : (unused)
 * @param   buf       : Destination of the read data
 * @param   count     : Data length must be 1
 * @param   offset    : (unused)
 * @retval  ONEBYTE(1): Success.
 * @retval  Negative  : Failure\n
 *            -EINVAL = Invalid argument\n
 *            -EIO    = Invalid module parameters\n
 *            -EFAULT = Cannot copy data to user space
 * @note
 */
static ssize_t felica_rws_write(struct file *file, const char __user *buf,
					size_t count, loff_t *offset)
{
	char kbuf;

	pr_err(PRT_NAME ": %s\n", __func__);

	if (1 != count  || !buf) {
		pr_err(PRT_NAME ": Error. Invalid arg @PON write.\n");
		return -EINVAL;
	}

	/* Copy value from user space */
	if (copy_from_user(&kbuf, buf, 1)) {
		pr_err(PRT_NAME ": Error. copy_from_user failure.\n");
		return -EFAULT;
	}
	atomic_set(&felica_drv->rws_value, kbuf);

	/* 1 byte write */
	return 1;
}

/**
 * @brief   Read file operation of FeliCa RWS controller
 * @details This function executes;\n
 *            # Check R/W module parameters\n
 *            # Determine RWS value\n
 *            # Copy the value to user space
 * @param   file      : (unused)
 * @param   buf       : Destination of the read data
 * @param   count     : Data length must be 1
 * @param   offset    : (unused)
 * @retval  ONEBYTE(1): Success.
 * @retval  Negative  : Failure\n
 *            -EINVAL = Invalid argument\n
 *            -EIO    = Invalid module parameters\n
 *            -EFAULT = Cannot copy data to user space
 * @note
 */
static ssize_t felica_rws_read(struct file *file, char __user *buf,
					size_t count, loff_t *offset)
{
	char kbuf;

	pr_debug(PRT_NAME ": %s\n", __func__);

	if (ONEBYTE != count  || !buf) {
		pr_err(PRT_NAME ": Error. Invalid arg @RWS read.\n");
		return -EINVAL;
	}

	/* Check R/W module parameters */
	kbuf = atomic_read(&felica_drv->rws_value);
	/* Copy the value to user space */
	if (copy_to_user(buf, &kbuf, ONEBYTE)) {
		pr_err(PRT_NAME ": Error. copy_to_user failure.\n");
		return -EFAULT;
	}

	/* 1 byte read */
	return ONEBYTE;
}

/***************** RWS FOPS ****************************/
static const struct file_operations felica_rws_fops = {
	.owner		= THIS_MODULE,
	.write		= felica_rws_write,
	.read		= felica_rws_read,
	.open		= felica_rws_open,
	.release	= felica_rws_release,
};

static struct miscdevice felica_rws_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "felica_rws",
	.fops = &felica_rws_fops,
};

/**
 * @brief  Initilialize FeliCa RWS controller
 * @details This function executes;\n
 *            # Initialize R/W module parameters\n
 *            # Create RWS character device (/dev/felica_rws)
 * @param   N/A
 * @retval  0        : Success
 * @retval  Negative : Initialization failed.\n
 *            -ENOMEM = Cannot create char dev
 * @note
 */
int felica_rws_probe_func(void)
{
	pr_debug(PRT_NAME ": %s\n", __func__);

	atomic_set(&felica_drv->rws_value, FELICA_NFC_LOW);
	/* Create RWS character device (/dev/felica_rws) */
	if (misc_register(&felica_rws_device)) {
		pr_err(PRT_NAME ": Error. Cannot register RWS.\n");
		return -ENOMEM;
	}

	return 0;
}

/**
 * @brief   Terminate FeliCa RWS controller
 * @details This function executes;\n
 *            # Deregister RWS character device (/dev/felica_rws)\n
 * @param   N/A
 * @retval  N/A
 * @note
 */
void felica_rws_remove_func(void)
{
	pr_debug(PRT_NAME ": %s\n", __func__);

	misc_deregister(&felica_rws_device);
}

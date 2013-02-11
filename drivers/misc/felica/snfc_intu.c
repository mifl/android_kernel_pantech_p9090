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
#include <linux/miscdevice.h>
#include <linux/workqueue.h>
#include <linux/felica.h>
#include "felica_master.h"

#define PRT_NAME "snfc intu"
#define INT_LOW  0x0
#define INT_HIGH 0x1


static void snfc_disable_irq(void)
{
	unsigned long flags;
	struct felica_platform_data *pfdata = felica_drv->pdev->dev.platform_data;

	spin_lock_irqsave(&felica_drv->snfc_intu_lock, flags);
	if (felica_drv->snfc_irq_enabled) {
		disable_irq_nosync(MSM_GPIO_TO_INT(pfdata->gpio_intu));
		felica_drv->snfc_irq_enabled = false;
	}
	spin_unlock_irqrestore(&felica_drv->snfc_intu_lock, flags);
}
/**
 * @brief   Interrupt handler of SNFC INTU controller
 * @details This function executes;\n
 *            # Disable INT interrupt\n
 *            # Schedule work for FeliCa push (keventd_wq)
 * @param   irq         : (unused)
 * @param   dev         : (unused)
 * @retval  IRQ_HANDLED : Success
 * @note
 */
static irqreturn_t snfc_intu_irq_handler(int irq, void *dev)
{
	pr_debug(PRT_NAME ": %s\n", __func__);

	/* Disable INT interrupt */
	snfc_disable_irq();
	
	/* Wake up waiting wq */
	wake_up(&felica_drv->snfc_intu_wq);

	return IRQ_HANDLED;
}

/**
 * @brief   Open file operation of SNFC INTU controller
 * @param   inode : (unused)
 * @param   file  : (unused)
 * @retval  0     : Success
 * @note
 */
static int snfc_intu_open(struct inode *inode, struct file *file)
{
	pr_debug(PRT_NAME ": %s\n", __func__);

	return 0;
}

/**
 * @brief   Close file operation of SNFC INTU controller
 * @param   inode    : (unused)
 * @param   file     : (unused)
 * @retval  0        : Success
 * @note
 */
static int snfc_intu_release(struct inode *inode, struct file *file)
{
	pr_debug(PRT_NAME ": %s\n", __func__);

	return 0;
}

/**
 * @brief   Write file operation of SNFC INTU controller
 * @details This function executes;\n
 *            # Read value from user space\n
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
static ssize_t snfc_intu_read(struct file *file, char __user *buf,
					size_t count, loff_t *offset)
{
	char kbuf;
	int ret;
	struct felica_platform_data *pfdata= felica_drv->pdev->dev.platform_data;

	pr_debug(PRT_NAME ": %s\n", __func__);

	if (1 != count  || !buf) {
		pr_err(PRT_NAME ": Error. Invalid arg @PON write.\n");
		return -EINVAL;
	}

	mutex_lock(&felica_drv->snfc_intu_mutex);

	if (gpio_get_value(pfdata->gpio_intu)) {

		felica_drv->snfc_irq_enabled = true;
		enable_irq(MSM_GPIO_TO_INT(pfdata->gpio_intu));
		ret = wait_event_interruptible(felica_drv->snfc_intu_wq,
										!gpio_get_value(pfdata->gpio_intu));
		snfc_disable_irq();

		if (ret)
			goto fail;
	}
	kbuf = (char)gpio_get_value(pfdata->gpio_intu);
	
	mutex_unlock(&felica_drv->snfc_intu_mutex);
	if (copy_to_user(buf, &kbuf, 1)) {
		return -EFAULT;	
	}
	/* 1 byte write */
	return 1;
fail:
	mutex_unlock(&felica_drv->snfc_intu_mutex);
	return ret;
}

/***************** INTU FOPS ****************************/
static const struct file_operations snfc_intu_fops = {
	.owner		= THIS_MODULE,
	.read		= snfc_intu_read,
	.open		= snfc_intu_open,
	.release	= snfc_intu_release,
};

static struct miscdevice snfc_intu_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "snfc_intu",
	.fops = &snfc_intu_fops,
};

/**
 * @brief   Initialize SNFC INTU controller
 * @details This function executes;\n
 *            # Check INT platform data\n
 *            # Alloc and init INT controller's data\n
 *            # Init work of FeliCa push\n
 *            # Request INT GPIO\n
 *            # Request IRQ for INT GPIO\n
 *            # Enable IRQ wake for INT GPIO\n
 *            # Create INT switch device (felica_push)\n
 *            # Set default state of the device
 * @param   pfdata   : Pointer to INT platform data
 * @retval  0        : Success
 * @retval  Negative : Failure\n
 *            -EINVAL = No platform data\n
 *            -ENOMEM = No enough memory / Cannot create switch dev\n
 *            -ENODEV = GPIO request failed\n
 *            -EIO    = IRQ request failed / Enabling IRQ wake failed
 */
int snfc_intu_probe_func(struct felica_device *pdevice)
{
	int ret;
	struct felica_platform_data *pfdata = pdevice->pdev->dev.platform_data;
	
	pr_debug(PRT_NAME ": %s\n", __func__);

	/* Check INT platform data */
	if (!pfdata) {
		pr_err(PRT_NAME ": Error. No platform data for INT.\n");
		return -EINVAL;
	}

	/* Init wait queue  */
	init_waitqueue_head(&pdevice->snfc_intu_wq);
	mutex_init(&pdevice->snfc_intu_mutex);
	spin_lock_init(&pdevice->snfc_intu_lock);

	/* Request INT GPIO */
	ret = gpio_request(pfdata->gpio_intu, "snfc_intu");
	if (ret) {
		pr_err(PRT_NAME ": Error. INTU GPIO request failed.\n");
		ret = -ENODEV;
		goto err_request_intu_gpio;
	}
	ret = gpio_direction_input(pfdata->gpio_intu);
	if (ret) {
		pr_err(PRT_NAME ": Error. INT GPIO direction failed.\n");
		ret = -ENODEV;
		goto err_direction_intu_gpio;
	}

	/* Request IRQ for INT GPIO */
	
	pdevice->snfc_irq_enabled = true;
	ret = request_irq(MSM_GPIO_TO_INT(pfdata->gpio_intu), snfc_intu_irq_handler,
						IRQF_TRIGGER_FALLING, "snfc_irq", NULL);
	if (ret) {
		pr_err(PRT_NAME ": Error. Request IRQ failed.\n");
		ret = -EIO;
		goto err_request_intu_irq;
	}

	snfc_disable_irq();

	/* Create INTU character device (/dev/snfc_intu) */
	if (misc_register(&snfc_intu_device)) {
		pr_err(PRT_NAME ": Error. Cannot register INTU.\n");
		ret = -ENOMEM;
		goto err_create_intu_dev;
	}

	return 0;

err_create_intu_dev:
	free_irq(MSM_GPIO_TO_INT(pfdata->gpio_intu), NULL);
err_request_intu_irq:
err_direction_intu_gpio:
	gpio_free(pfdata->gpio_intu);
err_request_intu_gpio:
	mutex_destroy(&pdevice->snfc_intu_mutex);
	return ret;
}

/**
 * @brief   Terminate FeliCa INT controller
 * @details This function executes;\n
 *            # Unregister switch device (felica_push)\n
 *            # Release IRQ for INT GPIO\n
 *            # Release INT GPIO resource\n
 *            # Release INT controller's data
 * @param   N/A
 * @retval  N/A
 * @note
 */
void snfc_intu_remove_func(void)
{
	struct felica_platform_data *pfdata = felica_drv->pdev->dev.platform_data;

	pr_debug(PRT_NAME ": %s\n", __func__);

	free_irq(MSM_GPIO_TO_INT(pfdata->gpio_intu), NULL);
	gpio_free(pfdata->gpio_intu);
}

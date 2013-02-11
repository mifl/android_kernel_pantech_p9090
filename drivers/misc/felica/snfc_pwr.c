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
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/felica.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/compat.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <mach/irqs.h>

#include "felica_master.h"

#define PRT_NAME "snfc pwr"

#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)

#define NFC_HVDD_PM_GPIO PM8921_GPIO_PM_TO_SYS(44)


/**
 * @brief   Open file operation of sony NFC & USIM PWR(HVDD) controller
 * @param   inode : (unused)
 * @param   file  : (unused)
 * @retval  0     : Success
 * @note
 */
static int snfc_pwr_open(struct inode *inode, struct file *file)
{
	return 0;
}

/**
 * @brief   Close file operation of sony NFC & USIM PWR(HVDD) controller
 * @param   inode : (unused)
 * @param   file  : (unused)
 * @retval  0     : Success
 * @note
 */
static int snfc_pwr_release(struct inode *inode, struct file *file)
{
	return 0;
}

/**
 * @brief   Write file operation of sony NFC & USIM PWR(HVDD) controller
 * @details This function executes;\n
 *            # Copy value from user space\n
 *            # [When writing High,]\n
 *            #   | Write High to HVDD GPIO\n
 *            #   | [Params meet the condition,] turn on HVDD.\n
 *            # [When writing Low,]\n
 *            #   | Write Low to HVDD GPIO\n
 *            #   | Forcedly, turn off HVDD.\n
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
static ssize_t snfc_pwr_write(struct file *file, const char __user *buf,
					size_t count, loff_t *offset)
{
	char kbuf;


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
		gpio_set_value_cansleep(NFC_HVDD_PM_GPIO, FELICA_NFC_HIGH);
	} else if (FELICA_NFC_LOW == kbuf) {
		/* Write LOW to PON GPIO */
		gpio_set_value_cansleep(NFC_HVDD_PM_GPIO, FELICA_NFC_LOW);
	} else {
		pr_err(PRT_NAME ": Error. Invalid val @HVDD write.\n");
		return -EINVAL;
	}

	/* 1 byte write */
	return 1;
}


/***************** sony NFC FOPS ****************************/
static const struct file_operations snfc_pwr_fops = {
	.owner			= THIS_MODULE,
	.write			= snfc_pwr_write,	
	.open			= snfc_pwr_open,
	.release		= snfc_pwr_release,
};

static struct miscdevice snfc_pwr_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "snfc_pwr",
	.fops = &snfc_pwr_fops,
};

/**
 * @brief  Initilialize sony NFC & USIM PWR(HVDD) controller
 * @details This function executes;\n
 *            # Check snfc_usim_device data\n
 *            # Alloc and init sony NFC & USIM PWR(HVDD)controller's data\n
 *            # Create snfc character device (/dev/snfc)
 * @param   pfdata   : Pointer to snfc platform data
 * @retval  0        : Success
 * @retval  Negative : Initialization failed.\n
 *            -EINVAL = No platform data\n
 *            -ENODEV = Requesting GPIO failed\n
 *            -ENOMEM = No enough memory / Cannot create char dev
 * @note
 */

int snfc_pwr_probe_func(struct felica_device *pdevice)
{
	int rc;
	static struct pm_gpio hvdd_enable = {
		.direction		  = PM_GPIO_DIR_OUT,
		.output_buffer	  = PM_GPIO_OUT_BUF_CMOS,
		.output_value	  = 0,
		.pull			  = PM_GPIO_PULL_NO,
		.vin_sel		  = PM_GPIO_VIN_L4,
		.out_strength	  = PM_GPIO_STRENGTH_HIGH,
		.function		  = PM_GPIO_FUNC_NORMAL,
		.inv_int_pol	  = 0,
		.disable_pin	  = 0,
	};

	pr_debug(PRT_NAME ": %s\n", __func__);

	/* pm8xxx: gpio-44, NFC HVDD Enable */
	rc = pm8xxx_gpio_config(NFC_HVDD_PM_GPIO,&hvdd_enable);
	if (rc != 0){
			pr_err("%s: sim_enabled failed\n", __func__);
			return -EINVAL;
	}

	rc = gpio_request(NFC_HVDD_PM_GPIO, "hvdd_power_en");
	if (rc) {
		pr_err("request pm gpio 44 failed, rc=%d\n", rc);
		return -EINVAL;
	}

	/* Create Felica Uart character device (/dev/snfc_pwr) */
	if (misc_register(&snfc_pwr_device)) {
		pr_err(PRT_NAME ": Error. Cannot register SNFC.\n");
		return -ENOMEM;
	}

	return 0;
}

/**
 * @brief   Terminate sony NFC & USIM PWR(HVDD) controller
 * @details This function executes;\n
 *            # Deregister sony NFC character device (/dev/snfc)\n
 *            # Release snfc controller's data
 * @param   N/A
 * @retval  N/A
 * @note
 */
void snfc_pwr_remove_func(void)
{
	pr_debug(PRT_NAME ": %s\n", __func__);
	
	misc_deregister(&snfc_pwr_device);
	gpio_free(NFC_HVDD_PM_GPIO);
}


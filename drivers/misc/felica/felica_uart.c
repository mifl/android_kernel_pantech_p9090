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
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/felica.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/compat.h>
#include <linux/delay.h>
#include <linux/termios.h>
#include <asm/ioctls.h>
#include "felica_master.h"

#define PRT_NAME "felica uart"

#define UART_PORT_NAME "/dev/ttyHSL1"

#define FELICA_UART_MAX_TIME   200
#define FELICA_OPEN_TIMEOUT    5000
static struct felica_sysfs_dev felica_uart_dev;

static ssize_t felica_uart_cnt_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n",atomic_read(&felica_drv->felica_uart_cnt));
}

static ssize_t felica_uart_cnt_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	int value;

	value = simple_strtoul(buf, NULL, 10);
	atomic_set(&felica_drv->felica_uart_cnt,value);
	return size;
}
static ssize_t felica_uart_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n",(felica_drv->felica_uart_status));
}
static ssize_t felica_uart_status_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	int value;

	value = simple_strtoul(buf, NULL, 10);
	felica_drv->felica_uart_status = value;
	return size;
}

static struct device_attribute felica_uart_sysfs_entries[] = 
{
	{
		.attr.name = "felica_uart_cnt",
		.attr.mode = S_IRUSR|S_IRGRP|S_IWUSR|S_IWGRP,
		.show = felica_uart_cnt_show,
		.store = felica_uart_cnt_store,
	},
	{
		.attr.name = "felica_uart_status",
		.attr.mode = S_IRUSR|S_IRGRP|S_IWUSR|S_IWGRP,
		.show = felica_uart_status_show,
		.store = felica_uart_status_store,
	}
};

static int felica_uart_sysfs_create(void)
{
	felica_uart_dev.attr = felica_uart_sysfs_entries;
	felica_uart_dev.attr_size = sizeof(felica_uart_sysfs_entries)/sizeof(struct device_attribute);
	felica_uart_dev.name = "felica_uart";
	felica_uart_dev.minor = 0;

	return felica_sysfs_register(&felica_uart_dev);
}

static void felica_uart_sysfs_remove(void)
{
	felica_sysfs_unregister(&felica_uart_dev);
}
#if 0
static void felica_uart_used_cnt_inc(void)
{
	if (felica_drv->felica_uart_used_cnt >= 0xFF)
		felica_drv->felica_uart_used_cnt = 0;
	}else {
		felica_drv->felica_uart_used_cnt++;
	}
}

static void felica_uart_checking_used(void)
{

	int i;
	if (felica_drv->felica_uart_used_cnt != felica_drv->felica_uart_check_cnt) {
		felica_drv->felica_uart_check_cnt = felica_drv->felica_uart_used_cnt;
		felica_drv->felica_uart_time_tick = 0;
	}else {
		felica_drv->felica_uart_time_tick++;

		if ((felica_drv->felica_uart_time_tick >= FELICA_UART_MAX_TIME) &&
			(felica_drv->felica_uart_cnt > 0)) {
			
		}
	}
}
#endif
/**
 * @brief   Open file operation of FeliCa UART controller
 * @param   inode : (unused)
 * @param   file  : (unused)
 * @retval  0     : Success
 * @note
 */
static int felica_uart_open(struct inode *inode, struct file *file)
{
	int old_fs;
	long res=0;
	unsigned long retval;
	int ret;
	
	pr_debug(PRT_NAME ": %s: 0x%x\n", __func__,file->f_flags);
	if (atomic_read(&felica_drv->snfc_start_req) == SNFC_START_AUTOPOLL) {
		res = wait_event_interruptible_timeout(felica_drv->felica_uart_open_queue,
								(atomic_read(&felica_drv->hsel_value) ==FELICA_NFC_LOW),
								msecs_to_jiffies(FELICA_OPEN_TIMEOUT));
	
		if (res <= 0) {
			pr_err("[%s]Timeout or Returing on Interrupt =>res=%ld \n", __func__,res);
			return -EAGAIN;
		} 
	}else {
		if (atomic_read(&felica_drv->hsel_value) == FELICA_NFC_HIGH)
			return -1;
	}

	mutex_lock(&felica_drv->felica_uart_mutex);
	atomic_inc(&felica_drv->felica_uart_cnt);
	
	if (atomic_read(&felica_drv->felica_uart_cnt) == 1) {
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		felica_drv->felica_uart_fd = sys_open(UART_PORT_NAME, O_RDWR|O_NDELAY |O_NOCTTY, 0);
		ret= sys_ioctl(felica_drv->felica_uart_fd, FIONREAD, (unsigned long)&retval);
		if (retval > 0) {
			sys_ioctl(felica_drv->felica_uart_fd, TCFLSH, TCIFLUSH);
		}
		set_fs(old_fs);
		felica_drv->felica_uart_used_cnt=0;
	}
	mutex_unlock(&felica_drv->felica_uart_mutex);

	if (atomic_read(&felica_drv->felica_uart_cnt) > 2) {
		pr_err(PRT_NAME":[%s] felica_uart_cnt=%d\n",__func__,atomic_read(&felica_drv->felica_uart_cnt)); 
	}
		
	return 0;
}

/**
 * @brief   Close file operation of FeliCa UART controller
 * @param   inode : (unused)
 * @param   file  : (unused)
 * @retval  0     : Success
 * @note
 */
static int felica_uart_release(struct inode *inode, struct file *file)
{
	int old_fs;
	unsigned long retval;
	unsigned long flags;
	int ret;
	
	pr_debug(PRT_NAME ": %s\n", __func__);

	mutex_lock(&felica_drv->felica_uart_mutex);

	atomic_dec(&felica_drv->felica_uart_cnt);

	if (atomic_read(&felica_drv->felica_uart_cnt) == 0) {
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		ret= sys_ioctl(felica_drv->felica_uart_fd, FIONREAD, (unsigned long)&retval);
		if (retval > 0) {
			sys_ioctl(felica_drv->felica_uart_fd, TCFLSH, TCIFLUSH);
		}
		sys_close(felica_drv->felica_uart_fd);
		set_fs(old_fs);
		
		spin_lock_irqsave(&felica_drv->felica_uart_lock, flags);
		felica_drv->felica_uart_status = FELICA_NFC_POLL_STATUS_ACTIVE;
		spin_unlock_irqrestore(&felica_drv->felica_uart_lock, flags);
		wake_up_interruptible(&felica_drv->felica_uart_close_queue);
	}

	mutex_unlock(&felica_drv->felica_uart_mutex);

	if (atomic_read(&felica_drv->felica_uart_cnt) < 0) {
		pr_err(PRT_NAME":[%s] felica_uart_cnt=%d\n",__func__, atomic_read(&felica_drv->felica_uart_cnt)); 
	}

	return 0;
}

/**
 * @brief   Read file operation of FeliCa RFS controller
 * @details This function executes;\n
 *            # Read UART Buffer value\n
 *            # Copy the value to user space
 * @param   inode    : (unused)
 * @param   file     : (unused)
 * @retval  len        : Success
 * @retval  Negative : Failure\n
 *            -EINVAL = Invalid argument\n
 *            -EIO    = GPIO read error\n
 *            -EFAULT = Cannot copy data to user space
 * @note
 */
static ssize_t felica_uart_read(struct file *file, char __user *buf,
					size_t count, loff_t *offset)
{
	int len;

	pr_debug(PRT_NAME ": %s, count=%d\n", __func__, (int) count);

	mutex_lock(&felica_drv->felica_uart_mutex);
	if (atomic_read(&felica_drv->hsel_value) != FELICA_NFC_LOW) {
		mutex_unlock(&felica_drv->felica_uart_mutex);
		return -1;
	}
	len = sys_read(felica_drv->felica_uart_fd, buf, count);

	if (len == -EAGAIN) {
		len =0;
	}
	mutex_unlock(&felica_drv->felica_uart_mutex);
	return len;
}

/**
 * @brief   Write file operation of FeliCa UART controller
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
static ssize_t felica_uart_write(struct file *file, const char __user *buf,
					size_t count, loff_t *offset)
{
	int len;
	
	pr_debug(PRT_NAME ": %s, count=%d\n", __func__, (int)count);

	mutex_lock(&felica_drv->felica_uart_mutex);
	if (atomic_read(&felica_drv->hsel_value) != FELICA_NFC_LOW){
		mutex_unlock(&felica_drv->felica_uart_mutex);
		return -1;
	}
	len = sys_write(felica_drv->felica_uart_fd, buf, count);
	mutex_unlock(&felica_drv->felica_uart_mutex);
	return len;
}

/**
 * @brief   Handle IOCTL system call of FeliCa UART
 * @details This function executes;\n
 *            # Check IO control number is Available (FIONREAD).\n
 *            # Issue AVAIABLE_SYSCALL event.\n
 *            # Copy result to User space\n
 *            # [Optional][If data not increase,] sleep
 * @param   inode    : (unused)
 * @param   file     : (unused)
 * @param   cmd      : IOCTL command number
 * @param   arg      : IOCTL arguments
 * @retval  0        : Success.
 * @retval  Negative : Failure.\n
 *            -EINVAL = The arguments are invalid for Available.\n
 *            -EFAULT = RX buffer/Data copy error to execute Available.\n
 *            -ENOTTY = Unsupported IOCTL number
 */
					
static long felica_uart_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret;
	int old_fs;
	unsigned long retval;
	void __user *argp = (void __user *)(arg);

	pr_debug(PRT_NAME ": %s, cmd=%u \n", __func__, cmd);

	mutex_lock(&felica_drv->felica_uart_mutex);
	if (atomic_read(&felica_drv->hsel_value) != FELICA_NFC_LOW) {
		mutex_unlock(&felica_drv->felica_uart_mutex);
		return -1;
	}
	if (FIONREAD == cmd) {
		if (!arg) {
			pr_err(PRT_NAME
				": Error. Invalid arg @available.\n");
			return -EINVAL;
		}
		
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		ret = sys_ioctl(felica_drv->felica_uart_fd,cmd, (unsigned long)&retval);
		set_fs(old_fs);
		if (ret) {
			pr_err(PRT_NAME ": Error. RX buffer check failed.ret=%d\n", ret);
			mutex_unlock(&felica_drv->felica_uart_mutex);
			return -EFAULT;
		}
		
		ret = copy_to_user((void __user *)argp,&retval,sizeof(unsigned long));
		if (ret) {
			pr_err(PRT_NAME
				": Error. copy_to_user failed.\n");
			mutex_unlock(&felica_drv->felica_uart_mutex);
			return -EFAULT;
		}
		mutex_unlock(&felica_drv->felica_uart_mutex);
		return 0;
	} else {
		pr_err(PRT_NAME ": Error. Unsupported IOCTL.\n");
		mutex_unlock(&felica_drv->felica_uart_mutex);
		return -ENOTTY;
	}


}
/**
 * @brief   Handle FSYNC system call of FeliCa UART
 * @details This function executes;\n
 *            # Issue This function is nothing on actually\n
*		             because tty driver is not supported for fsync file operation.
 * @param   file     : (unused)
 * @param   dentry   : (unused)
 * @param   datasync : (unused)
 * @retval  0        : Success.
 * 
 */
static int felica_uart_fsync(struct file *file, int datasync)
{
	return sys_fsync(felica_drv->felica_uart_fd);
}
/***************** RFS FOPS ****************************/
static const struct file_operations felica_uart_fops = {
	.owner			= THIS_MODULE,
	.read			= felica_uart_read,
	.write			= felica_uart_write,
	.unlocked_ioctl	= felica_uart_ioctl,
	.open			= felica_uart_open,
	.release		= felica_uart_release,
	.fsync			= felica_uart_fsync,
};

static struct miscdevice felica_uart_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "felica",
	.fops = &felica_uart_fops,
};

/**
 * @brief  Initilialize FeliCa UART controller
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
int felica_uart_probe_func(struct felica_device *pdevice)
{
	struct felica_platform_data *pfdata= pdevice->pdev->dev.platform_data;

	pr_debug(PRT_NAME ": %s\n", __func__);

	/* Check RFS platform data */
	if (!pfdata) {
		pr_err(PRT_NAME ": Error. No platform data for Felica Uart.\n");
		return -EINVAL;
	}

	spin_lock_init(&pdevice->felica_uart_lock);
	
	mutex_init(&pdevice->felica_uart_mutex);
	init_waitqueue_head(&pdevice->felica_uart_open_queue);
	init_waitqueue_head(&pdevice->felica_uart_close_queue);

	pdevice->felica_uart_status = FELICA_NFC_POLL_STATUS_INIT;
	atomic_set(&pdevice->felica_uart_cnt, 0);

	if (felica_uart_sysfs_create()) {
		pr_err(PRT_NAME ": Error. Cannot Felica Uart Sys.\n");
		return -ENOMEM;
	}
	/* Create Felica Uart character device (/dev/felica) */
	if (misc_register(&felica_uart_device)) {
		pr_err(PRT_NAME ": Error. Cannot register Felica Uart.\n");
		return -ENOMEM;
	}

	return 0;
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
void felica_uart_remove_func(void)
{
	pr_debug(PRT_NAME ": %s\n", __func__);

	misc_deregister(&felica_uart_device);
	felica_uart_sysfs_remove();
}


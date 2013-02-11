/* nfc/felica_master_int.h
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

#ifndef _FELICA_MASTER_H
#define _FELICA_MASTER_H
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/spinlock_types.h>
#include <linux/types.h>
#include <linux/switch.h>
#include <linux/device.h>

#define FELICA_NFC_HIGH 1
#define FELICA_NFC_LOW  0

#define FELICA_NFC_POLL_STATUS_INIT         -1
#define FELICA_NFC_POLL_STATUS_ACTIVE       1
#define FELICA_NFC_POLL_STATUS_INACTIVE     0

#define	SNFC_START_SETTING  0
#define SNFC_START_AUTOPOLL 1
#define SNFC_START_RW       2
#define SNFC_START_TARGET   3
#define SNFC_START_INTU     4
#define SNFC_START_END      5
	
struct felica_device {
	struct platform_device *pdev;
	struct switch_dev		swdev;
	
	struct work_struct      felica_work_int;
	struct work_struct      felica_work_rfs;
	
	struct mutex            snfc_intu_mutex;
	struct mutex            felica_cen_mutex;
	struct mutex            felica_uart_mutex;
	struct mutex            snfc_uart_mutex;
	
	wait_queue_head_t       snfc_intu_wq;
	
	spinlock_t              felica_cen_lock;
	spinlock_t              felica_rfs_lock;
	spinlock_t              felica_uart_lock;
	spinlock_t              snfc_intu_lock;
	spinlock_t              snfc_hsel_lock;
	
	bool                    snfc_irq_enabled;
	
	wait_queue_head_t       felica_cen_write_queue;
	wait_queue_head_t       felica_rfs_read_queue;
	wait_queue_head_t       felica_uart_open_queue;
	wait_queue_head_t       felica_uart_close_queue;

	atomic_t                cen_value;
	atomic_t                rfs_value;
	atomic_t                rws_value;
	atomic_t                hsel_value;
	int                     felica_uart_fd;
	int                     snfc_uart_fd;
	atomic_t                snfc_start_req;
	
	int                     cen_status;
	int                     rfs_status;
	int                     int_status;
	int                     intu_status;
	int                     avail_status;
	int                     snfc_uart_status;
	int                     felica_uart_status;
	
	atomic_t                felica_uart_cnt;
	unsigned char           felica_uart_used_cnt;  
	unsigned char           felica_uart_check_cnt;
	int                     felica_uart_time_tick;
};

struct felica_sysfs_dev
{
	int minor;
	char* name;
	struct device *dev;
	struct device_attribute *attr;
	int attr_size;
};

extern struct felica_device *felica_drv;

int felica_uart_probe_func(struct felica_device *);
void felica_uart_remove_func(void);

int felica_cen_probe_func(struct felica_device *);
void felica_cen_remove_func(void);

int felica_int_probe_func(struct felica_device *);
void felica_int_remove_func(void);

int felica_pon_probe_func(struct felica_device *);
void felica_pon_remove_func(void);

int felica_rfs_probe_func(struct felica_device *);
void felica_rfs_remove_func(void);

int felica_rws_probe_func(void);
void felica_rws_remove_func(void);

int snfc_pwr_probe_func(struct felica_device *);
void snfc_pwr_remove_func(void);

int snfc_intu_probe_func(struct felica_device *);
void snfc_intu_remove_func(void);

int snfc_hsel_probe_func(struct felica_device *);
void snfc_hsel_remove_func(void);

int snfc_uart_avail_probe_func(struct felica_device *);
void snfc_uart_avail_remove_func(void);

extern int felica_sysfs_register(struct felica_sysfs_dev *);
extern void felica_sysfs_unregister(struct felica_sysfs_dev *);
extern struct class *felica_class;
#endif

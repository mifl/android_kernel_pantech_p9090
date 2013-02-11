/*
 * Core Header for:
 * Cypress TrueTouch(TM) Standard Product (TTSP) touchscreen drivers.
 * For use with Cypress Gen4 and Solo parts.
 * Supported parts include:
 * CY8CTMA398
 * CY8CTMA884
 * CY8CTMA4XX
 *
 * Copyright (C) 2009-2011 Cypress Semiconductor, Inc.
 * Copyright (C) 2011 Motorola Mobility, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, and only version 2, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contact Cypress Semiconductor at www.cypress.com <kev@cypress.com>
 *
 */

#ifndef __CYTTSP4_CORE_H__
#define __CYTTSP4_CORE_H__

#if defined(CONFIG_MACH_MSM8960_SIRIUSLTE)
#include "cyttsp4_siriuslte.h"
#elif defined(CONFIG_MACH_MSM8960_MAGNUS)
#include "cyttsp4_magnus.h"
#else 
#include "cyttsp4_model.h"		// Default Config
#endif

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/input.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define CY_MAX_PRBUF_SIZE           PIPE_BUF

// p12279 Added for complie error
#define CY_PR_TRUNCATED	0

enum cyttsp4_unknown_debug {
	CY_DBG_LVL_0,
	CY_DBG_LVL_1,
	CY_DBG_LVL_2,
	CY_DBG_LVL_3,	
};

struct cyttsp4_bus_ops {
	int (*write)(void *handle, u16 subaddr, size_t length,
		const void *values, int i2c_addr, bool use_subaddr);
	int (*read)(void *handle, u16 subaddr, size_t length,
		void *values, int i2c_addr, bool use_subaddr);
	struct device *dev;
	int  tsdebug;
};


#define TRUE  1
#define FALSE 0

extern int chg_mode; 

void *cyttsp4_core_init(struct cyttsp4_bus_ops *bus_ops,
	struct device *dev, int irq, char *name);

void cyttsp4_core_release(void *handle);

#if !defined(CONFIG_HAS_EARLYSUSPEND)
#if defined(CONFIG_PM_SLEEP)
extern const struct dev_pm_ops cyttsp4_pm_ops;
#elif defined(CONFIG_PM)
int cyttsp4_resume(void *handle);
int cyttsp4_suspend(void *handle);
#endif
#endif

#endif /* __CYTTSP4_CORE_H__ */

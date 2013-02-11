/* linux/arch/arm/mach-msm/board-halibut-keypad.c
 *
 * Copyright (C) 2007 Google, Inc.
 * Author: Brian Swetland <swetland@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <asm/mach-types.h>
#include <linux/platform_device.h>
#include <linux/gpio_event.h>
#include <linux/gpio.h>

#define SLIDE_SENSOR_INT 54

static struct gpio_event_direct_entry starq_keypad_slide_map[] = {
	{54, SW_LID}
};

static struct gpio_event_input_info starq_keypad_slide_info = {
	.info.func = gpio_event_input_func,
	//.flags = 0,
	.flags = GPIOKPF_LEVEL_TRIGGERED_IRQ,
	//.flags = GPIOEDF_ACTIVE_HIGH,	// TBD
	.type = EV_SW,
	.keymap = starq_keypad_slide_map,
	.keymap_size = ARRAY_SIZE(starq_keypad_slide_map),
	.debounce_time.tv.nsec = 5 * NSEC_PER_MSEC,
	.poll_time.tv.nsec = 20 * NSEC_PER_MSEC,
	//.poll_time.tv.nsec = 20 * NSEC_PER_MSEC,
	//.poll_time.tv.nsec =	// default 20ms 
};

static struct gpio_event_info *starq_slide_info[] = {
	&starq_keypad_slide_info.info,	// added by jwcha100114
};

static struct gpio_event_platform_data starq_slide_data = {
	.name		= "slide_sensor",
	.info		= starq_slide_info,
	.info_count	= ARRAY_SIZE(starq_slide_info)
};

struct platform_device slide_device_starq = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &starq_slide_data,
	},
};

static int __init starq_init_slide(void)
{
	int rc = 0;
	rc = gpio_tlmm_config(GPIO_CFG(SLIDE_SENSOR_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	if (rc) {
		pr_err("%s: Could not configure gpio %d\n",
					 __func__, SLIDE_SENSOR_INT);
	}
	return platform_device_register(&slide_device_starq);
}

device_initcall(starq_init_slide);

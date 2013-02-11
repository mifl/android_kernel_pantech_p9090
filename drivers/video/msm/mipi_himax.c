/* Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_himax.h"
#include <mach/gpio.h>
#include <asm/irq.h>
#include <asm/system.h>

#define GPIO_HIGH_VALUE 1
#define GPIO_LOW_VALUE  0

#define NOP()	do {asm volatile ("NOP");} while(0);
#define DELAY_3NS() do { \
    asm volatile ("NOP"); \
    asm volatile ("NOP"); \
    asm volatile ("NOP");} while(0);

#define LCD_DEBUG_MSG

#ifdef LCD_DEBUG_MSG
#define ENTER_FUNC()        printk(KERN_INFO "[SKY_LCD] +%s \n", __FUNCTION__);
#define EXIT_FUNC()         printk(KERN_INFO "[SKY_LCD] -%s \n", __FUNCTION__);
#define ENTER_FUNC2()       printk(KERN_ERR "[SKY_LCD] +%s\n", __FUNCTION__);
#define EXIT_FUNC2()        printk(KERN_ERR "[SKY_LCD] -%s\n", __FUNCTION__);
#define PRINT(fmt, args...) printk(KERN_INFO fmt, ##args)
#define DEBUG_EN 1
#else
#define PRINT(fmt, args...)
#define ENTER_FUNC2()
#define EXIT_FUNC2()
#define ENTER_FUNC()
#define EXIT_FUNC()
#define DEBUG_EN 0
#endif

extern int gpio43, gpio16; /* gpio43 :reset, gpio16:lcd bl */

static struct msm_panel_common_pdata *mipi_himax_pdata;

static struct dsi_buf himax_tx_buf;
static struct dsi_buf himax_rx_buf;

struct lcd_state_type {
    boolean disp_powered_up;
    boolean disp_initialized;
    boolean disp_on;
};

static struct lcd_state_type himax_state = { 0, };
#if 0
char SETEXTC[4]   = {0xB9,0xFF,0x83,0x94};
char SETPOWER[16] = {0xB1,0x01,0x00,0x34,0x06,0x01,0x10 ,0x10,0x34,0x3C,0x2A,0x23,0x57,0x12,0x01,0xE6};
char INVOFF[1]    = {0x20};
char MADCTL[2]	  = {0x36,0x00};
char COLMOD[2]	  = {0x3A,0x70};
char SETCYC[19]	  = {0xB4,0x00,0x00,0x00,0x05,0x08 ,0x05,0x4C,0x04,0x05,0x4C,0x23,0x27,0x04,0xCA,0xCC,0x02,0x05,0x04};	
char SETPANEL[2]  = {0xCC,0x09};
char SETGAMMA[35] = {0xE0 ,0x01,0x08,0x0D,0x0E,0x18,0x36,0x13,0x26,0x4B,0x4E,0x52,0x55,0x05,0x15,0x15,0x0E,0x11,0x01,0x08,0x0D,0x15,0x1F,0x3D,0x17,0x29,0x4A,0x4E,0x51,0x53,0xD7,0x15,0x16,0x0E,0x11};
char SETGIP[25]   = {0xD5,0x00,0x00,0x00,0x01,0xCD,0x23,0xEf,0x45,0x67,0x89,0xAB,0xCC,0xCC,0xDC,0x10,0xFE,0x32,0xBA,0x98,0x76,0x54,0xCC,0xCC,0xC0};
char SETMIPI[2]   = {0xBA,0x03};
char SLPOUT[1]    = {0x11};
char DISPON[1]    = {0x29};

char DISPOFF[1]   = {0x28};
char SLPIN[1]    = {0x10};

static struct dsi_cmd_desc himax_display_init_cmds[]=
{
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(SETEXTC), SETEXTC},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(SETPOWER), SETPOWER},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(INVOFF), INVOFF},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(MADCTL), MADCTL},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(COLMOD), COLMOD},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(SETCYC), SETEXTC},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(SETPANEL), SETPANEL},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(SETGAMMA), SETGAMMA},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(SETGIP), SETGIP},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 5, sizeof(SETMIPI), SETMIPI},
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(SLPOUT), SLPOUT},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(DISPON), DISPON}
};
static struct dsi_cmd_desc himax_sleep_in_cmds[]=
{
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(DISPOFF), DISPOFF},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(SLPIN), SLPIN}
};
static struct dsi_cmd_desc himax_sleep_out_cmds[]=
{
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(SLPOUT), SLPOUT},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(DISPON), DISPON}
};

/*
 0. himax_display_init_cmds
 1. himax_display_veil_init0_cmds
 2. himax_display_veil_lut_cmds
 3. himax_display_veil_init1_cmds
 4. himax_display_veil_tex_cmds
 5. himax_display_veil_colormap_cmds
 6. himax_display_veil_init2_cmds
 7. dsi_cmd_desc himax_display_on_cmds
 */
#endif
char SLEEP_OUT[1] 			= {0x11};//long
char SET_HOR_ADDR[5] 	= {0x2A,0x00,0x00,0x01,0xDF};//long
char SET_VER_ADDR[5] 	= {0x2B,0x00,0x00,0x03,0x55};
char SET_ADDR_MODE[2] 	= {0x36,0x14};
char SET_PIX_FMT[2]		= {0x3A,0x77};
char DIS_ON[1]			= {0x29};


char DIS_OFF[1]			= {0x28};
char SLEEP_IN[1]		= {0x10};

static struct dsi_cmd_desc himax_display_init_cmds[]=
{
	{DTYPE_DCS_LWRITE, 1, 0, 0, 120, sizeof(SLEEP_OUT), SLEEP_OUT},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(SET_HOR_ADDR), SET_HOR_ADDR},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(SET_VER_ADDR), SET_VER_ADDR},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(SET_ADDR_MODE), SET_ADDR_MODE},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 150, sizeof(SET_PIX_FMT), SET_PIX_FMT},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(DIS_ON), DIS_ON},
};
static struct dsi_cmd_desc himax_sleep_in_cmds[]=
{
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(DIS_OFF), DIS_OFF},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 120, sizeof(SLEEP_IN), SLEEP_IN}
};
static struct dsi_cmd_desc himax_sleep_out_cmds[]=
{
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(SLEEP_OUT), SLEEP_OUT},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(DIS_ON), DIS_ON}
};


static int mipi_himax_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

    ENTER_FUNC2();

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (himax_state.disp_initialized == false) {
		//PRINT("[LIVED] LCD RESET!!\n");
        gpio_set_value_cansleep(gpio43, GPIO_LOW_VALUE); 
        usleep(10);
		gpio_set_value_cansleep(gpio43, GPIO_HIGH_VALUE);
		usleep(10);
		mipi_dsi_cmds_tx(mfd, &himax_tx_buf, himax_display_init_cmds,
				ARRAY_SIZE(himax_display_init_cmds));
		himax_state.disp_initialized = true;
		EXIT_FUNC2();
	}
	mipi_dsi_cmds_tx(mfd, &himax_tx_buf, himax_sleep_out_cmds,
			ARRAY_SIZE(himax_sleep_out_cmds));
	himax_state.disp_on = true;

	
	return 0;
}

static int mipi_himax_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

    ENTER_FUNC2();

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (himax_state.disp_on == true) {
		gpio_set_value_cansleep(gpio43, GPIO_LOW_VALUE);
		usleep(10);
		gpio_set_value_cansleep(gpio43, GPIO_HIGH_VALUE);
		usleep(10);

		mipi_dsi_cmds_tx(mfd, &himax_tx_buf, himax_sleep_in_cmds,
				ARRAY_SIZE(himax_sleep_in_cmds));
		himax_state.disp_on = false;
		himax_state.disp_initialized = false;
	    EXIT_FUNC2();
	}
      
	return 0;
}

static void mipi_himax_set_backlight(struct msm_fb_data_type *mfd)
{
	static int first_enable = 0;
	static int prev_bl_level = 0;
	int cnt, bl_level;
	//int count = 0;
	unsigned long flags;
	bl_level = mfd->bl_level;

	PRINT("[LIVED] set_backlight=%d,prev=%d\n", bl_level, prev_bl_level);
	if (bl_level == prev_bl_level || himax_state.disp_on == 0) {
		PRINT("[LIVED] same! or not disp_on\n");
	} else {
		if (bl_level == 0) {
			gpio_set_value_cansleep(gpio16 ,GPIO_LOW_VALUE);
			usleep(250);      // Disable hold time
			PRINT("[LIVED] same! or not disp_on\n");
		} else {
			if (prev_bl_level == 0) {
				//count++;
				gpio_set_value_cansleep(gpio16 ,GPIO_HIGH_VALUE);
				if (first_enable == 0) {
					first_enable = 1;
					msleep(25); // Initial enable time
				} else {
					udelay(300);      // Turn on time
				}
				//PRINT("[LIVED] (0) init!\n");
			}

			if (prev_bl_level < bl_level) {
				gpio_set_value_cansleep(gpio16 ,GPIO_LOW_VALUE);
				udelay(200);// TDIS
				cnt = BL_MAX - bl_level + 1;
			} else {
				cnt = prev_bl_level - bl_level;
			}
			//pr_info("[LIVED] cnt=%d, prev_bl_level=%d, bl_level=%d\n",
			//		cnt, prev_bl_level, bl_level);
			while (cnt) {
				local_save_flags(flags);
				local_irq_disable();
				gpio_set_value_cansleep(gpio16 ,GPIO_LOW_VALUE);
				DELAY_3NS();//udelay(3);      // Turn off time
				gpio_set_value_cansleep(gpio16 ,GPIO_HIGH_VALUE);
				local_irq_restore(flags);
				udelay(300);      // Turn on time
				cnt--;
			}

		}
		prev_bl_level = bl_level;
	}
}


static int __devinit mipi_himax_lcd_probe(struct platform_device *pdev)
{
    if (pdev->id == 0) {
        mipi_himax_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_himax_lcd_probe,
	.driver = {
		.name   = "mipi_himax",
	},
};

static struct msm_fb_panel_data himax_panel_data = {
       .on             = mipi_himax_lcd_on,
       .off            = mipi_himax_lcd_off,
       .set_backlight  = mipi_himax_set_backlight,
};

static int ch_used[3];

int mipi_himax_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_himax", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	himax_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &himax_panel_data,
		sizeof(himax_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init mipi_himax_lcd_init(void)
{
    ENTER_FUNC2();

    himax_state.disp_powered_up = true;

    mipi_dsi_buf_alloc(&himax_tx_buf, DSI_BUF_SIZE);
    mipi_dsi_buf_alloc(&himax_rx_buf, DSI_BUF_SIZE);

    EXIT_FUNC2();

    return platform_driver_register(&this_driver);
}

module_init(mipi_himax_lcd_init);

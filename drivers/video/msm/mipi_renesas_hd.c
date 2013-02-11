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
#include "mipi_renesas_hd.h"
#include <mach/gpio.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <mach/gpiomux.h>
#include <linux/mutex.h>
#define GPIO_HIGH_VALUE 1
#define GPIO_LOW_VALUE    0

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

DEFINE_MUTEX(bl_mutex);
#if (BOARD_VER < WS10)
#define FEATURE_SKY_BACKLIGHT_TPS61161
#else
#define FEATURE_RENESAS_HD_CABC_ON
#endif

#ifdef FEATURE_RENESAS_HD_CABC_ON
#define CABC_BACKLIGHT_CURRENT_DECREASE_RATIO 60
#endif

//#define MAX_BL_LEVEL 255

#ifdef FEATURE_SKY_BACKLIGHT_TPS61161
#define T_LOW_LB          8    //LOGIC 0  (T_HIGH_LB*2)
#define T_HIGH_LB         4    //LOGIC 0  (MIN 2us ~ 360us) 
#define T_LOW_HB          4    //LOGIC 1  (MIN 2us ~ 360us) 
#define T_HIGH_HB         8    //LOGIC 1  (T_LOW_HB*2)
#define T_START           4    //2us
#define T_EOS             4    //2us
#define T_ES_DELAY      200    //100us
#define T_ES_DETECT     500    //260us
#define T_ES_WIN          1    //1ms
#define T_SHUTDOWN        5    //5ms 
#define NUM_ADDR_DIGIT    8    //0x72 
#define NUM_DATA_DIGIT    8    //RFA(7)ADD1(6)ADD0(5)DATA(4:1)
#define DEVICE_ADDR    0x72
static int first_enable = 0;
#endif

static int prev_bl_level = 0;

extern int gpio43, gpio15, gpio24; // gpio15 - LCD_BL_EN, gpio24 - LCD_BL_CTL,  gpio43 - MIPI_DSI0_RESET_N
extern int charger_flag;

static struct msm_panel_common_pdata *mipi_renesas_hd_pdata;
static struct dsi_buf renesas_hd_tx_buf;
static struct dsi_buf renesas_hd_rx_buf;

struct lcd_state_type {
    boolean disp_powered_up;
    boolean disp_initialized;
    boolean disp_on;
    boolean first_light;
};

static struct lcd_state_type renesas_hd_state = { 0, };

// POWER ON
char SLEEP_OUT[1]               = {0x11};
char DISP_ON[1]                 = {0x29};
char PROTECT_UNLOCK[2]          = {0xB0, 0x00};
#if (BOARD_VER < WS10)
char GAMMA_CTRL[5]              = {0xC8, 0x00, 0x00, 0x1F, 0x00};
char GAMMA_CTRL_RED_P[14]       = {0xC9, 0x2D, 0x03, 0x01, 0x01, 0x02, 0x05, 0x11, 0x18, 0x10, 0x0C, 0x1F, 0x10, 0x20}; 
char GAMMA_CTRL_RED_N[14]       = {0xCA, 0x01, 0x2B, 0x50, 0x4F, 0x4E, 0x49, 0x3F, 0x36, 0x3E, 0x41, 0x34, 0x29, 0x19};
char GAMMA_CTRL_GREEN_P[14]     = {0xCB, 0x29, 0x21, 0x1F, 0x1E, 0x1A, 0x13, 0x19, 0x1C, 0x15, 0x11, 0x24, 0x14, 0x20};
char GAMMA_CTRL_GREEN_N[14]     = {0xCC, 0x09, 0x11, 0x32, 0x33, 0x36, 0x3D, 0x36, 0x33, 0x39, 0x3C, 0x2C, 0x21, 0x19}; 
char GAMMA_CTRL_BLUE_P[14]      = {0xCD, 0x17, 0x33, 0x32, 0x2A, 0x26, 0x14, 0x1B, 0x1F, 0x19, 0x16, 0x26, 0x10, 0x20};
char GAMMA_CTRL_BLUE_N[14]      = {0xCE, 0x25, 0x00, 0x1F, 0x26, 0x2A, 0x3B, 0x34, 0x2E, 0x34, 0x38, 0x28, 0x22, 0x19};
char PWR_SET_1[8]               = {0xD0, 0x6C, 0x65, 0x09, 0x18, 0x5F, 0x00, 0x14};
char VPLVL_VNLVL_SET[3]         = {0xD5, 0x09, 0x09};
#endif
#ifdef FEATURE_RENESAS_HD_CABC_ON
#if (CABC_BACKLIGHT_CURRENT_DECREASE_RATIO == 60)
char CABC_MAKER_SET_1[16]       = {0xB7, 0x18, 0x00, 0x18, 0x18, 0x0C, 0x12, 0x6C, 0x11, 0x6C, 0x12, 0x0C, 0x12, 0x00, 0x10, 0x00};
char CABC_MAKER_SET_2[14]       = {0xB8, 0xF8, 0xDA, 0x6D, 0xFB, 0xFF, 0xFF, 0xCF, 0x1F, 0x67, 0xA3, 0xDB, 0xFB, 0xFF};
char CABC_USER_PARAM[13]        = {0xBE, 0xFF, 0x0F, 0x00, 0x30, 0x18, 0x18, 0x00, 0x5D, 0x00, 0x00, 0x80, 0x32};
#elif (CABC_BACKLIGHT_CURRENT_DECREASE_RATIO == 45)
char CABC_MAKER_SET_1[16]       = {0xB7, 0x18, 0x00, 0x18, 0x18, 0x0C, 0x0E, 0x6C, 0x0E, 0x6C, 0x0E, 0x0C, 0x0E, 0x00, 0x10, 0x00};
char CABC_MAKER_SET_2[14]       = {0xB8, 0xF8, 0xDA, 0x6D, 0xFB, 0xFF, 0xFF, 0xCF, 0x1F, 0x8C, 0xD2, 0xFF, 0xFF, 0xFF};
char CABC_USER_PARAM[13]        = {0xBE, 0xFF, 0x0F, 0x00, 0x3F, 0x18, 0x18, 0x00, 0x5D, 0x00, 0x00, 0x80, 0x32};
#elif (CABC_BACKLIGHT_CURRENT_DECREASE_RATIO == 30)
char CABC_MAKER_SET_1[16]       = {0xB7, 0x18, 0x00, 0x18, 0x18, 0x0C, 0x0C, 0x6C, 0x0C, 0x6C, 0x0C, 0x0C, 0x0C, 0x00, 0x10, 0x00};
char CABC_MAKER_SET_2[14]       = {0xB8, 0xF8, 0xDA, 0x6D, 0xFB, 0xFF, 0xFF, 0xCF, 0x1F, 0xB3, 0xFB, 0xFF, 0xFF, 0xFF};
char CABC_USER_PARAM[13]        = {0xBE, 0xFF, 0x0F, 0x00, 0x3F, 0x18, 0x18, 0x00, 0x5D, 0x00, 0x00, 0x80, 0x32};
#elif (CABC_BACKLIGHT_CURRENT_DECREASE_RATIO == 15)
char CABC_MAKER_SET_1[16]       = {0xB7, 0x18, 0x00, 0x18, 0x18, 0x0C, 0x0B, 0x4C, 0x0B, 0x6C, 0x0B, 0x0C, 0x0B, 0x00, 0x10, 0x00};
char CABC_MAKER_SET_2[14]       = {0xB8, 0xF8, 0xDA, 0x6D, 0xFB, 0xFF, 0xFF, 0xCF, 0x1F, 0xD9, 0xFF, 0xFF, 0xFF, 0xFF};
char CABC_USER_PARAM[13]        = {0xBE, 0xFF, 0x0F, 0x00, 0x3F, 0x18, 0x18, 0x00, 0x5D, 0x00, 0x00, 0x80, 0x32};
#else
#endif
char CABC_ON[2]                 = {0xBB, 0x0B};
char PWM_DIM_ITRT_CTRL[2]       = {0xBC, 0x00};
#endif

// POWER OFF
char DISP_OFF[1]                = {0x28};
char SLEEP_IN[1]                = {0x10};

static struct dsi_cmd_desc renesas_hd_display_init_cmds[]=
{
	{DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(SLEEP_OUT), SLEEP_OUT},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(PROTECT_UNLOCK), PROTECT_UNLOCK}
#if (BOARD_VER < WS10)
	,{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(GAMMA_CTRL),GAMMA_CTRL},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(GAMMA_CTRL_RED_P),GAMMA_CTRL_RED_P},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(GAMMA_CTRL_RED_N),GAMMA_CTRL_RED_N},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(GAMMA_CTRL_GREEN_P),GAMMA_CTRL_GREEN_P},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(GAMMA_CTRL_GREEN_N),GAMMA_CTRL_GREEN_N},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(GAMMA_CTRL_BLUE_P),GAMMA_CTRL_BLUE_P},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(GAMMA_CTRL_BLUE_N),GAMMA_CTRL_BLUE_N},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(PWR_SET_1),PWR_SET_1},	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(VPLVL_VNLVL_SET),VPLVL_VNLVL_SET}
#endif
#ifdef FEATURE_RENESAS_HD_CABC_ON
	,{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CABC_MAKER_SET_1),CABC_MAKER_SET_1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CABC_MAKER_SET_2),CABC_MAKER_SET_2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CABC_ON),CABC_ON},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(PWM_DIM_ITRT_CTRL),PWM_DIM_ITRT_CTRL},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CABC_USER_PARAM), CABC_USER_PARAM}
#endif
};

static struct dsi_cmd_desc renesas_hd_screen_on_cmds[]=
{
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(DISP_ON), DISP_ON}
};

static struct dsi_cmd_desc renesas_hd_screen_off_cmds[]=
{
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(DISP_OFF), DISP_OFF},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(SLEEP_IN), SLEEP_IN}
};

#if defined(FEATURE_RENESAS_HD_CABC_ON) 
static struct dsi_cmd_desc renesas_hd_bl_cmds[]=
{
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(CABC_USER_PARAM), CABC_USER_PARAM}
};

static struct dsi_cmd_desc renesas_hd_sleep_in_cmds[]=
{
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(DISP_OFF), DISP_OFF},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(SLEEP_IN), SLEEP_IN}
};

static struct dsi_cmd_desc renesas_hd_sleep_out_cmds[]=
{
	{DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(SLEEP_OUT), SLEEP_OUT},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(DISP_ON), DISP_ON}
};
#endif

static int mipi_renesas_hd_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

        //ENTER_FUNC2();

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_set_tx_power_mode(0);
	
	if (renesas_hd_state.disp_initialized == false) {
		mdelay(1);
		gpio_set_value_cansleep(gpio43, GPIO_LOW_VALUE);  
	   	mdelay(1);
		gpio_set_value_cansleep(gpio43, GPIO_HIGH_VALUE);
	   	mdelay(5);

		mipi_dsi_cmds_tx(mfd, &renesas_hd_tx_buf, renesas_hd_display_init_cmds,
				ARRAY_SIZE(renesas_hd_display_init_cmds));

		mdelay(34);
		
		renesas_hd_state.disp_initialized = true;
	}

	mipi_dsi_cmds_tx(mfd, &renesas_hd_tx_buf, renesas_hd_screen_on_cmds,
			ARRAY_SIZE(renesas_hd_screen_on_cmds));

	mipi_set_tx_power_mode(1);
	
	renesas_hd_state.disp_on = true;
	renesas_hd_state.first_light =true;
	EXIT_FUNC2();

	return 0;
}

static int mipi_renesas_hd_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	//ENTER_FUNC2();

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (renesas_hd_state.disp_on == true) {
/*	   	mdelay(1);
		gpio_set_value_cansleep(gpio43, GPIO_LOW_VALUE);  
	   	mdelay(1);
		gpio_set_value_cansleep(gpio43, GPIO_HIGH_VALUE);
	   	mdelay(5);*/
		
		mipi_set_tx_power_mode(0);
	
		mipi_dsi_cmds_tx(mfd, &renesas_hd_tx_buf, renesas_hd_screen_off_cmds,
				ARRAY_SIZE(renesas_hd_screen_off_cmds));

//		gpio_set_value_cansleep(gpio43, GPIO_HIGH_VALUE);  
		gpio_set_value_cansleep(gpio43, GPIO_LOW_VALUE);  
	   	mdelay(5);

		mipi_set_tx_power_mode(1);
		
		renesas_hd_state.disp_on = false;
		renesas_hd_state.disp_initialized = false;
	}
	
	renesas_hd_state.first_light =false;
	EXIT_FUNC2();
	
	return 0;
}

static void mipi_renesas_hd_set_backlight(struct msm_fb_data_type *mfd)
{
#ifdef FEATURE_SKY_BACKLIGHT_TPS61161
	int idx;
	unsigned long flags = 0;
	int bl_level;
	
	//ENTER_FUNC2();
			
	bl_level = mfd->bl_level | 0x00;

	local_save_flags(flags);
  	local_irq_disable();
			
	if (bl_level == 0) {
		//gpio_set_value_cansleep(gpio15, GPIO_LOW_VALUE);
		gpio_set_value_cansleep(gpio24, GPIO_LOW_VALUE);
		mdelay(T_SHUTDOWN); 
		first_enable = 0;
	} else {
		if (first_enable == 0) {	
			//gpio_set_value_cansleep(gpio15, GPIO_HIGH_VALUE);
			local_save_flags(flags);
			local_irq_disable();
					
			gpio_set_value_cansleep(gpio24, GPIO_HIGH_VALUE);
			udelay(T_ES_DELAY); 
			gpio_set_value_cansleep(gpio24, GPIO_LOW_VALUE);
			udelay(T_ES_DETECT);
			gpio_set_value_cansleep(gpio24, GPIO_HIGH_VALUE);
			mdelay(T_ES_WIN); 

			first_enable = 1;
		}

		//8bit Device Address
		gpio_set_value_cansleep(gpio24, GPIO_HIGH_VALUE);
		udelay(T_START);
		for (idx=0; idx<NUM_ADDR_DIGIT; idx++) {
			uint8 bit = ((DEVICE_ADDR << idx) >>(NUM_ADDR_DIGIT-1)) & 0x01;
		
			if (bit == 1) {
				gpio_set_value_cansleep(gpio24, GPIO_LOW_VALUE);
				udelay(T_LOW_HB);
				gpio_set_value_cansleep(gpio24, GPIO_HIGH_VALUE);
				udelay(T_HIGH_HB);	
			} else {
				gpio_set_value_cansleep(gpio24, GPIO_LOW_VALUE);
				udelay(T_LOW_LB);
				gpio_set_value_cansleep(gpio24, GPIO_HIGH_VALUE);
				udelay(T_HIGH_LB);	
			}
		}
		gpio_set_value_cansleep(gpio24, GPIO_LOW_VALUE);
		udelay(T_EOS);
		
		// 5bit Data
		gpio_set_value_cansleep(gpio24, GPIO_HIGH_VALUE);
		udelay(T_START);
	
		for (idx=0; idx<NUM_DATA_DIGIT; idx++) {						  
			uint8 bit = (( bl_level<< idx) >>(NUM_DATA_DIGIT-1)) & 0x01;
			if (bit == 1) {
				gpio_set_value_cansleep(gpio24, GPIO_LOW_VALUE);
				udelay(T_LOW_HB);
				gpio_set_value_cansleep(gpio24, GPIO_HIGH_VALUE);
				udelay(T_HIGH_HB);	
			} else {
				gpio_set_value_cansleep(gpio24, GPIO_LOW_VALUE);
				udelay(T_LOW_LB);
				gpio_set_value_cansleep(gpio24, GPIO_HIGH_VALUE);
				udelay(T_HIGH_LB);	

			}
		}
		gpio_set_value_cansleep(gpio24, GPIO_LOW_VALUE);
		udelay(T_EOS);
		gpio_set_value_cansleep(gpio24, GPIO_HIGH_VALUE);
		
		local_irq_restore(flags);
	} 		 
			
	prev_bl_level = bl_level;

	local_irq_restore(flags);
			
#elif defined (FEATURE_RENESAS_HD_CABC_ON)
	int bl_level;

        //ENTER_FUNC2();

/* shinjg 
	if (prev_bl_level == mfd->bl_level)
		return;
*/ 
	bl_level = mfd->bl_level;
	//if(bl_level > MAX_BL_LEVEL)
	//	bl_level = MAX_BL_LEVEL;

	//printk("\nmipi_renesas_hd_set_backlight bl_level = %d\n",bl_level);

//	mutex_lock(&mfd->dma->ov_mutex);
	mutex_lock(&bl_mutex);
	mipi_set_tx_power_mode(0);
	
/*	if(first_enable == 0) {
	    	gpio_set_value_cansleep(gpio15, GPIO_HIGH_VALUE);
	   	first_enable = 1;
	}*/
	
	switch(bl_level) {
		case 10: CABC_USER_PARAM[1] = 249; break;
		case 9: CABC_USER_PARAM[1] = 225; break;
		case 8:	CABC_USER_PARAM[1] = 201; break;
		case 7:	CABC_USER_PARAM[1] = 177; break;
		case 6:	CABC_USER_PARAM[1] = 153; break;
		case 5:	CABC_USER_PARAM[1] = 129; break;
		case 4:	CABC_USER_PARAM[1] = 105; break;
		case 3:	CABC_USER_PARAM[1] = 81; break;
		case 2:	CABC_USER_PARAM[1] = 57; break;
		case 1:	CABC_USER_PARAM[1] = 35; break;
		case 0:
			CABC_USER_PARAM[1] = 0;   	
			mipi_dsi_cmds_tx(mfd, &renesas_hd_tx_buf, renesas_hd_sleep_in_cmds,
				ARRAY_SIZE(renesas_hd_sleep_in_cmds));
        		//first_enable = 0;
			break;
		case -1:
			if(prev_bl_level == 1) 
				CABC_USER_PARAM[1] = 20;
			else 
				CABC_USER_PARAM[1] = 35;
			break;
		default:
			break;
	}	

	printk("\n[%s] CABC_USER_PARAM = %d\n", __func__, CABC_USER_PARAM[1]);
		
	if(/*charger_flag == 1 &&*/prev_bl_level == 0 && bl_level > 0) {
		mipi_dsi_cmds_tx(mfd, &renesas_hd_tx_buf, renesas_hd_sleep_out_cmds,
                                ARRAY_SIZE(renesas_hd_sleep_out_cmds));
	}
	mipi_dsi_cmds_tx(mfd, &renesas_hd_tx_buf, renesas_hd_bl_cmds, 
		ARRAY_SIZE(renesas_hd_bl_cmds));

	mipi_set_tx_power_mode(1);
			
	prev_bl_level = bl_level;
//	mutex_unlock(&mfd->dma->ov_mutex);
	mutex_unlock(&bl_mutex);
	
#endif /*FEATURE_SKY_BACKLIGHT_TPS61165*/

}


static int __devinit mipi_renesas_hd_lcd_probe(struct platform_device *pdev)
{	
	if (pdev->id == 0) {
        mipi_renesas_hd_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_renesas_hd_lcd_probe,
	.driver = {
		.name   = "mipi_renesas_hd",
	},
};

static struct msm_fb_panel_data renesas_hd_panel_data = {
       .on             = mipi_renesas_hd_lcd_on,
       .off            = mipi_renesas_hd_lcd_off,
       .set_backlight  = mipi_renesas_hd_set_backlight,
};

static int ch_used[3];

int mipi_renesas_hd_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_renesas_hd", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	renesas_hd_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &renesas_hd_panel_data,
		sizeof(renesas_hd_panel_data));
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

static int __init mipi_renesas_hd_lcd_init(void)
{
    ENTER_FUNC2();

    renesas_hd_state.disp_powered_up = true;

    mipi_dsi_buf_alloc(&renesas_hd_tx_buf, DSI_BUF_SIZE);
    mipi_dsi_buf_alloc(&renesas_hd_rx_buf, DSI_BUF_SIZE);

    mutex_init(&bl_mutex);
    EXIT_FUNC2();

    return platform_driver_register(&this_driver);
}

module_init(mipi_renesas_hd_lcd_init);


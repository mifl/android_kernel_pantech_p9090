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
#include "mipi_ortus.h"
#include <mach/gpio.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/kernel.h>

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

#define FEATURE_SKY_BACKLIGHT_TPS61161

#ifdef FEATURE_SKY_BACKLIGHT_TPS61161
//#define LCD_BL_EN        70    //gpio24 
//#define BL_MAX           31    //BL LEVEL

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

#endif
//test
#ifdef FEATURE_SKY_BACKLIGHT_TPS61161
static int first_enable = 0;
static int prev_bl_level = 0;
#endif

extern int gpio43, gpio16, gpio24;; /* gpio43 :reset, gpio16:lcd bl */

static struct msm_panel_common_pdata *mipi_ortus_pdata;
static struct dsi_buf ortus_tx_buf;
static struct dsi_buf ortus_rx_buf;



static struct kobject *panel_brightness_kobj = NULL;
static int panel_data,panel_ch;

struct lcd_state_type {
    boolean disp_powered_up;
    boolean disp_initialized;
    boolean disp_on;
	boolean first_light;
};

enum{
	OFF =0,
	USER_IMAGE,
	PICTURE_IMG,
	MOVING_IMG

};



static struct lcd_state_type ortus_state = { 0, };


char SETEXTC[4]   = {0xB9,0xFF,0x83,0x94};

char SETPOWER[16] = {0xB1,0x7C,0x00,0x24,
					 0x06,0x01,0x10 ,0x10,
					 0x34,0x3C,0x2A,0x23,
					 0x57,0x12,0x01,0xE6};//0x34 ->0x24

char INVOFF[1]    = {0x20};

char MADCTL[2]	  = {0x36,0x00};

char COLMOD[2]	  = {0x3A,0x70};

char SETCYC[19]	  = {0xB4,0x00,0x00,0x00,
					 0x05,0x08,0x05,0x4C,
					 0x04,0x05,0x4C,0x23,
					 0x27,0x26,0xCA,0xCC,
					 0x02,0x05,0x04};	



char SETGIP[25]   = {0xD5,0x00,0x00,0x00,
					 0x01,0xCD,0x23,0xEf,
					 0x45,0x67,0x89,0xAB,
					 0xCC,0xCC,0xDC,0x10,
					 0xFE,0x32,0xBA,0x98,
					 0x76,0x54,0xCC,0xCC,
					 0xC0};

#if 0
char SETGAMMA[35] = {0xE0,0x01,0x08,0x0D,
					 0x0E,0x18,0x36,0x13,
					 0x26,0x4B,0x4E,0x52,
					 0x55,0x05,0x15,0x15,
					 0x0E,0x11,0x01,0x08,
					 0x0D,0x15,0x1F,0x3D,
					 0x17,0x29,0x4A,0x4E,
					 0x51,0x53,0xD7,0x15,
					 0x16,0x0E,0x11};
#else
char SETGAMMA[35] = {0xE0,0x01,0x0A,0x13,
					0x0E,0x17,0x33,0x1C,
					0x28,0x44,0x4C,0x91,
					0x56,0x97,0x16,0x16,
					0x0E,0x11,0x01,0x0A,
					0x14,0x15,0x1F,0x3F,
					0x1E,0x2B,0x44,0x4D,
					0x91,0x55,0x98,0x16,
					0x16,0x10,0x11};

#endif

char SETCABC[10] = {0xc9,0x0F,0x00,0x1E,
					0x1E,0x00,0x00,0x00,
					0x01,0x3E};

char WRDISBV[2]  = {0x51,0x00};//0xE4
char WRCTRLD[2]  = {0x53,0x24};//0x24
char SETVDD[2]  = {0xBC,0x07};//0x24

char WRCABC[2]   = {0x55,0x03};


char SETPANEL[2]  = {0xCC,0x09};

char SETMIPI[2]   = {0xBA,0x03};
char SLPOUT[1]    = {0x11};
char DISPON[1]    = {0x29};
char DISPOFF[1]   = {0x28};
char SLPIN[1]     = {0x10};


static struct dsi_cmd_desc ortus_display_init_cmds[]=
{
	{DTYPE_DCS_WRITE,  1, 0, 0, 200, sizeof(SLPOUT), SLPOUT},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(SETEXTC), SETEXTC},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(SETPOWER), SETPOWER},
	{DTYPE_DCS_WRITE,  1, 0, 0, 0, sizeof(INVOFF), INVOFF},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(MADCTL), MADCTL},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(COLMOD), COLMOD},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(SETCYC), SETCYC},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(SETGIP), SETGIP},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(SETGAMMA), SETGAMMA},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(SETCABC), SETCABC},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(WRDISBV),WRDISBV },
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(WRCTRLD), WRCTRLD},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 5, sizeof(SETVDD), SETVDD},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 5, sizeof(WRCABC), WRCABC},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 50, sizeof(SETPANEL), SETPANEL},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(SETMIPI), SETMIPI}

};

static struct dsi_cmd_desc ortus_sleep_in_cmds[]=
{
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(DISPOFF), DISPOFF},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(SLPIN), SLPIN}
};
static struct dsi_cmd_desc ortus_sleep_out_cmds[]=
{
	//{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(SLPOUT), SLPOUT},
	{DTYPE_DCS_WRITE, 1, 0, 0, 50, sizeof(DISPON), DISPON}
};

static struct dsi_cmd_desc ortus_bl_cmds[]=
{
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(WRDISBV),WRDISBV }, //pwm
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(WRCTRLD), WRCTRLD}, //ctl
	{DTYPE_DCS_WRITE1, 1, 0, 0, 5, sizeof(WRCABC), WRCABC}   //user
};


static ssize_t backlight_choice_show(struct  kobject *kobj, struct kobj_attribute  *attr, char *buf)
{
	return sprintf(buf, "%d\n", panel_ch);
}


static ssize_t backlight_choice_store(struct  kobject *kobj, struct kobj_attribute  *attr, const char *buf, size_t count)
{	
	
	sscanf(buf, "%du", &panel_ch);

	if(panel_ch == FALSE)
		WRCTRLD[1] |= (1<<2);
	else if(panel_ch == TRUE)
		WRCTRLD[1] &= (0<<2);

	return count;
}

static ssize_t panel_brightness_show(struct  kobject *kobj, struct kobj_attribute  *attr, char *buf)
{
	return sprintf(buf, "%d\n", panel_data);
}


static ssize_t panel_brightness_store(struct  kobject *kobj, struct kobj_attribute  *attr, const char *buf, size_t count)
{	
	
	sscanf(buf, "%du", &panel_data);

	if(panel_data == OFF){
		WRCABC[1] = OFF;
	}
	else if(panel_data == USER_IMAGE){
		WRCABC[1] = USER_IMAGE;
	}
	else if(panel_data ==PICTURE_IMG){
		WRCABC[1] = PICTURE_IMG;
	}
	else if(panel_data == MOVING_IMG){
		WRCABC[1] = MOVING_IMG;
	}
	printk(KERN_WARNING"[%s] WRCABC[1] = %d\n",__func__,WRCABC[1]);
	
	return count;
}


static struct kobj_attribute panel_brightness_attribute  = 
				__ATTR(panel_back, 0666, panel_brightness_show, panel_brightness_store);

static struct kobj_attribute back_choice_attribute  = 
				__ATTR(back_ch, 0666, backlight_choice_show, backlight_choice_store);

static struct attribute  *attrs[] = {
    &panel_brightness_attribute.attr,
	&back_choice_attribute.attr,
    NULL,   
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};

static int mipi_ortus_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

    ENTER_FUNC2();

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (ortus_state.disp_initialized == false) {
		
		mipi_dsi_cmds_tx(mfd, &ortus_tx_buf, ortus_display_init_cmds,
				ARRAY_SIZE(ortus_display_init_cmds));
		ortus_state.disp_initialized = true;
	}
	mipi_dsi_cmds_tx(mfd, &ortus_tx_buf, ortus_sleep_out_cmds,
			ARRAY_SIZE(ortus_sleep_out_cmds));
	ortus_state.disp_on = true;
	ortus_state.first_light =true;
	EXIT_FUNC2();
	printk("CABC-TEST WRCABC = 0x%x\n",WRCABC[1]);
	return 0;
}

static int mipi_ortus_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

    ENTER_FUNC2();

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (ortus_state.disp_on == true) {
	

		mipi_dsi_cmds_tx(mfd, &ortus_tx_buf, ortus_sleep_in_cmds,
				ARRAY_SIZE(ortus_sleep_in_cmds));
		ortus_state.disp_on = false;
		ortus_state.disp_initialized = false;
	}
    EXIT_FUNC2();
	ortus_state.first_light =false;
	return 0;
}


void CABC_control(struct msm_fb_data_type *mfd)
{
	unsigned long flags;

	
	if(!mfd)
		return;
	if(mfd->key != MFD_KEY)
		return;


	if(ortus_state.first_light){
		gpio_set_value_cansleep(gpio16, GPIO_HIGH_VALUE);
		ortus_state.first_light =false;
	}
	
	mipi_set_tx_power_mode(0);
	local_save_flags(flags);
	local_irq_disable();
	
	switch(mfd->bl_level){
		case 16 : WRDISBV[1] = 255;
				  mipi_dsi_cmds_tx(mfd, &ortus_tx_buf, ortus_bl_cmds,
						ARRAY_SIZE(ortus_bl_cmds));
				  break;
		case 15 : WRDISBV[1] = 240;
				  mipi_dsi_cmds_tx(mfd, &ortus_tx_buf, ortus_bl_cmds,
						ARRAY_SIZE(ortus_bl_cmds));
				  break;
		case 14 : WRDISBV[1] = 224;
				  mipi_dsi_cmds_tx(mfd, &ortus_tx_buf, ortus_bl_cmds,
						ARRAY_SIZE(ortus_bl_cmds));
				  break;
		case 13 : WRDISBV[1] = 208;
				  mipi_dsi_cmds_tx(mfd, &ortus_tx_buf, ortus_bl_cmds,
						ARRAY_SIZE(ortus_bl_cmds));
				  break;
		case 12 : WRDISBV[1] = 192;
				  mipi_dsi_cmds_tx(mfd, &ortus_tx_buf, ortus_bl_cmds,
						ARRAY_SIZE(ortus_bl_cmds));
				  break;
		case 11 : WRDISBV[1] = 176;
				  mipi_dsi_cmds_tx(mfd, &ortus_tx_buf, ortus_bl_cmds,
						ARRAY_SIZE(ortus_bl_cmds));
				  break;
		case 10 : WRDISBV[1] = 160;
				  mipi_dsi_cmds_tx(mfd, &ortus_tx_buf, ortus_bl_cmds,
						ARRAY_SIZE(ortus_bl_cmds));
				  break;
		case 9 : WRDISBV[1] = 144;
				  mipi_dsi_cmds_tx(mfd, &ortus_tx_buf, ortus_bl_cmds,
						ARRAY_SIZE(ortus_bl_cmds));
				  break;
		case 8 : WRDISBV[1] = 128;
				  mipi_dsi_cmds_tx(mfd, &ortus_tx_buf, ortus_bl_cmds,
						ARRAY_SIZE(ortus_bl_cmds));
				  break;
		case 7 : WRDISBV[1] = 112;
				  mipi_dsi_cmds_tx(mfd, &ortus_tx_buf, ortus_bl_cmds,
						ARRAY_SIZE(ortus_bl_cmds));
				  break;
		case 6 : WRDISBV[1] = 96;
				  mipi_dsi_cmds_tx(mfd, &ortus_tx_buf, ortus_bl_cmds,
						ARRAY_SIZE(ortus_bl_cmds));
				  break;
		case 5 : WRDISBV[1] = 80;
				  mipi_dsi_cmds_tx(mfd, &ortus_tx_buf, ortus_bl_cmds,
						ARRAY_SIZE(ortus_bl_cmds));
				  break;
		case 4 : WRDISBV[1] = 64;
				  mipi_dsi_cmds_tx(mfd, &ortus_tx_buf, ortus_bl_cmds,
						ARRAY_SIZE(ortus_bl_cmds));
				  break;
		case 3 : WRDISBV[1] = 48;
				  mipi_dsi_cmds_tx(mfd, &ortus_tx_buf, ortus_bl_cmds,
						ARRAY_SIZE(ortus_bl_cmds));
				  break;
		case 2 : WRDISBV[1] = 32;
				  mipi_dsi_cmds_tx(mfd, &ortus_tx_buf, ortus_bl_cmds,
						ARRAY_SIZE(ortus_bl_cmds));
				  break;
		case 1 : WRDISBV[1] = 16;
				  mipi_dsi_cmds_tx(mfd, &ortus_tx_buf, ortus_bl_cmds,
						ARRAY_SIZE(ortus_bl_cmds));
				  break;
		default : WRDISBV[1] = 0;
				  mipi_dsi_cmds_tx(mfd, &ortus_tx_buf, ortus_bl_cmds,
						ARRAY_SIZE(ortus_bl_cmds));
				  break;
		}
	
	local_irq_restore(flags);
	mipi_set_tx_power_mode(1);
	printk(KERN_WARNING"[%s] = %d\n",__func__,WRDISBV[1]);

}


void TPS61161(struct msm_fb_data_type *mfd)
{

#ifdef FEATURE_SKY_BACKLIGHT_TPS61161
		int idx;
		unsigned long flags;
		int bl_level;
	
		bl_level = mfd->bl_level | 0x00;
	
		//printk("mipi_rohm_set_backlight bl_level =%d, first_enable =%d, prev_bl_level =%d\n",bl_level, first_enable, prev_bl_level);
	
		if (prev_bl_level == 0) {
			mdelay(200);
		}
	
		if (bl_level == 0) {
			gpio_set_value_cansleep(gpio24, GPIO_LOW_VALUE);
			gpio_set_value_cansleep(gpio16, GPIO_LOW_VALUE);
			mdelay(T_SHUTDOWN); 
			first_enable =0;
		} else {
			if (first_enable == 0) {		  
				gpio_set_value_cansleep(gpio16, GPIO_HIGH_VALUE);	
				local_save_flags(flags);
				local_irq_disable();
				gpio_set_value_cansleep(gpio24, GPIO_HIGH_VALUE);
				udelay(T_ES_DELAY); 
				gpio_set_value_cansleep(gpio24, GPIO_LOW_VALUE);
				udelay(T_ES_DETECT);
				gpio_set_value_cansleep(gpio24, GPIO_HIGH_VALUE);
				mdelay(T_ES_WIN); 
	
				gpio_set_value_cansleep(gpio24, GPIO_HIGH_VALUE);
				udelay(T_START);
				for (idx=0; idx<NUM_ADDR_DIGIT; idx++) {
					uint8 bit = ((DEVICE_ADDR << idx) >>(NUM_ADDR_DIGIT-1)) & 0x01 ;
	
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
					uint8 bit = (( bl_level<< idx) >>(NUM_DATA_DIGIT-1)) & 0x01 ;
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
				first_enable = 1;
			} else {
	
				//8bit device Address
				local_save_flags(flags);
				local_irq_disable();
				gpio_set_value_cansleep(gpio24, GPIO_HIGH_VALUE);
				udelay(T_START);
				for (idx=0; idx<NUM_ADDR_DIGIT; idx++) {
					uint8 bit = ((DEVICE_ADDR << idx) >>(NUM_ADDR_DIGIT-1)) & 0x01 ;
	
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
					uint8 bit = (( bl_level<< idx) >>(NUM_DATA_DIGIT-1)) & 0x01 ;
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
		}
	
		prev_bl_level = bl_level;
		
#endif /*FEATURE_SKY_BACKLIGHT_TPS61165*/

}
static void mipi_ortus_set_backlight(struct msm_fb_data_type *mfd)
{

	if(panel_ch == FALSE){
		CABC_control(mfd);
	}
	else if(panel_ch == TRUE){
		TPS61161(mfd);
	}

}


static int __devinit mipi_ortus_lcd_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int retval;

	
	panel_brightness_kobj = kobject_create_and_add("panel_backlight",&dev->kobj);
	printk(KERN_DEBUG "%s : kobject create failed \n",__func__);
	if (!panel_brightness_kobj){
        return -ENOMEM;
	}

	retval = sysfs_create_group(panel_brightness_kobj,  &attr_group);

	if(retval){
		printk(KERN_DEBUG "%s : sysfs create file failed\n",__func__);
		kobject_put(panel_brightness_kobj);
	}
	
	if (pdev->id == 0) {
        mipi_ortus_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_ortus_lcd_probe,
	.driver = {
		.name   = "mipi_ortus",
	},
};

static struct msm_fb_panel_data ortus_panel_data = {
       .on             = mipi_ortus_lcd_on,
       .off            = mipi_ortus_lcd_off,
       .set_backlight  = mipi_ortus_set_backlight,
};

static int ch_used[3];

int mipi_ortus_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_ortus", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	ortus_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &ortus_panel_data,
		sizeof(ortus_panel_data));
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

static int __init mipi_ortus_lcd_init(void)
{
    ENTER_FUNC2();

    ortus_state.disp_powered_up = true;

    mipi_dsi_buf_alloc(&ortus_tx_buf, DSI_BUF_SIZE);
    mipi_dsi_buf_alloc(&ortus_rx_buf, DSI_BUF_SIZE);

    EXIT_FUNC2();

    return platform_driver_register(&this_driver);
}

module_init(mipi_ortus_lcd_init);


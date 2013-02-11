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
#include "mipi_samsung_octa.h"
#include <mach/gpio.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include "mdp4.h"

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

extern int gpio43;

struct lcd_state_type {
    boolean disp_powered_up;
    boolean disp_initialized;
    boolean disp_on;
	boolean backlightoff;
};

int chargerFlag = 0;
static struct lcd_state_type oscar_state = { 0, };


static struct msm_panel_common_pdata *mipi_oscar_pdata;
static struct dsi_buf oscar_tx_buf;
static struct dsi_buf oscar_rx_buf;


char Gamma_Set300[26] = {0xfa,0x02,0x58,0x42,0x66,0xAA,0xF1,0xAE,0xB5,0xC1,0xBE,0xB4,0xC0,0xB2,0x93,0x9F,0x93,0xA6,0xAD,0xA2,0x00,0xE9,0x00,0xDB,0x01,0x0F};
char Gamma_Set290[26] = {0xfa,0x02,0x58,0x42,0x66,0xAD,0xF3,0xB1,0xB3,0xC1,0xBB,0xB3,0xBF,0xB2,0x93,0x9F,0x93,0xA6,0xAD,0xA2,0x00,0xE5,0x00,0xD7,0x01,0x0B};
char Gamma_Set280[26] = {0xfa,0x02,0x58,0x42,0x66,0xAD,0xF3,0xB1,0xB5,0xC3,0xBE,0xB5,0xC1,0xB4,0x91,0x9E,0x91,0xA7,0xAE,0xA3,0x00,0xE2,0x00,0xD4,0x01,0x07};
char Gamma_Set270[26] = {0xfa,0x02,0x58,0x42,0x66,0xA9,0xEA,0xAD,0xB3,0xC3,0xBB,0xB5,0xC1,0xB4,0x92,0x9F,0x92,0xA7,0xAE,0xA4,0x00,0xDF,0x00,0xD1,0x01,0x04};
char Gamma_Set260[26] = {0xfa,0x02,0x58,0x42,0x66,0xA9,0xEA,0xAD,0xB5,0xC5,0xBE,0xB4,0xC0,0xB4,0x93,0x9F,0x93,0xA8,0xAF,0xA4,0x00,0xDC,0x00,0xCE,0x01,0x00};
char Gamma_Set250[26] = {0xfa,0x02,0x58,0x42,0x66,0xAC,0xED,0xB0,0xB3,0xC5,0xBB,0xB6,0xC2,0xB6,0x92,0x9F,0x92,0xA8,0xAF,0xA5,0x00,0xD9,0x00,0xCB,0x00,0xFC};
char Gamma_Set240[26] = {0xfa,0x02,0x58,0x42,0x66,0xAC,0xED,0xB0,0xB6,0xC7,0xBE,0xB6,0xC2,0xB6,0x92,0x9F,0x91,0xAA,0xB1,0xA7,0x00,0xD5,0x00,0xC8,0x00,0xF8};
char Gamma_Set230[26] = {0xfa,0x02,0x58,0x42,0x66,0xA8,0xE4,0xAC,0xB4,0xC7,0xBB,0xB8,0xC3,0xB9,0x91,0x9E,0x90,0xAB,0xB2,0xA8,0x00,0xD1,0x00,0xC5,0x00,0xF4};
char Gamma_Set220[26] = {0xfa,0x02,0x58,0x42,0x66,0xAC,0xE6,0xAF,0xB2,0xC7,0xB9,0xB8,0xC3,0xB9,0x92,0x9F,0x91,0xAC,0xB3,0xA9,0x00,0xCD,0x00,0xC1,0x00,0xEF};
char Gamma_Set210[26] = {0xfa,0x02,0x58,0x42,0x66,0xAC,0xE6,0xAF,0xB4,0xCA,0xBB,0xBA,0xC5,0xBB,0x91,0x9F,0x91,0xAB,0xB3,0xA9,0x00,0xCA,0x00,0xBE,0x00,0xEC};
char Gamma_Set200[26] = {0xfa,0x02,0x58,0x42,0x66,0xA7,0xDE,0xAA,0xB2,0xCA,0xB9,0xBC,0xC7,0xBE,0x90,0x9E,0x90,0xAC,0xB4,0xAA,0x00,0xC6,0x00,0xBA,0x00,0xE7};
char Gamma_Set190[26] = {0xfa,0x02,0x58,0x42,0x66,0xAB,0xE0,0xAE,0xB0,0xCA,0xB6,0xBB,0xC6,0xBE,0x91,0x9F,0x90,0xAE,0xB5,0xAC,0x00,0xC2,0x00,0xB6,0x00,0xE3};
char Gamma_Set180[26] = {0xfa,0x02,0x58,0x42,0x66,0xAB,0xE0,0xAE,0xB3,0xCC,0xB9,0xBE,0xC8,0xC1,0x90,0x9E,0x8F,0xAF,0xB7,0xAD,0x00,0xBE,0x00,0xB2,0x00,0xDE};
char Gamma_Set170[26] = {0xfa,0x02,0x58,0x42,0x66,0xAF,0xE2,0xB2,0xB1,0xCD,0xB6,0xC0,0xCA,0xC4,0x90,0x9E,0x8F,0xB0,0xB7,0xAE,0x00,0xBA,0x00,0xAF,0x00,0xDA};
char Gamma_Set160[26] = {0xfa,0x02,0x58,0x42,0x66,0xA6,0xD4,0xA9,0xB1,0xCF,0xB6,0xBF,0xCA,0xC4,0x90,0x9E,0x8E,0xB0,0xB8,0xAF,0x00,0xB6,0x00,0xAB,0x00,0xD5};
char Gamma_Set150[26] = {0xfa,0x02,0x58,0x42,0x66,0xA6,0xD4,0xA9,0xB4,0xD2,0xB9,0xC2,0xCC,0xC7,0x8E,0x9D,0x8D,0xB3,0xBA,0xB2,0x00,0xB2,0x00,0xA6,0x00,0xCF};
char Gamma_Set140[26] = {0xfa,0x02,0x58,0x42,0x66,0xAA,0xD6,0xAC,0xB1,0xD2,0xB6,0xC2,0xCC,0xC7,0x90,0x9F,0x8F,0xB4,0xBC,0xB3,0x00,0xAD,0x00,0xA2,0x00,0xCA};
char Gamma_Set130[26] = {0xfa,0x02,0x58,0x42,0x66,0xA2,0xC8,0xA4,0xB0,0xD4,0xB4,0xC4,0xCD,0xCB,0x8E,0x9D,0x8C,0xB6,0xBE,0xB5,0x00,0xA8,0x00,0x9D,0x00,0xC5};
char Gamma_Set120[26] = {0xfa,0x02,0x58,0x42,0x66,0xA7,0xCB,0xA9,0xAC,0xD3,0xB0,0xC6,0xCF,0xCD,0x8E,0x9E,0x8D,0xB7,0xBF,0xB7,0x00,0xA4,0x00,0x99,0x00,0xC0};
char Gamma_Set110[26] = {0xfa,0x02,0x58,0x42,0x66,0x9E,0xBC,0xA0,0xAB,0xD6,0xAE,0xC5,0xCE,0xCC,0x8F,0x9F,0x8F,0xB6,0xBF,0xB6,0x00,0x9F,0x00,0x95,0x00,0xBB};
char Gamma_Set100[26] = {0xfa,0x02,0x58,0x42,0x66,0xA4,0xBF,0xA6,0xA6,0xD5,0xA9,0xC7,0xD0,0xCE,0x90,0xA0,0x90,0xB8,0xC1,0xB8,0x00,0x9B,0x00,0x90,0x00,0xB6};
char Gamma_Set90[26] = {0xfa,0x02,0x58,0x42,0x66,0xAA,0xC1,0xAC,0xA6,0xD7,0xA8,0xC7,0xD3,0xCE,0x8F,0x9F,0x90,0xB9,0xC1,0xB9,0x00,0x96,0x00,0x8C,0x00,0xB0};
char Gamma_Set80[26] = {0xfa,0x02,0x58,0x42,0x66,0x9B,0xAD,0x9C,0xA6,0xD8,0xA9,0xC3,0xD3,0xC9,0x91,0xA1,0x92,0xBB,0xC3,0xBB,0x00,0x90,0x00,0x87,0x00,0xAA};
char Gamma_Set70[26] = {0xfa,0x02,0x58,0x42,0x66,0xA1,0xAF,0xA2,0xA7,0xD9,0xAA,0xC2,0xD6,0xC7,0x91,0xA1,0x93,0xBB,0xC4,0xBB,0x00,0x8B,0x00,0x81,0x00,0xA4};




char Etc_Cond_Set1_0[3] = {0xF0,0x5A,0x5A};
char Etc_Cond_Set1_1[3] = {0xF1,0x5A,0x5A};
char Etc_Cond_Set1_2[3] = {0xFC,0x5A,0x5A};

char Panel_Cond_Set[14] = {0xF8,0x27,0x27,0x08,0x08,0x4E,0xAA,0x5E,0x8A,0x10,0x3F,0x10,0x10,0x00};
char Gamma_Set[26]      = {0xFA,0x02,0x58,0x42,0x56,0xAA,0xC8,0xAE,0xB5,0xC1,0xBE,0xB4,0xC0,0xB2,0x93,0x9F,0x93,0xA6,0xAD,0xA2,0x00,0xE9,0x00,0xDB,0x01,0x0F};


char Gamma_Set_Eable[2] = {0xFA,0x03};

char Etc_cond_Set2_0[4] = {0xF6,0x00,0x84,0x09};

char Etc_cond_Set2_1[2] = {0xB0,0x09};
char Etc_cond_Set2_2[2] = {0xD5,0x64};
char Etc_cond_Set2_3[2] = {0xB0,0x0B};

char Etc_cond_Set2_4[4] = {0xD5,0xA4,0x7E,0x20};

char Etc_cond_Set2_5[2] = {0xB0,0x08};
char Etc_cond_Set2_6[2] = {0xFD,0xF8};
char Etc_cond_Set2_7[2] = {0xB0,0x04};
char Etc_cond_Set2_8[2] = {0xF2,0x4D};
char Etc_cond_Set2_9[2] = {0xB0,0x05};
char Etc_cond_Set2_10[2]= {0xFD,0x1F};

char Etc_cond_Set2_11[4]= {0xb1,0x01,0x00,0x16};
char Etc_cond_Set2_12[5]= {0xb2,0x15,0x15,0x15,0x15};
char Etc_cond_Set2_13[1]= {0x11};//0x05

char Memory_Window_Set2_0[1] = {0x35};
char Memory_Window_Set2_1[5] = {0x2a,0x00,0x1e,0x02,0x39};
char Memory_Window_Set2_2[5] = {0x2b,0x00,0x00,0x03,0xbf};
char Memory_Window_Set2_3[2] = {0xd1,0x8a};

char dis_on[1] 			= {0x29};//sleep_out

char dis_off[1] 		= {0x28};
char sleep_in[1]		= {0x10};
char acl_on[2] 		    = {0xc0,0x01};
char acl_data[29]       = {0xc1,0x47,0x53,0x13,
						   0x53,0x00,0x00,0x01,
						   0xdf,0x00,0x00,0x03,
						   0x1f,0x00,0x00,0x00,
					       0x00,0x00,0x01,0x02,
						   0x03,0x07,0x0e,0x14,
						   0x1c,0x24,0x2d,0x2d,
						   0x00};


static struct dsi_cmd_desc oscar_sleep_in[]=
{
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(dis_off), dis_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_in), sleep_in}
};
static struct dsi_cmd_desc oscar_panel_off[]=
{

	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(dis_off), dis_off}
};
static struct dsi_cmd_desc oscar_sleep_out[]=
{
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(dis_on), dis_on}
};



static struct dsi_cmd_desc oscar_display_init_cmds[]=
{
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Etc_Cond_Set1_0), Etc_Cond_Set1_0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Etc_Cond_Set1_1), Etc_Cond_Set1_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Etc_Cond_Set1_2), Etc_Cond_Set1_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Panel_Cond_Set), Panel_Cond_Set},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Gamma_Set), Gamma_Set},
	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Gamma_Set_Eable), Gamma_Set_Eable},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Etc_cond_Set2_0), Etc_cond_Set2_0},
	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Etc_cond_Set2_1), Etc_cond_Set2_1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Etc_cond_Set2_2), Etc_cond_Set2_2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Etc_cond_Set2_3), Etc_cond_Set2_3},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Etc_cond_Set2_4), Etc_cond_Set2_4},
	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Etc_cond_Set2_5), Etc_cond_Set2_5},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Etc_cond_Set2_6), Etc_cond_Set2_6},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Etc_cond_Set2_7), Etc_cond_Set2_7},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Etc_cond_Set2_8), Etc_cond_Set2_8},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Etc_cond_Set2_9), Etc_cond_Set2_9},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Etc_cond_Set2_10), Etc_cond_Set2_10},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Etc_cond_Set2_11), Etc_cond_Set2_11},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Etc_cond_Set2_12), Etc_cond_Set2_12},
	
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(Etc_cond_Set2_13), Etc_cond_Set2_13},

	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(Memory_Window_Set2_0), Memory_Window_Set2_0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Memory_Window_Set2_1), Memory_Window_Set2_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Memory_Window_Set2_2), Memory_Window_Set2_2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Memory_Window_Set2_3), Memory_Window_Set2_3},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(acl_on), acl_on},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(acl_data), acl_data}
	//{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(dis_on), dis_on},

};

static struct dsi_cmd_desc oscar_backlight_300_cmds[]=
{
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Gamma_Set300), Gamma_Set300},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Gamma_Set_Eable), Gamma_Set_Eable}
};
static struct dsi_cmd_desc oscar_backlight_280_cmds[]=
{
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Gamma_Set280), Gamma_Set280},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Gamma_Set_Eable), Gamma_Set_Eable}
};
static struct dsi_cmd_desc oscar_backlight_260_cmds[]=
{
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Gamma_Set260), Gamma_Set260},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Gamma_Set_Eable), Gamma_Set_Eable}
};
static struct dsi_cmd_desc oscar_backlight_240_cmds[]=
{
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Gamma_Set240), Gamma_Set240},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Gamma_Set_Eable), Gamma_Set_Eable}
};
static struct dsi_cmd_desc oscar_backlight_220_cmds[]=
{
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Gamma_Set220), Gamma_Set220},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Gamma_Set_Eable), Gamma_Set_Eable}
};
static struct dsi_cmd_desc oscar_backlight_200_cmds[]=
{
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Gamma_Set200), Gamma_Set200},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Gamma_Set_Eable), Gamma_Set_Eable}
};
static struct dsi_cmd_desc oscar_backlight_180_cmds[]=
{
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Gamma_Set180), Gamma_Set180},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Gamma_Set_Eable), Gamma_Set_Eable}
};
static struct dsi_cmd_desc oscar_backlight_160_cmds[]=
{
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Gamma_Set160), Gamma_Set160},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Gamma_Set_Eable), Gamma_Set_Eable}
};
static struct dsi_cmd_desc oscar_backlight_140_cmds[]=
{
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Gamma_Set140), Gamma_Set140},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Gamma_Set_Eable), Gamma_Set_Eable}
};
static struct dsi_cmd_desc oscar_backlight_120_cmds[]=
{
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Gamma_Set120), Gamma_Set120},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Gamma_Set_Eable), Gamma_Set_Eable}
};
static struct dsi_cmd_desc oscar_backlight_100_cmds[]=
{
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Gamma_Set100), Gamma_Set100},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Gamma_Set_Eable), Gamma_Set_Eable}
};
static struct dsi_cmd_desc oscar_backlight_80_cmds[]=
{
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(Gamma_Set80), Gamma_Set80},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(Gamma_Set_Eable), Gamma_Set_Eable}
};




static int mipi_oscar_lcd_on(struct platform_device *pdev)
{


	struct msm_fb_data_type *mfd;

    ENTER_FUNC2();

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (oscar_state.disp_initialized == false) {
		
		msleep(25);
		gpio_set_value_cansleep(gpio43, 0);
	 	usleep(10);
        gpio_set_value_cansleep(gpio43, 1);  // lcd panel reset 
        msleep(10);

		
		mipi_set_tx_power_mode(0);	
		mipi_dsi_cmds_tx(mfd, &oscar_tx_buf, oscar_display_init_cmds,
				ARRAY_SIZE(oscar_display_init_cmds));
		mipi_set_tx_power_mode(1);	
		oscar_state.disp_initialized = true;
	}
	if(oscar_state.disp_powered_up == true){
		mipi_set_tx_power_mode(0);	
		mipi_dsi_cmds_tx(mfd, &oscar_tx_buf, oscar_sleep_out,
				ARRAY_SIZE(oscar_sleep_out));
		mipi_set_tx_power_mode(1);	
	}
	
	oscar_state.disp_on = true;



	EXIT_FUNC2();

	return 0;
}
void mipi_oscar_sec_lcd_on(struct msm_fb_data_type *mfd)
{

 ENTER_FUNC2();
 	if(oscar_state.disp_powered_up == false){
		mipi_set_tx_power_mode(0);	
 		mipi_dsi_cmds_tx(mfd, &oscar_tx_buf, oscar_sleep_out,
			 		ARRAY_SIZE(oscar_sleep_out));
		mipi_set_tx_power_mode(1);	
		
 	}
 EXIT_FUNC2();


}

static int mipi_oscar_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

    ENTER_FUNC2();

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (oscar_state.disp_on == true) {
	
			mipi_dsi_cmds_tx(mfd, &oscar_tx_buf, oscar_sleep_in,
				ARRAY_SIZE(oscar_sleep_in));
		
		oscar_state.disp_on = false;
		oscar_state.disp_initialized = false;
	}
    
	oscar_state.disp_powered_up = false;
	EXIT_FUNC2();
	return 0;

}


static void mipi_oscar_set_backlight(struct msm_fb_data_type *mfd)
{
	
	if(!mfd)
		return;
	if(mfd->key != MFD_KEY)
		return;
	
	mutex_lock(&mfd->dma->ov_mutex);	

	mdp4_dsi_cmd_dma_busy_wait(mfd);
	mdp4_dsi_blt_dmap_busy_wait(mfd);
	mipi_dsi_mdp_busy_wait(mfd);
	
	
	mipi_set_tx_power_mode(0);	
	if(oscar_state.backlightoff == false && chargerFlag == true){
		
		mipi_dsi_cmds_tx(mfd, &oscar_tx_buf, oscar_sleep_out,
						ARRAY_SIZE(oscar_sleep_out));
		
	}
	switch(mfd->bl_level){
	
		case 12 : 
				mipi_dsi_cmds_tx(mfd, &oscar_tx_buf, oscar_backlight_300_cmds,
					ARRAY_SIZE(oscar_backlight_300_cmds));
				  break;
		case 11 :
				mipi_dsi_cmds_tx(mfd, &oscar_tx_buf, oscar_backlight_280_cmds,
					ARRAY_SIZE(oscar_backlight_280_cmds));
				  break;
		case 10 : 
				mipi_dsi_cmds_tx(mfd, &oscar_tx_buf, oscar_backlight_260_cmds,
					ARRAY_SIZE(oscar_backlight_260_cmds));
				  break;
		case 9 :
				mipi_dsi_cmds_tx(mfd, &oscar_tx_buf, oscar_backlight_240_cmds,
					ARRAY_SIZE(oscar_backlight_240_cmds));
				  break;
		case 8 : 
				mipi_dsi_cmds_tx(mfd, &oscar_tx_buf, oscar_backlight_220_cmds,
					ARRAY_SIZE(oscar_backlight_220_cmds));
				  break;
		case 7 : 
				mipi_dsi_cmds_tx(mfd, &oscar_tx_buf, oscar_backlight_200_cmds,
					ARRAY_SIZE(oscar_backlight_200_cmds));
				  break;
		case 6 :
				mipi_dsi_cmds_tx(mfd, &oscar_tx_buf, oscar_backlight_180_cmds,
					ARRAY_SIZE(oscar_backlight_180_cmds));
				  break;
		case 5 : 
				mipi_dsi_cmds_tx(mfd, &oscar_tx_buf, oscar_backlight_160_cmds,
					ARRAY_SIZE(oscar_backlight_160_cmds));
				  break;
		case 4 : 
				mipi_dsi_cmds_tx(mfd, &oscar_tx_buf, oscar_backlight_140_cmds,
					ARRAY_SIZE(oscar_backlight_140_cmds));
				  break;
		case 3 :
				mipi_dsi_cmds_tx(mfd, &oscar_tx_buf, oscar_backlight_120_cmds,
					ARRAY_SIZE(oscar_backlight_120_cmds));
				  break;
		case 2 : 
				mipi_dsi_cmds_tx(mfd, &oscar_tx_buf, oscar_backlight_100_cmds,
					ARRAY_SIZE(oscar_backlight_100_cmds));
				  break;
		case 1 : 
				mipi_dsi_cmds_tx(mfd, &oscar_tx_buf, oscar_backlight_80_cmds,
					ARRAY_SIZE(oscar_backlight_80_cmds));
				  break;
		case 0 :if(chargerFlag){ 
				mipi_dsi_cmds_tx(mfd, &oscar_tx_buf, oscar_panel_off,
					ARRAY_SIZE(oscar_panel_off));
				}else
				{
				mipi_dsi_cmds_tx(mfd, &oscar_tx_buf, oscar_backlight_80_cmds,
					ARRAY_SIZE(oscar_backlight_80_cmds));
				}
				  break;
		}
	mipi_set_tx_power_mode(1);
	mutex_unlock(&mfd->dma->ov_mutex);
	
	if(mfd->bl_level > 0)
	{
		oscar_state.backlightoff = true;
	}else
	{
		oscar_state.backlightoff = false;
	}
	printk(KERN_WARNING"[%s] = %d\n",__func__,mfd->bl_level);
}


static int __devinit mipi_oscar_lcd_probe(struct platform_device *pdev)
{
	
	
	if (pdev->id == 0) {
        mipi_oscar_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_oscar_lcd_probe,
	.driver = {
		.name   = "mipi_oscar",
	},
};

static struct msm_fb_panel_data oscar_panel_data = {
       .on             = mipi_oscar_lcd_on,
       .off            = mipi_oscar_lcd_off,
       .set_backlight  = mipi_oscar_set_backlight,
};

static int ch_used[3];

int mipi_oscar_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_oscar", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	oscar_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &oscar_panel_data,
		sizeof(oscar_panel_data));
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

static int __init mipi_oscar_lcd_init(void)
{
    ENTER_FUNC2();

    oscar_state.disp_powered_up = true;

    mipi_dsi_buf_alloc(&oscar_tx_buf, DSI_BUF_SIZE);
    mipi_dsi_buf_alloc(&oscar_rx_buf, DSI_BUF_SIZE);

    EXIT_FUNC2();

    return platform_driver_register(&this_driver);

}

module_init(mipi_oscar_lcd_init);


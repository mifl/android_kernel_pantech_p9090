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
#include "mipi_samsung_oled.h"
#include <mach/gpio.h>
#include <asm/irq.h>
#include <asm/system.h>


#ifdef CONFIG_PANTECH_F_HDMI_PW_CTRL_ON_LCD
extern int hdmi_autodetect_control(int on);
extern int HDMI_Schedule_Control(int on);
extern uint32_t HDMI_Get_Cable_State(void);
#define HDMI_PW_ON 1
#define HDMI_PW_OFF 0
#endif

#define LCD_RESET		43

#define LCD_DEBUG_MSG

#ifdef LCD_DEBUG_MSG
#define ENTER_FUNC()        printk(KERN_INFO "[PANTECH_LCD] +%s \n", __FUNCTION__);
#define EXIT_FUNC()         printk(KERN_INFO "[PANTECH_LCD] -%s \n", __FUNCTION__);
#define ENTER_FUNC2()    printk(KERN_ERR "[PANTECH_LCD] +%s\n", __FUNCTION__);
#define EXIT_FUNC2()       printk(KERN_ERR "[PANTECH_LCD] -%s\n", __FUNCTION__);
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

#define D_SKY_BACKLIGHT_CONTROL		1

extern int gpio43;
static struct msm_panel_common_pdata *mipi_samsung_oled_pdata;

static struct dsi_buf samsung_oled_tx_buf;
static struct dsi_buf samsung_oled_rx_buf;

struct lcd_state_type {
    boolean disp_powered_up;
    boolean disp_initialized;
    boolean disp_on;
};

static struct lcd_state_type samsung_oled_state = { 0, };

unsigned char is_first, is_sleep;
unsigned int prev_bl, now_bl;

//power on
char level2_key[3]          = {0xF0, 0x5A, 0x5A};
char internal_osc_on[6]     = {0xF3, 0x00, 0x00, 0x00, 0x2A, 0x02};
char dispctl[6]             = {0xF2, 0x02, 0x06, 0x0a, 0x10, 0x10};
char gtcon[2]               = {0xF7, 0x09};
#if 0 /* ver 1 */
char panelctl[24]           = {0xF8, 0x05, 0x5E, 0x96, 0x6b, 0x7d, 0x0d, 0x3f, 0x00, 0x00, 0x32,
                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x07, 0x20, 0x20,
                                     0x20, 0x00, 0x00};
#else /* ver 2 */
char panelctl[24]           = {0xF8, 0x05, 0x8D, 0xE0, 0xA0, 0xBB, 0x14, 0x5E, 0x00, 0x00, 0x4B, 
							   		 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x08, 0x2E, 0x2E,
									 0x2E, 0x00, 0x00};
#endif
char sleep_out[2]           = {0x11, 0x00};
char set_power_sontrol_1[7] = {0xF4, 0x0a, 0xa7, 0x25, 0x6a, 0x44, 0x02};
/* brightness 30 cd */
#if 0 /* ver 1 */
char gamma_set_1[22] = { 0xF9, 0x0C, 0xA6, 0xAE, 0xA9, 0xCD, 0x0, 0x3A, 0x0C, 0x7E, 0xB0, 
								0xAB, 0xCC, 0x0, 0x49, 0x0C, 0xA9, 0xBB, 0xAF, 0xCC, 0x0,
								0x49};
#else /* ver 2 */
char gamma_set_1[22] = {0xF9,0x0C,0x91,0xB4,0xB8,0xD1,0x00,0x48,0x0C,0x7A,0xAD,0xBD,0xD4,0x00,0x56,0x0C,0xBB,0xAE,0xC3,0xD5,0x00,0x56};
#endif
char gamma_control[3]       = {0xFB, 0x02, 0x5a};
char disp_on[2]             = {0x29, 0x00};


//sleep in
char disp_off[2]            = {0x28, 0x00};
char sleep_in[2]            = {0x10, 0x00};
char internal_osc_off[6]    = {0xF3, 0x00, 0x00, 0x00, 0x0A, 0x02};

#ifdef D_SKY_BACKLIGHT_CONTROL
/* Samsung OLED Gamma Setting */
#if 0 /* ver 1 */
char gamma_set_300[22] = { 0xF9, 0x0C, 0xA9, 0xB1, 0xA4, 0xC3, 0x0, 0x83, 0x0C, 0xA7, 0xAF,
								  0xA2, 0xC2, 0x0, 0x9C, 0x0C, 0xB3, 0xB4, 0xA5, 0xC1, 0x0, 
							   	0xA0};
char gamma_set_280[22] = { 0xF9, 0x0C, 0xA8, 0xAF, 0xA5, 0xC5, 0x0, 0x7F, 0x0C, 0xA6, 0xB0,
								0xA2, 0xC3, 0x0, 0x98, 0x0C, 0xB3, 0xB3, 0xA5, 0xC3, 0x0,
								0x9B};
char gamma_set_260[22] = { 0xF9, 0x0C, 0xA8, 0xAF, 0xA5, 0xC7, 0x0, 0x7B, 0x0C, 0xA6, 0xB0,
								0xA2, 0xC5, 0x0, 0x93, 0x0C, 0xB3, 0xB3, 0xA5, 0xC5, 0x0,
								0x97};
char gamma_set_240[22] = { 0xF9, 0x0C, 0xA8, 0xAF, 0xA5, 0xC7, 0x0, 0x78, 0x0C, 0xA3, 0xB1,
								0xA3, 0xC4, 0x0, 0x8F, 0x0C, 0xB3, 0xB4, 0xA5, 0xC4, 0x0, 
								0x93};
char gamma_set_220[22] = { 0xF9, 0x0C, 0xA8, 0xAF, 0xA5,0xC7, 0x0, 0x74, 0x0C, 0xA1, 0xB2,
								0xA3, 0xC5, 0x0, 0x8B, 0x0C, 0xB3, 0xB5, 0xA6, 0xC4, 0x0,
								0x8E};
char gamma_set_200[22] = { 0xF9, 0x0C, 0xA8, 0xAF, 0xA6, 0xC6, 0x0, 0x70, 0x0C, 0xA0, 0xB1,
								0xA4, 0xC5, 0x0, 0x87, 0x0C, 0xB2, 0xB5, 0xA8, 0xC4, 0x0,
								0x89};
char gamma_set_180[22] = { 0xF9, 0x0C, 0xA7, 0xB0, 0xA7, 0xC8, 0x0, 0x6B, 0x0C, 0x9E, 0xB2,
								0xA4, 0xC6, 0x0, 0x82, 0x0C, 0xB1, 0xB5, 0xA9, 0xC6, 0x0,
								0x83};
char gamma_set_160[22] = { 0xF9, 0x0C, 0xA7, 0xB0, 0xA8, 0xCA, 0x0, 0x66, 0x0C, 0x9E, 0xB2,
								0xA5, 0xC7, 0x0, 0x7C, 0x0C, 0xB1, 0xB5, 0xAA, 0xC7, 0x0,
								0x7E};
char gamma_set_140[22] = { 0xF9, 0x0C, 0xA6, 0xB0, 0xA7, 0xCA, 0x0, 0x62, 0x0C, 0x99, 0xB2,
								0xA5, 0xC8, 0x0, 0x77, 0x0C, 0xB0, 0xB6, 0xA9, 0xC7, 0x0,
								0x79};
char gamma_set_120[22] = { 0xF9, 0x0C, 0xA6, 0xB2, 0xA7, 0xCB, 0x0, 0x5D, 0x0C, 0x99, 0xB4,
								0xA6, 0xC8, 0x0, 0x71, 0x0C, 0xB0, 0xB8, 0xA9, 0xC7, 0x0,
								0x73};
char gamma_set_100[22] = { 0xF9, 0x0C, 0xA2, 0xB0, 0xA6, 0xC8, 0x0, 0x58, 0x0C, 0x81, 0xB2, 
								0xA6, 0xCA, 0x0, 0x69, 0x0C, 0xA5, 0xB7, 0xAB, 0xC9, 0x0,
								0x6A};
char gamma_set_80[22] = { 0xF9, 0x0C, 0xA2, 0xB1, 0xA5, 0xCA, 0x0, 0x52, 0x0C, 0x80, 0xB3,
								0xA6, 0xCB, 0x0, 0x62, 0x0C, 0xA5, 0xB9, 0xAB, 0xCA, 0x0,
								0x63};
char gamma_set_60[22] = { 0xF9, 0x0C, 0xA6, 0xB0, 0xA8, 0xCD, 0x0, 0x4A, 0x0C, 0x85, 0xB4, 
								0xA8, 0xCB, 0x0, 0x5C, 0x0C, 0xAB, 0xBB, 0xAD, 0xCB, 0x0,
								0x5B};
char gamma_set_40[22] = { 0xF9, 0x0C, 0xA6, 0xB1, 0xA8, 0xCD, 0x0, 0x42, 0x0C, 0x7E, 0xB4, 
								0xA9, 0xCC, 0x0, 0x53, 0x0C, 0xA9, 0xBC, 0xAE, 0xCB, 0x0,
								0x53};
char gamma_set_30[22] = { 0xF9, 0x0C, 0xA6, 0xAE, 0xA9, 0xCD, 0x0, 0x3A, 0x0C, 0x7E, 0xB0, 
								0xAB, 0xCC, 0x0, 0x49, 0x0C, 0xA9, 0xBB, 0xAF, 0xCC, 0x0,
								0x49};
//char gamma_set_0[22] =  { 0xF9, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00,
//							    0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00,
//								0x00};
#else /* ver 2 */
char gamma_set_300[22] = {0xF9,0x0C,0xA9,0xAF,0xA9,0xBC,0x00,0xAA,0x0C,0xAB,0xAE,0xA6,0xBB,0x00,0xC8,0x0C,0xB5,0xB1,0xA7,0xBC,0x00,0xCC}; 
char gamma_set_280[22] = {0xF9,0x0C,0xA7,0xB1,0xAA,0xBD,0x00,0xA6,0x0C,0xAB,0xAF,0xA7,0xBE,0x00,0xC3,0x0C,0xB6,0xB4,0xA8,0xBE,0x00,0xC7};
char gamma_set_260[22] = {0xF9,0x0C,0xA7,0xB2,0xAA,0xBE,0x00,0xA1,0x0C,0xA9,0xB1,0xA7,0xBF,0x00,0xBD,0x0C,0xB5,0xB6,0xA8,0xBF,0x00,0xC1};
char gamma_set_240[22] = {0xF9,0x0C,0xA7,0xB3,0xAA,0xC0,0x00,0x9C,0x0C,0xA9,0xB1,0xA9,0xC0,0x00,0xB7,0x0C,0xB5,0xB6,0xA9,0xC0,0x00,0xBB};
char gamma_set_220[22] = {0xF9,0x0C,0xA7,0xB3,0xAD,0xC1,0x00,0x96,0x0C,0xA7,0xB2,0xAA,0xC2,0x00,0xB0,0x0C,0xB4,0xB6,0xAB,0xC3,0x00,0xB4};
char gamma_set_200[22] = {0xF9,0x0C,0xA7,0xB3,0xAD,0xC2,0x00,0x91,0x0C,0xA7,0xB3,0xAA,0xC3,0x00,0xAB,0x0C,0xB4,0xB9,0xAB,0xC3,0x00,0xAE};
char gamma_set_180[22] = {0xF9,0x0C,0xA6,0xB4,0xAE,0xC4,0x00,0x8B,0x0C,0xA5,0xB5,0xAB,0xC5,0x00,0xA3,0x0C,0xB2,0xB9,0xAD,0xC5,0x00,0xA7};
char gamma_set_160[22] = {0xF9,0x0C,0xA5,0xB3,0xB1,0xC4,0x00,0x85,0x0C,0xA2,0xB5,0xAD,0xC6,0x00,0x9D,0x0C,0xB1,0xB9,0xB0,0xC6,0x00,0x9F};
char gamma_set_140[22] = {0xF9,0x0C,0xA5,0xB5,0xB1,0xC5,0x00,0x7F,0x0C,0xA0,0xB7,0xAE,0xC8,0x00,0x95,0x0C,0xB0,0xBB,0xB1,0xC8,0x00,0x97};
char gamma_set_120[22] = {0xF9,0x0C,0xA4,0xB6,0xB2,0xC7,0x00,0x77,0x0C,0x9D,0xB8,0xAF,0xCA,0x00,0x8C,0x0C,0xAF,0xBA,0xB3,0xCA,0x00,0x8E};
char gamma_set_100[22] = {0xF9,0x0C,0xA4,0xB6,0xB3,0xC9,0x00,0x70,0x0C,0x9B,0xB7,0xB2,0xCA,0x00,0x85,0x0C,0xB0,0xBA,0xB4,0xCB,0x00,0x86};
char gamma_set_80[22] = {0xF9,0x0C,0xA2,0xB7,0xB3,0xCC,0x00,0x67,0x0C,0x95,0xB8,0xB3,0xCE,0x00,0x79,0x0C,0xB1,0xB8,0xB7,0xCE,0x00,0x7B};
char gamma_set_60[22] = {0xF9,0x0C,0xA0,0xB5,0xB7,0xCE,0x00,0x5C,0x0C,0x8E,0xB6,0xB6,0xCF,0x00,0x6E,0x0C,0xB4,0xB4,0xBC,0xD1,0x00,0x6E};
char gamma_set_40[22] = {0xF9,0x0C,0x9D,0xB4,0xB9,0xCF,0x00,0x50,0x0C,0x88,0xB1,0xBA,0xD1,0x00,0x60,0x0C,0xB9,0xAE,0xC1,0xD3,0x00,0x60};
char gamma_set_30[22] = {0xF9,0x0C,0x91,0xB4,0xB8,0xD1,0x00,0x48,0x0C,0x7A,0xAD,0xBD,0xD4,0x00,0x56,0x0C,0xBB,0xAE,0xC3,0xD5,0x00,0x56};
#endif /* version */

#endif /* backlight */
static struct dsi_cmd_desc samsung_oled_display_init_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(level2_key), level2_key},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(internal_osc_on), internal_osc_on},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(dispctl), dispctl},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gtcon), gtcon},   
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_out), sleep_out},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(set_power_sontrol_1), set_power_sontrol_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_1), gamma_set_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_control), gamma_control},
    {DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(disp_on), disp_on},
};

static struct dsi_cmd_desc samsung_oled_screen_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(disp_off), disp_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(sleep_in), sleep_in},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 120, sizeof(internal_osc_off), internal_osc_off},	
};

static struct dsi_cmd_desc samsung_oled_display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(internal_osc_on), internal_osc_on},	
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_out), sleep_out},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(disp_on), disp_on},
};

#ifdef D_SKY_BACKLIGHT_CONTROL
/* backlight */
static struct dsi_cmd_desc samsung_oled_display_gamma_cmds_300[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_300), gamma_set_300},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_control), gamma_control},
};
static struct dsi_cmd_desc samsung_oled_display_gamma_cmds_280[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_280), gamma_set_280},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_control), gamma_control},
};
static struct dsi_cmd_desc samsung_oled_display_gamma_cmds_260[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_260), gamma_set_260},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_control), gamma_control},
};
static struct dsi_cmd_desc samsung_oled_display_gamma_cmds_240[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_240), gamma_set_240},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_control), gamma_control},
};
static struct dsi_cmd_desc samsung_oled_display_gamma_cmds_220[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_220), gamma_set_220},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_control), gamma_control},
};
static struct dsi_cmd_desc samsung_oled_display_gamma_cmds_200[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_200), gamma_set_200},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_control), gamma_control},
};
static struct dsi_cmd_desc samsung_oled_display_gamma_cmds_180[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_180), gamma_set_180},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_control), gamma_control},
};
static struct dsi_cmd_desc samsung_oled_display_gamma_cmds_160[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_160), gamma_set_160},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_control), gamma_control},
};
static struct dsi_cmd_desc samsung_oled_display_gamma_cmds_140[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_140), gamma_set_140},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_control), gamma_control},
};
static struct dsi_cmd_desc samsung_oled_display_gamma_cmds_120[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_120), gamma_set_120},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_control), gamma_control},
};
static struct dsi_cmd_desc samsung_oled_display_gamma_cmds_100[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_100), gamma_set_100},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_control), gamma_control},
};
static struct dsi_cmd_desc samsung_oled_display_gamma_cmds_80[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_80), gamma_set_80},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_control), gamma_control},
};
static struct dsi_cmd_desc samsung_oled_display_gamma_cmds_60[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_60), gamma_set_60},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_control), gamma_control},
};
static struct dsi_cmd_desc samsung_oled_display_gamma_cmds_40[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_40), gamma_set_40},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_control), gamma_control},
};
static struct dsi_cmd_desc samsung_oled_display_gamma_cmds_30[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_set_30), gamma_set_30},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(gamma_control), gamma_control},
};
#endif

static int mipi_samsung_oled_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

    ENTER_FUNC2();
    
	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

       if (samsung_oled_state.disp_initialized == false) {
			is_first = TRUE;
    	    samsung_oled_state.disp_initialized = true;
	 	  	usleep(10);
	   		
			gpio_set_value_cansleep(gpio43, 0);
	 		usleep(10);
           	gpio_set_value_cansleep(gpio43, 1);  // lcd panel reset 
            msleep(10);
		
			/* bllp bit initialized ... */
            mipi_dsi_cmds_tx(mfd, &samsung_oled_tx_buf, samsung_oled_display_init_cmds,
                   ARRAY_SIZE(samsung_oled_display_init_cmds));

			is_sleep = FALSE;
	        printk("[PANTECH_LCD] power on state (oled panel).... \n");

#ifdef CONFIG_PANTECH_F_HDMI_PW_CTRL_ON_LCD
		hdmi_autodetect_control(HDMI_PW_ON);
		HDMI_Schedule_Control(1);
#endif	
       }
       samsung_oled_state.disp_on = true;
       
   EXIT_FUNC2();
	return 0;
}

static int mipi_samsung_oled_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	u32 tmp;
    
    ENTER_FUNC2();
    
	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
    
	if (samsung_oled_state.disp_on == true) {
#if 0
	   	gpio_set_value_cansleep(gpio43, 0);
	  	usleep(10);
	  	gpio_set_value_cansleep(gpio43, 1);
	  	usleep(10);
		
		mipi_dsi_sw_reset();
#endif
		tmp = MIPI_INP(MIPI_DSI_BASE + 0xA8);
		tmp &= ~(1<<28);
		MIPI_OUTP(MIPI_DSI_BASE + 0xA8, tmp);
		wmb();
           
		is_sleep = FALSE;
        samsung_oled_state.disp_initialized = false;
        samsung_oled_state.disp_on = false;
	    printk("[PANTECH_LCD] power off state (oled panel).... \n");
#ifdef CONFIG_PANTECH_F_HDMI_PW_CTRL_ON_LCD
				if (!HDMI_Get_Cable_State()) {
					hdmi_autodetect_control(HDMI_PW_OFF);
					HDMI_Schedule_Control(0);
				}

#endif	

    }   
    
    EXIT_FUNC2();
	return 0;
}


static void mipi_samsung_oled_set_backlight(struct msm_fb_data_type *mfd)
{
	unsigned long flags;
	ENTER_FUNC2();


	if((prev_bl != mfd->bl_level) || (is_first==TRUE)){
    	now_bl = mfd->bl_level; 
		prev_bl = now_bl;
	} else {
		printk("[PANTECH_LCD:%s] Equal backlight value ... pass ....\n",__func__);
		return;
	}

	printk(KERN_ERR "[PANTECH_LCD:] %s:%d(is_sleep:%d,is_first:%d)\n", __FUNCTION__, (mfd->bl_level),is_sleep, is_first);

    if((samsung_oled_state.disp_initialized != true) || (samsung_oled_state.disp_on == false)) {
			printk("[PANTECH_LCD:%s] Panel is off state....Ignore backlight value... \n",__func__);
			return;
	}

#ifdef D_SKY_BACKLIGHT_CONTROL

	if((is_sleep==TRUE) && (samsung_oled_state.disp_on == true)) {
		   msleep(50);
		   is_sleep = FALSE;
           mipi_dsi_cmds_tx(mfd, &samsung_oled_tx_buf, samsung_oled_display_on_cmds,
                   ARRAY_SIZE(samsung_oled_display_on_cmds));
		   printk("[PANTECH_LCD:%s] wakeup state (oled panel).... \n",__func__);
	}
	local_save_flags(flags);
	local_irq_disable();

		switch (now_bl) {
	case 15:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_tx_buf, samsung_oled_display_gamma_cmds_300,
			ARRAY_SIZE(samsung_oled_display_gamma_cmds_300));
		break;
	case 14:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_tx_buf, samsung_oled_display_gamma_cmds_280,
			ARRAY_SIZE(samsung_oled_display_gamma_cmds_280));
		break;
	case 13:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_tx_buf, samsung_oled_display_gamma_cmds_260,
			ARRAY_SIZE(samsung_oled_display_gamma_cmds_260));
		break;
	case 12:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_tx_buf, samsung_oled_display_gamma_cmds_240,
			ARRAY_SIZE(samsung_oled_display_gamma_cmds_240));
		break;
	case 11:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_tx_buf, samsung_oled_display_gamma_cmds_220,
			ARRAY_SIZE(samsung_oled_display_gamma_cmds_220));
		break;
	case 10:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_tx_buf, samsung_oled_display_gamma_cmds_200,
			ARRAY_SIZE(samsung_oled_display_gamma_cmds_200));
		break;
	case 9:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_tx_buf, samsung_oled_display_gamma_cmds_180,
			ARRAY_SIZE(samsung_oled_display_gamma_cmds_180));
		break;
	case 8:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_tx_buf, samsung_oled_display_gamma_cmds_160,
			ARRAY_SIZE(samsung_oled_display_gamma_cmds_160));
		break;
	case 7:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_tx_buf, samsung_oled_display_gamma_cmds_140,
			ARRAY_SIZE(samsung_oled_display_gamma_cmds_140));
		break;
	case 6:
		default:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_tx_buf, samsung_oled_display_gamma_cmds_120,
			ARRAY_SIZE(samsung_oled_display_gamma_cmds_120));
		break;
	case 5:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_tx_buf, samsung_oled_display_gamma_cmds_100,
			ARRAY_SIZE(samsung_oled_display_gamma_cmds_100));
		break;
	case 4:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_tx_buf, samsung_oled_display_gamma_cmds_80,
			ARRAY_SIZE(samsung_oled_display_gamma_cmds_80));
		break;
	case 3:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_tx_buf, samsung_oled_display_gamma_cmds_60,
			ARRAY_SIZE(samsung_oled_display_gamma_cmds_60));
		break;
	case 2:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_tx_buf, samsung_oled_display_gamma_cmds_40,
			ARRAY_SIZE(samsung_oled_display_gamma_cmds_40));
		break;
	case 1:
	   mipi_dsi_cmds_tx(mfd, &samsung_oled_tx_buf, samsung_oled_display_gamma_cmds_30,
			ARRAY_SIZE(samsung_oled_display_gamma_cmds_30));
		break;
	case 0:
		if(!is_first) {
            mipi_dsi_cmds_tx(mfd, &samsung_oled_tx_buf, samsung_oled_screen_off_cmds,
             	      ARRAY_SIZE(samsung_oled_screen_off_cmds));
			is_sleep = TRUE;
		    printk("[PANTECH_LCD:%s] sleep state (oled panel).... \n",__func__);
		}else
			is_first = FALSE;
		break;
	}	

	local_irq_restore(flags);
#endif
	EXIT_FUNC2();
}

static struct msm_fb_panel_data samsung_oled_panel_data = {
       .on             = mipi_samsung_oled_on,
       .off            = mipi_samsung_oled_off,
       .set_backlight  = mipi_samsung_oled_set_backlight,
};

static int __devinit mipi_samsung_oled_probe(struct platform_device *pdev)
{
    if (pdev->id == 0) {
        mipi_samsung_oled_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_samsung_oled_probe,
	.driver = {
		.name   = "mipi_samsung_oled",
	},
};


static int ch_used[3];

int mipi_samsung_oled_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_samsung_oled", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	samsung_oled_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &samsung_oled_panel_data,
		sizeof(samsung_oled_panel_data));
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

static int __init mipi_samsung_oled_init(void)
{
    ENTER_FUNC2();

    samsung_oled_state.disp_powered_up = true;

    mipi_dsi_buf_alloc(&samsung_oled_tx_buf, DSI_BUF_SIZE);
    mipi_dsi_buf_alloc(&samsung_oled_rx_buf, DSI_BUF_SIZE);

    EXIT_FUNC2();

    return platform_driver_register(&this_driver);
}
module_init(mipi_samsung_oled_init);


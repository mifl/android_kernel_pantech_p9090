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
#include "mipi_lgd.h"
#include <mach/gpio.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/gpio.h>

#define D_SKY_BACKLIGHT_CONTROL	1
#define GPIO_HIGH_VALUE 1
#define GPIO_LOW_VALUE  0

#ifdef CONFIG_MACH_MSM8960_STARQ 
#if (BOARD_VER >= WS20)
#define LCD_DIM_CON		89
#else
#define LCD_DIM_CON		3  /* backlight msm gpio */
#endif //#if (BOARD_VER >= WS20)
#endif //ifdef CONFIG_MACH_MSM8960_STARQ

#define T_EN	10
#define T_ON	10
#define T_OFF   5	
#define	T_DIS	500

/*
//#define NOP()   do {asm volatile ("NOP");} while(0);
//#define DELAY_5NS() do { \
     asm volatile ("NOP"); \
     asm volatile ("NOP"); \
     asm volatile ("NOP"); \
     asm volatile ("NOP"); \
     asm volatile ("NOP");} while(0);
*/

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
/* 43 - reset_n 3 - backlight */

extern int gpio43;
static struct msm_panel_common_pdata *mipi_lgd_pdata;

static struct dsi_buf lgd_tx_buf;
static struct dsi_buf lgd_rx_buf;

struct lcd_state_type {
    boolean disp_powered_up;
    boolean disp_initialized;
    boolean disp_on;
};

static struct lcd_state_type lgd_state = { 0, };
int first_enable = 0;
int prev_bl_level = 0;

char display_inv_off[2]    = {0x20, 0x00};
char interface_pixel_format[2] = {0x3A, 0x70};
char RGB_interface_set[4] = {0xB1, 0x06, 0x43, 0x0A};
char panel_characteristic_set[3] = {0xB2, 0x00, 0xC8};
//char panel_drive_set[2] = {0xB3 , 0x02}; // ??
char panel_drive_set[2] = {0xB3 , 0x00}; // ??
char display_mode_control[2] = {0xB4, 0x04};
char display_control1[6] = {0xB5, 0x40, 0x18, 0x02, 0x00, 0x01};
char display_control2[7] = {0xB6, 0x0B, 0x0F, 0x02, 0x40, 0x10, 0xE8};
char power_control3[6] = {0xC3, 0x07, 0x02, 0x02, 0x02, 0x02};
char power_control4[7] = {0xC4, 0x12, 0x24, 0x17, 0x17, 0x01, 0x49};
char power_control5[2] = {0xC5, 0x71};
char power_control6[4] = {0xC6, 0x44, 0x63, 0x03};
char p_gamma_curve_for_red1[10] = { 0xD0, 0x71, 0x45, 0x66, 0x02, 0x00, 0x03, 0x51, 0x33, 0x02};
char p_gamma_curve_for_red2[10] = { 0xD1, 0x71, 0x45, 0x66, 0x02, 0x00, 0x03, 0x51, 0x33, 0x02};
char p_gamma_curve_for_green1[10] = { 0xD2, 0x71, 0x45, 0x66, 0x02, 0x00, 0x03, 0x51, 0x33, 0x02};
char p_gamma_curve_for_green2[10] = { 0xD3, 0x71, 0x45, 0x66, 0x02, 0x00, 0x03, 0x51, 0x33, 0x02};
char p_gamma_curve_for_blue1[10] = { 0xD4, 0x71, 0x45, 0x66, 0x02, 0x00, 0x03, 0x51, 0x33, 0x02};
char p_gamma_curve_for_blue2[10] = { 0xD5, 0x71, 0x45, 0x66, 0x02, 0x00, 0x03, 0x51, 0x33, 0x02};
char sleep_out[2]           = {0x11, 0x00};
char disp_on[2]             = {0x29, 0x00};
char disp_off[2]            = {0x28, 0x00};
char sleep_in[2] 			= {0x10, 0x00};

static struct dsi_cmd_desc lgd_display_init_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(display_inv_off), display_inv_off},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(interface_pixel_format), interface_pixel_format},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(RGB_interface_set), RGB_interface_set},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(panel_characteristic_set), panel_characteristic_set},   
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(panel_drive_set), panel_drive_set},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(display_mode_control), display_mode_control},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(display_control1), display_control1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(display_control2), display_control2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(power_control3), power_control3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(power_control4), power_control4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(power_control5), power_control5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(power_control6), power_control6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(p_gamma_curve_for_red1), p_gamma_curve_for_red1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(p_gamma_curve_for_red2), p_gamma_curve_for_red2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(p_gamma_curve_for_green1), p_gamma_curve_for_green1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(p_gamma_curve_for_green2), p_gamma_curve_for_green2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(p_gamma_curve_for_blue1), p_gamma_curve_for_blue1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(p_gamma_curve_for_blue2), p_gamma_curve_for_blue2},
    {DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_out), sleep_out},
    {DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(disp_on), disp_on},
};

static struct dsi_cmd_desc lgd_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(disp_off), disp_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 60, sizeof(sleep_in), sleep_in},
};
static struct dsi_cmd_desc lgd_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sleep_out), sleep_out},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(disp_on), disp_on},
};

static int mipi_lgd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

    ENTER_FUNC2();
    
	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

       if (lgd_state.disp_initialized == false) {
           gpio_set_value_cansleep(gpio43, GPIO_LOW_VALUE);  // lcd panel reset  
           msleep(10);
           gpio_set_value_cansleep(gpio43, GPIO_HIGH_VALUE);  // lcd panel reset  
	   	   msleep(10);
           
           mipi_dsi_cmds_tx(mfd, &lgd_tx_buf, lgd_display_init_cmds,
                   ARRAY_SIZE(lgd_display_init_cmds));
           lgd_state.disp_initialized = true;
       }
	   mipi_dsi_cmds_tx(mfd, &lgd_tx_buf, lgd_display_on_cmds,
	   			ARRAY_SIZE(lgd_display_on_cmds));
       lgd_state.disp_on = true;
       
   EXIT_FUNC2();
	return 0;
}

static int mipi_lgd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
    
    ENTER_FUNC2();
    
	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
    
	if (lgd_state.disp_on == true) {
	   gpio_set_value_cansleep(gpio43, GPIO_LOW_VALUE); 
	   msleep(20);
	   gpio_set_value_cansleep(gpio43, GPIO_HIGH_VALUE);
	   msleep(10);
	   mipi_dsi_cmds_tx(mfd, &lgd_tx_buf, lgd_display_off_cmds,
			ARRAY_SIZE(lgd_display_off_cmds));
       lgd_state.disp_on = false;
       lgd_state.disp_initialized = false;
    }   
    
    EXIT_FUNC2();
	return 0;
}


static void mipi_lgd_set_backlight(struct msm_fb_data_type *mfd)
{

    int bl_value,i,level;
	unsigned long flags;
	ENTER_FUNC2();
   
#if 0  // for 30Mhz  Because of brightness, H/W required it at 30.7Mhz shinjg(shinbrad)
     if(level == 6)  // android default 
       level += 1;
#endif

	 level = mfd->bl_level;
     bl_value = 32-(level*2);
     

     printk("[%s] Current Backlight value : %d, Dimming loop value : %d \n",__FUNCTION__, level , bl_value);

     if(!level) {
        gpio_set_value(LCD_DIM_CON,GPIO_LOW_VALUE);
        udelay(T_DIS);

		  printk("[%s] Backlight OFF!....\n",__FUNCTION__);

		return;
     }

	  /* shinbrad shinjg */
	  local_save_flags(flags);
	  local_irq_disable();

     gpio_set_value(LCD_DIM_CON,GPIO_LOW_VALUE);
     udelay(T_DIS);
     gpio_set_value(LCD_DIM_CON,GPIO_HIGH_VALUE);
     
     udelay(T_ON);


     for(i=0;i<(bl_value)-1;i++) {
        gpio_set_value(LCD_DIM_CON,GPIO_LOW_VALUE);
        udelay(T_OFF);
        gpio_set_value(LCD_DIM_CON,GPIO_HIGH_VALUE);
     	udelay(T_ON);
      }
	  /* shinbrad shinjg */
	  local_irq_restore(flags);	
	  
	EXIT_FUNC2();

}

static struct msm_fb_panel_data lgd_panel_data = {
       .on             = mipi_lgd_on,
       .off            = mipi_lgd_off,
       .set_backlight  = mipi_lgd_set_backlight,
};

static int __devinit mipi_lgd_probe(struct platform_device *pdev)
{
    if (pdev->id == 0) {
        mipi_lgd_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_lgd_probe,
	.driver = {
		.name   = "mipi_lgd",
	},
};


static int ch_used[3];

int mipi_lgd_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_lgd", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	lgd_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &lgd_panel_data,
		sizeof(lgd_panel_data));
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

static int __init mipi_lgd_init(void)
{
    ENTER_FUNC2();

    lgd_state.disp_powered_up = true;

    mipi_dsi_buf_alloc(&lgd_tx_buf, DSI_BUF_SIZE);
    mipi_dsi_buf_alloc(&lgd_rx_buf, DSI_BUF_SIZE);

    EXIT_FUNC2();

    return platform_driver_register(&this_driver);
}
module_init(mipi_lgd_init);


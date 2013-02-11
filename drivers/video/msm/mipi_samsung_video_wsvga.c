/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
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
#include "mipi_samsung.h"

static struct msm_panel_info pinfo;

/* 20110420, kkcho, for EF39S
* 4.5'' WXGA (800*1280), 500Mbps
*/
#if 0
static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
/* DSI_BIT_CLK at 500MHz, 2 lane, RGB888 */
		{0x03, 0x01, 0x01, 0x00},	/* regulator */
		/* timing   */
		{0x82, 0x31, 0x13, 0x0, 0x42, 0x4D, 0x18,
		0x35, 0x21, 0x03, 0x04},
		{0x7f, 0x00, 0x00, 0x00},	/* phy ctrl */
		{0xee, 0x02, 0x86, 0x00},	/* strength */
		/* pll control */
		{0x40, 0xf9, 0xb0, 0xda, 0x00, 0x50, 0x48, 0x63,
		0x30, 0x07, 0x03,
		0x05, 0x14, 0x03, 0x0, 0x0, 0x54, 0x06, 0x10, 0x04, 0x0},
};
#endif
#if 0
static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
	/* DSI Bit Clock at 500 MHz, 2 lane, RGB888 */
	/* regulator */
	{0x03, 0x01, 0x01, 0x00},
	/* timing   */
	{0xb9, 0x8e, 0x1f, 0x00, 0x98, 0x9c, 0x22, 0x90,
	0x18, 0x03, 0x04},
	/* phy ctrl */
	{0x7f, 0x00, 0x00, 0x00},
	/* strength */
	{0xbb, 0x02, 0x06, 0x00},
	/* pll control */
	{0x00, 0xec, 0x31, 0xd2, 0x00, 0x40, 0x37, 0x62,
	0x01, 0x0f, 0x07,
	0x05, 0x14, 0x03, 0x0, 0x0, 0x0, 0x20, 0x0, 0x02, 0x0},
};
#endif
#if 0
static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
/* DSI_BIT_CLK at 500MHz, 4 lane, RGB888 */
		{0x03, 0x01, 0x01, 0x00},	/* regulator */
		//{0x03, 0x0a, 0x04, 0x00, 0x20},
		/* timing   */
		{0x82, 0x31, 0x13, 0x0, 0x42, 0x4D, 0x18,
		0x35, 0x21, 0x03, 0x04},
		//{0xab, 0x8a, 0x18, 0x00, 0x92, 0x97, 0x1b, 0x8c,
		//0x0c, 0x03, 0x04, 0xa0},
		//{0x7f, 0x00, 0x00, 0x00},	/* phy ctrl */
		{0x5f, 0x00, 0x00, 0x10},
		{0xee, 0x03, 0x86, 0x03},	/* strength */
		/* pll control */
		{0x40, 0xf9, 0xb0, 0xda, 0x00, 0x50, 0x48, 0x63,  
		0x31, 0x0f, 0x03,
		0x05, 0x14, 0x03, 0x0, 0x0, 0x54, 0x06, 0x10, 0x04, 0x0},
};
#endif

#if 1
// Toshiba
static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
	/* 600*1024, RGB888, 3 Lane 55 fps video mode */
    /* regulator */
	{0x03, 0x0a, 0x04, 0x00, 0x20},
	/* timing */
	{0xab, 0x8a, 0x18, 0x00, 0x92, 0x97, 0x1b, 0x8c,
	0x0c, 0x03, 0x04, 0xa0},
    /* phy ctrl */
	{0x5f, 0x00, 0x00, 0x10},
    /* strength */
	{0xff, 0x00, 0x06, 0x00},
	/* pll control */
	{0x0, 0x7f, 0x1, 0x1a, 0x00, 0x50, 0x48, 0x63,
	0x41, 0x0f, 0x01,
	0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
};
#endif

static int __init mipi_video_samsung_wvga_init(void)
{
    int ret;

#ifdef CONFIG_FB_MSM_MIPI_PANEL_DETECT
    if (msm_fb_detect_client("mipi_video_samsung_wvga"))
        return 0;
#endif

/* Try-case
ref-2. 20110420, kkcho
3030->3035 qualcomm-patch이후, 
mipi_dsi.c의 mipi_dsi_phy_pll_config(panel_info->clk_rate);  코드를 막아야 함. 
그렇지 않으면.. clk value변경으로 화면 안나옴. 
*/
        pinfo.xres = 800;
        pinfo.yres = 1280;
        pinfo.type = MIPI_VIDEO_PANEL;
        pinfo.pdest = DISPLAY_1;
        pinfo.wait_cycle = 0;
        pinfo.bpp = 24;

		//pinfo.mipi.xres_pad = 200;
		//pinfo.mipi.yres_pad = 0;
		pinfo.mipi.xres_pad = 0;
		pinfo.mipi.yres_pad = 1;

        pinfo.lcdc.h_back_porch = 80;//96;//48;//10;
        pinfo.lcdc.h_front_porch = 16;

        pinfo.lcdc.h_pulse_width = 2;
        pinfo.lcdc.v_back_porch = 4;
        pinfo.lcdc.v_front_porch = 2;
        pinfo.lcdc.v_pulse_width = 2;
        pinfo.lcdc.border_clr = 0;	/* blk */
        pinfo.lcdc.underflow_clr = 0xff;	/* blue */
        pinfo.lcdc.hsync_skew = 0;
        pinfo.bl_max = 125;
        pinfo.bl_min = 1;
        pinfo.fb_num = 2;

        //pinfo.clk_rate = 500000000;
		pinfo.clk_rate = 384000000;

  	    pinfo.lcd.vsync_enable = FALSE;
  	    pinfo.lcd.hw_vsync_mode = FALSE;

        pinfo.mipi.mode = DSI_VIDEO_MODE;
        pinfo.mipi.pulse_mode_hsa_he = TRUE;
        pinfo.mipi.hfp_power_stop = TRUE;
        pinfo.mipi.hbp_power_stop = TRUE;
        pinfo.mipi.hsa_power_stop = TRUE;
        pinfo.mipi.eof_bllp_power_stop = TRUE;
        pinfo.mipi.bllp_power_stop =TRUE;
        pinfo.mipi.traffic_mode = DSI_NON_BURST_SYNCH_PULSE; //DSI_NON_BURST_SYNCH_EVENT
        pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
        pinfo.mipi.vc = 0;
        pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
        pinfo.mipi.data_lane0 = TRUE;
        pinfo.mipi.data_lane1 = TRUE;
        pinfo.mipi.data_lane2 = TRUE;
        pinfo.mipi.data_lane3 = TRUE;

		pinfo.mipi.t_clk_post = 0x20;
		pinfo.mipi.t_clk_pre = 0x2d;
		//pinfo.mipi.t_clk_post = 0x04;//10;
		//pinfo.mipi.t_clk_pre = 0x1C;//30;
		pinfo.mipi.stream = 0; /* dma_p */
        pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
        pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
        pinfo.mipi.dsi_phy_db = &dsi_video_mode_phy_db;
	    pinfo.mipi.tx_eot_append = TRUE;

	    pinfo.mipi.frame_rate = 58;
        ret = mipi_samsung_device_register(&pinfo, MIPI_DSI_PRIM,
        					MIPI_DSI_PANEL_WVGA);
        if (ret)
        	printk(KERN_ERR "%s: failed to register device!\n", __func__);

        return ret;
}

module_init(mipi_video_samsung_wvga_init);

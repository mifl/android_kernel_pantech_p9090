/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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
 */

#include "msm_sensor.h"
#include <linux/regulator/machine.h> //F_S5K3H2_POWER
#include "sensor_i2c.h"
#include "sensor_ctrl.h"
#if 0//def CONFIG_PANTECH_CAMERA_FLASH
#include <linux/i2c.h>
#include "msm_camera_i2c.h"
#endif

#define SENSOR_NAME "s5k3h2"
#define PLATFORM_DRIVER_NAME "msm_camera_s5k3h2"
#define s5k3h2_obj s5k3h2_##obj

/*=============================================================
	SENSOR REGISTER DEFINES
==============================================================*/
/* Integration Time */
#define REG_COARSE_INTEGRATION_TIME		0x0202
/* Gain */
#define REG_GLOBAL_GAIN					0x0204
#define S5K3H2_OFFSET			8

#define REG_FRAME_LENGTH_LINES			0x0340
#define REG_LINE_LENGTH_PCK			0x342
/* Test Pattern */
#define REG_TEST_PATTERN_MODE			0x0601
#define REG_VCM_NEW_CODE			0x30F2

/* Mode Select */
#define S5K3H2_MODE_SEL			0x0100

/* Sensor Model ID */
#define S5K3H2_PIDH_REG		0x0000
#define S5K3H2_MODEL_ID			0x382B

//wsyang_temp
#define F_S5K3H2_POWER

#ifdef F_S5K3H2_POWER
#define CAM1_IOVDD_EN	0
#define CAM1_AVDD_EN	1
#define CAM1_DVDD_EN    2
#define CAM1_STANDBY    3
#define CAM1_RST_N      4
#define CAMIO_MAX       5

static sgpio_ctrl_t sgpios[CAMIO_MAX] = {
	{CAM1_IOVDD_EN, "CAM1_IOVDD_EN", 77},
	{CAM1_AVDD_EN, "CAM1_AVDD_EN", 82},
	{CAM1_DVDD_EN, "CAM1_DVDD_EN", 58},
	{CAM1_STANDBY, "CAM1_STANDBY", 54},	
	{CAM1_RST_N, "CAM1_RST_N", 107},
};

#define CAMV_AF_2P8V	0
#define CAMV_MAX	1

static svreg_ctrl_t svregs[CAMV_MAX] = {
	{CAMV_AF_2P8V,    "8921_l16",  NULL, 2800},
};
#endif

#if 0//def CONFIG_PANTECH_CAMERA_FLASH
struct tps61050_work {
	struct work_struct work;
};

static struct  tps61050_work *tps61050_sensorw;
static struct  i2c_client *tps61050_client;
#endif

DEFINE_MUTEX(s5k3h2_mut);
static struct msm_sensor_ctrl_t s5k3h2_s_ctrl;

static struct msm_camera_i2c_reg_conf s5k3h2_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k3h2_stop_settings[] = {
	{0x0100, 0x00},
};

#if 1
static struct msm_camera_i2c_reg_conf s5k3h2_groupon_settings[] = {
	{0x0104, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k3h2_groupoff_settings[] = {
	{0x0104, 0x00},
};
#endif
static struct msm_camera_i2c_reg_conf s5k3h2_prev_settings[] = {
#if 0 //old
	/*Timing configuration*/
	{0x0200, 0x02},/*FINE_INTEGRATION_TIME_*/
	{0x0201, 0x50},
	{0x0202, 0x04},/*COARSE_INTEGRATION_TIME*/
	{0x0203, 0xDB},
	{0x0204, 0x00},/*ANALOG_GAIN*/
	{0x0205, 0x20},
	{0x0342, 0x0D},/*LINE_LENGTH_PCK*/
	{0x0343, 0x8E},
	{0x0340, 0x04},/*FRAME_LENGTH_LINES*/
	{0x0341, 0xE0},
	/*Output Size (1640x1232)*/
	{0x0344, 0x00},/*X_ADDR_START*/
	{0x0345, 0x00},
	{0x0346, 0x00},/*Y_ADDR_START*/
	{0x0347, 0x00},
	{0x0348, 0x0C},/*X_ADDR_END*/
	{0x0349, 0xCD},
	{0x034A, 0x09},/*Y_ADDR_END*/
	{0x034B, 0x9F},
	{0x0381, 0x01},/*X_EVEN_INC*/
	{0x0383, 0x03},/*X_ODD_INC*/
	{0x0385, 0x01},/*Y_EVEN_INC*/
	{0x0387, 0x03},/*Y_ODD_INC*/
	{0x0401, 0x00},/*DERATING_EN*/
	{0x0405, 0x10},
	{0x0700, 0x05},/*FIFO_WATER_MARK_PIXELS*/
	{0x0701, 0x30},
	{0x034C, 0x06},/*X_OUTPUT_SIZE*/
	{0x034D, 0x68},
	{0x034E, 0x04},/*Y_OUTPUT_SIZE*/
	{0x034F, 0xD0},
	/*Manufacture Setting*/
	{0x300E, 0xED},
	{0x301D, 0x80},
	{0x301A, 0x77},
#endif
    /* 2012_01_02_LSI recommend */    
    //  PLL
    {0x0305,    0x04},  // pre_pll_clk_div = 5
    {0x0306, 0x00},  // pll_multiplier 
    {0x0307, 0x6C},  // pll_multiplier  = 108
    {0x0303, 0x01},  // vt_sys_clk_div = 1
    {0x0301, 0x05},  // vt_pix_clk_div = 5
    {0x030B, 0x01},  // op_sys_clk_div = 1
    {0x0309, 0x05},  // op_pix_clk_div = 5
    {0x30CC,0xB0},   // DPHY_band_ctrl 640¢¦690MHz
    {0x31A1, 0x58},     
    // Readout   H:1/2 SubSampling binning, V:1/2 SubSampling binning
    {0x0344, 0x00},  // X addr start 0d
    {0x0345, 0x00},  // 
    {0x0346, 0x00},  // Y addr start 0d
    {0x0347, 0x00},  // 
    {0x0348, 0x0C},  // X addr end 3279d
    {0x0349, 0xCF},  // 
    {0x034A, 0x09},  // Y addr end 2463d
    {0x034B, 0x9F},  // 
    {0x0381, 0x01},  // x_even_inc = 1
    {0x0383, 0x03},  // x_odd_inc = 3
    {0x0385, 0x01},  // y_even_inc = 1
    {0x0387, 0x03},  // y_odd_inc = 3
    {0x0401, 0x00},  // Derating_en  = 0 (disable)
    {0x0405, 0x10},  // 
    {0x0700, 0x05},  // fifo_water_mark_pixels = 1328
    {0x0701, 0x30},  // 
    {0x034C, 0x06},  // x_output_size = 1640
    {0x034D, 0x68},  // 
    {0x034E, 0x04},  // y_output_size = 1232
    {0x034F, 0xD0},  // 
    {0x0200, 0x02},  // fine integration time
    {0x0201, 0x50},  // 
    {0x0202, 0x04},  // Coarse integration time
    {0x0203, 0xDB},  // 
    {0x0204, 0x00},  // Analog gain
    {0x0205, 0x20},  // 
    {0x0342, 0x0D},  // Line_length_pck 3470d
    {0x0343, 0x8E},  // 
    {0x0340, 0x04},  // Frame_length_lines 1260d
    {0x0341, 0xEC},  //      
    //Manufacture Setting    
    {0x300E,0x2D},
    {0x31A3,0x40},
    {0x301A,0x77},
    {0x3053,0xCF},
};

static struct msm_camera_i2c_reg_conf s5k3h2_snap_settings[] = {
#if 0 //old    
	/*Timing configuration*/
	{0x0200, 0x02},/*FINE_INTEGRATION_TIME_*/
	{0x0201, 0x50},
	{0x0202, 0x04},/*COARSE_INTEGRATION_TIME*/
	{0x0203, 0xE7},
	{0x0204, 0x00},/*ANALOG_GAIN*/
	{0x0205, 0x20},
	{0x0342, 0x0D},/*LINE_LENGTH_PCK*/
	{0x0343, 0x8E},
	{0x0340, 0x09},/*FRAME_LENGTH_LINES*/
	{0x0341, 0xC0},
	/*Output Size (3280x2464)*/
	{0x0344, 0x00},/*X_ADDR_START*/
	{0x0345, 0x00},
	{0x0346, 0x00},/*Y_ADDR_START*/
	{0x0347, 0x00},
	{0x0348, 0x0C},/*X_ADDR_END*/
	{0x0349, 0xCF},
	{0x034A, 0x09},/*Y_ADDR_END*/
	{0x034B, 0x9F},
	{0x0381, 0x01},/*X_EVEN_INC*/
	{0x0383, 0x01},/*X_ODD_INC*/
	{0x0385, 0x01},/*Y_EVEN_INC*/
	{0x0387, 0x01},/*Y_ODD_INC*/
	{0x0401, 0x00},/*DERATING_EN*/
	{0x0405, 0x10},
	{0x0700, 0x05},/*FIFO_WATER_MARK_PIXELS*/
	{0x0701, 0x30},
	{0x034C, 0x0C},/*X_OUTPUT_SIZE*/
	{0x034D, 0xD0},
	{0x034E, 0x09},/*Y_OUTPUT_SIZE*/
	{0x034F, 0xA0},
	/*Manufacture Setting*/
	{0x300E, 0xE9},
	{0x301D, 0x00},
	{0x301A, 0x77},
#endif
    /* 2012_01_02_LSI recommend */ 
    //  PLL
    {0x0305,    0x04},  //  pre_pll_clk_div = 4
    {0x0306, 0x00},  // pll_multiplier 
    {0x0307, 0x6C},  // pll_multiplier  = 108
    {0x0303, 0x01},  // vt_sys_clk_div = 1
    {0x0301, 0x05},  // vt_pix_clk_div = 5
    {0x030B,0x01},   // op_sys_clk_div = 1
    {0x0309, 0x05},  // op_pix_clk_div = 5
    {0x30CC,0xB0},   // DPHY_band_ctrl 640¢¦690MHz
    {0x31A1,0x58},     
    // Readout   H:1/2 SubSampling binning, V:1/2 SubSampling binning
    {0x0344, 0x00},  //  X addr start 0d
    {0x0345, 0x00},  // 
    {0x0346, 0x00},  // Y addr start 0d
    {0x0347, 0x00},  // 
    {0x0348, 0x0C},  // X addr end 3279d
    {0x0349, 0xCF},  // 
    {0x034A, 0x09},  // Y addr end 2463d
    {0x034B,0x9F},   // 
    {0x0381, 0x01},  // x_even_inc = 1
    {0x0383, 0x01},  // x_odd_inc = 1
    {0x0385, 0x01},  // y_even_inc = 1
    {0x0387, 0x01},  // y_odd_inc = 1
    {0x0401, 0x00},  // Derating_en  = 0 (disable)
    {0x0405, 0x10},  // 
    {0x0700, 0x05},  // fifo_water_mark_pixels = 1328
    {0x0701, 0x30},  // 
    {0x034C,0x0C},   // x_output_size = 3280
    {0x034D,0xD0},   // 
    {0x034E, 0x09},  // y_output_size = 2464
    {0x034F, 0xA0},  // 
    {0x0200, 0x02},  // fine integration time
    {0x0201, 0x50},  // 
    {0x0202, 0x04},  // Coarse integration time
    {0x0203, 0xE7},  // 
    {0x0204, 0x00},  // Analog gain
    {0x0205, 0x20},  // 
    {0x0342, 0x0D},  // Line_length_pck 3470d
    {0x0343, 0x8E},  // 
    {0x0340, 0x09},  // Frame_length_lines 2500d
    {0x0341, 0xC4},  //      
    //Manufacture Setting    
    {0x300E,0x29},
    {0x31A3,0x00},
    {0x301A,0x77},
    {0x3053,0xCF},
};

static struct msm_camera_i2c_reg_conf s5k3h2_recommend_settings[] = {
#if 0 //old
	/*mipi setting*/
	{0x0101, 0x00},
	{0x3065, 0x35},
	{0x310E, 0x00},
	{0x3098, 0xAB},
	{0x30C7, 0x0A},
	{0x309A, 0x01},
	{0x310D, 0xC6},
	{0x30c3, 0x40},
	{0x30BB, 0x02},
	{0x30BC, 0x38},
	{0x30BD, 0x40},
	{0x3110, 0x70},
	{0x3111, 0x80},
	{0x3112, 0x7B},
	{0x3113, 0xC0},
	{0x30C7, 0x1A},
	/*recommend setting*/
	{0x3000, 0x08},
	{0x3001, 0x05},
	{0x3002, 0x0D},
	{0x3003, 0x21},
	{0x3004, 0x62},
	{0x3005, 0x0B},
	{0x3006, 0x6D},
	{0x3007, 0x02},
	{0x3008, 0x62},
	{0x3009, 0x62},
	{0x300A, 0x41},
	{0x300B, 0x10},
	{0x300C, 0x21},
	{0x300D, 0x04},
	{0x307E, 0x03},
	{0x307F, 0xA5},
	{0x3080, 0x04},
	{0x3081, 0x29},
	{0x3082, 0x03},
	{0x3083, 0x21},
	{0x3011, 0x5F},
	{0x3156, 0xE2},
	{0x3027, 0x0E},
	{0x300f, 0x02},
	{0x3010, 0x10},
	{0x3017, 0x74},
	{0x3018, 0x00},
	{0x3020, 0x02},
	{0x3021, 0x24},
	{0x3023, 0x80},
	{0x3024, 0x04},
	{0x3025, 0x08},
	{0x301C, 0xD4},
	{0x315D, 0x00},
	{0x3053, 0xCF},
	{0x3054, 0x00},
	{0x3055, 0x35},
	{0x3062, 0x04},
	{0x3063, 0x38},
	{0x31A4, 0x04},
	{0x3016, 0x45},
	{0x3157, 0x02},
	{0x3158, 0x00},
	{0x315B, 0x02},
	{0x315C, 0x00},
	{0x301B, 0x05},
	{0x3028, 0x41},
	{0x302A, 0x00},
	{0x3060, 0x01},
	{0x302D, 0x19},
	{0x302B, 0x04},
	{0x3072, 0x13},
	{0x3073, 0x21},
	{0x3074, 0x82},
	{0x3075, 0x20},
	{0x3076, 0xA2},
	{0x3077, 0x02},
	{0x3078, 0x91},
	{0x3079, 0x91},
	{0x307A, 0x61},
	{0x307B, 0x28},
	{0x307C, 0x31},
	/*PLL setting*/
	{0x0305, 0x04},/*PRE_PLL_CLK_DIV*/
	{0x0306, 0x00},/*PLL_MULTIPLIER*/
	{0x0307, 0x6C},/*PLL_MULTIPLIER*/
	{0x0303, 0x01},/*VT_SYS_CLK_DIV*/
	{0x0301, 0x05},/*VT_PIX_CLK_DIV*/
	{0x030B, 0x01},/*OP_SYS_CLK_DIV*/
	{0x0309, 0x05},/*OP_PIX_CLK_DIV*/
	{0x30CC, 0xB0},/*DPHY_BAND_CTRL*/
	{0x31A1, 0x56},/*BINNING*/
#endif
    /* 2012_01_02_LSI recommend */  
    /* Flip/Mirror Setting*/   
    {0x0101,    0x00},  //Flip/Mirror ON 0x03      OFF 0x00   
    /* MIPI Setting */     
    {0x3065,     0x35}, 
    {0x310E,  0x00}, 
    {0x3098,  0xAB},  
    {0x30C7, 0x0A},  
    {0x309A,  0x01}, 
    {0x310D, 0xC6},  
    {0x30c3,  0x40}, 
    {0x30BB, 0x02},  
    {0x30BC, 0x38},  //According to MCLK, these registers should be changed!
    {0x30BD, 0x40},  
    {0x3110, 0x70},  
    {0x3111, 0x80},  
    {0x3112, 0x7B},  
    {0x3113, 0xC0},  
    {0x30C7, 0x1A },
    /* Manufacture Setting*/
    {0x3000, 0x08},  
    {0x3001, 0x05},  
    {0x3002, 0x0D},  
    {0x3003, 0x21},  
    {0x3004, 0x62},  
    {0x3005, 0x0B},  
    {0x3006, 0x6D},  
    {0x3007, 0x02},  
    {0x3008, 0x62},  
    {0x3009, 0x62},  
    {0x300A, 0x41},  
    {0x300B, 0x10},  
    {0x300C, 0x21},  
    {0x300D, 0x04},  
    {0x307E, 0x03},  
    {0x307F, 0xA5},  
    {0x3080, 0x04},  
    {0x3081, 0x29},  
    {0x3082, 0x03},  
    {0x3083, 0x21},  
    {0x3011, 0x5F},  
    {0x3156, 0xE2},  
    {0x3027, 0xBE},  //DBR_CLK enable for EMI
    {0x300f, 0x02},  
    {0x3010, 0x10},  
    {0x3017, 0x74},  
    {0x3018, 0x00},  
    {0x3020, 0x02},  
    {0x3021, 0x00},  
    {0x3023, 0x80},  
    {0x3024, 0x08},  
    {0x3025, 0x08},  
    {0x301C, 0xD4},  
    {0x315D, 0x00},  
    {0x3054, 0x00},  
    {0x3055, 0x35},  
    {0x3062, 0x04},  
    {0x3063, 0x38},  
    {0x31A4, 0x04},  
    {0x3016, 0x54},  
    {0x3157, 0x02},  
    {0x3158, 0x00},  
    {0x315B, 0x02},  
    {0x315C, 0x00},  
    {0x301B, 0x05},  
    {0x3028, 0x41},  
    {0x302A, 0x10},  
    {0x3060, 0x00},  
    {0x302D, 0x19},  
    {0x302B, 0x05},  
    {0x3072, 0x13},  
    {0x3073, 0x21},  
    {0x3074, 0x82},  
    {0x3075, 0x20},  
    {0x3076, 0xA2},  
    {0x3077, 0x02},  
    {0x3078, 0x91},  
    {0x3079, 0x91},  
    {0x307A, 0x61},  
    {0x307B, 0x28},  
    {0x307C, 0x31},  
    {0x304E, 0x40},  //Pedestal
    {0x304F, 0x01},  //Pedestal
    {0x3050, 0x00},  //Pedestal
    {0x3088, 0x01},  //Pedestal
    {0x3089, 0x00},  //Pedestal
    {0x3210, 0x01},  //Pedestal
    {0x3211, 0x00},  //Pedestal
    {0x308E, 0x01},  
    {0x308F, 0x8F},  
    {0x3064, 0x03},  
    {0x31A7, 0x0F},  
};

static struct v4l2_subdev_info s5k3h2_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array s5k3h2_init_conf[] = {
	{&s5k3h2_recommend_settings[0],
	ARRAY_SIZE(s5k3h2_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array s5k3h2_confs[] = {
	{&s5k3h2_snap_settings[0],
	ARRAY_SIZE(s5k3h2_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&s5k3h2_prev_settings[0],
	ARRAY_SIZE(s5k3h2_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};


#define S5K3H2_FULL_SIZE_DUMMY_PIXELS     0
#define S5K3H2_FULL_SIZE_DUMMY_LINES    0
#define S5K3H2_FULL_SIZE_WIDTH    3280 //1936;//3280; //1632;//3280;
#define S5K3H2_FULL_SIZE_HEIGHT   2464//1096;//2456;//1224;//2464; //

#define S5K3H2_QTR_SIZE_DUMMY_PIXELS  0
#define S5K3H2_QTR_SIZE_DUMMY_LINES   0
//changed by maliang for s5k3h2 preview 1632x1224 begin
#define  S5K3H2_QTR_SIZE_WIDTH        1640//1936;//1632;//1936;//3280;//1920
#define S5K3H2_QTR_SIZE_HEIGHT        1232//1456;//1224;//1096;//2464;//1080//1096

//changed by maliang for s5k3h2 preview 1632x720 end
#define S5K3H2_HRZ_FULL_BLK_PIXELS   190//304//696; //890;//
#define S5K3H2_VER_FULL_BLK_LINES     32//24//44; //44;

#define S5K3H2_HRZ_QTR_BLK_PIXELS    1830//1884//1648;//890;//888; //696;
#define S5K3H2_VER_QTR_BLK_LINES      16//40//924;//44;

#if 0//def CONFIG_PANTECH_CAMERA_FLASH
static DECLARE_WAIT_QUEUE_HEAD(tps61050_wait_queue);

static struct msm_camera_i2c_client tps61050_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};

static int tps61050_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&tps61050_wait_queue);
	return 0;
}


int32_t s5k3h2_sensor_flash(int mode)
{
	int rc;
    uint16_t timeout = 0;
//  int cnt=0;
	
	printk("%s start mode = %d\n",__func__, mode);
	
	switch(mode) {
		case 0://INIT_MODE:
			rc = gpio_request(2, "FLASH_CNTL_EN");
			if (!rc) {
			    printk("%s:%d INIT_MODE \n", __func__, __LINE__);
			    gpio_direction_output(2, 1);
			} else {
				printk("%s: gpio FLASH_CNTL_EN request fail", __func__);
			}
			mdelay(1);
#if 0
			rc = gpio_request(4, "TORCH_UP");
			if (!rc) {
			    printk("%s:%d\n", __func__, __LINE__);
			    gpio_direction_output(4, 0);
			} else {
				printk("%s: gpio TORCH_UP request fail", __func__);
			}
		    	mdelay(1);
#endif			
			msm_camera_i2c_write(&tps61050_sensor_i2c_client, 0x02, 0x40, MSM_CAMERA_I2C_BYTE_DATA);
//			msm_camera_i2c_write(&tps61050_sensor_i2c_client, 0x03, 0xD1, MSM_CAMERA_I2C_BYTE_DATA); // 557.6ms
			msm_camera_i2c_write(&tps61050_sensor_i2c_client, 0x03, 0xDF, MSM_CAMERA_I2C_BYTE_DATA); // 1016.8ms
            msm_camera_i2c_write(&tps61050_sensor_i2c_client, 0x01, 0x8C, MSM_CAMERA_I2C_BYTE_DATA); //flash 500mA
			break;
		case 1://TORCH_MODE:
			msm_camera_i2c_write(&tps61050_sensor_i2c_client, 0x00, 0x92, MSM_CAMERA_I2C_BYTE_DATA);
			break;
		case 2://FLASH_MODE:
            msm_camera_i2c_read(&tps61050_sensor_i2c_client, 0x01, &timeout, MSM_CAMERA_I2C_BYTE_DATA);
            printk("%s: FLASH_MODE read = %d\n", __func__,timeout);
            if(timeout & 0x20) {
//    		    msm_camera_i2c_write(&tps61050_sensor_i2c_client, 0x01, 0x8C+cnt, MSM_CAMERA_I2C_BYTE_DATA);
//              cnt++;
//              if(cnt>3) cnt=0;
                msm_camera_i2c_write(&tps61050_sensor_i2c_client, 0x01, 0x8C, MSM_CAMERA_I2C_BYTE_DATA); //flash 500mA
            }
			break;
		case 3://OFF_MODE:
			gpio_set_value_cansleep(2, 0);
			gpio_free(2);
	    	mdelay(1);
			break;
	}

	printk("%s end\n",__func__);
	return 0;

}

static int tps61050_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	printk("%s start\n",__func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	tps61050_sensorw = kzalloc(sizeof(struct tps61050_work), GFP_KERNEL);
	if (!tps61050_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, tps61050_sensorw);
	tps61050_init_client(client);
	tps61050_client = client;
	tps61050_sensor_i2c_client.client = client;

	printk("tps61050_probe succeeded!\n");

	return 0;

probe_failure:
	kfree(tps61050_sensorw);
	tps61050_sensorw = NULL;
	printk("tps61050_probe failed!\n");
	return rc;
}

static const struct i2c_device_id tps61050_i2c_id[] = {
	{ "tps61050_flash", 0},
	{ },
};

static struct i2c_driver tps61050_i2c_driver = {
	.id_table = tps61050_i2c_id,
	.probe  = tps61050_i2c_probe,
	.remove = __exit_p(tps61050_i2c_remove),
	.driver = {
		.name = "tps61050_flash",
	},
};

static int32_t tps61050_init_i2c(void)
{
	int32_t rc = 0;

	printk("%s start\n",__func__);

	rc = i2c_add_driver(&tps61050_i2c_driver);
	printk("%s tps61050_i2c_driver rc = %d\n",__func__, rc);
	if (IS_ERR_VALUE(rc))
		goto init_i2c_fail;

	printk("%s end\n",__func__);
	return 0;

init_i2c_fail:
	printk("%s failed! (%d)\n", __func__, rc);
	i2c_del_driver(&tps61050_i2c_driver);
	tps61050_client = NULL;
    
	return rc;
}
#endif

static struct msm_sensor_output_info_t s5k3h2_dimensions[] = {
	{
		.x_output = S5K3H2_FULL_SIZE_WIDTH,//0x1070,
		.y_output = S5K3H2_FULL_SIZE_HEIGHT,
		.line_length_pclk = S5K3H2_FULL_SIZE_WIDTH + S5K3H2_HRZ_FULL_BLK_PIXELS ,
		.frame_length_lines = S5K3H2_FULL_SIZE_HEIGHT+ S5K3H2_VER_FULL_BLK_LINES ,
		.vt_pixel_clk = 129600000,
		.op_pixel_clk = 129600000,
		.binning_factor = 1,
	},
	{
		.x_output = S5K3H2_QTR_SIZE_WIDTH,
		.y_output = S5K3H2_QTR_SIZE_HEIGHT,
		.line_length_pclk = S5K3H2_QTR_SIZE_WIDTH + S5K3H2_HRZ_QTR_BLK_PIXELS,
		.frame_length_lines = S5K3H2_QTR_SIZE_HEIGHT+ S5K3H2_VER_QTR_BLK_LINES,
		.vt_pixel_clk = 129600000,
		.op_pixel_clk = 129600000,
		.binning_factor = 1,
	},
};

static struct msm_camera_csid_vc_cfg s5k3h2_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
};

static struct msm_camera_csi2_params s5k3h2_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 2,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = s5k3h2_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 2,
		.settle_cnt = 0x1B,
	},
};

static struct msm_camera_csi2_params *s5k3h2_csi_params_array[] = {
	&s5k3h2_csi_params,
	&s5k3h2_csi_params,
};

static struct msm_sensor_output_reg_addr_t s5k3h2_reg_addr = {
	.x_output = 0x034C,
	.y_output = 0x034E,
	.line_length_pclk = REG_LINE_LENGTH_PCK,
	.frame_length_lines = REG_FRAME_LENGTH_LINES,
};


static struct msm_sensor_id_info_t s5k3h2_id_info = {
	.sensor_id_reg_addr = S5K3H2_PIDH_REG,
	.sensor_id = S5K3H2_MODEL_ID,
};

#define S5K3H2_GAIN         0x350B//0x3000
static struct msm_sensor_exp_gain_info_t s5k3h2_exp_gain_info = {
	.coarse_int_time_addr = REG_COARSE_INTEGRATION_TIME,
	.global_gain_addr = REG_GLOBAL_GAIN,
	.vert_offset = 3,
};

#if 1//def F_PANTECH_CAMERA_S5K3H2
int32_t msm_sensor_write_exp_gain_s5k3h2(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{
	uint16_t max_legal_gain = 0x0200;
	uint16_t min_ll_pck = 0x0AB2;
	uint32_t ll_pck, fl_lines;
	uint32_t ll_ratio;
	int32_t rc = 0;
	uint16_t prev_frame_length_lines = s5k3h2_dimensions[0].frame_length_lines;//msm_sensor_get_prev_lines_pf(s_ctrl);
	uint16_t prev_line_length_pck = s5k3h2_dimensions[0].line_length_pclk;//msm_sensor_get_prev_pixels_pl(s_ctrl);
	uint16_t snap_frame_length_lines = s5k3h2_dimensions[1].frame_length_lines;//msm_sensor_get_pict_lines_pf(s_ctrl);
	uint16_t snap_line_length_pck = s5k3h2_dimensions[1].line_length_pclk;//msm_sensor_get_pict_pixels_pl(s_ctrl);

	printk("%s : prev_frame_length_lines = %d\n", __func__, prev_frame_length_lines);
	printk("%s : prev_line_length_pck = %d\n", __func__, prev_line_length_pck);
	printk("%s : snap_frame_length_lines = %d\n", __func__, snap_frame_length_lines);
	printk("%s : snap_line_length_pck = %d\n", __func__, snap_line_length_pck);

	if (gain > max_legal_gain) {
		printk("Max legal gain Line:%d\n", __LINE__);
		gain = max_legal_gain;
	}

	if (s_ctrl->curr_res == MSM_SENSOR_RES_QTR) {
		//s5k3h2_ctrl->my_reg_gain = gain;
		//s5k3h2_ctrl->my_reg_line_count = (uint16_t) line;
		fl_lines = prev_frame_length_lines;
		ll_pck = prev_line_length_pck;
		ll_ratio = prev_line_length_pck * Q10 /
			(prev_frame_length_lines - 8);
	} else {
		fl_lines = snap_frame_length_lines;
		ll_pck = snap_line_length_pck;
		ll_ratio = (snap_line_length_pck * Q10 /
			(snap_frame_length_lines - 8));
	}

	if ((fl_lines - 8) < line) {
		ll_pck = line * ll_ratio / Q10;
		line = (fl_lines - 8);
	}

	if (ll_pck < min_ll_pck)
		ll_pck = min_ll_pck;

	printk("%s : gain = %d\n", __func__, gain);
	printk("%s : ll_pck = %d\n", __func__, ll_pck);
	printk("%s : line = %d\n", __func__, line);

#if 0//wsyang_temp for exposure tuning
	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0204, gain,MSM_CAMERA_I2C_WORD_DATA);
	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x342, ll_pck, MSM_CAMERA_I2C_WORD_DATA);//ll_pck);
	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0202, line, MSM_CAMERA_I2C_WORD_DATA);
	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
#endif

#if 1//def CONFIG_PANTECH_CAMERA_FLASH //for HW test
//	s_ctrl->func_tbl->sensor_flash_ctl(2);
#endif

	return rc;
}
#endif



//static struct sensor_calib_data s5k3h2_calib_data;


static const struct i2c_device_id s5k3h2_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&s5k3h2_s_ctrl},
	{ }
};

static struct i2c_driver s5k3h2_i2c_driver = {
	.id_table = s5k3h2_i2c_id,
	.probe  = msm_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};


static struct msm_camera_i2c_client s5k3h2_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};
#if 0
static struct msm_camera_i2c_client s5k3h2_eeprom_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};

static struct msm_camera_eeprom_read_t s5k3h2_eeprom_read_tbl[] = {
	{0x10, &s5k3h2_calib_data.r_over_g, 2, 1},
	{0x12, &s5k3h2_calib_data.b_over_g, 2, 1},
	{0x14, &s5k3h2_calib_data.gr_over_gb, 2, 1},
};

static struct msm_camera_eeprom_data_t s5k3h2_eeprom_data_tbl[] = {
	{&s5k3h2_calib_data, sizeof(struct sensor_calib_data)},
};

static struct msm_camera_eeprom_client s5k3h2_eeprom_client = {
	.i2c_client = &s5k3h2_eeprom_i2c_client,
	.i2c_addr = 0xA4,

	.func_tbl = {
		.eeprom_set_dev_addr = NULL,
		.eeprom_init = msm_camera_eeprom_init,
		.eeprom_release = msm_camera_eeprom_release,
		.eeprom_get_data = msm_camera_eeprom_get_data,
	},

	.read_tbl = s5k3h2_eeprom_read_tbl,
	.read_tbl_size = ARRAY_SIZE(s5k3h2_eeprom_read_tbl),
	.data_tbl = s5k3h2_eeprom_data_tbl,
	.data_tbl_size = ARRAY_SIZE(s5k3h2_eeprom_data_tbl),
};
#endif

#ifdef F_S5K3H2_POWER
static int s5k3h2_vreg_init(void)
{
    int rc = 0;

    printk("%s E\n", __func__);
    
    rc = sgpio_init(sgpios, CAMIO_MAX);
    if (rc < 0)
        goto sensor_init_fail;

    rc = svreg_init(svregs, CAMV_MAX);
    if (rc < 0)
        goto sensor_init_fail;
    
    printk("%s X\n", __func__);
    return 0;
    
sensor_init_fail:
    svreg_release(svregs, CAMV_MAX);
    sgpio_release(sgpios, CAMIO_MAX);
    return -ENODEV;
}


int32_t s5k3h2_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	printk("%s: %d\n", __func__, __LINE__);
#if 0
    msm_sensor_probe_on(&s_ctrl->sensor_i2c_client->client->dev);
    SKYCDBG("%s msm_sensor_probe_on ok\n", __func__); 
    msm_camio_clk_rate_set(MSM_SENSOR_MCLK_24HZ);
    SKYCDBG("%s msm_camio_clk_rate_set ok\n", __func__);
#else
    rc = msm_sensor_power_up(s_ctrl);
    printk(" %s : msm_sensor_power_up : rc = %d E\n",__func__, rc);  
#endif

    s5k3h2_vreg_init();

    if (sgpio_ctrl(sgpios, CAM1_RST_N, 0) < 0)     rc = -EIO;
    mdelay(1); 
    if (sgpio_ctrl(sgpios, CAM1_AVDD_EN, 1) < 0)    rc = -EIO;
    mdelay(1); 
    if (sgpio_ctrl(sgpios, CAM1_DVDD_EN, 1) < 0)     rc = -EIO;    
    mdelay(1); 
    if (svreg_ctrl(svregs, CAMV_AF_2P8V, 1) < 0)    rc = -EIO;
    mdelay(1); 
    if (sgpio_ctrl(sgpios, CAM1_IOVDD_EN, 1) < 0)   rc = -EIO;
    mdelay(1); 
    
    //(void)msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
    //msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);
    printk(" msm_camio_clk_rate_set E\n");    
    //msm_camio_clk_rate_set(24000000);
    printk(" msm_camio_clk_rate_set X\n");
    mdelay(1);

    if (sgpio_ctrl(sgpios, CAM1_STANDBY, 1) < 0)     rc = -EIO;
    msleep(10); /* > 50us */
    if (sgpio_ctrl(sgpios, CAM1_RST_N, 1) < 0)     rc = -EIO;
    mdelay(1); /* > 50us */

    printk("%s X (%d)\n", __func__, rc);
    return rc;
}

int32_t s5k3h2_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
    int32_t rc = 0;
	printk("%s\n", __func__);
#if 0
    msm_sensor_probe_off(&s_ctrl->sensor_i2c_client->client->dev);
#else
    msm_sensor_power_down(s_ctrl);
    printk(" %s : msm_sensor_power_down : rc = %d E\n",__func__, rc);  
#endif
    
    if (sgpio_ctrl(sgpios, CAM1_RST_N, 0) < 0)   rc = -EIO;
    mdelay(1); 
    if (sgpio_ctrl(sgpios, CAM1_STANDBY, 0) < 0)     rc = -EIO;
    mdelay(1); 
    if (sgpio_ctrl(sgpios, CAM1_DVDD_EN, 0) < 0)     rc = -EIO;    
    mdelay(1); 
    if (sgpio_ctrl(sgpios, CAM1_AVDD_EN, 0) < 0)    rc = -EIO;
    mdelay(1); 
    if (svreg_ctrl(svregs, CAMV_AF_2P8V, 0) < 0)    rc = -EIO;
    mdelay(1); 
    if (sgpio_ctrl(sgpios, CAM1_IOVDD_EN, 0) < 0)   rc = -EIO;
    mdelay(1); 

	svreg_release(svregs, CAMV_MAX);
	sgpio_release(sgpios, CAMIO_MAX);
    
    printk("%s X (%d)\n", __func__, rc);
    return rc;
}
#endif

static int __init msm_sensor_init_module(void)
{
	return i2c_add_driver(&s5k3h2_i2c_driver);
}

static struct v4l2_subdev_core_ops s5k3h2_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};
static struct v4l2_subdev_video_ops s5k3h2_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops s5k3h2_subdev_ops = {
	.core = &s5k3h2_subdev_core_ops,
	.video  = &s5k3h2_subdev_video_ops,
};

static struct msm_sensor_fn_t s5k3h2_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
#if 0	
	.sensor_get_prev_lines_pf = msm_sensor_get_prev_lines_pf,
	.sensor_get_prev_pixels_pl = msm_sensor_get_prev_pixels_pl,
	.sensor_get_pict_lines_pf = msm_sensor_get_pict_lines_pf,
	.sensor_get_pict_pixels_pl = msm_sensor_get_pict_pixels_pl,
	.sensor_get_pict_max_exp_lc = msm_sensor_get_pict_max_exp_lc,
	.sensor_get_pict_fps = msm_sensor_get_pict_fps,
#endif	
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = msm_sensor_write_exp_gain_s5k3h2,
	.sensor_write_snapshot_exp_gain = msm_sensor_write_exp_gain_s5k3h2,
	.sensor_setting = msm_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
#ifdef F_S5K3H2_POWER
	.sensor_power_up = s5k3h2_sensor_power_up,//msm_sensor_power_up,
	.sensor_power_down = s5k3h2_sensor_power_down,//msm_sensor_power_down,
#else
    .sensor_power_up = msm_sensor_power_up,
    .sensor_power_down = msm_sensor_power_down,
#endif
#if 0//def CONFIG_PANTECH_CAMERA_FLASH
    	.sensor_flash_ctl = s5k3h2_sensor_flash,
#endif	
};

static struct msm_sensor_reg_t s5k3h2_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = s5k3h2_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(s5k3h2_start_settings),
	.stop_stream_conf = s5k3h2_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(s5k3h2_stop_settings),
	.group_hold_on_conf = s5k3h2_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(s5k3h2_groupon_settings),
	.group_hold_off_conf = s5k3h2_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(s5k3h2_groupoff_settings),
	.init_settings = &s5k3h2_init_conf[0],
	.init_size = ARRAY_SIZE(s5k3h2_init_conf),
	.mode_settings = &s5k3h2_confs[0],
	.output_settings = &s5k3h2_dimensions[0],
	.num_conf = ARRAY_SIZE(s5k3h2_confs),
};

static struct msm_sensor_ctrl_t s5k3h2_s_ctrl = {
	.msm_sensor_reg = &s5k3h2_regs,
	.sensor_i2c_client = &s5k3h2_sensor_i2c_client,
	.sensor_i2c_addr = 0x6E,//0x20,//0x6C,//34,
//	.sensor_eeprom_client = &s5k3h2_eeprom_client,
	.sensor_output_reg_addr = &s5k3h2_reg_addr,
	.sensor_id_info = &s5k3h2_id_info,
	.sensor_exp_gain_info = &s5k3h2_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &s5k3h2_csi_params_array[0],
	.msm_sensor_mutex = &s5k3h2_mut,
	.sensor_i2c_driver = &s5k3h2_i2c_driver,
	.sensor_v4l2_subdev_info = s5k3h2_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(s5k3h2_subdev_info),
	.sensor_v4l2_subdev_ops = &s5k3h2_subdev_ops,
	.func_tbl = &s5k3h2_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Omivision 8MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");

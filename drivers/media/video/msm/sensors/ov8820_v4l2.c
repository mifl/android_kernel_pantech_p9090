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
#include <linux/regulator/machine.h> //F_OV8820_POWER
#include "sensor_i2c.h"
#include "sensor_ctrl.h"
#if 0//def CONFIG_PANTECH_CAMERA_FLASH
#include <linux/i2c.h>
#include "msm_camera_i2c.h"
#endif
#ifdef CONFIG_OV8820_ACT//CONFIG_PANTECH_CAMERA_AF_OFF_STEP
#include "../actuators/msm_actuator.h"
struct msm_actuator_ctrl_t *AF_off_step_ctrl;
int AF_off_step_check = 0;
#endif

#define SENSOR_NAME "ov8820"
#define PLATFORM_DRIVER_NAME "msm_camera_ov8820"
#define ov8820_obj ov8820_##obj

//wsyang_temp
#define F_OV8820_POWER
#define F_PANTECH_EEPROM
//#define F_1080P

#ifdef F_PANTECH_EEPROM
#define ov8820_EEPROM_SLAVE_ADDR 0xA0>>1
#endif

#ifdef F_OV8820_POWER
#define CAM1_IOVDD_EN	0
#define CAM1_AVDD_EN	1
//#define CAM1_DVDD_EN    2 //for only s5k3h2
#define CAM1_STANDBY    2
#define CAM1_RST_N      3
#define CAMIO_MAX       4

static sgpio_ctrl_t sgpios[CAMIO_MAX] = {
	{CAM1_IOVDD_EN, "CAM1_IOVDD_EN", 77},
	{CAM1_AVDD_EN, "CAM1_AVDD_EN", 82},
//	{CAM1_DVDD_EN, "CAM1_DVDD_EN", 58},//for only s5k3h2
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

DEFINE_MUTEX(ov8820_mut);
static struct msm_sensor_ctrl_t ov8820_s_ctrl;

static struct msm_camera_i2c_reg_conf ov8820_start_settings[] = {
//	{0x301A, 0x70},//mipi_init_high
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf ov8820_stop_settings[] = {
	{0x0100, 0x00},
//	{0x301A, 0x71},//mipi_init_high
};

#if 1
static struct msm_camera_i2c_reg_conf ov8820_groupon_settings[] = {
	{0x3208, 0x00},
};

static struct msm_camera_i2c_reg_conf ov8820_groupoff_settings[] = {
	{0x3208, 0x10},
	{0x3208, 0xA0},
};
#endif
#if 1//#ifdef F_PANTECH_CAMERA_1080P_PREVIEW	
static struct msm_camera_i2c_reg_conf ov8820_1080p_settings[] = {
	{0x3004, 0xd4},
	{0x3005, 0x00},
	{0x3006, 0x00},
	{0x3011, 0x01},
	{0x370a, 0x12},
	{0x3801, 0x00},
	{0x3802, 0x01},
	{0x3803, 0x30},
	{0x3805, 0xdf},
	{0x3806, 0x08},
	{0x3807, 0x67},
	{0x3808, 0x07},
	{0x3809, 0x80},
	{0x380a, 0x04},
	{0x380b, 0x40},
	{0x380c, 0x0d},
	{0x380d, 0xf0},
	{0x380e, 0x07},
	{0x380f, 0x4c},
	{0x3811, 0x10},
	{0x3813, 0x06},
	{0x3814, 0x11},
	{0x3815, 0x11},
	{0x3820, 0x80},
	{0x3821, 0x16},
	{0x3f00, 0x02},
	{0x4005, 0x1a},
	{0x4600, 0x04},
	{0x4601, 0x01},
	{0x4602, 0x00},
	{0x4837, 0x28},//16},
	{0x4843, 0x02},
	{0x5068, 0x53},
	{0x506a, 0x53},
	{0x5780, 0xfc},
	{0x6703, 0xd7},
};
#endif

static struct msm_camera_i2c_reg_conf ov8820_prev_settings[] = {
#ifdef F_1080P
	{0x3004, 0xd4},
	{0x3005, 0x00},
	{0x3006, 0x00},
	{0x3011, 0x01},
	{0x370a, 0x12},
	{0x3801, 0x00},
	{0x3802, 0x01},
	{0x3803, 0x30},
	{0x3805, 0xdf},
	{0x3806, 0x08},
	{0x3807, 0x67},
	{0x3808, 0x07},
	{0x3809, 0x80},
	{0x380a, 0x04},
	{0x380b, 0x40},
	{0x380c, 0x0d},
	{0x380d, 0xf0},
	{0x380e, 0x07},
	{0x380f, 0x4c},
	{0x3811, 0x10},
	{0x3813, 0x06},
	{0x3814, 0x11},
	{0x3815, 0x11},
	{0x3820, 0x80},
	{0x3821, 0x16},
	{0x3f00, 0x02},
	{0x4005, 0x1a},
	{0x4600, 0x04},
	{0x4601, 0x01},
	{0x4602, 0x00},
	{0x4837, 0x28},//16},
	{0x4843, 0x02},
	{0x5068, 0x53},
	{0x506a, 0x53},
	{0x5780, 0xfc},
	{0x6703, 0xd7},
#else
	{0x3004, 0xd4},
	{0x3005, 0x00},
	{0x3006, 0x10},
	{0x3011, 0x01},
	{0x370a, 0x12},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3805, 0xdf},
	{0x3806, 0x09},
	{0x3807, 0x9b},
	{0x3808, 0x06},
	{0x3809, 0x60},
	{0x380a, 0x04},
	{0x380b, 0xc8},
	{0x380c, 0x0d},
	{0x380d, 0xbc},
	{0x380e, 0x04},
	{0x380f, 0xf0},
	{0x3811, 0x08},
	{0x3813, 0x04},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3820, 0x01},
	{0x3821, 0x17},
	{0x3f00, 0x00},
	{0x4005, 0x18},//
	{0x4600, 0x04},
	{0x4601, 0x00},
	{0x4602, 0x30},
	{0x4837, 0x16},
	{0x5068, 0x00},
	{0x506a, 0x00},
#endif
};

static struct msm_camera_i2c_reg_conf ov8820_snap_settings[] = {
	{0x3004, 0xd4},//0xd8},
	{0x3005, 0x00},
	{0x3006, 0x10},
	{0x3011, 0x01},
	{0x370a, 0x12},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3805, 0xdf},
	{0x3806, 0x09},
	{0x3807, 0x9b},
	{0x3808, 0x0c},
	{0x3809, 0xd0},
	{0x380a, 0x09},
	{0x380b, 0x98},
	{0x380c, 0x0e},
	{0x380d, 0x00},
	{0x380e, 0x09},
	{0x380f, 0xb0},
	{0x3811, 0x08},
	{0x3813, 0x02},
	{0x3814, 0x11},
	{0x3815, 0x11},
	{0x3820, 0x00},
	{0x3821, 0x16},
	{0x3f00, 0x02},
	{0x4005, 0x1a},//
	{0x4600, 0x04},
	{0x4601, 0x00},
	{0x4602, 0x78},
	{0x4837, 0x16},//0x18},
	{0x5068, 0x00},
	{0x506a, 0x00},
};

static struct msm_camera_i2c_reg_conf ov8820_recommend_settings[] = {
	{0x0103, 0x01},
//	{0x301A, 0x71},//mipi_init_high
	{0x3000, 0x02},
	{0x3001, 0x00},
	{0x3002, 0x6c},
	{0x3003, 0xce},
	{0x3004, 0xd4},
	{0x3005, 0x00},
	{0x3006, 0x10},
	{0x3007, 0x3b},
	{0x300d, 0x00},
	{0x301f, 0x09},
	{0x3010, 0x00},
	{0x3011, 0x01},
	{0x3012, 0x80},
	{0x3013, 0x39},
	{0x3018, 0x00},
	{0x3104, 0x20},
	{0x3300, 0x00},
	{0x3500, 0x00},
	{0x3501, 0x4e},
	{0x3502, 0xa0},
	{0x3503, 0x07},
	{0x3509, 0x00},
	{0x350b, 0x1f},
	{0x3600, 0x05},
	{0x3601, 0x32},
	{0x3602, 0x44},
	{0x3603, 0x5c},
	{0x3604, 0x98},
	{0x3605, 0xe9},
	{0x3609, 0xb8},
	{0x360a, 0xbc},
	{0x360b, 0xb4},
	{0x360c, 0x0d},
	{0x3613, 0x02},
	{0x3614, 0x0f},
	{0x3615, 0x00},
	{0x3616, 0x03},
	{0x3617, 0x01},
	{0x3618, 0x00},
	{0x3619, 0x00},
	{0x361a, 0x00},
	{0x361b, 0x00},
	{0x3700, 0x20},
	{0x3701, 0x44},
	{0x3702, 0x70},
	{0x3703, 0x4f},
	{0x3704, 0x69},
	{0x3706, 0x7b},
	{0x3707, 0x63},
	{0x3708, 0x85},
	{0x3709, 0x40},
	{0x370a, 0x12},
	{0x370b, 0x01},
	{0x370c, 0x50},
	{0x370d, 0x0c},
	{0x370e, 0x00},
	{0x3711, 0x01},
	{0x3712, 0xcc},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3804, 0x0c},
	{0x3805, 0xdf},
	{0x3806, 0x09},
	{0x3807, 0x9b},
	{0x3808, 0x06},
	{0x3809, 0x60},
	{0x380a, 0x04},
	{0x380b, 0xc8},
	{0x380c, 0x0d},
	{0x380d, 0xbc},
	{0x380e, 0x04},
	{0x380f, 0xf0},
	{0x3810, 0x00},
	{0x3811, 0x08},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3816, 0x02},
	{0x3817, 0x40},
	{0x3818, 0x00},
	{0x3819, 0x40},
	{0x3820, 0x01},
	{0x3821, 0x17},
	{0x3d00, 0x00},
	{0x3d01, 0x00},
	{0x3d02, 0x00},
	{0x3d03, 0x00},
	{0x3d04, 0x00},
	{0x3d05, 0x00},
	{0x3d06, 0x00},
	{0x3d07, 0x00},
	{0x3d08, 0x00},
	{0x3d09, 0x00},
	{0x3d0a, 0x00},
	{0x3d0b, 0x00},
	{0x3d0c, 0x00},
	{0x3d0d, 0x00},
	{0x3d0e, 0x00},
	{0x3d0f, 0x00},
	{0x3d10, 0x00},
	{0x3d11, 0x00},
	{0x3d12, 0x00},
	{0x3d13, 0x00},
	{0x3d14, 0x00},
	{0x3d15, 0x00},
	{0x3d16, 0x00},
	{0x3d17, 0x00},
	{0x3d18, 0x00},
	{0x3d19, 0x00},
	{0x3d1a, 0x00},
	{0x3d1b, 0x00},
	{0x3d1c, 0x00},
	{0x3d1d, 0x00},
	{0x3d1e, 0x00},
	{0x3d1f, 0x00},
	{0x3d80, 0x00},
	{0x3d81, 0x00},
	{0x3d84, 0x00},
	{0x3f00, 0x00},
	{0x3f01, 0xfc},
	{0x3f05, 0x10},
	{0x3f06, 0x00},
	{0x3f07, 0x00},
	{0x4000, 0x29},
	{0x4001, 0x02},
	{0x4002, 0x45},
	{0x4003, 0x08},
	{0x4004, 0x04},
	{0x4005, 0x18},
	{0x404f, 0x8f}, //
	{0x4300, 0xff},
	{0x4303, 0x00},
	{0x4304, 0x08},
	{0x4307, 0x00},
	{0x4600, 0x04},
	{0x4601, 0x00},
	{0x4602, 0x30},
	{0x4800, 0x04},
	{0x4801, 0x0f},
	{0x5000, 0x06}, // 01.16.bsy DPC on 00 ->06
	{0x5001, 0x00},
	{0x5002, 0x00},
	{0x5068, 0x00},
	{0x506a, 0x00},
	{0x501f, 0x00},
	{0x5c00, 0x80},
	{0x5c01, 0x00},
	{0x5c02, 0x00},
	{0x5c03, 0x00},
	{0x5c04, 0x00},
	{0x5c05, 0x00},
	{0x5c06, 0x00},
	{0x5c07, 0x80},
	{0x5c08, 0x10},
	{0x6700, 0x05},
	{0x6701, 0x19},
	{0x6702, 0xfd},
	{0x6703, 0xd1},
	{0x6704, 0xff},
	{0x6705, 0xff},
	{0x6800, 0x10},
	{0x6801, 0x02},
	{0x6802, 0x90},
	{0x6803, 0x10},
	{0x6804, 0x59},
	{0x6900, 0x61},
	{0x6901, 0x04},
	{0x3612, 0x60},
	{0x3617, 0xa1},
	{0x3b1f, 0x00},
	{0x3000, 0x12},
	{0x3000, 0x16},
	{0x3b1f, 0x00},
	{0x0100, 0x01},
	{0x5800, 0x16},
	{0x5801, 0x0b},
	{0x5802, 0x09},
	{0x5803, 0x09},
	{0x5804, 0x0b},
	{0x5805, 0x15},
	{0x5806, 0x07},
	{0x5807, 0x05},
	{0x5808, 0x03},
	{0x5809, 0x03},
	{0x580a, 0x05},
	{0x580b, 0x06},
	{0x580c, 0x05},
	{0x580d, 0x02},
	{0x580e, 0x00},
	{0x580f, 0x00},
	{0x5810, 0x02},
	{0x5811, 0x05},
	{0x5812, 0x06},
	{0x5813, 0x02},
	{0x5814, 0x00},
	{0x5815, 0x00},
	{0x5816, 0x02},
	{0x5817, 0x05},
	{0x5818, 0x07},
	{0x5819, 0x05},
	{0x581a, 0x04},
	{0x581b, 0x03},
	{0x581c, 0x05},
	{0x581d, 0x06},
	{0x581e, 0x13},
	{0x581f, 0x0b},
	{0x5820, 0x09},
	{0x5821, 0x09},
	{0x5822, 0x0b},
	{0x5823, 0x16},
	{0x5824, 0x63},
	{0x5825, 0x23},
	{0x5826, 0x25},
	{0x5827, 0x23},
	{0x5828, 0x45},
	{0x5829, 0x23},
	{0x582a, 0x21},
	{0x582b, 0x41},
	{0x582c, 0x41},
	{0x582d, 0x05},
	{0x582e, 0x23},
	{0x582f, 0x41},
	{0x5830, 0x41},
	{0x5831, 0x41},
	{0x5832, 0x03},
	{0x5833, 0x25},
	{0x5834, 0x23},
	{0x5835, 0x21},
	{0x5836, 0x23},
	{0x5837, 0x05},
	{0x5838, 0x25},
	{0x5839, 0x43},
	{0x583a, 0x25},
	{0x583b, 0x23},
	{0x583c, 0x65},
	{0x583d, 0xcf},
	{0x5842, 0x00},
	{0x5843, 0xef},
	{0x5844, 0x01},
	{0x5845, 0x3f},
	{0x5846, 0x01},
	{0x5847, 0x3f},
	{0x5848, 0x00},
	{0x5849, 0xd5},
	{0x4818, 0x01},
	{0x4819, 0xC0},
	{0x4827, 0x80},
};

static struct v4l2_subdev_info ov8820_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array ov8820_init_conf[] = {
	{&ov8820_recommend_settings[0],
	ARRAY_SIZE(ov8820_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array ov8820_confs[] = {
	{&ov8820_snap_settings[0],
	ARRAY_SIZE(ov8820_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov8820_prev_settings[0],
	ARRAY_SIZE(ov8820_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
#if 1//#ifdef F_PANTECH_CAMERA_1080P_PREVIEW		
	{&ov8820_1080p_settings[0],
	ARRAY_SIZE(ov8820_1080p_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
#endif
};

#define OV8820_FULL_SIZE_WIDTH    3280 //1936;//3280; //1632;//3280;
#define OV8820_FULL_SIZE_HEIGHT   2456//1096;//2456;//1224;//2464; //
#if 1//#ifdef F_PANTECH_CAMERA_1080P_PREVIEW		
#define  OV8820_1080P_SIZE_WIDTH        1920//1936;//1632;//1936;//3280;//1920
#define OV8820_1080P_SIZE_HEIGHT        1088//1456;//1224;//1096;//2464;//1080//1096
#endif
#ifdef F_1080P
#define  OV8820_QTR_SIZE_WIDTH        1920//1936;//1632;//1936;//3280;//1920
#define OV8820_QTR_SIZE_HEIGHT        1088//1456;//1224;//1096;//2464;//1080//1096
#else
//changed by maliang for ov8820 preview 1632x1224 begin
#define  OV8820_QTR_SIZE_WIDTH        1632//1936;//1632;//1936;//3280;//1920
#define OV8820_QTR_SIZE_HEIGHT        1224//1456;//1224;//1096;//2464;//1080//1096
#endif

 
#define OV8820_HRZ_FULL_BLK_PIXELS   304
#define OV8820_VER_FULL_BLK_LINES     24//16//
#if 1//#ifdef F_PANTECH_CAMERA_1080P_PREVIEW		
#define OV8820_HRZ_1080P_BLK_PIXELS    1648//0x0df0//
#define OV8820_VER_1080P_BLK_LINES      780//  0x074C//
#endif
#ifdef F_1080P
#define OV8820_HRZ_QTR_BLK_PIXELS    1648//0x0df0//
#define OV8820_VER_QTR_BLK_LINES      780//  0x074C//
#else
#define OV8820_HRZ_QTR_BLK_PIXELS    1884
#define OV8820_VER_QTR_BLK_LINES      40//32//
#endif

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

int32_t ov8820_sensor_flash(int mode)
{
	int rc;
	uint16_t timeout = 0;
//	int cnt=0;
	
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
			msm_camera_i2c_write(&tps61050_sensor_i2c_client, 0x03, 0xD1, MSM_CAMERA_I2C_BYTE_DATA); // 557.6ms
//			msm_camera_i2c_write(&tps61050_sensor_i2c_client, 0x03, 0xDF, MSM_CAMERA_I2C_BYTE_DATA); // 1016.8ms
            msm_camera_i2c_write(&tps61050_sensor_i2c_client, 0x01, 0x8C, MSM_CAMERA_I2C_BYTE_DATA);//flash 500mA
			break;
		case 1://TORCH_MODE:
			msm_camera_i2c_write(&tps61050_sensor_i2c_client, 0x00, 0x92, MSM_CAMERA_I2C_BYTE_DATA);
			break;
		case 2://FLASH_MODE:
			msm_camera_i2c_read(&tps61050_sensor_i2c_client, 0x01, &timeout, MSM_CAMERA_I2C_BYTE_DATA);
			printk("%s: FLASH_MODE read = %d\n", __func__,timeout);
			if(timeout & 0x20) {
//				msm_camera_i2c_write(&tps61050_sensor_i2c_client, 0x01, 0x8C+cnt, MSM_CAMERA_I2C_BYTE_DATA);
//				cnt++;
//				if(cnt>3) cnt=0;
                msm_camera_i2c_write(&tps61050_sensor_i2c_client, 0x01, 0x8C, MSM_CAMERA_I2C_BYTE_DATA);//flash 500mA
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
 
static struct msm_sensor_output_info_t ov8820_dimensions[] = {
	{
		.x_output = OV8820_FULL_SIZE_WIDTH,//0x1070,
		.y_output = OV8820_FULL_SIZE_HEIGHT,
		.line_length_pclk = OV8820_FULL_SIZE_WIDTH + OV8820_HRZ_FULL_BLK_PIXELS ,
		.frame_length_lines = OV8820_FULL_SIZE_HEIGHT+ OV8820_VER_FULL_BLK_LINES ,
		.vt_pixel_clk = 133324800,//133333333,//66666666,//
		.op_pixel_clk = 133324800,//133333333,
		.binning_factor = 1,
	},
	{
		.x_output = OV8820_QTR_SIZE_WIDTH,
		.y_output = OV8820_QTR_SIZE_HEIGHT,
		.line_length_pclk = OV8820_QTR_SIZE_WIDTH + OV8820_HRZ_QTR_BLK_PIXELS,
		.frame_length_lines = OV8820_QTR_SIZE_HEIGHT+ OV8820_VER_QTR_BLK_LINES,
#ifdef F_1080P
		.vt_pixel_clk = 199950720,//177777777,//155555555,//133333333,//66666666,//
		.op_pixel_clk = 199950720,//177777777,//155555555,//133333333,
#else
		.vt_pixel_clk = 133326720,//133333333,//66666666,//
		.op_pixel_clk = 133326720,//133333333,
#endif
		.binning_factor = 2,// 1,
	},
#if 1//#ifdef F_PANTECH_CAMERA_1080P_PREVIEW		
	{
		.x_output = OV8820_1080P_SIZE_WIDTH,
		.y_output = OV8820_1080P_SIZE_HEIGHT,
		.line_length_pclk = OV8820_1080P_SIZE_WIDTH + OV8820_HRZ_1080P_BLK_PIXELS,
		.frame_length_lines = OV8820_1080P_SIZE_HEIGHT+ OV8820_VER_1080P_BLK_LINES,
		.vt_pixel_clk = 199950720,//177777777,//155555555,//133333333,//66666666,//
		.op_pixel_clk = 199950720,//177777777,//155555555,//133333333,
		.binning_factor = 2,// 1,
	},
#endif
};

static struct msm_camera_csid_vc_cfg ov8820_cid_cfg[] = {
	{0, CSI_RAW10, CSI_DECODE_10BIT},
	{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
#if 1//#ifdef F_PANTECH_CAMERA_1080P_PREVIEW	
	{2, CSI_RESERVED_DATA, CSI_DECODE_8BIT},
#endif
};

static struct msm_camera_csi2_params ov8820_csi_params = {
	.csid_params = {
		.lane_assign = 0xe4,
		.lane_cnt = 2,
		.lut_params = {
			.num_cid = 2,
			.vc_cfg = ov8820_cid_cfg,
		},
	},
	.csiphy_params = {
		.lane_cnt = 2,
		.settle_cnt = 0x1B,//0x3E,//0x1F,//
	},
};

static struct msm_camera_csi2_params *ov8820_csi_params_array[] = {
	&ov8820_csi_params,
	&ov8820_csi_params,
#if 1//#ifdef F_PANTECH_CAMERA_1080P_PREVIEW	
	&ov8820_csi_params,
#endif
};

static struct msm_sensor_output_reg_addr_t ov8820_reg_addr = {
	.x_output = 0x3808,
	.y_output = 0x380A,
	.line_length_pclk = 0x380C,
	.frame_length_lines = 0x380E,
};

#define OV8820_PIDH_REG         0x300A
#define OV8820_PID                    0x88

static struct msm_sensor_id_info_t ov8820_id_info = {
	.sensor_id_reg_addr = OV8820_PIDH_REG,
	.sensor_id = OV8820_PID,
};

#define OV8820_GAIN         0x350B//0x3000
static struct msm_sensor_exp_gain_info_t ov8820_exp_gain_info = {
	.coarse_int_time_addr = 0x3501,
	.global_gain_addr = OV8820_GAIN,
	.vert_offset = 8, // 3-->8 03_24_bsy
};


#if 1//def F_PANTECH_CAMERA_OV8820
static int32_t msm_sensor_write_exp_gain_ov8820(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line, uint16_t pre_snap)
{
	uint32_t fl_lines;//, ll_pclk, ll_ratio;
	uint8_t offset;
	uint16_t aec_low; 
	uint16_t aec_middle;
	uint16_t aec_high;

	CDBG("%s: %d\n", __func__, __LINE__);
/* comment out  03_24_bsy
	if(pre_snap == 0) {
		if(line>1256) line = 1256;
	}
	else
		if(line>2472) line = 2472;
*/

	fl_lines = (s_ctrl->curr_frame_length_lines * s_ctrl->fps_divider) / Q10;
	
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	if (line > (fl_lines - offset)) {
		fl_lines = line  + offset;
	}
	CDBG("Before %s: fl_lines = %d, offset = %d\n", __func__, fl_lines, offset);
	// 03_24_bsy fl odd 
	if ( (fl_lines%2) == 1)
		fl_lines++;
	
	CDBG("%s: gain = %d, line = %d\n", __func__, gain, line);
	CDBG("%s: fl_lines = %d, offset = %d\n", __func__, fl_lines, offset);
	aec_low = (uint16_t)(line & 0x000F) << 4;        
	aec_middle = (uint16_t)(line & 0x0FF0) >> 4; 
	aec_high = (uint16_t)(line & 0xF000) >> 12;   

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines, (fl_lines&0xFF00)>>8,//0x0e,
		MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_output_reg_addr->frame_length_lines + 0x0001, (fl_lines&0x00FF),//x00,
		MSM_CAMERA_I2C_BYTE_DATA);

//expo
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr - 0x0001, aec_high,
		MSM_CAMERA_I2C_BYTE_DATA);	
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr, aec_middle,
		MSM_CAMERA_I2C_BYTE_DATA);
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 0x0001, aec_low,
		MSM_CAMERA_I2C_BYTE_DATA);


//gain
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr - 0x0001, (gain&0xFF00)>>8,//x10, // 2x
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		s_ctrl->sensor_exp_gain_info->global_gain_addr, (gain&0x00FF),//0x10, // 2x
		MSM_CAMERA_I2C_BYTE_DATA);

	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);

#if 1//def CONFIG_PANTECH_CAMERA_FLASH //wsyang_temp for flash
//	s_ctrl->func_tbl->sensor_flash_ctl(2);
#endif

	return 0;

}
#endif

static int32_t msm_sensor_write_exp_gain_preview_ov8820(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{
 	msm_sensor_write_exp_gain_ov8820(s_ctrl, gain, line, 0);
	return 0;
}

static int32_t msm_sensor_write_exp_gain_snapshot_ov8820(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{
	msm_sensor_write_exp_gain_ov8820(s_ctrl, gain, line, 1);
	return 0;
}


#ifdef F_PANTECH_EEPROM
#define MESH_ROLLOFF_SIZE    (17 * 13)
int32_t ov8820_i2c_rxdata(struct msm_camera_i2c_client *dev_client, uint16_t saddr, 
	unsigned char *rxdata, int data_length)
{
	int32_t rc = 0;
		
	struct i2c_msg msgs[] = {
		{
			.addr  = saddr,
			.flags = 0,
			.len   = dev_client->addr_type,
			.buf   = rxdata,
		},
		{
			.addr  = saddr,
			.flags = I2C_M_RD,
			.len   = data_length,
			.buf   = rxdata,
		},
	};
	rc = i2c_transfer(dev_client->client->adapter, msgs, 2);
	if (rc < 0)
		CDBG("ov8820_i2c_rxdata failed 0x%x\n", saddr);
	return rc;
}

static int32_t ov8820_i2c_read_w_eeprom_with_size(struct msm_sensor_ctrl_t *s_ctrl, uint16_t raddr,
	uint8_t *rdata, unsigned int length)	
{
	//read data from "raddr" to "raddr+length-1"
	int32_t rc = 0;
	int i;
	unsigned char buf[2+length];
	 
	if (!rdata)
	  return -1;

	/*Read 1 byte in sequence */
	buf[0] = (raddr & 0xFF00) >> 8;
	buf[1] = (raddr & 0x00FF);
	
	rc = ov8820_i2c_rxdata(s_ctrl->sensor_i2c_client, ov8820_EEPROM_SLAVE_ADDR , buf, length);
	if (rc < 0) {
		CDBG("ov8820_i2c_read_eeprom 0x%x failed!\n", raddr);
		return rc;
	}
	for(i=0; i<length; i++)
	{
		rdata[i] = buf[i];
		//printk("Byte %d: 0x%x\n", i, buf[i]);
	}
	return rc;
}

static int ov8820_read_eeprom_data(struct msm_sensor_ctrl_t *s_ctrl, struct sensor_cfg_data *cfg)
{
	int32_t rc = 0;
	uint8_t eepromdata[2];
	uint16_t addr = 0;

	CDBG("ov8820_read_eeprom_data==============================>start\n");	
	//read wb cal-data	
	addr = 0x0000;
	rc = ov8820_i2c_read_w_eeprom_with_size(s_ctrl, addr, eepromdata, 2);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}
	cfg->cfg.calib_info.r_over_g_5100K= (eepromdata[0]<<8) |eepromdata[1]; //03_19_bsy 4100K cal add
	CDBG("[QCTK_EEPROM] r_over_g = 0x%4x\n", cfg->cfg.calib_info.r_over_g_5100K);	//3//_19_bsy_add
	addr += 0x02;
	
	rc = ov8820_i2c_read_w_eeprom_with_size(s_ctrl, addr, eepromdata, 2);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}
	cfg->cfg.calib_info.b_over_g_5100K = (eepromdata[0]<<8) |eepromdata[1]; //03_19_bsy 4100K cal add
	CDBG("[QCTK_EEPROM] b_over_g = 0x%4x\n", cfg->cfg.calib_info.b_over_g_5100K);	//3//_19_bsy_add
	addr += 0x02;
	
	rc = ov8820_i2c_read_w_eeprom_with_size(s_ctrl, addr, eepromdata, 2);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}
	cfg->cfg.calib_info.gr_over_gb_5100K = (eepromdata[0]<<8) |eepromdata[1]; //03_19_bsy 4100K cal add
	CDBG("[QCTK_EEPROM] gr_over_gb = 0x%4x\n", cfg->cfg.calib_info.gr_over_gb_5100K);	//3//_19_bsy_add
	addr += 0x02;
	
	//read TL84 cal-data	
	rc = ov8820_i2c_read_w_eeprom_with_size(s_ctrl, addr, cfg->cfg.calib_info.rolloff_TL84.r_gain, MESH_ROLLOFF_SIZE);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}	
	addr += MESH_ROLLOFF_SIZE;
	
	rc = ov8820_i2c_read_w_eeprom_with_size(s_ctrl, addr, cfg->cfg.calib_info.rolloff_TL84.gr_gain, MESH_ROLLOFF_SIZE);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}	
	addr += MESH_ROLLOFF_SIZE;
	
	rc = ov8820_i2c_read_w_eeprom_with_size(s_ctrl, addr, cfg->cfg.calib_info.rolloff_TL84.gb_gain, MESH_ROLLOFF_SIZE);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}	
	addr += MESH_ROLLOFF_SIZE;
	
	rc = ov8820_i2c_read_w_eeprom_with_size(s_ctrl, addr, cfg->cfg.calib_info.rolloff_TL84.b_gain, MESH_ROLLOFF_SIZE);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}
	addr += MESH_ROLLOFF_SIZE;
	
	//read D50 cal-data	
	rc = ov8820_i2c_read_w_eeprom_with_size(s_ctrl, addr, cfg->cfg.calib_info.rolloff_D50.r_gain, MESH_ROLLOFF_SIZE);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}
	addr += MESH_ROLLOFF_SIZE;
	
	rc = ov8820_i2c_read_w_eeprom_with_size(s_ctrl, addr, cfg->cfg.calib_info.rolloff_D50.gr_gain, MESH_ROLLOFF_SIZE);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}	
	addr += MESH_ROLLOFF_SIZE;
	
	rc = ov8820_i2c_read_w_eeprom_with_size(s_ctrl, addr, cfg->cfg.calib_info.rolloff_D50.gb_gain, MESH_ROLLOFF_SIZE);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}	
	addr += MESH_ROLLOFF_SIZE;
	
	rc = ov8820_i2c_read_w_eeprom_with_size(s_ctrl, addr, cfg->cfg.calib_info.rolloff_D50.b_gain, MESH_ROLLOFF_SIZE);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}

   //03_19_bsy 4100K cal add
	//read wb cal-data for 4100K
	addr = 0x06F1;
	rc = ov8820_i2c_read_w_eeprom_with_size(s_ctrl, addr, eepromdata, 2);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}
	cfg->cfg.calib_info.r_over_g_4100K= (eepromdata[0]<<8) |eepromdata[1];
	CDBG("[QCTK_EEPROM] r_over_g_4100K = 0x%4x\n", cfg->cfg.calib_info.r_over_g_4100K);//3_19_bsy_add	
	addr += 0x02;
	
	rc = ov8820_i2c_read_w_eeprom_with_size(s_ctrl, addr, eepromdata, 2);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}
	cfg->cfg.calib_info.b_over_g_4100K = (eepromdata[0]<<8) |eepromdata[1];
	CDBG("[QCTK_EEPROM] b_over_g_4100K = 0x%4x\n", cfg->cfg.calib_info.b_over_g_4100K);//3_19_bsy_add	
	addr += 0x02;
	
	rc = ov8820_i2c_read_w_eeprom_with_size(s_ctrl, addr, eepromdata, 2);
	if (rc < 0) {
		CDBG("%s: Error Reading EEPROM @ 0x%x\n", __func__, addr);
		return rc;
	}
	cfg->cfg.calib_info.gr_over_gb_4100K = (eepromdata[0]<<8) |eepromdata[1];
	CDBG("[QCTK_EEPROM] gr_over_gb_4100K = 0x%4x\n", cfg->cfg.calib_info.gr_over_gb_4100K); //3_19_bsy_add
	
	CDBG("ov8820_read_eeprom_data==============================>end\n");	
	return rc;
}

// End ## QCTK ##
#endif

#if 0
static struct sensor_calib_data ov8820_calib_data;
#endif

static const struct i2c_device_id ov8820_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&ov8820_s_ctrl},
	{ }
};

static struct i2c_driver ov8820_i2c_driver = {
	.id_table = ov8820_i2c_id,
	.probe  = msm_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};


static struct msm_camera_i2c_client ov8820_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};
#if 0
static struct msm_camera_i2c_client ov8820_eeprom_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};

static struct msm_camera_eeprom_read_t ov8820_eeprom_read_tbl[] = {
	{0x10, &ov8820_calib_data.r_over_g, 2, 1},
	{0x12, &ov8820_calib_data.b_over_g, 2, 1},
	{0x14, &ov8820_calib_data.gr_over_gb, 2, 1},
};

static struct msm_camera_eeprom_data_t ov8820_eeprom_data_tbl[] = {
	{&ov8820_calib_data, sizeof(struct sensor_calib_data)},
};

static struct msm_camera_eeprom_client ov8820_eeprom_client = {
	.i2c_client = &ov8820_eeprom_i2c_client,
	.i2c_addr = 0xA4,

	.func_tbl = {
		.eeprom_set_dev_addr = NULL,
		.eeprom_init = msm_camera_eeprom_init,
		.eeprom_release = msm_camera_eeprom_release,
		.eeprom_get_data = msm_camera_eeprom_get_data,
	},

	.read_tbl = ov8820_eeprom_read_tbl,
	.read_tbl_size = ARRAY_SIZE(ov8820_eeprom_read_tbl),
	.data_tbl = ov8820_eeprom_data_tbl,
	.data_tbl_size = ARRAY_SIZE(ov8820_eeprom_data_tbl),
};
#endif

#ifdef F_OV8820_POWER
static int ov8820_vreg_init(void)
{
    int rc = 0;

    SKYCDBG("%s E\n", __func__);
    
    rc = sgpio_init(sgpios, CAMIO_MAX);
    if (rc < 0)
        goto sensor_init_fail;

    rc = svreg_init(svregs, CAMV_MAX);
    if (rc < 0)
        goto sensor_init_fail;
    
    SKYCDBG("%s X\n", __func__);
    return 0;
    
sensor_init_fail:
    svreg_release(svregs, CAMV_MAX);
    sgpio_release(sgpios, CAMIO_MAX);
    return -ENODEV;
}


int32_t ov8820_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
    int32_t rc = 0;
    SKYCDBG("%s: %d\n", __func__, __LINE__);

#if 0
    msm_sensor_probe_on(&s_ctrl->sensor_i2c_client->client->dev);
    SKYCDBG("%s msm_sensor_probe_on ok\n", __func__); 
    msm_camio_clk_rate_set(MSM_SENSOR_MCLK_24HZ);
    SKYCDBG("%s msm_camio_clk_rate_set ok\n", __func__);
#else
    rc = msm_sensor_power_up(s_ctrl);
    SKYCDBG(" %s : msm_sensor_power_up : rc = %d E\n",__func__, rc);  
#endif

    ov8820_vreg_init();

    if (sgpio_ctrl(sgpios, CAM1_IOVDD_EN, 1) < 0)   rc = -EIO;
    mdelay(1); 
    if (sgpio_ctrl(sgpios, CAM1_RST_N, 0) < 0)     rc = -EIO;
    mdelay(1); 
    if (sgpio_ctrl(sgpios, CAM1_AVDD_EN, 1) < 0)    rc = -EIO;
    mdelay(1); 
//    if (sgpio_ctrl(sgpios, CAM1_DVDD_EN, 1) < 0)     rc = -EIO;    
//    mdelay(1); 
    if (svreg_ctrl(svregs, CAMV_AF_2P8V, 1) < 0)    rc = -EIO;
    mdelay(1); 
    
    //(void)msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
    //msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);
    SKYCDBG(" msm_camio_clk_rate_set E\n");    
    //msm_camio_clk_rate_set(24000000);
    SKYCDBG(" msm_camio_clk_rate_set X\n");
    mdelay(1);

    if (sgpio_ctrl(sgpios, CAM1_STANDBY, 1) < 0)     rc = -EIO;
    mdelay(10); //ov8820 spec > 1ms 
    if (sgpio_ctrl(sgpios, CAM1_RST_N, 1) < 0)     rc = -EIO;
    mdelay(25); //ov8820 spec: >20ms 

    SKYCDBG("%s X (%d)\n", __func__, rc);
    return rc;
}

int32_t ov8820_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
    int32_t rc = 0;
    SKYCDBG("%s\n", __func__);

#ifdef CONFIG_OV8820_ACT// 1//CONFIG_PANTECH_CAMERA_AF_OFF_STEP
    if(AF_off_step_check == 1) {// except AF_power_down for booting
        msm_actuator_af_power_down(AF_off_step_ctrl);
    }
    else {
        AF_off_step_check = 1;
    }
#endif
#if 0
    msm_sensor_probe_off(&s_ctrl->sensor_i2c_client->client->dev);
#else
    msm_sensor_power_down(s_ctrl);
    SKYCDBG(" %s : msm_sensor_power_down : rc = %d E\n",__func__, rc);  
#endif
    
    if (sgpio_ctrl(sgpios, CAM1_RST_N, 0) < 0)   rc = -EIO;
    mdelay(1); 
    if (sgpio_ctrl(sgpios, CAM1_STANDBY, 0) < 0)     rc = -EIO;
    mdelay(1); 
    if (svreg_ctrl(svregs, CAMV_AF_2P8V, 0) < 0)    rc = -EIO;
    mdelay(1); 
//    if (sgpio_ctrl(sgpios, CAM1_DVDD_EN, 0) < 0)     rc = -EIO;    
//    mdelay(1); 
    if (sgpio_ctrl(sgpios, CAM1_AVDD_EN, 0) < 0)    rc = -EIO;
    mdelay(1); 
    if (sgpio_ctrl(sgpios, CAM1_IOVDD_EN, 0) < 0)   rc = -EIO;
    mdelay(1); 

    svreg_release(svregs, CAMV_MAX);
    sgpio_release(sgpios, CAMIO_MAX);
    
    SKYCDBG("%s X (%d)\n", __func__, rc);
    return rc;
}
#endif

static int __init msm_sensor_init_module(void)
{
	return i2c_add_driver(&ov8820_i2c_driver);
}

static struct v4l2_subdev_core_ops ov8820_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops ov8820_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops ov8820_subdev_ops = {
	.core = &ov8820_subdev_core_ops,
	.video  = &ov8820_subdev_video_ops,
};

static struct msm_sensor_fn_t ov8820_func_tbl = {
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
	.sensor_write_exp_gain = msm_sensor_write_exp_gain_preview_ov8820,
	.sensor_write_snapshot_exp_gain = msm_sensor_write_exp_gain_snapshot_ov8820,
	.sensor_setting = msm_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
//	.sensor_adjust_frame_lines = msm_sensor_adjust_frame_lines,//wsyang_temp 1532 add
#ifdef F_OV8820_POWER
	.sensor_power_up = ov8820_sensor_power_up,//msm_sensor_power_up,
	.sensor_power_down = ov8820_sensor_power_down,//msm_sensor_power_down,
#else	
	.sensor_power_up = msm_sensor_power_up,
	.sensor_power_down = msm_sensor_power_down,
#endif
#if 0//def CONFIG_PANTECH_CAMERA_FLASH
	.sensor_flash_ctl = ov8820_sensor_flash,
#endif
#ifdef F_PANTECH_EEPROM
	.sensor_get_eeprom_data = ov8820_read_eeprom_data,
#endif
};

static struct msm_sensor_reg_t ov8820_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = ov8820_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(ov8820_start_settings),
	.stop_stream_conf = ov8820_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(ov8820_stop_settings),
	.group_hold_on_conf = ov8820_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(ov8820_groupon_settings),
	.group_hold_off_conf = ov8820_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(ov8820_groupoff_settings),
	.init_settings = &ov8820_init_conf[0],
	.init_size = ARRAY_SIZE(ov8820_init_conf),
	.mode_settings = &ov8820_confs[0],
	.output_settings = &ov8820_dimensions[0],
	.num_conf = ARRAY_SIZE(ov8820_confs),
};

static struct msm_sensor_ctrl_t ov8820_s_ctrl = {
	.msm_sensor_reg = &ov8820_regs,
	.sensor_i2c_client = &ov8820_sensor_i2c_client,
	.sensor_i2c_addr = 0x6C,//34,
//	.sensor_eeprom_client = &ov8820_eeprom_client,
	.sensor_output_reg_addr = &ov8820_reg_addr,
	.sensor_id_info = &ov8820_id_info,
	.sensor_exp_gain_info = &ov8820_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csi_params = &ov8820_csi_params_array[0],
	.msm_sensor_mutex = &ov8820_mut,
	.sensor_i2c_driver = &ov8820_i2c_driver,
	.sensor_v4l2_subdev_info = ov8820_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(ov8820_subdev_info),
	.sensor_v4l2_subdev_ops = &ov8820_subdev_ops,
	.func_tbl = &ov8820_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

//module_init(msm_sensor_init_module);
late_initcall(msm_sensor_init_module);
MODULE_DESCRIPTION("Omivision 8MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");

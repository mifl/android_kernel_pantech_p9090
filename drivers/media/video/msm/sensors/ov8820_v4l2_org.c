/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include <mach/camera.h>
#include <media/v4l2-subdev.h>
#include "msm.h"
#include "ov8820.h"
//#include "ov8820_hvca.h"

/*=============================================================
	SENSOR REGISTER DEFINES
==============================================================*/
#define Q8    0x00000100

/* Omnivision8810 product ID register address */
#define OV8820_PIDH_REG                       0x300A
#define OV8820_PIDL_REG                       0x300B
/* Omnivision8810 product ID */
#define OV8820_PID                    0x88
/* Omnivision8810 version */
//#define OV8820_VER                    0x10
#define OV8820_VER                    0x20 //changed by maliang for 8820 ID
/* Time in milisecs for waiting for the sensor to reset */
#define OV8820_RESET_DELAY_MSECS    66
#define OV8820_DEFAULT_CLOCK_RATE   12000000
/* Registers*/
#define OV8820_GAIN         0x350B//0x3000
#define OV8820_AEC_MSB      0x3501//0x3002
#define OV8820_AEC_LSB      0x3502//0x3003
#define OV8820_AF_MSB       0x3619//0x30EC
#define OV8820_AF_LSB       0x3618//0x30ED
//changed by maliang for ov8820 begin
#define OV8820_AEC_LOW      0x3502
#define OV8820_AEC_MIDDLE   0x3501
#define OV8820_AEC_HIGH     0x3500
//changed by maliang for ov8820 end

/* AF Total steps parameters */
#define OV8820_STEPS_NEAR_TO_CLOSEST_INF  43
#define OV8820_TOTAL_STEPS_NEAR_TO_FAR    43
/*Test pattern*/
/* Color bar pattern selection */
//#define OV8820_COLOR_BAR_PATTERN_SEL_REG      0x307B //changed by maliang	for ov8820
/* Color bar enabling control */
//#define OV8820_COLOR_BAR_ENABLE_REG           0x307D //changed by maliang for ov8820
/* Time in milisecs for waiting for the sensor to reset*/
#define OV8820_RESET_DELAY_MSECS    66
/* I2C Address of the Sensor */
#define OV8820_I2C_SLAVE_ID    0x6c

#define OV8820_OFFSET  6
//#define PRE_RAW //defined by maliang for ov8820 dirver guiting
#define P3M_MAL 1//defined by maliang for ov8820 3M preview setting
/*============================================================================
							 TYPE DECLARATIONS
============================================================================*/

typedef unsigned char byte;
/* 16bit address - 8 bit context register structure */
typedef struct reg_addr_val_pair_struct
{
	uint16_t  reg_addr;
	byte      reg_val;
}reg_struct_type;

enum ov8820_test_mode_t {
	TEST_OFF,
	TEST_1,
	TEST_2,
	TEST_3
};

#ifndef OV8820_HD //changed by maliang for 1080P
enum ov8820_resolution_t {
	QTR_SIZE,
	FULL_SIZE,
	INVALID_SIZE
};
#else
enum ov8820_resolution_t {
	QTR_SIZE,
	FULL_SIZE,
	QVGA_SIZE,
	INVALID_SIZE,
	HD_SIZE
};
#endif


/*============================================================================
							DATA DECLARATIONS
============================================================================*/
/*  24M MCLK 480M MipiClk 9.76fps 10bit/2lane */
//static int counter = 0;
static reg_struct_type ov8820_init_settings_array[] =
{
{0x3000,0x02},
{0x3001,0x00},
{0x3002,0x6c},
{0x300d,0x00},
{0x301f,0x09},
{0x3010,0x00},
{0x3018,0x00},
{0x3300,0x00},
{0x3500,0x00},
{0x3503,0x07},
{0x3509,0x10},
{0x3600,0x08},
{0x3601,0x44},
{0x3602,0x75},
{0x3603,0x5c},
{0x3604,0x98},
{0x3605,0xe9},
{0x3609,0xb8},
{0x360a,0xbc},
{0x360b,0xb4},
{0x360c,0x0d},
{0x3612,0x63},//maliang increase driver 
{0x3613,0x02},
{0x3614,0x0f},
{0x3615,0x00},
{0x3616,0x03},
{0x3617,0x01},
{0x3618,0x00},
{0x3619,0x00},
{0x361a,0xb0},//changed by maliang for af 00
{0x361b,0x04},//changed by maliang for af 00
{0x3700,0x20},
{0x3701,0x44},
{0x3702,0x50},
{0x3703,0xcc},
{0x3704,0x19},
{0x3706,0x4b},
{0x3707,0x63},
{0x3708,0x84},
{0x3709,0x40},
{0x370b,0x01},
{0x370c,0x50},
{0x370d,0x0c},
{0x370e,0x00},
{0x3711,0x01},
{0x3712,0x9c},
{0x3800,0x00},
{0x3804,0x0c},
{0x3810,0x00},
{0x3812,0x00},
{0x3816,0x02},
{0x3817,0x40},
{0x3818,0x00},
{0x3819,0x40},
{0x3d00,0x00},
{0x3d01,0x00},
{0x3d02,0x00},
{0x3d03,0x00},
{0x3d04,0x00},
{0x3d05,0x00},
{0x3d06,0x00},
{0x3d07,0x00},
{0x3d08,0x00},
{0x3d09,0x00},
{0x3d0a,0x00},
{0x3d0b,0x00},
{0x3d0c,0x00},
{0x3d0d,0x00},
{0x3d0e,0x00},
{0x3d0f,0x00},
{0x3d10,0x00},
{0x3d11,0x00},
{0x3d12,0x00},
{0x3d13,0x00},
{0x3d14,0x00},
{0x3d15,0x00},
{0x3d16,0x00},
{0x3d17,0x00},
{0x3d18,0x00},
{0x3d19,0x00},
{0x3d1a,0x00},
{0x3d1b,0x00},
{0x3d1c,0x00},
{0x3d1d,0x00},
{0x3d1e,0x00},
{0x3d1f,0x00},
{0x3d80,0x00},
{0x3d81,0x00},
{0x3d84,0x00},
{0x3f01,0xfc},
{0x3f05,0x10},
{0x3f06,0x00},
{0x3f07,0x00},
{0x4000,0x29},
{0x4001,0x02},
{0x4002,0x45},
{0x4003,0x08},
{0x4004,0x04},
{0x4005,0x18},
{0x4300,0xff},
{0x4303,0x00},
{0x4304,0x08},
{0x4307,0x00},
{0x4800,0x04},
{0x4801,0x0f},
{0x4843,0x02},
{0x5000,0x00},
{0x5001,0x00},
{0x5002,0x00},
{0x501f,0x00},
{0x5c00,0x80},
{0x5c01,0x00},
{0x5c02,0x00},
{0x5c03,0x00},
{0x5c04,0x00},
{0x5c05,0x00},
{0x5c06,0x00},
{0x5c07,0x80},
{0x5c08,0x10},
{0x6700,0x05},
{0x6701,0x19},
{0x6702,0xfd},
{0x6703,0xd1},
{0x6704,0xff},
{0x6705,0xff},
{0x6800,0x10},
{0x6801,0x02},
{0x6802,0x90},
{0x6803,0x10},
{0x6804,0x59},
{0x6900,0x61},
{0x6901,0x05},//changed by maliang for ov8820 from 0x04 to 0x05
{0x3612,0x00},
{0x3617,0xa1},
{0x3b1f,0x00},
{0x3000,0x12},
{0x3000,0x16},
{0x3b1f,0x00},
{0x3003,0xce},
{0x3004,0xd8},
{0x3005,0x00},
{0x3006,0x10},
{0x3007,0x3b},
{0x3012,0x80},
{0x3013,0x39},
{0x3104,0x20},
{0x3503,0x07},//maliang changed for 3503 07
{0x3500,0x00},
{0x3501,0x14},
{0x3502,0x80},
{0x350b,0xff},
{0x3400,0x04},
{0x3401,0x00},
{0x3402,0x04},
{0x3403,0x00},
{0x3404,0x04},
{0x3405,0x00},
{0x3406,0x01},
{0x5001,0x01}
};

/*1632x1224; 24M MCLK 480M MipiClk 30fps 10bit/2lane*/
static reg_struct_type ov8820_qtr_settings_array[] =
{
#if P3M_MAL
{0x3004, 0xd4},
{0x3005, 0x00},
{0x3006, 0x00},//changed by maliang for 15fps {0x3006, 0x00},
{0x3011, 0x01},
//{0x3501, 0x9a},
//{0x3502, 0xa0},
{0x370a, 0x12},
{0x3801, 0x00},
{0x3802, 0x00},
{0x3803, 0x00},
{0x3805, 0xdf},
{0x3806, 0x09},
{0x3807, 0x9b},
{0x3808, 0x07},
{0x3809, 0x90},
{0x380a, 0x05},
{0x380b, 0xb0},//ac
{0x380c, 0x0e},
{0x380d, 0x00},
{0x380e, 0x09},
{0x380f, 0xb0},
{0x3811, 0x10},
{0x3813, 0x06},
{0x3814, 0x11},
{0x3815, 0x11},
{0x3820, 0x00},
{0x3821, 0x16},
{0x3f00, 0x02},
{0x4600, 0x04},
{0x4601, 0x00},
{0x4602, 0x78},
{0x4837, 0x28},
{0x5068, 0x53},
{0x506a, 0x54}
#else
{0x3004,0xd4},
{0x3005,0x00},
{0x3006,0x10},
{0x3011,0x01},
//{0x3501,0x4e},//changed by maliang for aec test blink
//{0x3502,0xa0},//changed by maliang for aec test blink
{0x370a,0x12},
{0x3801,0x00},
{0x3802,0x00},
{0x3803,0x00},
{0x3805,0xdf},
{0x3806,0x09},
{0x3807,0x9b},
{0x3808,0x06},
{0x3809,0x60},
{0x380a,0x04},
{0x380b,0xc8},
{0x380c,0x0d},
{0x380d,0xbc},
{0x380e,0x04},
{0x380f,0xf0},
{0x3811,0x08},
{0x3813,0x04},
{0x3814,0x31},
{0x3815,0x31},
{0x3820,0x01},
{0x3821,0x17},
{0x3f00,0x00},
{0x4600,0x04},
{0x4601,0x00},
{0x4602,0x30},
{0x4837,0x16},
{0x5068,0x00},
{0x506a,0x00}
#endif
};
#ifdef OV8820_HD//add by maliang for 1080P
/*1920x1080; 24M MCLK 480M MipiClk 30fps 10bit/2lane*/
static reg_struct_type ov8820_hd_settings_array[] =
{
	{0x3004,0xd4},
	{0x3005,0x00},
	{0x3006,0x00},
	{0x3011,0x01},
	{0x370a,0x12},
	{0x3801,0x00},
	{0x3802,0x01},
	{0x3803,0x30},
	{0x3805,0xdf},
	{0x3806,0x08},
	{0x3807,0x67},
	{0x3808,0x07},
	{0x3809,0x90},//changed by maliang for 1936*1096{0x3809,0x88}
	{0x380a,0x04},
	{0x380b,0x48},//changed by maliang for 1936*1096{0x380b,0x38}
	{0x380c,0x0d},
	{0x380d,0xf0},
	{0x380e,0x07},
	{0x380f,0x4c},
	{0x3811,0x10},
	{0x3813,0x06},
	{0x3814,0x11},
	{0x3815,0x11},
	{0x3820,0x00},
	{0x3821,0x16},
	{0x3f00,0x02},
	{0x4600,0x04},
	{0x4601,0x01},
	{0x4602,0x00},
	{0x4837,0x16},
	{0x5068,0x53},
	{0x506a,0x53}
};
#endif
/* 3280x2456 Sensor Raw; 24M MCLK 480M MipiClk 15fps 10bit/2lane*/
static reg_struct_type ov8820_full_settings_array[] =
{
{0x3004,0xd8},
{0x3005,0x00},
{0x3006,0x10},
{0x3011,0x01},
{0x370a,0x12},
{0x3801,0x00},
{0x3802,0x00},
{0x3803,0x00},
{0x3805,0xdf},
{0x3806,0x09},
{0x3807,0x9b},
{0x3808,0x0c},
{0x3809,0xd0},
{0x380a,0x09},
{0x380b,0x98},
{0x380c,0x0e},
{0x380d,0x00},
{0x380e,0x09},
{0x380f,0xb0},
{0x3811,0x08},
{0x3813,0x02},
{0x3814,0x11},
{0x3815,0x11},
{0x3820,0x00},
{0x3821,0x16},
{0x3f00,0x02},
{0x4600,0x04},
{0x4601,0x00},
{0x4602,0x78},
{0x4837,0x18},
{0x5068,0x00},
{0x506a,0x00}
};

uint32_t OV8820_FULL_SIZE_DUMMY_PIXELS =    0;
uint32_t OV8820_FULL_SIZE_DUMMY_LINES  =    0;
uint32_t OV8820_FULL_SIZE_WIDTH        = 3280; //1936;//3280; //1632;//3280;
uint32_t OV8820_FULL_SIZE_HEIGHT       = 2456;//1096;//2456;//1224;//2464; //

uint32_t OV8820_QTR_SIZE_DUMMY_PIXELS  =    0;
uint32_t OV8820_QTR_SIZE_DUMMY_LINES   =    0;
//changed by maliang for ov8820 preview 1632x1224 begin
#if P3M_MAL
uint32_t  OV8820_QTR_SIZE_WIDTH         = 1936;
uint32_t OV8820_QTR_SIZE_HEIGHT        = 1456;
#else
uint32_t  OV8820_QTR_SIZE_WIDTH         = 1632;//1936;//1632;//1936;//3280;//1920
uint32_t OV8820_QTR_SIZE_HEIGHT        = 1224;//1456;//1224;//1096;//2464;//1080//1096
#endif
//changed by maliang for ov8820 preview 1632x720 end
uint32_t OV8820_HRZ_FULL_BLK_PIXELS   = 304;//696; //890;//
uint32_t OV8820_VER_FULL_BLK_LINES     =  24;//44; //44;
#if P3M_MAL
uint32_t OV8820_HRZ_QTR_BLK_PIXELS    = 1648;
uint32_t OV8820_VER_QTR_BLK_LINES      =  924;
#else
uint32_t OV8820_HRZ_QTR_BLK_PIXELS    = 1884;//1648;//890;//888; //696;
uint32_t OV8820_VER_QTR_BLK_LINES      =  40;//924;//44;
#endif
#ifdef OV8820_HD //add by maliang for ov8820 1080P
uint32_t OV8820_HD_SIZE_DUMMY_PIXELS =    0;

uint32_t OV8820_HD_SIZE_WIDTH        = 1936;//3280; //1632;//3280;
uint32_t OV8820_HD_SIZE_HEIGHT       = 1096;//2456;//1224;//2464; //

uint32_t OV8820_HRZ_HD_BLK_PIXELS   = 1580;//696; //890;//
uint32_t OV8820_VER_HD_BLK_LINES     =  168;//44; //44;
#endif
/* AF Tuning Parameters */
static uint16_t ov8820_step_position_table[OV8820_TOTAL_STEPS_NEAR_TO_FAR+1];
static uint8_t ov8820_damping_threshold = 10;
static uint32_t stored_line_length_ratio = 1 * Q8;
uint16_t ov8820_sw_damping_time_wait = 1;//add by maliang for ov8820 af
uint8_t S3_to_0 = 0x1; 
/* static Variables*/
static uint16_t step_position_table[OV8820_TOTAL_STEPS_NEAR_TO_FAR+1];
static uint16_t csi_config = 0;//changed by maliang for ov8820
/* FIXME: Changes from here */
struct ov8820_work_t {
	struct work_struct work;
};
static struct  ov8820_work_t *ov8820_sensorw;
static struct  i2c_client *ov8820_client;
struct ov8820_ctrl_t {
	const struct  msm_camera_sensor_info *sensordata;
	uint32_t sensormode;
	uint32_t fps_divider; 		/* init to 1 * 0x00000400 */
	uint32_t pict_fps_divider; 	/* init to 1 * 0x00000400 */
	uint16_t fps;
	int16_t  curr_lens_pos;
	uint16_t curr_step_pos;
	uint16_t my_reg_gain;
	uint32_t my_reg_line_count;
	uint16_t total_lines_per_frame;
	enum ov8820_resolution_t prev_res;
	enum ov8820_resolution_t pict_res;
#ifdef OV8820_HD //changed by maliang for ov8820 1080P
	enum ov8820_resolution_t video_res;
#endif
	enum ov8820_resolution_t curr_res;
	enum ov8820_test_mode_t  set_test;
	unsigned short imgaddr;

	struct v4l2_subdev *sensor_dev;
	struct ov8820_format *fmt;
};
static struct ov8820_ctrl_t *ov8820_ctrl;
static DECLARE_WAIT_QUEUE_HEAD(ov8820_wait_queue);
DEFINE_MUTEX(ov8820_mut);

struct ov8820_format {
	enum v4l2_mbus_pixelcode code;
	enum v4l2_colorspace colorspace;
	u16 fmt;
	u16 order;
};

static struct ov8820_format ov8820_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_YUYV8_2X8,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

/*=============================================================*/
static int ov8820_i2c_rxdata(unsigned short saddr,
	unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
	{
		.addr  = saddr,// << 1,
		.flags = 0,
		.len   = 2,
		.buf   = rxdata,
	},
	{
		.addr  = saddr,// << 1,
		.flags = I2C_M_RD,
		.len   = length,
		.buf   = rxdata,
	},
	};

	if (i2c_transfer(ov8820_client->adapter, msgs, 2) < 0) {
		//printk("ov8820_i2c_rxdata failed!\n");//add by maliang for ov8820 debug
		printk("ov8820_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}
static int32_t ov8820_i2c_txdata(unsigned short saddr,
				unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
		 .addr = saddr,// << 1,
		 .flags = 0,
		 .len = length,
		 .buf = txdata,
		 },
	};

	if (i2c_transfer(ov8820_client->adapter, msg, 1) < 0) {
		//printk("maliang ov8820_i2c_txdata faild 0x%x\n", ov8820_client->addr);//add by maliang for ov8820 debug
		printk("ov8820_i2c_txdata faild 0x%x\n", ov8820_client->addr);
		return -EIO;
	}

	return 0;
}


static int32_t ov8820_i2c_read(unsigned short raddr,
				unsigned short *rdata, int rlen)
{
	int32_t rc = 0;
	unsigned char buf[2];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));

	buf[0] = (raddr & 0xFF00) >> 8;
	buf[1] = (raddr & 0x00FF);
	
	rc = ov8820_i2c_rxdata(ov8820_client->addr, buf, rlen);

	if (rc < 0) {
		printk("ov8820_i2c_read 0x%x failed!\n", raddr);
		return rc;
	}

	*rdata = (rlen == 2 ? buf[0] << 8 | buf[1] : buf[0]);

	return rc;

}
static int32_t ov8820_i2c_write_b(unsigned short saddr,unsigned short waddr, uint8_t bdata)
{
	int32_t rc = -EFAULT;
	unsigned char buf[3];

	memset(buf, 0, sizeof(buf));
	buf[0] = (waddr & 0xFF00) >> 8;
	buf[1] = (waddr & 0x00FF);
	buf[2] = bdata;

	printk("i2c_write_b addr = 0x%x, val = 0x%x\n", waddr, bdata);
	//printk("i2c_write_b addr = 0x%x, val = 0x%x\n", waddr, bdata);
	rc = ov8820_i2c_txdata(saddr, buf, 3);

	if (rc < 0) {
		//printk("maliang i2c_write_b failed, addr = 0x%x, val = 0x%x!\n",
		//	 waddr, bdata);//add by maliang for ov8820 
		printk("i2c_write_b failed, addr = 0x%x, val = 0x%x!\n",
			 waddr, bdata);
	}

	return rc;
}
static int32_t ov8820_af_i2c_write(uint16_t data)
{
	uint8_t code_val_msb, code_val_lsb;//, S3_to_0;
	uint32_t rc = 0;
	//S3_to_0 = 0x9; /* S[3:0] */
	code_val_msb = data >> 4; /* D[9:4] */
	code_val_lsb = ((data & 0x000F) << 4) | S3_to_0;
	//printk("maliang code value = %d ,0x3619 = %x ,0x3618 = %x\n", data, code_val_msb, code_val_lsb);
	printk("code value = %d ,D[9:4] = %d ,D[3:0] = %d", data, code_val_msb, code_val_lsb);
	rc = ov8820_i2c_write_b(0x6C >>2 , OV8820_AF_MSB, code_val_msb);
	if (rc < 0) {
		printk("Unable to write code_val_msb = %d\n", code_val_msb);
		return rc;
	}
	rc = ov8820_i2c_write_b( 0x6C >>2, OV8820_AF_LSB, code_val_lsb);
	if (rc < 0) {
		printk("Unable to write code_val_lsb = %disclaimer\n", code_val_lsb);
		return rc;
	}

	return rc;
} /* ov8820_af_i2c_write */

//add by maliang for ov8820 af begin
#define DIV_CEIL(x, y) (x/y + (x%y) ? 1 : 0)
//add by maliang for ov8820 af end

static int32_t ov8820_move_focus( int direction,
	int32_t num_steps)
{
	int16_t step_direction, dest_lens_position, dest_step_position;
	int16_t target_dist, small_step, next_lens_position;
	if (direction == MOVE_NEAR)
		step_direction = 1;
	else
		step_direction = -1;

	dest_step_position = ov8820_ctrl->curr_step_pos
						+ (step_direction * num_steps);

	if (dest_step_position < 0)
		dest_step_position = 0;
	else if (dest_step_position > OV8820_TOTAL_STEPS_NEAR_TO_FAR)
		dest_step_position = OV8820_TOTAL_STEPS_NEAR_TO_FAR;

	if (dest_step_position == ov8820_ctrl->curr_step_pos)
		return 0;

	dest_lens_position = ov8820_step_position_table[dest_step_position];
	target_dist = step_direction *
		(dest_lens_position - ov8820_ctrl->curr_lens_pos);

	if (step_direction < 0 && (target_dist >=
		ov8820_step_position_table[ov8820_damping_threshold])) {
		small_step = DIV_CEIL(target_dist, 10);
		ov8820_sw_damping_time_wait = 10;
	} else {
		small_step = DIV_CEIL(target_dist, 4);
		ov8820_sw_damping_time_wait = 4;
	}

	for (next_lens_position = ov8820_ctrl->curr_lens_pos
		+ (step_direction * small_step);
		(step_direction * next_lens_position) <=
		(step_direction * dest_lens_position);
		next_lens_position += (step_direction * small_step)) {
		if(ov8820_af_i2c_write(next_lens_position) < 0)
			return -EBUSY;
		ov8820_ctrl->curr_lens_pos = next_lens_position;
		usleep(ov8820_sw_damping_time_wait*50);
	}

	if (ov8820_ctrl->curr_lens_pos != dest_lens_position) {
		if(ov8820_af_i2c_write(dest_lens_position) < 0) {
			return -EBUSY;
		}
		usleep(ov8820_sw_damping_time_wait*50);
	}
	ov8820_ctrl->curr_lens_pos = dest_lens_position;
	ov8820_ctrl->curr_step_pos = dest_step_position;
	return 0;
}
static int32_t ov8820_set_default_focus(uint8_t af_step)
{
	int32_t rc = 0;
	if (ov8820_ctrl->curr_step_pos != 0) {
		rc = ov8820_move_focus(MOVE_FAR,
		ov8820_ctrl->curr_step_pos);
	} else {
		rc = ov8820_af_i2c_write(ov8820_step_position_table[0]);
		//ov8820_i2c_write_w_sensor(REG_VCM_NEW_CODE, 0x00);
	}

	ov8820_ctrl->curr_lens_pos = 0;
	ov8820_ctrl->curr_step_pos = 0;

	return rc;
}
static void ov8820_get_pict_fps(uint16_t fps, uint16_t *pfps)
{
    uint32_t divider;	/*Q10 */
	uint32_t d1;
	uint32_t d2;
	uint16_t snapshot_height, preview_height, preview_width,snapshot_width;
    if (ov8820_ctrl->prev_res == QTR_SIZE) {
		preview_width = OV8820_QTR_SIZE_WIDTH  + OV8820_HRZ_QTR_BLK_PIXELS ;
		preview_height= OV8820_QTR_SIZE_HEIGHT + OV8820_VER_QTR_BLK_LINES ;
	}
	else {
		/* full size resolution used for preview. */
		preview_width = OV8820_FULL_SIZE_WIDTH + OV8820_HRZ_FULL_BLK_PIXELS ;
		preview_height= OV8820_FULL_SIZE_HEIGHT + OV8820_VER_FULL_BLK_LINES ;
	}
	if (ov8820_ctrl->pict_res == QTR_SIZE){
		snapshot_width  = OV8820_QTR_SIZE_WIDTH + OV8820_HRZ_QTR_BLK_PIXELS ;
		snapshot_height = OV8820_QTR_SIZE_HEIGHT + OV8820_VER_QTR_BLK_LINES ;
	}
	else {
		snapshot_width  = OV8820_FULL_SIZE_WIDTH + OV8820_HRZ_FULL_BLK_PIXELS;
		snapshot_height = OV8820_FULL_SIZE_HEIGHT + OV8820_VER_FULL_BLK_LINES;
	}

	d1 =
		(uint32_t)(
		(preview_height *
		0x00000400) /
		snapshot_height);

	d2 =
		(uint32_t)(
		(preview_width *
		0x00000400) /
		 snapshot_width);

	divider = (uint32_t) (d1 * d2) / 0x00000400;
	/* Verify PCLK settings and frame sizes. */
	*pfps = (uint16_t)(fps * divider / 0x00000400);
	printk("maliang: %s fps=%x pfps=%x divider=%x\n",__func__,fps,*pfps,divider);//changed by maliang for ov8820
	/* input fps is preview fps in Q8 format */


}/*endof ov8820_get_pict_fps*/

static uint16_t ov8820_get_prev_lines_pf(void)
{
#ifdef OV8820_HD //add by maliang for 8820 1080P
	if( ov8820_ctrl->sensormode == SENSOR_VIDEO1080P_MODE)
	{
		return (OV8820_HD_SIZE_HEIGHT + OV8820_VER_HD_BLK_LINES);
	}
	else
	{
#endif
		if (ov8820_ctrl->prev_res == QTR_SIZE) {
			return (OV8820_QTR_SIZE_HEIGHT + OV8820_VER_QTR_BLK_LINES);
		} else {
			return (OV8820_FULL_SIZE_HEIGHT + OV8820_VER_FULL_BLK_LINES);
		}
#ifdef OV8820_HD //add by maliang for 8820 1080P
	}
#endif
}

static uint16_t ov8820_get_prev_pixels_pl(void)
{
#ifdef OV8820_HD //add by maliang for 8820 1080P
	if( ov8820_ctrl->sensormode == SENSOR_VIDEO1080P_MODE)
	{
		return (OV8820_HD_SIZE_WIDTH + OV8820_HRZ_HD_BLK_PIXELS);
	}
	else
	{
#endif
		if (ov8820_ctrl->prev_res == QTR_SIZE) {
			return (OV8820_QTR_SIZE_WIDTH + OV8820_HRZ_QTR_BLK_PIXELS);
		}
		else{
			return (OV8820_FULL_SIZE_WIDTH + OV8820_HRZ_FULL_BLK_PIXELS);
		}
#ifdef OV8820_HD //add by maliang for 8820 1080P
	}
#endif
}
static uint16_t ov8820_get_pict_lines_pf(void)
{
	if (ov8820_ctrl->pict_res == QTR_SIZE) {
		return (OV8820_QTR_SIZE_HEIGHT + OV8820_VER_QTR_BLK_LINES);
	} else {
		return (OV8820_FULL_SIZE_HEIGHT + OV8820_VER_FULL_BLK_LINES);
	}
}
static uint16_t ov8820_get_pict_pixels_pl(void)
{
	if (ov8820_ctrl->pict_res == QTR_SIZE) {
		return (OV8820_QTR_SIZE_WIDTH + OV8820_HRZ_QTR_BLK_PIXELS);
	} else {
		return (OV8820_FULL_SIZE_WIDTH + OV8820_HRZ_FULL_BLK_PIXELS);
	}
}

static uint32_t ov8820_get_pict_max_exp_lc(void)
{
	if (ov8820_ctrl->pict_res == QTR_SIZE) {
		return (OV8820_QTR_SIZE_HEIGHT + OV8820_VER_QTR_BLK_LINES)*24;
	} else {
		return (OV8820_FULL_SIZE_HEIGHT + OV8820_VER_FULL_BLK_LINES)*24;
	}
}

static int32_t ov8820_set_fps(struct fps_cfg	*fps)
{
#if 0//changed by maliang for ov8820
	int32_t rc = 0;
	ov8820_ctrl->fps_divider = fps->fps_div;
	ov8820_ctrl->pict_fps_divider = fps->pict_fps_div;
	ov8820_ctrl->fps = fps->f_mult;
	printk("maliang %s fps_div=%x pict_fps_div=%x fps=%x\n" \
		,__func__, fps->fps_div, fps->pict_fps_div, fps->f_mult);//add by maliang for ov8820
	return rc;
#else
	uint16_t total_lines_per_frame;
	int32_t rc = 0;
#ifndef OV8820_HD //changed by maliang for ov8820 1080P
	if (ov8820_ctrl->sensormode == SENSOR_PREVIEW_MODE) {
		total_lines_per_frame = (uint16_t)
		((OV8820_QTR_SIZE_HEIGHT+OV8820_VER_QTR_BLK_LINES)
		* ov8820_ctrl->fps_divider/0x400);
	} else {
		total_lines_per_frame = (uint16_t)
		((OV8820_FULL_SIZE_HEIGHT + OV8820_VER_FULL_BLK_LINES)
		 * ov8820_ctrl->pict_fps_divider/0x400);
	}
#else
	if (ov8820_ctrl->sensormode == SENSOR_PREVIEW_MODE) {
		total_lines_per_frame = (uint16_t)
		((OV8820_QTR_SIZE_HEIGHT+OV8820_VER_QTR_BLK_LINES)
		* ov8820_ctrl->fps_divider/0x400);
	} else if(ov8820_ctrl->sensormode == SENSOR_VIDEO1080P_MODE){
		total_lines_per_frame = (uint16_t)
		(OV8820_HD_SIZE_HEIGHT+OV8820_VER_HD_BLK_LINES);
	}
	else	{
		total_lines_per_frame = (uint16_t)
		((OV8820_FULL_SIZE_HEIGHT + OV8820_VER_FULL_BLK_LINES)
		 * ov8820_ctrl->pict_fps_divider/0x400);
	}
	
#endif
	ov8820_ctrl->fps_divider = fps->fps_div;
	ov8820_ctrl->pict_fps_divider = fps->pict_fps_div;

       rc = ov8820_i2c_write_b(ov8820_client->addr, 0x380E, ((total_lines_per_frame & 0xFF00) >> 8));
//changed by for frame length delete + 6
	if (rc < 0) {
               return rc;
       }
 
       rc = ov8820_i2c_write_b(ov8820_client->addr,0x380F, (total_lines_per_frame & 0x00FF));
       if (rc < 0) {
                return rc;
       }
	printk("maliang %s fps_div=%x pict_fps_div=%x fps=%x\n" \
		,__func__, fps->fps_div, fps->pict_fps_div, fps->f_mult);//add by maliang for ov8820

	return rc;
#endif
}
//changed by maliang for ov8820 exposure begin
static int32_t ov8820_write_exp_gain(uint16_t gain, uint32_t line) 
{ 
        uint16_t aec_low; 
        uint16_t aec_middle; 
        uint16_t aec_high;         
        int32_t  rc = 0; 
        uint32_t total_lines_per_frame; 
        uint32_t total_pixels_per_line; 
        uint32_t line_length_ratio = 1 * 0x400; 
        uint16_t max_legal_gain = 0x00FF;//changed by maliang for 8820 debug 
        uint8_t ov8820_offset = 6; 
        printk("%s,%d: gain: %d: Linecount: %d\n",__func__,__LINE__,gain,line);
	  printk("%s: gain: %d: Linecount: %d\n",__func__,gain,line);
//add by maliang for ov8820 snapshot gain begin
           if (ov8820_ctrl->sensormode == SENSOR_PREVIEW_MODE) {
                            ov8820_ctrl->my_reg_gain = gain;
                            ov8820_ctrl->my_reg_line_count = (uint16_t) line;
            }

            if (gain > max_legal_gain) {
                            printk("Max legal gain Line:%d \n", __LINE__);
                            gain = max_legal_gain;
            }
            /* Verify no overflow */
#ifndef OV8820_HD //changed by maliang for ov8820 1080P
            if (ov8820_ctrl->sensormode == SENSOR_PREVIEW_MODE) {
                            line = (uint32_t) (line * ov8820_ctrl->fps_divider /
                                                            0x00000400);
				printk("maliang %s SENSOR_PREVIEW_MODE\n",__func__);//add by maliang for ov8820 debug
                           total_lines_per_frame = (OV8820_QTR_SIZE_HEIGHT+ OV8820_VER_QTR_BLK_LINES);
                            total_pixels_per_line= OV8820_QTR_SIZE_WIDTH + OV8820_HRZ_QTR_BLK_PIXELS;
            } else {
                            line = (uint32_t) (line * ov8820_ctrl->pict_fps_divider /
                                                               0x00000400);
     				printk("maliang %s SENSOR_SNAPSHOT_MODE\n",__func__);//add by maliang for ov8820 debug
                            total_lines_per_frame = (OV8820_FULL_SIZE_HEIGHT + OV8820_VER_FULL_BLK_LINES);
                            total_pixels_per_line= OV8820_FULL_SIZE_WIDTH + OV8820_HRZ_FULL_BLK_PIXELS;
             }
#else
            if (ov8820_ctrl->sensormode == SENSOR_PREVIEW_MODE) {
                            line = (uint32_t) (line * ov8820_ctrl->fps_divider /
                                                            0x00000400);
				printk("maliang %s SENSOR_PREVIEW_MODE\n",__func__);//add by maliang for ov8820 debug
                           total_lines_per_frame = (OV8820_QTR_SIZE_HEIGHT+ OV8820_VER_QTR_BLK_LINES);
                            total_pixels_per_line= OV8820_QTR_SIZE_WIDTH + OV8820_HRZ_QTR_BLK_PIXELS;
            } else if (ov8820_ctrl->sensormode == SENSOR_VIDEO1080P_MODE){
                            //line = (uint32_t) (line * ov8820_ctrl->fps_divider /0x00000400);
				printk("maliang %s SENSOR_PREVIEW_MODE\n",__func__);//add by maliang for ov8820 debug
                           total_lines_per_frame = (OV8820_HD_SIZE_HEIGHT+ OV8820_VER_HD_BLK_LINES);
                           total_pixels_per_line= OV8820_HD_SIZE_WIDTH + OV8820_HRZ_HD_BLK_PIXELS;

		}
            else{
                            line = (uint32_t) (line * ov8820_ctrl->pict_fps_divider /
                                                               0x00000400);
     				printk("maliang %s SENSOR_SNAPSHOT_MODE\n",__func__);//add by maliang for ov8820 debug
                            total_lines_per_frame = (OV8820_FULL_SIZE_HEIGHT + OV8820_VER_FULL_BLK_LINES);
                            total_pixels_per_line= OV8820_FULL_SIZE_WIDTH + OV8820_HRZ_FULL_BLK_PIXELS;
             }
#endif
//add by maliang for ov8820 snapshot gain end
        printk("%s,%dline length ratio is %d\n",__func__,__LINE__,line_length_ratio); 
        aec_low = (uint16_t)(line & 0x000F) << 4; 
        aec_middle = (uint16_t)(line & 0x0FF0) >> 4; 
        aec_high = (uint16_t)(line & 0xF000) >> 12;       

/*        rc = ov8820_i2c_read(OV8820_AEC_MIDDLE, &sensor_registor ,1); 
        if ( rc < 0) { 
                                printk("the ov8820_i2c_read is failed\n"); 
                                return rc; 
                        } 
        printk("the registor of OV8820_AEC_MIDDLE is  0x%x\n", line );*/
//changed by malaing for ov8820 debug 
//add by maliang for ov8820 exposure begin
        if ((total_lines_per_frame - ov8820_offset) < line) {
                        line_length_ratio = (uint32_t) (line * 0x00000400) /
                            (total_lines_per_frame - ov8820_offset);
        } else
                        line_length_ratio = 0x00000400;

         /*Use i2c group write to avoid possible not sync problem*/
    //rc = ov8820_i2c_write_b(ov8820_client->addr, 0x3208, 0);
    //if (rc < 0) {
     //    return rc;
   //}

       rc = ov8820_i2c_write_b(ov8820_client->addr, 0x380E, (((total_lines_per_frame * line_length_ratio / 0x00000400+ov8820_offset) & 0xFF00) >> 8));
//changed by for frame length delete + 6
	if (rc < 0) {
              return rc;
       }

       rc = ov8820_i2c_write_b(ov8820_client->addr,0x380F, ((total_lines_per_frame * line_length_ratio / 0x00000400+ov8820_offset) & 0x00FF));
       if (rc < 0) {
               return rc;
        }
        rc = ov8820_i2c_write_b(ov8820_client->addr, OV8820_AEC_LOW, (uint8_t)aec_low); 
        if (rc < 0) { 
                return rc; 
        } 
        rc = ov8820_i2c_write_b(ov8820_client->addr, OV8820_AEC_MIDDLE, (uint8_t)aec_middle); 
        if(rc < 0) 
                return rc; 
                
        rc = ov8820_i2c_write_b(ov8820_client->addr, OV8820_AEC_HIGH, (uint8_t)aec_high); 
        if(rc < 0) 
                return rc; 
                                
        rc = ov8820_i2c_write_b(ov8820_client->addr, OV8820_GAIN, (uint8_t)gain); 
        if (rc < 0) 
                return rc; 
	//lanch and hold end of group register 
	 //rc = ov8820_i2c_write_b(ov8820_client->addr, 0x3208, 0x10);
       // rc = ov8820_i2c_write_b(ov8820_client->addr, 0x3208, 0xa0);

/*	 printk("%s, pict_fps_divider=%x fps_divider=%x\n",__func__,\
					ov8820_ctrl->pict_fps_divider,\
					ov8820_ctrl->fps_divider);
	 printk("%s, 0x380e=%x 0x380f=%x\n",__func__,\
					(((total_lines_per_frame * line_length_ratio / 0x00000400) & 0xFF00) >> 8),\
					((total_lines_per_frame * line_length_ratio / 0x00000400) & 0x00FF) );

	 printk("%s, total_length=%d line=%d\n",__func__,total_lines_per_frame, line);
        printk("%s,%d line length ratio is %d\n",__func__,__LINE__,line_length_ratio);*/
        printk("%s: gain: %d: Linecount: %d\n",__func__,gain,line); 

        stored_line_length_ratio = line_length_ratio; 
        //printk("maliang %s rc is  %d\n", __func__, rc);//add by maliang for ov8820 
        return rc; 
}/* endof ov8820_write_exp_gain*/ 
//changed by maliang for ov8820 exposure end
static int32_t ov8820_set_pict_exp_gain(uint16_t gain, uint32_t line)
{
	int32_t rc = 0;
#ifdef PRE_RAW
	return 0;//changed by maliang for preview raw
#endif
	rc = ov8820_write_exp_gain(gain, line);
	return rc;
}/* endof ov8820_set_pict_exp_gain*/
static int32_t ov8820_test(enum ov8820_test_mode_t mo)
{
	int32_t rc = 0;
	if (mo == TEST_OFF){
		return rc;
	}
	/* Activate  the Color bar test pattern */
/*
	if(mo == TEST_1) {
		rc = ov8820_i2c_write_b(ov8820_client->addr,0x3303, 0x01);
		if (rc < 0) {
			return rc;
		}
		rc = ov8820_i2c_write_b(ov8820_client->addr, 0x3316, 0x02);
		if (rc < 0 ) {
			return rc;
		}
	}
	*/
	return rc;
}
static int32_t initialize_ov8820_registers(void)
{
	int32_t i, array_length;
	int32_t rc = 0;
	ov8820_ctrl->sensormode = SENSOR_PREVIEW_MODE ;
	array_length = sizeof(ov8820_init_settings_array) /
	sizeof(ov8820_init_settings_array[0]);
//add by maliang for ov8820 110725 begin
//for reset sensor
rc = ov8820_i2c_write_b(ov8820_client->addr,0x0103, 0x01);
	if(rc < 0) {
		return rc;
	}
	msleep(5);
//add by maliang for ov8820 110725 end
	printk("initialize_ov8820_registers: array_length: %d\n",array_length);
	/* Configure sensor for Preview mode and Snapshot mode */
	for (i=0; i<array_length; i++) {
		rc = ov8820_i2c_write_b(ov8820_client->addr, ov8820_init_settings_array[i].reg_addr,
				ov8820_init_settings_array[i].reg_val);
		if ( rc < 0) {
			return rc;
		}
	}
	printk("initialize_ov8820_registers: Done\n");
	return rc;
} /* end of initialize_ov8820_ov8m0vc_registers. */
static int32_t ov8820_setting( int rt)
{
	int32_t rc = 0;
	int32_t i, array_length;
	struct msm_camera_csid_params ov8820_csid_params;
	struct msm_camera_csiphy_params ov8820_csiphy_params;	
//add by maliang for ov8820 af begin
#if 0
	uint8_t stored_af_step = 0;
	printk("sensor_settings\n");
	stored_af_step = ov8820_ctrl->curr_step_pos;
	ov8820_set_default_focus(0);
#endif	
//add by maliang for ov8820 af end
	//uint16_t sensor_registor;//add by maliang for ov8820
	printk("maliang stop sensor stream\n");//add by maliang for ov8820 debug
	rc = ov8820_i2c_write_b(ov8820_client->addr,0x0100, 0x00);
	if(rc < 0) {
		return rc;
	}//changed by maliang 110725 stop stream
	msleep(5);
	//changed by maliang
	
	if (csi_config == 0) {
		struct msm_camera_csid_vc_cfg ov8820_vccfg[] = {
				{0, CSI_RAW10, CSI_DECODE_10BIT},
				{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
			};
		ov8820_csid_params.lane_cnt = 2;
		ov8820_csid_params.lane_assign = 0xe4;
		ov8820_csid_params.lut_params.num_cid =
			ARRAY_SIZE(ov8820_vccfg);
		ov8820_csid_params.lut_params.vc_cfg =
			&ov8820_vccfg[0];
		ov8820_csiphy_params.lane_cnt = 2;
		ov8820_csiphy_params.settle_cnt = 0x1B;
		rc = msm_camio_csid_config(&ov8820_csid_params);
		v4l2_subdev_notify(ov8820_ctrl->sensor_dev,
					NOTIFY_CID_CHANGE, NULL);
		dsb();
		rc = msm_camio_csiphy_config(&ov8820_csiphy_params);
		dsb();
		csi_config++;
	}

	switch(rt)
	{
		case QTR_SIZE:
			array_length = sizeof(ov8820_qtr_settings_array) /
					sizeof(ov8820_qtr_settings_array[0]);
			/* Configure sensor for preview mode */
			for (i=0; i<array_length; i++) {
				rc = ov8820_i2c_write_b(ov8820_client->addr, ov8820_qtr_settings_array[i].reg_addr,
					ov8820_qtr_settings_array[i].reg_val);
				if ( rc < 0) {
					return rc;
				}
			}
			ov8820_ctrl->curr_res = QTR_SIZE;
			break;
#ifdef OV8820_HD//add by maliang for ov8820 1080P
		case HD_SIZE:
			array_length = sizeof(ov8820_hd_settings_array) /
					sizeof(ov8820_hd_settings_array[0]);
			/* Configure sensor for preview mode */
			for (i=0; i<array_length; i++) {
				rc = ov8820_i2c_write_b(ov8820_client->addr, ov8820_hd_settings_array[i].reg_addr,
					ov8820_hd_settings_array[i].reg_val);
				if ( rc < 0) {
					return rc;
				}
			}
			ov8820_ctrl->curr_res = HD_SIZE;
			printk("maliang set to HD_SIZE\n");
			break;
#endif
		case FULL_SIZE:
			array_length = sizeof(ov8820_full_settings_array) /
				sizeof(ov8820_full_settings_array[0]);
			/* Configure sensor for capture mode */
			for (i=0; i<array_length; i++) {
				rc = ov8820_i2c_write_b(ov8820_client->addr, ov8820_full_settings_array[i].reg_addr,
						ov8820_full_settings_array[i].reg_val);
				if (rc < 0)
					return rc;
			}
			ov8820_ctrl->curr_res = FULL_SIZE;
			break;
		default:
			rc = -EFAULT;
			return rc;
	}
#if 0
//add by maliang for ov8820 af begin	
	ov8820_move_focus(MOVE_NEAR, stored_af_step);
//add by maliang for ov8820 af end
#endif
	printk("maliang start sensor stream\n");
	rc = ov8820_i2c_write_b(ov8820_client->addr,0x0100, 0x01);
	if (rc < 0)
		return rc;
	msleep(5);
	rc = ov8820_test(ov8820_ctrl->set_test);
	if ( rc < 0)
		return rc;
	return rc;
} /*endof  ov8820_setting*/
static int32_t ov8820_video_config(int mode )
{
	int32_t rc = 0;
	if( ov8820_ctrl->curr_res != ov8820_ctrl->prev_res){
		rc = ov8820_setting(ov8820_ctrl->prev_res);
		if (rc < 0)
			return rc;
	}
	else {
		ov8820_ctrl->curr_res = ov8820_ctrl->prev_res;
	}
	ov8820_ctrl->sensormode = mode;
	printk("maliang %s rc is  %d\n", __func__, rc);//add by maliang for ov8820
	return rc;
}/*end of ov354_video_config*/
#ifdef OV8820_HD //changed by maliang for 1080P
static int32_t ov8820_video2_config(int mode )
{
	int32_t rc = 0;
	if( ov8820_ctrl->curr_res != ov8820_ctrl->video_res){
		rc = ov8820_setting(ov8820_ctrl->video_res);
		if (rc < 0)
			return rc;
	}
	else {
		ov8820_ctrl->curr_res = ov8820_ctrl->prev_res;
	}
	ov8820_ctrl->sensormode = mode;
	printk("maliang %s rc is  %d\n", __func__, rc);//add by maliang for ov8820
	return rc;
}/*end of ov354_video_config*/
#endif

static int32_t ov8820_snapshot_config(int mode)
{
	int32_t rc = 0;
	printk("maliang %s FULL_SIZE\n",__func__);
	if (ov8820_ctrl->curr_res != ov8820_ctrl->pict_res) {
		rc = ov8820_setting(ov8820_ctrl->pict_res);
		if (rc < 0)
			return rc;
	}
	else {
		ov8820_ctrl->curr_res = ov8820_ctrl->pict_res;
	}
	ov8820_ctrl->sensormode = mode;
	//printk("maliang %s rc is  %d\n", __func__, rc);//add by maliang for ov8820
	return rc;
}/*end of ov8820_snapshot_config*/

static int32_t ov8820_raw_snapshot_config(int mode)
{
	int32_t rc = 0;
#ifdef PRE_RAW
	return 0;//changed by maliang for preview capture
#endif
	printk("maliang %s FULL_SIZE\n",__func__);
	ov8820_ctrl->sensormode = mode;
	if (ov8820_ctrl->curr_res != ov8820_ctrl->pict_res) {
		rc = ov8820_setting(ov8820_ctrl->pict_res);
		if (rc < 0)
			return rc;
	}
	else {
		ov8820_ctrl->curr_res = ov8820_ctrl->pict_res;
	}/* Update sensor resolution */
	ov8820_ctrl->sensormode = mode;
	//printk("maliang %s rc is  %d\n", __func__, rc);//add by maliang for ov8820
	return rc;
}/*end of ov8820_raw_snapshot_config*/
static int32_t ov8820_set_sensor_mode(int  mode,
			int  res)
{
	int32_t rc = 0;
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		rc = ov8820_video_config(mode);
		break;
	case SENSOR_SNAPSHOT_MODE:
		rc = ov8820_snapshot_config(mode);
		break;
	case SENSOR_RAW_SNAPSHOT_MODE:
		rc = ov8820_raw_snapshot_config(mode);
		break;
#ifdef OV8820_HD
	case SENSOR_VIDEO1080P_MODE:
		rc = ov8820_video2_config(mode);
		break;
#endif
	default:
		rc = -EINVAL;
		break;
	}
	return rc;
}
static int32_t ov8820_power_down(void)
{
	return 0;
}
static int ov8820_probe_init_sensor(const struct msm_camera_sensor_info *data)
{
	int32_t rc = 0;
	uint16_t  chipidl, chipidh;
	
	printk("%s : E",__func__);
	rc = gpio_request(data->sensor_platform_info->sensor_reset, "ov8820");
	if (!rc) {
		gpio_direction_output(data->sensor_platform_info->sensor_reset, 0);
		msleep(50);
		gpio_set_value_cansleep(data->sensor_platform_info->sensor_reset, 1);
		msleep(20);
	} else
		goto init_probe_done;
	/* 3. Read sensor Model ID: */
	if (ov8820_i2c_read(OV8820_PIDH_REG, &chipidh,1) < 0)
		goto init_probe_fail;
	if (ov8820_i2c_read(OV8820_PIDL_REG, &chipidl,1) < 0)
		goto init_probe_fail;
	
	printk(" ov8820 model_id = 0x%x  0x%x\n", chipidh, chipidl);
	/* 4. Compare sensor ID to OV8820 ID: */
	if (chipidh != OV8820_PID || chipidl < OV8820_VER) {
		rc = -ENODEV;
		goto init_probe_fail;
	}
	mdelay(OV8820_RESET_DELAY_MSECS);
	printk(" ov8820_probe_init_sensor ok\n");
	goto init_probe_done;
init_probe_fail:
	printk(" ov8820_probe_init_sensor fails\n");
	gpio_direction_output(data->sensor_platform_info->sensor_reset, 0);
	gpio_set_value_cansleep(data->sensor_platform_info->sensor_reset, 0);
	gpio_free(data->sensor_platform_info->sensor_reset);
	rc = -ENODEV;
init_probe_done:
	printk(" ov8820_probe_init_sensor finishes\n");
	return rc;
}
int ov8820_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int i;
	int32_t  rc;
	uint16_t ov8820_nl_region_boundary = 3;
	uint16_t ov8820_nl_region_code_per_step = 101;
	uint16_t ov8820_l_region_code_per_step = 18;

	printk("Calling ov8820_sensor_open_init\n");
	ov8820_ctrl->curr_lens_pos = -1;
	ov8820_ctrl->fps_divider = 1 * 0x00000400;
	ov8820_ctrl->pict_fps_divider = 1 * 0x00000400;
	ov8820_ctrl->set_test = TEST_OFF;
	ov8820_ctrl->prev_res = QTR_SIZE;//FULL_SIZE;//changed by maliang
	ov8820_ctrl->pict_res = FULL_SIZE;
#ifdef OV8820_HD //add by maliang for ov8820 1080P
	ov8820_ctrl->video_res = HD_SIZE;
#endif
	ov8820_ctrl->curr_res = INVALID_SIZE;
	csi_config = 0;//add by maliang for ov8820
	if (data)
		ov8820_ctrl->sensordata = data;
	/* enable mclk first */
	msm_camio_clk_rate_set(OV8820_DEFAULT_CLOCK_RATE);
	mdelay(20);
	printk("ov8820_sensor_open_init: Default clock is set.\n");

	rc = ov8820_probe_init_sensor(data);
	if (rc < 0)
		goto init_fail;
	/* Initialize Sensor registers */
	rc = initialize_ov8820_registers();
	if (rc < 0)
		goto init_fail;

	ov8820_ctrl->fps = 30*Q8;
	step_position_table[0] = 0;
	for(i = 1; i <= OV8820_TOTAL_STEPS_NEAR_TO_FAR; i++)
	{
		if ( i <= ov8820_nl_region_boundary) {
			ov8820_step_position_table[i] = ov8820_step_position_table[i-1] + ov8820_nl_region_code_per_step;
		}
		else {
			ov8820_step_position_table[i] = ov8820_step_position_table[i-1] + ov8820_l_region_code_per_step;
		}
	}

	if (rc < 0)
		goto init_fail;
	else
		goto init_done;
	/* reset the driver state */
init_fail:
	printk("init_fail\n");
	gpio_set_value_cansleep(data->sensor_platform_info->sensor_reset, 0);
	//kfree(ov8820_ctrl);
init_done:
	printk("init_done\n");
	return rc;
}

static int ov8820_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&ov8820_wait_queue);
	return 0;
}

static const struct i2c_device_id ov8820_i2c_id[] = {
	{ "ov8820", 0},
	{ }
};

static int ov8820_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	printk("ov8820_probe called!\n");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk("i2c_check_functionality failed\n");
		goto probe_failure;
	}
	ov8820_sensorw = kzalloc(sizeof(struct ov8820_work_t), GFP_KERNEL);
	if (!ov8820_sensorw) {
		printk("kzalloc failed.\n");
		rc = -ENOMEM;
		goto probe_failure;
	}
	i2c_set_clientdata(client, ov8820_sensorw);
	ov8820_init_client(client);
	ov8820_client = client;
	mdelay(50);
	printk("ov8820_probe successed! rc = %d\n", rc);
	return 0;
probe_failure:
	printk("ov8820_probe failed! rc = %d\n", rc);
	return rc;
}

static int __exit ov8820_remove(struct i2c_client *client)
{
	struct ov8820_work_t_t *sensorw = i2c_get_clientdata(client);
	free_irq(client->irq, sensorw);
	ov8820_client = NULL;
	kfree(sensorw);
	return 0;
}

static struct i2c_driver ov8820_i2c_driver = {
	.id_table = ov8820_i2c_id,
	.probe	= ov8820_i2c_probe,
	.remove = __exit_p(ov8820_i2c_remove),
	.driver = {
		.name = "ov8820",
	},
};

int ov8820_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cdata;
	long   rc = 0;

	if (copy_from_user(&cdata,
				(void *)argp,
				sizeof(struct sensor_cfg_data)))
		return -EFAULT;
	mutex_lock(&ov8820_mut);
	printk("ov8820_sensor_config: cfgtype = %d\n",
		cdata.cfgtype);
		switch (cdata.cfgtype) {
		case CFG_GET_PICT_FPS:
				ov8820_get_pict_fps(
				cdata.cfg.gfps.prevfps,
				&(cdata.cfg.gfps.pictfps));
			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;
		case CFG_GET_PREV_L_PF:
			cdata.cfg.prevl_pf =
			ov8820_get_prev_lines_pf();
			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;
		case CFG_GET_PREV_P_PL:
			cdata.cfg.prevp_pl =
				ov8820_get_prev_pixels_pl();
			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;
		case CFG_GET_PICT_L_PF:
			cdata.cfg.pictl_pf =
				ov8820_get_pict_lines_pf();
			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;
		case CFG_GET_PICT_P_PL:
			cdata.cfg.pictp_pl =
				ov8820_get_pict_pixels_pl();
			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;
		case CFG_GET_PICT_MAX_EXP_LC:
			cdata.cfg.pict_max_exp_lc =
				ov8820_get_pict_max_exp_lc();
			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;
		case CFG_SET_FPS:
		case CFG_SET_PICT_FPS:
			rc = ov8820_set_fps(&(cdata.cfg.fps));
			break;
		case CFG_SET_EXP_GAIN:
			rc =
				ov8820_write_exp_gain(
					cdata.cfg.exp_gain.gain,
					cdata.cfg.exp_gain.line);
			break;
		case CFG_SET_PICT_EXP_GAIN:
			rc =
				ov8820_set_pict_exp_gain(
					cdata.cfg.exp_gain.gain,
					cdata.cfg.exp_gain.line);
			break;
		case CFG_SET_MODE:
			rc = ov8820_set_sensor_mode(cdata.mode,
						cdata.rs);
			break;
		case CFG_PWR_DOWN:
			rc = ov8820_power_down();
			break;
		case CFG_MOVE_FOCUS:
			rc =	ov8820_move_focus(
					cdata.cfg.focus.dir,
					cdata.cfg.focus.steps);
			printk("malaing:%s move_focus\n",__func__);
			break;
		case CFG_SET_DEFAULT_FOCUS:
			rc =	ov8820_set_default_focus(
					cdata.cfg.focus.steps);
			printk("malaing:%s CFG_SET_DEFAULT_FOCUS\n",__func__);
			break;
		case CFG_GET_AF_MAX_STEPS:
			cdata.max_steps = OV8820_TOTAL_STEPS_NEAR_TO_FAR;
			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;
		case CFG_SET_EFFECT:
			rc = ov8820_set_default_focus(
						cdata.cfg.effect);
			printk("malaing:%s CFG_SET_EFFECT\n",__func__);
			break;
		default:
			rc = -EFAULT;
			break;
		}
	mutex_unlock(&ov8820_mut);
	return rc;
}
static int ov8820_probe_init_done(const struct msm_camera_sensor_info *data)
{
	gpio_direction_output(data->sensor_platform_info->sensor_reset, 0);
	gpio_free(data->sensor_platform_info->sensor_reset);
	return 0;
}

static int ov8820_sensor_release(void)
{
	int rc = -EBADF;
	mutex_lock(&ov8820_mut);
	ov8820_power_down();
	gpio_set_value_cansleep(ov8820_ctrl->sensordata->sensor_platform_info->sensor_reset, 0);
	gpio_free(ov8820_ctrl->sensordata->sensor_platform_info->sensor_reset);
	csi_config = 0;
	printk("ov8820_release completed\n");
	mutex_unlock(&ov8820_mut);
	return rc;
}

static int ov8820_sensor_probe(const struct msm_camera_sensor_info *info,
		struct msm_sensor_ctrl *s)
{
	int rc = 0;

	printk("%s : E",__func__);
	rc = i2c_add_driver(&ov8820_i2c_driver);
	if (rc < 0 || ov8820_client == NULL) {
		rc = -ENOTSUPP;
		printk("I2C add driver failed");
		goto probe_fail;
	}
	msm_camio_clk_rate_set(24000000);
	mdelay(20);
	rc = ov8820_probe_init_sensor(info);
	if (rc < 0)
		goto probe_fail;

	s->s_init = ov8820_sensor_open_init;
	s->s_release = ov8820_sensor_release;
	s->s_config  = ov8820_sensor_config;
	s->s_mount_angle = 90;
	ov8820_probe_init_done(info);

	return rc;

probe_fail:
	printk("SENSOR PROBE FAILS!\n");
	return rc;
}

static int ov8820_enum_fmt(struct v4l2_subdev *sd, unsigned int index,
			   enum v4l2_mbus_pixelcode *code)
{
	printk(KERN_DEBUG "Index is %d\n", index);
	if ((unsigned int)index >= ARRAY_SIZE(ov8820_subdev_info))
		return -EINVAL;

	*code = ov8820_subdev_info[index].code;
	return 0;
}

static struct v4l2_subdev_core_ops ov8820_subdev_core_ops;
static struct v4l2_subdev_video_ops ov8820_subdev_video_ops = {
	.enum_mbus_fmt = ov8820_enum_fmt,
};

static struct v4l2_subdev_ops ov8820_subdev_ops = {
	.core = &ov8820_subdev_core_ops,
	.video  = &ov8820_subdev_video_ops,
};


static int ov8820_sensor_probe_cb(const struct msm_camera_sensor_info *info,
	struct v4l2_subdev *sdev, struct msm_sensor_ctrl *s)
{
	int rc = 0;
	ov8820_ctrl = kzalloc(sizeof(struct ov8820_ctrl_t), GFP_KERNEL);
	if (!ov8820_ctrl) {
		printk("ov8820_sensor_probe failed!\n");
		return -ENOMEM;
	}

	rc = ov8820_sensor_probe(info, s);
	if (rc < 0)
		return rc;

	/* probe is successful, init a v4l2 subdevice */
	printk(KERN_DEBUG "going into v4l2_i2c_subdev_init\n");
	if (sdev) {
		v4l2_i2c_subdev_init(sdev, ov8820_client,
						&ov8820_subdev_ops);
		ov8820_ctrl->sensor_dev = sdev;
	}
	return rc;
}

static int __ov8820_probe(struct platform_device *pdev)
{
	return msm_sensor_register(pdev, ov8820_sensor_probe_cb);
}

static struct platform_driver msm_camera_driver = {
	.probe = __ov8820_probe,
	.driver = {
		.name = "msm_camera_ov8820",
		.owner = THIS_MODULE,
	},
};

static int __init ov8820_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(ov8820_init);
void ov8820_exit(void)
{
	i2c_del_driver(&ov8820_i2c_driver);
}
module_exit(ov8820_exit);

MODULE_DESCRIPTION("OVT 8 MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");

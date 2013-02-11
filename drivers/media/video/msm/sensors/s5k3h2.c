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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include <mach/camera.h>
#include <media/v4l2-subdev.h>
#include "s5k3h2.h"
#include "msm.h"
/*=============================================================
	SENSOR REGISTER DEFINES
==============================================================*/
#define REG_GROUPED_PARAMETER_HOLD		0x0104
#define GROUPED_PARAMETER_HOLD_OFF		0x00
#define GROUPED_PARAMETER_HOLD			0x01
/* Integration Time */
#define REG_COARSE_INTEGRATION_TIME		0x0202
/* Gain */
#define REG_GLOBAL_GAIN					0x0204

#define REG_FRAME_LENGTH_LINES			0x0340
#define REG_LINE_LENGTH_PCK			0x342
/* Test Pattern */
#define REG_TEST_PATTERN_MODE			0x0601
#define REG_VCM_NEW_CODE			0x30F2

/* Mode Select */
#define S5K3H2_MODE_SEL			0x0100

/* Sensor Model ID */
#define S5K3H2_MODEL_ID			0x382B
#define S5K3H2_OFFSET			8

/*============================================================================
							 TYPE DECLARATIONS
============================================================================*/

/* 16bit address - 8 bit context register structure */
#define Q8  0x00000100
#define Q10 0x00000400
#define S5K3H2_MASTER_CLK_RATE 24000000

/* AF Total steps parameters */
#define S5K3H2_TOTAL_STEPS_NEAR_TO_FAR    32

uint16_t s5k3h2_step_position_table[S5K3H2_TOTAL_STEPS_NEAR_TO_FAR+1];
uint16_t s5k3h2_nl_region_boundary1 = 3;
uint16_t s5k3h2_nl_region_code_per_step1 = 30;
uint16_t s5k3h2_l_region_code_per_step = 4;
uint16_t s5k3h2_damping_threshold = 10;
uint16_t s5k3h2_sw_damping_time_wait = 1;

struct s5k3h2_work_t {
	struct work_struct work;
};

static struct s5k3h2_work_t *s5k3h2_sensorw;
static struct i2c_client *s5k3h2_client;

struct s5k3h2_ctrl_t {
	const struct  msm_camera_sensor_info *sensordata;

	uint32_t sensormode;
	uint32_t fps_divider;/* init to 1 * 0x00000400 */
	uint32_t pict_fps_divider;/* init to 1 * 0x00000400 */
	uint16_t fps;

	uint16_t curr_lens_pos;
	uint16_t curr_step_pos;
	uint16_t my_reg_gain;
	uint32_t my_reg_line_count;
	uint16_t total_lines_per_frame;

	enum s5k3h2_resolution_t prev_res;
	enum s5k3h2_resolution_t pict_res;
	enum s5k3h2_resolution_t curr_res;
	enum s5k3h2_test_mode_t  set_test;

	struct v4l2_subdev *sensor_dev;
	struct s5k3h2_format *fmt;
};

static uint16_t prev_line_length_pck;
static uint16_t prev_frame_length_lines;
static uint16_t snap_line_length_pck;
static uint16_t snap_frame_length_lines;

static bool CSI_CONFIG;
static struct s5k3h2_ctrl_t *s5k3h2_ctrl;
static DECLARE_WAIT_QUEUE_HEAD(s5k3h2_wait_queue);
DEFINE_MUTEX(s5k3h2_mut);

static int cam_debug_init(void);
static struct dentry *debugfs_base;

struct s5k3h2_format {
	enum v4l2_mbus_pixelcode code;
	enum v4l2_colorspace colorspace;
	u16 fmt;
	u16 order;
};
/*=============================================================*/
static struct s5k3h2_format s5k3h2_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static int s5k3h2_i2c_rxdata(unsigned short saddr,
	unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
		{
			.addr  = saddr,
			.flags = 0,
			.len   = 2,
			.buf   = rxdata,
		},
		{
			.addr  = saddr,
			.flags = I2C_M_RD,
			.len   = 2,
			.buf   = rxdata,
		},
	};
	if (i2c_transfer(s5k3h2_client->adapter, msgs, 2) < 0) {
		printk("s5k3h2_i2c_rxdata faild 0x%x\n", saddr);
		return -EIO;
	}
	return 0;
}

static int32_t s5k3h2_i2c_txdata(unsigned short saddr,
				unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
			.addr = saddr,
			.flags = 0,
			.len = length,
			.buf = txdata,
		 },
	};
	if (i2c_transfer(s5k3h2_client->adapter, msg, 1) < 0) {
		printk("s5k3h2_i2c_txdata faild 0x%x\n", saddr);
		return -EIO;
	}

	return 0;
}

static int32_t s5k3h2_i2c_read(unsigned short raddr,
	unsigned short *rdata, int rlen)
{
	int32_t rc = 0;
	unsigned char buf[2];
	if (!rdata)
		return -EIO;
	memset(buf, 0, sizeof(buf));
	buf[0] = (raddr & 0xFF00) >> 8;
	buf[1] = (raddr & 0x00FF);

	rc = s5k3h2_i2c_rxdata(s5k3h2_client->addr>>1, buf, rlen);

	printk("s5k3h2_i2c_read addr: 0x%x data: 0x%x rc: %d\n", s5k3h2_client->addr>>1,raddr,rc);

	//rc = s5k3h2_i2c_rxdata(0x36, buf, rlen);
	//printk("s5k3h2_i2c_read addr: 0x%x data: 0x%x rc: %d\n", 0x36,raddr,rc);

	if (rc < 0) {
		printk("s5k3h2_i2c_read 0x%x failed!\n", raddr);
		return rc;
	}
	*rdata = (rlen == 2 ? buf[0] << 8 | buf[1] : buf[0]);
	printk("s5k3h2_i2c_read 0x%x val = 0x%x!\n", raddr, *rdata);
	return rc;
}

static int32_t s5k3h2_i2c_write_w_sensor(unsigned short waddr, uint16_t wdata)
{
	int32_t rc = -EFAULT;
	unsigned char buf[4];
	memset(buf, 0, sizeof(buf));
	buf[0] = (waddr & 0xFF00) >> 8;
	buf[1] = (waddr & 0x00FF);
	buf[2] = (wdata & 0xFF00) >> 8;
	buf[3] = (wdata & 0x00FF);
	printk("i2c_write_b addr = 0x%x, val = 0x%x\n", waddr, wdata);
	rc = s5k3h2_i2c_txdata(s5k3h2_client->addr>>1, buf, 4);
	if (rc < 0) {
		printk("i2c_write_b failed, addr = 0x%x, val = 0x%x!\n",
			waddr, wdata);
	}
	return rc;
}

static int32_t s5k3h2_i2c_write_b_sensor(unsigned short waddr, uint8_t bdata)
{
	int32_t rc = -EFAULT;
	unsigned char buf[3];
	memset(buf, 0, sizeof(buf));
	buf[0] = (waddr & 0xFF00) >> 8;
	buf[1] = (waddr & 0x00FF);
	buf[2] = bdata;
	printk("i2c_write_b addr = 0x%x, val = 0x%x\n", waddr, bdata);
	rc = s5k3h2_i2c_txdata(s5k3h2_client->addr>>1, buf, 3);
	if (rc < 0) {
		printk("i2c_write_b failed, addr = 0x%x, val = 0x%x!\n",
			waddr, bdata);
	}
	return rc;
}

static int32_t s5k3h2_i2c_write_b_table(struct s5k3h2_i2c_reg_conf const
					 *reg_conf_tbl, int num)
{
	int i;
	int32_t rc = -EIO;
	for (i = 0; i < num; i++) {
		rc = s5k3h2_i2c_write_b_sensor(reg_conf_tbl->waddr,
			reg_conf_tbl->wdata);
		if (rc < 0)
			break;
		reg_conf_tbl++;
	}
	return rc;
}

static void s5k3h2_group_hold_on(void)
{
	s5k3h2_i2c_write_b_sensor(REG_GROUPED_PARAMETER_HOLD,
						GROUPED_PARAMETER_HOLD);
}

static void s5k3h2_group_hold_off(void)
{
	s5k3h2_i2c_write_b_sensor(REG_GROUPED_PARAMETER_HOLD,
						GROUPED_PARAMETER_HOLD_OFF);
}

static void s5k3h2_start_stream(void)
{
	s5k3h2_i2c_write_b_sensor(S5K3H2_MODE_SEL, 0x01);
}

static void s5k3h2_stop_stream(void)
{
	s5k3h2_i2c_write_b_sensor(S5K3H2_MODE_SEL, 0x00);
}

static void s5k3h2_get_pict_fps(uint16_t fps, uint16_t *pfps)
{
	/* input fps is preview fps in Q8 format */
	uint32_t divider, d1, d2;

	d1 = prev_frame_length_lines * 0x00000400 / snap_frame_length_lines;
	d2 = prev_line_length_pck * 0x00000400 / snap_line_length_pck;
	divider = d1 * d2 / 0x400;

	/*Verify PCLK settings and frame sizes.*/
	*pfps = (uint16_t) (fps * divider / 0x400);
	/* 2 is the ratio of no.of snapshot channels
	to number of preview channels */
}

static uint16_t s5k3h2_get_prev_lines_pf(void)
{
	if (s5k3h2_ctrl->prev_res == QTR_SIZE)
		return prev_frame_length_lines;
	else
		return snap_frame_length_lines;
}

static uint16_t s5k3h2_get_prev_pixels_pl(void)
{
	if (s5k3h2_ctrl->prev_res == QTR_SIZE)
		return prev_line_length_pck;
	else
		return snap_line_length_pck;
}

static uint16_t s5k3h2_get_pict_lines_pf(void)
{
	if (s5k3h2_ctrl->pict_res == QTR_SIZE)
		return prev_frame_length_lines;
	else
		return snap_frame_length_lines;
}

static uint16_t s5k3h2_get_pict_pixels_pl(void)
{
	if (s5k3h2_ctrl->pict_res == QTR_SIZE)
		return prev_line_length_pck;
	else
		return snap_line_length_pck;
}

static uint32_t s5k3h2_get_pict_max_exp_lc(void)
{
	if (s5k3h2_ctrl->pict_res == QTR_SIZE)
		return prev_frame_length_lines * 24;
	else
		return snap_frame_length_lines * 24;
}

static int32_t s5k3h2_set_fps(struct fps_cfg   *fps)
{
	uint16_t total_lines_per_frame;
	int32_t rc = 0;
	if (s5k3h2_ctrl->curr_res == QTR_SIZE) {
		total_lines_per_frame = (uint16_t)
		((prev_frame_length_lines) * s5k3h2_ctrl->fps_divider/0x400);
	} else {
		total_lines_per_frame = (uint16_t)((snap_frame_length_lines) *
		s5k3h2_ctrl->pict_fps_divider/0x400);
	}
	s5k3h2_ctrl->fps_divider = fps->fps_div;
	s5k3h2_ctrl->pict_fps_divider = fps->pict_fps_div;

	s5k3h2_group_hold_on();
	rc = s5k3h2_i2c_write_w_sensor(REG_FRAME_LENGTH_LINES,
							total_lines_per_frame);
	s5k3h2_group_hold_off();
	return rc;
}

static int32_t s5k3h2_write_exp_gain(uint16_t gain, uint32_t line)
{
	uint16_t max_legal_gain = 0x0200;
	uint16_t min_ll_pck = 0x0AB2;
	uint32_t ll_pck, fl_lines;
	uint32_t ll_ratio;
	int32_t rc = 0;
	if (gain > max_legal_gain) {
		printk("Max legal gain Line:%d\n", __LINE__);
		gain = max_legal_gain;
	}

	if (s5k3h2_ctrl->curr_res == QTR_SIZE) {
		s5k3h2_ctrl->my_reg_gain = gain;
		s5k3h2_ctrl->my_reg_line_count = (uint16_t) line;
		fl_lines = prev_frame_length_lines;
		ll_pck = prev_line_length_pck;
		ll_ratio = prev_line_length_pck * Q10 /
			(prev_frame_length_lines - S5K3H2_OFFSET);
	} else {
		fl_lines = snap_frame_length_lines;
		ll_pck = snap_line_length_pck;
		ll_ratio = (snap_line_length_pck * Q10 /
			(snap_frame_length_lines - S5K3H2_OFFSET));
	}

	if ((fl_lines - S5K3H2_OFFSET) < line) {
		ll_pck = line * ll_ratio / Q10;
		line = (fl_lines - S5K3H2_OFFSET);
	}

	if (ll_pck < min_ll_pck)
		ll_pck = min_ll_pck;

	s5k3h2_group_hold_on();
	rc = s5k3h2_i2c_write_w_sensor(REG_GLOBAL_GAIN, gain);
	rc = s5k3h2_i2c_write_w_sensor(REG_LINE_LENGTH_PCK, ll_pck);
	rc = s5k3h2_i2c_write_w_sensor(REG_COARSE_INTEGRATION_TIME, line);
	s5k3h2_group_hold_off();
	return rc;
}

static int32_t s5k3h2_set_pict_exp_gain(uint16_t gain, uint32_t line)
{
	int32_t rc = 0;
	rc = s5k3h2_write_exp_gain(gain, line);
	return rc;
}

#define DIV_CEIL(x, y) (x/y + (x%y) ? 1 : 0)

static int32_t s5k3h2_move_focus(int direction,
	int32_t num_steps)
{
	int16_t step_direction, dest_lens_position, dest_step_position;
	int16_t target_dist, small_step, next_lens_position;
	return 0;
	if (direction == MOVE_NEAR)
		step_direction = 1;
	else
		step_direction = -1;

	dest_step_position = s5k3h2_ctrl->curr_step_pos
						+ (step_direction * num_steps);

	if (dest_step_position < 0)
		dest_step_position = 0;
	else if (dest_step_position > S5K3H2_TOTAL_STEPS_NEAR_TO_FAR)
		dest_step_position = S5K3H2_TOTAL_STEPS_NEAR_TO_FAR;

	if (dest_step_position == s5k3h2_ctrl->curr_step_pos)
		return 0;

	dest_lens_position = s5k3h2_step_position_table[dest_step_position];
	target_dist = step_direction *
		(dest_lens_position - s5k3h2_ctrl->curr_lens_pos);

	if (step_direction < 0 && (target_dist >=
		s5k3h2_step_position_table[s5k3h2_damping_threshold])) {
		small_step = DIV_CEIL(target_dist, 10);
		s5k3h2_sw_damping_time_wait = 10;
	} else {
		small_step = DIV_CEIL(target_dist, 4);
		s5k3h2_sw_damping_time_wait = 4;
	}

	for (next_lens_position = s5k3h2_ctrl->curr_lens_pos
		+ (step_direction * small_step);
		(step_direction * next_lens_position) <=
		(step_direction * dest_lens_position);
		next_lens_position += (step_direction * small_step)) {
		s5k3h2_i2c_write_w_sensor(REG_VCM_NEW_CODE,
		next_lens_position);
		s5k3h2_ctrl->curr_lens_pos = next_lens_position;
		usleep(s5k3h2_sw_damping_time_wait*50);
	}

	if (s5k3h2_ctrl->curr_lens_pos != dest_lens_position) {
		s5k3h2_i2c_write_w_sensor(REG_VCM_NEW_CODE,
		dest_lens_position);
		usleep(s5k3h2_sw_damping_time_wait*50);
	}
	s5k3h2_ctrl->curr_lens_pos = dest_lens_position;
	s5k3h2_ctrl->curr_step_pos = dest_step_position;
	return 0;
}

static int32_t s5k3h2_set_default_focus(uint8_t af_step)
{
	int32_t rc = 0;
	return 0;
	if (s5k3h2_ctrl->curr_step_pos != 0) {
		rc = s5k3h2_move_focus(MOVE_FAR,
		s5k3h2_ctrl->curr_step_pos);
	} else {
		s5k3h2_i2c_write_w_sensor(REG_VCM_NEW_CODE, 0x00);
	}

	s5k3h2_ctrl->curr_lens_pos = 0;
	s5k3h2_ctrl->curr_step_pos = 0;

	return rc;
}

static void s5k3h2_init_focus(void)
{
	uint8_t i;
	s5k3h2_step_position_table[0] = 0;
	for (i = 1; i <= S5K3H2_TOTAL_STEPS_NEAR_TO_FAR; i++) {
		if (i <= s5k3h2_nl_region_boundary1) {
			s5k3h2_step_position_table[i] =
				s5k3h2_step_position_table[i-1]
				+ s5k3h2_nl_region_code_per_step1;
		} else {
			s5k3h2_step_position_table[i] =
				s5k3h2_step_position_table[i-1]
				+ s5k3h2_l_region_code_per_step;
		}

		if (s5k3h2_step_position_table[i] > 255)
			s5k3h2_step_position_table[i] = 255;
	}
}

static int32_t s5k3h2_test(enum s5k3h2_test_mode_t mo)
{
	int32_t rc = 0;
	if (mo == TEST_OFF)
		return rc;
	else {
		/* REG_0x30D8[4] is TESBYPEN: 0: Normal Operation,
		1: Bypass Signal Processing
		REG_0x30D8[5] is EBDMASK: 0:
		Output Embedded data, 1: No output embedded data */
		if (s5k3h2_i2c_write_b_sensor(REG_TEST_PATTERN_MODE,
			(uint8_t) mo) < 0) {
			return rc;
		}
	}
	return rc;
}

static int32_t s5k3h2_sensor_setting(int update_type, int rt)
{

	int32_t rc = 0;
	struct msm_camera_csid_params s5k3h2_csid_params;
	struct msm_camera_csiphy_params s5k3h2_csiphy_params;
	/*uint8_t stored_af_step = 0;*/
	printk("sensor_settings\n");
	/*stored_af_step = s5k3h2_ctrl->curr_step_pos;*/
	/*s5k3h2_set_default_focus(0);*/
	s5k3h2_stop_stream();
	msleep(150);
	if (update_type == REG_INIT) {
		s5k3h2_i2c_write_b_table(s5k3h2_regs.reg_mipi,
			s5k3h2_regs.reg_mipi_size);
		s5k3h2_i2c_write_b_table(s5k3h2_regs.rec_settings,
			s5k3h2_regs.rec_size);
		s5k3h2_i2c_write_b_table(s5k3h2_regs.reg_pll,
			s5k3h2_regs.reg_pll_size);
		cam_debug_init();
		CSI_CONFIG = 0;
	} else if (update_type == UPDATE_PERIODIC) {
		if (rt == RES_PREVIEW) {
			s5k3h2_i2c_write_b_table(s5k3h2_regs.reg_prev,
				s5k3h2_regs.reg_prev_size);
		} else {
				s5k3h2_i2c_write_b_table(s5k3h2_regs.reg_snap,
					s5k3h2_regs.reg_snap_size);
		}
		msleep(10);
		if (!CSI_CONFIG) {
			struct msm_camera_csid_vc_cfg s5k3h2_vccfg[] = {
				{0, CSI_RAW10, CSI_DECODE_10BIT},
				{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
			};
			s5k3h2_csid_params.lane_cnt = 2;
			s5k3h2_csid_params.lane_assign = 0xe4;
			s5k3h2_csid_params.lut_params.num_cid =
				ARRAY_SIZE(s5k3h2_vccfg);
			s5k3h2_csid_params.lut_params.vc_cfg =
				&s5k3h2_vccfg[0];
			s5k3h2_csiphy_params.lane_cnt = 2;
			s5k3h2_csiphy_params.settle_cnt = 0x1B;
			rc = msm_camio_csid_config(&s5k3h2_csid_params);
			v4l2_subdev_notify(s5k3h2_ctrl->sensor_dev,
						NOTIFY_CID_CHANGE, NULL);
			dsb();
			rc = msm_camio_csiphy_config(&s5k3h2_csiphy_params);
			dsb();
			CSI_CONFIG = 1;
		}
		/*s5k3h2_move_focus(MOVE_NEAR, stored_af_step);*/
		s5k3h2_start_stream();
	}
	return rc;
}

static int32_t s5k3h2_video_config(int mode)
{

	int32_t rc = 0;
	int rt;
	printk("video config\n");
	/* change sensor resolution if needed */
	if (s5k3h2_ctrl->prev_res == QTR_SIZE)
		rt = RES_PREVIEW;
	else
		rt = RES_CAPTURE;
	if (s5k3h2_sensor_setting(UPDATE_PERIODIC, rt) < 0)
		return rc;
	if (s5k3h2_ctrl->set_test) {
		if (s5k3h2_test(s5k3h2_ctrl->set_test) < 0)
			return  rc;
	}

	s5k3h2_ctrl->curr_res = s5k3h2_ctrl->prev_res;
	s5k3h2_ctrl->sensormode = mode;
	return rc;
}

static int32_t s5k3h2_snapshot_config(int mode)
{
	int32_t rc = 0;
	int rt;
	/*change sensor resolution if needed */
	if (s5k3h2_ctrl->curr_res != s5k3h2_ctrl->pict_res) {
		if (s5k3h2_ctrl->pict_res == QTR_SIZE)
			rt = RES_PREVIEW;
		else
			rt = RES_CAPTURE;
	if (s5k3h2_sensor_setting(UPDATE_PERIODIC, rt) < 0)
		return rc;
	}

	s5k3h2_ctrl->curr_res = s5k3h2_ctrl->pict_res;
	s5k3h2_ctrl->sensormode = mode;
	return rc;
} /*end of s5k3h2_snapshot_config*/

static int32_t s5k3h2_raw_snapshot_config(int mode)
{
	int32_t rc = 0;
	int rt;
	/* change sensor resolution if needed */
	if (s5k3h2_ctrl->curr_res != s5k3h2_ctrl->pict_res) {
		if (s5k3h2_ctrl->pict_res == QTR_SIZE)
			rt = RES_PREVIEW;
		else
			rt = RES_CAPTURE;
		if (s5k3h2_sensor_setting(UPDATE_PERIODIC, rt) < 0)
			return rc;
	}

	s5k3h2_ctrl->curr_res = s5k3h2_ctrl->pict_res;
	s5k3h2_ctrl->sensormode = mode;
	return rc;
} /*end of s5k3h2_raw_snapshot_config*/

static int32_t s5k3h2_set_sensor_mode(int mode,
	int res)
{
	int32_t rc = 0;
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		s5k3h2_ctrl->prev_res = res;
		rc = s5k3h2_video_config(mode);
		break;
	case SENSOR_SNAPSHOT_MODE:
		s5k3h2_ctrl->pict_res = res;
		rc = s5k3h2_snapshot_config(mode);
		break;
	case SENSOR_RAW_SNAPSHOT_MODE:
		s5k3h2_ctrl->pict_res = res;
		rc = s5k3h2_raw_snapshot_config(mode);
		break;
	default:
		rc = -EINVAL;
		break;
	}
	return rc;
}

static int32_t s5k3h2_power_down(void)
{
	return 0;
}

static int s5k3h2_probe_init_done(const struct msm_camera_sensor_info *data)
{
	printk("probe done\n");
	gpio_free(data->sensor_platform_info->sensor_reset);
	return 0;
}

static int s5k3h2_probe_init_sensor(const struct msm_camera_sensor_info *data)
{
	int32_t rc = 0;
	uint16_t chipid = 0;
	printk("%s: %d\n", __func__, __LINE__);
	
	rc = gpio_request(data->sensor_platform_info->sensor_reset, "s5k3h2");
	printk("%s: data->sensor_platform_info->sensor_reset = %d\n", __func__, data->sensor_platform_info->sensor_reset);
	printk(" s5k3h2_probe_init_sensor\n");
	if (!rc) {
		printk("sensor_reset = %d\n", rc);
		gpio_direction_output(data->sensor_platform_info->sensor_reset, 0);
		msleep(50);
		gpio_set_value_cansleep(data->sensor_platform_info->sensor_reset, 1);
		msleep(13);
	} else {
		goto init_probe_done;
	}

	msleep(20);
	rc = s5k3h2_i2c_read(0x0000, &chipid, 2);
	//rc = s5k3h2_i2c_read(0x300A, &chipid, 1);
	printk("ID: %d\n", chipid);
	/* 4. Compare sensor ID to S5K3H2 ID: */
	if (chipid != S5K3H2_MODEL_ID) {
		rc = -ENODEV;
		printk("s5k3h2_probe_init_sensor fail chip id doesnot match\n");
		goto init_probe_fail;
	}

	s5k3h2_ctrl->fps_divider = 1 * 0x00000400;
	s5k3h2_ctrl->pict_fps_divider = 1 * 0x00000400;
	s5k3h2_ctrl->set_test = TEST_OFF;
	s5k3h2_ctrl->prev_res = QTR_SIZE;
	s5k3h2_ctrl->pict_res = FULL_SIZE;

	if (data)
		s5k3h2_ctrl->sensordata = data;

	goto init_probe_done;
init_probe_fail:
	printk(" s5k3h2_probe_init_sensor fails\n");
	gpio_set_value_cansleep(data->sensor_platform_info->sensor_reset, 0);
	s5k3h2_probe_init_done(data);
init_probe_done:
	printk(" s5k3h2_probe_init_sensor finishes\n");
	return rc;
}
/* camsensor_s5k3h2_reset */

int s5k3h2_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int32_t rc = 0;

	printk("%s: %d\n", __func__, __LINE__);
	printk("Calling s5k3h2_sensor_open_init\n");

	s5k3h2_ctrl->fps_divider = 1 * 0x00000400;
	s5k3h2_ctrl->pict_fps_divider = 1 * 0x00000400;
	s5k3h2_ctrl->set_test = TEST_OFF;
	s5k3h2_ctrl->prev_res = QTR_SIZE;
	s5k3h2_ctrl->pict_res = FULL_SIZE;

	prev_frame_length_lines =
		(s5k3h2_regs.reg_prev[S5K3H2_FRAME_LENGTH_LINES_H].wdata << 8) |
		s5k3h2_regs.reg_prev[S5K3H2_FRAME_LENGTH_LINES_L].wdata;
	prev_line_length_pck =
		(s5k3h2_regs.reg_prev[S5K3H2_LINE_LENGTH_PCK_H].wdata << 8) |
		s5k3h2_regs.reg_prev[S5K3H2_LINE_LENGTH_PCK_L].wdata;
	snap_frame_length_lines =
		(s5k3h2_regs.reg_snap[S5K3H2_FRAME_LENGTH_LINES_H].wdata << 8) |
		s5k3h2_regs.reg_snap[S5K3H2_FRAME_LENGTH_LINES_L].wdata;
	snap_line_length_pck =
		(s5k3h2_regs.reg_snap[S5K3H2_LINE_LENGTH_PCK_H].wdata << 8) |
		s5k3h2_regs.reg_snap[S5K3H2_LINE_LENGTH_PCK_L].wdata;

	if (data)
		s5k3h2_ctrl->sensordata = data;
	if (rc < 0) {
		printk("Calling s5k3h2_sensor_open_init fail1\n");
		return rc;
	}
	printk("%s: %d\n", __func__, __LINE__);
	/* enable mclk first */
	msm_camio_clk_rate_set(S5K3H2_MASTER_CLK_RATE);
	rc = s5k3h2_probe_init_sensor(data);
	if (rc < 0)
		goto init_fail;

	printk("init settings\n");
	if (s5k3h2_ctrl->prev_res == QTR_SIZE)
		rc = s5k3h2_sensor_setting(REG_INIT, RES_PREVIEW);
	else
		rc = s5k3h2_sensor_setting(REG_INIT, RES_CAPTURE);
	s5k3h2_ctrl->fps = 30*Q8;
	s5k3h2_init_focus();
	if (rc < 0) {
		gpio_set_value_cansleep(data->sensor_platform_info->sensor_reset, 0);
		goto init_fail;
	} else
		goto init_done;
init_fail:
	printk("init_fail\n");
	s5k3h2_probe_init_done(data);
init_done:
	printk("init_done\n");
	return rc;
} /*endof s5k3h2_sensor_open_init*/

static int s5k3h2_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&s5k3h2_wait_queue);
	return 0;
}

static const struct i2c_device_id s5k3h2_i2c_id[] = {
	{"s5k3h2", 0},
	{ }
};

static int s5k3h2_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	printk("s5k3h2_probe called!\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk("i2c_check_functionality failed\n");
		goto probe_failure;
	}

	s5k3h2_sensorw = kzalloc(sizeof(struct s5k3h2_work_t), GFP_KERNEL);
	if (!s5k3h2_sensorw) {
		printk("kzalloc failed.\n");
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, s5k3h2_sensorw);
	s5k3h2_init_client(client);
	s5k3h2_client = client;

	msleep(50);

	printk("s5k3h2_probe successed! rc = %d\n", rc);
	return 0;

probe_failure:
	printk("s5k3h2_probe failed! rc = %d\n", rc);
	return rc;
}

static int s5k3h2_send_wb_info(struct wb_info_cfg *wb)
{
	return 0;

} /*end of s5k3h2_snapshot_config*/

static int __exit s5k3h2_remove(struct i2c_client *client)
{
	struct s5k3h2_work_t_t *sensorw = i2c_get_clientdata(client);
	free_irq(client->irq, sensorw);
	s5k3h2_client = NULL;
	kfree(sensorw);
	return 0;
}

static struct i2c_driver s5k3h2_i2c_driver = {
	.id_table = s5k3h2_i2c_id,
	.probe  = s5k3h2_i2c_probe,
	.remove = __exit_p(s5k3h2_i2c_remove),
	.driver = {
		.name = "s5k3h2",
	},
};

int s5k3h2_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cdata;
	long   rc = 0;
	if (copy_from_user(&cdata,
		(void *)argp,
		sizeof(struct sensor_cfg_data)))
		return -EFAULT;
	mutex_lock(&s5k3h2_mut);
	printk("s5k3h2_sensor_config: cfgtype = %d\n",
	cdata.cfgtype);
		switch (cdata.cfgtype) {
		case CFG_GET_PICT_FPS:
			s5k3h2_get_pict_fps(
				cdata.cfg.gfps.prevfps,
				&(cdata.cfg.gfps.pictfps));

			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;

		case CFG_GET_PREV_L_PF:
			cdata.cfg.prevl_pf =
			s5k3h2_get_prev_lines_pf();

			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;

		case CFG_GET_PREV_P_PL:
			cdata.cfg.prevp_pl =
				s5k3h2_get_prev_pixels_pl();

			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;

		case CFG_GET_PICT_L_PF:
			cdata.cfg.pictl_pf =
				s5k3h2_get_pict_lines_pf();

			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;

		case CFG_GET_PICT_P_PL:
			cdata.cfg.pictp_pl =
				s5k3h2_get_pict_pixels_pl();

			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;

		case CFG_GET_PICT_MAX_EXP_LC:
			cdata.cfg.pict_max_exp_lc =
				s5k3h2_get_pict_max_exp_lc();

			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;

		case CFG_SET_FPS:
		case CFG_SET_PICT_FPS:
			rc = s5k3h2_set_fps(&(cdata.cfg.fps));
			break;

		case CFG_SET_EXP_GAIN:
			rc =
				s5k3h2_write_exp_gain(
					cdata.cfg.exp_gain.gain,
					cdata.cfg.exp_gain.line);
			break;

		case CFG_SET_PICT_EXP_GAIN:
			rc =
				s5k3h2_set_pict_exp_gain(
				cdata.cfg.exp_gain.gain,
				cdata.cfg.exp_gain.line);
			break;

		case CFG_SET_MODE:
			rc = s5k3h2_set_sensor_mode(cdata.mode,
					cdata.rs);
			break;

		case CFG_PWR_DOWN:
			rc = s5k3h2_power_down();
			break;

		case CFG_MOVE_FOCUS:
			rc =
				s5k3h2_move_focus(
				cdata.cfg.focus.dir,
				cdata.cfg.focus.steps);
			break;

		case CFG_SET_DEFAULT_FOCUS:
			rc =
				s5k3h2_set_default_focus(
				cdata.cfg.focus.steps);
			break;

		case CFG_GET_AF_MAX_STEPS:
			cdata.max_steps = S5K3H2_TOTAL_STEPS_NEAR_TO_FAR;
			if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			break;

		case CFG_SET_EFFECT:
			rc = s5k3h2_set_default_focus(
				cdata.cfg.effect);
			break;


		case CFG_SEND_WB_INFO:
			rc = s5k3h2_send_wb_info(
				&(cdata.cfg.wb_info));
			break;

		default:
			rc = -EFAULT;
			break;
		}

	mutex_unlock(&s5k3h2_mut);

	return rc;
}

static int s5k3h2_sensor_release(void)
{
	int rc = -EBADF;
	mutex_lock(&s5k3h2_mut);
	s5k3h2_power_down();
	gpio_set_value_cansleep(s5k3h2_ctrl->sensordata->sensor_platform_info->sensor_reset, 0);
	msleep(5);
	gpio_free(s5k3h2_ctrl->sensordata->sensor_platform_info->sensor_reset);
	printk("s5k3h2_release completed\n");
	mutex_unlock(&s5k3h2_mut);

	return rc;
}

static int s5k3h2_sensor_probe(const struct msm_camera_sensor_info *info,
		struct msm_sensor_ctrl *s)
{
	int rc = 0;
	rc = i2c_add_driver(&s5k3h2_i2c_driver);
	if (rc < 0 || s5k3h2_client == NULL) {
		rc = -ENOTSUPP;
		printk("I2C add driver failed");
		goto probe_fail;
	}
	msm_camio_clk_rate_set(S5K3H2_MASTER_CLK_RATE);
	rc = s5k3h2_probe_init_sensor(info);
	if (rc < 0)
		goto probe_fail;
	s->s_init = s5k3h2_sensor_open_init;
	s->s_release = s5k3h2_sensor_release;
	s->s_config  = s5k3h2_sensor_config;
	s->s_mount_angle = 90;
	gpio_set_value_cansleep(info->sensor_reset, 0);
	s5k3h2_probe_init_done(info);
	return rc;

probe_fail:
	printk("s5k3h2_sensor_probe: SENSOR PROBE FAILS!\n");
	return rc;
}

static int s5k3h2_enum_fmt(struct v4l2_subdev *sd, unsigned int index,
			   enum v4l2_mbus_pixelcode *code)
{
	printk(KERN_DEBUG "Index is %d\n", index);
	if ((unsigned int)index >= ARRAY_SIZE(s5k3h2_subdev_info))
		return -EINVAL;

	*code = s5k3h2_subdev_info[index].code;
	return 0;
}

static struct v4l2_subdev_core_ops s5k3h2_subdev_core_ops;
static struct v4l2_subdev_video_ops s5k3h2_subdev_video_ops = {
	.enum_mbus_fmt = s5k3h2_enum_fmt,
};

static struct v4l2_subdev_ops s5k3h2_subdev_ops = {
	.core = &s5k3h2_subdev_core_ops,
	.video  = &s5k3h2_subdev_video_ops,
};


static int s5k3h2_sensor_probe_cb(const struct msm_camera_sensor_info *info,
	struct v4l2_subdev *sdev, struct msm_sensor_ctrl *s)
{
	int rc = 0;
	s5k3h2_ctrl = kzalloc(sizeof(struct s5k3h2_ctrl_t), GFP_KERNEL);
	if (!s5k3h2_ctrl) {
		printk("s5k3h2_sensor_probe failed!\n");
		return -ENOMEM;
	}

	rc = s5k3h2_sensor_probe(info, s);
	if (rc < 0)
		return rc;

	/* probe is successful, init a v4l2 subdevice */
	printk(KERN_DEBUG "going into v4l2_i2c_subdev_init\n");
	if (sdev) {
		v4l2_i2c_subdev_init(sdev, s5k3h2_client,
						&s5k3h2_subdev_ops);
		s5k3h2_ctrl->sensor_dev = sdev;
	}
	return rc;
}

static int __s5k3h2_probe(struct platform_device *pdev)
{
	return msm_sensor_register(pdev, s5k3h2_sensor_probe_cb);
}

static struct platform_driver msm_camera_driver = {
	.probe = __s5k3h2_probe,
	.driver = {
		.name = "msm_camera_s5k3h2",
		.owner = THIS_MODULE,
	},
};

static int __init s5k3h2_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(s5k3h2_init);

void s5k3h2_exit(void)
{
	i2c_del_driver(&s5k3h2_i2c_driver);
}
module_exit(s5k3h2_exit);

MODULE_DESCRIPTION("SLSI 8 MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");

static bool streaming = 1;

static int s5k3h2_focus_test(void *data, u64 *val)
{
	int i = 0;
	s5k3h2_set_default_focus(0);

	for (i = 90; i < 256; i++) {
		s5k3h2_i2c_write_w_sensor(REG_VCM_NEW_CODE, i);
		msleep(5000);
	}
	msleep(5000);
	for (i = 255; i > 90; i--) {
		s5k3h2_i2c_write_w_sensor(REG_VCM_NEW_CODE, i);
		msleep(5000);
	}
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(cam_focus, s5k3h2_focus_test,
			NULL, "%lld\n");

static int s5k3h2_step_test(void *data, u64 *val)
{
	int i = 0;
	s5k3h2_set_default_focus(0);

	for (i = 0; i < S5K3H2_TOTAL_STEPS_NEAR_TO_FAR; i++) {
		s5k3h2_move_focus(MOVE_NEAR, 1);
		msleep(5000);
	}

	s5k3h2_move_focus(MOVE_FAR, S5K3H2_TOTAL_STEPS_NEAR_TO_FAR);
	msleep(5000);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(cam_step, s5k3h2_step_test,
			NULL, "%lld\n");

static int cam_debug_stream_set(void *data, u64 val)
{
	int rc = 0;

	if (val) {
		s5k3h2_start_stream();
		streaming = 1;
	} else {
		s5k3h2_stop_stream();
		streaming = 0;
	}

	return rc;
}

static int cam_debug_stream_get(void *data, u64 *val)
{
	*val = streaming;
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(cam_stream, cam_debug_stream_get,
			cam_debug_stream_set, "%llu\n");


static int cam_debug_init(void)
{
	struct dentry *cam_dir;
	debugfs_base = debugfs_create_dir("sensor", NULL);
	if (!debugfs_base)
		return -ENOMEM;

	cam_dir = debugfs_create_dir("s5k3h2", debugfs_base);
	if (!cam_dir)
		return -ENOMEM;

	if (!debugfs_create_file("focus", S_IRUGO | S_IWUSR, cam_dir,
							 NULL, &cam_focus))
		return -ENOMEM;
	if (!debugfs_create_file("step", S_IRUGO | S_IWUSR, cam_dir,
							 NULL, &cam_step))
		return -ENOMEM;
	if (!debugfs_create_file("stream", S_IRUGO | S_IWUSR, cam_dir,
							 NULL, &cam_stream))
		return -ENOMEM;

	return 0;
}



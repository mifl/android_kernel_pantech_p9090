/* Copyright (c) 2011, PANTECH. All rights reserved.
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

#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <media/msm_camera.h>
#include <mach/camera.h>
#include "sensor_i2c.h"
#include "sensor_ctrl.h"
#include "s5k6aafx13_r2_v4l2.h"
#include <media/v4l2-subdev.h>
#include "msm.h"
//#ifdef CONFIG_PANTECH_CAMERA_TUNER
//#include "ptune_parser.h"
//#endif

#undef F_PANTECH_CAMERA_TUP_LOAD_FILE

/* PANTECH_CAMERA_TODO */
/*
#undef CDBG
#undef SKYCERR
#define CDBG(fmt, args...) printk(KERN_ERR fmt, ##args)
#define SKYCERR(fmt, args...) printk(KERN_ERR fmt, ##args)
*/

#define VERSION_CHECK

/* I2C slave address */
#define SI2C_SA	(s5k6aafx13_client->addr)

#define s5k6aafx13_delay_msecs_stream 500//100

typedef struct {
	struct work_struct work;
} s5k6aafx13_work_t;

struct s5k6aafx13_ctrl_t{
	const struct msm_camera_sensor_info *sinfo;
	uint32_t sensormode;
	uint32_t fps_divider;/* init to 1 * 0x00000400 */
	uint32_t pict_fps_divider;/* init to 1 * 0x00000400 */
	uint16_t fps;
	int16_t curr_lens_pos;
	uint16_t curr_step_pos;
	uint16_t my_reg_gain;
	uint32_t my_reg_line_count;
	uint16_t total_lines_per_frame;
/*	
	enum s5k6aafx13_resolution_t prev_res;
	enum s5k6aafx13_resolution_t pict_res;
	enum s5k6aafx13_resolution_t curr_res;
	enum s5k6aafx13_test_mode_t set_test;
*/	
	unsigned short imgaddr;

	struct v4l2_subdev *sensor_dev;
	struct s5k6aafx13_format *fmt;
};


#if defined(CONFIG_MACH_MSM8X60_EF39S) || \
   (defined(CONFIG_MACH_MSM8X60_EF40S) && (BOARD_REV == WS10)) || \
   (defined(CONFIG_MACH_MSM8X60_EF40K) && (BOARD_REV == WS10)) || \
   (defined(CONFIG_PANTECH_CAMERA_S5K6AAFX13))

/* PANTECH_CAMERA_TODO, needs standby control */

static DECLARE_WAIT_QUEUE_HEAD(s5k6aafx13_wait_queue);

#define CAMIO_RST_N	0
#define CAMIO_STB_N	1
#define CAMIO_MAX	2

static sgpio_ctrl_t sgpios[CAMIO_MAX] = {
	{CAMIO_RST_N, "CAM_RST_N", 76},//137},
	{CAMIO_STB_N, "CAM_STB_N", 52},//139},
};
#ifdef CONFIG_MACH_MSM8960_CHEETAH
#define CAMV_IO_1P8V	0
#define CAMV_CORE_1P5V	1
#define CAMV_A_2P8V	2
#define CAMV_I2C_1P8V	3
#define CAMV_MAX	4
#else
#define CAMV_IO_1P8V	0
#define CAMV_CORE_1P5V	1
#define CAMV_A_2P8V	2
#ifdef IO_POWER_TEST
#define CAMV_I2C_1P8V	3
#define CAMV_MAX	4
#else
#define CAMV_MAX	3
#endif
#endif

static svreg_ctrl_t svregs[CAMV_MAX] = {
#ifdef CONFIG_MACH_MSM8960_CHEETAH
	{CAMV_IO_1P8V,   "8921_lvs5", NULL, 0},
	{CAMV_CORE_1P5V, "8921_l18",   NULL, 1500},
	{CAMV_A_2P8V,    "8921_l11",  NULL, 2800},
	{CAMV_I2C_1P8V,  "8921_lvs6",  NULL, 0},
#else
#ifdef IO_POWER_TEST
	{CAMV_IO_1P8V,   "8921_l29", NULL, 2200},
#else	
	{CAMV_IO_1P8V,   "8921_lvs5", NULL, 0},
#endif	
	{CAMV_CORE_1P5V, "8921_l12",   NULL, 1500},
	{CAMV_A_2P8V,    "8921_l11",  NULL, 2800},
#ifdef IO_POWER_TEST	
	{CAMV_I2C_1P8V,   "8921_lvs5", NULL, 0},
#endif	
#endif	
};

#else
#error "unknown machine!"
#endif


extern si2c_const_param_t s5k6aafx13_const_params[SI2C_PID_MAX];
static si2c_param_t s5k6aafx13_params[SI2C_PID_MAX];
#ifdef CONFIG_PANTECH_CAMERA_TUNER
static si2c_param_t s5k6aafx13_tuner_params[SI2C_PID_MAX];
#endif

static struct s5k6aafx13_ctrl_t *s5k6aafx13_ctrl = NULL;
static struct i2c_client *s5k6aafx13_client = NULL;
static s5k6aafx13_work_t *s5k6aafx13_work = NULL;
DEFINE_MUTEX(s5k6aafx13_mutex);

struct s5k6aafx13_format {
	enum v4l2_mbus_pixelcode code;
	enum v4l2_colorspace colorspace;
	u16 fmt;
	u16 order;
};

static int sensor_mode = SENSOR_PREVIEW_MODE;
static bool config_csi_done = false;


static int s5k6aafx13_power_on(void)
{
	int rc = 0;

	//SKYCDBG("%s\n", __func__);

	if (sgpio_ctrl(sgpios, CAMIO_RST_N, 0) < 0)	rc = -EIO;

	if (svreg_ctrl(svregs, CAMV_A_2P8V, 1) < 0)	rc = -EIO;
	mdelay(1); /* > 20us */
	if (svreg_ctrl(svregs, CAMV_CORE_1P5V, 1) < 0)	rc = -EIO;
	mdelay(1); /* > 15us */
	if (svreg_ctrl(svregs, CAMV_IO_1P8V, 1) < 0)	rc = -EIO;
	mdelay(1);
#ifdef CONFIG_MACH_MSM8960_CHEETAH//IO_POWER_TEST
	if (svreg_ctrl(svregs, CAMV_I2C_1P8V, 1) < 0)	rc = -EIO;
	mdelay(1);
#endif
	if (sgpio_ctrl(sgpios, CAMIO_STB_N, 1) < 0)	rc = -EIO;
	//(void)msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
	//msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);
	msm_camio_clk_rate_set(24000000);
	msleep(10); /* > 50us */
	if (sgpio_ctrl(sgpios, CAMIO_RST_N, 1) < 0)	rc = -EIO;
	mdelay(1); /* > 50us */

	//SKYCDBG("%s X (%d)\n", __func__, rc);
	return rc;
}


static int s5k6aafx13_power_off(void)
{
	int rc = 0;

	//SKYCDBG("%s E\n", __func__);

	if (sgpio_ctrl(sgpios, CAMIO_RST_N, 0) < 0)	rc = -EIO;
	mdelay(1); /* > 20 cycles (approx. 0.64us) */
	if (sgpio_ctrl(sgpios, CAMIO_STB_N, 0) < 0)	rc = -EIO;
	/* MCLK will be disabled once again after this. */
	//(void)msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);

#ifdef CONFIG_MACH_MSM8960_CHEETAH//IO_POWER_TEST
	if (svreg_ctrl(svregs, CAMV_I2C_1P8V, 0) < 0)	rc = -EIO;
#endif
	if (svreg_ctrl(svregs, CAMV_IO_1P8V, 0) < 0)	rc = -EIO;
	if (svreg_ctrl(svregs, CAMV_CORE_1P5V, 0) < 0)	rc = -EIO;
	if (svreg_ctrl(svregs, CAMV_A_2P8V, 0) < 0)	rc = -EIO;

	//SKYCDBG("%s X (%d)\n", __func__, rc);
	return rc;
}


static int s5k6aafx13_config_csi(void)
{
#if 0
	struct msm_camera_csi_params *csi = NULL;
	int rc = 0;

	//SKYCDBG("%s E\n", __func__);

	csi = kmalloc(sizeof(struct msm_camera_csi_params), GFP_KERNEL);
	if (!csi) {
		SKYCERR("%s err(-ENOMEM)", __func__);
		return -ENOMEM;
	}

	csi->data_format = CSI_8BIT;
	csi->lane_cnt    = 1;
	csi->lane_assign = 0xe4;
	csi->settle_cnt  = 0x14;
	csi->dpcm_scheme = 0;

	rc = msm_camio_csi_config(csi);
	if (rc < 0) {
		kfree(csi);
		SKYCERR("%s err(%d)\n", __func__, rc);
		return rc;
	}

	msleep(10);

	config_csi_done = true;
	kfree(csi);
#endif
	// int rc = 0; // 20111118 jylee for 102150 compile error
	struct msm_camera_csid_params s5k6aafx13_csid_params;
	struct msm_camera_csiphy_params s5k6aafx13_csiphy_params;
	
	struct msm_camera_csid_vc_cfg s5k6aafx13_vccfg[] = {
		//{0, CSI_RAW8, CSI_DECODE_8BIT},
		{0, 0x1E, CSI_DECODE_8BIT}, 
		{1, CSI_EMBED_DATA, CSI_DECODE_8BIT},
	};
	//SKYCDBG("%s E\n", __func__);
	s5k6aafx13_csid_params.lane_cnt = 1;
	s5k6aafx13_csid_params.lane_assign = 0xe4;
	s5k6aafx13_csid_params.lut_params.num_cid =
		ARRAY_SIZE(s5k6aafx13_vccfg);
	s5k6aafx13_csid_params.lut_params.vc_cfg =
		&s5k6aafx13_vccfg[0];
	s5k6aafx13_csiphy_params.lane_cnt = 1;
	s5k6aafx13_csiphy_params.settle_cnt = 0x1B;//0x1E;//0x30;//0x27;//
	//rc = msm_camio_csid_config(&s5k6aafx13_csid_params);  // 20111118 jylee for 102150 compile error
	v4l2_subdev_notify(s5k6aafx13_ctrl->sensor_dev,
						NOTIFY_CID_CHANGE, NULL);
	mb();
	//rc = msm_camio_csiphy_config
	//	(&s5k6aafx13_csiphy_params); // 20111118 jylee for 102150 compile error
	mb();
	/*s5k6aafx13_delay_msecs_stdby*/
	//msleep(s5k6aafx13_delay_msecs_stream);
	mdelay(s5k6aafx13_delay_msecs_stream);

	config_csi_done = true;
	

	//SKYCDBG("%s X\n", __func__);
	return 0;
}


static int s5k6aafx13_video_config(void)
{
	int rc = 0;

	//SKYCDBG("%s E\n", __func__);

//#ifdef CONFIG_PANTECH_CAMERA_TUNER //F_PANTECH_CAMERA_TUP_LOAD_FILE
//열려있으면 init 을 write 하지 않고, video_config 에서 write 하도록 수정
#if 0
	rc = si2c_write_param(SI2C_SA, SI2C_INIT, s5k6aafx13_params);
	if (rc < 0) {
		SKYCERR("%s init err(%d)\n", __func__, rc);
		return rc;
	}
#endif

	rc = si2c_write_param(SI2C_SA, SI2C_PREVIEW, s5k6aafx13_params);
	if (rc < 0) {
		//SKYCERR("%s video config err(%d)\n", __func__, rc);
		return rc;
	}

	sensor_mode = SENSOR_PREVIEW_MODE;

	//SKYCDBG("%s X\n", __func__);
	return 0;
}


static int s5k6aafx13_snapshot_config(void)
{
	int rc = 0;

	//SKYCDBG("%s E\n", __func__);

	rc = si2c_write_param(SI2C_SA, SI2C_SNAPSHOT, s5k6aafx13_params);
	if (rc < 0) {
		//SKYCERR("%s err(%d)\n", __func__, rc);
		return rc;
	}

	sensor_mode = SENSOR_SNAPSHOT_MODE;

	//SKYCDBG("%s X\n", __func__);
	return 0;
}


static int s5k6aafx13_set_brightness(int brightness)
{
	si2c_pid_t pid = SI2C_PID_MAX;
	int rc = 0;

	//SKYCDBG("%s brightness=%d\n", __func__, brightness);

	switch (brightness) {
	case 0: pid = SI2C_BRIGHTNESS_M4; break;
	case 1: pid = SI2C_BRIGHTNESS_M3; break;
	case 2: pid = SI2C_BRIGHTNESS_M2; break;
	case 3: pid = SI2C_BRIGHTNESS_M1; break;
	case 4: pid = SI2C_BRIGHTNESS_0; break;
	case 5: pid = SI2C_BRIGHTNESS_P1; break;
	case 6: pid = SI2C_BRIGHTNESS_P2; break;
	case 7: pid = SI2C_BRIGHTNESS_P3; break;
	case 8: pid = SI2C_BRIGHTNESS_P4; break;
	default:
		//SKYCERR("%s err(-EINVAL)\n", __func__);
		return -EINVAL;
	}

	rc = si2c_write_param(SI2C_SA, pid, s5k6aafx13_params);
	if (rc < 0) {
		//SKYCERR("%s err(%d)\n", __func__, rc);
		return rc;
	}

	//SKYCDBG("%s X\n", __func__);
	return 0;
}


static int s5k6aafx13_set_effect(int effect)
{
	si2c_pid_t pid = SI2C_PID_MAX;
	int rc = 0;

	//SKYCDBG("%s effect=%d\n", __func__, effect);

	switch (effect) {
	case CAMERA_EFFECT_OFF: pid = SI2C_EFFECT_OFF; break;
	case CAMERA_EFFECT_MONO: pid = SI2C_EFFECT_MONO; break;
	case CAMERA_EFFECT_NEGATIVE: pid = SI2C_EFFECT_NEGATIVE; break;
	case CAMERA_EFFECT_SEPIA: pid = SI2C_EFFECT_SEPIA; break;
	default:
		//SKYCERR("%s err(-EINVAL)\n", __func__);
		return -EINVAL;
	}

	rc = si2c_write_param(SI2C_SA, pid, s5k6aafx13_params);
	if (rc < 0) {
		//SKYCERR("%s err(%d)\n", __func__, rc);
		return rc;
	}

	//SKYCDBG("%s X\n", __func__);
	return 0;
}


static int s5k6aafx13_set_exposure(int exposure)
{
	si2c_pid_t pid = SI2C_PID_MAX;
	int rc = 0;

	//SKYCDBG("%s exposure=%d\n", __func__, exposure);

	switch (exposure) {
	case 1: pid = SI2C_EXPOSURE_AVERAGE; break;
	case 2: 
	case 3: pid = SI2C_EXPOSURE_CENTER; break;
	default:
		//SKYCERR("%s err(-EINVAL)\n", __func__);
		return -EINVAL;
	}

	rc = si2c_write_param(SI2C_SA, pid, s5k6aafx13_params);
	if (rc < 0) {
		//SKYCERR("%s err(%d)\n", __func__, rc);
		return rc;
	}

	//SKYCDBG("%s X\n", __func__);
	return 0;
}


static int s5k6aafx13_set_mode(int mode)
{
	int rc = 0;

	//SKYCDBG("%s mode=%d\n", __func__, mode);

	if (!config_csi_done) {
#ifdef CONFIG_PANTECH_CAMERA_TUNER
		rc = si2c_write_param(SI2C_SA, SI2C_INIT, s5k6aafx13_params);
		if (rc < 0) {
			CERR("%s init err(%d)\n", __func__, rc);
			return rc;
		}
#endif
		rc = s5k6aafx13_config_csi();
		if (rc < 0) {
			//SKYCERR("%s err(-EIO)\n", __func__);
			return -EIO;
		}
	}

	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		if (sensor_mode != SENSOR_PREVIEW_MODE)
			rc = s5k6aafx13_video_config();
		break;
		
	case SENSOR_SNAPSHOT_MODE:
		if (sensor_mode != SENSOR_SNAPSHOT_MODE)
			rc = s5k6aafx13_snapshot_config();
		break;

	default:
		rc = -EINVAL;
		break;
	}

	//SKYCDBG("%s X (%d)\n", __func__, rc);
	return rc;
}


static int s5k6aafx13_set_preview_fps(int preview_fps)
{
	si2c_pid_t pid = SI2C_PID_MAX;
	int rc = 0;

	//SKYCDBG("%s preview_fps=%d\n", __func__, preview_fps);

	switch (preview_fps) {
	case  0: pid = SI2C_FPS_VARIABLE; break;
	case  7: pid = SI2C_FPS_FIXED7; break;
	case  8: pid = SI2C_FPS_FIXED8; break;
	case 10: pid = SI2C_FPS_FIXED10; break;
	case 15: pid = SI2C_FPS_FIXED15; break;
	case 20: pid = SI2C_FPS_FIXED20; break;
	case 24: pid = SI2C_FPS_FIXED24; break;
	default:
		//PANTECH_CAMERA_TODO
		return 0;
		//SKYCERR("%s err(-EINVAL)\n", __func__);
		//return -EINVAL;
	}

	rc = si2c_write_param(SI2C_SA, pid, s5k6aafx13_params);
	if (rc < 0) {
		//SKYCERR("%s err(%d)\n", __func__, rc);
		return rc;
	}

	//SKYCDBG("%s X\n", __func__);
	return 0;
}


static int s5k6aafx13_set_reflect(int reflect)
{
	si2c_pid_t pid = SI2C_PID_MAX;
	int rc = 0;

	//SKYCDBG("%s reflect=%d\n", __func__, reflect);


	switch (reflect) {
	case 0: pid = SI2C_REFLECT_OFF; break;
	case 1: pid = SI2C_REFLECT_MIRROR; break;
	case 2: pid = SI2C_REFLECT_WATER; break;
	case 3: pid = SI2C_REFLECT_MIRROR_WATER; break;
	default:
		//SKYCERR("%s err(-EINVAL)\n", __func__);
		return -EINVAL;
	}

	rc = si2c_write_param(SI2C_SA, pid, s5k6aafx13_params);
	if (rc < 0) {
		//SKYCERR("%s err(%d)\n", __func__, rc);
		return rc;
	}

	//SKYCDBG("%s X\n", __func__);

	return 0;
}


#ifdef CONFIG_PANTECH_CAMERA_TUNER
static int s5k6aafx13_set_tuner(struct tuner_cfg tuner)
{
	si2c_cmd_t *cmds = NULL;
	char *fbuf = NULL;

	//SKYCDBG("%s fbuf=%p, fsize=%d\n", __func__, tuner.fbuf, tuner.fsize);

	if (!tuner.fbuf || (tuner.fsize == 0)) {
		//SKYCERR("%s err(-EINVAL)\n", __func__);
		return -EINVAL;
	}

	fbuf = (char *)kmalloc(tuner.fsize, GFP_KERNEL);
	if (!fbuf) {
		//SKYCERR("%s err(-ENOMEM)\n", __func__);
		return -ENOMEM;
	}

	if (copy_from_user(fbuf, tuner.fbuf, tuner.fsize)) {
		//SKYCERR("%s err(-EFAULT)\n", __func__);
		kfree(fbuf);
		return -EFAULT;
	}

	cmds = ptune_parse("@init", fbuf);
	if (!cmds) {
		//SKYCERR("%s no @init\n", __func__);
		kfree(fbuf);
		return -EFAULT;
	}

	s5k6aafx13_tuner_params[SI2C_INIT].cmds = cmds;
	s5k6aafx13_params[SI2C_INIT].cmds = cmds;

	kfree(fbuf);

	//SKYCDBG("%s X\n", __func__);
	return 0;
}
#endif


static int32_t s5k6aafx13_set_wb(int wb)
{
	si2c_pid_t pid = SI2C_PID_MAX;
	int rc = 0;

	//SKYCDBG("%s wb=%d\n", __func__, wb);

	switch (wb) {
	case 1: pid = SI2C_WB_AUTO; break;
	case 3: pid = SI2C_WB_INCANDESCENT; break;
	case 4: pid = SI2C_WB_FLUORESCENT; break;
	case 5: pid = SI2C_WB_DAYLIGHT; break;
	case 6: pid = SI2C_WB_CLOUDY; break;
	default:
		//SKYCERR("%s err(-EINVAL)\n", __func__);
		return -EINVAL;
	}

	rc = si2c_write_param(SI2C_SA, pid, s5k6aafx13_params);
	if (rc < 0) {
		//SKYCERR("%s err(%d)\n", __func__, rc);
		return rc;
	}

	//SKYCDBG("%s X\n", __func__);
	return 0;
}


int s5k6aafx13_sensor_init(const struct msm_camera_sensor_info *sinfo)
{
	int rc = 0;

	//SKYCDBG("%s E\n", __func__);

	config_csi_done = false;
	sensor_mode = SENSOR_PREVIEW_MODE;

	memset(s5k6aafx13_params, 0, sizeof(s5k6aafx13_params));
#ifdef CONFIG_PANTECH_CAMERA_TUNER
	memset(s5k6aafx13_tuner_params, 0, sizeof(s5k6aafx13_tuner_params));
#endif

	//PANTECH_CAMERA_TODO, adapter 설정과 param 설정을 분리할 것. ugly 하다...
	rc = si2c_init(s5k6aafx13_client->adapter, 
			s5k6aafx13_const_params, s5k6aafx13_params);
	if (rc < 0)
		goto sensor_init_fail;
/*
	s5k6aafx13_ctrl = kzalloc(sizeof(struct s5k6aafx13_ctrl_t), GFP_KERNEL);
	if (!s5k6aafx13_ctrl) {
		//SKYCERR("%s err(-ENOMEM)\n", __func__);
		goto sensor_init_fail;
	}
*/
	if (sinfo)
		s5k6aafx13_ctrl->sinfo = sinfo;

	rc = sgpio_init(sgpios, CAMIO_MAX);
	if (rc < 0)
		goto sensor_init_fail;

	rc = svreg_init(svregs, CAMV_MAX);
	if (rc < 0)
		goto sensor_init_fail;

	rc = s5k6aafx13_power_on();
	if (rc < 0)
		goto sensor_init_fail;

#ifdef CONFIG_PANTECH_CAMERA_TUNER
	//SKYCERR("%s tuner is enabled, skip writing INIT param!\n", __func__);
#else
	rc = si2c_write_param(SI2C_SA, SI2C_INIT, s5k6aafx13_params);
	if (rc < 0)
		goto sensor_init_fail;
#endif

	//SKYCDBG("%s X\n", __func__);
	return 0;

sensor_init_fail:
#if 1
	if (s5k6aafx13_ctrl) {
		kfree(s5k6aafx13_ctrl);
		s5k6aafx13_ctrl = NULL;
	}
	(void)s5k6aafx13_power_off();
	svreg_release(svregs, CAMV_MAX);
	sgpio_release(sgpios, CAMIO_MAX);
	si2c_release();
#endif
	//SKYCERR("%s err(%d)\n", __func__, rc);
	return rc;
}

static int s5k6aafx13_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&s5k6aafx13_wait_queue);
	return 0;
}

int s5k6aafx13_sensor_config(void __user *argp)
{
	struct sensor_cfg_data sc;
	int rc = 0;

	rc = copy_from_user(&sc, argp, sizeof(struct sensor_cfg_data));
	if (rc) {
		//SKYCERR("%s err(%d)\n", __func__, rc);
		return rc;
	}

	//SKYCDBG("%s type=%d, mode=%d\n", __func__, sc.cfgtype, sc.mode);

	mutex_lock(&s5k6aafx13_mutex);

	switch (sc.cfgtype) {
	case CFG_SET_BRIGHTNESS:
		rc = s5k6aafx13_set_brightness(sc.cfg.brightness);
		break;
	case CFG_SET_EFFECT:
		rc = s5k6aafx13_set_effect(sc.cfg.effect);
		break;
	//PANTECH_CAMERA_TODO, EXPOSURE_MODE -> EXPOSURE
	case CFG_SET_EXPOSURE_MODE:
		rc = s5k6aafx13_set_exposure(sc.cfg.exposure);
		break;
	case CFG_SET_MODE:
		rc = s5k6aafx13_set_mode(sc.mode);
		break;
	case CFG_SET_PREVIEW_FPS:
		rc = s5k6aafx13_set_preview_fps(sc.cfg.preview_fps);
		break;
	case CFG_SET_REFLECT:
		rc = s5k6aafx13_set_reflect(sc.cfg.reflect);
		break;
#ifdef CONFIG_PANTECH_CAMERA_TUNER
	case CFG_SET_TUNER:
		rc = s5k6aafx13_set_tuner(sc.cfg.tuner);
		break;
#endif
	case CFG_SET_WB:
		//PANTECH_CAMERA_TODO, whitebalance -> wb
		rc = s5k6aafx13_set_wb(sc.cfg.whitebalance);
		break;
	default:
		//SKYCDBG("%s err(-EINVAL)\n", __func__);
		rc = -EINVAL;
		break;
	}

	mutex_unlock(&s5k6aafx13_mutex);

	/*SKYCDBG("%s X (%d)\n", __func__, rc);*/
	return rc;
}


int s5k6aafx13_sensor_release(void)
{
#ifdef CONFIG_PANTECH_CAMERA_TUNER
	si2c_pid_t i = 0;
#endif

	//SKYCDBG("%s E\n", __func__);

	mutex_lock(&s5k6aafx13_mutex);
#if 0
	if (s5k6aafx13_ctrl) {
		kfree(s5k6aafx13_ctrl);
		s5k6aafx13_ctrl = NULL;
	}
#endif
	(void)s5k6aafx13_power_off();

	svreg_release(svregs, CAMV_MAX);
	sgpio_release(sgpios, CAMIO_MAX);

#ifdef CONFIG_PANTECH_CAMERA_TUNER
	for (i = 0; i < SI2C_PID_MAX; i++) {
		if (s5k6aafx13_tuner_params[i].cmds)
			kfree(s5k6aafx13_tuner_params[i].cmds);
	}
#endif

	si2c_release();

	mutex_unlock(&s5k6aafx13_mutex);

	//SKYCDBG("%s X\n", __func__);
	return 0;
}


static int s5k6aafx13_i2c_probe(
	struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	//SKYCDBG("%s E\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		//SKYCERR("%s (-ENOTSUPP)\n", __func__);
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	s5k6aafx13_work = kzalloc(sizeof(s5k6aafx13_work_t), GFP_KERNEL);
	if (!s5k6aafx13_work) {
		//SKYCERR("%s (-ENOMEM)\n", __func__);
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, s5k6aafx13_work);
	s5k6aafx13_init_client(client);
	s5k6aafx13_client = client;

	//SKYCDBG("%s X\n", __func__);
	return 0;
probe_failure:
	kfree(s5k6aafx13_work);
	s5k6aafx13_work = NULL;
	//SKYCERR("s5k6aafx13_probe failed!\n");
	return rc;
}


static const struct i2c_device_id s5k6aafx13_i2c_id[] = {
	{ "s5k6aafx13", 0},
	{ },
};


static struct i2c_driver s5k6aafx13_i2c_driver = {
	.id_table = s5k6aafx13_i2c_id,
	.probe  = s5k6aafx13_i2c_probe,
	.remove = __exit_p(s5k6aafx13_i2c_remove),
	.driver = {
		.name = "s5k6aafx13",
	},
};

static int32_t s5k6aafx13_init_i2c(void)
{
	int32_t rc = 0;

	//SKYCDBG("%s start\n",__func__);

	rc = i2c_add_driver(&s5k6aafx13_i2c_driver);
	//SKYCDBG("%s s5k6aafx13_i2c_driver rc = %d\n",__func__, rc);
	if (IS_ERR_VALUE(rc))
		goto init_i2c_fail;

	//SKYCDBG("%s end\n",__func__);
	return 0;

init_i2c_fail:
	//SKYCERR("%s failed! (%d)\n", __func__, rc);
	return rc;
}
static int s5k6aafx13_sensor_probe(
	const struct msm_camera_sensor_info *sinfo,
	struct msm_sensor_ctrl *sctrl)
{
	int rc = 0;
#ifdef VERSION_CHECK	
	unsigned short rd = 100;
#endif

	//SKYCDBG("%s E\n", __func__);

	rc = s5k6aafx13_init_i2c();	
	if (rc < 0 || s5k6aafx13_client == NULL)
	{
		//SKYCERR("%s rc = %d, s5k6aafx13_client = %x\n",__func__, rc, (unsigned int)s5k6aafx13_client);
		return -ENOTSUPP;
	}
	/*
	rc = i2c_add_driver(&s5k6aafx13_i2c_driver);
	if (rc < 0 || s5k6aafx13_client == NULL) {
		//SKYCERR("%s err(-ENOTSUPP)\n", __func__);
		return -ENOTSUPP;
	}
	*/
#ifdef VERSION_CHECK	
	rc = si2c_init(s5k6aafx13_client->adapter, 
			s5k6aafx13_const_params, s5k6aafx13_params);
	if (rc < 0)
		//SKYCERR("%s si2c_init fail\n", __func__);
	rc = sgpio_init(sgpios, CAMIO_MAX);
	if (rc < 0)
		//SKYCERR("%s 1\n", __func__);

	rc = svreg_init(svregs, CAMV_MAX);
	if (rc < 0)
		//SKYCERR("%s 2\n", __func__);

	rc = s5k6aafx13_power_on();
	if (rc < 0)
		//SKYCERR("%s 3\n", __func__);

	msm_camio_clk_rate_set(24000000);
	
	rc = si2c_read(SI2C_A2D2, SI2C_SA, 0x0130, &rd);

	//SKYCERR("%s : rd = %d \n", __func__,rd);

	(void)s5k6aafx13_power_off();

	svreg_release(svregs, CAMV_MAX);
	sgpio_release(sgpios, CAMIO_MAX);
	si2c_release();
#endif		

	sctrl->s_init    = s5k6aafx13_sensor_init;
	sctrl->s_release = s5k6aafx13_sensor_release;
	sctrl->s_config  = s5k6aafx13_sensor_config;

	sctrl->s_camera_type = FRONT_CAMERA_2D;
	sctrl->s_mount_angle = 270;

	//SKYCDBG("%s X\n", __func__);
	return 0;
}

static struct s5k6aafx13_format s5k6aafx13_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_YUYV8_2X8,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static int s5k6aafx13_enum_fmt(struct v4l2_subdev *sd, unsigned int index,
			   enum v4l2_mbus_pixelcode *code)
{
	//SKYCDBG(KERN_DEBUG "Index is %d\n", index);
	if ((unsigned int)index >= ARRAY_SIZE(s5k6aafx13_subdev_info))
		return -EINVAL;

	*code = s5k6aafx13_subdev_info[index].code;
	return 0;
}

static struct v4l2_subdev_core_ops s5k6aafx13_subdev_core_ops;
static struct v4l2_subdev_video_ops s5k6aafx13_subdev_video_ops = {
	.enum_mbus_fmt = s5k6aafx13_enum_fmt,
};

static struct v4l2_subdev_ops s5k6aafx13_subdev_ops = {
	.core = &s5k6aafx13_subdev_core_ops,
	.video  = &s5k6aafx13_subdev_video_ops,
};

static int s5k6aafx13_sensor_probe_cb(const struct msm_camera_sensor_info *info,
	struct v4l2_subdev *sdev, struct msm_sensor_ctrl *s)
{
	int rc = 0;
	rc = s5k6aafx13_sensor_probe(info, s);
	if (rc < 0)
		return rc;

	s5k6aafx13_ctrl = kzalloc(sizeof(struct s5k6aafx13_ctrl_t), GFP_KERNEL);
	if (!s5k6aafx13_ctrl) {
		//SKYCERR("s5k6aafx13_sensor_probe failed!\n");
		return -ENOMEM;
	}

	/* probe is successful, init a v4l2 subdevice */
	//SKYCDBG("going into v4l2_i2c_subdev_init\n");
	if (sdev) {
		v4l2_i2c_subdev_init(sdev, s5k6aafx13_client,
						&s5k6aafx13_subdev_ops);
		s5k6aafx13_ctrl->sensor_dev = sdev;
	}
	return rc;
}

static int __s5k6aafx13_probe(struct platform_device *pdev)
{
	//SKYCDBG("%s EX\n", __func__);
	//return msm_camera_drv_start(pdev, s5k6aafx13_sensor_probe);
	return msm_sensor_register(pdev, s5k6aafx13_sensor_probe_cb);
}


static struct platform_driver msm_camera_driver = {
	.probe = __s5k6aafx13_probe,
	.driver = {
		.name = "msm_camera_s5k6aafx13",
		.owner = THIS_MODULE,
	},
};


static int __init s5k6aafx13_init(void)
{
	//SKYCDBG("%s EX\n", __func__);
	return platform_driver_register(&msm_camera_driver);
}
module_init(s5k6aafx13_init);

#if 1
static void __exit s5k6aafx13_exit(void)
{
	//SKYCDBG("%s EX\n", __func__);
	i2c_del_driver(&s5k6aafx13_i2c_driver);
}
module_exit(s5k6aafx13_exit);


MODULE_DESCRIPTION("Samsung 1.3MP SoC Sensor Driver");
MODULE_LICENSE("GPL v2");
#endif

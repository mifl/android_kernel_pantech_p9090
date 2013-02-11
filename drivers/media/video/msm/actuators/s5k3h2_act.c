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

#if 1//dw9714->s5k3h2//wsyang_temp
#include "msm_actuator.h"
#include <linux/debugfs.h>

//#define DW9714_TOTAL_STEPS_NEAR_TO_FAR_MAX 43 // 37
#define	S5K3H2_TOTAL_STEPS_NEAR_TO_FAR			53//wsyang_temp 48 //52  bsy 2011_12_20

DEFINE_MUTEX(s5k3h2_act_mutex);
static int s5k3h2_actuator_debug_init(void);
static struct msm_actuator_ctrl_t s5k3h2_act_t;

static int32_t s5k3h2_wrapper_i2c_write(struct msm_actuator_ctrl_t *a_ctrl,
	int16_t next_lens_position, void *params)
{
	uint16_t msb = 0, lsb = 0;
	msb = (next_lens_position >> 4) & 0x3F;
	lsb = (next_lens_position << 4) & 0xF0;
	lsb |= (*(uint8_t *)params);
	CDBG("%s: Actuator MSB:0x%x, LSB:0x%x\n", __func__, msb, lsb);
	msm_camera_i2c_write(&a_ctrl->i2c_client,
		msb, lsb, MSM_CAMERA_I2C_BYTE_DATA);
	return next_lens_position;
}

static uint8_t s5k3h2_hw_params[] = {
	0x0,
	0x5,
	0x6,
	0x8,
	0xD, //0xB,
};

static uint16_t s5k3h2_macro_scenario[] = {
	/* MOVE_NEAR dir*/
	4,
	S5K3H2_TOTAL_STEPS_NEAR_TO_FAR,
};

static uint16_t s5k3h2_inf_scenario[] = {
	/* MOVE_FAR dir */
	8,
	22,
	S5K3H2_TOTAL_STEPS_NEAR_TO_FAR,
};

static struct region_params_t s5k3h2_regions[] = {
	/* step_bound[0] - macro side boundary
	 * step_bound[1] - infinity side boundary
	 */
	/* Region 1 */
	{
		.step_bound = {2, 0},
		.code_per_step = 50, //90,
	},
	/* Region 2 */
	{
		.step_bound = {S5K3H2_TOTAL_STEPS_NEAR_TO_FAR, 2},
		.code_per_step = 15, //9,
	}
};

static struct damping_params_t s5k3h2_macro_reg1_damping[] = {
	/* MOVE_NEAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 4500, //1500, 03_14_bsy
		.hw_params = &s5k3h2_hw_params[4], //[0], 03_14_bsy
	},
	/* Scene 2 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 4500, //1500, 03_14_bsy
		.hw_params = &s5k3h2_hw_params[4], //[0], 03_14_bsy
	},
};

static struct damping_params_t s5k3h2_macro_reg2_damping[] = {
	/* MOVE_NEAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 4500,
		.hw_params = &s5k3h2_hw_params[4],
	},
	/* Scene 2 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 4500,
		.hw_params = &s5k3h2_hw_params[4],
	},
};

static struct damping_params_t s5k3h2_inf_reg1_damping[] = {
	/* MOVE_FAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 4500, //1500, 03_14_bsy
		.hw_params = &s5k3h2_hw_params[4], //[1], 03_14_bsy
	},
	/* Scene 2 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 4500, //1500, 03_14_bsy
		.hw_params = &s5k3h2_hw_params[4], //[1], 03_14_bsy
	},
	/* Scene 3 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay =  4500, //1500, 03_14_bsy
		.hw_params = &s5k3h2_hw_params[4], //[1], 03_14_bsy
	},
};

static struct damping_params_t s5k3h2_inf_reg2_damping[] = {
	/* MOVE_FAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 0x1FF,
		.damping_delay = 4500,
		.hw_params = &s5k3h2_hw_params[4],
	},
	/* Scene 2 => Damping params */
	{
		.damping_step = 0x1FF,
		.damping_delay = 4500,
		.hw_params = &s5k3h2_hw_params[4],
	},
	/* Scene 3 => Damping params */
	{
		.damping_step = 27,
		.damping_delay = 2700,
		.hw_params = &s5k3h2_hw_params[4],
	},
};

static struct damping_t s5k3h2_macro_regions[] = {
	/* MOVE_NEAR dir */
	/* Region 1 */
	{
		.ringing_params = s5k3h2_macro_reg1_damping,
	},
	/* Region 2 */
	{
		.ringing_params = s5k3h2_macro_reg2_damping,
	},
};

static struct damping_t s5k3h2_inf_regions[] = {
	/* MOVE_FAR dir */
	/* Region 1 */
	{
		.ringing_params = s5k3h2_inf_reg1_damping,
	},
	/* Region 2 */
	{
		.ringing_params = s5k3h2_inf_reg2_damping,
	},
};


static int32_t s5k3h2_set_params(struct msm_actuator_ctrl_t *a_ctrl)
{
	return 0;
}

static const struct i2c_device_id s5k3h2_act_i2c_id[] = {
	{"s5k3h2_act", (kernel_ulong_t)&s5k3h2_act_t},
	{ }
};

static int s5k3h2_act_config(
	void __user *argp)
{
	CDBG("%s called\n", __func__);
	return (int) msm_actuator_config(&s5k3h2_act_t, argp);
}

static int s5k3h2_i2c_add_driver_table(
	void)
{
	CDBG("%s called\n", __func__);
	return (int) msm_actuator_init_table(&s5k3h2_act_t);
}

static struct i2c_driver s5k3h2_act_i2c_driver = {
	.id_table = s5k3h2_act_i2c_id,
	.probe  = msm_actuator_i2c_probe,
	.remove = __exit_p(s5k3h2_act_i2c_remove),
	.driver = {
		.name = "s5k3h2_act",
	},
};

static int __init s5k3h2_i2c_add_driver(
	void)
{
	CDBG("%s called\n", __func__);
	return i2c_add_driver(s5k3h2_act_t.i2c_driver);
}

static struct v4l2_subdev_core_ops s5k3h2_act_subdev_core_ops;

static struct v4l2_subdev_ops s5k3h2_act_subdev_ops = {
	.core = &s5k3h2_act_subdev_core_ops,
};

static int32_t s5k3h2_act_probe(
	void *board_info,
	void *sdev)
{
	s5k3h2_actuator_debug_init();

	return (int) msm_actuator_create_subdevice(&s5k3h2_act_t,
		(struct i2c_board_info const *)board_info,
		(struct v4l2_subdev *)sdev);
}

static struct msm_actuator_ctrl_t s5k3h2_act_t = {
	.i2c_driver = &s5k3h2_act_i2c_driver,
	.i2c_addr = 0x18,
	.act_v4l2_subdev_ops = &s5k3h2_act_subdev_ops,
	.actuator_ext_ctrl = {
		.a_init_table = s5k3h2_i2c_add_driver_table,
		.a_create_subdevice = s5k3h2_act_probe,
		.a_config = s5k3h2_act_config,
	},

	.i2c_client = {
		.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	},

	.set_info = {
		.total_steps = S5K3H2_TOTAL_STEPS_NEAR_TO_FAR,
	},

	.curr_step_pos = 0,
	.curr_region_index = 0,
	.initial_code = 0, //0x64, //0,
	.actuator_mutex = &s5k3h2_act_mutex,

	/* Initialize scenario */
	.ringing_scenario[MOVE_NEAR] = s5k3h2_macro_scenario,
	.scenario_size[MOVE_NEAR] = ARRAY_SIZE(s5k3h2_macro_scenario),
	.ringing_scenario[MOVE_FAR] = s5k3h2_inf_scenario,
	.scenario_size[MOVE_FAR] = ARRAY_SIZE(s5k3h2_inf_scenario),

	/* Initialize region params */
	.region_params = s5k3h2_regions,
	.region_size = ARRAY_SIZE(s5k3h2_regions),

	/* Initialize damping params */
	.damping[MOVE_NEAR] = s5k3h2_macro_regions,
	.damping[MOVE_FAR] = s5k3h2_inf_regions,

	.func_tbl = {
		.actuator_set_params = s5k3h2_set_params,
		.actuator_init_focus = NULL,
		.actuator_init_table = msm_actuator_init_table,
		.actuator_move_focus = msm_actuator_move_focus,
		.actuator_write_focus = msm_actuator_write_focus,
		.actuator_i2c_write = s5k3h2_wrapper_i2c_write,
		.actuator_set_default_focus = msm_actuator_set_default_focus,
	},

	.get_info = {
		 .focal_length_num = 42, //46,
		 .focal_length_den = 10,
		 .f_number_num = 280, //265,
		 .f_number_den = 100,
		 .f_pix_num = 14,
		 .f_pix_den = 10,
		 .total_f_dist_num = 197681,
		 .total_f_dist_den = 1000,
		 .hor_view_angle_num = 548, 
		 .hor_view_angle_den = 10, 
		 .ver_view_angle_num = 425, 
		 .ver_view_angle_den = 10, 
	},

};

static int s5k3h2_actuator_set_delay(void *data, u64 val)
{
	s5k3h2_inf_reg2_damping[1].damping_delay = val;
	return 0;
}

static int s5k3h2_actuator_get_delay(void *data, u64 *val)
{
	*val = s5k3h2_inf_reg2_damping[1].damping_delay;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(s5k3h2_delay,
	s5k3h2_actuator_get_delay,
	s5k3h2_actuator_set_delay,
	"%llu\n");

static int s5k3h2_actuator_set_jumpparam(void *data, u64 val)
{
	s5k3h2_inf_reg2_damping[1].damping_step = val & 0xFFF;
	return 0;
}

static int s5k3h2_actuator_get_jumpparam(void *data, u64 *val)
{
	*val = s5k3h2_inf_reg2_damping[1].damping_step;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(s5k3h2_jumpparam,
	s5k3h2_actuator_get_jumpparam,
	s5k3h2_actuator_set_jumpparam,
	"%llu\n");

static int s5k3h2_actuator_set_hwparam(void *data, u64 val)
{
	s5k3h2_hw_params[2] = val & 0xFF;
	return 0;
}

static int s5k3h2_actuator_get_hwparam(void *data, u64 *val)
{
	*val = s5k3h2_hw_params[2];
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(s5k3h2_hwparam,
	s5k3h2_actuator_get_hwparam,
	s5k3h2_actuator_set_hwparam,
	"%llu\n");

static int s5k3h2_actuator_debug_init(void)
{
	struct dentry *debugfs_base = debugfs_create_dir("s5k3h2_actuator", NULL);
	if (!debugfs_base)
		return -ENOMEM;

	if (!debugfs_create_file("s5k3h2_delay",
		S_IRUGO | S_IWUSR, debugfs_base, NULL, &s5k3h2_delay))
		return -ENOMEM;

	if (!debugfs_create_file("s5k3h2_jumpparam",
		S_IRUGO | S_IWUSR, debugfs_base, NULL, &s5k3h2_jumpparam))
		return -ENOMEM;

	if (!debugfs_create_file("s5k3h2_hwparam",
		S_IRUGO | S_IWUSR, debugfs_base, NULL, &s5k3h2_hwparam))
		return -ENOMEM;

	return 0;
}
subsys_initcall(s5k3h2_i2c_add_driver);
MODULE_DESCRIPTION("S5K3H2 actuator");
MODULE_LICENSE("GPL v2");

#else
#include "msm_actuator.h"
#include "msm_camera_i2c.h"

#define	S5K3H2_TOTAL_STEPS_NEAR_TO_FAR			48 //52  bsy 2011_12_20
DEFINE_MUTEX(s5k3h2_act_mutex);
static struct msm_actuator_ctrl_t s5k3h2_act_t;

static struct region_params_t g_regions[] = {
	/* step_bound[0] - macro side boundary
	 * step_bound[1] - infinity side boundary
	 */
	/* Initial Region 1 */
	{
		.step_bound = {16, 1}, // {5, 0}, bsy 2011_12_20
		.code_per_step = 12, // 18,//2, bsy 2011_12_20
	},
	/* Linear Region 2 */
	{
		.step_bound = {S5K3H2_TOTAL_STEPS_NEAR_TO_FAR, 16}, //5, bsy 2011_12_20
		.code_per_step = 12, // 18,//2, bsy 2011_12_20
	},
};

static uint16_t g_scenario[] = {
	/* MOVE_NEAR and MOVE_FAR dir*/
	S5K3H2_TOTAL_STEPS_NEAR_TO_FAR,
};

static struct damping_params_t g_damping[] = {
	/* MOVE_NEAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 0xff, // 1,// 2,
		.damping_delay = 0,
	},
	{
		.damping_step = 0xff, // 1,// 2,
		.damping_delay = 0,
	},
};

static struct damping_t g_damping_params[] = {
	/* MOVE_NEAR and MOVE_FAR dir */
	/* Region 1 */
	{
		.ringing_params = g_damping,
	},
};

static int32_t s5k3h2_wrapper_i2c_write(struct msm_actuator_ctrl_t *a_ctrl,
	int16_t next_lens_position, void *params)
{
	uint16_t data_1 = 0;
	uint16_t data_2 = 0;

	data_1 = (next_lens_position & 0xFF00) >> 8; 
	data_2 = next_lens_position & 0x00FF;
	
	msm_camera_i2c_write(&a_ctrl->i2c_client,
			     data_1,
			     data_2,
			     MSM_CAMERA_I2C_BYTE_DATA);
	return 0;
}

int32_t s5k3h2_act_write_focus(
	struct msm_actuator_ctrl_t *a_ctrl,
	uint16_t curr_lens_pos,
	struct damping_params_t *damping_params,
	int8_t sign_direction,
	int16_t code_boundary)
{
	int32_t rc = 0;
	uint16_t dac_value = 0;

	LINFO("%s called, curr lens pos = %d, code_boundary = %d\n",
		  __func__,
		  curr_lens_pos,
		  code_boundary);
#if 0
	if (sign_direction == 1)
		dac_value = (code_boundary - curr_lens_pos) | 0x80;
	else
		dac_value = (curr_lens_pos - code_boundary);

	LINFO("%s dac_value = %d\n",
	      __func__,
	      dac_value);
#endif

	dac_value = 0x0000 | (code_boundary<<4) | 0x0c;// 0x0

    LINFO("%s dac_value = %d\n",
              __func__,
              dac_value);

	rc = a_ctrl->func_tbl.actuator_i2c_write(a_ctrl, dac_value, NULL);

	return rc;
}

static int32_t s5k3h2_set_default_focus(struct msm_actuator_ctrl_t *a_ctrl)
{
	int32_t rc = 0;

	if (!a_ctrl->step_position_table)
		a_ctrl->func_tbl.actuator_init_table(a_ctrl);
#if 0
	if (a_ctrl->curr_step_pos != 0) {
		rc = a_ctrl->func_tbl.actuator_i2c_write(a_ctrl, 0x7F, NULL);
		rc = a_ctrl->func_tbl.actuator_i2c_write(a_ctrl, 0x7F, NULL);
		a_ctrl->curr_step_pos = 0;
	} else if (a_ctrl->func_tbl.actuator_init_focus)
		rc = a_ctrl->func_tbl.actuator_init_focus(a_ctrl);
#endif		
	return rc;
}

static int32_t s5k3h2_act_init_focus(struct msm_actuator_ctrl_t *a_ctrl)
{
	int32_t rc = 0;
	LINFO("%s called\n",
	      __func__);
	/* Initialize to infinity */
#if 0//
	msm_camera_i2c_write(&a_ctrl->i2c_client,
		0x01,
		0xA9,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(&a_ctrl->i2c_client,
		0x02,
		0xD2,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(&a_ctrl->i2c_client,
		0x03,
		0x0C,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(&a_ctrl->i2c_client,
		0x04,
		0x14,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(&a_ctrl->i2c_client,
		0x05,
		0xB6,
		MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(&a_ctrl->i2c_client,
		0x06,
		0x4F,
		MSM_CAMERA_I2C_BYTE_DATA);

	rc = a_ctrl->func_tbl.actuator_i2c_write(a_ctrl, 0x7F, NULL);
	rc = a_ctrl->func_tbl.actuator_i2c_write(a_ctrl, 0x7F, NULL);
#endif    
	a_ctrl->curr_step_pos = 0;
	return rc;
}

static const struct i2c_device_id s5k3h2_act_i2c_id[] = {
	{"s5k3h2_act", (kernel_ulong_t)&s5k3h2_act_t},
	{ }
};

static int s5k3h2_act_config(
	void __user *argp)
{
	LINFO("%s called\n", __func__);
	return (int) msm_actuator_config(&s5k3h2_act_t, argp);
}

static int s5k3h2_i2c_add_driver_table(
	void)
{
	LINFO("%s called\n", __func__);
	return (int) msm_actuator_init_table(&s5k3h2_act_t);
}

static struct i2c_driver s5k3h2_act_i2c_driver = {
	.id_table = s5k3h2_act_i2c_id,
	.probe  = msm_actuator_i2c_probe,
	.remove = __exit_p(s5k3h2_act_i2c_remove),
	.driver = {
		.name = "s5k3h2_act",
	},
};

static int __init s5k3h2_i2c_add_driver(
	void)
{
	LINFO("%s called\n", __func__);
	return i2c_add_driver(s5k3h2_act_t.i2c_driver);
}

static struct v4l2_subdev_core_ops s5k3h2_act_subdev_core_ops;

static struct v4l2_subdev_ops s5k3h2_act_subdev_ops = {
	.core = &s5k3h2_act_subdev_core_ops,
};

static int32_t s5k3h2_act_create_subdevice(
	void *board_info,
	void *sdev)
{
	LINFO("%s called\n", __func__);

	return (int) msm_actuator_create_subdevice(&s5k3h2_act_t,
		(struct i2c_board_info const *)board_info,
		(struct v4l2_subdev *)sdev);
}

static struct msm_actuator_ctrl_t s5k3h2_act_t = {
	.i2c_driver = &s5k3h2_act_i2c_driver,
	.i2c_addr = 0x18,
	.act_v4l2_subdev_ops = &s5k3h2_act_subdev_ops,
	.actuator_ext_ctrl = {
		.a_init_table = s5k3h2_i2c_add_driver_table,
		.a_create_subdevice = s5k3h2_act_create_subdevice,
		.a_config = s5k3h2_act_config,
	},

	.i2c_client = {
		.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	},

	.set_info = {
		.total_steps = S5K3H2_TOTAL_STEPS_NEAR_TO_FAR,
	},

	.curr_step_pos = 0,
	.curr_region_index = 0,
	.initial_code = 0x64,//0x7F,
	.actuator_mutex = &s5k3h2_act_mutex,

	.func_tbl = {
		.actuator_init_table = msm_actuator_init_table,
		.actuator_move_focus = msm_actuator_move_focus,
		.actuator_write_focus = s5k3h2_act_write_focus,
		.actuator_set_default_focus = s5k3h2_set_default_focus,
		.actuator_init_focus = s5k3h2_act_init_focus,
		.actuator_i2c_write = s5k3h2_wrapper_i2c_write,
	},
#if 1
	.get_info = {
		.focal_length_num = 42, //46,
		.focal_length_den = 10,
		.f_number_num = 280, //265,
		.f_number_den = 100,
		.f_pix_num = 14,
		.f_pix_den = 10,
		.total_f_dist_num = 197681,
		.total_f_dist_den = 1000,
	},
#endif
	/* Initialize scenario */
	.ringing_scenario[MOVE_NEAR] = g_scenario,
	.scenario_size[MOVE_NEAR] = ARRAY_SIZE(g_scenario),
	.ringing_scenario[MOVE_FAR] = g_scenario,
	.scenario_size[MOVE_FAR] = ARRAY_SIZE(g_scenario),

	/* Initialize region params */
	.region_params = g_regions,
	.region_size = ARRAY_SIZE(g_regions),

	/* Initialize damping params */
	.damping[MOVE_NEAR] = g_damping_params,
	.damping[MOVE_FAR] = g_damping_params,
};

subsys_initcall(s5k3h2_i2c_add_driver);
MODULE_DESCRIPTION("S5K3H2 actuator");
MODULE_LICENSE("GPL v2");
#endif

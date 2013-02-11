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

#include "msm_actuator.h"
#include <linux/debugfs.h>

#define DW9714_TOTAL_STEPS_NEAR_TO_FAR_MAX 43 // 37

DEFINE_MUTEX(dw9714_act_mutex);
static int dw9714_actuator_debug_init(void);
static struct msm_actuator_ctrl_t dw9714_act_t;

static int32_t dw9714_wrapper_i2c_write(struct msm_actuator_ctrl_t *a_ctrl,
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

static uint8_t dw9714_hw_params[] = {
	0x0,
	0x5,
	0x6,
	0x8,
	0xA, // D, //0xB, // 04_13_bsy_CAF_noise 
};

static uint16_t dw9714_macro_scenario[] = {
	/* MOVE_NEAR dir*/
	4,
	DW9714_TOTAL_STEPS_NEAR_TO_FAR_MAX,
};

static uint16_t dw9714_inf_scenario[] = {
	/* MOVE_FAR dir */
	8,
	22,
	DW9714_TOTAL_STEPS_NEAR_TO_FAR_MAX,
};

static struct region_params_t dw9714_regions[] = {
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
		.step_bound = {DW9714_TOTAL_STEPS_NEAR_TO_FAR_MAX, 2},
		.code_per_step = 15, //9,
	}
};

static struct damping_params_t dw9714_macro_reg1_damping[] = {
	/* MOVE_NEAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 4500, //1500, 03_14_bsy
		.hw_params = &dw9714_hw_params[4], //[0], 03_14_bsy
	},
	/* Scene 2 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 4500, //1500, 03_14_bsy
		.hw_params = &dw9714_hw_params[4], //[0], 03_14_bsy
	},
};

static struct damping_params_t dw9714_macro_reg2_damping[] = {
	/* MOVE_NEAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 4500,
		.hw_params = &dw9714_hw_params[4],
	},
	/* Scene 2 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 4500,
		.hw_params = &dw9714_hw_params[4],
	},
};

static struct damping_params_t dw9714_inf_reg1_damping[] = {
	/* MOVE_FAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 4500, //1500, 03_14_bsy
		.hw_params = &dw9714_hw_params[4], //[1], 03_14_bsy
	},
	/* Scene 2 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay = 4500, //1500, 03_14_bsy
		.hw_params = &dw9714_hw_params[4], //[1], 03_14_bsy
	},
	/* Scene 3 => Damping params */
	{
		.damping_step = 0xFF,
		.damping_delay =  4500, //1500, 03_14_bsy
		.hw_params = &dw9714_hw_params[4], //[1], 03_14_bsy
	},
};

static struct damping_params_t dw9714_inf_reg2_damping[] = {
	/* MOVE_FAR Dir */
	/* Scene 1 => Damping params */
	{
		.damping_step = 0x1FF,
		.damping_delay = 4500,
		.hw_params = &dw9714_hw_params[4],
	},
	/* Scene 2 => Damping params */
	{
		.damping_step = 0x1FF,
		.damping_delay = 4500,
		.hw_params = &dw9714_hw_params[4],
	},
	/* Scene 3 => Damping params */
	{
		.damping_step = 27,
		.damping_delay = 2700,
		.hw_params = &dw9714_hw_params[4],
	},
};

static struct damping_t dw9714_macro_regions[] = {
	/* MOVE_NEAR dir */
	/* Region 1 */
	{
		.ringing_params = dw9714_macro_reg1_damping,
	},
	/* Region 2 */
	{
		.ringing_params = dw9714_macro_reg2_damping,
	},
};

static struct damping_t dw9714_inf_regions[] = {
	/* MOVE_FAR dir */
	/* Region 1 */
	{
		.ringing_params = dw9714_inf_reg1_damping,
	},
	/* Region 2 */
	{
		.ringing_params = dw9714_inf_reg2_damping,
	},
};


static int32_t dw9714_set_params(struct msm_actuator_ctrl_t *a_ctrl)
{
	return 0;
}

static const struct i2c_device_id dw9714_act_i2c_id[] = {
	{"dw9714_act", (kernel_ulong_t)&dw9714_act_t},
	{ }
};

static int dw9714_act_config(
	void __user *argp)
{
	CDBG("%s called\n", __func__);
	return (int) msm_actuator_config(&dw9714_act_t, argp);
}

static int dw9714_i2c_add_driver_table(
	void)
{
	CDBG("%s called\n", __func__);
	return (int) msm_actuator_init_table(&dw9714_act_t);
}

static struct i2c_driver dw9714_act_i2c_driver = {
	.id_table = dw9714_act_i2c_id,
	.probe  = msm_actuator_i2c_probe,
	.remove = __exit_p(dw9714_act_i2c_remove),
	.driver = {
		.name = "dw9714_act",
	},
};

static int __init dw9714_i2c_add_driver(
	void)
{
	CDBG("%s called\n", __func__);
	return i2c_add_driver(dw9714_act_t.i2c_driver);
}

static struct v4l2_subdev_core_ops dw9714_act_subdev_core_ops;

static struct v4l2_subdev_ops dw9714_act_subdev_ops = {
	.core = &dw9714_act_subdev_core_ops,
};

static int32_t dw9714_act_probe(
	void *board_info,
	void *sdev)
{
	dw9714_actuator_debug_init();

	return (int) msm_actuator_create_subdevice(&dw9714_act_t,
		(struct i2c_board_info const *)board_info,
		(struct v4l2_subdev *)sdev);
}

static struct msm_actuator_ctrl_t dw9714_act_t = {
	.i2c_driver = &dw9714_act_i2c_driver,
	.i2c_addr = 0x18,
	.act_v4l2_subdev_ops = &dw9714_act_subdev_ops,
	.actuator_ext_ctrl = {
		.a_init_table = dw9714_i2c_add_driver_table,
		.a_create_subdevice = dw9714_act_probe,
		.a_config = dw9714_act_config,
	},

	.i2c_client = {
		.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	},

	.set_info = {
		.total_steps = DW9714_TOTAL_STEPS_NEAR_TO_FAR_MAX,
	},

	.curr_step_pos = 0,
	.curr_region_index = 0,
	.initial_code = 0, //0x64, //0,
	.actuator_mutex = &dw9714_act_mutex,

	/* Initialize scenario */
	.ringing_scenario[MOVE_NEAR] = dw9714_macro_scenario,
	.scenario_size[MOVE_NEAR] = ARRAY_SIZE(dw9714_macro_scenario),
	.ringing_scenario[MOVE_FAR] = dw9714_inf_scenario,
	.scenario_size[MOVE_FAR] = ARRAY_SIZE(dw9714_inf_scenario),

	/* Initialize region params */
	.region_params = dw9714_regions,
	.region_size = ARRAY_SIZE(dw9714_regions),

	/* Initialize damping params */
	.damping[MOVE_NEAR] = dw9714_macro_regions,
	.damping[MOVE_FAR] = dw9714_inf_regions,

	.func_tbl = {
		.actuator_set_params = dw9714_set_params,
		.actuator_init_focus = NULL,
		.actuator_init_table = msm_actuator_init_table,
		.actuator_move_focus = msm_actuator_move_focus,
		.actuator_write_focus = msm_actuator_write_focus,
		.actuator_i2c_write = dw9714_wrapper_i2c_write,
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

static int dw9714_actuator_set_delay(void *data, u64 val)
{
	dw9714_inf_reg2_damping[1].damping_delay = val;
	return 0;
}

static int dw9714_actuator_get_delay(void *data, u64 *val)
{
	*val = dw9714_inf_reg2_damping[1].damping_delay;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(dw9714_delay,
	dw9714_actuator_get_delay,
	dw9714_actuator_set_delay,
	"%llu\n");

static int dw9714_actuator_set_jumpparam(void *data, u64 val)
{
	dw9714_inf_reg2_damping[1].damping_step = val & 0xFFF;
	return 0;
}

static int dw9714_actuator_get_jumpparam(void *data, u64 *val)
{
	*val = dw9714_inf_reg2_damping[1].damping_step;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(dw9714_jumpparam,
	dw9714_actuator_get_jumpparam,
	dw9714_actuator_set_jumpparam,
	"%llu\n");

static int dw9714_actuator_set_hwparam(void *data, u64 val)
{
	dw9714_hw_params[2] = val & 0xFF;
	return 0;
}

static int dw9714_actuator_get_hwparam(void *data, u64 *val)
{
	*val = dw9714_hw_params[2];
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(dw9714_hwparam,
	dw9714_actuator_get_hwparam,
	dw9714_actuator_set_hwparam,
	"%llu\n");

static int dw9714_actuator_debug_init(void)
{
	struct dentry *debugfs_base = debugfs_create_dir("dw9714_actuator", NULL);
	if (!debugfs_base)
		return -ENOMEM;

	if (!debugfs_create_file("dw9714_delay",
		S_IRUGO | S_IWUSR, debugfs_base, NULL, &dw9714_delay))
		return -ENOMEM;

	if (!debugfs_create_file("dw9714_jumpparam",
		S_IRUGO | S_IWUSR, debugfs_base, NULL, &dw9714_jumpparam))
		return -ENOMEM;

	if (!debugfs_create_file("dw9714_hwparam",
		S_IRUGO | S_IWUSR, debugfs_base, NULL, &dw9714_hwparam))
		return -ENOMEM;

	return 0;
}
subsys_initcall(dw9714_i2c_add_driver);
MODULE_DESCRIPTION("OV8820 actuator");
MODULE_LICENSE("GPL v2");

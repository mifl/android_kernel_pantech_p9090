/************************************************************************************************
**
**    PANTECH AUDIO
**
**    FILE
**        pantech_audio.c
**
**    DESCRIPTION
**        This file contains pantech audio apis
**
**    Copyright (c) 2012 by PANTECH Incorporated.  All Rights Reserved.
*************************************************************************************************/

/************************************************************************************************
** Includes
*************************************************************************************************/
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <mach/gpio.h>
#include <asm/ioctls.h>
#include <linux/uaccess.h>

#include "pantech_aud_ctl.h"
#include "q6voice.h"


#if defined (CONFIG_MACH_MSM8960_SIRIUSLTE) //subamp gaincoltrol p15994
#include "../sky_snd_ak7811abb.h"
#endif

/*==========================================================================
** pantech_audio_ioctl
**=========================================================================*/

static long pantech_audio_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
#if defined(CONFIG_PANTECH_SND) && defined(CONFIG_MACH_MSM8960_VEGAPVW)  // FEATURE_PANTECH_SND_ELECTOVOX
	int data = 0;

	printk("[SKY SND] pantech_audio_ioctl, cmd=%i\n", cmd);

	switch (cmd) {
		case PANTECH_AUDIO_NR_CTL: {
			if (copy_from_user(&data, (void __user *)arg, sizeof(data))) {
				ret = -1;
				break;
			}

			printk("[SKY SND] PANTECH_AUDIO_NR_CTL, data=%i\n", data);
			
			ret = voice_send_set_oempp_enable_cmd(data);
			if (ret < 0) {
				pr_err("voice_send_set_oempp_enable_cmd failed\n");
				break;
			}
			break;
		}
		default: {
			printk("\n--------------- INVALID COMMAND ---------------");
			ret = -1;
			break;
		}
	}
#endif
#if defined (CONFIG_MACH_MSM8960_SIRIUSLTE) //Sub Amp Gain Control p15994
	switch (cmd) {
		case PANTECH_AUDIO_AMP_GAIN: {
			printk("PANTECH_AUDIO_AMP_GAIN\n");
	
			ret = snd_subsystem_setSPGain ((int)arg);
			if (ret < 0) {
				pr_err("snd_subsystem_setSPGain failed\n");
				break;
			}
			break;
		}
		case PANTECH_AUDIO_AMP_MODE: {
			printk("PANTECH_AUDIO_AMP_MODEINFO\n");
			ret = snd_subsystem_set_Modeinfo((int)arg);
			if (ret < 0) {
				pr_err("snd_subsystem_set_Modeinfo failed\n");
				break;
			}
			break;
		}
		case PANTECH_AUDIO_AMP_BE_SET: {
			printk("PANTECH_AUDIO_AMP_BE_SET\n");
			ret = snd_subsystem_set_BE((int)arg);
			if (ret < 0) {
				pr_err("snd_subsystem_set_BE failed\n");
				break;
			}
			break;
		}
		case PANTECH_AUDIO_AMP_LOUD_SET: {
			printk("PANTECH_AUDIO_AMP_LOUD_SET\n");
			ret = snd_subsystem_set_LOUD((int)arg);
			if (ret < 0) {
				pr_err("snd_subsystem_set_BE failed\n");
				break;
			}
			break;
		}
		case PANTECH_AUDIO_GET_AMP_GAIN: {
			printk("PANTECH_AUDIO_GET_AMP_GAIN\n");
			ret = snd_subsystem_getSPGain((int*)&arg);
			//printk("AMP GAIN = %d\n", (int)arg);
			ret = (int)arg;
			break;
		}
		case PANTECH_AUDIO_GET_MODEINFO: {
			printk("PANTECH_AUDIO_GET_MODEINFO\n");
			ret = snd_subsystem_get_Modeinfo((int*)&arg);
			//printk("ModeInfo = %d\n", (int)arg);
			ret = (int)arg;
			break;
		}
		case PANTECH_AUDIO_GET_BE_SET: {
			printk("PANTECH_AUDIO_GET_BE_SET\n");
			ret = snd_subsystem_get_BE((int*)&arg);
			//printk("BE set = %d\n", (int)arg);
			ret = (int)arg;
			break;
		}
		case PANTECH_AUDIO_GET_LOUD: {
			printk("PANTECH_AUDIO_GET_LOUD\n");
			ret = snd_subsystem_get_LOUD((int*)&arg);
			//printk("LOUD  = %d\n", (int)arg);
			ret = (int)arg;
			break;
		}
		default: {
			printk("\n--------------- INVALID COMMAND ---------------");
			ret = -1;
			break;
		}
	}
#endif

	return ret;
}

/*==========================================================================
** pantech_audio_open
**=========================================================================*/

static int pantech_audio_open(struct inode *inode, struct file *file)
{
	//printk("aud_sub_open");
	return 0;
}

/*==========================================================================
** pantech_audio_release
**=========================================================================*/

static int pantech_audio_release(struct inode *inode, struct file *file)
{
	//printk("aud_sub_release");
	return 0;	
}

/*=========================================================================*/

static struct file_operations snd_fops = {
	.owner = THIS_MODULE,
	.open = pantech_audio_open,
	.release = pantech_audio_release,
	.unlocked_ioctl	= pantech_audio_ioctl,
};

struct miscdevice pantech_audio_misc = 
{
	.minor = MISC_DYNAMIC_MINOR,
	.name = "pantech_aud_ctl",
	.fops = &snd_fops
};

/*==========================================================================
** pantech_audio_init
**=========================================================================*/

static int __init pantech_audio_init(void)
{
	int result = 0;

	result = misc_register(&pantech_audio_misc);
	if(result)
	{
		printk("pantech_audio_init: misc_register failed\n");
	}

	return result;
}

/*==========================================================================
** pantech_audio_exit
**=========================================================================*/

static void __exit pantech_audio_exit(void)
{
}

/*=========================================================================*/
#if defined (CONFIG_MACH_MSM8960_SIRIUSLTE)
subsys_initcall(pantech_audio_init); //p15994 subamp gaincoltrol
#else
module_init(pantech_audio_init);
#endif

module_exit(pantech_audio_exit);

MODULE_DESCRIPTION("Pantech audio driver");
MODULE_LICENSE("GPL v2");

/*=========================================================================*/

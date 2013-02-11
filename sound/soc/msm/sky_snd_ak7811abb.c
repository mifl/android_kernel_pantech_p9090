/************************************************************************************************
**
**    AK7811ABB AUDIO EXTERNAL AMP(SUBSYSTEM)
**
**    FILE
**        snd_sub_ak7811abb.c
**
**    DESCRIPTION
**        This file contains AK Audio Subsystem api
**
**          void snd_subsystem_Init()
**          void snd_subsystem_DeInit()
**          void snd_subsystem_SetPath()
**          void snd_subsystem_SetVolume()
**
**          Copyright (c) 2012 by Kim HyunWoo <kim.hyunwoo1@pantech.com>
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

#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/platform_device.h>
#include <linux/mfd/pm8xxx/pm8921.h>

#include <mach/board.h>
#include <mach/vreg.h>

//#include <linux/regulator/pmic8058-regulator.h>
//#include <linux/pmic8058-othc.h>

#include "sky_snd_ak7811abb.h"

#include <linux/i2c-gpio.h>


/************************************************************************************************
** Definition
*************************************************************************************************/

#define CONFIG_AK7811_DEBUG_PRINTK  // kernel debug message enable

/************************************************************************************************
** Variables
*************************************************************************************************/

//static subsystem_info_t tSubsystemInfo;
subsystem_info_t tSubsystemInfo;

static struct i2c_client *ak7811_i2c_client = NULL;

static int current_device = 0;

/************************************************************************************************
** Prototypes
*************************************************************************************************/

static int ak7811_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int __exit ak7811_remove(struct i2c_client *client);
static int ak7811_suspend(struct i2c_client *client, pm_message_t mesg);
static int ak7811_resume(struct i2c_client *client);
static void ak7811_shutdown(struct i2c_client *client);

static int snd_subsystem_i2c_write(u8 reg, u8 data);
static int snd_subsystem_i2c_read(u8 reg, u8 *data);
//static int snd_subsystem_setSPGain (u8 spAmpGain);
//int snd_subsystem_set_Modeinfo (int MODE);
//int snd_subsystem_set_BE (unsigned char BE_BAND_SEL, unsigned char BE_LEVEL, unsigned char TREBLE_LEVEL, unsigned char BASE_HEADROOM );






/************************************************************************************************
** Declararations
*************************************************************************************************/

static const struct i2c_device_id ak7811_id[] = {
	{ "ak7811-i2c", 0},
};

static struct i2c_driver ak7811_driver = {
	.id_table		= ak7811_id,
	.probe 		= ak7811_probe,
	.remove		= __exit_p(ak7811_remove),
	.suspend		= ak7811_suspend,
	.resume		= ak7811_resume,
	.shutdown	= ak7811_shutdown,
	.driver 	= {
	.name = "ak7811-i2c",
	},
};

/* Driver functions*/

/*==========================================================================
** Function: 		ak7811_probe
** Parameter:
** Return:
**				i2c functiality check fail	 0
**				subsystem init fail	-1
** Description:	probe function
**=========================================================================*/
static int ak7811_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;

	if (!(ret = i2c_check_functionality(client->adapter, I2C_FUNC_I2C))) 
	{
		ak7811_i2c_client = NULL;
#ifdef CONFIG_AK7811_DEBUG_PRINTK
		printk(KERN_ERR "[ak7811] ak7811_probe failed \n");
#endif
		return ret;
	}
	else 
		ak7811_i2c_client = client;

	snd_subsystem_standby(SYSTEM_OFF);
	
#ifdef CONFIG_AK7811_DEBUG_PRINTK
	printk("[ak7811] ak7811_probe success \n");
#endif

	return 0;
}


/*==========================================================================
** Function: 		ak7811_remove
** Parameter:
** Return:		Always 0
** Description:	Remove i2c client
**=========================================================================*/
static int __exit ak7811_remove(struct i2c_client *client)
{
	ak7811_i2c_client = NULL;
	//rc = i2c_detach_client(client);
	
#ifdef CONFIG_AK7811_DEBUG_PRINTK
	printk("[ak7811] ak7811_removed\n");	
#endif

	return 0;
}

/*==========================================================================
** Function: 		ak7811_suspend
** Parameter:
** Return:		Always 0		
** Description:	not used
**=========================================================================*/
static int ak7811_suspend(struct i2c_client *client, pm_message_t mesg)
{
#ifdef CONFIG_AK7811_DEBUG_PRINTK
	printk("[ak7811] ak7811_suspend\n");
#endif

	return 0;
}

/*==========================================================================
** Function: 		ak7811_resume
** Parameter: 	
** Return:		Always 0
** Description:	not used
**=========================================================================*/
static int ak7811_resume(struct i2c_client *client)
{
#ifdef CONFIG_AK7811_DEBUG_PRINTK
	printk("[ak7811] ak7811_resume\n");
#endif

	return 0;
}

/*==========================================================================
** Function: 		ak7811_shutdown
** Parameter: 
** Return:		void
** Description:	Turn FAB2210 driver off (Stanby mode)
**=========================================================================*/
static void ak7811_shutdown(struct i2c_client *client)
{
	int ret;
	
	ret = snd_subsystem_standby(SYSTEM_OFF);

#ifdef CONFIG_AK7811_DEBUG_PRINTK
	if (ret < 0)
		printk(KERN_ERR "[ak7811] ak7811_shutdown fail...\n");	

	printk("[ak7811] ak7811_shutdown \n");	
#endif
}


/* Public functions */

/*==========================================================================
** Function: 		snd_subsystem_Init
** Parameter: 	void
** Return:		void
** Description:	Add FAB2210 driver to i2c
**=========================================================================*/
void snd_subsystem_Init(void)
{
#ifdef CONFIG_AK7811_DEBUG_PRINTK
	printk("[ak7811] subsystem init!!\n");
#endif

	if (i2c_add_driver(&ak7811_driver))
#ifdef CONFIG_AK7811_DEBUG_PRINTK
		printk(KERN_ERR "ak7811_driver add failed.\n");
	else
		printk("ak7811_driver add success.\n");
#endif
	
}

/*==========================================================================
** Function: 		snd_subsystem_DeInit
** Parameter: 	void
** Return:		void
** Description:	Delete FAB2210 driver from i2c
**=========================================================================*/
void snd_subsystem_DeInit(void)
{
	i2c_del_driver(&ak7811_driver);

#ifdef CONFIG_AK7811_DEBUG_PRINTK
	printk("fab2210_driver delete success.\n");
#endif
}

/*==========================================================================
** Function: 		snd_subsystem_get_device
** Parameter: 	void
** Return:		Device type(number)
** Description:	Return current device type
**=========================================================================*/
int snd_subsystem_get_device(void)
{
	return current_device;
}

/*==========================================================================
** Function: 		snd_subsystem_sp_poweron
** Parameter: 	void
** Return:		success 0, fail -1
** Description:	Enable Speaker
**				Temporary function (will be modified later)
**=========================================================================*/

int snd_subsystem_sp_poweron(void)
{
	int ret=0;
	u8 Set_Gain = 0x13;
	u8 ModeInfo = 0x01;
	u8 Set_BE = 0x65;
	u8 Set_Loud = 0x09;

	ret = snd_subsystem_standby(SYSTEM_ON);
	msleep(50);

	if (ret < 0) {
		printk("[snd_AK7811abb] subsystem_standby Fail!!");
		return ret;
	}

	// Temp Setting
	//read_data=kmalloc(sizeof(u8), GFP_KERNEL);
	//ret = snd_subsystem_getSPGain(read_data);
	
	// AMP GAIN Setting (Pre Gain)
	if (tSubsystemInfo.spgain_info != 0) {
		//printk("[snd_AK7811abb] subsystem setting Gain Value !!");
		Set_Gain = (u8)tSubsystemInfo.spgain_info;
	}
	
	ret = snd_subsystem_setSPGain(Set_Gain);	// 19db
	msleep(50);

	if (ret < 0) {
		printk("[snd_AK7811abb] subsystem setting Gain Value Fail!!");
		return ret;
	}

	// Mode Info Setting
	if (tSubsystemInfo.mode_info != 0) {
		//printk("[snd_AK7811abb] subsystem setting MODE INFO Value !!");
		ModeInfo = (u8)tSubsystemInfo.mode_info;
	}
	ret = snd_subsystem_set_Modeinfo(ModeInfo);	// 0:BE  1:LOUD  2:BE&LOUD off  3:BE&LOUD off
	msleep(50);

	if (ret < 0) {
		printk("[snd_AK7811abb] subsystem set Modeinfo Fail!!");
		return ret;
	}

	// Set BE
	if (tSubsystemInfo.be_info  != 0) {
		//printk("[snd_AK7811abb] subsystem setting BE mode Value !!");
		Set_BE = (u8)tSubsystemInfo.be_info;
	}
	
	ret = snd_subsystem_set_BE(Set_BE);//(0x2,0x0,0x0,0x0);  // 0x3,0x2,0x1,0x2
	msleep(50);	

	if (ret < 0) {
		printk("[snd_AK7811abb] subsystem set BE Fail!!");
		return ret;
	}

	// Set LOUD
	if (tSubsystemInfo.loud_info  != 0) {
		//printk("[snd_AK7811abb] subsystem setting Loud Value !!");
		Set_Loud = (u8)tSubsystemInfo.loud_info;
	}
	
	ret = snd_subsystem_set_LOUD(Set_Loud);
	msleep(50);	

	if (ret < 0) {
		printk("[snd_AK7811abb] subsystem set BE Fail!!");
		return ret;
	}

//	snd_subsystem_i2c_write (0x83, 0x20);


	return ret;
}



int snd_subsystem_readInfo (void)
{
	int ret = 0;

	ret = snd_subsystem_standby(SYSTEM_ON);
	msleep(50);

	ret |= snd_subsystem_i2c_read (SLEEP_INFO, &tSubsystemInfo.sleep_info);

	if (ret < 0)
	{
#ifdef CONFIG_AK7811_DEBUG_PRINTK
		printk(KERN_ERR"[snd_subsystem_readInfo] Infomation load failed...\n");
#endif
		return ret;
	}

#ifdef CONFIG_AK7811_DEBUG_PRINTK
	printk("[snd_subsystem_readInfo] AK7811abb infomations loaded!!\n");
#endif
	return ret;
}



/*==========================================================================
** Function: 		snd_subsystem_standby
** Parameter: 	standby = SYSTEM_ON or SYSTEM_OFF 
** Return:		success 0, fail -1
** Description:	Enable or disable AK7811 driver
**=========================================================================*/

int snd_subsystem_standby (int standby)
{
	int ret = 0;

	if (standby == SYSTEM_OFF)
		tSubsystemInfo.sleep_info = SLEEP_OFF_VALUE;
	else if (standby == SYSTEM_ON)
		tSubsystemInfo.sleep_info = SLEEP_ON_VALUE;

	ret = snd_subsystem_i2c_write(SLEEP_INFO, tSubsystemInfo.sleep_info);

	if (ret < 0)
	{
#ifdef CONFIG_AK7811_DEBUG_PRINTK
	if (standby)
		printk(KERN_ERR"[snd_subsystem_standby] ak7811 subsystem enable fail\n");
	else
		printk(KERN_ERR"[snd_subsystem_standby] ak7811 subsystem disable fail\n");
#endif
		return ret;
	}
	
#ifdef CONFIG_AK7811_DEBUG_PRINTK
	if (standby)
		printk("[snd_subsystem_standby] ak7811 subsystem enable success\n");
	else
		printk("[snd_subsystem_standby] ak7811 subsystem disable success\n");
#endif

	return ret;
}



/*==========================================================================
** Function: 		snd_subsystem_i2c_write
** Parameter: 	reg = register address to write
**				data = register value (command)
** Return:		success 0, fail -1
** Description:	Send i2c message to subsystem
**=========================================================================*/
static int snd_subsystem_i2c_write(u8 reg, u8 data)
{
	static int ret = 0;
	unsigned char buf[2];

	struct i2c_msg msg[1];

	if(!ak7811_i2c_client){
		return -1;
	}

	buf[0] = (unsigned char)reg;
	buf[1] = (unsigned char)data;

	msg[0].addr = ak7811_i2c_client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = buf;
	
	if ((ret = i2c_transfer(ak7811_i2c_client->adapter, msg, 1)) < 0)
	{
#ifdef CONFIG_AK7811_DEBUG_PRINTK		
		printk("[ak7811] snd_subsystem_i2c_write fail !!!! ret : %d \n", ret);
#endif
		return ret;
	}

	return 0;
}

/*==========================================================================
** Function: 		snd_subsystem_i2c_read
** Parameter: 	reg = register address to read
**				data = register value (command)
** Return:		success 0, fail -1
** Description:	Recieve i2c message from subsystem
**=========================================================================*/
static int snd_subsystem_i2c_read(u8 reg, u8 *data)
{
	static int ret = 0;
	unsigned char buf[1];
	struct i2c_msg msgs[2];

	if(!ak7811_i2c_client){
		return -1;
	}

	buf[0] = reg;

	msgs[0].addr = ak7811_i2c_client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = buf;

	msgs[1].addr = ak7811_i2c_client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = 1;
	msgs[1].buf = buf;

	ret = i2c_transfer(ak7811_i2c_client->adapter, msgs, 2);
	if ( ret < 0) {
		return ret;
	}

	*data = (u8)buf[0];

	printk("[ak7811] register : 0x%x, data : 0x%x \n", reg, buf[0]);

	return 0;
}


/*==========================================================================
** Function: 		snd_subsystem_setSPGain
** Parameter: 	spAmpGain = Speaker AMP gain (see spAmpGainTbl)
** Return:		success 0, fail -1
** Description:	Configuration speaker AMP gain
**				This function control these register - SP_GAIN
**=========================================================================*/

int snd_subsystem_setSPGain (u8 spAmpGain)
{

	int ret = 0;

	ret = snd_subsystem_standby(SYSTEM_ON);
	msleep(50);


//	clear_bits (tSubsystemInfo.spgain_info, 0x03, SP_GAIN_VALUE);
//	tSubsystemInfo.spgain_info |= spAmpGain;
	tSubsystemInfo.spgain_info = spAmpGain;

	ret = snd_subsystem_i2c_write(SPGAIN_INFO, tSubsystemInfo.spgain_info);

	if (ret < 0)
	{
#ifdef CONFIG_AK7811_DEBUG_PRINTK
		printk (KERN_ERR "[snd_subsystem_setSPGain] ak7811 subsystem Speaker Amp gain failed...\n");
#endif
		return ret;
	}

/*
#ifdef CONFIG_AK7811_DEBUG_PRINTK
	printk("[snd_subsystem_setSPGain] ak7811 subsystem Speaker AMP gain success\n");
#endif
*/
	return ret;

	
}

/*==========================================================================
** Function: 		snd_subsystem_set_Modeinfo
** Parameter: 	MODE = MODE (see MODE - MODE0 / MODE1 / FUNC)
** Return:		success 0, fail -1
** Description:	Configuration BE mode
**				This function control these register - MODE
**=========================================================================*/

int snd_subsystem_set_Modeinfo (int MODE)
{

	int ret = 0;

	ret = snd_subsystem_standby(SYSTEM_ON);
	msleep(50);

	tSubsystemInfo.mode_info = MODE;
	
	ret = snd_subsystem_i2c_write(MODE_INFO, tSubsystemInfo.mode_info);

	if (ret < 0)
	{
#ifdef CONFIG_AK7811_DEBUG_PRINTK
		printk (KERN_ERR "[snd_subsystem_set_Modeinfo] ak7811 subsystem MODE INFO failed...\n");
#endif
	return ret;
	}
/*
#ifdef CONFIG_AK7811_DEBUG_PRINTK
	printk("[snd_subsystem_set_Modeinfo] ak7811 subsystem MODE INFO success\n");
#endif
*/
	return ret;

	
}


/*==========================================================================
** Function: 	snd_subsystem_set_BE
** Parameter: 	BE_SET
** Return:		success 0, fail -1
** Description:	Configuration set BE
**				This function control these register - BE_INFO
**=========================================================================*/

int snd_subsystem_set_BE(int BE_SET)
{

	int ret = 0;

	ret = snd_subsystem_standby(SYSTEM_ON);
	msleep(50);

	tSubsystemInfo.be_info = BE_SET;//(BE_BAND_SEL<<6) | (BE_LEVEL<<4) |(TREBLE_LEVEL<<2) | BASE_HEADROOM;
	
	ret = snd_subsystem_i2c_write(BE_INFO, tSubsystemInfo.be_info);

	if (ret < 0)
	{
#ifdef CONFIG_AK7811_DEBUG_PRINTK
		printk (KERN_ERR "[snd_subsystem_set_BE] ak7811 subsystem BE set failed...\n");
#endif
		return ret;
	}
/*
#ifdef CONFIG_AK7811_DEBUG_PRINTK
	printk("[snd_subsystem_set_BE] ak7811 subsystem BE set success\n");
#endif
*/
	return ret;
}

/*==========================================================================
** Function: 	snd_subsystem_set_LOUD
** Parameter: 	Loud_Set
** Return:		success 0, fail -1
** Description:	Configuration set LOUD
**				This function control these register - LOUD_INFO
**=========================================================================*/

int snd_subsystem_set_LOUD(int Loud_Set)
{

	int ret = 0;

	ret = snd_subsystem_standby(SYSTEM_ON);
	msleep(50);

	tSubsystemInfo.loud_info = Loud_Set;
	
	ret = snd_subsystem_i2c_write(LOUD_INFO, tSubsystemInfo.loud_info);

	if (ret < 0)
	{
#ifdef CONFIG_AK7811_DEBUG_PRINTK
		printk (KERN_ERR "[snd_subsystem_set_LOUD] ak7811 subsystem LOUD set failed...\n");
#endif
		return ret;
	}
/*
#ifdef CONFIG_AK7811_DEBUG_PRINTK
	printk("[snd_subsystem_set_BE] ak7811 subsystem BE set success\n");
#endif
*/
	return ret;
}



/*==========================================================================
** Function: 		snd_subsystem_getSPGain
** Parameter: 	spAmpGain = Speaker AMP gain (see spAmpGainTbl)
** Return:		Reg Value
** Description:	Configuration speaker AMP gain
**				This function control these register - SP_GAIN
**=========================================================================*/

int snd_subsystem_getSPGain (int *data)
{

	int ret = 0;

	ret = snd_subsystem_standby(SYSTEM_OFF);
	msleep(50);

//	clear_bits (tSubsystemInfo.spgain_info, 0x03, SP_GAIN_VALUE);
//	tSubsystemInfo.spgain_info |= spAmpGain;
//	tSubsystemInfo.spgain_info = spAmpGain;

	ret = snd_subsystem_i2c_read(SPGAIN_INFO,(u8*) data);

	// printk(KERN_ERR "[snd_subsystem_getSPGain] I2C read data = %d\n", *data);

	if (ret < 0)
	{
#ifdef CONFIG_AK7811_DEBUG_PRINTK
		printk (KERN_ERR "[snd_subsystem_getSPGain] ak7811 subsystem Speaker Amp gain READ failed...\n");
#endif
		return ret;
	}

/*
#ifdef CONFIG_AK7811_DEBUG_PRINTK
	printk("[snd_subsystem_getSPGain] ak7811 subsystem Speaker AMP gain success\n");
#endif
*/
	return ret;

	
}

/*==========================================================================
** Function: 		snd_subsystem_get_Modeinfo
** Parameter: 	ModeInfo = Speaker AMP Mode Info 
** Return:		Reg Value
** Description:	Configuration speaker AMP Mode Info Value
**				This function control these register - MODE
**=========================================================================*/

int snd_subsystem_get_Modeinfo (int *data)
{

	int ret = 0;

	ret = snd_subsystem_standby(SYSTEM_OFF);
	msleep(50);

//	clear_bits (tSubsystemInfo.spgain_info, 0x03, SP_GAIN_VALUE);
//	tSubsystemInfo.spgain_info |= spAmpGain;
//	tSubsystemInfo.spgain_info = spAmpGain;

	ret = snd_subsystem_i2c_read(MODE_INFO,(u8*) data);

	// printk(KERN_ERR "[snd_subsystem_get_Modeinfo] I2C read data = %d\n", *data);

	if (ret < 0)
	{
#ifdef CONFIG_AK7811_DEBUG_PRINTK
		printk (KERN_ERR "[snd_subsystem_get_Modeinfo] ak7811 subsystem Speaker Mode INFO READ failed...\n");
#endif
		return ret;
	}

/*
#ifdef CONFIG_AK7811_DEBUG_PRINTK
	printk("[snd_subsystem_get_Modeinfo] ak7811 subsystem Speaker AMP gain success\n");
#endif
*/
	return ret;

	
}

/*==========================================================================
** Function: 		snd_subsystem_get_BE
** Parameter: 	ModeInfo = Speaker AMP BE information
** Return:		Reg Value
** Description:	Configuration speaker AMP BE Value
**				This function control these register - BE_INFO
**=========================================================================*/

int snd_subsystem_get_BE (int *data)
{

	int ret = 0;

	ret = snd_subsystem_standby(SYSTEM_OFF);
	msleep(50);

	ret = snd_subsystem_i2c_read(BE_INFO,(u8*) data);

	// printk(KERN_ERR "[snd_subsystem_get_BE] I2C read data = %d\n", *data);

	if (ret < 0)
	{
#ifdef CONFIG_AK7811_DEBUG_PRINTK
		printk (KERN_ERR "[snd_subsystem_get_BE] ak7811 subsystem Speaker BE reg READ failed...\n");
#endif
		return ret;
	}

/*
#ifdef CONFIG_AK7811_DEBUG_PRINTK
	printk("[snd_subsystem_get_BE] ak7811 subsystem Speaker BE set success\n");
#endif
*/
	return ret;
	
}


/*==========================================================================
** Function: 		snd_subsystem_get_LOUD
** Parameter: 	ModeInfo = Speaker AMP LOUD information
** Return:		Reg Value
** Description:	Configuration speaker AMP LOUD Value
**				This function control these register - LOUD_INFO
**=========================================================================*/

int snd_subsystem_get_LOUD (int *data)
{

	int ret = 0;

	ret = snd_subsystem_standby(SYSTEM_OFF);
	msleep(50);

	ret = snd_subsystem_i2c_read(LOUD_INFO,(u8*) data);

	// printk(KERN_ERR "[snd_subsystem_get_LOUD] I2C read data = %d\n", *data);

	if (ret < 0)
	{
#ifdef CONFIG_AK7811_DEBUG_PRINTK
		printk (KERN_ERR "[snd_subsystem_get_LOUD] ak7811 subsystem Speaker LOUD reg READ failed...\n");
#endif
		return ret;
	}

/*
#ifdef CONFIG_AK7811_DEBUG_PRINTK
	printk("[snd_subsystem_get_LOUD] ak7811 subsystem Speaker LOUD reg set success\n");
#endif
*/
	return ret;
	
}


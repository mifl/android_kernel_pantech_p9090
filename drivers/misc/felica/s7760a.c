/*
 * S-7760A Driver
 */


/* drivers/misc/s7760a.c  (Felica Calibation driver)
 *
 * Copyright (C) 2011 Pantech 
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/***************header***************/


#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/input.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <asm/current.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/unistd.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/major.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <linux/device.h>

#include <linux/s7760a.h>


#define FELICA_PPC_READ_E2PROM				1
#define FELICA_READ_REGISTER				2
#define FELICA_WRITE_E2PROM					3
#define FELICA_WRITE_REGISTER				4
#define CHANGE_TO_REGISTER_MODE				5
#define CHANGE_TO_EEPROM_MODE				6
#define FELICA_CEN_ON						7
#define FELICA_CEN_OFF						8
#define FELICA_PON_ON						9
#define FELICA_PON_OFF						10
#define FELICA_PPC_RELOAD					11
#define FELICA_S7760A_GET_CEN_INFO			12
#define CEN_EEPROM_Write					13
#define CEN_TEMP_LOCK						14
#define	FELICA_AUTOWRITE					15
#define	FELICA_CEN_WRITE					16
#define	FELICA_CEN_READ						17





/////////////////////////////////////////////////////////////////////////
//#define S7760A_DBG  /*Log Message Feature*/

#ifdef S7760A_DBG
#define FELICA_DBG(fmt, args...) printk("##S7760A##[%d]: " fmt, __LINE__, ##args)
#else 
#define FELICA_DBG(fmt, args...)
#endif
/////////////////////////////////////////////////////////////////////////

/*Gpio Num*/
#if (BOARD_VER<WS10)
#define S7760A_WP_GPIO			27
#else
#define S7760A_WP_GPIO			19
#endif 

#define PON_GPIO				65
#define FELICA_CEN_GPIO			66

/*CMD */
																	/*	Cmd					byte	       |Device code  |        |flag		*/
#define S7760A_RELOAD_MODE							0xf0			/* reload command		0x00 = 0|101		|0000|0/1		*/
#define S7760A_SET_RW_COMMAND						0xf5			/* set port command		0x05 = 0|101		|1010|0			*/
#define S7760A_ACCESS_CHANGE_MODE					0xf1			/* Change Access Mode 	0x01 = 0|101		|0001|0(REG)/1(EEPROM)	*/

/*CEN and PON Control*/

/////////////////////////////////////////////////////////////////////////
//#define FELICA_S7760A_CEN_CTRL_VEGARJ 	/*Model Feature*/
#ifdef FELICA_S7760A_CEN_CTRL_VEGARJ
#define CEN_MASK	0xc0	//Vega Racer J
#else
#define	CEN_MASK	0x80	//Sirius LTE
#endif
/////////////////////////////////////////////////////////////////////////

#define PORT_CONTROLL_RESX_ON						0x80
#define TO_REG										0
#define TO_EEPROM									1
#define I2C_WRITE_WAIT_TIME 						10
#define NO_WAIT_FLAG								0				/*  no wait flag(i2c)				*/
#define NEED_WAIT_FLAG								1				/* 	wait flag						*/
#define FELICA_PPC_I2C_TRY_COUNT					3				/* I2C retry count					*/
#define FALSE										-1
#define TRUE										0
#define I2C_FAILURE									-1
#define I2C_SUCCESS									0
#define WP_GPIO_ON									1
#define WP_GPIO_OFF									0
#define ACCESS_REGISTER_MODE						2
#define ACCESS_EEPROM_MODE							3


/*Device Setting*/
#define D_S7760A_DEVS		(1)
#define D_S7760A_DEV_NAME	("s7760a")
static struct i2c_client *this_client;

struct s7760a_data {
	struct input_dev *input_dev;
};


/*Variable Setting*/
static struct class *felica_class = NULL;
static struct cdev s7760a_cdev;
static int wp_gpio_flag=1;
static int access_mode=2;





//////////////////////////////////////////////////////////////////////////////////////////////////
static int s7760a_i2c_read(unsigned char cmd, unsigned char* buffer);
static int s7760a_i2c_write(unsigned char cmd, unsigned char set_data, int wait_flg);
static int s7760a_ld_write(unsigned char cmd, unsigned char set_data, int wait_flg);
static int s7760a_write_process(unsigned char cmd, unsigned char set_data, int wait_flg);
static int s7760a_reload_process(void);
static int s7760a_reload(void);
static int s7760a_change_mode(unsigned char mode, unsigned long mode_flag);
static int s7760a_open( struct inode *inode, struct file *filp );
static int s7760a_release( struct inode *inode, struct file *filp );
ssize_t s7760a_read(struct file *file, char __user *buf, size_t count, loff_t *pos);
ssize_t s7760a_write(struct file *file, const char *buf, size_t count, loff_t *pos);
static int s7760a_write_ftn(unsigned char arg);
static int s7760a_auto_cal_write_ftn(unsigned char arg);
static int s7760a_change_mode_ftn(int ACCEESS_MODE_TO);
static int s7760a_read_ftn(void);
static unsigned char s7760a_get_cen_data(void);

//////////////////////////////////////////////////////////////////////////////////////////////////



static int s7760a_i2c_read(unsigned char cmd, unsigned char* buffer)
{
	int	i2c_ret	= 0;

	
	struct i2c_msg mesgs[] = 
	{

		{
			.addr	= this_client->addr&cmd,
			.flags	= 1,
			.len	= 1,
			.buf	= buffer,
		},
	};
	FELICA_DBG("INFO: s7760a_i2c_read=> client->adapter = [%lu]\n", (long unsigned int) this_client->adapter);
	FELICA_DBG("INFO: s7760a_i2c_read=> s7760a_i2c_read: Cmd = [%d]\n", cmd);	
	
	if (buffer == NULL) 
		{
			printk(KERN_ERR"ERROR: buffer is NULL");
			return -1;
		}
	i2c_ret = i2c_transfer(this_client->adapter, mesgs, 1);
	
	
	if (i2c_ret <= I2C_FAILURE) 
		{
			printk(KERN_ERR"ERROR: s7760a_i2c_read: ret = %d\n", i2c_ret);			
			return 0;
		} 
	else 
		{
			FELICA_DBG("INFO: s7760a_i2c_read: ret = %d\n", i2c_ret);		
			return 1;
		}
}


static int s7760a_read_ftn(void)
{

	int	ret	= 0;
	unsigned char read_data[1]={0x00};

	FELICA_DBG("START: Felica s7760a Read\n");
	ret = s7760a_i2c_read(S7760A_SET_RW_COMMAND, &read_data[0]);
	if(ret)
		{
			FELICA_DBG("INFO: Succeed to appoach S7760A\n");
			FELICA_DBG("INFO: Felica S7760A ioctl read=> Read Buffer Data is [%x]\n",read_data[0]);
			FELICA_DBG("INFO: Felica S7760A ioctl read=> Read Buffer Data is [%d]\n",(int) read_data[0]);
		}
	else
		{
			printk("ERROR: Fail to appoach S7760A\n");
			return -1;
		}
		
	
	

	
	return (int) read_data[0];


}


ssize_t s7760a_read(struct file *file, char __user *buf,
				size_t count, loff_t *pos)
{

	int	ret			= FALSE;
	int mode_ret	= 0;
	int read_ret 	= 0;
	unsigned char read_data[1];
	unsigned char mask_cen_data=0;
	static DEFINE_MUTEX(lock);
	access_mode = count;
	FELICA_DBG("START: Felica s7760a Read\n");
	
	mutex_lock(&lock);
	
if(access_mode==3)
	mode_ret = s7760a_change_mode(S7760A_ACCESS_CHANGE_MODE,TO_EEPROM);
else
	mode_ret = s7760a_change_mode(S7760A_ACCESS_CHANGE_MODE,TO_REG);

	if(mode_ret)
		FELICA_DBG("INFO: Change Mode Success\n");	
	else if(mode_ret==0)
		printk("ERROR: Change Mode Failed\n");
	else
		printk("ERROR: Read Buffer is NULL PTR\n");
	
	//mdelay(ftn_delay);

	read_ret = s7760a_i2c_read(S7760A_SET_RW_COMMAND, &read_data[0]);
	mask_cen_data=read_data[0];
	read_data[0]=mask_cen_data&0x3f;
	FELICA_DBG("INFO: s7760a_read=> Read Buffer Data is [%d]\n",read_data[0]);
	FELICA_DBG("INFO: s7760a_read=> read_ret is [%d]\n", read_ret);

	ret=copy_to_user(buf, &read_data[0], sizeof(&read_data[0]));	//All goes well from copy_to_user to Cal Tool

	mutex_unlock(&lock);

	if (ret)
		{
			printk("ERROR: fail to Proccess Copy to user\n");
			return 0;
		}

	if(read_ret)
	{
		printk("INFO: Read Success, Mode:[%s] DATA[0x%02x] RESX[%s]\n",(access_mode==2)?"REGISTER":"EEPROM", read_data[0], ((mask_cen_data&0x80)==0)?"OFF":"ON");
		return read_ret;
	}
	else
	return 0;	
}

static int s7760a_i2c_write(unsigned char cmd, unsigned char set_data, int wait_flg)
{	
	
	int		i2c_ret		= I2C_FAILURE;
	unsigned char	txData[1]={set_data};
	
	struct i2c_msg msg[] = 
	{
		{
			.addr	= this_client->addr & cmd,
			.flags	= 0,
			.len	= 1,
			.buf	= txData,
		},
	};
	

	FELICA_DBG("INFO: s7760a_i2c_write=> client->adapter = [%lu]\n", (long unsigned int) this_client->adapter);
	FELICA_DBG("INFO: s7760a_i2c_write=> s7760a_i2c_write: Cmd = [%d], Writing Data = [%d], wait_flg = [%d]\n", cmd, set_data, wait_flg);	

	
	i2c_ret = i2c_transfer(this_client->adapter, msg, 1);
	
	if (wait_flg == NEED_WAIT_FLAG) 
		{
			msleep(I2C_WRITE_WAIT_TIME);
		}
	if (i2c_ret <= I2C_FAILURE) 
		{
			printk("ERROR: s7760a_i2c_write=> i2c_ret = %d\n", i2c_ret);			
			return 0;
		} 
	else 
		{
			FELICA_DBG("INFO: s7760a_i2c_write=> i2c_ret = %d\n", i2c_ret);			
			return 1;
		}
}

static int s7760a_ld_write(unsigned char cmd, unsigned char set_data, int wait_flg)
{
	int ret	= 0;
	FELICA_DBG("INFO: s7760a_ld_write: cmd = [%d], data = [%d], wait_flg = [%d]\n", cmd, set_data, wait_flg);
	ret = s7760a_i2c_write(cmd, set_data, wait_flg);
	return ret;
}


static int s7760a_write_process(unsigned char cmd, unsigned char set_data, int wait_flg)
{
	int				ret				= FALSE;
	unsigned char	try_count			= 0;
						
	for (try_count = 0; try_count < FELICA_PPC_I2C_TRY_COUNT; try_count++) 
		{
			ret = s7760a_ld_write(cmd, set_data, wait_flg);		
				if (ret > 0)
					break;
				else
					continue;				
		}	
	return ret;
}

ssize_t s7760a_write(struct file *file, const char *buf,
				size_t count, loff_t *pos)
{
	char		    cmd_data[1];					/* cmd_data	*/
	static 			DEFINE_MUTEX(lock);				/* mutex       */
	int ret=0;
	unsigned char cen_data=0;
	unsigned char write_data=0;
	FELICA_DBG("START: Felica s7760a Write\n");
	access_mode = count;

	mutex_lock(&lock);
	cen_data=s7760a_get_cen_data();

	if (copy_from_user(cmd_data, buf, sizeof(cmd_data))) 
	{
		mutex_unlock(&lock);
		printk("ERROR LOG:copy_from_user error\n");
		return -1;
	}
	write_data =(unsigned char) cmd_data[0]|cen_data;
		
		FELICA_DBG("INFO: s7760a_write=> Access Mode is [%s], DATA is [0x%02x]->byte type\n",access_mode==2?"Register Mode":"EEPROM Mode",write_data);

				
				gpio_tlmm_config(GPIO_CFG(S7760A_WP_GPIO, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE); //Code isAdded because of Malfunction of GPIO S7760A_WP_GPIO
		if((wp_gpio_flag==0)&(access_mode==3))
			gpio_set_value(S7760A_WP_GPIO,0);		
		FELICA_DBG("INFO: s7760a_write=> WP Gpio is set to [%d] (When set 0, enable Write data to EEPROM)\n",gpio_get_value(S7760A_WP_GPIO));


		ret= s7760a_write_process(S7760A_SET_RW_COMMAND, write_data,access_mode==2?0:1);
		FELICA_DBG("INFO: s7760a_write_process ret= [%d]\n",ret);
		
	gpio_set_value(S7760A_WP_GPIO,1);
	mutex_unlock(&lock);		
	if (ret) 
	{
		printk("INFO: Write Success, Mode:[%s] DATA[0x%02x] RESX[%s]\n",(access_mode==2)?"REGISTER":"EEPROM", (unsigned char) cmd_data[0], (cen_data==0)?"OFF":"ON");
		return 1;
	} 
	else 
	{
		printk("ERROR: Write Data Proccess fail\n");
		return 0;
	}
}


static int s7760a_write_ftn(unsigned char arg)
{
	int ret=0;
	unsigned char Set_Data[1]; 

	static DEFINE_MUTEX(lock);

	mutex_lock(&lock);
	Set_Data[0]=arg;
	
	gpio_tlmm_config(GPIO_CFG(S7760A_WP_GPIO, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE); //Code isAdded because of Malfunction of GPIO S7760A_WP_GPIO

		FELICA_DBG("INFO: Access Mode is [%s], DATA is [0x%02x]\n",access_mode==2?"Register Mode":"EEPROM Mode",Set_Data[0]);
		if((wp_gpio_flag==WP_GPIO_OFF)&&(access_mode==ACCESS_EEPROM_MODE))
		gpio_set_value(S7760A_WP_GPIO,0);		
		FELICA_DBG("INFO: s7760a_write=> WP Gpio is set to [%d] (When set 0, enable Write data to EEPROM)\n",gpio_get_value(S7760A_WP_GPIO));


		ret= s7760a_write_process(S7760A_SET_RW_COMMAND, Set_Data[0],access_mode==2?0:1);
		FELICA_DBG("INFO: s7760a_write_process ret= [%d]\n",ret);
		
	gpio_set_value(S7760A_WP_GPIO,1);
	
	mutex_unlock(&lock);		
	if (ret == 1) 
		FELICA_DBG("INFO: Write Data Proccess Success\n");
	else 
		printk("ERROR: Write Data Proccess fail\n");
	return ret;

}

static int s7760a_auto_cal_write_ftn(unsigned char arg)
{
	int ret=0;
	unsigned char Set_Data[1]; 
	static DEFINE_MUTEX(lock);

	mutex_lock(&lock);
	Set_Data[0]=arg|PORT_CONTROLL_RESX_ON;
	
	ret= s7760a_i2c_write(S7760A_SET_RW_COMMAND, Set_Data[0],0);	
	mutex_unlock(&lock);		
	if (ret == 1) 
		FELICA_DBG("INFO: Write Data[0x%02x] Proccess Success\n",arg);
	else 
		printk("ERROR: Write Data[0x%02x] Proccess fail\n",arg);
	return ret;

}

static int s7760a_reload_process(void)
{
	int					ret		= FALSE;
	unsigned char	try_count	= 0;
	FELICA_DBG("START: s7760a_reload_process\n");
	for (try_count = 0; try_count < FELICA_PPC_I2C_TRY_COUNT; try_count++) 
	{
		ret = s7760a_reload();
		if (ret > 0) 
				break;
		else 
				FELICA_DBG("ERROR LOG: s7760a_reload_process-> i2c_retry count = %d\n", try_count);
	}
	FELICA_DBG("END: s7760a_reload_process->  i2c_ret = %d\n",ret);
	return ret;
}



static int s7760a_reload(void)
{
	int ret = 0;
	FELICA_DBG("START: s7760a_reload\n");
	ret= s7760a_i2c_write(S7760A_RELOAD_MODE,0,0);
	FELICA_DBG("INFO: s7760a_reload: ret = %d\n", ret);
	return ret;
}



static int s7760a_change_mode(unsigned char mode, unsigned long mode_flag)
{
	int	ret	= I2C_FAILURE;
	unsigned char	cmd_txData[1]	= {0x00};
	struct i2c_msg msg[] = 
	{
		{
			.addr	= this_client->addr & mode,
			.flags	= mode_flag,
			.len	= 1,
			.buf	= cmd_txData,
		
		},
	};
	FELICA_DBG("INFO: s7760a_change_mode=> this_client->adapter->name = %s\n", this_client->adapter->name); 
	FELICA_DBG("INFO: s7760a_change_mode=> mode= 0x%02x \n",mode); 
	FELICA_DBG("INFO: s7760a_change_mode=> mode_flag = %lu\n", mode_flag); 
	
	ret = i2c_transfer(this_client->adapter, msg, 1);
	if (ret <= I2C_FAILURE) 
		{
			FELICA_DBG("ERROR LOG: s7760a_change_mode=> i2c_ret = %d", ret);			
			return 0;
		} 
	else 
		{
			FELICA_DBG("INFO: s7760a_change_mode=> i2c_ret = %d , return true \n", ret);	
			access_mode=mode_flag+2;
			return 1;
		}
}





static int s7760a_change_mode_ftn(int ACCEESS_MODE_TO)
{

	int ret=0;
	
	FELICA_DBG("START: CHANGE TO %s\n",ACCEESS_MODE_TO?"EEPROM MODE":"REGISTER MODE");  
	
	if(ACCEESS_MODE_TO)
		wp_gpio_flag=0;
	else
		wp_gpio_flag=1;
	
	ret=s7760a_change_mode(S7760A_ACCESS_CHANGE_MODE, ACCEESS_MODE_TO);

	if (ret) 
	{
		FELICA_DBG("INFO: CHANGE TO %s succeed\n",ACCEESS_MODE_TO?"EEPROM MODE":"REGISTER MODE");  

		
	} 
	else 
	{
		printk("ERROR LOG: Fail to CHANGE %s\n",ACCEESS_MODE_TO?"EEPROM MODE":"REGISTER MODE");  
		
	}
	return ret;

}


static unsigned char s7760a_get_cen_data(void)
{	

	int ch_mode_rtn=0;
	int read_data=0;
	unsigned char CEN_data=0;
	ch_mode_rtn=s7760a_change_mode_ftn(access_mode-2);
	if (ch_mode_rtn)
		{
			FELICA_DBG("INFO: Change to %s Mode\n",access_mode==2?"REGISTER":"EEPROM");
			
		}
	else
		{
			printk("ERROR: Fail to changing %s Mode\n",access_mode==2?"REGISTER":"EEPROM");
			return 1;
		}
	//mdelay(ftn_delay);
	read_data=s7760a_read_ftn();	
	if(read_data<0)
		{
			printk("ERROR: Fail to Read Data\n");
			return 1;
		}
	else
	CEN_data=(unsigned char) read_data&CEN_MASK;
	FELICA_DBG("INFO: CEN_DATA: [0x%02x]\n",CEN_data);
	return CEN_data;

}



long s7760a_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	
	
	int ret;
	int cen_ret;
	int	reload_rtn;
	int read_data=0;
	int reload_ret=0;
	unsigned char read_CEN_data[1];
	unsigned char read_EEPROM_data[1];
	unsigned char read_REG_data[1];
	unsigned char Cen_ON_OFF_PIN_status=0;	
	unsigned char EEPROM_data;
	unsigned char data_from_eeprom;
	unsigned char data_from_REG;
	unsigned char data_from_chip;
	unsigned char Set_EEPROM_data;
	unsigned char Set_REG_data;
	unsigned char data_from_arg;
	static DEFINE_MUTEX(lock);
	int mode_rtn;


	gpio_tlmm_config(GPIO_CFG(S7760A_WP_GPIO, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE); //Code isAdded because of Malfunction of GPIO S7760A_WP_GPIO



	
	FELICA_DBG("INFO: s7760a_ioctl: CMD = %u\n",cmd);				
	FELICA_DBG("INFO: s7760a_ioctl: ARG = %lu\n",arg);			
	

	switch (cmd) 
	{

	/* 1 : read E2PROM					*/
	case FELICA_PPC_READ_E2PROM:

	
	ret = 0;
	mode_rtn = 0;
	read_data=0;
	reload_rtn=0;
	read_EEPROM_data[0]=0;
	mutex_lock(&lock);
	
	/*Reload EEPROM*/
	reload_rtn=s7760a_reload_process();
	if (reload_rtn)
		{
			FELICA_DBG("INFO: Felica S7760A is Reloaded\n");
			
		}
	else
		{	printk("ERROR: Fail to reload Felica S7760A\n");
			return -1;
	
		}
	
	/*Change REG MODE*/
	gpio_set_value(S7760A_WP_GPIO,1);
	mode_rtn = s7760a_change_mode(S7760A_ACCESS_CHANGE_MODE,TO_REG);

	if(mode_rtn)
		{
			wp_gpio_flag=WP_GPIO_ON;
			access_mode=ACCESS_REGISTER_MODE;
			FELICA_DBG("INFO: Change Access Mode Success\n");	
		}
	else
		printk("ERROR: Change Access Mode Failed\n");
		
		ret = s7760a_i2c_read(S7760A_SET_RW_COMMAND, &read_EEPROM_data[0]);
		if(ret)
			{
				FELICA_DBG("INFO: Felica S7760A Read EEPROM Data Success\n");
				FELICA_DBG("INFO: EEPROM DATA=[0x%02x]\n",read_EEPROM_data[0]);
				read_data=(int) (read_EEPROM_data[0]&0x3f);
				
			}
		else
			{
				printk("ERROR: Felica S7760A CEN ON Fail\n");
				return -1;
			}
	mutex_unlock(&lock);


		
		return read_data; 

		

	break;
	
	/* 2 : read register		*/
	case FELICA_READ_REGISTER:
	ret = 0;
	mode_rtn = 0;
	read_data=0;
	reload_rtn=0;
	read_REG_data[0]=0;
	mutex_lock(&lock);
	
	/*Change REG MODE*/
	gpio_set_value(S7760A_WP_GPIO,1);
	mode_rtn = s7760a_change_mode(S7760A_ACCESS_CHANGE_MODE,TO_REG);

	if(mode_rtn)
		{
			wp_gpio_flag=WP_GPIO_ON;
			access_mode=ACCESS_REGISTER_MODE;
			FELICA_DBG("INFO: Change Access Mode Success\n");	
		}
	else
		printk("ERROR: Change Access Mode Failed\n");
		
		ret = s7760a_i2c_read(S7760A_SET_RW_COMMAND, &read_REG_data[0]);
		if(ret)
			{
				FELICA_DBG("INFO: Felica S7760A Read EEPROM Data Success\n");
				FELICA_DBG("INFO: EEPROM DATA=[0x%02x]\n",read_REG_data[0]);
				read_data=(int) (read_REG_data[0]&0x3f);
				
			}
		else
			{
				printk("ERROR: Felica S7760A CEN ON Fail\n");
				return -1;
			}
	mutex_unlock(&lock);

	return read_data; 
	break;
	
	/* 3 : write E2PROM					*/
	case FELICA_WRITE_E2PROM:	
		reload_ret=0;
		EEPROM_data=0;
		read_data=0;
		read_CEN_data[0]=0;
		Set_EEPROM_data=0;
		ret=0;
		data_from_arg=0;
		mutex_lock(&lock);
		
		/*Get port INFO from EEPROM*/
		reload_ret=s7760a_reload_process();
		if (reload_ret)
				printk("INFO: Felica S7760A is Reloaded\n");	
		else
			{
				printk("ERROR: Fail to reload Felica S7760A\n");
				mutex_lock(&lock);
				return 0;
				
			}
		gpio_set_value(S7760A_WP_GPIO,1);
		if(!s7760a_change_mode_ftn(TO_REG))
			{
				mutex_unlock(&lock);
				return 0;
			}
		read_data=s7760a_read_ftn();
		if(read_data<0)
			{
				mutex_unlock(&lock);
				return 0;
			}
		/*Set CEN(RESX/On,Off)  PIN*/
		read_CEN_data[0]=(unsigned char) read_data&0xc0;

		/*Set write data*/
		data_from_arg=(unsigned char) arg&0x3f;
		Set_EEPROM_data= data_from_arg | read_CEN_data[0];
		/*Access mode Change, to EEPROM*/
		if(!s7760a_change_mode_ftn(TO_EEPROM))
			{
				mutex_unlock(&lock);
				return 0;
			}
		wp_gpio_flag=WP_GPIO_OFF;
		ret=s7760a_write_ftn(Set_EEPROM_data);
			
		FELICA_DBG("%s: Write EEPROM Data proccessing %s\n",ret?"INFO":"ERROR",ret?"Completed":"Failed");
		if(!s7760a_change_mode_ftn(TO_REG))
			{
				mutex_unlock(&lock);
				return 0;
			}
		wp_gpio_flag=WP_GPIO_ON;
		access_mode=ACCESS_REGISTER_MODE;
		mutex_unlock(&lock);
		return ret;
		

	break;

	/* 4 : write register					*/
	case FELICA_WRITE_REGISTER:


		read_data=0;
		read_CEN_data[0]=0;
		Set_REG_data=0;
		ret=0;
		data_from_arg=0;
		mutex_lock(&lock);
		
		/*Get port INFO from REG*/
		
		gpio_set_value(S7760A_WP_GPIO,1);
		if(!s7760a_change_mode_ftn(TO_REG))
			{
				mutex_unlock(&lock);
				return 0;
			}
		read_data=s7760a_read_ftn();
		if(read_data<0)
			{
				mutex_unlock(&lock);
				return 0;
			}
		/*Set CEN(RESX/On,Off)  PIN*/
		read_CEN_data[0]=(unsigned char) read_data&0xc0;

		/*Set write data*/
		data_from_arg=(unsigned char) arg&0x3f;
		Set_REG_data= data_from_arg | read_CEN_data[0];
		ret=s7760a_write_ftn(Set_REG_data);
			
		FELICA_DBG("%s: Write EEPROM Data proccessing %s\n",ret?"INFO":"ERROR",ret?"Completed":"Failed");

		wp_gpio_flag=WP_GPIO_ON;
		access_mode=ACCESS_REGISTER_MODE;
		mutex_unlock(&lock);
		return ret;

	break;

	//5: CHANGE_TO_REGISTER_MODE
	case CHANGE_TO_REGISTER_MODE:
		mutex_lock(&lock);
		FELICA_DBG("START: CHANGE_TO_REGISTER_MODE\n");  
		ret	= 0;
		gpio_set_value(S7760A_WP_GPIO,1);
		wp_gpio_flag=WP_GPIO_ON;
		ret=s7760a_change_mode(S7760A_ACCESS_CHANGE_MODE, TO_REG);
		mutex_unlock(&lock);
		if (ret) 
		{
			printk("INFO: CHANGE TO REGISTER MODE  Success\n");
			access_mode=ACCESS_REGISTER_MODE;
			
		} 
		else 
		{
			printk("ERROR:CHANGE_TO_REGISTER_MODE ret = %d\n", ret);
			
		}
		FELICA_DBG("CHANGE_TO_REGISTER_MODE==> wp gpio is %s\n",gpio_get_value(S7760A_WP_GPIO)?"ON":"OFF");
		return ret;
		
	break;

	//6: CHANGE_TO_EEPROM_MODE	
	case CHANGE_TO_EEPROM_MODE:
		ret=0;
		FELICA_DBG("START: CHANGE_TO_EEPROM_MODE\n");		
		mutex_lock(&lock);
		gpio_set_value(S7760A_WP_GPIO,0);
		ret=s7760a_change_mode(S7760A_ACCESS_CHANGE_MODE, TO_EEPROM);
		if (ret) 
		{	
			printk("INFO: CHANGE TO EEPROM MODE  Success\n");
			wp_gpio_flag=WP_GPIO_OFF;
			access_mode=ACCESS_EEPROM_MODE;
		} 
		else 
		{
			wp_gpio_flag=WP_GPIO_ON;
			access_mode=ACCESS_REGISTER_MODE;
			printk("ERROR:CHANGE_TO_EEPROM_MODE ret = %d\n", ret);
			
		}
		mutex_unlock(&lock);
		FELICA_DBG("CHANGE_TO_EEPROM_MODE==> wp gpio is %s\n",gpio_get_value(S7760A_WP_GPIO)?"ON":"OFF");
		return ret;
	break;
		
	//7: FELICA_CEN_ON	-> GPIO
	case FELICA_CEN_ON:
		cen_ret = 0;
		mutex_lock(&lock);
			gpio_set_value(FELICA_CEN_GPIO,1);			
			cen_ret=gpio_get_value(FELICA_CEN_GPIO);
			FELICA_DBG("FELICA CEN GPIO(66) is %s\n",(cen_ret==1)?"High.":"Low.");
			if(cen_ret==1)
				printk("INFO: Felica S7760A CEN ON Success\n");
			else
				printk("ERROR: Felica S7760A CEN ON Fail\n");
		mutex_unlock(&lock);
		return cen_ret;   //ret=1 => CEN ON  | ret=0 =>  CEN ON operation fail
	break;
	
	//8: FELICA_CEN_OFF	-> GPIO
	case FELICA_CEN_OFF:
		ret = 0;
			gpio_set_value(FELICA_CEN_GPIO,0);			
			cen_ret=gpio_get_value(FELICA_CEN_GPIO);
			FELICA_DBG("FELICA CEN GPIO(66) is %s\n",(cen_ret==1)?"High.":"Low.");
			if(cen_ret==0)
			{
				ret=1;
				printk("INFO: Felica S7760A CEN OFF Success\n");
			}
			else
			{
				ret=0;
				printk("ERROR: Felica S7760A CEN OFF Fail\n");
			}
		mutex_unlock(&lock);
		return ret;		//ret=1 => CEN OFF  | ret=0 =>  CEN OFF operation fail
	break;
	
	//9: FELICA_PON_ON	
	case FELICA_PON_ON:
		ret=0;
		mutex_lock(&lock);
		printk("START: S7760A PON ON\n");
		gpio_set_value(PON_GPIO,1);		
		FELICA_DBG("FELICA PON GPIO(65) is %s\n",(gpio_get_value(PON_GPIO)==1)?"High.":"Low.");
		mutex_unlock(&lock);
		return gpio_get_value(PON_GPIO);
	break;	

	//10: FELICA_PON_OFF
	case FELICA_PON_OFF:
	ret=0;
		mutex_lock(&lock);
		printk("START: S7760A PON OFF\n"); 
		gpio_set_value(PON_GPIO,0);
		FELICA_DBG("FELICA PON GPIO(65) is %s\n",(gpio_get_value(PON_GPIO)==1)?"High.":"Low.");
		mutex_unlock(&lock);
		return (gpio_get_value(PON_GPIO)==0)?1:0;
	break;
	
	/* 11: RELOAD				*/
	case FELICA_PPC_RELOAD:
		ret=0;
		mutex_lock(&lock);
		FELICA_DBG("START: S7760A_RELOAD => RELOAD FROM EEPROM TO REGISER\n"); 
		ret=s7760a_reload_process();
		if (ret)
			{
				printk("INFO: Felica S7760A is Reloaded\n");
				
			}
		else
			printk("ERROR: Fail to reload Felica S7760A\n");
		mutex_unlock(&lock);
		return ret;
		
		break;	

	//12: FELICA_S7760A_GET_CEN_INFO_FROM_EEPROM
	case (FELICA_S7760A_GET_CEN_INFO):
		ret = 0;
		mode_rtn = 0;
		reload_rtn=0;
		mutex_lock(&lock);
		/*Reload EEPROM for getting CEN DATA*/
		
		reload_rtn=s7760a_reload_process();
		if (reload_rtn)
			{
				FELICA_DBG("INFO: Felica S7760A is Reloaded\n");
				
			}
		else
			{	printk("ERROR: Fail to reload Felica S7760A\n");
				return -1;
		
			}
		/*Change REG MODE*/
		gpio_set_value(S7760A_WP_GPIO,1);
		mode_rtn = s7760a_change_mode(S7760A_ACCESS_CHANGE_MODE,TO_REG);
		if(mode_rtn)
			{
				wp_gpio_flag=WP_GPIO_ON;
				access_mode=ACCESS_REGISTER_MODE;
				FELICA_DBG("INFO: Change Access Mode Success\n");	
			}
		else
			printk("ERROR: Change Access Mode Failed\n");
			ret = s7760a_i2c_read(S7760A_SET_RW_COMMAND, &read_CEN_data[0]);
			if(ret)
				{
					FELICA_DBG("INFO: Felica S7760A CEN ON Success\n");
					Cen_ON_OFF_PIN_status=read_CEN_data[0]&0xc0;
					FELICA_DBG("INFO: REGISTER DATA=[%x]\n",read_CEN_data[0]);
					if (Cen_ON_OFF_PIN_status == 0) {
						printk("INFO: CEN is now [%s]\n","LOCK");
					}
				}
			else
				{
					printk("ERROR: Felica S7760A CEN ON Fail\n");
					return -1;
				}
		mutex_unlock(&lock);
			return Cen_ON_OFF_PIN_status==0?0:1;  //Cen_status=1 => CEN is now Unlock (CEN ON)  | Cen_status=0 =>  CEN is now lock (CEN OFF)
	break;	

//13 : FELICA_S7760A_CEN_DATA_WRITE_TO_EEPROM          
	case (CEN_EEPROM_Write):
		reload_ret=0;
		EEPROM_data=0;
		read_data=0;
		data_from_eeprom=0;
		Set_EEPROM_data=0;
		ret=0;
		data_from_arg=0;
		mutex_lock(&lock);
		/*Get port INFO from EEPROM*/
		reload_ret=s7760a_reload_process();
		if (reload_ret)
				FELICA_DBG("INFO: Felica S7760A is Reloaded\n");	
		else
			{
				printk("ERROR: Fail to reload Felica S7760A\n");
				mutex_lock(&lock);
				return 0;
			}
		gpio_set_value(S7760A_WP_GPIO,1);
		if(!s7760a_change_mode_ftn(TO_REG))
			{
				mutex_unlock(&lock);
				return 0;
			}
		read_data=s7760a_read_ftn();
		if(read_data<0)
			{
				mutex_unlock(&lock);
				return 0;
			}
		data_from_eeprom=(unsigned char) read_data&0x3f;
		/*Set CEN(RESX/On,Off)  PIN*/
		data_from_arg=(unsigned char) arg<<7;
		Set_EEPROM_data= data_from_arg | data_from_eeprom;
		if(!s7760a_change_mode_ftn(TO_EEPROM))
			{
				mutex_unlock(&lock);
				return 0;
			}
		wp_gpio_flag=WP_GPIO_OFF;
		ret=s7760a_write_ftn(Set_EEPROM_data);
		printk("%s: Write EEPROM Data proccessing %s\n",ret?"INFO":"ERROR",ret?"Completed":"Failed");
		if(!s7760a_change_mode_ftn(TO_REG))
			{
				mutex_unlock(&lock);
				return 0;
			}
		wp_gpio_flag=WP_GPIO_ON;
		access_mode=ACCESS_REGISTER_MODE;
		mutex_unlock(&lock);
		return ret;
		break;
		
//14 : FELICA_S7760A_CEN_DATA_WRITE_TO_REGISTER
	case (CEN_TEMP_LOCK):
		read_data=0;
		Set_REG_data=0;
		ret=0;
		mutex_lock(&lock);
		gpio_set_value(S7760A_WP_GPIO,1);
		if(!s7760a_change_mode_ftn(TO_REG))
			{
				mutex_unlock(&lock);
				return 0;
			}
		read_data=s7760a_read_ftn();
		if(read_data<0)
			{
				mutex_unlock(&lock);
				return 0;
			}
		data_from_REG=(unsigned char) read_data&0x3f;
		data_from_arg=(unsigned char) arg<<6;
		Set_REG_data= data_from_arg | data_from_REG;
		ret=s7760a_write_ftn(Set_REG_data);
		if(ret)
			{
				if(data_from_arg==1)
					{
						printk("INFO: Temporary LOCK SUCCESS => RESX(L) | ON/OFF(H)\n");	

					}
				if(data_from_arg==2||data_from_arg==3)
					{
						printk("INFO: UNLOCK To REGISTER SUCCESS => RESX(H) | ON/OFF(L)\n");	

					}
				else
					{
						printk("INFO: LOCK To REGISTER SUCCESS => RESX(L) | ON/OFF(L)\n");	

					}
			}
		printk("%s: Write REG Data proccessing %s\n",ret?"INFO":"ERROR",ret?"Completed":"Failed");
		wp_gpio_flag=WP_GPIO_ON;
		access_mode=ACCESS_REGISTER_MODE;
		mutex_unlock(&lock);
		return ret;
	break;
	
	/* 15 : autowrite	*/
		case (FELICA_AUTOWRITE):
			ret=0;		
			ret=s7760a_auto_cal_write_ftn(arg);
			return ret;
		break;


	//16: FELICA_CEN_WRITE	
	case FELICA_CEN_WRITE:
		ret = 0;
		mode_rtn = 0;
		read_data=0;
		data_from_chip=0;
		mutex_lock(&lock);
		
		mode_rtn = s7760a_change_mode(S7760A_ACCESS_CHANGE_MODE,access_mode-2);
		if(mode_rtn)
			{
				FELICA_DBG("INFO: Change Mode Success\n");	
			}
		else
			FELICA_DBG("ERROR: Change Mode Failed\n");
		gpio_set_value(S7760A_WP_GPIO,wp_gpio_flag);
		
			read_data=s7760a_read_ftn();
			FELICA_DBG("read_data=%d\n",read_data);
			if(read_data==-1)
				return -1;
			data_from_chip=(unsigned char) read_data&0x3f;
			FELICA_DBG("data_from_chip=0x%02x\n",data_from_chip);
			ret = s7760a_write_process(S7760A_SET_RW_COMMAND, (data_from_chip)|(unsigned char)(arg<<6),NO_WAIT_FLAG);


			if(ret==1)
				printk("INFO: Felica S7760A CEN OFF Success\n");
			else
				printk("ERROR: Felica S7760A CEN OFF Fail\n");
		mutex_unlock(&lock);
		return ret;		//ret=1 => CEN WRITE  | ret=0 =>  CEN WRITE operation fail

	break;

	
	//17: FELICA_CEN_READ
	case FELICA_CEN_READ:

	ret = 0;
			mode_rtn = 0;
			read_data=0;
			mutex_lock(&lock);
			
			mode_rtn = s7760a_change_mode(S7760A_ACCESS_CHANGE_MODE,access_mode-2);
			if(mode_rtn)
				{
					FELICA_DBG("INFO: Change Mode Success\n");	
				}
			else
				FELICA_DBG("ERROR: Change Mode Failed\n");
			gpio_set_value(S7760A_WP_GPIO,wp_gpio_flag);
			
				read_data=s7760a_read_ftn();
				FELICA_DBG("read_data=0x%02x\n",read_data);
				if(read_data==-1)
					return -1;
				ret=(read_data&0xc0)>>6;
				mutex_unlock(&lock);
				FELICA_DBG("ret=0x%02x\n",ret);
				return ret;

	break;

	/* command error */
	default:
		printk("ERROR LOG:DEFAULT cmd = %d\n", cmd);
		return -EINVAL;
	}
	return (0);
}


static struct file_operations s7760a_fileops = {
	.owner	 = THIS_MODULE,
	.open	 = s7760a_open,
	.release = s7760a_release,
	.read	 = s7760a_read,
	.write	 = s7760a_write,
	.unlocked_ioctl = s7760a_ioctl, 
};

static int s7760a_probe(struct i2c_client *client, const struct i2c_device_id * devid)
{
	struct s7760a_data *s7760a;
	int alloc_ret = 0;
	
	dev_t dev = MKDEV(MISC_MAJOR, 0);
	
	FELICA_DBG("Probe: Felica s7760a\n");
	
	s7760a = kzalloc(sizeof(struct s7760a_data), GFP_KERNEL);
	if (!s7760a) {
		printk("ERROR: Fail to allocate s7760a in Kernel\n");
		return -ENOMEM;								/* got no mem */
	}
	
	i2c_set_clientdata(client, s7760a);
	
	this_client = client;
	
	s7760a->input_dev = input_allocate_device();
	
	alloc_ret = alloc_chrdev_region(&dev , 0 , D_S7760A_DEVS, D_S7760A_DEV_NAME);
	if (alloc_ret) {
		FELICA_DBG("Felica s7760a probe: alloc_chrdev_region\n");
		return alloc_ret;
	}
	
	cdev_init(&s7760a_cdev, &s7760a_fileops);
	s7760a_cdev.owner = THIS_MODULE;
	
	cdev_add(&s7760a_cdev, dev, D_S7760A_DEVS);
	
	device_create(felica_class, NULL, dev, NULL, D_S7760A_DEV_NAME);
	
	printk("Felica s7760a: probe \n");
	
	return (0);
}


static int s7760a_open( struct inode *inode, struct file *filp )
{
	FELICA_DBG("OPEN: Felica S7760A Driver\n");
	return 0;
}

static int s7760a_release( struct inode *inode, struct file *filp )
{
	FELICA_DBG("RELEASE: Felica S7760A Driver\n");
	return 0;
}

static int s7760a_remove(struct i2c_client *client)
{
	struct s7760a_data *s7760a = i2c_get_clientdata(client);
	
	FELICA_DBG("REMOVE: Remove Felica S7760A Module\n");
	
	input_unregister_device(s7760a->input_dev);
	
	kfree(s7760a);
	
	printk("Felica S7760A Module Remove proccess is completed\n");
	
	return (0);
}




static const struct i2c_device_id s7760a_id[] = {
	{ "s7760a", 0 },
	{ }
};

static struct i2c_driver s7760a_dev_driver =
{

	.probe	 = s7760a_probe,
	.id_table = s7760a_id,
	.remove	 = s7760a_remove,
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "s7760a",
	},
	
};

static __init int s7760a_init(void)
{
	int ret;
	
	felica_class = class_create(THIS_MODULE, "s7760a");
	
	if (IS_ERR(felica_class))
		{
			printk("ERROR: Felica s7760a init error\n");
			return PTR_ERR(felica_class);
		}
	
	ret = i2c_add_driver(&s7760a_dev_driver);
	FELICA_DBG("Adding Felica S7760A Module in i2c driver list is %s, ret=%d\n ",ret==0?"completed":"failed",ret); 
	if( ret < 0 )
		return ret;
	
	return 0;
}

static void __exit s7760a_exit(void)
{
	class_destroy( felica_class );
	i2c_del_driver(&s7760a_dev_driver);
	return;
}


MODULE_LICENSE("GPL v2");

module_init(s7760a_init);
module_exit(s7760a_exit);



/* drivers/input/touchscreen/melfas_ts.c
 *
 * Copyright (C) 2010 Melfas, Inc.
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

/*=========================================================
	owner : dhyang(p11774)
	Build 1 - 2011.11.28
		1. compile error revised.
		
	Build 2 - 2011.12.07
		1. rearrange
		2. new feature: SKY_ANDROID_ICS, eventHub operation is changed for ICS
		
	Build 3 - 2011.12.08
		1. Sirius LTE PT10 Only
			LDO control pin for Touch IC analog Power is GPIO (PIN 56).
			after PT11, GPIO pin is 11 same with Vega Premia Verizon.
			
	Build 4 - 2011.12.15
	    1. Touch response time update for test code (By dhyang(P11774))
	        -> apply misc_driver. fops.open, fops.ioctl
	           TOUCH_IOCTL_EVENT_TIME_GET

	Buildl 5 - 2011.12.18 ~ 19
		1. Touch firmware download Enable and test
		2. Touch core firmware version -> 35(0x21)
		3. ioctl: get firmware version interface

	Build 6 - 2011.12.20 ~
		1. create mms100_download_porting.c file
		   -> IRQ pin control, power manager

    Build 7 - 2012.01.18
	    1. If no touch, call null input_report_abs.
		2. download API 

	Build 8 - 2012.01.25
		1. ISP download complete - for Vega Premia V WS10_WTR only
		2. ICS Multi-touch Protocol 
	           
	Build 9 - 2012.01.30
		1. Touch EventHub - protocol B applied.

	Build 10 - 2012.01.31
		1. ISC download
		  - change SW I2C(Melfas Original) to HW I2C (pantech Code)
		  - ISC download debug complete

	Build 11 - 2012.02.01
		1. Code Clean.

	Build 12 - 2012.02.08
		1. add : misc deregister
		2. add : TOUCH_IO (for using touch_monitor.apk)
		
	Build 13 - 2012.02.28
		1. add : process command
		
	Build 14 - 2012.03.15
		1. add : pwr ctrl for suspend&resume structure / FW & download method are updated

	Build 15 - 2012.03.20
		1. add : ESD case algorithm / init_global() 
		
	Build 16 - 2012.04.16
		1. add : TESTMODE #4648# (CM DELTA)
		2. code clean(disabled deep sleep, etc)

	Build 17 - 2012.07.03
		1. add : IOCTL write return value (due to EF44S touch&camera issue)
	
	Build 18 - 2012.07.20
		1. add : power GPIO free (due to power gpio's EBUSY case)
		2. fix : flush touch event routine (suspend)
	
	Build 19 - 2012.07.21
		1. add : chip reset routine to avoid i2c failure case.   
		2. code clean.

  =========================================================*/

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include "melfas_ts.h"

#include <linux/irq.h>
#include <mach/gpio.h>
#include <linux/miscdevice.h>
#include <linux/pm.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h> //TOUCH_REACTION_RATE
#include <linux/regulator/consumer.h>

// for mulit-touoch protocol b
#include <linux/input/mt.h>

//============================================================
//	Model Feature - wcjeong(p11309)
//============================================================
#define GPIO_TOUCH_CHG_INT		11
#define IRQ_TOUCH_INT			gpio_to_irq(GPIO_TOUCH_CHG_INT)

//	touch ic digital power for i2c - wcjeong(p11309) 
#define TOUCH_POWER_DVDD		"8921_lvs4"

//	touch ic analog power - wcjeong(p11309) 
//		if not, using pmic regulator
#define TOUCH_POWER_AVDD_USING_EXTRA_LDO

#ifdef TOUCH_POWER_AVDD_USING_EXTRA_LDO
	#if (defined(CONFIG_MACH_MSM8960_SIRIUSLTE) && (BOARD_VER == PT10))
		#define GPIO_TOUCH_POWER 	56
	#else 
		#define GPIO_TOUCH_POWER 	51
	#endif
#else
	#define TOUCH_POWER_AVDD	"8921_l17"
#endif

#define SKY_ANDROID_ICS					1
#define SKY_MULTI_TOUCH_PROTOCOL_B		1

#define DEBUG_KERN_ERR 1
#define DEBUG_KERN_MSG 1

#ifdef DEBUG_KERN_ERR
#define dbg_kern(fmt, args...) printk(KERN_ERR "[ ==> TOUCH ] " fmt, ##args)
#else 
#define dbg_kern(fmt, args...) 
#endif

#ifdef DEBUG_KERN_MSG
#define dbg(fmt, args...) printk("[ ==> TOUCH ] " fmt, ##args)
#else 
#define dbg(fmt, args...) 
#endif

#define SKY_PROCESS_CMD_KEY 1//TOUCH_REACTION_RATE
#define TOUCH_IO 1
//============================================================
#define MAX_CH 364
#ifdef TOUCH_IO
int CM_DELTA[MAX_CH] = {0,};
int counter = 0;
#define TXch 26
#define RXch 14
#define MELFAS_DELTA_MODE 5010
#define MELFAS_REFERENCE_MODE 5011
#define UNIVERSAL_CMD 0xA0
#define UNIVCMD_ENTER_TEST_MODE 0x40
#define UNIVCMD_EXIT_TEST_MODE 0x4F
#define UNIVCMD_TEST_CM_DELTA 0x41
#define UNIVCMD_GET_PIXEL_CM_DELTA 0x42
#define UNIVERSAL_CMD_RESULT_SIZE 0xAE
#define UNIVERSAL_CMD_RESULT 0xAF


#endif //TOUCH_IO

#define TS_MAX_Z_TOUCH				255
#define TS_MAX_W_TOUCH				100

#define TS_MAX_X_COORD 				720
#define TS_MAX_Y_COORD 				1280

#define FW_VERSION					0x21

#define TS_READ_START_ADDR 			0x0F
#define TS_READ_START_ADDR2 		0x10
#define TS_READ_VERSION_ADDR		0xF0
#define TS_READ_VERSION_CUST_ADDR	0xF6

#define TS_READ_REGS_LEN 			66
#define MELFAS_MAX_TOUCH			5

#define SET_DOWNLOAD_BY_GPIO		1
#define CONFIG_HAS_EARLYSUSPEND		1

#define ANTI_ESD 1

#if SET_DOWNLOAD_BY_GPIO
#include "mms100_ISC_download.h"
#endif // SET_DOWNLOAD_BY_GPIO

struct muti_touch_info
{
	int strength;
	int width;	
	int posX;
	int posY;
};

struct melfas_ts_data 
{
	uint16_t addr;
	struct i2c_client *client; 
	struct input_dev *input_dev;
	struct melfas_tsi_platform_data *pdata;
	struct work_struct  work;
	uint32_t flags;
	int (*power)(int on);
	struct early_suspend early_suspend;
};
struct melfas_ts_data *ts_g = NULL;
#ifdef ANTI_ESD
int ESD_CNT = 0;
#endif
int init_hw_setting(void);
void off_hw_setting(void);
static void init_global(struct melfas_ts_data *ts );

#ifdef SKY_PROCESS_CMD_KEY
static long ts_fops_ioctl(struct file *filp,unsigned int cmd, unsigned long arg);
static int ts_fops_open(struct inode *inode, struct file *filp);
static int diag_debug(int command);
#endif

#ifdef TOUCH_IO
static int open(struct inode *inode, struct file *file);
static int release(struct inode *inode, struct file *file);
static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *ppos);
static ssize_t read(struct file *file, char *buf, size_t count, loff_t *ppos);
static long ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int __devinit melfas_ts_init(void);
static void __exit melfas_ts_exit(void);
static int melfas_ts_remove(struct i2c_client *client);
#endif // TOUCH_IO

#ifdef SKY_PROCESS_CMD_KEY
static struct file_operations ts_fops = {
	.owner = THIS_MODULE,
	.open = ts_fops_open,
	.unlocked_ioctl = ts_fops_ioctl, // mirinae
};

static struct miscdevice touch_event = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "touch_fops",
	.fops = &ts_fops,
};

static int ts_fops_open(struct inode *inode, struct file *filp)
{
//	filp->private_data = ts;
	return 0;
}

typedef enum {	
	TOUCH_IOCTL_READ_LASTKEY=1001,	
	TOUCH_IOCTL_DO_KEY,	
	TOUCH_IOCTL_RELEASE_KEY, 
	TOUCH_IOCTL_RESTART=1006,
#if SET_DOWNLOAD_BY_GPIO
	TOUCH_IOCTL_SENSOR_X = 2005,
	TOUCH_IOCTL_SENSOR_Y,
	TOUCH_IOCTL_READ_IC_VERSION = 2008,
	TOUCH_IOCTL_READ_FW_VERSION,	
	TOUCH_IOCTL_START_UPDATE,
	TOUCH_IOCTL_SELF_TEST,
	TOUCH_IOCTL_DIAGNOSTIC_MIN_DEBUG,
	TOUCH_IOCTL_DIAGNOSTIC_MAX_DEBUG,
#endif
	TOUCH_IOCTL_OFF = 3002,
	TOUCH_IOCTL_EVENT_TIME_GET  = 4001, //TOUCH_REACTION_RATE
} TOUCH_IOCTL_CMD;


#ifdef CONFIG_HAS_EARLYSUSPEND
static void melfas_ts_early_suspend(struct early_suspend *h);
static void melfas_ts_late_resume(struct early_suspend *h);
#endif


static struct muti_touch_info g_Mtouch_info[MELFAS_MAX_TOUCH];

#if ( SKY_ANDROID_ICS & SKY_MULTI_TOUCH_PROTOCOL_B )
int g_track_id[MELFAS_MAX_TOUCH]= {0,};
#endif

static struct timespec tspec; //TOUCH_REACTION_RATE
static int axis_origin;
static int diagnostic_min =0;
static int diagnostic_max =0;


static long ts_fops_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{

#if SET_DOWNLOAD_BY_GPIO	
	int ret = 0;
	uint8_t buf[13] = {0,};
#endif
	void __user *argp = (void __user *)arg;
	long event_time; //TOUCH_REACTION_RATE
 
	//dbg("ts_fops_ioctl(%d, %d) \n",(int)cmd,(int)arg);
    
	switch (cmd) 
	{
	case TOUCH_IOCTL_READ_LASTKEY:
		break;
	case TOUCH_IOCTL_DO_KEY:
		dbg("TOUCH_IOCTL_DO_KEY  = %d\n",(int)argp);			
		if ( (int)argp == KEY_NUMERIC_STAR )
			input_report_key(ts_g->input_dev, 0xe3, 1);
		else if ( (int)argp == KEY_NUMERIC_POUND )
			input_report_key(ts_g->input_dev, 0xe4, 1);
		else
			input_report_key(ts_g->input_dev, (int)argp, 1);
			input_sync(ts_g->input_dev); 
		break;
	case TOUCH_IOCTL_RELEASE_KEY:		
		dbg("TOUCH_IOCTL_RELEASE_KEY  = %d\n",(int)argp);
		if ( (int)argp == KEY_NUMERIC_STAR )
			input_report_key(ts_g->input_dev, 0xe3, 0);
		else if ( (int)argp == KEY_NUMERIC_POUND )
			input_report_key(ts_g->input_dev, 0xe4, 0);
		else
			input_report_key(ts_g->input_dev, (int)argp, 0);
			input_sync(ts_g->input_dev); 
		break;		

#if SET_DOWNLOAD_BY_GPIO
	case TOUCH_IOCTL_READ_IC_VERSION:
		buf[0] = TS_READ_VERSION_ADDR;
		ret = i2c_master_send(ts_g->client, (const char *) buf, 1);
		if(ret < 0)
		{
			dbg_kern("melfas_ts_work_func : i2c_master_send [%d]\n", ret);
		}

		ret = i2c_master_recv(ts_g->client, (char *) buf, 13);
		if (ret<0) dbg_kern("melfas_ts_work_func : i2c_master_recv [%d]\n", ret);

		dbg(" melfas version: %u %u %u %u %u %u %u %u %u %u %u %u %u\n", 
			buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], 
			buf[7], buf[8], buf[9], buf[10], buf[11], buf[12] );

		if (copy_to_user(argp, &buf[4], 1))
		{
			printk("Copy error!!  \n");
			//return -EFAULT;
		}
		break;	
	case TOUCH_IOCTL_READ_FW_VERSION: 

		buf[0] = TS_READ_VERSION_ADDR;
		ret = i2c_master_send(ts_g->client, (const char *) buf, 1);
		if(ret < 0)
		{
			dbg_kern("melfas_ts_work_func : i2c_master_send [%d]\n", ret);
		}

		ret = i2c_master_recv(ts_g->client, (char *) buf, 13);
		if (ret<0) dbg_kern("melfas_ts_work_func : i2c_master_recv [%d]\n", ret);

		dbg(" melfas version: %u %u %u %u %u %u %u %u %u %u %u %u %u\n", 
			buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], 
			buf[7], buf[8], buf[9], buf[10], buf[11], buf[12] );

		if (copy_to_user(argp, &buf[5], 1))
		{
			printk("Copy error!!  \n");
		    //return -EFAULT;
		}

		buf[0] = TS_READ_VERSION_CUST_ADDR;
		ret = i2c_master_send(ts_g->client, (const char *) buf, 1);
		if(ret < 0)
		{
			dbg_kern("melfas_ts_work_func : i2c_master_send [%d]\n", ret);
		}

		ret = i2c_master_recv(ts_g->client, (char *) buf, 13);
		if (ret<0) dbg_kern("melfas_ts_work_func : i2c_master_recv [%d]\n", ret);		

		dbg(" melfas version 2: %u %u %u %u %u %u %u %u %u %u %u %u %u\n", 
			buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], 
			buf[7], buf[8], buf[9], buf[10], buf[11], buf[12] );
		
		break;
		
	case TOUCH_IOCTL_START_UPDATE: 

		buf[0] = TS_READ_VERSION_ADDR;
		ret = i2c_master_send(ts_g->client, (const char *) buf, 1);
		if(ret < 0)
		{
			dbg_kern("melfas_ts_work_func : i2c_master_send [%d]\n", ret);
		}
		ret = i2c_master_recv(ts_g->client, (char *) buf, 13);
		disable_irq(ts_g->client->irq);
		ret = mms100_download(ts_g->client);

		if (ret != 0) {
			dbg_kern("SET Download Fail - error code [%d]\n", ret);
		}
		else {			
			enable_irq(ts_g->client->irq);			
		}	
		break;

#endif

    case TOUCH_IOCTL_EVENT_TIME_GET:  //TOUCH_REACTION_RATE
             
	      event_time = (tspec.tv_sec * USEC_PER_MSEC) + (tspec.tv_nsec / NSEC_PER_MSEC);	 
		  if (copy_to_user(argp, &event_time, sizeof(event_time)))
          {
           	    printk("Copy error!!  \n");
		    //return -EFAULT;
          }
          dbg("Report Kernel event time: %ld \n",event_time);	  
		break;
		
	case TOUCH_IOCTL_SENSOR_X:
		{
			int send_data;
			send_data = RXch;
			counter = 0; // init for the next test
			if (copy_to_user(argp, &send_data, sizeof(send_data)))
				return false;
		}
		break;

	case TOUCH_IOCTL_SENSOR_Y:
		{
			int send_data;
			send_data = TXch;

			if (copy_to_user(argp, &send_data, sizeof(send_data)))
				return false;
		}
		break;
	
	case TOUCH_IOCTL_SELF_TEST : 
	{
		if(counter == 0) 
		{
			if(!diag_debug(5010))
			{
				printk("diag_debug error!\n");
				if(!diag_debug(5011)) //retry the test & this prevents untouchable state.
				{
					printk("diag_debug error!\n");
					return false;
				}
				return true; //return true for enabling test in case of missing interrupt
			}
			diagnostic_min = 200;
			diagnostic_max = 350;
		
			if (copy_to_user(argp, CM_DELTA, sizeof(int) * (MAX_CH)))
			{
				return false; 
			}
			counter++;
			return true; 
			break;
		}
		else
		{
			if(!diag_debug(5011))
			{
				printk("diag_debug error!\n");
				return false;
			}
			return true;
			break;
		}
		
	}
	case TOUCH_IOCTL_RESTART :
	{
		diag_debug(5011);
		break;
	}
		
	case TOUCH_IOCTL_DIAGNOSTIC_MIN_DEBUG:
		return diagnostic_min;
		break;
			
	case TOUCH_IOCTL_DIAGNOSTIC_MAX_DEBUG:
		return diagnostic_max;
		break;
		
	case TOUCH_IOCTL_OFF :
	{
		printk("Touch off \n");
		gpio_set_value(GPIO_TOUCH_POWER, 0); 
		break;
	}

		
	default:
		break;
	}

 	//unlock_kernel();  // mirinae_test
    	
	return true;
}
#endif

#ifdef TOUCH_IO

static struct file_operations fops = 
{
	.owner =    THIS_MODULE,
	.unlocked_ioctl = ioctl,  // mirinae
	.read = read,	
	.write = write,	
	.open = open,	
	.release = release,
};

static struct miscdevice touch_io = 
{
	.minor =    MISC_DYNAMIC_MINOR,
	.name =     "qt602240",
	.fops =     &fops
};

typedef struct {
	int* value;
	uint8_t size;
}config_table_element;

static int open(struct inode *inode, struct file *file) 
{
	return 0; 
}
static int release(struct inode *inode, struct file *file) 
{
	return 0; 
}
static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{	
	return 1; //EF44S camera issue
}
static ssize_t read(struct file *file, char *buf, size_t count, loff_t *ppos)
{	
	return 0; 
}

typedef enum 
{
	DIAG_DEBUG = 502,
	GET_TOUCH_CONFIG = 504, 
	SET_TOUCH_CONFIG = 505
}CONFIG_CMD;



static int diag_debug(int command) 
{
	
	int i = 0;
	int r, t = 0;
	uint16_t cmdata = 0;
	uint8_t master_write_buf[100] = {0,};
	uint8_t master_read_buf_array[200] = {0,};
	unsigned int check_time=0;
	
	//printk( "\n======================================================================\n");
	//printk( "   TEST Mode !!!!!");
	//printk( "\n======================================================================\n");

	if(command == 5010)
	{
		check_time = jiffies;
		master_write_buf[0] = UNIVERSAL_CMD;		//0xA0
		master_write_buf[1] = UNIVCMD_ENTER_TEST_MODE;	//0x40
		
		if (!i2c_master_send(ts_g->client, master_write_buf, 2))
		{
			printk("test mode failed#########################");
			return false;
		}
		
		while (gpio_get_value(GPIO_TOUCH_CHG_INT) == 1 ) //check interrupt high
		{
			usleep(50);
			if (5000<(jiffies_to_msecs(jiffies-check_time))) //to prevent the case of infinite loop
			{
				printk("too long time expended!\n");
				return false;
			}
		}		
		while (gpio_get_value(GPIO_TOUCH_CHG_INT) == 0 ) //check interrupt low
		{
			usleep(100);
		}

		printk(" self test cycle 1 = %u\n", jiffies_to_msecs(jiffies-check_time));
		
		master_write_buf[0] = UNIVERSAL_CMD;		//0xA0
		master_write_buf[1] = UNIVCMD_TEST_CM_DELTA;	//0x41
		if (!i2c_master_send(ts_g->client, master_write_buf, 2))
		{
			printk("test mode failed111111111111111111");
			return false;
		}
		
		while (gpio_get_value(GPIO_TOUCH_CHG_INT) == 0 ) //check interrupt low 
		{
			usleep(100);
		}
		printk(" self test cycle 2 = %u\n", jiffies_to_msecs(jiffies-check_time));
		disable_irq(ts_g->client->irq);
		for (r = 0; r < RXch; r++) //Model Dependent : 14
		{
			for (t = 0; t < TXch; t++) //Model Dependent : 26
			{
				master_write_buf[0] = UNIVERSAL_CMD;			//0xA0
				master_write_buf[1] = UNIVCMD_GET_PIXEL_CM_DELTA;	//0x42
				master_write_buf[2] = t; //Exciting CH.
				master_write_buf[3] = r; //Sensing CH.

				if (!i2c_master_send(ts_g->client, master_write_buf, 4))
				{
					printk("test mode failed222222222222222");
					return false;
				}

				master_write_buf[0] = UNIVERSAL_CMD_RESULT_SIZE;	//0xAE
				if (!i2c_master_send(ts_g->client, master_write_buf, 1))
				{
					printk("test mode failed33333333333333333333");
					return false;
				}
				if (!i2c_master_recv(ts_g->client, master_read_buf_array, 1))
				{
					printk("test mode failed4444444444444444444");
					return false;
				}

				master_write_buf[0] = UNIVERSAL_CMD_RESULT;		//0xAF
				if (!i2c_master_send(ts_g->client, master_write_buf, 1))
				{
					printk("test mode failed555555555555555555555");
					return false;
				}
				if (!i2c_master_recv(ts_g->client, master_read_buf_array, master_read_buf_array[0]))
				{
					printk("test mode failed666666666666666666666");
					return false;
				}
				cmdata = *(uint16_t*) master_read_buf_array;
				CM_DELTA[i] = (int)cmdata;
				i++;
			}
		}
		//printk( "\n======================================================================\n");
		//printk( " EXIT  TEST Mode !!!!!");
		//printk( "\n======================================================================\n");
		master_write_buf[0] = UNIVERSAL_CMD;		//0xA0
		master_write_buf[1] = UNIVCMD_EXIT_TEST_MODE;	//0x4F
		
		if (!i2c_master_send(ts_g->client, master_write_buf, 2))
		{
			printk("exit test mode failed#########################");
			return false;
		}
		enable_irq(ts_g->client->irq);
		msleep(300);
		printk(" self test cycle last = %u\n", jiffies_to_msecs(jiffies-check_time));
	}
	else if (command == 5011)
	{
		//printk( "\n======================================================================\n");
		//printk( " EXIT  TEST Mode !!!!!");
		//printk( "\n======================================================================\n");
		master_write_buf[0] = UNIVERSAL_CMD;		//0xA0
		master_write_buf[1] = UNIVCMD_EXIT_TEST_MODE;	//0x4F
		
		if (!i2c_master_send(ts_g->client, master_write_buf, 2))
		{
			printk("exit test mode failed#########################");
			return false;
		}
		msleep(300);
	}
	
	return true;
}

static long ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	//config_table_element config;
	int return_value = -1;
	int data, object_type, field_index;
	//dbg("ioctl-->(%d, %d) \n",(int)cmd,(int)arg);
	
	switch (cmd)
	{
		case SET_TOUCH_CONFIG:
			data		= (int)((arg & 0xFFFF0000) >> 16);
			object_type 	= (int)((arg & 0x0000FF00) >> 8);
			field_index 	= (int)((arg & 0x000000FF) >> 0);
			/*
			if (config_table[object_type] == 0) {
				printk("[TSP] Error! undefined object type! %d\n", object_type);
			break;
			}
			config = config_table[object_type][field_index];
			if (config.size == UINT8) {
				*((uint8_t*)config_table[object_type][field_index].value) = data;
			}
			else if (config.size == UINT16) {
				*((uint16_t*)config_table[object_type][field_index].value) = data;
			}
			else if (config.size == INT8) {
				*((int8_t*)config_table[object_type][field_index].value) = data;
			}
			else if (config.size == INT16) {
				*((int16_t*)config_table[object_type][field_index].value) = data;
			}
			else {
				// Error
			}
			*/
			printk("[TSP] set %d-%d with %d\n", object_type, field_index, data);
			break;
	
		case GET_TOUCH_CONFIG:
			object_type 	= (int)((arg & 0x0000FF00) >> 8);
			field_index 	= (int)((arg & 0x000000FF) >> 0);
			
			//config = config_table[object_type][field_index];
			if (object_type == 9 && field_index == 3) {
				return_value = TXch ;
			}
			if (object_type == 9 && field_index == 4) {
				return_value = RXch ;
			}
			if (object_type == 9 && field_index == 9) {
				return_value = 1;
			}
			return return_value;
			
			break;
		case DIAG_DEBUG:
			/*
			 * Run Diag and save result into reference_data array when arg. is 5010 or 5011. 
			 * Returns diag result when the arg. is in range of 0~223. 
			 */
			if (arg == 5010) 
			{
				diag_debug(MELFAS_DELTA_MODE);
				return 0;
			}
			if (arg == 5011) 
			{
				diag_debug(MELFAS_REFERENCE_MODE);
				return 0;
			}
			else if (arg > 364-1)
			{
				printk("[TSP] ERROR!");
				return 0;
			}
			//return (int)(CM_DELTA[arg/TXch][arg%TXch]); //will not use touchmonitor.apk

		default:
			break;
	}
	return 0;
}
#endif
int init_hw_setting(void)
{
	int rc; 
	unsigned gpioConfig;
	
//=============================================================
//	Touch IC Digital Power Setting for i2c
//=============================================================

#if ( defined(CONFIG_MACH_MSM8960_VEGAPVW) && (BOARD_VER >= WS11) )	
#else
	struct regulator *vreg_touch_dvdd; // 1.8V
	vreg_touch_dvdd = regulator_get(NULL, TOUCH_POWER_DVDD);
	if(vreg_touch_dvdd == NULL) 
		dbg_kern("%s: vreg_touch_dvdd\n", __func__);
	rc = regulator_enable(vreg_touch_dvdd);
	if (rc) {
		dbg_kern("%s: vreg_dvdd enable failed (%d)\n", __func__, rc);
		return 0;
	}
#endif
	
//=============================================================
//	Touch IC analog Power Setting
//=============================================================
#ifdef TOUCH_POWER_AVDD_USING_EXTRA_LDO
	gpio_request(GPIO_TOUCH_POWER, "touch_power_n");
	gpioConfig = GPIO_CFG(GPIO_TOUCH_POWER, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA);
	rc = gpio_tlmm_config(gpioConfig, GPIO_CFG_ENABLE);
	if (rc) {
		dbg_kern("%s: GPIO_TOUCH_RST failed (%d)\n",__func__, rc);
		return -1;
	}
	gpio_set_value(GPIO_TOUCH_POWER, 1);
#endif

//=============================================================
//	Touch IC GPIO Setting
//=============================================================
	// GPIO Config: interrupt pin
	gpio_request(GPIO_TOUCH_CHG_INT, "touch_chg_int");
	gpioConfig = GPIO_CFG(GPIO_TOUCH_CHG_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA);
	rc = gpio_tlmm_config(gpioConfig, GPIO_CFG_ENABLE);
	if (rc) {
		dbg_kern("%s: GPIO_TOUCH_CHG_INT failed (%d)\n",__func__, rc);
		return -1;
	}        
    msleep(200);
	return 0;
}

void off_hw_setting(void)
{

#if ( defined(CONFIG_MACH_MSM8960_VEGAPVW) && (BOARD_VER >= WS11) )	
#else
	int rc=0; 	
	struct regulator *vreg_touch_dvdd; // 1.8V	
	vreg_touch_dvdd = regulator_get(NULL, TOUCH_POWER_DVDD);

	if (IS_ERR(vreg_touch_dvdd)) {
		rc = PTR_ERR(vreg_touch_dvdd);
		dbg_kern("%s: regulator get of %s failed (%d)\n", __func__, TOUCH_POWER_DVDD, rc);
	}

	rc = regulator_disable(vreg_touch_dvdd);
	if (rc>0) dbg_kern("%s regulator_disable return:  %d \n", TOUCH_POWER_DVDD, rc);
	regulator_put(vreg_touch_dvdd);	
#endif

#ifdef TOUCH_POWER_AVDD_USING_EXTRA_LDO
	gpio_set_value(GPIO_TOUCH_POWER, 0);
	msleep(10);
#endif
	gpio_free(GPIO_TOUCH_POWER);
	gpio_free(GPIO_TOUCH_CHG_INT);
	msleep(100);
}

static void melfas_ts_get_data(struct melfas_ts_data *ts)
{
	int ret = 0, i;
	uint8_t buf[TS_READ_REGS_LEN];
	int read_num, FingerID, strength_temp;

#if ( SKY_ANDROID_ICS & SKY_MULTI_TOUCH_PROTOCOL_B )
	int move_finger[MELFAS_MAX_TOUCH] ={1, };
#endif

	buf[0] = TS_READ_START_ADDR;

	ret = i2c_master_send(ts->client, (const char *) buf, 1);
	if(ret < 0)
	{
		dbg_kern("melfas_ts_work_func: i2c failed : 1\n");
		goto i2cFAIL;
	}
	ret = i2c_master_recv(ts->client, (char *) buf, 1);
	if(ret < 0)
	{
		dbg_kern("melfas_ts_work_func: i2c failed : 2\n");
		goto i2cFAIL;
	}

	read_num = buf[0];
	
	if(read_num>0)
	{
		buf[0] = TS_READ_START_ADDR2;

		ret = i2c_master_send(ts->client, (const char *) buf, 1);
		if(ret < 0)
		{
			dbg_kern("melfas_ts_work_func: i2c failed : 3\n");
			return ;	
		}
		ret = i2c_master_recv(ts->client, (char *) buf, read_num);
		if(ret < 0)
		{
			dbg_kern("melfas_ts_work_func: i2c failed : 4\n");
			return ;	
		}
#ifdef SKY_PROCESS_CMD_KEY
		ktime_get_ts(&tspec); //TOUCH_REACTION_RATE
#endif

#ifdef ANTI_ESD
	//ESD_CASE 
	if (buf[0] == 0x0F) // chip reset
	{
		ESD_CNT ++;
		dbg("ESD_COUNT : %d \n", ESD_CNT); 
		off_hw_setting();
        msleep(20);//for power off slope 
		init_global(ts);
#if (SKY_ANDROID_ICS && SKY_MULTI_TOUCH_PROTOCOL_B) 
		input_mt_destroy_slots(ts->input_dev);
#endif
		init_hw_setting();
#if (SKY_ANDROID_ICS && SKY_MULTI_TOUCH_PROTOCOL_B)
		input_mt_init_slots(ts->input_dev, MELFAS_MAX_TOUCH);
#endif 
             return;
	}
#endif
		for(i=0; i<read_num; i=i+6)
		{
			FingerID = (buf[i] & 0x0F)-1;

			g_Mtouch_info[FingerID].posX= (uint16_t)(buf[i+1] & 0x0F) << 8 | buf[i+2];
			g_Mtouch_info[FingerID].posY= (uint16_t)(buf[i+1] & 0xF0) << 4 | buf[i+3];	

			g_Mtouch_info[FingerID].width= buf[i+4];					
			
			if((buf[i] & 0x80)==0) strength_temp = 0;
			else strength_temp = buf[i+5];

#if ( SKY_ANDROID_ICS & SKY_MULTI_TOUCH_PROTOCOL_B )

			move_finger[FingerID] = 1;

			//	check new touch track
			if ( g_Mtouch_info[FingerID].strength <= 0 && strength_temp > 0 ) {
				g_track_id[FingerID] = input_mt_new_trkid(ts->input_dev);
				move_finger[FingerID] = 0;
			//	dbg("melfas_ts ##DOWN: x: %d, y: %d, w: %d z: %d\n", 
			//	g_Mtouch_info[i].posX, g_Mtouch_info[i].posY, g_Mtouch_info[i].width, strength_temp); // p11774 for debug
			}

			//	Check Release 
			if ( strength_temp <= 0 ) {
				g_track_id[FingerID] = -1;
				move_finger[FingerID] = 0;
			//	dbg("melfas_ts ##UP: x: %d, y: %d, w: %d z: %d\n", 
			//	g_Mtouch_info[i].posX, g_Mtouch_info[i].posY, g_Mtouch_info[i].width, strength_temp); // p11774 for debug
			}
#endif

            g_Mtouch_info[FingerID].strength = strength_temp;
		}	
	}

	if (ret < 0)
	{
		dbg_kern("melfas_ts_work_func: i2c failed : 5\n");
		return ;	
	}
	else 
	{			

#if SKY_ANDROID_ICS
		int bTouched=0;
		for(i=0; i<MELFAS_MAX_TOUCH; i++) {
			if ( g_Mtouch_info[i].strength > 0 ) bTouched = 1;
		}

		input_report_key(ts->input_dev, BTN_TOUCH, bTouched );

#endif 
		for(i=0; i<MELFAS_MAX_TOUCH; i++)
		{	
			if( g_Mtouch_info[i].strength == -1 ) continue;
			
#if defined (CONFIG_MACH_MSM8960_VEGAPVW)
			if ( axis_origin == 1 ) {
				g_Mtouch_info[i].posX = TS_MAX_X_COORD - g_Mtouch_info[i].posX;
				g_Mtouch_info[i].posY = TS_MAX_Y_COORD - g_Mtouch_info[i].posY;
			}	
#endif 

#if SKY_ANDROID_ICS

	#if SKY_MULTI_TOUCH_PROTOCOL_B 

		//	Type B			
			if ( g_Mtouch_info[i].strength <= 0 ) g_track_id[i] = -1;

			//dbg("melfas_ts_work_func: Touch slot: %d, track=%d, x: %d, y: %d, w: %d z: %d\n", 
			//	i, g_track_id[i], g_Mtouch_info[i].posX, g_Mtouch_info[i].posY, g_Mtouch_info[i].width, g_Mtouch_info[i].strength);
		
			input_mt_slot(ts->input_dev, i);

			if (move_finger[i] == 0 )
				input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, g_track_id[i]);

			if ( g_track_id[i] >= 0 ) {
				input_report_abs(ts->input_dev, ABS_MT_POSITION_X, g_Mtouch_info[i].posX);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, g_Mtouch_info[i].posY);
				input_report_abs(ts->input_dev, ABS_MT_PRESSURE, g_Mtouch_info[i].strength );
				input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, g_Mtouch_info[i].width);
				input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, g_Mtouch_info[i].strength );
			}			
	#else

		//	Type A		

//			dbg("melfas_ts_work_func: Touch ID: %d, x: %d, y: %d, w: %d z: %d\n", 
//				i, g_Mtouch_info[i].posX, g_Mtouch_info[i].posY, g_Mtouch_info[i].width, g_Mtouch_info[i].strength);
		
			if ( g_Mtouch_info[i].strength > 0 ) {
				input_report_abs(ts->input_dev, ABS_MT_POSITION_X, g_Mtouch_info[i].posX);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, g_Mtouch_info[i].posY);		
				input_report_abs(ts->input_dev, ABS_MT_PRESSURE, g_Mtouch_info[i].strength );
				input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, g_Mtouch_info[i].width);
				input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, g_Mtouch_info[i].strength);
			}

			input_mt_sync(ts->input_dev);
	#endif


#else			

//			dbg("melfas_ts_work_func: Touch ID: %d, x: %d, y: %d, w: %d z: %d\n", 
//				i, g_Mtouch_info[i].posX, g_Mtouch_info[i].posY, g_Mtouch_info[i].width, g_Mtouch_info[i].strength);

			input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, i);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, g_Mtouch_info[i].posX);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, g_Mtouch_info[i].posY);
			input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, g_Mtouch_info[i].width);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, g_Mtouch_info[i].strength );			
			input_mt_sync(ts->input_dev);
#endif

			if (g_Mtouch_info[i].strength == 0)
				g_Mtouch_info[i].strength = -1;
		}		
		input_sync(ts->input_dev);
	}
	return;
i2cFAIL : 
	off_hw_setting();
	msleep(20);//for power off slope 
	init_global(ts);
#if (SKY_ANDROID_ICS && SKY_MULTI_TOUCH_PROTOCOL_B) 
	input_mt_destroy_slots(ts->input_dev);
#endif
	init_hw_setting();
#if (SKY_ANDROID_ICS && SKY_MULTI_TOUCH_PROTOCOL_B)
	input_mt_init_slots(ts->input_dev, MELFAS_MAX_TOUCH);
#endif 
        return; 
}

static irqreturn_t melfas_ts_irq_handler(int irq, void *handle)
{
	struct melfas_ts_data *ts = (struct melfas_ts_data *)handle;
	melfas_ts_get_data(ts);
	return IRQ_HANDLED;
}

static int melfas_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct melfas_ts_data *ts;
	int ret = 0, i; 
	
	uint8_t buf[6] = {0,};

	dbg_kern("kim ms : melfas_ts_probe\n");

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        dbg_kern("melfas_ts_probe: need I2C_FUNC_I2C\n");
        ret = -ENODEV;
        goto err_check_functionality_failed;
    }

    ts = kmalloc(sizeof(struct melfas_ts_data), GFP_KERNEL);
    if (ts == NULL)
    {
        dbg_kern("melfas_ts_probe: failed to create a state of melfas-ts\n");
        ret = -ENOMEM;
        goto err_alloc_data_failed;
    }
	printk("+-----------------------------------------+\n");
	printk("|  MELFAS Touch Driver Probe!             |\n");
	printk("+-----------------------------------------+\n");

	//INIT_WORK(ts->work, melfas_ts_get_data );
    ts->client = client;
    i2c_set_clientdata(client, ts);
    ret = i2c_master_send(ts->client, (const char *) buf, 1);

	dbg_kern("melfas_ts_probe: i2c_master_send() [%d], Add[%d]\n", ret, ts->client->addr);
	
#if SET_DOWNLOAD_BY_GPIO
	buf[0] = TS_READ_VERSION_ADDR; //0xF0
	ret = i2c_master_send(ts->client, (const char *) buf, 1);
	if(ret < 0)
	{
		dbg_kern("melfas_ts_work_func : i2c_master_send [%d]\n", ret);
	}
	ret = i2c_master_recv(ts->client, (char *) buf, 4);
	if (ret<0) dbg_kern("melfas_ts_work_func : i2c_master_recv [%d]\n", ret);

	dbg("melfas version: 0x%X 0x%X 0x%X 0x%X \n", buf[0], buf[1], buf[2], buf[3]);
	
	if(buf[2] < 0x44) // full download with comparing compatibility ver.
	{
		if(( buf[0] == 0x41 && buf[1] == 0x32 && buf[2] == 0x43 && buf[3] == 0x56) || ( buf[0] == 0x41 && buf[1] == 0x32 && buf[2] == 0x41 && buf[3] == 0x56)) 
		{		
			ret = mms100_download(ts->client);
			if (ret != 0) dbg_kern("SET Download Fail - error code [%d]\n", ret);
		}
	}
	else // v4 partial download (temp code)
	{
		buf[0] = TS_READ_VERSION_ADDR; //0xF0
		ret = i2c_master_send(ts->client, (const char *) buf, 1);
		if(ret < 0)
		{
			dbg_kern("melfas_ts_work_func : i2c_master_send [%d]\n", ret);
		}
		ret = i2c_master_recv(ts->client, (char *) buf, 6);
		if (ret<0) dbg_kern("melfas_ts_work_func : i2c_master_recv [%d]\n", ret);
		dbg("melfas v4 version: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
		
		//temp code // delete this after V4 download update
		if( buf[4] < 0x7 || buf[5] < 0x7 )
		{		
			ret = mms100_download(ts->client);
			if (ret != 0) dbg_kern("SET Download Fail - error code [%d]\n", ret);
		}
	}
	
		

#endif // SET_DOWNLOAD_BY_GPIO
	
	ts->input_dev = input_allocate_device();
    if (!ts->input_dev)
    {
		dbg_kern("melfas_ts_probe: Not enough memory\n");
		ret = -ENOMEM;
		goto err_input_dev_alloc_failed;
	} 

	ts->input_dev->name = "qt602240_ts_input"; //.kl file

	ts->input_dev->evbit[0] = BIT_MASK(EV_ABS) | BIT_MASK(EV_KEY);
	

	ts->input_dev->keybit[BIT_WORD(KEY_MENU)] |= BIT_MASK(KEY_MENU);
	ts->input_dev->keybit[BIT_WORD(KEY_HOME)] |= BIT_MASK(KEY_HOME);
	ts->input_dev->keybit[BIT_WORD(KEY_BACK)] |= BIT_MASK(KEY_BACK);		
	ts->input_dev->keybit[BIT_WORD(KEY_SEARCH)] |= BIT_MASK(KEY_SEARCH);			


	__set_bit(BTN_TOUCH, ts->input_dev->keybit);
//	__set_bit(EV_ABS,  ts->input_dev->evbit);
//	ts->input_dev->evbit[0] =  BIT_MASK(EV_SYN) | BIT_MASK(EV_ABS) | BIT_MASK(EV_KEY);	

#ifdef	SKY_PROCESS_CMD_KEY
	set_bit(KEY_RECENTAPPS, ts->input_dev->keybit);  // p11774_Touch_key
	set_bit(KEY_SEARCH, ts->input_dev->keybit);
	set_bit(KEY_HOMEPAGE, ts->input_dev->keybit);
	set_bit(KEY_0, ts->input_dev->keybit);
	set_bit(KEY_1, ts->input_dev->keybit);
	set_bit(KEY_2, ts->input_dev->keybit);
	set_bit(KEY_3, ts->input_dev->keybit);
	set_bit(KEY_4, ts->input_dev->keybit);
	set_bit(KEY_5, ts->input_dev->keybit);
	set_bit(KEY_6, ts->input_dev->keybit);
	set_bit(KEY_7, ts->input_dev->keybit);
	set_bit(KEY_8, ts->input_dev->keybit);
	set_bit(KEY_9, ts->input_dev->keybit);
	set_bit(0xe3, ts->input_dev->keybit); /* '*' */
	set_bit(0xe4, ts->input_dev->keybit); /* '#' */
	set_bit(0xe5, ts->input_dev->keybit); /* 'KEY_END' p13106 120105 */
	set_bit(KEY_POWER, ts->input_dev->keybit);
	set_bit(KEY_LEFTSHIFT, ts->input_dev->keybit);
	set_bit(KEY_RIGHTSHIFT, ts->input_dev->keybit);
	set_bit(KEY_LEFT, ts->input_dev->keybit);
	set_bit(KEY_RIGHT, ts->input_dev->keybit);
	set_bit(KEY_UP, ts->input_dev->keybit);
	set_bit(KEY_DOWN, ts->input_dev->keybit);
	set_bit(KEY_ENTER, ts->input_dev->keybit);
	set_bit(KEY_SEND, ts->input_dev->keybit);
	set_bit(KEY_END, ts->input_dev->keybit);
	set_bit(KEY_F1, ts->input_dev->keybit);
	set_bit(KEY_F2, ts->input_dev->keybit);
	set_bit(KEY_F3, ts->input_dev->keybit);				// P13106 VT_CALL for VT TEST 121019				
	set_bit(KEY_F4, ts->input_dev->keybit);
	set_bit(KEY_VOLUMEUP, ts->input_dev->keybit);
	set_bit(KEY_VOLUMEDOWN, ts->input_dev->keybit);
	set_bit(KEY_CLEAR, ts->input_dev->keybit);
	set_bit(KEY_CAMERA, ts->input_dev->keybit);
	
	//set_bit(KEY_BACKSPACE, ts->input_dev->keybit); 
	//set_bit(KEY_LEFTALT, ts->input_dev->keybit);
	//set_bit(KEY_RIGHTALT, ts->input_dev->keybit);
	//set_bit(KEY_COMMA, ts->input_dev->keybit);
	set_bit(KEY_SPACE, ts->input_dev->keybit); // p11774_Touch_key 
	set_bit(KEY_DOT, ts->input_dev->keybit);
	set_bit(KEY_A, ts->input_dev->keybit);
	set_bit(KEY_B, ts->input_dev->keybit);
	set_bit(KEY_C, ts->input_dev->keybit);
	set_bit(KEY_D, ts->input_dev->keybit);
	set_bit(KEY_E, ts->input_dev->keybit);
	set_bit(KEY_F, ts->input_dev->keybit);
	set_bit(KEY_G, ts->input_dev->keybit);
	set_bit(KEY_H, ts->input_dev->keybit);
	set_bit(KEY_I, ts->input_dev->keybit);
	set_bit(KEY_J, ts->input_dev->keybit);
	set_bit(KEY_K, ts->input_dev->keybit);
	set_bit(KEY_L, ts->input_dev->keybit);
	set_bit(KEY_M, ts->input_dev->keybit);
	set_bit(KEY_N, ts->input_dev->keybit);
	set_bit(KEY_O, ts->input_dev->keybit);
	set_bit(KEY_P, ts->input_dev->keybit);				
	set_bit(KEY_Q, ts->input_dev->keybit);
	set_bit(KEY_R, ts->input_dev->keybit);
	set_bit(KEY_S, ts->input_dev->keybit);
	set_bit(KEY_T, ts->input_dev->keybit);
	set_bit(KEY_U, ts->input_dev->keybit);
	set_bit(KEY_V, ts->input_dev->keybit);
	set_bit(KEY_W, ts->input_dev->keybit);
	set_bit(KEY_X, ts->input_dev->keybit);
	set_bit(KEY_Y, ts->input_dev->keybit);
	set_bit(KEY_Z, ts->input_dev->keybit);
	//    set_bit(KEY_HOLD, ts->input_dev->keybit);
#endif // SKY_PROCESS_CMD_KEY

	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, TS_MAX_X_COORD, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, TS_MAX_Y_COORD, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, TS_MAX_Z_TOUCH, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, MELFAS_MAX_TOUCH, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, TS_MAX_W_TOUCH, 0, 0);
//	__set_bit(EV_SYN, ts->input_dev->evbit); 
//	__set_bit(EV_KEY, ts->input_dev->evbit);	


    ret = input_register_device(ts->input_dev);
    if (ret)
    {
        dbg_kern("melfas_ts_probe: Failed to register device\n");
        ret = -ENOMEM;
        goto err_input_register_device_failed;
    }

	ts->client->irq = IRQ_TOUCH_INT;
	dbg_kern("IRQ_TOUCH_INT : %d \n", ts->client->irq);

    if (ts->client->irq)
    {
        dbg_kern("melfas_ts_probe: trying to request irq: %s-%d\n", ts->client->name, ts->client->irq);
       
	//	ret = request_threaded_irq(client->irq, NULL, melfas_ts_irq_handler,IRQF_TRIGGER_FALLING, ts->client->name, ts);
		ret = request_threaded_irq(client->irq, NULL, melfas_ts_irq_handler,IRQF_TRIGGER_LOW | IRQF_ONESHOT, ts->client->name, ts);

        if (ret > 0)
        {
            dbg_kern("melfas_ts_probe: Can't allocate irq %d, ret %d\n", ts->client->irq, ret);
            ret = -EBUSY;
            goto err_request_irq;
        }
    }	

	for (i = 0; i < MELFAS_MAX_TOUCH ; i++)  /* _SUPPORT_MULTITOUCH_ */
	{
		g_Mtouch_info[i].strength = -1;	
		g_track_id[i] = -1;
	}

	dbg_kern("melfas_ts_probe: succeed to register input device\n");

#if CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = melfas_ts_early_suspend;
	ts->early_suspend.resume = melfas_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif
#ifdef TOUCH_IO  
	ret = misc_register(&touch_io);
	if (ret) 
	{
		pr_err("::::::::: can''t register qt602240 misc\n");
	}
         ts_g = ts;
#endif //TOUCH_IO	
	dbg("melfas_ts_probe: Start touchscreen. name: %s, irq: %d\n", ts->client->name, ts->client->irq);	
	
#ifdef SKY_PROCESS_CMD_KEY

	dbg_kern("touch_fops - misc register\n");
	ret = misc_register(&touch_event);
	if (ret) {
		dbg("::::::::: can''t register touch_fops\n");
	}

	ts_g = ts;
	
#endif    

#if (SKY_ANDROID_ICS && SKY_MULTI_TOUCH_PROTOCOL_B)
	input_mt_init_slots(ts->input_dev, MELFAS_MAX_TOUCH);
#endif 

	buf[0] = TS_READ_VERSION_CUST_ADDR;
	ret = i2c_master_send(ts_g->client, (const char *) buf, 1);
	if(ret < 0) dbg_kern("melfas_ts_work_func : i2c_master_send [%d]\n", ret);

	ret = i2c_master_recv(ts_g->client, (char *) buf, 4);
	if (ret<0) dbg_kern("melfas_ts_work_func : i2c_master_recv [%d]\n", ret);		

	//	DALI - Vega Premia V WS10 WTR check
	if ( buf[0] == 0x44 && buf[1] == 0x41 && buf[2] == 0x4C && buf[3] == 0x49 )
		axis_origin = 1;
	else 
		axis_origin = 0;	

	return 0;


err_request_irq:
	dbg_kern("melfas-ts: err_request_irq failed\n");
	free_irq(client->irq, ts);
err_input_register_device_failed:
	dbg_kern("melfas-ts: err_input_register_device failed\n");
	input_free_device(ts->input_dev);
err_input_dev_alloc_failed:
	dbg_kern("melfas-ts: err_input_dev_alloc failed\n");
err_alloc_data_failed:
	dbg_kern("melfas-ts: err_alloc_data failed_\n");	
	
	/* //blocked to prevent build err
err_detect_failed:
	dbg_kern("melfas-ts: err_detect failed\n");
	kfree(ts);
	*/
	
err_check_functionality_failed:
	dbg_kern("melfas-ts: err_check_functionality failed_\n");

	return ret;
}

static int melfas_ts_remove(struct i2c_client *client)
{
	struct melfas_ts_data *ts = i2c_get_clientdata(client);

#if (SKY_ANDROID_ICS && SKY_MULTI_TOUCH_PROTOCOL_B)
	input_mt_destroy_slots(ts->input_dev);
#endif 
#ifdef SKY_PROCESS_CMD_KEY
	misc_deregister(&touch_event);
#endif

#ifdef TOUCH_IO
	misc_deregister(&touch_io);
#endif //TOUCH_IO
	unregister_early_suspend(&ts->early_suspend);
	free_irq(client->irq, ts);
	ts->power(false);  // modified
	input_unregister_device(ts->input_dev);
	kfree(ts);
	off_hw_setting(); // dhyang


	return 0;
}

static int melfas_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct melfas_ts_data *ts = i2c_get_clientdata(client);
	disable_irq(client->irq);
	off_hw_setting();
	init_global(ts);
#if (SKY_ANDROID_ICS && SKY_MULTI_TOUCH_PROTOCOL_B)
	input_mt_destroy_slots(ts->input_dev);
#endif 
	return 0;
}

static int melfas_ts_resume(struct i2c_client *client)
{
	struct melfas_ts_data *ts = i2c_get_clientdata(client);
#if (SKY_ANDROID_ICS && SKY_MULTI_TOUCH_PROTOCOL_B)
	input_mt_init_slots(ts->input_dev, MELFAS_MAX_TOUCH);
#endif 
	init_hw_setting();
	enable_irq(client->irq); // scl wave

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void melfas_ts_early_suspend(struct early_suspend *h)
{
	struct melfas_ts_data *ts;
	ts = container_of(h, struct melfas_ts_data, early_suspend);	
	melfas_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void melfas_ts_late_resume(struct early_suspend *h)
{
	struct melfas_ts_data *ts;
	ts = container_of(h, struct melfas_ts_data, early_suspend);
	melfas_ts_resume(ts->client);
}
#endif

static const struct i2c_device_id melfas_ts_id[] =
{
    { "melfas-ts-i2c", 0 },
    { }
};

static struct i2c_driver melfas_ts_driver =
{
	.driver = {
		.name	= "melfas-ts-i2c",
    },
    .id_table	= melfas_ts_id,
    .probe		= melfas_ts_probe,
    .remove		= __devexit_p(melfas_ts_remove),
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= melfas_ts_suspend,
	.resume		= melfas_ts_resume,
#endif
};

static void init_global(struct melfas_ts_data *ts )
{
	int i;
	input_report_key(ts->input_dev, BTN_TOUCH, 0);
	for (i = 0; i < MELFAS_MAX_TOUCH ; i++) // init global var
	{
		g_Mtouch_info[i].strength = -1;
		g_Mtouch_info[i].posX = 0;
		g_Mtouch_info[i].posY = 0;
		g_Mtouch_info[i].width = 0;
		g_track_id[i] = -1;
		input_mt_slot(ts->input_dev, i);
		input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, g_track_id[i]);	//release Track ID
	}
	input_sync(ts->input_dev);
}

static int __devinit melfas_ts_init(void)
{
	int rc;
	rc = init_hw_setting(); //dhyang
	if(rc<0)
	{
		dbg_kern("init_hw_setting failed. (rc=%d)\n", rc);
		return rc;
	}
	return i2c_add_driver(&melfas_ts_driver);
}

static void __exit melfas_ts_exit(void)
{
	i2c_del_driver(&melfas_ts_driver);
}

MODULE_DESCRIPTION("Driver for Melfas MTSI Touchscreen Controller");
MODULE_AUTHOR("MinSang, Kim <kimms@melfas.com>");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");

module_init(melfas_ts_init);
module_exit(melfas_ts_exit);

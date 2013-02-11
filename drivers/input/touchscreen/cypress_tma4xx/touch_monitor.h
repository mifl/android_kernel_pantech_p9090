/* 
 * Touch Monitor Interface 
 * Ver 0.1
 */
#include <linux/version.h>
#include "touch_log.h"

static int DebugON = 1;
#define dbg(fmt, args...) if(DebugON) printk("[CYTTSP]" fmt, ##args)

static int monitor_open(struct inode *inode, struct file *file);
static ssize_t monitor_read(struct file *file, char *buf, size_t count, loff_t *ppos);
static ssize_t monitor_write(struct file *file, const char *buf, size_t count, loff_t *ppos);
static int monitor_release(struct inode *inode, struct file *file);
#if ((LINUX_VERSION_CODE & 0xFFFF00) < KERNEL_VERSION(3,0,0))
static int monitor_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
#else
static long monitor_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
#endif

static void set_touch_config(int data, int object_type, int field_index);
static int get_touch_config(int object_type, int field_index);
static void apply_touch_config(void);
static void reset_touch_config(void);
static int ioctl_debug(unsigned long arg);
static int ioctl_diag_debug(unsigned long arg);
static int send_reference_data(unsigned long arg);

typedef enum {

	APPLY_TOUCH_CONFIG = 501,
	DIAG_DEBUG = 502,
	RESET_TOUCH_CONFIG = 503,
	GET_TOUCH_CONFIG = 504,
	SET_TOUCH_CONFIG = 505,
	READ_ITO_TYPE = 506,
	GET_REFERENCE_DATA = 507,

	TOUCH_IOCTL_READ_LASTKEY=1001,	
	TOUCH_IOCTL_DO_KEY,	
	TOUCH_IOCTL_RELEASE_KEY, 
	TOUCH_IOCTL_CLEAN,
	TOUCH_IOCTL_DEBUG,
	TOUCH_IOCTL_RESTART,
	TOUCH_IOCTL_PRESS_TOUCH,
	TOUCH_IOCTL_RELEASE_TOUCH,
	TOUCH_IOCTL_CHARGER_MODE,
	POWER_OFF,
	TOUCH_IOCTL_STYLUS_MODE,

	TOUCH_CALL_MODE_ENABLE = 1013,
	TOUCH_CALL_MODE_DISABLE = 1014,

	TOUCH_IOCTL_DELETE_ACTAREA = 2001,
	TOUCH_IOCTL_RECOVERY_ACTAREA,

	TOUCH_IOCTL_INIT = 3001,	
	TOUCH_IOCTL_OFF  = 3002,

	TOUCH_CHARGE_MODE_CTL = 4001,

	TOUCH_IOCTL_DIAG_DEBUG_DELTA = 5010,
	TOUCH_IOCTL_DIAG_DEBUG_REF,
	TOUCH_IOCTL_DIAG_DEBUG_OPERATEMODE,
} TOUCH_IOCTL_CMD;


/*
 * vendor_id : 
 * ateml(1) 
 * cypress(2)
 * model_id : 
 * ef39s(0390) ef40s(0400) ef40k(0401)
 * ef48s(0480) ef49k(0481) ef50l(0502)
 * presto(9001)
 * type : 
 * model manager would manage ito or color type.

 * return vendor_id*100*10000 + model_id*100 + type;
 */

static struct file_operations monitor_fops = 
{
	.owner =    THIS_MODULE,
#if ((LINUX_VERSION_CODE & 0xFFFF00) < KERNEL_VERSION(3,0,0))
	.ioctl =    monitor_ioctl,
#else
    .unlocked_ioctl =   monitor_ioctl,
#endif
	.read =     monitor_read,
	.write =    monitor_write,
	.open =     monitor_open,
	.release =  monitor_release
};

static struct miscdevice touch_monitor = 
{
	.minor =    MISC_DYNAMIC_MINOR,
	.name =     "touch_monitor",
	.fops =     &monitor_fops,
    .mode =     S_IRUGO | S_IWUGO
};

typedef struct
{
	int touch_count; 

} touch_monitor_info_t;

#if ((LINUX_VERSION_CODE & 0xFFFF00) < KERNEL_VERSION(3,0,0))
static int monitor_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
#else
static long monitor_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#endif
{
	int data, object_type, field_index;
	// Read Command 
	// Write, Etc.
    dbg("cmd: %d, arg: %ld\n",cmd,arg);
	switch (cmd)
	{
		case SET_TOUCH_CONFIG:
			data		= (int)((arg & 0xFFFF0000) >> 16);
			object_type 	= (int)((arg & 0x0000FF00) >> 8);
			field_index 	= (int)((arg & 0x000000FF) >> 0);

			set_touch_config(data, object_type, field_index);
			break;
		case GET_TOUCH_CONFIG:
			object_type 	= (int)((arg & 0x0000FF00) >> 8);
			field_index 	= (int)((arg & 0x000000FF) >> 0);
			return get_touch_config(object_type, field_index);
			break;
		case APPLY_TOUCH_CONFIG:
			apply_touch_config();
			break;
		case RESET_TOUCH_CONFIG:
			reset_touch_config();
			break;
#ifdef ITO_TYPE_CHECK 
			// TO BE DEPRECIATED
		case READ_ITO_TYPE:
			return 0; 
			break;
#endif
		case TOUCH_IOCTL_DEBUG:
			return ioctl_debug(arg);
			break;
		case TOUCH_IOCTL_CHARGER_MODE:
            return ioctl_debug(arg);
			break;
		case DIAG_DEBUG:
			return ioctl_diag_debug(arg);
			break;
		case GET_REFERENCE_DATA:
			return send_reference_data(arg);
			break;

		default:
			return 0;
			break;
	}
	return 0;
}

// call in driver init function
void touch_monitor_init(void) {
	int rc;
	rc = misc_register(&touch_monitor);
	if (rc) {
		pr_err("::::::::: can''t register touch_monitor\n");
	}
	init_proc();
}

// call in driver remove function
void touch_monitor_exit(void) {
	misc_deregister(&touch_monitor);
	remove_proc();
}

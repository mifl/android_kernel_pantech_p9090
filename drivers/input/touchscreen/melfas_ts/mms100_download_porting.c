//--------------------------------------------------------
//
//
//	Melfas MMS100 Series Download port v1.0 2011.12.20
//		by wcjeong(p11309) in pantech.
//

#include <linux/module.h>
#include <linux/kernel.h>
#include <mach/gpio.h>
#include "mms100_download_porting.h"


#define DEBUG_KERN_ERR
#define DEBUG_KERN_MSG

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

#define GPIO_TOUCH_POWER 		51
#define GPIO_TOUCH_CHG_INT		11
#define GPIO_TOUCH_I2C_SDA		16
#define GPIO_TOUCH_I2C_SCL		17

void MCSDL_VDD_SET_HIGH(void)
{
	gpio_set_value(GPIO_TOUCH_POWER, 1);
};
EXPORT_SYMBOL(MCSDL_VDD_SET_HIGH);

void MCSDL_VDD_SET_LOW(void)
{
	gpio_set_value(GPIO_TOUCH_POWER, 0);
};
EXPORT_SYMBOL(MCSDL_VDD_SET_LOW);

void MCSDL_RESETB_SET_HIGH(void)
{
	gpio_set_value(GPIO_TOUCH_CHG_INT, 1);
};
EXPORT_SYMBOL(MCSDL_RESETB_SET_HIGH);

void MCSDL_RESETB_SET_LOW(void)
{
	gpio_set_value(GPIO_TOUCH_CHG_INT, 0);
};
EXPORT_SYMBOL(MCSDL_RESETB_SET_LOW);

void MCSDL_RESETB_SET_OUTPUT(void)
{
	int rc = 0;
	unsigned gpioConfig=0;
//	gpio_request(GPIO_TOUCH_CHG_INT, "touch_chg_int");
	gpioConfig = GPIO_CFG(GPIO_TOUCH_CHG_INT, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA);
	rc = gpio_tlmm_config(gpioConfig, GPIO_CFG_ENABLE);
	if (rc) {
		dbg_kern("%s: GPIO_TOUCH_CHG_INT failed (%d)\n",__func__, rc);
		return;
	}        
};
EXPORT_SYMBOL(MCSDL_RESETB_SET_OUTPUT);

void MCSDL_RESETB_SET_INPUT(void)
{
	int rc = 0;
	unsigned gpioConfig=0;
//	gpio_request(GPIO_TOUCH_CHG_INT, "touch_chg_int");
	gpioConfig = GPIO_CFG(GPIO_TOUCH_CHG_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA);
	rc = gpio_tlmm_config(gpioConfig, GPIO_CFG_ENABLE);
	if (rc) {
		dbg_kern("%s: GPIO_TOUCH_CHG_INT failed (%d)\n",__func__, rc);
		return;
	}        
};
EXPORT_SYMBOL(MCSDL_RESETB_SET_INPUT);

void MCSDL_GPIO_SCL_SET_HIGH(void)
{
	gpio_set_value(GPIO_TOUCH_I2C_SCL, 1);
};
EXPORT_SYMBOL(MCSDL_GPIO_SCL_SET_HIGH);

void MCSDL_GPIO_SCL_SET_LOW(void)
{
	gpio_set_value(GPIO_TOUCH_I2C_SCL, 0);
}
EXPORT_SYMBOL(MCSDL_GPIO_SCL_SET_LOW);

void MCSDL_GPIO_SDA_SET_HIGH(void)
{
	gpio_set_value(GPIO_TOUCH_I2C_SDA, 1);
}
EXPORT_SYMBOL(MCSDL_GPIO_SDA_SET_HIGH);

void MCSDL_GPIO_SDA_SET_LOW(void)
{
	gpio_set_value(GPIO_TOUCH_I2C_SDA, 0);
}
EXPORT_SYMBOL(MCSDL_GPIO_SDA_SET_LOW);

void MCSDL_GPIO_SCL_SET_OUTPUT(void)
{
	int rc = 0;
	unsigned gpioConfig=0;
//	gpio_request(GPIO_TOUCH_I2C_SCL, "touch_i2c_scl");
	gpioConfig = GPIO_CFG(GPIO_TOUCH_I2C_SCL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA);
	rc = gpio_tlmm_config(gpioConfig, GPIO_CFG_ENABLE);
	if (rc) {
		dbg_kern("%s: GPIO_TOUCH_I2C_SCL failed (%d)\n",__func__, rc);
		return;
	}
}
EXPORT_SYMBOL(MCSDL_GPIO_SCL_SET_OUTPUT);

void MCSDL_GPIO_SCL_SET_INPUT(void)
{
	int rc = 0;
	unsigned gpioConfig=0;
//	gpio_request(GPIO_TOUCH_I2C_SCL, "touch_i2c_scl");
	gpioConfig = GPIO_CFG(GPIO_TOUCH_I2C_SCL, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA);
	rc = gpio_tlmm_config(gpioConfig, GPIO_CFG_ENABLE);
	if (rc) {
		dbg_kern("%s: GPIO_TOUCH_I2C_SCL failed (%d)\n",__func__, rc);
		return;
	}
}
EXPORT_SYMBOL(MCSDL_GPIO_SCL_SET_INPUT);

int MCSDL_GPIO_SCL_IS_HIGH(void)
{
	return gpio_get_value(GPIO_TOUCH_I2C_SCL);
}
EXPORT_SYMBOL(MCSDL_GPIO_SCL_IS_HIGH);

void MCSDL_GPIO_SDA_SET_OUTPUT(void)
{
	int rc = 0;
	unsigned gpioConfig=0;
//	gpio_request(GPIO_TOUCH_I2C_SDA, "touch_i2c_sda");
	gpioConfig = GPIO_CFG(GPIO_TOUCH_I2C_SDA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA);
	rc = gpio_tlmm_config(gpioConfig, GPIO_CFG_ENABLE);
	if (rc) {
		dbg_kern("%s: GPIO_TOUCH_I2C_SDA failed (%d)\n",__func__, rc);
		return;
	}
}
EXPORT_SYMBOL(MCSDL_GPIO_SDA_SET_OUTPUT);

void MCSDL_GPIO_SDA_SET_INPUT(void)
{
	int rc = 0;
	unsigned gpioConfig=0;
//	gpio_request(GPIO_TOUCH_I2C_SDA, "touch_i2c_sda");
	gpioConfig = GPIO_CFG(GPIO_TOUCH_I2C_SDA, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA);
	rc = gpio_tlmm_config(gpioConfig, GPIO_CFG_ENABLE);
	if (rc) {
		dbg_kern("%s: GPIO_TOUCH_I2C_SDA failed (%d)\n",__func__, rc);
		return;
	}
}
EXPORT_SYMBOL(MCSDL_GPIO_SDA_SET_INPUT);

int MCSDL_GPIO_SDA_IS_HIGH(void)
{
	return gpio_get_value(GPIO_TOUCH_I2C_SDA);
}
EXPORT_SYMBOL(MCSDL_GPIO_SDA_IS_HIGH);

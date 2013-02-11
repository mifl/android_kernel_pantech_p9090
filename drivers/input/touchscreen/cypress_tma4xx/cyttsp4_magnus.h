
#ifndef __CYTTSP4_MODEL_CORE_H__
#define __CYTTSP4_MODEL_CORE_H__

#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#define CY_NUM_RETRY                10 /* max retries for rd/wr ops */
#define CY_I2C_NAME                 "cyttsp4-i2c"
#define CY_SPI_NAME                 "cyttsp4-spi"
#define CY_DRIVER_VERSION           "Rev4-2M-26"
#define CY_DRIVER_DATE              "2012-01-24"

/* use the following define if the device is a TMA400 family part
 */
#define CY_USE_TMA400

/* use the following define if the device is a TMA884/616 family part
#define CY_USE_TMA884
 */

/* use the following define to allow auto load of firmware at startup
#define CY_AUTO_LOAD_FW
*/

/* use the following define to allow auto load of Touch Params at startup 
#define CY_AUTO_LOAD_TOUCH_PARAMS
 */

/* use the following define to allow auto load of Design Data at startup
#define CY_AUTO_LOAD_DDATA
 */

/* use the following define to allow auto load of Manufacturing Data at startup
#define CY_AUTO_LOAD_MDATA
 */

/* use the following define to allow autoload firmware for any version diffs;
 * otherwise only autoload if load version is greater than device image version
#define CY_ANY_DIFF_NEW_VER
 */

/* use the following define to include loader application
#define CY_USE_FORCE_LOAD
 */

/* use the following define to enable register peak/poke capability
#define CY_USE_REG_ACCESS
 */

/* use the following define to enable special debug tools for test only
#define CY_USE_DEBUG_TOOLS
 */

/* use the following define to enable additional debug tools for
 * development test only
#define CY_USE_DEV_DEBUG_TOOLS
 */

/* use the following define to use level interrupt method (else falling edge)
 * this method should only be used if the host processor misses edge interrupts
 */
/* recommand EDGE Trigger - on INT Async Mode - p11309 */
//#define CY_USE_LEVEL_IRQ
 

/* use the following define to enable driver watchdog timer
#define CY_USE_WATCHDOG
 */


/* Suspend/Resumt Power Control Mode - p11309 
 * Bootloader Process Time = 400~500msec 
 * Sleep Current is under 10uA					*/
#define CY_USE_SOFT_SUSPEND_RESUME_MODE

/* Use workqueue on ISR - p11309 */
//#define CY_USE_ISR_WORKQUEUE

/* Avoid twice interrupt problem on last release - p11309*/
#define CY_USE_AVOID_RELEASE_EVENT_BUG

#define SKY_PROCESS_CMD_KEY
#define TOUCH_IO

#define GPIO_TOUCH_CHG  (11)
#define GPIO_TOUCH_RST  (50)
#define GPIO_TOUCH_VDD  (51)

#define CYTTSP_MAX_TOUCHNUM	5

#define CY_MAXX 720
#define CY_MAXY 1280
#define CY_MINX 0
#define CY_MINY 0

#ifdef CY_USE_DEV_DEBUG_TOOLS
#ifndef CY_USE_REG_ACCESS
#define CY_USE_REG_ACCESS
#endif
#endif /* --CY_USE_DEV_DEBUG_TOOLS */

/* system includes are here in order to allow DEBUG option */
/* enable this define to enable debug prints
#define DEBUG = y
 */

/* enable this define to enable verbose debug prints
#define VERBOSE_DEBUG
 */

void off_hw_setting(void);
int init_hw_setting(void);


#endif /* __CYTTSP4_MODEL_CORE_H__ */

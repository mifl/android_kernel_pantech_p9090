/*
 * Core Source for:
 * Cypress TrueTouch(TM) Standard Product (TTSP) touchscreen drivers.
 * For use with Cypress Gen4 and Solo parts.
 * Supported parts include:
 * CY8CTMA398
 * CY8CTMA884
 * CY8CTMA4XX
 *
 * Copyright (C) 2009-2011 Cypress Semiconductor, Inc.
 * Copyright (C) 2011 Motorola Mobility, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, and only version 2, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contact Cypress Semiconductor at www.cypress.com <kev@cypress.com>
 *
 */


#include "cyttsp4_core.h"

#include <linux/module.h>
#include <linux/firmware.h>
#include "touch_platform.h"
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/byteorder/generic.h>
#include <linux/bitops.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif /* CONFIG_HAS_EARLYSUSPEND */

#include <linux/regulator/consumer.h>
#include <mach/vreg.h>
#include <linux/wakelock.h>

#include <linux/miscdevice.h>
#include <linux/uaccess.h>

#include <linux/uaccess.h>

// wcjeong - p11309 - for mulit-touoch protocol b
#include <linux/input/mt.h>
#include <linux/kernel.h>

#ifdef CY_CHECK_ADC
#include <linux/msm_adc.h>
#include <linux/completion.h> 
#endif /* --CY_CHECK_ADC */

#ifdef CY_USE_TOUCH_MONITOR
#include "touch_monitor.h"
#endif /* --CY_USE_TOUCH_MONITOR */

/* -------------------------------------------------------------------- */
/* debug option */
/* -------------------------------------------------------------------- */

#ifndef CY_USE_TOUCH_MONITOR
static int DebugON = 0;
#define dbg(fmt, args...) if(DebugON) printk("[CYTTSP]" fmt, ##args)
#else
extern int DebugON;
#endif /* --CY_USE_TOUCH_MONITOR */

#define dbg_func_in()		dbg("[+++] %s\n", __func__)
#define dbg_func_out()		dbg("[---] %s\n", __func__)
#define dbg_line()		dbg("line : %d | func : %s\n", __LINE__, __func__)

#define MAX_DATA_SIZE	1000

/* platform address lookup offsets */
#define CY_TCH_ADDR_OFS		0
#define CY_LDR_ADDR_OFS		1

/* helpers */
#define GET_NUM_TOUCHES(x)          ((x) & 0x1F)
#define IS_LARGE_AREA(x)            ((x) & 0x20)
#define IS_BAD_PKT(x)               ((x) & 0x20)
#define GET_HSTMODE(reg)            ((reg & 0x70) >> 4)
#define IS_BOOTLOADERMODE(reg) 	(reg & 0x01)

/* maximum number of concurrent tracks */
#define CY_NUM_TCH_ID               10

/* maximum number of track IDs */
#define CY_NUM_TRK_ID               16
/* maximum number of command data bytes */
#define CY_NUM_DAT                  6

/* maximum number of config block read data */
#define CY_NUM_CONFIG_BYTES        128

#define CY_REG_BASE                 0x00
#define CY_DELAY_DFLT               20		/* ms */
#define CY_DELAY_MAX                (500/CY_DELAY_DFLT)	/* half second */
#define CY_HALF_SEC_TMO_MS          500		/* half second in msecs */
#define CY_TEN_SEC_TMO_MS           10000	/* ten seconds in msecs */
#define CY_HANDSHAKE_BIT            0x80
#define CY_WAKE_DFLT                99	/* causes wake strobe on INT line
					 * in sample board configuration
					 * platform data->hw_recov() function
					 */
/* power mode select bits */
#define CY_SOFT_RESET_MODE          0x01
#define CY_DEEP_SLEEP_MODE          0x02
#define CY_LOW_POWER_MODE           0x04
/* device mode bits */
#define CY_MODE_CHANGE              0x08 /* rd/wr hst_mode */
#define CY_OPERATE_MODE             0x00 /* rd/wr hst_mode */
#define CY_SYSINFO_MODE             0x10 /* rd/wr hst_mode */
#define CY_CONFIG_MODE              0x20 /* rd/wr hst_mode */
#define CY_BL_MODE                  0x01 /* wr hst mode == soft reset
					  * was 0x10 to rep_stat for LTS
					  */
#define CY_IGNORE_VALUE             0xFFFF
#define CY_CMD_RDY_BIT              0x40

#define CY_REG_OP_START             0
#define CY_REG_SI_START             0
#define CY_REG_OP_END               0x20
#define CY_REG_SI_END               0x20

#ifdef CY_USE_TMA400
#define CY_TCH_CRC_LOC_TMA400       5884 /* location of CRC in touch EBID */
#endif /* --CY_USE_TMA400 */

/* register field lengths */
#define CY_NUM_REVCTRL              8
#define CY_NUM_MFGID                8
#define CY_NUM_TCHREC               10
#define CY_NUM_DDATA                32
#define CY_NUM_MDATA                64
#define CY_TMA884_MAX_BYTES         255 /*
					  * max reg access for TMA884
					  * in config mode
					  */
#define CY_TMA400_MAX_BYTES         512 /*
					  * max reg access for TMA400
					  * in config mode
					  */

/* touch event id codes */
#define CY_GET_EVENTID(reg)         ((reg & 0x60) >> 5)
#define CY_GET_TRACKID(reg)         (reg & 0x1F)
#define CY_NOMOVE                   0
#define CY_TOUCHDOWN                1
#define CY_MOVE                     2
#define CY_LIFTOFF                  3

#define CY_CFG_BLK_SIZE             126
#define CY_EBID_ROW_SIZE_DFLT       128

#define CY_BL_VERS_SIZE             12
#define CY_NUM_TMA400_TT_CFG_BLK    51 /* Rev84 mapping */

#if defined(CY_USE_FORCE_LOAD) || defined(CY_USE_INCLUDE_FBL)
#define CY_BL_FW_NAME_SIZE          NAME_MAX
#endif

#ifdef CY_USE_INCLUDE_FBL
#define CY_BL_TXT_FW_IMG_SIZE       128261
#define CY_BL_BIN_FW_IMG_SIZE       128261
#define CY_NUM_PKG_PKT              4
#define CY_NUM_PKT_DATA             32
#define CY_MAX_PKG_DATA             (CY_NUM_PKG_PKT * CY_NUM_PKT_DATA)
#define CY_MAX_IC_BUF               256
#endif /* --CY_USE_INCLUDE_FBL */

#ifdef CY_USE_REG_ACCESS
#define CY_RW_REGID_MAX             0xFFFF
#define CY_RW_REG_DATA_MAX          0xFF
#endif

/* abs settings */
/* abs value offset */
#define CY_SIGNAL_OST   0
#define CY_MIN_OST      1
#define CY_MAX_OST      2
#define CY_FUZZ_OST     3
#define CY_FLAT_OST     4
/* axis signal offset */
#define CY_NUM_ABS_SET  5 /* number of abs signals */
#define CY_ABS_X_OST    0
#define CY_ABS_Y_OST    1
#define CY_ABS_P_OST    2
#define CY_ABS_W_OST    3
#define CY_ABS_ID_OST   4

/* touch record system information offset masks and shifts */
#define CY_SIZE_FIELD_MASK          0x1F
#define CY_BOFS_MASK                0xE0
#define CY_BOFS_SHIFT               5

enum cyttsp4_driver_state {
	CY_IDLE_STATE,		/* IC cannot be reached */
	CY_READY_STATE,		/* pre-operational; ready to go to ACTIVE */
	CY_ACTIVE_STATE,	/* app is running, IC is scanning */
	CY_SLEEP_STATE,		/* app is running, IC is idle */
	CY_BL_STATE,		/* bootloader is running */
	CY_SYSINFO_STATE,	/* switching to sysinfo mode */
	CY_CMD_STATE,		/* command initiation mode */
	CY_EXIT_BL_STATE,	/* sync bl heartbeat to app ready int */
	CY_TRANSFER_STATE,	/* changing states */
	CY_INVALID_STATE	/* always last in the list */
};

static  char * cyttsp4_driver_state_string[] = {
	/* Order must match enum cyttsp4_driver_state above */
	"IDLE",
	"READY",
	"ACTIVE",
	"SLEEP",
	"BOOTLOADER",
	"SYSINFO",
	"CMD",
	"EXIT_BL",
	"TRANSFER",
	"INVALID"
};

enum cyttsp4_controller_mode {
	CY_MODE_BOOTLOADER,
	CY_MODE_SYSINFO,
	CY_MODE_OPERATIONAL,
	CY_MODE_CONFIG,
	CY_MODE_NUM
};

enum cyttsp4_ic_grpnum {
	CY_IC_GRPNUM_RESERVED = 0,
	CY_IC_GRPNUM_CMD_REGS,
	CY_IC_GRPNUM_TCH_REP,
	CY_IC_GRPNUM_DATA_REC,
	CY_IC_GRPNUM_TEST_REC,
	CY_IC_GRPNUM_PCFG_REC,
	CY_IC_GRPNUM_TCH_PARM_VAL,
	CY_IC_GRPNUM_TCH_PARM_SIZ,
	CY_IC_GRPNUM_RESERVED1,
	CY_IC_GRPNUM_RESERVED2,
	CY_IC_GRPNUM_OPCFG_REC,
	CY_IC_GRPNUM_DDATA_REC,
	CY_IC_GRPNUM_MDATA_REC,
	CY_IC_GRPNUM_TEST_REGS,
	CY_IC_GRPNUM_BTN_KEYS,
	CY_IC_GRPNUM_NUM
};

enum cyttsp4_ic_op_mode_commands {
	CY_GET_PARAM_CMD = 0x02,
	CY_SET_PARAM_CMD = 0x03,
	CY_GET_CFG_BLK_CRC = 0x05,
};

enum cyttsp4_ic_config_mode_commands {
	CY_GET_EBID_ROW_SIZE = 0x02,
	CY_READ_EBID_DATA = 0x03,
	CY_WRITE_EBID_DATA = 0x04,
	CY_VERIFY_EBID_CRC = 0x11,
};

#ifdef CY_USE_TMA400
enum cyttsp4_ic_ebid {
	CY_TCH_PARM_EBID = 0x00,
	CY_MDATA_EBID = 0x01,
	CY_DDATA_EBID = 0x02,
};
#endif /* --CY_USE_TMA400 */

enum cyttsp4_flags {
	CY_FLAG_NONE = 0x00,
	CY_FLAG_HOVER = 0x04,
#ifdef CY_USE_DEBUG_TOOLS
	CY_FLAG_FLIP = 0x08,
	CY_FLAG_INV_X = 0x10,
	CY_FLAG_INV_Y = 0x20,
#endif /* --CY_USE_DEBUG_TOOLS */
};

enum cyttsp4_event_id {
	CY_EV_NO_EVENT = 0,
	CY_EV_TOUCHDOWN = 1,
	CY_EV_MOVE = 2,		/* significant displacement (> act dist) */
	CY_EV_LIFTOFF = 3,	/* record reports last position */
};

enum cyttsp4_object_id {
	CY_OBJ_STANDARD_FINGER = 0,
	CY_OBJ_LARGE_OBJECT = 1,
	CY_OBJ_STYLUS = 2,
	CY_OBJ_HOVER = 3,
};

/* test modes and data */
#ifdef CY_USE_TMA400
enum cyttsp4_test_cmd {
	CY_TEST_CMD_NULL = 0,
	CY_TEST_CMD_RESERVED_01,
	CY_TEST_CMD_GET_CONFIG_BLOCK_SIZE,
	CY_TEST_CMD_READ_CONFIG_BLOCK,
	CY_TEST_CMD_WRITE_CONFIG_BLOCK,
	CY_TEST_CMD_RESERVED_05,
	CY_TEST_CMD_LOAD_SELF_TEST_DATA,
	CY_TEST_CMD_RUN_SELF_TEST,
	CY_TEST_CMD_GET_SELF_TEST_RESULTS,
	CY_TEST_CMD_CAL_IDACS,
	CY_TEST_CMD_INIT_BASELINES,
	CY_TEST_CMD_EXECUTE_PANEL_SCAN,
	CY_TEST_CMD_RETRIEVE_PANEL_SCAN,
	CY_TEST_CMD_START_SENSOR_DATA_MODE,
	CY_TEST_CMD_STOP_SENSOR_DATA_MODE,
	CY_TEST_CMD_SET_INT_PIN_MODE,
	CY_TEST_CMD_RETRIEVE_DATA_STRUCT,
	CY_TEST_CMD_VERIFY_CONFIG_BLOCK_CRC,
	CY_TEST_CMD_RESERVED_12,
	CY_TEST_CMD_RESERVED_13,
	CY_TEST_CMD_RESERVED_14,
	CY_TEST_CMD_RESERVED_15,
	CY_TEST_CMD_RESERVED_16,
	CY_TEST_CMD_RESERVED_17,
	CY_TEST_CMD_RESERVED_18,
	CY_TEST_CMD_RESERVED_19,
	CY_TEST_CMD_RESERVED_1A,
	CY_TEST_CMD_RESERVED_1B,
	CY_TEST_CMD_RESERVED_1C,
	CY_TEST_CMD_RESERVED_1D,
	CY_TEST_CMD_RESERVED_1E, 
	CY_TEST_CMD_RESERVED_1F,
};
#endif /* --CY_USE_TMA400 */

/* test mode NULL command driver codes; D */
enum cyttsp4_null_test_cmd_code {
	CY_NULL_CMD_NULL = 0,
	CY_NULL_CMD_MODE,
	CY_NULL_CMD_STATUS_SIZE,
	CY_NULL_CMD_HANDSHAKE,
};

enum cyttsp_test_mode {
	CY_TEST_MODE_NORMAL_OP = 0,	/* Send touch data to OS; normal op */
	CY_TEST_MODE_CAT,		/* Configuration and Test */
	CY_TEST_MODE_CLOSED_UNIT,	/* Send scan data to sysfs */
};

struct cyttsp4_test_mode {
	int cur_mode;
	int cur_cmd;
	size_t cur_status_size;
};


uint8_t CY_TEST_BUFF[X_SENSOR_NUM*Y_SENSOR_NUM];


/* GEN4/SOLO Operational interface definitions */
enum cyttsp4_tch_abs {	/* for ordering within the extracted touch data array */
	CY_TCH_X = 0,	/* X */
	CY_TCH_Y,	/* Y */
	CY_TCH_P,	/* P (Z) */
	CY_TCH_T,	/* TOUCH ID */
	CY_TCH_E,	/* EVENT ID */
	CY_TCH_O,	/* OBJECT ID */
	CY_TCH_W,	/* SIZE (SOLO - Corresponds to TOUCH_MAJOR) */
#ifdef CY_USE_TMA400_SP2
#ifdef CY_USE_TMA400
	CY_TCH_MAJ,	/* TOUCH_MAJOR */
	CY_TCH_MIN,	/* TOUCH_MINOR */
	CY_TCH_OR,	/* ORIENTATION */
#endif /* --CY_USE_TMA400 */
#endif /* --CY_USE_TMA400_SP2 */
	CY_TCH_NUM_ABS
};
static const char * const cyttsp4_tch_abs_string[] = {
	/* Order must match enum cyttsp4_tch_descriptor above */
	"X",
	"Y",
	"P",
	"T",
	"E",
	"O",
	"W",
#ifdef CY_USE_TMA400_SP2
#ifdef CY_USE_TMA400
	"MAJ",
	"MIN",
	"OR",
#endif /* --CY_USE_TMA400 */
#endif /* --CY_USE_TMA400_SP2 */
	"INVALID"
};

#ifdef CY_USE_TMA400
#ifdef CY_USE_TMA400_SP2
#define CY_NUM_NEW_TCH_FIELDS	3
#else
#define CY_NUM_NEW_TCH_FIELDS	0
#endif /* --CY_USE_TMA400_SP2 */
#endif /* --CY_USE_TMA400 */

#ifdef CY_USE_TMA884
#define CY_NUM_NEW_TCH_FIELDS	0
#endif /* --CY_USE_TMA884 */

#define CY_NUM_OLD_TCH_FIELDS	(CY_TCH_NUM_ABS - CY_NUM_NEW_TCH_FIELDS)

struct cyttsp4_touch {
	int abs[CY_TCH_NUM_ABS];
};

/* TMA400 TT_CFG interface definitions */
struct cyttsp4_tma400A_config_crc {
	u8 CONFIG_CRC[4];
};
struct cyttsp4_tma400A_sdk_controller_config {
	u8 SDK_CTRL_CFG_SIZE[4];
	u8 X_LEN_PHY[2];
	u8 Y_LEN_PHY[2];
	u8 HST_MODE0;
	u8 ACT_DIST0;
	u8 SCAN_TYP0;
	u8 ACT_INTRVL0;
	u8 ACT_LFT_INTRVL0;
	u8 Reserved_1;
	u8 TCH_TMOUT0[2];
	u8 LP_INTRVL0[2];
	u8 PWR_CFG;
	u8 INT_CFG;
	u8 INT_PULSE_DATA;
	u8 OPMODE_CFG;
	u8 HANDSHAKE_TIMEOUT[2];
	u8 TIMER_CAL_INTERVAL;
	u8 Reserved_2;
	u8 RP2P_MIN[2];
	u8 ILEAK_MAX[2];
	u8 RFB_P2P[2];
	u8 RFB_EXT[2];
	u8 IDACOPEN_LOW;
	u8 IDACOPEN_HIGH;
	u8 GIDAC_OPEN;
	u8 GAIN_OPEN;
	u8 POST_CFG;
	u8 GESTURE_CFG;
	u8 GEST_EN[32];
	u8 Reserved_align[52];
} __packed;

struct cyttsp4_tma400A_tt_cfg {
	struct cyttsp4_tma400A_config_crc config_crc;
	struct cyttsp4_tma400A_sdk_controller_config sdk_controller_config;
} __packed;

/* TTSP System Information interface definitions */
struct cyttsp4_cydata {
	u8 ttpidh;
	u8 ttpidl;
	u8 fw_ver_major;
	u8 fw_ver_minor;
	u8 revctrl[CY_NUM_REVCTRL];
	u8 blver_major;
	u8 blver_minor;
	u8 jtag_si_id3;
	u8 jtag_si_id2;
	u8 jtag_si_id1;
	u8 jtag_si_id0;
	u8 mfgid_sz;
	u8 mfg_id[CY_NUM_MFGID];
	u8 cyito_idh;
	u8 cyito_idl;
	u8 cyito_verh;
	u8 cyito_verl;
	u8 ttsp_ver_major;
	u8 ttsp_ver_minor;
	u8 device_info;
} __packed;

struct cyttsp4_test {
	u8 post_codeh;
	u8 post_codel;
} __packed;

struct cyttsp4_pcfg {
	u8 electrodes_x;
	u8 electrodes_y;
	u8 len_xh;	u8 len_xl;
	u8 len_yh;
	u8 len_yl;
	u8 axis_xh;
	u8 axis_xl;
	u8 axis_yh;
	u8 axis_yl;
	u8 max_zh;
	u8 max_zl;
} __packed;

struct cyttsp4_tch_rec_params {
	u8 loc;
	u8 size;
} __packed;

struct cyttsp4_opcfg {
	u8 cmd_ofs;
	u8 rep_ofs;
	u8 rep_szh;
	u8 rep_szl;
	u8 num_btns;
	u8 tt_stat_ofs;
	u8 obj_cfg0;
	u8 max_tchs;
	u8 tch_rec_siz;
	struct cyttsp4_tch_rec_params tch_rec_old[CY_NUM_OLD_TCH_FIELDS];
	u8 btn_rec_siz;	/* btn record size (in bytes) */
	u8 btn_diff_ofs;/* btn data loc ,diff counts, (Op-Mode byte ofs) */
	u8 btn_diff_siz;/* btn size of diff counts (in bits) */
#ifdef CY_USE_TMA400
	struct cyttsp4_tch_rec_params tch_rec_new[CY_NUM_NEW_TCH_FIELDS];
#endif /* --CY_USE_TMA400 */
} __packed;

struct cyttsp4_sysinfo_data {
	u8 hst_mode;
	u8 reserved;
	u8 map_szh;
	u8 map_szl;
	u8 cydata_ofsh;
	u8 cydata_ofsl;
	u8 test_ofsh;
	u8 test_ofsl;
	u8 pcfg_ofsh;
	u8 pcfg_ofsl;
	u8 opcfg_ofsh;
	u8 opcfg_ofsl;
	u8 ddata_ofsh;
	u8 ddata_ofsl;
	u8 mdata_ofsh;
	u8 mdata_ofsl;
} __packed;

struct cyttsp4_sysinfo_ptr {
	struct cyttsp4_cydata *cydata;
	struct cyttsp4_test *test;
	struct cyttsp4_pcfg *pcfg;
	struct cyttsp4_opcfg *opcfg;
	struct cyttsp4_ddata *ddata;
	struct cyttsp4_mdata *mdata;
} __packed;

struct cyttsp4_tch_abs_params {
	size_t ofs;	/* abs byte offset */
	size_t size;	/* size in bits */
	size_t max;	/* max value */
	size_t bofs;	/* bit offset */
};

struct cyttsp4_sysinfo_ofs {
	size_t cmd_ofs;
	size_t rep_ofs;
	size_t rep_sz;
	size_t num_btns;
	size_t num_btn_regs;	/* ceil(num_btns/4) */
	size_t tt_stat_ofs;
	size_t tch_rec_siz;
	size_t obj_cfg0;
	size_t max_tchs;
	size_t mode_size;
	size_t data_size;
	size_t map_sz;
	size_t cydata_ofs;
	size_t test_ofs;
	size_t pcfg_ofs;
	size_t opcfg_ofs;
	size_t ddata_ofs;
	size_t mdata_ofs;
	size_t cydata_size;
	size_t test_size;
	size_t pcfg_size;
	size_t opcfg_size;
	size_t ddata_size;
	size_t mdata_size;
	size_t btn_keys_size;
	struct cyttsp4_tch_abs_params tch_abs[CY_TCH_NUM_ABS];	
	size_t btn_rec_siz;		/* btn record size (in bytes) */
	size_t btn_diff_ofs;	/* btn data loc ,diff counts, (Op-Mode byte ofs) */
	size_t btn_diff_siz;	/* btn size of diff counts (in bits) */
};

/* button to keycode support */
#define CY_NUM_BTN_PER_REG	4
#define CY_NUM_BTN_EVENT_ID	4
#define CY_BITS_PER_BTN		2


#define Init_Resume_T 100

int  ResumeCnt_T;
bool Pow_ON_T;

enum cyttsp4_btn_state {
	CY_BTN_RELEASED = 0,
	CY_BTN_PRESSED = 1,
	CY_BTN_NUM_STATE
};

struct cyttsp4_btn {
	bool enabled;
	int state;	/* CY_BTN_PRESSED, CY_BTN_RELEASED */
	int key_code;
};

/* driver context structure definitions */
#ifdef CY_USE_INCLUDE_FBL
struct cyttsp4_dbg_pkg {
	bool ready;
	int cnt;
	u8 data[CY_MAX_PKG_DATA];
};
#endif /* --CY_USE_INCLUDE_FBL */

enum {
	CY_DIFF,
	CY_RAWCOUNT,
	CY_SELF_RAW,
	CY_SELF_DIFF	
};

struct cyttsp4 {
	struct device *dev;
	int irq;
	struct input_dev *input;
	struct mutex data_lock;		/* prevent concurrent accesses */
	struct workqueue_struct		*cyttsp4_wq;
	
	struct work_struct			cyttsp4_resume_startup_work;
#ifdef CY_USE_IRQ_WORKQUEUE
	struct work_struct			cyttsp4_irq_work;
#endif

	char phys[32];
	const struct bus_type *bus_type;
	const struct touch_platform_data *platform_data;
	u8 *xy_mode;			/* operational mode and status regs */
	u8 *xy_data;			/* operational touch regs */
	u8 *xy_data_touch1;		/* includes 1-byte for tt_stat */
	u8 *btn_rec_data;		/* button diff count data */
	struct cyttsp4_bus_ops *bus_ops;
	struct cyttsp4_sysinfo_data sysinfo_data;
	struct cyttsp4_sysinfo_ptr sysinfo_ptr;
	struct cyttsp4_sysinfo_ofs si_ofs;
	struct cyttsp4_btn *btn;
	struct cyttsp4_test_mode test;
	struct completion int_running;
	struct completion si_int_running;
	struct completion ready_int_running;
	enum cyttsp4_driver_state driver_state;
	enum cyttsp4_controller_mode current_mode;
	bool irq_enabled;
	bool powered; /* protect against multiple open */
	bool was_suspended;
	bool switch_flag;
	bool soft_reset_asserted;
	u16 flags;
	size_t max_config_bytes;
	size_t ebid_row_size;
	int num_prv_tch;
#ifdef CY_USE_TMA400
	bool starting_up;
#endif /* --CY_USE_TMA400 */
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
#ifdef CY_USE_WATCHDOG
	struct work_struct work;
	struct timer_list timer;
#endif
#if defined(CY_USE_FORCE_LOAD) || defined(CY_USE_INCLUDE_FBL)
	bool waiting_for_fw;
	char *fwname;
#endif
#ifdef CY_USE_INCLUDE_FBL
	u8 *pr_buf;
	bool debug_upgrade;
	int ic_grpnum;
	int ic_grpoffset;
	bool ic_grptest;
#endif /* --CY_USE_INCLUDE_FBL */
#ifdef CY_USE_REG_ACCESS
	size_t rw_regid;
#endif
};

//---------------------------------------------------------------------//
// wcjeong - p11309 - 0411

typedef struct
{
	int abs[CY_TCH_NUM_ABS];
	int mode;

} report_finger_info_t;

static int is_sleep_state = false;
static int last_charger_mode = -1;
static int pantech_fw_download_flag = false;

static report_finger_info_t fingerInfo[CYTTSP_MAX_TOUCHNUM+1];
static void _pantech_reset_fingerinfo(void);
static void _pantech_release_eventhub(struct cyttsp4 *ts);
//---------------------------------------------------------------------//

#ifdef CY_USE_SOFT_SUSPEND_RESUME_MODE
static int cyttsp4_hw_reset(void);
static int cyttsp4_hw_recov(int on);
static int cyttsp4_irq_stat(void);
#endif

#ifdef CY_USE_TMA400
#define CY_I2C_TCH_ADR	0x24
#define CY_I2C_LDR_ADR	0x24
#endif /* --CY_USE_TMA400 */

#ifdef CY_USE_TMA884
#define CY_I2C_TCH_ADR	0x67
#define CY_I2C_LDR_ADR	0x69
#endif /* --CY_USE_TMA884 */

#define CY_ABS_MIN_X CY_MINX
#define CY_ABS_MIN_Y CY_MINY
#define CY_ABS_MIN_P 0
#define CY_ABS_MIN_W 0
#ifdef CY_USE_TMA400
#define CY_ABS_MIN_T 0
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
#define CY_ABS_MIN_T 1
#endif /* --CY_USE_TMA884 */

#define CY_ABS_MAX_X CY_MAXX
#define CY_ABS_MAX_Y CY_MAXY
#define CY_ABS_MAX_P 255
#define CY_ABS_MAX_W 255
#ifdef CY_USE_TMA400
#define CY_ABS_MAX_T 15
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
#define CY_ABS_MAX_T 10
#endif /* --CY_USE_TMA884 */
#define CY_IGNORE_VALUE 0xFFFF

//	tagPlatform_data
#if defined(CONFIG_MACH_MSM8960_SIRIUSLTE)
#if (BOARD_VER < WS20)
	#include "./siriuslte/ws10/cyttsp4_img.h"
	#include "./siriuslte/ws10/cyttsp4_params.h"
#else
	#include "./siriuslte/ws20/cyttsp4_img.h"
	#include "./siriuslte/ws20/cyttsp4_params.h"
#endif
#endif

static struct touch_settings cyttsp4_sett_param_regs = {
	.data = (uint8_t *)&cyttsp4_param_regs[0],
	.size = ARRAY_SIZE(cyttsp4_param_regs),
	.tag = 0,
};

static struct touch_settings cyttsp4_sett_param_size = {
	.data = (uint8_t *)&cyttsp4_param_size[0],
	.size = ARRAY_SIZE(cyttsp4_param_size),
	.tag = 0,
};

/* Design Data Table */
static u8 cyttsp4_ddata[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23, 24 /* test padding
	, 25, 26, 27, 28, 29, 30, 31 */
};

static struct touch_settings cyttsp4_sett_ddata = {
	.data = (uint8_t *)&cyttsp4_ddata[0],
	.size = ARRAY_SIZE(cyttsp4_ddata),
	.tag = 0,
};

/* Manufacturing Data Table */
static u8 cyttsp4_mdata[] = {
	65, 64, /* test truncation */63, 62, 61, 60, 59, 58, 57, 56, 55,
	54, 53, 52, 51, 50, 49, 48,
	47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32,
	31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16,
	15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
};

static struct touch_settings cyttsp4_sett_mdata = {
	.data = (uint8_t *)&cyttsp4_mdata[0],
	.size = ARRAY_SIZE(cyttsp4_mdata),
	.tag = 0,
};

#if 0
/* Button to keycode conversion */
static u16 cyttsp4_btn_keys[] = {
	/* use this table to map buttons to keycodes (see input.h) */
	KEY_HOME,		/* 102 */
	KEY_MENU,		/* 139 */
	KEY_BACK,		/* 158 */
	KEY_SEARCH,		/* 217 */
	KEY_VOLUMEDOWN,	/* 114 */
	KEY_VOLUMEUP,	/* 115 */
	KEY_CAMERA,		/* 212 */
	KEY_POWER		/* 116 */
};

static struct touch_settings cyttsp4_sett_btn_keys = {
	.data = (uint8_t *)&cyttsp4_btn_keys[0],
	.size = ARRAY_SIZE(cyttsp4_btn_keys),
	.tag = 0,
};
#endif

#ifdef CY_USE_INCLUDE_FBL
static struct touch_firmware cyttsp4_firmware = {
	.img = cyttsp4_img,
	.size = ARRAY_SIZE(cyttsp4_img),
	.ver = cyttsp4_ver,
	.vsize = ARRAY_SIZE(cyttsp4_ver),
};
#else
static struct touch_firmware cyttsp4_firmware = {
	.img = NULL,
	.size = 0,
	.ver = NULL,
	.vsize = 0,
};
#endif

static const uint16_t cyttsp4_abs[] = {
	ABS_MT_POSITION_X, CY_ABS_MIN_X, CY_ABS_MAX_X, 0, 0,
	ABS_MT_POSITION_Y, CY_ABS_MIN_Y, CY_ABS_MAX_Y, 0, 0,
	ABS_MT_PRESSURE, CY_ABS_MIN_P, CY_ABS_MAX_P, 0, 0,
#ifdef CY_USE_TMA400
	CY_IGNORE_VALUE, CY_ABS_MIN_W, CY_ABS_MAX_W, 0, 0,
#endif /* --CY_USE_TMA400 */
#ifndef CY_USE_TMA400
	ABS_MT_TOUCH_MAJOR, CY_ABS_MIN_W, CY_ABS_MAX_W, 0, 0,
#endif /* --CY_USE_TMA400 */
	ABS_MT_TRACKING_ID, CY_ABS_MIN_T, CY_ABS_MAX_T, 0, 0,
};

struct touch_framework cyttsp4_framework = {
	.abs = (uint16_t *)&cyttsp4_abs[0],
	.size = ARRAY_SIZE(cyttsp4_abs),
	.enable_vkeys = 0,
};

static struct touch_platform_data cyttsp4_pdata = {
	.sett = {
		NULL,	/* Reserved */
		NULL,	/* Command Registers */
		NULL,	/* Touch Report */
		NULL,	/* Cypress Data Record */
		NULL,	/* Test Record */
		NULL,	/* Panel Configuration Record */
		&cyttsp4_sett_param_regs,
		&cyttsp4_sett_param_size,
		NULL,	/* Reserved */
		NULL,	/* Reserved */
		NULL,	/* Operational Configuration Record */
		&cyttsp4_sett_ddata, /* Design Data Record */
		&cyttsp4_sett_mdata, /* Manufacturing Data Record */
		NULL,	/* Config and Test Registers */
		NULL,   // &cyttsp4_sett_btn_keys,	/* button-to-keycode table */
	},
	.fw = &cyttsp4_firmware,
	.frmwrk = &cyttsp4_framework,
	.addr = {CY_I2C_TCH_ADR, CY_I2C_LDR_ADR},
	.flags = 0x00,
#ifdef CY_USE_SOFT_SUSPEND_RESUME_MODE
	.hw_reset = cyttsp4_hw_reset,
	.hw_recov = cyttsp4_hw_recov,
	.irq_stat = cyttsp4_irq_stat,
#endif

};

#if defined(CY_AUTO_LOAD_FW) || defined(CY_USE_FORCE_LOAD) || defined(CY_USE_INCLUDE_FBL)
static int _cyttsp4_load_app(struct cyttsp4 *ts, const u8 *fw, int fw_size);
#endif /* CY_AUTO_LOAD_FW || CY_USE_FORCE_LOAD || CY_USE_INCLUDE_FBL */
static int _cyttsp4_ldr_exit(struct cyttsp4 *ts);
static int _cyttsp4_startup(struct cyttsp4 *ts);
static int _cyttsp4_get_ic_crc(struct cyttsp4 *ts, enum cyttsp4_ic_ebid ebid, u8 *crc_h, u8 *crc_l);
static irqreturn_t cyttsp4_irq(int irq, void *handle);
static int _cyttsp4_set_mode(struct cyttsp4 *ts, u8 new_mode);
static int _cyttsp4_set_device_mode(struct cyttsp4 *ts, u8 new_mode, u8 new_cur_mode, char *mode);
static int _cyttsp4_set_operational_mode(struct cyttsp4 *ts);
static int _cyttsp4_set_config_mode(struct cyttsp4 *ts);
static int cyttsp4_chargermode(struct cyttsp4 *ts, uint8_t enable);
static int cyttsp4_testmode(struct cyttsp4 *ts, int mode);
static int _cyttsp4_set_config_mode(struct cyttsp4 *ts);
static int _cyttsp4_read_block_data(struct cyttsp4 *ts, u16 command, size_t length, void *buf, int i2c_addr, bool use_subaddr);
static int _cyttsp4_write_block_data(struct cyttsp4 *ts, u16 command, size_t length, const void *buf, int i2c_addr, bool use_subaddr);
static void _cyttsp4_change_state(struct cyttsp4 *ts, enum cyttsp4_driver_state new_state);

//	p11309 - release empty cal data for after download.
static int cyttsp4_need_manual_calibration = false;
static int cyttsp4_manual_calibration(struct cyttsp4 *ts);
static void _cyttsp4_wait_for_complete_bit(struct cyttsp4 *ts);

#ifdef CY_USE_INCLUDE_FBL
#ifdef CY_USE_TMA400
static int _cyttsp4_store_tch_param_tma400(struct cyttsp4 *ts,
	u8 *ic_buf, size_t length);
static int _cyttsp4_calc_ic_crc_tma400(struct cyttsp4 *ts,
	enum cyttsp4_ic_ebid ebid, u8 *crc_h, u8 *crc_l, bool read_back_verify);
#endif /* --CY_USE_TMA400 */
#endif /* --CY_USE_INCLUDE_FBL */

#if 0
static int TSP_Restart(void);
#endif

static void off_hw_setting(void);
static int init_hw_setting(void);

#ifdef  SKY_PROCESS_CMD_KEY
static long ts_fops_ioctl(struct file *filp,unsigned int cmd, unsigned long arg);

struct cyttsp4 *cyttsp4_data = NULL;
static struct file_operations ts_fops = {
	.owner = THIS_MODULE,	
	.unlocked_ioctl = ts_fops_ioctl,
};

static struct miscdevice touch_event = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "touch_fops",
	.fops = &ts_fops,
};

// To be depreciated.
static long ts_fops_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	
	dbg("[CYTTSP] ts_fops_ioctl (%d, %d) \n",(int)cmd,(int)arg);	
	
	switch (cmd) 
	{
		case TOUCH_IOCTL_READ_LASTKEY:
			break;
		case TOUCH_IOCTL_DO_KEY:
			dbg("TOUCH_IOCTL_DO_KEY  = %d\n",(int)argp);			
			if ( (int)argp == 0x20a )
				input_report_key(cyttsp4_data->input, 0xe3, 1);
			else if ( (int)argp == 0x20b )
				input_report_key(cyttsp4_data->input, 0xe4, 1);
			else
				input_report_key(cyttsp4_data->input, (int)argp, 1);
			input_sync(cyttsp4_data->input);
			break;
		case TOUCH_IOCTL_RELEASE_KEY:		
			dbg("TOUCH_IOCTL_RELEASE_KEY  = %d\n",(int)argp);
			if ( (int)argp == 0x20a )
				input_report_key(cyttsp4_data->input, 0xe3, 0);
			else if ( (int)argp == 0x20b )
				input_report_key(cyttsp4_data->input, 0xe4, 0);
			else
				input_report_key(cyttsp4_data->input, (int)argp, 0);
			input_sync(cyttsp4_data->input);
			break;		
		case TOUCH_IOCTL_DEBUG:
			dbg("Touch Screen Read Queue ~!!\n");			
			break;
		case TOUCH_IOCTL_CLEAN:
			dbg("Touch Screen Previous Data Clean ~!!\n");
			break;
		case TOUCH_IOCTL_RESTART:
			dbg("Touch Screen Calibration Restart ~!!\n");	
			break;
		case TOUCH_IOCTL_PRESS_TOUCH:
			input_report_key(cyttsp4_data->input, BTN_TOUCH, 1);
			input_report_abs(cyttsp4_data->input, ABS_MT_TOUCH_MAJOR, 255);
			input_report_abs(cyttsp4_data->input, ABS_MT_POSITION_X, (int)(arg&0x0000FFFF));
			input_report_abs(cyttsp4_data->input, ABS_MT_POSITION_Y, (int)((arg >> 16) & 0x0000FFFF));
			input_report_abs(cyttsp4_data->input, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(cyttsp4_data->input);
			input_sync(cyttsp4_data->input);
			break;
		case TOUCH_IOCTL_RELEASE_TOUCH:		
			input_report_key(cyttsp4_data->input, BTN_TOUCH, 0);			
			input_report_abs(cyttsp4_data->input, ABS_MT_TOUCH_MAJOR, 0);
			input_report_abs(cyttsp4_data->input, ABS_MT_POSITION_X, (int)(arg&0x0000FFFF));
			input_report_abs(cyttsp4_data->input, ABS_MT_POSITION_Y, (int)((arg >> 16) & 0x0000FFFF));
			input_report_abs(cyttsp4_data->input, ABS_MT_WIDTH_MAJOR, 1);
			input_mt_sync(cyttsp4_data->input);
			input_sync(cyttsp4_data->input); 
			break;
		case TOUCH_IOCTL_CHARGER_MODE:
			
			if ( is_sleep_state == false ) 
				cyttsp4_chargermode(cyttsp4_data,arg);

			break;
		case POWER_OFF:
			break;
		case TOUCH_IOCTL_DELETE_ACTAREA:
			break;
		case TOUCH_IOCTL_RECOVERY_ACTAREA:
			break;
		case TOUCH_IOCTL_STYLUS_MODE:
			break;
		case TOUCH_CHARGE_MODE_CTL:
			break;
		default:
			break;
	}

	return true;
}
#endif

#ifdef TOUCH_IO /* File IO */
static int open(struct inode *inode, struct file *file);
static int release(struct inode *inode, struct file *file);
static ssize_t read(struct file *file, char *buf, size_t count, loff_t *ppos);
static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *ppos);
static long ioctl(struct file *file, unsigned int cmd, unsigned long arg);
	
static struct file_operations fops = 
{
	.owner =    THIS_MODULE,
	.unlocked_ioctl = ioctl,
	.read =     read,
	.write =    write,
	.open =     open,
	.release =  release
};

static struct miscdevice touch_io = 
{
	.minor =    MISC_DYNAMIC_MINOR,
	.name =     "touch_dbg_io",
	.fops =     &fops
};

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
	int nBufSize=0;
	if((size_t)(*ppos) > 0) return 0;
	if(buf!=NULL)
	{
		nBufSize=strlen(buf);

		if(strncmp(buf, "debug", 5)==0)
		{			
			DebugON=1;	 
		}
		if(strncmp(buf, "debugoff", 8)==0)
		{			
			DebugON=0;	    
		}
	}
	*ppos +=nBufSize;
	return nBufSize;
}
static ssize_t read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	return 0; 
}

static long ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int return_value = -1;
	int object_type, field_index;
	// Read Command 
	// Write, Etc.

	switch (cmd)
	{
		case GET_TOUCH_CONFIG:
			object_type 	= (int)((arg & 0x0000FF00) >> 8);
			field_index 	= (int)((arg & 0x000000FF) >> 0);
			
			if (object_type == 9 && field_index == 3) {
				return_value = Y_SENSOR_NUM ;
			}
			if (object_type == 9 && field_index == 4) {
				return_value = X_SENSOR_NUM ;
			}
			if (object_type == 9 && field_index == 9) {
				return_value = 1;
			}
			return return_value;
		case DIAG_DEBUG:
			/*
			 * Run Diag and save result into reference_data array when arg. is 5010 or 5011. 
			 * Returns diag result when the arg. is in range of 0~223. 
			 */
			if (arg == TOUCH_IOCTL_DIAG_DEBUG_DELTA) 	// call function
			{
			//	if(ResumeCnt_T>0)
				//	ResumeCnt_T--;
			//	else if((Pow_ON_T==1) &&(ResumeCnt_T==0))
					cyttsp4_testmode(cyttsp4_data, CY_DIFF);

				return 0;
			}
			if (arg == TOUCH_IOCTL_DIAG_DEBUG_REF) 
			{
				cyttsp4_testmode(cyttsp4_data, CY_RAWCOUNT);
				return 0;
			}
			else if (arg > (X_SENSOR_NUM*Y_SENSOR_NUM)-1)
			{
				return 0;
			}

			return (int8_t)(CY_TEST_BUFF[arg]);

		default:
			break;
	}
	return 0;
}
#endif /*TOUCH_IO*/

#ifdef CY_USE_TOUCH_MONITOR
//int reference_data[256] = { 0 };
static int monitor_open(struct inode *inode, struct file *file) {
    return 0; 
}

static ssize_t monitor_read(struct file *file, char *buf, size_t count, loff_t *ppos) {
    return 0; 
}

static ssize_t monitor_write(struct file *file, const char *buf, size_t count, loff_t *ppos) {
    //todo
    int nBufSize=0;
    dbg("%s: (+)\n",__func__);
    if((size_t)(*ppos) > 0) return 0;
    if(buf!=NULL)
    {
        nBufSize=strlen(buf);
        if(strncmp(buf, "queue", 5)==0)
        {
        }
        if(strncmp(buf, "debug", 5)==0)
        {			
            DebugON=1;	 
        }
        if(strncmp(buf, "debugoff", 8)==0)
        {			
            DebugON=0;	    
        }
        if(strncmp(buf, "checkcal", 8)==0)
        {			
        }
        if(strncmp(buf, "cal", 3)==0)
        {			
        }
        if(strncmp(buf, "save", 4)==0)
        {			
        }
        if(strncmp(buf, "reset1", 6)==0)
        {	
        }
        if(strncmp(buf, "reset2", 6)==0)
        {	
        }
        if(strncmp(buf, "reset3", 6)==0)
        {	
        }
    }
    *ppos +=nBufSize;
    dbg("%s: (-)\n",__func__);
    return nBufSize;
}

static int monitor_release(struct inode *inode, struct file *file) {
    //todo
    return 0; 
}

static void set_touch_config(int data, int object_type, int field_index) {
    //todo
    return;
}

static int get_touch_config(int object_type, int field_index) {
    return 0;
}

static void apply_touch_config(void) {
    //todo
    return;
}

static void reset_touch_config(void) { 
    //todo
    return;
}

typedef enum  {
    IOCTL_DEBUG_SUSPEND = 0,	
    IOCTL_DEBUG_RESUME = 1,	
    IOCTL_DEBUG_GET_TOUCH_ANTITOUCH_INFO = 2,
    IOCTL_DEBUG_TCH_CH = 3,	
    IOCTL_DEBUG_ATCH_CH = 4,	
    IOCTL_DEBUG_GET_CALIBRATION_CNT = 5,	
    IOCTL_DEBUG_CALIBRATE = 6,	
    IOCTL_DEBUG_CHARGER_MODE_ON = 7,
    IOCTL_DEBUG_CHARGER_MODE_OFF = 8,
} ioctl_debug_cmd;
static int ioctl_debug(unsigned long arg) {
    dbg("arg=%ld\n",arg);
    switch (arg)
    {
    case IOCTL_DEBUG_SUSPEND:
        // early suspend
        break;
    case IOCTL_DEBUG_RESUME:
        // late resume
        break;
    case IOCTL_DEBUG_GET_TOUCH_ANTITOUCH_INFO:
        // get anti touch
        break;
    case IOCTL_DEBUG_TCH_CH:
        // debug touch channel
        break;
    case IOCTL_DEBUG_ATCH_CH:
        // debug anti touch channel
        break;
    case IOCTL_DEBUG_GET_CALIBRATION_CNT:
        // debug calbration count
        break;
    case IOCTL_DEBUG_CALIBRATE:
        // calibrate chip
        break;
    case IOCTL_DEBUG_CHARGER_MODE_ON:
        // charger mode on
        dbg("charger mode on\n");
        cyttsp4_chargermode(cyttsp4_data,0x01);
        break;
    case IOCTL_DEBUG_CHARGER_MODE_OFF:
        // charger mode off
        dbg("charger mode off\n");
        cyttsp4_chargermode(cyttsp4_data,0x00);
        break;
    case 100:
        // debug touch status
        break;
    case 101:
        break;
    case 102:
        break;
    default:
        break;
    }
    return 0;
}

static int ioctl_diag_debug(unsigned long arg) {
    /*
     * Run Diag and save result into reference_data array when arg. is 5010 or 5011. 
     * Returns diag result when the arg. is in range of 0~223. 
     */
    if (arg == TOUCH_IOCTL_DIAG_DEBUG_DELTA) 
    {
        cyttsp4_testmode(cyttsp4_data, CY_DIFF);
        return 0;
    }
    else if (arg == TOUCH_IOCTL_DIAG_DEBUG_REF) 
    {
        cyttsp4_testmode(cyttsp4_data, CY_RAWCOUNT);
        return 0;
    }
    else if (arg == TOUCH_IOCTL_DIAG_DEBUG_OPERATEMODE) 
    {
        _cyttsp4_set_operational_mode(cyttsp4_data);
	    mutex_unlock(&cyttsp4_data->data_lock);
        return 0;
    }
    else if (arg > 224-1)
    {
        dbg(KERN_ERR "DIAG_DEBUG Argument error.!");
        return 0;
    }
    return (int8_t)(CY_TEST_BUFF[arg]);
}

static int send_reference_data(unsigned long arg){ 
    void __user *argp = (void __user *)arg;
    if (copy_to_user(argp, CY_TEST_BUFF, sizeof(int) * X_SENSOR_NUM * Y_SENSOR_NUM))
        return 0;
    else return 1;
}
#endif /* --CY_USE_TOUCH_MONITOR */

static int cyttsp4_chargermode(struct cyttsp4 *ts, uint8_t enable)
{
    int retval=0;

    uint8_t charger_set_cmd[4] = {0x03, 0x51, 0x01, (uint8_t) enable};
    uint8_t charger_get_cmd[2] = {0x02, 0x51};
    uint8_t charger_get_value[3]={0,};


	//////////////////////////////////////////////////////////////////////////
	//	p11309
	if ( last_charger_mode == enable ) {
		dbg(" charger mode; last = new...Skip...enable=%d\n", enable);
		return retval;
	}
	//////////////////////////////////////////////////////////////////////////
		

    dbg(" charger mode enable=%d\n", enable);
    
	retval = _cyttsp4_set_operational_mode(ts);
    if (retval < 0)
    {
        dev_err(ts->dev, "%s : Failed to switch to operation mode\n", __func__);
    }
	mutex_unlock(&cyttsp4_data->data_lock);
    msleep(10);

    // Set Charger Mode
    retval = _cyttsp4_write_block_data(ts, (u16)0x02, sizeof(charger_set_cmd), (uint8_t *) &charger_set_cmd, ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
    if (retval < 0)
    {
        dev_err(ts->dev, "%s : Charger cmd fail(ret=%d)\n", __func__, retval);
    }

    msleep(50);

    // Get Charger Mode Status
    retval = _cyttsp4_write_block_data(ts, (u16)0x02, sizeof(charger_get_cmd), (uint8_t *) &charger_get_cmd, ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
    if (retval < 0)
    {
        dev_err(ts->dev, "%s : Charger cmd fail(ret=%d)\n", __func__, retval);
    }

    msleep(50);

    retval = _cyttsp4_read_block_data(ts, (u16)0x03, sizeof(charger_get_value), (uint8_t *) &charger_get_value, ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
    if (retval < 0)
    {
        dev_err(ts->dev, "%s: Fail read charger status r=%d\n", __func__, retval);
    }

    if (charger_get_value[2] != (uint8_t)enable)
    {
        dev_err(ts->dev, "%s: Charger Status=%d error\n", __func__, charger_get_value[2]);
    }

	last_charger_mode = charger_get_value[2]; //p11309

    dbg("charger_get_value[2]=%d\n", charger_get_value[2]);
    return retval;
}

static int cyttsp4_testmode(struct cyttsp4 *ts, int mode)
{
	int retval=0;

	int i, j;
	int cnt=0;

	uint8_t diff_cmd[5] = {0x00, 0x00, 0x01, 0xF4, 0x02};

	uint8_t rawcount_cmd[5] = {0x00, 0x00, 0x03, 0x3c, 0x00};
	uint8_t self_raw_cmd[5] = {0x00, 0x00, 0x03, 0x3c, 0x04};
	uint8_t self_diff_cmd[5] = {0x00, 0x00, 0x03, 0x3c, 0x05};


	uint8_t cmd=0;

	uint8_t tmp_buff[X_SENSOR_NUM*Y_SENSOR_NUM];	
	memset(CY_TEST_BUFF, 0, sizeof(CY_TEST_BUFF));

	/* Enter Test Mode */
	if ( (ts->driver_state)!= CY_CMD_STATE )

	{
		retval = _cyttsp4_set_config_mode(ts);
		if (retval < 0)
		{
			dev_err(ts->dev, "%s : Failed to switch to config mode\n", __func__);
		}
	}

	/* Executes full panel scan */
	cmd = 0x0B;
	retval = _cyttsp4_write_block_data(ts, (u16)0x02, sizeof(cmd), (uint8_t *) &cmd, ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
	if (retval < 0)
	{
		dev_err(ts->dev, "%s : Scan cmd fail(ret=%d)\n", __func__, retval);
	}	

	msleep(10);

	/* Send CMD_DATA */
	if (mode == CY_DIFF)
	{
		retval = _cyttsp4_write_block_data(ts, (u16)0x03, sizeof(diff_cmd), &diff_cmd[0], ts->platform_data->addr[CY_TCH_ADDR_OFS], true);	
	}
	else if (mode == CY_RAWCOUNT)
	{
		retval = _cyttsp4_write_block_data(ts, (u16)0x03, sizeof(rawcount_cmd), &rawcount_cmd[0], ts->platform_data->addr[CY_TCH_ADDR_OFS], true);	
	}
	else if (mode == CY_SELF_RAW)
	{
		retval = _cyttsp4_write_block_data(ts, (u16)0x03, sizeof(self_raw_cmd), &self_raw_cmd[0], ts->platform_data->addr[CY_TCH_ADDR_OFS], true);	
	}
	else if (mode == CY_SELF_DIFF)
	{
		retval = _cyttsp4_write_block_data(ts, (u16)0x03, sizeof(self_diff_cmd), &self_diff_cmd[0], ts->platform_data->addr[CY_TCH_ADDR_OFS], true);	
	}



	if (retval < 0)
	{
		dev_err(ts->dev, "%s : Set command fail(ret=%d)\n", __func__, retval);
	}		

	/* Retrieves full panel scan data */
	cmd = 0x0C;
	retval = _cyttsp4_write_block_data(ts, (u16)0x02, sizeof(cmd), (uint8_t *) &cmd, ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
	if (retval < 0)
	{
		dev_err(ts->dev, "%s : Retrieve cmd  fail(ret=%d)\n", __func__, retval);
	}		

	retval = _cyttsp4_read_block_data(ts, (u16)0x08, sizeof(CY_TEST_BUFF), &CY_TEST_BUFF[0], ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
	if (retval < 0)
	{
		dev_err(ts->dev, "%s : Read Data(ret=%d)\n", __func__, retval);
	}

	// KJHW 0110 - swap data between column and row

	for (i=0; i<Y_SENSOR_NUM; i++)
	{
		for (j=0; j<X_SENSOR_NUM; j++)
		{
			tmp_buff[cnt++] = CY_TEST_BUFF[j*Y_SENSOR_NUM+i];
		}
	}

	for (i=0; i<(X_SENSOR_NUM*Y_SENSOR_NUM); i++)
	{
		CY_TEST_BUFF[i] = tmp_buff[i];
	}	

	dbg("\n");
	for(i=0;i<Y_SENSOR_NUM;i++) {
		for(j=0;j<X_SENSOR_NUM;j++) {
			dbg("%3d ",CY_TEST_BUFF[i*X_SENSOR_NUM+j]);
		}

		dbg("\n");
	}


	return retval;	
}

static int _cyttsp4_set_config_mode(struct cyttsp4 *ts)
{
	enum cyttsp4_driver_state tmp_state;
	enum cyttsp4_driver_state new_state;
	int retval;
	char *mode = "config";

	tmp_state = ts->driver_state;
	retval = _cyttsp4_set_device_mode(ts,
		CY_CONFIG_MODE, CY_MODE_CONFIG, mode);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail switch to %s mode\n", __func__, mode);
		new_state = CY_IDLE_STATE;
	} else
		new_state = tmp_state;
	_cyttsp4_change_state(ts, new_state);
	return retval;
}

static int _cyttsp4_set_operational_mode(struct cyttsp4 *ts)
{
    enum cyttsp4_driver_state new_state;
    int retval;
    char *mode = "operational";
    char ttt[4];
    dbg("%s: (+)\n",__func__);
    retval = _cyttsp4_set_device_mode(ts, CY_OPERATE_MODE, CY_MODE_OPERATIONAL, mode);
    if (retval < 0) {
        //dev_err(ts->dev,
        printk(KERN_ERR
                "%s: Fail switch to %s mode\n", __func__, mode);
        new_state = CY_IDLE_STATE;
    } else
        new_state = CY_ACTIVE_STATE;
    _cyttsp4_change_state(ts, new_state);

	retval = _cyttsp4_read_block_data(ts, (u16)0x00, sizeof(ttt), &ttt[0], ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
    printk("operational: %d\n",(CY_TEST_BUFF[0] & 0x70)>>4);

    dbg("%s: (-)\n",__func__);
    return retval;
}

static void _cyttsp4_pr_state(struct cyttsp4 *ts)
{
	dev_info(ts->dev, "%s: %s\n", __func__,
		ts->driver_state< CY_INVALID_STATE ? cyttsp4_driver_state_string[ts->driver_state] :"INVALID");
}

static void _cyttsp4_pr_buf(struct cyttsp4 *ts, u8 *dptr, int size, const char *data_name)
{
#ifdef CY_USE_INCLUDE_FBL
	int i = 0;
	int max = (CY_MAX_PRBUF_SIZE - 1) - sizeof(CY_PR_TRUNCATED);

	if(DebugON == true)
	{
		if (ts == NULL)
			dev_err(ts->dev,"%s: ts=%p\n", __func__, ts);
		else if (ts->pr_buf == NULL)
			dev_err(ts->dev,"%s: ts->pr_buf=%p\n", __func__, ts->pr_buf);
		else if (ts->bus_ops->tsdebug >= CY_DBG_LVL_2) {
			ts->pr_buf[0] = 0;
			for (i = 0; i < size && i < max; i++)
				sprintf(ts->pr_buf, "%s %02X", ts->pr_buf, dptr[i]);

			dev_err(ts->dev,"%s:  %s[0..%d]=%s%s\n", __func__, 
				data_name, size-1, ts->pr_buf, size <= max ? "" : CY_PR_TRUNCATED);
		}
	}
	
#endif /* --CY_USE_INCLUDE_FBL */

	return;
}

static int _cyttsp4_read_block_data(struct cyttsp4 *ts, u16 command, size_t length, void *buf, int i2c_addr, bool use_subaddr)
{
	int retval = 0;
	int tries = 0;

	if ((buf == NULL) || (length == 0)) {
		dev_err(ts->dev, "%s: pointer or length error buf=%p length=%d\n", __func__, buf, length);
		retval = -EINVAL;
	} else {
		for (tries = 0, retval = -1; tries < CY_NUM_RETRY && (retval < 0); tries++) {
			retval = ts->bus_ops->read(ts->bus_ops, command, length, buf, i2c_addr, use_subaddr);
			if (retval < 0) {
				msleep(CY_DELAY_DFLT);
				/*
				 * TODO: remove the extra sleep delay when
				 * the loader exit sequence is streamlined
				  */
				msleep(150);
			}
		}

		if (retval < 0) {
			dev_err(ts->dev, "%s: bus read block data fail (ret=%d)\n", __func__, retval);
		}
	}

	return retval;
}

static int _cyttsp4_write_block_data(struct cyttsp4 *ts, u16 command,
	size_t length, const void *buf, int i2c_addr, bool use_subaddr)
{
	int retval = 0;
	int tries = 0;

	if ((buf == NULL) || (length == 0)) {
		dev_err(ts->dev,
			"%s: pointer or length error"
			" buf=%p length=%d\n", __func__, buf, length);
		retval = -EINVAL;
	} else {
		for (tries = 0, retval = -1;
			tries < CY_NUM_RETRY && (retval < 0);
			tries++) {
			retval = ts->bus_ops->write(ts->bus_ops, command,
				length, buf, i2c_addr, use_subaddr);
			if (retval < 0)
				msleep(CY_DELAY_DFLT);
		}

		if (retval < 0) {
			dev_err(ts->dev,
			"%s: bus write block data fail (ret=%d)\n",
				__func__, retval);
		}
	}

	return retval;
}

#ifdef CY_USE_TMA400
static int _cyttsp4_wait_ready_int_no_init(struct cyttsp4 *ts, unsigned long timeout_ms)
{
	unsigned long uretval;
	int retval = 0;

	mutex_unlock(&ts->data_lock);	
	uretval = wait_for_completion_interruptible_timeout(&ts->ready_int_running, msecs_to_jiffies(timeout_ms));
	mutex_lock(&ts->data_lock);
	
	if (uretval == 0) {
		dev_err(ts->dev, "%s: timeout waiting for interrupt\n", __func__);
		retval = -ETIMEDOUT;
	}

	return retval;
}
#endif /* --CY_USE_TMA400 */

static int _cyttsp4_wait_int_no_init(struct cyttsp4 *ts, unsigned long timeout_ms)
{
	unsigned long uretval;
	int retval = 0;

	mutex_unlock(&ts->data_lock);
	uretval = wait_for_completion_interruptible_timeout(&ts->int_running, msecs_to_jiffies(timeout_ms));
	mutex_lock(&ts->data_lock);
	if (uretval == 0) {
		dev_err(ts->dev, "%s: timeout waiting for interrupt\n", __func__);
		retval = -ETIMEDOUT;
	}

	return retval;
}

static int _cyttsp4_wait_int(struct cyttsp4 *ts, unsigned long timeout_ms)
{
	int retval = 0;

	INIT_COMPLETION(ts->int_running);
	retval = _cyttsp4_wait_int_no_init(ts, timeout_ms);
	if (retval < 0) {
		dev_err(ts->dev, "%s: timeout waiting for interrupt\n", __func__);
	}

	return retval;
}

static int _cyttsp4_wait_si_int(struct cyttsp4 *ts, unsigned long timeout_ms)
{
	unsigned long uretval;
	int retval = 0;

	mutex_unlock(&ts->data_lock);
	uretval = wait_for_completion_interruptible_timeout(&ts->si_int_running, msecs_to_jiffies(timeout_ms));
	mutex_lock(&ts->data_lock);
	if (uretval == 0) {
		dev_err(ts->dev, "%s: timeout waiting for bootloader interrupt\n", __func__);
		retval = -ETIMEDOUT;
	}

	return retval;
}

static void _cyttsp4_queue_startup(struct cyttsp4 *ts, bool was_suspended)
{
	dbg_func_in();
	ts->was_suspended = was_suspended;
	queue_work(ts->cyttsp4_wq, &ts->cyttsp4_resume_startup_work);
	dev_info(ts->dev, "%s: startup queued\n", __func__);
	dbg_func_out();
}

#if defined(CY_AUTO_LOAD_TOUCH_PARAMS) || defined(CY_AUTO_LOAD_DDATA) || defined(CY_AUTO_LOAD_MDATA) || \
	defined(CY_USE_DEV_DEBUG_TOOLS) || defined(CY_USE_INCLUDE_FBL)
static u16 _cyttsp4_calc_partial_crc(struct cyttsp4 *ts, u8 *pdata, size_t ndata, u16 crc)
{
	int i = 0;
	int j = 0;

	for (i = 0; i < ndata; i++) {
		crc ^= ((u16)pdata[i] << 8);

		for (j = 8; j > 0; --j) {
			if (crc & 0x8000)
				crc = (crc << 1) ^ 0x1021;
			else
				crc = crc << 1;
		}
	}

	return crc;
}

static void _cyttsp4_calc_crc(struct cyttsp4 *ts,
	u8 *pdata, size_t ndata, u8 *crc_h, u8 *crc_l)
{
	u16 crc = 0;

	if (pdata == NULL)
		dev_err(ts->dev,
			"%s: Null data ptr\n", __func__);
	else if (ndata == 0)
		dev_err(ts->dev,
			"%s: Num data is 0\n", __func__);
	else {
		/* Calculate CRC */
		crc = 0xFFFF;
		crc = _cyttsp4_calc_partial_crc(ts, pdata, ndata, crc);
		*crc_h = crc / 256;
		*crc_l = crc % 256;
	}
}
#endif /* --CY_AUTO_LOAD_TOUCH_PARAMS --CY_AUTO_LOAD_DDATA
          --CY_AUTO_LOAD_MDATA --CY_USE_DEV_DEBUG_TOOLS --CY_USE_INCLUDE_FBL */

static bool _cyttsp4_chk_cmd_rdy(struct cyttsp4 *ts, u8 cmd)
{
	bool cond = !!(cmd & CY_CMD_RDY_BIT);
	dev_vdbg(ts->dev,
		"%s: cmd=%02X cond=%d\n", __func__, cmd, (int)cond);

	return cond;
}

static bool _cyttsp4_chk_mode_change(struct cyttsp4 *ts, u8 cmd)
{
	bool cond = !(cmd & CY_MODE_CHANGE);
	dev_vdbg(ts->dev,
		"%s: cmd=%02X cond=%d\n", __func__, cmd, (int)cond);

	return cond;
}

static void _cyttsp4_change_state(struct cyttsp4 *ts, enum cyttsp4_driver_state new_state)
{
	ts->driver_state= new_state; 
	_cyttsp4_pr_state(ts);	
}

static int _cyttsp4_put_cmd_wait(struct cyttsp4 *ts, u16 ofs,
	size_t cmd_len, const void *cmd_buf, unsigned long timeout_ms,
	bool (*cond)(struct cyttsp4 *, u8), u8 *retcmd,
	int i2c_addr, bool use_subaddr)
{
	enum cyttsp4_driver_state tmp_state;
	unsigned long uretval = 0;
	u8 cmd = 0;
	int tries = 0;
	int retval = 0;

	/* unlock here to allow any pending irq to complete */
	tmp_state = ts->driver_state;
	_cyttsp4_change_state(ts, CY_TRANSFER_STATE);
	mutex_unlock(&ts->data_lock);
	mutex_lock(&ts->data_lock);
	_cyttsp4_change_state(ts, CY_CMD_STATE);
	INIT_COMPLETION(ts->int_running);
	mutex_unlock(&ts->data_lock);
	retval = _cyttsp4_write_block_data(ts, ofs, cmd_len,
		cmd_buf, i2c_addr, use_subaddr);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail writing cmd buf r=%d\n",
			__func__, retval);
		mutex_lock(&ts->data_lock);
		goto _cyttsp4_put_cmd_wait_exit;
	}
_cyttsp4_put_cmd_wait_retry:
	uretval = wait_for_completion_interruptible_timeout(
		&ts->int_running, msecs_to_jiffies(timeout_ms));
	mutex_lock(&ts->data_lock);

	retval = _cyttsp4_read_block_data(ts, ofs,
		sizeof(cmd), &cmd, i2c_addr, use_subaddr);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: fail read cmd status  r=%d\n",
			__func__, retval);
	}
	if ((cond != NULL) && !cond(ts, cmd)) {
		if (uretval == 0) {
			dev_err(ts->dev,
			"%s: timeout waiting for cmd ready\n",
				__func__);
			retval = -ETIMEDOUT;
		} else {
			if (tries++ < 2) {
				INIT_COMPLETION(ts->int_running);
				mutex_unlock(&ts->data_lock);
				goto _cyttsp4_put_cmd_wait_retry;
			} else {
				dev_err(ts->dev,
			"%s: cmd not ready error"
					" cmd_stat=0x%02X\n",
					__func__, cmd);
				retval = -EIO;
			}
		}
	} else {
		/* got command ready */
		if (retcmd != NULL)
			*retcmd = cmd;
		retval = 0;
		dev_vdbg(ts->dev,
			"%s: got command ready; cmd=%02X retcmd=%p tries=%d\n",
			__func__, cmd, retcmd, tries);
	}

_cyttsp4_put_cmd_wait_exit:
	_cyttsp4_change_state(ts, tmp_state);
	return retval;
}

static void _cyttsp4_wait_for_complete_bit(struct cyttsp4 *ts) 
{
	int retval;
	int loop_count = 0;
	while(loop_count<50) {
		retval = _cyttsp4_read_block_data(ts, (u16)0x02, sizeof(CY_TEST_BUFF), &CY_TEST_BUFF[0], ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
		if (retval < 0) {
			dev_err(ts->dev, "%s: Read Data(ret=%d)\n", __func__, retval);
		}
		dev_info(ts->dev, "%s: CY_TEST_BUFF[0]=%02x loop_count=%d\n", __func__, CY_TEST_BUFF[0],loop_count);
		if(CY_TEST_BUFF[0] & 0x40)
			break;
		msleep(10);
		loop_count++;
	}
}

static int cyttsp4_manual_calibration(struct cyttsp4 *ts) 
{
    int retval;
    uint8_t cal_cmd[2];
    uint8_t cal_cmd_rtn[2];
   

	dbg_func_in();

	/* Enter Configuration And Test Mode */
    retval = _cyttsp4_set_config_mode(ts);
    if (retval < 0) {
        dev_err(ts->dev, "%s : Failed to switch to config mode\n", __func__);
    }

    /* 
     * write iDAC Calibrate
     * 0 = Mutual Capacitance Fine
     * 1 = Mutual Capacitance Buttons
     * 2 = Self Capacitance
     * 3 = Balanced
     */
    cal_cmd[0] = 0x09;   /* iDAC calibrate command */
    cal_cmd[1] = 0x00;   /* Mutual Capacitance Fine */
    retval = _cyttsp4_write_block_data(ts, (u16)0x02, sizeof(cal_cmd), cal_cmd, ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
    if(retval < 0) {
        dev_err(ts->dev, "%s: fail to write calibration.\n", __func__);
    }
    _cyttsp4_wait_for_complete_bit(ts);
    retval = _cyttsp4_read_block_data(ts, (u16)0x03, sizeof(cal_cmd_rtn), &cal_cmd_rtn[0], ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
    if(retval < 0) {
        dev_err(ts->dev, "%s: fail to write calibration.\n", __func__);
    }
    dev_info(ts->dev,"cal_cmd_rtn[0]=%d\n",cal_cmd_rtn[0]);

    cal_cmd[1] = 0x02;   /* Self Capacitance */
    retval = _cyttsp4_write_block_data(ts, (u16)0x02, sizeof(cal_cmd), cal_cmd, ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
    if(retval < 0) {
        dev_err(ts->dev, "%s: fail to write calibration.\n", __func__);
    }
    _cyttsp4_wait_for_complete_bit(ts);
    retval = _cyttsp4_read_block_data(ts, (u16)0x03, sizeof(cal_cmd_rtn), &cal_cmd_rtn[0], ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
    if(retval < 0) {
        dev_err(ts->dev, "%s: fail to write calibration.\n", __func__);
    }
    dev_info(ts->dev,"cal_cmd_rtn[0]=%d\n",cal_cmd_rtn[0]);

    /* Enter Operational Mode */
    retval = _cyttsp4_set_operational_mode(ts);
    if (retval < 0) {
        dev_err(ts->dev, "%s : Failed to switch to operation mode\n", __func__);
    }

    if (mutex_is_locked(&ts->data_lock) != 0) {
        dev_info(ts->dev, "%s(%d: Ops..mutex is locked. we will be unlock mutex state.\n", __func__, __LINE__);
        mutex_unlock(&ts->data_lock);
    }

	dbg_func_out();
    return retval;
}

static int _cyttsp4_handshake(struct cyttsp4 *ts, u8 hst_mode)
{
	int retval = 0;
	u8 cmd = 0;

	cmd = (hst_mode & CY_HANDSHAKE_BIT) ? hst_mode & ~CY_HANDSHAKE_BIT : hst_mode | CY_HANDSHAKE_BIT;

	retval = _cyttsp4_write_block_data(ts, CY_REG_BASE, sizeof(cmd), (u8 *)&cmd, ts->platform_data->addr[CY_TCH_ADDR_OFS], true);

	if (retval < 0) {
		dev_err(ts->dev, "%s: bus write fail on handshake (ret=%d)\n", __func__, retval);
	}

	return retval;
}

#ifdef CY_USE_TMA400

#if defined(CY_AUTO_LOAD_TOUCH_PARAMS) || defined(CY_USE_DEV_DEBUG_TOOLS)
static void _cyttsp_read_table_crc(struct cyttsp4 *ts, const u8 *ptable,	u8 *crc_h, u8 *crc_l)
{
	size_t crc_loc = (ptable[3] * 256) + ptable[2];

	*crc_h = ptable[crc_loc];
	*crc_l = ptable[crc_loc + 1];
}
#endif

static int _cyttsp4_cmd_handshake(struct cyttsp4 *ts)
{
	u8 host_mode = 0;
	int retval = 0;

	retval = _cyttsp4_read_block_data(ts, CY_REG_BASE,
		sizeof(host_mode), &host_mode,
		ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail read host mode r=%d\n",
			__func__, retval);
	} else {
		retval = _cyttsp4_handshake(ts, host_mode);
		if (retval < 0) {
			dev_err(ts->dev,
				"%s: Fail handshake r=%d\n",
				__func__, retval);
		}
	}

	return retval;
}

/* Get EBID Row Size is a Config mode command */
static int _cyttsp4_get_ebid_row_size(struct cyttsp4 *ts)
{
	int retval = 0;
	u8 cmd = 0;
	u8 cmd_dat[CY_NUM_DAT + 1];	/* +1 for cmd byte */

	memset(cmd_dat, 0, sizeof(cmd_dat));
	cmd_dat[0] = CY_GET_EBID_ROW_SIZE;	/* get EBID row size command */

	retval = _cyttsp4_put_cmd_wait(ts, ts->si_ofs.cmd_ofs,
		sizeof(cmd_dat), cmd_dat, CY_HALF_SEC_TMO_MS,
		_cyttsp4_chk_cmd_rdy, &cmd,
		ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail Get EBID row size command r=%d\n",
			__func__, retval);
	} else {
		retval = _cyttsp4_read_block_data(ts, ts->si_ofs.cmd_ofs,
			sizeof(cmd_dat), cmd_dat,
			ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
		if (retval < 0) {
			dev_err(ts->dev,
				"%s: Fail get EBID row size r=%d\n",
				__func__, retval);
			ts->ebid_row_size = CY_EBID_ROW_SIZE_DFLT;
			dev_err(ts->dev,
				"%s: Use default EBID row size=%d\n",
				__func__, ts->ebid_row_size);
		} else {
			ts->ebid_row_size = (cmd_dat[1] * 256) + cmd_dat[2];
			retval = _cyttsp4_cmd_handshake(ts);
			if (retval < 0) {
				dev_err(ts->dev,
					"%s: Command handshake error r=%d\n",
					__func__, retval);
				/* continue anyway; rely on handshake tmo */
				retval = 0;
			}
		}
	}

	return retval;
}

#if defined(CY_AUTO_LOAD_TOUCH_PARAMS) || defined(CY_AUTO_LOAD_DDATA) || defined(CY_AUTO_LOAD_MDATA) || \
	defined(CY_USE_DEV_DEBUG_TOOLS) || defined(CY_USE_INCLUDE_FBL)
/* Get EBID Row Data is a Config mode command */
int _cyttsp4_get_ebid_data_tma400(struct cyttsp4 *ts, enum cyttsp4_ic_ebid ebid, size_t row_id, u8 *pdata)
{
	int rc = 0;
	int retval = 0;
	u8 crc_h = 0;
	u8 crc_l = 0;
	u8 cmd = 0;
	u8 status = 0;
	u8 cmd_dat[CY_NUM_DAT + 1];	/* +1 for cmd byte */

	memset(cmd_dat, 0, sizeof(cmd_dat));
	cmd_dat[0] = CY_READ_EBID_DATA;	/* get EBID data command */
	cmd_dat[1] = row_id / 256;
	cmd_dat[2] = row_id % 256;
	cmd_dat[3] = ts->ebid_row_size / 256;
	cmd_dat[4] = ts->ebid_row_size % 256;
	cmd_dat[5] = ebid;

	if (pdata == NULL) {
		dev_err(ts->dev,
			"%s: Get EBID=%d row=%d Data buffer err ptr=%p\n",
			__func__, ebid, row_id, pdata);
		goto _cyttsp4_get_ebid_data_tma400_exit;
	}

	retval = _cyttsp4_put_cmd_wait(ts, ts->si_ofs.cmd_ofs,
		sizeof(cmd_dat), cmd_dat, CY_HALF_SEC_TMO_MS,
		_cyttsp4_chk_cmd_rdy, &cmd,
		ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail Get EBID=%d row=%d Data cmd r=%d\n",
			__func__, ebid, row_id, retval);
		goto _cyttsp4_get_ebid_data_tma400_exit;
	}

	retval = _cyttsp4_read_block_data(ts, ts->si_ofs.cmd_ofs + 1,
		sizeof(status), &status,
		ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail get EBID=%d row=%d status r=%d\n",
			__func__, ebid, row_id, retval);
		goto _cyttsp4_get_ebid_data_tma400_exit;
	}

	if (status != 0x00) {
		dev_err(ts->dev,
			"%s: Get EBID=%d row=%d status=%d error\n",
			__func__, ebid, row_id, status);
		retval = -EIO;
		goto _cyttsp4_get_ebid_data_tma400_exit;
	}

	retval = _cyttsp4_read_block_data(ts, ts->si_ofs.cmd_ofs + 1 + 5,
		ts->ebid_row_size + 2, pdata,
		ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: fail EBID=%d row=%d data r=%d\n",
			__func__, ebid, row_id, retval);
		retval = -EIO;
	} else {
		_cyttsp4_calc_crc(ts, pdata, ts->ebid_row_size, &crc_h, &crc_l);
		if (pdata[ts->ebid_row_size] != crc_h ||
			pdata[ts->ebid_row_size + 1] != crc_l) {
				dev_err(ts->dev,
					"%s: EBID=%d row_id=%d row_data_crc=%02X%02X"
					" not equal to calc_crc=%02X%02X\n",
					__func__, ebid, row_id,
					pdata[ts->ebid_row_size],
					pdata[ts->ebid_row_size + 1],
					crc_h, crc_l);
				/* continue anyway; allow handshake */
				rc = -EIO;
			}
			retval = _cyttsp4_cmd_handshake(ts);
			if (retval < 0) {
				dev_err(ts->dev,
					"%s: Command handshake error r=%d\n",
					__func__, retval);
				/* continue anyway; rely on handshake tmo */
				retval = 0;
			}
			retval = rc;
	}

_cyttsp4_get_ebid_data_tma400_exit:
	return retval;
}

static const u8 cyttsp4_security_key[] = {
	0xA5, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD, 0x5A
};

/* Put EBID Row Data is a Config mode command */
static int _cyttsp4_put_ebid_data_tma400(struct cyttsp4 *ts,
	enum cyttsp4_ic_ebid ebid, size_t row_id, u8 *out_data)
{
	u8 calc_crc[2];
	u8 *pdata = NULL;
	u8 ret_cmd = 0;
	size_t psize = 0;
	u8 status = 0;
	int retval = 0;

	memset(calc_crc, 0, sizeof(calc_crc));
	psize = 1 + 5 + ts->ebid_row_size + sizeof(cyttsp4_security_key) + 2;
	pdata = kzalloc(psize, GFP_KERNEL);
	if (pdata == NULL || out_data == NULL) {
		dev_err(ts->dev,
			"%s: Buffer ptr err EBID=%d row=%d"
			" alloc_ptr=%p out_data=%p\n",
			__func__, ebid, row_id, pdata, out_data);
		retval = -EINVAL;
	} else {
		pdata[0] = CY_WRITE_EBID_DATA;	/* put ebid data command */
		pdata[1] = row_id / 256;
		pdata[2] = row_id % 256;
		pdata[3] = ts->ebid_row_size / 256;
		pdata[4] = ts->ebid_row_size % 256;
		pdata[5] = ebid;
		memcpy(&pdata[1 + 5], out_data, ts->ebid_row_size);
		memcpy(&pdata[1 + 5 + ts->ebid_row_size],
			cyttsp4_security_key, sizeof(cyttsp4_security_key));
		_cyttsp4_calc_crc(ts, &pdata[1 + 5], ts->ebid_row_size,
			&calc_crc[0], &calc_crc[1]);
		memcpy(&pdata[1 + 5 + ts->ebid_row_size +
			sizeof(cyttsp4_security_key)],
			calc_crc, sizeof(calc_crc));

		retval = _cyttsp4_put_cmd_wait(ts, ts->si_ofs.cmd_ofs,
			psize, pdata, CY_HALF_SEC_TMO_MS,
			_cyttsp4_chk_cmd_rdy, &ret_cmd,
			ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
		if (retval < 0) {
			dev_err(ts->dev,
				"%s: Fail Put EBID=%d row=%d Data cmd r=%d\n",
				__func__, ebid, row_id, retval);
			goto _cyttsp4_put_ebid_data_tma400_exit;
		}

		retval = _cyttsp4_read_block_data(ts, ts->si_ofs.cmd_ofs + 1,
			sizeof(status), &status,
			ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
		if (retval < 0) {
			dev_err(ts->dev,
				"%s: Fail put EBID=%d row=%d"
				" read status r=%d\n",
				__func__, ebid, row_id, retval);
			goto _cyttsp4_put_ebid_data_tma400_exit;
		}

		retval = _cyttsp4_cmd_handshake(ts);
		if (retval < 0) {
			dev_err(ts->dev,
				"%s: Fail handshake on Put EBID=%d row=%d"
				" r=%d\n", __func__, ebid, row_id, retval);
			/* continue; rely on handshake tmo */
			retval = 0;
		}

		if (status != 0x00) {
			dev_err(ts->dev,
				"%s: Put EBID=%d row=%d status=%d error\n",
				__func__, ebid, row_id, status);
			retval = -EIO;
		} else
			retval = 0;
	}
_cyttsp4_put_ebid_data_tma400_exit:
	if (pdata != NULL)
		kfree(pdata);
	return retval;
}
#endif /* --CY_AUTO_LOAD_TOUCH_PARAMS --CY_AUTO_LOAD_DDATA
          --CY_AUTO_LOAD_MDATA --CY_USE_DEV_DEBUG_TOOLS --CY_USE_INCLUDE_FBL */

#if defined(CY_AUTO_LOAD_TOUCH_PARAMS) || defined(CY_USE_DEV_DEBUG_TOOLS)
/* Put All Touch Params is a Config mode command */
static int _cyttsp4_put_all_params_tma400(struct cyttsp4 *ts)
{
	enum cyttsp4_ic_ebid ebid = CY_TCH_PARM_EBID;
	size_t row_id = 0;
	size_t num_rows = 0;
	size_t table_size = 0;
	size_t residue = 0;
	u8 *pdata = NULL;
	u8 *ptable = NULL;
	int retval = 0;

	pdata = kzalloc(ts->ebid_row_size, GFP_KERNEL);
	if (pdata == NULL) {
		dev_err(ts->dev, "%s: Alloc error ebid=%d\n", __func__, ebid);
		retval = -ENOMEM;
	} else if (ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL] == NULL)
		dev_err(ts->dev, "%s: NULL param values table\n", __func__);
	else if (ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL]->data == NULL)
		dev_err(ts->dev, "%s: NULL param values table data\n", __func__);
	else if (ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL]->size == 0)
		dev_err(ts->dev, "%s: param values table size is 0\n", __func__);
	else 
	{		
		ptable = (u8 *)ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL]->data;
		table_size = ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL]->size;
		num_rows = table_size / ts->ebid_row_size;
		dev_vdbg(ts->dev, "%s: num_rows=%d row_size=%d table_size=%d\n", __func__, num_rows, ts->ebid_row_size, table_size);

		for (row_id = 0; row_id < num_rows;) {
			memcpy(pdata, ptable, ts->ebid_row_size);
			dev_vdbg(ts->dev,"%s: row=%d pdata=%p\n",__func__, row_id, pdata);
			_cyttsp4_pr_buf(ts, pdata, ts->ebid_row_size, "ebid_data");
			retval = _cyttsp4_put_ebid_data_tma400(ts,ebid, row_id, pdata);
			if (retval < 0) {
				dev_err(ts->dev, "%s: Fail put row=%d r=%d\n", __func__, row_id, retval);
				break;
			} else {
				ptable += ts->ebid_row_size;
				row_id++;
			}
		}
		
		if (!(retval < 0)) {
			residue = table_size % ts->ebid_row_size;
			if (residue) {
				memset(pdata, 0, ts->ebid_row_size);
				memcpy(pdata, ptable, residue);
				dev_vdbg(ts->dev, "%s: ebid=%d row=%d data:\n", __func__, ebid, row_id);
				_cyttsp4_pr_buf(ts, pdata, ts->ebid_row_size, "ebid_data");
				retval = _cyttsp4_put_ebid_data_tma400(ts, ebid, row_id, pdata);
				if (retval < 0) {
					dev_err(ts->dev, "%s: Fail put row=%d r=%d\n", __func__, row_id, retval);
				}
			}
		}
	}

	if (pdata != NULL)
		kfree(pdata);

	return retval;
}
#endif /* --CY_AUTO_LOAD_TOUCH_PARAMS */

/* Check MDDATA is a Config mode command */
static int _cyttsp4_check_mddata_tma400(struct cyttsp4 *ts, bool *updated)
{
	bool ddata_updated = false;
	bool mdata_updated = false;
#if defined(CY_AUTO_LOAD_DDATA) || defined(CY_AUTO_LOAD_MDATA)
	enum cyttsp4_ic_ebid ebid = CY_DDATA_EBID;
	size_t num_data = 0;
	size_t crc_ofs = 0;
	u8 crc_h = 0;
	u8 crc_l = 0;
#endif
	u8 *pdata = NULL;
	u8 *pmddata = NULL;
	int retval = 0;

	if (ts->ebid_row_size == 0) {
		dev_err(ts->dev,
			"%s: fail allocate set MDDATA buffer\n", __func__);
		retval = -EINVAL;
		goto _cyttsp4_check_mddata_tma400_exit;
	}
	pdata = kzalloc(ts->ebid_row_size, GFP_KERNEL);
	if (pdata == NULL) {
		dev_err(ts->dev,
			"%s: fail allocate set MDDATA buffer\n", __func__);
		retval = -ENOMEM;
		goto _cyttsp4_check_mddata_tma400_exit;
	}
	pmddata = kzalloc(ts->ebid_row_size, GFP_KERNEL);
	if (pmddata == NULL) {
		dev_err(ts->dev,
			"%s: fail allocate set MDDATA buffer\n", __func__);
		retval = -ENOMEM;
		goto _cyttsp4_check_mddata_tma400_exit;
	}

#ifdef CY_AUTO_LOAD_DDATA
	/* check for platform_data DDATA */
	ebid = CY_DDATA_EBID;
	if (ts->platform_data->sett[CY_IC_GRPNUM_DDATA_REC] == NULL) {
		dev_vdbg(ts->dev,
			"%s: No platform DDATA table\n", __func__);
		goto _cyttsp4_check_mdata_block;
	}
	if (ts->platform_data->sett[CY_IC_GRPNUM_DDATA_REC]->data == NULL) {
		dev_vdbg(ts->dev,
			"%s: No platform DDATA table data\n", __func__);
		goto _cyttsp4_check_mdata_block;
	}
	if (ts->platform_data->sett[CY_IC_GRPNUM_DDATA_REC]->size == 0) {
		dev_vdbg(ts->dev,
			"%s: Platform DDATA table has size=0\n", __func__);
		goto _cyttsp4_check_mdata_block;
	}

	dev_vdbg(ts->dev,
		"%s: call get ebid data for DDATA\n", __func__);
	retval = _cyttsp4_get_ebid_data_tma400(ts, ebid, 0, pdata);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail get DDATA r=%d\n", __func__, retval);
		goto _cyttsp4_check_mdata_block;
	}

	dev_vdbg(ts->dev,
		"%s: copy pdata -> pmddata\n", __func__);
	memcpy(pmddata, pdata, 4);
	num_data = ts->platform_data->sett
		[CY_IC_GRPNUM_DDATA_REC]->size < CY_NUM_DDATA ?
		ts->platform_data->sett
		[CY_IC_GRPNUM_DDATA_REC]->size : CY_NUM_DDATA;
	dev_vdbg(ts->dev,
		"%s: copy %d bytes from platform data to ddata array\n",
		__func__, num_data);
	memcpy(&pmddata[4], ts->platform_data->sett
		[CY_IC_GRPNUM_DDATA_REC]->data, num_data);
	if (num_data < CY_NUM_DDATA)
		memset(&pmddata[4 + num_data], 0, CY_NUM_DDATA - num_data);
	crc_ofs = (pmddata[3] * 256) + pmddata[2];
	if (crc_ofs == 0)
		crc_ofs = 126;
	dev_vdbg(ts->dev,
		"%s: ddata crc_ofs=%d num_data=%d\n",
		__func__, crc_ofs, num_data);

	_cyttsp4_calc_crc(ts, pmddata, crc_ofs, &crc_h, &crc_l);
	pmddata[crc_ofs] = crc_l;
	pmddata[crc_ofs+1] = crc_h;
	_cyttsp4_pr_buf(ts, pdata, ts->ebid_row_size, "pdata");
	_cyttsp4_pr_buf(ts, pmddata, ts->ebid_row_size, "pmddata");
	if (pmddata[crc_ofs] != pdata[crc_ofs] ||
		pmddata[crc_ofs+1] != pdata[crc_ofs+1]) {
			retval = _cyttsp4_put_ebid_data_tma400(ts, ebid, 0, pmddata);
			if (retval < 0)
				dev_err(ts->dev,
				"%s: Fail put DDATA r=%d\n", __func__, retval);
			else
				ddata_updated = true;
		}

#else
	ddata_updated = false;
#endif /* --CY_AUTO_LOAD_DDATA */

#ifdef CY_AUTO_LOAD_DDATA

_cyttsp4_check_mdata_block:
#endif


#ifdef CY_AUTO_LOAD_MDATA
	/* check for platform_data MDATA */
	memset(pdata, 0, ts->ebid_row_size);
	memset(pmddata, 0, ts->ebid_row_size);
	ebid = CY_MDATA_EBID;
	if (ts->platform_data->sett[CY_IC_GRPNUM_MDATA_REC] == NULL) {
		dev_vdbg(ts->dev,
			"%s: No platform MDATA table\n", __func__);
		goto _cyttsp4_check_mddata_tma400_exit;
	}
	if (ts->platform_data->sett[CY_IC_GRPNUM_MDATA_REC]->data == NULL) {
		dev_vdbg(ts->dev,
			"%s: No platform MDATA table data\n", __func__);
		goto _cyttsp4_check_mddata_tma400_exit;
	}
	if (ts->platform_data->sett[CY_IC_GRPNUM_MDATA_REC]->size == 0) {
		dev_vdbg(ts->dev,
			"%s: Platform MDATA table has size=0\n", __func__);
		goto _cyttsp4_check_mddata_tma400_exit;
	}

	retval = _cyttsp4_get_ebid_data_tma400(ts, ebid, 0, pdata);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail get MDATA r=%d\n", __func__, retval);
		goto _cyttsp4_check_mddata_tma400_exit;
	}

	memcpy(pmddata, pdata, 4);
	num_data = ts->platform_data->sett
		[CY_IC_GRPNUM_MDATA_REC]->size < CY_NUM_MDATA ?
		ts->platform_data->sett
		[CY_IC_GRPNUM_MDATA_REC]->size : CY_NUM_MDATA;
	dev_vdbg(ts->dev,
		"%s: copy %d bytes from platform data to mdata array\n",
		__func__, num_data);
	memcpy(&pmddata[4], ts->platform_data->sett
		[CY_IC_GRPNUM_MDATA_REC]->data, num_data);
	if (num_data < CY_NUM_MDATA)
		memset(&pmddata[4 + num_data], 0, CY_NUM_MDATA - num_data);
	crc_ofs = (pmddata[3] * 256) + pmddata[2];
	if (crc_ofs == 0)
		crc_ofs = 124;
	dev_vdbg(ts->dev,
		"%s: mdata crc_ofs=%d num_data=%d\n",
		__func__, crc_ofs, num_data);
	_cyttsp4_calc_crc(ts, pmddata, crc_ofs, &crc_h, &crc_l);
	pmddata[crc_ofs] = crc_l;
	pmddata[crc_ofs+1] = crc_h;
	_cyttsp4_pr_buf(ts, pdata, ts->ebid_row_size, "pdata");
	_cyttsp4_pr_buf(ts, pmddata, ts->ebid_row_size, "pmddata");
	if (pmddata[crc_ofs] != pdata[crc_ofs] ||
		pmddata[crc_ofs+1] != pdata[crc_ofs+1]) {
			retval = _cyttsp4_put_ebid_data_tma400(ts, ebid, 0, pmddata);
			if (retval < 0)
				dev_err(ts->dev,
				"%s: Fail put MDATA r=%d\n", __func__, retval);
			else
				mdata_updated = true;
		}
#else
	mdata_updated = false;
#endif /* --CY_AUTO_LOAD_MDATA */

_cyttsp4_check_mddata_tma400_exit:

	if (pdata != NULL)
		kfree(pdata);
	if (pmddata != NULL)
		kfree(pmddata);
	if (updated != NULL)
		*updated = ddata_updated || mdata_updated;
	return retval;
}
#endif /* --CY_USE_TMA400 */

/*
 * change device mode - For example, change from
 * system information mode to operating mode
 */
static int _cyttsp4_set_device_mode(struct cyttsp4 *ts,
									u8 new_mode, u8 new_cur_mode, char *mode)
{
	u8 cmd = 0;
	int retval = 0;

	cmd = new_mode + CY_MODE_CHANGE;

	retval = _cyttsp4_put_cmd_wait(ts, CY_REG_BASE,
		sizeof(cmd), &cmd, CY_TEN_SEC_TMO_MS,
		_cyttsp4_chk_mode_change, &cmd,
		ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail Set mode command new_mode=%02X r=%d\n",
			__func__, new_mode, retval);
		goto _cyttsp4_set_device_mode_exit;
	}

	if (cmd != new_mode) {
		dev_err(ts->dev,
			"%s: failed to switch to %s mode\n", __func__, mode);
		retval = -EIO;
	} else {
		ts->current_mode = new_cur_mode;
		retval = _cyttsp4_handshake(ts, cmd);
		if (retval < 0) {
			dev_err(ts->dev,
				"%s: Fail handshake r=%d\n", __func__, retval);
			/* continue; rely on handshake tmo */
			retval = 0;
		}
	}

	dev_dbg(ts->dev,
		"%s: check op ready ret=%d host_mode=%02X\n",
		__func__, retval, cmd);

_cyttsp4_set_device_mode_exit:
	return retval;
}

static int _cyttsp4_set_mode(struct cyttsp4 *ts, u8 new_mode)
{
	enum cyttsp4_driver_state new_state = CY_TRANSFER_STATE;
	u8 new_cur_mode = CY_MODE_OPERATIONAL;
	char *mode = NULL;
#ifdef CY_USE_TMA400
	unsigned long uretval = 0;
#endif /* --CY_USE_TMA400 */
	int retval = 0;

	switch (new_mode) {
	case CY_OPERATE_MODE:
		new_cur_mode = CY_MODE_OPERATIONAL;
		mode = "operational";
		INIT_COMPLETION(ts->ready_int_running);
		_cyttsp4_change_state(ts, CY_READY_STATE);
		new_state = CY_ACTIVE_STATE;
		break;
	case CY_SYSINFO_MODE:
		new_cur_mode = CY_MODE_SYSINFO;
		mode = "sysinfo";
		new_state = CY_SYSINFO_STATE;
		break;
	case CY_CONFIG_MODE:
		new_cur_mode = CY_MODE_OPERATIONAL;
		mode = "config";
		new_state = ts->driver_state;

		break;
	default:
		dev_err(ts->dev,
			"%s: invalid mode change request m=0x%02X\n",
			__func__, new_mode);
		retval = -EINVAL;
		goto _cyttsp_set_mode_exit;
	}

	retval = _cyttsp4_set_device_mode(ts,
		new_mode, new_cur_mode, mode);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail switch to %s mode\n", __func__, mode);
		_cyttsp4_change_state(ts, CY_IDLE_STATE);
	} else {
#ifdef CY_USE_TMA400
		if ((new_mode == CY_OPERATE_MODE) && ts->starting_up){
			uretval = _cyttsp4_wait_ready_int_no_init(ts, CY_HALF_SEC_TMO_MS * 5);
		}
#endif /* --CY_USE_TMA400 */
		_cyttsp4_change_state(ts, new_state);
	}

_cyttsp_set_mode_exit:
	return retval;
}

static int _cyttsp4_bits_2_bytes(struct cyttsp4 *ts, int nbits, int *max)
{
	int nbytes;

	*max = 1 << nbits;

	for (nbytes = 0; nbits > 0;) {
		dev_vdbg(ts->dev,
			"%s: nbytes=%d nbits=%d\n", __func__, nbytes, nbits);
		nbytes++;
		if (nbits > 8)
			nbits -= 8;
		else
			nbits = 0;
		dev_vdbg(ts->dev,
			"%s: nbytes=%d nbits=%d\n", __func__, nbytes, nbits);
	}

	return nbytes;
}

static int _cyttsp4_get_sysinfo_regs(struct cyttsp4 *ts)
{
	int btn = 0;
	int num_defined_keys = 0;
	u16 *key_table = NULL;
	enum cyttsp4_tch_abs abs = 0;
#ifdef CY_USE_TMA400_SP2
#ifdef CY_USE_TMA400
	int i = 0;
#endif /* --CY_USE_TMA400 */
#endif /* --CY_USE_TMA400_SP2 */
	int retval = 0;

	/* pre-clear si_ofs structure */
	memset(&ts->si_ofs, 0, sizeof(struct cyttsp4_sysinfo_ofs));

	/* get the sysinfo data offsets */
	retval = _cyttsp4_read_block_data(ts, CY_REG_BASE,
		sizeof(ts->sysinfo_data), &(ts->sysinfo_data),
		ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: fail read sysinfo data offsets r=%d\n",
			__func__, retval);
		goto _cyttsp4_get_sysinfo_regs_exit_no_handshake;
	} else {
		/* Print sysinfo data offsets */
		_cyttsp4_pr_buf(ts, (u8 *)&ts->sysinfo_data,
			sizeof(ts->sysinfo_data), "sysinfo_data_offsets");

		/* convert sysinfo data offset bytes into integers */
		ts->si_ofs.map_sz = (ts->sysinfo_data.map_szh * 256) +
			ts->sysinfo_data.map_szl;
		ts->si_ofs.cydata_ofs = (ts->sysinfo_data.cydata_ofsh * 256) +
			ts->sysinfo_data.cydata_ofsl;
		ts->si_ofs.test_ofs = (ts->sysinfo_data.test_ofsh * 256) +
			ts->sysinfo_data.test_ofsl;
		ts->si_ofs.pcfg_ofs = (ts->sysinfo_data.pcfg_ofsh * 256) +
			ts->sysinfo_data.pcfg_ofsl;
		ts->si_ofs.opcfg_ofs = (ts->sysinfo_data.opcfg_ofsh * 256) +
			ts->sysinfo_data.opcfg_ofsl;
		ts->si_ofs.ddata_ofs = (ts->sysinfo_data.ddata_ofsh * 256) +
			ts->sysinfo_data.ddata_ofsl;
		ts->si_ofs.mdata_ofs = (ts->sysinfo_data.mdata_ofsh * 256) +
			ts->sysinfo_data.mdata_ofsl;
	}

	/* get the sysinfo cydata */
	ts->si_ofs.cydata_size = ts->si_ofs.test_ofs - ts->si_ofs.cydata_ofs;
	ts->sysinfo_ptr.cydata = kzalloc(ts->si_ofs.cydata_size, GFP_KERNEL);
	if (ts->sysinfo_ptr.cydata == NULL) {
		retval = -ENOMEM;
		dev_err(ts->dev,
			"%s: fail alloc cydata memory r=%d\n",
			__func__, retval);
		goto _cyttsp4_get_sysinfo_regs_exit;
	} else {
		memset(ts->sysinfo_ptr.cydata, 0, ts->si_ofs.cydata_size);
		retval = _cyttsp4_read_block_data(ts, ts->si_ofs.cydata_ofs,
			ts->si_ofs.cydata_size, ts->sysinfo_ptr.cydata,
			ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
		if (retval < 0) {
			dev_err(ts->dev,
				"%s: fail read cydata r=%d\n",
				__func__, retval);
			goto _cyttsp4_get_sysinfo_regs_exit;
		}
		/* Print sysinfo cydata */
		_cyttsp4_pr_buf(ts, (u8 *)ts->sysinfo_ptr.cydata,
			ts->si_ofs.cydata_size, "sysinfo_cydata");
	}
	/* get the sysinfo test data */
	ts->si_ofs.test_size = ts->si_ofs.pcfg_ofs - ts->si_ofs.test_ofs;
	ts->sysinfo_ptr.test = kzalloc(ts->si_ofs.test_size, GFP_KERNEL);
	if (ts->sysinfo_ptr.test == NULL) {
		retval = -ENOMEM;
		dev_err(ts->dev,
			"%s: fail alloc test memory r=%d\n",
			__func__, retval);
		goto _cyttsp4_get_sysinfo_regs_exit;
	} else {
		memset(ts->sysinfo_ptr.test, 0, ts->si_ofs.test_size);
		retval = _cyttsp4_read_block_data(ts, ts->si_ofs.test_ofs,
			ts->si_ofs.test_size, ts->sysinfo_ptr.test,
			ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
		if (retval < 0) {
			dev_err(ts->dev,
				"%s: fail read test data r=%d\n",
				__func__, retval);
			goto _cyttsp4_get_sysinfo_regs_exit;
		}
		/* Print sysinfo test data */
		_cyttsp4_pr_buf(ts, (u8 *)ts->sysinfo_ptr.test,
			ts->si_ofs.test_size, "sysinfo_test_data");
#ifdef CY_USE_TMA400
		if (ts->sysinfo_ptr.test->post_codel & 0x01) {
			dev_info(ts->dev,
				"%s: Reset was a WATCHDOG RESET codel=%02X\n",
				__func__, ts->sysinfo_ptr.test->post_codel);
		}

		if (!(ts->sysinfo_ptr.test->post_codel & 0x02)) {
			dev_info(ts->dev,
				"%s: Config Data CRC FAIL codel=%02X\n",
				__func__, ts->sysinfo_ptr.test->post_codel);
		}

		if (!(ts->sysinfo_ptr.test->post_codel & 0x04)) {
			dev_info(ts->dev,
				"%s: PANEL TEST FAIL codel=%02X\n",
				__func__, ts->sysinfo_ptr.test->post_codel);
		}

		dev_info(ts->dev,
			"%s: SCANNING is %s codel=%02X\n", __func__,
			ts->sysinfo_ptr.test->post_codel & 0x08 ? "ENABLED" :
			"DISABLED", ts->sysinfo_ptr.test->post_codel);
#endif /* --CY_USE_TMA400 */
	}
	/* get the sysinfo pcfg data */
	ts->si_ofs.pcfg_size = ts->si_ofs.opcfg_ofs - ts->si_ofs.pcfg_ofs;
	ts->sysinfo_ptr.pcfg = kzalloc(ts->si_ofs.pcfg_size, GFP_KERNEL);
	if (ts->sysinfo_ptr.pcfg == NULL) {
		retval = -ENOMEM;
		dev_err(ts->dev,
			"%s: fail alloc pcfg memory r=%d\n",
			__func__, retval);
		goto _cyttsp4_get_sysinfo_regs_exit;
	} else {
		memset(ts->sysinfo_ptr.pcfg, 0, ts->si_ofs.pcfg_size);
		retval = _cyttsp4_read_block_data(ts, ts->si_ofs.pcfg_ofs,
			ts->si_ofs.pcfg_size, ts->sysinfo_ptr.pcfg,
			ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
		if (retval < 0) {
			dev_err(ts->dev,
				"%s: fail read pcfg data r=%d\n",
				__func__, retval);
			goto _cyttsp4_get_sysinfo_regs_exit;
		}
		/* Print sysinfo pcfg data */
		_cyttsp4_pr_buf(ts, (u8 *)ts->sysinfo_ptr.pcfg,
			ts->si_ofs.pcfg_size, "sysinfo_pcfg_data");
	}
	/* get the sysinfo opcfg data */
	ts->si_ofs.opcfg_size = ts->si_ofs.ddata_ofs - ts->si_ofs.opcfg_ofs;
	ts->sysinfo_ptr.opcfg = kzalloc(ts->si_ofs.opcfg_size, GFP_KERNEL);
	if (ts->sysinfo_ptr.opcfg == NULL) {
		retval = -ENOMEM;
		dev_err(ts->dev,
			"%s: fail alloc opcfg memory r=%d\n",
			__func__, retval);
		goto _cyttsp4_get_sysinfo_regs_exit;
	} else {
		memset(ts->sysinfo_ptr.opcfg, 0, ts->si_ofs.opcfg_size);
		retval = _cyttsp4_read_block_data(ts, ts->si_ofs.opcfg_ofs,
			ts->si_ofs.opcfg_size, ts->sysinfo_ptr.opcfg,
			ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
		if (retval < 0) {
			dev_err(ts->dev,
				"%s: fail read opcfg data r=%d\n",
				__func__, retval);
			goto _cyttsp4_get_sysinfo_regs_exit;
		}
		ts->si_ofs.cmd_ofs = ts->sysinfo_ptr.opcfg->cmd_ofs;
		ts->si_ofs.rep_ofs = ts->sysinfo_ptr.opcfg->rep_ofs;
		ts->si_ofs.rep_sz = (ts->sysinfo_ptr.opcfg->rep_szh * 256) +
			ts->sysinfo_ptr.opcfg->rep_szl;
		ts->si_ofs.num_btns = ts->sysinfo_ptr.opcfg->num_btns;
		if (ts->si_ofs.num_btns == 0)
			ts->si_ofs.num_btn_regs = 0;
		else {
			ts->si_ofs.num_btn_regs = ts->si_ofs.num_btns /
				CY_NUM_BTN_PER_REG;
			if (ts->si_ofs.num_btns % CY_NUM_BTN_PER_REG)
				ts->si_ofs.num_btn_regs++;
		}
		ts->si_ofs.tt_stat_ofs = ts->sysinfo_ptr.opcfg->tt_stat_ofs;
		ts->si_ofs.obj_cfg0 = ts->sysinfo_ptr.opcfg->obj_cfg0;
		ts->si_ofs.max_tchs = ts->sysinfo_ptr.opcfg->max_tchs &
			CY_SIZE_FIELD_MASK;
		ts->si_ofs.tch_rec_siz = ts->sysinfo_ptr.opcfg->tch_rec_siz &
			CY_SIZE_FIELD_MASK;

		/* Get the old touch fields */
		for (abs = CY_TCH_X; abs < CY_NUM_OLD_TCH_FIELDS; abs++) {
			ts->si_ofs.tch_abs[abs].ofs =
				ts->sysinfo_ptr.opcfg->tch_rec_old[abs].loc;
			ts->si_ofs.tch_abs[abs].size =
				_cyttsp4_bits_2_bytes(ts,
				ts->sysinfo_ptr.opcfg->tch_rec_old[abs].size &
				CY_SIZE_FIELD_MASK,
				&ts->si_ofs.tch_abs[abs].max);
			ts->si_ofs.tch_abs[abs].bofs =
				(ts->sysinfo_ptr.opcfg->tch_rec_old[abs].size &
				CY_BOFS_MASK) >> CY_BOFS_SHIFT;
			dev_vdbg(ts->dev,
				"%s: tch_rec_%s\n", __func__,
				cyttsp4_tch_abs_string[abs]);
			dev_vdbg(ts->dev,
				"%s:     ofs =%2d\n", __func__,
				ts->si_ofs.tch_abs[abs].ofs);
			dev_vdbg(ts->dev,
				"%s:     siz =%2d\n", __func__,
				ts->si_ofs.tch_abs[abs].size);
			dev_vdbg(ts->dev,
				"%s:     max =%2d\n", __func__,
				ts->si_ofs.tch_abs[abs].max);
			dev_vdbg(ts->dev,
				"%s:     bofs=%2d\n", __func__,
				ts->si_ofs.tch_abs[abs].bofs);
		}

#ifdef CY_USE_TMA400_SP2
#ifdef CY_USE_TMA400
		/* skip over the button fields */

		/* Get the new touch fields */
		for (i = 0; abs < CY_TCH_NUM_ABS; abs++, i++) {
			ts->si_ofs.tch_abs[abs].ofs =
				ts->sysinfo_ptr.opcfg->tch_rec_new[i].loc;
			ts->si_ofs.tch_abs[abs].size =
				_cyttsp4_bits_2_bytes(ts,
				ts->sysinfo_ptr.opcfg->tch_rec_new[i].size &
				CY_SIZE_FIELD_MASK,
				&ts->si_ofs.tch_abs[abs].max);
			ts->si_ofs.tch_abs[abs].bofs =
				(ts->sysinfo_ptr.opcfg->tch_rec_new[i].size &
				CY_BOFS_MASK) >> CY_BOFS_SHIFT;
			dev_vdbg(ts->dev,
				"%s: tch_rec_%s\n", __func__,
				cyttsp4_tch_abs_string[abs]);
			dev_vdbg(ts->dev,
				"%s:     ofs =%2d\n", __func__,
				ts->si_ofs.tch_abs[abs].ofs);
			dev_vdbg(ts->dev,
				"%s:     siz =%2d\n", __func__,
				ts->si_ofs.tch_abs[abs].size);
			dev_vdbg(ts->dev,
				"%s:     max =%2d\n", __func__,
				ts->si_ofs.tch_abs[abs].max);
			dev_vdbg(ts->dev,
				"%s:     bofs=%2d\n", __func__,
				ts->si_ofs.tch_abs[abs].bofs);
		}
#endif /* --CY_USE_TMA400 */
#endif /* --CY_USE_TMA400_SP2 */

		ts->si_ofs.btn_rec_siz = ts->sysinfo_ptr.opcfg->btn_rec_siz;
		ts->si_ofs.btn_diff_ofs = ts->sysinfo_ptr.opcfg->btn_diff_ofs;
		ts->si_ofs.btn_diff_siz = ts->sysinfo_ptr.opcfg->btn_diff_siz;
		ts->si_ofs.mode_size = ts->si_ofs.tt_stat_ofs + 1;
		ts->si_ofs.data_size = ts->si_ofs.max_tchs *
			ts->sysinfo_ptr.opcfg->tch_rec_siz;
		if (ts->si_ofs.num_btns)
			ts->si_ofs.mode_size += ts->si_ofs.num_btn_regs;

		/* Print sysinfo opcfg data */
		_cyttsp4_pr_buf(ts, (u8 *)ts->sysinfo_ptr.opcfg,
			ts->si_ofs.opcfg_size, "sysinfo_opcfg_data");
	}

	/* get the sysinfo ddata data */
	ts->si_ofs.ddata_size = ts->si_ofs.mdata_ofs - ts->si_ofs.ddata_ofs;
	ts->sysinfo_ptr.ddata = kzalloc(ts->si_ofs.ddata_size, GFP_KERNEL);
	if (ts->sysinfo_ptr.ddata == NULL) {
		dev_err(ts->dev,
			"%s: fail alloc ddata memory r=%d\n",
			__func__, retval);
		/* continue */
	} else {
		memset(ts->sysinfo_ptr.ddata, 0, ts->si_ofs.ddata_size);
		retval = _cyttsp4_read_block_data(ts, ts->si_ofs.ddata_ofs,
			ts->si_ofs.ddata_size, ts->sysinfo_ptr.ddata,
			ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
		if (retval < 0) {
			dev_err(ts->dev,
				"%s: fail read ddata data r=%d\n",
				__func__, retval);
			goto _cyttsp4_get_sysinfo_regs_exit;
		}
		/* Print sysinfo ddata */
		_cyttsp4_pr_buf(ts, (u8 *)ts->sysinfo_ptr.ddata,
			ts->si_ofs.ddata_size, "sysinfo_ddata");
	}
	/* get the sysinfo mdata data */
	ts->si_ofs.mdata_size = ts->si_ofs.map_sz - ts->si_ofs.mdata_ofs;
	ts->sysinfo_ptr.mdata = kzalloc(ts->si_ofs.mdata_size, GFP_KERNEL);
	if (ts->sysinfo_ptr.mdata == NULL) {
		dev_err(ts->dev,
			"%s: fail alloc mdata memory r=%d\n",
			__func__, retval);
		/* continue */
	} else {
		memset(ts->sysinfo_ptr.mdata, 0, ts->si_ofs.mdata_size);
		retval = _cyttsp4_read_block_data(ts, ts->si_ofs.mdata_ofs,
			ts->si_ofs.mdata_size, ts->sysinfo_ptr.mdata,
			ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
		if (retval < 0) {
			dev_err(ts->dev,
				"%s: fail read mdata data r=%d\n",
				__func__, retval);
			goto _cyttsp4_get_sysinfo_regs_exit;
		}
		/* Print sysinfo mdata */
		_cyttsp4_pr_buf(ts, (u8 *)ts->sysinfo_ptr.mdata,
			ts->si_ofs.mdata_size, "sysinfo_mdata");
	}

	if (ts->si_ofs.num_btns) {
		ts->si_ofs.btn_keys_size = ts->si_ofs.num_btns *
			sizeof(struct cyttsp4_btn);
		ts->btn = kzalloc(ts->si_ofs.btn_keys_size, GFP_KERNEL);
		if (ts->btn == NULL) {
			dev_err(ts->dev,
				"%s: fail alloc btn_keys memory r=%d\n",
				__func__, retval);
		} else {
			if (ts->platform_data->sett
				[CY_IC_GRPNUM_BTN_KEYS] == NULL)
				num_defined_keys = 0;
			else if (ts->platform_data->sett
				[CY_IC_GRPNUM_BTN_KEYS]->data == NULL)
				num_defined_keys = 0;
			else
				num_defined_keys = ts->platform_data->sett
				[CY_IC_GRPNUM_BTN_KEYS]->size;
			for (btn = 0; btn < ts->si_ofs.num_btns &&
				btn < num_defined_keys; btn++) {
					key_table = (u16 *)ts->platform_data->sett
						[CY_IC_GRPNUM_BTN_KEYS]->data;
					ts->btn[btn].key_code = key_table[btn];
					ts->btn[btn].enabled = true;
			}
			for (; btn < ts->si_ofs.num_btns; btn++) {
				ts->btn[btn].key_code = KEY_RESERVED;
				ts->btn[btn].enabled = true;
			}
		}
	} else {
		ts->si_ofs.btn_keys_size = 0;
		ts->btn = NULL;
	}

	dev_vdbg(ts->dev,
		"%s: cydata_ofs =%4d siz=%4d\n", __func__,
		ts->si_ofs.cydata_ofs, ts->si_ofs.cydata_size);
	dev_vdbg(ts->dev,
		"%s: test_ofs   =%4d siz=%4d\n", __func__,
		ts->si_ofs.test_ofs, ts->si_ofs.test_size);
	dev_vdbg(ts->dev,
		"%s: pcfg_ofs   =%4d siz=%4d\n", __func__,
		ts->si_ofs.pcfg_ofs, ts->si_ofs.pcfg_size);
	dev_vdbg(ts->dev,
		"%s: opcfg_ofs  =%4d siz=%4d\n", __func__,
		ts->si_ofs.opcfg_ofs, ts->si_ofs.opcfg_size);
	dev_vdbg(ts->dev,
		"%s: ddata_ofs  =%4d siz=%4d\n", __func__,
		ts->si_ofs.ddata_ofs, ts->si_ofs.ddata_size);
	dev_vdbg(ts->dev,
		"%s: mdata_ofs  =%4d siz=%4d\n", __func__,
		ts->si_ofs.mdata_ofs, ts->si_ofs.mdata_size);

	dev_vdbg(ts->dev,
		"%s: cmd_ofs       =%4d\n", __func__, ts->si_ofs.cmd_ofs);
	dev_vdbg(ts->dev,
		"%s: rep_ofs       =%4d\n", __func__, ts->si_ofs.rep_ofs);
	dev_vdbg(ts->dev,
		"%s: rep_sz        =%4d\n", __func__, ts->si_ofs.rep_sz);
	dev_vdbg(ts->dev,
		"%s: num_btns      =%4d\n", __func__, ts->si_ofs.num_btns);
	dev_vdbg(ts->dev,
		"%s: num_btn_regs  =%4d\n", __func__, ts->si_ofs.num_btn_regs);
	dev_vdbg(ts->dev,
		"%s: tt_stat_ofs   =%4d\n", __func__, ts->si_ofs.tt_stat_ofs);
	dev_vdbg(ts->dev,
		"%s: tch_rec_siz   =%4d\n", __func__, ts->si_ofs.tch_rec_siz);
	dev_vdbg(ts->dev,
		"%s: max_tchs      =%4d\n", __func__, ts->si_ofs.max_tchs);
	dev_vdbg(ts->dev,
		"%s: mode_siz      =%4d\n", __func__, ts->si_ofs.mode_size);
	dev_vdbg(ts->dev,
		"%s: data_siz      =%4d\n", __func__, ts->si_ofs.data_size);
	dev_vdbg(ts->dev,
		"%s: map_sz        =%4d\n", __func__, ts->si_ofs.map_sz);

	dev_vdbg(ts->dev,
		"%s: btn_rec_siz   =%2d\n", __func__, ts->si_ofs.btn_rec_siz);
	dev_vdbg(ts->dev,
		"%s: btn_diff_ofs  =%2d\n", __func__, ts->si_ofs.btn_diff_ofs);
	dev_vdbg(ts->dev,
		"%s: btn_diff_siz  =%2d\n", __func__, ts->si_ofs.btn_diff_siz);

	dev_vdbg(ts->dev,
		"%s: mode_size     =%2d\n", __func__, ts->si_ofs.mode_size);
	dev_vdbg(ts->dev,
		"%s: data_size     =%2d\n", __func__, ts->si_ofs.data_size);

	if (ts->xy_mode == NULL)
		ts->xy_mode = kzalloc(ts->si_ofs.mode_size, GFP_KERNEL);
	if (ts->xy_data == NULL)
		ts->xy_data = kzalloc(ts->si_ofs.data_size, GFP_KERNEL);
	if (ts->xy_data_touch1 == NULL) {
		ts->xy_data_touch1 = kzalloc(ts->si_ofs.tch_rec_siz + 1,
			GFP_KERNEL);
	}
	if (ts->btn_rec_data == NULL) {
		ts->btn_rec_data = kzalloc(ts->si_ofs.btn_rec_siz *
			ts->si_ofs.num_btns, GFP_KERNEL);
	}
	if ((ts->xy_mode == NULL) || (ts->xy_data == NULL) ||
		(ts->xy_data_touch1 == NULL) || (ts->btn_rec_data == NULL)) {
			dev_err(ts->dev,
				"%s: fail memory alloc xy_mode=%p xy_data=%p"
				"xy_data_touch1=%p btn_rec_data=%p\n", __func__,
				ts->xy_mode, ts->xy_data,
				ts->xy_data_touch1, ts->btn_rec_data);
			/* continue */
	}

	dev_vdbg(ts->dev,
		"%s: xy_mode=%p xy_data=%p xy_data_touch1=%p\n",
		__func__, ts->xy_mode, ts->xy_data, ts->xy_data_touch1);

_cyttsp4_get_sysinfo_regs_exit:
	/* provide flow control handshake */
	retval = _cyttsp4_handshake(ts, ts->sysinfo_data.hst_mode);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: handshake fail on sysinfo reg\n",
			__func__);
		/* continue; rely on handshake tmo */
	}

_cyttsp4_get_sysinfo_regs_exit_no_handshake:
	return retval;
}

static int _cyttsp4_load_status_regs(struct cyttsp4 *ts)
{
	int rep_stat_ofs = 0;
	int retval = 0;

	rep_stat_ofs = ts->si_ofs.rep_ofs + 1;
	if (ts->xy_mode == NULL) {
		dev_err(ts->dev,
			"%s: mode ptr not yet initialized xy_mode=%p\n",
			__func__, ts->xy_mode);
		/* continue */
	} else {
		retval = _cyttsp4_read_block_data(ts, CY_REG_BASE,
			ts->si_ofs.mode_size, ts->xy_mode,
			ts->platform_data->addr[CY_TCH_ADDR_OFS], true);


		if (retval < 0) {
			dev_err(ts->dev,
				"%s: fail read mode regs r=%d\n",
				__func__, retval);
			retval = -EIO;
		}
		_cyttsp4_pr_buf(ts, ts->xy_mode, ts->si_ofs.mode_size, "xy_mode");
	}
	return retval;
}

static void _cyttsp4_get_touch_axis(struct cyttsp4 *ts,	enum cyttsp4_tch_abs abs, int *axis, int size, int max, u8 *xy_data, int bofs)
{
	int nbyte = 0;
	int next = 0;

	for (nbyte = 0, *axis = 0, next = 0; nbyte < size; nbyte++) {
		dev_vdbg(ts->dev,
			"%s: *axis=%02X(%d) size=%d max=%08X xy_data=%p"
			" xy_data[%d]=%02X(%d)\n",
			__func__, *axis, *axis, size, max, xy_data, next,
			xy_data[next], xy_data[next]);
		*axis = (*axis * 256) + (xy_data[next] >> bofs);
		next++;
	}

	*axis &= max - 1;

#ifdef CY_USE_TMA400_SP2
#ifdef CY_USE_TMA400
	/* sign extend signals that can have negative values */
	if (abs == CY_TCH_OR) {
		if (*axis >= (max / 2))
			*axis = -((~(*axis) & (max - 1)) + 1);
	}
#endif /* --CY_USE_TMA400 */
#endif /* --CY_USE_TMA400_SP2 */

	dev_vdbg(ts->dev,
		"%s: *axis=%02X(%d) size=%d max=%08X xy_data=%p"
		" xy_data[%d]=%02X(%d)\n",
		__func__, *axis, *axis, size, max, xy_data, next,
		xy_data[next], xy_data[next]);
}

static void _cyttsp4_get_touch(struct cyttsp4 *ts, struct cyttsp4_touch *touch, u8 *xy_data)
{

	enum cyttsp4_tch_abs abs = 0;

#ifdef CY_USE_DEBUG_TOOLS
	int tmp = 0;
	bool flipped = false;
#endif /* --CY_USE_DEBUG_TOOLS */

	for (abs = CY_TCH_X; abs < CY_TCH_NUM_ABS; abs++) {

		_cyttsp4_get_touch_axis(
			ts, 
			abs, 
			&touch->abs[abs],
			ts->si_ofs.tch_abs[abs].size,
			ts->si_ofs.tch_abs[abs].max,
			xy_data + ts->si_ofs.tch_abs[abs].ofs,
			ts->si_ofs.tch_abs[abs].bofs
		);

		dev_vdbg(ts->dev, "%s: get %s=%08X(%d) size=%d ofs=%d max=%d xy_data+ofs=%p bofs=%d\n", __func__, 
			cyttsp4_tch_abs_string[abs],
			touch->abs[abs], 
			touch->abs[abs],
			ts->si_ofs.tch_abs[abs].size,
			ts->si_ofs.tch_abs[abs].ofs,
			ts->si_ofs.tch_abs[abs].max,
			xy_data + ts->si_ofs.tch_abs[abs].ofs,
			ts->si_ofs.tch_abs[abs].bofs
		);
	}

#ifdef CY_USE_DEBUG_TOOLS

	if (ts->flags & CY_FLAG_FLIP) {
		tmp = touch->abs[CY_TCH_X];
		touch->abs[CY_TCH_X] = touch->abs[CY_TCH_Y];
		touch->abs[CY_TCH_Y] = tmp;
		flipped = true;
	}
	
	if (ts->flags & CY_FLAG_INV_X) {
		if (!flipped) {
			touch->abs[CY_TCH_X] = ts->platform_data->frmwrk->abs[(CY_ABS_X_OST * CY_NUM_ABS_SET) + CY_MAX_OST] - touch->abs[CY_TCH_X];
		} else {
			touch->abs[CY_TCH_X] = ts->platform_data->frmwrk->abs[(CY_ABS_Y_OST * CY_NUM_ABS_SET) + CY_MAX_OST] - touch->abs[CY_TCH_X];
		}
	}
	if (ts->flags & CY_FLAG_INV_Y) {
		if (!flipped) {
			touch->abs[CY_TCH_Y] = ts->platform_data->frmwrk->abs[(CY_ABS_Y_OST * CY_NUM_ABS_SET) + CY_MAX_OST] - touch->abs[CY_TCH_Y];
		} else {
			touch->abs[CY_TCH_Y] = ts->platform_data->frmwrk->abs[(CY_ABS_X_OST * CY_NUM_ABS_SET) + CY_MAX_OST] - touch->abs[CY_TCH_Y];
		}
	}

#endif /* --CY_USE_DEBUG_TOOLS */

}

static void _pantech_reset_fingerinfo(void)
{
	int i=0, j=0;
	for (i=0; i<CYTTSP_MAX_TOUCHNUM+1; i++) {
		for ( j=0; j<CY_TCH_NUM_ABS; j++) {
			fingerInfo[i].abs[j] = -1;
		}		
		fingerInfo[i].mode = -1;
	}
}

static void _pantech_release_eventhub(struct cyttsp4 *ts)
{
	int i=0; 

	_pantech_reset_fingerinfo();
	input_report_key(ts->input, BTN_TOUCH, 0);

	for (i=0; i<CYTTSP_MAX_TOUCHNUM; i++) {
		input_mt_slot(ts->input, i);
		input_report_abs(ts->input, ABS_MT_TRACKING_ID, -1);
	}

	input_sync(ts->input);
}


#define ABS(_x, _y) ((_x) > (_y)) ? ((_x) -(_y)) : ((_y) -(_x))

static int _cyttsp4_xy_worker(struct cyttsp4 *ts)
{
	struct cyttsp4_touch touch;
	u8 num_cur_tch = 0;
	u8 hst_mode = 0;
	u8 rep_len = 0;
	u8 rep_stat = 0;
	u8 tt_stat = 0;
	int i = 0;	
	int retval = 0;
	
	//	p11309
#ifdef CY_USE_AVOID_RELEASE_EVENT_BUG
	int same_finger_cnt=0;
#endif

	int touch_mask[CY_NUM_TCH_ID];
	for (i=0;i<CY_NUM_TCH_ID;i++) touch_mask[i] = -1;   


	/*
	 * Get event data from CYTTSP device.
	 * The event data includes all data
	 * for all active touches.
	 */
	/*
	 * Use 2 reads: first to get mode bytes,
	 * second to get status (touch count) and touch 1 data.
	 * An optional 3rd read to get touch 2 - touch n data.
	 */

	memset(&touch, 0x0, sizeof(struct cyttsp4_touch));
	memset(ts->xy_mode, 0x0, ts->si_ofs.mode_size);
	memset(ts->xy_data_touch1, 0x0, 1 + ts->si_ofs.tch_rec_siz);	

	retval = _cyttsp4_load_status_regs(ts);
	if (retval < 0) {
		/*
		 * bus failure implies Watchdog -> bootloader running
		 * on TMA884 parts
		*/
		dev_err(ts->dev, "%s: 1st read fail on mode regs r=%d\n", __func__, retval);
		retval = -EIO;
		goto _cyttsp4_xy_worker_exit;
	}

	retval = _cyttsp4_read_block_data(ts, ts->si_ofs.tt_stat_ofs,
		1+ts->si_ofs.tch_rec_siz, ts->xy_data_touch1,
		ts->platform_data->addr[CY_TCH_ADDR_OFS], true);

	if (retval < 0) {
		/* bus failure may imply bootloader running */
		dev_err(ts->dev, "%s: read fail on mode regs r=%d\n", __func__, retval);
		retval = -EIO;
		goto _cyttsp4_xy_worker_exit;
	}

	hst_mode = ts->xy_mode[CY_REG_BASE];
	rep_len = ts->xy_mode[ts->si_ofs.rep_ofs];
	rep_stat = ts->xy_mode[ts->si_ofs.rep_ofs + 1];
	tt_stat = ts->xy_data_touch1[0];

	dev_dbg(ts->dev, "%s: hst_mode=%02X rep_len=%d rep_stat=%d tt_stat=%02X\n",
		__func__, hst_mode, rep_len, (rep_stat & 0x1C)>>2, tt_stat);
	

	if (rep_len == 0) {
		dev_err(ts->dev, "%s: report length error rep_len=%d\n", __func__, rep_len);
		goto _cyttsp4_xy_worker_exit;
	}

	if (GET_NUM_TOUCHES(tt_stat) > 0) {
		memcpy(ts->xy_data, ts->xy_data_touch1 + 1, ts->si_ofs.tch_rec_siz);
	}

	if (GET_NUM_TOUCHES(tt_stat) > 1) {
		retval = _cyttsp4_read_block_data(ts, ts->si_ofs.tt_stat_ofs +
			1 + ts->si_ofs.tch_rec_siz,
			(GET_NUM_TOUCHES(tt_stat) - 1) * ts->si_ofs.tch_rec_siz,
			ts->xy_data + ts->si_ofs.tch_rec_siz,
			ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
		if (retval < 0) {
			dev_err(ts->dev, "%s: read fail on touch regs r=%d\n", __func__, retval);
			goto _cyttsp4_xy_worker_exit;
		}
	}

#ifdef CY_USE_INCLUDE_FBL
	if(DebugON == true)
	{
		if (ts->si_ofs.num_btns > 0) {
			retval = _cyttsp4_read_block_data(ts,
				(ts->si_ofs.tt_stat_ofs + 1) +
				(ts->si_ofs.max_tchs * ts->si_ofs.tch_rec_siz),
				ts->si_ofs.btn_rec_siz * ts->si_ofs.num_btns,
				ts->btn_rec_data,
				ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
			if (retval < 0) {
				dev_err(ts->dev, "%s: read fail on button records r=%d\n", __func__, retval);
				goto _cyttsp4_xy_worker_exit;
			}
			_cyttsp4_pr_buf(ts, ts->btn_rec_data, ts->si_ofs.btn_rec_siz * ts->si_ofs.num_btns, "btn_rec_data");
		}
	}
#endif /* --CY_USE_INCLUDE_FBL */

	/* provide flow control handshake */
	retval = _cyttsp4_handshake(ts, hst_mode);
	if (retval < 0) {
		dev_err(ts->dev, "%s: handshake fail on operational reg\n", __func__);
		/* continue; rely on handshake tmo */
		retval = 0;
	}

	/* determine number of currently active touches */
	num_cur_tch = GET_NUM_TOUCHES(tt_stat);


	/* print xy data */
	_cyttsp4_pr_buf(ts, ts->xy_data, num_cur_tch * ts->si_ofs.tch_rec_siz, "xy_data");

	/* check for any error conditions */
	if (ts->driver_state == CY_IDLE_STATE) {
		dev_err(ts->dev, "%s: IDLE STATE detected\n", __func__);
		retval = 0;
		goto _cyttsp4_xy_worker_exit;
	} else if (IS_BAD_PKT(rep_stat)) {
		dev_err(ts->dev, "%s: Invalid buffer detected\n", __func__);
		retval = 0;
		goto _cyttsp4_xy_worker_exit;
	} else if (IS_BOOTLOADERMODE(rep_stat)) {
		dev_info(ts->dev, "%s: BL mode found in ACTIVE state\n", __func__);
		retval = -EIO;
		goto _cyttsp4_xy_worker_exit;
	} else if (GET_HSTMODE(hst_mode) == GET_HSTMODE(CY_SYSINFO_MODE)) {
		/* if in sysinfo mode switch to op mode */
		dev_err(ts->dev, "%s: Sysinfo mode=0x%02X detected in ACTIVE state\n", 	__func__, hst_mode);
		retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
		if (retval < 0) {
			_cyttsp4_change_state(ts, CY_IDLE_STATE);
			dev_err(ts->dev, "%s: Fail set operational mode (r=%d)\n", __func__, retval);
		} else {
			_cyttsp4_change_state(ts, CY_ACTIVE_STATE);
			dev_vdbg(ts->dev, "%s: enable handshake\n", __func__);
		}
		goto _cyttsp4_xy_worker_exit;
	} else if (IS_LARGE_AREA(tt_stat)) {
		/* terminate all active tracks */
		num_cur_tch = 0;			
		dev_err(ts->dev, "%s: Large area detected\n", __func__);
	} else if (num_cur_tch > ts->si_ofs.max_tchs) {
		if (num_cur_tch == 0x1F) {
			/* terminate all active tracks */
			dev_err(ts->dev, "%s: Num touch err detected (n=%d)\n", __func__, num_cur_tch);
			num_cur_tch = 0;
		} else {
			dev_err(ts->dev,
			"%s: too many tch; set to max tch (n=%d c=%d)\n", __func__, num_cur_tch, CY_NUM_TCH_ID);
			num_cur_tch = CY_NUM_TCH_ID;
		}
	}

	//martin
	dev_dbg(ts->dev, "%s: num_cur_tch=%d\n", __func__, num_cur_tch);	
	
//	printk("[CYTTSP4] FingerNum : %d (%d)\n", num_cur_tch, ts->num_prv_tch);

#ifdef CY_USE_AVOID_RELEASE_EVENT_BUG
_cyttsp4_xy_worker_set_eventhub:

	if ( ts->num_prv_tch != 0 && num_cur_tch == 0 ) {
#else
	if ( num_cur_tch == 0 ) {
#endif
		
		for (i=0; i < CYTTSP_MAX_TOUCHNUM; i++)
		{
			if ( fingerInfo[i].abs[CY_TCH_T] >= 0 ) {
				fingerInfo[i].abs[CY_TCH_T] = -1;
				input_mt_slot(ts->input, i);				
				input_report_abs(ts->input, ABS_MT_TRACKING_ID, fingerInfo[i].abs[CY_TCH_T]);

//				printk("[++++ cyttsp4: All Release] %d\n", i);
			}			
		}

		input_report_key(ts->input, BTN_TOUCH, 0 );
		input_sync(ts->input);
//		printk("[++++ cyttsp4: Sync]\n");
	}
#ifdef CY_USE_AVOID_RELEASE_EVENT_BUG
	else if ( ts->num_prv_tch == 0 && num_cur_tch == 0 ) {
		//	do nothing
	}
#endif 
	else {

		for (i=0;i<num_cur_tch;i++) {
			_cyttsp4_get_touch(ts, &touch, ts->xy_data + (i * ts->si_ofs.tch_rec_siz));
			touch_mask[touch.abs[CY_TCH_T]] = i;

// 			printk("[++++ cyttsp4: %d] %d, %d, (%d, %d), %d, %d, %d, %d\n",
// 				num_cur_tch, 
// 				i, 
// 				touch.abs[CY_TCH_T], 
// 				touch.abs[CY_TCH_X],
// 				touch.abs[CY_TCH_Y], 
// 				touch.abs[CY_TCH_P], 
// 				touch.abs[CY_TCH_E], 
// 				touch.abs[CY_TCH_O], 
// 				touch.abs[CY_TCH_W]
// 			);
		}

#ifdef CY_USE_AVOID_RELEASE_EVENT_BUG
		//	check same finger for release event
		for (i=0; i<CYTTSP_MAX_TOUCHNUM; i++) {

			if ( touch_mask[i] >= 0 ) 
			{
				_cyttsp4_get_touch(ts, &touch, ts->xy_data + (touch_mask[i] * ts->si_ofs.tch_rec_siz));				

				if (	fingerInfo[touch.abs[CY_TCH_T]].abs[CY_TCH_X] == touch.abs[CY_TCH_X]	&&
						fingerInfo[touch.abs[CY_TCH_T]].abs[CY_TCH_Y] == touch.abs[CY_TCH_Y]	&&
						fingerInfo[touch.abs[CY_TCH_T]].abs[CY_TCH_P] == touch.abs[CY_TCH_P]	&&
						fingerInfo[touch.abs[CY_TCH_T]].abs[CY_TCH_E] == touch.abs[CY_TCH_E]	&&
						fingerInfo[touch.abs[CY_TCH_T]].abs[CY_TCH_O] == touch.abs[CY_TCH_O]	&&
						fingerInfo[touch.abs[CY_TCH_T]].abs[CY_TCH_W] == touch.abs[CY_TCH_W]	) 
						same_finger_cnt++;

				if ( same_finger_cnt == num_cur_tch	) {
					num_cur_tch = 0;
					goto _cyttsp4_xy_worker_set_eventhub;
				}				
			}			
		}
#endif

		for (i=0; i<CYTTSP_MAX_TOUCHNUM; i++) {

			if ( touch_mask[i] < 0 ) 
			{
				if ( fingerInfo[i].abs[CY_TCH_T] >= 0 ) {
					fingerInfo[i].abs[CY_TCH_T] = -1;
					input_mt_slot(ts->input, i);					
					input_report_abs(ts->input, ABS_MT_TRACKING_ID, fingerInfo[i].abs[CY_TCH_T]);

//					printk("[++++ cyttsp4: Release] %d (%d, %d)\n", i, touch.abs[CY_TCH_X], touch.abs[CY_TCH_Y]);
				}
			}
			else {

				_cyttsp4_get_touch(ts, &touch, ts->xy_data + (touch_mask[i] * ts->si_ofs.tch_rec_siz));

				input_mt_slot(ts->input, i);				
				if ( fingerInfo[i].abs[CY_TCH_T] == -1 ) {
					fingerInfo[i].abs[CY_TCH_T] = input_mt_new_trkid(ts->input);
					input_report_abs(ts->input, ABS_MT_TRACKING_ID, fingerInfo[i].abs[CY_TCH_T]);
//					printk("[++++ cyttsp4: Press] %d (%d, %d)\n", i, touch.abs[CY_TCH_X], touch.abs[CY_TCH_Y]);
				}
				else {
//					printk("[++++ cyttsp4: Move] %d (%d, %d)\n", i, touch.abs[CY_TCH_X], touch.abs[CY_TCH_Y]);
				}

				fingerInfo[touch.abs[CY_TCH_T]].abs[CY_TCH_X] = touch.abs[CY_TCH_X];
				fingerInfo[touch.abs[CY_TCH_T]].abs[CY_TCH_Y] = touch.abs[CY_TCH_Y];
				fingerInfo[touch.abs[CY_TCH_T]].abs[CY_TCH_P] = touch.abs[CY_TCH_P];
				fingerInfo[touch.abs[CY_TCH_T]].abs[CY_TCH_E] = touch.abs[CY_TCH_E];
				fingerInfo[touch.abs[CY_TCH_T]].abs[CY_TCH_O] = touch.abs[CY_TCH_O];
				fingerInfo[touch.abs[CY_TCH_T]].abs[CY_TCH_W] = touch.abs[CY_TCH_W];

				input_report_abs(ts->input, ABS_MT_POSITION_X, touch.abs[CY_TCH_X]);
				input_report_abs(ts->input, ABS_MT_POSITION_Y, touch.abs[CY_TCH_Y]);
				input_report_abs(ts->input, ABS_MT_PRESSURE, touch.abs[CY_TCH_P]);
				input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR, touch.abs[CY_TCH_W]);
#ifdef CY_USE_TMA400_SP2
#ifdef CY_USE_TMA400
				input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, touch.abs[CY_TCH_MAJ]);
				input_report_abs(ts->input, ABS_MT_TOUCH_MINOR, touch.abs[CY_TCH_MIN]);
				input_report_abs(ts->input, ABS_MT_ORIENTATION, touch.abs[CY_TCH_OR]);
#endif /* --CY_USE_TMA400 */
#endif /* --CY_USE_TMA400_SP2 */

			}			
		}
		
		input_report_key(ts->input, BTN_TOUCH, 1);
		input_sync(ts->input);				
//		printk("[++++ cyttsp4: Sync]\n");
	}
	

_cyttsp4_xy_worker_exit:

	ts->num_prv_tch = num_cur_tch;

#ifdef CY_USE_LEVEL_IRQ
	udelay(500);
#endif

	return retval;
}

#ifdef CY_USE_WATCHDOG
#define CY_TIMEOUT msecs_to_jiffies(1000)
static void _cyttsp4_start_wd_timer(struct cyttsp4 *ts)
{
	mod_timer(&ts->timer, jiffies + CY_TIMEOUT);

	return;
}

static void _cyttsp4_stop_wd_timer(struct cyttsp4 *ts)
{
	del_timer(&ts->timer);
	cancel_work_sync(&ts->work);

	return;
}

static void cyttsp4_timer_watchdog(struct work_struct *work)
{
	struct cyttsp4 *ts = container_of(work, struct cyttsp4, work);
	u8 rep_stat = 0;
	int retval = 0;

	if (ts == NULL) {
		dev_err(ts->dev,
			"%s: NULL context pointer\n", __func__);
		return;
	}

	mutex_lock(&ts->data_lock);
	if (ts->driver_state == CY_ACTIVE_STATE) {
		retval = _cyttsp4_load_status_regs(ts);
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: failed to access device"
				" in watchdog timer r=%d\n", __func__, retval);
			_cyttsp4_queue_startup(ts, false);
			goto cyttsp4_timer_watchdog_exit_error;
		}
		rep_stat = ts->xy_mode[ts->si_ofs.rep_ofs + 1];
		if (IS_BOOTLOADERMODE(rep_stat)) {
			dev_err(ts->dev,
			"%s: device found in bootloader mode"
				" when operational mode rep_stat=0x%02X\n",
				__func__, rep_stat);
			_cyttsp4_queue_startup(ts, false);
			goto cyttsp4_timer_watchdog_exit_error;
		}
	}

	_cyttsp4_start_wd_timer(ts);

cyttsp4_timer_watchdog_exit_error:
	mutex_unlock(&ts->data_lock);
	return;
}

static void cyttsp4_timer(unsigned long handle)
{
	struct cyttsp4 *ts = (struct cyttsp4 *)handle;

	if (!work_pending(&ts->work))
		schedule_work(&ts->work);

	return;
}
#endif

static int _cyttsp4_soft_reset(struct cyttsp4 *ts)
{
	u8 cmd = CY_SOFT_RESET_MODE;

	return _cyttsp4_write_block_data(ts, CY_REG_BASE,
		sizeof(cmd), &cmd,
		ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
}

static int _cyttsp4_reset(struct cyttsp4 *ts)
{
	enum cyttsp4_driver_state tmp_state = ts->driver_state;
	int retval = 0;

	dbg_func_in();
#if 0
	if(TSP_Restart())
	{
		 // HW Reset fail
		retval = _cyttsp4_soft_reset(ts);
		ts->soft_reset_asserted = true;
	}
	else
		ts->soft_reset_asserted = false;

	ts->current_mode = CY_MODE_BOOTLOADER;
	ts->driver_state = CY_BL_STATE;
	if (tmp_state != CY_BL_STATE)
		_cyttsp4_pr_state(ts);
	return retval;

#else

	if (ts->platform_data->hw_reset) {
		retval = ts->platform_data->hw_reset();
		if (retval == -ENOSYS) {
			retval = _cyttsp4_soft_reset(ts);
			ts->soft_reset_asserted = true;
		} else
			ts->soft_reset_asserted = false;
	} else {
		retval = _cyttsp4_soft_reset(ts);
		ts->soft_reset_asserted = true;
	}

	if (retval < 0) {
		_cyttsp4_pr_state(ts);
		return retval;
	} else {
		ts->current_mode = CY_MODE_BOOTLOADER;
		ts->driver_state = CY_BL_STATE;
		if (tmp_state != CY_BL_STATE)
			_cyttsp4_pr_state(ts);
		return retval;
	}
	dbg_func_out();

#endif
}

static void cyttsp4_ts_work_func(struct work_struct *work)
{
	struct cyttsp4 *ts = container_of(work, struct cyttsp4, cyttsp4_resume_startup_work);
	int retval = 0;

	mutex_lock(&ts->data_lock);
	
	retval = _cyttsp4_startup(ts);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Startup failed with error code %d\n", __func__, retval);
		_cyttsp4_change_state(ts, CY_IDLE_STATE);
	} 
	mutex_unlock(&ts->data_lock);

	return;
}

static int _cyttsp4_enter_sleep(struct cyttsp4 *ts)
{
	int retval = 0;
#if defined(CONFIG_PM_SLEEP) || 	defined(CONFIG_PM) || 	defined(CONFIG_HAS_EARLYSUSPEND)
	uint8_t sleep = CY_DEEP_SLEEP_MODE;

	dev_vdbg(ts->dev,
		"%s: Put the part back to sleep\n", __func__);

	retval = _cyttsp4_write_block_data(ts, CY_REG_BASE,
		sizeof(sleep), &sleep,
		ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Failed to write sleep bit r=%d\n",
			__func__, retval);
	} else
		_cyttsp4_change_state(ts, CY_SLEEP_STATE);
#endif
	return retval;
}

static int _cyttsp4_wakeup(struct cyttsp4 *ts)	
{
	int retval = 0;
#if defined(CONFIG_PM_SLEEP) || 	defined(CONFIG_PM) || 	defined(CONFIG_HAS_EARLYSUSPEND)
	unsigned long timeout = 0;
	unsigned long uretval = 0;
	u8 hst_mode = 0;
#ifdef CY_USE_TMA400
	u8 rep_stat = 0;
#endif /* --CY_USE_TMA400 */
	int wake = CY_WAKE_DFLT;

	_cyttsp4_change_state(ts, CY_CMD_STATE);
	INIT_COMPLETION(ts->int_running);


	if (ts->platform_data->hw_recov == NULL) {
		dev_vdbg(ts->dev, "%s: no hw_recov function\n", __func__);
		retval = -ENOSYS;
	} else {
		/* wake using strobe on host alert pin */
		retval = ts->platform_data->hw_recov(wake);
		if (retval < 0) {
			if (retval == -ENOSYS) {
				dev_vdbg(ts->dev, "%s: no hw_recov wake code=%d function\n", __func__, wake);
			} else {
				dev_err(ts->dev,"%s: fail hw_recov(wake=%d) function r=%d\n", __func__, wake, retval);
				retval = -ENOSYS;
			}
		}
	}

	if (retval == -ENOSYS) {
		/*
		 * Wake the chip with bus traffic
		 * The first few reads should always fail because
		 * the part is not ready to respond,
		 * but the retries should succeed.
		 */
		/*
		 * Even though this is hardware-specific, it is done
		 * here because the board config file doesn't have
		 * access to the bus read routine
		 */
		retval = _cyttsp4_read_block_data(ts, CY_REG_BASE,
			sizeof(hst_mode), &hst_mode,
			ts->platform_data->addr[CY_TCH_ADDR_OFS],
			true);
		if (retval < 0) {
			/* device may not be ready even with the
			 * bus read retries so just go ahead and
			 * wait for the cmd rdy interrupt or timeout
			 */
			retval = 0;
		} else {
			/* IC is awake but still need to check for
			 * proper mode
			 */
		}
	} else
		retval = 0;


	/* Wait for cmd rdy interrupt to signal device wake */
	timeout = msecs_to_jiffies(0); //CY_HALF_SEC_TMO_MS);
	mutex_unlock(&ts->data_lock);
	uretval = wait_for_completion_interruptible_timeout(&ts->int_running, timeout);
	mutex_lock(&ts->data_lock);

	/* read registers even if wait ended with timeout */
	retval = _cyttsp4_read_block_data(ts,
		CY_REG_BASE, sizeof(hst_mode), &hst_mode,
		ts->platform_data->addr[CY_TCH_ADDR_OFS], true);

	/* TMA884 indicates bootloader mode by changing addr */
	if (retval < 0) {
		dev_err(ts->dev, "%s: failed to resume or in bootloader (r=%d)\n",	__func__, retval);
	}
	else {
#ifdef CY_USE_TMA400
		/* read rep stat register for bootloader status */
		
		retval = _cyttsp4_load_status_regs(ts);
		
		if (retval < 0) {
			dev_err(ts->dev, "%s: failed to access device on resume r=%d\n", __func__, retval);
			goto _cyttsp4_wakeup_exit;
		}
		rep_stat = ts->xy_mode[ts->si_ofs.rep_ofs + 1];
		if (IS_BOOTLOADERMODE(rep_stat)) {
			dev_err(ts->dev, "%s: device in bootloader mode on wakeup rep_stat=0x%02X\n", __func__, rep_stat);
			retval = -EIO;
			goto _cyttsp4_wakeup_exit;
		}
#endif /* --CY_USE_TMA400 */
		retval = _cyttsp4_handshake(ts, hst_mode);
		if (retval < 0) {
			dev_err(ts->dev, "%s: fail resume INT handshake (r=%d)\n", __func__, retval);
			/* continue; rely on handshake tmo */
			retval = 0;
		}
		_cyttsp4_change_state(ts, CY_ACTIVE_STATE);
	}

#ifdef CY_USE_TMA400
_cyttsp4_wakeup_exit:
#endif /* --CY_USE_TMA400 */
#endif
	return retval;
}

#if defined(CONFIG_PM) || defined(CONFIG_PM_SLEEP) ||	defined(CONFIG_HAS_EARLYSUSPEND)

#if defined(CONFIG_HAS_EARLYSUSPEND)
int cyttsp4_suspend(void *handle)
{
	struct cyttsp4 *ts = handle;

/*
#elif defined(CONFIG_PM_SLEEP)
static int cyttsp4_suspend(struct device *dev)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);
*/

#else
int cyttsp4_suspend(void *handle)
{
	struct cyttsp4 *ts = handle;

#endif

	int retval = 0;

	is_sleep_state = true;

#if defined(CY_USE_SOFT_SUSPEND_RESUME_MODE)	

	if (ts->test.cur_mode != CY_TEST_MODE_NORMAL_OP) {
		retval = -EBUSY;
		dev_err(ts->dev, "%s: Suspend Blocked while in test mode=%d\n", __func__, ts->test.cur_mode);
	} 
	else 
	{
		switch (ts->driver_state) {

		case CY_ACTIVE_STATE:

#if defined(CY_USE_FORCE_LOAD) || defined(CY_USE_INCLUDE_FBL)
			if (ts->waiting_for_fw) {
				retval = -EBUSY;
				dev_err(ts->dev, "%s: Suspend Blocked while waiting for fw load in %s state\n", __func__, cyttsp4_driver_state_string[ts->driver_state]);
				break;
			}
#endif
			dev_vdbg(ts->dev, "%s: Suspending...\n", __func__);

#ifdef CY_USE_WATCHDOG
			_cyttsp4_stop_wd_timer(ts);
#endif
			if (ts->irq_enabled) disable_irq(ts->irq);

			mutex_lock(&ts->data_lock);

			retval = _cyttsp4_enter_sleep(ts);
			if (retval < 0) {
				dev_err(ts->dev, "%s: fail enter sleep r=%d\n", __func__, retval);
			} 
			else 
			{
				_cyttsp4_change_state(ts, CY_SLEEP_STATE);
			}				
			
			mutex_unlock(&ts->data_lock);

            off_hw_setting();	
			break;
		case CY_SLEEP_STATE:
			dev_err(ts->dev, "%s: already in Sleep state\n", __func__);
			break;
		/*
		 * These states could be changing the device state
		 * Some of these states don't directly change device state
		 * but the next state could happen at any time and that
		 * state DOES modify the device state
		 * they must complete before allowing suspend.
		 */
		case CY_BL_STATE:
		case CY_CMD_STATE:
		case CY_SYSINFO_STATE:
		case CY_READY_STATE:
		case CY_TRANSFER_STATE:
			retval = -EBUSY;
			dev_err(ts->dev, "%s: Suspend Blocked while in %s state\n", __func__, cyttsp4_driver_state_string[ts->driver_state]);
			break;
		case CY_IDLE_STATE:
		case CY_INVALID_STATE:
		default:
			dev_err(ts->dev, "%s: Cannot enter suspend from %s state\n", __func__, cyttsp4_driver_state_string[ts->driver_state]);
			break;
		}
	}
#else
	if (ts->irq_enabled) disable_irq(ts->irq);
	off_hw_setting();
	_cyttsp4_change_state(ts, CY_IDLE_STATE);	
	ts->powered = false;
#endif

	_pantech_release_eventhub(ts);
	input_mt_destroy_slots(ts->input);

	ResumeCnt_T=Init_Resume_T;
	Pow_ON_T=0;

	return retval;
}
EXPORT_SYMBOL_GPL(cyttsp4_suspend);

#if defined(CONFIG_HAS_EARLYSUSPEND)
int cyttsp4_resume(void *handle)
{
	struct cyttsp4 *ts = handle;
#elif defined(CONFIG_PM_SLEEP)
static int cyttsp4_resume(struct device *dev)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);
#else
int cyttsp4_resume(void *handle)
{
	struct cyttsp4 *ts = handle;
#endif

	int retval = 0;
	//u8 touch_st=0;
	//u8 i=0;

	is_sleep_state = false;

	mutex_lock(&ts->data_lock);

#ifdef CY_USE_LEVEL_IRQ
	/* workaround level interrupt unmasking issue */
	if (ts->irq_enabled) {
		disable_irq_nosync(ts->irq);
		udelay(5);
		enable_irq(ts->irq);
	}
#endif

	switch (ts->driver_state) {

	case CY_SLEEP_STATE:
		
#if defined(CY_USE_SOFT_SUSPEND_RESUME_MODE)
		retval = _cyttsp4_wakeup(ts);
		if (retval < 0)
		{
			dev_err(ts->dev, "%s: wakeup fail r=%d\n", __func__, retval);
			_cyttsp4_pr_state(ts);

			if (ts->irq_enabled) enable_irq(ts->irq);
			_cyttsp4_queue_startup(ts, false);
			break;
		}
		        
		input_mt_init_slots(ts->input, CYTTSP_MAX_TOUCHNUM);
        if (ts->irq_enabled) enable_irq(ts->irq);		
        _cyttsp4_change_state(ts, CY_ACTIVE_STATE);

#ifdef CY_USE_WATCHDOG
		_cyttsp4_start_wd_timer(ts);
#endif
#endif
		break;


	case CY_IDLE_STATE:
#if defined(CY_USE_SOFT_SUSPEND_RESUME_MODE)		
#else
		init_hw_setting();

		if (ts->irq_enabled) enable_irq(ts->irq);

		retval = _cyttsp4_startup(ts);
		/* powered if no hard failure */
		if (retval < 0) {
			ts->powered = false;
			_cyttsp4_change_state(ts, CY_IDLE_STATE);
			if (ts->irq_enabled) disable_irq(ts->irq);		
			dev_err(ts->dev, "%s: startup fail at power on r=%d\n",	__func__, retval);
		} 
		else
		{
			ts->powered = true;
			input_mt_init_slots(ts->input, CYTTSP_MAX_TOUCHNUM);			
			_cyttsp4_change_state(ts, CY_ACTIVE_STATE);
#ifdef CY_USE_WATCHDOG
			_cyttsp4_start_wd_timer(ts);
#endif
		}			

		dev_info(ts->dev, "%s: Powered ON(%d) r=%d\n", __func__, (int)ts->powered, retval);		

		break;
#endif

	case CY_READY_STATE:
	case CY_ACTIVE_STATE:
	case CY_BL_STATE:
	case CY_SYSINFO_STATE:
	case CY_CMD_STATE:
	case CY_TRANSFER_STATE:
	case CY_INVALID_STATE:

	default:
		dev_err(ts->dev, "%s: Already in %s state\n", __func__,	cyttsp4_driver_state_string[ts->driver_state]);
		break;
	}
	
	mutex_unlock(&ts->data_lock);

	dev_vdbg(ts->dev, "%s: exit Resume r=%d\n", __func__, retval);

	Pow_ON_T=1;
	ResumeCnt_T=Init_Resume_T;
	dev_dbg(ts->dev, "enter xy_worker\n");

	return  retval;
}

EXPORT_SYMBOL_GPL(cyttsp4_resume);
#endif
#if !defined(CONFIG_HAS_EARLYSUSPEND) && defined(CONFIG_PM_SLEEP)
const struct dev_pm_ops cyttsp4_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(cyttsp4_suspend, cyttsp4_resume)
};
EXPORT_SYMBOL_GPL(cyttsp4_pm_ops);
#endif


#if defined(CONFIG_HAS_EARLYSUSPEND)
void cyttsp4_early_suspend(struct early_suspend *h)
{
	struct cyttsp4 *ts = container_of(h, struct cyttsp4, early_suspend);
	int retval = 0;

	is_sleep_state = true;
	dev_vdbg(ts->dev, "%s: EARLY SUSPEND ts=%p\n",
		__func__, ts);
	retval = cyttsp4_suspend(ts);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Early suspend failed with error code %d\n",
			__func__, retval);
	}
}
void cyttsp4_late_resume(struct early_suspend *h)
{
	struct cyttsp4 *ts = container_of(h, struct cyttsp4, early_suspend);
	int retval = 0;

	is_sleep_state = false;

	dev_vdbg(ts->dev, "%s: LATE RESUME ts=%p\n",
		__func__, ts);
	retval = cyttsp4_resume(ts);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Late resume failed with error code %d\n",
			__func__, retval);
	}
}
#endif

#ifdef CY_AUTO_LOAD_FW
#ifdef CY_CHECK_ADC
static int _cyttsp4_check_adc(int channel)
{
    int rslt;
    void *h;
    struct adc_chan_result acr;
    struct completion conv_completion_evt;
    struct cyttsp4 *ts = container_of(h, struct cyttsp4, early_suspend);

    // open channel
    rslt = adc_channel_open(channel, &h);
    if(rslt) {
        dev_err(ts->dev, "fail to open channel %d. rslt=%d\n",channel, rslt);
        return -EINVAL;
    }

    // start adc convert
    init_completion(&conv_completion_evt);
    rslt = adc_channel_request_conv(h, &conv_completion_evt);
    if(rslt) {
        dev_err(ts->dev, "fail to request channel %d. rslt=%d\n",channel, rslt);
        return -EINVAL;
    }

    // read adb
    wait_for_completion(&conv_completion_evt);
    rslt = adc_channel_read_result(h, &acr);
    if(rslt) {
        dev_err(ts->dev, "fail to read channel %d. rslt=%d\n",channel, rslt);
        return -EINVAL;
    }

    // close channel
    rslt = adc_channel_close(h);
    if(rslt) {
        dev_err(ts->dev, "fail to close channel %d. rslt=%d\n",channel, rslt);
        return -EINVAL;
    }

    // return adc measurement
    return acr.measurement;
}
#endif /* --CY_CHECK_ADC */

static int _cyttsp4_boot_loader(struct cyttsp4 *ts, bool *upgraded)
{
	int retval = 0;
	int i = 0;
	u32 fw_vers_platform = 0;
	u32 fw_vers_img = 0;
	u32 fw_revctrl_platform_h = 0;
	u32 fw_revctrl_platform_l = 0;
	u32 fw_revctrl_img_h = 0;
	u32 fw_revctrl_img_l = 0;
	bool new_fw_vers = false;
	bool new_fw_revctrl = false;
	bool new_vers = false;
	
#ifdef CY_CHECK_ADC
    int adc_measurement;
    adc_measurement = _cyttsp4_check_adc(0x84);
    dbg(KERN_INFO "adc_measurement=%d\n",adc_measurement);
#endif /* --CY_CHECK_ADC */

	dbg_func_in();

	*upgraded = false;
	if (ts->driver_state == CY_SLEEP_STATE) {
		dev_err(ts->dev, "%s: cannot load firmware in sleep state\n", __func__);
		retval = 0;
	} else if ((ts->platform_data->fw->ver == NULL) ||(ts->platform_data->fw->img == NULL)) {
		dev_err(ts->dev, "%s: empty version list or no image\n", __func__);
		retval = 0;
	} else if (ts->platform_data->fw->vsize != CY_BL_VERS_SIZE) {
		dev_err(ts->dev, "%s: bad fw version list size=%d\n", __func__, ts->platform_data->fw->vsize);
		retval = 0;
	} else {

		/* automatically update firmware if new version detected */
		fw_vers_img = (ts->sysinfo_ptr.cydata->fw_ver_major * 256);
		fw_vers_img += ts->sysinfo_ptr.cydata->fw_ver_minor;
		fw_vers_platform = ts->platform_data->fw->ver[2] * 256;		// from cyttsp4_img.h
		fw_vers_platform += ts->platform_data->fw->ver[3];

#ifdef CY_ANY_DIFF_NEW_VER
		if (fw_vers_platform != fw_vers_img)
			new_fw_vers = true;
		else
			new_fw_vers = false;
#else
		if (fw_vers_platform > fw_vers_img)
			new_fw_vers = true;
		else
			new_fw_vers = false;
#endif
		dbg("%s: fw_vers_platform=%04X fw_vers_img=%04X\n", __func__, fw_vers_platform, fw_vers_img);		

		fw_revctrl_img_h = ts->sysinfo_ptr.cydata->revctrl[0];
		fw_revctrl_img_l = ts->sysinfo_ptr.cydata->revctrl[4];
		fw_revctrl_platform_h = ts->platform_data->fw->ver[4];
		fw_revctrl_platform_l = ts->platform_data->fw->ver[8];		

		for (i = 1; i < 4; i++) {
			fw_revctrl_img_h = (fw_revctrl_img_h * 256) + ts->sysinfo_ptr.cydata->revctrl[0+i];
			fw_revctrl_img_l = (fw_revctrl_img_l * 256) + ts->sysinfo_ptr.cydata->revctrl[4+i];
			fw_revctrl_platform_h = (fw_revctrl_platform_h * 256) + ts->platform_data->fw->ver[4+i];
			fw_revctrl_platform_l = (fw_revctrl_platform_l * 256) + ts->platform_data->fw->ver[8+i];
		}

#ifdef CY_ANY_DIFF_NEW_VER
		if (fw_revctrl_platform_h != fw_revctrl_img_h)
			new_fw_revctrl = true;
		else if (fw_revctrl_platform_h == fw_revctrl_img_h) {
			if (fw_revctrl_platform_l != fw_revctrl_img_l)
				new_fw_revctrl = true;
			else
				new_fw_revctrl = false;
		} else
			new_fw_revctrl = false;
#else
		if (fw_revctrl_platform_h > fw_revctrl_img_h)
			new_fw_revctrl = true;
		else if (fw_revctrl_platform_h == fw_revctrl_img_h) {
			if (fw_revctrl_platform_l > fw_revctrl_img_l)
				new_fw_revctrl = true;
			else
				new_fw_revctrl = false;
		} else
			new_fw_revctrl = false;
#endif
		
		if (new_fw_vers || new_fw_revctrl)
			new_vers = true;

		if ( pantech_fw_download_flag == false ) {
			pantech_fw_download_flag = true;
		}
		else {
			new_vers = false;
		}

		dbg("%s: fw_revctrl_platform_h=%08X fw_revctrl_img_h=%08X\n", __func__, fw_revctrl_platform_h, fw_revctrl_img_h);
		dbg("%s: fw_revctrl_platform_l=%08X fw_revctrl_img_l=%08X\n", __func__, fw_revctrl_platform_l, fw_revctrl_img_l);
		dbg("%s: new_fw_vers=%d new_fw_revctrl=%d new_vers=%d\n", __func__, (int)new_fw_vers, (int)new_fw_revctrl, (int)new_vers);

		if (new_vers) {
			dev_info(ts->dev, "%s: upgrading firmware...\n", __func__);
			retval = _cyttsp4_load_app(ts, ts->platform_data->fw->img, ts->platform_data->fw->size);
			
			if (retval < 0) {
				dev_err(ts->dev, "%s: communication fail on load fw r=%d\n", __func__, retval);
				_cyttsp4_change_state(ts, CY_IDLE_STATE);
				retval = -EIO;
			} 
			else {
				*upgraded = true;
				// p11309_new_feature
				cyttsp4_need_manual_calibration = true;
			}
		} 
		else 
		{
			dev_vdbg(ts->dev, "%s: No auto firmware upgrade required\n", __func__);
		}
	}

	dbg("[---] %s, %d\n", __func__, retval);

	return retval;
}
#endif /* --CY_AUTO_LOAD_FW */

static ssize_t cyttsp4_ic_ver_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);

	return sprintf(buf, "%s: 0x%02X 0x%02X\n%s: 0x%02X\n%s: 0x%02X\n%s: "
		"0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n",
		"TrueTouch Product ID",
		ts->sysinfo_ptr.cydata->ttpidh,
		ts->sysinfo_ptr.cydata->ttpidl,
		"Firmware Major Version", ts->sysinfo_ptr.cydata->fw_ver_major,
		"Firmware Minor Version", ts->sysinfo_ptr.cydata->fw_ver_minor,
		"Revision Control Number", ts->sysinfo_ptr.cydata->revctrl[0],
		ts->sysinfo_ptr.cydata->revctrl[1],
		ts->sysinfo_ptr.cydata->revctrl[2],
		ts->sysinfo_ptr.cydata->revctrl[3],
		ts->sysinfo_ptr.cydata->revctrl[4],
		ts->sysinfo_ptr.cydata->revctrl[5],
		ts->sysinfo_ptr.cydata->revctrl[6],
		ts->sysinfo_ptr.cydata->revctrl[7]
	);
}
static DEVICE_ATTR(ic_ver, S_IRUGO, cyttsp4_ic_ver_show, NULL);

/* Driver version */
static ssize_t cyttsp4_drv_ver_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);

	return snprintf(buf, CY_MAX_PRBUF_SIZE,
		"Driver: %s\nVersion: %s\nDate: %s\n",
		ts->input->name, CY_DRIVER_VERSION, CY_DRIVER_DATE);
}
static DEVICE_ATTR(drv_ver, S_IRUGO, cyttsp4_drv_ver_show, NULL);


/* Driver status */
static ssize_t cyttsp4_drv_stat_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);

	return snprintf(buf, CY_MAX_PRBUF_SIZE,
		"Driver state is %s\n",
		cyttsp4_driver_state_string[ts->driver_state]);
}
static DEVICE_ATTR(drv_stat, S_IRUGO, cyttsp4_drv_stat_show, NULL);

#ifdef CY_USE_INCLUDE_FBL
static ssize_t cyttsp_ic_irq_stat_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int retval;
	struct cyttsp4 *ts = dev_get_drvdata(dev);

	if (ts->platform_data->irq_stat) {
		retval = ts->platform_data->irq_stat();
		switch (retval) {
		case 0:
			return snprintf(buf, CY_MAX_PRBUF_SIZE,
				"Interrupt line is LOW.\n");
		case 1:
			return snprintf(buf, CY_MAX_PRBUF_SIZE,
				"Interrupt line is HIGH.\n");
		default:
			return snprintf(buf, CY_MAX_PRBUF_SIZE,
				"Function irq_stat() returned %d.\n", retval);
		}
	} else {
		return snprintf(buf, CY_MAX_PRBUF_SIZE,
			"Function irq_stat() undefined.\n");
	}
}
static DEVICE_ATTR(hw_irqstat, S_IRUSR | S_IWUSR,
	cyttsp_ic_irq_stat_show, NULL);
#endif /* --CY_USE_INCLUDE_FBL */

#ifdef CY_USE_INCLUDE_FBL
/* Disable Driver IRQ */
static ssize_t cyttsp4_drv_irq_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	static const char *fmt_disabled = "Driver interrupt is DISABLED\n";
	static const char *fmt_enabled = "Driver interrupt is ENABLED\n";
	struct cyttsp4 *ts = dev_get_drvdata(dev);

	if (ts->irq_enabled == false)
		return snprintf(buf, strlen(fmt_disabled)+1, fmt_disabled);
	else
		return snprintf(buf, strlen(fmt_enabled)+1, fmt_enabled);
}
static ssize_t cyttsp4_drv_irq_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int retval = 0;
	struct cyttsp4 *ts = dev_get_drvdata(dev);
	unsigned long value;

	mutex_lock(&(ts->data_lock));

	if (size > 2) {
		dev_err(ts->dev,
			"%s: Err, data too large\n", __func__);
		retval = -EOVERFLOW;
		goto cyttsp4_drv_irq_store_error_exit;
	}

	retval = strict_strtoul(buf, 10, &value);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Failed to convert value\n", __func__);
		goto cyttsp4_drv_irq_store_error_exit;
	}

	if (ts->irq_enabled == false) {
		if (value == 1) {
			/* Enable IRQ */
			enable_irq(ts->irq);
			dev_info(ts->dev,
			"%s: Driver IRQ now enabled\n", __func__);
			ts->irq_enabled = true;
		} else {
			dev_info(ts->dev,
			"%s: Driver IRQ already disabled\n", __func__);
		}
	} else {
		if (value == 0) {
			/* Disable IRQ */
			disable_irq_nosync(ts->irq);
			dev_info(ts->dev,
			"%s: Driver IRQ now disabled\n", __func__);
			ts->irq_enabled = false;
		} else {
			dev_info(ts->dev,
			"%s: Driver IRQ already enabled\n", __func__);
		}
	}

	retval = size;

cyttsp4_drv_irq_store_error_exit:
	mutex_unlock(&(ts->data_lock));
	return retval;
}
static DEVICE_ATTR(drv_irq, S_IRUSR | S_IWUSR,
	cyttsp4_drv_irq_show, cyttsp4_drv_irq_store);
#endif /* --CY_USE_INCLUDE_FBL */

#ifdef CY_USE_INCLUDE_FBL
/* Driver debugging */
static ssize_t cyttsp4_drv_debug_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);

#ifdef CY_USE_DEBUG_TOOLS
	return snprintf(buf, CY_MAX_PRBUF_SIZE,
		"Debug Setting: %u hover=%d flip=%d inv-x=%d inv-y=%d\n",
		ts->bus_ops->tsdebug,
		(int)(!!(ts->flags & CY_FLAG_HOVER)),
		(int)(!!(ts->flags & CY_FLAG_FLIP)),
		(int)(!!(ts->flags & CY_FLAG_INV_X)),
		(int)(!!(ts->flags & CY_FLAG_INV_Y)));
#else
	return snprintf(buf, CY_MAX_PRBUF_SIZE,
		"Debug Setting: %u\n", ts->bus_ops->tsdebug);
#endif /* --CY_USE_DEBUG_TOOLS */
}

static ssize_t cyttsp4_drv_debug_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);
	int retval = 0;
	unsigned long value = 0;

	retval = strict_strtoul(buf, 10, &value);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Failed to convert value\n", __func__);
		goto cyttsp4_drv_debug_store_exit;
	}

	switch (value) {
	case CY_DBG_LVL_0:
	case CY_DBG_LVL_1:
	case CY_DBG_LVL_2:
	case CY_DBG_LVL_3:
		dev_info(ts->dev, "%s: Debug setting=%d\n", __func__, (int)value);
		ts->bus_ops->tsdebug = value;
		break;
#ifdef CY_USE_DEBUG_TOOLS
	case CY_DBG_SUSPEND:
		dev_info(ts->dev, "%s: SUSPEND (ts=%p)\n", __func__, ts);
#if defined(CONFIG_HAS_EARLYSUSPEND)
		cyttsp4_early_suspend(&ts->early_suspend);
#elif defined(CONFIG_PM_SLEEP)
		cyttsp4_suspend(ts->dev);
#elif defined(CONFIG_PM) || defined(CONFIG_HAS_EARLYSUSPEND)
		cyttsp4_suspend(ts);
#endif
		break;
	case CY_DBG_RESUME:
		dev_info(ts->dev, "%s: RESUME (ts=%p)\n", __func__, ts);
#if defined(CONFIG_HAS_EARLYSUSPEND)
		cyttsp4_late_resume(&ts->early_suspend);
#elif defined(CONFIG_PM_SLEEP)
		cyttsp4_resume(ts->dev);
#elif defined(CONFIG_PM)
		cyttsp4_resume(ts);
#endif
		break;
	case CY_DBG_SOFT_RESET:
		dev_info(ts->dev, "%s: SOFT RESET (ts=%p)\n", __func__, ts);
		retval = _cyttsp4_soft_reset(ts);
		dev_info(ts->dev, "%s: return from _cyttsp4_soft_reset r=%d\n", __func__, retval);
		break;
	case CY_DBG_RESET:
		dev_info(ts->dev, "%s: RESET (ts=%p)\n", __func__, ts);
		mutex_lock(&ts->data_lock);
		retval = _cyttsp4_startup(ts);
		mutex_unlock(&ts->data_lock);
		dev_info(ts->dev, "%s: return from _cyttsp4_startup test r=%d\n", __func__, retval);
		break;
#ifdef CY_USE_DEV_DEBUG_TOOLS
	case CY_DBG_PUT_ALL_PARAMS:
#ifdef CY_USE_TMA400
	{
		enum cyttsp4_ic_ebid ebid = CY_TCH_PARM_EBID;
		u8 ic_crc[2];

		dev_info(ts->dev, "%s: PUT_ALL_PARAMS (ts=%p)\n", __func__, ts);
		mutex_lock(&ts->data_lock);
		memset(ic_crc, 0, sizeof(ic_crc));
		retval = _cyttsp4_set_mode(ts, CY_CONFIG_MODE);
		if (retval < 0) {
			dev_err(ts->dev, "%s: Fail switch to config mode r=%d\n", __func__, retval);
		} else {
			retval = _cyttsp4_put_all_params_tma400(ts);
			if (retval < 0) {
				dev_err(ts->dev, "%s: fail put all params r=%d\n", __func__, retval);
			} else {
				retval = _cyttsp4_calc_ic_crc_tma400(ts, ebid, &ic_crc[0], &ic_crc[1], true);
				if (retval < 0) { 
					dev_err(ts->dev, "%s: fail verify params r=%d\n", __func__, retval);
				}
				_cyttsp4_pr_buf(ts, ic_crc, sizeof(ic_crc), "verify_params_ic_crc");
			}
			retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
			if (retval < 0) {
				dev_err(ts->dev, "%s: Fail switch op mode r=%d\n", __func__, retval);
			}
		}
		mutex_unlock(&ts->data_lock);
		break;
	}
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
		dev_err(ts->dev, "%s: INVALID debug setting=%d\n", __func__, (int)value);
		break;
#endif /* --CY_USE_TMA884 */
	case CY_DBG_CHECK_MDDATA:
#ifdef CY_USE_TMA400
#ifdef CY_USE_REG_ACCESS
	{
		bool updated = false;

		dev_info(ts->dev, "%s: CHECK MDDATA ts=%p\n", __func__, ts); 
		mutex_lock(&ts->data_lock);
		retval = _cyttsp4_set_mode(ts, CY_CONFIG_MODE);
		if (retval < 0) {
			dev_err(ts->dev, "%s: Fail switch config mode r=%d\n", __func__, retval);
		} else {
			dev_vdbg(ts->dev, "%s: call check_mddata ts=%p\n", __func__, ts);
			retval = _cyttsp4_check_mddata_tma400(ts, &updated);
			if (retval < 0) {
				dev_err(ts->dev, "%s: Fail Check mddata r=%d\n", __func__, retval);
			}
			dev_vdbg(ts->dev, "%s: mddata updated=%s\n", __func__, updated ? "true" : "false");
			retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
			if (retval < 0) {
				dev_err(ts->dev, "%s: Fail switch operate mode r=%d\n", __func__, retval);
			}
		}
		mutex_unlock(&ts->data_lock);
		break;
	}
#endif /* --CY_USE_REG_ACCESS */
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
		dev_err(ts->dev, "%s: INVALID debug setting=%d\n", __func__, (int)value);
		break;
#endif /* --CY_USE_TMA884 */
	case CY_DBG_GET_MDDATA:
#ifdef CY_USE_TMA400
#ifdef CY_USE_REG_ACCESS
	{
		/* to use this command first use the set rw_regid
		 * to the ebid of interest 1:MDATA 2:DDATA
		 */
		enum cyttsp4_ic_ebid ebid = ts->rw_regid;
		u8 *pdata = NULL;

		dev_info(ts->dev, "%s: GET IC MDDATA=%d (ts=%p)\n", __func__, ebid, ts);
		mutex_lock(&ts->data_lock);
		retval = _cyttsp4_set_mode(ts, CY_CONFIG_MODE);
		if (retval < 0) { 
			dev_err(ts->dev, "%s: Fail switch config mode r=%d\n", __func__, retval);
		} else {
			pdata = kzalloc(ts->ebid_row_size, GFP_KERNEL);
			if (pdata == NULL) {
				dev_err(ts->dev, "%s: Fail allocate block buffer\n", __func__);
				retval = -ENOMEM;
			} else {
				retval = _cyttsp4_get_ebid_data_tma400(ts, ebid, 0, pdata);
				if (retval < 0) {
					dev_err(ts->dev, "%s: Fail get touch ebid=%d data at row=0 r=%d\n", __func__, ebid, retval);
				}
				dev_vdbg(ts->dev, "%s: ebid=%d row=0 data:\n", __func__, ebid);
				_cyttsp4_pr_buf(ts, pdata, ts->ebid_row_size, "ebid_data");
				retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
				if (retval < 0) {
					dev_err(ts->dev, "%s: Fail switch operate mode r=%d\n", __func__, retval);
				}
			}
		}
		if (pdata != NULL)
			kfree(pdata);

		retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
		mutex_unlock(&ts->data_lock);
		break;
	}
#endif /* --CY_USE_REG_ACCESS */
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
		dev_err(ts->dev, "%s: INVALID debug setting=%d\n", __func__, (int)value);
		break;
#endif /* --CY_USE_TMA884 */
	case CY_DBG_GET_IC_TCH_CRC:
#ifdef CY_USE_TMA400
	{
		u8 ic_crc[2];

		memset(ic_crc, 0, sizeof(ic_crc));
		dev_info(ts->dev, "%s: GET TOUCH CRC (ts=%p)\n", __func__, ts);
		mutex_lock(&ts->data_lock);
		retval = _cyttsp4_get_ic_crc(ts, CY_TCH_PARM_EBID, &ic_crc[0], &ic_crc[1]);
		if (retval < 0) { 
			dev_err(ts->dev, "%s: Fail read ic crc r=%d\n", __func__, retval);
		}
		_cyttsp4_pr_buf(ts, ic_crc, sizeof(ic_crc), "read_ic_crc");
		mutex_unlock(&ts->data_lock);
		break;
	}
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
		dev_err(ts->dev, "%s: INVALID debug setting=%d\n", __func__, (int)value);
		break;
#endif /* --CY_USE_TMA884 */
	case CY_DBG_GET_IC_TCH_ROW:
#ifdef CY_USE_TMA400
#ifdef CY_USE_REG_ACCESS
	{
		/* to use this command first use the set rw_regid
		 * to the row of interest
		 */
		u8 *pdata = NULL;

		dev_info(ts->dev, "%s: GET TOUCH BLOCK ROW=%d (ts=%p)\n", __func__, ts->rw_regid, ts);
		mutex_lock(&ts->data_lock);
		retval = _cyttsp4_set_mode(ts, CY_CONFIG_MODE);
		if (retval < 0) {
			dev_err(ts->dev, "%s: Fail switch config mode r=%d\n", __func__, retval);
			goto CY_DBG_GET_IC_TCH_ROW_exit;
		}
		pdata = kzalloc(ts->ebid_row_size, GFP_KERNEL);
		if (pdata == NULL) {
			dev_err(ts->dev, "%s: Fail allocate block buffer\n", __func__);
			retval = -ENOMEM;
			goto CY_DBG_GET_IC_TCH_ROW_exit;
		}
		retval = _cyttsp4_get_ebid_data_tma400(ts, CY_TCH_PARM_EBID,
			ts->rw_regid, pdata);
		if (retval < 0) {
			dev_err(ts->dev, "%s: Fail get touch ebid data at row=%d r=%d\n", __func__, ts->rw_regid, retval);
		}
		dev_vdbg(ts->dev, "%s: tch ebid row=%d data:\n", __func__, ts->rw_regid);
		_cyttsp4_pr_buf(ts, pdata, ts->ebid_row_size, "ebid_data");

CY_DBG_GET_IC_TCH_ROW_exit:
		if (pdata != NULL)
			kfree(pdata);

		retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
		mutex_unlock(&ts->data_lock);
		break;
	}
#endif /* --CY_USE_REG_ACCESS */
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
		dev_err(ts->dev, "%s: INVALID debug setting=%d\n", __func__, (int)value);
		break;
#endif /* --CY_USE_TMA884 */
	case CY_DBG_READ_IC_TCH_CRC:
#ifdef CY_USE_TMA400
#ifdef CY_USE_REG_ACCESS
	{
		/* to use this command first use the set rw_regid
		 * to the row of interest
		 */
		u8 *pdata = NULL;
		size_t location = 0;
		size_t ofs = 0;
		size_t row = 0;

		dev_info(ts->dev, "%s: READ TOUCH BLOCK CRC (ts=%p)\n", __func__, ts);
		mutex_lock(&ts->data_lock);
		_cyttsp4_change_state(ts, CY_READY_STATE);
		retval = _cyttsp4_set_mode(ts, CY_CONFIG_MODE);
		if (retval < 0) {
			dev_err(ts->dev, "%s: Fail switch config mode r=%d\n", __func__, retval);
			goto CY_DBG_READ_IC_TCH_CRC_exit;
		}
		pdata = kzalloc(ts->ebid_row_size, GFP_KERNEL);
		if (pdata == NULL) {
			dev_err(ts->dev, "%s: Fail allocate block buffer\n", __func__);
			retval = -ENOMEM;
			goto CY_DBG_READ_IC_TCH_CRC_exit;
		}

		retval = _cyttsp4_get_ebid_data_tma400(ts, CY_TCH_PARM_EBID,
			row, pdata);
		if (retval < 0) {
			dev_err(ts->dev, "%s: Fail get touch ebid data at row=%d r=%d\n", __func__, row, retval);
		}
		dev_vdbg(ts->dev, "%s: tch ebid row=%d data:\n", __func__, 0);
		_cyttsp4_pr_buf(ts, pdata, ts->ebid_row_size, "ebid_data");
		location = (pdata[3] * 256) + pdata[2];
		row = location / ts->ebid_row_size;
		ofs = location % ts->ebid_row_size;
		memset(pdata, 0, ts->ebid_row_size);
		dev_vdbg(ts->dev, "%s: tch ebid crc_loc=%08X row=%d ofs=%d:\n", __func__, location, row, ofs);
		retval = _cyttsp4_get_ebid_data_tma400(ts, CY_TCH_PARM_EBID, row, pdata);

		if (retval < 0) {
			dev_err(ts->dev, "%s: Fail get touch ebid data at row=%d r=%d\n", __func__, row, retval);
		}
		dev_vdbg(ts->dev, "%s: tch ebid row=%d data:\n", __func__, row);
		_cyttsp4_pr_buf(ts, pdata, ts->ebid_row_size, "ebid_data");
		_cyttsp4_pr_buf(ts, &pdata[ofs], 4, "crc_data");
		dev_vdbg(ts->dev, "%s: tch ebid crc=%02X %02X\n", __func__, pdata[ofs], pdata[ofs+1]);

CY_DBG_READ_IC_TCH_CRC_exit:
		if (pdata != NULL)
			kfree(pdata);

		retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
		if (retval < 0) {
			dev_err(ts->dev, "%s: Fail switch operate mode r=%d\n", __func__, retval);
		}

		mutex_unlock(&ts->data_lock);
		break;
	}
#endif /* --CY_USE_REG_ACCESS */
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
		dev_err(ts->dev, "%s: INVALID debug setting=%d\n", __func__, (int)value);
		break;
#endif /* --CY_USE_TMA884 */
	case CY_DBG_CALC_IC_TCH_CRC:
#ifdef CY_USE_TMA400
	{
		u8 ic_crc[2];

		memset(ic_crc, 0, sizeof(ic_crc));
		dev_info(ts->dev, "%s: CALC IC TOUCH CRC (ts=%p)\n", __func__, ts);
		mutex_lock(&ts->data_lock);
		retval = _cyttsp4_set_mode(ts, CY_CONFIG_MODE);
		if (retval < 0) { 
			dev_err(ts->dev, "%s: Fail switch to config mode r=%d\n", __func__, retval);
		} else {
			retval = _cyttsp4_calc_ic_crc_tma400(ts, CY_TCH_PARM_EBID, &ic_crc[0], &ic_crc[1], true);
			if (retval < 0) {
				dev_err(ts->dev, "%s: Fail read ic crc r=%d\n", __func__, retval);
			}
			_cyttsp4_pr_buf(ts, ic_crc, sizeof(ic_crc), "calc_ic_crc_tma400");
			retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
			if (retval < 0) {
				dev_err(ts->dev, "%s: Fail switch to operational mode r=%d\n", __func__, retval);
			}
		}
		mutex_unlock(&ts->data_lock);
		break;
	}
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
		dev_err(ts->dev, "%s: INVALID debug setting=%d\n", __func__, (int)value);
		break;
#endif /* --CY_USE_TMA884 */
	case CY_DBG_READ_TABLE_TCH_CRC:
#ifdef CY_USE_TMA400
	{
		u8 *ptable = NULL;
		u8 ic_crc[2];

		memset(ic_crc, 0, sizeof(ic_crc));
		dev_info(ts->dev, "%s: GET TABLE TOUCH CRC (ts=%p)\n", __func__, ts);
		mutex_lock(&ts->data_lock);

		if (ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL] == NULL)
			dev_err(ts->dev, "%s: NULL param values table\n", __func__);
		else if (ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL]->data == NULL)
			dev_err(ts->dev, "%s: NULL param values table data\n", __func__);
		else if (ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL]->size == 0)
			dev_err(ts->dev, "%s: param values table size is 0\n", __func__);
		else {
			ptable = (u8 *)ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL]->data;
			_cyttsp_read_table_crc(ts, ptable, &ic_crc[0], &ic_crc[1]);
			_cyttsp4_pr_buf(ts, ic_crc, sizeof(ic_crc), "read_table_crc_400");
		}

		mutex_unlock(&ts->data_lock);
		break;
	}
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
		dev_err(ts->dev, "%s: INVALID debug setting=%d\n", __func__, (int)value);
		break;
#endif /* --CY_USE_TMA884 */
	case CY_DBG_CALC_TABLE_TCH_CRC:
#ifdef CY_USE_TMA400
	{
		u8 ic_crc[2];
		u8 *pdata = NULL;
		size_t table_size = 0;

		memset(ic_crc, 0, sizeof(ic_crc));
		dev_info(ts->dev, "%s: CALC TABLE TOUCH CRC (ts=%p)\n", __func__, ts);
		mutex_lock(&ts->data_lock);
		retval = _cyttsp4_set_mode(ts, CY_CONFIG_MODE);
		if (retval < 0) {
			dev_err(ts->dev, "%s: Fail switch to config mode r=%d\n", __func__, retval);
		} else {
			if (ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL] == NULL)
				dev_err(ts->dev, "%s: NULL param values table\n", __func__);
			else if (ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL]->data == NULL)
				dev_err(ts->dev, "%s: NULL param values table data\n", __func__);
			else if (ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL]->size == 0)
				dev_err(ts->dev, "%s: param values table size is 0\n", __func__);
			else {
				pdata = (u8 *)ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL]->data;
				table_size = ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL]->size;
				table_size -= 2;
				dev_vdbg(ts->dev, "%s: calc table size=%d\n", __func__, table_size);
				_cyttsp4_calc_crc(ts, pdata, table_size, &ic_crc[0], &ic_crc[1]);
				_cyttsp4_pr_buf(ts, ic_crc, sizeof(ic_crc), "calc_table_crc_400");
				dev_vdbg(ts->dev, "%s: calc table size=%d\n", __func__, table_size);
				retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE); 
				if (retval < 0) {
					dev_err(ts->dev, "%s: Fail switch to operational mode r=%d\n", __func__, retval);
				}
			}
		}
		mutex_unlock(&ts->data_lock);
		break;
	}
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
		dev_err(ts->dev, "%s: INVALID debug setting=%d\n", __func__, (int)value);
		break;
#endif /* --CY_USE_TMA884 */
#ifdef CY_USE_TMA400
	case CY_DBG_PUT_IC_TCH_ROW:
	{
		enum cyttsp4_ic_ebid ebid = CY_TCH_PARM_EBID;
		size_t row_id = 0;
		size_t num_rows = 0;
		size_t table_size = 0;
		size_t residue = 0;
		size_t ndata = 0;
		int i = 0;
		bool match = false;
		u8 *pdata = NULL;
		u8 *ptable = NULL;

		mutex_lock(&ts->data_lock);
		retval = _cyttsp4_set_mode(ts, CY_CONFIG_MODE);
		if (retval < 0) {
			dev_err(ts->dev, "%s: Fail switch to config mode r=%d\n", __func__, retval);
			goto CY_DBG_PUT_IC_TCH_ROW_exit;
		}
		pdata = kzalloc(ts->ebid_row_size, GFP_KERNEL);
		if (pdata == NULL) {
			dev_err(ts->dev, "%s: Alloc error ebid=%d\n", __func__, ebid);
			retval = -ENOMEM;
		} else if (ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL] == NULL)
			dev_err(ts->dev, "%s: NULL param values table\n", __func__); 
		else if (ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL]->data == NULL)
			dev_err(ts->dev, "%s: NULL param values table data\n", __func__);
		else if (ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL]->size == 0)
			dev_err(ts->dev, "%s: param values table size is 0\n", __func__);
		else {
			ptable = (u8 *)ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL]->data;
			table_size = ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL]->size;
			row_id = ts->rw_regid;
			num_rows = table_size / ts->ebid_row_size;
			residue = table_size % ts->ebid_row_size;

			if (residue)
				num_rows++;

			dev_vdbg(ts->dev, "%s: num_rows=%d row_size=%d table_size=%d residue=%d\n", 
				__func__, num_rows, ts->ebid_row_size, table_size,residue);

			if (row_id < num_rows) {
				ptable += row_id * ts->ebid_row_size;
				if (row_id < num_rows - 1)
					ndata = ts->ebid_row_size;
				else
					ndata = residue;
				memcpy(pdata, ptable, ndata);
				dev_vdbg(ts->dev, "%s: row=%d pdata=%p ndata=%d\n", __func__, ts->rw_regid, pdata, ndata);
				_cyttsp4_pr_buf(ts, pdata, ndata, "ebid_data");
				retval = _cyttsp4_put_ebid_data_tma400(ts, ebid, row_id, pdata);
				if (retval < 0) {
					dev_err(ts->dev, "%s: Fail put row=%d r=%d\n", __func__, row_id, retval);
					break;
				}
				/* read back and compare to table */
				dev_vdbg(ts->dev, "%s: read back and compare to table\n", __func__);
				retval = _cyttsp4_get_ebid_data_tma400(ts, ebid, ts->rw_regid, pdata);
				if (retval < 0) {
					dev_err(ts->dev, "%s: Fail get row to cmp r=%d\n", __func__, retval);
					break;
				}
				_cyttsp4_pr_buf(ts, pdata, ndata, "read_back");
				for (i = 0, match = true; i < ndata && match; i++) {
					if (*ptable != *pdata) {
						dev_vdbg(ts->dev, "%s: read back err, table[%d]=%02X, pdata[%d]=%02X\n", __func__, i, ptable[i], i, pdata[i]);
						match = false;
					}
					ptable++;
					pdata++;
				}
				if (match) {
					dev_vdbg(ts->dev, "%s: row=%d matches after put ebid=%d row\n", __func__, row_id, ebid);
				} else {
					dev_err(ts->dev, "%s: row=%d fails match after put ebid=%d row\n", __func__, row_id, ebid);
				}
			} else {
				dev_err(ts->dev, "%s: row_id=ts->rw_regid=%d > num_rows=%d\n", __func__, row_id, num_rows);
			}
		}
		retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
		if (retval < 0) {
			dev_err(ts->dev, "%s: Fail switch to operational mode r=%d\n", __func__, retval);
		}

CY_DBG_PUT_IC_TCH_ROW_exit:
		mutex_unlock(&ts->data_lock);
	}
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
		dev_err(ts->dev, "%s: INVALID debug setting=%d\n", __func__, (int)value);
		break;
#endif /* --CY_USE_TMA884 */
#endif /* --CY_USE_DEV_DEBUG_TOOLS */
#endif /* --CY_USE_DEBUG_TOOLS */
	default:
		dev_err(ts->dev, "%s: INVALID debug setting=%d\n", __func__, (int)value);
		break;
	}

	retval = size;

cyttsp4_drv_debug_store_exit:
	return retval;
}

static DEVICE_ATTR(drv_debug, S_IWUSR | S_IRUGO, cyttsp4_drv_debug_show, cyttsp4_drv_debug_store);
#endif /* --CY_USE_INCLUDE_FBL */

#ifdef CY_USE_REG_ACCESS
static ssize_t cyttsp_drv_rw_regid_show(struct device *dev,	struct device_attribute *attr, char *buf)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);
	return snprintf(buf, CY_MAX_PRBUF_SIZE, "Current Read/Write Regid=%02X(%d)\n", ts->rw_regid, ts->rw_regid);
}

static ssize_t cyttsp_drv_rw_regid_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);
	int retval = 0;
	unsigned long value;

	mutex_lock(&ts->data_lock);
	retval = strict_strtoul(buf, 10, &value);
	if (retval < 0) {
		retval = strict_strtoul(buf, 16, &value);
		if (retval < 0) {
			dev_err(ts->dev, "%s: Failed to convert value\n", __func__);
			goto cyttsp_drv_rw_regid_store_exit;
		}
	}

	if (value > CY_RW_REGID_MAX) {
		ts->rw_regid = CY_RW_REGID_MAX;
		dev_err(ts->dev, "%s: Invalid Read/Write Regid; set to max=%d\n", __func__, ts->rw_regid);
	} else
		ts->rw_regid = value;

	retval = size;

cyttsp_drv_rw_regid_store_exit:
	mutex_unlock(&ts->data_lock);
	return retval;
}

static DEVICE_ATTR(drv_rw_regid, S_IWUSR | S_IRUGO, cyttsp_drv_rw_regid_show, cyttsp_drv_rw_regid_store);


static ssize_t cyttsp_drv_rw_reg_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);
	int retval;
	u8 reg_data;

	retval = _cyttsp4_read_block_data(ts, ts->rw_regid, sizeof(reg_data), &reg_data, ts->platform_data->addr[CY_TCH_ADDR_OFS], true);

	if (retval < 0)
		return snprintf(buf, CY_MAX_PRBUF_SIZE, "Read/Write Regid(%02X(%d) Failed\n", ts->rw_regid, ts->rw_regid);
	else
		return snprintf(buf, CY_MAX_PRBUF_SIZE, "Read/Write Regid=%02X(%d) Data=%02X(%d)\n", ts->rw_regid, ts->rw_regid, reg_data, reg_data);
}

static ssize_t cyttsp_drv_rw_reg_data_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);
	int retval = 0;
	unsigned long value;
	u8 reg_data = 0;

	retval = strict_strtoul(buf, 10, &value);
	if (retval < 0) {
		retval = strict_strtoul(buf, 16, &value);
		if (retval < 0) {
			dev_err(ts->dev, "%s: Failed to convert value\n", __func__);
			goto cyttsp_drv_rw_reg_data_store_exit;
		}
	}

	if (value > CY_RW_REG_DATA_MAX) {
		dev_err(ts->dev, "%s: Invalid Register Data Range; no write\n", __func__);
	} else {
		reg_data = (u8)value;
		retval = _cyttsp4_write_block_data(ts, ts->rw_regid, sizeof(reg_data), &reg_data,
			ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
		if (retval < 0) {
			dev_err(ts->dev, "%s: Failed write to Regid=%02X(%d)\n", __func__, ts->rw_regid, ts->rw_regid);
		}
	}

	retval = size;

cyttsp_drv_rw_reg_data_store_exit:
	return retval;
}

static DEVICE_ATTR(drv_rw_reg_data, S_IWUSR | S_IRUGO, cyttsp_drv_rw_reg_data_show, cyttsp_drv_rw_reg_data_store);
#endif

#ifdef CY_USE_INCLUDE_FBL
/* Group Number */
static ssize_t cyttsp4_ic_grpnum_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);

	return snprintf(buf, CY_MAX_PRBUF_SIZE, "Current Group: %d\n", ts->ic_grpnum);
}

static ssize_t cyttsp4_ic_grpnum_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);
	unsigned long value = 0;
	int retval = 0;

	mutex_lock(&(ts->data_lock));
	retval = strict_strtoul(buf, 10, &value);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Failed to convert value\n", __func__);
		goto cyttsp4_ic_grpnum_store_error_exit;
	}

	if (value > 0xFF) {
		value = 0xFF;
		dev_err(ts->dev, "%s: value is greater than max; set to %d\n", __func__, (int)value);
	}
	ts->ic_grpnum = value;

	dev_vdbg(ts->dev, "%s: grpnum=%d\n", __func__, ts->ic_grpnum);

cyttsp4_ic_grpnum_store_error_exit:
	retval = size;
	mutex_unlock(&(ts->data_lock));
	return retval;
}

static DEVICE_ATTR(ic_grpnum, S_IRUSR | S_IWUSR, cyttsp4_ic_grpnum_show, cyttsp4_ic_grpnum_store);

/* Group Offset */
static ssize_t cyttsp4_ic_grpoffset_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);

	return snprintf(buf, CY_MAX_PRBUF_SIZE,	"Current Offset: %u\n", ts->ic_grpoffset);
}

static ssize_t cyttsp4_ic_grpoffset_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);
	unsigned long value;
	int retval = 0;

	mutex_lock(&(ts->data_lock));
	retval = strict_strtoul(buf, 10, &value);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Failed to convert value\n", __func__);
		goto cyttsp4_ic_grpoffset_store_error_exit;
	}

#ifdef CY_USE_TMA400
	if (value > 0xFFFF) {
		value = 0xFFFF;
		dev_err(ts->dev, "%s: value is greater than max; set to %d\n", __func__, (int)value);
	}
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
	if (value > 0xFF) {
		value = 0xFF;
		dev_err(ts->dev, "%s: value is greater than max; set to %d\n", __func__, (int)value);
	}
#endif /* --CY_USE_TMA884 */
	ts->ic_grpoffset = value;

	dev_vdbg(ts->dev, "%s: grpoffset=%d\n", __func__, ts->ic_grpoffset);

cyttsp4_ic_grpoffset_store_error_exit:
	retval = size;
	mutex_unlock(&(ts->data_lock));
	return retval;
}
static DEVICE_ATTR(ic_grpoffset, S_IRUSR | S_IWUSR,	cyttsp4_ic_grpoffset_show, cyttsp4_ic_grpoffset_store);

/* Group Data */
#ifdef CY_USE_TMA400
static int _cyttsp4_show_tch_param_tma400(struct cyttsp4 *ts,
	u8 *ic_buf, size_t *num_data)
{
	/*
	 * get data from ts->ic_grpoffset to
	 * end of block containing ts->ic_grpoffset
	 */
	enum cyttsp4_ic_ebid ebid = CY_TCH_PARM_EBID;
	int start_addr;
	int row_id;
	u8 *pdata;
	int retval = 0;

	retval = _cyttsp4_set_mode(ts, CY_CONFIG_MODE);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Fail switch to config mode r=%d\n",__func__, retval);
		goto _cyttsp4_show_tch_param_tma400_err;
	}

	dev_vdbg(ts->dev, "%s: read block_size=%d pdata=%p\r", __func__, ts->ebid_row_size, pdata);

	pdata = kzalloc(ts->ebid_row_size, GFP_KERNEL);
	if (pdata == NULL) {
		dev_err(ts->dev, "%s: Fail allocate block buffer\n", __func__);
		retval = -ENOMEM;
		goto _cyttsp4_show_tch_param_tma400_exit;
	}

	start_addr = ts->ic_grpoffset;
	row_id = start_addr / ts->ebid_row_size;
	start_addr %= ts->ebid_row_size;

	dev_vdbg(ts->dev, "%s: read row=%d size=%d pdata=%p\r", __func__, row_id, ts->ebid_row_size, pdata);

	retval = _cyttsp4_get_ebid_data_tma400(ts, ebid, row_id, pdata);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Fail get row=%d r=%d\n", __func__, row_id, retval);
		goto _cyttsp4_show_tch_param_tma400_exit;
	}

	*num_data = ts->ebid_row_size - start_addr;
	memcpy(&ic_buf[0], &pdata[start_addr], *num_data);

_cyttsp4_show_tch_param_tma400_exit:
	retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Fail switch to operational mode r=%d\n", __func__, retval);
	}
	
	if (pdata != NULL)
		kfree(pdata); 

_cyttsp4_show_tch_param_tma400_err:
	return retval;
}
#endif /* --CY_USE_TMA400 */

static ssize_t _cyttsp4_ic_grpdata_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);
	int i;
	int retval = 0;
	size_t num_read = 0;
	u8 *ic_buf;
	u8 *pdata;
#ifdef CY_USE_TMA884
	size_t ndata = 0;
	u8 blockid = 0;
#endif /* --CY_USE_TMA884 */

	ic_buf = kzalloc(CY_MAX_PRBUF_SIZE, GFP_KERNEL);
	if (ic_buf == NULL) {
		dev_err(ts->dev,
			"%s: Failed to allocate buffer for %s\n",
			__func__, "ic_grpdata_show");
		return snprintf(buf, CY_MAX_PRBUF_SIZE,
			"Group %d buffer allocation error.\n",
			ts->ic_grpnum);
	}
	dev_vdbg(ts->dev,
		"%s: grpnum=%d grpoffset=%u\n",
		__func__, ts->ic_grpnum, ts->ic_grpoffset);

	if (ts->ic_grpnum >= CY_IC_GRPNUM_NUM) {
		dev_err(ts->dev,
			"%s: Group %d does not exist.\n",
			__func__, ts->ic_grpnum);
		kfree(ic_buf);
		return snprintf(buf, CY_MAX_PRBUF_SIZE,
			"Group %d does not exist.\n",
			ts->ic_grpnum);
	}

	switch (ts->ic_grpnum) {
	case CY_IC_GRPNUM_RESERVED:
		goto cyttsp4_ic_grpdata_show_grperr;
		break;
	case CY_IC_GRPNUM_CMD_REGS:
		num_read = ts->si_ofs.rep_ofs - ts->si_ofs.cmd_ofs;
		dev_vdbg(ts->dev,
			"%s: GRP=CMD_REGS: num_read=%d at ofs=%d + grpofs=%d\n",
			__func__, num_read,
			ts->si_ofs.cmd_ofs, ts->ic_grpoffset);
		if (ts->ic_grpoffset >= num_read)
			goto cyttsp4_ic_grpdata_show_ofserr;
		else {
			num_read -= ts->ic_grpoffset;
			retval = _cyttsp4_read_block_data(ts, ts->ic_grpoffset +
				ts->si_ofs.cmd_ofs, num_read, ic_buf,
				ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
			if (retval < 0)
				goto cyttsp4_ic_grpdata_show_prerr;
		}
		break;
	case CY_IC_GRPNUM_TCH_REP:
		num_read = ts->si_ofs.rep_sz;
		dev_vdbg(ts->dev,
			"%s: GRP=TCH_REP: num_read=%d at ofs=%d + grpofs=%d\n",
			__func__, num_read,
			ts->si_ofs.rep_ofs, ts->ic_grpoffset);
		if (ts->ic_grpoffset >= num_read)
			goto cyttsp4_ic_grpdata_show_ofserr;
		else {
			num_read -= ts->ic_grpoffset;
			retval = _cyttsp4_read_block_data(ts, ts->ic_grpoffset +
				ts->si_ofs.rep_ofs, num_read, ic_buf,
				ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
			if (retval < 0)
				goto cyttsp4_ic_grpdata_show_prerr;
		}
		break;
	case CY_IC_GRPNUM_DATA_REC:
		num_read = ts->si_ofs.cydata_size;
		dev_vdbg(ts->dev,
			"%s: GRP=DATA_REC: num_read=%d at ofs=%d + grpofs=%d\n",
			__func__, num_read,
			ts->si_ofs.cydata_ofs, ts->ic_grpoffset);
		if (ts->ic_grpoffset >= num_read)
			goto cyttsp4_ic_grpdata_show_ofserr;
		else {
			num_read -= ts->ic_grpoffset;
			retval = _cyttsp4_set_mode(ts, CY_SYSINFO_MODE);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Fail enter Sysinfo mode r=%d\n",
					__func__, retval);
				goto cyttsp4_ic_grpdata_show_data_rderr;
			}
			retval = _cyttsp4_read_block_data(ts, ts->ic_grpoffset +
				ts->si_ofs.cydata_ofs, num_read, ic_buf,
				ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Fail read Sysinfo ddata r=%d\n",
					__func__, retval);
				goto cyttsp4_ic_grpdata_show_data_rderr;
			}
			retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Fail enter Operational mode r=%d\n",
					__func__, retval);
			}
		}
		break;
cyttsp4_ic_grpdata_show_data_rderr:
		dev_err(ts->dev,
			"%s: Fail read cydata record\n", __func__);
		goto cyttsp4_ic_grpdata_show_prerr;
		break;
	case CY_IC_GRPNUM_TEST_REC:
		num_read =  ts->si_ofs.test_size;
		dev_vdbg(ts->dev,
			"%s: GRP=TEST_REC: num_read=%d at ofs=%d + grpofs=%d\n",
			__func__, num_read,
			ts->si_ofs.test_ofs, ts->ic_grpoffset);
		if (ts->ic_grpoffset >= num_read)
			goto cyttsp4_ic_grpdata_show_ofserr;
		else {
			num_read -= ts->ic_grpoffset;
			retval = _cyttsp4_set_mode(ts, CY_SYSINFO_MODE);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Fail enter Sysinfo mode r=%d\n",
					__func__, retval);
				goto cyttsp4_ic_grpdata_show_test_rderr;
			}
			retval = _cyttsp4_read_block_data(ts, ts->ic_grpoffset +
				ts->si_ofs.test_ofs, num_read, ic_buf,
				ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Fail read Sysinfo ddata r=%d\n",
					__func__, retval);
				goto cyttsp4_ic_grpdata_show_test_rderr;
			}
			retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Fail enter Operational mode r=%d\n",
					__func__, retval);
			}
		}
		break;
cyttsp4_ic_grpdata_show_test_rderr:
		dev_err(ts->dev,
			"%s: Fail read test record\n", __func__);
		goto cyttsp4_ic_grpdata_show_prerr;
		break;
	case CY_IC_GRPNUM_PCFG_REC:
		num_read = ts->si_ofs.pcfg_size;
		dev_vdbg(ts->dev,
			"%s: GRP=PCFG_REC: num_read=%d at ofs=%d + grpofs=%d\n",
			__func__, num_read,
			ts->si_ofs.pcfg_ofs, ts->ic_grpoffset);
		if (ts->ic_grpoffset >= num_read)
			goto cyttsp4_ic_grpdata_show_ofserr;
		else {
			num_read -= ts->ic_grpoffset;
			retval = _cyttsp4_set_mode(ts, CY_SYSINFO_MODE);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Fail enter Sysinfo mode r=%d\n",
					__func__, retval);
				goto cyttsp4_ic_grpdata_show_pcfg_rderr;
			}
			retval = _cyttsp4_read_block_data(ts, ts->ic_grpoffset +
				ts->si_ofs.pcfg_ofs, num_read, ic_buf,
				ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Fail read Sysinfo ddata r=%d\n",
					__func__, retval);
				goto cyttsp4_ic_grpdata_show_pcfg_rderr;
			}
			retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Fail enter Operational mode r=%d\n",
					__func__, retval);
			}
		}
		break;
cyttsp4_ic_grpdata_show_pcfg_rderr:
		dev_err(ts->dev,
			"%s: Fail read pcfg record\n", __func__);
		goto cyttsp4_ic_grpdata_show_prerr;
		break;
	case CY_IC_GRPNUM_OPCFG_REC:
		num_read = ts->si_ofs.opcfg_size;
		dev_vdbg(ts->dev,
			"%s: GRP=OPCFG_REC:"
			" num_read=%d at ofs=%d + grpofs=%d\n",
			__func__, num_read,
			ts->si_ofs.opcfg_ofs, ts->ic_grpoffset);
		if (ts->ic_grpoffset >= num_read)
			goto cyttsp4_ic_grpdata_show_ofserr;
		else {
			num_read -= ts->ic_grpoffset;
			retval = _cyttsp4_set_mode(ts, CY_SYSINFO_MODE);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Fail enter Sysinfo mode r=%d\n",
					__func__, retval);
				goto cyttsp4_ic_grpdata_show_opcfg_rderr;
			}
			retval = _cyttsp4_read_block_data(ts, ts->ic_grpoffset +
				ts->si_ofs.opcfg_ofs, num_read, ic_buf,
				ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Fail read Sysinfo ddata r=%d\n",
					__func__, retval);
				goto cyttsp4_ic_grpdata_show_opcfg_rderr;
			}
			retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Fail enter Operational mode r=%d\n",
					__func__, retval);
			}
		}
		break;
cyttsp4_ic_grpdata_show_opcfg_rderr:
		dev_err(ts->dev,
			"%s: Fail read opcfg record\n", __func__);
		goto cyttsp4_ic_grpdata_show_prerr;
		break;
	case CY_IC_GRPNUM_TCH_PARM_VAL:
#ifdef CY_USE_TMA400
		retval = _cyttsp4_show_tch_param_tma400(ts, ic_buf, &num_read);
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: Fail show Touch Parameters"
				" for TMA400 r=%d\n", __func__, retval);
			goto cyttsp4_ic_grpdata_show_tch_rderr;
		}
		break;
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
		ndata = CY_NUM_CONFIG_BYTES;
		/* do not show cmd, block size and end of block bytes */
		num_read = ndata - (6+4+6);
		dev_vdbg(ts->dev,
			"%s: GRP=PARM_VAL: num_read=%d at ofs=%d + grpofs=%d\n",
			__func__, num_read,
			0, ts->ic_grpoffset);
		if (ts->ic_grpoffset >= num_read)
			goto cyttsp4_ic_grpdata_show_ofserr;
		else {
			blockid = CY_TCH_PARM_EBID;
			pdata = kzalloc(ndata, GFP_KERNEL);
			if (pdata == NULL) {
				dev_err(ts->dev,
			"%s: Failed to allocate read buffer"
					" for %s\n",
					__func__, "platform_touch_param_data");
				retval = -ENOMEM;
				goto cyttsp4_ic_grpdata_show_tch_rderr;
			}
			dev_vdbg(ts->dev,
				"%s: read config block=0x%02X\n",
				__func__, blockid);
			retval = _cyttsp4_set_mode(ts, CY_CONFIG_MODE);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Failed to switch to config mode\n",
					__func__);
				goto cyttsp4_ic_grpdata_show_tch_rderr;
			}
			retval = _cyttsp4_read_config_block(ts,
				blockid, pdata, ndata,
				"platform_touch_param_data");
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Failed read config block %s r=%d\n",
					__func__, "platform_touch_param_data",
					retval);
				goto cyttsp4_ic_grpdata_show_tch_rderr;
			}
			retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
			if (retval < 0) {
				_cyttsp4_change_state(ts, CY_IDLE_STATE);
				dev_err(ts->dev,
			"%s: Fail set operational mode (r=%d)\n",
					__func__, retval);
				goto cyttsp4_ic_grpdata_show_tch_rderr;
			}
			dev_vdbg(ts->dev,
				"%s: memcpy config block=0x%02X\n",
				__func__, blockid);
			num_read -= ts->ic_grpoffset;
			/*
			 * cmd+rdy_bit, status, ebid, lenh, lenl, reserved,
			 * data[0] .. data[ndata-6]
			 * skip data[0] .. data[3] - block size bytes
			 */
			memcpy(ic_buf,
				&pdata[6+4] + ts->ic_grpoffset, num_read);
			kfree(pdata);
		}
		break;
#endif /* --CY_USE_TMA884 */
cyttsp4_ic_grpdata_show_tch_rderr:
		if (pdata != NULL)
			kfree(pdata);
		goto cyttsp4_ic_grpdata_show_prerr;
	case CY_IC_GRPNUM_TCH_PARM_SIZ:
		if (ts->platform_data->sett
			[CY_IC_GRPNUM_TCH_PARM_SIZ] == NULL) {
			dev_err(ts->dev,
			"%s: Missing platform data"
				" Touch Parameters Sizes table\n", __func__);
			goto cyttsp4_ic_grpdata_show_prerr;
		}
		if (ts->platform_data->sett
			[CY_IC_GRPNUM_TCH_PARM_SIZ]->data == NULL) {
			dev_err(ts->dev,
			"%s: Missing platform data"
				" Touch Parameters Sizes table data\n",
				__func__);
			goto cyttsp4_ic_grpdata_show_prerr;
		}
		num_read = ts->platform_data->sett
			[CY_IC_GRPNUM_TCH_PARM_SIZ]->size;
		dev_vdbg(ts->dev,
			"%s: GRP=PARM_SIZ:"
			" num_read=%d at ofs=%d + grpofs=%d\n",
			__func__, num_read,
			0, ts->ic_grpoffset);
		if (ts->ic_grpoffset >= num_read)
			goto cyttsp4_ic_grpdata_show_ofserr;
		else {
			num_read -= ts->ic_grpoffset;
			memcpy(ic_buf, (u8 *)ts->platform_data->sett
				[CY_IC_GRPNUM_TCH_PARM_SIZ]->data +
				ts->ic_grpoffset, num_read);
		}
		break;
	case CY_IC_GRPNUM_DDATA_REC:
		num_read = ts->si_ofs.ddata_size;
		dev_vdbg(ts->dev,
			"%s: GRP=DDATA_REC:"
			" num_read=%d at ofs=%d + grpofs=%d\n",
			__func__, num_read,
			ts->si_ofs.ddata_ofs, ts->ic_grpoffset);
		if (ts->ic_grpoffset >= num_read)
			goto cyttsp4_ic_grpdata_show_ofserr;
		else {
			num_read -= ts->ic_grpoffset;
			retval = _cyttsp4_set_mode(ts, CY_SYSINFO_MODE);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Fail enter Sysinfo mode r=%d\n",
					__func__, retval);
				goto cyttsp4_ic_grpdata_show_ddata_rderr;
			}
			retval = _cyttsp4_read_block_data(ts, ts->ic_grpoffset +
				ts->si_ofs.ddata_ofs, num_read, ic_buf,
				ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Fail read Sysinfo ddata r=%d\n",
					__func__, retval);
				goto cyttsp4_ic_grpdata_show_ddata_rderr;
			}
			retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Fail enter Operational mode r=%d\n",
					__func__, retval);
			}
		}
		break;
cyttsp4_ic_grpdata_show_ddata_rderr:
		dev_err(ts->dev,
			"%s: Fail read ddata\n", __func__);
		goto cyttsp4_ic_grpdata_show_prerr;
		break;
	case CY_IC_GRPNUM_MDATA_REC:
		num_read = ts->si_ofs.mdata_size;
		dev_vdbg(ts->dev,
			"%s: GRP=MDATA_REC:"
			" num_read=%d at ofs=%d + grpofs=%d\n",
			__func__, num_read,
			ts->si_ofs.mdata_ofs, ts->ic_grpoffset);
		if (ts->ic_grpoffset >= num_read)
			goto cyttsp4_ic_grpdata_show_ofserr;
		else {
			num_read -= ts->ic_grpoffset;
			retval = _cyttsp4_set_mode(ts, CY_SYSINFO_MODE);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Fail enter Sysinfo mode r=%d\n",
					__func__, retval);
				goto cyttsp4_ic_grpdata_show_mdata_rderr;
			}
			retval = _cyttsp4_read_block_data(ts, ts->ic_grpoffset +
				ts->si_ofs.mdata_ofs, num_read, ic_buf,
				ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Fail read Sysinfo regs r=%d\n",
					__func__, retval);
				goto cyttsp4_ic_grpdata_show_mdata_rderr;
			}
			retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Fail enter Operational mode r=%d\n",
					__func__, retval);
			}
		}
		break;
cyttsp4_ic_grpdata_show_mdata_rderr:
		dev_err(ts->dev,
			"%s: Fail read mdata\n", __func__);
		goto cyttsp4_ic_grpdata_show_prerr;
		break;
	case CY_IC_GRPNUM_TEST_REGS:
		if (ts->test.cur_cmd == CY_TEST_CMD_NULL) {
			num_read = 1;
			retval = _cyttsp4_load_status_regs(ts);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: failed to read host mode"
					" r=%d\n", __func__, retval);
				ic_buf[0] = (u8)CY_IGNORE_VALUE;
			} else
				ic_buf[0] = ts->xy_mode[0];
			dev_vdbg(ts->dev,
				"%s: GRP=TEST_REGS: NULL CMD: host_mode"
				"=%02X\n", __func__, ic_buf[0]);
		} else if (ts->test.cur_mode == CY_TEST_MODE_CAT) {
			num_read = ts->test.cur_status_size;
			dev_vdbg(ts->dev,
				"%s: GRP=TEST_REGS: num_rd=%d at ofs=%d"
				" + grpofs=%d\n", __func__, num_read,
				ts->si_ofs.cmd_ofs, ts->ic_grpoffset);
			retval = _cyttsp4_read_block_data(ts,
				ts->ic_grpoffset + ts->si_ofs.cmd_ofs,
				num_read, ic_buf,
				ts->platform_data->addr
				[CY_TCH_ADDR_OFS], true);
			if (retval < 0)
				goto cyttsp4_ic_grpdata_show_prerr;
		} else
			dev_err(ts->dev,
			"%s: Not in Config/Test mode\n", __func__);
		break;
	default:
		goto cyttsp4_ic_grpdata_show_grperr;
		break;
	}

	snprintf(buf, CY_MAX_PRBUF_SIZE,
		"Group %d, Offset %u:\n", ts->ic_grpnum, ts->ic_grpoffset);
	for (i = 0; i < num_read; i++) {
		snprintf(buf, CY_MAX_PRBUF_SIZE,
			"%s0x%02X\n", buf, ic_buf[i]);
	}
	kfree(ic_buf);
	return snprintf(buf, CY_MAX_PRBUF_SIZE,
		"%s(%d bytes)\n", buf, num_read);

cyttsp4_ic_grpdata_show_ofserr:
	dev_err(ts->dev,
			"%s: Group Offset=%d exceeds Group Read Length=%d\n",
		__func__, ts->ic_grpoffset, num_read);
	kfree(ic_buf);
	snprintf(buf, CY_MAX_PRBUF_SIZE,
		"Cannot read Group %d Data.\n",
		ts->ic_grpnum);
	return snprintf(buf, CY_MAX_PRBUF_SIZE,
		"%sGroup Offset=%d exceeds Group Read Length=%d\n",
		buf, ts->ic_grpoffset, num_read);
cyttsp4_ic_grpdata_show_prerr:
	dev_err(ts->dev,
			"%s: Cannot read Group %d Data.\n",
		__func__, ts->ic_grpnum);
	kfree(ic_buf);
	return snprintf(buf, CY_MAX_PRBUF_SIZE,
		"Cannot read Group %d Data.\n",
		ts->ic_grpnum);
cyttsp4_ic_grpdata_show_grperr:
	dev_err(ts->dev,
			"%s: Group %d does not exist.\n",
		__func__, ts->ic_grpnum);
	kfree(ic_buf);
	return snprintf(buf, CY_MAX_PRBUF_SIZE,
		"Group %d does not exist.\n",
		ts->ic_grpnum);
}
static ssize_t cyttsp4_ic_grpdata_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);
	ssize_t retval = 0;

	mutex_lock(&ts->data_lock);
	if (ts->driver_state == CY_SLEEP_STATE) {
		dev_err(ts->dev,
			"%s: Group Show Test blocked: IC suspended\n",
			__func__);
		retval = snprintf(buf, CY_MAX_PRBUF_SIZE,
			"Group %d Show Test blocked: IC suspended\n",
			ts->ic_grpnum);
	} else
		retval = _cyttsp4_ic_grpdata_show(dev, attr, buf);
	mutex_unlock(&ts->data_lock);

	return retval;
}

#ifdef CY_USE_TMA400
static int _cyttsp4_store_tch_param_tma400(struct cyttsp4 *ts,
	u8 *ic_buf, size_t length)
{
	int retval = 0;
	int next_data = 0;
	int num_data = 0;
	int start_addr = 0;
	int end_addr = 0;
	int start_row = 0;
	int end_row = 0;
	int row_id = 0;
	int row_ofs = 0;
	int num_rows = 0;
	int crc_loc = 0;
	enum cyttsp4_ic_ebid ebid = CY_TCH_PARM_EBID;
	u8 calc_ic_crc[2];
	u8 *pdata = NULL;

	memset(calc_ic_crc, 0, sizeof(calc_ic_crc));
	retval = _cyttsp4_set_mode(ts, CY_CONFIG_MODE);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail switch to config mode r=%d\n",
			__func__, retval);
		goto _cyttsp4_store_tch_param_tma400_err;
	}

	start_addr = ts->ic_grpoffset;
	next_data = 0;
	end_addr = start_addr + length;
	start_row = start_addr / ts->ebid_row_size;
	start_addr %= ts->ebid_row_size;
	end_row = end_addr / ts->ebid_row_size;
	end_addr %= ts->ebid_row_size;
	num_rows = end_row - start_row + 1;

	dev_vdbg(ts->dev,
		"%s: start_addr=0x%04X(%d) size=%d start_row=%d end_row=%d"
		" end_addr=%04X(%d) num_rows=%d\n",
		__func__,
		start_addr, start_addr, ts->ebid_row_size, start_row,
		end_row, end_addr, end_addr, num_rows);

	pdata = kzalloc(ts->ebid_row_size, GFP_KERNEL);
	if (pdata == NULL) {
		dev_err(ts->dev,
			"%s: Fail allocate block buffer\n", __func__);
		retval = -ENOMEM;
		goto _cyttsp4_store_tch_param_tma400_exit;
	}

	for (row_id = start_row;
		row_id < start_row + num_rows; row_id++) {
		dev_vdbg(ts->dev,
			"%s: get EBID row=%d\n", __func__, row_id);
		retval = _cyttsp4_get_ebid_data_tma400(ts, ebid, row_id, pdata);
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: Fail get EBID row=%d r=%d\n",
				__func__, row_id, retval);
			goto _cyttsp4_store_tch_param_tma400_exit;
		}
		num_data = ts->ebid_row_size - start_addr;
		if (row_id == end_row)
			num_data -= ts->ebid_row_size - end_addr;
		memcpy(&pdata[start_addr], &ic_buf[next_data], num_data);
		next_data += num_data;
		dev_vdbg(ts->dev,
			"%s: put_row=%d size=%d pdata=%p start_addr=%04X"
			" &pdata[start_addr]=%p num_data=%d\n", __func__,
			row_id, ts->ebid_row_size, pdata, start_addr,
			&pdata[start_addr], num_data);
		_cyttsp4_pr_buf(ts, &pdata[start_addr], num_data, "put_block");
		_cyttsp4_pr_buf(ts, pdata, ts->ebid_row_size, "print_block");
		retval = _cyttsp4_put_ebid_data_tma400(ts,
			ebid, row_id, pdata);
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: Fail put EBID row=%d r=%d\n",
				__func__, row_id, retval);
			goto _cyttsp4_store_tch_param_tma400_exit;
		}

		start_addr = 0;
		ts->ic_grptest = true;
	}

	/* Update CRC bytes to force restore on reboot */
	if (ts->ic_grptest) {
		memset(calc_ic_crc, 0, sizeof(calc_ic_crc));
		dev_vdbg(ts->dev,
			"%s: Calc IC CRC values\n", __func__);
		retval = _cyttsp4_calc_ic_crc_tma400(ts, ebid,
			&calc_ic_crc[1], &calc_ic_crc[0], false);
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: Fail calc ic crc r=%d\n",
				__func__, retval);
		}
		retval = _cyttsp4_get_ebid_data_tma400(ts, ebid, 0, pdata);
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: Fail get EBID row=%d r=%d\n",
				__func__, row_id, retval);
			goto _cyttsp4_store_tch_param_tma400_exit;
		}
		crc_loc = (pdata[3] * 256) + pdata[2];
		row_ofs = crc_loc % ts->ebid_row_size;
		row_id = crc_loc / ts->ebid_row_size;
		dev_vdbg(ts->dev,
		"%s: tch ebid=%d crc_loc=%08X crc_row=%d crc_ofs=%d data:\n",
		__func__, ebid, crc_loc, row_id, row_ofs);
		retval = _cyttsp4_get_ebid_data_tma400(ts, ebid, row_id, pdata);
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: Fail get EBID row=%d r=%d\n",
				__func__, row_id, retval);
			goto _cyttsp4_store_tch_param_tma400_exit;
		}
		memcpy(&pdata[row_ofs], calc_ic_crc, sizeof(calc_ic_crc));
		retval = _cyttsp4_put_ebid_data_tma400(ts,
			ebid, row_id, pdata);
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: Fail put EBID row=%d r=%d\n",
				__func__, row_id, retval);
		}
	}

_cyttsp4_store_tch_param_tma400_exit:
	retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail switch to operational mode r=%d\n",
			__func__, retval);
	}
	if (pdata != NULL)
		kfree(pdata);
_cyttsp4_store_tch_param_tma400_err:
	return retval;
}
#endif /* --CY_USE_TMA400 */

#ifdef CY_USE_TMA884
static int _cyttsp4_write_mddata(struct cyttsp4 *ts, size_t write_length,
								 size_t mddata_length, u8 blkid, size_t mddata_ofs,
								 u8 *ic_buf, const char *mddata_name)
{
	bool mddata_updated = false;
	u8 *pdata;
	int retval = 0;

	pdata = kzalloc(CY_MAX_PRBUF_SIZE, GFP_KERNEL);
	if (pdata == NULL) {
		dev_err(ts->dev,
			"%s: Fail allocate data buffer\n", __func__);
		retval = -ENOMEM;
		goto cyttsp4_write_mddata_exit;
	}
	if (ts->current_mode != CY_MODE_OPERATIONAL) {
		dev_err(ts->dev,
			"%s: Must be in operational mode to start write of"
			" %s (current mode=%d)\n",
			__func__, mddata_name, ts->current_mode);
		retval = -EPERM;
		goto cyttsp4_write_mddata_exit;
	}
	if ((write_length + ts->ic_grpoffset) > mddata_length) {
		dev_err(ts->dev,
			"%s: Requested length(%d) is greater than"
			" %s size(%d)\n", __func__,
			write_length, mddata_name, mddata_length);
		retval = -EINVAL;
		goto cyttsp4_write_mddata_exit;
	}
	retval = _cyttsp4_set_mode(ts, CY_SYSINFO_MODE);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail to enter Sysinfo mode r=%d\n",
			__func__, retval);
		goto cyttsp4_write_mddata_exit;
	}
	dev_vdbg(ts->dev,
		"%s: blkid=%02X mddata_ofs=%d mddata_length=%d"
		" mddata_name=%s write_length=%d grpofs=%d\n",
		__func__, blkid, mddata_ofs, mddata_length, mddata_name,
		write_length, ts->ic_grpoffset);
	_cyttsp4_read_block_data(ts, mddata_ofs, mddata_length, pdata,
		ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail to read %s regs r=%d\n",
			__func__, mddata_name, retval);
		goto cyttsp4_write_mddata_exit;
	}
	memcpy(pdata + ts->ic_grpoffset, ic_buf, write_length);
	_cyttsp4_set_data_block(ts, blkid, pdata,
		mddata_length, mddata_name, true, &mddata_updated);
	if ((retval < 0) || !mddata_updated) {
		dev_err(ts->dev,
			"%s: Fail while writing %s block r=%d updated=%d\n",
			__func__, mddata_name, retval, (int)mddata_updated);
	}
	retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail to enter Operational mode r=%d\n",
			__func__, retval);
	}

cyttsp4_write_mddata_exit:
	kfree(pdata);
	return retval;
}
#endif /* --CY_USE_TMA884 */

static ssize_t _cyttsp4_ic_grpdata_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);
	unsigned long value = 0;
	int retval = 0;
	const char *pbuf = buf;
	int i = 0;
	int j = 0;
	char last = 0;
	char *scan_buf = NULL;
	u8 *ic_buf = NULL;
	size_t length = 0;
	u8 host_mode = 0;
	enum cyttsp4_driver_state save_state = CY_INVALID_STATE;
#ifdef CY_USE_TMA884
	u8 *pdata = NULL;
	size_t mddata_length = 0;
	size_t ndata = 0;
	u8 blockid = 0;
	bool mddata_updated = false;
	const char *mddata_name = "invalid name";
#endif /* --CY_USE_TMA884 */

	scan_buf = kzalloc(CY_MAX_PRBUF_SIZE, GFP_KERNEL);
	if (scan_buf == NULL) {
		dev_err(ts->dev,
			"%s: Failed to allocate scan buffer for"
			" Group Data store\n", __func__);
		goto cyttsp4_ic_grpdata_store_exit;
	}
	ic_buf = kzalloc(CY_MAX_PRBUF_SIZE, GFP_KERNEL);
	if (ic_buf == NULL) {
		dev_err(ts->dev,
			"%s: Failed to allocate ic buffer for"
			" Group Data store\n", __func__);
		goto cyttsp4_ic_grpdata_store_exit;
	}
	dev_vdbg(ts->dev,
		"%s: grpnum=%d grpoffset=%u\n",
		__func__, ts->ic_grpnum, ts->ic_grpoffset);

	if (ts->ic_grpnum >= CY_IC_GRPNUM_NUM) {
		dev_err(ts->dev,
			"%s: Group %d does not exist.\n",
			__func__, ts->ic_grpnum);
		retval = size;
		goto cyttsp4_ic_grpdata_store_exit;
	}
	dev_vdbg(ts->dev,
		"%s: pbuf=%p buf=%p size=%d sizeof(scan_buf)=%d buf=%s\n",
		__func__, pbuf, buf, size, sizeof(scan_buf), buf);

	i = 0;
	last = 0;
	while (pbuf <= (buf + size)) {
		while (((*pbuf == ' ') || (*pbuf == ',')) &&
			(pbuf < (buf + size))) {
			last = *pbuf;
			pbuf++;
		}
		if (pbuf < (buf + size)) {
			memset(scan_buf, 0, CY_MAX_PRBUF_SIZE);
			if ((last == ',') && (*pbuf == ',')) {
				dev_err(ts->dev,
			"%s: Invalid data format. "
					"\",,\" not allowed.\n",
					__func__);
				retval = size;
				goto cyttsp4_ic_grpdata_store_exit;
			}
			for (j = 0; j < sizeof("0xHH") &&
				*pbuf != ' ' && *pbuf != ','; j++) {
				last = *pbuf;
				scan_buf[j] = *pbuf++;
			}	
			retval = strict_strtoul(scan_buf, 16, &value);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Invalid data format. "
					"Use \"0xHH,...,0xHH\" instead.\n",
					__func__);
				retval = size;
				goto cyttsp4_ic_grpdata_store_exit;
			} else {
				if (i >= ts->max_config_bytes) {
					dev_err(ts->dev,
			"%s: Max command size exceeded"
					" (size=%d max=%d)\n", __func__,
					i, ts->max_config_bytes);
					goto cyttsp4_ic_grpdata_store_exit;
				}
				ic_buf[i] = value;
				dev_vdbg(ts->dev,
					"%s: ic_buf[%d] = 0x%02X\n",
					__func__, i, ic_buf[i]);
				i++;
			}
		} else
			break;
	}
	length = i;

	/* write ic_buf to log */
	_cyttsp4_pr_buf(ts, ic_buf, length, "ic_buf");

	switch (ts->ic_grpnum) {
	case CY_IC_GRPNUM_CMD_REGS:
		if ((length + ts->ic_grpoffset + ts->si_ofs.cmd_ofs) >
			ts->si_ofs.rep_ofs) {
			dev_err(ts->dev,
			"%s: Length(%d) + offset(%d) + cmd_offset(%d)"
				" is beyond cmd reg space[%d..%d]\n", __func__,
				length, ts->ic_grpoffset, ts->si_ofs.cmd_ofs,
				ts->si_ofs.cmd_ofs, ts->si_ofs.rep_ofs - 1);
			goto cyttsp4_ic_grpdata_store_exit;
		}
		retval = _cyttsp4_write_block_data(ts, ts->ic_grpoffset +
			ts->si_ofs.cmd_ofs, length, ic_buf,
			ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: Fail write command regs r=%d\n",
				__func__, retval);
		}
		if (!ts->ic_grptest) {
			dev_info(ts->dev,
			"%s: Disabled settings checksum verifications"
				" until next boot.\n", __func__);
			ts->ic_grptest = true;
		}
		break;
	case CY_IC_GRPNUM_TCH_PARM_VAL:
#ifdef CY_USE_TMA400
		retval = _cyttsp4_store_tch_param_tma400(ts, ic_buf, length);
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: Fail store Touch Parameters"
				" for TMA400 r=%d\n", __func__, retval);
		}
		break;
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
		mddata_name = "Touch Parameters";
		ndata = CY_NUM_CONFIG_BYTES;
		blockid = CY_TCH_PARM_EBID;
		/* do not show cmd, block size and end of block bytes */
		mddata_length = ndata - (6+4+6);
		dev_vdbg(ts->dev,
			"%s: GRP=PARM_VAL: write length=%d at ofs=%d +"
			" grpofs=%d\n", __func__, length,
			0, ts->ic_grpoffset);
		if ((length + ts->ic_grpoffset) > mddata_length) {
			dev_err(ts->dev,
			"%s: Requested length(%d) is greater than"
				" %s size(%d)\n", __func__,
				length, mddata_name, mddata_length);
			retval = -EINVAL;
			goto cyttsp4_ic_grpdata_store_tch_wrerr;
		}
		pdata = kzalloc(ndata, GFP_KERNEL);
		if (pdata == NULL) {
			dev_err(ts->dev,
			"%s: Failed to allocate read/write buffer"
				" for %s\n",
				__func__, "platform_touch_param_data");
			retval = -ENOMEM;
			goto cyttsp4_ic_grpdata_store_tch_wrerr;
		}
		dev_vdbg(ts->dev,
			"%s: read config block=0x%02X\n",
			__func__, blockid);
		retval = _cyttsp4_set_mode(ts, CY_CONFIG_MODE);
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: Failed to switch to config mode\n",
				__func__);
			goto cyttsp4_ic_grpdata_store_tch_wrerr;
		}
		retval = _cyttsp4_read_config_block(ts,
			blockid, pdata, ndata,
			"platform_touch_param_data");
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: Failed read config block %s r=%d\n",
				__func__, "platform_touch_param_data",
				retval);
			goto cyttsp4_ic_grpdata_store_tch_wrerr;
		}
		/*
		 * cmd+rdy_bit, status, ebid, lenh, lenl, reserved,
		 * data[0] .. data[ndata-6]
		 * skip data[0] .. data[3] - block size bytes
		 */
		memcpy(&pdata[6+4+ts->ic_grpoffset], ic_buf, length);
		_cyttsp4_set_data_block(ts, blockid, &pdata[6+4],
			mddata_length, mddata_name, true, &mddata_updated);
		if ((retval < 0) || !mddata_updated) {
			dev_err(ts->dev,
			"%s: Fail while writing %s block r=%d"
				" updated=%d\n", __func__,
				mddata_name, retval, (int)mddata_updated);
		}
		if (!ts->ic_grptest) {
			dev_info(ts->dev,
			"%s: Disabled settings checksum verifications"
				" until next boot.\n", __func__);
			ts->ic_grptest = true;
		}
		retval = _cyttsp4_startup(ts);
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: Fail restart after writing params r=%d\n",
				__func__, retval);
		}
cyttsp4_ic_grpdata_store_tch_wrerr:
		kfree(pdata);
		break;
#endif /* --CY_USE_TMA884 */
	case CY_IC_GRPNUM_DDATA_REC:
#ifdef CY_USE_TMA400
		dev_err(ts->dev,
			"%s: Group=%d is read only for TMA400\n",
			__func__, ts->ic_grpnum);
		break;
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
		mddata_length = ts->si_ofs.ddata_size;
		dev_vdbg(ts->dev,
			"%s: DDATA_REC length=%d mddata_length=%d blkid=%02X"
			" ddata_ofs=%d name=%s\n", __func__, length,
			mddata_length, CY_DDATA_EBID, ts->si_ofs.ddata_ofs,
			"Design Data");
		_cyttsp4_pr_buf(ts, ic_buf, length, "Design Data");
		retval = _cyttsp4_write_mddata(ts, length, mddata_length,
			CY_DDATA_EBID, ts->si_ofs.ddata_ofs, ic_buf,
			"Design Data");
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: Fail writing Design Data\n",
				__func__);
		} else if (!ts->ic_grptest) {
			dev_info(ts->dev,
			"%s: Disabled settings checksum verifications"
				" until next boot.\n", __func__);
			ts->ic_grptest = true;
		}
		break;
#endif /* --CY_USE_TMA884 */
	case CY_IC_GRPNUM_MDATA_REC:
#ifdef CY_USE_TMA400
		dev_err(ts->dev,
			"%s: Group=%d is read only for TMA400\n",
			__func__, ts->ic_grpnum);
		break;
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
		mddata_length = ts->si_ofs.mdata_size;
		dev_vdbg(ts->dev,
			"%s: MDATA_REC length=%d mddata_length=%d blkid=%02X"
			" ddata_ofs=%d name=%s\n", __func__, length,
			mddata_length, CY_MDATA_EBID, ts->si_ofs.mdata_ofs,
			"Manufacturing Data");
		_cyttsp4_pr_buf(ts, ic_buf, length, "Manufacturing Data");
		retval = _cyttsp4_write_mddata(ts, length, mddata_length,
			CY_MDATA_EBID, ts->si_ofs.mdata_ofs, ic_buf,
			"Manufacturing Data");
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: Fail writing Manufacturing Data\n",
				__func__);
		} else if (!ts->ic_grptest) {
			dev_info(ts->dev,
			"%s: Disabled settings checksum verifications"
				" until next boot.\n", __func__);
			ts->ic_grptest = true;
		}
		break;
#endif /* --CY_USE_TMA884 */
	case CY_IC_GRPNUM_TEST_REGS:
		ts->test.cur_cmd = ic_buf[0];
		if (ts->test.cur_cmd == CY_TEST_CMD_NULL) {
			switch (ic_buf[1]) {
			case CY_NULL_CMD_NULL:
				dev_err(ts->dev,
			"%s: empty NULL command\n", __func__);
				break;
			case CY_NULL_CMD_MODE:
				save_state = ts->driver_state;
				_cyttsp4_change_state(ts, CY_CMD_STATE);
				host_mode = ic_buf[2] | CY_MODE_CHANGE;
				retval = _cyttsp4_write_block_data(ts,
					CY_REG_BASE, sizeof(host_mode),
					&host_mode, ts->platform_data->addr
					[CY_TCH_ADDR_OFS], true);
				if (retval < 0) {
					dev_err(ts->dev,
			"%s: Fail write host_mode=%02X"
					" r=%d\n", __func__, ic_buf[2], retval);
				} else {
					INIT_COMPLETION(ts->int_running);
					retval = _cyttsp4_wait_int_no_init(ts,
						CY_HALF_SEC_TMO_MS * 5);
					if (retval < 0) {
						dev_err(ts->dev,
			"%s: timeout waiting"
						" host_mode=0x%02X"
						" change  r=%d\n",
						__func__, ic_buf[1], retval);
						/* continue anyway */
					}
					retval = _cyttsp4_cmd_handshake(ts);
					if (retval < 0) {
						dev_err(ts->dev,
			"%s: Fail mode handshake"
							" r=%d\n",
							__func__, retval);
					}
					if (GET_HSTMODE(ic_buf[2]) ==
						GET_HSTMODE(CY_CONFIG_MODE)) {
						ts->test.cur_mode =
							CY_TEST_MODE_CAT;
					} else {
						ts->test.cur_mode =
							CY_TEST_MODE_NORMAL_OP;
					}
				}
				_cyttsp4_change_state(ts, save_state);
				break;
			case CY_NULL_CMD_STATUS_SIZE:
				ts->test.cur_status_size = ic_buf[2] +
					(ic_buf[3] * 256);
				break;
			case CY_NULL_CMD_HANDSHAKE:
				retval = _cyttsp4_cmd_handshake(ts);
				if (retval < 0) {
					dev_err(ts->dev,
			"%s: Fail test cmd handshake"
						" r=%d\n",
						__func__, retval);
				}			
			default:
				break;
			}
		} else {
			dev_dbg(ts->dev,
				"%s: TEST CMD=0x%02X length=%d"
				" cmd_ofs+grpofs=%d\n", __func__, ic_buf[0],
				length, ts->ic_grpoffset + ts->si_ofs.cmd_ofs);
			_cyttsp4_pr_buf(ts, ic_buf, length, "test_cmd");
			retval = _cyttsp4_write_block_data(ts,
				ts->ic_grpoffset + ts->si_ofs.cmd_ofs,
				length, ic_buf,
				ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
			if (retval < 0) {
				dev_err(ts->dev,
			"%s: Fail write command regs r=%d\n",
					__func__, retval);
			}
		}
		break;
	default:
		dev_err(ts->dev,
			"%s: Group=%d is read only\n",
			__func__, ts->ic_grpnum);
		break;
	}

cyttsp4_ic_grpdata_store_exit:
	if (scan_buf != NULL)
		kfree(scan_buf);
	if (ic_buf != NULL)
		kfree(ic_buf);
	return size;
}
static ssize_t cyttsp4_ic_grpdata_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);
	ssize_t retval = 0;

	mutex_lock(&ts->data_lock);
	if (ts->driver_state == CY_SLEEP_STATE) {
		dev_err(ts->dev,
			"%s: Group Store Test blocked: IC suspended\n",
			__func__);
		retval = size;
	} else
		retval = _cyttsp4_ic_grpdata_store(dev, attr, buf, size);
	mutex_unlock(&ts->data_lock);

	return retval;
}
static DEVICE_ATTR(ic_grpdata, S_IRUSR | S_IWUSR,
	cyttsp4_ic_grpdata_show, cyttsp4_ic_grpdata_store);

static ssize_t cyttsp4_drv_flags_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);

	return snprintf(buf, CY_MAX_PRBUF_SIZE,
		"Current Driver Flags: 0x%04X\n", ts->flags);
}
static ssize_t cyttsp4_drv_flags_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);
	unsigned long value = 0;
	ssize_t retval = 0;

	mutex_lock(&(ts->data_lock));
	retval = strict_strtoul(buf, 16, &value);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Failed to convert value\n", __func__);
		goto cyttsp4_drv_flags_store_error_exit;
	}

	if (value > 0xFFFF) {
		dev_err(ts->dev,
			"%s: value=%lu is greater than max;"
			" drv_flags=0x%04X\n", __func__, value, ts->flags);
	} else {
		ts->flags = value;
	}

	dev_vdbg(ts->dev,
		"%s: drv_flags=0x%04X\n", __func__, ts->flags);

cyttsp4_drv_flags_store_error_exit:
	retval = size;
	mutex_unlock(&(ts->data_lock));
	return retval;
}
static DEVICE_ATTR(drv_flags, S_IRUSR | S_IWUSR,
	cyttsp4_drv_flags_show, cyttsp4_drv_flags_store);

static ssize_t cyttsp4_hw_reset_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);
	ssize_t retval = 0;

	mutex_lock(&(ts->data_lock));
	retval = _cyttsp4_startup(ts);
	mutex_unlock(&(ts->data_lock));
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: fail hw_reset device restart r=%d\n",
			__func__, retval);
	}

	retval = size;
	return retval;
}
static DEVICE_ATTR(hw_reset, S_IWUSR, NULL, cyttsp4_hw_reset_store);

static ssize_t cyttsp4_hw_recov_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t size)
{
	struct cyttsp4 *ts = dev_get_drvdata(dev);
	unsigned long value = 0;
	ssize_t retval = 0;

	mutex_lock(&(ts->data_lock));
	retval = strict_strtoul(buf, 10, &value);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Failed to convert value\n", __func__);
		goto cyttsp4_hw_recov_store_error_exit;
	}

	if (ts->platform_data->hw_recov == NULL) {
		dev_err(ts->dev,
			"%s: no hw_recov function\n", __func__);
		goto cyttsp4_hw_recov_store_error_exit;
	}

	retval = ts->platform_data->hw_recov((int)value);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: fail hw_recov(value=%d) function r=%d\n",
			__func__, (int)value, retval);
	}

cyttsp4_hw_recov_store_error_exit:
	retval = size;
	mutex_unlock(&(ts->data_lock));
	return retval;
}
static DEVICE_ATTR(hw_recov, S_IWUSR, NULL, cyttsp4_hw_recov_store);
#endif /* --CY_USE_INCLUDE_FBL */

#define CY_CMD_I2C_ADDR					0
#define CY_STATUS_SIZE_BYTE				1
#define CY_STATUS_TYP_DELAY				2
#define CY_CMD_TAIL_LEN					3
#define CY_CMD_BYTE					1
#define CY_STATUS_BYTE					1
#define CY_MAX_STATUS_SIZE				32
#define CY_MIN_STATUS_SIZE				5
#define CY_START_OF_PACKET				0x01
#define CY_END_OF_PACKET				0x17
#define CY_DATA_ROW_SIZE				288
#define CY_DATA_ROW_SIZE_TMA400				128
#define CY_PACKET_DATA_LEN				96
#define CY_MAX_PACKET_LEN				512
#define CY_COMM_BUSY					0xFF
#define CY_CMD_BUSY					0xFE
#define CY_SEPARATOR_OFFSET				0
#define CY_ARRAY_ID_OFFSET				0
#define CY_ROW_NUM_OFFSET				1
#define CY_ROW_SIZE_OFFSET				3
#define CY_ROW_DATA_OFFSET				5
#define CY_FILE_SILICON_ID_OFFSET			0
#define CY_FILE_REV_ID_OFFSET				4
#define CY_CMD_LDR_HOST_SYNC				0xFF /* tma400 */
#define CY_CMD_LDR_EXIT					0x3B
#define CY_CMD_LDR_EXIT_CMD_SIZE			7
#define CY_CMD_LDR_EXIT_STAT_SIZE			7

enum ldr_status {
	ERROR_SUCCESS = 0,
	ERROR_COMMAND = 1,
	ERROR_FLASH_ARRAY = 2,
	ERROR_PACKET_DATA = 3,
	ERROR_PACKET_LEN = 4,
	ERROR_PACKET_CHECKSUM = 5,
	ERROR_FLASH_PROTECTION = 6,
	ERROR_FLASH_CHECKSUM = 7,
	ERROR_VERIFY_IMAGE = 8,
	ERROR_UKNOWN1 = 9,
	ERROR_UKNOWN2 = 10,
	ERROR_UKNOWN3 = 11,
	ERROR_UKNOWN4 = 12,
	ERROR_UKNOWN5 = 13,
	ERROR_UKNOWN6 = 14,
	ERROR_INVALID_COMMAND = 15,
	ERROR_INVALID
};

static u16 _cyttsp4_compute_crc(struct cyttsp4 *ts, u8 *buf, int size)
{
	u16 crc = 0xffff;
	u16 tmp;
	int i;

	/* RUN CRC */

	if (size == 0)
		crc = ~crc;
	else {

		do {
			for (i = 0, tmp = 0x00ff & *buf++; i < 8;
				i++, tmp >>= 1) {
				if ((crc & 0x0001) ^ (tmp & 0x0001))
					crc = (crc >> 1) ^ 0x8408;
				else
					crc >>= 1;
			}
		} while (--size);

		crc = ~crc;
		tmp = crc;
		crc = (crc << 8) | (tmp >> 8 & 0xFF);
	}

	return crc;
}

static int _cyttsp4_get_status(struct cyttsp4 *ts,
	u8 *buf, int size, unsigned long timeout_ms)
{
	unsigned long uretval = 0;
	int tries = 0;
	int retval = 0;

	if (timeout_ms != 0) {
		/* wait until status ready interrupt or timeout occurs */
		uretval = wait_for_completion_interruptible_timeout(
			&ts->int_running, msecs_to_jiffies(timeout_ms));

		/* read the status packet */
		if (buf == NULL) {
			dev_err(ts->dev,
			"%s: Status buf ptr is NULL\n", __func__);
			retval = -EINVAL;
			goto _cyttsp4_get_status_exit;
		}
		for (tries = 0; tries < 2; tries++) {
			retval = _cyttsp4_read_block_data(ts, CY_REG_BASE, size,
				buf, ts->platform_data->addr[CY_LDR_ADDR_OFS],
#ifdef CY_USE_TMA400
				true);
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
				false);
#endif /* --CY_USE_TMA884 */
			/*
			 * retry if bus read error or
			 * status byte shows not ready
			 */
			if ((buf[1] == CY_COMM_BUSY) || (buf[1] == CY_CMD_BUSY))
				msleep(CY_DELAY_DFLT);
			else
				break;
		}
		dev_vdbg(ts->dev,"%s: tries=%d ret=%d status=%02X\n",__func__, tries, retval, buf[1]);
	}

_cyttsp4_get_status_exit:
	mutex_lock(&ts->data_lock);
	return retval;
}

/*
 * Send a bootloader command to the device;
 * Wait for the ISR to execute indicating command
 * was received and status is ready;
 * Releases data_lock mutex to allow ISR to run,
 * then locks it again.
 */
static int _cyttsp4_send_cmd(struct cyttsp4 *ts, const u8 *cmd_buf,
							 int cmd_size, u8 *stat_ret, size_t num_stat_byte,
							 size_t status_size, unsigned long timeout_ms)
{
	u8 *status_buf = NULL;
	int retval = 0;

	if (timeout_ms > 0) {
		status_buf = kzalloc(CY_MAX_STATUS_SIZE, GFP_KERNEL);
		if (status_buf == NULL) {
			dev_err(ts->dev,
				"%s: Fail alloc status buffer=%p\n",
				__func__, status_buf);
			goto _cyttsp4_send_cmd_exit;
		}
	}

	if (cmd_buf == NULL) {
		dev_err(ts->dev,
			"%s: bad cmd_buf=%p\n", __func__, cmd_buf);
		goto _cyttsp4_send_cmd_exit;
	}

	if (cmd_size == 0) {
		dev_err(ts->dev,
			"%s: bad cmd_size=%d\n", __func__, cmd_size);
		goto _cyttsp4_send_cmd_exit;
	}

	_cyttsp4_pr_buf(ts, (u8 *)cmd_buf, cmd_size, "send_cmd");

	mutex_unlock(&ts->data_lock);
	if (timeout_ms > 0)
		INIT_COMPLETION(ts->int_running);
	retval = _cyttsp4_write_block_data(ts, CY_REG_BASE, cmd_size, cmd_buf,
		ts->platform_data->addr[CY_LDR_ADDR_OFS],
#ifdef CY_USE_TMA400
		true);
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
	false);
#endif /* --CY_USE_TMA884 */
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail writing command=%02X\n",
			__func__, cmd_buf[CY_CMD_BYTE]);
		mutex_lock(&ts->data_lock);
		goto _cyttsp4_send_cmd_exit;
	}

	/* get the status and lock the mutex */
	if (timeout_ms > 0) {
		retval = _cyttsp4_get_status(ts, status_buf,
			status_size, timeout_ms);
		if ((retval < 0) || (status_buf[0] != CY_START_OF_PACKET)) {
			dev_err(ts->dev,
				"%s: Error getting status r=%d"
				" status_buf[0]=%02X\n",
				__func__, retval, status_buf[0]);
			if (!(retval < 0))
				retval = -EIO;
			goto _cyttsp4_send_cmd_exit;
		} else {
			if (status_buf[CY_STATUS_BYTE] != ERROR_SUCCESS) {
				dev_err(ts->dev,
					"%s: Status=0x%02X error\n",
					__func__, status_buf[CY_STATUS_BYTE]);
				retval = -EIO;
			} else if (stat_ret != NULL) {
				if (num_stat_byte < status_size)
					*stat_ret = status_buf[num_stat_byte];
				else
					*stat_ret = 0;
			}
		}
	} else {
		if (stat_ret != NULL)
			*stat_ret = ERROR_SUCCESS;
		mutex_lock(&ts->data_lock);
	}

_cyttsp4_send_cmd_exit:
	if (status_buf != NULL)
		kfree(status_buf);
	return retval;
}

struct cyttsp4_dev_id {
	u32 silicon_id;
	u8 rev_id;
	u32 bl_ver;
};

#if defined(CY_AUTO_LOAD_FW) || 	defined(CY_USE_FORCE_LOAD) || 	defined(CY_USE_INCLUDE_FBL)
#define CY_CMD_LDR_ENTER				0x38
#define CY_CMD_LDR_ENTER_CMD_SIZE			7
#define CY_CMD_LDR_ENTER_STAT_SIZE			15
#define CY_CMD_LDR_INIT					0x48
#define CY_CMD_LDR_INIT_CMD_SIZE			15
#define CY_CMD_LDR_INIT_STAT_SIZE			7
#define CY_CMD_LDR_ERASE_ROW				0x34
#define CY_CMD_LDR_ERASE_ROW_CMD_SIZE			10
#define CY_CMD_LDR_ERASE_ROW_STAT_SIZE			7
#define CY_CMD_LDR_SEND_DATA				0x37
#define CY_CMD_LDR_SEND_DATA_CMD_SIZE			4 /* hdr bytes only */
#define CY_CMD_LDR_SEND_DATA_STAT_SIZE			8
#define CY_CMD_LDR_PROG_ROW				0x39
#define CY_CMD_LDR_PROG_ROW_CMD_SIZE			7 /* hdr bytes only */
#define CY_CMD_LDR_PROG_ROW_STAT_SIZE			7
#define CY_CMD_LDR_VERIFY_ROW				0x3A
#define CY_CMD_LDR_VERIFY_ROW_STAT_SIZE			8
#define CY_CMD_LDR_VERIFY_ROW_CMD_SIZE			10
#define CY_CMD_LDR_VERIFY_CHKSUM			0x31
#define CY_CMD_LDR_VERIFY_CHKSUM_CMD_SIZE		7
#define CY_CMD_LDR_VERIFY_CHKSUM_STAT_SIZE		8

#ifdef CY_USE_INCLUDE_FBL
static const char * const ldr_status_string[] = {
	/* Order must match enum ldr_status above */
	"Error Success",
	"Error Command",
	"Error Flash Array",
	"Error Packet Data",
	"Error Packet Length",
	"Error Packet Checksum",
	"Error Flash Protection",
	"Error Flash Checksum",
	"Error Verify Image",
	"Error Invalid Command",
	"Error Invalid Command",
	"Error Invalid Command",
	"Error Invalid Command",
	"Error Invalid Command",
	"Error Invalid Command",
	"Error Invalid Command",
	"Invalid Error Code"
};

static void _cyttsp4_pr_status(struct cyttsp4 *ts, int level, int status)
{
	if (status > ERROR_INVALID) status = ERROR_INVALID;
	dbg("%s: status error(%d)=%s\n", __func__, status, ldr_status_string[status]);
}
#endif /* --CY_USE_INCLUDE_FBL */

static u16 _cyttsp4_get_short(u8 *buf)
{
	return ((u16)(*buf) << 8) + *(buf+1);
}

static u8 *_cyttsp4_get_row(struct cyttsp4 *ts,
	u8 *row_buf, u8 *image_buf, int size)
{
	int i;
	for (i = 0; i < size; i++) {
		/* copy a row from the image */
		row_buf[i] = image_buf[i];
	}

	image_buf = image_buf + size;
	return image_buf;
}

static int _cyttsp4_ldr_enter(struct cyttsp4 *ts, struct cyttsp4_dev_id *dev_id)
{
	u16 crc;
	int i = 0;
	size_t cmd_size;
	u8 status_buf[CY_MAX_STATUS_SIZE];
	u8 status = 0;
	int retval = 0;
	/* +1 for TMA400 host sync byte */
	u8 ldr_enter_cmd[CY_CMD_LDR_ENTER_CMD_SIZE+1];

	memset(status_buf, 0, sizeof(status_buf));
	dev_id->bl_ver = 0;
	dev_id->rev_id = 0;
	dev_id->silicon_id = 0;

#ifdef CY_USE_TMA400
	ldr_enter_cmd[i++] = CY_CMD_LDR_HOST_SYNC;
#endif /* --CY_USE_TMA400 */
	ldr_enter_cmd[i++] = CY_START_OF_PACKET;
	ldr_enter_cmd[i++] = CY_CMD_LDR_ENTER;
	ldr_enter_cmd[i++] = 0x00;	/* data len lsb */
	ldr_enter_cmd[i++] = 0x00;	/* data len msb */
#ifdef CY_USE_TMA400
	crc = _cyttsp4_compute_crc(ts, &ldr_enter_cmd[1], i - 1);
	cmd_size = sizeof(ldr_enter_cmd);
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
	crc = _cyttsp4_compute_crc(ts, ldr_enter_cmd, i);
	cmd_size = sizeof(ldr_enter_cmd) - 1;
#endif /* --CY_USE_TMA884 */
	ldr_enter_cmd[i++] = (u8)crc;
	ldr_enter_cmd[i++] = (u8)(crc >> 8);
	ldr_enter_cmd[i++] = CY_END_OF_PACKET;

	mutex_unlock(&ts->data_lock);
	INIT_COMPLETION(ts->int_running);
	retval = _cyttsp4_write_block_data(ts, CY_REG_BASE, cmd_size,
		ldr_enter_cmd, ts->platform_data->addr[CY_LDR_ADDR_OFS],
#ifdef CY_USE_TMA400
		true);
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
		false);
#endif /* --CY_USE_TMA884 */
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: write block failed %d\n", __func__, retval);
		goto _cyttsp4_ldr_enter_exit;
	}

	/* Wait for ISR, get status and lock mutex */
	retval = _cyttsp4_get_status(ts, status_buf,
		CY_CMD_LDR_ENTER_STAT_SIZE, CY_HALF_SEC_TMO_MS);

	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail get status to Enter Loader command r=%d\n",
			__func__, retval);
	} else {
		status = status_buf[CY_STATUS_BYTE];
		if (status == ERROR_SUCCESS) {
			dev_id->bl_ver =
				status_buf[11] << 16 |
				status_buf[10] <<  8 |
				status_buf[9] <<  0;
			dev_id->rev_id =
				status_buf[8] <<  0;
			dev_id->silicon_id =
				status_buf[7] << 24 |
				status_buf[6] << 16 |
				status_buf[5] <<  8 |
				status_buf[4] <<  0;
			retval = 0;
		} else
			retval = -EIO;
#ifdef CY_USE_INCLUDE_FBL
		_cyttsp4_pr_status(ts, CY_DBG_LVL_3, status);
#endif /* --CY_USE_INCLUDE_FBL */
		dev_vdbg(ts->dev, "%s: status=%d bl_ver=%08X rev_id=%02X silicon_id=%08X\n",
			__func__, status, dev_id->bl_ver, dev_id->rev_id, dev_id->silicon_id);
	}

_cyttsp4_ldr_enter_exit:
	return retval;
}

#ifdef CY_USE_TMA400
static int _cyttsp4_ldr_init(struct cyttsp4 *ts)
{
	u16 crc;
	int i = 0;
	int retval = 0;
	/* +1 for TMA400 host sync byte */
	u8 ldr_init_cmd[CY_CMD_LDR_INIT_CMD_SIZE+1];

	ldr_init_cmd[i++] = CY_CMD_LDR_HOST_SYNC;
	ldr_init_cmd[i++] = CY_START_OF_PACKET;
	ldr_init_cmd[i++] = CY_CMD_LDR_INIT;
	ldr_init_cmd[i++] = 0x08;	/* data len lsb */
	ldr_init_cmd[i++] = 0x00;	/* data len msb */
	memcpy(&ldr_init_cmd[i], cyttsp4_security_key,
		sizeof(cyttsp4_security_key));
	i += sizeof(cyttsp4_security_key);
	crc = _cyttsp4_compute_crc(ts, &ldr_init_cmd[1], i - 1);
	ldr_init_cmd[i++] = (u8)crc;
	ldr_init_cmd[i++] = (u8)(crc >> 8);
	ldr_init_cmd[i++] = CY_END_OF_PACKET;

	retval = _cyttsp4_send_cmd(ts, ldr_init_cmd, i, NULL, 0,
		CY_CMD_LDR_INIT_STAT_SIZE, CY_TEN_SEC_TMO_MS);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail ldr init r=%d\n",
			__func__, retval);
	}

	return retval;
}
#endif /* --CY_USE_TMA400 */

struct cyttsp4_hex_image {
	u8 array_id;
	u16 row_num;
	u16 row_size;
	u8 row_data[CY_DATA_ROW_SIZE];
} __packed;

#ifdef CY_USE_TMA884
static int _cyttsp4_ldr_erase_row(struct cyttsp4 *ts,
	struct cyttsp4_hex_image *row_image)
{
	u16 crc;
	int i = 0;
	int retval = 0;
	/* +1 for TMA400 host sync byte */
	u8 ldr_erase_row_cmd[CY_CMD_LDR_ERASE_ROW_CMD_SIZE+1];

#ifdef CY_USE_TMA400
	ldr_erase_row_cmd[i++] = CY_CMD_LDR_HOST_SYNC;
#endif /* --CY_USE_TMA400 */
	ldr_erase_row_cmd[i++] = CY_START_OF_PACKET;
	ldr_erase_row_cmd[i++] = CY_CMD_LDR_ERASE_ROW;
	ldr_erase_row_cmd[i++] = 0x03;	/* data len lsb */
	ldr_erase_row_cmd[i++] = 0x00;	/* data len msb */
	ldr_erase_row_cmd[i++] = row_image->array_id;
	ldr_erase_row_cmd[i++] = (u8)row_image->row_num;
	ldr_erase_row_cmd[i++] = (u8)(row_image->row_num >> 8);
#ifdef CY_USE_TMA400
	crc = _cyttsp4_compute_crc(ts, &ldr_erase_row_cmd[1], i - 1);
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
	crc = _cyttsp4_compute_crc(ts, ldr_erase_row_cmd, i);
#endif /* --CY_USE_TMA884 */
	ldr_erase_row_cmd[i++] = (u8)crc;
	ldr_erase_row_cmd[i++] = (u8)(crc >> 8);
	ldr_erase_row_cmd[i++] = CY_END_OF_PACKET;

	retval = _cyttsp4_send_cmd(ts, ldr_erase_row_cmd, i, NULL, 0,
		CY_CMD_LDR_ERASE_ROW_STAT_SIZE, CY_HALF_SEC_TMO_MS);

	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail erase row=%d r=%d\n",
			__func__, row_image->row_num, retval);
	}
	return retval;
}
#endif

static int _cyttsp4_ldr_parse_row(struct cyttsp4 *ts, u8 *row_buf,
	struct cyttsp4_hex_image *row_image)
{
	u16 i, j;
	int retval = 0;

	if (!row_buf) {
		dev_err(ts->dev,
			"%s parse row error - buf is null\n", __func__);
		retval = -EINVAL;
		goto cyttsp4_ldr_parse_row_exit;
	}

	row_image->array_id = row_buf[CY_ARRAY_ID_OFFSET];
	row_image->row_num = _cyttsp4_get_short(&row_buf[CY_ROW_NUM_OFFSET]);
	row_image->row_size = _cyttsp4_get_short(&row_buf[CY_ROW_SIZE_OFFSET]);

	if (row_image->row_size > ARRAY_SIZE(row_image->row_data)) {
		dev_err(ts->dev,
			"%s: row data buffer overflow\n", __func__);
		retval = -EOVERFLOW;
		goto cyttsp4_ldr_parse_row_exit;
	}

	for (i = 0, j = CY_ROW_DATA_OFFSET;
		i < row_image->row_size; i++)
		row_image->row_data[i] = row_buf[j++];

	retval = 0;

cyttsp4_ldr_parse_row_exit:
	return retval;
}

static int _cyttsp4_ldr_prog_row(struct cyttsp4 *ts,
	struct cyttsp4_hex_image *row_image)
{
	u16 crc;
	int next;
	int data;
	int row_data;
	u16 row_sum = 0;
	size_t data_len;
#ifdef CY_USE_TMA884
	int segment;
#endif /* --CY_USE_TMA884 */
	int retval = 0;

	u8 *cmd = kzalloc(CY_MAX_PACKET_LEN, GFP_KERNEL);

	if (cmd != NULL) {
		row_data = 0;
		row_sum = 0;

#ifdef CY_USE_TMA884
		for (segment = 0; segment <
			(CY_DATA_ROW_SIZE/CY_PACKET_DATA_LEN)-1;
			segment++) {
			next = 0;
			cmd[next++] = CY_START_OF_PACKET;
			cmd[next++] = CY_CMD_LDR_SEND_DATA;
			cmd[next++] = (u8)CY_PACKET_DATA_LEN;
			cmd[next++] = (u8)(CY_PACKET_DATA_LEN >> 8);

			for (data = 0;
				data < CY_PACKET_DATA_LEN; data++) {
				cmd[next] = row_image->row_data
					[row_data++];
				row_sum += cmd[next];
				next++;
			}

			crc = _cyttsp4_compute_crc(ts, cmd, next);
			cmd[next++] = (u8)crc;
			cmd[next++] = (u8)(crc >> 8);
			cmd[next++] = CY_END_OF_PACKET;

			retval = _cyttsp4_send_cmd(ts, cmd, next, NULL,
				0, CY_CMD_LDR_SEND_DATA_STAT_SIZE,
				CY_HALF_SEC_TMO_MS);

			if (retval < 0) {
				dev_err(ts->dev,
			"%s: send row=%d segment=%d"
					" fail r=%d\n",
					__func__, row_image->row_num,
					segment, retval);
				goto cyttsp4_ldr_prog_row_exit;
			}
		}
#endif /* --CY_USE_TMA884 */

		next = 0;
#ifdef CY_USE_TMA400
		cmd[next++] = CY_CMD_LDR_HOST_SYNC;
#endif /* --CY_USE_TMA400 */
		cmd[next++] = CY_START_OF_PACKET;
		cmd[next++] = CY_CMD_LDR_PROG_ROW;
		/*
		 * include array id size and row id size in CY_PACKET_DATA_LEN
		 */
#ifdef CY_USE_TMA400
		data_len = CY_DATA_ROW_SIZE_TMA400;
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
		data_len = CY_PACKET_DATA_LEN;
#endif /* --CY_USE_TMA884 */
		cmd[next++] = (u8)(data_len+3);
		cmd[next++] = (u8)((data_len+3) >> 8);
		cmd[next++] = row_image->array_id;
		cmd[next++] = (u8)row_image->row_num;
		cmd[next++] = (u8)(row_image->row_num >> 8);

		for (data = 0;
			data < data_len; data++) {
			cmd[next] = row_image->row_data[row_data++];
			row_sum += cmd[next];
			next++;
		}

#ifdef CY_USE_TMA400
		crc = _cyttsp4_compute_crc(ts, &cmd[1], next - 1);
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
		crc = _cyttsp4_compute_crc(ts, cmd, next);
#endif /* --CY_USE_TMA884 */
		cmd[next++] = (u8)crc;
		cmd[next++] = (u8)(crc >> 8);
		cmd[next++] = CY_END_OF_PACKET;

		retval = _cyttsp4_send_cmd(ts, cmd, next, NULL, 0,
			CY_CMD_LDR_PROG_ROW_STAT_SIZE, CY_HALF_SEC_TMO_MS);

		if (retval < 0) {
			dev_err(ts->dev,
			"%s: prog row=%d fail r=%d\n",
				__func__, row_image->row_num, retval);
			goto cyttsp4_ldr_prog_row_exit;
		}

	} else {
		dev_err(ts->dev,
			"%s prog row error - cmd buf is NULL\n", __func__);
		retval = -EIO;
	}

cyttsp4_ldr_prog_row_exit:
	if (cmd != NULL)
		kfree(cmd);
	return retval;
}

static int _cyttsp4_ldr_verify_row(struct cyttsp4 *ts,
	struct cyttsp4_hex_image *row_image)
{
	u16 crc;
	int i = 0;
	u8 verify_checksum;
	int retval = 0;
	/* +1 for TMA400 host sync byte */
	u8 ldr_verify_row_cmd[CY_CMD_LDR_VERIFY_ROW_CMD_SIZE+1];

#ifdef CY_USE_TMA400
	ldr_verify_row_cmd[i++] = CY_CMD_LDR_HOST_SYNC;
#endif /* --CY_USE_TMA400 */
	ldr_verify_row_cmd[i++] = CY_START_OF_PACKET;
	ldr_verify_row_cmd[i++] = CY_CMD_LDR_VERIFY_ROW;
	ldr_verify_row_cmd[i++] = 0x03;	/* data len lsb */
	ldr_verify_row_cmd[i++] = 0x00;	/* data len msb */
	ldr_verify_row_cmd[i++] = row_image->array_id;
	ldr_verify_row_cmd[i++] = (u8)row_image->row_num;
	ldr_verify_row_cmd[i++] = (u8)(row_image->row_num >> 8);
#ifdef CY_USE_TMA400
	crc = _cyttsp4_compute_crc(ts, &ldr_verify_row_cmd[1], i - 1);
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
	crc = _cyttsp4_compute_crc(ts, ldr_verify_row_cmd, i);
#endif /* --CY_USE_TMA884 */
	ldr_verify_row_cmd[i++] = (u8)crc;
	ldr_verify_row_cmd[i++] = (u8)(crc >> 8);
	ldr_verify_row_cmd[i++] = CY_END_OF_PACKET;

	retval = _cyttsp4_send_cmd(ts, ldr_verify_row_cmd, i,
		&verify_checksum, 4,
		CY_CMD_LDR_VERIFY_ROW_STAT_SIZE, CY_HALF_SEC_TMO_MS);

	if (retval < 0) {
		dev_err(ts->dev,
			"%s: verify row=%d fail r=%d\n",
			__func__, row_image->row_num, retval);
	}

	return retval;
}

static int _cyttsp4_ldr_verify_chksum(struct cyttsp4 *ts, u8 *app_chksum)
{
	u16 crc;
	int i = 0;
	int retval = 0;
	/* +1 for TMA400 host sync byte */
	u8 ldr_verify_chksum_cmd[CY_CMD_LDR_VERIFY_CHKSUM_CMD_SIZE+1];

#ifdef CY_USE_TMA400
	ldr_verify_chksum_cmd[i++] = CY_CMD_LDR_HOST_SYNC;
#endif /* --CY_USE_TMA400 */
	ldr_verify_chksum_cmd[i++] = CY_START_OF_PACKET;
	ldr_verify_chksum_cmd[i++] = CY_CMD_LDR_VERIFY_CHKSUM;
	ldr_verify_chksum_cmd[i++] = 0x00;	/* data len lsb */
	ldr_verify_chksum_cmd[i++] = 0x00;	/* data len msb */
#ifdef CY_USE_TMA400
	crc = _cyttsp4_compute_crc(ts, &ldr_verify_chksum_cmd[1], i - 1);
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
	crc = _cyttsp4_compute_crc(ts, ldr_verify_chksum_cmd, i);
#endif /* --CY_USE_TMA884 */
	ldr_verify_chksum_cmd[i++] = (u8)crc;
	ldr_verify_chksum_cmd[i++] = (u8)(crc >> 8);
	ldr_verify_chksum_cmd[i++] = CY_END_OF_PACKET;

	retval = _cyttsp4_send_cmd(ts, ldr_verify_chksum_cmd, i,
		app_chksum, 4,
		CY_CMD_LDR_VERIFY_CHKSUM_STAT_SIZE, CY_HALF_SEC_TMO_MS);

	if (retval < 0) {
		dev_err(ts->dev,
			"%s: verify checksum fail r=%d\n",
			__func__, retval);
	}

	return retval;
}

static int _cyttsp4_load_app(struct cyttsp4 *ts, const u8 *fw, int fw_size)
{
	u8 *p;
#ifdef CY_USE_TMA884
	u8 tries;
#endif
	int ret;
	int retval;	/* need separate return value at exit stage */
	struct cyttsp4_dev_id *file_id = NULL;
	struct cyttsp4_dev_id *dev_id = NULL;
	struct cyttsp4_hex_image *row_image = NULL;
	u8 app_chksum;

	u8 *row_buf = NULL;
	size_t image_rec_size;
	size_t row_buf_size = 1024 > CY_MAX_PRBUF_SIZE ? 1024 : CY_MAX_PRBUF_SIZE;
	int row_count = 0;

	dbg_func_in();

#ifdef CY_USE_TMA400
	image_rec_size = CY_DATA_ROW_SIZE_TMA400 + (sizeof(struct cyttsp4_hex_image) - CY_DATA_ROW_SIZE);
#endif /* --CY_USE_TMA400 */

#ifdef CY_USE_TMA884
	image_rec_size = sizeof(struct cyttsp4_hex_image);
#endif /* --CY_USE_TMA884 */

	if (!fw_size || (fw_size % image_rec_size != 0)) {
		dev_err(ts->dev, "%s: Firmware image is misaligned\n", __func__);
		retval = -EINVAL;
		goto _cyttsp4_load_app_exit;
	}

#ifdef CY_USE_WATCHDOG
	_cyttsp4_stop_wd_timer(ts);
#endif

	dev_info(ts->dev, "%s: start load app\n", __func__);

	row_buf = kzalloc(row_buf_size, GFP_KERNEL);
	row_image = kzalloc(sizeof(struct cyttsp4_hex_image), GFP_KERNEL);
	file_id = kzalloc(sizeof(struct cyttsp4_dev_id), GFP_KERNEL);
	dev_id = kzalloc(sizeof(struct cyttsp4_dev_id), GFP_KERNEL);

	if ((row_buf == NULL) || (row_image == NULL) || (file_id == NULL) || (dev_id == NULL)) {
		dev_err(ts->dev, "%s: Unable to alloc row buffers(%p %p %p %p)\n", __func__, row_buf, row_image, file_id, dev_id);
		retval = -ENOMEM;
		goto _cyttsp4_load_app_error_exit;
	}

	p = (u8 *)fw;
	/* Enter Loader and return Silicon ID and Rev */

	retval = _cyttsp4_reset(ts);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Fail reset device r=%d\n", __func__, retval);
		goto _cyttsp4_load_app_exit;
	}
	retval = _cyttsp4_wait_int(ts, CY_TEN_SEC_TMO_MS);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Fail waiting for bootloader interrupt\n", __func__);
		goto _cyttsp4_load_app_exit;
	}

	_cyttsp4_change_state(ts, CY_BL_STATE);

	dev_info(ts->dev, "%s: Send BL Loader Enter\n", __func__);
	retval = _cyttsp4_ldr_enter(ts, dev_id);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Error cannot start Loader (ret=%d)\n", __func__, retval);
		goto _cyttsp4_load_app_error_exit;
	}

	dbg("%s: dev: silicon id=%08X rev=%02X bl=%08X\n", __func__, dev_id->silicon_id, dev_id->rev_id, dev_id->bl_ver);

#ifdef CY_USE_TMA400
	udelay(1000);
	retval = _cyttsp4_ldr_init(ts);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Error cannot init Loader (ret=%d)\n", __func__, retval);
		goto _cyttsp4_load_app_error_exit;
	}
#endif /* --CY_USE_TMA400 */

	dev_info(ts->dev, "%s: Send BL Loader Blocks\n", __func__);

	while (p < (fw + fw_size)) {
		/* Get row */
		dev_dbg(ts->dev, "%s: read row=%d\n", __func__, ++row_count); 
		memset(row_buf, 0, row_buf_size);
		p = _cyttsp4_get_row(ts, row_buf, p, image_rec_size);

		/* Parse row */
		dev_vdbg(ts->dev, "%s: p=%p buf=%p buf[0]=%02X\n", __func__, p, row_buf, row_buf[0]);
		retval = _cyttsp4_ldr_parse_row(ts, row_buf, row_image);

		dev_vdbg(ts->dev, "%s: array_id=%02X row_num=%04X(%d) row_size=%04X(%d)\n", 
			__func__, row_image->array_id, row_image->row_num, row_image->row_num,
			row_image->row_size, row_image->row_size);

		if (retval < 0) {
			dev_err(ts->dev, "%s: Parse Row Error (a=%d r=%d ret=%d\n", 
				__func__, row_image->array_id, row_image->row_num, retval);
			goto bl_exit;
		} else {
			dev_vdbg(ts->dev, "%s: Parse Row (a=%d r=%d ret=%d\n", 
				__func__, row_image->array_id, row_image->row_num, retval);
		}

#ifdef CY_USE_TMA884
		/* erase row */
		tries = 0;
		do {
			retval = _cyttsp4_ldr_erase_row(ts, row_image);
			if (retval < 0) {
				dev_err(ts->dev, "%s: Erase Row Error (array=%d row=%d ret=%d try=%d)\n",
					__func__, row_image->array_id, row_image->row_num, retval, tries);
			}
		} while (retval && tries++ < 5);

		if (retval < 0)
			goto _cyttsp4_load_app_error_exit;
#endif

		/* program row */
		retval = _cyttsp4_ldr_prog_row(ts, row_image);
		if (retval < 0) {
			dev_err(ts->dev, "%s: Program Row Error (array=%d row=%d ret=%d)\n",
				__func__, row_image->array_id, row_image->row_num, retval);
			goto _cyttsp4_load_app_error_exit;
		}

		/* verify row */
		retval = _cyttsp4_ldr_verify_row(ts, row_image);
		if (retval < 0) {
			dev_err(ts->dev, "%s: Verify Row Error (array=%d row=%d ret=%d)\n",
				__func__, row_image->array_id, row_image->row_num, retval);
			goto _cyttsp4_load_app_error_exit;
		}

		dev_vdbg(ts->dev, "%s: array=%d row_cnt=%d row_num=%04X\n", 
			__func__, row_image->array_id, row_count, row_image->row_num);
	}

	/* verify app checksum */
	retval = _cyttsp4_ldr_verify_chksum(ts, &app_chksum);
	dev_dbg(ts->dev, "%s: Application Checksum = %02X r=%d\n", __func__, app_chksum, retval);
	if (retval < 0) {
		dev_err(ts->dev, "%s: ldr_verify_chksum fail r=%d\n", __func__, retval);
		retval = 0;
	}

	/* exit loader */
bl_exit:

	dev_info(ts->dev, "%s: Send BL Loader Terminate\n", __func__); 
	ret = _cyttsp4_ldr_exit(ts);
	if (ret) {
		dev_err(ts->dev, "%s: Error on exit Loader (ret=%d)\n", __func__, ret);
		retval = ret;
		goto _cyttsp4_load_app_error_exit;
	}

	/*
	 * this is a temporary parking state;
	 * the driver will always run startup
	 * after the loader has completed
	 */
	_cyttsp4_change_state(ts, CY_TRANSFER_STATE);
	goto _cyttsp4_load_app_exit;

_cyttsp4_load_app_error_exit:
	_cyttsp4_change_state(ts, CY_BL_STATE);
_cyttsp4_load_app_exit:
	kfree(row_buf);
	kfree(row_image);
	kfree(file_id);
	kfree(dev_id);

	dbg_func_out();

	return retval;
}
#endif /* CY_AUTO_LOAD_FW || CY_USE_FORCE_LOAD || CY_USE_INCLUDE_FBL */

/* Constructs loader exit command and sends via _cyttsp4_send_cmd() */
static int _cyttsp4_ldr_exit(struct cyttsp4 *ts)
{
	u16 crc;
	int i = 0;
	int retval = 0;
	/* +1 for TMA400 host sync byte */
	u8 ldr_exit_cmd[CY_CMD_LDR_EXIT_CMD_SIZE+1];

#ifdef CY_USE_TMA400
	ldr_exit_cmd[i++] = CY_CMD_LDR_HOST_SYNC;
#endif /* --CY_USE_TMA400 */
	ldr_exit_cmd[i++] = CY_START_OF_PACKET;
	ldr_exit_cmd[i++] = CY_CMD_LDR_EXIT;
	ldr_exit_cmd[i++] = 0x00;	/* data len lsb */
	ldr_exit_cmd[i++] = 0x00;	/* data len msb */
#ifdef CY_USE_TMA400
	crc = _cyttsp4_compute_crc(ts, &ldr_exit_cmd[1], i - 1);
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
	crc = _cyttsp4_compute_crc(ts, ldr_exit_cmd, i);
#endif /* --CY_USE_TMA884 */
	ldr_exit_cmd[i++] = (u8)crc;
	ldr_exit_cmd[i++] = (u8)(crc >> 8);
	ldr_exit_cmd[i++] = CY_END_OF_PACKET;

	retval = _cyttsp4_send_cmd(ts, ldr_exit_cmd, i, NULL, 0,
		CY_CMD_LDR_EXIT_STAT_SIZE, 0);

	if (retval < 0) {
		dev_err(ts->dev,
			"%s: BL Loader exit fail r=%d\n",
			__func__, retval);
	}

	dev_vdbg(ts->dev,
		"%s: Exit BL Loader r=%d\n", __func__, retval);

	return retval;
}

#if defined(CY_USE_FORCE_LOAD) || defined(CY_USE_INCLUDE_FBL)
/* Force firmware upgrade */
static void cyttsp4_firmware_cont(const struct firmware *fw, void *context)
{
	int retval = 0;
	struct device *dev = context;
	struct cyttsp4 *ts = dev_get_drvdata(dev);
	u8 header_size = 0;

	mutex_lock(&ts->data_lock);

	if (fw == NULL) {
		dev_err(ts->dev,
			"%s: Firmware not found\n", __func__);
		goto cyttsp4_firmware_cont_exit;
	}

	if ((fw->data == NULL) || (fw->size == 0)) {
		dev_err(ts->dev,
			"%s: No firmware received\n", __func__);
		goto cyttsp4_firmware_cont_release_exit;
	}

	header_size = fw->data[0];
	if (header_size >= (fw->size + 1)) {
		dev_err(ts->dev,
			"%s: Firmware format is invalid\n", __func__);
		goto cyttsp4_firmware_cont_release_exit;
	}
	retval = _cyttsp4_load_app(ts, &(fw->data[header_size + 1]),
		fw->size - (header_size + 1));
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Firmware update failed with error code %d\n",
			__func__, retval);
		_cyttsp4_change_state(ts, CY_IDLE_STATE);
		retval = -EIO;
		goto cyttsp4_firmware_cont_release_exit;
	}

#ifdef CY_USE_INCLUDE_FBL
	ts->debug_upgrade = true;
#endif /* --CY_USE_INCLUDE_FBL */

	retval = _cyttsp4_startup(ts);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Failed to restart IC with error code %d\n",
			__func__, retval);
		_cyttsp4_change_state(ts, CY_IDLE_STATE);
	}

cyttsp4_firmware_cont_release_exit:
	release_firmware(fw);

cyttsp4_firmware_cont_exit:
	ts->waiting_for_fw = false;
	mutex_unlock(&ts->data_lock);
	return;
}
static ssize_t cyttsp4_ic_reflash_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	static const char *wait_fw_ld = "Driver is waiting for firmware load\n";
	static const char *no_fw_ld = "No firmware loading in progress\n";
	struct cyttsp4 *ts = dev_get_drvdata(dev);

	if (ts->waiting_for_fw)
		return snprintf(buf, strlen(wait_fw_ld)+1, wait_fw_ld);
	else
		return snprintf(buf, strlen(no_fw_ld)+1, no_fw_ld);
}
static ssize_t cyttsp4_ic_reflash_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int i;
	int retval = 0;
	struct cyttsp4 *ts = dev_get_drvdata(dev);

	if (ts->waiting_for_fw) {
		dev_err(ts->dev,
			"%s: Driver is already waiting for firmware\n",
			__func__);
		retval = -EALREADY;
		goto cyttsp4_ic_reflash_store_exit;
	}

	/*
	 * must configure FW_LOADER in .config file
	 * CONFIG_HOTPLUG=y
	 * CONFIG_FW_LOADER=y
	 * CONFIG_FIRMWARE_IN_KERNEL=y
	 * CONFIG_EXTRA_FIRMWARE=""
	 * CONFIG_EXTRA_FIRMWARE_DIR=""
	 */

	if (size > CY_BL_FW_NAME_SIZE) {
		dev_err(ts->dev,
			"%s: Filename too long\n", __func__);
		retval = -ENAMETOOLONG;
		goto cyttsp4_ic_reflash_store_exit;
	} else {
		/*
		 * name string must be in alloc() memory
		 * or is lost on context switch
		 * strip off any line feed character(s)
		 * at the end of the buf string
		 */
		for (i = 0; buf[i]; i++) {
			if (buf[i] < ' ')
				ts->fwname[i] = 0;
			else
				ts->fwname[i] = buf[i];
		}
	}

	dev_vdbg(ts->dev,
		"%s: Enabling firmware class loader\n", __func__);

	retval = request_firmware_nowait(THIS_MODULE,
		FW_ACTION_NOHOTPLUG, (const char *)ts->fwname, ts->dev,
		GFP_KERNEL, ts->dev, cyttsp4_firmware_cont);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail request firmware class file load\n",
			__func__);
		ts->waiting_for_fw = false;
		goto cyttsp4_ic_reflash_store_exit;
	} else {
		ts->waiting_for_fw = true;
		retval = size;
	}

cyttsp4_ic_reflash_store_exit:
	return retval;
}
static DEVICE_ATTR(ic_reflash, S_IRUSR | S_IWUSR,
	cyttsp4_ic_reflash_show, cyttsp4_ic_reflash_store);
#endif /* CY_USE_FORCE_LOAD || CY_USE_INCLUDE_FBL */

#ifdef CY_USE_TMA884
static int _cyttsp4_calc_data_crc(struct cyttsp4 *ts, size_t ndata, u8 *pdata,
	u8 *crc_h, u8 *crc_l, const char *name)
{
	int retval = 0;
	u8 *buf = NULL;

	*crc_h = 0;
	*crc_l = 0;

	buf = kzalloc(sizeof(uint8_t) * 126, GFP_KERNEL);
	if (buf == NULL) {
		dev_err(ts->dev,
			"%s: Failed to allocate buf\n", __func__);
		retval = -ENOMEM;
		goto _cyttsp4_calc_data_crc_exit;
	}

	if (pdata == NULL) {
		dev_err(ts->dev,
			"%s: bad data pointer\n", __func__);
		retval = -ENXIO;
		goto _cyttsp4_calc_data_crc_exit;
	}

	if (ndata > 122) {
		dev_err(ts->dev,
			"%s: %s is too large n=%d size=%d\n",
			__func__, name, ndata, 126);
		retval = -EOVERFLOW;
		goto _cyttsp4_calc_data_crc_exit;
	}

	buf[0] = 0x00; /* num of config bytes + 4 high */
	buf[1] = 0x7E; /* num of config bytes + 4 low */
	buf[2] = 0x00; /* max block size w/o crc high */
	buf[3] = 0x7E; /* max block size w/o crc low */

	/* Copy platform data */
	memcpy(&(buf[4]), pdata, ndata);

	/* Calculate CRC */
	_cyttsp4_calc_crc(ts, buf, 126, crc_h, crc_l);

	dev_vdbg(ts->dev,
		"%s: crc=%02X%02X\n", __func__, *crc_h, *crc_l);

_cyttsp4_calc_data_crc_exit:
	kfree(buf);
	return retval;
}
#endif /* --CY_USE_TMA884 */

#ifdef CY_USE_INCLUDE_FBL
#ifdef CY_USE_TMA400
static int _cyttsp4_calc_ic_crc_tma400(struct cyttsp4 *ts,
									   enum cyttsp4_ic_ebid ebid, u8 *crc_h, u8 *crc_l, bool read_back_verify)
{
	u16 crc = 0x0000;
	size_t crc_loc = 0;
	size_t crc_row = 0;
	size_t crc_ofs = 0;
	size_t ndata = 0;
	int row_id = 0;
	u8 *pdata = NULL;
	size_t ntable = 0;
	size_t tsize = 0;
	u8 *ptable = NULL;
	bool match = true;
	int i = 0;
	int retval = 0;

	pdata = kzalloc(ts->ebid_row_size, GFP_KERNEL);
	if (pdata == NULL) {
		dev_err(ts->dev,
			"%s: Fail allocate block buffer\n", __func__);
		retval = -ENOMEM;
		goto _cyttsp4_calc_ic_tch_crc_tma400_exit;
	}

	if (read_back_verify) {
		if (ts->platform_data->sett
			[CY_IC_GRPNUM_TCH_PARM_VAL] == NULL) {
				dev_err(ts->dev,
					"%s: NULL param values table\n",
					__func__);
				goto _cyttsp4_calc_ic_tch_crc_tma400_exit;
			} else if (ts->platform_data->sett
				[CY_IC_GRPNUM_TCH_PARM_VAL]->data == NULL) {
					dev_err(ts->dev,
						"%s: NULL param values table data\n",
						__func__);
					goto _cyttsp4_calc_ic_tch_crc_tma400_exit;
				} else if (ts->platform_data->sett
					[CY_IC_GRPNUM_TCH_PARM_VAL]->size == 0) {
						dev_err(ts->dev,
							"%s: param values table size is 0\n",
							__func__);
						goto _cyttsp4_calc_ic_tch_crc_tma400_exit;
					} else {
						ptable = (u8 *)ts->platform_data->sett
							[CY_IC_GRPNUM_TCH_PARM_VAL]->data;
						tsize = ts->platform_data->sett
							[CY_IC_GRPNUM_TCH_PARM_VAL]->size;
					}
	}

	crc = 0xFFFF;
	row_id = 0;
	dev_vdbg(ts->dev,
		"%s: tch ebid=%d row=%d data:\n", __func__, ebid, row_id);
	retval = _cyttsp4_get_ebid_data_tma400(ts, ebid, row_id, pdata);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail get ebid=%d row=%d data r=%d\n",
			__func__, ebid, row_id, retval);
		retval = -EIO;
		goto _cyttsp4_calc_ic_tch_crc_tma400_exit;
	}
	_cyttsp4_pr_buf(ts, pdata, ts->ebid_row_size, "ebid_data");
	/* determine CRC location */
	crc_loc = (pdata[3] * 256) + pdata[2];
	crc_ofs = crc_loc % ts->ebid_row_size;
	crc_row = crc_loc / ts->ebid_row_size;
	dev_vdbg(ts->dev,
		"%s: tch ebid=%d crc_loc=%08X crc_row=%d crc_ofs=%d data:\n",
		__func__, ebid, crc_loc, crc_row, crc_ofs);

	ndata = 0;
	/* if CRC is in row 0, then the loop is skipped */
	for (row_id = 0; row_id < crc_row; row_id++) {
		dev_vdbg(ts->dev,
			"%s: Get CRC bytes for ebid=%d row=%d crc_row=%d\n",
			__func__, ebid, row_id, crc_row);
		retval = _cyttsp4_get_ebid_data_tma400(ts, ebid, row_id, pdata);
		if (retval < 0) {
			dev_err(ts->dev,
				"%s: Fail get row=%d data r=%d\n",
				__func__, row_id, retval);
			retval = -EIO;
			goto _cyttsp4_calc_ic_tch_crc_tma400_exit;
		}
		_cyttsp4_pr_buf(ts, pdata, ts->ebid_row_size, "ebid_data");
		crc = _cyttsp4_calc_partial_crc(ts,
			pdata, ts->ebid_row_size, crc);
		if (read_back_verify  && (ntable < tsize)) {
			for (i = 0; match && i < ts->ebid_row_size; i++) {
				if (ptable[ntable] != pdata[i]) {
					dev_vdbg(ts->dev,
						"%s: read back err row=%d"
						" table[%d]=%02X"
						" pdata[%d]=%02X\n",
						__func__, row_id,
						ntable, ptable[ntable],
						i, pdata[i]);
					match = false;
				}
				ntable++;
				if (ntable >= tsize) {
					dev_err(ts->dev,
						"%s: row=%d ntbl=%d tsz=%d\n",
						__func__, row_id,
						ntable, tsize);
					break;
				}
			}
		}
		ndata += ts->ebid_row_size;
	}
	/* last row is partial and contains the CRC */
	dev_vdbg(ts->dev,
		"%s: Get CRC bytes for row=%d crc_row=%d\n",
		__func__, crc_row, crc_row);
	retval = _cyttsp4_get_ebid_data_tma400(ts, ebid, crc_row, pdata);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail get row=%d data r=%d\n",
			__func__, crc_row, retval);
		retval = -EIO;
		goto _cyttsp4_calc_ic_tch_crc_tma400_exit;
	}
	_cyttsp4_pr_buf(ts, pdata, ts->ebid_row_size, "ebid_data");
	crc = _cyttsp4_calc_partial_crc(ts, pdata, crc_ofs, crc);
	ndata += crc_ofs;
	dev_vdbg(ts->dev,
		"%s: ndata=%d\n", __func__, ndata);
	if (read_back_verify  && (ntable < tsize)) {
		dev_vdbg(ts->dev,
			"%s: crc_row=%d ntbl=%d tsz=%d crc_ofs=%d\n",
			__func__, crc_row, ntable, tsize, crc_ofs);
		for (i = 0; match && i < crc_ofs; i++) {
			if (ptable[ntable] != pdata[i]) {
				dev_vdbg(ts->dev,
					"%s: read back err crc_row=%d"
					" table[%d]=%02X"
					" pdata[%d]=%02X\n",
					__func__, crc_row,
					ntable, ptable[ntable],
					i, pdata[i]);
				match = false;
			}
			ntable++;
			if (ntable > tsize) {
				dev_err(ts->dev,
					"%s: crc_row=%d ntbl=%d tsz=%d\n",
					__func__, crc_row, ntable, tsize);
				break;
			}
		}
	}
_cyttsp4_calc_ic_tch_crc_tma400_exit:
	*crc_h = crc / 256;
	*crc_l = crc % 256;

	if (pdata != NULL)
		kfree(pdata);
	if (read_back_verify) {
		if (!match)
			retval = -EIO;
	}
	return retval;
}
#endif /* --CY_USE_TMA400 */
#endif /* --CY_USE_INCLUDE_FBL */

#ifdef CY_USE_TMA884
static int _cyttsp4_calc_settings_crc(struct cyttsp4 *ts, u8 *crc_h, u8 *crc_l)
{
	int retval = 0;
	u8 *buf = NULL;
	u8 size = 0;

	buf = kzalloc(sizeof(uint8_t) * 126, GFP_KERNEL);
	if (buf == NULL) {
		dev_err(ts->dev,
			"%s: Failed to allocate buf\n", __func__);
		retval = -ENOMEM;
		goto _cyttsp4_calc_settings_crc_exit;
	}

	if (ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL] == NULL) {
		dev_err(ts->dev,
			"%s: Missing Platform Touch Parameter"
			" values table\n",  __func__);
		retval = -ENXIO;
		goto _cyttsp4_calc_settings_crc_exit;
	}
	if ((ts->platform_data->sett
		[CY_IC_GRPNUM_TCH_PARM_VAL]->data == NULL) ||
		(ts->platform_data->sett
		[CY_IC_GRPNUM_TCH_PARM_VAL]->size == 0)) {
			dev_err(ts->dev,
				"%s: Missing Platform Touch Parameter"
				" values table data\n", __func__);
			retval = -ENXIO;
			goto _cyttsp4_calc_settings_crc_exit;
		}

		size = ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL]->size;

		if (size > 122) {
			dev_err(ts->dev,
				"%s: Platform data is too large\n", __func__);
			retval = -EOVERFLOW;
			goto _cyttsp4_calc_settings_crc_exit;
		}

		buf[0] = 0x00; /* num of config bytes + 4 high */
		buf[1] = 0x7E; /* num of config bytes + 4 low */
		buf[2] = 0x00; /* max block size w/o crc high */
		buf[3] = 0x7E; /* max block size w/o crc low */

		/* Copy platform data */
		memcpy(&(buf[4]),
			ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL]->data,
			size);

		/* Calculate CRC */
		_cyttsp4_calc_crc(ts, buf, 126, crc_h, crc_l);

_cyttsp4_calc_settings_crc_exit:
		kfree(buf);
		return retval;
}
#endif /* --CY_USE_TMA884 */

/* Get IC CRC is operational mode command */
static int _cyttsp4_get_ic_crc(struct cyttsp4 *ts,
							   enum cyttsp4_ic_ebid ebid, u8 *crc_h, u8 *crc_l)
{
	int retval = 0;
	u8 cmd_dat[CY_NUM_DAT + 1];	/* +1 for cmd byte */

	memset(cmd_dat, 0, sizeof(cmd_dat));
	cmd_dat[0] = CY_GET_CFG_BLK_CRC;/* pack cmd */
	cmd_dat[1] = ebid;		/* pack EBID id */

	retval = _cyttsp4_put_cmd_wait(ts, ts->si_ofs.cmd_ofs,
		sizeof(cmd_dat), cmd_dat, CY_HALF_SEC_TMO_MS,
		_cyttsp4_chk_cmd_rdy, NULL,
		ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail Get CRC command r=%d\n",
			__func__, retval);
		goto _cyttsp4_get_ic_crc_exit;
	}

	memset(cmd_dat, 0, sizeof(cmd_dat));
	retval = _cyttsp4_read_block_data(ts, ts->si_ofs.cmd_ofs,
		sizeof(cmd_dat), cmd_dat,
		ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail Get CRC status r=%d\n",
			__func__, retval);
		goto _cyttsp4_get_ic_crc_exit;
	}

	/* Check CRC status and assign values */
	if (cmd_dat[1] != 0) {
		dev_err(ts->dev,
			"%s: Get CRC status=%d error\n",
			__func__, cmd_dat[1]);
		retval = -EIO;
		goto _cyttsp4_get_ic_crc_exit;
	}

	*crc_h = cmd_dat[2];
	*crc_l = cmd_dat[3];

#ifdef CY_USE_TMA400
	retval = _cyttsp4_cmd_handshake(ts);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Command handshake error r=%d\n",
			__func__, retval);
		/* continue anyway; rely on handshake tmo */
		retval = 0;
	}
#endif /* --CY_USE_TMA400 */

_cyttsp4_get_ic_crc_exit:
	return retval;
}

#ifdef CY_USE_TMA400
static int _cyttsp4_startup(struct cyttsp4 *ts)
{
	int tries;
	int retval = 0;
	u8 ic_crc[2];
#ifdef CY_AUTO_LOAD_TOUCH_PARAMS
	u8 table_crc[2];
#endif /* --CY_AUTO_LOAD_TOUCH_PARAMS */
	bool put_all_params_done = false;
	bool upgraded = false;
	bool mddata_updated = false;

	dbg_func_in();

	tries = 0;
	ts->starting_up = true;
	memset(&ts->test, 0, sizeof(struct cyttsp4_test_mode));

#ifdef CY_USE_WATCHDOG
	_cyttsp4_stop_wd_timer(ts);
#endif

_cyttsp4_startup_tma400_restart:

	dev_err(ts->dev, "%s: enter driver_state=%d\n", __func__, ts->driver_state);
	ts->current_mode = CY_MODE_BOOTLOADER;

	retval = _cyttsp4_reset(ts);

	if (retval < 0) {
		dev_err(ts->dev, "%s: Fail reset device r=%d\n", __func__, retval);
		/* continue anyway in case device was already in bootloader */
	}
	/*
	 * Wait for heartbeat interrupt. If we didn't get the CPU quickly, this
	 * may not be the first interupt.
	 */
	dev_err(ts->dev, "%s: wait for first bootloader interrupt\n", __func__);

	retval = _cyttsp4_wait_int(ts, CY_TEN_SEC_TMO_MS);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Fail waiting for bootloader interrupt\n", __func__);
		goto _cyttsp4_startup_tma400_exit;
	}

	/*
	 * exit BL mode and eliminate race between heartbeat and
	 * command / response interrupts
	 */
	_cyttsp4_change_state(ts, CY_EXIT_BL_STATE);

	ts->switch_flag = true;
	retval = _cyttsp4_wait_si_int(ts, CY_TEN_SEC_TMO_MS);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Fail wait switch to Sysinfo r=%d\n", __func__, retval);
		/* continue anyway in case sync missed */
	}

	if (ts->driver_state != CY_SYSINFO_STATE) {
		dev_err(ts->dev, "%s: Fail set sysinfo mode; switch to sysinfo anyway\r", __func__);
		_cyttsp4_change_state(ts, CY_SYSINFO_STATE);
	} else {
		dev_err(ts->dev, "%s: Exit BL ok; now in sysinfo mode\n", __func__); 
		_cyttsp4_pr_state(ts);
	}

 	dev_err(ts->dev, "%s: Read Sysinfo regs and get rev numbers try=%d\n", __func__, tries);
 	retval = _cyttsp4_get_sysinfo_regs(ts);	

	if (retval < 0) {
		dev_err(ts->dev, "%s: Read Block fail -get sys regs (r=%d)\n", __func__, retval);
		dev_err(ts->dev, "%s: Fail to switch from Bootloader to Application r=%d\n", __func__, retval);

		_cyttsp4_change_state(ts, CY_BL_STATE);

		if (upgraded) {
			dev_err(ts->dev, "%s: app failed to launch after platform firmware upgrade\n", __func__);
			retval = -EIO;
			goto _cyttsp4_startup_tma400_exit;
		}

#ifdef CY_AUTO_LOAD_FW
		dev_info(ts->dev, "%s: attempting to reflash IC...\n", __func__);

		if (ts->platform_data->fw->img == NULL || ts->platform_data->fw->size == 0) {
			dev_err(ts->dev, "%s: no platform firmware available for reflashing\n", __func__);
			_cyttsp4_change_state(ts, CY_INVALID_STATE);
			retval = -ENODATA;
			goto _cyttsp4_startup_tma400_exit;
		}
		
		retval = _cyttsp4_load_app(ts, ts->platform_data->fw->img, ts->platform_data->fw->size);
		if (retval) {
			dev_err(ts->dev, "%s: failed to reflash IC (r=%d)\n", __func__, retval);
			_cyttsp4_change_state(ts, CY_INVALID_STATE);
			retval = -EIO;
			goto _cyttsp4_startup_tma400_exit;
		}
		upgraded = true;
		dev_info(ts->dev, "%s: resetting IC after reflashing\n", __func__);
		goto _cyttsp4_startup_tma400_restart; /* Reset the part */
#endif /* --CY_AUTO_LOAD_FW */
	}

#ifdef CY_AUTO_LOAD_FW
#ifdef CY_USE_INCLUDE_FBL
	if (!ts->ic_grptest && !(ts->debug_upgrade)) {
#endif /* --CY_USE_INCLUDE_FBL */

		retval = _cyttsp4_boot_loader(ts, &upgraded);
		if (retval < 0) {
			dev_err(ts->dev, "%s: fail boot loader r=%d)\n", __func__, retval);
			_cyttsp4_change_state(ts, CY_IDLE_STATE);
			goto _cyttsp4_startup_tma400_exit;
		}
		if (upgraded)
			goto _cyttsp4_startup_tma400_restart;

#ifdef CY_USE_INCLUDE_FBL
	}
#endif /* --CY_USE_INCLUDE_FBL */
#endif /* --CY_AUTO_LOAD_FW */

	retval = _cyttsp4_set_mode(ts, CY_CONFIG_MODE);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Fail set config mode 1 r=%d\n", __func__, retval);
		goto _cyttsp4_startup_tma400_bypass_crc_check;
	}

	retval = _cyttsp4_get_ebid_row_size(ts);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Fail get EBID row size; using default r=%d\n", __func__, retval);
	}
	dev_vdbg(ts->dev, "%s: get EBID row size=%d\n", __func__, ts->ebid_row_size);

#ifdef CY_USE_INCLUDE_FBL
	if (ts->ic_grptest)
		goto _cyttsp4_startup_tma400_bypass_crc_check;
#endif /* --CY_USE_INCLUDE_FBL */

	memset(ic_crc, 0, sizeof(ic_crc));
	dev_vdbg(ts->dev, "%s: Read IC CRC values\n", __func__);
	/* Get settings CRC from touch IC */
	retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Fail set operational mode 1 (r=%d)\n", __func__, retval);
		goto _cyttsp4_startup_tma400_exit;
	}
	retval = _cyttsp4_get_ic_crc(ts, CY_TCH_PARM_EBID, &ic_crc[0], &ic_crc[1]);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Fail read ic crc r=%d\n", __func__, retval);
	}

	_cyttsp4_pr_buf(ts, ic_crc, sizeof(ic_crc), "read_ic_crc");

	retval = _cyttsp4_set_mode(ts, CY_CONFIG_MODE);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Fail set config mode 2 r=%d\n", __func__, retval);
		goto _cyttsp4_startup_tma400_exit;
	}

#ifdef CY_AUTO_LOAD_TOUCH_PARAMS
	if (!put_all_params_done) {
		if (ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL] == NULL) {
			dev_err(ts->dev, "%s: missing param table\n", __func__);
			goto _cyttsp4_startup_tma400_bypass_crc_check;
		} else if (ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL]->data == NULL) {
			dev_err(ts->dev, "%s: missing param table data\n", __func__);
			goto _cyttsp4_startup_tma400_bypass_crc_check;
		} else if (ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL]->size == 0) {
			dev_err(ts->dev, "%s: param values table size is 0\n", __func__);
			goto _cyttsp4_startup_tma400_bypass_crc_check;
		}
		_cyttsp_read_table_crc(ts, ts->platform_data->sett[CY_IC_GRPNUM_TCH_PARM_VAL]->data, &table_crc[0], &table_crc[1]);
		_cyttsp4_pr_buf(ts, table_crc, sizeof(table_crc), "read_table_crc");
		if ((ic_crc[0] != table_crc[0]) || (ic_crc[1] != table_crc[1])) {
			retval = _cyttsp4_put_all_params_tma400(ts);
			if (retval < 0) {
				dev_err(ts->dev, "%s: Fail put all params r=%d\n", __func__, retval);
				goto _cyttsp4_startup_tma400_bypass_crc_check;
			}
			put_all_params_done = true;
			goto _cyttsp4_startup_tma400_restart;
		}
	}
#else

	put_all_params_done = true;

#endif /* --CY_AUTO_LOAD_TOUCH_PARAMS */

_cyttsp4_startup_tma400_bypass_crc_check:

	if (!mddata_updated) {
		retval = _cyttsp4_check_mddata_tma400(ts, &mddata_updated);
		if (retval < 0) {
			dev_err(ts->dev, "%s: Fail update MDDATA r=%d\n", __func__, retval);
		} else if (mddata_updated)
			goto _cyttsp4_startup_tma400_restart;
	}

	dev_vdbg(ts->dev, "%s: enter operational mode\n", __func__);
	/* mode=operational mode, state = active_state */
	retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Fail set operational mode 2 (r=%d)\n", __func__, retval);
		goto _cyttsp4_startup_tma400_exit;
	}

	if (ts->was_suspended) {
		ts->was_suspended = false;
		retval = _cyttsp4_enter_sleep(ts);
		if (retval < 0) {
			dev_err(ts->dev, "%s: fail resume sleep r=%d\n", __func__, retval);
		}
	} 

_cyttsp4_startup_tma400_exit:
	ts->starting_up = false;

	dbg_func_out();

	return retval;
}
#endif /* --CY_USE_TMA400 */

#ifdef CY_USE_TMA884
#define CY_IRQ_DEASSERT	1
#define CY_IRQ_ASSERT	0
static int _cyttsp4_startup(struct cyttsp4 *ts)
{
	int retval = 0; 
	int i = 0;
	u8 pdata_crc[2];
	u8 ic_crc[2];
	bool upgraded = false;
	bool mddata_updated = false;
	bool wrote_sysinfo_regs = false;
	bool wrote_settings = false;

	memset(&ts->test, 0, sizeof(struct cyttsp4_test_mode));
#ifdef CY_USE_WATCHDOG
	_cyttsp4_stop_wd_timer(ts);
#endif
_cyttsp4_startup_start:
	memset(pdata_crc, 0, sizeof(pdata_crc));
	memset(ic_crc, 0, sizeof(ic_crc));
	
	_cyttsp4_change_state(ts, CY_BL_STATE);
	dev_vdbg(ts->dev, "%s: enter driver_state=%d\n", __func__, ts->driver_state);

	retval = _cyttsp4_reset(ts);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Fail reset device r=%d\n", __func__, retval);
		/* continue anyway in case device was already in bootloader */
	}

	/* wait for interrupt to set ready completion */
	retval = _cyttsp4_wait_int(ts, CY_TEN_SEC_TMO_MS);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Fail waiting for bootloader interrupt\n", __func__);
		goto _cyttsp4_startup_exit;
	}

	INIT_COMPLETION(ts->si_int_running);
	_cyttsp4_change_state(ts, CY_EXIT_BL_STATE);
	ts->switch_flag = true;
	retval = _cyttsp4_wait_si_int(ts, CY_TEN_SEC_TMO_MS);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Fail wait switch to Sysinfo r=%d\n", __func__, retval);
		/* continue anyway in case sync missed */
	}
	if (ts->driver_state != CY_SYSINFO_STATE)
		_cyttsp4_change_state(ts, CY_SYSINFO_STATE);
	else
		_cyttsp4_pr_state(ts);

	/*
	 * TODO: remove this wait for toggle high when
	 * startup from ES10 firmware is no longer required
	 */
	/* Wait for IRQ to toggle high */
	dev_vdbg(ts->dev, "%s: wait for irq toggle high\n", __func__);
	retval = -ETIMEDOUT;
	for (i = 0; i < CY_DELAY_MAX * 10 * 5; i++) {
		if (ts->platform_data->irq_stat() == CY_IRQ_DEASSERT) {
			retval = 0;
			break;
		}
		mdelay(CY_DELAY_DFLT);
	}
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: timeout waiting for irq to de-assert\n",
			__func__);
		goto _cyttsp4_startup_exit;
	}

	dev_vdbg(ts->dev,
		"%s: read sysinfo 1\n", __func__);
	memset(&ts->sysinfo_data, 0,
		sizeof(struct cyttsp4_sysinfo_data));
	retval = _cyttsp4_read_block_data(ts, CY_REG_BASE,
		sizeof(struct cyttsp4_sysinfo_data), &ts->sysinfo_data,
		ts->platform_data->addr[CY_TCH_ADDR_OFS], true);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Fail to switch from Bootloader "
			"to Application r=%d\n",
			__func__, retval);

		_cyttsp4_change_state(ts, CY_BL_STATE);

		if (upgraded) {
			dev_err(ts->dev,
			"%s: app failed to launch after"
				" platform firmware upgrade\n", __func__);
			retval = -EIO;
			goto _cyttsp4_startup_exit;
		}

		dev_info(ts->dev,
			"%s: attempting to reflash IC...\n", __func__);
		if (ts->platform_data->fw->img == NULL ||
			ts->platform_data->fw->size == 0) {
			dev_err(ts->dev,
			"%s: no platform firmware available"
				" for reflashing\n", __func__);
			_cyttsp4_change_state(ts, CY_INVALID_STATE);
			retval = -ENODATA;
			goto _cyttsp4_startup_exit;
		}
		retval = _cyttsp4_load_app(ts,
			ts->platform_data->fw->img,
			ts->platform_data->fw->size);
		if (retval) {
			dev_err(ts->dev,
			"%s: failed to reflash IC (r=%d)\n",
				__func__, retval);
			_cyttsp4_change_state(ts, CY_INVALID_STATE);
			retval = -EIO;
			goto _cyttsp4_startup_exit;
		}
		upgraded = true;
		dev_info(ts->dev,
			"%s: resetting IC after reflashing\n", __func__);
		goto _cyttsp4_startup_start; /* Reset the part */
	}

	/*
	 * read system information registers
	 * get version numbers and fill sysinfo regs
	 */
	dev_vdbg(ts->dev,
		"%s: Read Sysinfo regs and get version numbers\n", __func__);
	retval = _cyttsp4_get_sysinfo_regs(ts);
	if (retval < 0) {
		dev_err(ts->dev,
			"%s: Read Block fail -get sys regs (r=%d)\n",
			__func__, retval);
		_cyttsp4_change_state(ts, CY_IDLE_STATE);
		goto _cyttsp4_startup_exit;
	}

#ifdef CY_AUTO_LOAD_FW
#ifdef CY_USE_INCLUDE_FBL
	if (!ts->ic_grptest && !(ts->debug_upgrade)) {
#endif /* --CY_USE_INCLUDE_FBL */
		retval = _cyttsp4_boot_loader(ts, &upgraded);
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: fail boot loader r=%d)\n",
				__func__, retval);
			_cyttsp4_change_state(ts, CY_IDLE_STATE);
			goto _cyttsp4_startup_exit;
		}
		if (upgraded)
			goto _cyttsp4_startup_start;
#ifdef CY_USE_INCLUDE_FBL
	}
#endif /* --CY_USE_INCLUDE_FBL */
#endif /* --CY_AUTO_LOAD_FW */

	if (!wrote_sysinfo_regs) {
#ifdef CY_USE_INCLUDE_FBL
		if (ts->ic_grptest)
			goto _cyttsp4_startup_set_sysinfo_done;
#endif /* --CY_USE_INCLUDE_FBL */
		dev_vdbg(ts->dev,
			"%s: Set Sysinfo regs\n", __func__);
		retval = _cyttsp4_set_mode(ts, CY_SYSINFO_MODE);
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: Set SysInfo Mode fail r=%d\n",
				__func__, retval);
			_cyttsp4_change_state(ts, CY_IDLE_STATE);
			goto _cyttsp4_startup_exit;
		}
		retval = _cyttsp4_set_sysinfo_regs(ts, &mddata_updated);
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: Set SysInfo Regs fail r=%d\n",
				__func__, retval);
			_cyttsp4_change_state(ts, CY_IDLE_STATE);
			goto _cyttsp4_startup_exit;
		} else
			wrote_sysinfo_regs = true;
	}

#ifdef CY_USE_INCLUDE_FBL
_cyttsp4_startup_set_sysinfo_done:
#endif /* --CY_USE_INCLUDE_FBL */
	dev_vdbg(ts->dev,
		"%s: enter operational mode\n", __func__);
	retval = _cyttsp4_set_mode(ts, CY_OPERATE_MODE);
	if (retval < 0) {
		_cyttsp4_change_state(ts, CY_IDLE_STATE);
		dev_err(ts->dev,
			"%s: Fail set operational mode (r=%d)\n",
			__func__, retval);
		goto _cyttsp4_startup_exit;
	} else {
#ifdef CY_AUTO_LOAD_TOUCH_PARAMS
#ifdef CY_USE_INCLUDE_FBL
		if (ts->ic_grptest)
			goto _cyttsp4_startup_settings_valid;
#endif /* --CY_USE_INCLUDE_FBL */
		/* Calculate settings CRC from platform settings */
		dev_vdbg(ts->dev,
			"%s: Calculate settings CRC and get IC CRC\n",
			__func__);
		retval = _cyttsp4_calc_settings_crc(ts,
			&pdata_crc[0], &pdata_crc[1]);
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: Unable to calculate settings CRC\n",
				__func__);
			goto _cyttsp4_startup_exit;
		}

		/* Get settings CRC from touch IC */
		retval = _cyttsp4_get_ic_crc(ts, CY_TCH_PARM_EBID,
			&ic_crc[0], &ic_crc[1]);
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: Unable to get settings CRC\n", __func__);
			goto _cyttsp4_startup_exit;
		}

		/* Compare CRC values */
		dev_vdbg(ts->dev,
			"%s: PDATA CRC = 0x%02X%02X, IC CRC = 0x%02X%02X\n",
			__func__, pdata_crc[0], pdata_crc[1],
			ic_crc[0], ic_crc[1]);

		if ((pdata_crc[0] == ic_crc[0]) &&
			(pdata_crc[1] == ic_crc[1]))
			goto _cyttsp4_startup_settings_valid;

		/* Update settings */
		dev_info(ts->dev,
			"%s: Updating IC settings...\n", __func__);

		if (wrote_settings) {
			dev_err(ts->dev,
			"%s: Already updated IC settings\n",
				__func__);
			goto _cyttsp4_startup_settings_valid;
		}

		retval = _cyttsp4_set_op_params(ts, pdata_crc[0], pdata_crc[1]);
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: Set Operational Params fail r=%d\n",
				__func__, retval);
			goto _cyttsp4_startup_exit;
		}

		wrote_settings = true;
#else
		wrote_settings = false;
#endif /* --CY_AUTO_LOAD_TOUCH_PARAMS */
	}

_cyttsp4_startup_settings_valid:
	if (mddata_updated || wrote_settings) {
		dev_info(ts->dev,
			"%s: Resetting IC after writing settings\n",
			__func__);
		mddata_updated = false;
		wrote_settings = false;
		goto _cyttsp4_startup_start; /* Reset the part */
	}
	dev_vdbg(ts->dev,
		"%s: enable handshake\n", __func__);
	retval = _cyttsp4_handshake_enable(ts);
	if (retval < 0)
		dev_err(ts->dev,
			"%s: fail enable handshake r=%d", __func__, retval);

	_cyttsp4_change_state(ts, CY_ACTIVE_STATE);

	if (ts->was_suspended) {
		ts->was_suspended = false;
		retval = _cyttsp4_enter_sleep(ts);
		if (retval < 0) {
			dev_err(ts->dev,
			"%s: fail resume sleep r=%d\n",
				__func__, retval);
		}
	}
	}

_cyttsp4_startup_exit:
	return retval;
}
#endif /* --CY_USE_TMA884 */

#ifdef CY_USE_IRQ_WORKQUEUE
static irqreturn_t cyttsp4_irq(int irq, void *handle)
{	
	struct cyttsp4 *ts = handle;

	if(!queue_work(ts->cyttsp4_wq, &ts->cyttsp4_irq_work)) {
		printk("[+++++ cyttsp4] irq_handler queue_work failed\n");
	}

	return IRQ_HANDLED;
}

static void cyttsp4_irq_work_func(struct work_struct *work)
{
	struct cyttsp4 *ts = container_of(work, struct cyttsp4, cyttsp4_irq_work);

	u8 rep_stat = 0;
	int retval = 0;

	dev_vdbg(ts->dev,"%s: GOT IRQ ps=%d\n", __func__, ts->driver_state);
	mutex_lock(&ts->data_lock);

	dev_vdbg(ts->dev,"%s: DO IRQ ps=%d\n", __func__, ts->driver_state);

	switch (ts->driver_state) {

		case CY_IDLE_STATE:
			if (ts->xy_mode == NULL) {
				/* initialization is not complete; invalid pointers */
				break;
			}

			/* device now available; signal initialization */
			dev_info(ts->dev, "%s: Received IRQ in IDLE state\n", __func__);
			/* Try to determine the IC's current state */
			retval = _cyttsp4_load_status_regs(ts);
			if (retval < 0) {
				dev_err(ts->dev, "%s: Still unable to access IC after IRQ r=%d\n", __func__, retval);
				break;
			}
			rep_stat = ts->xy_mode[ts->si_ofs.rep_ofs + 1];
			if (IS_BOOTLOADERMODE(rep_stat)) {
				dev_info(ts->dev, "%s: BL mode found in IDLE state\n", __func__);
				_cyttsp4_queue_startup(ts, false);
				break;
			}
			dev_err(ts->dev, "%s: interrupt received in IDLE state - try processing touch\n", __func__);
			_cyttsp4_change_state(ts, CY_ACTIVE_STATE);
#ifdef CY_USE_WATCHDOG
			_cyttsp4_start_wd_timer(ts);
#endif
			retval = _cyttsp4_xy_worker(ts);
			if (retval < 0) {
				dev_err(ts->dev, "%s: xy_worker IDLE fail r=%d\n", __func__, retval);
				_cyttsp4_queue_startup(ts, false);
				break;
			}

#ifdef CY_USE_LEVEL_IRQ
			udelay(500);
#endif
			break;

		case CY_READY_STATE:
			complete(&ts->ready_int_running);
			/* do not break; do worker */
		case CY_ACTIVE_STATE:
			if (ts->test.cur_mode == CY_TEST_MODE_CAT) {
				complete(&ts->int_running);
#ifdef CY_USE_LEVEL_IRQ
				udelay(500);
#endif
			} else {
				/* process the touches */
				retval = _cyttsp4_xy_worker(ts);
				if (retval < 0) {
					dev_err(ts->dev, "%s: XY Worker fail r=%d\n", __func__, retval);
					_cyttsp4_queue_startup(ts, false);
				}
			}
			break;

		case CY_SLEEP_STATE:
			dev_vdbg(ts->dev, "%s: Attempt to process touch after enter sleep or unexpected wake event\n", __func__);
			retval = _cyttsp4_wakeup(ts); /* in case its really asleep */
			if (retval < 0) {
				dev_err(ts->dev, "%s: wakeup fail r=%d\n", __func__, retval);
				_cyttsp4_pr_state(ts);
				_cyttsp4_queue_startup(ts, true);
				break;
			}
			/* Put the part back to sleep */
			retval = _cyttsp4_enter_sleep(ts);
			if (retval < 0) {
				dev_err(ts->dev, "%s: fail resume sleep r=%d\n", __func__, retval);
				_cyttsp4_pr_state(ts);
				_cyttsp4_queue_startup(ts, true);
			}
			break;

		case CY_BL_STATE:
		case CY_CMD_STATE:
			complete(&ts->int_running);
#ifdef CY_USE_LEVEL_IRQ
			udelay(1000);
#endif
			break;

		case CY_SYSINFO_STATE:
			complete(&ts->si_int_running);
#ifdef CY_USE_LEVEL_IRQ
			udelay(500);
#endif
			break;
		case CY_EXIT_BL_STATE:
#ifdef CY_USE_LEVEL_IRQ
			udelay(1000);
#endif
			if (ts->switch_flag == true) {
				ts->switch_flag = false;
				retval = _cyttsp4_ldr_exit(ts);
				if (retval < 0) {
					dev_err(ts->dev, "%s: Fail bl exit r=%d\n", __func__, retval);
				} 
				else {
					ts->driver_state = CY_SYSINFO_STATE;
				}
			}
			break;	

		default:
			break;
	}

	mutex_unlock(&ts->data_lock);
	dev_vdbg(ts->dev, "%s: DONE IRQ ps=%d\n", __func__, ts->driver_state);	

	return;
}


#else
static irqreturn_t cyttsp4_irq(int irq, void *handle)
{
	struct cyttsp4 *ts = handle;
	u8 rep_stat = 0;
	int retval = 0;

	dev_vdbg(ts->dev,"%s: GOT IRQ ps=%d\n", __func__, ts->driver_state);
	mutex_lock(&ts->data_lock);

	dev_vdbg(ts->dev,"%s: DO IRQ ps=%d\n", __func__, ts->driver_state);

	switch (ts->driver_state) {

		case CY_IDLE_STATE:
			if (ts->xy_mode == NULL) {
				/* initialization is not complete; invalid pointers */
				break;
			}

			/* device now available; signal initialization */
			dev_info(ts->dev, "%s: Received IRQ in IDLE state\n", __func__);
			/* Try to determine the IC's current state */
			retval = _cyttsp4_load_status_regs(ts);
			if (retval < 0) {
				dev_err(ts->dev, "%s: Still unable to access IC after IRQ r=%d\n", __func__, retval);
				break;
			}
			rep_stat = ts->xy_mode[ts->si_ofs.rep_ofs + 1];
			if (IS_BOOTLOADERMODE(rep_stat)) {
				dev_info(ts->dev, "%s: BL mode found in IDLE state\n", __func__);
				_cyttsp4_queue_startup(ts, false);
				break;
			}
			dev_err(ts->dev, "%s: interrupt received in IDLE state - try processing touch\n", __func__);
			_cyttsp4_change_state(ts, CY_ACTIVE_STATE);
#ifdef CY_USE_WATCHDOG
			_cyttsp4_start_wd_timer(ts);
#endif
			retval = _cyttsp4_xy_worker(ts);
			if (retval < 0) {
				dev_err(ts->dev, "%s: xy_worker IDLE fail r=%d\n", __func__, retval);
				_cyttsp4_queue_startup(ts, false);
				break;
			}

#ifdef CY_USE_LEVEL_IRQ
			udelay(500);
#endif
			break;

		case CY_READY_STATE:
			complete(&ts->ready_int_running);
			/* do not break; do worker */
		case CY_ACTIVE_STATE:
			if (ts->test.cur_mode == CY_TEST_MODE_CAT) {
				complete(&ts->int_running);
#ifdef CY_USE_LEVEL_IRQ
				udelay(500);
#endif
			} else {
				/* process the touches */
				retval = _cyttsp4_xy_worker(ts);
				if (retval < 0) {
					dev_err(ts->dev, "%s: XY Worker fail r=%d\n", __func__, retval);
					_cyttsp4_queue_startup(ts, false);
				}
			}
			break;

		case CY_SLEEP_STATE:
			dev_vdbg(ts->dev, "%s: Attempt to process touch after enter sleep or unexpected wake event\n", __func__);
			retval = _cyttsp4_wakeup(ts); /* in case its really asleep */
			if (retval < 0) {
				dev_err(ts->dev, "%s: wakeup fail r=%d\n", __func__, retval);
				_cyttsp4_pr_state(ts);
				_cyttsp4_queue_startup(ts, true);
				break;
			}
			/* Put the part back to sleep */
			retval = _cyttsp4_enter_sleep(ts);
			if (retval < 0) {
				dev_err(ts->dev, "%s: fail resume sleep r=%d\n", __func__, retval);
				_cyttsp4_pr_state(ts);
				_cyttsp4_queue_startup(ts, true);
			}
			break;

		case CY_BL_STATE:
		case CY_CMD_STATE:
			complete(&ts->int_running);
#ifdef CY_USE_LEVEL_IRQ
			udelay(1000);
#endif
			break;

		case CY_SYSINFO_STATE:
			complete(&ts->si_int_running);
#ifdef CY_USE_LEVEL_IRQ
			udelay(500);
#endif
			break;
		case CY_EXIT_BL_STATE:
#ifdef CY_USE_LEVEL_IRQ
			udelay(1000);
#endif
			if (ts->switch_flag == true) {
				ts->switch_flag = false;
				retval = _cyttsp4_ldr_exit(ts);
				if (retval < 0) {
					dev_err(ts->dev, "%s: Fail bl exit r=%d\n", __func__, retval);
				} 
				else {
					ts->driver_state = CY_SYSINFO_STATE;
				}
			}
			break;	

		default:
			break;
	}

	mutex_unlock(&ts->data_lock);
	dev_vdbg(ts->dev, "%s: DONE IRQ ps=%d\n", __func__, ts->driver_state);	

	return IRQ_HANDLED;
}

#endif

static void _cyttsp4_file_init(struct cyttsp4 *ts)
{

#ifdef CY_USE_INCLUDE_FBL
	if(DebugON == true)
	{
		if (device_create_file(ts->dev, &dev_attr_drv_debug))
			dev_err(ts->dev, "%s: Error, could not create drv_debug\n", __func__);

		if (device_create_file(ts->dev, &dev_attr_drv_flags))
			dev_err(ts->dev, "%s: Error, could not create drv_flags\n", __func__);

		if (device_create_file(ts->dev, &dev_attr_drv_irq))
			dev_err(ts->dev, "%s: Error, could not create drv_irq\n", __func__);
	}
#endif /* --CY_USE_INCLUDE_FBL */

	if (device_create_file(ts->dev, &dev_attr_drv_stat))
		dev_err(ts->dev,
			"%s: Error, could not create drv_stat\n", __func__);

	if (device_create_file(ts->dev, &dev_attr_drv_ver))
		dev_err(ts->dev,
			"%s: Error, could not create drv_ver\n", __func__);

#if defined(CY_USE_FORCE_LOAD) || defined(CY_USE_INCLUDE_FBL)
	if(DebugON == true)
	{
		if (device_create_file(ts->dev, &dev_attr_ic_reflash))
			dev_err(ts->dev, "%s: Error, could not create ic_reflash\n", __func__);
	}
#endif

#ifdef CY_USE_INCLUDE_FBL
	if(DebugON == true)
	{
		if (device_create_file(ts->dev, &dev_attr_hw_irqstat))
			dev_err(ts->dev, "%s: Error, could not create hw_irqstat\n", __func__);

		if (device_create_file(ts->dev, &dev_attr_hw_reset))
			dev_err(ts->dev, "%s: Error, could not create hw_reset\n", __func__);

		if (device_create_file(ts->dev, &dev_attr_hw_recov))
			dev_err(ts->dev, "%s: Error, could not create hw_recov\n", __func__);

		if (device_create_file(ts->dev, &dev_attr_ic_grpdata))
			dev_err(ts->dev, "%s: Error, could not create ic_grpdata\n", __func__);

		if (device_create_file(ts->dev, &dev_attr_ic_grpnum))
			dev_err(ts->dev, "%s: Error, could not create ic_grpnum\n", __func__);

		if (device_create_file(ts->dev, &dev_attr_ic_grpoffset))
			dev_err(ts->dev, "%s: Error, could not create ic_grpoffset\n", __func__);

	}
#endif /* --CY_USE_INCLUDE_FBL */

	if (device_create_file(ts->dev, &dev_attr_ic_ver))
		dev_err(ts->dev, 
			"%s: Cannot create ic_ver\n", __func__);

#ifdef CY_USE_REG_ACCESS
	if (device_create_file(ts->dev, &dev_attr_drv_rw_regid))
		dev_err(ts->dev,
			"%s: Cannot create drv_rw_regid\n", __func__);

	if (device_create_file(ts->dev, &dev_attr_drv_rw_reg_data))
		dev_err(ts->dev,
			"%s: Cannot create drv_rw_reg_data\n", __func__);
#endif

	return;
}

static void _cyttsp4_file_free(struct cyttsp4 *ts)
{
	device_remove_file(ts->dev, &dev_attr_drv_ver);
	device_remove_file(ts->dev, &dev_attr_drv_stat);
	device_remove_file(ts->dev, &dev_attr_ic_ver);
#if defined(CY_USE_FORCE_LOAD) || defined(CY_USE_INCLUDE_FBL)
	device_remove_file(ts->dev, &dev_attr_ic_reflash);
#endif
#ifdef CY_USE_INCLUDE_FBL
	device_remove_file(ts->dev, &dev_attr_ic_grpnum);
	device_remove_file(ts->dev, &dev_attr_ic_grpoffset);
	device_remove_file(ts->dev, &dev_attr_ic_grpdata);
	device_remove_file(ts->dev, &dev_attr_hw_irqstat);
	device_remove_file(ts->dev, &dev_attr_drv_irq);
	device_remove_file(ts->dev, &dev_attr_drv_debug);
	device_remove_file(ts->dev, &dev_attr_drv_flags);
	device_remove_file(ts->dev, &dev_attr_hw_reset);
	device_remove_file(ts->dev, &dev_attr_hw_recov);
#endif /* --CY_USE_INCLUDE_FBL */
#ifdef CY_USE_REG_ACCESS
	device_remove_file(ts->dev, &dev_attr_drv_rw_regid);
	device_remove_file(ts->dev, &dev_attr_drv_rw_reg_data);
#endif
}

void off_hw_setting(void)
{
	dbg("[+++++ CYTTSP4] Off HW Setting\n");

#if defined(CY_USE_SOFT_SUSPEND_RESUME_MODE)
#else	

	gpio_free(GPIO_TOUCH_CHG);
	gpio_free(GPIO_TOUCH_RST);
	gpio_free(GPIO_TOUCH_VDD);

	gpio_set_value(GPIO_TOUCH_VDD, 0);

	msleep(100);
#endif

	dbg("[----- CYTTSP4] Off HW Setting\n");
}

#if 0
static int TSP_Restart(void)
{
	dbg_func_in();

	gpio_set_value(GPIO_TOUCH_RST, 1);
	msleep(20);
	gpio_set_value(GPIO_TOUCH_RST, 0);
	dbg("(skytouch)set TOUCH_RST High.\n");
	msleep(40);
	gpio_set_value(GPIO_TOUCH_RST, 1);
	dbg("(skytouch)set TOUCH_RST Low.\n");
	msleep(20);

	dbg_func_out();

	return 0;
}
#endif

int init_hw_setting(void)
{
	int rc;
	unsigned gpioConfig;

#if CY_USE_PMIC_POWER
	struct regulator *vreg_touch_3_3, *vreg_touch_1_8;	
#endif

	dbg("[+++++ CYTTSP4] Init HW Setting\n");

#ifdef CY_USE_SOFT_SUSPEND_RESUME_MODE
	// GPIO Config: reset pin
	gpio_request(GPIO_TOUCH_RST, "touch_rst_n");
	gpioConfig = GPIO_CFG(GPIO_TOUCH_RST, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA);
	rc = gpio_tlmm_config(gpioConfig, GPIO_CFG_ENABLE);
	if (rc) {
		printk(KERN_ERR "%s: GPIO_TOUCH_RST failed (%d)\n",__func__, rc);
		return 0;
	} 
#endif

	// GPIO Config: interrupt pin
	gpio_request(GPIO_TOUCH_CHG, "touch_chg_int");
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_CHG, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc) {
		printk(KERN_ERR "%s: GPIO_TOUCH_CHG failed (%d)\n",__func__, rc);
		return 0;
	}
	gpio_set_value(GPIO_TOUCH_CHG, 0);	

#if CY_USE_PMIC_POWER 
	
	dbg_line();
	// printk("VCC: %s VDD: %s\n",TOUCH_POWER_VCC, TOUCH_POWER_VDD);

	vreg_touch_1_8 = regulator_get(NULL, TOUCH_POWER_I2C);
	rc = regulator_set_voltage(vreg_touch_1_8, 1800000, 1800000);

	if (rc) { 
		pr_err("set_voltage %s failed, rc=%d\n", TOUCH_POWER_I2C, rc);
		return -EINVAL;
	}

	vreg_touch_3_3 = regulator_get(NULL, TOUCH_POWER_VDD);
	rc = regulator_set_voltage(vreg_touch_3_3, 2900000, 2900000);

	if (rc) { 
		pr_err("set_voltage 8921_l17 failed, rc=%d\n", rc);
		return -EINVAL;
	}

	rc = regulator_enable(vreg_touch_1_8);
	if (rc) { 
		pr_err("regulator_enable vreg_touch_1_8 failed, rc=%d\n", rc);
		return -EINVAL;
	}

	rc = regulator_enable(vreg_touch_3_3);
	if (rc) { 
		pr_err("regulator_enable vreg_touch_3_3 failed, rc=%d\n", rc);
		return -EINVAL;
	}

#else

	// Power On, AVDD
	gpio_request(GPIO_TOUCH_VDD, "touch_power_n");
	gpioConfig = GPIO_CFG(GPIO_TOUCH_VDD, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA);
	rc = gpio_tlmm_config(gpioConfig, GPIO_CFG_ENABLE);
	if (rc) {
		printk(KERN_ERR "%s: GPIO_TOUCH_VDD failed (%d)\n",__func__, rc);
		return 0;
	}
	gpio_set_value(GPIO_TOUCH_VDD, 1);

	
#endif
	
	gpio_set_value(GPIO_TOUCH_CHG, 1);
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_CHG, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc) {
		printk(KERN_ERR "%s: GPIO_TOUCH_VDD failed (%d)\n",__func__, rc);
		return 0;
	}
	msleep(100);

	dbg("[----- CYTTSP4] Init HW Setting\n");

	return 0;
}


static int cyttsp4_open(struct input_dev *dev)
{
	int retval = 0;

	struct cyttsp4 *ts = input_get_drvdata(dev);
	
	dev_dbg(ts->dev, "%s: Open call ts=%p\n", __func__, ts);
	mutex_lock(&ts->data_lock);

	if (!ts->powered) {
		/*
		 * execute complete startup procedure.  After this
		 * call the device is in active state and the worker
		 * is running
		 */

		// added by KJHW
		retval = init_hw_setting();
		if (retval < 0) {
			_cyttsp4_change_state(ts, CY_IDLE_STATE);
			dev_err(ts->dev,	"%s: Init HW setting r=%d\n", __func__, retval);
		}
		
		retval = _cyttsp4_startup(ts);

		/* powered if no hard failure */
		if (retval < 0) {
			ts->powered = false;
			_cyttsp4_change_state(ts, CY_IDLE_STATE);
			dev_err(ts->dev, "%s: startup fail at power on r=%d\n",	__func__, retval);
		} else
			ts->powered = true;

		dev_info(ts->dev, "%s: Powered ON(%d) r=%d\n", __func__, (int)ts->powered, retval);
	}
	mutex_unlock(&ts->data_lock);
	return 0;
}

static void cyttsp4_close(struct input_dev *dev)
{
	/*
	 * close() normally powers down the device
	 * this call simply returns unless power
	 * to the device can be controlled by the driver
	 */
	return;
}

#ifdef CY_USE_SOFT_SUSPEND_RESUME_MODE

static int cyttsp4_hw_reset(void)
{	
	int retval = 0;

	dbg_func_in();

#if defined(CY_USE_SOFT_SUSPEND_RESUME_MODE)
#else
	retval = gpio_request(GPIO_TOUCH_RST, "touch_rst_n");
	if (retval < 0) {
		pr_err("%s: Fail request RST pin r=%d\n", __func__, retval);
		pr_err("%s: Try free RST gpio=%d\n", __func__, GPIO_TOUCH_RST);
		gpio_free(GPIO_TOUCH_RST);
		retval = gpio_request(GPIO_TOUCH_RST, "touch_rst_n");
		if (retval < 0) {
			pr_err("%s: Fail 2nd request RST pin r=%d\n", __func__, retval);
		}
	}
#endif	
		
	if (!(retval < 0)) {
		
		dbg("%s: strobe RST(%d) pin\n", __func__, GPIO_TOUCH_RST);

		gpio_set_value(GPIO_TOUCH_RST, 1);
		msleep(20);
		gpio_set_value(GPIO_TOUCH_RST, 0);
		msleep(40);
		gpio_set_value(GPIO_TOUCH_RST, 1);
		msleep(20);

	//	gpio_free(GPIO_TOUCH_RST);
	}

	dbg_func_out();
	return retval;
}

#define CY_WAKE_DFLT                99	

/* causes wake strobe on INT line
 * in sample board configuration
 * platform data->hw_recov() function
 */
static int cyttsp4_hw_recov(int on)
{
	int retval = 0;

	dbg_func_in();

	switch (on) {
	case 0:
		cyttsp4_hw_reset();
		retval = 0;
		break;
	case CY_WAKE_DFLT:
		retval = gpio_request(GPIO_TOUCH_CHG, NULL);
		if (retval < 0) {
			pr_err("%s: Fail request IRQ pin r=%d\n", __func__, retval);
			pr_err("%s: Try free IRQ gpio=%d\n", __func__, GPIO_TOUCH_CHG);
			gpio_free(GPIO_TOUCH_CHG);
			retval = gpio_request(GPIO_TOUCH_CHG, NULL);
			if (retval < 0) {
				pr_err("%s: Fail 2nd request IRQ pin r=%d\n", __func__, retval);
			}
		}

		if (!(retval < 0)) {
			retval = gpio_direction_output
				(GPIO_TOUCH_CHG, 0);
			if (retval < 0) {
				pr_err("%s: Fail switch IRQ pin to OUT r=%d\n", __func__, retval);
			} else {
				udelay(2000);
				retval = gpio_direction_input(GPIO_TOUCH_CHG);
				if (retval < 0) {
					pr_err("%s: Fail switch IRQ pin to IN r=%d\n", __func__, retval);
				}
			}
			gpio_free(GPIO_TOUCH_CHG);
		}
		break;
	default:
		retval = -ENOSYS;
		break;
	}

	dbg_func_out();
	return retval;
}

static int cyttsp4_irq_stat(void)
{
	int irq_stat = 0;
	int retval = 0;

	dbg_func_in();

	retval = gpio_request(GPIO_TOUCH_CHG, NULL);
	if (retval < 0) {
		pr_err("%s: Fail request IRQ pin r=%d\n", __func__, retval);
		pr_err("%s: Try free IRQ gpio=%d\n", __func__, GPIO_TOUCH_CHG);
		gpio_free(GPIO_TOUCH_CHG);
		retval = gpio_request(GPIO_TOUCH_CHG, NULL);
		if (retval < 0) {
			pr_err("%s: Fail 2nd request IRQ pin r=%d\n", __func__, retval);
		}
	}

	if (!(retval < 0)) {
		irq_stat = gpio_get_value(GPIO_TOUCH_CHG);
		gpio_free(GPIO_TOUCH_CHG);
	}

	dbg_func_out();
	return irq_stat;
}

#endif

void cyttsp4_core_release(void *handle)
{
	struct cyttsp4 *ts = handle;

	dev_dbg(ts->dev, "%s: Release call ts=%p\n",
		__func__, ts);
	if (ts == NULL) {
		dev_err(ts->dev,
			"%s: Null context pointer on driver release\n",
			__func__);
		goto cyttsp4_core_release_exit;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif

#ifdef CY_USE_TOUCH_MONITOR
    touch_monitor_exit();
#endif /* --CY_USE_TOUCH_MONITOR */
	_cyttsp4_file_free(ts);
	if (mutex_is_locked(&ts->data_lock))
		mutex_unlock(&ts->data_lock);
	mutex_destroy(&ts->data_lock);
	free_irq(ts->irq, ts);
	input_unregister_device(ts->input);
	if (ts->cyttsp4_wq) {
		destroy_workqueue(ts->cyttsp4_wq);
		ts->cyttsp4_wq = NULL;
	}

	if (ts->sysinfo_ptr.cydata != NULL)
		kfree(ts->sysinfo_ptr.cydata);
	if (ts->sysinfo_ptr.test != NULL)
		kfree(ts->sysinfo_ptr.test);
	if (ts->sysinfo_ptr.pcfg != NULL)
		kfree(ts->sysinfo_ptr.pcfg);
	if (ts->sysinfo_ptr.opcfg != NULL)
		kfree(ts->sysinfo_ptr.opcfg);
	if (ts->sysinfo_ptr.ddata != NULL)
		kfree(ts->sysinfo_ptr.ddata);
	if (ts->sysinfo_ptr.mdata != NULL)
		kfree(ts->sysinfo_ptr.mdata);
	if (ts->xy_mode != NULL)
		kfree(ts->xy_mode);
	if (ts->xy_data != NULL)
		kfree(ts->xy_data);
	if (ts->xy_data_touch1 != NULL)
		kfree(ts->xy_data_touch1);

	kfree(ts);
cyttsp4_core_release_exit:
	return;
}
EXPORT_SYMBOL_GPL(cyttsp4_core_release);

void *cyttsp4_core_init(struct cyttsp4_bus_ops *bus_ops, struct device *dev, int irq, char *name)
{
	unsigned long irq_flags = 0;
	int i = 0;

	int min = 0;
	int max = 0;
	u16 signal = 0;
	int retval = 0;

	struct input_dev *input_device = NULL;
	struct cyttsp4 *ts = NULL;

	dbg("+++++ cyttsp4 core init\n\n");

	if (dev == NULL) {
		pr_err("%s: Error, dev pointer is Null\n", __func__);
		goto error_alloc_data;
	}

	if (bus_ops == NULL) {
		pr_err("%s: Error, bus_ops Pointer is Null\n", __func__);
		goto error_alloc_data;
	}
	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (ts == NULL) {
		pr_err("%s: Error, kzalloc context memory\n", __func__);
		goto error_alloc_data;
	}

#if defined(CY_USE_FORCE_LOAD) || defined(CY_USE_INCLUDE_FBL)
	ts->fwname = kzalloc(CY_BL_FW_NAME_SIZE, GFP_KERNEL);
	if (ts->fwname == NULL) {
		pr_err("%s: Error, kzalloc fwname\n", __func__);
		goto error_alloc_failed;
	}
#endif

#ifdef CY_USE_INCLUDE_FBL
	ts->pr_buf = kzalloc(CY_MAX_PRBUF_SIZE, GFP_KERNEL);
	if (ts->pr_buf == NULL) {
		pr_err("%s: Error, kzalloc pr_buf\n", __func__);
		goto error_alloc_failed;
	}
#endif /* --CY_USE_INCLUDE_FBL */

	ts->cyttsp4_wq = create_singlethread_workqueue("cyttsp4_resume_startup_wq");

	if (ts->cyttsp4_wq == NULL) {
		pr_err("%s: No memory for cyttsp4_resume_startup_wq\n", __func__);
		goto error_alloc_failed;
	}

	ts->driver_state = CY_INVALID_STATE;
	ts->current_mode = CY_MODE_BOOTLOADER;
	ts->powered = false;
	ts->was_suspended = false;
	ts->switch_flag = false;
	ts->soft_reset_asserted = false;
	ts->num_prv_tch = 0;

	ts->xy_data = NULL;
	ts->xy_mode = NULL;
	ts->xy_data_touch1 = NULL;
	ts->btn_rec_data = NULL;
	memset(&ts->test, 0, sizeof(struct cyttsp4_test_mode));

	ts->dev = dev;
	ts->bus_ops = bus_ops;
	
	// wcjeong-p11309 0411
	//ts->platform_data = dev->platform_data;
	ts->platform_data = &cyttsp4_pdata;

	if (ts->platform_data == NULL) {
		dev_err(ts->dev, "%s: Error, platform data is Null\n", __func__);
		goto error_alloc_failed;
	}

	if (ts->platform_data->frmwrk == NULL) {
		dev_err(ts->dev, "%s: Error, platform data framework is Null\n", __func__);
		goto error_alloc_failed;
	}

	if (ts->platform_data->frmwrk->abs == NULL) {
		dev_err(ts->dev, "%s: Error, platform data framework array is Null\n", __func__);
		goto error_alloc_failed;
	}

	mutex_init(&ts->data_lock);
	init_completion(&ts->int_running);
	init_completion(&ts->si_int_running);
	init_completion(&ts->ready_int_running);
	ts->flags = ts->platform_data->flags;
#if defined(CY_USE_FORCE_LOAD) || defined(CY_USE_INCLUDE_FBL)
	ts->waiting_for_fw = false;
#endif
#ifdef CY_USE_INCLUDE_FBL
	ts->debug_upgrade = false;
	ts->ic_grpnum = CY_IC_GRPNUM_RESERVED;
	ts->ic_grpoffset = 0;
	ts->ic_grptest = false;
	ts->bus_ops->tsdebug = CY_DBG_LVL_0;
#endif /* --CY_USE_INCLUDE_FBL */

#ifdef CY_USE_TMA400
	ts->max_config_bytes = CY_TMA400_MAX_BYTES;
#endif /* --CY_USE_TMA400 */
#ifdef CY_USE_TMA884
	ts->max_config_bytes = CY_TMA884_MAX_BYTES;
#endif /* --CY_USE_TMA884 */

	ts->irq = irq;
	if (ts->irq <= 0) {
		dev_vdbg(ts->dev, "%s: Error, failed to allocate irq\n", __func__);
		goto error_init;
	}

	/* Create the input device and register it. */
	dev_vdbg(ts->dev, "%s: Create the input device and register it\n", __func__);
	input_device = input_allocate_device();
	if (input_device == NULL) {
		dev_err(ts->dev, "%s: Error, failed to allocate input device\n", __func__);
		goto error_init;
	}

	ts->input = input_device;
	
	//	wcjeong-p11309 0411
	//input_device->name = name;
	input_device->name = "qt602240_ts_input";

	snprintf(ts->phys, sizeof(ts->phys)-1, "%s", dev_name(dev));
	input_device->phys = ts->phys;
	input_device->dev.parent = ts->dev;
	ts->bus_type = bus_ops->dev->bus;

#ifdef CY_USE_WATCHDOG
	INIT_WORK(&ts->work, cyttsp4_timer_watchdog);
	setup_timer(&ts->timer, cyttsp4_timer, (unsigned long)ts);
#endif

	input_device->open = cyttsp4_open;
	input_device->close = cyttsp4_close;
	input_set_drvdata(input_device, ts);
	dev_set_drvdata(dev, ts);

	dev_vdbg(ts->dev, "%s: Initialize event signals\n", __func__);
	

	for (i = 0; i < (ts->platform_data->frmwrk->size / CY_NUM_ABS_SET); i++) {
		
		signal = ts->platform_data->frmwrk->abs[(i * CY_NUM_ABS_SET) + CY_SIGNAL_OST];

		if (signal != CY_IGNORE_VALUE) {
			
			min = ts->platform_data->frmwrk->abs[(i * CY_NUM_ABS_SET) + CY_MIN_OST];
			max = ts->platform_data->frmwrk->abs[(i * CY_NUM_ABS_SET) + CY_MAX_OST];
			
			if (i == CY_ABS_ID_OST) {
				/* shift track ids down to start at 0 */
				max = max - min;
				min = min - min;
			}

			input_set_abs_params(input_device, signal, min, max,
				ts->platform_data->frmwrk->abs[(i * CY_NUM_ABS_SET) + CY_FUZZ_OST],
				ts->platform_data->frmwrk->abs[(i * CY_NUM_ABS_SET) + CY_FLAT_OST]);
		}
	}

#ifdef CY_USE_DEBUG_TOOLS
	if (ts->flags & CY_FLAG_FLIP) {
		input_set_abs_params(input_device, ABS_MT_POSITION_X,
			ts->platform_data->frmwrk->abs[(CY_ABS_Y_OST * CY_NUM_ABS_SET) + CY_MIN_OST],
			ts->platform_data->frmwrk->abs[(CY_ABS_Y_OST * CY_NUM_ABS_SET) + CY_MAX_OST],
			ts->platform_data->frmwrk->abs[(CY_ABS_Y_OST * CY_NUM_ABS_SET) + CY_FUZZ_OST],
			ts->platform_data->frmwrk->abs[(CY_ABS_Y_OST * CY_NUM_ABS_SET) + CY_FLAT_OST]);

		input_set_abs_params(input_device, ABS_MT_POSITION_Y,
			ts->platform_data->frmwrk->abs[(CY_ABS_X_OST * CY_NUM_ABS_SET) + CY_MIN_OST],
			ts->platform_data->frmwrk->abs[(CY_ABS_X_OST * CY_NUM_ABS_SET) + CY_MAX_OST],
			ts->platform_data->frmwrk->abs[(CY_ABS_X_OST * CY_NUM_ABS_SET) + CY_FUZZ_OST],
			ts->platform_data->frmwrk->abs[(CY_ABS_X_OST * CY_NUM_ABS_SET) + CY_FLAT_OST]);
	}
#endif /* --CY_USE_DEBUG_TOOLS */

	//	input_set_events_per_packet(input_device, 6 * CY_NUM_TCH_ID);

// 	__set_bit(EV_ABS, input_device->evbit);
// 	__set_bit(EV_REL, input_device->evbit);
// 	__set_bit(EV_KEY, input_device->evbit);
	
	set_bit(EV_SYN, input_device->evbit);
	set_bit(EV_KEY, input_device->evbit);
	set_bit(EV_ABS, input_device->evbit);
	bitmap_fill(input_device->keybit, KEY_MAX);
	bitmap_fill(input_device->relbit, REL_MAX);
	bitmap_fill(input_device->absbit, ABS_MAX);

	set_bit(BTN_TOUCH, input_device->keybit);	

#ifdef SKY_PROCESS_CMD_KEY
	set_bit(KEY_MENU, input_device->keybit);
	set_bit(KEY_HOME, input_device->keybit);
	set_bit(KEY_BACK, input_device->keybit);
	set_bit(KEY_SEARCH, input_device->keybit);
	set_bit(KEY_HOME, input_device->keybit);

	set_bit(KEY_0, input_device->keybit);
	set_bit(KEY_1, input_device->keybit);
	set_bit(KEY_2, input_device->keybit);
	set_bit(KEY_3, input_device->keybit);
	set_bit(KEY_4, input_device->keybit);
	set_bit(KEY_5, input_device->keybit);
	set_bit(KEY_6, input_device->keybit);
	set_bit(KEY_7, input_device->keybit);
	set_bit(KEY_8, input_device->keybit);
	set_bit(KEY_9, input_device->keybit);
	set_bit(0xe3, input_device->keybit); /* '*' */
	set_bit(0xe4, input_device->keybit); /* '#' */

	set_bit(KEY_LEFTSHIFT, input_device->keybit);
	set_bit(KEY_RIGHTSHIFT, input_device->keybit);

	set_bit(KEY_LEFT, input_device->keybit);
	set_bit(KEY_RIGHT, input_device->keybit);
	set_bit(KEY_UP, input_device->keybit);
	set_bit(KEY_DOWN, input_device->keybit);
	set_bit(KEY_ENTER, input_device->keybit);

	set_bit(KEY_SEND, input_device->keybit);
	set_bit(KEY_END, input_device->keybit);

	set_bit(KEY_VOLUMEUP, input_device->keybit);
	set_bit(KEY_VOLUMEDOWN, input_device->keybit);

	set_bit(KEY_CLEAR, input_device->keybit);

	set_bit(KEY_CAMERA, input_device->keybit);
	set_bit(KEY_F3, input_device->keybit);
	//    set_bit(KEY_HOLD, input_device->keybit);
#endif // SKY_PROCESS_CMD_KEY

	// wcjeong - p11309 - for ICS - multi-touch protocol B
    input_mt_init_slots(input_device, CYTTSP_MAX_TOUCHNUM);	
	_pantech_reset_fingerinfo();
	//

	input_set_abs_params(input_device, ABS_X, 0, CY_MAXX, 0, 0);
	input_set_abs_params(input_device, ABS_Y, 0, CY_MAXY, 0, 0);
	input_set_abs_params(input_device, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(input_device, ABS_TOOL_WIDTH, 0, 15, 0, 0);
	input_set_abs_params(input_device, ABS_MT_POSITION_X, 0, CY_MAXX-1, 0, 0);
	input_set_abs_params(input_device, ABS_MT_POSITION_Y, 0, CY_MAXY-1, 0, 0);
	input_set_abs_params(input_device, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input_device, ABS_MT_WIDTH_MAJOR, 0, 15, 0, 0);

	dev_vdbg(ts->dev, "%s: Initialize irq\n", __func__);
#ifdef CY_USE_LEVEL_IRQ
	irq_flags = IRQF_TRIGGER_LOW | IRQF_ONESHOT;
#else
	irq_flags = IRQF_TRIGGER_FALLING | IRQF_ONESHOT;
#endif

#ifdef CY_USE_IRQ_WORKQUEUE
	retval = request_irq(ts->irq, cyttsp4_irq, irq_flags, ts->input->name, ts);
#else
	retval = request_threaded_irq(ts->irq, NULL, cyttsp4_irq, irq_flags, ts->input->name, ts);
#endif

	if (retval < 0) {
		dev_err(ts->dev, "%s: failed to init irq r=%d name=%s\n", __func__, retval, ts->input->name);
		ts->irq_enabled = false;
		goto error_init;
	} 
	else {
		ts->irq_enabled = true;
	}

	retval = input_register_device(input_device);
	if (retval < 0) {
		dev_err(ts->dev, "%s: Error, failed to register input device r=%d\n", __func__, retval);
		goto error_init;
	}

#ifdef CY_USE_TOUCH_MONITOR
    touch_monitor_init();	
#endif /* --CY_USE_TOUCH_MONITOR */

	/* add /sys files */
	_cyttsp4_file_init(ts);

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = cyttsp4_early_suspend;
	ts->early_suspend.resume = cyttsp4_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif


#ifdef SKY_PROCESS_CMD_KEY
	cyttsp4_data = ts;

	retval = misc_register(&touch_event);
	if (retval) {
		dev_err(ts->dev, "%s::::::::: can''t register touch_fops...%d\n", __func__, retval);
	}
#endif

#ifdef TOUCH_IO  
	retval = misc_register(&touch_io);
	if (retval) 
	{
		dev_err(ts->dev,"%s::::::::: can''t register qt602240 misc...%d\n", __func__, retval);
	}
#endif

#ifdef CY_USE_IRQ_WORKQUEUE
	INIT_WORK(&ts->cyttsp4_irq_work, cyttsp4_irq_work_func);
#endif

	INIT_WORK(&ts->cyttsp4_resume_startup_work, cyttsp4_ts_work_func);



	goto no_error;

error_init:
	mutex_destroy(&ts->data_lock);
	if (ts->cyttsp4_wq) {
		destroy_workqueue(ts->cyttsp4_wq);
		ts->cyttsp4_wq = NULL;
	}
error_alloc_failed:
#ifdef CY_USE_INCLUDE_FBL
	if (ts->fwname != NULL) {
		kfree(ts->fwname);
		ts->fwname = NULL;
	}
	if (ts->pr_buf != NULL) {
		kfree(ts->pr_buf);
		ts->pr_buf = NULL;
	}
#endif /* --CY_USE_INCLUDE_FBL */
	if (ts != NULL) {
		kfree(ts);
		ts = NULL;
	}
error_alloc_data:
	dev_err(ts->dev, "%s: Failed Initialization\n", __func__);

no_error:
	Pow_ON_T=1;
	dbg("\n ----- cyttsp4 core init\n\n");

	if ( cyttsp4_need_manual_calibration == true ) {
		cyttsp4_need_manual_calibration = false;
		cyttsp4_manual_calibration(ts);
	}

	return ts;
}
EXPORT_SYMBOL_GPL(cyttsp4_core_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Cypress TrueTouch(R) Standard touchscreen driver core");
MODULE_AUTHOR("Cypress");

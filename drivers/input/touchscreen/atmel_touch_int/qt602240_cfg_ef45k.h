/* -------------------------------------------------------------------- */
/* GPIO, VREG & resolution */
/* -------------------------------------------------------------------- */
#ifdef QT_MULTITOUCH_ENABLE
#define MAX_NUM_FINGER	5
#else
#define MAX_NUM_FINGER	1
#endif

// Screen resolution
#define SCREEN_RESOLUTION_X	720  //800 // 480
#define SCREEN_RESOLUTION_Y	1280 // 800

// Interrupt GPIO Pin
#define GPIO_TOUCH_CHG		11
#define GPIO_TOUCH_RST		50

#if (BOARD_VER >= WS10) 
#define GPIO_TOUCH_POWER 	51
#endif
#define TOUCH_KEY_Y		1300 // 833
#define TOUCH_MENU_MIN		45 // 27	// 0 + 27
#define TOUCH_MENU_MAX		198 // 119	// 27 + 92
#define TOUCH_HOME_MIN		316 // 190	// 480/2 - 50
#define TOUCH_HOME_MAX		483 // 290	// 480/2 + 50
#define TOUCH_BACK_MIN		601 // 361	// 453 - 92
#define TOUCH_BACK_MAX		755 // 453	// 480 - 27


/* -------------------------------------------------------------------- */
/* DEVICE   : mxT224 CONFIGURATION */
/* -------------------------------------------------------------------- */

/* _SPT_USERDATA_T38 INSTANCE 0 */
#define T7_IDLEACQINT			32
#define T7_IDLEACQINT_PLUG		255
#define T7_ACTVACQINT			255
#define T7_ACTV2IDLETO			30

/* _GEN_ACQUISITIONCONFIG_T8 INSTANCE 0 */
#define T8_CHRGTIME               	32
#define T8_TCHDRIFT              	5	//20
#define T8_DRIFTST              	5	
#define T8_TCHAUTOCAL            	0
#define T8_SYNC                  	0
#define T8_ATCHCALST             	9
#define T8_ATCHCALSTHR           	35
#define T8_ATCHFRCCALTHR         	50         
#define T8_ATCHFRCCALRATIO       	20      

/* _TOUCH_MULTITOUCHSCREEN_T9 INSTANCE 0 */
#define T9_CTRL                         143
#define T9_XORIGIN                      0
#define T9_YORIGIN                      0
#define T9_XSIZE                        19
#define T9_YSIZE                        11
#define T9_AKSCFG                       0
#define T9_BLEN                         16
#define T9_TCHTHR                       50
#define T9_TCHDI                      	2
#define T9_ORIENT                       1
#define T9_MRGTIMEOUT                   10
#define T9_MOVHYSTI                     10 // 1
#define T9_MOVHYSTN                     2
#define T9_MOVFILTER                  18
#define T9_NUMTOUCH                     MAX_NUM_FINGER
#define T9_MRGHYST                      10
#define T9_MRGTHR                       25 // 0
#define T9_AMPHYST                      10
#define T9_XRANGE                       1279 // (1280-1)    // 898 = (97.65/87.1) * 800 
#define T9_YRANGE                       719 //  (720-1)    
#define T9_XLOCLIP                      0
#define T9_XHICLIP                      0
#define T9_YLOCLIP                      0
#define T9_YHICLIP                      0
#define T9_XEDGECTRL                    0
#define T9_XEDGEDIST                    0
#define T9_YEDGECTRL                    0
#define T9_YEDGEDIST                    0
#define T9_JUMPLIMIT                    20 
#define T9_TCHHYST                 (T9_TCHTHR/4)  /* V2.0 or MXT224E added */
#define T9_XPITCH                   	0  /* MXT224E added */
#define T9_YPITCH                   	0  /* MXT224E added */
#define T9_NEXTTCHDI                	2     

/*
//Multitouch screen config
touch_multitouchscreen_t9_config_t touchscreen_config_default = {
.ctrl		= T9_CTRL,
.xorigin	= T9_XORIGIN,
.yorigin	= T9_YORIGIN,
.xsize		= T9_XSIZE,
.ysize		= T9_YSIZE,
.akscfg		= T9_AKSCFG,
.blen		= T9_BLEN,
.tchthr		= T9_TCHTHR,
.tchdi		= T9_TCHDI,
.orient		= T9_ORIENT,
.mrgtimeout	= T9_MRGTIMEOUT,
.movhysti	= T9_MOVHYSTI,
.movhystn	= T9_MOVHYSTN,
.movfilter	= T9_MOVFILTER,
.numtouch	= MAX_NUM_FINGER,
.mrghyst	= T9_MRGHYST,
.mrgthr		= T9_MRGTHR,
.amphyst	= T9_AMPHYST,
.xrange		= T9_XRANGE,
.yrange		= T9_YRANGE,
.xloclip	= T9_XLOCLIP,
.xhiclip	= T9_XHICLIP,
.yloclip	= T9_YLOCLIP,
.yhiclip	= T9_YHICLIP,
.xedgectrl	= T9_XEDGECTRL,
.xedgedist	= T9_XEDGEDIST,
.yedgectrl	= T9_YEDGECTRL,
.yedgedist	= T9_YEDGEDIST,
.jumplimit	= T9_JUMPLIMIT,
.tchhyst	= T9_TCHHYST, // PLUG: (T9_TCHTHR_PLUG/4);
.xpitch		= T9_XPITCH,	
.ypitch		= T9_YPITCH,
.nexttchdi	= T9_NEXTTCHDI
};
*/
/* [TOUCH_KEYARRAY_T15 INSTANCE 0]    */
#define T15_CTRL                        0
#define T15_XORIGIN                     0
#define T15_YORIGIN                     0
#define T15_XSIZE                       0
#define T15_YSIZE                       0
#define T15_AKSCFG                      0
#define T15_BLEN                        0
#define T15_TCHTHR                      0
#define T15_TCHDI                       0
#define T15_RESERVED_0                  0
#define T15_RESERVED_1                  0

/*  [SPT_COMMSCONFIG_T18 INSTANCE 0]        */
#define T18_CTRL                        0
#define T18_COMMAND                     0

/* _SPT_GPIOPWM_T19 INSTANCE 0 */
#define T19_CTRL                        0
#define T19_REPORTMASK                  0
#define T19_DIR                         0
#define T19_INTPULLUP                   0
#define T19_OUT                         0
#define T19_WAKE                        0
#define T19_PWM                         0
#define T19_PERIOD                      0
#define T19_DUTY_0                      0
#define T19_DUTY_1                      0
#define T19_DUTY_2                      0
#define T19_DUTY_3                      0
#define T19_TRIGGER_0                   0
#define T19_TRIGGER_1                   0
#define T19_TRIGGER_2                   0
#define T19_TRIGGER_3                   0

/* [TOUCH_PROXIMITY_T23 INSTANCE 0] */
#define T23_CTRL                        0
#define T23_XORIGIN               	0
#define T23_YORIGIN               	0
#define T23_XSIZE                 	0
#define T23_YSIZE                 	0
#define T23_RESERVED              	0
#define T23_BLEN                  	0
#define T23_FXDDTHR               	0
#define T23_FXDDI                 	0
#define T23_AVERAGE               	0
#define T23_MVNULLRATE            	0
#define T23_MVDTHR                	0
/* [SPT_SELFTEST_T25 INSTANCE 0] */
#define T25_CTRL                        0
#define T25_CMD                         0
#define T25_SIGLIM_0_UPSIGLIM        	0
#define T25_SIGLIM_0_LOSIGLIM        	0
#define T25_SIGLIM_1_UPSIGLIM        	0
#define T25_SIGLIM_1_LOSIGLIM        	0
#define T25_SIGLIM_2_UPSIGLIM        	0
#define T25_SIGLIM_2_LOSIGLIM        	0

/* SPT_USERDATA_T38 INSTANCE 0 */
#define T38_USERDATA0           	0
#define T38_USERDATA1           	0 /* CAL_THR */
#define T38_USERDATA2           	0 /* num_of_antitouch */
#define T38_USERDATA3           	0 /* max touch for palm recovery  */
#define T38_USERDATA4           	0 /* MXT_ADR_T8_ATCHFRCCALRATIO for normal */
#define T38_USERDATA5           	0     
#define T38_USERDATA6           	0 
#define T38_USERDATA7           	0 /* max touch for check_auto_cal */

#define T40_CTRL                	0
#define T40_XLOGRIP             	0
#define T40_XHIGRIP             	0
#define T40_YLOGRIP             	0
#define T40_YHIGRIP					0

/* PROCI_TOUCHSUPPRESSION_T42 */

#define T42_CTRL                	0
#define T42_APPRTHR             	0   /* 0 (TCHTHR/4), 1 to 255 */
#define T42_MAXAPPRAREA         	0   /* 0 (40ch), 1 to 255 */
#define T42_MAXTCHAREA          	0   /* 0 (35ch), 1 to 255 */
#define T42_SUPSTRENGTH         	0   /* 0 (128), 1 to 255 */
#define T42_SUPEXTTO            	0  /* 0 (never expires), 1 to 255 (timeout in cycles) */
#define T42_MAXNUMTCHS          	0  /* 0 to 9 (maximum number of touches minus 1) */
#define T42_SHAPESTRENGTH       	0  /* 0 (10), 1 to 31 */

/* SPT_CTECONFIG_T46  */
#define T46_CTRL                	4//0     /*Reserved */
#define T46_MODE                	3 /*3*/     /*0: 16X14Y, 1: 17X13Y, 2: 18X12Y, 3: 19X11Y, 4: 20X10Y, 5: 21X15Y, 6: 22X8Y, */
#define T46_IDLESYNCSPERX       	16//16/*40*/
#define T46_ACTVSYNCSPERX       	24//48/*40*/
#define T46_ADCSPERSYNC         	0 
#define T46_PULSESPERADC        	0     /*0:1  1:2   2:3   3:4 pulses */
#define T46_XSLEW               	1/*0*/     /*0:500nsec,  1:350nsec */
#define T46_SYNCDELAY           	0 

/* PROCI_STYLUS_T47 */              
#define T47_CTRL                	0
#define T47_CONTMIN             	0
#define T47_CONTMAX             	0
#define T47_STABILITY           	0
#define T47_MAXTCHAREA          	0
#define T47_AMPLTHR             	0
#define T47_STYSHAPE            	0
#define T47_HOVERSUP            	0
#define T47_CONFTHR             	0
#define T47_SYNCSPERX           	0

/* PROCG_NOISESUPPRESSION_T48  */
#define T48_CTRL                	1  
#define T48_CFG                 	132 
#define T48_CALCFG              	96
#define T48_CALCFG_PLUG             	112
#define T48_BASEFREQ            	0  
#define T48_RESERVED0           	0  
#define T48_RESERVED1           	0  
#define T48_RESERVED2           	0  
#define T48_RESERVED3           	0  
#define T48_MFFREQ_2            	10 
#define T48_MFFREQ_3            	20  
#define T48_RESERVED4           	0  
#define T48_RESERVED5           	0  
#define T48_RESERVED6           	0  
#define T48_GCACTVINVLDADCS     	6  
#define T48_GCIDLEINVLDADCS     	6  
#define T48_RESERVED7           	0  
#define T48_RESERVED8           	0  
#define T48_GCMAXADCSPERX       	100
#define T48_GCLIMITMIN          	6//4  
#define T48_GCLIMITMAX          	64 
#define T48_GCCOUNTMINTGT       	10 
#define T48_MFINVLDDIFFTHR      	32//0 
#define T48_MFINCADCSPXTHR      	5  
#define T48_MFERRORTHR          	38 
#define T48_SELFREQMAX          	8//20 
#define T48_RESERVED9           	0  
#define T48_RESERVED10          	0  
#define T48_RESERVED11          	0  
#define T48_RESERVED12          	0  
#define T48_RESERVED13          	0  
#define T48_RESERVED14          	0  
#define T48_BLEN                	0  
#define T48_TCHTHR              	60
#define T48_TCHDI               	3  
#define T48_MOVHYSTI            	10  
#define T48_MOVHYSTN            	2  
#define T48_MOVFILTER           	16 
#define T48_NUMTOUCH            	5  
#define T48_MRGHYST             	20 
#define T48_MRGTHR              	25
#define T48_XLOCLIP             	0
#define T48_XHICLIP             	0 
#define T48_YLOCLIP             	0 
#define T48_YHICLIP             	0 
#define T48_XEDGECTRL           	148
#define T48_XEDGEDIST           	55
#define T48_YEDGECTRL           	0
#define T48_YEDGEDIST           	0
#define T48_JUMPLIMIT           	13
#define T48_TCHHYST             	15
#define T48_NEXTTCHDI           	0











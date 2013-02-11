#ifndef CUST_PANTECH_H
#define CUST_PANTECH_H

#include "BOARD_REV.h"

#if defined(T_CHEETAH)
    #define CONFIG_PANTECH_CHEETAH_BOARD
#endif

/* Emergency Dload USB */
/* define after merging dload module #define FEATURE_PANTECH_DLOAD_USB*/

/*******************************************************************************
  **  PDL (LK(emergency), bootimage(phoneinfo), KERNEL(idle download))
  *******************************************************************************/
#define FEATURE_PANTECH_PDL_DLOADINFO
#define FEATURE_PANTECH_PDL_DLOAD
#define FEATURE_PANTECH_FLASH_ACCESS
#define FEATURE_PANTECH_DLOAD_USB
#define FEATURE_PANTECH_REBOOT_FOR_IDLE_DL
#define FEATURE_PANTECH_GPT_RECOVERY     //chjeon20120412@LS1 add

/*************************************************************************/
/*                            PANTECH STABILITY		                     */
/*************************************************************************/
#define F_PANTECH_VZW_PS_STABILITY_AT_CMD

#ifdef __BOOTBUILD__
/*
    ONLY BOOT BUILD FEATURE
*/

/*
      !!!!!!!!!!!!!!!!!! MUST BE DEFINED AS FOLLOWS (ONLY MODEM)!!!!!!!!!!!!!!!!!!
      FEATURE_{COMPANY_NAME}_{FUNCTION_NAME}_{ACTION_NAME}
      ex) PMIC function.

      #define FEATURE_PANTECH_PMIC
*/
// TO DO ..

#define FEATURE_PANTECH_BOOT_PMIC

#if defined(FEATURE_PANTECH_BOOT_PMIC)
#define FEATURE_PANTECH_BOOT_PMIC_POWER_ON_PROCESS
#define FEATURE_PANTECH_BOOT_PMIC_POWER_ON_MPP
#endif

#define FEATURE_PANTECH_BOOT_CHARGER
#if defined(FEATURE_PANTECH_BOOT_CHARGER)
#define FEATURE_PANTECH_BOOT_CHARGER_NO_WAIT
#endif

#define FEATURE_PANTECH_ERR_CRASH_LOGGING

/*
 * Caution!!! Enable before FEATURE_PANTECH_ERR_CRASH_LOGGING must enable under feature
 * by tarial 20120111
 */
#ifdef FEATURE_PANTECH_ERR_CRASH_LOGGING
//#define FEATURE_QHSUSB_HDLC_CDCACM   // tarial 20120111 add for USB DUMP for Modem port 
#endif

#define FEATURE_PANTECH_DDR_TINIT3_MODIFY  // 20120131 jylee , Tinit3 

#endif /* __BOOTBUILD__ */

#ifdef __MODEMBUILD__
/* 
    ONLY MODEM BUILD FEATURE
*/

/*
      !!!!!!!!!!!!!!!!!! MUST BE DEFINED AS FOLLOWS (ONLY MODEM)!!!!!!!!!!!!!!!!!!
      FEATURE_{COMPANY_NAME}_{FUNCTION_NAME}_{ACTION_NAME}
      ex) PMIC function.

      #define FEATURE_PANTECH_PMIC
*/

// TO DO ..

#define FEATURE_PANTECH_MODEL                       //chjeon20111031@LS1 add CS tool.

#endif /* __MODEMBUILD__ */


#if !defined(__KERNELBUILD__) && !defined(__MODEMBUILD__)
/*
    ONLY ANROID BUILD FEATURE
*/

/*
      !!!!!!!!!!!!!!!!!! MUST BE DEFINED AS FOLLOWS (ANDROID)!!!!!!!!!!!!!!!!!!
      CONFIG_{COMPANY_NAME}_{FUNCTION_NAME}_{ACTION_NAME}
      ex) PMIC function.
      #define CONFIG_PANTECH_PMIC

      for using BOTH (android & kernel) definition, please read engineer note about chapter 5 Arm Linux Kernel.

      IF YOU ADD FEATURE IN KERNEL , YOU CHECK THE RELEASE ENGINNER NOTE

      __KERNELBUILD__ :  for avoid redefined , this is predefined name in kernel makefile.

*/

/* TO DO define */
#define FEATURE_PANTECH_BMS_TEST   // equals 'CONFIG_PANTECH_BMS_TEST' at \LINUX\android\kernel\arch\arm\mach-msm\cust\Kconfig
/* [LS2_SYS_KIM.DONGSU 20110927] Enable AT command for OEM stability test */
#define FEATURE_PANTECH_STABILITY_AT_COMMAND
#define CONFIG_PANTECH_ERR_CRASH_LOGGING
#endif /* !defined(__KERNELBUILD__) && !defined(__MODEMBUILD__) */


#if !defined(__KERNELBUILD__)
/*
  MODEM and ANDROID Feature 
*/



#endif /* !defined(__KERNELBUILD__) */

#if !defined(__KERNELBUILD__) && !defined(__MODEMBUILD__) && !defined(__BOOTBUILD__)
/* TO DO define */

#define CONFIG_PANTECH

#include "CUST_PANTECH_CAMERA.h"

/*******************************************************************************
**  Display
*******************************************************************************/
#include "CUST_PANTECH_DISPLAY.h"

/*******************************************************************************
**  TDMB
*******************************************************************************/
#include "CUST_PANTECH_TDMB.h"

/*******************************************************************************
**  USER DATA REBUILDING VERSION
*******************************************************************************/
#define FEATURE_SKY_USER_DATA_VER
#define FEATURE_SKY_FAT16_FOR_SBL3
//20111220 jwheo Data Encryption
#define FEATURE_SKY_DATA_ENCRYPTION
//20120117 jwheo SD Card Block Encryption
#if defined(T_OSCAR) || defined(T_MAGNUS) 
#define FEATURE_SKY_SD_BLOCK_ENCRYPTION
#endif
//20120701 ydkim Copy Preload data
#if defined(T_MAGNUS) /* || defined(T_EF44S)*/
#define F_PANTECH_COPY_PRELOAD_DATA
//20120713 p14527 Use Preload Partition
#if 0 /* defined(T_EF44S) */ 
#define F_PANTECH_USE_PRELOAD_PARTITION
#endif
#endif

// 20120516 - App CRC CHECK For Factory Command(LS1)
#define F_PANTECH_APP_CRC_CHECK

#include "CUST_PANTECH_MMP.h"

/****************************************************
** SOUND
****************************************************/
#include "CUST_PANTECH_SOUND.h"

/****************************************************
** MMC(eMMC, MicroSD)
****************************************************/
#if defined(PANTECH_STORAGE_INTERNAL_FAT)
#define FEATURE_SKY_MMC
#endif

/*************************************************************************/
/****************************  PANTECH UIM ********************************/
/*************************************************************************/
#define F_PANTECH_UIM_TESTMENU


/*******************************************************************************
**  FACTORY_COMMAND
*******************************************************************************/
#define FEATURE_PANTECH_FACTORY_COMMAND
#ifdef FEATURE_PANTECH_FACTORY_COMMAND
#define PANTECH_DIAG_MSECTOR
#define FEATURE_PANTECH_CS_AUTO_TAKEOVER
#define FEATURE_PANTECH_BT_FC
#define FEATURE_PANTECH_WLAN_FC
#define F_SKYLCD_FACTORY_PROCESS_CMD
#define FEATURE_PANTECH_MEDIA_FILE_CHECK
#if defined(T_MAGNUS)
#define FEATURE_PANTECH_VERSION_CHECK	// 20120731 LS1_hskim add for Factory Command ver 10.58 
#endif
#endif
#if defined(T_VEGAPVW) || defined(T_MAGNUS)
#else
#define FEATURE_PANTECH_VOLUME_CTL          // LS2-p11309
#endif

/*******************************************************************************
**  RAWDATA PARTITION ACCESS, FOR BACKUP
*******************************************************************************/
#define FEATURE_SKY_RAWDATA_ACCESS

#define PANTECH_DIAG_MSECTOR

/*******************************************************************************
**  GOTA
*******************************************************************************/
#include "CUST_PANTECH_GOTA.h"

/*******************************************************************************
**  PMIC
*******************************************************************************/
#define CONFIG_A_PANTECH_PMIC
#if defined(CONFIG_A_PANTECH_PMIC)
#define CONFIG_A_PANTECH_PMIC_SHARED_DATA
#define CONFIG_A_PANTECH_PMIC_HW_REVISION
#define CONFIG_A_PANTECH_PMIC_SILENT_BOOT
#define CONFIG_A_PANTECH_PMIC_RESET_REASON
#define CONFIG_A_PANTECH_PMIC_THERM
#if defined(T_EF44S)
#if(BOARD_VER >= WS20)
#define CONFIG_A_PANTECH_MAX17058
#endif
#elif defined(T_MAGNUS)
#define CONFIG_A_PANTECH_MAX17058
#elif defined(T_SIRIUSLTE)
#define CONFIG_A_PANTECH_MAX17058
#elif defined(T_VEGAPVW)
#define CONFIG_A_PANTECH_MAX17058
#define CONFIG_A_PANTECH_MAX17058_GPIO_CONTROL
#endif
#endif

#define FEATURE_PANTECH_CHARGER
#if defined(FEATURE_PANTECH_CHARGER)
#define FEATURE_PANTECH_CHARGER_OFFLINE
#endif /* FEATURE_PANTECH_CHARGER */

/****************************************************
** POWER ON/OFF REASON COUNT
****************************************************/
#define FEATURE_PANTECH_PWR_ONOFF_REASON_CNT

/*******************************************************************************
 **  WIDEVINE DRM
*******************************************************************************/
#define FEATURE_PANTECH_WIDEVINE_DRM

/****************************************************
** OPEN MOBILE API for NFC
****************************************************/
//120420. PS5_MAGNUS_NFCPatch for OPEN_MOBILE_API
#if defined(T_MAGNUS)
#define FEATURE_PANTECH_OPEN_MOBILE_API
#ifdef FEATURE_PANTECH_OPEN_MOBILE_API
#define FEATURE_PANTECH_SMART_CARD_API
#define FEATURE_PANTECH_SMART_CARD_API_EMULATOR
//#define FEATURE_PANTECH_BIP_EXTENSION
#define FEATURE_PANTECH_ACCESS_CONTROL
#endif
#endif

/*******************************************************************************
 **  PANTECH CERTIFICATION FOR Image_verify
*******************************************************************************/
#define FEATURE_PANTECH_KEY_CERTIFICATION

/*******************************************************************************
 **  F_PANTECH_ASTOOL //lsi@ls1.20120509
*******************************************************************************/
#if defined(T_SIRIUSLTE)
#define F_PANTECH_ASTOOL
#endif

/* 120421 LS1-JHM modified : user data backup */
#if defined(T_EF44S)
#define FEATURE_CS_USERDATA_BACKUP
#endif

#endif /*!defined(__KERNELBUILD__) && !defined(__MODEMBUILD__) && !defined(__BOOTBUILD__)*/


/*******************************************************************************
**  WLAN
*******************************************************************************/
#if defined(T_VEGAPVW) || defined(T_SIRIUSLTE)
#define FEATURE_PANTECH_WLAN_WCN3660
#define FEATURE_PANTECH_WLAN_PROCESS_CMD
#define FEATURE_PANTECH_WLAN_TESTMENU
#define FEATURE_PANTECH_WLAN_RAWDATA_ACCESS
#define FEATURE_PANTECH_WLAN_MAC_ADDR_BACKUP_WCN3660
#define FEATURE_PANTECH_WLAN_QCOM_WOW
#define FEATURE_PANTECH_WLAN_FOUR_MAC_ADDRESS_WCN3660  // 20120204 thkim_wifi This feauture is applied to MSM8960-WCN3660 MODEL
// 2012-04-02, Pantech only, ymlee_p11019, On Auto test mode 5G DPD enabled
#define FEATURE_PANTECH_5G_DPD_ENABLE_AUTO_TEST
#define FEATURE_PANTECH_WLAN_QCOM_PATCH
#define FEATURE_PANTECH_WLAN_TRP_TIS // 2012-04-09, Pantech only, ymlee_p11019, to config & test TRP TIS
#define FEATURE_PANTECH_WLAN_SCAN  // 20120824 thkim_wifi modify wifi scan time because of ps4's request
#endif  

// 20120229 lcj@LS3 feature revised
#if defined(T_EF44S)
#define FEATURE_PANTECH_WLAN
#define FEATURE_PANTECH_WLAN_PROCESS_CMD
#define FEATURE_PANTECH_WLAN_TESTMENU
#define FEATURE_PANTECH_WLAN_BCM4334
#define FEATURE_PANTECH_WLAN_BCM
#define FEATURE_PANTECH_WLAN_MAC_ADDR_BACKUP_BCM
#ifdef FEATURE_PANTECH_WLAN_MAC_ADDR_BACKUP_BCM
	#define FEATURE_PANTECH_PANMAC
	#define FEATURE_PANTECH_WLAN_RAWDATA_ACCESS // it's the same as FEATURE_SKY_RAWDATA_ACCESS
#endif // 
// 20120625 lcj@LS3 supporting for SEMCO module
#define FEATURE_PANTECH_WLAN_SEMCO
#endif // T_EF44S

#if defined(T_MAGNUS)
#define FEATURE_PANTECH_WLAN_WCN3660
#define FEATURE_PANTECH_WLAN_PROCESS_CMD
#define FEATURE_PANTECH_WLAN_TESTMENU
#define FEATURE_PANTECH_WLAN_RAWDATA_ACCESS
#define FEATURE_PANTECH_WLAN_MAC_ADDR_BACKUP_WCN3660
#define FEATURE_PANTECH_WLAN_QCOM_WOW
#define FEATURE_PANTECH_WLAN_FOUR_MAC_ADDRESS_WCN3660  // 20120204 thkim_wifi This feauture is applied to MSM8960-WCN3660 MODEL
// 2012-04-02, Pantech only, ymlee_p11019, On Auto test mode 5G DPD enabled
#define FEATURE_PANTECH_5G_DPD_ENABLE_AUTO_TEST // 20120405 jhpark_p16436 : add feature for magnus
#define FEATURE_PANTECH_WLAN_QCOM_PATCH
#define FEATURE_PANTECH_WLAN_TRP_TIS // 2012-04-09, Pantech only, ymlee_p11019, to config & test TRP TIS
#endif // T_MAGNUS

/*************************************************************************/
/*                               PST                                     */
/*************************************************************************/
#if defined(T_STARQ) || defined(T_VEGAPVW)
#define F_PANTECH_PST
#define F_PANTECH_PST_ROOT_PROCESS
#endif

/*******************************************************************************
**  SENSOR
*******************************************************************************/
#define FEATURE_PANTECH_DSPS
#define FEATURE_PANTECH_GSIFF_DELAY
#if defined(T_VEGAPVW)
#define FEATURE_PANTECH_MULTY_BATTERY_COVER
#endif
#define PANTECH_DSPS_ENCRYPTION_SOLUTION

/*******************************************************************************
 **  PANTECH ROOTING CHECK		//lsi@ls1
*******************************************************************************/
#define F_PANTECH_OEM_ROOTING

/*******************************************************************************
 **  PANTECH SECURE BOOT		//lsi@ls1
*******************************************************************************/
#if defined(T_STARQ) || defined(T_VEGAPVW)
#define F_PANTECH_SECBOOT
#endif

/*******************************************************************************
**  PM
*******************************************************************************/
#define PANTECH_CHARGER_MONITOR_TEST //20120229_khlee_pm : for chargerMonitor test(#8378522#)
#define FEATURE_PANTECH_BATTERY_DUMMY
#if defined(T_SIRIUSLTE) //20120413_khlee_pm : for battery charging/discharging test(#2288378#)
//#define PANTECH_BATTERY_CHARING_DISCHARING_TEST
#endif
/*******************************************************************************
** USB 
*******************************************************************************/
#define FEATURE_ANDROID_PANTECH_USB_CDFREE
#define FEATURE_ANDROID_PANTECH_USB_PROPERTY_SETTING
#define FEATURE_ANDROID_PANTECH_USB_TESTMENU
#define FEATURE_HSUSB_SET_SIGNALING_PARAM
#define FEATURE_PANTECH_USB_CABLE_CONNECT
#define FEATURE_QC_OTG_PATCH/* Qualcomm OTG Comparator patch*/
/*T_EF47S, T_EF46L, T_EF45K, T_CHEETAH, T_VEGAVZW    */
#if defined(T_EF46L) ||  defined(T_EF47S) || defined(T_EF45K)
#if(BOARD_VER >= TP10)
    #define FEATURE_ANDROID_PANTECH_USB_OTG_MODE
#endif
#endif
#if defined(T_EF44S)
#if (BOARD_VER >= WS15)
    #define FEATURE_ANDROID_PANTECH_USB_OTG_MODE
#endif
#endif
#if defined(T_SIRIUSLTE)
#if(BOARD_VER >= WS11)
    #define FEATURE_ANDROID_PANTECH_USB_SMB_OTG_MODE	
#endif
#endif
#define FEATURE_PANTECH_MSG_ONOFF // QXDM MSG ONOFF
/****************************************************
** MTC
****************************************************/
#if defined(T_EF45K) || defined(T_EF46L) || defined(T_EF47S)
#define CONFIG_FEATURE_PANTECH_MDS_MTC   /* MTC */
#define CONFIG_FEATURE_PANTECH_MAT      /* MAT */
#define CONFIG_FEATURE_DIAG_LARGE_PACKET
#endif

/****************************************************
** RF
****************************************************/
#if defined(T_EF46L) || defined(T_EF44S)
#define FEATURE_RF_TUNABLE_ANT_TEST //EF46L tunable ant test bin
#endif

/****************************************************
** CPRM
****************************************************/
#if defined(T_SIRIUSLTE)
#define FEATURE_CPRM_INTERFACE //Sirius LTE
#endif

/****************************************************
** DRM
****************************************************/
#define FEATURE_PANTECH_DRM

/*******************************************************************************
** Release/Debug User/Eng mode 
*******************************************************************************/
#if defined(FEATURE_AARM_RELEASE_MODE)
#define FEATURE_SW_RESET_RELEASE_MODE // use in release mode
#endif

/****************************************************
** DRM
****************************************************/
#define FEATURE_PANTECH_DRM

/****************************************************
** MediaFramework
****************************************************/
#undef FEATURE_PANTECH_MEDIAFRAMEWORK_DEBUG
/*******************************************************************************
** Android Patternlock Reset
*******************************************************************************/
#define FEATURE_PANTECH_PATTERN_UNLOCK
#if defined(T_STARQ) || defined(T_OSCAR) || defined(T_VEGAPVW) || defined(T_MAGNUS) 
	#define FEATURE_PANTECH_PATTERN_MIN
#endif

#endif/* CUST_PANTECH_H */

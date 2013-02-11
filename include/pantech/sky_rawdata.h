#if !defined( __SKY_RAWDATA_H__ )
#define __SKY_RAWDATA_H__

/*===========================================================================

               SKY_RAWDATA.H

DESCRIPTION

Copyright (c) 2011 by Pantech All Rights Reserved.

===========================================================================*/

/*===========================================================================
                        EDIT HISTORY FOR MODULE

when        who              what,where,why
----------  ------------     ------------------------------------------------
2011-01-11  choiseulkee      Create.
============================================================================*/

#define SECTOR_SIZE               512
#define SKY_RAWDATA_MAX           (8192*SECTOR_SIZE) // 4MB
#define SECTOR_SIZE_DEFAULT       1

//----------------------------------------------------------------------------------
// 20110929, albatros, partition size�� Ŀ���� ���� �ø���.
// ����� Ŀ���Ƿ� �˳��ϰ� 80�� ���´�.
// MEMMAP_PAGE_SIZE*32= 16K + backup size 16K = 32K (512*64)
#define GPT_PARTITION_SIZE        (SECTOR_SIZE*80) 

#define DLOAD_SECTOR_START        0
#define DLOAD_INFO_OFFSET         (DLOAD_SECTOR_START)
#define PARTITION_INFO_OFFSET     (DLOAD_SECTOR_START+SECTOR_SIZE)
#define BACKUP_DLOAD_INFO_OFFSET  (DLOAD_SECTOR_START + GPT_PARTITION_SIZE)
#define DLOAD_STATUS_OFFSET       (DLOAD_SECTOR_START + GPT_PARTITION_SIZE*2)
#define DLOAD_HISTORY_OFFSET      (DLOAD_SECTOR_START + GPT_PARTITION_SIZE*2 + SECTOR_SIZE)

#ifdef FEATURE_GOTA_UPDATE_INFO
/* gieil, 201202009, the next two items are used to store update status for AT&T directed devices */ 
#define GOTA_UPDATE_STATUS_OFFSET           (DLOAD_HISTORY_OFFSET + SECTOR_SIZE)	// indicates which update process is performed (ex, FOTA, PDL, SKY station, etc)
#define GOTA_UPDATE_STATUS_LENGTH           (SECTOR_SIZE)
#define GOTA_RESET_STATUS_OFFSET            (GOTA_UPDATE_STATUS_OFFSET + GOTA_UPDATE_STATUS_LENGTH)	// indicates whether the device was reset by update process
#define GOTA_RESET_STATUS_LENGTH            (SECTOR_SIZE)
#define GOTA_BACKUP_OFFSET                  (GOTA_RESET_STATUS_OFFSET + GOTA_RESET_STATUS_LENGTH)	// gieil, 20120210, added fot FOTA DM info backups
#define GOTA_BACKUP_LENGTH                  (SECTOR_SIZE)
#define DLOAD_SECTOR_MAX                    (GOTA_BACKUP_OFFSET + (GOTA_BACKUP_LENGTH*17))	// gieil, 20120209
#else
#define DLOAD_SECTOR_MAX                    (DLOAD_HISTORY_OFFSET + SECTOR_SIZE*20)
#endif /* FEATURE_GOTA_UPDATE_INFO */


//----------------------------------------------------------------------------------
//#define xxxxx_xxxxx_BACKUP_INDEX i : added to implement FACTORY_MAGIC_BLOCK_ERASE_I(i = 1, 255 ...backup area number)

#define BACKUP_SECTOR_START           (DLOAD_SECTOR_START + DLOAD_SECTOR_MAX + SECTOR_SIZE)

#define SECUREESN_START                     BACKUP_SECTOR_START
#define SECUREESN_LENGTH                    ((SECTOR_SIZE*25) * 2) // use sectors as pages // "*2" means 2 blocks

#define RFCAL_BACKUP_START                  (SECUREESN_START+SECUREESN_LENGTH)
#define RFCAL_BACKUP_LENGTH                 (SECTOR_SIZE*150)
#define RFCAL_BACKUP_INDEX                   1

#define FACTORY_EFS_INIT_START               (RFCAL_BACKUP_START+RFCAL_BACKUP_LENGTH)
#define FACTORY_EFS_INIT_LENGTH              (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

#define MSEC_BACKUP_START                    (FACTORY_EFS_INIT_START+FACTORY_EFS_INIT_LENGTH)
#define MSEC_BACKUP_LENGTH                   (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

#define FUNCTEST_RESULT_INIT_START           (MSEC_BACKUP_START+MSEC_BACKUP_LENGTH)
#define FUNCTEST_RESULT_INIT_LENGTH          (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

#define WIFI_DEVICE_INFO_START               (FUNCTEST_RESULT_INIT_START+FUNCTEST_RESULT_INIT_LENGTH)
#define WIFI_DEVICE_INFO_LENGTH              (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

#define BT_DEVICE_INFO_START                 (WIFI_DEVICE_INFO_START+WIFI_DEVICE_INFO_LENGTH)
#define BT_DEVICE_INFO_LENGTH                (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

#define PWR_ON_CNT_START                     (BT_DEVICE_INFO_START+BT_DEVICE_INFO_LENGTH)
#define PWR_ON_CNT_LENGTH                    (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

#define SDCARD_UPDATE_START                   (PWR_ON_CNT_START+PWR_ON_CNT_LENGTH)
#define SDCARD_UPDATE_LENGTH                  (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

#define USB_CHARGING_START                    (SDCARD_UPDATE_START+SDCARD_UPDATE_LENGTH)
#define USB_CHARGING_LENGTH                   (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

#define PERMANENTMEMORY_START                 (USB_CHARGING_START+USB_CHARGING_LENGTH)
#define PERMANENTMEMORY_LENGTH                (SECTOR_SIZE*2)

//F_PANTECH_MEID_IMEI_ADDR_BACKUP
#define NON_SECURE_IMEI_START                 (PERMANENTMEMORY_START+PERMANENTMEMORY_LENGTH)
#define NON_SECURE_IMEI_LENGTH                (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

//Reset_data for CIQ
#define CIQ_RESET_DATA_START                  (NON_SECURE_IMEI_START+NON_SECURE_IMEI_LENGTH)
#define CIQ_RESET_DATA_LENGTH                 (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

// P12554 MMP DivX DRM
#define PANTECH_DIVX_DRM_FRAG1_START          (CIQ_RESET_DATA_START+CIQ_RESET_DATA_LENGTH)
#define PANTECH_DIVX_DRM_FRAG1_LENGTH         (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

#define PANTECH_DIVX_DRM_FRAG2_START          (PANTECH_DIVX_DRM_FRAG1_START+PANTECH_DIVX_DRM_FRAG1_LENGTH)
#define PANTECH_DIVX_DRM_FRAG2_LENGTH         (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

#define PANTECH_DIVX_DRM_FRAG3_START          (PANTECH_DIVX_DRM_FRAG2_START+PANTECH_DIVX_DRM_FRAG2_LENGTH)
#define PANTECH_DIVX_DRM_FRAG3_LENGTH         (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)
// P12554 MMP DivX DRM

//F_PANTECH_MEID_IMEI_ADDR_BACKUP
#define NON_SECURE_MEID_START                 (PANTECH_DIVX_DRM_FRAG3_START+PANTECH_DIVX_DRM_FRAG3_LENGTH)
#define NON_SECURE_MEID_LENGTH                (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

//CONFIG_PANTECH_ERR_CRASH_LOGGING
#define PANTECH_ERR_CRASH_DUMP_START                 (NON_SECURE_MEID_START+NON_SECURE_MEID_LENGTH)
#define PANTECH_ERR_CRASH_DUMP_LENGTH         (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

//FEATURE_GOTA_UPGRADE
#define PANTECH_GOTA_SBL_UPDATE_FLAG_START                 (PANTECH_ERR_CRASH_DUMP_START+PANTECH_ERR_CRASH_DUMP_LENGTH)
#define PANTECH_GOTA_SBL_UPDATE_FLAG_LENGTH         (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

//PANTECH_CPRM
#define PANTECH_CPRM_START                 (PANTECH_GOTA_SBL_UPDATE_FLAG_START+PANTECH_GOTA_SBL_UPDATE_FLAG_LENGTH)
#define PANTECH_CPRM_LENGTH         (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

//F_PANTECH_OEM_ROOTING
#define PANTECH_ROOTING_CHECK_START           (PANTECH_CPRM_START+PANTECH_CPRM_LENGTH)
#define PANTECH_ROOTING_CHECK_LENGTH          (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

// <!-- F_PANTECH_SECBOOT 
#define PANTECH_SECBOOT_FLAG_START            (PANTECH_ROOTING_CHECK_START+PANTECH_ROOTING_CHECK_LENGTH)
#define PANTECH_SECBOOT_FLAG_LENGTH           (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

#define PANTECH_SECBOOT_RESULT_START          (PANTECH_SECBOOT_FLAG_START+PANTECH_SECBOOT_FLAG_LENGTH)
#define PANTECH_SECBOOT_RESULT_LENGTH         (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)
// F_PANTECH_SECBOOT --!>

// <!-- FEATURE_PANTECH_WIDEVINE_DRM
#define PANTECH_WV_DRM_DEVICEID_START         (PANTECH_SECBOOT_RESULT_START+PANTECH_SECBOOT_RESULT_LENGTH)
#define PANTECH_WV_DRM_DEVICEID_LENGTH        (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

#define PANTECH_WV_DRM_KEYBOX_START           (PANTECH_WV_DRM_DEVICEID_START+PANTECH_WV_DRM_DEVICEID_LENGTH)
#define PANTECH_WV_DRM_KEYBOX_LENGTH          (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)
// FEATURE_PANTECH_WIDEVINE_DRM --!>

// <!-- P14527 Verizon Hard Reset
#define HARD_RESET_START                      (PANTECH_WV_DRM_KEYBOX_START+PANTECH_WV_DRM_KEYBOX_LENGTH)
#define HARD_RESET_LENGTH                     (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)
// Verizon Hard Reset --!>

//20120426 LS1_hskim add for F_PANTECH_FACTORY_INIT_MODEM
#define PANTECH_FACTORY_INIT_MODEM_START            (HARD_RESET_START+HARD_RESET_LENGTH)
#define PANTECH_FACTORY_INIT_MODEM_LENGTH          (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

/* PANTECH_DTCP_IP_KEYBOX */
#define PANTECH_DTCP_IP_KEYBOX_START          (PANTECH_FACTORY_INIT_MODEM_START+PANTECH_FACTORY_INIT_MODEM_LENGTH)
#define PANTECH_DTCP_IP_KEYBOX_LENGTH         (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

// p13783 : add for Factory Init All command
#define FACTORY_INIT_ALL_PROCESS_START        (PANTECH_DTCP_IP_KEYBOX_START+PANTECH_DTCP_IP_KEYBOX_LENGTH)
#define FACTORY_INIT_ALL_PROCESS_LENGTH       (SECTOR_SIZE*SECTOR_SIZE_DEFAULT)

#define BACKUP_SECTOR_LENGTH                  (FACTORY_INIT_ALL_PROCESS_START+FACTORY_INIT_ALL_PROCESS_LENGTH-BACKUP_SECTOR_START)
#define BACKUP_SECTOR_BACKUP_INDEX            255
// Backup Item �߰��� BACKUP_SECTOR_LENGTH ���� �߰� �Ͻð� �̳��� ������ �ּ���
// ���� MDM�� sky_rawdata.h(core/pantech/sky_rawdata/inc)���� �����ϰ� update ���ּ���!!!
// FACTORY_MAGIC_BLOCK_ERASE_I should be updated.

//----------------------------------------------------------------------------------


extern int sky_rawdata_write( unsigned int address, unsigned int size, char *buf );
extern int sky_rawdata_read( unsigned int address, unsigned int size, char *buf );

#define GOTA_SBL_UPDATE_START_MAGIC_NUM           0xCF3D

#if defined(CONFIG_PANTECH_ERR_CRASH_LOGGING) ||defined(FEATURE_SKY_SAVE_LOG_SDCARD)
#define ERR_CRASH_DUMP_START_MAGIC_NUM           0xDEADDEAD
#define ERR_CRASH_DUMP_END_MAGIC_NUM             0xEFBEEFBE

typedef struct{
  unsigned int err_crash_dump_start_magic_num;		   // Start Magic Number
  unsigned int ram_dump_enable;
  unsigned int usb_dump_enable;
  unsigned int err_crash_dump_end_magic_num;             // End Magic Number
}err_crash_dump_type;

extern void pantech_write_ramdump_enable_sysfs( unsigned int ramdump );
extern void pantech_write_usbdump_enable_sysfs( unsigned int usbdump  );
extern int pantech_apanic_sysfs_init( unsigned int init_once );
#endif

extern int sky_rawdata_get_cksum_rom(char *buf); //goni
extern int sky_rawdata_read_cksum_file(char *buf); 

#endif /* __SKY_RAWDATA_H__ */

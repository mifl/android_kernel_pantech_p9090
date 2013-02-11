//--------------------------------------------------------
//
//
//	Melfas MMS100 Series Download base v1.8 2011.11.25
//

// wcjeong - add linux/kernel.h
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include "mms100_ISP_download.h"

//============================================================
//
//	Include MELFAS Binary code File ( ex> MELFAS_FIRM_bin.c)
//
//	Warning!!!!
//		Please, don't add binary.c file into project
//		Just #include here !!
//
//============================================================

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

struct i2c_client *i2c = NULL; 
static UINT8 write_buffer[1024 + 3 + 2] = {0xFF, };

extern const UINT16 MELFAS_binary_nLength;
extern const  UINT8 MELFAS_binary[];

#if ISC_CUSTOM_FILE_DL_ENABLE
extern const UINT16 MELFAS_binary_nLength_2;
extern const  UINT8 MELFAS_binary_2[];
#endif

UINT8  ucSlave_addr = ISC_MODE_SLAVE_ADDRESS;
UINT8 ucInitial_Download = FALSE;


extern void mcsdl_delay(UINT32 nCount);
//---------------------------------
//	Downloading functions
//---------------------------------

static int mms100_ISC_download(const UINT8 *pBianry, const UINT16 unLength, const UINT8 nMode);


static void mms100_ISC_set_ready(void);
static void mms100_ISC_reboot_mcs(void);

static UINT8 mms100_ISC_read_data(UINT8 addr);

static void mms100_ISC_enter_download_mode(void);	
static void mms100_ISC_firmware_update_mode_enter(void);
static UINT8 mms100_ISC_read_firmware_status(void);
static void mms100_ISC_leave_firmware_update_mode(void);

static UINT8 mms100_ISC_firmware_update(UINT8 *_pBinary_reordered, UINT16 _unDownload_size, UINT8 flash_start, UINT8 flash_end);

#if MELFAS_2CHIP_DOWNLOAD_ENABLE
static int mms100_ISC_Slave_download(UINT8 nMode);
static void mms100_ISC_slave_download_start(UINT8 nMode);
static UINT8 mms100_ISC_slave_crc_ok(void);
#endif 

//---------------------------------
//	For debugging display
//---------------------------------
#if MELFAS_ENABLE_DBG_PRINT
static void mcsdl_ISC_print_result(int nRet);
#endif


//----------------------------------
// Download enable command
//----------------------------------
#if MELFAS_USE_PROTOCOL_COMMAND_FOR_DOWNLOAD

void melfas_send_download_enable_command(void)
{
	// TO DO : Fill this up
}

#endif

int mms100_download(void *pi2c_client)
{
	int ret =0;		// 0 is success.	
	unsigned long old_jiffies;

	dbg(" ++++++++++ Download START\n");

	i2c = (struct i2c_client *) pi2c_client;
	old_jiffies = jiffies;	

#if MELFAS_ISP_DOWNLOAD
	ret = mms100_ISP_download_binary_data(MELFAS_ISP_DOWNLOAD);
	if (ret) dbg("<MELFAS> SET Download ISP Fail\n");
#else	
	ret = mms100_ISC_download_binary_data();
	if (ret) {
		dbg("<MELFAS> SET Download ISC Fail...ISP Try\n");

		ret = mms100_ISP_download_binary_data(1);
		if (ret) {
			dbg("<MELFAS> SET Download ISP Fail\n");
		}
		else {
			dbg("<MELFAS> SET Download ISP Success...\n");
		}
	}
	else {
		dbg("<MELFAS> SET Download ISC Success...\n");
	}

	ucInitial_Download = TRUE;

#endif

	dbg(" ++++++++++ Download END: %u msec\n", jiffies_to_msecs(jiffies - old_jiffies) );

    return ret;

}

EXPORT_SYMBOL(mms100_download);


//============================================================
//
//	Main Download furnction
//
//   1. Run mms100_ISC_download(*pBianry,unLength, nMode)
//       nMode : 0 (Core download)
//       nMode : 1 (Private Custom download)
//       nMode : 2 (Public Custom download)
//
//============================================================

int mms100_ISC_download_binary_data(void)
{

	int i, nRet;
    INT8 dl_enable_bit = 0x00;
    INT8 version_info = 0;

#if MELFAS_USE_PROTOCOL_COMMAND_FOR_DOWNLOAD
	melfas_send_download_enable_command();
	mcsdl_delay(MCSDL_DELAY_100US);
#endif

//	MELFAS_DISABLE_BASEBAND_ISR();					// Disable Baseband touch interrupt ISR.
//	MELFAS_DISABLE_WATCHDOG_TIMER_RESET();			// Disable Baseband watchdog timer

    //---------------------------------
    // set download enable mode
    //---------------------------------
    
    if(ucInitial_Download) ucSlave_addr = ISC_DEFAULT_SLAVE_ADDR;
    if(MELFAS_CORE_FIRWMARE_UPDATE_ENABLE || ucInitial_Download) 
	{
//		version_info = mms100_ISC_read_data(MELFAS_FIRMWARE_VER_REG_CORE);
//		dbg("<MELFAS> CORE_VERSION : 0x%2X\n",version_info);
//		if(version_info< MELFAS_DOWNLAOD_CORE_VERSION || version_info==0xFF)
		dl_enable_bit |= 0x01;
	}
    if(MELFAS_PRIVATE_CONFIGURATION_UPDATE_ENABLE)
	{
		version_info = mms100_ISC_read_data(MELFAS_FIRMWARE_VER_REG_PRIVATE_CUSTOM);
		dbg("<MELFAS> PRIVATE_CUSTOM_VERSION : 0x%2X\n",version_info);
		if(version_info < MELFAS_DOWNLAOD_PRIVATE_VERSION || version_info==0xFF)
		dl_enable_bit |= 0x02;
	}
    if(MELFAS_PUBLIC_CONFIGURATION_UPDATE_ENABLE)
	{
		version_info = mms100_ISC_read_data(MELFAS_FIRMWARE_VER_REG_PUBLIC_CUSTOM);
		dbg("<MELFAS> PUBLIC_CUSTOM_VERSION : 0x%2X\n",version_info);
		if(version_info < MELFAS_DOWNLAOD_PUBLIC_VERSION || version_info==0xFF)
		dl_enable_bit |= 0x04;
	}


	//------------------------
	// Run Download
	//------------------------
	
    for(i=0;i<3;i++)
    {
        if(dl_enable_bit & (1<<i))
        {
        
            if(i<2) // 0: core, 1: private custom
            {
                nRet = mms100_ISC_download( (const UINT8*) MELFAS_binary, (const UINT16)MELFAS_binary_nLength, (const INT8)i);
                ucSlave_addr = ISC_MODE_SLAVE_ADDRESS;
                ucInitial_Download = FALSE;
            }
            else 	// 2: public custom
            {
#if ISC_CUSTOM_FILE_DL_ENABLE
				nRet = mms100_ISC_download( (const UINT8*) MELFAS_binary_2, (const UINT16)MELFAS_binary_nLength_2, (const INT8)i);
#else 
                nRet = mms100_ISC_download( (const UINT8*) MELFAS_binary, (const UINT16)MELFAS_binary_nLength, (const INT8)i);
#endif
            }
			
            if (nRet) goto fw_error;
			
#if MELFAS_2CHIP_DOWNLOAD_ENABLE
            nRet = mms100_ISC_Slave_download((const INT8) i); // Slave Binary data download
            if (nRet)
                goto fw_error;
#endif
        }
    }

	MELFAS_ROLLBACK_BASEBAND_ISR(); 				// Roll-back Baseband touch interrupt ISR.
	MELFAS_ROLLBACK_WATCHDOG_TIMER_RESET(); 		// Roll-back Baseband watchdog timer
	return 0;

fw_error:
//	mcsdl_erase_flash(0);
//	mcsdl_erase_flash(1);
	return nRet;

}

int mms100_ISC_download_binary_file(void)
{
	int i, nRet;
    INT8 dl_enable_bit = 0x00;
    INT8 version_info = 0;
    
	UINT8  *pBinary[2] = {NULL,NULL};
	UINT16 nBinary_length[2] ={0,0};
    UINT8 IdxNum = MELFAS_2CHIP_DOWNLOAD_ENABLE; 
	//==================================================
	//
	//	1. Read '.bin file'
	//   2. *pBinary[0]       : Binary data(Core + Private Custom)
	//       *pBinary[1]       : Binary data(Public)
	//	   nBinary_length[0] : Firmware size(Core + Private Custom)
	//	   nBinary_length[1] : Firmware size(Public)	
    //   3. Run mms100_ISC_download(*pBianry,unLength, nMode)
    //       nMode : 0 (Core download)
    //       nMode : 1 (Private Custom download)
    //       nMode : 2 (Public Custom download)
	//
	//==================================================

	#if 0

		// TO DO : File Process & Get file Size(== Binary size)
		//			This is just a simple sample

		FILE *fp;
		INT  nRead;

		//------------------------------
		// Open a file
		//------------------------------

		if( fopen( fp, "MELFAS_FIRMWARE.bin", "rb" ) == NULL ){
			return MCSDL_RET_FILE_ACCESS_FAILED;
		}

		//------------------------------
		// Get Binary Size
		//------------------------------

		fseek( fp, 0, SEEK_END );

		nBinary_length = (UINT16)ftell(fp);

		//------------------------------
		// Memory allocation
		//------------------------------

		pBinary = (UINT8*)malloc( (INT)nBinary_length );

		if( pBinary == NULL ){

			return MCSDL_RET_FILE_ACCESS_FAILED;
		}

		//------------------------------
		// Read binary file
		//------------------------------

		fseek( fp, 0, SEEK_SET );

		nRead = fread( pBinary, 1, (INT)nBinary_length, fp );		// Read binary file

		if( nRead != (INT)nBinary_length ){

			fclose(fp);												// Close file

			if( pBinary != NULL )										// free memory alloced.
				free(pBinary);

			return MCSDL_RET_FILE_ACCESS_FAILED;
		}

		//------------------------------
		// Close file
		//------------------------------

		fclose(fp);

	#endif

#if MELFAS_USE_PROTOCOL_COMMAND_FOR_DOWNLOAD
    melfas_send_download_enable_command();
    mcsdl_delay(MCSDL_DELAY_100US);
#endif
    
    MELFAS_DISABLE_BASEBAND_ISR();                  // Disable Baseband touch interrupt ISR.
    MELFAS_DISABLE_WATCHDOG_TIMER_RESET();          // Disable Baseband watchdog timer

    for(i=0;i<=IdxNum;i++){
    	if( pBinary[i] != NULL && nBinary_length[i] > 0 && nBinary_length[i] < MELFAS_FIRMWARE_MAX_SIZE ){
    	}else{
    		nRet = MCSDL_RET_WRONG_BINARY;
    	}
	}
    //---------------------------------
    // set download enable mode
    //---------------------------------
    if(MELFAS_CORE_FIRWMARE_UPDATE_ENABLE) 
    {
        version_info = mms100_ISC_read_data(MELFAS_FIRMWARE_VER_REG_CORE);
        dbg("<MELFAS> CORE_VERSION : 0x%2X\n",version_info);
        if(version_info< 0x01 || version_info==0xFF)
        dl_enable_bit |= 0x01;
    }
    if(MELFAS_PRIVATE_CONFIGURATION_UPDATE_ENABLE)
    {
        version_info = mms100_ISC_read_data(MELFAS_FIRMWARE_VER_REG_PRIVATE_CUSTOM);
        dbg("<MELFAS> PRIVATE_CUSTOM_VERSION : 0x%2X\n",version_info);
        if(version_info < 0x01 || version_info==0xFF)
        dl_enable_bit |= 0x02;
    }
    if(MELFAS_PUBLIC_CONFIGURATION_UPDATE_ENABLE)
    {
        version_info = mms100_ISC_read_data(MELFAS_FIRMWARE_VER_REG_PUBLIC_CUSTOM);
        dbg("<MELFAS> PUBLIC_CUSTOM_VERSION : 0x%2X\n",version_info);
        if(version_info < 0x01 || version_info==0xFF)
        dl_enable_bit |= 0x04;
    }
    

    //------------------------
    // Run Download
    //------------------------
    
    for(i=0;i<3;i++)
    {
        if(dl_enable_bit & (1<<i))
        {
        
            if(i<2) // 0: core, 1: private custom
                nRet = mms100_ISC_download( (const UINT8*) pBinary[0], (const UINT16)nBinary_length[0], (const INT8)i);
            else    // 2: public custom
                nRet = mms100_ISC_download( (const UINT8*) pBinary[1], (const UINT16)nBinary_length[1], (const INT8)i);
            if (nRet)
                goto fw_error;
#if MELFAS_2CHIP_DOWNLOAD_ENABLE
            nRet = mms100_ISC_Slave_download((const INT8) i); // Slave Binary data download
            if (nRet)
                goto fw_error;
#endif
        }
    }
    
    
    MELFAS_ROLLBACK_BASEBAND_ISR();                 // Roll-back Baseband touch interrupt ISR.
    MELFAS_ROLLBACK_WATCHDOG_TIMER_RESET();         // Roll-back Baseband watchdog timer
    return 0;

fw_error:
//  mcsdl_erase_flash(0);
//  mcsdl_erase_flash(1);
    return nRet;

}

//------------------------------------------------------------------
//
//	Download function
//
//------------------------------------------------------------------

static int mms100_ISC_download(const UINT8 *pBianry, const UINT16 unLength, const UINT8 nMode)
{
	int nRet=0;
    UINT8 fw_status = 0;

    UINT8 private_flash_start = ISC_PRIVATE_CONFIG_FLASH_START;
    UINT8 public_flash_start = ISC_PUBLIC_CONFIG_FLASH_START;
	
    UINT8 flash_start[3] = {0, 0, 0};
    UINT8 flash_end[3] =  {31, 31, 31};
      
        
	//---------------------------------
	// Check Binary Size
	//---------------------------------
	if( unLength > MELFAS_FIRMWARE_MAX_SIZE ){

		nRet = MCSDL_RET_PROGRAM_SIZE_IS_WRONG;
		goto MCSDL_DOWNLOAD_FINISH;
	}

	//---------------------------------
	// Make it ready
	//---------------------------------
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	dbg("<MELFAS> Ready\n");
#endif

//	mms100_ISC_set_ready();

#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
    if(nMode==0) dbg("<MELFAS> Core_firmware_download_via_ISC start!!!\n");
    else if(nMode==1) dbg("<MELFAS> Private_Custom_firmware_download_via_ISC start!!!\n");
    else dbg("<MELFAS> Public_Custom_firmware_download_via_ISC start!!!\n");
#endif


    //--------------------------------------------------------------
    // INITIALIZE
    //--------------------------------------------------------------
    dbg("<MELFAS> ISC_DOWNLOAD_MODE_ENTER\n");
    mms100_ISC_enter_download_mode();
	mcsdl_delay(MCSDL_DELAY_100MS);

#if ISC_READ_DOWNLOAD_POSITION
    dbg("<MELFAS> Read download position.\n");
    if(1 << nMode !=ISC_CORE_FIRMWARE_DL_MODE )
    {
        private_flash_start = mms100_ISC_read_data(ISC_PRIVATE_CONFIGURATION_START_ADDR);
        public_flash_start = mms100_ISC_read_data(ISC_PUBLIC_CONFIGURATION_START_ADDR);
    }
#endif

    flash_start[0] = 0;
    flash_end[0] = flash_end[2] = 31;
    flash_start[1] = private_flash_start;
    flash_start[2] = flash_end[1] = public_flash_start;
    dbg("<MELFAS> Private Configuration start at %2dKB, Public Configuration start at %2dKB\n",private_flash_start ,public_flash_start );

	mcsdl_delay(MCSDL_DELAY_60MS);

    //--------------------------------------------------------------
    // FIRMWARE UPDATE MODE ENTER
    //--------------------------------------------------------------
    dbg("<MELFAS> FIRMWARE_UPDATE_MODE_ENTER\n\n");            
    mms100_ISC_firmware_update_mode_enter();
	mcsdl_delay(MCSDL_DELAY_60MS);
#if 1
    fw_status = mms100_ISC_read_firmware_status();
    if(fw_status == 0x01)
        dbg("<MELFAS> Firmware update mode enter success!!!\n");
    else
    {
        dbg("<MELFAS> Error detected!! firmware status is 0x%02x.\n", fw_status);
        nRet = MCSDL_FIRMWARE_UPDATE_MODE_ENTER_FAILED;
        goto MCSDL_DOWNLOAD_FINISH;
    }
	 
	mcsdl_delay(MCSDL_DELAY_60MS);
#endif

     //--------------------------------------------------------------
    // FIRMWARE UPDATE 
    //--------------------------------------------------------------
	dbg("<MELFAS> FIRMWARE UPDATE\n\n");
    nRet = mms100_ISC_firmware_update((UINT8 *)pBianry, (UINT16)unLength, flash_start[nMode],flash_end[nMode]);
    if(nRet != MCSDL_RET_SUCCESS) goto MCSDL_DOWNLOAD_FINISH;
	dbg("<MELFAS> FIRMWARE UPDATE Complete...\n\n");
    
    //--------------------------------------------------------------
    // LEAVE FIRMWARE UPDATE MODE
    //--------------------------------------------------------------
	
	dbg("<MELFAS> LEAVE FIRMWARE UPDATE MODE\n\n");            	
	mms100_ISC_leave_firmware_update_mode();
	mcsdl_delay(MCSDL_DELAY_60MS);	

// 	fw_status = mms100_ISC_read_firmware_status();
// 	if(fw_status == 0xFF || fw_status == 0x00 )
// 	{
// 		dbg("<MELFAS> Living firmware update mode success!!!\n");	
// 	}
// 	else
// 	{
// 		dbg("<MELFAS> Error detected!! firmware status is 0x%02x.\n", fw_status);
// 		nRet = MCSDL_LEAVE_FIRMWARE_UPDATE_MODE_FAILED;		
// 		goto MCSDL_DOWNLOAD_FINISH;
// 	}		

	nRet = MCSDL_RET_SUCCESS;

MCSDL_DOWNLOAD_FINISH :

#if MELFAS_ENABLE_DBG_PRINT
	mcsdl_ISC_print_result( nRet );								// Show result
#endif


#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	dbg("<MELMAS> Rebooting\n");
	dbg("<MELMAS>  - Fin.\n\n");
#endif

	mms100_ISC_reboot_mcs();


	return nRet;
}

// wcjeong - unused code
#if MELFAS_2CHIP_DOWNLOAD_ENABLE
static int mms100_ISC_Slave_download(UINT8 nMode)
{
	int nRet;
	
	//---------------------------------
	// Make it ready
	//---------------------------------
#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	dbg("<MELFAS> Ready\n");
#endif

	mms100_ISC_set_ready();

	#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
    if(nMode==0) dbg("<MELFAS> Core_firmware_slave_download_via_ISC start!!!\n");
    else if(nMode==1) dbg("<MELFAS> Private_Custom_firmware_slave_download_via_ISC start!!!\n");
    else dbg("<MELFAS> Public_Custom_firmware_slave_download_via_ISC start!!!\n");
	#endif

    //--------------------------------------------------------------
    // INITIALIZE
    //--------------------------------------------------------------
    dbg("<MELFAS> ISC_DOWNLOAD_MODE_ENTER\n\n");            
    mms100_ISC_enter_download_mode();
	mcsdl_delay(MCSDL_DELAY_100MS);

    //--------------------------------------------------------------
    // Slave download start
    //--------------------------------------------------------------
    mms100_ISC_slave_download_start(nMode+1);
    nRet = mms100_ISC_slave_crc_ok();
    if(nRet != MCSDL_RET_SUCCESS) goto MCSDL_DOWNLOAD_FINISH;


	nRet = MCSDL_RET_SUCCESS;


MCSDL_DOWNLOAD_FINISH :

	#if MELFAS_ENABLE_DBG_PRINT
	mcsdl_ISC_print_result( nRet );								// Show result
	#endif


	#if MELFAS_ENABLE_DBG_PROGRESS_PRINT
	dbg("<MELMAS> Rebooting\n");
	dbg("<MELMAS>  - Fin.\n\n");
	#endif

	mms100_ISC_reboot_mcs();
	

	return nRet;
}
#endif

//------------------------------------------------------------------
//
//	Sub functions
//
//------------------------------------------------------------------
static UINT8 mms100_ISC_read_data(UINT8 addr)
{
	UINT8 write_buffer;
	UINT8 read_buffer;
	int ret=0;

	write_buffer = addr;
	ret = i2c_master_send(i2c, &write_buffer, 1);
	if (ret<0) dbg_kern("[ISC] melfas_ts_work_func : i2c_master_send [%d]\n", ret);

	ret = i2c_master_recv(i2c, &read_buffer, 1);
	if (ret<0) dbg_kern("[ISC] melfas_ts_work_func : i2c_master_recv [%d]\n", ret);

	return read_buffer;
}

static void mms100_ISC_enter_download_mode(void)
{
	UINT8  write_buffer[4];
	int ret=0;

	write_buffer[0] = ISC_DOWNLOAD_MODE_ENTER; // command
	write_buffer[1] = 0x01; // sub_command

	ret = i2c_master_send(i2c, write_buffer, 2);	
	if (ret<0) dbg_kern("[ISC] melfas_ts_work_func : i2c_master_send [%d]\n", ret);
}

static void mms100_ISC_firmware_update_mode_enter(void)
{
	UINT8  write_buffer[10];
	int ret=0;

	write_buffer[0] = 0xAE;
	write_buffer[1] = 0x55;

	write_buffer[2] = 0x00;
	write_buffer[3] = 0x00;
	write_buffer[4] = 0x00;
	write_buffer[5] = 0x00;
	write_buffer[6] = 0x00;
	write_buffer[7] = 0x00;
	write_buffer[8] = 0x00;
	write_buffer[9] = 0x00;

	ret = i2c_master_send(i2c, write_buffer, 10);
	if (ret<0) dbg_kern("[ISC] melfas_ts_work_func : i2c_master_send [%d]\n", ret);
}

static UINT8 mms100_ISC_firmware_update(UINT8 *_pBinary_reordered, UINT16 _unDownload_size, UINT8 flash_start, UINT8 flash_end)
{
	int i=0,j=0,n,m, retry=0;

	UINT8 fw_status;
	UINT16 reg_addr;

	UINT16 nOffset = 0;
	UINT16 cLength = 8;
	UINT16 CRC_check_buf,CRC_send_buf,IN_data;
	UINT16 XOR_bit_1,XOR_bit_2,XOR_bit_3;

	int ret=0;

	nOffset = 0;
	cLength = 8; //256

	dbg("<MELFAS> flash start : %2d, flash end : %2d\n", flash_start, flash_end);    

	while(flash_start + nOffset < flash_end)
	{  
		CRC_check_buf = 0xFFFF;

		write_buffer[0] = 0XAE; // command
		write_buffer[1] = 0XF1; // sub_command 
		write_buffer[2] = flash_start + nOffset;

		for(m=7;m>=0;m--)
		{
			IN_data =(write_buffer[2] >>m) & 0x01;
			XOR_bit_1 = (CRC_check_buf & 0x0001) ^ IN_data;
			XOR_bit_2 = XOR_bit_1^(CRC_check_buf>>11 & 0x01);
			XOR_bit_3 = XOR_bit_1^(CRC_check_buf>>4 & 0x01);
			CRC_send_buf = (XOR_bit_1 <<4) | (CRC_check_buf >> 12 & 0x0F);
			CRC_send_buf = (CRC_send_buf<<7) | (XOR_bit_2 <<6) | (CRC_check_buf >>5 & 0x3F);
			CRC_send_buf = (CRC_send_buf<<4) | (XOR_bit_3 <<3) | (CRC_check_buf>>1 & 0x0007);
			CRC_check_buf = CRC_send_buf;
		}

		for(j=0;j<32;j++){
			for(i=0;i<8;i++){
				reg_addr = j*32+i*4;
				write_buffer[3 + reg_addr + 3] = _pBinary_reordered[(flash_start+nOffset)*1024+reg_addr+0];
				write_buffer[3 + reg_addr + 2] = _pBinary_reordered[(flash_start+nOffset)*1024+reg_addr+1];
				write_buffer[3 + reg_addr + 1] = _pBinary_reordered[(flash_start+nOffset)*1024+reg_addr+2];
				write_buffer[3 + reg_addr + 0] = _pBinary_reordered[(flash_start+nOffset)*1024+reg_addr+3];

				for(n=0;n<4;n++){
					for(m=7;m>=0;m--){
						IN_data =(write_buffer[3+reg_addr+n]>>m) & 0x0001;
						XOR_bit_1 = (CRC_check_buf & 0x0001) ^ IN_data;
						XOR_bit_2 = XOR_bit_1^(CRC_check_buf>>11 & 0x01);
						XOR_bit_3 = XOR_bit_1^(CRC_check_buf>>4 & 0x01);
						CRC_send_buf = (XOR_bit_1 <<4) | (CRC_check_buf >> 12 & 0x0F);
						CRC_send_buf = (CRC_send_buf<<7) | (XOR_bit_2 <<6) | (CRC_check_buf >>5 & 0x3F);
						CRC_send_buf = (CRC_send_buf<<4) | (XOR_bit_3 <<3) | (CRC_check_buf>>1 & 0x0007);
						CRC_check_buf = CRC_send_buf;
					}
				}
			}
		}


		write_buffer[1024 + 3] = CRC_check_buf >> 8 & 0xFF;
		write_buffer[1024 + 3 + 1] =  CRC_check_buf & 0xFF;		

		ret = i2c_master_send(i2c, write_buffer, 1029);
		if (ret<0) dbg_kern("[ISC] melfas_ts_work_func : i2c_master_send [%d]\n", ret);

		mcsdl_delay(MCSDL_DELAY_60MS);

		for (retry=0; retry<3; retry++) {
			fw_status = mms100_ISC_read_firmware_status();
			if(fw_status == 0x03)
			{
				dbg("<MELFAS> Firmware update success!!!\n");
				break;
			}
			else
			{
				dbg("<MELFAS> Error detected!! firmware status is 0x%02x.\n", fw_status);
				ret = i2c_master_send(i2c, write_buffer, 1029);
				if (ret<0) dbg_kern("[ISC] melfas_ts_work_func : i2c_master_send [%d]\n", ret);
				mcsdl_delay(MCSDL_DELAY_60MS);
			}
		}

		if(fw_status != 0x03) {
			return MCSDL_FIRMWARE_UPDATE_FAILED;
		}		

		nOffset += 1;
		dbg("<MELFAS> %d KB Downloaded...\n",nOffset);
	}

	return MCSDL_RET_SUCCESS;
}
    
static UINT8 mms100_ISC_read_firmware_status(void)
{
	UINT8 fw_status;
	UINT8 write_buffer = 0xAF;
	int ret=0;

	i2c_master_send(i2c, &write_buffer, 1);
	if (ret<0) dbg_kern("[ISC] melfas_ts_work_func : i2c_master_send [%d]\n", ret);

	i2c_master_recv(i2c, &fw_status, 1);
	if (ret<0) dbg_kern("[ISC] melfas_ts_work_func : i2c_master_recv [%d]\n", ret);

    return fw_status;
}

// wcjeong - unused code
#if MELFAS_2CHIP_DOWNLOAD_ENABLE
static void mms100_ISC_slave_download_start(UINT8 nMode)
{
	UINT32 wordData = 0x00000000;
    UINT8  write_buffer[4];
    
    mcsdl_i2c_start();
    // WRITE 0xAF
    write_buffer[0] = ucSlave_addr << 1;
    write_buffer[1] = ISC_DOWNLOAD_MODE; // command
    write_buffer[2] = nMode;  // 0x01: core, 0x02: private custom, 0x03: public custsom
    wordData = (write_buffer[0] << 24) | (write_buffer[1]<< 16) | (write_buffer[2]<< 8);
    mcsdl_ISC_write_bits( wordData, 24 );
    mcsdl_i2c_stop();
    mcsdl_delay(MCSDL_DELAY_100MS);
}

static UINT8 mms100_ISC_slave_crc_ok(void)
{
	UINT32 wordData = 0x00000000;
    UINT8 CRC_status = 0;
    UINT8  write_buffer[4];
    UINT8 check_count = 0;

	mcsdl_i2c_start();
	write_buffer[0] = ucSlave_addr << 1;
	write_buffer[1] = ISC_READ_SLAVE_CRC_OK; // command
	wordData = (write_buffer[0] << 24) | (write_buffer[1]<< 16);

	mcsdl_ISC_write_bits( wordData, 16 );
        
	while(CRC_status != 0x01 && check_count < 200) // check_count 200 : 20sec
    {
        mcsdl_i2c_start();
        // 1byte read
        wordData = (ucSlave_addr << 1 | 0x01) << 24;
        mcsdl_ISC_write_bits( wordData, 8 );
        CRC_status = mcsdl_read_byte();
        wordData = (0x01) << 31;
        mcsdl_ISC_write_bits( wordData, 1 ); //Nack
		if(check_count % 10 == 0) dbg("<MELFAS> %d sec...\n",check_count/10);
        mcsdl_i2c_stop();

        if(CRC_status == 1) return MCSDL_RET_SUCCESS;
        else if(CRC_status == 2) return MCSDL_RET_ISC_SLAVE_CRC_CHECK_FAILED;
        mcsdl_delay(MCSDL_DELAY_100MS);
        check_count++;
    }
	return MCSDL_RET_ISC_SLAVE_DOWNLOAD_TIME_OVER;
}
#endif

static void mms100_ISC_leave_firmware_update_mode()
{
	UINT8 write_buffer[3];
	int ret=0;

	write_buffer[0] = 0xAE;
	write_buffer[1] = 0x0F;
	write_buffer[2] = 0xF0;
	
	ret = i2c_master_send(i2c, write_buffer, 3);
	if (ret<0) dbg_kern("[ISC] melfas_ts_work_func : i2c_master_send [%d]\n", ret);
}

static void mms100_ISC_set_ready(void)
{
	//--------------------------------------------
	// Tkey module reset
	//--------------------------------------------

	MCSDL_VDD_SET_LOW(); // power 

	MCSDL_RESETB_SET_INPUT();
	
//	MCSDL_CE_SET_HIGH();
//	MCSDL_CE_SET_OUTPUT();
    
//	mcsdl_delay(MCSDL_DELAY_60MS);		// Delay for Stable VDD
	mcsdl_delay(MCSDL_DELAY_100MS);		// Delay for Stable VDD
	MCSDL_VDD_SET_HIGH();
	mcsdl_delay(MCSDL_DELAY_500MS);		// Delay for Stable VDD
}

static void mms100_ISC_reboot_mcs(void)
{
	//--------------------------------------------
	// Tkey module reset
	//--------------------------------------------
    mms100_ISC_set_ready();
}

//============================================================
//
//	Debugging print functions.
//
//============================================================

#if MELFAS_ENABLE_DBG_PRINT

static void mcsdl_ISC_print_result(int nRet)
{
	if( nRet == MCSDL_RET_SUCCESS ){

		dbg("<MELFAS> ISC - Firmware downloading SUCCESS.\n");

	}else{

		dbg("<MELFAS> ISC - Firmware downloading FAILED  :  ");

		switch( nRet ){

			case MCSDL_RET_SUCCESS                  		:   printk("<MELFAS> MCSDL_RET_SUCCESS\n" );                 		break;
			case MCSDL_RET_ERASE_FLASH_VERIFY_FAILED		:   printk("<MELFAS> MCSDL_RET_ERASE_FLASH_VERIFY_FAILED\n" );    	break;
			case MCSDL_RET_PROGRAM_VERIFY_FAILED			:   printk("<MELFAS> MCSDL_RET_PROGRAM_VERIFY_FAILED\n" );      	break;			
			case MCSDL_FIRMWARE_UPDATE_MODE_ENTER_FAILED    :   printk("<MELFAS> MCSDL_FIRMWARE_UPDATE_MODE_ENTER_FAILED\n" );	break;
			case MCSDL_FIRMWARE_UPDATE_FAILED            	:   printk("<MELFAS> MCSDL_FIRMWARE_UPDATE_FAILED\n" );			break;
			case MCSDL_LEAVE_FIRMWARE_UPDATE_MODE_FAILED 	:   printk("<MELFAS> MCSDL_LEAVE_FIRMWARE_UPDATE_MODE_FAILED\n" );	break;
			

			case MCSDL_RET_PROGRAM_SIZE_IS_WRONG			:   printk("<MELFAS> MCSDL_RET_PROGRAM_SIZE_IS_WRONG\n" );    		break;
			case MCSDL_RET_VERIFY_SIZE_IS_WRONG				:   printk("<MELFAS> MCSDL_RET_VERIFY_SIZE_IS_WRONG\n" );      	break;
			case MCSDL_RET_WRONG_BINARY						:   printk("<MELFAS> MCSDL_RET_WRONG_BINARY\n" );      			break;

			case MCSDL_RET_READING_HEXFILE_FAILED       	:   printk("<MELFAS> MCSDL_RET_READING_HEXFILE_FAILED\n" );		break;
			case MCSDL_RET_FILE_ACCESS_FAILED       		:   printk("<MELFAS> MCSDL_RET_FILE_ACCESS_FAILED\n" );			break;
			case MCSDL_RET_MELLOC_FAILED     		  		:   printk("<MELFAS> MCSDL_RET_MELLOC_FAILED\n" );      			break;
            
			case MCSDL_RET_ISC_SLAVE_CRC_CHECK_FAILED       :   printk("<MELFAS> MCSDL_RET_ISC_SLAVE_CRC_CHECK_FAILED\n" );    break;
			case MCSDL_RET_ISC_SLAVE_DOWNLOAD_TIME_OVER     :   printk("<MELFAS> MCSDL_RET_ISC_SLAVE_DOWNLOAD_TIME_OVER\n" );  break;
            
			case MCSDL_RET_WRONG_MODULE_REVISION     		:   printk("<MELFAS> MCSDL_RET_WRONG_MODULE_REVISION\n" );      	break;

			default                             			:	printk("<MELFAS> UNKNOWN ERROR. [0x%02X].\n", nRet );      	break;
		}

		dbg("\n");
	}
}

#endif

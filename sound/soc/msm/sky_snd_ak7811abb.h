#ifndef SNDSUBSYSTEM_H
#define SNDSUBSYSTEM_H
/************************************************************************************************
**
**    AK7811ABB AUDIO EXTERNAL AMP(SUBSYSTEM)
**
**    FILE
**        snd_sub_ak7811abb.h
**
**    DESCRIPTION
**        This file contains AK  Audio Subsystem api
**
**    Copyright (c) 2012 by Kim HyunWoo <kim.hyunwoo1@pantech.com>
*************************************************************************************************/


/************************************************************************************************
** Includes
*************************************************************************************************/
#include <linux/kernel.h>


/************************************************************************************************
** Definitions
*************************************************************************************************/
#define AK7811_SLAVE_ADDR	  	0x76	//1110110 (Ak SUBSYSTEM)
	
#define SYSTEM_ON					1
#define SYSTEM_OFF					0
	
#define SP_ZCS_ENABLE				0
#define SP_ZCS_DISBLE				1
#define HP_ZCS_ENABLE				0
#define HP_ZCS_DISBLE				1
#define SP_SV_ENABLE				0
#define SP_SV_DISABLE				1
#define HP_SV_ENABLE				0
#define HP_SV_DISABLE				1
#define SP_MIXER_ENABLE			1
#define SP_MIXER_DISABLE			0
#define HP_MIXER_ENABLE			1
#define HP_MIXER_DISABLE			0
#define NCLIP_LIMIT_ON				1
#define NCLIP_LIMIT_OFF				0


//FAB2210 Register Address
/*
#define REVISION_DCERR_REG				0x15
#define SOFTVOL_REG						0x1A
#define DRCMIN_REG						0x1B
#define SSMT_ERC_REG					0x1D
#define SRST_REG						0x80
#define DRCMODE_DATRT_NG_ATRT_REG	0x81
#define DPLT_HP_NG_REG					0x82
#define NCLIP_SP_NG_REG				0x83
#define VOLUME_REG						0x84
#define DIF_SVOFF_HIZ_REG				0x85
#define SP_ATT_REG						0x86
#define HP_ATT_REG						0x87
#define ERR_MIX_REG						0x88
#define DALC_GAIN_REG					0xC0
#define ZCSOFF_REG						0xC1
*/

//FAB2210 Register Contents
/*
// #1	(ADR 0x15)
#define REVISION_ID_VALUE		5
#define DCERR_TIME_VALUE		1
// #2	(ADR 0x1A)
#define SOFTVOL_VALUE			(0x01<<6)
// #3	(ADR 0x1B)
#define DRCMIN_VALUE			0
// #4	(ADR 0x1D)
#define MCSSMT_VALUE			5
#define SSMT_VALUE				2
#define ERC_VALUE				(0x01<<1)
// #5	(ADR 0x80)
#define SRST_VALUE				(0x01<<7)
// #6	(ADR 0x81)
#define DRC_MODE_VALUE			6
#define DATRT_VALUE				4
#define NG_ATRT_VALUE			2
#define MODESEL_VALUE			(0x01)
// #7	(ADR 0x82)
#define DPLT_VALUE				5
#define HP_NG_RAT_VALUE		2
#define HP_NG_ATT_VALUE		0
// #8	(ADR 0x83)
#define NCLIP_VALUE				(0x01<<5)
#define SP_NG_RAT_VALUE		2
#define SP_NG_ATT_VALUE		0
// #9	(ADR 0x84)
#define VA_VALUE				4	
#define VB_VALUE				0
// #10	(ADR 0x85)
#define DIFA_VALUE				(0x01<<7)
#define DIFB_VALUE				(0x01<<6)
#define HP_SVOFF_VALUE			(0x01<<3)
#define HP_HIZ_VALUE			(0x01<<2)
#define SP_SVOFF_VALUE			(0x01<<1)
#define SP_HIZ_VALUE			(0x01)
// #11	(ADR 0x86)
#define SP_ATT_VALUE			0	
// #12	(ADR 0x87)
#define HP_ATT_VALUE			0
// #13	(ADR 0x88)
#define OCP_ERR_VALUE			(0x01<<7)
#define OTP_ERR_VALUE			(0x01<<6)
#define DC_ERR_VALUE			(0x01<<5)
#define HP_MONO_VALUE			(0x01<<4)
#define HP_AMIX_VALUE			(0x01<<3)
#define HP_BMIX_VALUE			(0x01<<2)
#define SP_AMIX_VALUE			(0x01<<1)
#define SP_BMIX_VALUE			(0x01)
// #14	(ADR 0xC0)
#define DALC_VALUE				4
#define HP_GAIN_VALUE			2
#define SP_GAIN_VALUE			0
// #15	(ADR 0xC1)
#define HP_ZCSOFF_VALUE		(0x01<<2)
#define SP_ZCSOFF_VALUE		(0x01)
*/

#define SP_GAIN_VALUE			0


#define SYSRST_INFO	 0x01
#define SLEEP_INFO	0x02
#define SPGAIN_INFO	0x03
#define MODE_INFO	0x04
#define BE_INFO		0x05
#define LOUD_INFO	0x06


/*fab
typedef struct
{
  u8 revision_dcerr_info;		//0x15
  u8 softvol_info;		//0x1A
  u8 drcmin_info;		//0x1B
  u8 ssmt_erc_info; //0x1D
  u8 srst_info; 	//0x80
  u8 drcmode_datrt_ng_atrt_info;		//0x81
  u8 dplt_hp_ng_info;		//0x82
  u8 nclip_sp_ng_info;	//0x83
  u8 volume_info;		//0x84
  u8 dif_svoff_hiz_info;		//0x85
  u8 sp_att_info;		//0x86
  u8 hp_att_info;		//0x87
  u8 err_mix_info;	//0x88
  u8 dalc_gain_info;	//0xc0
  u8 zcsoff_info;		//0xc1
}subsystem_info_t;
*/

#define SLEEP_ON_VALUE		1
#define SLEEP_OFF_VALUE		0



typedef struct
{
  u8 sysrst_info;		//0x01
  u8 sleep_info;		//0x02
  u8 spgain_info;		//0x03
  u8 mode_info;		//0x04
  u8 be_info;		//0x05  
  u8 loud_info;		//0x06  
}subsystem_info_t;


/************************************************************************************************
** Prototypes
*************************************************************************************************/

void snd_subsystem_Init(void);
void snd_subsystem_DeInit(void);
int snd_subsystem_get_device(void);

int snd_subsystem_standby (int stanby);
int snd_subsystem_sp_poweron(void);
int snd_subsystem_readInfo(void);
int snd_subsystem_setSPGain (u8 spAmpGain);
int snd_subsystem_set_Modeinfo (int MODE);
int snd_subsystem_set_BE (int BE_MODE);
int snd_subsystem_set_LOUD(int Loud_Set);
int snd_subsystem_getSPGain (int *data);
int snd_subsystem_get_Modeinfo (int *data);
int snd_subsystem_get_BE (int *data);
int snd_subsystem_get_LOUD (int *data);

#endif /*#ifndef SNDSUBSYSTEM_H */

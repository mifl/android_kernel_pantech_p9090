#ifndef _CUST_PS_RIL_
#define _CUST_PS_RIL_

/***************************************************************************
                                 TARGET
****************************************************************************/
#define FEATURE_PS_ATT_SPECIFIC /* USA - AT&T */


/***************************************************************************
                                 COMMON
****************************************************************************/
/* 2012.2.22. kdhyun : PS Telephony Manager 정의 및 Interface 정의
              - 수정 파일 : service_manager.c, GSMPhone.java, Android.mk
              - 추가 파일 : IPSTelephony.aidl, PSInterfaceManager.java
*/
#define FEATURE_PS_TELEPHONY_MGR

/* 20110324 PS10 SUNI : FTM Mode/Debug Screen in LINUX */
#define FEATURE_PS_DEBUG_MENU

#ifdef FEATURE_PS_DEBUG_MENU
  #define FEATURE_FTM_UICC_MENU_RIL
#endif

/* 2011.08.10 PS1 Soobeen: SYSTEM DEBUG MENU */
#define FEATURE_PS_SYSTEM_DEBUGMODE

/* LTE band Locking */
#define FEATURE_LTE_BAND_LOCK

/*
2012.2.22. kdhyun : QMI_SAR를 이용하여 OEM QMI interface 사용하도록 함
                    - 수정 파일 : specific_absorption_rate_v01.c, qcril_other.c
*/
#define FEATURE_SKY_CP_OEM_QMI_ACCESS

/*
2012.2.22. kdhyun : QMI를 통한 NV Interface 추가 (QMI_SAR 이용)
                    - 수정 파일 : qcril_other.c, qcril_system_debug_nv.c, qmi_proxy.c, specific_absorption_rate_v01.c/h
*/
#define FEATURE_SKY_CP_NV_ACCESS_WITH_QMI

/*
2012.2.22. kdhyun : QMI를 통한 command 추가 (QMI_SAR 이용)
                    - 수정 파일 : qmi_proxy.c, specific_absorption_rate_v01.c/h
*/
#define FEATURE_SKY_CP_OEM_COMMANDS_WITH_QMI

/*
2012.4.6. kdhyun : CTS 테스트시 CDMA 제거
                   - 수정 파일 : LINUX/android/device/qcom/common/common.mk
*/
#define FEATURE_PS_CTS_CDMA_REMOVE

// android/build/core/config.mk 에 define되어있음...
#ifdef T_SKY_CP_DEBUG_LOG_FUNC
/*
    SD 카드를 통한 DM Logging
        - android\vendor\qcom\proprietary\dm-monitor
        - android\vendor\qcom\proprietary\syslog
        - android\packages\apps\DMLogging
*/
#define FEATURE_SKY_CP_DM_LOG_STORE_TEMP_MEMORY
#endif/* T_SKY_CP_DEBUG_LOG_FUNC */

/***************************************************************************
                                 AS
****************************************************************************/

/* 20101101 PSTeam Sungoh AT&T RSSI Requirement implementation
13340 V_42 <CDR-RBP-1030>*/
#define FEATURE_ATNT_RSSI_BAR
#define FEATRUE_PS_NO_SERVICE_AFTER_RLF 
/***************************************************************************
                                 MM
****************************************************************************/
/*
2010.11.09. kdhyun : MCC/MNC 001/01, 001/02, 002/01인 경우 network name을 올려주지 않도록 함
                     - 수정소스 : qcril_qmi_nas2.c
*/
#define FEATURE_PS_NOT_USED_QUALCOMM_DEFAULT

/*
2011.07.21. Soobeen : Manual selection의 결과에 RAT 정보 추가
2012.4.2. kdhyun : Disable Managed Roaming
                   - 수정파일 : GsmServiceStateTracker.java
2012.6.20. kdhyun : Manual selection, Limited svc 상태에서 automatic 선택시 LU Accept 되었어도 timeout 걸리는 문제 수정
                    - 수정파일 : qcril_qmi_nas.c
*/
#define FEATURE_PS_MANUAL_SELECTION_RAT
#define FEATURE_PS_MANUAL_SELECTION_RAT_AUTOMATIC

/*
2011.11.22. kdhyun : TIMEZONE 정보 display할 때 DST는 고려하지 않음
                     - 관련 TC : GSM-BTR-1-9275-2, LTE-BTR-1-4212
                     - 수정파일 : DateTimeSettings.java
*/
#define FEATURE_PS_TIMEZONE_WITHOUT_DST

/*
2012.1.2. kdhyun : EF_SPN의 값이 공백일 때 null로 return 하도록 함
                   - 수정파일 : IccRecords.java
*/
#define FEATURE_PS_EF_SPN_CHECK

/*
2012.1.17. kdhyun : MM reject cause display for AT&T requirement
2012.7.25. byungju : when reject is #13 and #15, not display emergency call only 
                    - 수정파일 : GsmServiceStateTracker.java, strings.xml
*/
#ifdef FEATURE_PS_ATT_SPECIFIC
#define FEATURE_PS_MM_REJECT_CAUSE_DISPLAY
#endif

/*
2012.1.17. kdhyun : Out of service state와 Emergency only state를 구분해서 적절한 display 하도록 함
                    - 수정파일 : MSimSubscriptionStatus.java, RadioInfo.java, Status.java
*/
#ifdef FEATURE_PS_ATT_SPECIFIC
#define FEATURE_PS_SEPERATE_OUT_OF_SERVIE_AND_EMERGENCY_ONLY
#endif

/*
2012.1.27. kdhyun : EF_PNN의 LAC range 체크 오류 수정
                    - 수정파일 : OplRecords.java
*/
#define FEATURE_PS_PNN_LAC_RANGE_BUG_FIX

/*
2012.3.13. kdhyun : NITZ를 통해 받은 시간 정보 중 Timezone 및 DST가 제대로 업데이트 되지 않는 문제로 최신 zoneinfo 파일로 교체함(기존 2011l, 교체 2012b)
                    (관련 TC : GSM-BTR-1-9275, GSM-BTR-1-9350 fail)
2012.04.02 kim youngkyun : GSM-BTR-1-9275 항목 fail로 인하여 zoneinfo 2012b파일을 2011g로 교체
                    - 수정파일 : LINUX\android\bionic\libc\zoneinfo\zoneinfo.dat, zoneinfo.idx, zoneinfo.version
*/
#define FEATURE_PS_ZONEINFO_BUG_FIX

/*
2012.3.21. kdhyun : android단의 pref mode 초기값 GWL로 변경
                    - 수정파일 : RILConstants.java
*/
#define FEATURE_PS_DEFAULT_NETWORK_MODE_GWL

/*
2012.4.5. kdhyun : AlphaLong이 null일 경우 AlphaShort 사용하도록 함
                   - 수정파일 : GsmServiceStateTracker.java
*/
#define FEATURE_PS_ALT_OPERATOR_NAME_USE

/*
2012.4.5. kdhyun : Restricted notification 보이지 않도록 함
                   - 수정파일 : GsmServiceStateTracker.java
*/
#define FEATURE_PS_RESTRICTED_NOTIFICATION_NOT_SUPPORT

/*
2012.4.27. kdhyun : 30145 patch후 qcril.c의 android_request_id가 QMI_RIL_FW_ANDROID_REQUEST_HNDL_MAX_EVT_ID (127)로 제한됨.
                    Ril.h에 추가된 event id값을 추가하여 132 값보다 큰 값으로 설정함.
                    - 수정파일 : qcril.c, ril.h
                    - 1048 버전 only
*/
#define FEATURE_SKY_CP_RIL_FW_ANDROID_REQUEST_HNDL_MAX_EVT_ID_FIX

/*
2012.4.27. kdhyun : 30145 patch후 qcril.c 추가된 RIL관련 ITEM 처리 구조의 변화로 간헐적으로 이밴트 처리에 문제가 됨.
                    Voice & Data REG STATE, Auto & Malual Network Selection 관련 RIL event는 30145 patch 이전과 같이 처리되도록 
                    qmi_ril_fw_dedicated_thrd_exec_android_requests_set 테이블에서 제외함.
                    - 1048 버전 only
*/
#define FEATURE_SKY_CP_FW_DEDICATED_ANDROID_REQ_BUG_FIX

/*
2012.04.27. cmjung : GWL모드에서 데이터 차단 설정 후 네트웍 메뉴 진입시 GWL로 보이지 않고 GW 모드로 설정된 것처럼 보임.
                                        Modem pref mode nv(00010)값과 Sync.
                    - 수정파일 : GsmServiceStateTracker.java, ServiceStateTracker.java
*/
#define FEATURE_SKY_CP_GET_CM_PERSIST_PREF_MODE

/*
2012.5.30. kdhyun : *#*#4636#*#* -> Phone information -> Set preferred network type 동작안하도록 함
                   - 수정파일 : RadioInfo.java
*/
#define FEATURE_PS_BLOCK_GOOGLE_NETWORK_MODE_SETTING

/*
2012.06.05. cmjung : deleted 'No service' in Lock screen
                    - Modification file : KeyGuardUpdateMonitor.java, CarrierLabel.java(modified by US3team )
*/
#define FEATURE_PS_NO_SVC_BLOCK

/*
2012.6.13. kdhyun : India에서 MCC 404, 405가 혼재되는 경우 hard-coding된 operator name을 제대로 못 가져오는 문제 수정
                   - 수정파일 : qcril_qmi_nas2.c
*/
#define FEATURE_PS_REORDERING_MCC_404_405

/*
2012.6.19. cmjung : modify that do not connect LTE network because of Q patch 1536
2012.7.18. kdhyun : 153613에 적용됨 -> undefine
*/
//#define FEATURE_PS_CONNECT_LTE_NETWORK_BUG_FIX

/*
2012.6.29. cmjung : modified the network name  mismatch in notification after reject from network.
                    - modified file : GSMPhone.java, NotificationMgr.java
*/
#define FEATURE_PS_MATCH_NETWORK_NAME_IN_NOTIFICATION_AFTER_REJECT

// centralized_eons_supported 기능 동작에 따라 network name 및 manual search list 표시 오류가 발생함. (153613 버전 부터)
#define FEATURE_SKY_CP_CENTRALIZED_EONS_NOT_SUPPORTED

/***************************************************************************
                                 CC
****************************************************************************/

/*ygkim 2012.1.3 AT&T Local CLIR
Added 10776 TC & Local CLIR for AT&T, for PTCRB CLIR TC, this feature have to be blocked.
*/
#define FEATURE_GET_CLIR_LOCAL

/*Eunseong, 2012.1.3 Error Cause by FDN Enabled
Processed USSD FDN Check Failure as Not Unsolicited res But OnRequestComplete
*/
#define FEATURE_PS_USSD_FDN_CHECK_ERROR_RELAY

/*ygkim 2012.1.3 Qualcomm bug fix
Shortened timing of airplane mode change (on/off), since PRESTO GB  
RADIO_POWER 에 대해서 MDM으로 부터 Callback message가 QMI interface에서 사라지는 현상이 확인 됨 
이 경우 처리를 위해  10초 timer 만료 후 RADIO_POWER complete를 보내고, Qcril 관련 state를 update 한다.
*/
//#define FEATURE_RADIO_POWER_CB_SUCCESS

/*hnlee 2012.1.3 UICC Check for call waiting
Blocked to access call forwarding and call waiting menu on without SIM.
 */
//#define FEATURE_SIM_PRESENT_CHECK_FOR_CALL_SETTING 

/*hnlee 2012.1.3 Call end Reason 
Added Call End raeson for Reject Indication
Added Emergency call fail cause
*/
#define FEATURE_ADD_CALL_END_REASON

#define FEATURE_PRESTO_ADD_ECC_FAIL_CAUSE

/*ygkim 2012.1.3 Qulaocmm bug fix
Ref> Presto GB DTMF issue related to critical section 
dial xxxx P(W) xxxxxxxx DTMF state machine 동작 오류 ; DTMF Start Resp 이후 token id  delete 후  DTMF Stop Req Event 처리 하면서 token id 새로 등록 되는 순서에 
unsync 현상 발생해서, token id delete를 하기전에 , DTMF Stop Req Event 가 먼저 처리 되면서, token id 등록 못하는 현상 발생
dial xxxx P(W) xxxxx 에서는  > DTMF 를 보내 준다. 
active call 상태에서 DTMF 를 보내는 것은 >DTMF Start ,  > DTMF Stop을 보내 주는 경우는 영향이 없다.
*/
//#define FEATURE_DTMF_EVENT_UNSYNC

/* SUNI 20120215 OSCAR PLM 317
  After the call was ended at local, the state 'ACTIVE' is returned in result to get current call 
  Eunseong, Juhyun 2012. 06.09, 
  modified call active state report symptom after call hangup, because this makes multicall-not-end issue.
 */
#define FEATURE_PS_CC_RETURN_NULL_DURING_CALL_DISCONNECTING

/* Eunseong, Juhyun, 20120320, Incoming call ring issue.
Fixed Qualcomm bug whichi is not sending UNSOL_CALL_RING
Blocked this feature due to release Qualcomm patch
*/
//#define FEATURE_INCOMING_ALERTING_UNSOL_REP

/*Eunseong, 20120330, SKY feature
Made call cause update at first call after registrataion,
*/
#define FEATURE_SKY_CP_LAST_CALL_FAILURE_FORCED_INIT

/*Eunseong, 20120418 
  USSD failure return in case of not getting respond of NW
*/
#define FEATURE_PS_USSD_FAIL_RETURN_NW_NOT_RESPONSE

/*Eunseong, SKY feature
When ps service is only in service state,  cs domain sevice will be no service, and no service icon without this feature
*/
#define FEATURE_SKY_CP_SUPPORT_PS_ONLY_MODE

/*
2012.06.13, From 8th MT missed call, MT call couldn't be forwarded to UI on qcril 1534 bug, it is a temporary code 
SKY feature,
Blocked this feature because of releasing CR370978 
*/
//#define FEATURE_SKY_CP_MISSED_CALL_BUG_FIX

/*Juhyun, Eunseong 2012.05.17 
Added not to use QCRIL.c Flow control 
becasue Qualcomm qcril flow control couldn't process same name-ril requests which occured fast continuosly
*/
#define FEATURE_PS_CC_DTMF_BLOCK_FLOW_CONTROL

/* juhyun 2021.10.10 MAGNUS 
Except  " timeout value( 2000 -->5000)"
ygkim 2012.05.31 OSCAR
we soemtimes have no return value to qmi_client_send_msg_sync(QMI_DMS_SET_OPERATING_MODE_REQ_V01) callback. but Modem's operating mode action is success.
this is unsync issue between modem and Android.
if this is failure by timeout for callback. we change failure to success. also change timeout value( 2000 -->5000) 
*/
#define FEATURE_HANDLE_QMI_TIMEOUT_SET_MODEM_OP_MODE
/***************************************************************************
                                 Data
****************************************************************************/

/* 20110422_PS11_DATA_PKH
 - init.rc 파일 
 - net.tcp.buffersize.default 4096,87380,262140,4096,16384,262140
 - tcp advertisement window scals value  3 : init.qcom.rc
*/
#define FEATURE_DATA_CHANGE_TCP_CONFIGRATION

/* 20110422_PS11_DATA_PKH
 - 특정 VPN 서버 접속 안되는 문제 (참고 : android 는  SSL VPN 과 Cisco VPN 미지원)
 - external\mtpd\L2tp.c 및 kernel config ( kernel\arch\arm\config\ 에서 이미 define된 내용은 주석처리하고 y로 설정 )
*/
#define FEATURE_DATA_VPN_FIX

/* 20110422_PS11_DATA_PKH
 - no service event 가 발생할 경우 실제 data 종료되지 않고 disconnected 로 broadcast 되어 default route 삭제되고 
   바로 in service 오게되면 아래단으로는 data 연결되어 있지만 상위에는 연결되지 않은 것으로 보이는 문제 
 - dataconnectiontracker.java , BSP 별로 주석처리되어 필요 없는 경우 잇음. 
 - ICS 버전 (OSCAR 인 경우) phonebase.java에  mOosIsDisconnect 제공 -> false로 수정.
*/
#define FEATURE_DATA_NO_SERVICE_BUG_FIX

/* 20110422_PS11_DATA_PKH
 - CTS testTrafficStatsForLocalhost test 를 위한 kernel config 에 CONFIG_UID_STAT=y 로 수정. 
 - FEATURE_DATA_CONFIG_UID_STAT 를 FEATURE_DATA_FOR_CTS_TEST rename
 - dun file permission 변경 및 /dev/dun -> /dev/pantech/dun 으로 이동 -> OSCAR ICS 기준 으로 /ata/dun 으로 폴더 이동되어 배포 // 미적용 !!
 - ip permission 수정 4775 -> 0775 (init.qcom.modem_links.sh) OSCAR ICS 기준으로 -> init.qcom.rc 에서 SUID bit 제거되어 배포 // 미적용  !! 
 */
#define FEATURE_DATA_FOR_CTS_TEST

/* 2012/02/01 by kwanghee
    Not support IPv6 type DNS Query
    Ignore test connection for disconnection problem
    getaddrinfo.c
*/
#define FEATURE_PS_NOT_SUPPORT_IPV6_DNS_QUERY

/* 2012/06/04 by kwanghee
    system.prop
    Fit WIFI MTU size for RMNET when tethering
*/
#define FEATURE_PS_CHANGE_MTU


/* 2011/02/07 by kwanghee
    for Disable DUN
*/
#ifdef FEATURE_SKY_CP_NV_ACCESS_WITH_QMI
#define FEATURE_PS_DISABLE_DUN
#endif

/* 2011/06/25 by kwanghee
    for Disable CNE 

*/
#define FEATURE_PS_DISABLE_CNE

/* 2011.11.24 Yi Dongseok
 Fix the problem that data and WIFI icon displayed side by side
 notify SERVICE_TYPE_DEFAULT connected one more */
//#define FEATURE_NOTIFY_SERVICE_TYPE_DEFAULT_ONE_MORE

/* 2011/11/28 Yi Dongseok */
#define FEATURE_PS_APN_TYPES

/* 2011/11/28 Yi Dongseok */
#ifdef FEATURE_PS_APN_TYPES
#define FEATURE_PS_ENTITLEMENT
#endif /* FEATURE_PS_APN_TYPES */

/* 2012/01/31 by kwanghee 
   for AlwaysOn Settings
*/
#define FEATURE_PS_ALWAYSON


// 2011/12/02 Yi Dongseok
// change dhcp lease time 1h -> 24h for tethering
#define FEATURE_TETHERING_DHCP_LEASE_TIME
/* 2012/01/31 by kwanghee 
   for debugging, will be deleted.
*/
#define FEATURE_PS_DEBUG_TEMP

/* 2012/02/27 by kwanghee 
   Whenever data was disabled/enabled in easysetting, cannot update ui  in CallSetting.
*/
#define FEATURE_PS_UPDATE_MOBILE_DATA_IN_CALLSETTINGS

/* 2012.03.05 msseo
     Domestic roaming feature migration : domestic roaming인 경우 roaming false로 처리하여 data service 가능하도록 함.
*/
#define FEATURE_PS_DOMESTIC_ROAMING

/* 2012.03.05 msseo
     Acitng HPLMN feature migration : SIMRecords에서 Acting HPLMN 읽어옴.
*/
#define FEATURE_PS_ACTING_HPLMN

/* 2012.03.05 msseo
     PS attach 시에 attached icon 보여주도록 추가
*/
#define FEATURE_PS_ATTACHED_ICON

/* 2012/04/02 by kwanghee 
  for adding network type for hspa+
*/
#define FEATURE_PS_ADD_NETWORK_TYPE_FOR_HSPAP


/* 2012/04/02 by kwanghee 
  for prevent to send mms toward secondary dns address
  Qualcomm original source code have bug when writePidDns excuted in ConnecitivityService.java
*/
#define FEATURE_PS_WRITE_PID_DNS_BUF_FIX

/* 2012/04/02 by kwanghee 
  for porting authentec module
*/
#define FEATURE_PS_DATA_AUTHENTEC_VPNCLIENT

/* 2012/05/08 by kwanghee 
  for setting tcp liberal enable in init.qcom.rc
*/
#define FEATURE_PS_DATA_TCP_BE_LIBERAL_ENABLE
/* 2012/05/24 Yi Dongseok
  Disable current data system
*/
#define FEATURE_PS_DISABLE_CURRENT_DATA_SYSTEM

/* 20120731 park.kwanghee
 for fix VPN UI bug when VPN Connection Closed in Wi-Fi */
#define FEATURE_DISCONNECT_VPN_CLOSE_DIALOG

/* 
 - Phone Interface를 접근 못하는 APP를 위하여 AIDL을 추가
 - aidl 선언 : ISkyDataConnection.aidl
 - 인터페이스 구현 : MMDataConnectionTracker.java
 - 추가 구현 파일 : DataPhone.java, Phone.java , SkyDataConInterfaceManager.java
 - aidl 서비스 추가 : service_manager.c에 .aidl 추가
 - make 파일 수정 : android/framework/base/Android.mk 수정
 - telephony/java/com/android/internal/telephony/ISkyDataConnection.aidl 추가
*/
#define FEATURE_ISKY_DATA_CONNECTION


/*
- datausage 메뉴에서 background data 차단 시 for loop 를 돌며 UID 별로 ip table 에 set 해 주어 
   app 에서 시간 내 받지 못해 anr 발생 및 background data 차단 설정 시 booting 중 system 에서 
   anr 발생하는 문제 수정   =>각 UID 별로 event 로 처리되도록 함.
- android_filesystem_config.h  에 Define 된 UID 로 system UID 값 변경. 
- system/bin/iptables 는 system 권한으로 되어 잇으나 ip6tables 는 shell 로 되어 잇어 system 으로 변경.
   -NetworkPolicyManagerService.java
*/
#define FEATURE_SKY_DS_BACKGROUND_RESTRICT_BUG_FIX

/*
- system/bin/iptables 는 system 권한으로 되어 잇으나 ip6tables 는 shell 로 되어 잇어 system 으로 변경.
   -android_filesystem_config.h
*/

#define FEATURE_SKY_DS_IP6TABLE_UID_BUG_FIX

/* 
 - startUsingNetworkFeature에서 Reconnect 호출시 Fail이 발생하여도 Phone.APN_REQUEST_STARTED을 리턴하여 Application에서 혼동을 제공함
 - reconnect 실패시 APN_REQUEST_FAILED를 리턴하도록 수정
*/

#define FEATURE_SKY_DS_BUG_FIX_STARTUSINGNETWORKFEATURE

/*
 - 데이터로밍 체크 후 팝업에서 조명 off 되고 홀드 해제하여 팝업 허용 선택시 체크되지 않는 현상 수정
*/
#define FEATURE_SKY_DS_FIX_ROAM_CHECK_UI_BUG

/*
 - efs easer 후 최초 data 연결 시 preferred apn 등록하면서 data 재 연결하는 문제 수정 from 내수
*/
#define FEATRUE_SKY_SET_PREFERAPN_BUG_FIX

/*
- data registration statue query 시 fail 발생하면 data service state 가 no service, radio tech 는 none 으로 처리되는 문제로 
   이전 값을 newSS 에 저장하여 이후 response fail 발생시 이전 값 유지하도록 함. 
- GsmServiceStateTracker.java
*/
#define FEATURE_SKY_DS_DATA_REGISTRATION_QUERY_FAIL_RECOVERY

/*
-  usb tethering/wifi hotspot 시 google dns 서버 사용으로 일부 사이트 연결되지 않는 문제 
- ConnectivityService.java  tethering.java
*/
#define FEATURE_SKY_DS_SET_TETHERED_DNS
/*
  Park.kwanghee added 20120329
  tethering.java 에서 addUpstreamV6Interface()/ removeUpstreamV6Interface() 시 
  add/remove 할때 NetworkManagementService 에서 IllegalStateException 을 throw 하는데
  해당 함수의 catch 에서는 RemoteException만 catch 해서 exception 처리되지 않고 상위로 전달되면서 발생한 문제 
  from 내수.
*/
#define FEATURE_SKY_DS_EXCEPTION_CATCH_BUG_FIX

#define FEATURE_SKY_DS_SYNC_CS_SERVICE_STATE

#define FEATURE_SKY_DS_SET_TCPBUF_IN_RAT_CHANGE

/*
 added by park.kwanghee
 wifi/3G icon appeared when thering is working.
 Tethering on -> data off -> data on -> wifi on
*/
#define FEATURE_SKY_DS_TETHERED_BUG_FIX

#define FEATURE_SKY_DS_ICON_NO_SRV_CR347576

#define FEATURE_SKT_DS_RESUME_DOWNLOAD_FOR_WIFI_TO_DATA_CHANGE

#define FEATURE_SKY_DS_SBA_1045_CR350813

#define FEATURE_SKY_DS_QOS_DISABLE

/* 2012.05.10 Yi Dongseok 
 * AT commands for last and total data usage
 */
#define FEATURE_PS_ATCMD_DATA_USAGE

#define FEATURE_SKY_DS_FAST_DORMANCY

/* 2012.06.05 msseo
    2G voice 콜 중 NetworkInfo의 isAvailable()이 false로 리턴되는 증상 수정
    Data suspend 되더라도 network availalbe 상태 유지
*/
#define FEATURE_PS_DATA_ALLOWED_CONDITION

/* 2012.07.06 Yi Dongseok 
 * Disable early data connections.
 * Qualcomm's default setting is 3. 3 SYNs concurrently.
 */
#define FEATURE_PS_DISABLE_EARLY_DATA_CONNECTIONS

/* 2012.07.16 Yi Dongseok 
 * Enable CONFIG_IP_MULTICAST for IGMP support.
 */
#define FEATURE_PS_ENABLE_CONFIG_IP_MULTICAST

/* 2012.07.02 jun.hyojin
    APN update for FOTA and Apn changed
*/
//#define FEATURE_APN_FOTA_UPDATE

/* 2012.08.17 Yi Dongseok 
 * do NOT try TYPE_HIPRI as upstreamType after tethering activated.
 * please DELETE this feature if Android version is Jelly Bean or above.
 */
#define FEATURE_PS_DISABLE_TRY_TYPE_HIPRI_TETHERING

/* 2012.08.16 msseo
    Do not display data icon when encryption password isn't verified
*/
#define FEATURE_PS_DATA_ICON_IN_ENCRYPTION_STATE

/* 2012/10/30 by kwanghee
    delay time to turn on mobile hotspot during wifi connected
    EntitlementPDPService.java
*/
#define FEATURE_PS3_WIFI_HOTSPOT_DELAY


/***************************************************************************
                                 SIM
****************************************************************************/
/*ygkim 2012.1.3 PIN retry flag
SIM State change 정보에 SIM Retry 정보를 포함해서 전달.
*/
#define FEATURE_PS_PIN_RETRY_INIT
/* ygkim 2012.1.3  PS Card Type
GingerBread 이후 OS에서는 Android UICC Manager내에서  제공 한다.
*/
#define FEATURE_PS_CARD_TYPE
/*ygkim 2012.1.3 virtual sim flag
SIM State Change 정보에 Virtual sim 정보를 포함해서 전달한다.
SIM Manager 생성 및 Factory Command 과정에서 ECC List handling에 이용 
*/
#define FEATURE_PS_IS_VIRTUAL_SIM
/*ygkim 2012.1.3 Sim State structure
Android 와 Qcril 사이의 SIM State Structure 정의 값 통일 
*/
#define FEATURE_PS_ILLEGAL_SIM_STATE_MISMATCH

/*ygkim 2012.1.3 Perso Ready state
MSC 10776 testcase 과정에서 
Android SIM Ready Delay 현상 대응 
Modem은 Ready상태이지만, 같은 시점에 Android는 Not Ready로 남음 
2012.04.09 현재 Qc version 에서는 오류 동작 발생 안함.
*/
#undef  FEATURE_EARLY_APP_STATE_READY

/*ygkim 2012.1.3  Card Power down 
set Phone booting Sequence without Card power down action. 
1) ~1045 Patch Source: this feature Disable
2) 1048 Patch Source ~ 1053,~ : this feature Enabled 
*/
#define FEATURE_QMI_NO_CARD_POWER_DOWN_IN_AIRPLANE 


/*<---Start UICC Debug menu handling */
/*ygkim 2012.1.3 Debug menu interface
Android OS 에서 IMPU 정보 읽어 들이는 interface 제공 
*/
#define FEATURE_PS_IMPU_READ
/*ygkim 2012.1.3 Debug menu interface
Android OS에서 ISIM 정보 읽어 드링는 interface 제공 
*/
#define FEATURE_PS_ISIM_READ
/*<---End UICC Debug menu handling  */

/*QE   */
/*ygkim 2012.1.3 Qcril 안정화 
Qualcomm에서 Buff size를 255로 증가 시킴. MDM9200:50 -->Pantech:100 -->Q8960:255
*/

/*ygkim 2012.1.3 Card Power down
Q8960: airplane on /off is handling without Card Power down/up action 
1)~1045 Patch Socure: This feature Enabled
2)1048~ 1053,~:this feature Enabled but code is modified
*/
#define FEATURE_RADIO_POWER_WITHOUT_CARD_PWDL


/*ygkim 2012.2.27 Phone power on without No Card Power down 
Q8960 :qcril_qmi_nas_is_apm_enabled() APM handling modifed
*/
#define FEAURE_APM_DISABLED


/*ygkim 2012.2.27 Limited Card error 
정상 적인 Card Error 상태가 아닌대 Card error event가 trigger  되는 경우 android interface에서 check 해서
event 를 전달 하도록 수정. 
*/
#define FEAURE_LIMITED_CARD_ERROR_TRIGGER

/*ygkim 2012.03.26  
Andorid 에서 보내는 STK RIL Ready 수신 후 First proactive command(SETUP) 전송을 위해 사용 되는 RESEND timer(5sec)를 수정
5sec delay 없이 바로 보내도록 수정.(0.001초 후 전달 )
*/
#define FEATURE_PS_CHG_RESEND_TIMER

/*ygkim 2012.04.01
IMS Service (VoLTE , Instance Message, Push to talk,  Session based service ....) 지원을 위해 , ISIM(USIM Family App) Application 지원 
을 위해 제공 . 2H.2013 출시 모델부터  적용 
*/
#undef FEATURE_APP_ISIM

/*ygkim 2012.4.16 CSIM or RUIM exception error 
CSIM or RUIM card type이 활성 화 될 경우 ,  Android Frameworks 작업에 exception handling이 발생함.
인식 안 하도록 수정. 
iccCardApplicationStatus returns App type , Ril.java do not update CSIM or RUIM app type status
*/
#define FEATURE_DISABLE_CSIM_RUIM

/*ygkim 2012.4.17 Ril demon debug
Ril.cpp interface를 이용하는 msg들에 대한 Raw Data trace를 위해  Radio Log 에 PRINT를 한다. 
*/
#undef FEATURE_PS_PRINT_RIL_RAWDATA

/*ygkim 2012.4.17 Card Status debug
IccCardApplicationStatus.java에서  Card Status Raw Data Trace를 위해 Radio Log에 PRINT를 한다.
*/
#define RIL_UICC_DEBUG_PRINT

/*ygkim 2012.07.10 
fix NO SIM Card issue caused byunknown state : CR363577 from SBA153612
2012.7.18. kdhyun : 153613에 적용됨 -> undefine
*/
//#define FEATURE_CR363677_SBA153612

/***************************************************************************
                                 SIM-WIFI
****************************************************************************/

/****************************************************************************
					NFC-BIP(ISIS) AT&T , VZW
***************************************************************************/
/*ygkim 2012.05-02 Android NFC interface
add NFC-BIP interfce [PS1 占쏙옙占쏙옙占쏙옙]
*/
#define FEATURE_QCRIL_UIM_QMI_APDU_ACCESS 
#ifdef FEATURE_QCRIL_UIM_QMI_APDU_ACCESS
/*NFC -BIP 용 RIL command handling
1)2012.05-02  RIL_REQUEST_SIM_APDU -->RIL_REQUEST_SIM_TRANSMIT_BASIC 사용하도록 대체 
2)2012.05.14 Rild reset Patch 추가 
3)2012.05.20 include Smart card API request message for capability  handling to do flow control
*/
#define FEATURE_PANTECH_SMART_CARD_API

#endif
/**ygkim 2012.05-07 Android NFC interface
add NFC-BIP interfce 
info : Change error value from RIL_E_GENERIC_FAILURE 
       to RIL_E_MISSING_RESOURCE, 
          RIL_E_NO_SUCH_ELEMENT, 
          RIL_E_INVALID_PARAMETER
       or RIL_E_GENERIC_FAILURE 
when : 20120503
who  : JKA
*/
#define FEATURE_P_UICC_APDU_ACCESS_RESP_DETAIL

#endif //CUST_PS_RIL_H


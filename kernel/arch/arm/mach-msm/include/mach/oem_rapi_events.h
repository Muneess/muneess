
#ifndef OEM_RAPI_EVENT_H
#define OEM_RAPI_EVENT_H

#define OEM_RAPI_MAX_CLIENT_INPUT_BUFFER_SIZE  128
#define OEM_RAPI_MAX_CLIENT_OUTPUT_BUFFER_SIZE 128
#define OEM_RAPI_MAX_SERVER_INPUT_BUFFER_SIZE  128
#define OEM_RAPI_MAX_SERVER_OUTPUT_BUFFER_SIZE 128

typedef enum oem_rapi_client_event
{
  OEM_RAPI_CLIENT_EVENT_NONE = 0,
  OEM_RAPI_CLIENT_EVENT_EM_RF_TX_ON,
  OEM_RAPI_CLIENT_EVENT_EM_RF_TX_OFF,
  OEM_RAPI_CLIENT_EVENT_EM_RF_RX_ON,
  OEM_RAPI_CLIENT_EVENT_EM_RF_RX_OFF,
  OEM_RAPI_CLIENT_EVENT_EM_PM_LOG_CONTROL,
#ifdef FEATURE_QISDA_ENG_MODE_PSDATA
  OEM_RAPI_CLIENT_EVENT_EM_PS_DATA_GET_CUR_CNT_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_PS_DATA_GET_LT_CNT_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_PS_DATA_RESET_LT_CNT_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_PS_DATA_STOP_LT_CNT_REQ,
#endif
  OEM_RAPI_CLIENT_EVENT_SYSTEM_GET_VER_REQ,
  OEM_RAPI_CLIENT_EVENT_EXP_QXDM_FILTER,
  OEM_RAPI_CLIENT_EVENT_EXP_QXDM_SAVING,
  OEM_RAPI_CLIENT_EVENT_EXP_EFS_SAVING,
  #ifdef FEATURE_QISDA_ENG_MODE_REG_MM
  OEM_RAPI_CLIENT_EVENT_EM_NAS_INFO_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_REG_INFO_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_MM_MM_INFO_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_MM_PLMN_LIST_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_MM_GMM_INFO_REQ,
  #endif
  OEM_RAPI_CLIENT_EVENT_EXP_QUERY_ERR_OFFSET,
  OEM_RAPI_CLIENT_EVENT_EXP_TRIGGER_ERR_FATAL,      
  OEM_RAPI_CLIENT_EVENT_EXP_EFS_DIR_LOOKUP,  
  #ifdef FEATURE_QISDA_ENG_MODE_SYS_INFO
  OEM_RAPI_CLIENT_EVENT_EM_GSM_MEAS_INFO_1_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_GSM_MEAS_INFO_2_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_GSM_SCELL_INFO_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_GSM_DED_INFO_1_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_GSM_DED_INFO_2_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_GSM_PT_INFO_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_WCDMA_MEAS_INFO_1_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_WCDMA_MEAS_INFO_2_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_WCDMA_MEAS_INFO_3_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_WCDMA_SCELL_INFO_1_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_WCDMA_SCELL_INFO_2_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_WCDMA_CONN_INFO_1_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_WCDMA_CONN_INFO_2_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_WCDMA_HSDPA_INFO_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_WCDMA_HSUPA_INFO_1_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_WCDMA_HSUPA_INFO_2_REQ,
  #endif
  OEM_RAPI_CLIENT_EVENT_EM_GPIO_READ_0_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_GPIO_READ_1_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_GPIO_READ_2_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_GPIO_WRITE_REQ,		        
  OEM_RAPI_CLIENT_EVENT_AIRPLANE_MODE_SETTING, 				
  OEM_RAPI_CLIENT_EVENT_EXP_QUERY_QXDM_OFFSET,  
  #ifdef FEATURE_QISDA_ENG_MODE_PDP
  OEM_RAPI_CLIENT_EVENT_EM_PDP_ABSTRACT_INFO_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_PDP_GENERAL_INFO_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_QOS_INFO_REQ,
  #endif
  OEM_RAPI_CLIENT_EVENT_NV_OP_REQ,
  OEM_RAPI_CLIENT_EVENT_GET_DRMKEY_REQ,
  OEM_RAPI_CLIENT_EVENT_TIME_REMOTE_SECURE_GET,
  OEM_RAPI_CLIENT_EVENT_TIME_REMOTE_SECURE_GET_JULIAN,
  OEM_RAPI_CLIENT_EVENT_TIME_REMOTE_SECURE_SET,
  OEM_RAPI_CLIENT_EVENT_TIME_REMOTE_SECURE_SET_JULIAN,
  OEM_RAPI_CLIENT_EVENT_GET_BT_MAC_ADDR_REQ,
  OEM_RAPI_CLIENT_EVENT_GET_WLAN_MAC_ADDR_REQ,
  OEM_RAPI_CLIENT_EVENT_GET_SERVICE_TAG_REQ,
  #ifdef FEATURE_QISDA_ENG_MODE_GPS_TESTMODE
  OEM_RAPI_CLIENT_EVENT_EM_GPS_SARF_MODE_SWITCH_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_GPS_SINGLE_SV_REQ,
  #endif
  OEM_RAPI_CLIENT_EVENT_UART_LOG_SETTING,
  OEM_RAPI_CLIENT_EVENT_DEFAULT_NV_RESTORE_REQ,
  OEM_RAPI_CLIENT_EVENT_ERASE_FLAGS_PARTITION_REQ,
  OEM_RAPI_CLIENT_EVENT_RECOVERY_FAIL_PAHES1_FLAG_SETTING, 				
  OEM_RAPI_CLIENT_EVENT_RECOVERY_FAIL_PAHES2_FLAG_SETTING, 				
  OEM_RAPI_CLIENT_EVENT_QCT_USB_DRIVER_FLAG_SETTING,
  #ifdef  FEATURE_QISDA_SHORTEN_SLEEP_MODE 
  OEM_RAPI_CLIENT_EVENT_SHORTEN_SEARCH_SLEEP_TIMER,
  #endif
  OEM_RAPI_CLIENT_EVENT_ADB_PERMISSION_FLAG_SETTING, 
  OEM_RAPI_CLIENT_EVENT_SET_TIMEZONE,
  OEM_RAPI_CLIENT_EVENT_NV_BACKUP_REQ,
  OEM_RAPI_CLIENT_EVENT_EXP_GET_RESET_OPTION,
  OEM_RAPI_CLIENT_EVENT_EXP_SET_RESET_OPTION,
  OEM_RAPI_CLIENT_EVENT_PBM_SYN_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_GPIO_READ_IN_SLEEP_0_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_GPIO_READ_IN_SLEEP_1_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_GPIO_READ_IN_SLEEP_2_REQ,
  OEM_RAPI_CLIENT_EVENT_EM_GPIO_WRITE_IN_SLEEP_REQ,              
  OEM_RAPI_CLIENT_EVENT_SIM_INFO_REQ,
  OEM_RAPI_CLIENT_EVENT_USB_CONTROL_REQ,
  OEM_RAPI_CLIENT_EVENT_MAX
} oem_rapi_client_event_e_type;


typedef enum oem_rapi_server_event
{
  OEM_RAPI_SERVER_EVENT_NONE = 0,
#ifdef FEATURE_QISDA_ENG_MODE_PSDATA
  OEM_RAPI_SERVER_EVENT_EM_PS_DATA_GET_CUR_CNT_IND,
  OEM_RAPI_SERVER_EVENT_EM_PS_DATA_GET_LT_CNT_IND,
#endif
  OEM_RAPI_SERVER_EVENT_EXP_QXDM_SAVING,
  OEM_RAPI_SERVER_EVENT_EXP_EFS_SAVING,
  #ifdef FEATURE_QISDA_ENG_MODE_REG_MM
  OEM_RAPI_SERVER_EVENT_EM_NAS_INFO_IND,
  OEM_RAPI_SERVER_EVENT_EM_REG_INFO_IND,
  OEM_RAPI_SERVER_EVENT_EM_MM_MM_INFO_IND,
  OEM_RAPI_SERVER_EVENT_EM_MM_PLMN_LIST_IND,
  OEM_RAPI_SERVER_EVENT_EM_MM_GMMINFO_IND,
  #endif
  OEM_RAPI_SERVER_EVENT_EXP_QUERY_ERR_OFFSET,
  OEM_RAPI_SERVER_EVENT_EXP_TRIGGER_ERR_FATAL,      
  OEM_RAPI_SERVER_EVENT_EXP_EFS_DIR_LOOKUP,  
  #ifdef FEATURE_QISDA_ENG_MODE_SYS_INFO
  OEM_RAPI_SERVER_EVENT_EM_GSM_MEAS_INFO_1_IND,
  OEM_RAPI_SERVER_EVENT_EM_GSM_MEAS_INFO_2_IND,
  OEM_RAPI_SERVER_EVENT_EM_GSM_SCELL_INFO_IND,
  OEM_RAPI_SERVER_EVENT_EM_GSM_DED_INFO_1_IND,
  OEM_RAPI_SERVER_EVENT_EM_GSM_DED_INFO_2_IND,
  OEM_RAPI_SERVER_EVENT_EM_GSM_PT_INFO_IND,
  OEM_RAPI_SERVER_EVENT_EM_WCDMA_MEAS_INFO_1_IND,
  OEM_RAPI_SERVER_EVENT_EM_WCDMA_MEAS_INFO_2_IND,
  OEM_RAPI_SERVER_EVENT_EM_WCDMA_MEAS_INFO_3_IND,
  OEM_RAPI_SERVER_EVENT_EM_WCDMA_SCELL_INFO_1_IND,
  OEM_RAPI_SERVER_EVENT_EM_WCDMA_SCELL_INFO_2_IND,
  OEM_RAPI_SERVER_EVENT_EM_WCDMA_CONN_INFO_1_IND,
  OEM_RAPI_SERVER_EVENT_EM_WCDMA_CONN_INFO_2_IND,
  OEM_RAPI_SERVER_EVENT_EM_WCDMA_HSDPA_INFO_IND,
  OEM_RAPI_SERVER_EVENT_EM_WCDMA_HSUPA_INFO_1_IND,
  OEM_RAPI_SERVER_EVENT_EM_WCDMA_HSUPA_INFO_2_IND,
  #endif
  OEM_RAPI_SERVER_EVENT_EM_GPIO_READ_0_IND,
  OEM_RAPI_SERVER_EVENT_EM_GPIO_READ_1_IND,
  OEM_RAPI_SERVER_EVENT_EM_GPIO_READ_2_IND,
  OEM_RAPI_SERVER_EVENT_EXP_QUERY_QXDM_OFFSET,  
  #ifdef FEATURE_QISDA_ENG_MODE_PDP
  OEM_RAPI_SERVER_EVENT_EM_PDP_ABSTRACT_INFO_IND,
  OEM_RAPI_SERVER_EVENT_EM_PDP_GENERAL_INFO_IND,
  OEM_RAPI_SERVER_EVENT_EM_QOS_INFO_IND,
  #endif
  #ifdef FEATURE_QISDA_ENG_MODE_GPS_TESTMODE
  OEM_RAPI_SERVER_EVENT_EM_GPS_SARF_MODE_SWITCH_IND,
  OEM_RAPI_SERVER_EVENT_EM_GPS_SINGLE_SV_IND,
  #endif
  OEM_RAPI_CLIENT_EVENT_DEFAULT_NV_RESTORE_IND,
  OEM_RAPI_SERVER_EVENT_SET_TIMEZONE,
  OEM_RAPI_SERVER_EVENT_NV_BACKUP_IND,
  OEM_RAPI_SERVER_EVENT_EXP_GET_RESET_OPTION,
  OEM_RAPI_SERVER_EVENT_EXP_SET_RESET_OPTION,
  OEM_RAPI_SERVER_EVENT_EM_GPIO_READ_IN_SLEEP_0_IND,
  OEM_RAPI_SERVER_EVENT_EM_GPIO_READ_IN_SLEEP_1_IND,
  OEM_RAPI_SERVER_EVENT_EM_GPIO_READ_IN_SLEEP_2_IND,
  OEM_RAPI_CLIENT_EVENT_SIM_INFO_IND,
  OEM_RAPI_SERVER_EVENT_MAX
} oem_rapi_server_event_e_type;
#endif

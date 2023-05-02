/***************************************************************************
*   Copyright (C)2011 - 2017 ARM Ltd (or its subsidiaries)
*   All Rights Reserved
****************************************************************************/

#ifndef _ARM_HDCP_ERRORS_H
#define _ARM_HDCP_ERRORS_H

/*! \file ARM_Hdcp_Errors.h
This module includes all HDCP error codes
*/
#define ARM_ERROR_CODE_BITS 24
#define	ARM_SUBSYSTEM_HDCP_DEF 0x0C	// to sync with DX_VOS_Errors.h definition

#ifndef DX_SUCCESS
#define DX_SUCCESS 0
#endif

typedef enum tagEArmHdcpErrors {
    ARM_HDCP_ERROR_FIRST = (ARM_SUBSYSTEM_HDCP_DEF << ARM_ERROR_CODE_BITS),            // 0x0C000000
    ARM_HDCP_UNSUPPORTED = ARM_HDCP_ERROR_FIRST,                                       // 0x0C000000
    ARM_HDCP_ILLEGAL_HDCP_MESSAGE_RECEIVED,                                            // 0x0C000001
    ARM_HDCP_UNEXPECTED_HDCP_MESSAGE_LENGTH,                                           // 0x0C000002
    ARM_HDCP_UNEXPECTED_OUTBOUND_TRANSPORT_THREAD_MESSAGE,                             // 0x0C000003
    ARM_HDCP_INTERFACE_PARAMETERS_CONFLICT,                                            // 0x0C000004
    ARM_HDCP_BAD_PARAMETERS,                                                           // 0x0C000005
    ARM_HDCP_CONNECTION_OVERALL_TIMEOUT,                                               // 0x0C000006
    ARM_HDCP_STREAM_COUNTER_OVERFLOW,                                                  // 0x0C000007
    ARM_HDCP_DEVICE_TYPE_MISMATCH,                                                     // 0x0C000008
    ARM_HDCP_NON_ACTIVE_SESSION,                                                       // 0x0C000009
    ARM_HDCP_INCORRECT_SESSION_NUMBER,                                                 // 0x0C00000A
    ARM_HDCP_TEE_API_UNDEFINED_REQUEST_ID,                                             // 0x0C00000B
    ARM_HDCP_SESSION_ALREADY_OPEN,                                                     // 0x0C00000C
    ARM_HDCP_TEE_INVALID_CONNECTION_INDEX,                                             // 0x0C00000D
    ARM_HDCP_CONNECTION_ALREADY_OPEN,                                                  // 0x0C00000E
    ARM_HDCP_CERT_SIGNATURE_VERIFICATION_FAILED,                                       // 0x0C00000F
    ARM_HDCP_SRM_SIGNATURE_VERIFICATION_FAILED,                                        // 0x0C000010
    ARM_HDCP_H_PRIM_TIMEOUT,                                                           // 0x0C000011
    ARM_HDCP_PAIRING_INFO_TIMEOUT,                                                     // 0x0C000012
    ARM_HDCP_ILLEGAL_RECEIVER_AUTHSTATUS_LENGTH,                                       // 0x0C000013
    ARM_HDCP_H_VERIFICATION_FAILED,                                                    // 0x0C000014
    ARM_HDCP_LOCALITY_CHECK_FAILED,                                                    // 0x0C000015
    ARM_HDCP_SRM_FILE_INEXIST,                                                         // 0x0C000016
    ARM_HDCP_SRM_FILE_TOO_OLD,                                                         // 0x0C000017
    ARM_HDCP_SRM_INVALID_FORMAT,                                                       // 0x0C000018
    ARM_HDCP_SRM_REVOCATION_LIST_CONTAINS_ACTIVE_RECEIVER_ID,                          // 0x0C000019
    ARM_HDCP_CIPHER_NOT_ALLOWED_ON_CONNECTION,                                         // 0x0C00001A
    ARM_HDCP_CIPHER_NOT_ALLOWED_NO_SESSION_KEY,                                        // 0x0C00001B
    ARM_HDCP_SESSION_KEY_ENCRYPTION_FAILED,                                            // 0x0C00001C
    ARM_HDCP_INVALID_PES_PRIVATE_DATA_STRUCT,                                          // 0x0C00001D
    ARM_HDCP_UPSTREAM_PROPAGATION_TYPE1_HDCP_CONFLICT,                                 // 0x0C00001E
    ARM_HDCP_NO_ACTIVE_CONNECTION_ON_SESSION,                                          // 0x0C00001F
    ARM_HDCP_ILLEGAL_PROTOCOL_VERSION,                                                 // 0x0C000020
    ARM_HDCP_SHARED_MEMORY_ERROR,                                                      // 0x0C000021
    ARM_HDCP_HLOS_TEE_VERSIONS_MISMATCH,                                               // 0x0C000022
    ARM_HDCP_CERTIFICATE_NOT_RECEIVED_WITHIN_ALLOCATED_TIME,                           // 0x0C000023
    ARM_HDCP_2_2_DEVICE_NOT_SENDING_CERTIFICATE,                                       // 0x0C000024
    ARM_HDCP_DEVICE_ROOTED,                                                            // 0x0C000025
    ARM_HDCP_CIPHER_INIT_FAILED,                                                       // 0x0C000026
    ARM_HDCP_COMPUTE_KH_FAILED,                                                        // 0x0C000027
    ARM_HDCP_COMPUTE_KP_FAILED,                                                        // 0x0C000028
    ARM_HDCP_COMPUTE_KD_FAILED,                                                        // 0x0C000029
    ARM_HDCP_COMPUTE_H_FAILED,                                                         // 0x0C00002A
    ARM_HDCP_DECRYPT_KM_WITH_KPRIV_FAILED,                                             // 0x0C00002B
    ARM_HDCP_DECRYPT_KM_WITH_KH_FAILED,                                                // 0x0C00002C
    ARM_HDCP_VERIFY_DATA_SIGNATURE_FAILED,                                             // 0x0C00002D
    ARM_HDCP_GET_ENCRYPTED_NEW_KM_WITH_KPUB_FAILED,                                    // 0x0C00002E
    ARM_HDCP_GET_ENCRYPTED_KM_WITH_KH_FAILED,                                          // 0x0C00002F
    ARM_HDCP_COMPUTE_L_FAILED,                                                         // 0x0C000030
    ARM_HDCP_CIPHER_FAILED,                                                            // 0x0C000031
    ARM_HDCP_DECRYPT_FAILED_ON_ILLEGAL_INPUTCTR,                                       // 0x0C000032
    ARM_HDCP_GET_STORED_KM_DATA_FAILED,                                                // 0x0C000033
    ARM_HDCP_STORE_EKH_FAILED,                                                         // 0x0C000034
    ARM_HDCP_GET_H_PRIM_STORED_KM_FAILED,                                              // 0x0C000035
    ARM_HDCP_GET_H_PRIM_NO_STORED_KM_FAILED,                                           // 0x0C000036
    ARM_HDCP_COMPUTE_V_FAILED,                                                         // 0x0C000037
    ARM_HDCP_VERIFY_V_FAILED,                                                          // 0x0C000038
    ARM_HDCP_COMPUTE_M_FAILED,                                                         // 0x0C000039
    ARM_HDCP_DOWNSTREAM_PROPAGATION_M_VERIFICATION_FAILED,                             // 0x0C00003A
    ARM_HDCP_CERTIFICATE_RETRIEVAL_FAILED,                                             // 0x0C00003B
    ARM_HDCP_UPSTREAM_PROPAGATION_TOPOLOGY_MAX_DEVS_EXCEEDED,                          // 0x0C00003C
    ARM_HDCP_UPSTREAM_PROPAGATION_TOPOLOGY_CASCADE_EXCEEDED,                           // 0x0C00003D
    ARM_HDCP_UPSTREAM_PROPAGATION_SEQNUMV_ROLLOVER,                                    // 0x0C00003E
    ARM_HDCP_REAUTH_REQUEST_RECEIVED,                                                  // 0x0C00003F
    ARM_HDCP_DOWNSTREAM_PROPAGATION_SEQNUMM_ROLLOVER,                                  // 0x0C000040
    ARM_HDCP_UPSTREAM_PROPAGATION_REPEATERAUTH_SEND_ACK_TOO_LATE,                      // 0x0C000041
    ARM_HDCP_PROVISIONING_VALIDATION_FAILED,                                           // 0x0C000042
    ARM_HDCP_PROVISIONING_CEK_STORE_FAILED,                                            // 0x0C000043
    ARM_HDCP_PROVISIONING_CEK_REMOVAL_FAILED,                                          // 0x0C000044
    ARM_HDCP_PROVISIONING_DATA_STORE_FAILED,                                           // 0x0C000045
    ARM_HDCP_PROVISIONING_DATA_REMOVAL_FAILED,                                         // 0x0C000046
    ARM_HDCP_PROVISIONING_LC128_MISSING_FAILED,                                        // 0x0C000047
    ARM_HDCP_PROVISIONING_CERTIFICATE_MISSING_FAILED,                                  // 0x0C000048
    ARM_HDCP_PROVISIONING_PRIVATE_KEY_MISSING_FAILED,                                  // 0x0C000049
    ARM_HDCP_CONTENT_CATEGORY_SUPPORT_NOT_ENABLED,                                     // 0x0C00004A
    ARM_HDCP_TEE_REQ_FAILED,                                                           // 0x0C00004B
    ARM_HDCP_CIPHER_UNSECURED_DATA_PATH,                                               // 0x0C00004C
    ARM_HDCP_AUDIO_PACKET_DECRYPT_TO_SECURE_BUF_IS_NOT_ALLOWED,                        // 0x0C00004D
    ARM_HDCP_ILLEGAL_TOPOLOGY,                                                         // 0x0C00004E
    ARM_HDCP_WIRED_CONNECTION_FAILURE,                                                 // 0x0C00004F
    ARM_HDCP_WIRED_CONNECTION_INCONSISTENCY,                                           // 0x0C000050
    ARM_HDCP_STREAM_READ_CONTENT_TYPE_ERROR,                                           // 0x0C000051
    ARM_HDCP_TEE_SET_GLOBAL_CONFIG_NOT_ALLOWED,                                        // 0x0C000052
    ARM_HDCP_KDF_CTR_OVERFLOW,                                                         // 0x0C000053
    ARM_HDCP_INIT_RTX_FAILED,                                                          // 0x0C000054
    ARM_HDCP_INIT_RIV_FAILED,                                                          // 0x0C000055
    ARM_HDCP_TEE_UNAUTHORIZED_DEVICE_TYPE,                                             // 0x0C000056
    ARM_HDCP_INVALID_STORED_PAIRING_INFO,                                              // 0x0C000057
    ARM_HDCP_STREAM_TYPE1_CONFLICT,                                                    // 0x0C000058
    ARM_HDCP_SOCKET_ERROR,                                                             // 0x0C000059
    ARM_HDCP_PRODUCTION_KEYS_INVALID_HDCP_PROTOCOL_VERSION,                            // 0x0C00005A
    ARM_HDCP_HDCP_PROTOCOL_VERSION_MISMATCH,                                           // 0x0C00005B
    ARM_HDCP_CONTENT_PROTECTION_POLICY_ERROR,                                          // 0x0C00005C
    ARM_HDCP_CIPHER_NOT_ALLOWED_DUE_TO_DRM_POLICY,                                     // 0x0C00005D
    ARM_HDCP_ENGINE_WAS_NOT_INITIALIZED,                                               // 0x0C00005E
    ARM_HDCP_ENGINE_ALREADY_INITIALIZED,                                               // 0x0C00005F
    ARM_HDCP_ENGINE_INITIALIZATION_FAILED,                                             // 0x0C000060
    ARM_HDCP_SESSION_CREATE_FAIL,                                                      // 0x0C000061
    ARM_HDCP_CONNECTION_CREATE_FAIL,                                                   // 0x0C000062
    ARM_HDCP_STREAM_CREATE_FAIL,                                                       // 0x0C000063
    ARM_HDCP_TZINFRA_SESSION_ALREADY_INITIALIZED,                                      // 0x0C000064
    ARM_HDCP_SESSION_REMOVE_CONNECTION_FAILED,                                         // 0x0C000065
    ARM_HDCP_STREAM_LIST_SIZE_TOO_SMALL,                                               // 0x0C000066
    ARM_HDCP_INTERNAL_ERROR,                                                           // 0x0C000067
    ARM_HDCP_UNAUTHORIZED_OPEN_CONNECTION_REQUEST,                                     // 0x0C000068
    ARM_HDCP_MAX_ALLOWED_OPEN_CONNECTIONS_LIMIT,                                       // 0x0C000069
    ARM_HDCP_UNAUTHORIZED_OPEN_SESSION_REQUEST,                                        // 0x0C00006A
    ARM_HDCP_MAX_ALLOWED_OPEN_SESSIONS_LIMIT,                                          // 0x0C00006B
    ARM_HDCP_DEVICE_DESTROY_FAIL,                                                      // 0x0C00006C
    ARM_HDCP_SESSION_DESTROY_FAIL,                                                     // 0x0C00006D
    ARM_HDCP_ILLEGAL_DEVICE_HANDLE_PARAM,                                              // 0x0C00006E
    ARM_HDCP_ILLEGAL_OPERATION_ON_TRANSPORT_HANDLE,                                    // 0x0C00006F
    ARM_HDCP_MAX_ALLOWED_OPEN_STREAMS_LIMIT,                                           // 0x0C000070
    ARM_HDCP_INCORRECT_STREAM_NUMBER,                                                  // 0x0C000071
    ARM_HDCP_TZINFRA_CONTEXT_ALREADY_INITIALIZED,                                      // 0x0C000072
    ARM_HDCP_DEVICE_CREATE_FAIL,                                                       // 0x0C000073
    ARM_HDCP_OBJECT_VALIDATION_FAILED,                                                 // 0x0C000074
    ARM_HDCP_VERSION_BUFFER_TOO_SMALL,                                                 // 0x0C000075
    ARM_HDCP_MEM_POOL_INSUFFICIENT_PREALLOCATED_MEMORY,                                // 0x0C000076
    ARM_HDCP_MEM_POOL_LEAK,                                                            // 0x0C000077
    ARM_HDCP_MEM_POOL_CORRUPTION,                                                      // 0x0C000078
    ARM_HDCP_MSG_QUE_DELAYED_ENQUE_MALFUNCTION,                                        // 0x0C000079
    ARM_HDCP_MSG_QUE_BAD_QUEUE,                                                        // 0x0C00007A
    ARM_HDCP_MSG_QUE_BAD_MSG_ITEM,                                                     // 0x0C00007B
    ARM_HDCP_SEND_CALLBACK_FAILED,                                                     // 0x0C00007C
    ARM_HDCP_TZINFRA_CP_ERROR,                                                         // 0x0C00007D
    ARM_HDCP_TZSERVICE_CIPHER_CONTENT_QUERY_FAILED,                                    // 0x0C00007E
    ARM_HDCP_API_ITF_LOCK_FAILED,                                                      // 0x0C00007F
    ARM_HDCP_MSG_QUEUE_FAILED_ON_MSG_ALLOC,                                            // 0x0C000080
    ARM_HDCP_MSG_QUEUE_FAILED_ON_MSG_ENQUE,                                            // 0x0C000081
    ARM_HDCP_MSG_QUEUE_FAILED_ON_MSG_FREE,                                             // 0x0C000082
    ARM_HDCP_MSG_QUEUE_FAILED_ON_MSG_DEQUE,                                            // 0x0C000083
    ARM_HDCP_CIPHER_FAILURE_CONVERTER_DOWNSTREAM_AUTHETICATING_TOO_LONG,               // 0x0C000084
    ARM_HDCP_PROVISIONING_FAILED,                                                      // 0x0C000085
    ARM_HDCP_DOWNSTREAM_PROPAGATION_FAILED,                                            // 0x0C000086
    ARM_HDCP_ILLEGAL_CONFIG_PARAM,                                                     // 0x0C000087
    ARM_HDCP_UNEXPECTED_STATE_ERROR,                                                   // 0x0C000088
    ARM_HDCP_UNEXPECTED_CERTIFICATE_PROTOCOL_DESCRIPTOR,                               // 0x0C000089
    ARM_HDCP_UNEXPECTED_STATUS_ERROR,                                                  // 0x0C00008A
    ARM_HDCP_AKE_RECEIVER_INFO_TIMEOUT,                                                // 0x0C00008B
    ARM_HDCP_FILE_DELETE_FAILURE,                                                      // 0x0C00008C
    ARM_HDCP_OBJECT_NOT_REGISTERED,                                                    // 0x0C00008D
    ARM_HDCP_TEE_REQ_LOCALITY_CHECK_ATTEMPTS_RETRY_EXCEEDED,                           // 0x0C00008E
    ARM_HDCP_LOCALITY_CHECK_ATTEMPTS_RETRY_EXCEEDED,                                   // 0x0C00008F
    ARM_HDCP_UPSTREAM_PROPAGATION_REPEATER_AUTH_SEND_RECEIVER_ID_LIST_TIMEOUT,         // 0x0C000090
    ARM_HDCP_UPSTREAM_PROPAGATION_REPEATER_AUTH_SEND_ACK_TIMEOUT,                      // 0x0C000091
    ARM_HDCP_DOWNSTREAM_PROPAGATION_REPEATER_AUTH_STREAM_READY_TIMEOUT,                // 0x0C000092
    ARM_HDCP_INTERNAL_ERROR_MESSAGE_SENDING_BLOCKED,                                   // 0x0C000093
    ARM_HDCP_DOWNSTREAM_PROPAGATION_REPEATERAUTH_STREAM_READY_TOO_LATE,                // 0x0C000094
    ARM_HDCP_TEE_OPEN_CONTEXT_FAILED,                                                  // 0x0C000095
    ARM_HDCP_TEE_LOAD_FAILED,                                                          // 0x0C000096
    ARM_HDCP_TEE_OPEN_SESSION_FAILED,                                                  // 0x0C000097
    ARM_HDCP_DECODE_UUID_FAILED,                                                       // 0x0C000098
    ARM_HDCP_DOWNSTREAM_PROP_STREAMSNUM_INCONSISTENT,                                  // 0x0C000099
    ARM_HDCP_TZSERVICE_CIPHER_CONTEXT_ERROR,                                           // 0x0C00009A
    ARM_HDCP_TEE_SET_INTERNAL_SRM_VERSION_FAILED,                                      // 0x0C00009B
    ARM_HDCP_NOT_ENOUGHT_SPACE_IN_BUFFER,                                              // 0x0C00009C
    ARM_HDCP_TZINFRA_SMR_ERROR,                                                        // 0x0C00009D
    ARM_HDCP_TZSERVICE_CIPHER_ERROR,                                                   // 0x0C00009E
    ARM_HDCP_TZSERVICE_STRING_ERROR,                                                   // 0x0C00009F
    ARM_HDCP_TZSERVICE_SECURE_STORAGE_ERROR,                                           // 0x0C0000A0
    ARM_HDCP_TZSERVICE_SECURE_TIMER_ERROR,                                             // 0x0C0000A1
    ARM_HDCP_ILLEGAL_UPSTREAM_PROPAGATION_MESSAGE,                                     // 0x0C0000A2
    ARM_HDCP_2_0_DEVICE_DOES_NOT_SUPPORT_HOT_PLUG,                                     // 0x0C0000A3
    ARM_HDCP_MAX_ALLOWED_DEVICES_LIMIT,                                                // 0x0C0000A4
    ARM_HDCP_REAUTH_FAILED,                                                            // 0x0C0000A5
    ARM_HDCP_DOWNSTREAM_PROPAGATION_ALLOWED_DURING_AUTH_ONLY,                          // 0x0C0000A6
    ARM_HDCP_TZSERVICE_SECURE_BUFFER_QUERY_ERROR,                                      // 0x0C0000A7
    ARM_HDCP_INIT_SESSION_KEY_FAILED,                                                  // 0x0C0000A8
    ARM_HDCP_COMPUTE_RIV_FAILED,                                                       // 0x0C0000A9
    ARM_HDCP_COMPUTE_HMAC_RIV_FAILED,                                                  // 0x0C0000AA

    ARM_HDCP_ERROR_LAST,
    ARM_HDCP_ERROR_MAX = 0x7FFFFFFF
} EArmHdcpErrors;



#define  ARM_IS_VALID_HDCP_EXTERNAL_ERROR_CODE(errCode) \
    ( (errCode == DX_SUCCESS) || \
      ( (errCode >= ARM_HDCP_ERROR_FIRST) && (errCode < ARM_HDCP_ERROR_LAST) ) \
    )

#endif //_ARM_HDCP_ERRORS_H

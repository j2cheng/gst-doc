/***************************************************************************
*   Copyright (C)2011 - 2017 ARM Ltd (or its subsidiaries)
*   All Rights Reserved
****************************************************************************/

#ifndef _ARM_HDCP_TYPES_H
#define _ARM_HDCP_TYPES_H


/*! \file ARM_Hdcp_Types.h
This module includes all HDCP API definitions
*/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#if defined (__cplusplus) && !defined (HDCP_TESTS_CPP_COMPILATION)
extern "C" {
#endif
 
#define ARM_HDCP_RECEIVER_ID_LENGTH              5
#define ARM_HDCP_SYSTEM_MAX_NUM_DEVICES          32

#ifndef IN 
    #define IN
#endif
#ifndef OUT 
    #define OUT
#endif
#ifndef INOUT 
    #define INOUT
#endif

#if (defined _WIN32)
#include <windows.h>
#define CALLBACK_API WINAPIV
#else
#define CALLBACK_API
#endif

#ifndef IGNORE_VISIBILITY

#if (defined _WIN32) || (defined __CYGWIN__)
#ifdef BUILDING_DLL
#ifdef __GNUC__
#define ARM_EXTERNAL_API __attribute__ ((dllexport))
#else
#define ARM_EXTERNAL_API __declspec(dllexport)
#endif
#else
#ifdef __GNUC__
#define ARM_EXTERNAL_API __attribute__ ((dllimport))
#else
#define ARM_EXTERNAL_API __declspec(dllimport)
#endif
#endif
#else
#if __GNUC__ >= 4
#define ARM_EXTERNAL_API __attribute__ ((visibility ("default")))
#else
#define ARM_EXTERNAL_API
#endif
#endif
#else //IGNORE_VISIBILITY
#define ARM_EXTERNAL_API 
#endif // IGNORE_VISIBILITY

typedef enum tagEArmHdcpVersion {
    ARM_HDCP_VERSION_UNKNOWN,
    ARM_HDCP_VERSION_1_X,
    ARM_HDCP_VERSION_2_0,
    ARM_HDCP_VERSION_2_1,
    ARM_HDCP_VERSION_2_2,
    ARM_HDCP_VERSION_2_3,
    ARM_HDCP_VERSION_MAX = 0x7FFFFFFF       //impose 4 bytes enumerator
} EArmHdcpVersion;

typedef enum tagEArmHdcpElementType {
    ARM_HDCP_TYPE_UNKNOWN,
    ARM_HDCP_TYPE_RECEIVER,
    ARM_HDCP_TYPE_REPEATER,
    ARM_HDCP_TYPE_MAX = 0x7FFFFFFF         //impose 4 bytes enumerator
} EArmHdcpElementType;

#define ARM_HDCP_IS_VALID_EVENT_TYPE(evType) (evType >= ARM_HDCP_EVENT_FIRST && evType < ARM_HDCP_EVENT_LAST)

typedef enum tagEArmHdcpStreamMediaType{
    ARM_HDCP_STREAM_MEDIA_TYPE_UNKNOWN,
    ARM_HDCP_STREAM_MEDIA_TYPE_VIDEO,
    ARM_HDCP_STREAM_MEDIA_TYPE_AUDIO,

    // do not use values greater than 0x000000FF. 
    // 24 msb bits are reserved for additional stream information(like video resolution)
    ARM_HDCP_STREAM_MEDIA_TYPE_LAST,               // should not be used
    ARM_HDCP_STREAM_MEDIA_TYPE_MAX = 0x7FFFFFFF    // should not be used
} EArmHdcpStreamMediaType;


typedef uintptr_t ArmHdcpDeviceHandle;
typedef uintptr_t ArmHdcpSessionHandle;
typedef uintptr_t ArmHdcpConnectionHandle;
typedef uintptr_t ArmHdcpStreamHandle;
typedef uintptr_t ArmHdcpTransportHandle;

typedef uint8_t ArmPesData_t[16];


/* 
Values that represent HDCP event types.
An event is used as a communication method between the HDCP engine and upper layer application (WFD),
asynchronously notifying about noticeable things that happened :
1) A change of a status
2) Internal process outcome (positive or negative) 
3) An error

This is done independent of the main program flow. 
API calls outcome is returned via a return code. But any other notification which is not
perfectly synchronized with the main program flow, is reported via events mechanism, 
by invoking the callback function given by the user while creating the HDCP device.
An HDCP client application (WFD) should implement a callback function which is able to react
instantly to any event reported.

Some general remarks and rules about how events work :
1) Status event always update about the new status. If no status is known yet, an event will be sent as early as possible to announce the current status. For example :
    status event ARM_HDCP_STATUS_EVENT_CONNECTION_UNAUTHENTICATED will be usually sent immediately on the beginning to HDCP interface authentication, to announce that currently connection is unauthenticated. 
    When the authentication (peer to peer) is done, a new status event ARM_HDCP_STATUS_EVENT_CONNECTION_AUTHENTICATED is sent. User should never see the same status sent twice in sequential order.
2) Process event will always have a beginning (START event)( , and a result (SUCCESS event or FAILURE event). In the usual scenario, the status will always be DX_SUCESS on start and success events, and with a failure reason on process failure.
    The only exception to that is the case where re-authentication starts (authentication restarts) for some reasons. In that case , we will usually have the reason of the re-authentication in the status field.
*/
typedef enum tagEArmHdcpEventType {   
        
    //////////////////////////////////////////////////////////////////////////
    // Connectivity events
    //////////////////////////////////////////////////////////////////////////
    /* Transport notification event - HDCP sink (server) only - this event is sent after calling 
    ARM_HDCP_Repeater_Upstream_Listen. This event indicates that HDCP sink is ready to accept connections */
    ARM_HDCP_CONNECTIVITY_EVENT_UPSTREAM_LISTEN_READY = 0,
    ARM_HDCP_EVENT_FIRST = ARM_HDCP_CONNECTIVITY_EVENT_UPSTREAM_LISTEN_READY,

    //////////////////////////////////////////////////////////////////////////
    // HDCP process notification events
    //////////////////////////////////////////////////////////////////////////
    
    /* Interface peer to peer authentication started (or restarted) */
    ARM_HDCP_PROCESS_EVENT_AUTHETICATION_START = 1,      

    /* Interface peer to peer authentication finished with success */
    ARM_HDCP_PROCESS_EVENT_AUTHETICATION_SUCCESS = 2,

    /* Interface peer to peer authentication finished with some failure */
    ARM_HDCP_PROCESS_EVENT_AUTHETICATION_FAILURE = 3,

    /* downstream propagation started */
    ARM_HDCP_PROCESS_EVENT_DOWNSTREAM_PROPAGATION_START = 4,

    /* downstream propagation finished with success */
    ARM_HDCP_PROCESS_EVENT_DOWNSTREAM_PROPAGATION_SUCCESS = 5,

    /* downstream propagation finished with some failure */
    ARM_HDCP_PROCESS_EVENT_DOWNSTREAM_PROPAGATION_FAILURE = 6,

    /* downstream propagation started */
    ARM_HDCP_PROCESS_EVENT_UPSTREAM_PROPAGATION_START = 7,

    /* upstream propagation finished with success */
    ARM_HDCP_PROCESS_EVENT_UPSTREAM_PROPAGATION_SUCCESS = 8,

    /* upstream propagation finished with some failure */
    ARM_HDCP_PROCESS_EVENT_UPSTREAM_PROPAGATION_FAILURE = 9,
  
    //////////////////////////////////////////////////////////////////////////
    // Interface/Session status notification events
    //////////////////////////////////////////////////////////////////////////

    /* interface is authenticated. Sent by source and sink after SKE is finished.
    Sent also in case that sink is converter. */
    ARM_HDCP_STATUS_EVENT_CONNECTION_AUTHENTICATED = 10,

    /* interface is unauthenticated. Sent by source and sink only if ARM_HDCP_STATUS_EVENT_CONNECTION_AUTHENTICATED has already
    been sent and interface is */
    ARM_HDCP_STATUS_EVENT_CONNECTION_UNAUTHENTICATED = 11,

    /* Cipher is enabled on specific session. Reasons :
    1) All connections paired with given session are authenticated
    2) For Converter use-case: both: upstream and downstream propagations on given session are succeeded. */
    ARM_HDCP_STATUS_EVENT_SESSION_CIPHER_ENABLED = 12,

    /* Cipher is disabled on specific session. Reasons :
    1) Any connection paired with given session is not yet authenticated (or during re-authentication process)
    2) Any connection paired with given session is in the process of upstream/downstream propagation */
    ARM_HDCP_STATUS_EVENT_SESSION_CIPHER_DISABLED = 13,

    //////////////////////////////////////////////////////////////////////////
    // HDCP error notification events
    //////////////////////////////////////////////////////////////////////////

    /* This event is used in order to report asynchronous error which happened in the system */
    ARM_HDCP_ERROR_EVENT_INTERNAL = 14,

    //keep these 2 events last!
    ARM_HDCP_EVENT_LAST,
    ARM_HDCP_EVENT_MAX = 0x7FFFFFFF

} EArmHdcpEventType;

/** Basic Information for an event
*
* eventType and eventStatus are valid for all event types.
*
* The following events raises on HDCP Sink only:
* deviceHandle is valid for the following events:
* ARM_HDCP_CONNECTIVITY_EVENT_UPSTREAM_LISTEN_READY
*
*
* The following events raises on both HDCP Sink and Source:
*
* For HDCP Source: deviceHandle, sessionHandle and connectionHandle are valid for these events.
* For HDCP Sink: deviceHandle is valid for these events.
*
* ARM_HDCP_PROCESS_EVENT_AUTHETICATION_START
* ARM_HDCP_PROCESS_EVENT_AUTHETICATION_SUCCESS
* ARM_HDCP_PROCESS_EVENT_AUTHETICATION_FAILURE
* ARM_HDCP_PROCESS_EVENT_DOWNSTREAM_PROPAGATION_START
* ARM_HDCP_PROCESS_EVENT_DOWNSTREAM_PROPAGATION_SUCCESS
* ARM_HDCP_PROCESS_EVENT_DOWNSTREAM_PROPAGATION_FAILURE
* ARM_HDCP_PROCESS_EVENT_UPSTREAM_PROPAGATION_START
* ARM_HDCP_PROCESS_EVENT_UPSTREAM_PROPAGATION_SUCCESS
* ARM_HDCP_PROCESS_EVENT_UPSTREAM_PROPAGATION_FAILURE
* ARM_HDCP_STATUS_EVENT_CONNECTION_AUTHENTICATED
* ARM_HDCP_STATUS_EVENT_CONNECTION_UNAUTHENTICATED
*
* For HDCP Source: deviceHandle and sessionHandleis are valid for these events.
* For HDCP Sink: deviceHandle is valid for these events.
* ARM_HDCP_STATUS_EVENT_SESSION_CIPHER_ENABLED
* ARM_HDCP_STATUS_EVENT_SESSION_CIPHER_DISABLED
*
* In case of error, a special event ARM_HDCP_ERROR_EVENT_INTERNAL rises.
* This event may include any combination of valid handles (depends on error type).
* For general errors that not related to any handle all handles can be equal to '0'.
* In this case, pvClientData value of event callback function (ArmEventCbFunction) will be set to NULL
*/

typedef struct tagArmHdcpEvent {

    ///< Event type enumeration
    EArmHdcpEventType        eventType;
    
    ////< for a positive outcome event DX_SUCCESS, for a negative outcome event - one of the errors under ARM_Hdcp_Errors.h
    uint32_t                eventStatus;

    ///< Device Handle, created using ARM_HDCP_Transmitter_Create() or ARM_HDCP_Repeater_Create() APIs.
    ArmHdcpDeviceHandle      deviceHandle;

    ///< Session Handle, created using ARM_HDCP_Transmitter_Open_Session() API.
    ArmHdcpSessionHandle     sessionHandle;

    ///< Stream Handle, created using ARM_HDCP_Transmitter_Open_Stream() API.
    ArmHdcpStreamHandle      streamHandle;

    ///< Connection Handle, created using ARM_HDCP_Transmitter_Authentication_Start() API.
    ArmHdcpConnectionHandle  connectionHandle;

    ///< This parameter is reserved for future use
    uint32_t param1;

    ///< This parameter is reserved for future use
    void* param2;
}ArmHdcpEvent;


/**
* Callback function that will be called for special events
*
* \param [in]      ev - Event information.
* \param [in]      pvClientData - Client data provided by the caller in ARM_HDCP_Transmitter_Create()
*                  or ARM_HDCP_Repeater_Create() APIs. pvClientData is passed back to the caller
*                  using this callback function when an even occurs.
*
* \return  void.
*/

typedef void(*ArmEventCbFunction)(ArmHdcpEvent* ev, void* pvClientData);

typedef enum {
    ARM_HDCP_BUFFER_TYPE_VIRTUAL_POINTER = 1,
    ARM_HDCP_BUFFER_TYPE_SHARED_MEMORY_HANDLE = 2,
    ARM_HDCP_BUFFER_TYPE_CRYPTO_VIRTUAL_POINTER = 3,
    ARM_HDCP_BUFFER_TYPE_MAX = 0x7FFFFFFF
} EArmHdcpBufferType;


typedef struct tagArmHdcpPlainTextInfo
{
    EArmHdcpBufferType   type;               // virtual memory address or shared memory handle
    uint64_t            handle;             // handle of type <type> -  if type = ARM_HDCP_BUFFER_TYPE_VIRTUAL_POINTER it  represents a user-space pointer, 
                                            // if type = ARM_HDCP_BUFFER_TYPE_SHARED_MEMORY_HANDLE it represents a shared (secured, if supported) memory handle
    uint32_t            offset;             // offset in bytes from the beginning of the memory buffer referenced by the handle
}ArmHdcpPlainTextInfo;

typedef struct tagArmHdcpCipherTxtInfo
{
    EArmHdcpBufferType   type;               // virtual memory address or shared memory handle
    uint64_t            handle;             // handle of type <type> -  if type = ARM_HDCP_BUFFER_TYPE_VIRTUAL_POINTER it  represents a user-space pointer, 
                                            // if type = ARM_HDCP_BUFFER_TYPE_SHARED_MEMORY_HANDLE it represents a shared (secured, if supported) memory handle
    ArmPesData_t         PES_PrivateData;    // PES private data
    bool                bBypassCipher;      // if True - PES_PrivateData is ignored, data is not decrypted but copied as as to output buffer
                                            // This parameter is only relevant when used in ARM_HDCP_Repeater_Decrypt(), it should be set to False otherwise
}ArmHdcpCipherTxtInfo;

typedef struct tagArmHdcpReceiverId_t {
    uint8_t bytes[ARM_HDCP_RECEIVER_ID_LENGTH];
} ArmHdcpReceiverId_t;

typedef struct tagArmHdcpTopologyElementData_t {
    ArmHdcpReceiverId_t  rcvId;
    EArmHdcpElementType  type;
    EArmHdcpVersion      version;
} ArmHdcpTopologyElementData_t;

typedef struct tagArmHdcpTopologyData_t {
    uint8_t                     devicesNum;                              /* number of unique devices (receiver Ids) in devices array  */
    uint8_t                     depth;                                   /* DEPTH protocol value */
    ArmHdcpTopologyElementData_t devices[ARM_HDCP_SYSTEM_MAX_NUM_DEVICES];
} ArmHdcpTopologyData_t;

typedef struct tagArmHdcpStreamInfo_t {
    ArmHdcpStreamHandle  handle;
    uint16_t            contentStreamId;
    uint8_t             contentType;
} ArmHdcpStreamInfo_t;


//  --- Transport related definitions ---

typedef enum {
    ARM_HDCP_TRANSPORT_DIRECTION_FIRST      = 0,
    ARM_HDCP_TRANSPORT_DIRECTION_DOWNSTREAM = 1,
    ARM_HDCP_TRANSPORT_DIRECTION_UPSTREAM   = 2,

    // should be before last 
    ARM_HDCP_TRANSPORT_DIRECTION_LAST       = 3,
    // should be last
    ARM_HDCP_TRANSPORT_DIRECTION_MAX = 0x7FFFFFFF  //impose 4 bytes enumerator

} EArmTransportDirection;


// This callback function shall be thread-safe for multi-connections.
// Meaning, a single HDCP thread will call this callback function for a given connection.
// However, if multiple transports are created for multiple connections, multiple threads
// will call this callback function simultaneously, one thread per connection.
// A return value 0 means success.
typedef uint32_t(CALLBACK_API *ARM_HDCP_SendDataCallback)(
    const void*  sendDataCallbackArg,
    const void*  data,
    size_t       dataSizeInBytes);


#if defined (__cplusplus) && ! defined (HDCP_TESTS_CPP_COMPILATION)
}
#endif

#endif // #ifndef _ARM_HDCP_TYPES_H

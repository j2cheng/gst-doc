/***************************************************************************
*   Copyright (C)2011 - 2017 ARM Ltd (or its subsidiaries)
*   All Rights Reserved
****************************************************************************/

#ifndef _ARM_HDCP_REPEATER_H
#define _ARM_HDCP_REPEATER_H

/*! \file ARM_Hdcp_Repeater.h
This module provides HDCP repeater services.
*/

#include "ARM_Hdcp_Types.h"

#if defined (__cplusplus) && ! defined (HDCP_TESTS_CPP_COMPILATION)
extern "C"
{
#endif

/*General API Calls
******************************************/

/*! Initialize a repeater device instance
\parameters:    
    notifyCallbackFunction      - Callback function reference for events notification. Note: Callback function must return immediately!
    pvClientData                - Pointer to Client data that will be returned to caller using the callback function. pvClientData can be NULL;
    pRptDeviceHandle            - (value returned by function) - a pointer to a device handle, to be filled with the handle value of to the new repeater instance
\return listen status
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Repeater_Create( IN  ArmEventCbFunction notifyCallbackFunc,
                                                    IN  void *pvClientData,
                                                    OUT ArmHdcpDeviceHandle *pRptDeviceHandle);


/*! Closes repeater.
\parameters:
repeaterDeviceHandle -	A pointer to a handleof a repeater device instance which was created by a successful call to ARM_HDCP_Repeater_Create()
                            This pointer will be nulled  after freeing the memory it was referring to
\return close status
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Repeater_Destroy(INOUT ArmHdcpDeviceHandle *pRptDeviceHandle);

/*! decrypts data to be transferred.
\parameters:
repeaterDeviceHandle -  A handle to a repeater device instance, which was created by a successful call to ARM_HDCP_Repeater_Create().
streamMediaType -	    An opaque value that constructed from 2 parts:
                        -- 8 low bits is stream type (video/audio)
                        -- higher 24 bits are reserved for additional stream information(like video resolution)
dataSize           -    size of data to be decrypted (this is relevant to both input and output buffers)
pInputBufferInfo    -	input buffer info (cipher text) - Includes the PES private data and the cipher text.
If PES private data is NULL, input encrypted data will be copied to the output decrypted data as-is.
pOutputBufferInfo   -	output buffer info (plain text) - output buffer result will be assigned into a a secured memory buffer represented by the given handle.
\return decryption status
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Repeater_Decrypt( IN ArmHdcpDeviceHandle       repeaterDeviceHandle,
                                                IN EArmHdcpStreamMediaType   streamMediaType,
                                                IN uint32_t                 dataSize,
                                                IN ArmHdcpCipherTxtInfo      *pInputBufferInfo,
                                                IN ArmHdcpPlainTextInfo      *pOutputBufferInfo);

/*Upstream API Calls
******************************************/


/*! Opens repeater's connection (listens to transmitter's authentication request). 
\parameters:
    transportHandle - a handle to an already created transport handle. This transport will be paired to the upstream connection.
    The transport will be destroyed
    when calling ARM_HDCP_Repeater_Destroy, together with the paired upstream connection. A call to ARM_HDCP_Transport_Destroy 
    is possible only after connection is fully authenticated (1st cipher enable event). 
    repeaterDeviceHandle - A handle to a transmitter device instance, which was created by a successful call to ARM_HDCP_Repeater_Create(). 
    The opened session will reside in this device.
\return listen status
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Repeater_Upstream_Listen(IN ArmHdcpTransportHandle transportHandle, IN ArmHdcpDeviceHandle repeaterDeviceHandle);

/*! Returns the repeater's upstream authentication status. 
\parameters:
    repeaterDeviceHandle - A handle to a transmitter device instance, which was created by a successful call to ARM_HDCP_Repeater_Create(). The opened session will reside in this device.
    isAuthenticated -	            (value returned by function) set to true if upstream is authenticated
    pIsAuthenticatedAsRepeater -	(value returned by function) set to true if last authentication sent a REPEATER flag in upstream
\return GetStatus status
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Repeater_Upstream_Get_Status(IN ArmHdcpDeviceHandle repeaterDeviceHandle, OUT bool *pIsAuthenticated, OUT bool *pIsAuthenticatedAsRepeater);

/*! Returns the list of connected streams sent by the transmitter.
\parameters:
repeaterDeviceHandle - A handle to a transmitter device instance, which was created by a successful call to ARM_HDCP_Repeater_Create(). The opened session will reside in this device.
pStreamList -	     (value returned by function) list of streams. per-stream data includes the stream's ID & type
maxStreamListLen - max size of array passed in streams argument
pActualStreamListLen - (value returned by function) number of streams actually recorded
\return GetStatus status
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Repeater_Upstream_Get_Streams(IN ArmHdcpDeviceHandle   repeaterDeviceHandle,
                                                            OUT ArmHdcpStreamInfo_t  *pStreamList,
                                                            IN uint32_t             maxStreamListLen,
                                                            OUT uint32_t            *pActualStreamListLen);





#if defined (__cplusplus) && ! defined (HDCP_TESTS_CPP_COMPILATION)
}
#endif

#endif

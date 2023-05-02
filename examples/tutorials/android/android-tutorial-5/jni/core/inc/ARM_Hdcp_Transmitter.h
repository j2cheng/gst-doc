/***************************************************************************
*   Copyright (C)2011 - 2017 ARM Ltd (or its subsidiaries)
*   All Rights Reserved
****************************************************************************/

#ifndef _ARM_HDCP_TRANSMITTER_H
#define _ARM_HDCP_TRANSMITTER_H

/*! \file ARM_Hdcp_Transmitter.h
This module provides HDCP transmitter services.
*/

#include "ARM_Hdcp_Types.h"

#if defined (__cplusplus) && ! defined (HDCP_TESTS_CPP_COMPILATION)
extern "C"
{
#endif

/*! Initialize a transmitter device instance
\parameters:
    notifyCallbackFunc    - Callback function reference for events notification. Note: Callback function must return immediately!
    pvClientData          - pointer to Client data that will be returned to caller using the callback function. pvClientData can be NULL;
    pTsmtDeviceHandle     - (value returned by function) - a pointer to a device handle, to be filled with the handle value of to the new transmitter instance
\return initialization status as return value and fills
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Transmitter_Create(IN ArmEventCbFunction notifyCallbackFunc, IN void *pvClientData, OUT ArmHdcpDeviceHandle *pTsmtDeviceHandle);

/*! Opens transmitter's new session. 
\parameters:
    transmitterDeviceHandle - A handle to a transmitter device instance, which was created by a successful call to ARM_HDCP_Transmitter_Create(). The opened session will reside in this device.
    pSessionHandle -			(value returned by function) a pointer to a session handle, to be filled with the handle value of to the new Session 
\return session open status
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Transmitter_Open_Session(IN ArmHdcpDeviceHandle transmitterDeviceHandle, OUT ArmHdcpSessionHandle *pSessionHandle);


/*! Start authentication between the local device and the remote device connected via transport object
 on success, A connection is opened and a handle pConnectionHandle is filled.
 The connection is paired with transportHandle (communication object) and with a session handle (data path object)
 In order to restart authentication (re-authenticate),use ARM_HDCP_Transmitter_Authentication_Restart on te same hanle.
 Function will return on authentication protocol start (after sending the first HDCP messages).
 In order to get acknowledge for authentication success on the specific
 HDCP interface, caller must wait on callback function for ARM_HDCP_STATUS_EVENT_CONNECTION_AUTHENTICATED. 
 That doesn't mean cipher data can be started ,  in some cases ARM_HDCP_STATUS_EVENT_SESSION_CIPHER_ENABLED will come straight after, 
 and in other cases upstream/downstream propagation must be completed before cipher is enabled
 The pairing between transportHandle and the connectionHandle is done once here. upon call to ARM_HDCP_Transmitter_Authentication_Destroy
 both connection handle and the paired transport handle are invalidated and closed.
\parameters:    
    transportHandle     - A handle to an already created transport object, to preform the underlayer communication tasks on the specific downstream HDCP interface
    sessionHandle  -   A handle to a session, which was created by a successful call to ARM_HDCP_Transmitter_Open_Session()
    pConnectionHandle -	(value returned by function) a pointer to a connection handle, to be filled with the handle value of to the new connection 	
\return connection status
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Transmitter_Authentication_Start(IN ArmHdcpTransportHandle transportHandle,
                                                              IN ArmHdcpSessionHandle sessionHandle, 
                                                              OUT ArmHdcpConnectionHandle *pConnectionHandle);


/*! Destroy an authentication process, together with the paired connection and transport object. 
The session object which was paired with the connection is not destroyed (might be paired with other connection)
\parameters:
pConnectionHandle -			A pointer to a handle of a connection, which was created by a successful call to ARM_HDCP_Transmitter_Authentication_Start()

This pointer will be nulled  after freeing the memory it was referring to
\return connection status
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Transmitter_Authentication_Destroy(INOUT ArmHdcpConnectionHandle *pConnectionHandle);

/*! set an authenticated connection , to an unauthenticated state
\parameters:
pConnectionHandle -			A pointer to a handle of a connection, which was created by a successful call to ARM_HDCP_Transmitter_Authentication_Start()

In order to authenticate connection later on , call ARM_HDCP_Transmitter_Authentication_Restart
transport object remain paired with that connection
\return Status
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Transmitter_Authentication_Invalidate(IN ArmHdcpConnectionHandle connectionHandle);

/*! restart authentication on an already exist connection, the same transport object and session which were used in ARM_HDCP_Transmitter_Authentication_Start
are still used here. In order to use new session handle or a new transport object on authentication, 
fully creation process should be applied using ARM_HDCP_Transmitter_Authentication_Start. In that case transport object/ and (or) session handle should be destroyed.
\parameters:
connectionHandle -	a handle of a connection, which was created by a successful call to ARM_HDCP_Transmitter_Authentication_Start()

\return Status
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Transmitter_Authentication_Restart(IN ArmHdcpConnectionHandle connectionHandle);



/*! Opens transmitter's new stream.
\parameters:
    sessionHandle -    A handle to a session which was created by a successful call to ARM_HDCP_Transmitter_Open_Session()
    contentStreamID -	Stream content stream ID (this is a reserved HDCP specification term)
    streamMediaType -	Stream media type (audio/video etc)			
    pStreamHandle -	    (value returned by function) a pointer to a stream handle, to be filled with the handle value of to the new stream 
 action: 
    If there are authenticated(ing) connections related to the session associated with the new stream, such connections will start 
    re-authentication before stream is opened. This way transmitter sends RepeaterAuth_Stream_Manage message only once per authentication.
    This is done in order to prevent spec violation and security issue.
 note: Users are strongly recommended to open/close all streams before connections are authenticated. Calling open/close stream(s) after authentication
    cause re-authentication which takes significant time. 
\return stream open status
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Transmitter_Open_Stream(IN ArmHdcpSessionHandle sessionHandle, 
                                                      IN uint32_t contentStreamID, 
                                                      IN EArmHdcpStreamMediaType streamMediaType, 
                                                      OUT ArmHdcpStreamHandle *pStreamHandle);

/*! Closes transmitter's stream.
\parameters:
    pStreamHandle - A pointer to a handleof a stream which was created by a successful call to ARM_HDCP_Transmitter_Open_Stream()
                    This pointer will be nulled  after freeing the memory it was referring to
 action:
    If there are authenticated(ing) connections related to the session associated with the closed stream, such connections will start 
    re-authentication before stream is opened. This way transmitter sends RepeaterAuth_Stream_Manage message only once per connection
    authentication. This is done in order to prevent spec violation and security issue.
 note: Users are strongly recommended to open/close all streams before connections are authenticated. Calling open/close stream(s) after authentication
    cause re-authentication which takes significant time.
\return stream close status
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Transmitter_Close_Stream(INOUT ArmHdcpStreamHandle *pStreamHandle);


/*! Encrypts data to be transmitted. 
\parameters:
    streamHandle       -	    A handle to a stream which was created by a successful call to ARM_HDCP_Transmitter_Open_Stream().
    dataSize           -   size of data to be encrypted (this is relevant to both input and output buffers)
    pInputBufferInfo    -	input buffer info (plain text)
    pOutputBufferInfo   -	output buffer info (cipher text) - Includes the PES private data and the cipher text. 
                            output buffer result will be filled into pOutputBufferInfo->pBuff.
\return encryption status
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Transmitter_Encrypt(  IN ArmHdcpStreamHandle streamHandle, 
                                                    IN uint32_t dataSize, 
                                                    IN ArmHdcpPlainTextInfo *pInputBufferInfo, 
                                                    IN ArmHdcpCipherTxtInfo *pOutputBufferInfo);

/*! Stops transmitter's session.
\parameters:
    ArmHdcpSessionHandle -       A handle to a session which was created by a successful call to ARM_HDCP_Transmitter_Open_Session()
                                This pointer will be nulled  after freeing the memory it was referring to
\return close session status
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Transmitter_Close_Session(INOUT ArmHdcpSessionHandle *pSessionHandle);

/*! Closes transmitter
\parameters:
    pTsmtDeviceHandle -	A pointer to handle to a transmitter device instance which was created by a successful call to ARM_HDCP_Transmitter_Create()
                                This pointer will be nulled  after freeing the memory it was referring to
                                
\return close status
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Transmitter_Destroy(INOUT ArmHdcpDeviceHandle *pTsmtDeviceHandle);

/*! Get full topology of receivers & repeaters. 
    Few important remarks :
    1)  The returned list of devices, is unique. That means, no duplications of any receiver Ids.
        In case that there are multiple Software implemented HDCP devices on a single hardware devices, 
        they will be represented by a single receiver ID in the list. That is because the "root of trust" is single and the 
        certificate's used (probably) to authenticate with the upstream Transmitter deice, is the the same.
        Another case is where 2 separate HW devices (for example) which  use the same certificate - they will be represented 
        as a single device (since the same receiver ID is equal for both).
    2)  The returned value of ArmHdcpTopologyData_t::depth equal:
        (maximum depth the Transmitter has got from its downstream devices) + 1. 
        If transmitter has only directly connected devices (no converters) then depth=1.
        If transmitter has no connected devices (authenticated connections) the depth=0.
        For more information, please refer to pages 25 and 26 on the HDCP 2.2 specification.
        
                             +-----------------+
                             |                 |
                             |     CLIENT APP  |
                             |      (WFD)      |
                             |                 |
                             +-----------------+
                        Call ARM_HDCP_Transmitter_Get_Topology()
              +-------------------------------------------------------+  

                             +-----------------+
Get : DEPTH=1 from Converter |                 |
Return : DEPTH=2 to Client   |   TRANSMITTER   |
                             |                 |
                             +--------+--------+
                                      |
                                      |
                                      |
                             +--------+--------+
                             |                 |
                             |                 |
                             |   CONVERTER     |
                             |                 |
                             +-------+---------+
                                     |
                      +----------------------------+  <---+ WIRED HDCP INTERFACE
                      |              |             |
                 +----+---+    +-----+----+   +----+----+
                 |        |    |          |   |         |
                 |   RX1  |    |   RX2    |   |   RX3   |
                 |        |    |          |   |         |
                 +--------+    +----------+   +---------+

\parameters:
    transmitterDeviceHandle - A handle to a transmitter device instance which was created by a successful 
    call to ARM_HDCP_Transmitter_Create()
    topologyData -	(value returned by function) Aggregated Receivers / Repeaters topology data . The list does not include the local transmitter device ID.
    [Note: is the maximum depth the Transmitter has got from its downstream devices. If no downstream devices, depth=0].
\return Status
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Transmitter_Get_Topology(IN ArmHdcpDeviceHandle transmitterDeviceHandle, 
                                                        OUT ArmHdcpTopologyData_t *pTopologyData);


#if defined (__cplusplus) && ! defined (HDCP_TESTS_CPP_COMPILATION)
}
#endif

#endif

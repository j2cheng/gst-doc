/***************************************************************************
*   Copyright (C)2011 - 2017 ARM Ltd (or its subsidiaries)
*   All Rights Reserved
****************************************************************************/

#ifndef _ARM_HDCP_TRANSPORT_H
#define _ARM_HDCP_TRANSPORT_H

#include "ARM_Hdcp_Types.h"

#if defined (__cplusplus) && ! defined (HDCP_TESTS_CPP_COMPILATION)
extern "C"
{
#endif

/**
*  Creates a TCP transport object.
*  If the given direction is downstream, a TCP connection to the remote host will be up
*  when this API returns successfully. If the API fails to create and/or connect, a DX_NULL
*  transport object will be return via pNewTransportObj.
*
* \param [in]   ipAddress            : IP address (4 bytes).
* \param [in]   ctrlPort             : Control port (for HDCP authentication messages).
* \param [in]   direction            : Direction of transport object (downstream/upstream)
* \param [out]  pNewTransportHandle  : A reference to the new transport object.
*
* \return TCP transport object creation status.
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Transport_Create_Tcp(
    IN  const uint8_t*            pIpAddress,
    IN  uint16_t                  ctrlPort,
    IN  EArmTransportDirection    direction,
    OUT ArmHdcpTransportHandle*   pNewTransportHandle);

/**
*  Creates a Callback-driven transport object where the user is responsible for the underline
*  connection and provides send callback function and uses ARM_HDCP_Transport_HandleReceivedData
*  to feed in data to the HDCP.
*
* \param [in]   sendDataCallback      : A pointer to callback function for sending data outside.
* \param [in]   sendDataCallbackArg   : Parameter that passed to the callback function.
* \param [in]   direction             : Direction of transport object (downstream/upstream)
* \param [out]  pNewTransportHandle   : A reference to the new transport object.
*
* \return Callback-driven transport object creation status.
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Transport_Create_CallbackDriven(
    IN  ARM_HDCP_SendDataCallback sendDataCallback,
    IN  const uintptr_t           sendDataCallbackArg,
    IN  EArmTransportDirection    direction,
    OUT ArmHdcpTransportHandle*   pNewTransportHandle);


/**
*  Handle received HDCP data from outside when using Callback-driven transport.
*
* \param [in]   transportHandle   : Transport object.
* \param [in]   data              : A pointer to a buffer containing the received data.
* \param [in]   dataSizeInBytes   : Data size in bytes.
*
* \return Message reception status.
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Transport_HandleReceivedData(
    IN  ArmHdcpTransportHandle    transportHandle,
    IN  const void*               pvData,
    IN  size_t                    dataSizeInBytes);


/**
*  Destroy a transport object created using ARM_HDCP_Transport_Create_CallbackDriven / ARM_HDCP_Transport_Create_Tcp
*  If transport object is not yet paired with a connection, call can be done with no restrictions
*  If transport object is already paired with a connection, call will fail (we don not allow close of local control socket after or during authentication).
* \param [in]   pTransportHandle : Reference to transport object to destroy, will be invalidated (set to NULL) on success
*
* \return Message reception status.
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Transport_Destroy(INOUT ArmHdcpTransportHandle*  pTransportHandle);
    
#if defined (__cplusplus) && ! defined (HDCP_TESTS_CPP_COMPILATION)
} // end extern "C"
#endif

#endif // _ARM_HDCP_TRANSPORT_H

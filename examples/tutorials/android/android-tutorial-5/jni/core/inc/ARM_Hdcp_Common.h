/***************************************************************************
*   Copyright (C)2011 - 2017 ARM Ltd (or its subsidiaries)
*   All Rights Reserved
****************************************************************************/



#ifndef _ARM_HDCP_COMMON_H
#define _ARM_HDCP_COMMON_H

/*! \file ARM_Hdcp_Common.h
This module provides HDCP Common devices services (services which are preformed per single HW device)
*/

#include "ARM_Hdcp_Types.h"

#if defined (__cplusplus) && ! defined (HDCP_TESTS_CPP_COMPILATION)
extern "C"
{
#endif


/*! Updates received SRM data (to all currently active transmitter and repeater instances) and write data to a file
\parameters:
srmData -			SRM data structure
srmDtaLen -			SRM data structure length
\return SRM update status
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Common_Srm_Update(IN uint8_t *pSrmData, IN uint32_t srmDataLen);


/*! Send query whether a given device appears in the SRM. This function can be called straight after calling ARM_HDCP_Engine_Init().
\parameters:
remoteDeviceID -   Requested receiver's ID
pIsRevoked -	   A pointer to a boolean to fill with True/False indication (value returned by function). if returned status is not 0 (success), 
                   this value should be ignored
\return SRM query status
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Common_Srm_Query(IN ArmHdcpReceiverId_t remoteDeviceID, OUT bool *pIsRevoked);



/*! Registers the needed HDCP1.x CONNECTION (single HDMI port) for all converters in the HDCP system. Engine might have existing created repeaters
(converters) or might not, the updated information will be sent to all existing converters, and to all newly created converters
\parameters:
pKsvList             - List of downstream device KSV's
numKsvs              - Number of entries in ksvList
depth                - Downstream device depth

In more details: Number of connection levels below the HDCP1 connection, not including the HDCP1 connection itself.
For example, if our Converter has one HDCP1 Repeater connected to it, which is connected to one Receiver, this API should be called with num_ksvs=2 and depth=1
In another case, if our Converter has only one HDCP1 Receiver connected to it, this API should be called with numKsvs=1 and depth=0
max_cascade_exceeded - Downstream device max_cascade_exceeded indication
max_devs_exceeded    - Downstream device max_devs_exceeded indication
The only difference vs HDCP 2.2/2.1 - the message will contain also the ksv of the sender (We must describe  all direct HDCP 1.x connections too).
Explanation : Lets assume we have HDCP 2.2 repeater A, with HDCP 2.2 repeater B on it's downstream interface. repeater B is connected to 3 HDCP 2.2 receivers. Repeater B will send an upstream prop.
message with DEVICE_COUNT=3 and depth=1 (A knows B because they have already authenticated using HDCP 2.2 protocol messaging)
we must include B in the message since it has never authenticated vs HDCP 2.2 protocol messaging with A. in that case num_ksvs=4 and depth=1
conclusion : if depth = 0, numKsvs must be 1, if depth  > 0 it must always be smaller than numKsvs (depth < numKsvs)
1) When numKsvs==0  depth must be 0,max_cascade_exceeded = false , max_devs_exceeded = false , pKsvList is ignored, else an error will be returned
2) When numKsvs>0  depth must fulfill (depth < numKsvs) , pKsvList must be valid list, else an error will be returned

Note:
If this API fails after the device has been authenticated, the device continues to function as receiver and can display content locally; however,
content should not be streamed to downstream devices since the topology was not accepted. Furthermore, if 2.0 device is connected and the config parameter
isSinkDualModeEnabled is set to true, this API should not be used after authentication since there is no way to initiate a re-authentication process
(which is required when topology is updated) with 2.0 devices. If it will be used, it will fail with ARM_HDCP_2_0_DEVICE_DOES_NOT_SUPPORT_HOT_PLUG.
If isSinkDualModeEnabled is false, than it is okay to call this API after 2.0 device authentication since the sink will initially and always identify
itself as a repeater and so re-authentication is not needed (just upstream propagation).
\return Status 

*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Common_Update_Converters_HDCP1_Topology(IN ArmHdcpReceiverId_t   *pKsvList,
                                                                      IN uint32_t             numKsvs,
                                                                      IN uint32_t             depth,
                                                                      IN bool                 maxCascadeExceeded,
                                                                      IN bool                 maxDevsExceeded);
#if defined (__cplusplus) && ! defined (HDCP_TESTS_CPP_COMPILATION)
}
#endif

#endif // #ifndef _ARM_HDCP_COMMON_H


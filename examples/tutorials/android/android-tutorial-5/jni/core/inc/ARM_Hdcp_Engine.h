/***************************************************************************
*   Copyright (C)2011 - 2017 ARM Ltd (or its subsidiaries)
*   All Rights Reserved
****************************************************************************/


#ifndef _ARM_HDCP_ENGINE_H
#define _ARM_HDCP_ENGINE_H

/*! \file ARM_Hdcp_Engine.h
This module provides the most basic operations that must be done before interaction with any HDCP services, and after completion of such interaction
*/


#include "ARM_Hdcp_Types.h"

#if defined (__cplusplus) && ! defined (HDCP_TESTS_CPP_COMPILATION)
extern "C"
{
#endif


/*! create AND Initialize the HDCP engine. This function must be called before any API function, except ARM_HDCP_Engine_GetSwVersion
    which can be called before. All other calls will fail if called before a successful call to ARM_HDCP_Engine_Init
\parameters:
pConfigFileNameAndPath - absolute config file location (path concatenated with name, OS dependent). This parameter can be also NULL. If NULL, default configuration will be set by engine.
\return engine init status
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Engine_Init(IN const char *pConfigFileNameAndPath);


/*! Finalize and terminate the HDCP engine. Calling this API will also free all previously allocated devices, sessions, streams, transport objects and connections
    This function must be called last. After calling this function, no other function can be called except ARM_HDCP_Engine_Init or ARM_HDCP_Engine_GetSwVersion
\return finalize status
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Engine_Finalize(void);


/*! Returns HDCP version, can be called any time
\parameters:
    pVersion - Buffer where null-terminated version string will be stored.
    versionSize - Size of version string. versionSize value should be equal to or grater than 40.
\return uint32_t - 0 on success, error code on failure.
*/
ARM_EXTERNAL_API uint32_t ARM_HDCP_Engine_GetSwVersion(OUT char* pVersion, IN uint32_t versionSize);

#if defined (__cplusplus) && ! defined (HDCP_TESTS_CPP_COMPILATION)
} // end extern "C"
#endif

#endif //end #ifndef _ARM_HDCP_ENGINE_H

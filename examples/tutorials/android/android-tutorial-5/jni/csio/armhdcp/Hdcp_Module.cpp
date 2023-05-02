/*
 * Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
    
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <fstream>
#include <android/log.h>
#include "ARM_Hdcp_Errors.h"
#include "ARM_Hdcp_Types.h"
#include "Hdcp_Module.h"

/* Macro constants */
#define STRINGIFY(x)                                #x
#define TOSTRING(x)                                 STRINGIFY(x)
#define AT                                          TOSTRING(__LINE__)
#define LOG_TAG                                     "ArmHdcpModule::" AT
#define LOGI(...)                                   __android_log_print(ANDROID_LOG_INFO,LOG_TAG ,  ##__VA_ARGS__)
#define LOGW(...)                                   __android_log_print(ANDROID_LOG_WARN,LOG_TAG,  ##__VA_ARGS__)
#define LOGE(...)                                   __android_log_print(ANDROID_LOG_ERROR,LOG_TAG, ##__VA_ARGS__)
#define LOGI_ENTER                                  LOGI("Enter")
#define ARM_HDCP_SHARED_OBJ_LIB_NAME                "libDxHdcp.so"
#define MAX_NUMBER_OF_STREAMS                       1
#define CONTENT_STREAM_ID                           1000
#define AUTHETICATION_ATTEMPTS_MAX_COUNT            20
#define DELAY_DURATION_SEC_BETWEEN_AUTH_ATTEMPTS    1UL         
#define MAX_DURATION_MILLISEC_TO_CIPHER_ENABLED     11000UL        
#define HDCP_CFG_FILE_NAME                          "ArmHDCP_Qualcomm_Android.cfg"

/*
Assumptions for ARM HDCP libstagefright module implementation:
1) Will work above Qualcomm Platforms Only
2) Supports Single Transmitter HDCP Interface
3) Supports Single HDCP Session
4) Stream type is not given by HDCPAPI.h base class, therefore we support only a single stream (with media type video)
5) contentStreamID (see HDCP Interface Independent Adaptation Specification Rev2_2, section 4.3.15) is not defined as part of the HDCPAPI.h base class. We will use a constant here. User
    might change this constant, and compile it's code with the relevant constant 
6) Provisioning is out of local implementation scope
7) HDCP config file must have the name under macro HDCP_CFG_FILE_NAME. Search path is the first found under array sSharedLibSearchPathArr. If not found, calling will be called with NULL (default config)
8) Transport is TCP based
9) During normal operation, and after cipher is enabled, on non-static networks (content type changed or topology changed), cipher might be disabled for short periods of time.
   Since there is no special event to notify about cipher being disabled, client should expect encrypt/encryptNative to fail with a matching error. While this happen, client should
   retry the same cipher after short period of time (~0.5 sec)
10) The only results de facto to InitAsync are HDCP_INITIALIZATION_COMPLETE / HDCP_INITIALIZATION_FAILED. Error code is not sent to upper layer 
    since we might have AUTHETICATION_ATTEMPTS_MAX_COUNT attempts, and client is interested on the overall result.
    Specific error code for a failed event will be printed to log
11) On libstagefreight_hdcp HDCP config file, parameter SizeBytesForHlosPtrAsSharedMemRef is of size 2MB (must be none zero) by default. 
    This parameter should be set to the maximum needed buffer size per encrypt operation.
12) The supplied make files under jni folder will compile only when building under with a complete Android tree, where Android includes such as android/log.h, utils/Errors.h, system/window can be found by compiler.
    User may modify the make files as he wishes.
13) This source code is intended to by compiled for a 32 bit application only.
*/

/* Macro functions */
#define LOAD_DX_SYMS(__func) \
    do { \
        *(void**)&__func = dlsym(m_handle, #__func); \
        if (0 == __func) { \
            LOGE("Failed to load symbol for " #__func " (error =  %s)", dlerror()); \
        } \
    } while(0)

namespace android {

// static members initialization
ArmHdcpModule *ArmHdcpModule::s_Instance = nullptr;

/* ctor - open shared library and load symbols */
ArmHdcpModule::ArmHdcpLib::ArmHdcpLib() :
    m_isLoaded(false),
    ARM_HDCP_Engine_Init(nullptr),
    ARM_HDCP_Engine_Finalize(nullptr),
    ARM_HDCP_Engine_GetSwVersion(nullptr),
    ARM_HDCP_Transmitter_Create(nullptr),
    ARM_HDCP_Transmitter_Open_Session(nullptr),
    ARM_HDCP_Transmitter_Authentication_Start(nullptr),
    ARM_HDCP_Transmitter_Authentication_Destroy(nullptr),
    ARM_HDCP_Transmitter_Authentication_Invalidate(nullptr),
    ARM_HDCP_Transmitter_Authentication_Restart(nullptr),
    ARM_HDCP_Transmitter_Open_Stream(nullptr),
    ARM_HDCP_Transmitter_Close_Stream(nullptr),
    ARM_HDCP_Transmitter_Encrypt(nullptr),
    ARM_HDCP_Transmitter_Close_Session(nullptr),
    ARM_HDCP_Transmitter_Destroy(nullptr),
    ARM_HDCP_Transmitter_Get_Topology(nullptr),
    ARM_HDCP_Transport_Create_Tcp(nullptr),
    ARM_HDCP_Transport_Destroy(nullptr)
{
    /* load shared object library while All necessary relocations shall be performed when the object is first loaded.  */
    m_handle = dlopen(ARM_HDCP_SHARED_OBJ_LIB_NAME, RTLD_NOW);
    if (0 != m_handle) {     		   
        /* Load all symbols using macro LOAD_DX_SYMS */
        LOAD_DX_SYMS(ARM_HDCP_Engine_Init);
        LOAD_DX_SYMS(ARM_HDCP_Engine_Finalize);
        LOAD_DX_SYMS(ARM_HDCP_Engine_GetSwVersion);
        LOAD_DX_SYMS(ARM_HDCP_Transmitter_Create);
        LOAD_DX_SYMS(ARM_HDCP_Transmitter_Open_Session);
        LOAD_DX_SYMS(ARM_HDCP_Transmitter_Authentication_Start);
        LOAD_DX_SYMS(ARM_HDCP_Transmitter_Authentication_Destroy);
        LOAD_DX_SYMS(ARM_HDCP_Transmitter_Authentication_Invalidate);
        LOAD_DX_SYMS(ARM_HDCP_Transmitter_Authentication_Restart);
        LOAD_DX_SYMS(ARM_HDCP_Transmitter_Open_Stream);
        LOAD_DX_SYMS(ARM_HDCP_Transmitter_Close_Stream);
        LOAD_DX_SYMS(ARM_HDCP_Transmitter_Encrypt);
        LOAD_DX_SYMS(ARM_HDCP_Transmitter_Close_Session);
        LOAD_DX_SYMS(ARM_HDCP_Transmitter_Destroy);
        LOAD_DX_SYMS(ARM_HDCP_Transmitter_Get_Topology);
        LOAD_DX_SYMS(ARM_HDCP_Transport_Create_Tcp);
        LOAD_DX_SYMS(ARM_HDCP_Transport_Destroy);
        m_isLoaded = true;
    }
}

/* dtor - close shared object library and de-initialize members */
ArmHdcpModule::ArmHdcpLib::~ArmHdcpLib()
{
    if (0 != m_handle) {
        dlclose(m_handle);
    }
    m_handle = nullptr;
    ARM_HDCP_Engine_Init = nullptr;
    ARM_HDCP_Engine_Finalize = nullptr;;
    ARM_HDCP_Engine_GetSwVersion = nullptr;
    ARM_HDCP_Transmitter_Create = nullptr;
    ARM_HDCP_Transmitter_Open_Session = nullptr;
    ARM_HDCP_Transmitter_Authentication_Start = nullptr;
    ARM_HDCP_Transmitter_Authentication_Destroy = nullptr;
    ARM_HDCP_Transmitter_Authentication_Invalidate = nullptr;
    ARM_HDCP_Transmitter_Authentication_Restart = nullptr;
    ARM_HDCP_Transmitter_Open_Stream = nullptr;
    ARM_HDCP_Transmitter_Close_Stream = nullptr;
    ARM_HDCP_Transmitter_Destroy = nullptr;
    ARM_HDCP_Transmitter_Get_Topology = nullptr;
    ARM_HDCP_Transport_Create_Tcp = nullptr;
    ARM_HDCP_Transport_Destroy = nullptr;
    m_isLoaded = false;
}

ArmHdcpModule::ArmHdcpModule(void *pvCookie, ObserverFunc f)
    : HDCPModule(pvCookie, f), m_connectionHandle(0), m_port(0), m_transmitterDeviceHandle(0), m_sessionHandle(0), m_streamHandle(0), m_transportHandle(0),
    m_shutdownFlag(false)
{
    m_observerFunc = f;
    m_pvCookie = pvCookie;
    s_Instance = this;
    memset(m_ipAddress, 0, sizeof(m_ipAddress));
    m_InitStatus = HDCP_INITIALIZATION_FAILED;
}

ArmHdcpModule::~ArmHdcpModule() {
    s_Instance = nullptr;
}

status_t ArmHdcpModule::initAsync(const char *strHost, unsigned port)
{
    status_t retStatus = OK;
    struct sigevent sigEv = { 0 };
    struct itimerspec t = { 0 };
    struct TimerInfo *pTimerInfo = new TimerInfo();
    int addr[4];
    int rc;
    
    LOGI_ENTER;
    memset(pTimerInfo, 0, sizeof(struct TimerInfo ));
    //This section is executed in client context, we must lock it since timer thread might access class members as well -> lock
    {
        std::lock_guard<std::mutex> lock(m_lock);

        if (HDCP_INITIALIZATION_COMPLETE == m_InitStatus) {
            LOGE("HDCP already initialized!");;
            retStatus = HDCP_INITIALIZATION_COMPLETE;
            goto end;
        }

        //parse host and port
        if (sscanf(strHost, "%d.%d.%d.%d", &addr[0], &addr[1], &addr[2], &addr[3]) != 4) {
            LOGE("Bad IPv4 address %s\n", strHost);
            retStatus = HDCP_INITIALIZATION_FAILED;
            goto end;
        }
        for (uint32_t i = 0; i < 4; i++) {
            m_ipAddress[i] = (uint8_t)addr[i];
        }
        m_port = port;
    }

    /*
    We do not wish to preform the full HDCP authentication in the current thread context, that might take very long time (up to 10 seconds or more)
    Lets create timer and preform the authentication in another context - using timer event handler thread
    */
    pTimerInfo->pEvent = new DxHDCPModuleEvent(this, &ArmHdcpModule::initArmHdcp);

    /* fill signal event for timer_create. Upon timer expiration, invoke ArmHdcpModule::handleAsyncEvents as if it were the start function of a new thread. */
    sigEv.sigev_notify = SIGEV_THREAD;
    sigEv.sigev_notify_function = ArmHdcpModule::handleAsyncEvents;
    sigEv.sigev_notify_attributes = nullptr;
    sigEv.sigev_value.sival_ptr = pTimerInfo;

    /* create a monotonic timer */
    rc = timer_create(CLOCK_MONOTONIC, &sigEv, &pTimerInfo->timer_id);
    if (0 != rc) { // On success, timer_create() returns 0
        LOGE("Failed to create a timer. timer_create() returned error=%d", rc);
        retStatus = HDCP_INITIALIZATION_FAILED;
        goto end;
    }

    //start the timer, timeout will trigger in 10 micro-sec. current context will wait for events to know if authentication succeeded
    t.it_value.tv_sec = 0;
    t.it_value.tv_nsec = 10000;
    rc = timer_settime(pTimerInfo->timer_id, 0, &t, nullptr);
    if (0 != rc) {
        LOGE("Failed to set timer. timer_settime returned error=%d", rc);
        retStatus = HDCP_INITIALIZATION_FAILED;
        goto end;
    }

end:
    if (OK != retStatus) {
        if (pTimerInfo->pEvent) {
            delete pTimerInfo->pEvent;
        }
        delete pTimerInfo;
    }

    return retStatus;  // On success, only OK returned .HDCP_INITIALIZATION_COMPLETE is sent only upon full initialization completion by timer thread
}


status_t ArmHdcpModule::shutdownAsync()
{
    status_t retStatus = OK;  
    struct sigevent s = { 0 };
    struct TimerInfo *pTimerInfo = new TimerInfo();
    struct itimerspec t = { 0 };
    int rc;

    LOGI_ENTER;
    memset(pTimerInfo, 0, sizeof(struct TimerInfo));

    /*
    We do not wish to wait for the HDCP to shut down, this may take up to 50 millisec or even more
    Lets create timer and preform the authentication in another context - using timer event handler thread
    */
    pTimerInfo->pEvent = new DxHDCPModuleEvent(this, &ArmHdcpModule::shutdownArmHdcp);

    /* fill signal event for timer_create. Upon timer expiration, invoke ArmHdcpModule::handleAsyncEvents as if it were the start function of a new thread. */
    s.sigev_notify = SIGEV_THREAD;
    s.sigev_notify_function = ArmHdcpModule::handleAsyncEvents;
    s.sigev_notify_attributes = nullptr;
    s.sigev_value.sival_ptr = pTimerInfo;

    /* create a monotonic timer */
    rc = timer_create(CLOCK_MONOTONIC, &s, &pTimerInfo->timer_id);
    if (0 != rc) {
        LOGE("Failed to create a timer. timer_create() returned error=%d", rc);
        retStatus = HDCP_SHUTDOWN_FAILED;
        goto end;
    }

    //start the timer, timeout will trigger in 10 micro-sec. current context will wait for events to know if authentication succeeded
    
    t.it_value.tv_sec = 0;
    t.it_value.tv_nsec = 10000;
    rc = timer_settime(pTimerInfo->timer_id, 0, &t, nullptr);
    if (0 != rc) {
        LOGE("Failed to set timer. timer_settime returned error=%d", rc);
        retStatus = HDCP_SHUTDOWN_FAILED;
        goto end;
    }

end:
    if (OK != retStatus) {
        if (pTimerInfo->pEvent) {
            delete pTimerInfo->pEvent;
        }
        delete pTimerInfo;
    }

    return retStatus; // On success, only OK returned .HDCP_SHUTDOWN_COMPLETE is sent only upon full shutdown successful completion by timer thread
}

void ArmHdcpModule::hdcpNotify(ArmHdcpEvent *pEv, void *pvClientData)
{
    LOGI_ENTER;

    if (nullptr == s_Instance){
        LOGE("NULL ArmHdcpModule singelton! (ArmHdcpModule == nullptr)!");
        return;
    }
    LOGI("Got callback : eventType=%d eventStatus=%" PRIu32 " param1=%" PRIu32 " param2=%p", (int)pEv->eventType, pEv->eventStatus, pEv->param1, pEv->param2);
    
    //lock m_ArrivedEventsVec
    std::unique_lock<std::mutex> _lock(s_Instance->m_hdcpNotificationQMutex);

    //store the event in events vector
    s_Instance->m_hdcpNotificationQueue.push(*pEv);

    // Manual unlocking is done before notifying, to avoid waking up
    // the waiting thread only to block again 
    _lock.unlock();
    s_Instance->m_hdcpNotificationQNotifier.notify_one();
}

void ArmHdcpModule::clearQueue()
{
    while (false == m_hdcpNotificationQueue.empty()) {
        m_hdcpNotificationQueue.pop();
    }            
}

/* returns true if process was finished :
1) Cipher enable (ARM_HDCP_STATUS_EVENT_SESSION_CIPHER_ENABLED) event received
2) Some error happened during authentication / propagation process - 
*/
bool ArmHdcpModule::processAutheticationResult()
{
    bool ret = false;

    while (false == m_hdcpNotificationQueue.empty()) {
        ArmHdcpEvent ev = m_hdcpNotificationQueue.front();        
        switch (ev.eventType)
        {
        case ARM_HDCP_PROCESS_EVENT_AUTHETICATION_FAILURE:
        case ARM_HDCP_PROCESS_EVENT_DOWNSTREAM_PROPAGATION_FAILURE:
        case ARM_HDCP_PROCESS_EVENT_UPSTREAM_PROPAGATION_FAILURE:
        case ARM_HDCP_ERROR_EVENT_INTERNAL:
            LOGE("eventType=%d - HDCP_INITIALIZATION_FAILED with error=%" PRIu32, ev.eventType, ev.eventStatus);            
            ret = true;
            break;
        case ARM_HDCP_STATUS_EVENT_SESSION_CIPHER_ENABLED:
            LOGI("eventType=%d - HDCP_INITIALIZATION_COMPLETE", (int)ev.eventType);            
            ret = true;
            m_InitStatus = HDCP_INITIALIZATION_COMPLETE;
            break;
        default:
            break;
        }
        m_hdcpNotificationQueue.pop();
    }
    
    return ret;
}

bool is_file_exist(const char *fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

void ArmHdcpModule::initArmHdcp()
{
    bool bCloseTransportExplicit = false;
    uint32_t status;
    int nAttempts;
    char sVersion[40] = { 0 };
    static const int32_t SO_SEARCH_PATH_MAX_ELEMENTS = 3;
    static const int32_t SO_SEARCH_PATH_MAX_LEN = 60;
    static const char sSharedLibSearchPathArr[SO_SEARCH_PATH_MAX_ELEMENTS][SO_SEARCH_PATH_MAX_LEN] = {
        "/system/vendor/etc/" HDCP_CFG_FILE_NAME,
        "/vendor/etc/" HDCP_CFG_FILE_NAME,
        "/system/etc/" HDCP_CFG_FILE_NAME
    };
    const char *sConfigFileAndPath = nullptr;


    LOGI_ENTER;
    std::lock_guard<std::mutex> lock(m_lock);

    if (false == m_ArmHdcpLib.isLoaded()) {
        LOGE("Failed to init ArmHdcpLib");
        goto init_fail;
    }

    if (m_shutdownFlag) {
        LOGE("ArmHdcp was shut down!");
        goto init_fail;
    }

    status = m_ArmHdcpLib.ARM_HDCP_Engine_GetSwVersion(sVersion, sizeof(sVersion));
    if (status != DX_SUCCESS) {
        LOGE("ARM_HDCP_Engine_GetSwVersion() failed! status=%" PRIu32, status);
        goto init_fail;
    }
    LOGI("ARM_HDCP_Engine_GetSwVersion() returned version string=%s", sVersion);

    //search for a patch which has DxHdcp.cfg. If no such file exist, insert nullptr to ARM_HDCP_Engine_Init
    for (int32_t i = 0; i < SO_SEARCH_PATH_MAX_ELEMENTS; i++) {
        std::ifstream infile(sSharedLibSearchPathArr[i]);
        if (infile.good()) {
            sConfigFileAndPath = sSharedLibSearchPathArr[i];
            break;
        }
    }
    if (nullptr != sConfigFileAndPath) {
        LOGI("HDCP Config file found under %s", sConfigFileAndPath);
    }
    else {
        LOGI("HDCP Config was not found! ARM_HDCP_Engine_Init() will use defaults!");
    }
    status = m_ArmHdcpLib.ARM_HDCP_Engine_Init(sConfigFileAndPath);
    if (status != DX_SUCCESS) {
        LOGE("ARM_HDCP_Engine_Init() failed! status=%" PRIu32, status);
        goto init_fail;
    }
    LOGI("ARM_HDCP_Engine_Init() succeed");

    status = m_ArmHdcpLib.ARM_HDCP_Transmitter_Create(hdcpNotify, (void*)this, &m_transmitterDeviceHandle);
    if (status != DX_SUCCESS) {
        LOGE("ARM_HDCP_Transmitter_Create() failed! status=%" PRIu32, status);
        goto init_fail;
    }
    LOGI("ARM_HDCP_Transmitter_Create() succeed");

    status = m_ArmHdcpLib.ARM_HDCP_Transmitter_Open_Session(m_transmitterDeviceHandle, &m_sessionHandle);
    if (status != DX_SUCCESS) {
        LOGE("ARM_HDCP_Transmitter_Open_Session() failed! status=%" PRIu32, status);
        goto init_fail;
    }
    LOGI("ARM_HDCP_Transmitter_Open_Session() succeed");

    status = m_ArmHdcpLib.ARM_HDCP_Transmitter_Open_Stream(m_sessionHandle, CONTENT_STREAM_ID, ARM_HDCP_STREAM_MEDIA_TYPE_VIDEO, &m_streamHandle);
    if (status != DX_SUCCESS) {
        LOGE("ARM_HDCP_Transmitter_Open_Stream() failed! status=%" PRIu32, status);
        goto init_fail;
    }
    LOGI("ARM_HDCP_Transmitter_Open_Stream() succeed");

    status = m_ArmHdcpLib.ARM_HDCP_Transport_Create_Tcp(m_ipAddress, m_port, ARM_HDCP_TRANSPORT_DIRECTION_DOWNSTREAM, &m_transportHandle);
    if (status != DX_SUCCESS) {
        LOGE("ARM_HDCP_Transport_Create_Tcp() failed! status=%" PRIu32, status);
        goto init_fail;
    }
    bCloseTransportExplicit = true;
    LOGI("ARM_HDCP_Transport_Create_Tcp() succeed");


    // Try to authenticate up to AUTHETICATE_RETRY_COUNT times. Abort immediately on some fatal errors
    LOGI("Autheticate with remote device on ip:port=%d.%d.%d.%d:%d", m_ipAddress[0], m_ipAddress[1], m_ipAddress[2], m_ipAddress[3], m_port);      
    for (m_connectionHandle = 0 , nAttempts = 0; (nAttempts < AUTHETICATION_ATTEMPTS_MAX_COUNT) && (false == m_shutdownFlag); ++nAttempts) {
        LOGI("Attmpt # %d...",  nAttempts+1);
        clearQueue();

        if ((nAttempts == 0) || (m_connectionHandle == 0)) {
            //1st time, or if there was a failure at a very early stage where connection handle was not returned
            status = m_ArmHdcpLib.ARM_HDCP_Transmitter_Authentication_Start(m_transportHandle, m_sessionHandle, &m_connectionHandle);
        } else {
            //re-try with a known connection handle (transport is paired)
            status = m_ArmHdcpLib.ARM_HDCP_Transmitter_Authentication_Restart(m_connectionHandle);
        }
        if (status != DX_SUCCESS) {
            LOGI("Authentication attempt failed! status=%" PRIu32, status);
            continue;
        }
        bCloseTransportExplicit = false;

        //////////////////////////////////////////////////////////////////////////
        //lock m_ArrivedEventsVec
        std::unique_lock<std::mutex> lock(m_hdcpNotificationQMutex);

        //Wait for ARM_HDCP_STATUS_EVENT_SESSION_CIPHER_ENABLED or ARM_HDCP_STATUS_EVENT_SESSION_CIPHER_DISABLED (cipher enable) event as the last one:
        //There is theoretical chance that notify_one was triggered by HDCP  thread before arriving wait_for. 
        //1) Check m_hdcpNotificationQueue first, if it's empty then :
        //2) Try wait_for on m_hdcpNotificationQNotifier conditional variable. If there is timeout
        //3) Then check m_hdcpNotificationQueue again on timeout
        if (processAutheticationResult() == false) {
            bool status = m_hdcpNotificationQNotifier.wait_for(lock, std::chrono::milliseconds(MAX_DURATION_MILLISEC_TO_CIPHER_ENABLED), [=] { return processAutheticationResult(); });
            if (false == status){
                if (processAutheticationResult() == false) {
                    notify(HDCP_UNKNOWN_ERROR, 0, 0);
                }
            }    
        }

        if (m_InitStatus != HDCP_INITIALIZATION_COMPLETE) {
            sleep(DELAY_DURATION_SEC_BETWEEN_AUTH_ATTEMPTS);
        }
        else break;
    }//for
		
    if (AUTHETICATION_ATTEMPTS_MAX_COUNT == nAttempts) {
        LOGE("HDCP: Failed to connect %x", nAttempts);
        goto init_fail;      
    }

    notify(HDCP_INITIALIZATION_COMPLETE, 0, 0);
    LOGI("initArmHdcp success!");
    return;
init_fail:
   
    if (bCloseTransportExplicit && m_transportHandle) {
        status = m_ArmHdcpLib.ARM_HDCP_Transport_Destroy(&m_transportHandle);
        if (status != DX_SUCCESS) {
            LOGE("ARM_HDCP_Transport_Destroy() failed! status=%" PRIu32, status);
        }
    }
    status = m_ArmHdcpLib.ARM_HDCP_Engine_Finalize();
    if (status != DX_SUCCESS) {
        LOGE("m_ArmHdcpLib() failed! status=%" PRIu32, status);
        m_transmitterDeviceHandle = 0;
        m_sessionHandle = 0;
        m_connectionHandle = 0;
        m_streamHandle = 0;
    }
    m_InitStatus = HDCP_INITIALIZATION_FAILED;
    notify(HDCP_INITIALIZATION_FAILED, 0, 0); //ext1 not sent since retry count > 0 , so it is irrelevan, we can't send all error codes for each attempt
}

void ArmHdcpModule::shutdownArmHdcp()
{   
    int msg;

    LOGI_ENTER;
    
    std::lock_guard<std::mutex> lock(m_lock);
    m_shutdownFlag = true;
    
    if (HDCP_INITIALIZATION_COMPLETE == m_InitStatus) {
       uint32_t status = m_ArmHdcpLib.ARM_HDCP_Engine_Finalize();
       if (DX_SUCCESS != status){
           LOGE("ARM_HDCP_Engine_Finalize() failed! status=%" PRIu32, status);
           msg = HDCP_SHUTDOWN_FAILED;
       }
       else {
            msg = HDCP_SHUTDOWN_COMPLETE;
            m_InitStatus = HDCP_INITIALIZATION_FAILED;
            m_connectionHandle = 0;
            m_streamHandle = 0;
            m_transportHandle = 0;
            m_transmitterDeviceHandle = 0;
            m_sessionHandle = 0;
       }      
       notify(msg, 0, 0);      
    } else {
        notify(HDCP_SHUTDOWN_COMPLETE, 0, 0);
    }
    
    m_shutdownFlag = false;
}

void ArmHdcpModule::notify(int msg, int ext1, int ext2)
{
    LOGI("Notify observer %d, %d, %d", msg, ext1, ext2);
    if (s_Instance->m_observerFunc) {
        s_Instance->m_observerFunc(s_Instance->m_pvCookie, msg, ext1, ext2);
    }
}

void ArmHdcpModule::handleAsyncEvents(union sigval e)
{
    TimerInfo *pTimerInfo = (TimerInfo *)e.sival_ptr;

    //handle the event
    pTimerInfo->pEvent->fire();

    //delete timer, free event and TimerInfo
    timer_delete(pTimerInfo->timer_id);
    delete pTimerInfo->pEvent;
    delete pTimerInfo;
}

uint32_t ArmHdcpModule::getCaps()
{
    uint32_t cap = HDCP_CAPS_ENCRYPT | HDCP_CAPS_ENCRYPT_NATIVE;
    LOGI("getCaps() return 0x%" PRIx32, cap);
    return cap;
}

static inline uint16_t U16_AT(const uint8_t *ptr)
{
    return ptr[0] << 8 | ptr[1];
}

status_t  ArmHdcpModule::encryptCommon(uint32_t streamCtr, ArmHdcpPlainTextInfo *pPlainTextIfo, ArmHdcpCipherTxtInfo *pCipherTextInfo, size_t size, OUT uint64_t *pInputCtr)
{
    uint16_t inputCtrSegments[5] = { 0 };
    uint32_t status;

    /* SANITY CHECKS : all pointers must be non-NULL , size must be non-zero, streamCTR must be 0, since only 1 stream is supported */
    if (streamCtr != 0) {
        LOGE("Illegal streamCtr=%" PRIu32 , streamCtr);
        return  BAD_INDEX;
    }
    
    if (0 == pPlainTextIfo->handle) {
        LOGE("input memory handle is NULL!");
        return BAD_VALUE;
    }
    if (nullptr == pInputCtr) {
        LOGE("pInputCtr is NULL!");
        return BAD_VALUE;
    }
    if (0 == pCipherTextInfo->handle) {
        LOGE("output memory handle is NULL!");
        return BAD_VALUE;
    }
    if (0 == size) {
        LOGE("size is 0!");
        return BAD_VALUE;
    }

    //LOGI("encrypt() %s %d bytes", (pPlainTextIfo->type == ARM_HDCP_BUFFER_TYPE_SHARED_MEMORY_HANDLE) ? "native" : "", size);
    std::lock_guard<std::mutex> lock(m_lock);
    
    status = m_ArmHdcpLib.ARM_HDCP_Transmitter_Encrypt(m_streamHandle, (uint32_t)size, pPlainTextIfo, pCipherTextInfo);
    if (DX_SUCCESS != status) {
        LOGE("Encrypt returned error 0x%" PRIx32, status);
        return FAILED_TRANSACTION; //ERROR_IO;
    }

    //Now we need to parse the pesData for the inputCTR as defined in the spec (p.54) :
    //as such:
    // PES_private_data() {
    //  reserved_bits
    //  streamCtr[31..30]
    //  marker_bit
    //  streamCtr[29..15]
    //  marker_bit
    //  streamCtr[14..0]
    //  marker_bit
    //  reserved_bits
    //  inputCtr[63..60]
    //  marker_bit
    //  inputCtr[59..45]
    //  marker_bit
    //  inputCtr[44..30]
    //  marker_bit
    //  inputCtr[29..15]
    //  marker_bit
    //  inputCtr[14..0]
    //  marker_bit
    // }
    inputCtrSegments[0] = (U16_AT(&pCipherTextInfo->PES_PrivateData[6]) >> 1) & 0x000f;
    inputCtrSegments[1] = (U16_AT(&pCipherTextInfo->PES_PrivateData[8]) >> 1) & 0x7fff;
    inputCtrSegments[2] = (U16_AT(&pCipherTextInfo->PES_PrivateData[10]) >> 1) & 0x7fff;
    inputCtrSegments[3] = (U16_AT(&pCipherTextInfo->PES_PrivateData[12]) >> 1) & 0x7fff;
    inputCtrSegments[4] = (U16_AT(&pCipherTextInfo->PES_PrivateData[14]) >> 1) & 0x7fff;

    *pInputCtr =
        (((uint64_t)inputCtrSegments[0]) << 60) |
        (((uint64_t)inputCtrSegments[1]) << 45) |
        (((uint64_t)inputCtrSegments[2]) << 30) |
        (((uint64_t)inputCtrSegments[3]) << 15) |
        inputCtrSegments[4];

    return OK;
}

status_t ArmHdcpModule::encrypt(const void *pDataIn, size_t size, uint32_t streamCtr, OUT uint64_t *pInputCtr, void *pDataOut)
{
    LOGI_ENTER;

    ArmHdcpPlainTextInfo plainTextIfo = { ARM_HDCP_BUFFER_TYPE_VIRTUAL_POINTER, (uint64_t)(uintptr_t)pDataIn, 0 };
    ArmHdcpCipherTxtInfo cipherTextInfo = { ARM_HDCP_BUFFER_TYPE_VIRTUAL_POINTER, (uint64_t)(uintptr_t)pDataOut, 0, false };   
    return encryptCommon(streamCtr, &plainTextIfo, &cipherTextInfo, size, pInputCtr);
}


status_t ArmHdcpModule::encryptNative(buffer_handle_t hBuffer, size_t offset, size_t size, uint32_t streamCtr, OUT uint64_t *pInputCtr, void *pDataOut)
{
    LOGI_ENTER;

    ArmHdcpPlainTextInfo plainTextIfo = { ARM_HDCP_BUFFER_TYPE_SHARED_MEMORY_HANDLE, (uint64_t)hBuffer, 0 };
    ArmHdcpCipherTxtInfo cipherTextInfo = { ARM_HDCP_BUFFER_TYPE_VIRTUAL_POINTER, (uint64_t)(uintptr_t)pDataOut, 0, false };
    return encryptCommon(streamCtr, &plainTextIfo, &cipherTextInfo, size, pInputCtr);
}

extern "C" {
    android::HDCPModule *createHDCPModule(void *pCookie, HDCPModule::ObserverFunc f)
    {      
        return new ArmHdcpModule(pCookie, f);
    }
} //extern "C"

} // namespace android 
//////////////////////////////////////////////////////////////////////////END of namespace

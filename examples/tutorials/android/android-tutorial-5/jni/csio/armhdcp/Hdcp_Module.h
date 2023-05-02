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

#include "HDCPAPI.h"
#include <queue>
#include <mutex>

namespace android {

class ArmHdcpModule;

struct DxHDCPModuleEvent {
    DxHDCPModuleEvent(ArmHdcpModule *pServer, void (ArmHdcpModule::*method)())
        : m_pServer(pServer), m_Method(method) {}
    void fire() { (m_pServer->*m_Method)(); }

private:
    ArmHdcpModule        *m_pServer;
    void (ArmHdcpModule::*m_Method)();
};

struct TimerInfo {
    timer_t             timer_id;
    DxHDCPModuleEvent   *pEvent;
};

class ArmHdcpModule : public HDCPModule {
    public:
        //ctor
        ArmHdcpModule(void *pvCookie, ObserverFunc f);

        //dtor
        ~ArmHdcpModule();

        //Abstract class interface + overriding
        status_t initAsync(const char *strHost, unsigned port);
        status_t shutdownAsync();
        uint32_t getCaps();
        status_t encrypt(const void *pDataIn, size_t size, uint32_t streamCtr, OUT uint64_t *pInputCtr, void *pDataOut);
        status_t encryptNative(buffer_handle_t hBuffer, size_t offset, size_t size, uint32_t streamCtr, OUT uint64_t *pInputCtr, void *pDataOut);

    private:
        //Wrapper class to HDCP API
        class ArmHdcpLib {
        public:
            ArmHdcpLib();
            ~ArmHdcpLib();
            bool isLoaded() { return m_isLoaded; }            

            /* HDCP API function pointers */
            uint32_t(*ARM_HDCP_Engine_Init)(IN const char *pConfigFileNameAndPath);
            uint32_t(*ARM_HDCP_Engine_Finalize)(void);
            uint32_t(*ARM_HDCP_Engine_GetSwVersion)(OUT char* pVersion, IN uint32_t versionSize);
            uint32_t(*ARM_HDCP_Transmitter_Create)(IN ArmEventCbFunction notifyCallbackFunc, IN void *pvClientData, OUT ArmHdcpDeviceHandle *pTsmtDeviceHandle);
            uint32_t(*ARM_HDCP_Transmitter_Open_Session)(IN ArmHdcpDeviceHandle transmitterDeviceHandle, OUT ArmHdcpSessionHandle *pSessionHandle);
            uint32_t(*ARM_HDCP_Transmitter_Authentication_Start)(IN ArmHdcpTransportHandle transportHandle,
                IN ArmHdcpSessionHandle sessionHandle,
                OUT ArmHdcpConnectionHandle *pConnectionHandle);
            uint32_t(*ARM_HDCP_Transmitter_Authentication_Destroy)(INOUT ArmHdcpConnectionHandle *pConnectionHandle);
            uint32_t(*ARM_HDCP_Transmitter_Authentication_Invalidate)(IN ArmHdcpConnectionHandle connectionHandle);
            uint32_t(*ARM_HDCP_Transmitter_Authentication_Restart)(IN ArmHdcpConnectionHandle connectionHandle);
            uint32_t(*ARM_HDCP_Transmitter_Open_Stream)(IN ArmHdcpSessionHandle sessionHandle,
                IN uint32_t contentStreamID,
                IN EArmHdcpStreamMediaType streamMediaType,
                OUT ArmHdcpStreamHandle *pStreamHandle);
            uint32_t(*ARM_HDCP_Transmitter_Close_Stream)(INOUT ArmHdcpStreamHandle *pStreamHandle);
            uint32_t(*ARM_HDCP_Transmitter_Encrypt)(IN ArmHdcpStreamHandle streamHandle,
                IN uint32_t dataSize,
                IN ArmHdcpPlainTextInfo *pInputBufferInfo,
                IN ArmHdcpCipherTxtInfo *pOutputBufferInfo);
            uint32_t(*ARM_HDCP_Transmitter_Close_Session)(INOUT ArmHdcpSessionHandle *pSessionHandle);
            uint32_t(*ARM_HDCP_Transmitter_Destroy)(INOUT ArmHdcpDeviceHandle *pTsmtDeviceHandle);
            uint32_t(*ARM_HDCP_Transmitter_Get_Topology)(IN ArmHdcpDeviceHandle transmitterDeviceHandle,
                OUT ArmHdcpTopologyData_t *pTopologyData);
            uint32_t(*ARM_HDCP_Transport_Create_Tcp)(
                IN  const uint8_t*            pIpAddress,
                IN  uint16_t                  ctrlPort,
                IN  EArmTransportDirection    direction,
                OUT ArmHdcpTransportHandle*   pNewTransportHandle);         
            uint32_t (*ARM_HDCP_Transport_Destroy)(INOUT ArmHdcpTransportHandle*  pTransportHandle);
        private:
            void *m_handle;
            bool  m_isLoaded;
        }; // class ArmHdcpLib 

        /* function members */
        status_t        encryptCommon(uint32_t streamCtr, ArmHdcpPlainTextInfo *pPlainTextIfo, ArmHdcpCipherTxtInfo *pCipherTextInfo, size_t size, OUT uint64_t *pInputCtr);
        void            clearQueue();
        bool            processAutheticationResult();
        void            initArmHdcp();
        void            shutdownArmHdcp();        
        void            notify(int msg, int ext1, int ext2);
        
        // upper layer notification members
        ObserverFunc            m_observerFunc;
        void*                   m_pvCookie;

        std::mutex              m_lock;

        //peer socket
        uint8_t       		    m_ipAddress[4];
        uint16_t		        m_port;

        //status flags
        status_t                m_InitStatus;
        bool                    m_shutdownFlag;

        //HDCP API handles
        ArmHdcpLib              m_ArmHdcpLib;
        ArmHdcpConnectionHandle m_connectionHandle;
        ArmHdcpDeviceHandle     m_transmitterDeviceHandle;
        ArmHdcpSessionHandle    m_sessionHandle;
        ArmHdcpStreamHandle     m_streamHandle;
        ArmHdcpTransportHandle  m_transportHandle;
     
        //ARM HDCP to client notification mechanism members
        std::queue<ArmHdcpEvent> m_hdcpNotificationQueue;       
        std::mutex               m_hdcpNotificationQMutex;
        std::condition_variable  m_hdcpNotificationQNotifier;

        /* static members */
        static ArmHdcpModule *s_Instance; //Singleton instance ptr
        static void     hdcpNotify(ArmHdcpEvent *pEv, void* pvClientData);
        static void     handleAsyncEvents(union sigval e);
    }; // class ArmHdcpModule
} // namespace android

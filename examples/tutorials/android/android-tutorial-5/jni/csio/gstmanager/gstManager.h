#ifndef _GST_MANAGER_H_
#define _GST_MANAGER_H_

#include "../csio.h"
#include "../csioCommBase.h"
#include "gstTx/gst_app_server.h"

class csioProjectClass;
// class CCresRTSPServer;
// class CCresRTSPClient;

//Note: make sure to match with gst_manager_timestamp_names[]
enum
{
    CSIO_MANAGER_TIMESTAMP_INIT = 0,
    CSIO_MANAGER_TIMESTAMP_START,
    CSIO_MANAGER_TIMESTAMP_STOP,

    CSIO_MANAGER_TIMESTAMP_MAX
};
enum
{
    //Note: not used for now
    CSIO_MANAGER_TIMER_TICKS = 0,

    CSIO_MANAGER_TIMER_MAX
};

class GstAppServer;
class gstManager : public csioThreadBaseClass
{
public:

    gstManager(int iId);
    ~gstManager();

    void   DumpClassPara(int);
    void*  ThreadEntry();
    void   sendEvent(csioEventQueueStruct* pEvntQ);

    void setParent(csioProjectClass* p) {m_parent = p;}
    // void configManager(CresRTSPConfig* pconfig);

    csioEventQueueListBase *m_ManagerEventQList;
    csioTimerClockBase* m_ManagerTimeArray;

    csioProjectClass* m_parent;

    virtual void exitThread();
    // virtual void setDebugLevel(int level);
    void lockManager(){if(mLock) mLock->lock();}
    void unlockManager(){if(mLock) mLock->unlock();}

    int  m_gstStreamId;

    // char * m_sourceURL;
    // int  m_CresRTSPManagerId;
    // int  m_servManagerMode;
    // int  m_servListenPort;
    // int  m_clientConnPort, m_rx_UDPPort,m_tx_UDPPort;

    // char m_multicast_addr[MAX_STR_LEN];
    // int  m_multicast_mode;

    // int  m_restart_client_if_waiting;
    // void setClientResartFlag() {m_restart_client_if_waiting = 1;}
    // void resetClientResartFlag() {m_restart_client_if_waiting = 0;}
    // int  getClientResartFlag() {return m_restart_client_if_waiting;}
    // CCresRTSPClient*  getClientObj() {return m_RTSPClientClass;}
private:

    Mutex* mLock;
    GstAppServer* m_app_servs;
    // CCresRTSPServer* m_RTSPServClass;
    // CCresRTSPClient* m_RTSPClientClass;
    // void fixOverflow();
    // void increaseCntOfthisType(EVENTLISTPAIR* listArr,int listSize,int type);
    // int  findIndexOfthisType(EVENTLISTPAIR* listArr,int listSize,int type);
    // EVENTLISTPAIR* makeEventList(int * listsize);
    // int m_WaitTaskToExitFlag,m_start_stop_delay;    
};

// GSTMode used by manager class
typedef enum _eGSTMode
{
    SERVER_ENCODER_NORMAL = 0,
    CLIENT_DECODER_NORMAL,

} eGSTMode;

extern int gstManager_init();

#endif /* _GST_MANAGER_H_ */

#include <android/log.h>
#include <gst/gst.h>
#include "gstManager.h"

GST_DEBUG_CATEGORY_STATIC (debug_category);
#define GST_CAT_DEFAULT debug_category
#define GST_CAT_GSTMANAGER "gstmanager"

const WFD_STRNUMPAIR gst_manager_timestamp_names[] =
{
    {"gst manager init time " ,   CSIO_MANAGER_TIMESTAMP_INIT},
    {"gst manager start time" ,   CSIO_MANAGER_TIMESTAMP_START},
    {"gst manager stop time " ,   CSIO_MANAGER_TIMESTAMP_STOP},

    {0,0}//terminate the list
};

int gstManager_init()
{
    GST_DEBUG_CATEGORY_INIT (debug_category, GST_CAT_GSTMANAGER, 0,
                             "Android csio");
    gst_debug_set_threshold_for_name (GST_CAT_GSTMANAGER, GST_LEVEL_DEBUG);

    gst_server_init();
    return 0;
}
/***************************** CresRTSP manager class **************************************/
gstManager::gstManager(int iId):
m_parent(NULL),
m_gstStreamId(iId),
m_app_serv(NULL)
{
    mLock        = new Mutex();

    m_debugLevel = eLogLevel_debug;

    m_ManagerEventQList = new csioEventQueueListBase(CSIO_DEFAULT_QUEUE_SIZE);

    m_ManagerTimeArray  = new csioTimerClockBase(CSIO_MANAGER_TIMESTAMP_MAX,CSIO_MANAGER_TIMER_MAX);

    if(m_ManagerTimeArray)
        m_ManagerTimeArray->recordEventTimeStamp(CSIO_MANAGER_TIMESTAMP_INIT);

    gst_debug_set_threshold_for_name (GST_CAT_GSTMANAGER, GST_LEVEL_DEBUG);
    GST_DEBUG( "gstManager: set GST_CAT_GSTMANAGER.\n");
}

gstManager::~gstManager()
{
    if(m_ManagerEventQList)
    {
        //get list counts
        int cnt = m_ManagerEventQList->GetEvntQueueCount();

        //remove lists
        if(cnt > 0)
        {
            //loop through all contents of the list
            for(int i = 0; i < cnt; i++)
            {
                csioEventQueueStruct* evntQPtr = NULL;
                if(m_ManagerEventQList->GetFromQueueList(&evntQPtr) && evntQPtr)
                {
                    if( evntQPtr->buf_size && evntQPtr->buffPtr)
                    {
                        deleteCharArray(evntQPtr->buffPtr);
                    }

                    //delete event queue
                    delete evntQPtr;
                }
            }
        }

        delete m_ManagerEventQList;
        m_ManagerEventQList = NULL;
    }

    if(m_ManagerTimeArray)
    {
        delete m_ManagerTimeArray;
        m_ManagerTimeArray = NULL;
    }


    // if (m_sourceURL)
    // {
    //     delete [] m_sourceURL;
    //     m_sourceURL = NULL;
    // }

    if(mLock)
    {
        delete mLock;
        mLock = NULL;
    }
   
    if(m_app_serv)
    {
        delete m_app_serv;
        m_app_serv = NULL;
    }

    // if(m_RTSPClientClass)
    // {
    //     delete m_RTSPClientClass;
    //     m_RTSPClientClass = NULL;
    // }

    GST_DEBUG("gstManager: ~gstManager is DONE\n");
}

void gstManager::DumpClassPara(int l)
{
    GST_DEBUG("---gstManager: m_gstStreamId:  %d.\n",m_gstStreamId);
    GST_DEBUG("---gstManager: m_threadObjLoopCnt:   %d.\n",m_threadObjLoopCnt);
    GST_DEBUG("---gstManager: m_ThreadIsRunning     %d\n", m_ThreadIsRunning);
   
    if(m_ManagerEventQList)
        GST_DEBUG("---gstManager: Event queued up %d\n", m_ManagerEventQList->GetEvntQueueCount());

    // if(m_sourceURL)
    //     gstManager("---gstManager: m_sourceURL: %s\n", m_sourceURL); 

    // if(m_RTSPServClass)
    // {
    //     m_RTSPServClass->DumpClassPara(l);
    // }

    // if(m_RTSPClientClass)
    // {
    //     m_RTSPClientClass->DumpClassPara(l);
    // }
}

void gstManager::exitThread()
{
    m_forceThreadExit = 1;

    csioEventQueueStruct evntQ;
    memset(&evntQ,0,sizeof(csioEventQueueStruct));
    sendEvent(&evntQ);

    GST_DEBUG( "gstManager: exitThread[%d] sent.\n", m_forceThreadExit);
}

void gstManager::sendEvent(csioEventQueueStruct* pEvntQ)
{
    if(pEvntQ)
    {
        csioEventQueueStruct evntQ;
        memset(&evntQ,0,sizeof(csioEventQueueStruct));
        evntQ.obj_id  = pEvntQ->obj_id;
        evntQ.event_type    = pEvntQ->event_type;
        evntQ.buf_size      = 0;
        evntQ.buffPtr       = NULL;
        evntQ.ext_obj       = pEvntQ->ext_obj;
        evntQ.ext_obj2      = pEvntQ->ext_obj2;
        evntQ.voidPtr       = pEvntQ->voidPtr;
        memcpy(evntQ.reserved,pEvntQ->reserved,sizeof(pEvntQ->reserved));
        GST_DEBUG("gstManager: evntQ.reserved[0]=%d,pEvntQ->reserved[0]=%d.\n", evntQ.reserved[0],pEvntQ->reserved[0]);

        void* bufP = pEvntQ->buffPtr;
        int dataSize = pEvntQ->buf_size;

        GST_DEBUG( "gstManager::sendEvent: iId[%d],evnt[%d],dataSize[%d],bufP[0x%x]\n",\
                pEvntQ->obj_id,pEvntQ->event_type,pEvntQ->buf_size,pEvntQ->buffPtr);

        if(bufP && dataSize)
        {
            switch (evntQ.event_type)
            {               
                default:
                {
                    char* tmp = (char*)createCharArray(dataSize + 1);
                    if(tmp)
                    {
                        //first copy configure structure
                        memcpy(tmp,(char*)bufP,dataSize);
                        tmp[dataSize] = 0;
                        evntQ.buffPtr = tmp;
                        evntQ.buf_size = dataSize;
                    }
                    else
                    {
                        GST_DEBUG("gstManager::sendEvent: create buffer failed\n");
                    }

                    break;
                }
            }
        }
        //else

        if(m_ManagerEventQList)
            m_ManagerEventQList->EnqueueAndSignal(evntQ);

        GST_DEBUG( "gstManager::sendEvent[%d]: event added to the queue[0x%x].\n",evntQ.obj_id,evntQ.buffPtr);
        GST_DEBUG( "gstManager::sendEvent[%d]: event[%d] sent.\n",evntQ.obj_id,evntQ.event_type);
    }
    else
    {
        GST_DEBUG( "gstManager::sendEvent: pEvntQ is NULL\n");
    }
}

void* gstManager::ThreadEntry()
{
    GST_DEBUG( "gstManager: Enter ThreadEntry.\n");

    int wtRtn  = 0;
    csioEventQueueStruct* evntQPtr = NULL;

    //log thread init. time stamp
    if(m_ManagerTimeArray)
        m_ManagerTimeArray->recordEventTimeStamp(CSIO_MANAGER_TIMESTAMP_START);

    if(!m_ManagerEventQList)
    {
        GST_DEBUG("gstManager::m_ManagerEventQList is NULL!\n");
        return NULL;
    }

    //create m_app_serv only for testing
    {
        m_app_serv = new GstAppServer();
        char name[100];
        // sprintf(name, "RTSP_SERVER%d", m_CresRTSPManagerId);
        m_app_serv->CreateNewThread("appserv0",NULL);
    }   

    for(;;)
    {
        m_threadObjLoopCnt++;

        wtRtn  = m_ManagerEventQList->waitMsgQueueSignal(CSIO_PROJ_EVNT_POLL_SLEEP_MS);

        GST_DEBUG( "gstManager: waitMsgQueueSignal return:%d, m_threadObjLoopCnt[%d]",wtRtn,m_threadObjLoopCnt);

        evntQPtr = NULL;

        if(m_ManagerEventQList->GetFromQueueList(&evntQPtr) && evntQPtr)
        {
            GST_DEBUG( "gstManager: evntQ is:size[%d],type[%d],iId[%d],GetEvntQueueCount[%d]",\
                            evntQPtr->buf_size,evntQPtr->event_type,evntQPtr->obj_id,m_ManagerEventQList->GetEvntQueueCount());

            switch (evntQPtr->event_type)
            {
                default:
                {
                    GST_DEBUG( "gstManager: unknown type[%d].\n",evntQPtr->event_type);
                    break;
                }
            }

            delete evntQPtr;
        }

        if(m_forceThreadExit)
        {
            //exit all child thread and wait here
            if(m_app_serv)
            {
                //tell thread to exit
                m_app_serv->exitThread();

                //wait until thread exits
                GST_DEBUG("gstManager: call WaitForThreadToExit[0x%x]\n",m_app_serv);
                m_app_serv->WaitForThreadToExit();
                GST_DEBUG("gstManager: Wait is done\n");

                //delete the object
                delete m_app_serv;
                m_app_serv = NULL;
            }
            else
            {

            }

            break;
        }
    }

    GST_DEBUG( "gstManager: exiting...\n");

    //thread exit here
    m_ThreadIsRunning = 0;

    return NULL;
}
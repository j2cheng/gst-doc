#include <android/log.h>
#include <gst/gst.h>

#include "csio.h"
#include "gst_element_print_properties.h"

const WFD_STRNUMPAIR csio_proj_timestamp_names[] =
{
    {"csio project init time     " ,   CSIO_PROJ_TIMESTAMP_INIT},
    {"csio tcp connect start time" ,   CSIO_PROJ_TIMESTAMP_START},
    {"csio tcp connect stop time " ,   CSIO_PROJ_TIMESTAMP_STOP},
    {"csio request idr time      " ,   CSIO_PROJ_TIMESTAMP_REQ_IDR},

    {0,0}//terminate the list
};
static Mutex gProjectsLock;
static void csioProjSendEvent(int iId, int evnt, int data_size, void* bufP);

GST_DEBUG_CATEGORY_STATIC (debug_category);
#define GST_CAT_DEFAULT debug_category
#define GST_CAT_CSIO "csio"

csioProjectClass** csioProjList = NULL ;
/**
 * \detail: function that invoking the shell and return stdoutput.
 *          cmd example: (echo keys *; sleep 0.1) | nc 127.0.0.1 6379
 *          Make sure add sleep here, or 'nc' will get closed too soon! 
 *          Notice MAX_STR_LEN us used here!
 * @param cmd
 * @return string from stdoutput
 */
static std::string GetFromRedis(std::string cmd)
{
    std::string data;
    FILE * stream;
    char buffer[MAX_STR_LEN];

    /** 0 is stdin
        1 is stdout
        2 is stderr
        2>&1 : redirect merger operator, redirect stderr to stdout.
    */
    cmd.append(" 2>&1");//not really needed here

    //using popen for read-only
    stream = popen(cmd.c_str(), "r");
    if (stream) {
        while (!feof(stream))
            if (fgets(buffer, MAX_STR_LEN, stream) != NULL) data.append(buffer);
        pclose(stream);
    }
    else
    {
        __android_log_print (ANDROID_LOG_ERROR, "csio",
                             "%s popen failed with command[%s].",__FUNCTION__,cmd.c_str());
    }
    return data;
}
int csio_init()
{
    GST_DEBUG("%s: enter",__FUNCTION__);
    __android_log_print (ANDROID_LOG_ERROR, "csio",
                         "%s enter.",__FUNCTION__);

    GST_DEBUG_CATEGORY_INIT (debug_category, GST_CAT_CSIO, 0,
                             "Android csio");
    gst_debug_set_threshold_for_name (GST_CAT_CSIO, GST_LEVEL_DEBUG);

//testing testing testing
{
    char buff[300];
    sprintf(buff, "(echo info; sleep 1) | nc %s %d", "127.0.0.1", 6379);
    int retv = system(buff);
    GST_DEBUG("%s: sent command '%s' to Redis, retv = %d", __FUNCTION__,buff,retv);

    retv = system("ls -l &gt; /data/app/test.txt");
//    std::cout << std::ifstream("/data/app/test.txt").rdbuf();

    __android_log_print (ANDROID_LOG_ERROR, "csio","%s sent command '%s' to Redis, retv = %d.",__FUNCTION__,buff,retv);

        std::string ls = GetFromRedis("ls -la; sleep 1");
        __android_log_print (ANDROID_LOG_ERROR, "csio","%s GetFromRedis ls -al[%d] '%s'.",
                             __FUNCTION__,ls.size(),ls.c_str());

        std::string info = GetFromRedis("(echo info; sleep 1) | nc 127.0.0.1 6379");
        __android_log_print (ANDROID_LOG_ERROR, "csio","%s GetFromRedis info[%d] '%s'.",__FUNCTION__,info.size(),info.c_str());

        info = GetFromRedis("(echo keys *; sleep 0.1) | nc 127.0.0.1 6379");
        __android_log_print (ANDROID_LOG_ERROR, "csio","%s GetFromRedis [%d]keys *: '%s'.",__FUNCTION__,info.size(),info.c_str());

        info = GetFromRedis("(echo get key1; sleep 0.1) | nc 127.0.0.1 6379");
        __android_log_print (ANDROID_LOG_ERROR, "csio","%s GetFromRedis get[%d] key1 '%s'.",__FUNCTION__,info.size(),info.c_str());
        const char* tmp = info.c_str();
        for(int i = 0; i < info.size(); i++)
            __android_log_print (ANDROID_LOG_ERROR, "csio","%s GetFromRedis get key1 '0x%x'.",__FUNCTION__,tmp[i]);

        info = GetFromRedis("(echo get key_nothing; sleep 0.1) | nc 127.0.0.1 6379");
        __android_log_print (ANDROID_LOG_ERROR, "csio","%s GetFromRedis get[%d] key_nothing '%s'.",__FUNCTION__,info.size(),info.c_str());
        tmp = info.c_str();
        for(int i = 0; i < info.size(); i++)
            __android_log_print (ANDROID_LOG_ERROR, "csio","%s GetFromRedis get key_nothing '0x%x'.",__FUNCTION__,tmp[i]);

}
    gProjectsLock.lock();

    if(csioProjList == NULL)
    {
        csioProjList = new csioProjectClass* [MAX_PROJCT_OBJ];
        GST_DEBUG("csio_init: new csioProjList:0x%x\n",csioProjList);

        //clean up the list
        for(int i = 0; i < MAX_PROJCT_OBJ; i++)
            csioProjList[i] = NULL;

        //create only one project object for now
        csioProjList[0] = new csioProjectClass(0);

        GST_DEBUG("csio_init: csioProjList[0]:0x%x\n",csioProjList[0]);

        //create only one project thread for now
        csioProjList[0]->CreateNewThread("CSIO_PROJ0",NULL);
    }
    else
    {
        GST_DEBUG("csio_init: project already created.\n");
    }


    gProjectsLock.unlock();
    __android_log_print (ANDROID_LOG_ERROR, "csio",
                         "%s exit.",__FUNCTION__);
    GST_DEBUG("%s: exit",__FUNCTION__);

    gstManager_init();

    return 0;
}
void csioProjStartServer(int streamID)
{
    GST_DEBUG("csioProjStartServer(%d) enter\n",streamID);

    gProjectsLock.lock();
    csioProjSendEvent(streamID, CSIOPROJ_EVENT_CSIO_START_SERV,0,NULL);
    gProjectsLock.unlock();
    GST_DEBUG("csioProjStartServer(%d) exit\n",streamID);
}
void csioProjStopServer(int streamID)
{
    GST_DEBUG("csioProjStopServer(%d) enter\n",streamID);
    gProjectsLock.lock();
    csioProjSendEvent(streamID, CSIOPROJ_EVENT_CSIO_STOP_SERV,0,NULL);
    gProjectsLock.unlock();
    GST_DEBUG("csioProjStopServer(%d) exit\n",streamID);
}

//local static functio without lock
static void csioProjSendEvent(int iId, int evnt, int data_size, void* bufP)
{
    //only one project for now
    if(csioProjList && csioProjList[0])
    {
        GST_DEBUG("csioProjSendEvent: [%d]call sendEvent.\n",iId);
        csioEventQueueStruct EvntQ;
        memset(&EvntQ,0,sizeof(csioEventQueueStruct));
        EvntQ.obj_id = iId;
        EvntQ.event_type = evnt;
        EvntQ.buf_size   = data_size;
        EvntQ.buffPtr    = bufP;

        csioProjList[0]->sendEvent(&EvntQ);
        GST_DEBUG("csioProjSendEvent: [%d]done sendEvent.\n",iId);
    }
    else
    {
        GST_DEBUG("csioProjSendEvent: no project is running\n");
    }
}
/********** csioProjectClass class *******************/
csioProjectClass::csioProjectClass(int iId):
m_projectID(iId),
csioProjectTimeArray(NULL),
m_csioManagerTaskObjList(),
m_projEventQList(NULL)
{
    m_debugLevel = eLogLevel_debug;

    m_projEventQList = new csioEventQueueListBase(CSIO_DEFAULT_QUEUE_SIZE);

    csioProjectTimeArray = new csioTimerClockBase(CSIO_PROJ_TIMESTAMP_MAX,CSIO_PROJ_TIMER_MAX);

    if(csioProjectTimeArray)
        csioProjectTimeArray->recordEventTimeStamp(CSIO_PROJ_TIMESTAMP_INIT);

    m_csioManagerTaskObjList = new gstManager* [MAX_STREAM];
    for(int i = 0; i < MAX_STREAM; i++)
    {
        m_csioManagerTaskObjList[i] = NULL;
    }


    GST_DEBUG( "csioProjectClass: creating csioProjectClass.\n");
}
csioProjectClass::~csioProjectClass()
{
    GST_DEBUG( "csioProjectClass: ~csioProjectClass.\n");

    if(m_projEventQList)
    {
        //get list counts
        int cnt = m_projEventQList->GetEvntQueueCount();

        //remove lists
        if(cnt > 0)
        {
            //loop through all contents of the list
            for(int i = 0; i < cnt; i++)
            {
                csioEventQueueStruct* evntQPtr = NULL;
                if(m_projEventQList->GetFromQueueList(&evntQPtr) && evntQPtr)
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

        delete m_projEventQList;
        m_projEventQList = NULL;
    }

    if(csioProjectTimeArray)
    {
        delete csioProjectTimeArray;
        csioProjectTimeArray = NULL;
    }

    for(int i = 0; i < MAX_STREAM; i++)
    {
        if(m_csioManagerTaskObjList[i])
        {
            GST_DEBUG("csioProjectClass: calling exit manager id[%d]\n",i);

            m_csioManagerTaskObjList[i]->exitThread();
            GST_DEBUG("csioProjectClass: call WaitForThreadToExit[0x%x]\n",m_csioManagerTaskObjList[i]);
            m_csioManagerTaskObjList[i]->WaitForThreadToExit();
            GST_DEBUG("csioProjectClass: Wait is done\n");
          
            //delete the object, and set list to NULL
            delete m_csioManagerTaskObjList[i];
            m_csioManagerTaskObjList[i] = NULL;
        }//else
    }
}

void csioProjectClass::DumpClassPara(int id)
{
    GST_DEBUG( "csioProjectClass: threadObjLoopCnt:   %d.\n",m_threadObjLoopCnt);

    GST_DEBUG( "csioProjectClass: ThreadId            0x%x\n", (unsigned long int)getThredId());
    GST_DEBUG( "csioProjectClass: m_debugLevel        %d\n", m_debugLevel);
    GST_DEBUG( "csioProjectClass: m_ThreadIsRunning   %d\n", m_ThreadIsRunning);

    if(m_projEventQList)
    {
        GST_DEBUG( "csioProjectClass: Event queued up     %d\n", m_projEventQList->GetEvntQueueCount());
    }

    if(csioProjectTimeArray)
    {
        char  time_string[40];
        long  milliseconds;

        for(int i = 0; i < CSIO_PROJ_TIMESTAMP_MAX; i++)
        {
            csioProjectTimeArray->convertTime(i,time_string,40,milliseconds);
            GST_DEBUG( "csioProjectClass: %s:  %s.%03ld\n",
                       csio_proj_timestamp_names[i].pStr, time_string, milliseconds);
        }
    }

    for(int i = 0; i < MAX_STREAM; i++)
    {
        if(m_csioManagerTaskObjList[i])
        {
            m_csioManagerTaskObjList[i]->DumpClassPara(0);
        }
    }
}

void csioProjectClass::exitThread()
{
    m_forceThreadExit = 1;

    csioEventQueueStruct evntQ;
    memset(&evntQ,0,sizeof(csioEventQueueStruct));
    sendEvent(&evntQ);

    GST_DEBUG( "csioProjectClass: exitThread[%d] sent.\n", m_forceThreadExit);
}

void csioProjectClass::sendEvent(csioEventQueueStruct* pEvntQ)
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
        GST_DEBUG("csioProjectClass: evntQ.reserved[0]=%d,pEvntQ->reserved[0]=%d.\n", evntQ.reserved[0],pEvntQ->reserved[0]);

        void* bufP = pEvntQ->buffPtr;
        int dataSize = pEvntQ->buf_size;

        GST_DEBUG( "csioProjectClass::sendEvent: iId[%d],evnt[%d],dataSize[%d],bufP[0x%x]\n",\
                pEvntQ->obj_id,pEvntQ->event_type,pEvntQ->buf_size,pEvntQ->buffPtr);

        if(bufP && dataSize)
        {
            switch (evntQ.event_type)
            {
                // case CSIO_EVENTS_RTSP_IN_SESSION_EVENT:
                // {
                //     GST_PIPELINE_CONFIG* new_config = new GST_PIPELINE_CONFIG();
                //     GST_PIPELINE_CONFIG* gst_config = (GST_PIPELINE_CONFIG*)bufP;
                //     if(gst_config)
                //     {
                //         new_config->ts_port = gst_config->ts_port;
                //         new_config->ssrc    = gst_config->ssrc;
                //         new_config->rtcp_dest_port = gst_config->rtcp_dest_port;
                //         new_config->pSrcVersionStr = NULL;

                //         if(gst_config->pSrcVersionStr)
                //         {
                //             int strSize = strlen(gst_config->pSrcVersionStr);
                //             char* tmp = (char*)createCharArray(strSize + 1);
                //             if(tmp)
                //             {
                //                 memcpy(tmp,gst_config->pSrcVersionStr,strSize);
                //                 tmp[strSize] = 0;
                //                 new_config->pSrcVersionStr = tmp;
                //             }
                //         }

                //         evntQ.buffPtr = (void*)new_config;
                //         evntQ.buf_size = sizeof(GST_PIPELINE_CONFIG);
                //     }
                //     break;
                // }
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
                        GST_DEBUG("csioProjectClass::sendEvent: create buffer failed\n");
                    }

                    break;
                }
            }
        }
        //else

        if(m_projEventQList)
            m_projEventQList->EnqueueAndSignal(evntQ);

        GST_DEBUG( "csioProjectClass::sendEvent[%d]: event added to the queue[0x%x].\n",evntQ.obj_id,evntQ.buffPtr);
        GST_DEBUG( "csioProjectClass::sendEvent[%d]: event[%d] sent.\n",evntQ.obj_id,evntQ.event_type);
    }
    else
    {
        GST_DEBUG( "csioProjectClass::sendEvent: pEvntQ is NULL\n");
    }
}

void* csioProjectClass::ThreadEntry()
{
    GST_DEBUG( "csioProjectClass: Enter ThreadEntry.\n");

    int wtRtn  = 0;
    csioEventQueueStruct* evntQPtr = NULL;

    //log thread init. time stamp
    if(csioProjectTimeArray)
        csioProjectTimeArray->recordEventTimeStamp(CSIO_PROJ_TIMESTAMP_INIT);

    if(!m_projEventQList)
    {
        GST_DEBUG("csioProjectClass::m_projEventQList is NULL!\n");
        return NULL;
    }

    for(;;)
    {
        m_threadObjLoopCnt++;

        wtRtn  = m_projEventQList->waitMsgQueueSignal(CSIO_PROJ_EVNT_POLL_SLEEP_MS);

        GST_DEBUG( "csioProjectClass: waitMsgQueueSignal return:%d, m_threadObjLoopCnt[%d]\n",wtRtn,m_threadObjLoopCnt);

        evntQPtr = NULL;

        if(m_projEventQList->GetFromQueueList(&evntQPtr) && evntQPtr)
        {
            GST_DEBUG( "csioProjectClass: evntQ is:size[%d],type[%d],iId[%d],GetEvntQueueCount[%d]\n",\
                            evntQPtr->buf_size,evntQPtr->event_type,evntQPtr->obj_id,m_projEventQList->GetEvntQueueCount());

            switch (evntQPtr->event_type)
            {
                case CSIOPROJ_EVENT_CSIO_START_SERV:
                case CSIOPROJ_EVENT_CSIO_START_CLIENT:
                {
                    int id = evntQPtr->obj_id;
                    GST_DEBUG("csioProjectClass: processing cmd[%d] = %d\n",evntQPtr->event_type,id);

                    // int evntTimeType = RTSPPROJ_EVENTTIME_CMD_SEV_START;
                    // if( evntQ.event_type == CRESRTSP_EVENT_CSIO_START_CLIENT)
                    //     evntTimeType = RTSPPROJ_EVENTTIME_CMD_CLIENT_START;

                    if( !IsValidStream(id) )
                    {
                        GST_DEBUG("csioProjectClass: obj ID is invalid = %d\n",id);
                        break;
                    }

                    //check configuration
                    // if(m_ConfigTable[id].CresRTSPStreamID == -1)
                    // {
                    //     STRLOG(LOGLEV_ERROR,LOGCAT_GSRTSPDEFAULT, "CresRTSP_project ERROR: need configuration first\n");
                    //     break;
                    // }

                    if(m_csioManagerTaskObjList)
                    {
                        if(m_csioManagerTaskObjList[id])
                        {
                            GST_DEBUG("csioProjectClass: gstmanager already exist,id[%d].\n",id);
                        }
                        else
                        {
                            m_csioManagerTaskObjList[id] = new gstManager(id);

                            //set parameters
                            m_csioManagerTaskObjList[id]->setParent(this);
                            // m_csioManagerTaskObjList[id]->configManager(&m_ConfigTable[id]);
                            m_csioManagerTaskObjList[id]->setDebugLevel(m_debugLevel);

                            //init default variables

                            //start thread
                            char name[100];
                            sprintf(name, "GST_MANAGER%d", id);
                            m_csioManagerTaskObjList[id]->CreateNewThread(name,NULL);

                            //log start time stamp
                            // gettimeofday(&eventTime[evntTimeType], NULL);
                            GST_DEBUG("csioProjectClass: new gstmanager started:id[%d]\n",id);
                        }
                    }
                    break;
                }
                case CSIOPROJ_EVENT_CSIO_STOP_SERV:
                case CSIOPROJ_EVENT_CSIO_STOP_CLIENT:
                {
                    int id = evntQPtr->obj_id;
                    GST_DEBUG("csioProjectClass: processing cmd[%d] = %d\n",evntQPtr->event_type,id);

                    if( !IsValidStream(id) )
                    {
                        GST_DEBUG("csioProjectClass: obj ID is invalid = %d\n",id);
                        break;
                    }

                    if(m_csioManagerTaskObjList && m_csioManagerTaskObjList[id])
                    {                        
                        //tell thread to exit
                        m_csioManagerTaskObjList[id]->exitThread();

                        //wait until thread exits
                        GST_DEBUG("csioProjectClass: call WaitForThreadToExit[0x%x]\n",m_csioManagerTaskObjList[id]);
                        m_csioManagerTaskObjList[id]->WaitForThreadToExit();
                        GST_DEBUG("csioProjectClass: Wait is done\n");

                        //delete the object, and set list to NULL
                        delete m_csioManagerTaskObjList[id];
                        m_csioManagerTaskObjList[id] = NULL;

                        GST_DEBUG("csioProjectClass: m_csioManagerTaskObjList[%d] deleted.\n",id);
                    }
                    else
                    {
                        GST_DEBUG("csioProjectClass: gstmanager NOT exist[0x%x], id[%d].\n",id);
                    }

                    break;
                }
                default:
                {
                    GST_DEBUG( "csioProjectClass: unknown type[%d].\n",evntQPtr->event_type);
                    break;
                }
            }

            delete evntQPtr;
        }

        if(checkStartSrv())
        {
            GST_DEBUG( "csioProjectClass: need to start server...\n");
            csioProjStartServer(0);
        }

        if(checkStopSrv())
        {
            GST_DEBUG( "csioProjectClass: need to stop server...\n");
            csioProjStopServer(0);
        }

        if(m_forceThreadExit)
        {
            //TODO: exit all child thread and wait here
            break;
        }
    }

    GST_DEBUG( "csioProjectClass: exiting...\n");

    //thread exit here
    m_ThreadIsRunning = 0;

    return NULL;
}

bool csioProjectClass::checkStartSrv()
{
    std::string  info = GetFromRedis("(echo get STARTSERV; sleep 0.1) | nc 127.0.0.1 6379");
    GST_DEBUG ("%s GetFromRedis get[%d] STARTSERV '%s'.",__FUNCTION__,info.size(),info.c_str());
    const char* tmp = info.c_str();
    bool gotkey = false;

    if(tmp[0] != '$')
    {
        GST_DEBUG ("%s GetFromRedis get STARTSERV return false(no '$'). '0x%x -- %c'.",__FUNCTION__,tmp[0],tmp[0]);
        return false;
    }

    if(tmp[1] == '-')
    {
        GST_DEBUG ("%s GetFromRedis get STARTSERV return false(no key). '0x%x -- %c'.",__FUNCTION__,tmp[1],tmp[1]);
        return false;
    }


    for(int i = 2; i < info.size(); i++)
    {
        GST_DEBUG ("%s GetFromRedis get STARTSERV '0x%x -- %c'.",__FUNCTION__,tmp[i],tmp[i]);
        gotkey = true;
    }

    if(gotkey)
    {
        info = GetFromRedis("(echo del STARTSERV; sleep 0.1) | nc 127.0.0.1 6379");
        GST_DEBUG ("%s GetFromRedis del STARTSERV called",__FUNCTION__);
    }

    return gotkey;
}
bool csioProjectClass::checkStopSrv()
{
    std::string  info = GetFromRedis("(echo get STOPSERV; sleep 0.1) | nc 127.0.0.1 6379");
    GST_DEBUG ("%s GetFromRedis get[%d] STOPSERV '%s'.",__FUNCTION__,info.size(),info.c_str());
    const char* tmp = info.c_str();
    bool gotkey = false;

    if(tmp[0] != '$')
    {
        GST_DEBUG ("%s GetFromRedis get STOPSERV return false(no '$'). '0x%x -- %c'.",__FUNCTION__,tmp[0],tmp[0]);
        return false;
    }

    if(tmp[1] == '-')
    {
        GST_DEBUG ("%s GetFromRedis get STOPSERV return false(no key). '0x%x -- %c'.",__FUNCTION__,tmp[1],tmp[1]);
        return false;
    }


    for(int i = 2; i < info.size(); i++)
    {
        GST_DEBUG ("%s GetFromRedis get STOPSERV '0x%x -- %c'.",__FUNCTION__,tmp[i],tmp[i]);
        gotkey = true;
    }

    if(gotkey)
    {
        info = GetFromRedis("(echo del STOPSERV; sleep 0.1) | nc 127.0.0.1 6379");
        GST_DEBUG ("%s GetFromRedis del STOPSERV called",__FUNCTION__);
    }

    return gotkey;
}
/********** end of csioProjectClass class *******************/

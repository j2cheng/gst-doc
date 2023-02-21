#ifndef CSIO_COMMON_BASE_H_
#define CSIO_COMMON_BASE_H_

#include <string>
#include <list>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <android/log.h>
#include <pthread.h>

//
//#define STRLOG(level, category,  )         __android_log_print (ANDROID_LOG_ERROR, "csioComm",
//#define GST_ERROR_OBJECT(obj,...)	GST_CAT_LEVEL_LOG (GST_CAT_DEFAULT, GST_LEVEL_ERROR,   obj,  __VA_ARGS__)

typedef struct
{
    const char * pStr;
    unsigned int num;
} WFD_STRNUMPAIR;

#define CSIO_DEFAULT_QUEUE_SIZE 1000
#define MAX_DEBUG_ARRAY_SIZE 100
#define MAX_THREAD_NAME_SIZE 16
#define MILLION 1000000LL     //make sure it is defined 64bit long
#define MAX_STR_LEN    256

#define ABOVE_DEBUG_VERB(a)    (a+1)
#define ABOVE_DEBUG_XTRVERB(a) (a+2)
typedef enum
{
    eLogLevel_error = 0,
    eLogLevel_warning,
    eLogLevel_info,
    eLogLevel_debug,
    eLogLevel_verbose,
    eLogLevel_extraVerbose,

    // Do not add anything after the Last state
    eLogLevel_LAST
}eLogLevel;
#define CSIO_DEFAULT_LOG_LEVEL eLogLevel_debug

typedef struct _CSIO_EVNT_QUEUE_STRUCT
{
    unsigned int   obj_id;
    int   event_type;
    int   buf_size;
    void* buffPtr;

    int   ext_obj;
    int   ext_obj2;
    void* voidPtr;
    char  reserved[56];
} csioEventQueueStruct;

class Mutex
{
public:
    Mutex()
    {
        pthread_mutex_init(&m_lock, NULL);
        is_locked = false;
    }

    ~Mutex()
    {
        if( is_locked )
            unlock();

        pthread_mutex_destroy(&m_lock);
    }

    void lock()
    {
        pthread_mutex_lock(&m_lock);
        is_locked = true;
    }

    void unlock()
    {
        is_locked = false; //do it BEFORE unlocking to avoid race condition
        pthread_mutex_unlock(&m_lock);
    }

    pthread_mutex_t *get_mutex_ptr()
    {
        return &m_lock;
    }

//private:
    pthread_mutex_t m_lock;
    volatile bool is_locked;
};

/***************************** csioThreadBaseClass class ******************************************/
class csioThreadBaseClass
{
public:
    csioThreadBaseClass();
    virtual ~csioThreadBaseClass();

    /** Returns true if created, false otherwise */
    virtual int CreateNewThread(const char* name, pthread_attr_t* attr);

    /* Will not return until thread has exited. */
    virtual void WaitForThreadToExit() { if(_validThread) pthread_join(_thread, NULL); }

    virtual void setDebugLevel(int level) { m_debugLevel = level; }
    int  getDebugLevel()          { return m_debugLevel;  }

    virtual void exitThread() { m_forceThreadExit = 1; }
    int getThredId()          { return m_threadBaseID;  }

    virtual void DumpClassPara( int ) = 0;

    char  m_forceThreadExit;

    int  m_ThreadIsRunning,m_threadObjLoopCnt;

    int  eventPushArray[MAX_DEBUG_ARRAY_SIZE];
    int  eventPushIndex;
    void logEventPush(int e);

    int  eventPopArray[MAX_DEBUG_ARRAY_SIZE];
    int  eventPopIndex;
    void logEventPop(int e);

protected:
    /** Must implement in subclass. */
    virtual void* ThreadEntry() = 0;

    int   m_debugLevel;
    static int   m_threadBaseID;
    void* createCharArray(int size) { return new char [size]; }
    void deleteCharArray(void* buf)
    {
        if(buf)
        {
            char* tmp = (char*)buf;
            delete [] tmp;
        }
    }
private:
    static void * ThreadEntryFunc(void * This) {((csioThreadBaseClass *)This)->ThreadEntry(); return NULL;}
protected:
    pthread_t _thread;
    bool _validThread;
};

/***************************** end of csioThreadBaseClass class ************************************************/

/***************************** csioEventQueueListBase class ******************************************/
class csioEventQueueListBase
{
public:
    csioEventQueueListBase(int maxCnt);
    virtual  ~csioEventQueueListBase();

    virtual  void closeAllSig();

    int  EnqueueAndSignal( csioEventQueueStruct& pEvntQueue , bool lock = true);
    virtual bool GetFromQueueList(csioEventQueueStruct** ppQueue ,bool lock = true);
    int  GetEvntQueueCount(void) ;
    void clearQAndDeleteQ(csioEventQueueStruct* evntQueue);

    std::list<csioEventQueueStruct*> m_message_list;

    virtual void signalWaitingThread(int v);
    virtual int  waitMsgQueueSignal(int pollTimeoutInMs);

    void lockMsgQ()   {if(m_MsgQMtx)   m_MsgQMtx->lock();}
    void unlockMsgQ() {if(m_MsgQMtx)   m_MsgQMtx->unlock();}

    void setDebugLevel(int level) { m_debugLevel = level; }
    int  getDebugLevel() { return m_debugLevel; }

    int  m_debugLevel;

    int m_msgQ_pipe_fd[2];
    int m_msgQ_pipe_readfd   ;
    int m_msgQ_pipe_writefd  ;
protected:
    Mutex *m_MsgQMtx;
private:
    int   m_iOverFlowThds,m_iMaxCount;  //overflow threshold

    void del_Q_buf(void* buffPtr)
    {
        if(buffPtr)
        {
            char* tmp = (char*)buffPtr;
            delete [] tmp;
        }
    }
};
/***************************** end of csioEventQueueListBase class ************************************************/

/***************************** end of csioTimerClockBase class ************************************************/
class csioTimerClockBase
{
public:
    csioTimerClockBase(int MaxEvts,int MaxTimers);
    virtual  ~csioTimerClockBase();

    virtual void   resetTimeout(int);
    virtual void   setTimeout(int index, int inMs);
    virtual bool   isTimeout(int index);
    virtual void   recordEventTimeStamp(int );

    void setDebugLevel(int level) { m_debugLevel = level; }
    int  getDebugLevel() { return m_debugLevel; }

    void   convertTime(int index,char* time_string,int sizeofstring,long& milliseconds);

    static int takeDiff(int _start_sec, int _start_usec, int _end_sec, int _end_usec);

private:
    int  m_debugLevel;
    int m_MaxEvts,m_MaxTimers;
public:
    int* pTimeoutInMsArray;
    struct timeval* pEventTime;
    struct timespec* pTimeoutWaitEvents;
};
/***************************** end of csioTimerClockBase class ************************************************/
#endif /* CSIO_COMMON_BASE_H_ */

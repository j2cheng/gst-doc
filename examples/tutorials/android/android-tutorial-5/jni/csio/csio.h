//
// Created by builduser on 2/15/23.
//
#ifndef ANDROID_CSIO_H
#define ANDROID_CSIO_H

#define MAX_PROJCT_OBJ          1

#define MAX_STREAM 10

#define IsValidStream(a)  ( (a >= 0) && (a < MAX_STREAM) )

//Note: make sure to match with csio_proj_timestamp_names[]
enum
{
    CSIO_PROJ_TIMESTAMP_INIT = 0,
    CSIO_PROJ_TIMESTAMP_START,
    CSIO_PROJ_TIMESTAMP_STOP,
    CSIO_PROJ_TIMESTAMP_REQ_IDR,

    CSIO_PROJ_TIMESTAMP_MAX
};
enum
{
    //Note: not used for now
    CSIO_PROJ_TIMER_TICKS = 0,

    CSIO_PROJ_TIMER_MAX
};

typedef enum _eCsioProjEvents
{
//events come from CSIO to the project
    CSIOPROJ_EVENTS_CSIO_CONFIG = 100,

    CSIOPROJ_EVENT_CSIO_START_SERV,
	CSIOPROJ_EVENT_CSIO_STOP_SERV,

	CSIOPROJ_EVENT_CSIO_START_CLIENT,
	CSIOPROJ_EVENT_CSIO_STOP_CLIENT,

	CSIOPROJ_EVENT_CSIO_RESTART_CLIENT,

	CSIOPROJ_EVENTS_MAX
}eCsioProjEvents;

#define CSIO_PROJ_EVNT_POLL_SLEEP_MS   1000   //1000ms


#ifdef __cplusplus
#include "csioCommBase.h""
#include "gstmanager/gstManager.h"

class gstManager;

class csioProjectClass : public csioThreadBaseClass
{
public:

    csioProjectClass(int iId);
    ~csioProjectClass();

    csioTimerClockBase* csioProjectTimeArray;

    virtual void exitThread();
    virtual void DumpClassPara(int);

    void sendEvent(csioEventQueueStruct* pEvntQ);

    // char localIPName[MAX_WFD_TCP_CONN][32];
    // const char* getLocIPName(int id){return localIPName[id];}

    int getDebugLevel(){ return(m_debugLevel); }

private:
    int  m_projectID;
    void* ThreadEntry();

    gstManager** m_csioManagerTaskObjList ;

    csioEventQueueListBase* m_projEventQList;
};

extern void csioProjStartServer(int streamID);
extern void csioProjStopServer(int streamID);

#endif

#ifdef __cplusplus
extern "C" {
#endif

int csio_init();

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif //ANDROID_CSIO_H
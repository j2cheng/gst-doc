#ifndef __GSTAPPSERVER_H__
#define __GSTAPPSERVER_H__

//////////////////////////////////////////////////////////////////////////////
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include "../gstManager.h"


class gstManager;
class GstAppServer : public csioThreadBaseClass
{
public:

    GstAppServer();
    ~GstAppServer();

    void   DumpClassPara(int);
    void*  ThreadEntry();
    void   setParent(gstManager* p) {m_parent = p;}

    gstManager* m_parent;

    void lock_gst_app(){if(mLock) mLock->lock();}
    void unlock_gst_app(){if(mLock) mLock->unlock();}

    virtual void setDebugLevel(int level);

    virtual void exitThread() ;

    char m_rtsp_port[MAX_STR_LEN];
    void setPort(char* p){strcpy(m_rtsp_port, p);}

    GMainLoop * getMainloop(){ return m_loop;}
    static void media_configure_cb(GstRTSPMediaFactory * factory, GstRTSPMedia * media,
                                   gpointer user_data);

    static GstRTSPFilterResult filter_cb(GstRTSPStream *stream,
            GstRTSPStreamTransport *trans, gpointer user_data);

    static void client_connected_new_client_cb (GstRTSPServer * server,
            GstRTSPClient * client, gpointer user_data);

    // static void setup_request_from_client (GstRTSPClient * client,
    //         GstRTSPContext * ctx, gpointer user_data);

    // static void play_request_from_client (GstRTSPClient * client,
    //         GstRTSPContext * ctx, gpointer user_data);

    // static void closed_from_client (GstRTSPClient * client,gpointer user_data);

    static void media_unprepared (GstRTSPMedia * media,gpointer user_data);
    
    static int m_client_count ;
    static int m_media_status ;
private:
    Mutex* mLock;
    GMainLoop *  m_loop;
    GstRTSPMediaFactory *m_factory;
    GstRTSPMedia * m_pMedia ;

    char m_client_ip_addr[MAX_STR_LEN];
};

extern int gst_server_init();

#endif //__GSTAPPSERVER_H__


/**
 * Copyright (C) 2016 to the present, Crestron Electronics, Inc.
 * All rights reserved.
 * No part of this software may be reproduced in any form, machine
 * or natural, without the express written consent of Crestron Electronics.
 * 
 * \file        gst_app_server.cpp
 *
 * \brief       Implementation of RTSP stream out
 *
 * \author      John Cheng
 *
 * \date        2/22/2023
 *
 * \note
 */

///////////////////////////////////////////////////////////////////////////////
#include <gst/gst.h>

#include "gst_app_server.h"
#include <arpa/inet.h>
//#include "rtsp-media.h"
// #define CRES_UNPREPARE_MEDIA       1
// #define CRES_OVERLOAD_MEDIA_CLASS  1

#ifdef SNVX_CSIO_USLEEP
#include <sys/unistd.h>
#endif

GST_DEBUG_CATEGORY_STATIC (debug_category);
#define GST_CAT_DEFAULT debug_category
#define GST_CAT_GSTSERVER "gstserver"

static int appDebugLevel = eLogLevel_verbose;
int GstAppServer::m_client_count = 0;
int GstAppServer::m_media_status = 0;

int gst_server_init()
{
    GST_DEBUG_CATEGORY_INIT (debug_category, GST_CAT_GSTSERVER, 0,
                             "Android csio");
    gst_debug_set_threshold_for_name (GST_CAT_GSTSERVER, GST_LEVEL_DEBUG);

    return 0;
}

static bool gst_rtsp_server_get_pipeline(char * pDest, int destSize)
{
    FILE * pf;

    pf = fopen("/data/app/rtsp_server_pipeline", "r");
    GST_DEBUG("server_get_pipeline: fopen 0x%x\n", pf);
    if(pf)
    {
        fgets(pDest, destSize, pf);
        fclose(pf);

        GST_DEBUG("server_get_pipeline: fopen %s\n", pDest);
        return true;
    }

    return false;
}
/**********************GstAppServer class static functions***************************************/
/* static function, called when a new media pipeline is constructed. We can query the
 * pipeline and configure our ourappsrc
 * */
void GstAppServer::media_configure_cb(GstRTSPMediaFactory * factory, GstRTSPMedia * media,
        gpointer user_data)
{
    GST_DEBUG("media_configure_cb: set media reusable to true media");
    gst_rtsp_media_set_reusable(media, true);

    //get media unprepared signal, only for debugging
    //g_signal_connect (media, "unprepared", (GCallback)GstAppServer::media_unprepared, NULL);
}

GstRTSPFilterResult GstAppServer::filter_cb(GstRTSPStream *stream,
        GstRTSPStreamTransport *trans, gpointer user_data)
{
    GST_DEBUG("filter_cb: filter_cb on stream[0x%x]",stream);
    return GST_RTSP_FILTER_REMOVE;
}

void GstAppServer::client_connected_new_client_cb (GstRTSPServer * server,
    GstRTSPClient * client, gpointer user_data)
{
    GST_DEBUG("client_connected_new_client_cb: new_client -[0x%x]\n",client);
    // g_signal_connect (client, "setup-request", (GCallback)GstAppServer::setup_request_from_client, NULL);
    // g_signal_connect (client, "play-request", (GCallback)GstAppServer::play_request_from_client, NULL);
    // g_signal_connect (client, "closed", (GCallback)GstAppServer::closed_from_client, NULL);

    GstAppServer::m_client_count++;
    GST_DEBUG("client_connected_new_client_cb: m_client_count -[%d]\n",GstAppServer::m_client_count);
}

// void GstAppServer::setup_request_from_client (GstRTSPClient * client,
//         GstRTSPContext * ctx, gpointer user_data)
// {
//     GstRTSPStream *stream;
//     GstRTSPTransport *tr;
//     GstRTSPStreamTransport * trans = ctx->trans;
//     GstRTSPConnection* conn ;
//     char* remote_ip;

//     GST_DEBUG("CresRTSP_gstappserver: setup_request_from_client-[0x%x]\n",client);
//     if(trans)
//     {
//         tr = gst_rtsp_stream_transport_get_transport(trans);

//         if(tr)
//         {
//             uint32_t dstIp;
//             inet_pton(AF_INET, (char*)tr->destination, &dstIp);
// #ifdef USED_FOR_UNICAST
//             csio_JstrSetTxDestIpPort(eWindowId_0, htonl(dstIp), tr->port.min); // TODO: THIS SHOULD NOT BE HARDCODED TO 0!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//             csio_JstrFeedbackDstPort(eWindowId_0, tr->port.min);
// #endif
//             GST_DEBUG("CresRTSP_gstappserver: setup_request_from_client server_ip[0x%x], tr->server_port.min[%d]\n",
//         	         htonl(dstIp), tr->port.min);
//             GST_DEBUG("CresRTSP_gstappserver: setup_request_from_client client_port min[%d],man[%d]\n",
//                      tr->client_port.min,tr->client_port.max);
//             GST_DEBUG("CresRTSP_gstappserver: server_port min[%d],man[%d]\n",
//                      tr->server_port.min,tr->server_port.max);
//             GST_DEBUG("CresRTSP_gstappserver: destination[%s],source[%s]\n",
//                      tr->destination,tr->source);
//             GST_DEBUG("CresRTSP_gstappserver: multicast port[%d][%d],ttl[%d]\n",
//                      tr->port.min,tr->port.max,tr->ttl);
//             GST_DEBUG("CresRTSP_gstappserver: ctx->uri->host[%s],port[%d]\n",
//                     ctx->uri->host,ctx->uri->port);
//         }
//     }

//     if(client)
//     {
//         conn = gst_rtsp_client_get_connection(client);
//         if(conn)
//         {
//             remote_ip = gst_rtsp_connection_get_ip(conn);
//             if(remote_ip)
//                 GST_DEBUG("CresRTSP_gstappserver: setup_request_from_client remote_ip[%s]\n",remote_ip);
//         }
//     }
//     return;
// }
// void GstAppServer::play_request_from_client (GstRTSPClient * client,
//         GstRTSPContext * ctx, gpointer user_data)
// {
//     GST_DEBUG("CresRTSP_gstappserver: play_request_from_client-[0x%x]\n",client);

//     return;
// }
// void GstAppServer::closed_from_client (GstRTSPClient * client,gpointer user_data)
// {
//     GST_DEBUG("CresRTSP_gstappserver: closed_from_client-[0x%x]\n",client);

//     GstAppServer::m_client_count--;
//     GST_DEBUG("GstAppServer::closed_from_client: m_client_count -[%d]\n",GstAppServer::m_client_count);

//     return;
// }
void GstAppServer::media_unprepared (GstRTSPMedia * media, gpointer user_data)
{
    GstAppServer::m_media_status = 0;
    GST_DEBUG("media_unprepared-[0x%x]\n",media);

    return;
}
/**********************GstAppServer class implementation***************************************/
GstAppServer::GstAppServer():
m_parent(NULL),m_loop(NULL),m_factory(NULL),m_pMedia(NULL)
{
    mLock            = new Mutex();

    if(!mLock)
        GST_DEBUG("GstAppServer: CCresRTSPServer malloc failed:[0x%x]\n",\
                 mLock);

    memset(m_rtsp_port,0,sizeof(m_rtsp_port));
    memset(m_client_ip_addr,0,sizeof(m_client_ip_addr));

    GstAppServer::m_client_count = 0;
    GstAppServer::m_media_status = 0;
}

GstAppServer::~GstAppServer()
{
    if(mLock)
    {
        delete mLock;
        mLock = NULL;
    }

    GST_DEBUG("gstappserver: ~GstAppServer()\n");
}
void GstAppServer::DumpClassPara(int level)
{
    GST_DEBUG("gstappserver: ThredId 0x%x\n", (unsigned long int)getThredId());
    // GST_DEBUG("-----CresRTSP_gstappserver: m_debugLevel %d\n", m_debugLevel);

    GST_DEBUG("gstappserver: m_parent 0x%x\n", m_parent);

    GST_DEBUG("gstappserver: m_rtsp_port %s\n", m_rtsp_port);
}
void GstAppServer::setDebugLevel(int level)
{
    m_debugLevel  = level;
    appDebugLevel = level;
}
//overloaded from base
void GstAppServer::exitThread()
{
    GST_DEBUG("exitThread: try to quit g_main_loop[0x%x]\n",m_loop);

    m_forceThreadExit = 1;

    lock_gst_app();

    if(m_loop)
    {
        //if(m_factory)
        //    gst_rtsp_media_factory_set_eos_shutdown (m_factory, TRUE);

        g_main_loop_quit(m_loop);
        GST_DEBUG("exitThread: g_main_loop_quit returned\n");
    }
    else
    {
        GST_DEBUG("exitThread: g_main_loop is not running\n");
    }

    unlock_gst_app();
}
void* GstAppServer::ThreadEntry()
{
    char pipeline[1024];
    guint server_id;
    GMainContext*        context = NULL;
    GstRTSPServer *      server  = NULL;
    GstRTSPMountPoints * mounts  = NULL;
    GSource *  server_source     = NULL;
    GstRTSPThreadPool* server_threadPool  = NULL;
    GError    *err = NULL;

    m_factory = NULL;

    if(m_forceThreadExit)
        goto exitThread;

    lock_gst_app();

    //create new context
    context = g_main_context_new ();
    g_main_context_push_thread_default(context);
    GST_DEBUG( "gstappserver: creste new context: 0x%x\n", context );
    if(!context)
    {
        GST_DEBUG("gstappserver: Failed to create rtsp server context\n");
        unlock_gst_app();
        goto exitThread;
    }

    m_loop = g_main_loop_new (context, FALSE);
    if(!m_loop)
    {
        GST_DEBUG("gstappserver: Failed to create rtsp server loop\n");
        unlock_gst_app();
        goto exitThread;
    }

    server = gst_rtsp_server_new();
    if(!server)
    {
        GST_DEBUG("gstappserver: Failed to create rtsp server\n");
        unlock_gst_app();
        goto exitThread;
    }

    //setup listening port
    //GST_DEBUG("gstappserver: set_service to port:%s\n",m_rtsp_port);

    //gst_rtsp_server_set_address(server,"127.0.0.1");
    //gst_rtsp_server_set_service (server, m_rtsp_port);

    /* get the mount points for this server, every server has a default object
    * that be used to map uri mount points to media factories */
    mounts = gst_rtsp_server_get_mount_points (server);
    if(!mounts)
    {
        GST_DEBUG("gstappserver: Failed to create server mounts\n");
        unlock_gst_app();
        goto exitThread;
    }

    m_factory = gst_rtsp_media_factory_new ();
    if(!m_factory)
    {
        GST_DEBUG("gstappserver: Failed to create factory\n");
        unlock_gst_app();
        goto exitThread;
    }
    gst_rtsp_media_factory_set_launch (m_factory, "( "
        "videotestsrc ! video/x-raw,width=352,height=288,framerate=15/1 ! "
        "x264enc ! rtph264pay name=pay0 pt=96 "
        "audiotestsrc ! audio/x-raw,rate=8000 ! "
        "alawenc ! rtppcmapay name=pay1 pt=97 " ")");
    
    // gst_rtsp_media_factory_set_launch (m_factory, 
    //     "( "
    //     "videotestsrc ! x264enc ! rtph264pay name=pay0 pt=96 "
    //     ")"   );

//    gst_rtsp_media_factory_set_launch (m_factory, "( "
//        "audiotestsrc ! audio/x-raw,rate=8000 ! "
//        "alawenc ! rtppcmapay name=pay0 pt=97 " ")");

    gst_rtsp_media_factory_set_shared (m_factory, TRUE);
    //gst_rtsp_media_factory_set_latency (m_factory, 10);

    //if(gst_rtsp_server_get_pipeline(pipeline, sizeof(pipeline)) == false)
    //{
    //    snprintf(pipeline, sizeof(pipeline), "appsrc  name=mysrc is-live=true  ! "
    //                                                 "queue ! rtph264pay pt=33 name=pay0 ");
    //}
    //GST_DEBUG("CresRTSP_gstappserver: rtsp server pipeline: [%s]\n", pipeline);
    //gst_rtsp_media_factory_set_launch (m_factory, pipeline);

    /* notify when our media is ready, This is called whenever someone asks for
       * the media and a new pipeline with our appsrc is created */
    g_signal_connect (m_factory, "media-configure", (GCallback) media_configure_cb,this);

    g_signal_connect (server, "client-connected",
                      G_CALLBACK (client_connected_new_client_cb), NULL);

    gst_rtsp_mount_points_add_factory (mounts, "/test", m_factory);
    g_object_unref (mounts);

//correct way to create source and attatch to mainloop
    server_source = gst_rtsp_server_create_source(server,NULL,&err);
    if(server_source)
    {
        GST_DEBUG("gstappserver: create_source , server_source [0x%x]\n", server_source);
    }
    else
    {
        GST_DEBUG("gstappserver: Failed to create_source(err:0x%x)\n",err);
        if(err)
        {
            g_error_free (err);
            err = NULL;
        }//else

        unlock_gst_app();
        goto exitThread;
    }
    server_id = g_source_attach (server_source, context);

    if(server_id)
    {
        GST_DEBUG("gstappserver: Attached server to maincontext, server_id %u\n", server_id);
    }
    else
    {
        GST_DEBUG("gstappserver: Failed to attach server\n");
        unlock_gst_app();
        goto exitThread;
    }

    GST_DEBUG("gstappserver: call g_main_loop_run,stream at rtsp://localhost:8554/test, m_forceThreadExit[%d]\n",
             m_forceThreadExit);

    unlock_gst_app();

    if(m_forceThreadExit)
    {
        goto exitThread;
    }
    else
    {
        g_main_loop_run (m_loop);
        GST_DEBUG("gstappserver: g_main_loop_run returned\n");
    }

exitThread:
    /* cleanup */

#ifdef CRES_UNPREPARE_MEDIA
/*   please check out this bug: https://bugzilla.gnome.org/show_bug.cgi?id=747801
 *   if it is fixed, we should take it. */

    if(m_pMedia)
    {
        gst_rtsp_media_suspend (m_pMedia);

        //remove stream from session before unprepare media
        guint i, n_streams;
        n_streams = gst_rtsp_media_n_streams (m_pMedia);
        GST_DEBUG("CresRTSP_gstappserver: -------media status[%d], n_streams[%d]---\n",
                gst_rtsp_media_get_status(m_pMedia),
                n_streams);

        for (i = 0; i < n_streams; i++)
        {
            GstRTSPStream *stream = gst_rtsp_media_get_stream (m_pMedia, i);            

            if (stream == NULL)  continue;

            gst_rtsp_stream_transport_filter (stream, filter_cb, NULL);
        }

        //Note: 3-11-2020 https://github.com/pexip/gst-rtsp-server/commit/ba5b78ff2ff223049188eb456e228c709ccd3e05
        //      Calling gst_rtsp_media_unprepare breaks shared medias. gst_rtsp_media_finalize will
        //      call gst_rtsp_media_unprepare when a media isn't being used anymore, otherwise it will crash.
        //gst_rtsp_media_unprepare (m_pMedia);
        for(i = 0; i < 200; i++)//20s timeout
        {
            //waiting for all clients to be disconnected,
            //this is more important than unprepared signal.
            if(GstAppServer::m_client_count > 0)
            {
                usleep(100000);
                STRLOG(LOGLEV_EXTRAVERBOSE,LOGCAT_GSRTSPDEFAULT, "CresRTSP_gstappserver: wait until clients are all disconnected[%d][%d][%d].\n",
                        i,GstAppServer::m_client_count,GstAppServer::m_media_status);
            }
            else
            {
                break;
            }
        }

        GST_DEBUG("CresRTSP_gstappserver: i[%d], m_client_count[%d],m_media_status[%d]\n",
                 i,GstAppServer::m_client_count,GstAppServer::m_media_status);
     }
#endif

/* You must use g_source_destroy() for sources added to a non-default main context.  */
    if(server_source)
    {
        /*You must use g_source_destroy() for sources added to a non-default main context.*/
        g_source_destroy (server_source);
        g_source_unref(server_source);
        GST_DEBUG("gstappserver: g_source_destroy server_source[0x%x]\n",server_source);
    }

    /*Make sure you send a EOS first (if you have a queue in a pipeline have
flush-on-eos=true),
catch it on the bus handler then later do a mail_loop quit unref that object,
remove the factory (gst_rtsp_mount_points_remove_factory ()), unref
the mounts object,
unref the factory object and lastly unref the server object. */

    lock_gst_app();

    if(m_loop) g_main_loop_unref (m_loop);

    //Note:  if you unref m_factory, then unref server will give you and err
    //       seems unref server is enough, it will unref m_factory also.
    //if(m_factory) g_object_unref (m_factory);
    if(server) g_object_unref (server);

    //need to create a cleanup function and call here
    m_loop = NULL;
    m_factory = NULL;

    if(context)
    {
        g_main_context_pop_thread_default(context);
        g_main_context_unref (context);
        context = NULL;
    }

    GST_DEBUG("gstappserver: rtsp_server ended------\n");

    //thread exit here
    m_ThreadIsRunning = 0;

    unlock_gst_app();

    return NULL;
}

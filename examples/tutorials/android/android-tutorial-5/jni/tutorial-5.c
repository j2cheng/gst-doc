#include <string.h>
#include <stdint.h>
#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/videooverlay.h>
#include <pthread.h>

#include "csio/csio.h"

//Crestron change starts
// Android headers
//#include "hardware/gralloc.h"           // for GRALLOC_USAGE_PROTECTED
//#include "android/native_window.h"      // for ANativeWindow_ functions
//
//#include <gui/GLConsumer.h>
//#include <gui/IProducerListener.h>
//#include <gui/Surface.h>
//#include <gui/SurfaceComposerClient.h>
//
//#include <gui/ISurfaceComposer.h>
//
//
//#include <ui/DisplayInfo.h>
//Crestron change ends

GST_DEBUG_CATEGORY_STATIC (debug_category);
#define GST_CAT_DEFAULT debug_category
// #define USE_PLAYBIN 1

/*
 * These macros provide a way to store the native pointer to CustomData, which might be 32 or 64 bits, into
 * a jlong, which is always 64 bits, without warnings.
 */
#if GLIB_SIZEOF_VOID_P == 8
# define GET_CUSTOM_DATA(env, thiz, fieldID) (CustomData *)(*env)->GetLongField (env, thiz, fieldID)
# define SET_CUSTOM_DATA(env, thiz, fieldID, data) (*env)->SetLongField (env, thiz, fieldID, (jlong)data)
#else
# define GET_CUSTOM_DATA(env, thiz, fieldID) (CustomData *)(jint)(*env)->GetLongField (env, thiz, fieldID)
# define SET_CUSTOM_DATA(env, thiz, fieldID, data) (*env)->SetLongField (env, thiz, fieldID, (jlong)(jint)data)
#endif

/* Do not allow seeks to be performed closer than this distance. It is visually useless, and will probably
 * confuse some demuxers. */
#define SEEK_MIN_DELAY (500 * GST_MSECOND)

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData
{
  jobject app;                  /* Application instance, used to call its methods. A global reference is kept. */
  GstElement *pipeline;         /* The running pipeline */
  GMainContext *context;        /* GLib context used to run the main loop */
  GMainLoop *main_loop;         /* GLib main loop */
  gboolean initialized;         /* To avoid informing the UI multiple times about the initialization */
  ANativeWindow *native_window; /* The Android native window where video will be rendered */
  GstState state;               /* Current pipeline state */
  GstState target_state;        /* Desired pipeline state, to be set once buffering is complete */
  gint64 duration;              /* Cached clip duration */
  gint64 desired_position;      /* Position to seek to, once the pipeline is running */
  GstClockTime last_seek_time;  /* For seeking overflow prevention (throttling) */
  gboolean is_live;             /* Live streams do not use buffering */
  gchar* pipeline_string;       /* built pipeline by the string.
                                 * eg.: videotestsrc ! video/x-raw,format=YUY2 ! videoconvert ! glimagesink
                                 *      videotestsrc ! video/x-raw,width=1080,height=720 ! autovideosink
                                 *      rtspsrc location=rtsp://170.93.143.139/rtplive/e40037d1c47601b8004606363d235daa ! rtph264depay ! decodebin ! videoconvert ! autovideosink */
  GstElement *video_sink;       /* The video sink from the pipeline */                                 
} CustomData;

/* playbin2 flags */
typedef enum
{
  GST_PLAY_FLAG_TEXT = (1 << 2) /* We want subtitle output */
} GstPlayFlags;

/* These global variables cache values which are not changing during execution */
static pthread_t gst_app_thread;
static pthread_key_t current_jni_env;
static JavaVM *java_vm;
static jfieldID custom_data_field_id;
static jmethodID set_message_method_id;
static jmethodID set_current_position_method_id;
static jmethodID on_gstreamer_initialized_method_id;
static jmethodID on_media_size_changed_method_id;

/*
 * Private methods
 */

/* Register this thread with the VM */
static JNIEnv *
attach_current_thread (void)
{
  JNIEnv *env;
  JavaVMAttachArgs args;

  GST_DEBUG ("Attaching thread %p", g_thread_self ());
  args.version = JNI_VERSION_1_4;
  args.name = NULL;
  args.group = NULL;

  if ((*java_vm)->AttachCurrentThread (java_vm, &env, &args) < 0) {
    GST_ERROR ("Failed to attach current thread");
    return NULL;
  }

  return env;
}

/* Unregister this thread from the VM */
static void
detach_current_thread (void *env)
{
  GST_DEBUG ("Detaching thread %p", g_thread_self ());
  (*java_vm)->DetachCurrentThread (java_vm);
}

/* Retrieve the JNI environment for this thread */
static JNIEnv *
get_jni_env (void)
{
  JNIEnv *env;

  if ((env = pthread_getspecific (current_jni_env)) == NULL) {
    env = attach_current_thread ();
    pthread_setspecific (current_jni_env, env);
  }

  return env;
}

/* Change the content of the UI's TextView */
static void
set_ui_message (const gchar * message, CustomData * data)
{
  JNIEnv *env = get_jni_env ();
  GST_DEBUG ("Setting message to: %s", message);
  jstring jmessage = (*env)->NewStringUTF (env, message);
  (*env)->CallVoidMethod (env, data->app, set_message_method_id, jmessage);
  if ((*env)->ExceptionCheck (env)) {
    GST_ERROR ("Failed to call Java method");
    (*env)->ExceptionClear (env);
  }
  (*env)->DeleteLocalRef (env, jmessage);
}

/* Tell the application what is the current position and clip duration */
static void
set_current_ui_position (gint position, gint duration, CustomData * data)
{
  JNIEnv *env = get_jni_env ();
  (*env)->CallVoidMethod (env, data->app, set_current_position_method_id,
      position, duration);
  if ((*env)->ExceptionCheck (env)) {
    GST_ERROR ("Failed to call Java method");
    (*env)->ExceptionClear (env);
  }
}

/* If we have pipeline and it is running, query the current position and clip duration and inform
 * the application */
static gboolean
refresh_ui (CustomData * data)
{
  gint64 current = -1;
  gint64 position;

  /* We do not want to update anything unless we have a working pipeline in the PAUSED or PLAYING state */
  if (!data || !data->pipeline || data->state < GST_STATE_PAUSED)
    return TRUE;

  /* If we didn't know it yet, query the stream duration */
  if (!GST_CLOCK_TIME_IS_VALID (data->duration)) {
    if (!gst_element_query_duration (data->pipeline, GST_FORMAT_TIME,
            &data->duration)) {
      GST_WARNING
          ("Could not query current duration (normal for still pictures)");
      data->duration = 0;
    }
  }

  if (!gst_element_query_position (data->pipeline, GST_FORMAT_TIME, &position)) {
    GST_WARNING
        ("Could not query current position (normal for still pictures)");
    position = 0;
  }

  /* Java expects these values in milliseconds, and GStreamer provides nanoseconds */
  set_current_ui_position (position / GST_MSECOND, data->duration / GST_MSECOND,
      data);
  return TRUE;
}

/* Forward declaration for the delayed seek callback */
static gboolean delayed_seek_cb (CustomData * data);

/* Perform seek, if we are not too close to the previous seek. Otherwise, schedule the seek for
 * some time in the future. */
static void
execute_seek (gint64 desired_position, CustomData * data)
{
  gint64 diff;

  if (desired_position == GST_CLOCK_TIME_NONE)
    return;

  diff = gst_util_get_timestamp () - data->last_seek_time;

  if (GST_CLOCK_TIME_IS_VALID (data->last_seek_time) && diff < SEEK_MIN_DELAY) {
    /* The previous seek was too close, delay this one */
    GSource *timeout_source;

    if (data->desired_position == GST_CLOCK_TIME_NONE) {
      /* There was no previous seek scheduled. Setup a timer for some time in the future */
      timeout_source =
          g_timeout_source_new ((SEEK_MIN_DELAY - diff) / GST_MSECOND);
      g_source_set_callback (timeout_source, (GSourceFunc) delayed_seek_cb,
          data, NULL);
      g_source_attach (timeout_source, data->context);
      g_source_unref (timeout_source);
    }
    /* Update the desired seek position. If multiple petitions are received before it is time
     * to perform a seek, only the last one is remembered. */
    data->desired_position = desired_position;
    GST_DEBUG ("Throttling seek to %" GST_TIME_FORMAT ", will be in %"
        GST_TIME_FORMAT, GST_TIME_ARGS (desired_position),
        GST_TIME_ARGS (SEEK_MIN_DELAY - diff));
  } else {
    /* Perform the seek now */
    GST_DEBUG ("Seeking to %" GST_TIME_FORMAT,
        GST_TIME_ARGS (desired_position));
    data->last_seek_time = gst_util_get_timestamp ();
    gst_element_seek_simple (data->pipeline, GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, desired_position);
    data->desired_position = GST_CLOCK_TIME_NONE;
  }
}

/* Delayed seek callback. This gets called by the timer setup in the above function. */
static gboolean
delayed_seek_cb (CustomData * data)
{
  GST_DEBUG ("Doing delayed seek to %" GST_TIME_FORMAT,
      GST_TIME_ARGS (data->desired_position));
  execute_seek (data->desired_position, data);
  return FALSE;
}

/* Retrieve errors from the bus and show them on the UI */
static void
error_cb (GstBus * bus, GstMessage * msg, CustomData * data)
{
  GError *err;
  gchar *debug_info;
  gchar *message_string;

  gst_message_parse_error (msg, &err, &debug_info);
  message_string =
      g_strdup_printf ("Error received from element %s: %s",
      GST_OBJECT_NAME (msg->src), err->message);
  g_clear_error (&err);
  g_free (debug_info);
  set_ui_message (message_string, data);
  g_free (message_string);
  data->target_state = GST_STATE_NULL;
  gst_element_set_state (data->pipeline, GST_STATE_NULL);
}

/* Called when the End Of the Stream is reached. Just move to the beginning of the media and pause. */
static void
eos_cb (GstBus * bus, GstMessage * msg, CustomData * data)
{
  data->target_state = GST_STATE_PAUSED;
  data->is_live |=
      (gst_element_set_state (data->pipeline,
          GST_STATE_PAUSED) == GST_STATE_CHANGE_NO_PREROLL);
  execute_seek (0, data);
}

/* Called when the duration of the media changes. Just mark it as unknown, so we re-query it in the next UI refresh. */
static void
duration_cb (GstBus * bus, GstMessage * msg, CustomData * data)
{
  data->duration = GST_CLOCK_TIME_NONE;
}

static void
element_cb (GstBus * bus, GstMessage * msg, CustomData * data)
{
  GST_ERROR("element_cb from element., tartget state[%s]",gst_element_state_get_name(data->target_state));

  if(GST_IS_ELEMENT(GST_MESSAGE_SRC(msg)))
  {
    GstObject *obj = GST_MESSAGE_SRC(msg);
    // g_strdup_printf ("element_cb from element %s",
    //                  GST_OBJECT_NAME (msg->src));

    GST_ERROR("%s: obj[%s]",__FUNCTION__,GST_OBJECT_NAME(obj));
    GST_ERROR("%s: g_type_name[%s]",__FUNCTION__, g_type_name(G_OBJECT_TYPE(obj)));
    GST_ERROR("%s: gst_plugin_feature_get_name %s",__FUNCTION__,
              gst_plugin_feature_get_name(GST_ELEMENT_GET_CLASS(obj)->elementfactory) );                     

    GST_ERROR("element_cb from element %s,videsink[0x%x]",GST_OBJECT_NAME (msg->src),data->video_sink);



    data->video_sink = gst_bin_get_by_interface(GST_BIN(data->pipeline), GST_TYPE_VIDEO_OVERLAY);

    if(data->video_sink)
    {
      GST_DEBUG ("looking for video sink: 0x%x",data->video_sink);

#if USE_PLAYBIN
      GST_DEBUG ("skipp overlay_set_window");
#else
      GST_DEBUG ("calling overlay_set_window");
      gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY(data->video_sink), (guintptr)data->native_window);
#endif
      GST_DEBUG ("video_overlay_set_window to video sink: 0x%x",data->video_sink);
    }
    else
    {
      GST_DEBUG ("failed to get video sink!!!");
    }



  }
}
/* Called when buffering messages are received. We inform the UI about the current buffering level and
 * keep the pipeline paused until 100% buffering is reached. At that point, set the desired state. */
static void
buffering_cb (GstBus * bus, GstMessage * msg, CustomData * data)
{
  gint percent;

  GST_ERROR("buffering_cb data->is_live %d, tartget state[%s]",data->is_live,gst_element_state_get_name(data->target_state));

  if (data->is_live)
    return;

  gst_message_parse_buffering (msg, &percent);
  GST_ERROR("buffering_cb percent %d",percent);

  if (percent < 100 && data->target_state >= GST_STATE_PAUSED) {
    gchar *message_string = g_strdup_printf ("Buffering %d%%", percent);
    gst_element_set_state (data->pipeline, GST_STATE_PAUSED);
    set_ui_message (message_string, data);
    g_free (message_string);
  } else if (data->target_state >= GST_STATE_PLAYING) {
    gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
  } else if (data->target_state >= GST_STATE_PAUSED) {
    set_ui_message ("Buffering complete", data);
  }
}

/* Called when the clock is lost */
static void
clock_lost_cb (GstBus * bus, GstMessage * msg, CustomData * data)
{
  if (data->target_state >= GST_STATE_PLAYING) {
    gst_element_set_state (data->pipeline, GST_STATE_PAUSED);
    gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
  }
}

/* Retrieve the video sink's Caps and tell the application about the media size */
static void
check_media_size (CustomData * data)
{
  JNIEnv *env = get_jni_env ();
  GstElement *video_sink;
  GstPad *video_sink_pad;
  GstCaps *caps;
  GstVideoInfo info;

  GST_DEBUG ("check_media_size enter");

#if USE_PLAYBIN
  /* Retrieve the Caps at the entrance of the video sink */
  GST_DEBUG ("Retrieve the video sink from playbin");
  g_object_get (data->pipeline, "video-sink", &video_sink, NULL);
  video_sink_pad = gst_element_get_static_pad (video_sink, "sink");
  caps = gst_pad_get_current_caps (video_sink_pad);

  if (gst_video_info_from_caps (&info, caps)) {
    info.width = info.width * info.par_n / info.par_d;
    GST_DEBUG ("Media size is %dx%d, notifying application", info.width,
        info.height);

    (*env)->CallVoidMethod (env, data->app, on_media_size_changed_method_id,
        (jint) info.width, (jint) info.height);
    if ((*env)->ExceptionCheck (env)) {
      GST_ERROR ("Failed to call Java method");
      (*env)->ExceptionClear (env);
    }
  }

  gst_caps_unref (caps);
  gst_object_unref (video_sink_pad);
  gst_object_unref (video_sink);
#else
  /* Retrieve the Caps at the entrance of the video sink */
  // g_object_get (data->pipeline, "video-sink", &video_sink, NULL);
  video_sink = data->video_sink;

  GST_DEBUG ("Should have video_sink[0x%x]",video_sink);

  video_sink_pad = gst_element_get_static_pad (video_sink, "sink");
  caps = gst_pad_get_current_caps (video_sink_pad);

  if (gst_video_info_from_caps (&info, caps)) {
    info.width = info.width * info.par_n / info.par_d;
    GST_DEBUG ("Media size is %dx%d, notifying application", info.width,
        info.height);

    (*env)->CallVoidMethod (env, data->app, on_media_size_changed_method_id,
        (jint) info.width, (jint) info.height);
    if ((*env)->ExceptionCheck (env)) {
      GST_ERROR ("Failed to call Java method");
      (*env)->ExceptionClear (env);
    }
  }

  gst_caps_unref (caps);
  gst_object_unref (video_sink_pad);
  gst_object_unref (video_sink);

#endif
GST_DEBUG ("check_media_size exit");

}


static void cres_add_graph (CustomData *data)
{
  //Crestron change starts
  // CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
  if (!data)
    return;

  GST_DEBUG ("cres_add_graph /data/data/org.freedesktop.gstreamer.tutorials.tutorial_5/graph");

  /**
   *  set path GST_DEBUG_DUMP_DOT_DIR to /data/app/gst-graph
   */
  if(data->pipeline)
  {
    gchar *full_file_name = NULL;
    FILE *out;
    GstBin * bin = data->pipeline;

    //Note: this app has its own space :  /data/data/org.freedesktop.gstreamer.tutorials.tutorial_5
    //      and lib file is installed in: /data/app/org.freedesktop.gstreamer.tutorials.tutorial_5-2W78GrP_rHsTe_bHsPvJZg==
    GstDebugGraphDetails details = GST_DEBUG_GRAPH_SHOW_ALL;
    full_file_name = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "%s.dot",
            "/data/data/org.freedesktop.gstreamer.tutorials.tutorial_5/graph", "pipeline");
    if ((out = fopen (full_file_name, "wb")))
    {
      gchar *buf;

      buf = gst_debug_bin_to_dot_data (bin, details);
      fputs (buf, out);

      g_free (buf);
      fclose (out);

      GST_INFO ("wrote bin graph to : '%s'", full_file_name);
    }
    else
    {
      GST_WARNING ("Failed to open file '%s' for writing: %s", full_file_name,
                   g_strerror (errno));
    }
    g_free (full_file_name);
  }
  //Crestron change ends
}


/* Notify UI about pipeline state changes */
static void
state_changed_cb (GstBus * bus, GstMessage * msg, CustomData * data)
{
  GstState old_state, new_state, pending_state;
  gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
  /* Only pay attention to messages coming from the pipeline, not its children */
  if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->pipeline)) {
    data->state = new_state;
    gchar *message = g_strdup_printf ("State changed to %s",
        gst_element_state_get_name (new_state));
    set_ui_message (message, data);
    g_free (message);

    if (new_state == GST_STATE_NULL || new_state == GST_STATE_READY)
      data->is_live = FALSE;

    /* The Ready to Paused state change is particularly interesting: */
    if (old_state == GST_STATE_READY && new_state == GST_STATE_PAUSED) {
      /* By now the sink already knows the media size */
      check_media_size (data);

      /* If there was a scheduled seek, perform it now that we have moved to the Paused state */
      if (GST_CLOCK_TIME_IS_VALID (data->desired_position))
        execute_seek (data->desired_position, data);
    }

    if(new_state == GST_STATE_PLAYING)
    {
      GST_DEBUG("new_state is in GST_STATE_PLAYING");
      cres_add_graph(data);
    }
  }
}

/* Check if all conditions are met to report GStreamer as initialized.
 * These conditions will change depending on the application */
static void
check_initialization_complete (CustomData * data)
{
  JNIEnv *env = get_jni_env ();
  if (!data->initialized && data->native_window && data->main_loop) {
    GST_DEBUG
        ("Initialization complete, notifying application. native_window:%p main_loop:%p",
        data->native_window, data->main_loop);

#if USE_PLAYBIN
    GST_DEBUG("calling video_overlay_set_window here.");
    /* The main loop is running and we received a native window, inform the sink about it */
    gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (data->pipeline),
        (guintptr) data->native_window);
#else
    GST_DEBUG ("check_initialization_complete video sink: %p, window: %p",data->video_sink,(guintptr)data->native_window);

    if(data->video_sink && data->native_window)
    {
      GST_DEBUG ("calling overlay_set_window: native_window: %p",data->native_window);
      gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY(data->video_sink), (guintptr)data->native_window);

      GST_DEBUG ("video_overlay_set_window to video sink: %p, window: %p",data->video_sink,(guintptr)data->native_window);
    }
    else
    {
      GST_DEBUG ("failed to get video sink!!!");
    }
#endif

    (*env)->CallVoidMethod (env, data->app, on_gstreamer_initialized_method_id);
    if ((*env)->ExceptionCheck (env)) {
      GST_ERROR ("Failed to call Java method");
      (*env)->ExceptionClear (env);
    }
    data->initialized = TRUE;
  }
}

gboolean csio_GstMsgHandler(GstBus *bus, GstMessage *msg, void *arg)
{
  CustomData *data = (CustomData *) arg;
  GError    *err;
  gchar     *debug_info = NULL;
  gint      percent;

  if(GST_IS_ELEMENT(GST_MESSAGE_SRC(msg)))
  {
    GstObject *obj = GST_MESSAGE_SRC(msg);
    GST_DEBUG("%s: obj[%s]",__FUNCTION__,GST_OBJECT_NAME(obj));
    GST_DEBUG("%s: g_type_name[%s]",__FUNCTION__, g_type_name(G_OBJECT_TYPE(obj)));
    GST_DEBUG("%s: gst_plugin_feature_get_name %s",__FUNCTION__,
             gst_plugin_feature_get_name(GST_ELEMENT_GET_CLASS(obj)->elementfactory) );

    if(!data->video_sink)
    {
      data->video_sink = gst_bin_get_by_interface(GST_BIN(data->pipeline), GST_TYPE_VIDEO_OVERLAY);
      GST_DEBUG ("looking for video sink: %p,native_window: %p",data->video_sink,data->native_window);
      
      if(data->video_sink && data->native_window)
      {       

#if USE_PLAYBIN
        GST_DEBUG ("skipp overlay_set_window");
#else
        GST_DEBUG ("calling overlay_set_window: native_window: %p",data->native_window);
        gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY(data->video_sink), (guintptr)data->native_window);
#endif
        GST_DEBUG ("video_overlay_set_window to video sink: %p, window: %p",data->video_sink,(guintptr)data->native_window);
      }
      else
      {
        GST_DEBUG ("failed to get video sink!!!");
      }
    }

  }

  switch (GST_MESSAGE_TYPE(msg))
  {
    case GST_MESSAGE_NEW_CLOCK:
    {
        GstClock *clock;

        gst_message_parse_new_clock (msg, &clock);

        GST_DEBUG("New clock: %s\n", (clock ? GST_OBJECT_NAME (clock) : "NULL"));
        break;
    }
    case GST_MESSAGE_CLOCK_LOST:
    {
      GST_DEBUG("Clock lost, selecting a new one\n");    
      if (data->target_state >= GST_STATE_PLAYING) 
      {
        gst_element_set_state (data->pipeline, GST_STATE_PAUSED);
        gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
      }     
      break;
    }
    case GST_MESSAGE_WARNING:
    {
      gst_message_parse_warning( msg, &err, &debug_info );
      if(err->message)
      {
          GST_DEBUG("%s: WARNING MESSAGE: %s", __FUNCTION__, err->message);
      }

      if( debug_info )
      {
        GST_DEBUG("%s: debug_info: %s", __FUNCTION__, debug_info);
               
        g_free( debug_info );
      }

      g_clear_error( &err );
      break;
    }
    case GST_MESSAGE_ERROR:
    {
      gst_message_parse_error( msg, &err, &debug_info );
      GST_DEBUG("%s: err->code: %d", __FUNCTION__, err->code);
      if( debug_info )
      {
        GST_DEBUG("%s: debug_info: %s", __FUNCTION__, debug_info);

        g_free( debug_info );
      }

      g_clear_error( &err );
      break;
    }
    case GST_MESSAGE_EOS:
    {
      GST_DEBUG("%s: GST_MESSAGE_EOS", __FUNCTION__);
      break;
    }
    case GST_MESSAGE_STATE_CHANGED:
    {
      if( GST_MESSAGE_SRC(msg) == GST_OBJECT( data->pipeline) )
      {
        GstState old_state, new_state, pending_state;
        gst_message_parse_state_changed( msg, &old_state, &new_state,
                                          &pending_state );        
        
        data->state = new_state;

        gchar *message = g_strdup_printf ("State changed to %s",
        gst_element_state_get_name (new_state));
        set_ui_message (message, data);
        g_free (message);

        GST_DEBUG("Pipeline state changed from %s to %s:\n",
                  gst_element_state_get_name( old_state ),
                  gst_element_state_get_name( new_state) );

        if (new_state == GST_STATE_NULL || new_state == GST_STATE_READY)
          data->is_live = FALSE;

        /* The Ready to Paused state change is particularly interesting: */
        if (old_state == GST_STATE_READY && new_state == GST_STATE_PAUSED) {
          /* By now the sink already knows the media size */
          check_media_size (data);

          /* If there was a scheduled seek, perform it now that we have moved to the Paused state */
          if (GST_CLOCK_TIME_IS_VALID (data->desired_position))
            execute_seek (data->desired_position, data);
        }
        
        if(new_state == GST_STATE_PLAYING)
        {
          GST_DEBUG("new_state is in GST_STATE_PLAYING");
          cres_add_graph(data);
        }  
      }   
      break;
    }
    case GST_MESSAGE_BUFFERING:
    {
        gst_message_parse_buffering (msg, &percent);
        GST_DEBUG("buffer pct=%d\n", percent);   
        break;      
    }
    case GST_MESSAGE_QOS:
    {
        GST_DEBUG( "%s: GST_MESSAGE_QOS: SEQNUM: %d received %s from %s..\n",
                  __FUNCTION__,
                  GST_MESSAGE_SEQNUM(msg),
                  GST_MESSAGE_TYPE_NAME(msg),
                  GST_MESSAGE_SRC_NAME(msg));

        GstFormat format;
        guint64 processed;
        guint64 dropped;
        gst_message_parse_qos_stats(msg,&format,&processed,&dropped);
        GST_DEBUG("%s: GST_MESSAGE_QOS: format[%s]----- processed[%lld],dropped[%lld]",
                  __FUNCTION__,gst_format_get_name(format),processed,dropped);

        gint64 jitter;
        gdouble proportion;
        gint quality;
        gst_message_parse_qos_values(msg,&jitter,&proportion,&quality);
        GST_DEBUG("%s: GST_MESSAGE_QOS: jitter[%lld]----- processed[%f],quality[%d]",
                  __FUNCTION__,jitter,proportion,quality);

        gboolean live;
        guint64 running_time;
        guint64 stream_time;
        guint64 timestamp;
        guint64 duration;
        gst_message_parse_qos(msg,&live,&running_time,&stream_time,&timestamp,&duration);
        GST_DEBUG("%s: GST_MESSAGE_QOS: live stream[%d]----- running_time[%lld],stream_time[%lld],timestamp[%lld],duration[%ld]",
                  __FUNCTION__,live,running_time,stream_time,timestamp,duration);

        break;
    }
    case GST_MESSAGE_LATENCY:
    {
        GST_DEBUG("%s: GST_MESSAGE_LATENCY\n",__FUNCTION__);        
        break;
    }
  }
  GST_DEBUG("%s: exit GST_MESSAGE_TYPE name[%s]",__FUNCTION__,GST_MESSAGE_TYPE_NAME(msg));
  return TRUE;
}
/* Functions below print the Capabilities in a human-friendly format */
static gboolean print_field (GQuark field, const GValue * value, gpointer pfx) {
    gchar *str = gst_value_serialize (value);

    g_print ("%s  %15s: %s\n", (gchar *) pfx, g_quark_to_string (field), str);
    g_free (str);
    return TRUE;
}
static void print_caps (const GstCaps * caps, const gchar * pfx) {
    guint i;

    g_return_if_fail (caps != NULL);

    if (gst_caps_is_any (caps)) {
        g_print ("%sANY\n", pfx);
        return;
    }
    if (gst_caps_is_empty (caps)) {
        g_print ("%sEMPTY\n", pfx);
        return;
    }

    for (i = 0; i < gst_caps_get_size (caps); i++) {
        GstStructure *structure = gst_caps_get_structure (caps, i);

        g_print ("%s%s\n", pfx, gst_structure_get_name (structure));
        gst_structure_foreach (structure, print_field, (gpointer) pfx);
    }
}

/* Main method for the native code. This is executed on its own thread. */
#if USE_PLAYBIN
static void *
app_function (void *userdata)
{
  JavaVMAttachArgs args;
  GstBus *bus;
  CustomData *data = (CustomData *) userdata;
  GSource *timeout_source;
  GSource *bus_source;
  GError *error = NULL;
  guint flags;
  guint m_bus_id; //Crestron change

  GST_DEBUG ("Creating pipeline in CustomData at %p", data);

  //Crestron change starts
  if(1)
  {
    GstRegistry *registry = NULL;
    GstElementFactory *factory = NULL;

    factory = gst_element_factory_find ("amcviddec-omxqcomvideodecoderavc");
    GST_DEBUG ("Creating GST_ELEMENT_FACTORY amcviddec-omxqcomvideodecoderavc: %p", factory);

      GST_DEBUG ("Pad Templates for %s:\n", gst_element_factory_get_longname (factory));
    if (!gst_element_factory_get_num_pad_templates (factory)) {
      GST_DEBUG ("get_num_pad returns  none\n");
    }
    else
    {
      const GList *pads = gst_element_factory_get_static_pad_templates (factory);
      GstStaticPadTemplate *padtemplate;

        while (pads) {
            padtemplate = pads->data;
            pads = g_list_next (pads);

            if (padtemplate->direction == GST_PAD_SRC)
                GST_DEBUG ("  SRC template: '%s'\n", padtemplate->name_template);
            else if (padtemplate->direction == GST_PAD_SINK)
                GST_DEBUG ("  SINK template: '%s'\n", padtemplate->name_template);
            else
                GST_DEBUG ("  UNKNOWN!!! template: '%s'\n", padtemplate->name_template);

            if (padtemplate->presence == GST_PAD_ALWAYS)
                GST_DEBUG ("    Availability: Always\n");
            else if (padtemplate->presence == GST_PAD_SOMETIMES)
                GST_DEBUG ("    Availability: Sometimes\n");
            else if (padtemplate->presence == GST_PAD_REQUEST)
                GST_DEBUG ("    Availability: On request\n");
            else
                GST_DEBUG ("    Availability: UNKNOWN!!!\n");

            if (padtemplate->static_caps.string) {
                GstCaps *caps;
                GST_DEBUG ("    Capabilities:\n");
                caps = gst_static_caps_get (&padtemplate->static_caps);
                print_caps (caps, "      ");
                gst_caps_unref (caps);

            }

            GST_DEBUG ("\n");
        }


    }
  }


  if(0)
  {
        GstRegistry *registry = NULL;
        GstElementFactory *factory = NULL;

//        registry = gst_registry_get_default ();
          registry = gst_registry_get();
          if (!registry)
              GST_DEBUG ("Creating GST_ELEMENT_FACTORY registry: %p", registry);

        factory = gst_element_factory_find ("amcviddec-omxgooglehevcdecoder");
        GST_DEBUG ("Creating GST_ELEMENT_FACTORY amcviddec-omxgooglehevcdecoder: %p", factory);

        factory = gst_element_factory_find ("amcviddec-omxqcomvideodecoderavc");
        GST_DEBUG ("Creating GST_ELEMENT_FACTORY amcviddec-omxqcomvideodecoderavc: %p", factory);

        factory = gst_element_factory_find ("amcviddec-omxqcomvideodecoderhevc");
        if(factory && registry)
        {
            GST_DEBUG ("Creating GST_ELEMENT_FACTORY amcviddec-omxqcomvideodecoderhevc: %p", factory);

            //gst_plugin_feature_set_rank (GST_PLUGIN_FEATURE (factory), GST_RANK_PRIMARY + 1);
            //gst_registry_add_feature (registry, GST_PLUGIN_FEATURE (factory));
        }
  }
  //Crestron change ends

  /* Create our own GLib Main Context and make it the default one */
  data->context = g_main_context_new ();
  g_main_context_push_thread_default (data->context);

  /* Build pipeline */
  data->pipeline = gst_parse_launch ("playbin", &error);

  {
    GST_DEBUG ("try to print  playbin: %p", data->pipeline);
    gst_element_print_properties(data->pipeline);

    GST_DEBUG ("try to create pipeline  videotestsrc ! ....");
    GstElement* pipeline = gst_parse_launch ("videotestsrc ! video/x-raw,width=1080,height=720 ! autovideosink", &error);
    if(GST_IS_ELEMENT (pipeline))
    {
      GST_DEBUG ("Created element: %p", pipeline);
      gst_element_print_properties(pipeline);

      if(GST_IS_PIPELINE(pipeline))
      {
        GST_DEBUG ("This element is pipeline");

        GstElement* video_sink = gst_bin_get_by_interface(GST_BIN(pipeline), GST_TYPE_VIDEO_OVERLAY);

        if(video_sink)
        {
          GST_DEBUG ("looking for video sink: 0x%x",video_sink);
          gst_element_print_properties(video_sink);
        }
        else
        {
          GST_DEBUG ("failed to get video sink!!!");
        }
      }
      else
      {
        GST_DEBUG ("Failed to create pipeline!!!");
      }
    }
    else
    {
      GST_DEBUG ("Failed to create element!!!");
    }
  }

  if (error) {
    gchar *message =
        g_strdup_printf ("Unable to build pipeline: %s", error->message);
    g_clear_error (&error);
    set_ui_message (message, data);
    g_free (message);
    return NULL;
  }

  /* Disable subtitles */
  g_object_get (data->pipeline, "flags", &flags, NULL);
  flags &= ~GST_PLAY_FLAG_TEXT;
  g_object_set (data->pipeline, "flags", flags, NULL);

  /* Set the pipeline to READY, so it can already accept a window handle, if we have one */
  data->target_state = GST_STATE_READY;
  gst_element_set_state (data->pipeline, GST_STATE_READY);

  /* Instruct the bus to emit signals for each received message, and connect to the interesting signals */
  bus = gst_element_get_bus (data->pipeline);
  bus_source = gst_bus_create_watch (bus);
  g_source_set_callback (bus_source, (GSourceFunc) gst_bus_async_signal_func,
      NULL, NULL);
  g_source_attach (bus_source, data->context);
  g_source_unref (bus_source);
  g_signal_connect (G_OBJECT (bus), "message::error", (GCallback) error_cb,
      data);
  g_signal_connect (G_OBJECT (bus), "message::eos", (GCallback) eos_cb, data);
  g_signal_connect (G_OBJECT (bus), "message::state-changed",
      (GCallback) state_changed_cb, data);
  g_signal_connect (G_OBJECT (bus), "message::duration",
      (GCallback) duration_cb, data);
  g_signal_connect (G_OBJECT (bus), "message::buffering",
      (GCallback) buffering_cb, data);
  g_signal_connect (G_OBJECT (bus), "message::element",
      (GCallback) element_cb, data);
  g_signal_connect (G_OBJECT (bus), "message::clock-lost",
      (GCallback) clock_lost_cb, data);
  
//  bus already got an event source
//  m_bus_id = gst_bus_add_watch( G_OBJECT (bus), (GstBusFunc) csio_GstMsgHandler, data );//Crestron change
//  GST_DEBUG ("app_function m_bus_id: %d", m_bus_id);


  gst_object_unref (bus);

  /* Register a function that GLib will call 4 times per second */
  timeout_source = g_timeout_source_new (250);
  g_source_set_callback (timeout_source, (GSourceFunc) refresh_ui, data, NULL);
  g_source_attach (timeout_source, data->context);
  g_source_unref (timeout_source);

  /* Create a GLib Main Loop and set it to run */
  GST_DEBUG ("Entering main loop... (CustomData:%p)", data);
  data->main_loop = g_main_loop_new (data->context, FALSE);
  check_initialization_complete (data);
  g_main_loop_run (data->main_loop);
  GST_DEBUG ("Exited main loop");
//  g_source_remove( m_bus_id );//Crestron change

  g_main_loop_unref (data->main_loop);
  data->main_loop = NULL;

  /* Free resources */
  g_main_context_pop_thread_default (data->context);
  g_main_context_unref (data->context);
  data->target_state = GST_STATE_NULL;
  gst_element_set_state (data->pipeline, GST_STATE_NULL);
  gst_object_unref (data->pipeline);

  return NULL;
}
#else
static void *
app_function (void *userdata)
{
  JavaVMAttachArgs args;
  GstBus *bus;
  CustomData *data = (CustomData *) userdata;
  GSource *timeout_source;
  GSource *bus_source;
  GError *error = NULL;
  guint flags;
  guint m_bus_id; //Crestron change

  GST_DEBUG ("Creating pipeline by hand. data: %p", data);  

  /* Create our own GLib Main Context and make it the default one */
  data->context = g_main_context_new ();
  g_main_context_push_thread_default (data->context);

  /* Build pipeline */
  data->pipeline = gst_parse_launch ("rtspsrc location=rtsp://170.93.143.139/rtplive/e40037d1c47601b8004606363d235daa latency=45 !"
                                     " rtph264depay ! decodebin ! videoconvert ! glimagesink", &error);

  // data->pipeline = gst_parse_launch ("videotestsrc ! video/x-raw,width=1080,height=720 ! autovideosink", &error);
  // data->pipeline = gst_parse_launch ("videotestsrc ! video/x-raw,format=YUY2 ! videoconvert ! glimagesink", &error);
  // data->pipeline = gst_parse_launch ("rtspsrc location=rtsp://170.93.143.139/rtplive/e40037d1c47601b8004606363d235daa !"
  //                                    " rtph264depay ! decodebin ! queue ! videoconvert ! autovideosink", &error);
  // data->pipeline = gst_parse_launch ("rtspsrc location=rtsp://170.93.143.139/rtplive/e40037d1c47601b8004606363d235daa latency=45 !"
  //                                    " rtph264depay ! decodebin ! queue ! videoconvert ! glimagesink", &error);

  // data->pipeline = gst_parse_launch ("rtspsrc location=rtsp://170.93.143.139/rtplive/e40037d1c47601b8004606363d235daa latency=45 !"
  //                                   " decodebin ! queue ! videoconvert ! videoscale ! video/x-raw(memory:GLMemory) ! glimagesink", &error);
  // data->pipeline = gst_parse_launch ("rtspsrc location=rtsp://170.93.143.139/rtplive/e40037d1c47601b8004606363d235daa latency=45 !"
  //                                    " decodebin ! video/x-raw, format=RGBA ! queue ! videoconvert ! videoscale ! glimagesink", &error);

  //data->pipeline = gst_parse_launch ("audiotestsrc ! audioconvert ! audioresample ! autoaudiosink ", &error); ends up with fakesink
  // data->pipeline = gst_parse_launch ("audiotestsrc ! audioconvert ! audioresample ! openslessink ", &error); //not working
  // data->pipeline = gst_parse_launch ("rtspsrc location=rtsp://10.116.165.119:8554/test latency=45 !"
  //                                    " rtph264depay ! decodebin ! videoconvert ! glimagesink", &error);


  // data->pipeline = gst_parse_launch ("playbin uri=rtsp://10.116.165.119:8554/test", &error);

//  data->pipeline = gst_parse_launch ("audiotestsrc ! audioconvert ! audioresample ! audio/x-raw, rate=48000 ! openslessink ", &error);

  if (error) {
    gchar *message =
        g_strdup_printf ("Unable to build pipeline: %s", error->message);
    g_clear_error (&error);
    set_ui_message (message, data);
    g_free (message);
    return NULL;
  }

  /* Disable subtitles */
  // g_object_get (data->pipeline, "flags", &flags, NULL);
  // flags &= ~GST_PLAY_FLAG_TEXT;
  // g_object_set (data->pipeline, "flags", flags, NULL);

  /* Set the pipeline to READY, so it can already accept a window handle, if we have one */
  data->target_state = GST_STATE_READY;
  gst_element_set_state (data->pipeline, GST_STATE_READY);

  //using csio_GstMsgHandler()
  bus = gst_element_get_bus (data->pipeline);
  m_bus_id = gst_bus_add_watch( G_OBJECT (bus), (GstBusFunc) csio_GstMsgHandler, data );//Crestron change
  GST_DEBUG ("app_function m_bus_id: %d", m_bus_id);


  /* Register a function that GLib will call 4 times per second */
  timeout_source = g_timeout_source_new (250);
  g_source_set_callback (timeout_source, (GSourceFunc) refresh_ui, data, NULL);
  g_source_attach (timeout_source, data->context);
  g_source_unref (timeout_source);

  /* Create a GLib Main Loop and set it to run */
  GST_DEBUG ("Entering main loop... (CustomData:%p)", data);
  data->main_loop = g_main_loop_new (data->context, FALSE);
  check_initialization_complete (data);
  g_main_loop_run (data->main_loop);
  GST_DEBUG ("Exited main loop");
//  g_source_remove( m_bus_id );//Crestron change

  g_main_loop_unref (data->main_loop);
  data->main_loop = NULL;

  /* Free resources */
  g_main_context_pop_thread_default (data->context);
  g_main_context_unref (data->context);
  data->target_state = GST_STATE_NULL;
  gst_element_set_state (data->pipeline, GST_STATE_NULL);
  gst_object_unref (data->pipeline);

  return NULL;
}
#endif
/*
 * Java Bindings
 */

/* Instruct the native code to create its internal data structure, pipeline and thread */
static void
gst_native_init (JNIEnv * env, jobject thiz)
{
  __android_log_print (ANDROID_LOG_ERROR, "GStreamer",
                       "gst_native_init in tutorial-5.c");

  CustomData *data = g_new0 (CustomData, 1);
  data->desired_position = GST_CLOCK_TIME_NONE;
  data->last_seek_time = GST_CLOCK_TIME_NONE;
  SET_CUSTOM_DATA (env, thiz, custom_data_field_id, data);
  GST_DEBUG_CATEGORY_INIT (debug_category, "tutorial-5", 0,
      "Android tutorial 5");
  gst_debug_set_threshold_for_name ("tutorial-5", GST_LEVEL_DEBUG);
  GST_DEBUG ("Created CustomData at %p", data);
  data->app = (*env)->NewGlobalRef (env, thiz);
  GST_DEBUG ("Created GlobalRef for app object at %p", data->app);

  gst_debug_set_threshold_for_name ("GST_ELEMENT_FACTORY", GST_LEVEL_DEBUG);//Crestron change
  gst_debug_set_threshold_for_name ("rtpjitterbuffer", GST_LEVEL_INFO);//Crestron change
  gst_debug_set_threshold_for_name ("amcvideodec", GST_LEVEL_WARNING);//Crestron change

  pthread_create (&gst_app_thread, NULL, &app_function, data);

  csio_init();
}

/* Quit the main loop, remove the native thread and free resources */
static void
gst_native_finalize (JNIEnv * env, jobject thiz)
{
  CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
  if (!data)
    return;
  GST_DEBUG ("Quitting main loop...");
  g_main_loop_quit (data->main_loop);
  GST_DEBUG ("Waiting for thread to finish...");
  pthread_join (gst_app_thread, NULL);
  GST_DEBUG ("Deleting GlobalRef for app object at %p", data->app);
  (*env)->DeleteGlobalRef (env, data->app);
  GST_DEBUG ("Freeing CustomData at %p", data);
  g_free (data);
  SET_CUSTOM_DATA (env, thiz, custom_data_field_id, NULL);
  GST_DEBUG ("Done finalizing");
}

/* Set playbin2's URI */
void
gst_native_set_uri (JNIEnv * env, jobject thiz, jstring uri)
{
  CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
  if (!data || !data->pipeline)
    return;
  const gchar *char_uri = (*env)->GetStringUTFChars (env, uri, NULL);
  GST_DEBUG ("Setting URI to %s, target state: %s.", 
              char_uri,
              gst_element_state_get_name(data->target_state));
              
  if (data->target_state >= GST_STATE_READY)
    gst_element_set_state (data->pipeline, GST_STATE_READY);
  g_object_set (data->pipeline, "uri", char_uri, NULL);
  (*env)->ReleaseStringUTFChars (env, uri, char_uri);
  data->duration = GST_CLOCK_TIME_NONE;
  data->is_live |=
      (gst_element_set_state (data->pipeline,
          data->target_state) == GST_STATE_CHANGE_NO_PREROLL);
}

/* Set pipeline to PLAYING state */
static void
gst_native_play (JNIEnv * env, jobject thiz)
{
  CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
  if (!data)
    return;
  GST_DEBUG ("Setting state to PLAYING");
  data->target_state = GST_STATE_PLAYING;
  data->is_live |=
      (gst_element_set_state (data->pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_NO_PREROLL);
}

/* Set pipeline to PAUSED state */
static void
gst_native_pause (JNIEnv * env, jobject thiz)
{
  CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
  if (!data)
    return;
  GST_DEBUG ("Setting state to PAUSED");
  data->target_state = GST_STATE_PAUSED;
  data->is_live |=
      (gst_element_set_state (data->pipeline,
          GST_STATE_PAUSED) == GST_STATE_CHANGE_NO_PREROLL);

  cres_add_graph(data);//Crestron change
}

/* Instruct the pipeline to seek to a different position */
void
gst_native_set_position (JNIEnv * env, jobject thiz, int milliseconds)
{
  CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
  if (!data)
    return;
  gint64 desired_position = (gint64) (milliseconds * GST_MSECOND);
  if (data->state >= GST_STATE_PAUSED) {
    execute_seek (desired_position, data);
  } else {
    GST_DEBUG ("Scheduling seek to %" GST_TIME_FORMAT " for later",
        GST_TIME_ARGS (desired_position));
    data->desired_position = desired_position;
  }
}

/* Static class initializer: retrieve method and field IDs */
static jboolean
gst_native_class_init (JNIEnv * env, jclass klass)
{
  custom_data_field_id =
      (*env)->GetFieldID (env, klass, "native_custom_data", "J");
  set_message_method_id =
      (*env)->GetMethodID (env, klass, "setMessage", "(Ljava/lang/String;)V");
  set_current_position_method_id =
      (*env)->GetMethodID (env, klass, "setCurrentPosition", "(II)V");
  on_gstreamer_initialized_method_id =
      (*env)->GetMethodID (env, klass, "onGStreamerInitialized", "()V");
  on_media_size_changed_method_id =
      (*env)->GetMethodID (env, klass, "onMediaSizeChanged", "(II)V");

  if (!custom_data_field_id || !set_message_method_id
      || !on_gstreamer_initialized_method_id || !on_media_size_changed_method_id
      || !set_current_position_method_id) {
    /* We emit this message through the Android log instead of the GStreamer log because the later
     * has not been initialized yet.
     */
    __android_log_print (ANDROID_LOG_ERROR, "tutorial-4",
        "The calling class does not implement all necessary interface methods");
    return JNI_FALSE;
  }
  return JNI_TRUE;
}

static void
gst_native_surface_init (JNIEnv * env, jobject thiz, jobject surface)
{
  CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
  if (!data)
    return;
  ANativeWindow *new_native_window = ANativeWindow_fromSurface (env, surface);
  GST_DEBUG ("Received surface %p (native window %p)", surface,
      new_native_window);

  if (data->native_window) {
    ANativeWindow_release (data->native_window);
    if (data->native_window == new_native_window) {
      GST_DEBUG ("New native window is the same as the previous one %p",
          data->native_window);
      if (data->pipeline) {
        gst_video_overlay_expose (GST_VIDEO_OVERLAY (data->pipeline));
        gst_video_overlay_expose (GST_VIDEO_OVERLAY (data->pipeline));
      }
      return;
    } else {
      GST_DEBUG ("Released previous native window %p", data->native_window);
      data->initialized = FALSE;
    }
  }
  data->native_window = new_native_window;

  //Crestron changestarts
  {
    int queuesToNativeWindow = 0;
    GST_ERROR("ANativeWindow format is 0x%x", ANativeWindow_getFormat(new_native_window));
//    int err = ANativeWindow_query(new_native_window)   query(NATIVE_WINDOW_QUEUES_TO_WINDOW_COMPOSER, &queuesToNativeWindow);
//    NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS
//            ANativeWindow_query
  }
  //Crestron change ends

  check_initialization_complete (data);
}

static void
gst_native_surface_finalize (JNIEnv * env, jobject thiz)
{
  CustomData *data = GET_CUSTOM_DATA (env, thiz, custom_data_field_id);
  if (!data)
    return;
  GST_DEBUG ("Releasing Native Window %p", data->native_window);

  if (data->pipeline) {
#if USE_PLAYBIN
    GST_DEBUG("calling video_overlay_set_window NULL here.");
    gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (data->pipeline),
        (guintptr) NULL);
#else
    if (data->video_sink)
    {  
        GST_DEBUG("calling video_overlay_set_window directly to video sink to NULL.");
        gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (data->video_sink),
          (guintptr) NULL);
    }
#endif

    gst_element_set_state (data->pipeline, GST_STATE_READY);
  }

  ANativeWindow_release (data->native_window);
  data->native_window = NULL;
  data->initialized = FALSE;
}

/* List of implemented native methods */
static JNINativeMethod native_methods[] = {
  {"nativeInit", "()V", (void *) gst_native_init},
  {"nativeFinalize", "()V", (void *) gst_native_finalize},
  {"nativeSetUri", "(Ljava/lang/String;)V", (void *) gst_native_set_uri},
  {"nativePlay", "()V", (void *) gst_native_play},
  {"nativePause", "()V", (void *) gst_native_pause},
  {"nativeSetPosition", "(I)V", (void *) gst_native_set_position},
  {"nativeSurfaceInit", "(Ljava/lang/Object;)V",
      (void *) gst_native_surface_init},
  {"nativeSurfaceFinalize", "()V", (void *) gst_native_surface_finalize},
  {"nativeClassInit", "()Z", (void *) gst_native_class_init}
};

/* Library initializer */
jint
JNI_OnLoad (JavaVM * vm, void *reserved)
{
  JNIEnv *env = NULL;

  java_vm = vm;

  __android_log_print (ANDROID_LOG_ERROR, "GStreamer",
                       "JNI_OnLoad in tutorial-5.c[%s]",getenv("GST_DEBUG"));
  // csio_init();

  //Crestron change starts
  setenv("GST_DEBUG","*:3",1);

  //Note: do not use GST_DEBUG here.
  //      use GST_DEBUG_CATEGORY_INIT in gst_native_init()
  //      to set each category level by gst_debug_set_threshold_for_name().
  //setenv("GST_DEBUG","rtpjitterbuffer:5",1);
  //setenv("GST_DEBUG","amc:5",1);
  //setenv("GST_DEBUG","GST_ELEMENT_FACTORY:5",1);


  //setenv("GST_AMC_IGNORE_UNKNOWN_COLOR_FORMATS", "yes", 1);
  /**
   * did not work,we are not using gst-launch-1.0 here.
   * setenv("GST_DEBUG_DUMP_DOT_DIR","/data/app/gst-graph",1);
   * */
  //Crestron change ends

  __android_log_print (ANDROID_LOG_ERROR, "tutorial-5",
                       "JNI_OnLoad in tutorial-5: GST_DEBUG set to [%s]",getenv("GST_DEBUG"));

  if ((*vm)->GetEnv (vm, (void **) &env, JNI_VERSION_1_4) != JNI_OK) {
    __android_log_print (ANDROID_LOG_ERROR, "tutorial-5",
        "Could not retrieve JNIEnv");
    return 0;
  }
  jclass klass = (*env)->FindClass (env,
      "org/freedesktop/gstreamer/tutorials/tutorial_5/Tutorial5");
  (*env)->RegisterNatives (env, klass, native_methods,
      G_N_ELEMENTS (native_methods));

  pthread_key_create (&current_jni_env, detach_current_thread);

    __android_log_print (ANDROID_LOG_ERROR, "GStreamer",
                         "JNI_OnLoad returned in tutorial-5.c[%s]",getenv("GST_DEBUG"));

  return JNI_VERSION_1_4;
}

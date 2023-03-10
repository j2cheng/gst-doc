package org.freedesktop.gstreamer.tutorials.tutorial_5;

import static android.view.View.SYSTEM_UI_FLAG_FULLSCREEN;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.TimeZone;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;

//Crestron change starts
//import android.opengl.GLES31;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;

//Crestron change ends

import android.os.Bundle;
import android.os.Environment;
import android.os.PowerManager;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import android.widget.Toast;
import android.media.MediaCodecList;
import android.media.MediaCodecInfo;

import org.freedesktop.gstreamer.GStreamer;
import com.lamerman.FileDialog;

public class Tutorial5 extends Activity implements SurfaceHolder.Callback, OnSeekBarChangeListener {
    private native void nativeInit();     // Initialize native code, build pipeline, etc
    private native void nativeFinalize(); // Destroy pipeline and shutdown native code
    private native void nativeSetUri(String uri); // Set the URI of the media to play
    private native void nativePlay();     // Set pipeline to PLAYING
    private native void nativeSetPosition(int milliseconds); // Seek to the indicated position, in milliseconds
    private native void nativePause();    // Set pipeline to PAUSED
    private static native boolean nativeClassInit(); // Initialize native class: cache Method IDs for callbacks
    private native void nativeSurfaceInit(Object surface); // A new surface is available
    private native void nativeSurfaceFinalize(); // Surface about to be destroyed
    private long native_custom_data;      // Native code will use this to keep private data

    private boolean is_playing_desired;   // Whether the user asked to go to PLAYING
    private int position;                 // Current position, reported by native code
    private int duration;                 // Current clip duration, reported by native code
    private boolean is_local_media;       // Whether this clip is stored locally or is being streamed
    private int desired_position;         // Position where the users wants to seek to
    private String mediaUri;              // URI of the clip being played

    //private final String defaultMediaUri = "https://www.freedesktop.org/software/gstreamer-sdk/data/media/sintel_trailer-480p.ogv";
//    private final String defaultMediaUri = "rtsp://10.116.165.119:8554/test";
    //private final String defaultMediaUri = "rtsp://\"CrestronTest\":\"AVIcrestron34!\"@62.31.255.82:53554/Streaming/Channels/102";
    private final String defaultMediaUri = "rtsp://170.93.143.139/rtplive/e40037d1c47601b8004606363d235daa";
//    private final String defaultMediaUri = "file:///data/app/AV1.mp4";//this one audio works only on AM3k

    static private final int PICK_FILE_CODE = 1;
    private String last_folder;

    private PowerManager.WakeLock wake_lock;

    // Called when the activity is first created.
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        // Initialize GStreamer and warn if it fails
        try {
            GStreamer.init(this);
        } catch (Exception e) {
            Toast.makeText(this, e.getMessage(), Toast.LENGTH_LONG).show();
            finish();
            return;
        }

        setContentView(R.layout.main);

        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        wake_lock = pm.newWakeLock(PowerManager.FULL_WAKE_LOCK, "GStreamer tutorial 5");
        wake_lock.setReferenceCounted(false);

//        ImageButton play = (ImageButton) this.findViewById(R.id.button_play);
//        play.setOnClickListener(new OnClickListener() {
//            public void onClick(View v) {
//                is_playing_desired = true;
//                wake_lock.acquire();
//                nativePlay();
//            }
//        });
//
//        ImageButton pause = (ImageButton) this.findViewById(R.id.button_stop);
//        pause.setOnClickListener(new OnClickListener() {
//            public void onClick(View v) {
//                is_playing_desired = false;
//                wake_lock.release();
//                nativePause();
//            }
//        });

//        ImageButton select = (ImageButton) this.findViewById(R.id.button_select);
//        select.setOnClickListener(new OnClickListener() {
//            public void onClick(View v) {
//                Intent i = new Intent(getBaseContext(), FileDialog.class);
//                i.putExtra(FileDialog.START_PATH, last_folder);
//                startActivityForResult(i, PICK_FILE_CODE);
//            }
//        });

        SurfaceView sv = (SurfaceView) this.findViewById(R.id.surface_video);
        SurfaceHolder sh = sv.getHolder();
        sh.addCallback(this);

        //setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
        //sv.setSystemUiVisibility(SYSTEM_UI_FLAG_FULLSCREEN);
        //setContentView(sv);
        //sv.setLayoutDirection(View.LAYOUT_DIRECTION_INHERIT);
        Configuration newConfig = getResources().getConfiguration();
        if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
            Log.i ("GStreamer", "onCreate landscape");
        } else if (newConfig.orientation == Configuration.ORIENTATION_PORTRAIT){
            Log.i ("GStreamer", "onCreate portrait");
        }

        //SeekBar sb = (SeekBar) this.findViewById(R.id.seek_bar);
        //sb.setOnSeekBarChangeListener(this);

        // Retrieve our previous state, or initialize it to default values
        if (savedInstanceState != null) {
            is_playing_desired = savedInstanceState.getBoolean("playing");
            position = savedInstanceState.getInt("position");
            duration = savedInstanceState.getInt("duration");
            mediaUri = savedInstanceState.getString("mediaUri");
            last_folder = savedInstanceState.getString("last_folder");
            Log.i ("GStreamer", "Activity created with saved state:");
        } else {
            is_playing_desired = false;
            position = duration = 0;
            last_folder = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_MOVIES).getAbsolutePath();
            Intent intent = getIntent();
            android.net.Uri uri = intent.getData();
            if (uri == null)
                mediaUri = defaultMediaUri;
            else {
                Log.i ("GStreamer", "Received URI: " + uri);
                if (uri.getScheme().equals("content")) {
                    android.database.Cursor cursor = getContentResolver().query(uri, null, null, null, null);
                    cursor.moveToFirst();
                    mediaUri = "file://" + cursor.getString(cursor.getColumnIndex(android.provider.MediaStore.Video.Media.DATA));
                    cursor.close();
                } else
                    mediaUri = uri.toString();
            }
            Log.i ("GStreamer", "Activity created with no saved state:");
        }
        is_local_media = false;
        Log.i ("GStreamer", "  playing:" + is_playing_desired + " position:" + position +
                " duration: " + duration + " uri: " + mediaUri);

        // Start with disabled buttons, until native code is initialized
//        this.findViewById(R.id.button_play).setEnabled(false);
//        this.findViewById(R.id.button_stop).setEnabled(false);
        Log.i ("GStreamer", "Calling nativeInit in OnCreate");

        nativeInit();

        Log.i ("GStreamer", "nativeInit returned");


        //Crestron change starts
//        int count = MediaCodecList.getCodecCount();
//        for (int i = 0; i < count; ++i) {
//            MediaCodecInfo info = MediaCodecList.getCodecInfoAt(i);
//            if (info.isEncoder()) {
//                Log.i ("GStreamer", "This [" + i + "] is encoder ");
//
//                String[] supportedTypes = info.getSupportedTypes();
//                for (int j = 0; j < supportedTypes.length; ++j) {
//                    Log.i ("GStreamer", "This encoder name is: " + info.getName());
//                }
//            }
//            else
//            {
//                Log.i ("GStreamer", "This [" + i + "] is decoder ");
//
//                //static
//                //bool MediaCodecList::isSoftwareCodec(const AString &componentName) {
//                //return componentName.startsWithIgnoreCase("OMX.google.")
//                //        || componentName.startsWithIgnoreCase("c2.android.")
//                //        || (!componentName.startsWithIgnoreCase("OMX.")
//                //        && !componentName.startsWithIgnoreCase("c2."));
//
//
//                String[] supportedTypes = info.getSupportedTypes();
//                for (int j = 0; j < supportedTypes.length; ++j) {
//                    Log.i ("GStreamer", "This decoder name is: " + info.getName());
//                }
//
//            }
//        }


//        if (
//                GLES20.glGetString(GLES20.GL_RENDERER) == null ||
//                        GLES20.glGetString(GLES20.GL_VENDOR) == null ||
//                        GLES20.glGetString(GLES20.GL_VERSION) == null ||
//                        GLES20.glGetString(GLES20.GL_EXTENSIONS) == null ||
//                        GLES10.glGetString(GLES10.GL_RENDERER) == null ||
//                        GLES10.glGetString(GLES10.GL_VENDOR) == null ||
//                        GLES10.glGetString(GLES10.GL_VERSION) == null ||
//                        GLES10.glGetString(GLES10.GL_EXTENSIONS) == null) {
//            // try to use SurfaceView
//        } else {
//            // try to use TextureView
//        }

//        Log.i ("GStreamer", "GLES31 GL_RENDERER is: " + GLES31.glGetString(GLES31.GL_RENDERER));
//        Log.i ("GStreamer", "GLES31 GL_VENDOR is: " + GLES31.glGetString(GLES31.GL_VENDOR));
//        Log.i ("GStreamer", "GLES31 GL_VERSION is: " + GLES31.glGetString(GLES31.GL_VERSION));


        //Crestron change ends
    }

    protected void onSaveInstanceState (Bundle outState) {
        Log.d ("GStreamer", "Saving state, playing:" + is_playing_desired + " position:" + position +
                " duration: " + duration + " uri: " + mediaUri);
        outState.putBoolean("playing", is_playing_desired);
        outState.putInt("position", position);
        outState.putInt("duration", duration);
        outState.putString("mediaUri", mediaUri);
        outState.putString("last_folder", last_folder);
    }

    protected void onDestroy() {
        nativeFinalize();
        if (wake_lock.isHeld())
            wake_lock.release();
        super.onDestroy();
    }

    // Called from native code. This sets the content of the TextView from the UI thread.
    private void setMessage(final String message) {
        final TextView tv = (TextView) this.findViewById(R.id.textview_message);
        runOnUiThread (new Runnable() {
          public void run() {
            tv.setText(message);
          }
        });
    }

    // Set the URI to play, and record whether it is a local or remote file
    private void setMediaUri() {
        nativeSetUri (mediaUri);
        is_local_media = mediaUri.startsWith("file://");
    }

    // Called from native code. Native code calls this once it has created its pipeline and
    // the main loop is running, so it is ready to accept commands.
    private void onGStreamerInitialized () {
        Log.i ("GStreamer", "GStreamer initialized:");
        Log.i ("GStreamer", "  playing:" + is_playing_desired + " position:" + position + " uri: " + mediaUri);

        // Restore previous playing state
        setMediaUri ();
        nativeSetPosition (position);
        if (is_playing_desired) {
            nativePlay();
            wake_lock.acquire();
        } else {
            nativePlay();//Crestron changed
            wake_lock.release();
        }

        // Re-enable buttons, now that GStreamer is initialized
        final Activity activity = this;
        runOnUiThread(new Runnable() {
            public void run() {
//                activity.findViewById(R.id.button_play).setEnabled(true);
//                activity.findViewById(R.id.button_stop).setEnabled(true);
                Log.i ("GStreamer", "  button_play  button_stop removed." );
            }
        });
    }

    // The text widget acts as an slave for the seek bar, so it reflects what the seek bar shows, whether
    // it is an actual pipeline position or the position the user is currently dragging to.
    private void updateTimeWidget () {
//        TextView tv = (TextView) this.findViewById(R.id.textview_time);
//        SeekBar sb = (SeekBar) this.findViewById(R.id.seek_bar);
//        int pos = sb.getProgress();
//
//        SimpleDateFormat df = new SimpleDateFormat("HH:mm:ss");
//        df.setTimeZone(TimeZone.getTimeZone("UTC"));
//        String message = df.format(new Date (pos)) + " / " + df.format(new Date (duration));
//        tv.setText(message);
    }

    // Called from native code
    private void setCurrentPosition(final int position, final int duration) {
//        final SeekBar sb = (SeekBar) this.findViewById(R.id.seek_bar);
//
//        // Ignore position messages from the pipeline if the seek bar is being dragged
//        if (sb.isPressed()) return;

//        runOnUiThread (new Runnable() {
//          public void run() {
//            sb.setMax(duration);
//            sb.setProgress(position);
//            updateTimeWidget();
//            sb.setEnabled(duration != 0);
//          }
//        });
        this.position = position;
        this.duration = duration;
    }

    static {
        System.loadLibrary("gstreamer_android");
        System.loadLibrary("tutorial-5");
        nativeClassInit();
    }

    public void surfaceChanged(SurfaceHolder holder, int format, int width,
            int height) {
        Log.d("GStreamer", "Surface changed to format " + format + " width "
                + width + ", height " + height);
        nativeSurfaceInit (holder.getSurface());
    }

    public void surfaceCreated(SurfaceHolder holder) {
        Log.d("GStreamer", "Surface created: " + holder.getSurface());
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.d("GStreamer", "Surface destroyed");
        nativeSurfaceFinalize ();
    }

    // Called from native code when the size of the media changes or is first detected.
    // Inform the video surface about the new size and recalculate the layout.
    private void onMediaSizeChanged (int width, int height) {
        Log.i ("GStreamer", "Media size changed to " + width + ", x" + height);
        final GStreamerSurfaceView gsv = (GStreamerSurfaceView) this.findViewById(R.id.surface_video);
        gsv.media_width = width;
        gsv.media_height = height;
        runOnUiThread(new Runnable() {
            public void run() {
                gsv.requestLayout();
            }
        });
    }

    // The Seek Bar thumb has moved, either because the user dragged it or we have called setProgress()
    public void onProgressChanged(SeekBar sb, int progress, boolean fromUser) {
        if (fromUser == false) return;
        desired_position = progress;
        // If this is a local file, allow scrub seeking, this is, seek as soon as the slider is moved.
        if (is_local_media) nativeSetPosition(desired_position);
        updateTimeWidget();
    }

    // The user started dragging the Seek Bar thumb
    public void onStartTrackingTouch(SeekBar sb) {
        nativePause();
    }

    // The user released the Seek Bar thumb
    public void onStopTrackingTouch(SeekBar sb) {
        // If this is a remote file, scrub seeking is probably not going to work smoothly enough.
        // Therefore, perform only the seek when the slider is released.
        if (!is_local_media) nativeSetPosition(desired_position);
        if (is_playing_desired) nativePlay();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        if (resultCode == RESULT_OK && requestCode == PICK_FILE_CODE) {
            mediaUri = "file://" + data.getStringExtra(FileDialog.RESULT_PATH);
            position = 0;
            last_folder = new File (data.getStringExtra(FileDialog.RESULT_PATH)).getParent();
            Log.i("GStreamer", "Setting last_folder to " + last_folder);
            setMediaUri();
        }
    }
    //Crestron change starts
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        // Checks the orientation of the screen
        if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
//            Toast.makeText(this, "landscape", Toast.LENGTH_SHORT).show();
            Log.i("GStreamer", "onConfigurationChanged landscape");


            //setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
        } else if (newConfig.orientation == Configuration.ORIENTATION_PORTRAIT){
//            Toast.makeText(this, "portrait", Toast.LENGTH_SHORT).show();
            Log.i("GStreamer", "onConfigurationChanged portrait");

            //dsetRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);


//            final GStreamerSurfaceView gsv = (GStreamerSurfaceView) this.findViewById(R.id.surface_video);
//            gsv.setSystemUiVisibility(SYSTEM_UI_FLAG_FULLSCREEN);
//            setContentView(gsv);
            Log.i("GStreamer", "onConfigurationChanged set to full screen");
        }
    }//Crestron change ends
}

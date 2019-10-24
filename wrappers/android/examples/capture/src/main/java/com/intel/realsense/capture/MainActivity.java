package com.intel.realsense.capture;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.widget.TextView;

import com.intel.realsense.librealsense.Config;
import com.intel.realsense.librealsense.DepthFrame;
import com.intel.realsense.librealsense.DeviceListener;
import com.intel.realsense.librealsense.Frame;
import com.intel.realsense.librealsense.FrameCallback;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.Pipeline;
import com.intel.realsense.librealsense.RsContext;
import com.intel.realsense.librealsense.StreamFormat;
import com.intel.realsense.librealsense.StreamType;

import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {
    private static final int MY_PERMISSIONS_REQUEST_CAMERA = 0;
    private static final String TAG = "librs";

    private RsContext mRsContext;
    private int mIteration = 0;

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, MY_PERMISSIONS_REQUEST_CAMERA);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();

        if(!mStreamingThread.isAlive())
            mStreamingThread.start();
    }

    @Override
    protected void onPause() {
        super.onPause();
        mStreamingThread.interrupt();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        //RsContext.init must be called once in the application's lifetime before any interaction with physical RealSense devices.
        //For multi activities applications use the application context instead of the activity context
        RsContext.init(getApplicationContext());

        //Register to notifications regarding RealSense devices attach/detach events via the DeviceListener.
        mRsContext = new RsContext();
        mRsContext.setDevicesChangedCallback(new DeviceListener() {
            @Override
            public void onDeviceAttach() {
                mStreamingThread.start();
            }

            @Override
            public void onDeviceDetach() {
                mStreamingThread.interrupt();
            }
        });

        // Android 9 also requires camera permissions
        if (android.os.Build.VERSION.SDK_INT > android.os.Build.VERSION_CODES.O &&
                ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, MY_PERMISSIONS_REQUEST_CAMERA);
        }
    }

    private Thread mStreamingThread = new Thread(new Runnable() {
        @Override
        public void run() {
            try(Config cfg = new Config()) {
//                    cfg.enableStream(StreamType.DEPTH, 640, 480);
                cfg.enableStream(StreamType.COLOR, 0,1280, 720, StreamFormat.YUYV, 30);
//                    cfg.enableStream(StreamType.INFRARED, 640, 480);
//                    cfg.enableStream(StreamType.GYRO);
//                    cfg.enableStream(StreamType.ACCEL);

                try (Pipeline pipe = new Pipeline()) {
                    while(!mStreamingThread.isInterrupted()) {
                        try {
                            stream(pipe, cfg);
//                            Thread.sleep(500);
                        } catch (Exception e) {
                            runOnUiThread(new Runnable() {
                                @Override
                                public void run() {
                                    TextView textView = findViewById(R.id.connectCameraText);
                                    textView.setText(textView.getText() + "\nwait for frames failed");
                                }
                            });
                            e.printStackTrace();
                            break;
                        }
                    }
                }
            }
        }
    });

    //Start streaming and print the distance of the center pixel in the depth frame.
    private void stream(Pipeline pipe, Config cfg) throws Exception {
        Log.i(TAG, "start iteratoin: " + ++mIteration );
//        pipe.start(cfg);
        pipe.start();

        Log.i(TAG, "iteratoin: " + mIteration +" started" );


        for(int i = 0; i < 10; i++) {
            try (FrameSet frames = pipe.waitForFrames()) {
                final List<String> msgs =new ArrayList<>();

                frames.foreach(new FrameCallback() {
                    @Override
                    public void onFrame(Frame frame) {
                        msgs.add(frame.getProfile().getType() + ", number: " + frame.getNumber() + "\n");
                    }
                });
//                Log.i(TAG, "frame received");
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        TextView textView = findViewById(R.id.connectCameraText);
                        String msg = "iteratoin: " + mIteration + "\n";
                        for (String s : msgs)
                            msg += s;
                        textView.setText(msg);
                    }
                });
            }
        }
        Log.i(TAG, "stopping iteratoin: " + mIteration );

        pipe.stop();
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                TextView textView = findViewById(R.id.connectCameraText);
                textView.setText("stopped: " + mIteration);
            }
        });
        Log.i(TAG, "iteratoin: " + mIteration + " stopped");
    }
}

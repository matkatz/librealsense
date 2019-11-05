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
import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.DeviceListener;
import com.intel.realsense.librealsense.Extension;
import com.intel.realsense.librealsense.Frame;
import com.intel.realsense.librealsense.FrameCallback;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.GLRsSurfaceView;
import com.intel.realsense.librealsense.Pipeline;
import com.intel.realsense.librealsense.RegionOfInterest;
import com.intel.realsense.librealsense.RoiSensor;
import com.intel.realsense.librealsense.RsContext;
import com.intel.realsense.librealsense.Sensor;
import com.intel.realsense.librealsense.StreamFormat;
import com.intel.realsense.librealsense.StreamProfile;
import com.intel.realsense.librealsense.StreamType;

import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {
    private static final int MY_PERMISSIONS_REQUEST_CAMERA = 0;
    private static final String TAG = "librs";

    private boolean mPermissionsGrunted = false;
    private int mFrameCount = 0;
    private Sensor mDepthSensor;
    private RoiSensor mColorSensor;
    private Device mDevice;
    private boolean mState = false;

    private RsContext mRsContext;
    private int mIteration = 0;

    private int mWidth = 640;
    private int mHeight = 480;
    private GLRsSurfaceView mGLSurfaceView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mGLSurfaceView = findViewById(R.id.glSurfaceView);

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, MY_PERMISSIONS_REQUEST_CAMERA);
            return;
        }

        mPermissionsGrunted = true;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, MY_PERMISSIONS_REQUEST_CAMERA);
            return;
        }
        mPermissionsGrunted = true;
    }

    private synchronized void init(){
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
        try(DeviceList dl = mRsContext.queryDevices()) {
            if (dl.getDeviceCount() > 0){
                mStreamingThread.start();
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();

        if(mPermissionsGrunted) {
            init();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        mStreamingThread.interrupt();
    }

    private Thread mStreamingThread = new Thread(new Runnable() {
        @Override
        public void run() {
            try(DeviceList dl = mRsContext.queryDevices()){
                if(dl.getDeviceCount() > 0) {
                    mDevice = dl.createDevice(0);
                    List<Sensor> sl = mDevice.querySensors();
                    for (int si = 0; si < sl.size(); ++ si) {
                        Sensor sensor = sl.get(si);
                        List<StreamProfile> spl = sensor.getStreamProfiles();
                        for (int spi = 0; spi < spl.size(); ++spi) {
                            StreamProfile sp = spl.get(spi);
                            Log.d(TAG, String.format("si %d spi %d type %s", si, spi, sp.getType().toString()));
                            if (sp.getType() == StreamType.COLOR) {
                                mColorSensor = sensor.as(Extension.ROI);
                                break;
                            } else if (sp.getType() == StreamType.DEPTH) {
                                mDepthSensor = sensor;
                                break;
                            }
                        }
                    }
                }
            }

            try(Config cfg = new Config()) {
                cfg.enableStream(StreamType.DEPTH, 0, mWidth, mHeight, StreamFormat.Z16, 30);
                cfg.enableStream(StreamType.COLOR, 0,mWidth, mHeight, StreamFormat.RGB8, 30);
                cfg.enableStream(StreamType.INFRARED, 1,mWidth, mHeight, StreamFormat.Y8, 30);

                try (Pipeline pipe = new Pipeline()) {
                    while(!mStreamingThread.isInterrupted()) {
                        try {
                            stream(pipe, cfg);
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
        pipe.start(cfg);

        Log.i(TAG, "iteratoin: " + mIteration +" started" );

        try {
            mState = !mState;
            RegionOfInterest roi = mState ?
                    new RegionOfInterest(0, 0, mWidth / 2, mHeight / 2) :
                    new RegionOfInterest(mWidth/2, mHeight /2, mWidth - 1, mHeight - 1);
            Log.i("roi start","set roi start");
            mColorSensor.setRegionOfInterest(roi);
            Log.i("roi got", "set roi end");
            RegionOfInterest roiOut = mColorSensor.getRegionOfInterest();
            Log.i(TAG, String.format("ROI change, frame count: %d", mFrameCount));
            Log.i(TAG, String.format("ROI change, expected ROI: %d, %d, %d, %d", roi.minX, roi.minY, roi.maxX, roi.maxY));
            Log.i(TAG, String.format("ROI change, actual ROI: %d, %d, %d, %d", roiOut.minX, roiOut.minY, roiOut.maxX, roiOut.maxY));
        }
        catch (Exception e) {
            Log.e(TAG, "ROI change, Failed to set ROI: " + e.getMessage());
        }

        for(int i = 0; i < 100; i++) {
            try (FrameSet frames = pipe.waitForFrames()) {
                mGLSurfaceView.upload(frames);
                final List<String> msgs =new ArrayList<>();
                mFrameCount ++;
//                Log.d(TAG, String.format("waitForFrames %d size %d", mFrameCount, frames.getSize()));

//                try {
//                    if (mFrameCount % 10 == 0) {
//                        mState = !mState;
//                        RegionOfInterest roi = mState ?
//                                new RegionOfInterest(0, 0, mWidth / 2, mHeight / 2) :
//                                new RegionOfInterest(mWidth/2, mHeight /2, mWidth - 1, mHeight - 1);
//                        Log.e("roi start","set roi start");
//                        mColorSensor.setRegionOfInterest(roi);
//                        Log.e("roi got", "set roi end");
//                        RegionOfInterest roiOut = mColorSensor.getRegionOfInterest();
//                        Log.d(TAG, String.format("ROI change, frame count: %d", mFrameCount));
//                        Log.d(TAG, String.format("ROI change, expected ROI: %d, %d, %d, %d", roi.minX, roi.minY, roi.maxX, roi.maxY));
//                        Log.d(TAG, String.format("ROI change, actual ROI: %d, %d, %d, %d", roiOut.minX, roiOut.minY, roiOut.maxX, roiOut.maxY));
//                    }
//                }
//                catch (Exception e) {
//                    Log.e(TAG, "ROI change, Failed to set ROI: " + e.getMessage());
//                }

                frames.foreach(new FrameCallback() {
                    @Override
                    public void onFrame(Frame frame) {
                        msgs.add(frame.getProfile().getType() + ", number: " + frame.getNumber() + "\n");
                    }
                });
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
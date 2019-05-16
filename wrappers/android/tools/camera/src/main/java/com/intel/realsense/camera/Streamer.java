package com.intel.realsense.camera;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Handler;
import android.util.Log;
import android.widget.Toast;

import com.intel.realsense.librealsense.CameraInfo;
import com.intel.realsense.librealsense.Config;
import com.intel.realsense.librealsense.DebugProtocol;
import com.intel.realsense.librealsense.Device;
import com.intel.realsense.librealsense.DeviceList;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.Pipeline;
import com.intel.realsense.librealsense.RsContext;
import com.intel.realsense.librealsense.VideoStreamProfile;

import java.util.Arrays;
import java.util.List;
import java.util.Map;

public class Streamer {
    private static final String TAG = "librs camera streamer";

    interface Listener{
        void config(Config config);
        void onFrameset(FrameSet frameSet);
    }

    private final Context mContext;
    private final boolean mLoadConfig;

    private boolean mIsStreaming = false;
    private final Handler mHandler = new Handler();
    private final Handler mFwLogHandler = new Handler();
    private final Listener mListener;

    private Pipeline mPipeline;

    public Streamer(Context context, boolean loadConfig, Listener listener){
        mContext = context;
        mListener = listener;
        mLoadConfig = loadConfig;
    }

    private Runnable mStreaming = new Runnable() {
        @Override
        public void run() {
            try {
                try(FrameSet frames = mPipeline.waitForFrames(1000)) {
                    mListener.onFrameset(frames);
                }
                mHandler.post(mStreaming);
            }
            catch (Exception e) {
                Log.e(TAG, "streaming, error: " + e.getMessage());
            }
        }
    };

    private Runnable mLogger = new Runnable() {
        @Override
        public void run() {
            try {
                RsContext mRsContext = new RsContext();
                DeviceList devices = mRsContext.queryDevices();
                Device device = devices.createDevice(0);
                DebugProtocol dp = device.as(DebugProtocol.class);
                String strOpCode = device.getInfo(CameraInfo.DEBUG_OP_CODE);
                byte opCode = Byte.parseByte(strOpCode);
                byte[] buff = {0x14, 0x00, (byte) 0xab, (byte) 0xcd, opCode, 0x00, 0x00, 0x00,
                        (byte) 0xf4, 0x01, 0x00, 0x00, 0x00,    0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00,    0x00, 0x00, 0x00};

                byte[] res = dp.SendAndReceiveRawData(buff);

                String strArray[] = new String[res.length];
                for(int i = 0; i < res.length; i++)
                    strArray[i] = String.valueOf((int)res[i]);

                String newString = Arrays.toString(strArray);

                Log.d(TAG, "fw_log: " + newString);
                mFwLogHandler.post(mLogger);
            }
            catch (Exception e) {
                Log.e(TAG, "logging, error: " + e.getMessage());
            }
        }
    };

    private void configStream(Config config){
        config.disableAllStreams();
        RsContext ctx = new RsContext();
        try(DeviceList devices = ctx.queryDevices()) {
            if (devices.getDeviceCount() == 0) {
                return;
            }
        }
        String pid;
        Map<Integer, List<VideoStreamProfile>> profilesMap;
        try(Device device = ctx.queryDevices().createDevice(0)){
            pid = device.getInfo(CameraInfo.PRODUCT_ID);
            profilesMap = SettingsActivity.createProfilesMap(device);
        }

        SharedPreferences sharedPref = mContext.getSharedPreferences(mContext.getString(R.string.app_settings), Context.MODE_PRIVATE);

        for(Map.Entry e : profilesMap.entrySet()){
            List<VideoStreamProfile> profiles = (List<VideoStreamProfile>) e.getValue();
            VideoStreamProfile p = profiles.get(0);
            if(!sharedPref.getBoolean(SettingsActivity.getEnabledDeviceConfigString(pid, p.getType(), p.getIndex()), false))
                continue;
            int index = sharedPref.getInt(SettingsActivity.getIndexdDeviceConfigString(pid, p.getType(), p.getIndex()), 0);
            if(index == -1 || index >= profiles.size())
                throw new IllegalArgumentException("Failed to resolve config");
            VideoStreamProfile sp = profiles.get(index);
            VideoStreamProfile vsp = sp.as(VideoStreamProfile.class);
            config.enableStream(vsp.getType(), vsp.getIndex(), vsp.getWidth(), vsp.getHeight(), vsp.getFormat(), vsp.getFrameRate());
        }
    }

    void configAndStart() throws Exception {
        try(Config config = new Config()){
            if(mLoadConfig)
                configStream(config);
            if(mListener != null)
                mListener.config(config);
            mPipeline.start(config);
        }
    }

    public synchronized void start() throws Exception {
        if(mIsStreaming)
            return;
        try{
            mPipeline = new Pipeline();
            Log.d(TAG, "try start streaming");
            configAndStart();
            try(FrameSet frames = mPipeline.waitForFrames(3000)){} // w/a for l500
            mIsStreaming = true;
            mFwLogHandler.post(mLogger);
            mHandler.post(mStreaming);
            Log.d(TAG, "streaming started successfully");
        } catch (Exception e) {
            Log.e(TAG, "failed to start streaming");
            mPipeline.close();
            throw e;
        }
    }

    public synchronized void stop() {
        if(!mIsStreaming)
            return;
        try {
            Log.d(TAG, "try stop streaming");
            mIsStreaming = false;
            mHandler.removeCallbacks(mStreaming);
            mFwLogHandler.removeCallbacks(mLogger);
            mPipeline.stop();
            mPipeline.close();
            Log.d(TAG, "streaming stopped successfully");
        }  catch (Exception e) {
            Log.w(TAG, "failed to stop streaming");
            mPipeline = null;
        }
    }
}

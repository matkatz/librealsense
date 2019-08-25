package com.intel.realsense.camera;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Bundle;
import android.support.design.widget.FloatingActionButton;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.util.Pair;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.intel.realsense.librealsense.Config;
import com.intel.realsense.librealsense.FrameSet;
import com.intel.realsense.librealsense.GLRsSurfaceView;

import java.util.HashMap;
import java.util.Map;

import static android.view.View.GONE;

public class PreviewActivity extends AppCompatActivity {
    private static final String TAG = "librs camera preview";

    private GLRsSurfaceView mGLSurfaceView;
    private FloatingActionButton mStartRecordFab;
    private TextView mPlaybackButton;
    private TextView mSettingsButton;
    private TextView mStatisticsButton;
    private View m3dControls;
    private TextView m3dButton;
    private FloatingActionButton m3dControlRotateButton;
    private FloatingActionButton m3dControlTranslateButton;
    private FloatingActionButton m3dControlZoomOutButton;
    private FloatingActionButton m3dControlZoomInButton;

    private TextView mStatsView;
    private Map<Integer, TextView> mLabels;

    private Streamer mStreamer;
    private StreamingStats mStreamingStats;

    private boolean statsToggle = false;
    private boolean mShow3D = false;

    public synchronized void toggleStats(){
        statsToggle = !statsToggle;
        if(statsToggle){
            mGLSurfaceView.setVisibility(GONE);
            mStatsView.setVisibility(View.VISIBLE);
            mStatisticsButton.setText("Preview");
        }
        else {
            mGLSurfaceView.setVisibility(View.VISIBLE);
            mStatsView.setVisibility(GONE);
            mStatisticsButton.setText("Statistics");
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_preview);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        mGLSurfaceView = findViewById(R.id.glSurfaceView);
        mStatsView = findViewById(R.id.streaming_stats_text);
        mStartRecordFab = findViewById(R.id.start_record_fab);
        mPlaybackButton = findViewById(R.id.preview_playback_button);
        mSettingsButton = findViewById(R.id.preview_settings_button);
        mStatisticsButton = findViewById(R.id.preview_stats_button);
        m3dButton = findViewById(R.id.preview_3d_button);
        m3dControls = findViewById(R.id.preview_3d_controls);
        m3dControlRotateButton = findViewById(R.id.controls_3d_rotate_fab);
        m3dControlTranslateButton = findViewById(R.id.controls_3d_translate_fab);
        m3dControlZoomOutButton = findViewById(R.id.controls_3d_zoom_out_fab);
        m3dControlZoomInButton = findViewById(R.id.controls_3d_zoom_in_fab);

        mStartRecordFab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(PreviewActivity.this, RecordingActivity.class);
                startActivity(intent);
                finish();
            }
        });
        mPlaybackButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(PreviewActivity.this, PlaybackActivity.class);
                startActivity(intent);
            }
        });
        mSettingsButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(PreviewActivity.this, SettingsActivity.class);
                startActivity(intent);
            }
        });
        m3dButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mGLSurfaceView.setVisibility(GONE);
                mGLSurfaceView.clear();
                clearLables();
                mShow3D = !mShow3D;
                m3dButton.setTextColor(mShow3D ? Color.YELLOW : Color.WHITE);
                m3dControls.setVisibility(mShow3D ? View.VISIBLE : GONE);
                mGLSurfaceView.setVisibility(View.VISIBLE);
                SharedPreferences sharedPref = getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);
                SharedPreferences.Editor editor = sharedPref.edit();
                editor.putBoolean(getString(R.string.show_3d), mShow3D);
                editor.commit();
            }
        });
        m3dControlRotateButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mGLSurfaceView.controlRotation();
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        m3dControlRotateButton.setVisibility(View.GONE);
                        m3dControlTranslateButton.setVisibility(View.VISIBLE);
                    }
                });
            }
        });
        m3dControlTranslateButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mGLSurfaceView.controlTranslation();
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        m3dControlTranslateButton.setVisibility(View.GONE);
                        m3dControlRotateButton.setVisibility(View.VISIBLE);
                    }
                });
            }
        });
        m3dControlZoomOutButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mGLSurfaceView.zoomOut();
            }
        });
        m3dControlZoomInButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mGLSurfaceView.zoomIn();
            }
        });
        mStatisticsButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                toggleStats();
            }
        });
        SharedPreferences sharedPref = getSharedPreferences(getString(R.string.app_settings), Context.MODE_PRIVATE);
        mShow3D = sharedPref.getBoolean(getString(R.string.show_3d), false);
        m3dButton.setTextColor(mShow3D ? Color.YELLOW : Color.WHITE);
        mGLSurfaceView.controlRotation();
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                m3dControlRotateButton.setVisibility(View.GONE);
                m3dControlTranslateButton.setVisibility(View.VISIBLE);
            }
        });
    }

    private synchronized Map<Integer, TextView> createLabels(Map<Integer, Pair<String, Rect>> rects){
        if(rects == null)
            return null;
        mLabels = new HashMap<>();

        final RelativeLayout rl = findViewById(R.id.labels_layout);
        for(Map.Entry<Integer, Pair<String, Rect>> e : rects.entrySet()){
            TextView tv = new TextView(getApplicationContext());
            ViewGroup.LayoutParams lp = new RelativeLayout.LayoutParams(
                    ViewGroup.LayoutParams.WRAP_CONTENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT);
            tv.setLayoutParams(lp);
            tv.setTextColor(Color.parseColor("#ffffff"));
            tv.setTextSize(14);
            rl.addView(tv);
            mLabels.put(e.getKey(), tv);
        }
        return mLabels;
    }

    private void printLables(final Map<Integer, Pair<String, Rect>> rects){
        if(rects == null)
            return;
        final Map<Integer, String> lables = new HashMap<>();
        if(mLabels == null)
            mLabels = createLabels(rects);
        for(Map.Entry<Integer, Pair<String, Rect>> e : rects.entrySet()){
            lables.put(e.getKey(), e.getValue().first);
        }

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                for(Map.Entry<Integer,TextView> e : mLabels.entrySet()){
                    Integer uid = e.getKey();
                    if(rects.get(uid) == null)
                        continue;
                    Rect r = rects.get(uid).second;
                    TextView tv = e.getValue();
                    tv.setX(r.left);
                    tv.setY(r.top);
                    tv.setText(lables.get(uid));
                }
            }
        });
    }

    private void clearLables(){
        if(mLabels != null){
            for(Map.Entry<Integer, TextView> label : mLabels.entrySet())
                label.getValue().setVisibility(GONE);
            mLabels = null;
        }
    }

    @Override
    protected void onResume() {
        super.onResume();

        mStreamingStats  = new StreamingStats();
        mStreamer = new Streamer(this, true, new Streamer.Listener() {
            @Override
            public void config(Config config) {

            }

            @Override
            public void onFrameset(FrameSet frameSet) {
                mStreamingStats.onFrameset(frameSet);
                if(statsToggle){
                    clearLables();
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            mStatsView.setText(mStreamingStats.prettyPrint());
                        }
                    });
                }
                else{
                    mGLSurfaceView.showPointcloud(mShow3D);
                    mGLSurfaceView.upload(frameSet);
                    Map<Integer, Pair<String, Rect>> rects = mGLSurfaceView.getRectangles();
                    printLables(rects);
                }
            }
        });

        try {
            mGLSurfaceView.clear();
            mStreamer.start();
        }
        catch (Exception e){
            if(mStreamer != null)
                mStreamer.stop();
            Log.e(TAG, e.getMessage());
            Toast.makeText(this, "Failed to set streaming configuration ", Toast.LENGTH_LONG).show();
            Intent intent = new Intent(PreviewActivity.this, SettingsActivity.class);
            startActivity(intent);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();

        clearLables();
        if(mStreamer != null)
            mStreamer.stop();
        if(mGLSurfaceView != null)
            mGLSurfaceView.clear();
    }
}

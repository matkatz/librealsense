package com.intel.realsense.librealsense;

import android.content.Context;
import android.graphics.Rect;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Pair;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;

import java.util.Map;

public class GLRsSurfaceView extends GLSurfaceView implements AutoCloseable{

    private final GLRenderer mRenderer;
    private float mZoom = 1;
    private final float mZoomStep = 0.05f;
    private float mPointSize = 1;
    private final float mPointSizeStep = 0.5f;
    private float mPreviousX = 0;
    private float mPreviousY = 0;

    private boolean mControlRotation = false;
    private boolean mControlTranslation = true;

    public void increasePointSize(){
        mPointSize *= (1 + mPointSizeStep);
        mRenderer.setPointSize(mPointSize);
    }

    public void decreasePointSize(){
        mPointSize *= (1 - mPointSizeStep);
        mRenderer.setPointSize(mPointSize);
    }

    public void zoomIn(){
        mZoom *= (1 + mZoomStep);
        mRenderer.setScale(mZoom);
    }

    public void zoomOut(){
        mZoom *= (1 - mZoomStep);
        mRenderer.setScale(mZoom);
    }

    public void controlTranslation(){
        mControlTranslation = true;
        mControlRotation = false;
    }

    public void controlRotation(){
        mControlTranslation = false;
        mControlRotation = true;
    }

    public GLRsSurfaceView(Context context) {
        super(context);
        mRenderer = new GLRenderer();
        setRenderer(mRenderer);
    }

    public GLRsSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mRenderer = new GLRenderer();
        setRenderer(mRenderer);
    }

    public Map<Integer, Pair<String,Rect>> getRectangles() {
        return mRenderer.getRectangles();
    }

    public void upload(FrameSet frames) {
        mRenderer.upload(frames);
    }

    public void upload(Frame frame) {
        mRenderer.upload(frame);
    }

    public void clear() {
        mRenderer.clear();
    }

    @Override
    public boolean onTouchEvent(MotionEvent e) {
        float x = e.getX();
        float y = e.getY();

        switch (e.getAction()) {
            case MotionEvent.ACTION_MOVE:

                float dx = x - mPreviousX;
                float dy = y - mPreviousY;
                if(mControlRotation)
                    mRenderer.rotate(dx, dy);
                if(mControlTranslation)
                    mRenderer.translate(dx, dy);
        }

        mPreviousX = x;
        mPreviousY = y;
        return true;
    }

    public void showPointcloud(boolean showPoints) {
        mRenderer.showPointcloud(showPoints);
    }

    @Override
    public void close() {
        if(mRenderer != null)
            mRenderer.close();
    }
}

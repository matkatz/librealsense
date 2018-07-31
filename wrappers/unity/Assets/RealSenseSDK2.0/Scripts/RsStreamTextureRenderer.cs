﻿using Intel.RealSense;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Threading;
using UnityEngine;
using UnityEngine.Events;

public class RsStreamTextureRenderer : MonoBehaviour
{
    private static TextureFormat Convert(Format lrsFormat)
    {
        switch (lrsFormat)
        {
            case Format.Z16: return TextureFormat.R16;
            case Format.Disparity16: return TextureFormat.R16;
            case Format.Rgb8: return TextureFormat.RGB24;
            case Format.Rgba8: return TextureFormat.RGBA32;
            case Format.Bgra8: return TextureFormat.BGRA32;
            case Format.Y8: return TextureFormat.Alpha8;
            case Format.Y16: return TextureFormat.R16;
            case Format.Raw16: return TextureFormat.R16;
            case Format.Raw8: return TextureFormat.Alpha8;
            case Format.Yuyv:
            case Format.Bgr8:
            case Format.Raw10:
            case Format.Xyz32f:
            case Format.Uyvy:
            case Format.MotionRaw:
            case Format.MotionXyz32f:
            case Format.GpioRaw: 
            case Format.Any: 
            default:
                throw new Exception(string.Format("{0} librealsense format: " + lrsFormat + ", is not supported by Unity"));
        }
    }

    [System.Serializable]
    public class TextureEvent : UnityEvent<Texture> { }

    public Stream _stream;
    public Format _format;
    public int _streamIndex;

    public FilterMode filterMode = FilterMode.Point;

    [SerializeField]
    private RsDevice _realSenseDevice;
    public RsDevice RealSenseDevice
    {
        get
        {
            if (_realSenseDevice == null)
            {
                _realSenseDevice = FindObjectOfType<RsDevice>();
            }
            UnityEngine.Assertions.Assert.IsNotNull(_realSenseDevice);
            return _realSenseDevice;
        }
        set
        {
            _realSenseDevice = value;
        }
    }

    private RsVideoStreamRequest _videoStreamFilter;
    private RsVideoStreamRequest _currVideoStreamFilter;

    public Texture2D Texture { get; protected set; }

    [Space]
    public TextureEvent textureBinding;

    [System.NonSerialized]
    private byte[] data;

    readonly AutoResetEvent f = new AutoResetEvent(false);
    protected int threadId;
    protected bool bound;

    virtual protected void Awake()
    {
        threadId = Thread.CurrentThread.ManagedThreadId;
        _videoStreamFilter = new RsVideoStreamRequest() { Stream = _stream, Format = _format, StreamIndex = _streamIndex };
        _currVideoStreamFilter = _videoStreamFilter.Clone();
    }

    void Start()
    {
        RealSenseDevice.OnStart += OnStartStreaming;
        RealSenseDevice.OnStop += OnStopStreaming;
    }

    void OnDestroy()
    {
        if (Texture != null) {
            Destroy(Texture);
            Texture = null;
        }
    }

    protected virtual void OnStopStreaming()
    {
        RealSenseDevice.OnNewSample -= OnNewSampleUnityThread;
        RealSenseDevice.OnNewSample -= OnNewSampleThreading;

        f.Reset();
        data = null;
    }

    protected virtual void OnStartStreaming(PipelineProfile activeProfile)
    {

        if (RealSenseDevice.processMode == RsDevice.ProcessMode.UnityThread)
        {
            UnityEngine.Assertions.Assert.AreEqual(threadId, Thread.CurrentThread.ManagedThreadId);
            RealSenseDevice.OnNewSample += OnNewSampleUnityThread;
        }
        else
            RealSenseDevice.OnNewSample += OnNewSampleThreading;
    }

    public void OnFrame(Frame f)
    {
        if (RealSenseDevice.processMode == RsDevice.ProcessMode.UnityThread)
        {
            UnityEngine.Assertions.Assert.AreEqual(threadId, Thread.CurrentThread.ManagedThreadId);
            OnNewSampleUnityThread(f);
        }
        else
        {
            OnNewSampleThreading(f);
        }
    }

    private void UpdateData(Frame frame)
    {
        var vidFrame = frame as VideoFrame;
        data = data ?? new byte[vidFrame.Stride * vidFrame.Height];
        vidFrame.CopyTo(data);
        if ((vidFrame as Frame) != frame)
            vidFrame.Dispose();
    }

    private void ResetTexture(RsVideoStreamRequest vsr)
    {
        if (Texture != null)
        {
            Destroy(Texture);
        }

        Texture = new Texture2D(vsr.Width, vsr.Height, Convert(vsr.Format), false, true)
        {
            wrapMode = TextureWrapMode.Clamp,
            filterMode = filterMode
        };

        _currVideoStreamFilter = vsr.Clone();

        Texture.Apply();
        textureBinding.Invoke(Texture);
    }

    private bool HasTextureConflict(Frame frame)
    {
        var vidFrame = frame as VideoFrame;
        if (_videoStreamFilter.Width == vidFrame.Width && _videoStreamFilter.Height == vidFrame.Height && _videoStreamFilter.Format == vidFrame.Profile.Format)
            return false;
        _videoStreamFilter.CopyProfile(vidFrame);
        data = null;
        return true;
    }

    private bool HasRequestConflict(Frame frame)
    {
        if (!(frame is VideoFrame))
            return true;
        VideoFrame vf = frame as VideoFrame;
        if (_videoStreamFilter.Stream != vf.Profile.Stream ||
            _videoStreamFilter.Format != vf.Profile.Format ||
            (_videoStreamFilter.StreamIndex != vf.Profile.Index && _videoStreamFilter.StreamIndex != 0))
            return true;
        return false;
    }

    private void OnNewSampleThreading(Frame frame)
    {
        if (HasRequestConflict(frame))
            return;
        if (HasTextureConflict(frame))
            return;
        UpdateData(frame);
        f.Set();
    }

    private void OnNewSampleUnityThread(Frame frame)
    {
        var vidFrame = frame as VideoFrame;

        if (HasRequestConflict(vidFrame))
            return;
        if (HasTextureConflict(frame))
            return;

        UnityEngine.Assertions.Assert.AreEqual(threadId, Thread.CurrentThread.ManagedThreadId);

        Texture.LoadRawTextureData(vidFrame.Data, vidFrame.Stride * vidFrame.Height);

        if ((vidFrame as Frame) != frame)
            vidFrame.Dispose();

        // Applied later (in Update) to sync all gpu uploads
        // texture.Apply();
        f.Set();
    }

    protected void Update()
    {
        if (!_currVideoStreamFilter.Equals(_videoStreamFilter))
            ResetTexture(_videoStreamFilter);

        if (f.WaitOne(0))
        {
            try
            {
                if (data != null)
                    Texture.LoadRawTextureData(data);
            }
            catch
            {
                OnStopStreaming();
                Debug.LogError("Error loading texture data, check texture and stream formats");
                throw;
            }
            Texture.Apply();

            if (!bound)
            {
                textureBinding.Invoke(Texture);
                bound = true;
            }
        }
    }
}


public class DefaultStreamAttribute : Attribute
{
    public Stream stream;
    public TextureFormat format;

    public DefaultStreamAttribute(Stream stream)
    {
        this.stream = stream;
    }

    public DefaultStreamAttribute(Stream stream, TextureFormat format)
    {
        this.stream = stream;
        this.format = format;
    }
}
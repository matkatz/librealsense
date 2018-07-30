using System.Collections.Generic;
using UnityEngine;
using Intel.RealSense;
using System;
using System.Collections;
using System.Linq;

public class RsDeviceInspector : MonoBehaviour
{
    public bool streaming;
    public Device device;
    public StreamProfileList streams;
    public readonly Dictionary<string, Sensor> sensors = new Dictionary<string, Sensor>();
    public readonly Dictionary<string, List<Sensor.CameraOption>> sensorOptions = new Dictionary<string, List<Sensor.CameraOption>>();

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

    void Awake()
    {
        StartCoroutine(WaitForDevice());
    }

    private IEnumerator WaitForDevice()
    {
        yield return new WaitUntil(() => RealSenseDevice != null);
        RealSenseDevice.OnStart += onStartStreaming;
        RealSenseDevice.OnStop += onStopStreaming;

        if(RealSenseDevice.Streaming)
            onStartStreaming(RealSenseDevice.ActiveProfile);
    }

    private void onStopStreaming()
    {
        streaming = false;

        if (device != null)
        {
            device.Dispose();
            device = null;
        }

        if (streams != null)
        {
            streams.Dispose();
            streams = null;
        }

        foreach (var s in sensors)
        {
            var sensor = s.Value;
            if (sensor != null)
                sensor.Dispose();
        }
        sensors.Clear();
        sensorOptions.Clear();
    }

    private void onStartStreaming(PipelineProfile profile)
    {
        device = profile.Device;
        streams = profile.Streams;
        using (var sensorList = device.Sensors)
        {
            foreach (var s in sensorList)
            {
                var sensorName = s.Info[CameraInfo.Name];
                sensors.Add(sensorName, s);
                sensorOptions.Add(sensorName, s.Options.ToList());
            }
        }
        streaming = true;
    }
}
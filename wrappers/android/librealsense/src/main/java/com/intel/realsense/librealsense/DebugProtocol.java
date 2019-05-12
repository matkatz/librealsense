package com.intel.realsense.librealsense;

public class DebugProtocol extends Device {
    DebugProtocol(long handle) {
        super(handle);
    }

    public String SendAndReceiveRawData(String command){
        return nSendRawData(mHandle, command);
    }

    public byte[] SendAndReceiveRawData(byte[] buffer){
        return nSendAndReceiveRawData(mHandle, buffer);
    }

    private static native byte[] nSendAndReceiveRawData(long handle, byte[] buffer);
    private static native String nSendRawData(long handle, String command);
}

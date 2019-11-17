package com.intel.realsense.librealsense;

public class DebugProtocol extends Device {
    DebugProtocol(long handle) {
        super(handle);
    }

    public byte[] SendAndReceiveRawData(HWCommand command){
        byte val = command.value();
        byte[] buff = {0x14, 0x00, (byte) 0xab, (byte) 0xcd, val, 0x00, 0x00, 0x00,
                (byte) 0xf4, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        return SendAndReceiveRawData(buff);
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

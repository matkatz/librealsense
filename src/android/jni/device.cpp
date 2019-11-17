// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include <memory>
#include <vector>
#include "error.h"

#include "../../../include/librealsense2/rs.h"
#include "../../../include/librealsense2/hpp/rs_device.hpp"
#include "../../api.h"
#include "../../../common/parser.hpp"


extern "C" JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_Device_nSupportsInfo(JNIEnv *env, jclass type, jlong handle,
                                                           jint info) {
    rs2_error *e = NULL;
    auto rv = rs2_supports_device_info(reinterpret_cast<const rs2_device *>(handle),
                                       static_cast<rs2_camera_info>(info), &e);
    handle_error(env, e);
    return rv > 0;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_intel_realsense_librealsense_Device_nGetInfo(JNIEnv *env, jclass type, jlong handle,
                                                      jint info) {
    rs2_error *e = NULL;
    const char* rv = rs2_get_device_info(reinterpret_cast<const rs2_device *>(handle),
                                         static_cast<rs2_camera_info>(info), &e);
    handle_error(env, e);
    if (NULL == rv)
        rv = "";
    return env->NewStringUTF(rv);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Device_nRelease(JNIEnv *env, jclass type, jlong handle) {
    rs2_delete_device(reinterpret_cast<rs2_device *>(handle));
}

extern "C"
JNIEXPORT jlongArray JNICALL
Java_com_intel_realsense_librealsense_Device_nQuerySensors(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = nullptr;
    std::shared_ptr<rs2_sensor_list> list(
            rs2_query_sensors(reinterpret_cast<const rs2_device *>(handle), &e),
            rs2_delete_sensor_list);
    handle_error(env, e);

    auto size = rs2_get_sensors_count(list.get(), &e);
    handle_error(env, e);

    std::vector<rs2_sensor *> sensors;
    for (auto i = 0; i < size; i++) {
        auto s = rs2_create_sensor(list.get(), i, &e);
        handle_error(env, e);
        sensors.push_back(s);
    }
    jlongArray rv = env->NewLongArray(sensors.size());
    env->SetLongArrayRegion(rv, 0, sensors.size(), reinterpret_cast<const jlong *>(sensors.data()));
    return rv;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Updatable_nEnterUpdateState(JNIEnv *env, jclass type,
                                                                  jlong handle) {
    rs2_error *e = NULL;
    rs2_enter_update_state(reinterpret_cast<const rs2_device *>(handle), &e);
    handle_error(env, e);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Updatable_nUpdateFirmwareUnsigned(JNIEnv *env,
                                                                        jobject instance,
                                                                        jlong handle,
                                                                        jbyteArray image_,
                                                                        jint update_mode) {
    jbyte *image = env->GetByteArrayElements(image_, NULL);
    auto length = env->GetArrayLength(image_);
    rs2_error *e = NULL;
    jclass cls = env->GetObjectClass(instance);
    jmethodID id = env->GetMethodID(cls, "onProgress", "(F)V");
    auto cb = [&](float progress){ env->CallVoidMethod(instance, id, progress); };
    rs2_update_firmware_unsigned_cpp(reinterpret_cast<const rs2_device *>(handle), image, length,
                                     new rs2::update_progress_callback<decltype(cb)>(cb), update_mode, &e);
    handle_error(env, e);
    env->ReleaseByteArrayElements(image_, image, 0);
}

extern "C"
JNIEXPORT jbyteArray JNICALL
Java_com_intel_realsense_librealsense_Updatable_nCreateFlashBackup(JNIEnv *env, jobject instance,
                                                                   jlong handle) {
    rs2_error* e = NULL;
    jclass cls = env->GetObjectClass(instance);
    jmethodID id = env->GetMethodID(cls, "onProgress", "(F)V");
    auto cb = [&](float progress){ env->CallVoidMethod(instance, id, progress); };

    std::shared_ptr<const rs2_raw_data_buffer> raw_data_buffer(
            rs2_create_flash_backup_cpp(reinterpret_cast<rs2_device *>(handle), new rs2::update_progress_callback<decltype(cb)>(cb), &e),
            [](const rs2_raw_data_buffer* buff){ if(buff) delete buff;});
    handle_error(env, e);

    jbyteArray rv = env->NewByteArray(raw_data_buffer->buffer.size());
    env->SetByteArrayRegion(rv, 0, raw_data_buffer->buffer.size(),
        reinterpret_cast<const jbyte *>(raw_data_buffer->buffer.data()));
    return rv;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_UpdateDevice_nUpdateFirmware(JNIEnv *env, jobject instance,
                                                                   jlong handle,
                                                                   jbyteArray image_) {
    jbyte *image = env->GetByteArrayElements(image_, NULL);
    auto length = env->GetArrayLength(image_);
    rs2_error *e = NULL;
    jclass cls = env->GetObjectClass(instance);
    jmethodID id = env->GetMethodID(cls, "onProgress", "(F)V");
    auto cb = [&](float progress){ env->CallVoidMethod(instance, id, progress); };
    rs2_update_firmware_cpp(reinterpret_cast<const rs2_device *>(handle), image, length,
                            new rs2::update_progress_callback<decltype(cb)>(cb), &e);
    handle_error(env, e);
    env->ReleaseByteArrayElements(image_, image, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Device_nHardwareReset(JNIEnv *env, jclass type,
                                                            jlong handle) {
    rs2_error *e = NULL;
    rs2_hardware_reset(reinterpret_cast<const rs2_device *>(handle), &e);
    handle_error(env, e);
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_Device_nIsDeviceExtendableTo(JNIEnv *env, jclass type,
                                                                   jlong handle, jint extension) {
    rs2_error *e = NULL;
    int rv = rs2_is_device_extendable_to(reinterpret_cast<const rs2_device *>(handle),
                                         static_cast<rs2_extension>(extension), &e);
    handle_error(env, e);
    return rv > 0;
}

std::vector<uint8_t> send_and_receive_raw_data(JNIEnv *env, rs2_device * dev, const std::vector<uint8_t>& input)
{
    std::vector<uint8_t> results;

    rs2_error *e = NULL;
    std::shared_ptr<const rs2_raw_data_buffer> list(
            rs2_send_and_receive_raw_data(dev, (void*)input.data(), (uint32_t)input.size(), &e),
            rs2_delete_raw_data);
    handle_error(env, e);

    auto size = rs2_get_raw_data_size(list.get(), &e);
    handle_error(env, e);

    auto start = rs2_get_raw_data(list.get(), &e);

    results.insert(results.begin(), start, start + size);

    return results;
}

std::string hex_mode(JNIEnv *env, rs2_device * dev, const std::string& line)
{
    std::vector<uint8_t> raw_data;
    std::stringstream ss(line);
    std::string word;
    while (ss >> word)
    {
        std::stringstream converter;
        int temp;
        converter << std::hex << word;
        converter >> temp;
        raw_data.push_back(temp);
    }
    if (raw_data.empty())
        throw std::runtime_error("Wrong input!");

    auto result = send_and_receive_raw_data(env, dev, raw_data);

    std::stringstream rv;
    for (auto& elem : result)
        rv << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(elem) << " ";

    return rv.str();
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_intel_realsense_librealsense_DebugProtocol_nSendRawData(JNIEnv *env, jclass type,
                                                                 jlong handle, jstring command_) {
    const char *command = env->GetStringUTFChars(command_, 0);

    const std::string &returnValue = hex_mode(env, reinterpret_cast<rs2_device *>(handle), command);

    env->ReleaseStringUTFChars(command_, command);

    return env->NewStringUTF(returnValue.c_str());
}

extern "C"
JNIEXPORT jbyteArray JNICALL
Java_com_intel_realsense_librealsense_DebugProtocol_nSendAndReceiveRawData(JNIEnv *env, jclass type,
                                                                           jlong handle,
                                                                           jbyteArray buffer_) {
    jbyte *buffer = env->GetByteArrayElements(buffer_, NULL);
    jsize length = env->GetArrayLength(buffer_);
    std::vector<uint8_t> buff(reinterpret_cast<uint8_t*>(buffer), reinterpret_cast<uint8_t*>(buffer) + length);
    auto ret = send_and_receive_raw_data(env, reinterpret_cast<rs2_device *>(handle), buff);
    env->ReleaseByteArrayElements(buffer_, buffer, 0);
    jbyteArray rv = env->NewByteArray(ret.size());
    env->SetByteArrayRegion (rv, 0, ret.size(), reinterpret_cast<const jbyte *>(ret.data()));
    return rv;
}
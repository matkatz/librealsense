// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include "error.h"
#include "../../../include/librealsense2/rs.h"
#include "../fw-logger/rs-fw-logger.h"

#include "../../../include/librealsense2/hpp/rs_context.hpp"
#include "../../../include/librealsense2/hpp/rs_pipeline.hpp"

#include <memory>
#include <android/log.h>

#define TAG "rs_fw_log"

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_RsContext_nCreate(JNIEnv *env, jclass type) {
    rs2_error* e = NULL;
    rs2_context* handle = rs2_create_context(RS2_API_VERSION, &e);
    handle_error(env, e);
    return (jlong) handle;
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_RsContext_nDelete(JNIEnv *env, jclass type, jlong handle) {
    rs2_delete_context((rs2_context *) handle);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_RsContext_nQueryDevices(JNIEnv *env, jclass type,
                                                              jlong handle, jint mask) {
    rs2_error* e = NULL;
    rs2_device_list* device_list_handle = rs2_query_devices_ex((rs2_context *) handle, mask, &e);
    handle_error(env, e);
    return (jlong) device_list_handle;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_intel_realsense_librealsense_RsContext_nGetVersion(JNIEnv *env, jclass type) {
    return env->NewStringUTF(RS2_API_VERSION_STR);
}

void logi(std::string message)
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "%s", message.c_str());
}

std::shared_ptr<android_fw_logger> g_fw_logger;

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_RsContext_nStartReadingFwLogs(JNIEnv *env, jclass clazz,
                                                                    jstring file_path) {
    const char *filePath = env->GetStringUTFChars(file_path, 0);
    g_fw_logger = std::make_shared<android_fw_logger>(filePath);

    env->ReleaseStringUTFChars(file_path, filePath);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_RsContext_nStopReadingFwLogs(JNIEnv *env, jclass clazz) {
    g_fw_logger.reset();
}

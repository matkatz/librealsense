/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

/** \file rs_pipeline.h
* \brief
* Exposes RealSense processing-block functionality for C compilers
*/


#ifndef LIBREALSENSE_RS2_STREAMER_H
#define LIBREALSENSE_RS2_STREAMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rs_types.h"
#include "rs_sensor.h"
#include "rs_config.h"

void rs2_streamer_stop(rs2_streamer* streamer, rs2_error ** error);

void rs2_delete_streamer(rs2_streamer* streamer);

rs2_pipeline_profile* rs2_streamer_start(rs2_streamer* streamer, rs2_error ** error);

rs2_pipeline_profile* rs2_streamer_start_with_config(rs2_streamer* streamer, rs2_config* config, rs2_error ** error);

rs2_pipeline_profile* rs2_streamer_get_active_profile(rs2_streamer* streamer, rs2_error ** error);

rs2_device* rs2_streamer_profile_get_device(rs2_pipeline_profile* profile, rs2_error ** error);

rs2_stream_profile_list* rs2_streamer_profile_get_streams(rs2_pipeline_profile* profile, rs2_error** error);

void rs2_delete_streamer_profile(rs2_pipeline_profile* profile);

// async streamer
rs2_streamer* rs2_create_async_streamer(rs2_context* ctx, rs2_error ** error);

void rs2_async_streamer_set_callbak(rs2_streamer* streamer, rs2_frame_callback* on_frame, rs2_stream stream, int index, rs2_error** error);

// sync streamer
rs2_streamer* rs2_create_sync_streamer(rs2_context* ctx, rs2_error ** error);

rs2_frame* rs2_sync_streamer_wait_for_frames(rs2_sync_streamer* streamer, unsigned int timeout_ms, rs2_error ** error);

int rs2_sync_streamer_poll_for_frames(rs2_streamer* streamer, rs2_frame** output_frame, rs2_error ** error);

int rs2_sync_streamer_try_wait_for_frames(rs2_sync_streamer* streamer, rs2_frame** output_frame, unsigned int timeout_ms, rs2_error ** error);
#ifdef __cplusplus
}
#endif
#endif

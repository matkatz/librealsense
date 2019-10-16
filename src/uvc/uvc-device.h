// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#include "../backend.h"
#include "uvc-types.h"
#include "../usb/usb-messenger.h"
#include "../usb/usb-enumerator.h"
#include "../concurrency.h"
#include "../types.h"
#include "uvc-parser.h"
#include "uvc-streamer.h"

#include "stdio.h"
#include "stdlib.h"
#include <cstring>
#include <string>
#include <chrono>
#include <thread>

/** Converts an unaligned one-byte integer into an int8 */
#define B_TO_BYTE(p) ((int8_t)(p)[0])

/** Converts an unaligned two-byte little-endian integer into an int16 */
#define SW_TO_SHORT(p) ((uint8_t)(p)[0] | \
                       ((int8_t)(p)[1] << 8))

/** Converts an unaligned four-byte little-endian integer into an int32 */
#define DW_TO_INT(p) ((uint8_t)(p)[0] | \
                     ((uint8_t)(p)[1] << 8) | \
                     ((uint8_t)(p)[2] << 16) | \
                     ((int8_t)(p)[3] << 24))

/** Converts an unaligned eight-byte little-endian integer into an int64 */
#define QW_TO_QUAD(p) (((uint64_t)(p)[0]) | \
                      (((uint64_t)(p)[1]) << 8) | \
                      (((uint64_t)(p)[2]) << 16) | \
                      (((uint64_t)(p)[3]) << 24) | \
                      (((uint64_t)(p)[4]) << 32) | \
                      (((uint64_t)(p)[5]) << 40) | \
                      (((uint64_t)(p)[6]) << 48) | \
                      (((int64_t)(p)[7]) << 56))


/** Converts an int16 into an unaligned two-byte little-endian integer */
#define SHORT_TO_SW(s, p) \
  (p)[0] = (uint8_t)(s); \
  (p)[1] = (uint8_t)((s) >> 8);

/** Converts an int32 into an unaligned four-byte little-endian integer */
#define INT_TO_DW(i, p) \
  (p)[0] = (uint8_t)(i); \
  (p)[1] = (uint8_t)((i) >> 8); \
  (p)[2] = (uint8_t)((i) >> 16); \
  (p)[3] = (uint8_t)((i) >> 24);

/** Converts an int64 into an unaligned eight-byte little-endian integer */
#define QUAD_TO_QW(i, p) \
  (p)[0] = (uint8_t)(i); \
  (p)[1] = (uint8_t)((i) >> 8); \
  (p)[2] = (uint8_t)((i) >> 16); \
  (p)[3] = (uint8_t)((i) >> 24); \
  (p)[4] = (uint8_t)((i) >> 32); \
  (p)[5] = (uint8_t)((i) >> 40); \
  (p)[6] = (uint8_t)((i) >> 48); \
  (p)[7] = (uint8_t)((i) >> 56); \

typedef void(uvc_frame_callback_t)(struct librealsense::platform::frame_object *frame, void *user_ptr);

namespace librealsense
{
    namespace platform
    {
        std::vector<uvc_device_info> query_uvc_devices_info();
        std::shared_ptr<uvc_device> create_rsuvc_device(uvc_device_info info);

        struct profile_and_callback
        {
            stream_profile profile;
            frame_callback callback = nullptr;
        };

        class rs_uvc_device : public uvc_device
        {
        public:
            rs_uvc_device(const rs_usb_device& usb_device, const uvc_device_info &info, uint8_t usb_request_count = 2);
            virtual ~rs_uvc_device();

            virtual void probe_and_commit(stream_profile profile, frame_callback callback, int buffers = DEFAULT_V4L2_FRAME_BUFFERS) override;
            virtual void stream_on(std::function<void(const notification& n)> error_handler = [](const notification& n){}) override;
            virtual void start_callbacks() override;
            virtual void stop_callbacks() override;
            virtual void close(stream_profile profile) override;

            virtual void set_power_state(power_state state) override;
            virtual power_state get_power_state() const override;

            virtual void init_xu(const extension_unit& xu) override;
            virtual bool set_xu(const extension_unit& xu, uint8_t ctrl, const uint8_t* data, int len) override;
            virtual bool get_xu(const extension_unit& xu, uint8_t ctrl, uint8_t* data, int len) const override;
            virtual control_range get_xu_range(const extension_unit& xu, uint8_t ctrl, int len) const override;

            virtual bool get_pu(rs2_option opt, int32_t& value) const override;
            virtual bool set_pu(rs2_option opt, int32_t value) override;
            virtual control_range get_pu_range(rs2_option opt) const override;

            virtual std::vector<stream_profile> get_profiles() const override;

            virtual void lock() const override;
            virtual void unlock() const override;

            virtual std::string get_device_location() const override;
            virtual usb_spec  get_usb_specification() const override;

        private:
            friend class source_reader_callback;

            void close_uvc_device();

            usb_status probe_stream_ctrl(const std::shared_ptr<uvc_stream_ctrl_t>& control);
            usb_status get_stream_ctrl_format_size(uvc_format_t format, const std::shared_ptr<uvc_stream_ctrl_t>& control);
            usb_status query_stream_ctrl(const std::shared_ptr<uvc_stream_ctrl_t>& control, uint8_t probe, int req);
            std::vector<uvc_format_t> get_available_formats_all() const;

            bool uvc_get_ctrl(uint8_t unit, uint8_t ctrl, void *data, int len, uvc_req_code req_code) const;
            bool uvc_set_ctrl(uint8_t unit, uint8_t ctrl, void *data, int len);

            int32_t rs2_value_translate(uvc_req_code action, rs2_option option, int32_t value) const;
            void play_profile(stream_profile profile, frame_callback callback);
            void stop_stream_cleanup(const stream_profile& profile, std::vector<profile_and_callback>::iterator& elem);
            void check_connection() const;

            int rs2_option_to_ctrl_selector(rs2_option option, int &unit) const;
            int32_t get_data_usb(uvc_req_code action, int control, int unit, unsigned int length = sizeof(uint32_t)) const;
            void set_data_usb(uvc_req_code action, int control, int unit, int value) const;

            void listen_to_interrupts();

            const uvc_device_info                   _info;
            power_state                             _power_state = D3; // power state change is unsupported

            std::vector<profile_and_callback>       _streams;

            std::string                             _location;
            std::vector<stream_profile>             _profiles;
            std::vector<frame_callback>             _frame_callbacks;

            rs_usb_device                           _usb_device = nullptr;
            rs_usb_messenger                        _messenger;
            rs_usb_request                          _interrupt_request;
            rs_usb_request_callback                 _interrupt_callback;
            uint8_t                                 _usb_request_count;

            mutable std::mutex                      _power_mutex;

            // uvc internal
            std::shared_ptr<uvc_parser>             _parser;
            std::vector<std::shared_ptr<uvc_streamer>> _streamers;
        };
    }
}

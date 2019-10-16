// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#include "uvc-types.h"
#include "../types.h"

#include "stdio.h"
#include "stdlib.h"
#include <cstring>
#include <string>
#include <chrono>
#include <thread>

typedef void(uvc_frame_callback_t)(struct librealsense::platform::frame_object *frame, void *user_ptr);

namespace librealsense
{
    namespace platform
    {
        struct uvc_streamer_context
        {
            stream_profile profile;
            frame_callback user_cb;
            std::shared_ptr<uvc_stream_ctrl_t> control;
            rs_usb_device usb_device;
            rs_usb_messenger messenger;
            uint8_t request_count;
        };

        class watchdog
        {
        public:
            watchdog(std::function<void()> operation, uint64_t timeout_ms) :
                    _operation(std::move(operation)), _timeout_ms(timeout_ms)
            {
                _watcher = std::make_shared<active_object<>>([this](dispatcher::cancellable_timer cancellable_timer)
                           {
                                if(cancellable_timer.try_sleep(_timeout_ms))
                                {
                                    if(!_kicked)
                                        _operation();

                                    std::lock_guard<std::mutex> lk(_m);
                                    _kicked = false;
                                }
                           });
            }

            ~watchdog()
            {
                stop();
            }

            void start() { std::lock_guard<std::mutex> lk(_m); _watcher->start(); _running = true; }
            void stop() { { std::lock_guard<std::mutex> lk(_m); _running = false; } _watcher->stop(); }
            bool running() { std::lock_guard<std::mutex> lk(_m); return _running; }
            void set_timeout(uint64_t timeout_ms) { std::lock_guard<std::mutex> lk(_m); _timeout_ms = timeout_ms; }
            void kick() { std::lock_guard<std::mutex> lk(_m); _kicked = true; }


        private:
            std::mutex _m;
            uint64_t _timeout_ms;
            bool _kicked = false;
            bool _running = false;
            bool _blocker = true;
            std::function<void()> _operation;
            std::shared_ptr<active_object<>> _watcher;
        };

        class uvc_streamer
        {
        public:
            uvc_streamer(uvc_streamer_context context);
            virtual ~uvc_streamer();

            const uvc_streamer_context get_context() { return _context; }

            bool running() { return _running; }
            void start();
            void stop();

        private:
            bool _running = false;
            std::mutex _mutex;
            int64_t _watchdog_timeout;
            uvc_streamer_context _context;

            std::shared_ptr<watchdog> _watchdog;
            uint32_t _read_buff_length;
            backend_frames_queue _queue;
            rs_usb_endpoint _read_endpoint;
            std::vector<rs_usb_request> _requests;
            std::shared_ptr<backend_frames_archive> _frames_archive;
            std::shared_ptr<active_object<>> _publish_frame_thread;
            std::shared_ptr<platform::usb_request_callback> _request_callback;

            void init();
            void flush();
        };
    }
}

// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>
#include <utility>

#include "device_hub.h"
#include "sync.h"
#include "profile.h"
#include "config.h"
#include "resolver.h"

namespace librealsense
{
    namespace frame_streamer
    {
        class streamer : public std::enable_shared_from_this<streamer>
        {
        public:
            //Top level API
            explicit streamer(std::shared_ptr<librealsense::context> ctx);
            ~streamer();
            std::shared_ptr<profile> start(std::shared_ptr<config> conf);
            std::shared_ptr<profile> start_with_record(std::shared_ptr<config> conf, const std::string& file);
            void stop();
            std::shared_ptr<profile> get_active_profile() const;
            void set_callback(rs2_stream stream, int index, frame_callback_ptr callback) { _streams_callbacks[{stream, index}] = callback; }
            //Non top level API
            std::shared_ptr<device_interface> wait_for_device(const std::chrono::milliseconds& timeout = std::chrono::hours::max(),
                const std::string& serial = "");
            std::shared_ptr<librealsense::context> get_context() const;


        protected:
            virtual frame_callback_ptr get_callback();
            virtual void on_start(std::shared_ptr<profile> profile) {}

            void unsafe_start(std::shared_ptr<config> conf);
            void unsafe_stop();
            std::shared_ptr<profile> unsafe_get_active_profile() const;

            mutable std::mutex _mtx;
            std::shared_ptr<profile> _active_profile;
            device_hub _hub;
            std::shared_ptr<config> _prev_conf;

        private:
            std::shared_ptr<librealsense::context> _ctx;
            int _playback_stopped_token = -1;
            dispatcher _dispatcher;
            std::map<std::pair<rs2_stream, int>, frame_callback_ptr> _streams_callbacks;
        };
    }
}

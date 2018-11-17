// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "streamer.h"

namespace librealsense
{
    namespace frame_streamer
    {
        class async_streamer : public streamer
        {
        public:
            //Top level API
            explicit async_streamer(std::shared_ptr<librealsense::context> ctx);
            virtual ~async_streamer() = default;
            void set_callback(rs2_stream stream, int index, frame_callback_ptr callback);

        protected:
            frame_callback_ptr get_callback() override;

        private:
            bool get_callback(std::shared_ptr<stream_profile_interface> s, frame_callback_ptr& out);

            std::map<std::pair<rs2_stream, int>, frame_callback_ptr> _streams_callbacks;
        };
    }
}

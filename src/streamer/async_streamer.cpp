// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <algorithm>
#include "async_streamer.h"
#include "stream.h"
#include "media/record/record_device.h"
#include "media/ros/ros_writer.h"

namespace librealsense
{
    namespace frame_streamer
    {
        async_streamer::async_streamer(std::shared_ptr<librealsense::context> ctx) : streamer(ctx) {}

        void async_streamer::set_callback(rs2_stream stream, int index, frame_callback_ptr callback)
        {
            _streams_callbacks[{stream, index}] = callback; 
        }


        frame_callback_ptr async_streamer::get_callback()
        {
            auto to_callback = [&](frame_holder fref)
            {
                auto s = fref.frame->get_stream();
                auto it = _streams_callbacks.find({s->get_stream_type(), s->get_stream_index()});
                if (it == _streams_callbacks.end())
                    throw librealsense::invalid_value_exception("no callback was set for the requested profile");
                it->second->on_frame((rs2_frame*)fref.frame);
            };

            frame_callback_ptr rv = {
                new internal_frame_callback<decltype(to_callback)>(to_callback),
                [](rs2_frame_callback* p) { p->release(); }
            };

            return rv;
        }
    } 
}

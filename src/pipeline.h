// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>
#include <utility>

#include "device_hub.h"
#include "sync.h"
#include "streamer/config.h"
#include "streamer/profile.h"
#include "streamer/streamer.h"
namespace librealsense
{
    class processing_block;
    class pipeline_processing_block : public processing_block
    {
        std::mutex _mutex;
        std::map<stream_id, frame_holder> _last_set;
        std::unique_ptr<single_consumer_frame_queue<frame_holder>> _queue;
        std::vector<int> _streams_ids;
        void handle_frame(frame_holder frame, synthetic_source_interface* source);
    public:
        pipeline_processing_block(const std::vector<int>& streams_to_aggregate);
        bool dequeue(frame_holder* item, unsigned int timeout_ms = 5000);
        bool try_dequeue(frame_holder* item);
    };

    class pipeline : public frame_streamer::streamer
    {
    public:
        explicit pipeline(std::shared_ptr<librealsense::context> ctx);
        ~pipeline();

        //Top level API
        frame_holder wait_for_frames(unsigned int timeout_ms = 5000);
        bool poll_for_frames(frame_holder* frame);
        bool try_wait_for_frames(frame_holder* frame, unsigned int timeout_ms);

    protected:
        frame_callback_ptr get_callback() override;
        void on_start(std::shared_ptr<frame_streamer::profile> profile) override;
    private:
        std::unique_ptr<syncer_process_unit> _syncer;
        std::unique_ptr<pipeline_processing_block> _pipeline_process;
    };
}

// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <algorithm>
#include "sync_streamer.h"
#include "stream.h"

namespace librealsense
{
    namespace frame_streamer
    {
        pipeline_processing_block::pipeline_processing_block(const std::vector<int>& streams_to_aggregate) :
            _queue(new single_consumer_frame_queue<frame_holder>(1)),
            _streams_ids(streams_to_aggregate)
        {
            auto processing_callback = [&](frame_holder frame, synthetic_source_interface* source)
            {
                handle_frame(std::move(frame), source);
            };

            set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(
                new internal_frame_processor_callback<decltype(processing_callback)>(processing_callback)));
        }

        void pipeline_processing_block::handle_frame(frame_holder frame, synthetic_source_interface* source)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto comp = dynamic_cast<composite_frame*>(frame.frame);
            if (comp)
            {
                for (auto i = 0; i < comp->get_embedded_frames_count(); i++)
                {
                    auto f = comp->get_frame(i);
                    f->acquire();
                    _last_set[f->get_stream()->get_unique_id()] = f;
                }

                for (int s : _streams_ids)
                {
                    if (!_last_set[s])
                        return;
                }

                std::vector<frame_holder> set;
                for (auto&& s : _last_set)
                {
                    set.push_back(s.second.clone());
                }
                auto fref = source->allocate_composite_frame(std::move(set));
                if (!fref)
                {
                    LOG_ERROR("Failed to allocate composite frame");
                    return;
                }
                _queue->enqueue(fref);
            }
            else
            {
                LOG_ERROR("Non composite frame arrived to pipeline::handle_frame");
                assert(false);
            }
        }

        bool pipeline_processing_block::dequeue(frame_holder* item, unsigned int timeout_ms)
        {
            return _queue->dequeue(item, timeout_ms);
        }

        bool pipeline_processing_block::try_dequeue(frame_holder* item)
        {
            return _queue->try_dequeue(item);
        }

        /*
            .______    __  .______    _______  __       __  .__   __.  _______
            |   _  \  |  | |   _  \  |   ____||  |     |  | |  \ |  | |   ____|
            |  |_)  | |  | |  |_)  | |  |__   |  |     |  | |   \|  | |  |__
            |   ___/  |  | |   ___/  |   __|  |  |     |  | |  . `  | |   __|
            |  |      |  | |  |      |  |____ |  `----.|  | |  |\   | |  |____
            | _|      |__| | _|      |_______||_______||__| |__| \__| |_______|
        */

        sync_streamer::sync_streamer(std::shared_ptr<librealsense::context> ctx) : streamer(ctx) {}

        void sync_streamer::on_start(std::shared_ptr<frame_streamer::profile> profile)
        {
            std::vector<int> unique_ids;
            for (auto&& s : profile->get_active_streams())
            {
                unique_ids.push_back(s->get_unique_id());
            }

            _syncer = std::unique_ptr<syncer_process_unit>(new syncer_process_unit());
            _pipeline_process = std::unique_ptr<pipeline_processing_block>(new pipeline_processing_block(unique_ids));
        }

        frame_callback_ptr sync_streamer::get_callback()
        {
            auto pipeline_process_callback = [&](frame_holder fref)
            {
                _pipeline_process->invoke(std::move(fref));
            };

            frame_callback_ptr to_pipeline_process = {
                new internal_frame_callback<decltype(pipeline_process_callback)>(pipeline_process_callback),
                [](rs2_frame_callback* p) { p->release(); }
            };

            _syncer->set_output_callback(to_pipeline_process);

            auto to_syncer = [&](frame_holder fref)
            {
                _syncer->invoke(std::move(fref));
            };

            frame_callback_ptr rv = {
                new internal_frame_callback<decltype(to_syncer)>(to_syncer),
                [](rs2_frame_callback* p) { p->release(); }
            };

            return rv;
        }

        frame_holder sync_streamer::wait_for_frames(unsigned int timeout_ms)
        {
            std::lock_guard<std::mutex> lock(_mtx);
            if (!_active_profile)
            {
                throw librealsense::wrong_api_call_sequence_exception("wait_for_frames cannot be called before start()");
            }

            frame_holder f;
            if (_pipeline_process->dequeue(&f, timeout_ms))
            {
                return f;
            }

            //hub returns true even if device already reconnected
            if (!_hub.is_connected(*_active_profile->get_device()))
            {
                try
                {
                    auto prev_conf = _prev_conf;
                    unsafe_stop();
                    unsafe_start(prev_conf);

                    if (_pipeline_process->dequeue(&f, timeout_ms))
                    {
                        return f;
                    }

                }
                catch (const std::exception& e)
                {
                    throw std::runtime_error(to_string() << "Device disconnected. Failed to recconect: " << e.what() << timeout_ms);
                }
            }
            throw std::runtime_error(to_string() << "Frame didn't arrived within " << timeout_ms);
        }

        bool sync_streamer::poll_for_frames(frame_holder* frame)
        {
            std::lock_guard<std::mutex> lock(_mtx);

            if (!_active_profile)
            {
                throw librealsense::wrong_api_call_sequence_exception("poll_for_frames cannot be called before start()");
            }

            if (_pipeline_process->try_dequeue(frame))
            {
                return true;
            }
            return false;
        }

        bool sync_streamer::try_wait_for_frames(frame_holder* frame, unsigned int timeout_ms)
        {
            std::lock_guard<std::mutex> lock(_mtx);
            if (!_active_profile)
            {
                throw librealsense::wrong_api_call_sequence_exception("try_wait_for_frames cannot be called before start()");
            }

            if (_pipeline_process->dequeue(frame, timeout_ms))
            {
                return true;
            }

            //hub returns true even if device already reconnected
            if (!_hub.is_connected(*_active_profile->get_device()))
            {
                try
                {
                    auto prev_conf = _prev_conf;
                    unsafe_stop();
                    unsafe_start(prev_conf);
                    return _pipeline_process->dequeue(frame, timeout_ms);
                }
                catch (const std::exception& e)
                {
                    LOG_INFO(e.what());
                    return false;
                }
            }
            return false;
        }
    }
}

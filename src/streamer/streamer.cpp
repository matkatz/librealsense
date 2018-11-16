// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <algorithm>
#include "streamer.h"
#include "stream.h"
#include "media/record/record_device.h"
#include "media/ros/ros_writer.h"

namespace librealsense
{
    namespace frame_streamer
    {
        streamer::streamer(std::shared_ptr<librealsense::context> ctx)
            :_ctx(ctx), _hub(ctx, RS2_PRODUCT_LINE_ANY_INTEL), _dispatcher(10)
        {}

        streamer::~streamer()
        {
            try
            {
                unsafe_stop();
            }
            catch (...) {}
        }

        std::shared_ptr<profile> streamer::start(std::shared_ptr<config> conf)
        {
            std::lock_guard<std::mutex> lock(_mtx);
            if (_active_profile)
            {
                throw librealsense::wrong_api_call_sequence_exception("start() cannot be called before stop()");
            }
            unsafe_start(conf);
            return unsafe_get_active_profile();
        }

        std::shared_ptr<profile> streamer::start_with_record(std::shared_ptr<config> conf, const std::string& file)
        {
            std::lock_guard<std::mutex> lock(_mtx);
            if (_active_profile)
            {
                throw librealsense::wrong_api_call_sequence_exception("start() cannot be called before stop()");
            }
            conf->enable_record_to_file(file);
            unsafe_start(conf);
            return unsafe_get_active_profile();
        }

        std::shared_ptr<profile> streamer::get_active_profile() const
        {
            std::lock_guard<std::mutex> lock(_mtx);
            return unsafe_get_active_profile();
        }

        std::shared_ptr<profile> streamer::unsafe_get_active_profile() const
        {
            if (!_active_profile)
                throw librealsense::wrong_api_call_sequence_exception("get_active_profile() can only be called between a start() and a following stop()");

            return _active_profile;
        }

        frame_callback_ptr streamer::get_callback()
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

        void streamer::unsafe_start(std::shared_ptr<config> conf)
        {
            std::shared_ptr<profile> profile = nullptr;
            //first try to get the previously resolved profile (if exists)
            auto cached_profile = conf->get_cached_resolved_profile();
            if (cached_profile)
            {
                profile = cached_profile;
            }
            else
            {
                const int NUM_TIMES_TO_RETRY = 3;
                for (int i = 1; i <= NUM_TIMES_TO_RETRY; i++)
                {
                    try
                    {
                        profile = conf->resolve(shared_from_this(), std::chrono::seconds(5));
                        break;
                    }
                    catch (...)
                    {
                        if (i == NUM_TIMES_TO_RETRY)
                            throw;
                    }
                }
            }

            assert(profile);
            assert(profile->_multistream.get_profiles().size() > 0);

            on_start(profile);

            frame_callback_ptr callbacks = get_callback();

            auto dev = profile->get_device();
            if (auto playback = As<librealsense::playback_device>(dev))
            {
                _playback_stopped_token = playback->playback_status_changed += [this, callbacks](rs2_playback_status status)
                {
                    if (status == RS2_PLAYBACK_STATUS_STOPPED)
                    {
                        _dispatcher.invoke([this, callbacks](dispatcher::cancellable_timer t)
                        {
                            //If the streamer holds a playback device, and it reached the end of file (stopped)
                            //Then we restart it
                            if (_active_profile && _prev_conf->get_repeat_playback())
                            {
                                _active_profile->_multistream.open();
                                _active_profile->_multistream.start(callbacks);
                            }
                        });
                    }
                };
            }

            _dispatcher.start();
            profile->_multistream.open();
            profile->_multistream.start(callbacks);
            _active_profile = profile;
            _prev_conf = std::make_shared<config>(*conf);
        }

        void streamer::stop()
        {
            std::lock_guard<std::mutex> lock(_mtx);
            if (!_active_profile)
            {
                throw librealsense::wrong_api_call_sequence_exception("stop() cannot be called before start()");
            }
            unsafe_stop();
        }

        void streamer::unsafe_stop()
        {
            if (_active_profile)
            {
                try
                {
                    auto dev = _active_profile->get_device();
                    if (auto playback = As<librealsense::playback_device>(dev))
                    {
                        playback->playback_status_changed -= _playback_stopped_token;
                    }
                    _active_profile->_multistream.stop();
                    _active_profile->_multistream.close();
                    _dispatcher.stop();
                }
                catch (...)
                {
                } // Stop will throw if device was disconnected. TODO - refactoring anticipated
            }
            _active_profile.reset();
            _prev_conf.reset();
        }

        std::shared_ptr<device_interface> streamer::wait_for_device(const std::chrono::milliseconds& timeout, const std::string& serial)
        {
            // streamer's device selection shall be deterministic
            return _hub.wait_for_device(timeout, false, serial);
        }

        std::shared_ptr<librealsense::context> streamer::get_context() const
        {
            return _ctx;
        }
    } 
}

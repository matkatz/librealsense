// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "uvc-streamer.h"
#include "../backend.h"

const int UVC_PAYLOAD_MAX_HEADER_LENGTH         = 256;
const int DEQUEUE_MILLISECONDS_TIMEOUT          = 50;
const int ENDPOINT_RESET_MILLISECONDS_TIMEOUT   = 100;
const int WATCHDOG_RESET_MILLISECONDS_TIMEOUT   = 1000;

void cleanup_frame(backend_frame *ptr) {
    if (ptr) ptr->owner->deallocate(ptr);
}

namespace librealsense
{
    namespace platform
    {
        uvc_streamer::uvc_streamer(uvc_streamer_context context) :
            _context(context)
        {
            auto inf = context.usb_device->get_interface(context.control->bInterfaceNumber);
            if (inf == nullptr)
                throw std::runtime_error("can't find UVC streaming interface of device: " + context.usb_device->get_info().id);
            _read_endpoint = inf->first_endpoint(platform::RS2_USB_ENDPOINT_DIRECTION_READ);

            _read_buff_length = UVC_PAYLOAD_MAX_HEADER_LENGTH + _context.control->dwMaxVideoFrameSize;
            LOG_INFO("endpoint " << (int)_read_endpoint->get_address() << " read buffer size: " << _read_buff_length);

            _watchdog_timeout = (1000.0 / _context.profile.fps) * 10;
            init();
        }

        uvc_streamer::~uvc_streamer()
        {
            flush();
        }

        void uvc_process_bulk_payload(backend_frame_ptr fp, size_t payload_len, backend_frames_queue& queue) {

            /* ignore empty payload transfers */
            if (!fp || payload_len < 2)
                return;

            uint8_t header_len = fp->pixels[0];
            uint8_t header_info = fp->pixels[1];

            size_t data_len = payload_len - header_len;

            if (header_info & 0x40)
            {
                LOG_ERROR("bad packet: error bit set");
                return;
            }
            if (header_len > payload_len)
            {
                LOG_ERROR("bogus packet: actual_len=" << payload_len << ", header_len=" << header_len);
                return;
            }


            LOG_DEBUG("Passing packet to user CB with size " << (data_len + header_len));
            librealsense::platform::frame_object fo{ data_len, header_len,
                                                     fp->pixels.data() + header_len , fp->pixels.data() };
            fp->fo = fo;

            queue.enqueue(std::move(fp));
        }

        void uvc_streamer::init()
        {
            _frames_archive = std::make_shared<backend_frames_archive>();
            // Get all pointers from archive and initialize their content
            std::vector<backend_frame *> frames;
            for (auto i = 0; i < _frames_archive->CAPACITY; i++) {
                auto ptr = _frames_archive->allocate();
                ptr->pixels.resize(_read_buff_length, 0);
                ptr->owner = _frames_archive.get();
                frames.push_back(ptr);
            }

            for (auto ptr : frames) {
                _frames_archive->deallocate(ptr);
            }

            _publish_frame_thread = std::make_shared<active_object<>>([this](dispatcher::cancellable_timer cancellable_timer)
            {
                backend_frame_ptr fp(nullptr, [](backend_frame *) {});
                if (_queue.dequeue(&fp, DEQUEUE_MILLISECONDS_TIMEOUT))
                {
                    if(running())
                        _context.user_cb(_context.profile, fp->fo, []() mutable {});
                }
            });

            _watchdog = std::make_shared<watchdog>([this]()
                         {
                             _context.messenger->reset_endpoint(_read_endpoint, ENDPOINT_RESET_MILLISECONDS_TIMEOUT);
                             LOG_ERROR("uvc streamer watchdog triggered on endpoint: " << (int)_read_endpoint->get_address());
                             _watchdog->set_timeout(WATCHDOG_RESET_MILLISECONDS_TIMEOUT);
                         }, _watchdog_timeout);

            _request_callback = std::make_shared<usb_request_callback>([this](platform::rs_usb_request r)
            {
                if(!r)
                    return;
                if(!_watchdog->running())
                    _watchdog->start();
                _watchdog->set_timeout(_watchdog_timeout);
                if(r->get_actual_length() >= _context.control->dwMaxVideoFrameSize)
                {
                    auto f = backend_frame_ptr(_frames_archive->allocate(), &cleanup_frame);
                    if(f)
                    {
                        _watchdog->kick();
                        memcpy(f->pixels.data(), r->get_buffer().data(), r->get_buffer().size());
                        uvc_process_bulk_payload(std::move(f), r->get_actual_length(), _queue);
                    }
                }
                if(running())
                {
                    auto sts = _context.messenger->submit_request(r);
                    if(sts != platform::RS2_USB_STATUS_SUCCESS)
                        LOG_ERROR("failed to submit UVC request, error: " << sts);
                }
            });

            _context.messenger->reset_endpoint(_read_endpoint, ENDPOINT_RESET_MILLISECONDS_TIMEOUT);

            _requests = std::vector<rs_usb_request>(_context.request_count);
            for(auto&& r : _requests)
            {
                r = _context.messenger->create_request(_read_endpoint);
                r->set_buffer(std::vector<uint8_t>(_read_buff_length));
                r->set_callback(_request_callback);
            }
        }

        void uvc_streamer::start()
        {
            std::lock_guard<std::mutex> lk(_mutex);
            if(_running)
                return;

            _running = true;

            for(auto&& r : _requests)
            {
                _context.messenger->submit_request(r);
            }

            _publish_frame_thread->start();
        }

        void uvc_streamer::stop()
        {
            std::lock_guard<std::mutex> lk(_mutex);
            if(!_running)
                return;
            _running = false;

            _request_callback->cancel();

            _queue.clear();
            _frames_archive->stop_allocation();
            _frames_archive->wait_until_empty();

            for(auto&& r : _requests)
            {
                _context.messenger->cancel_request(r);
            }

            _context.messenger->reset_endpoint(_read_endpoint, RS2_USB_ENDPOINT_DIRECTION_READ);

            _watchdog->stop();
            _publish_frame_thread->stop();
        }

        void uvc_streamer::flush()
        {
            stop();

            _read_endpoint.reset();

            _watchdog.reset();
            _publish_frame_thread.reset();
            _request_callback.reset();

            _frames_archive.reset();
        }
    }
}

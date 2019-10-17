// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "usbhost.h"
#include "../types.h"

#include <memory>
#include <mutex>
#include <vector>

namespace librealsense
{
    namespace platform
    {
        class request_pool
        {
        public:
            request_pool(::usb_device* device) : _device(device) {}
            ~request_pool()
            {
                for(auto&& r : _request_pool)
                {
                    usb_request_free(r);
                }
            }

            std::shared_ptr<::usb_request> get_request(usb_endpoint_descriptor desc)
            {
                std::lock_guard<std::mutex> lk(_mutex);
                auto it = std::find_if(_request_pool.begin(), _request_pool.end(),
                        [desc] (const ::usb_request* r) { return r->endpoint == desc.bEndpointAddress; });
                ::usb_request* req = NULL;
                if(it == _request_pool.end())
                    req = usb_request_new(_device, &desc);
                else
                    req = *it;
                return std::shared_ptr<::usb_request>(req, [this](::usb_request* req)
                        {
                            std::lock_guard<std::mutex> lk(_mutex);
                            _request_pool.push_back(req);
                        });
            }

        private:
            std::mutex _mutex;
            ::usb_device *_device;
            std::vector<::usb_request*> _request_pool;
        };
    }
}

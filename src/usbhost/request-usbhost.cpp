// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "request-usbhost.h"
#include "endpoint-usbhost.h"
#include "device-usbhost.h"

namespace librealsense
{
    namespace platform
    {
        usb_request_usbhost::usb_request_usbhost(rs_usb_device device, rs_usb_endpoint endpoint)
        {
            _endpoint = endpoint;
            auto dev = std::static_pointer_cast<usb_device_usbhost>(device);
            auto read_ep = std::static_pointer_cast<usb_endpoint_usbhost>(_endpoint);
            auto desc = read_ep->get_descriptor();
            _native_request = dev->get_request(desc);
            _native_request->client_data = this;
        }

        usb_request_usbhost::~usb_request_usbhost()
        {
            _native_request->client_data = NULL;
        }

        int usb_request_usbhost::get_native_buffer_length()
        {
            return _native_request->buffer_length;
        }

        void usb_request_usbhost::set_native_buffer_length(int length)
        {
            _native_request->buffer_length = length;
        }

        int usb_request_usbhost::get_actual_length() const
        {
            return _native_request->actual_length;
        }

        void usb_request_usbhost::set_native_buffer(uint8_t* buffer)
        {
            _native_request->buffer = buffer;
        }

        uint8_t* usb_request_usbhost::get_native_buffer() const
        {
            return (uint8_t*)_native_request->buffer;
        }

        void* usb_request_usbhost::get_native_request() const
        {
            return _native_request.get();
        }

        std::shared_ptr<usb_request> usb_request_usbhost::get_shared() const
        {
            return _shared.lock();
        }

        void usb_request_usbhost::set_shared(const std::shared_ptr<usb_request>& shared)
        {
            _shared = shared;
        }
    }
}

// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "request-winusb.h"
#include "endpoint-winusb.h"
#include "device-winusb.h"

namespace librealsense
{
    namespace platform
    {
        usb_request_winusb::usb_request_winusb(rs_usb_device device, rs_usb_endpoint endpoint)
        {
            _endpoint = endpoint;
            _overlapped = std::make_shared<OVERLAPPED>();
            _safe_handle = std::make_shared<safe_handle>(CreateEvent(nullptr, false, false, nullptr));
            _overlapped->hEvent = _safe_handle->GetHandle();
            _request_holder = std::make_shared<request_holder>();
            _client_data = _request_holder.get();
        }

        usb_request_winusb::~usb_request_winusb()
        {

        }

        int usb_request_winusb::get_actual_length() const
        {
            return _overlapped->InternalHigh;// _request->actual_length;
        }

        void usb_request_winusb::set_buffer_length(int length)
        {
            _buffer_length = length;
        }

        int usb_request_winusb::get_buffer_length()
        {
            return _buffer_length;
        }

        void usb_request_winusb::set_buffer(uint8_t* buffer)
        {
            _buffer = buffer;
        }

        uint8_t* usb_request_winusb::get_buffer() const
        {
            return _buffer;
        }

        void* usb_request_winusb::get_native_request() const
        {
            return _overlapped.get();
        }
    }
}

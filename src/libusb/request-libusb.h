// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "libusb.h"
#include "../usb/usb-request.h"
#include "../usb/usb-device.h"


namespace librealsense
{
    namespace platform
    {
        class usb_request_libusb : public usb_request_base
        {
        public:
            usb_request_libusb(libusb_device_handle *dev_handle, rs_usb_endpoint endpoint);
            virtual ~usb_request_libusb();

            virtual void set_buffer_length(int length) override;
            virtual int get_buffer_length() override;
            virtual void set_buffer(uint8_t* buffer) override;
            virtual uint8_t* get_buffer() const override;
            virtual int get_actual_length() const override;
            virtual void* get_native_request() const override;

        private:
            mutable std::vector<uint8_t> _buffer;
            std::shared_ptr<libusb_transfer> _transfer;
        };
    }
}

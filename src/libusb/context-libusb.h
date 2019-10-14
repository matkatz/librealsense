// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb/usb-types.h"

#include <memory>
#include <libusb.h>

namespace librealsense
{
    namespace platform
    {
        class usb_context
        {
        public:
            usb_context();
            
            ~usb_context();
            
            libusb_context* get();            
            
        private:
            struct libusb_context* _ctx;
        };
        
        class usb_device_list
        {
        public:
            usb_device_list();
            
            ~usb_device_list();
            
            libusb_device* get(uint8_t index);
            size_t count();

            std::shared_ptr<usb_context> get_context();
        private:
            libusb_device **_list;
            size_t _count;
            std::shared_ptr<usb_context> _ctx;
        };
    }
}

// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "context-libusb.h"

namespace librealsense
{
    namespace platform
    {
        std::shared_ptr<usb_context> instance()
        {
            static std::shared_ptr<usb_context> instance = std::make_shared<usb_context>();
            return instance;
        }
        
        usb_context::usb_context()
        {
            libusb_init(&_ctx);
        }
        
        usb_context::~usb_context()
        {
            libusb_exit(_ctx);
        }
        
        libusb_context* usb_context::get()
        {
            return _ctx;
        } 
    
        usb_device_list::usb_device_list() : _list(NULL), _count(0)
        {
            _ctx = std::make_shared<usb_context>();
            _count = libusb_get_device_list(_ctx->get(), &_list);
        }
        
        usb_device_list::~usb_device_list()
        {
            libusb_free_device_list(_list, true);
        }
        
        libusb_device* usb_device_list::get(uint8_t index)
        {
            return index < _count ? _list[index] : NULL;
        }
        
        size_t usb_device_list::count()
        {
            return _count;
        }

        std::shared_ptr<usb_context> usb_device_list::get_context()
        {
            return _ctx;
        }
    }
}

// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb/usb-types.h"
#include "messenger-libusb.h"
#include "usb/usb-device.h"

#include <string>
#include <regex>
#include <sstream>
#include <mutex>

#include <libusb.h>

namespace librealsense
{
    namespace platform
    {
        class usb_context
        {
        public:
            usb_context() { libusb_init(&_ctx); }
            ~usb_context() { libusb_exit(_ctx); }
            libusb_context* get() { return _ctx; }
        private:
            struct libusb_context* _ctx;
        };

        class usb_device_libusb : public usb_device, public std::enable_shared_from_this<usb_device_libusb>
        {
        public:
            usb_device_libusb(libusb_device* device, const libusb_device_descriptor& desc, const usb_device_info& info, std::shared_ptr<usb_context> context);
            virtual ~usb_device_libusb();

            virtual const usb_device_info get_info() const override { return _info; }
            virtual const std::vector<rs_usb_interface> get_interfaces() const override { return _interfaces; }
            virtual const rs_usb_messenger open(uint8_t interface_number) override;
            virtual const std::vector<usb_descriptor> get_descriptors() const override { return _descriptors; }
            libusb_device* get_device() { return _device; }
            void release();

        private:
            libusb_device* _device;
            libusb_device_handle* _handle;
            libusb_device_descriptor _usb_device_descriptor;
            const usb_device_info _info;
            std::vector<std::shared_ptr<usb_interface>> _interfaces;
            std::vector<usb_descriptor> _descriptors;
            int _kill_handler_thread = 0;
            std::shared_ptr<usb_context> _context;
            std::shared_ptr<active_object<>> _event_handler;

            std::shared_ptr<handle_libusb> get_handle(uint8_t interface_number);
        };
    }
}

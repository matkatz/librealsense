// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "usb-types.h"
#include "usb-interface.h"

#include <memory>
#include <vector>
#include <stdint.h>

namespace librealsense
{
    namespace platform
    {
        class usb_device
        {
        public:
            virtual ~usb_device() = default;

            virtual const usb_device_info get_info() const = 0;
            virtual const std::vector<usb_device_info> get_subdevices_infos() const = 0;
            virtual const rs_usb_interface get_interface(uint8_t interface_number) const = 0;
            virtual const std::vector<rs_usb_interface> get_interfaces(usb_subclass filter) const = 0;
            virtual const rs_usb_messenger open() = 0;
        };

        typedef std::shared_ptr<usb_device> rs_usb_device;

        class usb_device_mock : public usb_device
        {
        public:
            virtual ~usb_device_mock() = default;

            virtual const usb_device_info get_info() const override { return usb_device_info(); }
            virtual const std::vector<usb_device_info> get_subdevices_infos() const override { return std::vector<usb_device_info>(); }
            virtual const rs_usb_interface get_interface(uint8_t interface_number) const override { return nullptr; }
            virtual const std::vector<rs_usb_interface> get_interfaces(usb_subclass filter) const override { return std::vector<std::shared_ptr<usb_interface>>(); }
            virtual const rs_usb_messenger open() override { return nullptr; }
        };
    }
}
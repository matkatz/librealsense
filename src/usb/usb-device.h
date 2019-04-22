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
        class command_transfer
        {
        public:
            virtual std::vector<uint8_t> send_receive(
                const std::vector<uint8_t>& data,
                int timeout_ms = 5000,
                bool require_response = true) = 0;

            virtual ~command_transfer() = default;
        };

        class usb_device : public command_transfer
        {
        public:
            virtual ~usb_device() = default;

            virtual const usb_device_info get_info() const = 0;
            virtual const rs_usb_interface get_interface(uint8_t interface_number) const = 0;
            virtual const std::vector<rs_usb_interface> get_interfaces(usb_subclass filter) const = 0;
            virtual const rs_usb_messenger open() = 0;

            virtual std::vector<uint8_t> send_receive(
                const std::vector<uint8_t>& data,
                int timeout_ms = 5000,
                bool require_response = true)
            {
                const auto& m = open();
                return m->send_receive_transfer(data, timeout_ms);
            }
        };

        typedef std::shared_ptr<usb_device> rs_usb_device;

        class usb_device_mock : public usb_device
        {
        public:
            virtual ~usb_device_mock() = default;

            virtual const usb_device_info get_info() const override { return usb_device_info(); }
            virtual const rs_usb_interface get_interface(uint8_t interface_number) const override { return nullptr; }
            virtual const std::vector<rs_usb_interface> get_interfaces(usb_subclass filter) const override { return std::vector<std::shared_ptr<usb_interface>>(); }
            virtual const rs_usb_messenger open() override { return nullptr; }

            virtual std::vector<uint8_t> send_receive(
                    const std::vector<uint8_t>& data,
                    int timeout_ms = 5000,
                    bool require_response = true) override
            {
                return std::vector<uint8_t>();
            }
        };
    }
}
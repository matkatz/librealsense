// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "usb/usb-enumerator.h"
#include "libusb/device-libusb.h"
#include "types.h"

#include <libusb.h>

namespace librealsense
{
    namespace platform
    {
        struct usb_context
        {
            usb_context() { libusb_init(&_ctx); }
            ~usb_context() { libusb_exit(_ctx); }
            libusb_context* get() { return _ctx; }
        private:
            struct libusb_context* _ctx;
        };

        std::shared_ptr<usb_context> get_context()
        {
            static std::shared_ptr<usb_context> context = std::make_shared<usb_context>();
            return context;
        }

        bool usb_enumerator::is_device_connected(const std::shared_ptr<usb_device> device)
        {
            return false;
        }

        std::vector<std::shared_ptr<usb_device>> usb_enumerator::query_devices()
        {
            std::vector<std::shared_ptr<usb_device>> rv;
            ssize_t count = 0;
            libusb_device **list = NULL;

            auto ctx = get_context()->get();
            count = libusb_get_device_list(ctx, &list);

            for (ssize_t idx = 0; idx < count; ++idx)
            {
                libusb_device *device = list[idx];
                libusb_device_descriptor desc = { 0 };

                auto rc = libusb_get_device_descriptor(device, &desc);
                if (desc.idVendor != 0x8086)
                    continue;
                rv.push_back(std::make_shared<usb_device_libusb>(device, desc));
            }
            libusb_free_device_list(list, 0);
            return rv;
        }
    }
}

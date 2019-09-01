// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "l500-fw-update-device.h"
#include "l500-private.h"

namespace librealsense
{
    l500_update_device::l500_update_device(std::shared_ptr<context> ctx, bool register_device_notifications, std::shared_ptr<platform::usb_device> usb_device)
        : update_device(ctx, register_device_notifications, usb_device), _product_line("L500")
    {
        auto info = usb_device->get_info();
        _name = ivcam2::rs500_sku_names.find(info.pid) != ivcam2::rs500_sku_names.end() ? ivcam2::rs500_sku_names.at(info.pid) : "unknown";
    }

    void l500_update_device::update(const void* fw_image, int fw_image_size, update_progress_callback_ptr callback) const
    {
        update_device::update(fw_image, fw_image_size, callback);
    }
}

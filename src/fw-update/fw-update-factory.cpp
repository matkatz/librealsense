// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "fw-update-factory.h"
#include "fw-update-device.h"
#include "usb/usb-enumerator.h"
#include "ds5/ds5-private.h"

namespace librealsense
{
    bool is_recovery_pid(uint16_t pid)
    {
        return std::find(ds::rs400_sku_recovery_pid.begin(), ds::rs400_sku_recovery_pid.end(), pid) != ds::rs400_sku_recovery_pid.end();
    }

    std::vector<std::shared_ptr<device_info>> fw_update_info::pick_recovery_devices(
        std::shared_ptr<context> ctx,
        const std::vector<platform::usb_device_info>& usb_devices)
    {
        std::vector<std::shared_ptr<device_info>> list;
        for (auto&& usb : usb_devices)
        {
            if (is_recovery_pid(usb.pid))
            {
                list.push_back(std::make_shared<fw_update_info>(ctx, usb));
            }
        }
        return list;
    }

    std::shared_ptr<device_interface> fw_update_info::create(std::shared_ptr<context> ctx, bool register_device_notifications) const
    {
        for (auto&& usb : platform::usb_enumerator::query_devices())
        {
            if(usb->get_info().id == _dfu.id)
                return std::make_shared<fw_update_device>(ctx, register_device_notifications, usb);
        }
        throw std::runtime_error("dfu device not found");
    }
}

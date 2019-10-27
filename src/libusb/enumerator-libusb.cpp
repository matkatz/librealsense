// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "usb/usb-enumerator.h"
#include "device-libusb.h"
#include "context-libusb.h"
#include "types.h"

#include <libusb.h>

namespace librealsense
{
    namespace platform
    {
        std::string get_device_path(libusb_device* usb_device)
        {
            int bus = libusb_get_bus_number(usb_device);
            int address = libusb_get_device_address(usb_device);
            std::stringstream ss;
            ss << "/dev/bus/usb/"
               << std::setw(3) << std::setfill('0') << bus << "/"
               << std::setw(3) << std::setfill('0') << address;

            return ss.str();
        }

        std::vector<usb_device_info> get_subdevices(libusb_device* device, libusb_device_descriptor desc)
        {
            std::vector<usb_device_info> rv;
            for (ssize_t c = 0; c < desc.bNumConfigurations; ++c)
            {
                libusb_config_descriptor *config;
                auto rc = libusb_get_config_descriptor(device, c, &config);

                for (ssize_t i = 0; i < config->bNumInterfaces; ++i)
                {
                    auto inf = config->interface[i];
                    
                    //avoid publish streaming interfaces TODO:MK
                    if(inf.altsetting->bInterfaceSubClass == 2)
                        continue;
                    // when device is in DFU state, two USB devices are detected, one of RS2_USB_CLASS_VENDOR_SPECIFIC (255) class
                    // and the other of RS2_USB_CLASS_APPLICATION_SPECIFIC (254) class.
                    // in order to avoid listing two usb devices for a single physical device we ignore the application specific class
                    // https://www.usb.org/defined-class-codes#anchor_BaseClassFEh
                    if(inf.altsetting->bInterfaceClass == RS2_USB_CLASS_APPLICATION_SPECIFIC)
                        continue;

                    usb_device_info info{};
                    auto path = get_device_path(device);
                    info.id = path;
                    info.unique_id = path;
                    info.conn_spec = usb_spec(desc.bcdUSB);
                    info.vid = desc.idVendor;
                    info.pid = desc.idProduct;
                    info.mi = i;
                    info.cls = usb_class(inf.altsetting->bInterfaceClass);
                    rv.push_back(info);
                }

                libusb_free_config_descriptor(config);
            }
            return rv;
        }

        std::vector<usb_device_info> usb_enumerator::query_devices_info()
        {
            std::vector<usb_device_info> rv;
            auto ctx = std::make_shared<usb_context>();

            for (ssize_t idx = 0; idx < ctx->device_count(); ++idx)
            {
                auto device = ctx->get_device(idx);
                if(device == NULL)
                    continue;
                libusb_device_descriptor desc = { 0 };


                auto rc = libusb_get_device_descriptor(device, &desc);
                auto sd = get_subdevices(device, desc);
                rv.insert(rv.end(), sd.begin(), sd.end());
            }
            return rv;
        }

        rs_usb_device usb_enumerator::create_usb_device(const usb_device_info& info)
        {
            auto ctx = std::make_shared<usb_context>();

            for (ssize_t idx = 0; idx < ctx->device_count(); ++idx)
            {
                auto device = ctx->get_device(idx);
                if(device == NULL || get_device_path(device) != info.id)
                    continue;

                libusb_device_descriptor desc = { 0 };
                auto rc = libusb_get_device_descriptor(device, &desc);

                try
                {
                    return std::make_shared<usb_device_libusb>(device, desc, info, ctx);
                }
                catch(std::exception e)
                {
                    LOG_WARNING("failed to create usb device at index: %d" << idx);
                }
            }
            return nullptr;
        }

    }
}

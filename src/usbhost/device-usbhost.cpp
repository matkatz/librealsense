// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef RS2_USE_ANDROID_BACKEND

#include "device-usbhost.h"
#include "endpoint-usbhost.h"
#include "interface-usbhost.h"
#include "usbhost.h"
#include "../types.h"

#include <string>
#include <regex>
#include <sstream>
#include <mutex>

namespace librealsense
{
    namespace platform
    {
        usb_device_usbhost::usb_device_usbhost(::usb_device* handle) :
            _handle(handle)
        {
            usb_descriptor_iter it;
            ::usb_descriptor_iter_init(handle, &it);
            usb_descriptor_header *h = usb_descriptor_iter_next(&it);
            if (h != nullptr && h->bDescriptorType == USB_DT_DEVICE) {
                usb_device_descriptor *device_descriptor = (usb_device_descriptor *) h;
                _info.conn_spec = librealsense::platform::usb_spec(device_descriptor->bcdUSB);
            }

            do {
                h = usb_descriptor_iter_next(&it);
                if(h == NULL)
                    break;
                if (h->bDescriptorType == USB_DT_INTERFACE_ASSOCIATION) {
                    auto iad = *(usb_interface_assoc_descriptor *) h;
                }
                if (h->bDescriptorType == USB_DT_INTERFACE) {
                    auto id = *(usb_interface_descriptor *) h;
                    auto in = id.bInterfaceNumber;
                    _interfaces[in] = std::make_shared<usb_interface_usbhost>(id, it);
                }
            } while (h != nullptr);

            _usb_device_descriptor = usb_device_get_device_descriptor(_handle);
            _info.vid = usb_device_get_vendor_id(_handle);
            _info.pid = usb_device_get_product_id(_handle);
            _info.id = std::string(usb_device_get_name(_handle));
            _info.unique_id = std::string(usb_device_get_name(_handle));
//            _info.serial = std::string(usb_device_get_serial(_handle, 10));
//            _desc_length = usb_device_get_descriptors_length(_handle);
        }

        void usb_device_usbhost::release()
        {
            _messenger.reset();
            LOG_DEBUG("usb device: " << get_info().unique_id << ", released");
        }

        const std::shared_ptr<usb_interface> usb_device_usbhost::get_interface(uint8_t interface_number) const
        {
            return _interfaces.at(interface_number);
        }

        const std::vector<std::shared_ptr<usb_interface>> usb_device_usbhost::get_interfaces(usb_subclass filter) const
        {
            std::vector<std::shared_ptr<usb_interface>> rv;
            for (auto&& entry : _interfaces)
            {
                auto i = entry.second;
                if(filter == USB_SUBCLASS_ANY ||
                   i->get_subclass() & filter ||
                   (filter == 0 && i->get_subclass() == 0))
                    rv.push_back(i);
            }
            return rv;
        }

        const std::shared_ptr<usb_messenger> usb_device_usbhost::open()
        {
            if(!_messenger)
                _messenger = std::make_shared<usb_messenger_usbhost>(shared_from_this());
            return _messenger;
        }
    }
}
#endif
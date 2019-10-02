// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "device-libusb.h"
#include "endpoint-libusb.h"
#include "interface-libusb.h"
#include "types.h"

#include <string>
#include <regex>
#include <sstream>
#include <mutex>

namespace librealsense
{
    namespace platform
    {
        usb_device_libusb::usb_device_libusb(libusb_device* device, const libusb_device_descriptor& desc, const usb_device_info& info, std::shared_ptr<usb_context> context) :
                _device(device), _usb_device_descriptor(desc), _info(info), _context(context)
        {
            libusb_open(_device, &_handle);
            usb_descriptor ud = {desc.bLength, desc.bDescriptorType, std::vector<uint8_t>(desc.bLength)};
            memcpy(ud.data.data(), &desc, desc.bLength);
            _descriptors.push_back(ud);

            for (ssize_t c = 0; c < desc.bNumConfigurations; ++c)
            {
                libusb_config_descriptor *config;
                auto rc = libusb_get_config_descriptor(device, c, &config);

                std::shared_ptr<usb_interface_libusb> curr_ctrl_intf;
                for (ssize_t i = 0; i < config->bNumInterfaces; ++i)
                {
                    auto inf = config->interface[i];
                    auto curr_inf = std::make_shared<usb_interface_libusb>(inf);
                    _interfaces.push_back(curr_inf);
                    switch (inf.altsetting->bInterfaceClass)
                    {
                        case RS2_USB_CLASS_VIDEO:
                        {
                            if(inf.altsetting->bInterfaceSubClass == RS2_USB_SUBCLASS_VIDEO_CONTROL)
                                curr_ctrl_intf = curr_inf;
                            if(inf.altsetting->bInterfaceSubClass == RS2_USB_SUBCLASS_VIDEO_STREAMING)
                                curr_ctrl_intf->add_associated_interface(curr_inf);
                            break;
                        }
                        default:
                            break;
                    }
                    for(int j = 0; j < inf.num_altsetting; j++)
                    {
                        auto d = inf.altsetting[j];
                        usb_descriptor ud = {d.bLength, d.bDescriptorType, std::vector<uint8_t>(d.bLength)};
                        memcpy(ud.data.data(), &d, d.bLength);
                        _descriptors.push_back(ud);
                        for(int k = 0; k < d.extra_length; )
                        {
                            auto l = d.extra[k];
                            auto dt = d.extra[k+1];
                            usb_descriptor ud = {l, dt, std::vector<uint8_t>(l)};
                            memcpy(ud.data.data(), &d.extra[k], l);
                            _descriptors.push_back(ud);
                            k += l;
                        }
                    }

                }

                libusb_free_config_descriptor(config);
            }

            _event_handler = std::make_shared<active_object<>>([&](dispatcher::cancellable_timer cancellable_timer)
                                                               {
                                                                   auto sts = libusb_handle_events_completed(_context->get(), &_kill_handler_thread);
                                                               });
            _event_handler->start();
        }

        usb_device_libusb::~usb_device_libusb()
        {
            _kill_handler_thread = 1;
            libusb_close(_handle);
            _event_handler->stop();
            if(_device)
                libusb_unref_device(_device);
        }

        void usb_device_libusb::release()
        {
            LOG_DEBUG("usb device: " << get_info().unique_id << ", released");
        }

        std::shared_ptr<handle_libusb> usb_device_libusb::get_handle(uint8_t interface_number)
        {
            auto it = std::find_if(_interfaces.begin(), _interfaces.end(), [interface_number](const rs_usb_interface& i)
            { return interface_number == i->get_number(); });
            if (it == _interfaces.end())
                return nullptr;
            auto intf = std::dynamic_pointer_cast<usb_interface_libusb>(*it);
            return std::make_shared<handle_libusb>(_device, intf);
        }

        const std::shared_ptr<usb_messenger> usb_device_libusb::open(uint8_t interface_number)
        {
            auto h = get_handle(interface_number);
            return std::make_shared<usb_messenger_libusb>(shared_from_this(), h);
        }
    }
}

// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "context-libusb.h"

#include <chrono>

#include <libusb.h>

namespace librealsense
{
    namespace platform
    {
        static usb_status libusb_status_to_rs(int sts)
        {
            switch (sts)
            {
                case LIBUSB_SUCCESS: return RS2_USB_STATUS_SUCCESS;
                case LIBUSB_ERROR_IO: return RS2_USB_STATUS_IO;
                case LIBUSB_ERROR_INVALID_PARAM: return RS2_USB_STATUS_INVALID_PARAM;
                case LIBUSB_ERROR_ACCESS: return RS2_USB_STATUS_ACCESS;
                case LIBUSB_ERROR_NO_DEVICE: return RS2_USB_STATUS_NO_DEVICE;
                case LIBUSB_ERROR_NOT_FOUND: return RS2_USB_STATUS_NOT_FOUND;
                case LIBUSB_ERROR_BUSY: return RS2_USB_STATUS_BUSY;
                case LIBUSB_ERROR_TIMEOUT: return RS2_USB_STATUS_TIMEOUT;
                case LIBUSB_ERROR_OVERFLOW: return RS2_USB_STATUS_OVERFLOW;
                case LIBUSB_ERROR_PIPE: return RS2_USB_STATUS_PIPE;
                case LIBUSB_ERROR_INTERRUPTED: return RS2_USB_STATUS_INTERRUPTED;
                case LIBUSB_ERROR_NO_MEM: return RS2_USB_STATUS_NO_MEM;
                case LIBUSB_ERROR_NOT_SUPPORTED: return RS2_USB_STATUS_NOT_SUPPORTED;
                case LIBUSB_ERROR_OTHER: return RS2_USB_STATUS_OTHER;
                default: return RS2_USB_STATUS_OTHER;
            }
        }

        class handle_libusb
        {
        public:
            handle_libusb(std::shared_ptr<usb_context> context, libusb_device* device, std::shared_ptr<usb_interface_libusb> interface) :
                    _first_interface(interface), _context(context)
            {
                claim_interface(device, interface->get_number());
                for(auto&& i : interface->get_associated_interfaces())
                    claim_interface(device, i->get_number());
                _context->start_event_handler();
            }

            ~handle_libusb()
            {
                _context->stop_event_handler();
                for(auto&& h : _handles)
                {
                    if(h.second == NULL)
                        continue;
                    libusb_release_interface(h.second, h.first);
                    if(h.first == _first_interface->get_number())
                        libusb_close(h.second);
                }
            }

            libusb_device_handle* get_first_handle()
            {
                return _handles.at(_first_interface->get_number());
            }

            libusb_device_handle* get_handle(uint8_t interface)
            {
                return _handles.at(interface);
            }

        private:
            usb_status claim_interface(libusb_device* device, uint8_t interface)
            {
                auto& h = _handles[interface];
                auto sts = libusb_open(device, &h);
                if(sts != LIBUSB_SUCCESS)
                {
                    auto rs_sts =  libusb_status_to_rs(sts);
                    LOG_ERROR("libusb open failed, error: " << usb_status_to_string.at(rs_sts));
                    return rs_sts;
                }

                //libusb_set_auto_detach_kernel_driver(h, true);

                 if (libusb_kernel_driver_active(h, interface) == 1)//find out if kernel driver is attached
                     if (libusb_detach_kernel_driver(h, interface) == 0)// detach driver from device if attached.
                         LOG_DEBUG("handle_libusb - detach kernel driver");

                sts = libusb_claim_interface(h, interface);
                if(sts != LIBUSB_SUCCESS)
                {
                    auto rs_sts =  libusb_status_to_rs(sts);
                    LOG_ERROR("libusb claim interface failed, error: " << usb_status_to_string.at(rs_sts));
                    return rs_sts;
                }

                return RS2_USB_STATUS_SUCCESS;
            }

            std::shared_ptr<usb_context> _context;
            std::shared_ptr<usb_interface_libusb> _first_interface;
            std::map<int,libusb_device_handle*> _handles;
        };
    }
}

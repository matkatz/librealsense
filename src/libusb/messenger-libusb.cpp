// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "messenger-libusb.h"
#include "device-libusb.h"
#include "usb/usb-enumerator.h"
#include "hw-monitor.h"
#include "endpoint-libusb.h"
#include "interface-libusb.h"
#include "uvc/uvc-types.h"
#include "handle-libusb.h"
#include "request-libusb.h"

#include <string>
#include <regex>
#include <sstream>
#include <mutex>

#define CLEAR_FEATURE 0x01
#define UVC_FEATURE 0x02

namespace librealsense
{
    namespace platform
    {
        usb_messenger_libusb::usb_messenger_libusb(const std::shared_ptr<usb_device_libusb>& device,
                                                   std::shared_ptr<handle_libusb> handle)
                : _device(device), _handle(handle)
        {

        }

        usb_messenger_libusb::~usb_messenger_libusb()
        {

        }

        usb_status usb_messenger_libusb::reset_endpoint(const rs_usb_endpoint& endpoint, uint32_t timeout_ms)
        {
            uint32_t transferred = 0;
            int request_type = UVC_FEATURE;
            int request = CLEAR_FEATURE;
            int value = 0;
            int ep = endpoint->get_address();
            uint8_t* buffer = NULL;
            int length = 0;
            auto h = _handle->get_handle(endpoint->get_interface_number());
            if(h == nullptr)
                return RS2_USB_STATUS_NO_DEVICE;
            auto sts = libusb_control_transfer(h, request_type, request, value, ep, buffer, length, timeout_ms);
            if(sts < 0)
            {
                std::string strerr = strerror(errno);
                LOG_WARNING("reset_endpoint-control_transfer returned error, index: " << ep << ", error: " << strerr << ", number: " << (int)errno);
                return libusb_status_to_rs(sts);
            }
            transferred = sts;
            return RS2_USB_STATUS_SUCCESS;
        }

        std::shared_ptr<usb_interface_libusb> usb_messenger_libusb::get_interface(int number)
        {
            auto intfs = _device->get_interfaces();
            auto it = std::find_if(intfs.begin(), intfs.end(),
                                   [&](const rs_usb_interface& i) { return i->get_number() == number; });
            if (it == intfs.end())
                return nullptr;
            return std::static_pointer_cast<usb_interface_libusb>(*it);
        }

        usb_status usb_messenger_libusb::control_transfer(int request_type, int request, int value, int index, uint8_t* buffer, uint32_t length, uint32_t& transferred, uint32_t timeout_ms)
        {
            auto h = _handle->get_handle(index & 0xFF);
            if(h == nullptr)
                return RS2_USB_STATUS_NO_DEVICE;
            auto sts = libusb_control_transfer(h, request_type, request, value, index, buffer, length, timeout_ms);
            if(sts < 0)
            {
                std::string strerr = strerror(errno);
                LOG_WARNING("control_transfer returned error, index: " << index << ", error: " << strerr << ", number: " << (int)errno);
                return libusb_status_to_rs(sts);
            }
            transferred = sts;
            return RS2_USB_STATUS_SUCCESS;
        }

        usb_status usb_messenger_libusb::bulk_transfer(const std::shared_ptr<usb_endpoint>&  endpoint, uint8_t* buffer, uint32_t length, uint32_t& transferred, uint32_t timeout_ms)
        {
            auto h = _handle->get_handle(endpoint->get_interface_number());
            if(h == nullptr)
                return RS2_USB_STATUS_NO_DEVICE;
            int actual_length = 0;
            auto sts = libusb_bulk_transfer(h, endpoint->get_address(), buffer, length, &actual_length, timeout_ms);
            if(sts < 0)
            {
                std::string strerr = strerror(errno);
                LOG_WARNING("bulk_transfer returned error, endpoint: " << (int)endpoint->get_address() << ", error: " << strerr << ", number: " << (int)errno);
                return libusb_status_to_rs(sts);
            }
            transferred = actual_length;
            return RS2_USB_STATUS_SUCCESS;
        }

        rs_usb_request usb_messenger_libusb::create_request(rs_usb_endpoint endpoint)
        {
            auto rv = std::make_shared<usb_request_libusb>(_handle->get_handle(endpoint->get_interface_number()), endpoint);
            auto rh = rv->get_holder();
            rh->request = rv;
            return rv;
        }

        usb_status usb_messenger_libusb::submit_request(const rs_usb_request& request)
        {
            auto nr = reinterpret_cast<libusb_transfer*>(request->get_native_request());
            if(nr->dev_handle == NULL)
                return RS2_USB_STATUS_NO_DEVICE;
            auto sts = libusb_submit_transfer(nr);
            if (sts < 0)
            {
                std::string strerr = strerror(errno);
                LOG_WARNING("usb_request_queue returned error, endpoint: " << (int)request->get_endpoint()->get_address() << " error: " << strerr << ", number: " << (int)errno);
                return libusb_status_to_rs(errno);
            }
            return RS2_USB_STATUS_SUCCESS;
        }

        usb_status usb_messenger_libusb::cancel_request(const rs_usb_request& request)
        {
            auto nr = reinterpret_cast<libusb_transfer*>(request->get_native_request());
            auto sts = libusb_cancel_transfer(nr);
            if (sts < 0)
            {
                std::string strerr = strerror(errno);
                LOG_WARNING("usb_request_cancel returned error, endpoint: " << (int)request->get_endpoint()->get_address() << " error: " << strerr << ", number: " << (int)errno);
                return libusb_status_to_rs(errno);
            }
            return RS2_USB_STATUS_SUCCESS;
        }
    }
}

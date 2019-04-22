// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "messenger-libusb.h"
#include "device-libusb.h"
#include "usb/usb-enumerator.h"
#include "hw-monitor.h"
#include "endpoint-libusb.h"
#include "interface-libusb.h"
#include "libuvc/uvc_types.h"
#include "handle-libusb.h"

#include <string>
#include <regex>
#include <sstream>
#include <mutex>

#define INTERRUPT_BUFFER_SIZE 1024

namespace librealsense
{
    namespace platform
    {
        usb_messenger_libusb::usb_messenger_libusb(const std::shared_ptr<usb_device_libusb>& device)
            : _device(device)
        {
            
        }

        usb_messenger_libusb::~usb_messenger_libusb()
        {

        }

        bool usb_messenger_libusb::reset_endpoint(std::shared_ptr<usb_endpoint> endpoint)
        {
            int ep = endpoint->get_address();
            bool rv = control_transfer(0x02, //UVC_FEATURE,
                                       0x01, //CLEAR_FEATURE
                                       0, ep, NULL, 0, 10) == UVC_SUCCESS;
            if(rv)
                LOG_DEBUG("USB pipe " << ep << " reset successfully");
            else
                LOG_DEBUG("Failed to reset the USB pipe " << ep);
            return rv;
        }

        std::shared_ptr<usb_interface_libusb> usb_messenger_libusb::get_interface_by_endpoint(const std::shared_ptr<usb_endpoint>& endpoint)
        {
            auto i = _device->get_interface(endpoint->get_interface_number());
            return std::static_pointer_cast<usb_interface_libusb>(i);
        }

        int usb_messenger_libusb::control_transfer(int request_type, int request, int value, int index, uint8_t* buffer, uint32_t length, uint32_t timeout_ms)
        {
            handle_libusb handle(_device->get_device(), 0);
            auto h = handle.get_handle();
            auto rv = libusb_control_transfer(h, request_type, request, value, index, buffer, length, timeout_ms);
            return rv;
        }

        int usb_messenger_libusb::bulk_transfer(const std::shared_ptr<usb_endpoint>& endpoint, uint8_t* buffer, uint32_t length, uint32_t timeout_ms)
        {
            handle_libusb handle(_device->get_device(), get_interface_by_endpoint(endpoint)->get_number());
            auto h = handle.get_handle();
            int actual_length = -1;
            auto rv = libusb_bulk_transfer(h, endpoint->get_address(), buffer, length, &actual_length, timeout_ms);
            return rv == 0 ? actual_length : rv;
        }

        std::vector<uint8_t> usb_messenger_libusb::send_receive_transfer(std::vector<uint8_t> data, int timeout_ms)
        {
            auto intfs = _device->get_interfaces(USB_SUBCLASS_HWM);
            if(intfs.size() == 0)
                throw std::runtime_error("can't find HWM interface");

            auto hwm = intfs[0];

            int transfered_count = bulk_transfer(hwm->first_endpoint(USB_ENDPOINT_DIRECTION_WRITE),
                                                 data.data(), data.size(), timeout_ms);

            if (transfered_count < 0)
                throw std::runtime_error("USB command timed-out!");

            std::vector<uint8_t> output(HW_MONITOR_BUFFER_SIZE);
            transfered_count = bulk_transfer(hwm->first_endpoint(USB_ENDPOINT_DIRECTION_READ),
                                             output.data(), output.size(), timeout_ms);

            if (transfered_count < 0)
                throw std::runtime_error("USB command timed-out!");

            output.resize(transfered_count);
            return output;
        }
    }
}

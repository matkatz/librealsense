// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "catch/catch.hpp"
#include "usb/usb-enumerator.h"
#include "usb/usb-device.h"
#include "hw-monitor.h"

#include <map>

using namespace librealsense;
using namespace librealsense::platform;

TEST_CASE("query_devices", "[live][usb]") 
{
    auto  devices = usb_enumerator::query_devices();

    REQUIRE(devices.size());

    switch (devices.size())
    {
        case 0: printf("\nno USB device found\n\n");
        case 1: printf("\n1 USB device found:\n\n");
        default:
            printf("\n%d USB devices found:\n\n", devices.size());
    }

    int device_counter = 0;
    for (auto&& dev : devices)
    {
        auto device_info = dev->get_info();
        printf("%d)\tid: %s\n", ++device_counter, device_info.id.c_str());

        auto subdevices = dev->get_subdevices_infos();
        for (auto&& subdevice_info : subdevices)
        {            
            printf("\tmi: %d\n", subdevice_info.mi);
        }
    }
}

TEST_CASE("is_device_connected", "[live][usb]")
{
    auto  devices = usb_enumerator::query_devices();
    int device_counter = 0;
    for (auto&& dev : devices)
    {
        auto sts = usb_enumerator::is_device_connected(dev);
        REQUIRE(sts);
    }
}

TEST_CASE("filter_subdevices", "[live][usb]")
{
    auto  devices = usb_enumerator::query_devices();
    int device_counter = 0;
    std::map<usb_subclass, std::vector<rs_usb_interface>> interfaces;
    for (auto&& dev : devices)
    {
        interfaces[USB_SUBCLASS_CONTROL] = dev->get_interfaces(USB_SUBCLASS_CONTROL);
        interfaces[USB_SUBCLASS_STREAMING] = dev->get_interfaces(USB_SUBCLASS_STREAMING);
        interfaces[USB_SUBCLASS_HWM] = dev->get_interfaces(USB_SUBCLASS_HWM);
        auto any = dev->get_interfaces(USB_SUBCLASS_ANY);

        auto count = interfaces.at(USB_SUBCLASS_CONTROL).size() +
            interfaces.at(USB_SUBCLASS_STREAMING).size() +
            interfaces.at(USB_SUBCLASS_HWM).size();
        REQUIRE(any.size() == count);

        for (auto&& intfs : interfaces)
        {
            for (auto&& intf : intfs.second)
            {
                REQUIRE(intf->get_subclass() == intfs.first);
            }
        }
    }
}

TEST_CASE("first_endpoints_direction", "[live][usb]")
{
    auto  devices = usb_enumerator::query_devices();
    int device_counter = 0;
    for (auto&& dev : devices)
    {
        auto interfaces = dev->get_interfaces(USB_SUBCLASS_HWM);

        for (auto&& i : interfaces)
        {
            auto w = i->first_endpoint(USB_ENDPOINT_DIRECTION_WRITE);
            if(w)
                REQUIRE(USB_ENDPOINT_DIRECTION_WRITE == w->get_direction());
            auto r = i->first_endpoint(USB_ENDPOINT_DIRECTION_READ);
            if(r)
                REQUIRE(USB_ENDPOINT_DIRECTION_READ == r->get_direction());
        }
    }
}

TEST_CASE("query_controls", "[live][usb]")
{
    auto  devices = usb_enumerator::query_devices();
    int device_counter = 0;
    const int REQ_TYPE_GET = 0xa1;
    const int UVC_GET_CUR = 129;
    int ctrl = 11;
    int unit = 3;
    for (auto&& dev : devices)
    {
        auto m = dev->open();
        std::vector<rs_usb_interface> interfaces = dev->get_interfaces(USB_SUBCLASS_CONTROL);

        for (auto&& intf : interfaces)
        {
            //int control_transfer(
            //int request_type,
            //int request,
            //int value,
            //int index,
            //uint8_t* buffer,
            //uint32_t length,
            //uint32_t timeout_ms);

            uint8_t data;
            auto sts = m->control_transfer(
                        REQ_TYPE_GET,
                        UVC_GET_CUR,
                        ctrl << 8,
                        unit << 8 | intf->get_number(),
                        &data,
                        1,
                        1000);
            REQUIRE(sts);
        }
    }
}

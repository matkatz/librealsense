// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#if (_MSC_FULL_VER < 180031101)
#error At least Visual Studio 2013 Update 4 is required to compile this backend
#endif

#include "device-winusb.h"
#include "win/win-helpers.h"

#include "endpoint-winusb.h"
#include "interface-winusb.h"
#include "types.h"

#include <atlstr.h>
#include <Windows.h>
#include <Sddl.h>
#include <string>
#include <regex>
#include <sstream>
#include <mutex>
#include <usbspec.h>

#include <SetupAPI.h>


#pragma comment(lib, "winusb.lib")

namespace librealsense
{
    namespace platform
    {
        void usb_device_winusb::parse_descriptor(WINUSB_INTERFACE_HANDLE handle)
        {
            USB_CONFIGURATION_DESCRIPTOR cfgDesc;
            ULONG returnLength = 0;
            if (!WinUsb_GetDescriptor(handle, USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, (PUCHAR)&cfgDesc, sizeof(cfgDesc), &returnLength))
            {
                throw winapi_error("WinUsb action failed, last error: " + GetLastError());
            }

            std::vector<uint8_t> config(cfgDesc.wTotalLength);

            // Returns configuration descriptor - including all interface, endpoint, class-specific, and vendor-specific descriptors
            if (!WinUsb_GetDescriptor(handle, USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, config.data(), cfgDesc.wTotalLength, &returnLength))
            {
                throw winapi_error("WinUsb action failed, last error: " + GetLastError());
            }

            for (int i = 0; i <config.size(); )
            {
                auto l = config[i];
                auto dt = config[i + 1];
                usb_descriptor ud = { l, dt, std::vector<uint8_t>(l) };
                memcpy(ud.data.data(), &config[i], l);
                _descriptors.push_back(ud);
                i += l;
            }
        }

        std::vector<std::shared_ptr<usb_interface>> usb_device_winusb::query_device_interfaces(const std::wstring& path)
        {
            std::vector<std::shared_ptr<usb_interface>> rv;
            handle_winusb handle;
            auto sts = handle.open(path);
            if (sts != RS2_USB_STATUS_SUCCESS)
                throw winapi_error("WinUsb action failed, " + GetLastError());

            auto handles = handle.get_handles();
            auto descriptors = handle.get_descriptors();

            for (auto&& i : handle.get_handles())
            {
                auto d = descriptors.at(i.first);
                auto intf = std::make_shared<usb_interface_winusb>(i.second, d, path);
                rv.push_back(intf);
            }

            parse_descriptor(handle.get_first_interface());


            return rv;
        }

        usb_device_winusb::usb_device_winusb(const usb_device_info& info, std::vector<std::wstring> devices_path)
            : _info(info)
        {
            for (auto&& device_path : devices_path)
            {
                auto intfs = query_device_interfaces(device_path);
                _interfaces.insert(_interfaces.end(), intfs.begin(), intfs.end());
            }
        }

        const std::shared_ptr<usb_messenger> usb_device_winusb::open(uint8_t interface_number)
        {
            if (interface_number >= _interfaces.size())
                return nullptr;
            auto intf = std::static_pointer_cast<usb_interface_winusb>(_interfaces[interface_number]);
            auto dh = std::make_shared<handle_winusb>();
            int tries = 5;
            while (tries-- > 0 && dh->open(intf->get_device_path()) != RS2_USB_STATUS_SUCCESS)
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if (tries == 0)
                return nullptr;
            return std::make_shared<usb_messenger_winusb>(shared_from_this(), dh);
        }
    }
}

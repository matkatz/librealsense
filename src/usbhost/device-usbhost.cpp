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
        usb_device_info generate_info(::usb_device *handle, int mi, usb_spec conn_spec, usb_class cls)
        {
            std::string name = usb_device_get_name(handle);
            usb_device_info rv{};
            rv.id = name;
            rv.unique_id = name;
            rv.vid = usb_device_get_vendor_id(handle);
            rv.pid = usb_device_get_product_id(handle);
            rv.mi = mi;
            rv.cls = cls;
            rv.conn_spec = conn_spec;
            return rv;
        }

        usb_device_usbhost::usb_device_usbhost(::usb_device* handle) :
                _handle(handle)
        {
            usb_descriptor_iter it;
            std::shared_ptr<usb_interface_usbhost> curr_ctrl_intf;
            ::usb_descriptor_iter_init(handle, &it);
            usb_descriptor_header *h = usb_descriptor_iter_next(&it);
            librealsense::platform::usb_spec conn_spec;
            if (h != nullptr && h->bDescriptorType == USB_DT_DEVICE) {
                usb_device_descriptor *device_descriptor = (usb_device_descriptor *) h;
                usb_descriptor ud = {h->bLength, h->bDescriptorType, std::vector<uint8_t>(h->bLength)};
                memcpy(ud.data.data(), h, h->bLength);
                _descriptors.push_back(ud);
                conn_spec = librealsense::platform::usb_spec(device_descriptor->bcdUSB);
            }

            do {
                h = usb_descriptor_iter_next(&it);
                if(h == NULL)
                    break;
                usb_descriptor ud = {h->bLength, h->bDescriptorType, std::vector<uint8_t>(h->bLength)};
                memcpy(ud.data.data(), h, h->bLength);
                _descriptors.push_back(ud);
                if (h->bDescriptorType == USB_DT_INTERFACE_ASSOCIATION) {
                    auto iad = *(usb_interface_assoc_descriptor *) h;
                    auto info = generate_info(_handle, iad.bFirstInterface, conn_spec,
                                              static_cast<usb_class>(iad.bFunctionClass));
                    _infos.push_back(info);
                }
                if (h->bDescriptorType == USB_DT_INTERFACE) {
                    auto id = *(::usb_interface_descriptor *) h;
                    auto intf = std::make_shared<usb_interface_usbhost>(id, it);
                    _interfaces.push_back(intf);
                    switch (id.bInterfaceClass)
                    {
                        case RS2_USB_CLASS_VENDOR_SPECIFIC:
                        case RS2_USB_CLASS_HID:
                        {
                            auto info = generate_info(_handle, id.bInterfaceNumber, conn_spec,
                                                      static_cast<usb_class>(id.bInterfaceClass));
                            _infos.push_back(info);
                            break;
                        }
                        case RS2_USB_CLASS_VIDEO:
                        {
                            if(id.bInterfaceSubClass == RS2_USB_SUBCLASS_VIDEO_CONTROL)
                                curr_ctrl_intf = intf;
                            if(id.bInterfaceSubClass == RS2_USB_SUBCLASS_VIDEO_STREAMING)
                                curr_ctrl_intf->add_associated_interface(intf);
                            break;
                        }
                        default:
                            break;
                    }
                }
            } while (h != nullptr);

            _usb_device_descriptor = usb_device_get_device_descriptor(_handle);

            _dispatcher = std::make_shared<dispatcher>(10);
            _dispatcher->start();
        }

        usb_device_usbhost::~usb_device_usbhost()
        {
            if(_dispatcher)
            {
                _dispatcher->stop();
                _dispatcher.reset();
            }
        }

        const rs_usb_interface usb_device_usbhost::get_interface(uint8_t interface_number) const
        {
            auto it = std::find_if(_interfaces.begin(), _interfaces.end(),
                                   [interface_number](const rs_usb_interface& i) { return interface_number == i->get_number(); });
            if (it == _interfaces.end())
                return nullptr;
            return *it;
        }

        const std::shared_ptr<usb_messenger> usb_device_usbhost::open(uint8_t interface_number)
        {
            auto i = get_interface(interface_number);
            if (!i)
                return nullptr;
            auto intf = std::dynamic_pointer_cast<usb_interface_usbhost>(i);
            auto h = std::make_shared<handle_usbhost>(_handle, intf);
            return std::make_shared<usb_messenger_usbhost>(shared_from_this(), h);
        }

        const std::vector<usb_descriptor> usb_device_usbhost::get_descriptors() const
        {
            return _descriptors;
        }

        usb_status usb_device_usbhost::submit_request(const rs_usb_request& request)
        {
            std::lock_guard<std::mutex> lk(_mutex);

            auto nr = reinterpret_cast<::usb_request*>(request->get_native_request());
            auto sts = usb_request_queue(nr);
            if(sts < 0)
            {
                std::string strerr = strerror(errno);
                LOG_WARNING("usb_request_queue returned error, endpoint: " << (int)request->get_endpoint()->get_address() << " error: " << strerr << ", number: " << (int)errno);
                return usbhost_status_to_rs(errno);
            }
            _active_requsts.push_back(request);
            invoke();
            return RS2_USB_STATUS_SUCCESS;
        }

        usb_status usb_device_usbhost::cancel_request(const rs_usb_request& request)
        {
            std::lock_guard<std::mutex> lk(_mutex);

            auto nr = reinterpret_cast<::usb_request*>(request->get_native_request());
            auto sts = usb_request_cancel(nr);

            if(sts < 0)
            {
                std::string strerr = strerror(errno);
                LOG_WARNING("usb_request_cancel returned error, endpoint: " << (int)request->get_endpoint()->get_address() << ", error: " << strerr << ", number: " << (int)errno);
                return usbhost_status_to_rs(errno);
            }
            return RS2_USB_STATUS_SUCCESS;
        }

        void usb_device_usbhost::invoke()
        {
            _dispatcher->invoke([&](dispatcher::cancellable_timer c)
            {
                auto r = usb_request_wait(get_handle(), -1);

                if(r)
                {
                    auto urb = reinterpret_cast<usb_request_usbhost*>(r->client_data);

                    if(urb)
                    {
                        auto response = urb->get_shared();
                        if(response)
                        {
                            auto cb = response->get_callback();
                            cb->callback(response);
                        }

                        std::lock_guard<std::mutex> lk(_mutex);
                        auto it = std::find_if(_active_requsts.begin(), _active_requsts.end(),
                                [response] (const rs_usb_request& urb) { return urb == response; });
                        if(it != _active_requsts.end())
                            _active_requsts.erase(it);
                    }
                }
            });
        }
    }
}
#endif
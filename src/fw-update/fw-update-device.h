// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/streaming.h"
#include "usb/usb-device.h"

namespace librealsense
{
    class fw_update_device : public device_interface
    {
    public:
        fw_update_device(std::shared_ptr<context> ctx, bool register_device_notifications, std::shared_ptr<platform::usb_device> usb_device);
        virtual ~fw_update_device();

        void update_fw(const void* fw_image, int fw_image_size, fw_update_progress_callback_ptr = nullptr);
        
        virtual sensor_interface& get_sensor(size_t i) override;

        virtual const sensor_interface& get_sensor(size_t i) const override;

        virtual size_t get_sensors_count() const override;

        virtual void hardware_reset() override;

        virtual std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        virtual std::shared_ptr<context> get_context() const override;

        virtual platform::backend_device_group get_device_data() const override;

        virtual std::pair<uint32_t, rs2_extrinsics> get_extrinsics(const stream_interface& stream) const override;

        virtual bool is_valid() const override;

        virtual std::vector<tagged_profile> get_profiles_tags() const override;

        virtual void tag_profiles(stream_profiles profiles) const override;

        virtual bool compress_while_record() const override;

        virtual bool contradicts(const stream_profile_interface* a, const std::vector<stream_profile>& others) const override { return false; }

        virtual void enter_to_fw_update_mode() const override;

        //info_interface
        virtual const std::string& get_info(rs2_camera_info info) const override;

        virtual bool supports_info(rs2_camera_info info) const override;

        //recordable
        virtual void create_snapshot(std::shared_ptr<info_interface>& snapshot) const override;

        virtual void enable_recording(std::function<void(const info_interface&)> recording_function) override;

    private:
        std::shared_ptr<context> _context;
        std::shared_ptr<platform::usb_device> _usb_device;
        std::string _serial_number;
    };

    MAP_EXTENSION(RS2_EXTENSION_FW_UPDATE_DEVICE, fw_update_device);
}

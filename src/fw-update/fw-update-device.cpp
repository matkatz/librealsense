// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "fw-update-device.h"
#include "../types.h"
#include "../context.h"

#include <chrono>
#include <stdexcept>
#include <algorithm>
#include <thread>

typedef enum rs2_dfu_status {
    RS2_DFU_STATUS_OK = 0x00,
    RS2_DFU_STATUS_TARGET = 0x01,       // File is not targeted for use by this device.
    RS2_DFU_STATUS_FILE = 0x02,         // File is for this device, but fails some vendor-specific verification test.
    RS2_DFU_STATUS_WRITE = 0x03,        // Device is unable to write memory.
    RS2_DFU_STATUS_ERASE = 0x04,        // Memory erase function failed.
    RS2_DFU_STATUS_CHECK_ERASED = 0x05, // Memory erase check failed.
    RS2_DFU_STATUS_PROG = 0x06,         // Program memory function failed.
    RS2_DFU_STATUS_VERIFY = 0x07,       // Programmed memory failed verification.
    RS2_DFU_STATUS_ADDRESS = 0x08,      // Cannot program memory due to received address that is out of range.
    RS2_DFU_STATUS_NOTDONE = 0x09,      // Received DFU_DNLOAD with wLength = 0, but device does    not think it has all of the data yet.
    RS2_DFU_STATUS_FIRMWARE = 0x0A,     // Device’s firmware is corrupt.It cannot return to run - time    (non - DFU) operations.
    RS2_DFU_STATUS_VENDOR = 0x0B,       // iString indicates a vendor - specific RS2_DFU_STATUS_or.
    RS2_DFU_STATUS_USBR = 0x0C,         // Device detected unexpected USB reset signaling.
    RS2_DFU_STATUS_POR = 0x0D,          // Device detected unexpected power on reset.
    RS2_DFU_STATUS_UNKNOWN = 0x0E,      //  Something went wrong, but the device does not know what it    was.
    RS2_DFU_STATUS_STALLEDPKT = 0x0F    // Device stalled an unexpected request.
} rs2_dfu_status;

typedef enum rs2_dfu_state {
    RS2_DFU_STATE_APP_IDLE = 0,
    RS2_DFU_STATE_APP_DETACH = 1,
    RS2_DFU_STATE_DFU_IDLE = 2,
    RS2_DFU_STATE_DFU_DOWNLOAD_SYNC = 3,
    RS2_DFU_STATE_DFU_DOWNLOAD_BUSY = 4,
    RS2_DFU_STATE_DFU_DOWNLOAD_IDLE = 5,
    RS2_DFU_STATE_DFU_MANIFEST_SYNC = 6,
    RS2_DFU_STATE_DFU_MANIFEST = 7,
    RS2_DFU_STATE_DFU_MANIFEST_WAIT_RESET = 8,
    RS2_DFU_STATE_DFU_UPLOAD_IDLE = 9,
    RS2_DFU_STATE_DFU_ERROR = 10
} rs2_dfu_state;

typedef enum rs2_dfu_command {
    RS2_DFU_DETACH = 0,
    RS2_DFU_DOWNLOAD = 1,
    RS2_DFU_UPLOAD = 2,
    RS2_DFU_GET_STATUS = 3,
    RS2_DFU_CLEAR_STATUS = 4,
    RS2_DFU_GET_STATE = 5,
    RS2_DFU_ABORT = 6
} rs2_dfu_command;

struct dfu_status_payload {
    uint32_t    bStatus : 8;
    uint32_t    bwPollTimeout : 24;
    uint8_t    bState;
    uint8_t    iString;

    dfu_status_payload()
    {
        bStatus = RS2_DFU_STATUS_UNKNOWN;
        bwPollTimeout = 0;
        bState = RS2_DFU_STATE_DFU_ERROR;
        iString = 0;
    }

    bool is_in_state(const rs2_dfu_state state) const
    {
        return bStatus == RS2_DFU_STATUS_OK && bState == state;
    }

    bool is_error_state() const { return bState == RS2_DFU_STATE_DFU_ERROR; }
    bool is_ok() const { return bStatus == RS2_DFU_STATUS_OK; }
    rs2_dfu_state get_state() { return static_cast<rs2_dfu_state>(bState); }
    rs2_dfu_status get_status() { return static_cast<rs2_dfu_status>(bStatus); }
};

struct serial_number_data
{
    uint8_t serial[6];
    uint8_t spare[2];
};

struct dfu_fw_status_payload
{
    uint32_t spare1;
    uint32_t fw_last_version;
    uint32_t fw_highest_version;
    uint16_t fw_download_status;
    uint16_t dfu_is_locked;
    uint16_t dfu_version;
    serial_number_data serial_number;
    uint8_t spare2[42];
};

namespace librealsense
{
    rs2_dfu_state get_dfu_state(std::shared_ptr<platform::usb_messenger> messenger)
    {
        uint8_t state = RS2_DFU_STATE_DFU_ERROR;
        messenger->control_transfer(0xa1 /*DFU_GETSTATUS_PACKET*/, RS2_DFU_GET_STATE, 0, 0, &state, 1, 10);
        return (rs2_dfu_state)state;
    }

    void detach(std::shared_ptr<platform::usb_messenger> messenger)
    {
        auto timeout = 1000;
        messenger->control_transfer(0x21 /*DFU_DETACH_PACKET*/, RS2_DFU_DETACH, timeout, 0, NULL, 0, timeout);
    }

    bool wait_for_state(std::shared_ptr<platform::usb_messenger> messenger, const rs2_dfu_state state, size_t timeout = 1000)
    {
        std::chrono::milliseconds elapsed_milliseconds;
        auto start = std::chrono::system_clock::now();
        do {
            dfu_status_payload status;
            messenger->control_transfer(0xa1 /*DFU_GETSTATUS_PACKET*/, RS2_DFU_GET_STATUS, 0, 0, (uint8_t*)&status, sizeof(status), 5000);

            if (status.is_in_state(state)) {
                return true;
            }

            //test for dfu error state
            if (status.is_error_state()) {
                return false;
            }

            // FW doesn't set the bwPollTimeout value, therefore it is wrong to use status.bwPollTimeout
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            auto curr = std::chrono::system_clock::now();
            elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(curr - start);
        } while (elapsed_milliseconds < std::chrono::milliseconds(timeout));

        return false;
    }

    fw_update_device::fw_update_device(std::shared_ptr<context> ctx, bool register_device_notifications, std::shared_ptr<platform::usb_device> usb_device) : _context(ctx), _usb_device(usb_device)
    {
        auto messenger = _usb_device->open();

        auto state = get_dfu_state(messenger);
        detach(messenger);
        state = get_dfu_state(messenger);

        if (state != RS2_DFU_STATE_DFU_IDLE)
            throw std::runtime_error("failed to enter into dfu state");

        dfu_fw_status_payload paylaod;
        auto rv = messenger->control_transfer(0xa1, RS2_DFU_UPLOAD, 0, 0, reinterpret_cast<uint8_t*>(&paylaod), sizeof(paylaod), 10);

        std::stringstream formattedBuffer;
        for (auto i = 0; i < sizeof(paylaod.serial_number.serial); i++)
            formattedBuffer << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(paylaod.serial_number.serial[i]);

        _serial_number = formattedBuffer.str();
    }

    fw_update_device::~fw_update_device()
    {

    }

    void fw_update_device::update_fw(const void* fw_image, int fw_image_size, fw_update_progress_callback_ptr update_progress_callback)
    {
        auto messenger = _usb_device->open();

        const size_t transfer_size = 1024;

        size_t remaining_bytes = fw_image_size;
        uint16_t blocks_count = fw_image_size / transfer_size;
        uint16_t block_number = 0;

        size_t offset = 0;

        while (remaining_bytes > 0) 
        {
            size_t chunk_size = std::min(transfer_size, remaining_bytes);

            auto curr_block = ((uint8_t*)fw_image + offset);
            messenger->control_transfer(0x21 /*DFU_DOWNLOAD_PACKET*/, RS2_DFU_DOWNLOAD, block_number, 0, curr_block, chunk_size, 10);
            if(!wait_for_state(messenger, RS2_DFU_STATE_DFU_DOWNLOAD_IDLE))
                throw std::runtime_error("failed to download firmware");

            block_number++;
            remaining_bytes -= chunk_size;
            offset += chunk_size;

            float progress = (float)block_number / (float)blocks_count;
            LOG_DEBUG("fw update progress: " << progress);
            if (update_progress_callback)
                update_progress_callback->on_fw_update_progress(progress);
        }

        // After the final block of firmware has been sent to the device and the status solicited, the host sends a
        // DFU_DNLOAD request with the wLength field cleared to 0 and then solicits the status again.If the
        // result indicates that the device is ready and there are no errors, then the Transfer phase is complete and
        // the Manifestation phase begins.
        messenger->control_transfer(0x21 /*DFU_DOWNLOAD_PACKET*/, RS2_DFU_DOWNLOAD, block_number, 0, NULL, 0, 10);

        // After the zero length DFU_DNLOAD request terminates the Transfer
        // phase, the device is ready to manifest the new firmware. As described
        // previously, some devices may accumulate the firmware image and perform
        // the entire reprogramming operation at one time. Others may have only a
        // small amount remaining to be reprogrammed, and still others may have
        // none. Regardless, the device enters the dfuMANIFEST-SYNC state and
        // awaits the solicitation of the status report by the host. Upon receipt
        // of the anticipated DFU_GETSTATUS, the device enters the dfuMANIFEST
        // state, where it completes its reprogramming operations.

        // WaitForDFU state sends several DFU_GETSTATUS requests, until we hit
        // either RS2_DFU_STATE_DFU_MANIFEST_WAIT_RESET or RS2_DFU_STATE_DFU_ERROR status.
        // This command also reset the device
        if (!wait_for_state(messenger, RS2_DFU_STATE_DFU_MANIFEST_WAIT_RESET))
            throw std::runtime_error("firmware manifest failed");
    }

    sensor_interface& fw_update_device::get_sensor(size_t i)
    { 
        throw std::runtime_error("try to get sensor from fw loader device");
    }

    const sensor_interface& fw_update_device::get_sensor(size_t i) const
    {
        throw std::runtime_error("try to get sensor from fw loader device");
    }

    size_t fw_update_device::get_sensors_count() const
    {
        return 0;
    }

    void fw_update_device::hardware_reset()
    {
        //TODO_MK
    }

    std::shared_ptr<matcher> fw_update_device::create_matcher(const frame_holder& frame) const
    {
        return nullptr;
    }

    std::shared_ptr<context> fw_update_device::get_context() const
    {
        return _context;
    }

    platform::backend_device_group fw_update_device::get_device_data() const
    {
        throw std::runtime_error("get_device_data is not supported by fw_update_device");//TODO_MK
    }

    std::pair<uint32_t, rs2_extrinsics> fw_update_device::get_extrinsics(const stream_interface& stream) const
    {
        throw std::runtime_error("get_extrinsics is not supported by fw_update_device");
    }

    bool fw_update_device::is_valid() const
    {
        return true;
    }

    std::vector<tagged_profile> fw_update_device::get_profiles_tags() const
    {
        return std::vector<tagged_profile>();
    }

    void fw_update_device::tag_profiles(stream_profiles profiles) const
    {
    
    }

    bool fw_update_device::compress_while_record() const
    {
        return false;
    }

    void fw_update_device::enter_to_fw_update_mode() const
    {
        throw std::runtime_error("enter_to_fw_update_mode is not supported by fw_update_device");
    }

    const std::string& fw_update_device::get_info(rs2_camera_info info) const
    {
        if (info == RS2_CAMERA_INFO_SERIAL_NUMBER)
            return _serial_number;
        throw std::runtime_error(std::string(rs2_camera_info_to_string(info)) + " is not supported by fw update device");
    }

    bool fw_update_device::supports_info(rs2_camera_info info) const
    {
        if (info == RS2_CAMERA_INFO_SERIAL_NUMBER)
            return true;
        return false;
    }

    void fw_update_device::create_snapshot(std::shared_ptr<info_interface>& snapshot) const
    {
        
    }
    void fw_update_device::enable_recording(std::function<void(const info_interface&)> record_action)
    {
        
    }
}

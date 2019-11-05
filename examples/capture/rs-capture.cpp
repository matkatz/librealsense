// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering

int main(int argc, char * argv[]) try
{
    rs2::log_to_console(RS2_LOG_SEVERITY_ERROR);
    window app(1280, 720, "RealSense Capture Example");

    rs2::colorizer color_map;

    rs2::pipeline pipe;
    rs2::config cfg;
    int width = 640;
    int height = 480;
    cfg.enable_stream(RS2_STREAM_COLOR, width, height);
    cfg.enable_stream(RS2_STREAM_DEPTH, width, height);
    cfg.enable_stream(RS2_STREAM_INFRARED, width, height);

    auto p = cfg.resolve(pipe);
    auto dev = p.get_device();

    auto sensors = dev.query_sensors();

    rs2::sensor cs;
    rs2::sensor ds;

    for (auto&& s : sensors)
    {
        for (auto&& p : s.get_stream_profiles())
        {
            if (p.stream_type() == RS2_STREAM_COLOR && !cs)
                cs = s;
            if (p.stream_type() == RS2_STREAM_DEPTH && !cs)
                ds = s;
        }
    }
    auto rcs = cs.as<rs2::roi_sensor>();

    pipe.start(cfg);

    bool state = false;
    int frame_count = 0;
    while (app) // Application still alive?
    {
        rs2::frameset data = pipe.wait_for_frames().apply_filter(color_map);
        if (frame_count++ % 10 == 0)
        {
            auto roi = state ? rs2::region_of_interest{ 0, 0, width / 2, height / 2 } : rs2::region_of_interest{ width / 2, height / 2, width - 1, height - 1 };
            data.foreach_rs([](const rs2::frame& f) { printf(" %s: %d", f.get_profile().stream_name().c_str(), f.get_frame_number()); });
            printf(", frameset: %d\n", frame_count);

            printf("set new roi: %d, %d, %d, %d\n", roi.min_x, roi.min_y, roi.max_x, roi.max_y);
            rcs.set_region_of_interest(roi);
            auto new_roi = rcs.get_region_of_interest();
            printf("get new roi: %d, %d, %d, %d\n\n", new_roi.min_x, new_roi.min_y, new_roi.max_x, new_roi.max_y);
            state = !state;
        }
        app.show(data);
    }

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
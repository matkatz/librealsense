
// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering
#include <thread>
#include <condition_variable>
#include <mutex>
// Capture Example demonstrates how to
// capture depth and color video streams and render them to the screen
int main(int argc, char * argv[]) try
{
    rs2::log_to_console(RS2_LOG_SEVERITY_ERROR);
    // window app(1280, 720, "RealSense Capture Example");
    rs2::colorizer color_map;

    rs2::context ctx;
    auto devs = ctx.query_devices(RS2_PRODUCT_LINE_ANY);
    auto dev = devs[0];

    auto sensors = dev.query_sensors();
    rs2::sensor color_sensor;
    rs2::stream_profile color_stream_profile;
    for (auto&& s : sensors)
    {
        auto profiles = s.get_stream_profiles();
        for (auto&& p : profiles)
        {
            if (p.is<rs2::video_stream_profile>() && p.stream_type() == RS2_STREAM_COLOR && p.as<rs2::video_stream_profile>().width() == 1920)
            {
                color_sensor = s;
                color_stream_profile = p;
                break;
            }
        }
    }

    std::condition_variable cv;
    std::mutex m;

    int frames = 1;
    int iteration = 0;
    while (true)
    {
        printf("iteration: %d\n", ++iteration);
        int frame_count = 0;

        color_sensor.open(color_stream_profile);
        color_sensor.start([&](const rs2::frame& f)
        {
            if (auto vf = f.as<rs2::video_frame>())
                printf("frame: %d, width: %d, height: %d\n", vf.get_frame_number(), vf.get_width(), vf.get_height());
            if (frames == ++frame_count)
                cv.notify_one();
        });

        std::unique_lock<std::mutex> lk(m);
        if (!cv.wait_for(lk, std::chrono::milliseconds(5000), [&]() { return frame_count >= frames; }))
        {
            throw std::runtime_error("no frame arrived");
        }
        color_sensor.stop();
        color_sensor.close();
        // if(iteration == 3)
        //     break;
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
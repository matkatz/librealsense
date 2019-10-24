
// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering

// Capture Example demonstrates how to
// capture depth and color video streams and render them to the screen
int main(int argc, char * argv[]) try
{
    rs2::log_to_console(RS2_LOG_SEVERITY_WARN);
    window app(1280, 720, "RealSense Capture Example");
    rs2::colorizer color_map;


    rs2::pipeline pipe;

    int frames = 10; //30 * 5;
    int iteration = 0;
    while (true)
    {
        printf("iteration: %d\n", ++iteration);

        pipe.start();
        for (int i = 0; i < frames; i++)
        {
            rs2::frameset data = pipe.wait_for_frames();
            printf("frame: %d, size: %d\n", data.get_frame_number(), data.size());
            if (app)
            {
                auto v = data.apply_filter(color_map);
                app.show(v);
            }
        }

        pipe.stop();
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

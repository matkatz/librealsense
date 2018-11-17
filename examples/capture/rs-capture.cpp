// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering

// Capture Example demonstrates how to
// capture depth and color video streams and render them to the screen
int main(int argc, char * argv[]) try
{
    rs2::log_to_console(RS2_LOG_SEVERITY_ERROR);

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::async_streamer pipe;

    pipe.set_callback([&](rs2::frame f) 
    {
        std::cout << "stream type: " << f.get_profile().stream_name() << ", number: " << f.get_frame_number() << std::endl;
    });

    // Start streaming with default recommended configuration
    pipe.start();

    while (true)
    {
        char q;
        std::cin >> q;
        if(q == 'q')
            break;
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

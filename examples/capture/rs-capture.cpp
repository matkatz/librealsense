// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering
#include <fstream>

void save(const rs2::frame& f)
{
    if (!f)
        return;
    auto n = f.get_frame_number();
    auto d = f.get_data();
    auto s = f.get_data_size();
    std::string name = std::string("images\\img_") + std::to_string(n) + std::string(".jpg");
    std::ofstream ofile(name, std::ios::binary);
    ofile.write((char*)d, s);
    ofile.close();
}

// Capture Example demonstrates how to
// capture depth and color video streams and render them to the screen
int main(int argc, char * argv[]) try
{
    rs2::log_to_console(RS2_LOG_SEVERITY_ERROR);
    // Create a simple OpenGL window for rendering:
    window app(1280, 720, "RealSense Capture Example");

    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;
    // Declare rates printer for showing streaming rates of the enabled streams.
    rs2::rates_printer printer;

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;
    rs2::config cfg;

    cfg.enable_stream(RS2_STREAM_COLOR, RS2_FORMAT_MJPEG);
    // Start streaming with default recommended configuration
    // The default video configuration contains Depth and Color streams
    // If a device is capable to stream IMU data, both Gyro and Accelerometer are enabled by default
    pipe.start(cfg);

    while (app) // Application still alive?
    {

        auto c = pipe.wait_for_frames().first(RS2_STREAM_COLOR);
        save(c);
        //if (c) {
        //    auto d = c.get_data();
        //    auto s = c.get_data_size();
        //    printf("%d\n", s);
        //}
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
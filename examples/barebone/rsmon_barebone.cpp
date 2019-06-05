#include "librealsense2/rs.hpp"
#include <cstdio>
#include <string>
#include <chrono>
#include <thread>
#include "rsmon_utils.h"

FILE* log_file;

/* This class mimics the way RS is normally used in our applications. */
struct RealsenseHandle
{
    RealsenseHandle()
    {
        const int width = 640, height = 480;
        fprintf(log_file, "[%s] RealsenseHandle(): Entry\n", utils::localtime_str().c_str());

        rs2::config cfg;
        cfg.enable_stream(RS2_STREAM_COLOR, width, height, RS2_FORMAT_RGB8);
        fprintf(log_file, "  enable_stream(RS2_STREAM_COLOR, %d, %d, RS2_FORMAT_RGB8)\n", width, height);

        cfg.enable_stream(RS2_STREAM_DEPTH, width, height, RS2_FORMAT_Z16);
        fprintf(log_file, "  enable_stream(RS2_STREAM_DEPTH, %d, %d, RS2_FORMAT_Z16)\n", width, height);
        
        fprintf(log_file, "  Starting pipeline...\n");
        pipe.start(cfg);
        fprintf(log_file, "  Pipeline started\n");

        model = pipe.get_active_profile().get_device().get_info(RS2_CAMERA_INFO_NAME);
        
        depth_scale = pipe.get_active_profile().get_device().first<rs2::depth_sensor>().get_depth_scale();
        depth_scale = depth_scale * 100 * 10;
        fprintf(log_file, "  Depth sensor scale factor: %.5f\n", depth_scale);

        auto sensors = pipe.get_active_profile().get_device().query_sensors();
        for(auto& sensor : sensors)
        {
            if(sensor.supports(RS2_OPTION_ENABLE_AUTO_EXPOSURE))
            {
                sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 1.0f);
                sensor.set_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE, 1.0f);
            }
        }

        fprintf(log_file, "  Device '%s' started\n\n", model.c_str());
    }

    ~RealsenseHandle()
    {
        fprintf(log_file, "[%s] ~RealsenseHandle(): Entry\n", utils::localtime_str().c_str());
        fprintf(log_file, "  Device '%s' stopped\n\n", model.c_str());
    }

    void run_monitoring()
    {
        fprintf(log_file, "[%s] run_monitoring(): Entry\n", utils::localtime_str().c_str());
        auto dev = pipe.get_active_profile().get_device();

        fprintf(log_file, "  Device: %s\n", dev.get_info(RS2_CAMERA_INFO_NAME));
        fprintf(log_file, "  Serial: %s\n", dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
        fprintf(log_file, "  Firmware: %s\n", dev.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION));

        auto sensors = dev.query_sensors();
        fprintf(log_file, "  Found %d sensor(s):\n", sensors.size());

        std::vector<float> temps;
        
        int index = -1;
        for(rs2::sensor sensor : sensors)
        {
            ++index;
            fprintf(log_file, "    Sensor #%d (%s):\n", index, sensor.get_info(RS2_CAMERA_INFO_NAME));

            float asic_temp = -1.0f, proj_temp = -1.0f, mov_temp = -1.0f;
            float laser_power = 0.0f;
            float emitter_enabled = 0.0f;

            if(sensor.supports(RS2_OPTION_ASIC_TEMPERATURE))
                fprintf(log_file, "      ASIC T.:         %.2f\n", sensor.get_option(RS2_OPTION_ASIC_TEMPERATURE));
            else
                fprintf(log_file, "      ASIC T.:         N/A\n");

            if(sensor.supports(RS2_OPTION_PROJECTOR_TEMPERATURE))
                fprintf(log_file, "      Projector T.:    %.2f\n", sensor.get_option(RS2_OPTION_PROJECTOR_TEMPERATURE));
            else
                fprintf(log_file, "      Projector T.:    N/A\n");

            if(sensor.supports(RS2_OPTION_MOTION_MODULE_TEMPERATURE))
                fprintf(log_file, "      Motion mod. T.:  %.2f\n",
                        sensor.get_option(RS2_OPTION_MOTION_MODULE_TEMPERATURE));
            else
                fprintf(log_file, "      Motion mod. T.:  N/A\n");

            if(sensor.supports(RS2_OPTION_EMITTER_ENABLED))
                fprintf(log_file, "      Emitter enabled: %s\n",
                        (sensor.get_option(RS2_OPTION_EMITTER_ENABLED) > 0 ? "true" : "false"));
            else
                fprintf(log_file, "      Emitter enabled: N/A\n");

            if(sensor.supports(RS2_OPTION_LASER_POWER))
                fprintf(log_file, "      Laser power:     %.2f\n", sensor.get_option(RS2_OPTION_LASER_POWER));
            else
                fprintf(log_file, "      Laser power:     N/A\n");
        }
        fprintf(log_file, "\n");
    }

    rs2::pipeline pipe;
    std::string model;
    float depth_scale;
};

bool open_log()
{
    log_file = fopen("log.txt", "ab");
    if(!log_file)
    {
        printf("FATAL ERROR: Failed to open 'log.txt': %s\n", strerror(errno));
        return false;
    }
    return true;
}

void close_log()
{
    fclose(log_file);
}

int main()
{
    setbuf(stdout, NULL); // disable buffering
    rs2::log_to_file(RS2_LOG_SEVERITY_DEBUG, "rs_log.txt");

    std::chrono::seconds interval{5};

    open_log();
    fprintf(log_file, "---------------------------------------------\n");
    fprintf(log_file, "               SESSION START                 \n");
    close_log();

    /* This setup simulates the way RS camera is used in our application:
    pipeline is started and at the same time camera info is read. */
    int index = -1;
    while(true)
    {
        ++index;
        rs2::log_to_file(RS2_LOG_SEVERITY_DEBUG, "rs_log.txt"); // force file flush
        if(open_log())
        {
            fprintf(log_file, "---------------------------------------------\n");
            fprintf(log_file, "[%s] Attempt #%d:\n", utils::localtime_str().c_str(), index);

            printf("[%s] Attempt #%d... ", utils::localtime_str().c_str(), index);
            try
            {
                RealsenseHandle cam;
                cam.run_monitoring();
                printf("OK\n");
            }
            catch(const std::exception& e)
            {
                printf("FAILED\n");
                fprintf(log_file, "[%s] ERROR: %s\n\n", utils::localtime_str().c_str(), e.what());
            }
            fprintf(log_file, "\n");
            close_log();
        }
        else
        {
            return -1;
        }
        //std::this_thread::sleep_for(interval);
    }
    return 0;
}

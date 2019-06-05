#pragma once
#include <time.h>
#include <string>

namespace utils
{
    /* Returns current local time as string. */
    inline auto localtime_str() -> std::string
    {
        char timestamp[64];
        memset(timestamp, 0, sizeof(timestamp));

        time_t now_raw;
        tm* now_local;
        time(&now_raw);
        now_local = localtime(&now_raw);

        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", now_local);

        return std::string(timestamp);
    }
} // namespace utils

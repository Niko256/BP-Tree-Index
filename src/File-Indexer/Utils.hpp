#pragma once
#include <ctime>
#include <string>
#include <cstddef>

namespace utils {
    std::string format_size(size_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unit = 0;
        double size = bytes;
        
        while (size >= 1024 && unit < 4) {
            size /= 1024;
            unit++;
        }
        
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unit]);
        return buffer;
    }

    std::string format_time(std::time_t time) {
        char buffer[64];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&time));
        return buffer;
    }
}

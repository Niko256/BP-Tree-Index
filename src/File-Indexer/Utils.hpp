#pragma once
#include <ctime>
#include <string>
#include <cstddef>


namespace utils {

    /**
     * @brief Formats a size in bytes into a human-readable string.
     *
     * This function converts a size in bytes into a more readable format,
     */
    inline std::string format_size(size_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"}; // Array of size units.
        int unit = 0; // Index of the current unit.
        double size = bytes; // Size in the current unit.
        
        // Convert size to the appropriate unit.
        while (size >= 1024 && unit < 4) {
            size /= 1024;
            unit++;
        }
        
        // Format the size into a string.
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unit]);
        return buffer;
    }

    /**
     * @brief Formats a timestamp into a human-readable string.
     *
     * This function converts a `std::time_t` timestamp into a formatted string
     * using the format `YYYY-MM-DD HH:MM:SS`.
     *
     * @return A string representing the formatted time (e.g., "2025-01-27 14:30:00").
     */
    inline std::string format_time(std::time_t time) {
        char buffer[64];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&time));
        return buffer;
    }
}

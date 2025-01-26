#pragma once

#include <cstddef>
#include <ctime>
#include <string>

/**
 * @struct FileInfo
 * @brief Represents metadata about a file or directory.
 */

struct FileInfo {
    size_t id_; // Unique identifier for the file.

    std::string name_; // Name of the file or directory.

    std::string path_; // Full path to the file or directory.
    
    size_t size_; // Size of the file in bytes.
    
    std::string extension_; // File extension (e.g., ".txt", ".jpg").
    
    std::time_t modified_time_; // Last modification time of the file.
    
    bool is_dir_; // Whether the entry is a directory.
    
    std::string content_type_; // MIME type or content type of the file.
    
    size_t get_id() const { return id_; }
};

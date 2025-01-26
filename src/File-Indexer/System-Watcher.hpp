#pragma once

#include <atomic>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <map>
#include <system_error>

/**
 * @enum FileSystemEvent
 * @brief Represents the type of file system event.
 */
enum class FileSystemEvent {
    CREATED, // A file or directory was created.
    MODIFIED, // A file or directory was modified.
    DELETED  // A file or directory was deleted.
};


/**
 * @class FileSystemWatcher
 * @brief Monitors a directory for changes and invokes a callback when changes are detected.
 *
 * This class uses a separate thread to monitor the specified directory for changes.
 * When a change is detected, the provided callback function is invoked with the path of the affected file
 * and the type of event.
 */
class FileSystemWatcher {
private:
    std::thread watch_thread_; // Thread used to monitor the directory.
    std::atomic<bool> should_stop_{false}; // Flag to stop the monitoring thread.
    std::string watch_path_; // Path of the directory to monitor.
    std::function<void(const std::string&, FileSystemEvent)> callback_; // Callback function to invoke on changes.

    /**
     * @struct FileInfo
     * @brief Represents metadata about a file or directory for monitoring purposes.
     */
    struct FileInfo {
        std::filesystem::file_time_type last_write_time_; // Last modification time of the file.
        uintmax_t size_; // Size of the file in bytes.
        bool is_directory; // Whether the entry is a directory.
        
        /**
         * @brief Compares two FileInfo objects for inequality.
         */
        bool operator!=(const FileInfo& other) const {
            return last_write_time_ != other.last_write_time_ ||
                   size_ != other.size_;
        }
    };

public:
    /**
     * @brief Starts monitoring the specified directory.
     *
     * @param callback A callback function that will be invoked when a file system event is detected.
     */
    void start(const std::function<void(const std::string&, FileSystemEvent)>& callback) {
        callback_ = callback;
        should_stop_ = false;
        watch_thread_ = std::thread([this]() { this->watch_directory(); });
    }

    /**
     * @brief Stops monitoring the directory.
     */
    void stop() {
        should_stop_ = true;
        if (watch_thread_.joinable()) {
            watch_thread_.join();
        }
    }

private:

    /**
     * @brief Retrieves information about a file or directory.
     *
     * @param path The path to the file or directory.
     * @return FileInfo containing the last write time, size, and whether it is a directory.
     */
    FileInfo get_file_info(const std::filesystem::path& path) {
        FileInfo info;
        std::error_code ec;
        
        info.is_directory = std::filesystem::is_directory(path, ec);
        if (ec) return info;
        
        info.last_write_time_ = std::filesystem::last_write_time(path, ec);
        if (ec) return info;
        
        if (!info.is_directory) {
            info.size_ = std::filesystem::file_size(path, ec);
            if (ec) info.size_ = 0;
        }
        
        return info;
    }

    /**
     * @brief Monitors the directory for changes.
     */
    void watch_directory() {
        std::map<std::string, FileInfo> file_status; // Tracks the state of files in the directory
        
        init_file_status(file_status); // Initialize the initial state of files
        
        while (!should_stop_) {
            try {
                check_for_changes(file_status); // Check for changes in the directory
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep to avoid busy-waiting
            } catch (const std::exception& e) {
                std::cerr << "Error watching directory: " << e.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait before retrying
            }
        }
    }

    
    /**
     * @brief Initializes the file status map with the current state of the directory.
     *
     * @param file_status A map to store the initial state of files.
     */
    void init_file_status(std::map<std::string, FileInfo>& file_status) {
        namespace fs = std::filesystem;
        std::error_code ec;
        
        for (const auto& entry : fs::recursive_directory_iterator(
                watch_path_, 
                fs::directory_options::skip_permission_denied, ec)) {
            if (ec) {
                ec.clear();
                continue;
            }
            
            std::string path = entry.path().string();
            file_status[path] = get_file_info(entry.path());
        }
    }

    /**
     * @brief Checks for changes in the directory and invokes the callback if changes are detected.
     *
     * @param file_status A map containing the previous state of files.
     */
    void check_for_changes(std::map<std::string, FileInfo>& file_status) {
        namespace fs = std::filesystem;
        std::error_code ec;
        std::map<std::string, FileInfo> current_status; // Tracks the current state of files
        
        for (const auto& entry : fs::recursive_directory_iterator(
                watch_path_,
                fs::directory_options::skip_permission_denied, ec)) {
            if (ec) {
                ec.clear();
                continue;
            }
            
            std::string path = entry.path().string();
            current_status[path] = get_file_info(entry.path());
            
            auto it = file_status.find(path);
            if (it == file_status.end()) {
                // File was created
                callback_(path, FileSystemEvent::CREATED);
            } else if (current_status[path] != it->second) {
                // File was modified
                callback_(path, FileSystemEvent::MODIFIED);
            }
        }
        
        // Check for deleted files
        for (const auto& [path, info] : file_status) {
            if (current_status.find(path) == current_status.end()) {
                // File was deleted
                callback_(path, FileSystemEvent::DELETED);
            }
        }
        
        // Update the file status map
        file_status = std::move(current_status);
    }
};

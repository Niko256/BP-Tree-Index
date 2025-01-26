#pragma once

#include "../Index.hpp"
#include "File-Info.hpp"
#include <ctime>
#include <utility>


/**
 * @class NameIndex
 * @brief Indexes files by their names.
 */
class NameIndex : public Index<FileInfo, std::string> {
  public:
    NameIndex() : Index([](const FileInfo& file) { return file.path_; }) {}
};


/**
 * @class SizeIndex
 * @brief Indexes files by their sizes.
 */
class SizeIndex : public Index<FileInfo, std::pair<size_t, std::string>> {
  public:
    SizeIndex() : Index([](const FileInfo& file) { 
        return std::make_pair(file.size_, file.path_); 
    }) {}
};


/**
 * @class ExtensionIndex
 * @brief Indexes files by their extensions.
 */
class ExtensionIndex : public Index<FileInfo, std::pair<std::string, std::string>> {
  public:
    ExtensionIndex() : Index([](const FileInfo& file) { 
        return std::make_pair(file.extension_, file.path_);
    }) {}
};


/**
 * @class TimeIndex
 * @brief Indexes files by their modification times.
 */
class TimeIndex : public Index<FileInfo, std::pair<std::time_t, std::string>> {
  public:
    TimeIndex() : Index([](const FileInfo& file) { 
        return std::make_pair(file.modified_time_, file.path_); 
    }) {}
};

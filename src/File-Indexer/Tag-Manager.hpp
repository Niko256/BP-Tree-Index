#pragma once

#include "../Index.hpp"
#include <map>

/**
 * @class TagManager
 * @brief Manages tags for files.
 *
 * This class allows users to associate tags with files, retrieve tags for a specific file,
 * and find files that match specific tags. 
 */
class TagManager {
  private:
    std::map<std::string, DynamicArray<std::string>> file_tags_; // Maps file paths to their associated tags.
    std::map<std::string, DynamicArray<std::string>> tags_to_files_; // Maps tags to the list of files that have them.

  public:

    /**
     * @brief Adds a tag to a file.
     *
     * Associates a tag with a specific file. If the tag already exists for the file, it is not duplicated.
     */
    void add_tag(const std::string& path, const std::string& tag) {
        auto& tags = file_tags_[path];
        bool tag_exists = false;
        for (size_t i = 0; i < tags.size(); ++i) {
            if (tags[i] == tag) {
                tag_exists = true;
                break;
            }
        }
        if (!tag_exists) {
            tags.push_back(tag);
        }

        auto& files = tags_to_files_[tag];
        bool file_exists = false;
        for (size_t i = 0; i < files.size(); ++i) {
            if (files[i] == path) {
                file_exists = true;
                break;
            }
        }
        if (!file_exists) {
            files.push_back(path);
        }
    }


    /**
     * @brief Retrieves the tags associated with a file.
     *
     * Returns a list of tags that have been assigned to the specified file.
     */
    DynamicArray<std::string> get_tags(const std::string& path) {
        auto it = file_tags_.find(path);
        if (it != file_tags_.end()) {
            return it->second;
        }
        return DynamicArray<std::string>();
    }


    /**
     * @brief Finds files associated with a specific tag.
     *
     * Returns a list of file paths that have been tagged with the specified tag.
     *
     */
    DynamicArray<std::string> find_by_tag(const std::string& tag) {
        // Find the tag in the `tags_to_files_` map.
        auto it = tags_to_files_.find(tag);
        
        // If the tag is found, return the list of files associated with it.
        if (it != tags_to_files_.end()) {
            return it->second;
        }
        
        // If the tag is not found, return an empty array.
        return DynamicArray<std::string>();
    }
};

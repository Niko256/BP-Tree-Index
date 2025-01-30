#include "Tag-Manager.hpp"


void TagManager::add_tag(const std::string& path, const std::string& tag) {
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

DynamicArray<std::string> TagManager::get_tags(const std::string& path) {
    auto it = file_tags_.find(path);
    if (it != file_tags_.end()) {
        return it->get_value();
    }
    return DynamicArray<std::string>();
}


DynamicArray<std::string> TagManager::find_by_tag(const std::string& tag) {
        // Find the tag in the `tags_to_files_` HashTable.
        auto it = tags_to_files_.find(tag);
        
        // If the tag is found, return the list of files associated with it.
        if (it != tags_to_files_.end()) {
            return it->get_value();
        }
        
        // If the tag is not found, return an empty array.
        return DynamicArray<std::string>();
}

#pragma once

#include "../Index.hpp"
#include "../../external/Data_Structures/Containers/HashTable/Hash_Table.hpp"

/**
 * @class TagManager
 * @brief Manages tags for files.
 *
 * This class allows users to associate tags with files, retrieve tags for a specific file,
 * and find files that match specific tags. 
 */
class TagManager {
  private:
    HashTable<std::string, DynamicArray<std::string>> file_tags_; // Maps file paths to their associated tags.
    HashTable<std::string, DynamicArray<std::string>> tags_to_files_; // Maps tags to the list of files that have them.

  public:

    /**
     * @brief Adds a tag to a file.
     *
     * Associates a tag with a specific file
     */
    void add_tag(const std::string& path, const std::string& tag); 


    /**
     * @brief Retrieves the tags associated with a file.
     *
     * Returns a list of tags that have been assigned to the specified file.
     */
    DynamicArray<std::string> get_tags(const std::string& path);

    /**
     * @brief Finds files associated with a specific tag.
     * Returns a list of file paths that have been tagged with the specified tag.
     */
    DynamicArray<std::string> find_by_tag(const std::string& tag);
};

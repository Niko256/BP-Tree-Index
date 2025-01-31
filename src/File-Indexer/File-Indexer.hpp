#pragma once

#include <fstream>
#include <filesystem>
#include <openssl/sha.h>
#include "File-Info.hpp"
#include "Indices.hpp"
#include "Tag-Manager.hpp"
#include "Search-Criteria.hpp"
#include "Search-Result.hpp"
#include <openssl/evp.h>
#include <set>
#include "../../external/Data_Structures/SmartPtrs/include/UniquePtr.hpp"
#include <atomic>
/**
 * @class FileIndexer
 * @brief A class responsible for indexing and managing files in a directory.
 *
 * This class provides functionality to index files, monitor file system changes,
 * search for files based on criteria, manage tags
 */
class FileIndexer {
  private:
    
    UniquePtr<NameIndex> name_index_; ///< Index for file names.
    UniquePtr<SizeIndex> size_index_; ///< Index for file sizes.
    UniquePtr<ExtensionIndex> extension_index_; ///< Index for file extensions.
    UniquePtr<TimeIndex> time_index_; ///< Index for file modification times.
    
    TagManager tag_manager_; ///< Manager for file tags.
    
    DynamicArray<FileInfo> files_; ///< Array of indexed file information.
    std::atomic<size_t> next_id_{0}; ///< Next unique ID for file indexing.
    

    /**
     * @brief Adds a file to the index.
     */
    void add_file(const std::string& path); 

    /**
     * @brief Updates a file in the index.
     */
    void update_file(const std::string& path); 
    
    /**
     * @brief Removes a file from the index.
     */
    void remove_file(const std::string& path); 

  public:

    /**
     * @brief Constructs a FileIndexer object.
     */
    FileIndexer(); 
    
    /**
     * @brief Indexes all files in a directory.
     */
    void index_directory(const std::string& path); 
    
    /**
     * @brief Searches for files matching the given criteria.
     */
    DynamicArray<SearchResult> search(const SearchCriteria& criteria); 
    
    /**
     * @brief Adds a tag to a file.
     */
    void add_tag(const std::string& path, const std::string& tag); 
   
    /**
     * @brief Gets the list of indexed files.
     */
    DynamicArray<FileInfo>& get_files(); 

    /**
     * @brief Finds files by tag.
     */
    DynamicArray<std::string> find_by_tag(const std::string& tag); 

    /**
     * @brief Gets file system statistics.
     */
    FileSystemStats get_statistics(); 

};


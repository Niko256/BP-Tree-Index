#include "File-Indexer.hpp"


void FileIndexer::add_file(const std::string& path) {
    FileInfo info;
    info.id_ = next_id_++;
    info.path_ = path;
    info.name_ = std::filesystem::path(path).filename().string();
    info.is_dir_ = std::filesystem::is_directory(path);
    
    if (!info.is_dir_) {
        info.size_ = std::filesystem::file_size(path);
        info.extension_ = std::filesystem::path(path).extension().string();
        info.modified_time_ = std::filesystem::last_write_time(path)
            .time_since_epoch().count();
    }
    
    files_.push_back(info);
    
    name_index_->insert(info);
    size_index_->insert(info);
    extension_index_->insert(info);
    time_index_->insert(info);
}



void FileIndexer::update_file(const std::string& path) {
    remove_file(path);
    add_file(path);
}



void FileIndexer::remove_file(const std::string& path) {
    for (size_t i = 0; i < files_.size(); ++i) {
        if (files_[i].path_ == path) {
            name_index_->remove(files_[i].path_);
            size_index_->remove(Pair<size_t, std::string>(files_[i].size_, files_[i].path_));
            extension_index_->remove(Pair<std::string, std::string>(files_[i].extension_, files_[i].path_));
            time_index_->remove(Pair<std::time_t, std::string>(files_[i].modified_time_, files_[i].path_));
            files_.erase(files_.begin() + i);
            break;
        }
    }
}



FileIndexer::FileIndexer() 
    : name_index_(make_unique<NameIndex>()),
      size_index_(make_unique<SizeIndex>()),
      extension_index_(make_unique<ExtensionIndex>()),
      time_index_(make_unique<TimeIndex>()) {}


void FileIndexer::index_directory(const std::string& path) {
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            if (!entry.is_directory()) {  
                FileInfo info;
                info.id_ = next_id_++;
                info.path_ = std::filesystem::canonical(entry.path()).string(); 
                info.name_ = entry.path().filename().string();
                info.is_dir_ = false;
                info.size_ = std::filesystem::file_size(entry.path());
                info.extension_ = entry.path().extension().string();
                info.modified_time_ = std::filesystem::last_write_time(entry.path()).time_since_epoch().count();
        
                std::cout << "Processing file: " << info.path_ << std::endl;
            
                try {
                    files_.push_back(info);
                    name_index_->insert(info);
                    size_index_->insert(info);
                    extension_index_->insert(info);
                    time_index_->insert(info);
                } catch (const std::exception& e) {
                    std::cerr << "Error indexing file " << info.path_ << ": " << e.what() << std::endl;
                    throw;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error during directory traversal: " << e.what() << std::endl;
        throw;
    }
}


void FileIndexer::add_tag(const std::string& path, const std::string& tag) {
    tag_manager_.add_tag(path, tag);
}

DynamicArray<FileInfo>& FileIndexer::get_files() {
    return files_;
}

DynamicArray<std::string> FileIndexer::find_by_tag(const std::string& tag) {
    return tag_manager_.find_by_tag(tag);
}


DynamicArray<SearchResult> FileIndexer::search(const SearchCriteria& criteria) {
    DynamicArray<SearchResult> results;
    std::cout << "Searching...\n";

    for (const auto& file : files_) {
        if (criteria.matches(file)) {
            results.push_back({file, "", 1.0});
        }
    }

    std::cout << "Found " << results.size() << " files matching criteria.\n";
    return results;
}


/**
 * @brief Collects and returns file system statistics
 * 
 * Gathers information about total files, directories,
 * file extensions, and size distribution
 */
FileSystemStats FileIndexer::get_statistics() {
    FileSystemStats stats{0, 0};
        
    for (auto& file : files_) {
        if (file.is_dir_) {
            stats.total_dirs++;  // Count directories
        } else {
            stats.total_files++;  // Count files
            stats.extensions_count[file.extension_]++;  // Count by extension
                
            // Categorize file by size
            size_t size_mb = file.size_ / (1024 * 1024);
            std::string size_category;
                
            if (size_mb < 1) size_category = "<1MB";
            else if (size_mb < 10) size_category = "1-10MB";
            else if (size_mb < 100) size_category = "10-100MB";
            else size_category = ">100MB";
                
            stats.size_distribution[size_category]++;  // Count by size category
        }
    }
        
    return stats;
}

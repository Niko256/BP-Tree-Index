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
    // Log search criteria
    std::cout << "Searching with criteria:\n";
    if (!criteria.get_terms().empty())
        std::cout << "- Name contains: '" << criteria.get_terms() << "'\n";
    if (!criteria.get_size_filter().empty())
        std::cout << "- Size: " << criteria.get_size_filter() << "\n";
    if (!criteria.get_date_filter().empty())
        std::cout << "- Date: " << criteria.get_date_filter() << "\n";

    std::set<std::string> matching_paths;
    bool first_filter = true;  // Flag for first applied filter

    // Search by name terms if provided
    if (!criteria.get_terms().empty()) {
        std::set<std::string> name_matches;
        // Find files matching name criteria
        auto files = name_index_->find_if([&](const FileInfo& file) {
            return file.name_.find(criteria.get_terms()) != std::string::npos; 
        });
        // Convert results to path set
        for (const auto& file : files) {
            name_matches.insert(file.path_);
        }
        
        // Handle first filter or combine with previous results
        if (first_filter) {
            matching_paths = std::move(name_matches);
            first_filter = false;
        } else {
            // Perform set intersection for AND 
            std::set<std::string> intersection;
            std::set_intersection(
                matching_paths.begin(), matching_paths.end(),
                name_matches.begin(), name_matches.end(),
                std::inserter(intersection, intersection.begin())
            );
            matching_paths = std::move(intersection);
        }
    }

    // Search by size if filter provided
    if (!criteria.get_size_filter().empty()) {
        std::set<std::string> size_matches;
        // Find files matching size criteria
        auto files = size_index_->find_if([&](const FileInfo& file) {
            return criteria.matches_size_filter(file.size_); 
        });
        // Convert results to path set
        for (const auto& file : files) {
            size_matches.insert(file.path_);
        }

        // Handle first filter or combine with previous results
        if (first_filter) {
            matching_paths = std::move(size_matches);
            first_filter = false;
        } else {
            // Perform set intersection for AND
            std::set<std::string> intersection;
            std::set_intersection(
                matching_paths.begin(), matching_paths.end(),
                size_matches.begin(), size_matches.end(),
                std::inserter(intersection, intersection.begin())
            );
            matching_paths = std::move(intersection);
        }
    }

    // Convert matching paths to SearchResult objects
    DynamicArray<SearchResult> results;
    for (const auto& path : matching_paths) {
        for (const auto& file : files_) {
            if (file.path_ == path) {
                results.push_back({file, "", 1.0});  // Add with empty context and full relevance
                break;
            }
        }
    }

    std::cout << "\nFound " << results.size() << " files matching all criteria.\n";
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

/**
 * @brief Calculates SHA-256 hash of a file
 * 
 * Reads file in chunks and calculates its cryptographic hash
 * Uses OpenSSL's EVP interface for hash calculation
 */
std::string FileIndexer::calculate_file_hash(const std::string& path) {

    // Open file in binary mode
    std::ifstream file(path, std::ios::binary);
    if (!file) return "";

    // Initialize OpenSSL hash context
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return "";

    // Initialize SHA-256 hashing
    if (!EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr)) {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    // Read and hash file in chunks
    char buffer[8192];
    while (file.read(buffer, sizeof(buffer))) {
        if (!EVP_DigestUpdate(ctx, buffer, file.gcount())) {
            EVP_MD_CTX_free(ctx);
            return "";
        }
    }

    // Finalize hash calculation
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    
    if (!EVP_DigestFinal_ex(ctx, hash, &hash_len)) {
        EVP_MD_CTX_free(ctx);
        return "";
    }

    EVP_MD_CTX_free(ctx);

    // Convert hash to hexadecimal string
    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') 
           << static_cast<int>(hash[i]);
    }
    return ss.str();
}


/**
 * @brief Finds duplicate files based on content hash
 * 
 * Groups files by their SHA-256 hash to identify duplicates
 */
DynamicArray<DuplicateGroup> FileIndexer::find_duplicates() {
    // Map to group files by hash
    HashTable<std::string, DynamicArray<std::string>> hash_groups;
        
    // Calculate hash for each file and group them
    for (auto& file : files_) {
        if (!file.is_dir_) {
            std::string hash = calculate_file_hash(file.path_);
            hash_groups[hash].push_back(file.path_);
        }
    }
        
    // Create result array with groups of duplicates
    DynamicArray<DuplicateGroup> result;
    for (const auto& [hash, paths] : hash_groups) {
        if (paths.size() > 1) {  // Only include groups with multiple files
            result.push_back({hash, paths});
        }
    }
    return result;
}

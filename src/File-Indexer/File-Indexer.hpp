#pragma once

#include <fstream>
#include <memory>
#include <filesystem>
#include <map>
#include <openssl/sha.h>
#include "File-Info.hpp"
#include "Indices.hpp"
#include "System-Watcher.hpp"
#include "Tag-Manager.hpp"
#include "Search-Criteria.hpp"
#include "Search-Result.hpp"
#include <openssl/evp.h>
#include <set>



class FileIndexer {
  private:

    std::unique_ptr<NameIndex> name_index_;
    std::unique_ptr<SizeIndex> size_index_;
    std::unique_ptr<ExtensionIndex> extension_index_;
    std::unique_ptr<TimeIndex> time_index_;
    
    TagManager tag_manager_;
    
    FileSystemWatcher fs_watcher_;
    
    DynamicArray<FileInfo> files_;
    std::atomic<size_t> next_id_{0};
    
    std::string calculate_file_hash(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return "";

        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if (!ctx) return "";

        if (!EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr)) {
            EVP_MD_CTX_free(ctx);
            return "";
        }

        char buffer[8192];
        while (file.read(buffer, sizeof(buffer))) {
            if (!EVP_DigestUpdate(ctx, buffer, file.gcount())) {
                EVP_MD_CTX_free(ctx);
                return "";
            }
        }

        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int hash_len;
    
        if (!EVP_DigestFinal_ex(ctx, hash, &hash_len)) {
            EVP_MD_CTX_free(ctx);
            return "";
        }

        EVP_MD_CTX_free(ctx);

        std::stringstream ss;
        for (unsigned int i = 0; i < hash_len; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        return ss.str();
    }
    
    void handle_fs_event(const std::string& path, FileSystemEvent event) {
        switch (event) {
            case FileSystemEvent::CREATED:
                add_file(path);
                break;
            case FileSystemEvent::MODIFIED:
                update_file(path);
                break;
            case FileSystemEvent::DELETED:
                remove_file(path);
                break;
        }
    }
    
    void add_file(const std::string& path) {
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
    
    void update_file(const std::string& path) {
        remove_file(path);
        add_file(path);
    }
    
    void remove_file(const std::string& path) {
        for (size_t i = 0; i < files_.size(); ++i) {
            if (files_[i].path_ == path) {
                name_index_->remove(files_[i].path_);
            
                size_index_->remove(std::make_pair(files_[i].size_, files_[i].path_));
            
                extension_index_->remove(std::make_pair(files_[i].extension_, files_[i].path_));
            
                time_index_->remove(std::make_pair(files_[i].modified_time_, files_[i].path_));
            
                files_.erase(files_.begin() + i);
                break;
            }
        }
    }

public:
    FileIndexer() 
        : name_index_(std::make_unique<NameIndex>()),
          size_index_(std::make_unique<SizeIndex>()),
          extension_index_(std::make_unique<ExtensionIndex>()),
          time_index_(std::make_unique<TimeIndex>()) {}
    
    void index_directory(const std::string& path) {
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
    
    DynamicArray<SearchResult> search(const SearchCriteria& criteria) {
        std::cout << "Searching with criteria:\n";
        if (!criteria.terms_.empty()) 
            std::cout << "- Name contains: '" << criteria.terms_ << "'\n";
        if (!criteria.size_filter.empty()) 
            std::cout << "- Size: " << criteria.size_filter << "\n";
        if (!criteria.date_filter.empty()) 
            std::cout << "- Date: " << criteria.date_filter << "\n";

    
        std::set<std::string> matching_paths;
        bool first_filter = true;

        if (!criteria.terms_.empty()) {
            std::set<std::string> name_matches;
            auto files = name_index_->find_if([&](const FileInfo& file) {
                return file.name_.find(criteria.terms_) != std::string::npos;
            });
            for (const auto& file : files) {
                name_matches.insert(file.path_);
            }
        
            if (first_filter) {
                matching_paths = std::move(name_matches);
                first_filter = false;
            } else {
                std::set<std::string> intersection;
                std::set_intersection(
                    matching_paths.begin(), matching_paths.end(),
                    name_matches.begin(), name_matches.end(),
                    std::inserter(intersection, intersection.begin())
                );
                matching_paths = std::move(intersection);
            }
        }

        if (!criteria.size_filter.empty()) {
            std::set<std::string> size_matches;
            auto files = size_index_->find_if([&](const FileInfo& file) {
                return criteria.matches_size_filter(file.size_);
            });
            for (const auto& file : files) {
                size_matches.insert(file.path_);
            }

            if (first_filter) {
                matching_paths = std::move(size_matches);
                first_filter = false;
            } else {
                std::set<std::string> intersection;
                std::set_intersection(
                    matching_paths.begin(), matching_paths.end(),
                    size_matches.begin(), size_matches.end(),
                    std::inserter(intersection, intersection.begin())
                );
                matching_paths = std::move(intersection);
            }
        }

        DynamicArray<SearchResult> results;
        for (const auto& path : matching_paths) {
            for (const auto& file : files_) {
                if (file.path_ == path) {
                    results.push_back({file, "", 1.0});
                    break;
                }
            }
        }

        std::cout << "\nFound " << results.size() << " files matching all criteria.\n";
        return results;
    }
    
    void add_tag(const std::string& path, const std::string& tag) {
        tag_manager_.add_tag(path, tag);
    }
   
    DynamicArray<FileInfo>& get_files() {
        return files_;
    }

    DynamicArray<std::string> find_by_tag(const std::string& tag) {
        return tag_manager_.find_by_tag(tag);
    }

    FileSystemStats get_statistics() {
        FileSystemStats stats{0, 0};
        
        for (auto& file : files_) {
            if (file.is_dir_) {
                stats.total_dirs++;
            } else {
                stats.total_files++;
                stats.extensions_count[file.extension_]++;
                
                size_t size_mb = file.size_ / (1024 * 1024);
                std::string size_category;
                
                if (size_mb < 1) size_category = "<1MB";
                else if (size_mb < 10) size_category = "1-10MB";
                else if (size_mb < 100) size_category = "10-100MB";
                else size_category = ">100MB";
                
                stats.size_distribution[size_category]++;
            }
        }
        
        return stats;
    }
    
    DynamicArray<DuplicateGroup> find_duplicates() {
        std::map<std::string, DynamicArray<std::string>> hash_groups;
        
        for (auto& file : files_) {
            if (!file.is_dir_) {
                std::string hash = calculate_file_hash(file.path_);
                hash_groups[hash].push_back(file.path_);
            }
        }
        
        DynamicArray<DuplicateGroup> result;
        for (const auto& [hash, paths] : hash_groups) {
            if (paths.size() > 1) {
                result.push_back({hash, paths});
            }
        }
        return result;
    }
    
    void start_monitoring(const std::string& path) {
        fs_watcher_.start([this](const std::string& p, FileSystemEvent e) {
            handle_fs_event(p, e);
        });
    }
    
    void stop_monitoring() {
        fs_watcher_.stop();
    }
};

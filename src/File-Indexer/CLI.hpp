#pragma once

#include "File-Indexer.hpp"
#include "Search-Criteria.hpp"
#include <string>
#include "Search-Result.hpp"
#include "Utils.hpp"


class CLI {
  private:

    FileIndexer indexer_;
    std::string current_dir_;

    void print_menu() {
        std::cout << "\nFile System Indexer\n"
            << "==================\n"
            << "1. Index Directory\n"
            << "2. Search files\n"
            << "3. Manage tags\n"
            << "4. Show statictics\n"
            << "5. Find dublicates\n"
            << "6. Monitor directory\n"
            << "7. Exit\n"
            << "Choice: ";
    }

    void handle_index_dir() {
        std::cout << "Enter directory path: ";
        std::string path;
        std::getline(std::cin, path);

        if (path.empty()) {
            std::cout << "Error: Directory path cannot be empty.\n";
            return;
        }

        try {
            if (path[0] == '~') {
                const char* home = std::getenv("HOME");
                if (home) {
                    path.replace(0, 1, home);
                }
            }

            std::filesystem::path fs_path = std::filesystem::absolute(path);
        
            if (!std::filesystem::exists(fs_path)) {
                std::cout << "Error: Directory does not exist: " << fs_path << "\n";
                return;
            }

            if (!std::filesystem::is_directory(fs_path)) {
                std::cout << "Error: Path is not a directory: " << fs_path << "\n";
                return;
            }

            std::cout << "Indexing directory: " << fs_path << "\n";
            indexer_.index_directory(fs_path.string());
            current_dir_ = fs_path.string();
            std::cout << "Indexing complete.\n";
        
        } catch (const std::filesystem::filesystem_error& e) {
            std::cout << "Error: Cannot access directory: " << e.what() << "\n";
        }
    }

    void handle_search() {
        SearchCriteria criteria;

        std::cout << "Enter search terms (empty to skip): ";
        std::string terms;
        
        std::getline(std::cin, terms);

        if (!terms.empty()) {
            criteria.add_terms(terms);
        }

        std::cout << "Enter size filter (e.g., >1M, <500K, empty to skip):";

        std::string size_filter;
        std::getline(std::cin, size_filter);

        if (!size_filter.empty()) {
            criteria.add_size_filter(size_filter);
        }


        std::cout << "Enter date filter (e.g., >2025-01-01, <2025-12-31, empty to skip): ";
        std::string date_filter;
        std::getline(std::cin, date_filter);
        if (!date_filter.empty()) {
            criteria.add_date_filter(date_filter);
        }

        std::cout << "Searching...\n";
        auto results = indexer_.search(criteria);
        display_results(results);
    }

    void handle_tags() {
        if (current_dir_.empty()) {
            std::cout << "Please index a directory first using option 1.\n";
            return;
        }

        std::cout << "Tag Management:\n"
                  << "1. Add tag\n"
                  << "2. Search by tag\n"
                << "Choice: ";
              
        std::string choice;
        std::getline(std::cin, choice);
    
        if (choice == "1") {
            std::cout << "Enter file path: ";
            std::string path;
            std::getline(std::cin, path);

            bool file_indexed = false;
            for (auto& file : indexer_.get_files()) {
                if (file.path_ == path) {
                    file_indexed = true;
                    break;
                }
            }

            if (!file_indexed) {
                std::cout << "Warning: This file is not in the index. Would you like to:\n"
                         << "1. Add this file's directory to index\n"
                         << "2. Cancel\n"
                         << "Choice: ";
                std::string index_choice;
                std::getline(std::cin, index_choice);

                if (index_choice == "1") {
                    std::filesystem::path file_path(path);
                    indexer_.index_directory(file_path.parent_path().string());
                } else {
                    std::cout << "Operation cancelled.\n";
                    return;
                }
            }
        
            std::cout << "Enter tag: ";
            std::string tag;
            std::getline(std::cin, tag);
        
            indexer_.add_tag(path, tag);
            std::cout << "Tag added.\n";
        }
        else if (choice == "2") {
            std::cout << "Enter tag to search: ";
            std::string tag;
            std::getline(std::cin, tag);
        
            auto matches = indexer_.find_by_tag(tag);
            if (matches.empty()) {
                std::cout << "No files found with tag '" << tag << "'\n";
            } else {
                std::cout << "\nFiles tagged with '" << tag << "':\n";
                for (const auto& path : matches) {
                    std::cout << path << "\n";
                }
            }
        }
    }

    void handle_statistics() {
        auto stats = indexer_.get_statistics();
        
        std::cout << "\nFile System Statistics\n"
                  << "=====================\n"
                  << "Total files: " << stats.total_files << "\n"
                  << "Total directories: " << stats.total_dirs << "\n\n"
                  << "Extension distribution:\n";
                  
        for (const auto& [ext, count] : stats.extensions_count) {
            std::cout << ext << ": " << count << "\n";
        }
        
        std::cout << "\nSize distribution:\n";
        for (const auto& [range, count] : stats.size_distribution) {
            std::cout << range << ": " << count << "\n";
        }
    }
    
    void handle_duplicates() {
        std::cout << "Searching for duplicates...\n";
        auto duplicates = indexer_.find_duplicates();
        
        size_t total_space = 0;
        for (auto& group : duplicates) {
            if (group.paths.size() > 1) {
                std::cout << "\nDuplicate files (hash: " << group.hash << "):\n";
                for (auto& path : group.paths) {
                    std::cout << "  " << path << "\n";
                }
            }
        }
    }

    void handle_monitor() {
        if (current_dir_.empty()) {
            std::cout << "Please index a directory first.\n";
            return;
        }
        
        std::cout << "Monitoring " << current_dir_ << "\n"
                  << "Press Enter to stop monitoring.\n";
                  
        indexer_.start_monitoring(current_dir_);
        std::cin.get();
        indexer_.stop_monitoring();
    }
    
    void display_results(DynamicArray<SearchResult>& results) {
        if (results.empty()) {
            std::cout << "No results found.\n";
            return;
        }
        
        std::cout << "\nFound " << results.size() << " results:\n"
                  << std::string(80, '-') << "\n";
                  
        for (auto& result : results) {
            std::cout << result.file.path_ << " ("
                     << (result.file.is_dir_ ? "DIR" : 
                         utils::format_size(result.file.size_))
                     << ")\n";
            if (!result.context.empty()) {
                std::cout << "Context: " << result.context << "\n";
            }
            std::cout << std::string(80, '-') << "\n";
        }
    }

  public:

    void run() {
        while (true) {
            print_menu();
            
            std::string choice;
            std::getline(std::cin, choice);
            
            if (choice == "1") handle_index_dir();
            else if (choice == "2") handle_search();
            else if (choice == "3") handle_tags();
            else if (choice == "4") handle_statistics();
            else if (choice == "5") handle_duplicates();
            else if (choice == "6") handle_monitor();
            else if (choice == "7") break;
            else std::cout << "Invalid choice. Try again.\n";
        }
    }
};

#include "CLI.hpp"

void CLI::print_menu() {
    std::cout << "\nFile Indexer Menu\n"
            << "================\n"
            << "1. Index Directory\n"
            << "2. Search\n"
            << "3. Tag Management\n"
            << "4. Statistics\n"
            << "5. Find Duplicates\n"
            << "6. Exit\n"
            << "Choice: ";
}

void CLI::handle_index_dir() {
    std::cout << "Enter directory path to index: ";
    std::string path;
    std::getline(std::cin, path);

    if (path.empty()) {
        std::cout << "Error: Directory path cannot be empty.\n";
        return;
    }

    std::cout << "Indexing directory: " << path << "\n";
    try {
        indexer_.index_directory(path);
        current_dir_ = path;
        std::cout << "Directory indexed successfully.\n";
    } catch (const std::exception& e) {
        std::cout << "Error indexing directory: " << e.what() << "\n";
    }
}

void CLI::handle_search() {
    // Create search criteria object to collect search parameters
    SearchCriteria criteria;

    // Get search terms from user
    std::cout << "Enter search terms (empty to skip): ";
    std::string terms;
    std::getline(std::cin, terms);

    // Add search terms if provided
    if (!terms.empty()) {
        criteria.add_terms(terms);
    }

    // Get size filter from user
    std::cout << "Enter size filter (e.g., >1M, <500K, empty to skip): ";
    std::string size_filter;
    std::getline(std::cin, size_filter);

    // Add size filter if provided
    if (!size_filter.empty()) {
        criteria.add_size_filter(size_filter);
    }

    // Get date filter from user
    std::cout << "Enter date filter (e.g., >2025-01-01, <2025-12-31, empty to skip): ";
    std::string date_filter;
    std::getline(std::cin, date_filter);
    
    // Add date filter if provided
    if (!date_filter.empty()) {
        criteria.add_date_filter(date_filter);
    }

    // Perform search and display results
    std::cout << "Searching...\n";
    auto results = indexer_.search(criteria);
    display_results(results);
}

void CLI::handle_tags() {
    // Verify that a directory has been indexed
    if (current_dir_.empty()) {
        std::cout << "Please index a directory first using option 1.\n";
        return;
    }

    // Display tag management menu
    std::cout << "Tag Management:\n"
              << "1. Add tag\n"
              << "2. Search by tag\n"
              << "Choice: ";
              
    std::string choice;
    std::getline(std::cin, choice);
    
    if (choice == "1") {
        // Handle adding a new tag
        std::cout << "Enter file path: ";
        std::string path;
        std::getline(std::cin, path);

        // Check if file is in index
        bool file_indexed = false;
        for (auto& file : indexer_.get_files()) {
            if (file.path_ == path) {
                file_indexed = true;
                break;
            }
        }

        // Handle unindexed file
        if (!file_indexed) {
            std::cout << "Warning: This file is not in the index. Would you like to:\n"
                     << "1. Add this file's directory to index\n"
                     << "2. Cancel\n"
                     << "Choice: ";
            std::string index_choice;
            std::getline(std::cin, index_choice);

            if (index_choice == "1") {
                // Index the parent directory of the file
                std::filesystem::path file_path(path);
                indexer_.index_directory(file_path.parent_path().string());
            } else {
                std::cout << "Operation cancelled.\n";
                return;
            }
        }
    
        // Get and add the tag
        std::cout << "Enter tag: ";
        std::string tag;
        std::getline(std::cin, tag);
    
        indexer_.add_tag(path, tag);
        std::cout << "Tag added.\n";
    }
    else if (choice == "2") {
        // Handle searching by tag
        std::cout << "Enter tag to search: ";
        std::string tag;
        std::getline(std::cin, tag);
    
        // Perform tag search and display results
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

void CLI::handle_statistics() {
    // Get statistics from indexer
    auto stats = indexer_.get_statistics();
    
    // Display general statistics
    std::cout << "\nFile System Statistics\n"
              << "=====================\n"
              << "Total files: " << stats.total_files << "\n"
              << "Total directories: " << stats.total_dirs << "\n\n";
    
    // Display extension distribution
    std::cout << "Extension distribution:\n";          
    for (const auto& [ext, count] : stats.extensions_count) {
        std::cout << ext << ": " << count << "\n";
    }
    
    // Display size distribution
    std::cout << "\nSize distribution:\n";
    for (const auto& [range, count] : stats.size_distribution) {
        std::cout << range << ": " << count << "\n";
    }
}

void CLI::handle_duplicates() {
    std::cout << "Searching for duplicates...\n";
    
    // Get duplicate groups from indexer
    auto duplicates = indexer_.find_duplicates();
    
    // Initialize total space counter
    size_t total_space = 0;
    
    // Display duplicate groups
    for (auto& group : duplicates) {
        if (group.paths.size() > 1) {
            std::cout << "\nDuplicate files (hash: " << group.hash << "):\n";
            for (auto& path : group.paths) {
                std::cout << "  " << path << "\n";
            }
        }
    }
}


void CLI::display_results(DynamicArray<SearchResult>& results) {
    // Check if there are any results
    if (results.empty()) {
        std::cout << "No results found.\n";
        return;
    }
    
    // Display header with result count
    std::cout << "\nFound " << results.size() << " results:\n"
              << std::string(80, '-') << "\n";
              
    // Display each result with formatting
    for (auto& result : results) {
        // Show file path and size/type
        std::cout << result.file.path_ << " ("
                 << (result.file.is_dir_ ? "DIR" : 
                     utils::format_size(result.file.size_))
                 << ")\n";
        
        // Show context if available
        if (!result.context.empty()) {
            std::cout << "Context: " << result.context << "\n";
        }
        std::cout << std::string(80, '-') << "\n";
    }
}

void CLI::run() {
    // Main program loop
    while (true) {
        // Display menu and get user choice
        print_menu();
        
        std::string choice;
        std::getline(std::cin, choice);
        
        // Handle user choice
        if (choice == "1") handle_index_dir();
        else if (choice == "2") handle_search();
        else if (choice == "3") handle_tags();
        else if (choice == "4") handle_statistics();
        else if (choice == "5") handle_duplicates();
        else if (choice == "6") break;  // Exit program
        else std::cout << "Invalid choice. Try again.\n";
    }
}

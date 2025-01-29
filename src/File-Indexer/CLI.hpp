// CLI.hpp
#pragma once

#include "File-Indexer.hpp"
#include "Search-Criteria.hpp"
#include <string>
#include "Search-Result.hpp"
#include "Utils.hpp"

/**
 * @class CLI
 * @brief Command Line Interface for the File System Indexer
 *
 * This class provides a user interface for interacting with the file indexing system.
 * It handles user input and displays results in a formatted manner.
 */
class CLI {
  private:
    FileIndexer indexer_;        // The file indexer instance
    std::string current_dir_;    // Currently indexed directory path

    /**
     * @brief Displays the main menu options
     */
    void print_menu();

    /**
     * @brief Handles the directory indexing operation
     */
    void handle_index_dir();

    /**
     * @brief Handles file search operations
     */
    void handle_search();

    /**
     * @brief Handles tag management operations
     */
    void handle_tags();

    /**
     * @brief Displays file system statistics
     */
    void handle_statistics();

    /**
     * @brief Handles duplicate file detection
     */
    void handle_duplicates();

    /**
     * @brief Displays search results in a formatted manner
     */
    void display_results(DynamicArray<SearchResult>& results);

  public:
    /**
     * @brief Starts the CLI interface
     */
    void run();
};

#include "CLI.tpp"

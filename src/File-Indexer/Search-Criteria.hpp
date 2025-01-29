#pragma once

#include <string>
#include <algorithm>
#include "../../external/Data_Structures/Containers/Dynamic_Array.hpp"

/**
 * @class SearchCriteria
 * @brief Represents search criteria for filtering files or other entities.
 *
 * This class provides a flexible way to specify search parameters for file system queries.
 * It supports multiple filter types including:
 * - Text-based search terms
 * - File size filters (e.g., ">1M", "<100K")
 * - Date-based filters
 * - Tag-based filters
 * - Result limit specification
 */
class SearchCriteria {
  private:
    
    std::string terms_;              // Search terms or keywords to match
    std::string size_filter_;        // Filter for file size (e.g., ">1M", "<100K")
    std::string date_filter_;        // Filter for modification date
    DynamicArray<std::string> tags_; // List of tags to filter by
    size_t max_results_;             // Maximum number of results to return

    /**
     * @brief Parses a size string into bytes
     * @param size_str String representing size ("10K", "100M", ...)
     * @return Size in bytes
     */
    size_t parse_size(const std::string& size_str) const;

  public:
    
    SearchCriteria();

    /**
     * @brief Adds search terms to the criteria
     * @param search_terms The search terms or keywords to match
     */
    SearchCriteria& add_terms(const std::string& search_terms);

    /**
     * @brief Adds a size filter to the criteria
     * @param filter Size filter string (">1M", "<100K", ...)
     */
    SearchCriteria& add_size_filter(const std::string& filter);

    /**
     * @brief Adds a date filter to the criteria
     */
    SearchCriteria& add_date_filter(const std::string& filter);

    /**
     * @brief Adds a tag to the criteria
     */
    SearchCriteria& add_tag(const std::string& tag);

    /**
     * @brief Sets the maximum number of results to return
     */
    SearchCriteria& set_max_results(size_t max);

    /**
     * @brief Checks if a file size matches the size filter
     */
    bool matches_size_filter(size_t file_size) const;

    // Getters
    const std::string& get_terms() const;
    const std::string& get_size_filter() const;
    const std::string& get_date_filter() const;
    const DynamicArray<std::string>& get_tags() const;
    size_t get_max_results() const;
};

#include "Search-Criteria.tpp"

#pragma once

#include <string>
#include <algorithm>
#include "../../external/Data_Structures/Containers/Dynamic_Array.hpp"

/**
 * @class SearchCriteria
 * @brief Represents search criteria for filtering files or other entities.
 *
 * This class allows users to define search parameters such as keywords, size constraints,
 * date ranges, tags, and the maximum number of results to return. It supports a fluent interface
 * for method chaining.
 */
class SearchCriteria {
  private:
    
    /**
     * @brief Parses a size string (e.g., "10K", "100M") into bytes.
     *
     */
    size_t parse_size(const std::string& size_str) const {
        if (size_str.empty()) return 0;
        
        try {
            // Try parsing as a bare number first
            if (std::all_of(size_str.begin(), size_str.end(), ::isdigit)) {
                return std::stoull(size_str);
            }
            
            // Find the last digit position
            size_t last_digit_pos = size_str.find_last_of("0123456789");
            if (last_digit_pos == std::string::npos) return 0;
            
            // Parse the numeric part
            size_t size = std::stoull(size_str.substr(0, last_digit_pos + 1));
            
            // Get the unit part
            std::string unit = size_str.substr(last_digit_pos + 1);
            std::transform(unit.begin(), unit.end(), unit.begin(), ::toupper);
            
            // Remove 'B' suffix if present
            if (!unit.empty() && unit.back() == 'B') {
                unit.pop_back();
            }
            
            // Convert based on unit
            if (unit == "K") return size * 1024;
            if (unit == "M") return size * 1024 * 1024;
            if (unit == "G") return size * 1024 * 1024 * 1024;
            
            return size; // No recognized unit, return as bytes
            
        } catch (const std::exception&) {
            return 0; // Return 0 for any parsing errors
        }
    }

  public:
    std::string terms_; // Search terms or keywords to match.
    std::string size_filter; // Filter for file size (e.g., ">1M", "<100K").
    std::string date_filter; // Filter for modification date (e.g., "last_week", "2023-01-01").
    DynamicArray<std::string> tags; // List of tags to filter by.
    size_t max_results = 100; // Maximum number of results to return (default: 100).

    /**
     * @brief Adds search terms to the criteria.
     *
     * @param search_terms The search terms or keywords to match.
     */
    SearchCriteria& add_terms(const std::string& search_terms) {
        terms_ = search_terms;
        return *this;
    }

    /**
     * @brief Adds a size filter to the criteria.
     *
     * @param filter The size filter (e.g., ">1M", "<100K").
     */
    SearchCriteria& add_size_filter(const std::string& filter) {
        size_filter = filter;
        return *this;
    }

    /**
     * @brief Adds a date filter to the criteria.
     *
     * @param filter The date filter (e.g., "last_week", "2023-01-01").
     */
    SearchCriteria& add_date_filter(const std::string& filter) {
        date_filter = filter;
        return *this;
    }

    SearchCriteria& add_tag(const std::string& tag) {
        tags.push_back(tag);
        return *this;
    }

    SearchCriteria& set_max_results(size_t max) {
        max_results = max;
        return *this;
    }

    /**
     * @brief Checks if a file size matches the size filter.
     *
     * @param file_size The size of the file to check.
     * @return True if the file size matches the filter, false otherwise.
     */
    bool matches_size_filter(size_t file_size) const {
        if (size_filter.empty()) return true; // No filter means all sizes match.
    
        char comparison = size_filter[0]; // Extract the comparison operator (e.g., '>', '<', '=').
        std::string size_str = size_filter.substr(1); // Extract the size value.
        size_t filter_size = parse_size(size_str); // Parse the size value into bytes.
    
        switch (comparison) {
            case '>': return file_size > filter_size;
            case '<': return file_size < filter_size;
            case '=': return file_size == filter_size;
            default: return true; // Invalid comparison operator, assume match.
        }
    }
};

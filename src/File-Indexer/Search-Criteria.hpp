#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include "File-Info.hpp"

/**
 * @class SearchCriteria
 * @brief Represents search criteria for filtering files or other entities.
 *
 * This class allows users to define search parameters such as size constraints,
 * date ranges, name patterns.
 */
class SearchCriteria {
  private:
    /**
     * @enum CompareOp
     * @brief Represents comparison operators for filters.
     */
    enum class CompareOp {
        None,    // No comparison operation.
        Greater, // Greater than comparison.
        Less,    // Less than comparison.
        Equal    // Equal to comparison.
    };

    /**
     * @struct SizeFilter
     * @brief Represents a filter for file size.
     */
    struct SizeFilter {
        CompareOp op = CompareOp::None; // Comparison operator (>, <, =).
        size_t value = 0; // Size value in bytes.
        bool enabled = false; // Whether the filter is active.
    };

    /**
     * @struct DateFilter
     * @brief Represents a filter for file modification date.
     */
    struct DateFilter {
        CompareOp op = CompareOp::None; // Comparison operator (>, <, =).
        std::time_t value = 0; // Date value as a timestamp.
        bool enabled = false; // Whether the filter is active.
    };

    SizeFilter size_filter_; // Filter for file size.
    DateFilter date_filter_; // Filter for file modification date.
    std::string name_pattern_; // Pattern for filtering file names.
    bool use_name_filter_ = false; 
    size_t max_results_ = 100; // Maximum number of results to return.

    /**
     * @brief Converts a size string into bytes.
     * @param size_str The size string to convert.
     * @return The size in bytes, or 0 if the input is invalid.
     */
    size_t convert_to_bytes(const std::string& size_str) const;

  public:

    /**
     * @brief Adds a size filter to the search criteria.
     * @param filter The size filter string (e.g., ">1M", "<500K").
     */
    bool add_size_filter(const std::string& filter);

    /**
     * @brief Adds a date filter to the search criteria.
     * @param filter The date filter string (e.g., ">2025-01-01", "<2025-12-12").
     */
    bool add_date_filter(const std::string& filter);

    /**
     * @brief Adds a name pattern filter to the search criteria.
     * @param pattern The pattern to match in file names.
     */
    void add_name_filter(const std::string& pattern);

    /**
     * @brief Checks if a file matches the search criteria.
     * @param file The file to check.
     * @return True if the file matches all active filters, false otherwise.
     */
    bool matches(const FileInfo& file) const;

    /**
     * @brief Sets the maximum number of results to return.
     */
    void set_max_results(size_t max);

    /**
     * @brief Gets the maximum number of results to return.
     */
    size_t get_max_results() const;
};

#include "Search-Criteria.tpp"

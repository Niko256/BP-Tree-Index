#include "Search-Criteria.hpp"
#include <algorithm>

size_t SearchCriteria::convert_to_bytes(const std::string& size_str) const {
    
    // Find the position where the numerical part ends
    size_t num_end = size_str.find_first_not_of("0123456789");
    if (num_end == 0) return 0; // No numeric part found.

    try {
        // Extract the numerical value from the string
        size_t value = std::stoull(size_str.substr(0, num_end));
        
        // If there's a unit, convert it to bytes
        if (num_end != std::string::npos) {
            std::string unit = size_str.substr(num_end);
            std::transform(unit.begin(), unit.end(), unit.begin(), ::toupper);
            
            // Remove 'B' suffix if present (e.g., "KB" -> "K")
            if (!unit.empty() && unit.back() == 'B') {
                unit.pop_back();
            }

            // Convert based on the unit.
            if (unit == "K") value *= 1024; // Kilobytes to bytes.
            else if (unit == "M") value *= 1024 * 1024; // Megabytes to bytes.
            else if (unit == "G") value *= 1024 * 1024 * 1024; // Gigabytes to bytes.
            else if (!unit.empty()) return 0; // Invalid unit.
        }
        return value;
    } catch (...) {
        return 0; // Return 0 for any parsing errors.
    }
}


bool SearchCriteria::add_size_filter(const std::string& filter) {

    // Reset the size filter.
    size_filter_.enabled = false;
    size_filter_.value = 0;
    size_filter_.op = CompareOp::None;

    // If the filter is empty, it is considered valid but inactive.
    if (filter.empty()) return true; 

    // A valid filter must have at least an operator and a number.
    if (filter.length() < 2) return false;

    // Parse the comparison operator.
    switch(filter[0]) {
        case '>': size_filter_.op = CompareOp::Greater; break;
        case '<': size_filter_.op = CompareOp::Less; break;
        case '=': size_filter_.op = CompareOp::Equal; break;
        default: return false; // Invalid operator.
    }

    // Extract the size part of the filter.
    std::string size_part = filter.substr(1);
    if (size_part.empty() || !std::isdigit(size_part[0])) {
        return false; // Invalid size part.
    }

    // Convert the size string to bytes.
    size_t value = convert_to_bytes(size_part);
    if (value == 0) {
        return false; // Invalid size value.
    }

    // Enable the filter and set its value.
    size_filter_.value = value;
    size_filter_.enabled = true;
    return true;
}



bool SearchCriteria::add_date_filter(const std::string& filter) {
    // A valid filter must have at least an operator and a date.
    if (filter.empty() || filter.length() < 2) return false;

    // Parse the comparison operator.
    switch(filter[0]) {
        case '>': date_filter_.op = CompareOp::Greater; break;
        case '<': date_filter_.op = CompareOp::Less; break;
        case '=': date_filter_.op = CompareOp::Equal; break;
        default: return false; // Invalid operator.
    }

    // Parse the date part of the filter.
    std::tm tm = {};
    std::istringstream ss(filter.substr(1));
    ss >> std::get_time(&tm, "%Y-%m-%d");
    
    if (ss.fail()) return false; // Invalid date format.

    // Enable the filter and set its value.
    date_filter_.value = std::mktime(&tm);
    date_filter_.enabled = true;
    return true;
}



void SearchCriteria::add_name_filter(const std::string& pattern) {
    if (!pattern.empty()) {
        name_pattern_ = pattern;
        use_name_filter_ = true;
    }
}



bool SearchCriteria::matches(const FileInfo& file) const {
    // Check size filter.
    if (size_filter_.enabled && size_filter_.value > 0) {
        bool matches_size = false;
        switch(size_filter_.op) {
            case CompareOp::Greater:
                matches_size = file.size_ > size_filter_.value;
                break;
            case CompareOp::Less:
                matches_size = file.size_ < size_filter_.value;
                break;
            case CompareOp::Equal:
                matches_size = file.size_ == size_filter_.value;
                break;
            default:
                matches_size = true;
        }
        if (!matches_size) return false;
    }

    // Check date filter.
    if (date_filter_.enabled) {
        bool matches_date = false;
        switch(date_filter_.op) {
            case CompareOp::Greater:
                matches_date = file.modified_time_ > date_filter_.value;
                break;
            case CompareOp::Less:
                matches_date = file.modified_time_ < date_filter_.value;
                break;
            case CompareOp::Equal:
                matches_date = file.modified_time_ == date_filter_.value;
                break;
            default:
                matches_date = true;
        }
        if (!matches_date) return false;
    }

    // Check name filter.
    if (use_name_filter_ && !name_pattern_.empty()) {
        if (file.name_.find(name_pattern_) == std::string::npos) {
            return false;
        }
    }

    return true;
}



void SearchCriteria::set_max_results(size_t max) {
    max_results_ = max;
}

size_t SearchCriteria::get_max_results() const {
    return max_results_;
}

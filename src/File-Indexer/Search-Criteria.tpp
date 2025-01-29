#include <algorithm>

// Constructor implementation
SearchCriteria::SearchCriteria() : terms_(),
    size_filter_(), date_filter_(), tags_(), max_results_(100) {}

// Parse size implementation
size_t SearchCriteria::parse_size(const std::string& size_str) const {

    // Return early if empty string
    if (size_str.empty()) return 0;
    
    try {
        // Check if the string contains only digits
        if (std::all_of(size_str.begin(), size_str.end(), ::isdigit)) {
            return std::stoull(size_str);
        }
        
        // Find the position of the last digit
        size_t last_digit_pos = size_str.find_last_of("0123456789");
        if (last_digit_pos == std::string::npos) return 0;
        
        // Extract and parse the numeric part
        size_t size = std::stoull(size_str.substr(0, last_digit_pos + 1));
        
        // Extract and normalize the unit part
        std::string unit = size_str.substr(last_digit_pos + 1);
        std::transform(unit.begin(), unit.end(), unit.begin(), ::toupper);
        
        // Remove 'B' suffix if present
        if (!unit.empty() && unit.back() == 'B') {
            unit.pop_back();
        }
        
        // Convert to bytes based on unit
        if (unit == "K") return size * 1024;
        if (unit == "M") return size * 1024 * 1024;
        if (unit == "G") return size * 1024 * 1024 * 1024;
        
        return size; // Return as bytes if no unit recognized
        
    } catch (const std::exception&) {
        return 0; // Return 0 for any parsing errors
    }
}

// Method implementations
SearchCriteria& SearchCriteria::add_terms(const std::string& search_terms) {
    terms_ = search_terms;
    return *this;
}

SearchCriteria& SearchCriteria::add_size_filter(const std::string& filter) {
    size_filter_ = filter;
    return *this;
}

SearchCriteria& SearchCriteria::add_date_filter(const std::string& filter) {
    date_filter_ = filter;
    return *this;
}

SearchCriteria& SearchCriteria::add_tag(const std::string& tag) {
    tags_.push_back(tag);
    return *this;
}

SearchCriteria& SearchCriteria::set_max_results(size_t max) {
    max_results_ = max;
    return *this;
}

bool SearchCriteria::matches_size_filter(size_t file_size) const {
    // If no filter specified, all sizes match
    if (size_filter_.empty()) return true;
    
    // Extract comparison operator and size value
    char comparison = size_filter_[0];
    std::string size_str = size_filter_.substr(1);
    size_t filter_size = parse_size(size_str);
    
    // Compare based on operator
    switch (comparison) {
        case '>': return file_size > filter_size;
        case '<': return file_size < filter_size;
        case '=': return file_size == filter_size;
        default: return true; // Invalid operator, assume match
    }
}

// Getter implementations
const std::string& SearchCriteria::get_terms() const {
    return terms_;
}

const std::string& SearchCriteria::get_size_filter() const {
    return size_filter_;
}

const std::string& SearchCriteria::get_date_filter() const {
    return date_filter_;
}

const DynamicArray<std::string>& SearchCriteria::get_tags() const {
    return tags_;
}

size_t SearchCriteria::get_max_results() const {
    return max_results_;
}

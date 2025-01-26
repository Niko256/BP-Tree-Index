#pragma once
#include "File-Info.hpp"
#include <map>
#include "../../external/Data_Structures/Containers/Dynamic_Array.hpp"

struct SearchResult {
    FileInfo file;
    std::string context;
    double relevance;
};

struct FileSystemStats {
    size_t total_files;
    size_t total_dirs;

    std::map<std::string, size_t> extensions_count;
    std::map<std::string, size_t> size_distribution;
    std::map<std::string, size_t> age_distribution;
};

struct DuplicateGroup {
    std::string hash;
    DynamicArray<std::string> paths;
};

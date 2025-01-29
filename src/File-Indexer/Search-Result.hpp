#pragma once
#include "File-Info.hpp"
#include "../../external/Data_Structures/Containers/Dynamic_Array.hpp"
#include "../../external/Data_Structures/Containers/HashTable/Hash_Table.hpp"


struct SearchResult {
    FileInfo file;
    std::string context;
    double relevance;
};

struct FileSystemStats {
    size_t total_files;
    size_t total_dirs;

    HashTable<std::string, size_t> extensions_count;
    HashTable<std::string, size_t> size_distribution;
    HashTable<std::string, size_t> age_distribution;
};

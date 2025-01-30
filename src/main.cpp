#include "File-Indexer/CLI.hpp"
#include <iostream>

int main() {
    try {
        
        CLI cli;
        cli.run();
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

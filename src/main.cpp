#include "File-Indexer/CLI.hpp"
#include <iostream>
#include <openssl/evp.h>

int main() {
    try {
        OpenSSL_add_all_digests();
        
        CLI cli;
        cli.run();
        
        EVP_cleanup();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

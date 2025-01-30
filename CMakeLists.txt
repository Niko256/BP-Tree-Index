cmake_minimum_required(VERSION 3.30)

project(B-Plus-Tree-Index)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

find_package(GTest REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

file(GLOB_RECURSE IMPL src/*.tpp)
add_library(BPlusTreeLib STATIC
    ${IMPL}    
)

set_target_properties(BPlusTreeLib PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(BPlusTreeLib PUBLIC ${CMAKE_SOURCE_DIR}/src)

file(GLOB INDEXER_SOURCES
    src/File-Indexer/*.hpp
    src/File-Indexer/*.cpp
)

add_library(FileIndexerLib STATIC ${INDEXER_SOURCES})
target_include_directories(FileIndexerLib PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external
)
set_target_properties(FileIndexerLib PROPERTIES LINKER_LANGUAGE CXX)

add_executable(Index src/main.cpp)
target_link_libraries(Index PRIVATE 
    BPlusTreeLib
    FileIndexerLib
    OpenSSL::Crypto
    Threads::Threads
)

if(GTEST_FOUND)
    enable_testing()

    file(GLOB_RECURSE TEST_FILES tests/*.cpp)

    add_executable(tests ${TEST_FILES})
    target_link_libraries(tests
        PRIVATE
        BPlusTreeLib
        FileIndexerLib
        OpenSSL::Crypto
        Threads::Threads
        GTest::GTest
        GTest::Main
    )

    target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

    gtest_discover_tests(tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
        PROPERTIES
            LABELS "unit"
            TIMEOUT 10  
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(tests PRIVATE --coverage)
        target_link_options(tests PRIVATE --coverage)
    endif()
    
    add_custom_target(check 
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
        DEPENDS tests
    )
endif()

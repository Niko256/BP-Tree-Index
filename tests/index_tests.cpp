#include <cmath>
#include <gtest/gtest.h>
#include "../src/Index.hpp"
#include <string>

using TestRecord = Record<std::string, int, double>; // Name, age, height

class IndexTest : public ::testing::Test {
protected:
    Index<TestRecord, int> age_index;
    
    IndexTest() : age_index([](const TestRecord& r) { return r.get<1>(); }) {}
    
    void SetUp() override {
        age_index.insert(TestRecord(0, "Victor", 25, 1.75));
        age_index.insert(TestRecord(1, "Vladimir", 30, 1.80));
        age_index.insert(TestRecord(2, "Charlie", 35, 1.70));
    }
};

TEST_F(IndexTest, Insert) {
    EXPECT_EQ(age_index.size(), 3);
    age_index.insert(TestRecord(3, "David", 40, 1.85));
    EXPECT_EQ(age_index.size(), 4);
}

TEST_F(IndexTest, Find) {
    auto results = age_index.find(25);
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].get<0>(), "Victor");
    EXPECT_EQ(results[0].get<1>(), 25);
}

TEST_F(IndexTest, RangeSearch) {
    auto results = age_index.range_search(25, 35);
    EXPECT_EQ(results.size(), 3);
    EXPECT_EQ(results[0].get<1>(), 25);
    EXPECT_EQ(results[1].get<1>(), 30);
    EXPECT_EQ(results[2].get<1>(), 35);
}

TEST_F(IndexTest, PredicateSearch) {
    auto results = age_index.find_if([](const TestRecord& r) {
        return r.get<1>() > 27;
    });
    EXPECT_EQ(results.size(), 2);
    for (const auto& record : results) {
        EXPECT_GT(record.get<1>(), 27);
    }
}

class CompositeIndexTest : public ::testing::Test {
protected:
    CompositeIndex<TestRecord, std::string, int> name_age_index;
    
    CompositeIndexTest() 
        : name_age_index(
            [](const TestRecord& r) { return r.get<0>(); },
            [](const TestRecord& r) { return r.get<1>(); }
          ) {}
    
    void SetUp() override {
        name_age_index.insert(TestRecord(0, "Victor", 25, 1.75));
        name_age_index.insert(TestRecord(1, "Vladimir", 30, 1.80));
        name_age_index.insert(TestRecord(2, "Charlie", 35, 1.70));
    }
};

TEST_F(CompositeIndexTest, FindByKey) {
    std::string name = "Victor";
    auto results = name_age_index.find(CompositeKey<std::string, int>(name, 25));
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].get<0>(), "Victor");
    EXPECT_EQ(results[0].get<1>(), 25);
}

TEST_F(CompositeIndexTest, FindByComponent) {
    std::string name = "Victor";
    auto results = name_age_index.find_by_component<0>(name);
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].get<0>(), "Victor");
    
    results = name_age_index.find_by_component<1>(30);
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].get<1>(), 30);
    EXPECT_EQ(results[0].get<0>(), "Vladimir");
}

TEST_F(IndexTest, EmptyIndex) {
    Index<TestRecord, int> empty_index([](const TestRecord& r) { return r.get<1>(); });
    EXPECT_EQ(empty_index.size(), 0);
    EXPECT_TRUE(empty_index.find(25).empty());
}

TEST_F(IndexTest, Update) {
    TestRecord old_record(0, "Victor", 25, 1.75);
    TestRecord new_record(0, "Victor", 26, 1.75);
    age_index.update(old_record, new_record);
    
    EXPECT_TRUE(age_index.find(25).empty());
    auto results = age_index.find(26);
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].get<1>(), 26);
}

TEST_F(IndexTest, RemoveAndReinsert) {
    age_index.remove(25);
    EXPECT_TRUE(age_index.find(25).empty());
    
    age_index.insert(TestRecord(0, "Victor", 25, 1.75));
    auto results = age_index.find(25);
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].get<0>(), "Victor");
}


class HeightIndexTest : public ::testing::Test {
  protected:
    Index<TestRecord, double> height_index;
    
    HeightIndexTest() : height_index([](const TestRecord& r) { return r.get<2>(); }) {}
    
    void SetUp() override {
        height_index.insert(TestRecord(0, "Victor", 25, 1.75));
        height_index.insert(TestRecord(1, "Vladimir", 30, 1.80));
        height_index.insert(TestRecord(2, "Charlie", 35, 1.70));
    }
};

TEST_F(HeightIndexTest, FindExact) {
    auto results = height_index.find(1.75);
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].get<2>(), 1.75);
    EXPECT_EQ(results[0].get<0>(), "Victor");
}

TEST_F(HeightIndexTest, RangeSearchHeight) {
    auto results = height_index.range_search(1.70, 1.80);
    EXPECT_EQ(results.size(), 3);
    EXPECT_EQ(results[0].get<2>(), 1.70);
    EXPECT_EQ(results[2].get<2>(), 1.80);
}

class CompositeIndexExtendedTest : public ::testing::Test {
  protected:
    CompositeIndex<TestRecord, std::string, double> name_height_index;
    
    CompositeIndexExtendedTest() 
        : name_height_index(
            [](const TestRecord& r) { return r.get<0>(); },
            [](const TestRecord& r) { return r.get<2>(); }
          ) {}
    
    void SetUp() override {
        name_height_index.insert(TestRecord(0, "Victor", 25, 1.75));
        name_height_index.insert(TestRecord(1, "Vladimir", 30, 1.80));
        name_height_index.insert(TestRecord(2, "Charlie", 35, 1.70));
    }
};

TEST_F(CompositeIndexExtendedTest, FindByCompositeKey) {
    auto results = name_height_index.find(CompositeKey<std::string, double>("Victor", 1.75));
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].get<0>(), "Victor");
    EXPECT_EQ(results[0].get<2>(), 1.75);
}

TEST_F(CompositeIndexExtendedTest, FindByHeight) {
    auto results = name_height_index.find_by_component<1>(1.80);
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].get<2>(), 1.80);
    EXPECT_EQ(results[0].get<0>(), "Vladimir");
}

TEST_F(IndexTest, OutOfRangeSearch) {
    auto results = age_index.range_search(50, 60);
    EXPECT_TRUE(results.empty());
}

TEST_F(IndexTest, SingleElementRange) {
    auto results = age_index.range_search(25, 25);
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].get<1>(), 25);
}

TEST_F(IndexTest, ComplexPredicate) {
    auto results = age_index.find_if([](const TestRecord& r) {
        return r.get<1>() > 27 && r.get<2>() >= 1.75;
    });
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].get<0>(), "Vladimir");
}

TEST_F(CompositeIndexTest, EmptyIndexOperations) {
    CompositeIndex<TestRecord, std::string, int> empty_index(
        [](const TestRecord& r) { return r.get<0>(); },
        [](const TestRecord& r) { return r.get<1>(); }
    );
    
    auto results = empty_index.find(CompositeKey<std::string, int>("Victor", 25));
    EXPECT_TRUE(results.empty());
    
    results = empty_index.find_by_component<0>("Victor");
    EXPECT_TRUE(results.empty());
}

TEST_F(IndexTest, MultiplePredicates) {
    auto young = age_index.find_if([](const TestRecord& r) { return r.get<1>() < 30; });
    auto tall = age_index.find_if([](const TestRecord& r) { return r.get<2>() > 1.77; });
    
    EXPECT_EQ(young.size(), 1);
    EXPECT_EQ(tall.size(), 1);
    EXPECT_EQ(young[0].get<0>(), "Victor");
    EXPECT_EQ(tall[0].get<0>(), "Vladimir");
}

TEST_F(IndexTest, ResultOrder) {
    auto results = age_index.range_search(25, 35);
    EXPECT_EQ(results.size(), 3);
    for (size_t i = 1; i < results.size(); ++i) {
        EXPECT_GT(results[i].get<1>(), results[i-1].get<1>());
    }
}



class PerformanceTest : public ::testing::Test {
  protected:
    static constexpr size_t DATA_SIZE = 1000000;
    
    Index<TestRecord, int> indexed_storage;
    std::vector<TestRecord> vector_storage;
    
    PerformanceTest() : indexed_storage([](const TestRecord& r) { return r.get<1>(); }) {}
    
    void SetUp() override {
        for(size_t i = 0; i < DATA_SIZE; i++) {
            auto record = TestRecord(
                i,
                "Name" + std::to_string(i),
                i,  // age from 0 to 99
                1.5 + (static_cast<double>(i % 50) / 100.0)  // height from 1.5 to 2.0
            );
            
            indexed_storage.insert(record);
            vector_storage.push_back(record);
        }
    }

    template<typename Func>
    double measure_time(Func func) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();

        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }
};


TEST_F(PerformanceTest, SingleSearchComparison) {
    int search_age = 50;
    
    double index_time = measure_time([&]() {
        auto results = indexed_storage.find(search_age);
    });
    
    // linear search in vector
    double vector_time = measure_time([&]() {
        std::vector<TestRecord> results;
        for(const auto& record : vector_storage) {
            if(record.get<1>() == search_age) {
                results.push_back(record);
            }
        }
    });
    
    std::cout << "Single search time (microseconds):\n";
    std::cout << "B+Tree Index: " << index_time << "\n";
    std::cout << "Vector linear: " << vector_time << "\n";
    
    EXPECT_LT(index_time, vector_time);
}

TEST_F(PerformanceTest, RangeSearchComparison) {
    int min_age = 30;
    int max_age = 60;
    
    double index_time = measure_time([&]() {
        auto results = indexed_storage.range_search(min_age, max_age);
    });
    
    double vector_time = measure_time([&]() {
        std::vector<TestRecord> results;
        for(const auto& record : vector_storage) {
            if(record.get<1>() >= min_age && record.get<1>() <= max_age) {
                results.push_back(record);
            }
        }
    });
    
    std::cout << "Range search time (microseconds):\n";
    std::cout << "B+Tree Index: " << index_time << "\n";
    std::cout << "Vector linear: " << vector_time << "\n";
    
    EXPECT_LT(index_time, vector_time);
}

TEST_F(PerformanceTest, InsertionPerformance) {
    Index<TestRecord, int> new_index([](const TestRecord& r) { return r.get<1>(); });
    std::vector<TestRecord> new_vector;
    
    size_t test_size = 10000;
    size_t start_id = DATA_SIZE;
    
    double index_time = measure_time([&]() {
        for(size_t i = 0; i < test_size; i++) {
            auto record = TestRecord(
                start_id + i,
                "Name" + std::to_string(i),
                start_id + i,  
                1.5 + (static_cast<double>(i % 50) / 100.0)
            );
            new_index.insert(record);
        }
    });
    
    double vector_time = measure_time([&]() {
        for(size_t i = 0; i < test_size; i++) {
            auto record = TestRecord(
                start_id + i,
                "Name" + std::to_string(i),
                start_id + i, 
                1.5 + (static_cast<double>(i % 50) / 100.0)
            );
            new_vector.push_back(record);
        }
    });
    
    std::cout << "Insertion time for " << test_size << " records (microseconds):\n";
    std::cout << "B+Tree Index: " << index_time << "\n";
    std::cout << "Vector: " << vector_time << "\n";
    
    EXPECT_GT(index_time, vector_time);
}


TEST_F(PerformanceTest, SortedSearchComparison) {
    std::sort(vector_storage.begin(), vector_storage.end(),
              [](const TestRecord& a, const TestRecord& b) {
                  return a.get<1>() < b.get<1>();
              });
    
    int search_age = 50;
    
    double index_time = measure_time([&]() {
        auto results = indexed_storage.find(search_age);
    });
    
    double binary_search_time = measure_time([&]() {
        auto it = std::lower_bound(vector_storage.begin(), vector_storage.end(),
            search_age,
            [](const TestRecord& record, int age) {
                return record.get<1>() < age;
            });
    });
    
    std::cout << "Search time in sorted data (microseconds):\n";
    std::cout << "B+Tree Index: " << index_time << "\n";
    std::cout << "Binary search: " << binary_search_time << "\n";
}


TEST_F(PerformanceTest, OperationsScalability) {
    std::vector<size_t> sizes = {1000, 10000, 100000};
    
    for(size_t size : sizes) {
        Index<TestRecord, int> index([](const TestRecord& r) { return r.get<1>(); });
        
        double insert_time = measure_time([&]() {
            for(size_t i = 0; i < size; i++) {
                index.insert(TestRecord(i, "Name", i, 1.75));
            }
        }) / size;
        
        const int NUM_SEARCHES = 1000;
        double search_time = measure_time([&]() {
            for(int i = 0; i < NUM_SEARCHES; i++) {
                int search_key = rand() % size;
                auto results = index.find(search_key);
            }
        }) / NUM_SEARCHES;
        
        double delete_time = measure_time([&]() {
            for(size_t i = 0; i < size; i++) {
                index.remove(i);
            }
        }) / size;
        
        std::cout << "Size: " << size << "\n"
                  << "Average insert time: " << insert_time << " microseconds\n"
                  << "Average search time: " << search_time << " microseconds\n"
                  << "Average delete time: " << delete_time << " microseconds\n\n";
                  
        EXPECT_LT(insert_time, std::log2(size) * 10);
        EXPECT_LT(search_time, std::log2(size) * 10);
        EXPECT_LT(delete_time, std::log2(size) * 10);
    }
}

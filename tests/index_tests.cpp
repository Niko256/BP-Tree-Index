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

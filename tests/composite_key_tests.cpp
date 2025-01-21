#include "../src/BP-Tree.hpp"
#include "gtest/gtest.h"


class CompositeKeyTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};


TEST_F(CompositeKeyTest, BasicOperations) {
    CompositeKey<int, std::string> key1(1, "hello");
    CompositeKey<int, std::string> key2(1, "world");
    CompositeKey<int, std::string> key3(2, "hello");

    EXPECT_EQ(key1.get<0>(), 1);
    EXPECT_EQ(key1.get<1>(), "hello");

    EXPECT_TRUE(key1 < key3);  
    EXPECT_TRUE(key1 < key2);

    CompositeKey<int, std::string> key1_copy(1, "hello");
    EXPECT_TRUE(key1 == key1_copy);
    EXPECT_FALSE(key1 == key2);
}


TEST_F(CompositeKeyTest, MultipleComponents) {
    CompositeKey<int, std::string, double> key1(1, "test", 3.14);
    CompositeKey<int, std::string, double> key2(1, "test", 2.71);

    EXPECT_EQ(key1.get<0>(), 1);
    EXPECT_EQ(key1.get<1>(), "test");
    EXPECT_DOUBLE_EQ(key1.get<2>(), 3.14);

    EXPECT_FALSE(key1 < key2);
    EXPECT_TRUE(key2 < key1);
}


TEST_F(CompositeKeyTest, CompositeKeyOperations) {
    using CompositeKeyType = CompositeKey<int, std::string>;
    BPlusTree<CompositeKeyType, std::string> composite_tree;

    composite_tree.insert(CompositeKeyType(1, "a"), "value1");
    composite_tree.insert(CompositeKeyType(1, "b"), "value2");
    composite_tree.insert(CompositeKeyType(2, "a"), "value3");

    auto result1 = composite_tree.find(CompositeKeyType(1, "a"));
    ASSERT_EQ(result1.size(), 1);
    EXPECT_EQ(result1[0], "value1");

    auto range_result = composite_tree.range_search(
        CompositeKeyType(1, "a"),
        CompositeKeyType(2, "a")
    );
    ASSERT_EQ(range_result.size(), 3);
}


TEST_F(CompositeKeyTest, Comparison) {
    std::vector<CompositeKey<int, std::string>> keys = {
        CompositeKey<int, std::string>(2, "b"),
        CompositeKey<int, std::string>(1, "c"),
        CompositeKey<int, std::string>(1, "a"),
        CompositeKey<int, std::string>(2, "a")
    };

    std::sort(keys.begin(), keys.end());

    EXPECT_EQ(keys[0].get<0>(), 1);
    EXPECT_EQ(keys[0].get<1>(), "a");
    EXPECT_EQ(keys[1].get<0>(), 1);
    EXPECT_EQ(keys[1].get<1>(), "c");
    EXPECT_EQ(keys[2].get<0>(), 2);
    EXPECT_EQ(keys[2].get<1>(), "a");
    EXPECT_EQ(keys[3].get<0>(), 2);
    EXPECT_EQ(keys[3].get<1>(), "b");
}


TEST_F(CompositeKeyTest, EdgeCases) {
    CompositeKey<int, std::string> key1(0, "");
    CompositeKey<int, std::string> key2(0, "");
    EXPECT_TRUE(key1 == key2);

    CompositeKey<int, int> key3(std::numeric_limits<int>::max(), 
                               std::numeric_limits<int>::min());
    CompositeKey<int, int> key4(std::numeric_limits<int>::max(), 
                               std::numeric_limits<int>::min());
    EXPECT_TRUE(key3 == key4);
}



TEST_F(CompositeKeyTest, InsertAndFind) {
    using KeyType = CompositeKey<int, std::string>;
    BPlusTree<KeyType, std::string> tree;

    tree.insert(KeyType(1, "first"), "value1");
    auto result1 = tree.find(KeyType(1, "first"));
    ASSERT_EQ(result1.size(), 1);
    EXPECT_EQ(result1[0], "value1");

    tree.insert(KeyType(1, "second"), "value2");
    tree.insert(KeyType(2, "first"), "value3");

    auto result2 = tree.find(KeyType(1, "second"));
    ASSERT_EQ(result2.size(), 1);
    EXPECT_EQ(result2[0], "value2");

    auto result3 = tree.find(KeyType(2, "first"));
    ASSERT_EQ(result3.size(), 1);
    EXPECT_EQ(result3[0], "value3");

    auto result4 = tree.find(KeyType(3, "nonexistent"));
    EXPECT_TRUE(result4.empty());
}


TEST_F(CompositeKeyTest, UpdateExistingKey) {
    using KeyType = CompositeKey<int, std::string>;
    BPlusTree<KeyType, std::string> tree;

    tree.insert(KeyType(1, "key"), "initial_value");
    
    EXPECT_THROW({
        tree.insert(KeyType(1, "key"), "updated_value");
    }, std::runtime_error);

    auto result = tree.find(KeyType(1, "key"));
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "initial_value");
}


TEST_F(CompositeKeyTest, RangeQueries) {
    using KeyType = CompositeKey<int, std::string>;
    BPlusTree<KeyType, std::string> tree;

    std::vector<KeyType> keys = {
        KeyType(1, "a"), KeyType(1, "b"), KeyType(1, "c"),
        KeyType(2, "a"), KeyType(2, "b"), KeyType(2, "c"),
        KeyType(3, "a"), KeyType(3, "b"), KeyType(3, "c")
    };

    for (size_t i = 0; i < keys.size(); ++i) {
        tree.insert(keys[i], "value" + std::to_string(i));
    }

    auto range1 = tree.range_search(KeyType(1, "a"), KeyType(1, "z"));
    EXPECT_EQ(range1.size(), 3);

    auto range2 = tree.range_search(KeyType(1, "b"), KeyType(2, "b"));
    EXPECT_EQ(range2.size(), 4);

    auto range3 = tree.range_search(KeyType(4, "a"), KeyType(5, "a"));
    EXPECT_TRUE(range3.empty());
}


TEST_F(CompositeKeyTest, DeletionOperations) {
    using KeyType = CompositeKey<int, std::string>;
    BPlusTree<KeyType, std::string> tree;

    for (int i = 1; i <= 3; ++i) {
        for (char c = 'a'; c <= 'c'; ++c) {
            tree.insert(KeyType(i, std::string(1, c)), 
                       "value" + std::to_string(i) + c);
        }
    }

    tree.remove(KeyType(1, "a"));
    auto result = tree.find(KeyType(1, "a"));
    EXPECT_TRUE(result.empty());

    result = tree.find(KeyType(1, "b"));
    ASSERT_FALSE(result.empty());
    EXPECT_EQ(result[0], "value1b");
}


TEST_F(CompositeKeyTest, ComplexCompositeKeys) {
    using ComplexKey = CompositeKey<int, std::string, double, char>;
    BPlusTree<ComplexKey, std::string> tree;

    tree.insert(ComplexKey(1, "test", 3.14, 'a'), "value1");
    tree.insert(ComplexKey(1, "test", 3.14, 'b'), "value2");
    tree.insert(ComplexKey(1, "test", 3.15, 'a'), "value3");

    auto result = tree.find(ComplexKey(1, "test", 3.14, 'a'));
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "value1");

    auto range = tree.range_search(
        ComplexKey(1, "test", 3.14, 'a'),
        ComplexKey(1, "test", 3.15, 'a')
    );
    EXPECT_EQ(range.size(), 3);
}


TEST_F(CompositeKeyTest, IteratorOperations) {
    using KeyType = CompositeKey<int, std::string>;
    BPlusTree<KeyType, std::string> tree;

    std::vector<std::pair<KeyType, std::string>> data = {
        {KeyType(1, "a"), "value1"},
        {KeyType(1, "b"), "value2"},
        {KeyType(2, "a"), "value3"},
        {KeyType(2, "b"), "value4"}
    };

    for (const auto& [key, value] : data) {
        tree.insert(key, value);
    }

    size_t count = 0;
    for (auto it = tree.begin(); it != tree.end(); ++it) {
        count++;
    }
    EXPECT_EQ(count, data.size());
}

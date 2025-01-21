#include "gtest/gtest.h"
#include <cmath>
#include <memory>
#include "../src/BP-Tree.hpp"
#include "string"
#include <thread>



class BPlusTreeTest : public ::testing::Test {
  protected:

    static const size_t TEST_ORDER = 128;
    std::unique_ptr<BPlusTree<int, std::string, TEST_ORDER>> tree_;

    void SetUp() override {
        tree_ = std::make_unique<BPlusTree<int, std::string>>();
    }

    void TearDown() override {
        tree_.reset();
    }
};


TEST_F(BPlusTreeTest, InsertSingleElement) {
    std::string value = "value1";
    tree_->insert(10, value);

    auto result = tree_->find(10);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "value1");
}

TEST_F(BPlusTreeTest, InsertMultipleElements) {
    std::string value1 = "value1";
    std::string value2 = "value2";
    tree_->insert(10, value1);
    tree_->insert(20, value2);

    auto result1 = tree_->find(10);
    auto result2 = tree_->find(20);
    ASSERT_EQ(result1.size(), 1);
    ASSERT_EQ(result2.size(), 1);
    EXPECT_EQ(result1[0], "value1");
    EXPECT_EQ(result2[0], "value2");
}

TEST_F(BPlusTreeTest, InsertDuplicateKeyThrowsException) {
    std::string value1 = "value1";
    std::string value2 = "value2";
    tree_->insert(10, value1);

    EXPECT_THROW(tree_->insert(10, value2), std::runtime_error);
}

TEST_F(BPlusTreeTest, InsertInAscendingOrder) {
    const int COUNT = 100;
    for (int i = 0; i < COUNT; i++) {
        tree_->insert(i, "value" + std::to_string(i));
    }

    for (int i = 0; i < COUNT; i++) {
        auto result = tree_->find(i);
        ASSERT_EQ(result.size(), 1);
        EXPECT_EQ(result[0], "value" + std::to_string(i));
    }
}

TEST_F(BPlusTreeTest, InsertInDescendingOrder) {
    const int COUNT = 100;
    for (int i = COUNT - 1; i >= 0; i--) {
        tree_->insert(i, "value" + std::to_string(i));
    }

    for (int i = 0; i < COUNT; i++) {
        auto result = tree_->find(i);
        ASSERT_EQ(result.size(), 1);
        EXPECT_EQ(result[0], "value" + std::to_string(i));
    }
}

TEST_F(BPlusTreeTest, InsertRandomOrder) {
    std::vector<int> keys = {5, 3, 8, 1, 9, 6, 4, 2, 7};
    for (int key : keys) {
        tree_->insert(key, "value" + std::to_string(key));
    }

    for (int key : keys) {
        auto result = tree_->find(key);
        ASSERT_EQ(result.size(), 1);
        EXPECT_EQ(result[0], "value" + std::to_string(key));
    }
}

TEST_F(BPlusTreeTest, InsertCausingNodeSplit) {
    // Assuming Order = 4 for this test
    tree_->insert(10, "value10");
    tree_->insert(20, "value20");
    tree_->insert(30, "value30");
    tree_->insert(40, "value40"); // This should cause a split

    auto result1 = tree_->find(10);
    auto result2 = tree_->find(20);
    auto result3 = tree_->find(30);
    auto result4 = tree_->find(40);

    ASSERT_EQ(result1.size(), 1);
    ASSERT_EQ(result2.size(), 1);
    ASSERT_EQ(result3.size(), 1);
    ASSERT_EQ(result4.size(), 1);

    EXPECT_EQ(result1[0], "value10");
    EXPECT_EQ(result2[0], "value20");
    EXPECT_EQ(result3[0], "value30");
    EXPECT_EQ(result4[0], "value40");
}

TEST_F(BPlusTreeTest, InsertWithNegativeKeys) {
    tree_->insert(-10, "value-10");
    tree_->insert(-5, "value-5");
    tree_->insert(-15, "value-15");

    auto result1 = tree_->find(-10);
    auto result2 = tree_->find(-5);
    auto result3 = tree_->find(-15);

    ASSERT_EQ(result1.size(), 1);
    ASSERT_EQ(result2.size(), 1);
    ASSERT_EQ(result3.size(), 1);

    EXPECT_EQ(result1[0], "value-10");
    EXPECT_EQ(result2[0], "value-5");
    EXPECT_EQ(result3[0], "value-15");
}

TEST_F(BPlusTreeTest, InsertEmptyString) {
    tree_->insert(1, "");
    auto result = tree_->find(1);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "");
}

TEST_F(BPlusTreeTest, InsertLargeValues) {
    std::string large_value(1000, 'x'); // Create a string with 1000 'x' characters
    tree_->insert(1, large_value);
    auto result = tree_->find(1);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], large_value);
}

TEST_F(BPlusTreeTest, ConcurrentInsert) {
    const int THREAD_COUNT = 4;
    const int INSERTS_PER_THREAD = 25;
    std::vector<std::thread> threads;

    for (int i = 0; i < THREAD_COUNT; i++) {
        threads.emplace_back([this, i]() {
            for (int j = 0; j < INSERTS_PER_THREAD; j++) {
                int key = i * INSERTS_PER_THREAD + j;
                tree_->insert(key, "value" + std::to_string(key));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    for (int i = 0; i < THREAD_COUNT * INSERTS_PER_THREAD; i++) {
        auto result = tree_->find(i);
        ASSERT_EQ(result.size(), 1);
        EXPECT_EQ(result[0], "value" + std::to_string(i));
    }
}


TEST_F(BPlusTreeTest, InsertSameKeyDifferentValue) {
    tree_->insert(1, "value1");
    EXPECT_THROW({
        tree_->insert(1, "value2");
    }, std::runtime_error);

    auto result = tree_->find(1);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "value1"); // Original value should remain
}



TEST_F(BPlusTreeTest, RemoveFromEmptyTree) {
    tree_->remove(10);
    EXPECT_TRUE(tree_->empty());
}

TEST_F(BPlusTreeTest, RemoveNonexistentElement) {
    tree_->insert(10, "value1");
    tree_->remove(20);
    auto result = tree_->find(10);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "value1");
}

TEST_F(BPlusTreeTest, RemoveSingleElement) {
    tree_->insert(10, "value1");
    tree_->remove(10);
    EXPECT_TRUE(tree_->empty());
}

TEST_F(BPlusTreeTest, RemoveWithoutRebalancing) {
    tree_->insert(10, "value1");
    tree_->insert(20, "value2");
    tree_->insert(30, "value3");
    
    tree_->remove(20);
    
    auto result1 = tree_->find(10);
    auto result2 = tree_->find(30);
    ASSERT_EQ(result1.size(), 1);
    ASSERT_EQ(result2.size(), 1);
    EXPECT_EQ(result1[0], "value1");
    EXPECT_EQ(result2[0], "value3");
}

TEST_F(BPlusTreeTest, RemoveWithLeftRedistribution) {
    for(int i = 1; i <= 10; i++) {
        tree_->insert(i * 10, "value" + std::to_string(i));
    }
    
    tree_->remove(50);
    
    for(int i = 1; i <= 10; i++) {
        if(i != 5) {
            auto result = tree_->find(i * 10);
            ASSERT_EQ(result.size(), 1);
            EXPECT_EQ(result[0], "value" + std::to_string(i));
        }
    }
}

TEST_F(BPlusTreeTest, RemoveWithRightRedistribution) {
    for(int i = 1; i <= 10; i++) {
        tree_->insert(i * 10, "value" + std::to_string(i));
    }
    
    tree_->remove(60);
    
    for(int i = 1; i <= 10; i++) {
        if(i != 6) {
            auto result = tree_->find(i * 10);
            ASSERT_EQ(result.size(), 1);
            EXPECT_EQ(result[0], "value" + std::to_string(i));
        }
    }
}

TEST_F(BPlusTreeTest, RemoveWithMerging) {
    for(int i = 1; i <= 5; i++) {
        tree_->insert(i * 10, "value" + std::to_string(i));
    }
    
    tree_->remove(30);
    tree_->remove(40);
    
    for(int i = 1; i <= 5; i++) {
        if(i != 3 && i != 4) {
            auto result = tree_->find(i * 10);
            ASSERT_EQ(result.size(), 1);
            EXPECT_EQ(result[0], "value" + std::to_string(i));
        }
    }
}

TEST_F(BPlusTreeTest, RemoveAllElements) {
    for(int i = 1; i <= 5; i++) {
        tree_->insert(i * 10, "value" + std::to_string(i));
    }
    
    for(int i = 1; i <= 5; i++) {
        tree_->remove(i * 10);
    }
    
    EXPECT_TRUE(tree_->empty());
}

TEST_F(BPlusTreeTest, RemoveWithRootChange) {
    for(int i = 1; i <= 10; i++) {
        tree_->insert(i * 10, "value" + std::to_string(i));
    }
    
    for(int i = 1; i <= 8; i++) {
        tree_->remove(i * 10);
    }
    
    auto result1 = tree_->find(90);
    auto result2 = tree_->find(100);
    ASSERT_EQ(result1.size(), 1);
    ASSERT_EQ(result2.size(), 1);
    EXPECT_EQ(result1[0], "value9");
    EXPECT_EQ(result2[0], "value10");
}

TEST_F(BPlusTreeTest, RemoveAndReinsert) {
    tree_->insert(10, "value1");
    tree_->remove(10);
    tree_->insert(10, "value2");
    
    auto result = tree_->find(10);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "value2");
}


TEST_F(BPlusTreeTest, FindExistingKey) {
    tree_->insert(10, "value1");
    auto result = tree_->find(10);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "value1");
}

TEST_F(BPlusTreeTest, FindNonExistingKey) {
    tree_->insert(10, "value1");
    auto result = tree_->find(20);
    EXPECT_TRUE(result.empty());
}

TEST_F(BPlusTreeTest, RangeSearch) {
    tree_->insert(10, "value1");
    tree_->insert(20, "value2");
    tree_->insert(30, "value3");
    tree_->insert(40, "value4");

    auto result = tree_->range_search(15, 35);
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], "value2");
    EXPECT_EQ(result[1], "value3");
}

TEST_F(BPlusTreeTest, EmptyRangeSearch) {
    tree_->insert(10, "value1");
    tree_->insert(30, "value2");

    auto result = tree_->range_search(15, 25);
    EXPECT_TRUE(result.empty());
}

TEST_F(BPlusTreeTest, IteratorTest) {
    tree_->insert(10, "value1");
    tree_->insert(20, "value2");
    tree_->insert(30, "value3");

    std::vector<int> keys;
    std::vector<std::string> values;
    
    for (const auto& pair : *tree_) {
        keys.push_back(pair.first_);
        values.push_back(pair.second_);
    }

    ASSERT_EQ(keys.size(), 3);
    EXPECT_EQ(keys[0], 10);
    EXPECT_EQ(keys[1], 20);
    EXPECT_EQ(keys[2], 30);
    EXPECT_EQ(values[0], "value1");
    EXPECT_EQ(values[1], "value2");
    EXPECT_EQ(values[2], "value3");
}


TEST_F(BPlusTreeTest, Height) {
    EXPECT_EQ(tree_->height(), 0);
    
    tree_->insert(1, "one");
    EXPECT_EQ(tree_->height(), 1);
    
    for (int i = 2; i <= 200; ++i) {
        tree_->insert(i, std::to_string(i));
    }
    
    EXPECT_GT(tree_->height(), 1);
    
    size_t max_theoretical_height = 
        static_cast<size_t>(std::ceil(std::log(200) / std::log(TEST_ORDER / 2))) + 1;
    EXPECT_LE(tree_->height(), max_theoretical_height);
}


TEST_F(BPlusTreeTest, FillFactor) {
    EXPECT_DOUBLE_EQ(tree_->fill_factor(), 0.0);
    
    tree_->insert(1, "one");
    EXPECT_GT(tree_->fill_factor(), 0.0);
    EXPECT_LT(tree_->fill_factor(), 1.0);
    
    size_t optimal_elements = (2 * TEST_ORDER) / 3;
    for (size_t i = 2; i <= optimal_elements; ++i) {
        tree_->insert(i, std::to_string(i));
    }
    
    EXPECT_NEAR(tree_->fill_factor(), 0.67, 0.1);
    
    for (size_t i = optimal_elements + 1; i <= TEST_ORDER - 1; ++i) {
        tree_->insert(i, std::to_string(i));
    }
    
    EXPECT_NEAR(tree_->fill_factor(), 1.0, 0.1);
    
    for (size_t i = 1; i <= (TEST_ORDER - 1) / 2; ++i) {
        tree_->remove(i);
    }
    
    EXPECT_LT(tree_->fill_factor(), 0.7);
}

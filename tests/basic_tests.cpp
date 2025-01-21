#include "gtest/gtest.h"
#include <memory>
#include "../src/BP-Tree.hpp"
#include "string"


class BPlusTreeTest : public ::testing::Test {
  protected:

    std::unique_ptr<BPlusTree<int, std::string>> tree_;

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


TEST_F(BPlusTreeTest, RemoveExistingElement) {
    std::string value = "value1";
    tree_->insert(10, value);
    tree_->remove(10);

    auto result = tree_->find(10);
    EXPECT_TRUE(result.empty());
}

TEST_F(BPlusTreeTest, RemoveNonExistingElement) {
    std::string value = "value1";
    tree_->insert(10, value);
    tree_->remove(20);

    auto result = tree_->find(10);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "value1");
}


TEST_F(BPlusTreeTest, FindExistingElement) {
    std::string value = "value1";
    tree_->insert(10, value);

    auto result = tree_->find(10);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "value1");
}

TEST_F(BPlusTreeTest, FindNonExistingElement) {
    std::string value = "value1";
    tree_->insert(10, value);

    auto result = tree_->find(20);
    EXPECT_TRUE(result.empty());
}


TEST_F(BPlusTreeTest, InsertLvalue) {
    std::string value = "value1"; 
    tree_->insert(10, value);

    auto result = tree_->find(10);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "value1");
}

TEST_F(BPlusTreeTest, InsertRvalue) {
    tree_->insert(10, std::string("value1")); 

    auto result = tree_->find(10);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "value1");
}

TEST_F(BPlusTreeTest, InsertStringLiteral) {
    tree_->insert(10, "value1"); 

    auto result = tree_->find(10);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "value1");
}


TEST_F(BPlusTreeTest, RangeSearch) {
    tree_->insert(10, std::string("value1"));
    tree_->insert(20, std::string("value2"));
    tree_->insert(30, std::string("value3"));
    tree_->insert(40, std::string("value4"));

    auto result = tree_->range_search(15, 35);
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], "value2");
    EXPECT_EQ(result[1], "value3");
}

TEST_F(BPlusTreeTest, Iterator) {
    tree_->insert(10, std::string("value1"));
    tree_->insert(20, std::string("value2"));
    tree_->insert(30, std::string("value3"));

    std::vector<std::string> values;
    for (auto it = tree_->begin(); it != tree_->end(); ++it) {
        values.push_back((*it).second_);
    }

    ASSERT_EQ(values.size(), 3);
    EXPECT_EQ(values[0], "value1");
    EXPECT_EQ(values[1], "value2");
    EXPECT_EQ(values[2], "value3");
}

TEST_F(BPlusTreeTest, BalanceAfterInsert) {
    for (int i = 1; i <= 100; ++i) {
        tree_->insert(i, std::string("value") + std::to_string(i));
    }

    for (int i = 1; i <= 100; ++i) {
        auto result = tree_->find(i);
        ASSERT_EQ(result.size(), 1);
        EXPECT_EQ(result[0], std::string("value") + std::to_string(i));
    }
}

TEST_F(BPlusTreeTest, BalanceAfterRemove) {
    for (int i = 1; i <= 100; ++i) {
        tree_->insert(i, std::string("value") + std::to_string(i));
    }

    for (int i = 1; i <= 50; ++i) {
        tree_->remove(i);
    }

    for (int i = 51; i <= 100; ++i) {
        auto result = tree_->find(i);
        ASSERT_EQ(result.size(), 1);
        EXPECT_EQ(result[0], std::string("value") + std::to_string(i));
    }
}

TEST_F(BPlusTreeTest, CopyConstructor) {
    tree_->insert(10, std::string("value1"));
    tree_->insert(20, std::string("value2"));

    BPlusTree<int, std::string> treeCopy(*tree_);
    auto result1 = treeCopy.find(10);
    auto result2 = treeCopy.find(20);
    ASSERT_EQ(result1.size(), 1);
    ASSERT_EQ(result2.size(), 1);
    EXPECT_EQ(result1[0], "value1");
    EXPECT_EQ(result2[0], "value2");
}

TEST_F(BPlusTreeTest, MoveConstructor) {
    tree_->insert(10, std::string("value1"));
    tree_->insert(20, std::string("value2"));

    BPlusTree<int, std::string> treeMoved(std::move(*tree_));
    auto result1 = treeMoved.find(10);
    auto result2 = treeMoved.find(20);
    ASSERT_EQ(result1.size(), 1);
    ASSERT_EQ(result2.size(), 1);
    EXPECT_EQ(result1[0], "value1");
    EXPECT_EQ(result2[0], "value2");

    EXPECT_TRUE(tree_->empty());
}

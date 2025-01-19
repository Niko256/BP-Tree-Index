#include "BP-Tree.hpp"
#include <algorithm>
#include <cstddef>
#include <mutex>
#include <stdexcept>

// ---------------- INTERNAL NODE METHODS IMPLEMENTATION ----------------

template <typename Key, typename RecordId, size_t Order>
bool InternalNode<Key, RecordId, Order>::is_full() const {
    return keys_.size() >= Order - 1;
}

template <typename Key, typename RecordId, size_t Order>
size_t InternalNode<Key, RecordId, Order>::size() const {
    return keys_.size();
}

template <typename Key, typename RecordId, size_t Order>
void InternalNode<Key, RecordId, Order>::insert_key_at(size_t index, const Key& key) {
    keys_.insert(keys_.begin() + index, key);
}

template <typename Key, typename RecordId, size_t Order>
void InternalNode<Key, RecordId, Order>::insert_key_at(size_t index, Key&& key) {
    keys_.insert(keys_.begin() + index, std::move(key));
}

// ---------------- LEAF NODE METHODS IMPLEMENTATION ----------------

template <typename Key, typename RecordId, size_t Order>
size_t LeafNode<Key, RecordId, Order>::size() const {
    return keys_.size();
}

template <typename Key, typename RecordId, size_t Order>
bool LeafNode<Key, RecordId, Order>::is_full() const {
    return keys_.size() >= Order - 1;
}

template <typename Key, typename RecordId, size_t Order>
RecordId LeafNode<Key, RecordId, Order>::get_record(size_t index) const {
    if (index >= values_.size()) {
        throw std::out_of_range("Index out of range in get_record method");
    }
    return values_[index];
}

// ---------------- ITERATOR METHODS IMPLEMENTATION ----------------

template <typename Key, typename RecordId, size_t Order, typename compare>
BPlusTree<Key, RecordId, Order, compare>::Iterator::Iterator(LeafNodePtr node, size_t index)
    : current_node_(node), current_index_(index) {}


template <typename Key, typename RecordId, size_t Order, typename compare>
typename BPlusTree<Key, RecordId, Order, compare>::Iterator&
BPlusTree<Key, RecordId, Order, compare>::Iterator::operator++() {
    if (current_node_) {
        current_index_++;
        if (current_index_ >= current_node_->keys_.size()) {
            current_node_ = current_node_->next_;
            current_index_ = 0;
        }
    }
    return *this;
}

template <typename Key, typename RecordId, size_t Order, typename compare>
Pair<const Key&, RecordId&> BPlusTree<Key, RecordId, Order, compare>::Iterator::operator*() const {
    if (current_node_) {
        return { current_node_->keys_[current_index_], current_node_->values_[current_index_] };
    }
    throw std::out_of_range("Iterator is out of range");
}

template <typename Key, typename RecordId, size_t Order, typename compare>
bool BPlusTree<Key, RecordId, Order, compare>::Iterator::operator==(const Iterator& other) const {
    return current_node_ == other.current_node_ && current_index_ == other.current_index_;
}

template <typename Key, typename RecordId, size_t Order, typename compare>
bool BPlusTree<Key, RecordId, Order, compare>::Iterator::operator!=(const Iterator& other) const {
    return !(*this == other);
}

// ---------------- CONST ITERATOR METHODS IMPLEMENTATION ----------------

template <typename Key, typename RecordId, size_t Order, typename compare>
typename BPlusTree<Key, RecordId, Order, compare>::ConstIterator& 
BPlusTree<Key, RecordId, Order, compare>::ConstIterator::operator++() {
    if (!current_node_) {
        return *this;
    }

    current_index_++;
    
    if (current_index_ >= current_node_->size()) {
        current_node_ = current_node_->next_;
        current_index_ = 0;
    }
    
    return *this;
}

template <typename Key, typename RecordId, size_t Order, typename compare>
const Pair<const Key&, const RecordId&> 
BPlusTree<Key, RecordId, Order, compare>::ConstIterator::operator*() const {
    if (!current_node_ || current_index_ >= current_node_->size()) {
        throw std::runtime_error("Invalid iterator dereference");
    }
    
    return Pair<const Key&, const RecordId&>(
        current_node_->keys_[current_index_],
        current_node_->get_record(current_index_)
    );
}

template <typename Key, typename RecordId, size_t Order, typename compare>
bool BPlusTree<Key, RecordId, Order, compare>::ConstIterator::operator==(const ConstIterator& other) const {
    return current_node_ == other.current_node_ && current_index_ == other.current_index_;
}

template <typename Key, typename RecordId, size_t Order, typename compare>
bool BPlusTree<Key, RecordId, Order, compare>::ConstIterator::operator!=(const ConstIterator& other) const {
    return !(*this == other);
}


// ---------------- B+TREE IMPLEMENTATION ----------------


/**
 * @brief Splits a child node when it becomes full.
 * @param parent The parent node containing the child to split.
 * @param child_index The index of the child node to split.
 * @details This method is called when a child node (either internal or leaf) becomes full.
 * It splits the child into two nodes and promotes the middle key to the parent.
 * If the parent becomes full after the promotion, it recursively splits the parent.
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::split_child(VariantNode<Key, RecordId, Order>& parent, size_t child_index) {

    // Cast the parent node to an InternalNodePtr since we know it's an internal node
    auto parent_internal = std::get<InternalNodePtr>(parent);
    
    // Get the child node that needs to be split
    auto child = parent_internal->children_[child_index];

    // Check if the child is an internal node (non-leaf node)
    
    if (std::holds_alternative<InternalNodePtr>(child)) {
        // Cast the child to an InternalNodePtr
        auto child_internal = std::get<InternalNodePtr>(child);
        
        // Create a new internal node to hold the second half of the keys and children
        auto new_node = SharedPtr<InternalNode<Key, RecordId, Order>>(new InternalNode<Key, RecordId, Order>());

        // Calculate the midpoint for splitting
        size_t mid = (Order - 1) / 2;
        
        // Get the median key which will be promoted to the parent
        Key median = child_internal->keys_[mid];

        // Assign the second half of the keys and children to the new node
        new_node->keys_.assign(child_internal->keys_.begin() + mid + 1, child_internal->keys_.end());
        new_node->children_.assign(child_internal->children_.begin() + mid + 1, child_internal->children_.end());
        
        // Resize the original child node to contain only the first half of the keys and children
        child_internal->keys_.resize(mid);
        child_internal->children_.resize(mid + 1);

        // Insert the median key into the parent node at the appropriate index
        parent_internal->insert_key_at(child_index, median);
        
        // Insert the new node into the parent's children array
        parent_internal->children_.insert(
            parent_internal->children_.begin() + child_index + 1,
            VariantNode<Key, RecordId, Order>(new_node));

        // If the parent node is now full, recursively split it
        if (parent_internal->is_full()) {
            split_child(parent, child_index);
        }
    
    } else {
        // If the child is a leaf node, perform a similar split operation
        auto child_leaf = std::get<LeafNodePtr>(child);
        
        // Create a new leaf node to hold the second half of the keys and values
        auto new_leaf = SharedPtr<LeafNode<Key, RecordId, Order>>(new LeafNode<Key, RecordId, Order>());

        // Calculate the midpoint for splitting
        size_t mid = Order / 2;

        // Assign the second half of the keys and values to the new leaf node
        new_leaf->keys_.assign(child_leaf->keys_.begin() + mid, child_leaf->keys_.end());
        new_leaf->values_.assign(child_leaf->values_.begin() + mid, child_leaf->values_.end());
        
        // Resize the original leaf node to contain only the first half of the keys and values
        child_leaf->keys_.resize(mid);
        child_leaf->values_.resize(mid);

        // Update the linked list of leaf nodes
        new_leaf->next_ = child_leaf->next_;
        child_leaf->next_ = new_leaf;

        // Insert the first key of the new leaf node into the parent node
        parent_internal->insert_key_at(child_index, new_leaf->keys_[0]);
        
        // Insert the new leaf node into the parent's children array
        parent_internal->children_.insert(
            parent_internal->children_.begin() + child_index + 1,
            VariantNode<Key, RecordId, Order>(new_leaf));

        // If the parent node is now full, recursively split it
        if (parent_internal->is_full()) {
            split_child(parent, child_index);
        }
    }

    size_++;
}


/**
 * @brief Finds the leaf node that should contain the specified key.
 * @param key The key to search for.
 * @return A pointer to the leaf node that should contain the key.
 * @details This method traverses the tree from the root to the appropriate leaf node
 * by comparing the key with the keys in internal nodes.
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
typename BPlusTree<Key, RecordId, Order, compare>::LeafNodePtr
BPlusTree<Key, RecordId, Order, compare>::find_leaf(const Key& key) {
    auto node = root_;

    // Traverse the tree until a leaf node is reached
    while (!std::holds_alternative<LeafNodePtr>(node)) {
        auto internal_node = std::get<InternalNodePtr>(node);
        size_t i = 0;

        // Find the appropriate child node to traverse
        while (i < internal_node->keys_.size() && is_less_or_eq(internal_node->keys_[i], key)) {
            ++i;
        }
        node = internal_node->children_[i];
    }
    return std::get<LeafNodePtr>(node);
}


template <typename Key, typename RecordId, size_t Order, typename compare>
bool BPlusTree<Key, RecordId, Order, compare>::is_less_or_eq(const Key& key1, const Key& key2) const {
    return !comparator_(key2, key1);
} 


/**
 * @brief Finds all records associated with the specified key.
 * @param key The key to search for.
 * @return A dynamic array of record IDs associated with the key.
 * @details This method searches for the specified key in the B+ Tree. It starts by acquiring
 * a shared lock to ensure thread safety during the search. It then finds the leaf node that
 * should contain the key using the `find_leaf` method. Finally, it iterates through the keys
 * in the leaf node to find all matching records and returns them in a dynamic array.
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
DynamicArray<RecordId> BPlusTree<Key, RecordId, Order, compare>::find(const Key& key) {

    // Acquire a shared lock to ensure thread safety
    std::shared_lock lock(root_mutex_);
    DynamicArray<RecordId> result;

    // Find the leaf node that should contain the key
    auto leaf = find_leaf(key);

    // Iterate through the keys in the leaf node to find matching records
    for (size_t i = 0; i < leaf->keys_.size(); ++i) {
        if (key == leaf->keys_[i]) {
            result.push_back(leaf->values_[i]); 
        }
    }
    return result;
}


/**
 * @brief Performs a range search for records with keys between `from` and `to`.
 * @param from The lower bound of the range.
 * @param to The upper bound of the range.
 * @return A dynamic array of record IDs within the specified range.
 * @details This method performs a range search in the B+ Tree. It starts by acquiring
 * a shared lock to ensure thread safety. It then finds the leaf node that should contain
 * the lower bound (`from`) using the `find_leaf` method. It iterates through the keys in
 * the leaf node and subsequent leaf nodes, adding records with keys within the range to
 * the result array. The search stops when a key exceeds the upper bound (`to`).
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
DynamicArray<RecordId> BPlusTree<Key, RecordId, Order, compare>::range_search(const Key& from, const Key& to) {
    
    // Acquire a shared lock to ensure thread safety
    std::shared_lock lock(root_mutex_);
    DynamicArray<RecordId> result;

    // Find the leaf node that should contain the lower bound (`from`)
    auto leaf = find_leaf(from);

    // Iterate through the leaf nodes and their keys
    while (leaf) {
        for (size_t i = 0; i < leaf->keys_.size(); i++) {
            // Check if the key is within the range [from, to]
            if (is_less_or_eq(from, leaf->keys_[i]) && is_less_or_eq(leaf->keys_[i], to)) {
                result.push_back(leaf->values_[i]);
            }
        }
        // Move to the next leaf node
        leaf = leaf->next_;
    }
    return result;
}


/**
 * @brief Inserts a key-record pair into the B+ Tree.
 * @param key The key to insert.
 * @param id The record ID associated with the key.
 * @throws std::runtime_error If a duplicate key is inserted.
 * @details This method inserts a key-value pair into the tree. If the tree is empty,
 * it creates a new leaf node. If the leaf node is full, it splits the node and updates
 * the tree structure.
 */

template <typename Key, typename RecordId, size_t Order, typename Compare>
void BPlusTree<Key, RecordId, Order, Compare>::insert(const Key& key, RecordId& id) {

    std::unique_lock lock(root_mutex_);
    if (std::holds_alternative<std::monostate>(root_)) {
        // If the tree is empty, create a new leaf node
        auto new_leaf = SharedPtr<LeafNode<Key, RecordId, Order>>(new LeafNode<Key, RecordId, Order>());
        new_leaf->keys_.push_back(key);
        new_leaf->values_.push_back(id);
        root_ = new_leaf;
    } else {
        // Find the leaf node where the key should be inserted
        auto leaf = find_leaf(key);

        // Check if the key already exists in the leaf node
        auto it = std::lower_bound(leaf->keys_.begin(), leaf->keys_.end(), key, comparator_);
        if (it != leaf->keys_.end() && *it == key) {
            throw std::runtime_error("Duplicate key insertion is not allowed");
        }

        // If the leaf node is full, split it
        if (leaf->is_full()) {
            if (std::holds_alternative<LeafNodePtr>(root_)) {
                // If the root is a leaf node, create a new internal node as the root
                auto new_root = SharedPtr<InternalNode<Key, RecordId, Order>>(new InternalNode<Key, RecordId, Order>());
                new_root->children_.push_back(root_);
                root_ = new_root;
            }
            split_child(root_, 0);
            insert(key, id);
        } else {
            // Insert the key and value into the leaf node
            leaf->keys_.push_back(key);
            leaf->values_.push_back(id);
        }
    }
    size_++;
}



/**
 * @brief Redistributes keys and children between two nodes to balance the tree.
 * @param lhs The left-hand side node.
 * @param rhs The right-hand side node.
 * @details This method is used to balance two adjacent nodes (either internal or leaf nodes).
 * If one node is underfull, it borrows a key and child from the other node. If both nodes are
 * internal nodes, it redistributes keys and children. If both nodes are leaf nodes, it redistributes
 * keys and values.
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::redistribute_nodes(
        VariantNode<Key, RecordId, Order>& lhs, VariantNode<Key, RecordId, Order>& rhs) {
    
    // Check if both nodes are internal nodes
    if (std::holds_alternative<InternalNodePtr>(lhs) && std::holds_alternative<InternalNodePtr>(rhs)) {
        auto left_internal = std::get<InternalNodePtr>(lhs);
        auto right_internal = std::get<InternalNodePtr>(rhs);

        // If the left node is underfull, borrow from the right node
        if (left_internal->size() < Order / 2) {
            left_internal->keys_.push_back(right_internal->keys_.front());
            left_internal->children_.push_back(right_internal->children_.front());

            right_internal->keys_.erase(right_internal->keys_.begin());
            right_internal->children_.erase(right_internal->children_.begin());
        
        } else {
            // If the right node is underfull, borrow from the left node
            right_internal->keys_.insert(right_internal->keys_.begin(), left_internal->keys_.back());
            right_internal->children_.insert(right_internal->children_.begin(), left_internal->children_.back());

            left_internal->keys_.pop_back();
            left_internal->children_.pop_back();
        }

    } else if (std::holds_alternative<LeafNodePtr>(lhs) && std::holds_alternative<LeafNodePtr>(rhs)) {
        // If both nodes are leaf nodes
        auto left_leaf = std::get<LeafNodePtr>(lhs);
        auto right_leaf = std::get<LeafNodePtr>(rhs);

        // If the left leaf node is underfull, borrow from the right leaf node
        if (left_leaf->size() < Order / 2) {
            left_leaf->keys_.push_back(right_leaf->keys_.front());
            left_leaf->values_.push_back(right_leaf->values_.front());
            
            right_leaf->keys_.erase(right_leaf->keys_.begin());
            right_leaf->values_.erase(right_leaf->values_.begin());

        } else {
            // If the right leaf node is underfull, borrow from the left leaf node
            right_leaf->keys_.insert(right_leaf->keys_.begin(), left_leaf->keys_.back());
            right_leaf->values_.insert(right_leaf->values_.begin(), left_leaf->values_.back());

            left_leaf->keys_.pop_back();
            left_leaf->values_.pop_back();
        }
    }
} 


/**
 * @brief Merges two nodes into one to balance the tree.
 * @param left The left node to merge.
 * @param right The right node to merge.
 * @details This method merges two adjacent nodes (either internal or leaf nodes) into one.
 * If both nodes are internal nodes, it combines their keys and children. If both nodes are
 * leaf nodes, it combines their keys and values and updates the linked list of leaf nodes.
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::merge_nodes(
    VariantNode<Key, RecordId, Order>& left, VariantNode<Key, RecordId, Order>& right) {

    // Check if both nodes are internal nodes
    if (std::holds_alternative<InternalNodePtr>(left) && std::holds_alternative<InternalNodePtr>(right)) {
        auto left_internal = std::get<InternalNodePtr>(left);
        auto right_internal = std::get<InternalNodePtr>(right);

        // Combine keys and children from the right node into the left node
        left_internal->keys_.insert(left_internal->keys_.end(), right_internal->keys_.begin(), right_internal->keys_.end());
        left_internal->children_.insert(left_internal->children_.end(), right_internal->children_.begin(), right_internal->children_.end());

        // Clear the right node
        right_internal->keys_.clear();
        right_internal->children_.clear();

    } else if (std::holds_alternative<LeafNodePtr>(left) && std::holds_alternative<LeafNodePtr>(right)) {
        // If both nodes are leaf nodes
        auto left_leaf = std::get<LeafNodePtr>(left);
        auto right_leaf = std::get<LeafNodePtr>(right);

        // Combine keys and values from the right node into the left node
        left_leaf->keys_.insert(left_leaf->keys_.end(), right_leaf->keys_.begin(), right_leaf->keys_.end());
        left_leaf->values_.insert(left_leaf->values_.end(), right_leaf->values_.begin(), right_leaf->values_.end());

        // Update the linked list of leaf nodes
        left_leaf->next_ = right_leaf->next_;

        // Clear the right node
        right_leaf->keys_.clear();
        right_leaf->values_.clear();
    }
}



/**
 * @brief Removes a key from the B+ Tree.
 * @param key The key to remove.
 * @details This method removes a key from the tree. If the key is not found, it does nothing.
 * If the leaf node becomes underfull after removal, it balances the tree by redistributing
 * or merging nodes.
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::remove(const Key& key) {
    std::unique_lock lock(root_mutex_);
    
    // If the tree is empty, return
    if (std::holds_alternative<std::monostate>(root_)) {
        return;
    }

    // Find the leaf node containing the key
    auto leaf = find_leaf(key);
    auto it = std::lower_bound(leaf->keys_.begin(), leaf->keys_.end(), key, comparator_);

    // If the key is not found, return
    if (it == leaf->keys_.end() || *it != key) {
        return;
    }

    // Remove the key and value from the leaf node
    size_t index = std::distance(leaf->keys_.begin(), it);
    leaf->keys_.erase(leaf->keys_.begin() + index);
    leaf->values_.erase(leaf->values_.begin() + index);
    --size_;

    // If the leaf node becomes underfull, balance the tree
    if (leaf->size() < Order / 2) {
        balance_after_remove(leaf);
    }
}


/**
 * @brief Balances the tree after a removal operation.
 * @param node The node to balance.
 * @details This method is called when a node becomes underfull after a removal.
 * It checks if the node can borrow keys from a sibling or if it needs to merge with a sibling.
 * If the parent node becomes underfull after merging, it recursively balances the parent.
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::balance_after_remove(LeafNodePtr node) {
    // If the node is the root and becomes empty, set the root to null
    if (node == std::get<LeafNodePtr>(root_)) {
        if (node->size() == 0) {
            root_ = LeafNodePtr{};
        }
        return;
    }

    // Find the parent of the node
    auto parent = find_parent(node);
    size_t index = 0;
    while (index < parent->children_.size() && std::get<LeafNodePtr>(parent->children_[index]) != node) {
        index++;
    }

    // Check if the left sibling has extra keys
    if (index > 0) {
        auto left_sibling = std::get<LeafNodePtr>(parent->children_[index - 1]);
        if (left_sibling->size() > Order / 2) {
            redistribute_nodes(parent->children_[index - 1], parent->children_[index]);
            return;
        }
    }

    // Check if the right sibling has extra keys
    if (index < parent->children_.size() - 1) {
        auto right_sibling = std::get<LeafNodePtr>(parent->children_[index + 1]);
        if (right_sibling->size() > Order / 2) {
            redistribute_nodes(parent->children_[index], parent->children_[index + 1]);
            return;
        }
    }

    // If no sibling has extra keys, merge the node with a sibling
    if (index > 0) {
        merge_nodes(parent->children_[index - 1], parent->children_[index]);
    } else {
        merge_nodes(parent->children_[index], parent->children_[index + 1]);
    }

    // If the parent becomes underfull, balance it
    if (parent->size() < Order / 2) {
        if (parent == std::get<InternalNodePtr>(root_)) {
            root_ = parent->children_[0];
        } else {
            balance_after_remove(parent);
        }
    }
}


/**
 * @brief Finds the parent of a given leaf node.
 * @param node The leaf node to find the parent of.
 * @return A pointer to the parent internal node.
 * @details This method traverses the tree from the root to find the parent of the specified leaf node.
 * It checks each internal node's children to locate the leaf node.
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
SharedPtr<InternalNode<Key, RecordId, Order>>
BPlusTree<Key, RecordId, Order, compare>::find_parent(LeafNodePtr node) {

    // If the node is the root, it has no parent
    if (node == root_) {
        return nullptr;
    }

    auto current = root_;
    while (!std::holds_alternative<LeafNodePtr>(current)) {
        auto internal_node = std::get<InternalNodePtr>(current);
        for (size_t i = 0; i < internal_node->children_.size(); i++) {
            if (std::holds_alternative<LeafNodePtr>(internal_node->children_[i]) &&
                std::get<LeafNodePtr>(internal_node->children_[i]) == node) {
                return internal_node;
            }
        }
        current = internal_node->children_[0];
    }
    return nullptr;
}


/**
 * @brief Bulk loads keys and records into the B+ Tree.
 * @param first The beginning of the input range.
 * @param last The end of the input range.
 * @throws std::runtime_error If duplicate or unsorted keys are encountered.
 * @details This method bulk loads keys and records into the B+ Tree. 
 * If the input range is empty, it clears the tree.
 * Otherwise, it creates leaf nodes and fills them with keys and records from the input range.
 * It ensures that the keys are sorted and unique. After creating the leaf nodes, it builds
 * the internal nodes level by level until the root is created.
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
template <typename InputIt>
void BPlusTree<Key, RecordId, Order, compare>::bulk_load(InputIt first, InputIt last) {
    // Acquire a unique lock to ensure thread safety
    std::unique_lock lock(root_mutex_);

    // If the input range is empty, clear the tree and return
    if (first == last) {
        clear();
        return;
    }

    // Initialize the root and size
    root_ = LeafNodePtr{};
    size_ = 0;

    // Create a list of leaf nodes
    DynamicArray<LeafNodePtr> leaves;
    LeafNodePtr prev_leaf = nullptr;

    // Fill leaf nodes with keys and records from the input range
    while (first != last) {
        auto new_leaf = SharedPtr<LeafNode<Key, RecordId, Order>>(new LeafNode<Key, RecordId, Order>());

        // Fill the leaf node until it reaches the maximum capacity or the input range is exhausted
        while (new_leaf->keys_.size() < Order && first != last) {
            // Ensure keys are sorted and unique
            if (!new_leaf->keys_.empty() && first->first_ <= new_leaf->keys_.back()) {
                throw std::runtime_error("Duplicate or unsorted key in bulk load");
            }
            new_leaf->keys_.push_back(first->first_);
            new_leaf->values_.push_back(first->second_);

            ++first;
            ++size_;
        }

        // Link the new leaf node to the previous one
        if (prev_leaf) {
            prev_leaf->next_ = new_leaf;
        }

        prev_leaf = new_leaf;
        leaves.push_back(new_leaf);
    }

    // Build the internal nodes level by level
    DynamicArray<VariantNode<Key, RecordId, Order>> current_level;
    for (const auto& leaf : leaves) {
        current_level.push_back(leaf);
    }

    while (current_level.size() > 1) {
        DynamicArray<VariantNode<Key, RecordId, Order>> next_level;
        auto it = current_level.begin();

        while (it != current_level.end()) {
            auto new_internal = SharedPtr<InternalNode<Key, RecordId, Order>>(new InternalNode<Key, RecordId, Order>());

            // Fill the internal node with keys and children
            while (new_internal->keys_.size() < Order - 1 && it != current_level.end()) {
                if (std::holds_alternative<LeafNodePtr>(*it)) {
                    auto leaf = std::get<LeafNodePtr>(*it);
                    new_internal->keys_.push_back(leaf->keys_.front());
                } else {
                    auto internal = std::get<InternalNodePtr>(*it);
                    new_internal->keys_.push_back(internal->keys_.front());
                }
                new_internal->children_.push_back(*it);
                ++it;
            }

            // Add the last child if necessary
            if (it != current_level.end()) {
                new_internal->children_.push_back(*it);
                ++it;
            }

            next_level.push_back(new_internal);
        }

        current_level = next_level;
    }

    // Set the root to the top-level internal node
    if (!current_level.empty()) {
        root_ = current_level.front();
    }
}



template <typename Key, typename RecordId, size_t Order, typename compare>
typename BPlusTree<Key, RecordId, Order, compare>::Iterator
BPlusTree<Key, RecordId, Order, compare>::begin() {
    auto leaf = std::get<LeafNodePtr>(root_);
    while (leaf && leaf->children_[0]) {
        leaf = std::get<LeafNodePtr>(leaf->children_[0]);
    }
    return Iterator(leaf, 0);
}

template <typename Key, typename RecordId, size_t Order, typename compare>
typename BPlusTree<Key, RecordId, Order, compare>::Iterator
BPlusTree<Key, RecordId, Order, compare>::end() {
    return Iterator(nullptr, 0);
}

template <typename Key, typename RecordId, size_t Order, typename compare>
bool BPlusTree<Key, RecordId, Order, compare>::empty() const {
    return size_ == 0;
}

template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::clear() {
    std::unique_lock lock(root_mutex_);
    root_ = LeafNodePtr{};
    size_ = 0;
}



/**
 * @brief Recursively copies an internal node and its children.
 * @param node The internal node to copy.
 * @return A shared pointer to the copied internal node.
 * @details This method creates a deep copy of an internal node, including all its keys and children.
 * If a child is an internal node, it recursively copies it. If a child is a leaf node, it also copies it.
 */
template <typename Key, typename RecordId, size_t Order, typename compare>
SharedPtr<InternalNode<Key, RecordId, Order>> BPlusTree<Key, RecordId, Order, compare>::copy_nodes(
        const SharedPtr<InternalNode<Key, RecordId, Order>>& node) {
    
    // Create a new internal node
    auto new_node = SharedPtr<InternalNode<Key, RecordId, Order>>(new InternalNode<Key, RecordId, Order>());
    // Copy the keys from the original node
    new_node->keys_ = node->keys_;

    // Copy the children
    for (const auto& child : node->children_) {
        if (std::holds_alternative<InternalNodePtr>(child)) {
            // If the child is an internal node, recursively copy it
            new_node->children_.push_back(copy_nodes(std::get<InternalNodePtr>(child)));
        } else {
            // If the child is a leaf node, copy it
            new_node->children_.push_back(copy_nodes(std::get<LeafNodePtr>(child)));
        }
    }
    return new_node;
}


/**
 * @brief Copies a leaf node.
 * @param node The leaf node to copy.
 * @return A shared pointer to the copied leaf node.
 * @details This method creates a deep copy of a leaf node, including its keys, values, and the next pointer.
 */
template <typename Key, typename RecordId, size_t Order, typename compare>
SharedPtr<LeafNode<Key, RecordId, Order>> BPlusTree<Key, RecordId, Order, compare>::copy_nodes(
        const SharedPtr<LeafNode<Key, RecordId, Order>>& node) {
    
    // Create a new leaf node
    auto new_node = SharedPtr<LeafNode<Key, RecordId, Order>>(new LeafNode<Key, RecordId, Order>());
    
    // Copy the keys, values, and next pointer
    new_node->keys_ = node->keys_;
    new_node->values_ = node->values_;
    new_node->next_ = node->next_;
    return new_node;
}


template <typename Key, typename RecordId, size_t Order, typename compare>
BPlusTree<Key, RecordId, Order, compare>::BPlusTree(const BPlusTree& other) {
    std::shared_lock lock(root_mutex_);

    if (std::holds_alternative<std::monostate>(other.root_)) {
        root_ = LeafNodePtr{};
        size_ = 0;
    } else {
        if (std::holds_alternative<InternalNodePtr>(other.root_)) {
            auto other_internal = std::get<InternalNodePtr>(other.root_);
            root_ = copy_nodes(other_internal);
        } else {
            auto other_leaf = std::get<LeafNodePtr>(other.root_);
            root_ = copy_nodes(other_leaf);
        }
        size_ = other.size_;
    }
}


template <typename Key, typename RecordId, size_t Order, typename compare>
BPlusTree<Key, RecordId, Order, compare>::BPlusTree(BPlusTree&& other) noexcept {
    std::unique_lock lock(other.root_mutex_); 
    root_ = std::move(other.root_);
    size_ = other.size_;
    other.root_ = LeafNodePtr{}; 
    other.size_ = 0;
}


template <typename Key, typename RecordId, size_t Order, typename compare>
BPlusTree<Key, RecordId, Order, compare>&
BPlusTree<Key, RecordId, Order, compare>::operator=(const BPlusTree& other) {
    if (this != &other) { 
        BPlusTree temp(other);
        std::unique_lock lock(root_mutex_);
        std::swap(root_, temp.root_);
        std::swap(size_, temp.size_);
    }
    return *this;
}


template <typename Key, typename RecordId, size_t Order, typename compare>
BPlusTree<Key, RecordId, Order, compare>& BPlusTree<Key, RecordId, Order, compare>::operator=(BPlusTree&& other) noexcept {
    if (this != &other) {

        std::unique_lock this_lock(root_mutex_);
        std::unique_lock other_lock(other.root_mutex_);

        root_ = std::move(other.root_);
        size_ = other.size_;
        other.root_ = LeafNodePtr{};
        other.size_ = 0;
    }
    return *this;
}

#include "BP-Tree.hpp"
#include <memory>
#include <algorithm>
#include <cstddef>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <variant>

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
    if (!current_node_) {
        return *this;
    }

    current_index_++;
    if (current_index_ >= current_node_->keys_.size()) {
        current_node_ = current_node_->next_;
        current_index_ = 0;
    }

    return *this;
}

template <typename Key, typename RecordId, size_t Order, typename compare>
Pair<const Key&, RecordId&> 
BPlusTree<Key, RecordId, Order, compare>::Iterator::operator*() const {
    if (!current_node_ || current_index_ >= current_node_->keys_.size()) {
        throw std::out_of_range("Iterator is out of range");
    }
    return {current_node_->keys_[current_index_], 
            current_node_->values_[current_index_]};
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
 * @brief Locates the leaf node where a given key should be found in the B+ tree
 * 
 * @param key The key value to search for
 * 
 * @return LeafNodePtr A pointer to the leaf node where the key should be located
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
typename BPlusTree<Key, RecordId, Order, compare>::LeafNodePtr 
BPlusTree<Key, RecordId, Order, compare>::find_leaf(const Key& key) {

    // Check if the tree is empty (root is monostate)
    if (std::holds_alternative<std::monostate>(root_)) {
        return nullptr;
    }
    
    // If root is a leaf node, return it directly
    if (std::holds_alternative<LeafNodePtr>(root_)) {
        return std::get<LeafNodePtr>(root_);
    }

    // Get the root as internal node to start traversal
    auto current = std::get<InternalNodePtr>(root_);
    while (true) {
        // Use binary search to find the first key greater than or equal to target key
        auto it = std::lower_bound(current->keys_.begin(),
                                 current->keys_.end(),
                                 key,
                                 comparator_);
        // Calculate the index where the key should be inserted
        size_t index = it - current->keys_.begin();
        
        // If the key is greater than all keys in node, use the last child
        if (index == current->keys_.size()) {
            index--;
        }
        
        // Get the appropriate child node
        auto& child = current->children_[index];
        
        // If child is a leaf node, we've reached our destination
        if (std::holds_alternative<LeafNodePtr>(child)) {
            return std::get<LeafNodePtr>(child);
        }
        // Otherwise, continue traversing down the tree
        current = std::get<InternalNodePtr>(child);
    }
}


template <typename Key, typename RecordId, size_t Order, typename compare>
bool BPlusTree<Key, RecordId, Order, compare>::is_less_or_eq(const Key& key1, const Key& key2) const {
    return !comparator_(key2, key1);
} 



/**
 * @brief Inserts a key-value pair into the B+ tree
 * 
 * @param key The key to insert
 * @param id The record ID associated with the key
 * 
 * @throws std::runtime_error If a duplicate key is found or if leaf node finding fails
 * 
 * @details
 * This method handles the insertion of new key-value pairs into the B+ tree.
 * It's thread-safe and handles the following cases:
 * 1. Insertion into an empty tree
 * 2. Insertion into an existing leaf node
 * 3. Splitting of full nodes when necessary
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
template <typename T>
void BPlusTree<Key, RecordId, Order, compare>::insert(const Key& key, T&& id) {

    // Acquire exclusive lock for the root to prevent concurrent modifications
    std::unique_lock<std::shared_mutex> write_lock(root_mutex_);

    // Handle insertion into empty tree
    if (std::holds_alternative<std::monostate>(root_)) {
        // Create new leaf node as root
        auto new_leaf = std::make_shared<LeafNode<Key, RecordId, Order>>();

        new_leaf->keys_.push_back(key);
        new_leaf->values_.push_back(std::forward<T>(id));  // Perfect forward the record ID
        
        root_ = new_leaf;
        size_++;
        return;
    }

    // Find the appropriate leaf node for insertion
    LeafNodePtr leaf = find_leaf(key);
    if (!leaf) {
        throw std::runtime_error("Failed to find leaf node");
    }

    std::unique_lock<std::shared_mutex> leaf_lock(leaf->mutex_);

    // Find the position where the key should be inserted
    auto it = std::lower_bound(leaf->keys_.begin(), 
                             leaf->keys_.end(), 
                             key, 
                             comparator_);
    size_t insert_pos = it - leaf->keys_.begin();

    // Check for duplicate keys
    if (it != leaf->keys_.end() && !comparator_(key, *it) && !comparator_(*it, key)) {
        throw std::runtime_error("Duplicate key");
    }

    // Insert the key-value pair at the appropriate position
    leaf->keys_.insert(leaf->keys_.begin() + insert_pos, key);
    leaf->values_.insert(leaf->values_.begin() + insert_pos, std::forward<T>(id));
    size_++;

    // If the leaf node is full after insertion, split it
    if (leaf->size() >= Order) {
        split_leaf(leaf);
    }
}


/**
 * @brief Splits a full leaf node into two nodes in the B+ tree
 * 
 * @param leaf Pointer to the leaf node that needs to be split
 * 
 * @details
 * This method handles the splitting of a full leaf node
 * The process includes:
 * 1. Creating a new leaf node
 * 2. Redistributing the keys and values
 * 3. Updating the linked list of leaf nodes
 * 4. Handling the parent node modifications
 * 
 * @note This method assumes the leaf node is already locked by the caller
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::split_leaf(LeafNodePtr leaf) {

    // Create a new leaf node to hold half of the elements
    auto new_leaf = std::make_shared<LeafNode<Key, RecordId, Order>>();
    
    size_t mid = leaf->keys_.size() / 2;
    
    // Copy the second half of keys and values to the new leaf
    new_leaf->keys_.assign(leaf->keys_.begin() + mid, leaf->keys_.end());
    new_leaf->values_.assign(leaf->values_.begin() + mid, leaf->values_.end());
    
    // Resize the original leaf to keep only the first half
    leaf->keys_.resize(mid);
    leaf->values_.resize(mid);
    
    // Update the linked list pointers
    // new_leaf->next_ points to whatever leaf->next_ was pointing to
    new_leaf->next_ = leaf->next_;
    // leaf->next_ now points to the new leaf
    leaf->next_ = new_leaf;

    // Handle the case when we're splitting the root leaf node
    if (std::holds_alternative<LeafNodePtr>(root_) && 
        std::get<LeafNodePtr>(root_) == leaf) {
     
        // Create new internal node as the root
        auto new_root = std::make_shared<InternalNode<Key, RecordId, Order>>();
        // Add the first key of the new leaf as the splitting key
        new_root->keys_.push_back(new_leaf->keys_.front());
        
        // Set up the children pointers
        new_root->children_.push_back(leaf);
        new_root->children_.push_back(new_leaf);
        // Update the root
        root_ = new_root;
    
    } else {
        // Handle the case when we're splitting a non-root leaf node
        auto parent = find_parent(leaf);
    
        if (parent) {
            // Find the position where the new key should be inserted in the parent
            size_t insert_pos = std::lower_bound(parent->keys_.begin(), 
                                               parent->keys_.end(), 
                                               new_leaf->keys_.front(), 
                                               comparator_) - parent->keys_.begin();
            
            // Insert the new key and child pointer into the parent
            parent->keys_.insert(parent->keys_.begin() + insert_pos, 
                               new_leaf->keys_.front());
            parent->children_.insert(parent->children_.begin() + insert_pos + 1, 
                                   new_leaf);

            // If the parent becomes full after insertion, split it
            if (parent->is_full()) {
                split_internal(parent);
            }
        }
    }
}


/**
 * @brief Splits a full internal node into two nodes in the B+ tree
 * 
 * @param node Pointer to the internal node that needs to be split
 * 
 * @details
 * This method handles the splitting of a full internal node.
 * The process includes:
 * 1. Creating a new internal node
 * 2. Moving half of the keys and children to the new node
 * 3. Promoting the middle key to the parent
 * 4. Handling special case when splitting the root
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::split_internal(InternalNodePtr node) {

    // Create a new internal node to hold right half of elements
    auto new_node = std::make_shared<InternalNode<Key, RecordId, Order>>();
    
    // Find the middle point and the key that will be promoted
    size_t mid = node->keys_.size() / 2;
    Key mid_key = node->keys_[mid];  // This key will be promoted to the parent
    
    // Move keys and children after the middle to the new node
    // !!!: For internal nodes, middle key goes up, not copied
    new_node->keys_.assign(node->keys_.begin() + mid + 1, node->keys_.end());
    new_node->children_.assign(node->children_.begin() + mid + 1, node->children_.end());
    
    // Resize the original node to remove transferred elements
    node->keys_.resize(mid);  // Remove middle key and everything after
    node->children_.resize(mid + 1);  // Keep one more child than keys

    // Handle the case when splitting the root internal node
    if (std::holds_alternative<InternalNodePtr>(root_) && 
        std::get<InternalNodePtr>(root_) == node) {

        auto new_root = std::make_shared<InternalNode<Key, RecordId, Order>>();
        
        // Add the promoted middle key
        new_root->keys_.push_back(mid_key);
        
        // Set up the children pointers
        new_root->children_.push_back(node);
        new_root->children_.push_back(new_node);
        
        // Update the root
        root_ = new_root;
    
    } else {
        // Handle the case when splitting a non-root internal node
        auto parent = find_parent(node);
        if (parent) {
            // Find the position where the promoted key should go in the parent
            size_t insert_pos = std::lower_bound(parent->keys_.begin(), 
                                               parent->keys_.end(), 
                                               mid_key, 
                                               comparator_) - parent->keys_.begin();
            
            // Insert the promoted key and new node pointer into the parent
            parent->keys_.insert(parent->keys_.begin() + insert_pos, mid_key);
            parent->children_.insert(parent->children_.begin() + insert_pos + 1, 
                                   new_node);

            // If the parent becomes full after insertion, split it recursively
            if (parent->is_full()) {
                split_internal(parent);
            }
        }
    }
}


template <typename Key, typename RecordId, size_t Order, typename compare>
typename BPlusTree<Key, RecordId, Order, compare>::InternalNodePtr
BPlusTree<Key, RecordId, Order, compare>::find_parent(const VariantNode<Key, RecordId, Order>& target) {
    // If tree is empty or target is root, return nullptr
    if (std::holds_alternative<std::monostate>(root_) || root_ == target) {
        return nullptr;
    }
    
    // Make sure root is an internal node
    if (!std::holds_alternative<InternalNodePtr>(root_)) {
        return nullptr;
    }
    
    auto current = std::get<InternalNodePtr>(root_);
    InternalNodePtr parent = nullptr;
    
    // Keep searching until we find the parent or exhaust all possibilities
    while (current) {
        bool found = false;
        
        // Search through all children of current node
        for (size_t i = 0; i < current->children_.size(); ++i) {
            if (current->children_[i] == target) {
                return current;  // Found the parent
            }
            
            // If child is internal node, we need to search deeper
            if (std::holds_alternative<InternalNodePtr>(current->children_[i])) {
                parent = current;
                current = std::get<InternalNodePtr>(current->children_[i]);
                found = true;
                break;
            }
        }
        
        // If we didn't find a path to continue searching, stop
        if (!found) {
            break;
        }
    }
    
    return nullptr;  // Parent not found
}


/**
 * @brief Removes a key and its associated value from the B+ tree
 * 
 * @param key The key to remove
 * 
 * @details
 * This method handles the removal of a key-value pair, including:
 * 1. Finding the appropriate leaf node
 * 2. Removing the key-value pair
 * 3. Handling underflow situations through rebalancing
 * 4. Special handling for root node
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::remove(const Key& key) {

    std::unique_lock write_lock(root_mutex_);

    // Return if tree is empty
    if (std::holds_alternative<std::monostate>(root_)) {
        return;
    }

    // Find the leaf node containing the key
    LeafNodePtr leaf = find_leaf(key);
    if (!leaf) {
        return;
    }

    // Lock the leaf node for exclusive access
    std::unique_lock leaf_lock(leaf->mutex_);

    // Find the position of the key in the leaf
    auto it = std::lower_bound(leaf->keys_.begin(), leaf->keys_.end(), key, comparator_);
    
    // Return if key doesn't exist
    if (it == leaf->keys_.end() || comparator_(key, *it) || comparator_(*it, key)) {
        return; 
    }

    // Calculate position and remove key-value pair
    size_t remove_pos = it - leaf->keys_.begin();
    leaf->keys_.erase(leaf->keys_.begin() + remove_pos);
    leaf->values_.erase(leaf->values_.begin() + remove_pos);
    --size_;

    // Handle case where root becomes empty
    if (std::holds_alternative<LeafNodePtr>(root_) && leaf->keys_.empty()) {
        root_ = std::monostate{};
        return;
    }

    // Check if the leaf needs rebalancing
    const size_t min_size = (Order - 1) / 2;
    if (leaf->keys_.size() < min_size) {
        balance_after_remove(leaf);
    }
}


/**
 * @brief Balances a node (leaf or internal) after removal
 * 
 * @param node The variant node that needs balancing
 * 
 * @details
 * This method provides a balancing algorithm that works with both, handling redistribution and merging as needed.
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::balance_after_remove(VariantNode<Key, RecordId, Order> node) {

    // Skip if node is empty
    if (std::holds_alternative<std::monostate>(node)) {
        return;
    }

    // Find parent node
    auto parent = find_parent(node);
    if (!parent) {
        return;
    }

    // Find node's index in parent's children
    size_t node_idx = 0;
    for (; node_idx < parent->children_.size(); ++node_idx) {
        if (parent->children_[node_idx] == node) {
            break;
        }
    }

    // Try to redistribute with left sibling
    if (node_idx > 0) {
        auto& left_sibling = parent->children_[node_idx - 1];
    
        if (std::holds_alternative<LeafNodePtr>(left_sibling)) {
            auto leaf = std::get<LeafNodePtr>(left_sibling);
        
            if (leaf->keys_.size() > (Order - 1) / 2) {
                redistribute_nodes(left_sibling, node);
                return;
            }
        }
    }



    // Try to redistribute with right sibling
    if (node_idx < parent->children_.size() - 1) {
        auto& right_sibling = parent->children_[node_idx + 1];
        
        if (std::holds_alternative<LeafNodePtr>(right_sibling)) {
            auto leaf = std::get<LeafNodePtr>(right_sibling);
        
            if (leaf->keys_.size() > (Order - 1) / 2) {
                redistribute_nodes(node, right_sibling);
                return;
            }
        }
    }

    // If redistribution isn't possible, merge with a sibling
    if (node_idx > 0) {
        merge_nodes(parent->children_[node_idx - 1], node);
    } else {
        merge_nodes(node, parent->children_[node_idx + 1]);
    }

    // Recursively balance the parent if needed
    if (parent->keys_.size() < (Order - 1) / 2) {
        balance_after_remove(parent);
    }
}

/**
 * @brief Redistributes keys between two adjacent nodes
 * 
 * @param lhs Left node in the redistribution
 * @param rhs Right node in the redistribution
 * 
 * @details
 * Handles the redistribution of keys between adjacent nodes
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::redistribute_nodes(
    VariantNode<Key, RecordId, Order>& lhs, 
    VariantNode<Key, RecordId, Order>& rhs) {
    
    // Handle redistribution between leaf nodes
    if (std::holds_alternative<LeafNodePtr>(lhs) && std::holds_alternative<LeafNodePtr>(rhs)) {
        auto left = std::get<LeafNodePtr>(lhs);
        auto right = std::get<LeafNodePtr>(rhs);

        // Move last key-value pair from left to right
        right->keys_.insert(right->keys_.begin(), left->keys_.back());
        right->values_.insert(right->values_.begin(), left->values_.back());

        left->keys_.pop_back();
        left->values_.pop_back();

        // Update parent's key
        auto parent = find_parent(lhs);
        if (parent) {
            size_t idx = std::find(parent->children_.begin(), 
                                 parent->children_.end(), 
                                 rhs) - parent->children_.begin();
            if (idx > 0) {
                parent->keys_[idx - 1] = right->keys_.front();
            }
        }
    }
}


/**
 * @brief Merges two adjacent nodes in the B+ tree
 * 
 * @details
 * Merges two nodes by moving all contents from the right node to the left node
 * and updating the parent node accordingly. 
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::merge_nodes(
    VariantNode<Key, RecordId, Order>& left, 
    VariantNode<Key, RecordId, Order>& right) {
    
    // Handle merging of leaf nodes
    if (std::holds_alternative<LeafNodePtr>(left) && std::holds_alternative<LeafNodePtr>(right)) {
        auto left_leaf = std::get<LeafNodePtr>(left);
        auto right_leaf = std::get<LeafNodePtr>(right);

        // Move all keys from right leaf to left leaf
        left_leaf->keys_.insert(left_leaf->keys_.end(), 
                              right_leaf->keys_.begin(), 
                              right_leaf->keys_.end());
        // Move all values from right leaf to left leaf
        left_leaf->values_.insert(left_leaf->values_.end(), 
                                right_leaf->values_.begin(), 
                                right_leaf->values_.end());

        // Update the next pointer to maintain leaf node chain
        left_leaf->next_ = right_leaf->next_;

        // Update parent node by removing the right child and its corresponding key
        auto parent = find_parent(left);
        if (parent) {
            size_t idx = std::find(parent->children_.begin(), parent->children_.end(), right) - parent->children_.begin();

            parent->children_.erase(parent->children_.begin() + idx);
            parent->keys_.erase(parent->keys_.begin() + idx - 1);
        }
    }
}

template <typename Key, typename RecordId, size_t Order, typename compare>
bool BPlusTree<Key, RecordId, Order, compare>::empty() const {
    // Tree is empty if root is monostate
    if (std::holds_alternative<std::monostate>(root_)) {
        return true;    
    }
    return false;
}

/**
 * @brief Finds and returns the record ID associated with a given key
 * 
 * @param key The key to search for
 * 
 * @details
 * Returns an empty vector if the key is not found.
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
std::vector<RecordId> BPlusTree<Key, RecordId, Order, compare>::find(const Key& key) {

    // Acquire shared lock for reading
    std::shared_lock read_lock(root_mutex_);

    // Return empty vector if tree is empty
    if (std::holds_alternative<std::monostate>(root_)) {
        return {};
    }

    // Find the leaf node containing the key
    LeafNodePtr leaf = find_leaf(key);
    if (!leaf) {
        return {};
    }

    // Acquire shared lock for leaf node
    std::shared_lock leaf_lock(leaf->mutex_);

    // Search for the key in the leaf node
    auto it = std::lower_bound(leaf->keys_.begin(), leaf->keys_.end(), key, comparator_);
    if (it != leaf->keys_.end() && !comparator_(key, *it) && !comparator_(*it, key)) {
        size_t index = it - leaf->keys_.begin();
        return {leaf->values_[index]};
    }

    // Return empty vector if key not found
    return {};
}



/**
 * @brief Performs a range search in the B+ tree
 * 
 * @param from The lower bound of the range 
 * @param to The upper bound of the range 
 * 
 * @details
 * Performs a range search by traversing leaf nodes from the lower bound
 * until reaching the upper bound or the end of the tree.
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
std::vector<RecordId> BPlusTree<Key, RecordId, Order, compare>::range_search(
    const Key& from, const Key& to) {

    // Acquire shared lock for reading
    std::shared_lock read_lock(root_mutex_);
    std::vector<RecordId> result;

    // Return empty result if tree is empty
    if (std::holds_alternative<std::monostate>(root_)) {
        return result;
    }

    // Find the leaf node containing the lower bound
    LeafNodePtr current = find_leaf(from);
    if (!current) {
        return result;
    }

    // Traverse through leaf nodes
    while (current) {
        // Lock current leaf for reading
        std::shared_lock leaf_lock(current->mutex_);

        // Find the first key greater than or equal to 'from'
        auto start_it = std::lower_bound(current->keys_.begin(), current->keys_.end(), from, comparator_);
        
        // Collect all keys in range [from, to)
        for (auto it = start_it; it != current->keys_.end(); ++it) {
            
            // Stop if we've reached the upper bound
            if (comparator_(to, *it)) {
                return result;
            }
            
            // Add matching record ID to results
            size_t index = it - current->keys_.begin();
            result.push_back(current->values_[index]);
        }

        // Move to next leaf node
        current = current->next_;
    }

    return result;
}



/**
 * @brief Finds all records whose keys satisfy a given predicate
 * 
 * @param pred Predicate function that takes a Key and returns bool
 * 
 * @return Vector of record IDs whose keys satisfy the predicate
 * 
 * @details
 * Traverses all leaf nodes and applies the predicate to each key.
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
template<typename Predicate>
std::vector<RecordId> BPlusTree<Key, RecordId, Order, compare>::find_if(Predicate pred) {
    // Acquire shared lock for reading
    std::shared_lock read_lock(root_mutex_);
    std::vector<RecordId> result;
    
    // Return empty result if tree is empty
    if (std::holds_alternative<std::monostate>(root_)) {
        return result;
    }

    // Find the leftmost leaf node
    LeafNodePtr leaf = std::holds_alternative<LeafNodePtr>(root_) ?
                      std::get<LeafNodePtr>(root_) :  // Root is leaf
                      find_leaf(std::get<InternalNodePtr>(root_)->keys_.front());  // Find leftmost leaf
                      
    // Traverse all leaf nodes
    while (leaf) {
        // Lock current leaf for reading
        std::shared_lock leaf_lock(leaf->mutex_);
        
        // Check each key against the predicate
        for (size_t i = 0; i < leaf->size(); ++i) {
            if (pred(leaf->keys_[i])) {
                result.push_back(leaf->values_[i]);
            }
        }
        
        // Move to next leaf
        leaf = leaf->next_;
    }
    
    return result;
}



/**
 * @brief Performs a prefix search in the B+ tree
 * 
 * @param prefix The string prefix to search for
 * 
 * @details
 * Performs a prefix search by finding the first potential match and then
 * scanning sequentially through leaf nodes until the prefix no longer matches.
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
std::vector<RecordId> BPlusTree<Key, RecordId, Order, compare>::prefix_search(const std::string& prefix) {

    // Acquire shared lock for reading
    std::shared_lock read_lock(root_mutex_);
    std::vector<RecordId> result;
    
    // Return empty result if tree is empty
    if (std::holds_alternative<std::monostate>(root_)) {
        return result;
    }

    // Find the leaf node where the prefix might first appear
    LeafNodePtr leaf = find_leaf(prefix);
    
    // Traverse leaf nodes while checking for prefix matches
    while (leaf) {

        // Lock current leaf for reading
        std::shared_lock leaf_lock(leaf->mutex_);
        
        // Check each key in the current leaf
        for (size_t i = 0; i < leaf->keys_.size(); ++i) {
            const auto& key = leaf->keys_[i];
        
            // If key starts with prefix, add to results
            if (key.substr(0, prefix.size()) == prefix) {
                result.push_back(leaf->values_[i]);
            } 
            // If we've passed possible matches, return results
            else if (key.substr(0, prefix.size()) > prefix) {
                return result;
            }
        }
        // Move to next leaf
        leaf = leaf->next_;
    }
    return result;
}


template <typename Key, typename RecordId, size_t Order, typename compare>
typename BPlusTree<Key, RecordId, Order, compare>::Iterator 
BPlusTree<Key, RecordId, Order, compare>::begin() {

    // Acquire shared lock for reading
    std::shared_lock read_lock(root_mutex_);

    // Return empty iterator if tree is empty
    if (std::holds_alternative<std::monostate>(root_)) {
        return Iterator();
    }

    LeafNodePtr current;
    // If root is a leaf node, use it directly
    if (std::holds_alternative<LeafNodePtr>(root_)) {
        current = std::get<LeafNodePtr>(root_);
    } 
    
    // Otherwise, traverse to leftmost leaf
    else {
        auto node = std::get<InternalNodePtr>(root_);
        // Follow leftmost child pointers until reaching a leaf
        while (!std::holds_alternative<LeafNodePtr>(node->children_.front())) {
            node = std::get<InternalNodePtr>(node->children_.front());
        }
        current = std::get<LeafNodePtr>(node->children_.front());
    }

    return Iterator(current, 0);
}


template <typename Key, typename RecordId, size_t Order, typename compare>
typename BPlusTree<Key, RecordId, Order, compare>::Iterator 
BPlusTree<Key, RecordId, Order, compare>::end() {
    return Iterator();
}



/**
 * @brief Calculates the height of the B+ tree
 * 
 * @return size_t The height of the tree (0 for empty tree, 1 for only root)
 * 
 * @details
 * Calculates tree height by traversing from root to leftmost leaf.
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
size_t BPlusTree<Key, RecordId, Order, compare>::height() const {

    // Acquire shared lock for reading
    std::shared_lock read_lock(root_mutex_);
    
    if (std::holds_alternative<std::monostate>(root_)) {
        return 0;  
    }
    
    size_t h = 1; 
    
    // If root is internal node, traverse to leaf counting levels
    if (std::holds_alternative<InternalNodePtr>(root_)) {
        auto current = std::get<InternalNodePtr>(root_);
        
        while (current) {
            h++;
            
            // Break if node has no children
            if (current->children_.empty()) {
                break;
            }
            
            // Get first child of current node
            const auto& first_child = current->children_[0];

            // Break if we've reached a leaf level
            if (std::holds_alternative<LeafNodePtr>(first_child)) {
                break;
            }
            
            // Move to next level
            current = std::get<InternalNodePtr>(first_child);
        }
    }
    
    return h;
}



/**
 * @brief Calculates the fill factor of the B+ tree
 * 
 * @return double The fill factor as a ratio between 0.0 and 1.0
 * 
 * @details
 * Calculates the ratio of used keys to total capacity across all nodes.
 * Fill factor = total keys used / total possible keys
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
double BPlusTree<Key, RecordId, Order, compare>::fill_factor() const {

    // Acquire shared lock for reading
    std::shared_lock read_lock(root_mutex_);
    
    // Return 0 if tree is empty
    if (std::holds_alternative<std::monostate>(root_)) {
        return 0.0;  
    }
    
    size_t total_capacity = 0;
    size_t total_used = 0;
    
    // Define recursive lambda function to calculate fill factor
    std::function<void(const VariantNode<Key, RecordId, Order>&)> calculate_fill =
        [&](const VariantNode<Key, RecordId, Order>& node) {
            
            if (std::holds_alternative<LeafNodePtr>(node)) {
        
                // Handle leaf node
                auto leaf = std::get<LeafNodePtr>(node);
                total_capacity += Order - 1;  // Maximum keys possible
                total_used += leaf->keys_.size();  // Current keys
            }
            else if (std::holds_alternative<InternalNodePtr>(node)) {
                // Handle internal node
                auto internal = std::get<InternalNodePtr>(node);
            
                total_capacity += Order - 1;  // Maximum keys possible
                total_used += internal->keys_.size();  // Current keys
                
                // Recursively process all children
                for (const auto& child : internal->children_) {
                    calculate_fill(child);
                }
            }
        };
    
    // Calculate fill factor starting from root
    calculate_fill(root_);
    
    // Return ratio of used space to total capacity
    return total_capacity > 0 ? static_cast<double>(total_used) / total_capacity : 0.0;
}



/**
 * @brief Copy constructor for BPlusTree.
 *
 * Creates a deep copy of the BPlusTree, including all nodes and their contents.
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
BPlusTree<Key, RecordId, Order, compare>::BPlusTree(const BPlusTree& other)
    : size_(other.size_), comparator_(other.comparator_) {

    // Acquire a shared lock on the other tree's root mutex 
    std::shared_lock read_lock(other.root_mutex_);
    
    if (std::holds_alternative<std::monostate>(other.root_)) {
        root_ = std::monostate{};
        return;
    }

    // If the other tree's root is a leaf node, create a new leaf node and copy its contents.
    if (std::holds_alternative<LeafNodePtr>(other.root_)) {
        auto other_leaf = std::get<LeafNodePtr>(other.root_);
        auto new_leaf = std::make_shared<LeafNode<Key, RecordId, Order>>();
        
        // Copy the keys and values from the other leaf node.
        new_leaf->keys_ = other_leaf->keys_;
        new_leaf->values_ = other_leaf->values_;

        // Set the next pointer to nullptr, as this is a new leaf node.
        new_leaf->next_ = nullptr;  
        
        // Set our root to the new leaf node.
        root_ = new_leaf;
    } else {
        // If the other tree's root is an internal node, recursively copy the entire subtree using the deep_copy_node function.
        root_ = deep_copy_node(std::get<InternalNodePtr>(other.root_));
    }

    // Rebuild the leaf links to ensure they are correct in the new tree.
    rebuild_leaf_links();
}



/**
 * @brief Move constructor for BPlusTree.
 *
 * Transfers ownership of the BPlusTree from the other tree to this one.
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
BPlusTree<Key, RecordId, Order, compare>::BPlusTree(BPlusTree&& other) noexcept
    : root_(std::monostate{}), size_(0), comparator_() {

    // Acquire an exclusive lock on the other tree's root mutex
    std::unique_lock write_lock(other.root_mutex_);
    
    root_ = std::move(other.root_);
    size_ = other.size_;
    comparator_ = std::move(other.comparator_);
    
    // Reset the other tree's members to their default values.
    other.root_ = std::monostate{};
    other.size_ = 0;
}



template <typename Key, typename RecordId, size_t Order, typename compare>
BPlusTree<Key, RecordId, Order, compare>& 
BPlusTree<Key, RecordId, Order, compare>::operator=(const BPlusTree& other) {
    if (this != &other) {
        BPlusTree temp(other);
        *this = std::move(temp);
    }
    return *this;
}



template <typename Key, typename RecordId, size_t Order, typename compare>
BPlusTree<Key, RecordId, Order, compare>& 
BPlusTree<Key, RecordId, Order, compare>::operator=(BPlusTree&& other) noexcept {

    if (this != &other) {
        std::unique_lock write_lock1(root_mutex_, std::defer_lock);
        std::unique_lock write_lock2(other.root_mutex_, std::defer_lock);
        std::lock(write_lock1, write_lock2);
        
        clear();
        root_ = std::move(other.root_);
        size_ = other.size_;
        comparator_ = std::move(other.comparator_);
        
        other.root_ = std::monostate{};
        other.size_ = 0;
    }
    return *this;
}


/**
 * @brief Recursively creates a deep copy of an internal node.
 *
 * Copies the keys and children of the given internal node, and recursively
 * copies any child nodes.
 *
 * @param node The internal node to copy.
 * @return A shared pointer to the new internal node.
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
typename BPlusTree<Key, RecordId, Order, compare>::InternalNodePtr
BPlusTree<Key, RecordId, Order, compare>::deep_copy_node(const InternalNodePtr& node) {

    // If the node is nullptr, return nullptr.
    if (!node) return nullptr;

    // Create a new internal node.
    auto new_node = std::make_shared<InternalNode<Key, RecordId, Order>>();
    
    // Copy the keys from the original node.
    new_node->keys_ = node->keys_;

    // Iterate over the children of the original node.
    for (const auto& child : node->children_) {
    
        // If the child is an internal node, recursively copy it.
        if (std::holds_alternative<InternalNodePtr>(child)) {
            new_node->children_.push_back(deep_copy_node(std::get<InternalNodePtr>(child)));

        // If the child is a leaf node, create a new leaf node and copy its contents.
        } else if (std::holds_alternative<LeafNodePtr>(child)) {
        
            auto leaf = std::get<LeafNodePtr>(child);
            auto new_leaf = std::make_shared<LeafNode<Key, RecordId, Order>>();
            
            // Copy the keys and values from the original leaf node.
            new_leaf->keys_ = leaf->keys_;
            new_leaf->values_ = leaf->values_;
        
            // Set the next pointer to nullptr, as this is a new leaf node.
            new_leaf->next_ = nullptr;  
            
            // Add the new leaf node to the children of the new internal node.
            new_node->children_.push_back(new_leaf);
        }
    }

    // Return the new internal node.
    return new_node;
}



/**
 * @brief Rebuilds the leaf links in the BPlusTree.
 *
 * Iterates over all leaf nodes and sets the next pointer of each node to the
 * next node in the sequence.
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::rebuild_leaf_links() {

    // If the tree is empty, return immediately.
    if (std::holds_alternative<std::monostate>(root_)) return;

    std::vector<LeafNodePtr> leaves;
    collect_leaves(root_, leaves);

    // Iterate over the leaf nodes and set the next pointer of each node.
    for (size_t i = 0; i < leaves.size() - 1; ++i) {
        leaves[i]->next_ = leaves[i + 1];
    }
}



/**
 * @brief Recursively collects all leaf nodes in the BPlusTree.
 *
 * Iterates over all nodes in the tree and adds any leaf nodes to the given vector.
 *
 * @param node The current node to process.
 */

template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::collect_leaves(
    const VariantNode<Key, RecordId, Order>& node, 
    std::vector<LeafNodePtr>& leaves) {
    
    // If the node is a leaf node, add it to the vector.
    if (std::holds_alternative<LeafNodePtr>(node)) {
        leaves.push_back(std::get<LeafNodePtr>(node));

    // If the node is an internal node, recursively process its children.
    } else if (std::holds_alternative<InternalNodePtr>(node)) {
    
        auto internal = std::get<InternalNodePtr>(node);
        for (const auto& child : internal->children_) {
            collect_leaves(child, leaves);
        }
    }
}


template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::clear() {
    std::unique_lock write_lock(root_mutex_);
    root_ = std::monostate{};
    size_ = 0;
}

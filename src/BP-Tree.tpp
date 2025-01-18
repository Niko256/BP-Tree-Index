#include "BP-Tree.hpp"
#include <algorithm>
#include <cstddef>
#include <iterator>
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


template <typename Key, typename RecordId, size_t Order, typename compare>
typename BPlusTree<Key, RecordId, Order, compare>::LeafNodePtr
BPlusTree<Key, RecordId, Order, compare>::find_leaf(const Key& key) {
    auto node = root_;

    while (!std::holds_alternative<LeafNodePtr>(node)) {
        auto internal_node = std::get<InternalNodePtr>(node);
        size_t i = 0;

        while (i < internal_node->keys_.size() && is_less_or_eq(internal_node->keys_[i]), key) {
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


template <typename Key, typename RecordId, size_t Order, typename compare>
DynamicArray<RecordId> BPlusTree<Key, RecordId, Order, compare>::find(const Key& key) {

    std::shared_lock lock(root_mutex_);
    DynamicArray<RecordId> result;

    auto leaf = find_leaf(key);
    for (size_t i = 0; i < leaf->keys_.size(); ++i) {
        if (key == leaf->keys_[i]) {
            result.push_back(leaf->values_[i]); 
        }
    }
    return result;
}


template <typename Key, typename RecordId, size_t Order, typename compare>
DynamicArray<RecordId> BPlusTree<Key, RecordId, Order, compare>::range_search(const Key& from, const Key& to) {
    
    std::shared_lock lock(root_mutex_);
    DynamicArray<RecordId> result;

    auto leaf = find_leaf(from);

    while (leaf) {
        for (size_t i = 0; i < leaf->keys_.size(); i++) {
            if (from, leaf->keys_[i] && is_less_or_eq(leaf->keys_[i], to)) {
                result.push_back(leaf->values_[i]);
            }
        }
        leaf = leaf->next_;
    }
    return result;
}


template <typename Key, typename RecordId, size_t Order, typename Compare>
void BPlusTree<Key, RecordId, Order, Compare>::insert(const Key& key, RecordId id) {
    std::unique_lock lock(root_mutex_);
    if (std::holds_alternative<std::monostate>(root_)) {
        auto new_leaf = SharedPtr<LeafNode<Key, RecordId, Order>>(new LeafNode<Key, RecordId, Order>());
        new_leaf->keys_.push_back(key);
        new_leaf->values_.push_back(id);
        root_ = new_leaf;
    } else {
        auto leaf = find_leaf(key);

        auto it = std::lower_bound(leaf->keys_.begin(), leaf->keys_.end(), key, comparator_);
        if (it != leaf->keys_.end() && *it == key) {
            throw std::runtime_error("Duplicate key insertion is not allowed");
        }

        if (leaf->is_full()) {
            if (std::holds_alternative<LeafNodePtr>(root_)) {
                auto new_root = SharedPtr<InternalNode<Key, RecordId, Order>>(new InternalNode<Key, RecordId, Order>());
                new_root->children_.push_back(root_);
                root_ = new_root;
            }
            split_child(root_, 0);
            insert(key, id);
        } else {
            leaf->keys_.push_back(key);
            leaf->values_.push_back(id);
        }
    }
    size_++;
}



template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::redistrebute_nodes(
        VariantNode<Key, RecordId, Order>& lhs, VariantNode<Key, RecordId, Order>& rhs) {
    
    if (std::holds_alternative<InternalNodePtr>(lhs) && std::holds_alternative<InternalNodePtr>(rhs)) {
        auto left_internal = std::get<InternalNodePtr>(lhs);
        auto right_internal = std::get<InternalNodePtr>(rhs);

        if (left_internal->size() < Order / 2) {
            left_internal->keys_.push_back(right_internal->keys_.front());
            left_internal->children_.push_back(right_internal->children_.front());

            right_internal->keys_.erase(right_internal->keys_.begin());
            right_internal->children_.erase(right_internal->children.begin());
        
        } else {
            right_internal->keys_.insert(right_internal->keys_.begin(), left_internal->keys_.back());
            right_internal->children_.insert(right_internal->cgildren_.begin(), left_internal->children_.back());

            left_internal->keys_.pop_back();
            left_internal->children_.pop_back();
        }

    } else if (std::holds_alternative<LeafNodePtr>(lhs) && std::holds_alternative<LeafNodePtr>(rhs)) {
        
        auto left_leaf = std::get<LeafNodePtr>(lhs);
        auto right_leaf = std::get<>(rhs);

        if (left_leaf->size() < Order / 2) {
            
            left_leaf->keys_.push_back(right_leaf->keys_.front());
            left_leaf->values_.push_back(right_leaf->values_.front());
            
            right_leaf->keys_.erase(right_leaf->keys_.begin());
            right_leaf->values_.erase(right_leaf->values_.begin());

        } else {
            right_leaf->keys_.insert(right_leaf->keys_.begin(), left_leaf->keys_.back());
            right_leaf->values_.insert(right_leaf->values_.begin(), left_leaf->values_.back());

            left_leaf->keys_.pop_back();
            left_leaf->values_.pop_back();
        }
    }
} 


template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::merge_nodes(
    VariantNode<Key, RecordId, Order>& left, VariantNode<Key, RecordId, Order>& right) {

    if (std::holds_alternative<InternalNodePtr>(left) && std::holds_alternative<InternalNodePtr>(right)) {
        auto left_internal = std::get<InternalNodePtr>(left);
        auto right_internal = std::get<InternalNodePtr>(right);

        left_internal->keys_.push_back(right_internal->keys_.front());
        left_internal->keys_.insert(left_internal->keys_.end(), right_internal->keys_.begin() + 1, right_internal->keys_.end());
        left_internal->children_.insert(left_internal->children_.end(), right_internal->children_.begin(), right_internal->children_.end());

        right_internal->keys_.clear();
        right_internal->children_.clear();

    } else if (std::holds_alternative<LeafNodePtr>(left) && std::holds_alternative<LeafNodePtr>(right)) {
        
        auto left_leaf = std::get<LeafNodePtr>(left);
        auto right_leaf = std::get<LeafNodePtr>(right);

        left_leaf->keys_.insert(left_leaf->keys_.end(), right_leaf->keys_.begin(), right_leaf->keys_.end());
        left_leaf->values_.insert(left_leaf->values_.end(), right_leaf->values_.begin(), right_leaf->values_.end());

        left_leaf->next_ = right_leaf->next_;

        right_leaf->keys_.clear();
        right_leaf->values_.clear();
    }
}

template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::remove(const Key& key) {
    
    std::unique_lock lock(root_mutex_);
    
    if (std::holds_alternative<std::monostate>(root_)) {
        return;
    }

    auto leaf = find_leaf(key);
    auto it = std::lower_bound(leaf->keys_.begin(), leaf->keys_.end(), key, comparator_);

    if (it == leaf->keys_.end() || *it != key) {
        return;
    }

    size_t index = std::distance(leaf->keys_.begin(), it);
    leaf->keys_.erase(leaf->keys_.begin() + index);
    leaf->values_.erase(leaf->values_.begin() + index);
    --size_;

    if (leaf->size() < Order / 2) {
        balance_after_remove(leaf);
    }
}


template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::balance_after_remove(LeafNodePtr node) {
    if (node == std::get<LeafNodePtr>(root_)) {
        if (node->size() == 0) {
            root_ = LeafNodePtr{};
        }
        return;
    }

    auto parent = find_parent(node);
    size_t index = 0;
    while (index < parent->children_.size() && std::get<LeafNodePtr>(parent->children_[index]) != node) {
        index++;
    }

    if (index > 0) {
        auto left_sibling = std::get<LeafNodePtr>(parent->children_[index - 1]);
        if (left_sibling->size() > Order / 2) {
            redistribute_nodes(parent->children_[index - 1], parent->children_[index]);
            return;
        }
    }

    if (index < parent->children_.size() - 1) {
        auto right_sibling = std::get<LeafNodePtr>(parent->children_[index + 1]);
        if (right_sibling->size() > Order / 2) {
            redistribute_nodes(parent->children_[index], parent->children_[index + 1]);
            return;
        }
    }

    if (index > 0) {
        merge_nodes(parent->children_[index - 1], parent->children_[index]);
    } else {
        merge_nodes(parent->children_[index], parent->children_[index + 1]);
    }

    if (parent->size() < Order / 2) {
        balance_after_remove(parent);
    }
}


template <typename Key, typename RecordId, size_t Order, typename compare>
SharedPtr<InternalNode<Key, RecordId, Order>>
BPlusTree<Key, RecordId, Order, compare>::find_parent(LeafNodePtr node) {

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



template <typename Key, typename RecordId, size_t Order, typename compare>
SharedPtr<InternalNode<Key, RecordId, Order>> BPlusTree<Key, RecordId, Order, compare>::copy_nodes(
        const SharedPtr<InternalNode<Key, RecordId, Order>>& node) {
    
    auto new_node = SharedPtr<InternalNode<Key, RecordId, Order>>(new InternalNode<Key, RecordId, Order>());
    new_node->keys_ = node->keys_;

    for (const auto& child : node->children_) {
        if (std::holds_alternative<InternalNodePtr>(child)) {
            new_node->children_.push_back(copy_nodes(std::get<InternalNodePtr>(child)));
        } else {
            new_node->children_.push_back(copy_nodes(std::get<LeafNodePtr>(child)));
        }
    }
    return new_node;
}



template <typename Key, typename RecordId, size_t Order, typename compare>
SharedPtr<InternalNode<Key, RecordId, Order>> BPlusTree<Key, RecordId, Order, compare>::copy_nodes(
        const SharedPtr<LeafNode<Key, RecordId, Order>>& node) {
    
    auto new_node = SharedPtr<LeafNode<Key, RecordId, Order>>(new LeafNode<Key, RecordId, Order>());
    
    new_node->keys_ = node->keys_;
    new_node->values_ = node-> values_;
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
        std::unique_lock lock(root_mutex_);
        std::unique_lock other_lock(other.root_mutex_);

        root_ = std::move(other.root_);
        size_ = other.size_;
        other.root_ = LeafNodePtr{};
        other.size_ = 0;
    }
    return *this;
}

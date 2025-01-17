#include "BP-Tree.hpp"
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

template <typename Key, typename RecordId, size_t Order, typename Compare>
BPlusTree<Key, RecordId, Order, Compare>::Iterator::Iterator(LeafNodePtr node, size_t index)
    : current_node_(node), current_index_(index) {}

template <typename Key, typename RecordId, size_t Order, typename Compare>
typename BPlusTree<Key, RecordId, Order, Compare>::Iterator&
BPlusTree<Key, RecordId, Order, Compare>::Iterator::operator++() {
    if (current_node_) {
        current_index_++;
        if (current_index_ >= current_node_->keys_.size()) {
            current_node_ = current_node_->next_;
            current_index_ = 0;
        }
    }
    return *this;
}

template <typename Key, typename RecordId, size_t Order, typename Compare>
Pair<const Key&, RecordId&> BPlusTree<Key, RecordId, Order, Compare>::Iterator::operator*() const {
    if (current_node_) {
        return { current_node_->keys_[current_index_], current_node_->values_[current_index_] };
    }
    throw std::out_of_range("Iterator is out of range");
}

template <typename Key, typename RecordId, size_t Order, typename Compare>
bool BPlusTree<Key, RecordId, Order, Compare>::Iterator::operator==(const Iterator& other) const {
    return current_node_ == other.current_node_ && current_index_ == other.current_index_;
}

template <typename Key, typename RecordId, size_t Order, typename Compare>
bool BPlusTree<Key, RecordId, Order, Compare>::Iterator::operator!=(const Iterator& other) const {
    return !(*this == other);
}

// ---------------- CONST ITERATOR METHODS IMPLEMENTATION ----------------

template <typename Key, typename RecordId, size_t Order, typename Compare>
typename BPlusTree<Key, RecordId, Order, Compare>::ConstIterator& 
BPlusTree<Key, RecordId, Order, Compare>::ConstIterator::operator++() {
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

template <typename Key, typename RecordId, size_t Order, typename Compare>
const Pair<const Key&, const RecordId&> 
BPlusTree<Key, RecordId, Order, Compare>::ConstIterator::operator*() const {
    if (!current_node_ || current_index_ >= current_node_->size()) {
        throw std::runtime_error("Invalid iterator dereference");
    }
    
    return Pair<const Key&, const RecordId&>(
        current_node_->keys_[current_index_],
        current_node_->get_record(current_index_)
    );
}

template <typename Key, typename RecordId, size_t Order, typename Compare>
bool BPlusTree<Key, RecordId, Order, Compare>::ConstIterator::operator==(const ConstIterator& other) const {
    return current_node_ == other.current_node_ && current_index_ == other.current_index_;
}

template <typename Key, typename RecordId, size_t Order, typename Compare>
bool BPlusTree<Key, RecordId, Order, Compare>::ConstIterator::operator!=(const ConstIterator& other) const {
    return !(*this == other);
}

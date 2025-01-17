#include "BP-Tree.hpp"
#include <stdexcept>

template <typename Key, typename RecordId>
size_t InternalNode<Key, RecordId>::size() const {
    return keys_.size();
}


template <typename Key, typename RecordId>
bool InternalNode<Key, RecordId>::is_full() const {
    return keys_.size() >= BPlusTree<Key, RecordId>::Order - 1;
}

template <typename Key, typename RecordId>
void InternalNode<Key, RecordId>::insert_key_at(size_t index, const Key& key) {
    keys_.insert(keys_.begin() + index, key);
}


template <typename Key, typename RecordId>
void InternalNode<Key, RecordId>::insert_key_at(size_t index, Key&& key) {
    keys_.insert(keys_.begin() + index, std::move(key));
}


template <typename Key, typename RecordId>
size_t LeafNode<Key, RecordId>::size() const {
    return keys_.size();
}

template <typename Key, typename RecordId>
bool LeafNode<Key, RecordId>::is_full() const {
    return keys_.size() >= BPlusTree<Key, RecordId>::Order - 1;
}



template <typename Key, typename RecordId, size_t Order, typename Compare>
BPlusTree<Key, RecordId, Order, Compare>::Iterator::Iterator(LeafNodePtr node, size_t index)
    : current_node_(node), current_index_(index) {}


template <typename Key. typename RecordId>
typename BPlusTree<Key, RecordId>::Iterator&
BPlusTree<Key, RecordId>::Iterator::operator++() {
    if (current_node_) {
        current_index_++;

        if (current_index_ >= current_node_.keys_.size()) {
            current_node_ = current_node_->next_;
            current_index_ = 0;
        }
    }
    return *this;
}


template <typename Key, typename RecordId, size_t Order, typename Compare>
Pair<const Key&, RecordId> BPlusTree<Key, RecordId, Order, Compare>::Iterator::operator*() const {
    if (current_node_) {
        return { current_node_->keys_[current_index_], current_node_->values_[current_index_] };
    }
    throw std::out_of_range("Iterator is out of range");
}


template <typename Key, typename RecordId>
bool BPlusTree<Key, RecordId, Order, Compare>::Iterator::operator==(const Iterator& other) {
    return current_node_ == other.current_node_ && current_index_ == other.current_index_;
}



// -------------------------------------------------------
// BP-TREE IMPL



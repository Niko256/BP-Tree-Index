#include "BP-Tree.hpp"

// ---------------- NODE METHODS IMPLEMENTATION ---------------- 

template <typename Key, typename RecordId, size_t Order>
size_t BPlusTree<Key, RecordId, Order>::Node::size() const {
    return keys_.size();
}

template <typename Key, typename RecordId, size_t Order>
SharedPtr<typename BPlusTree<Key, RecordId, Order>::Node>
BPlusTree<Key, RecordId, Order>::Node::get_child(size_t index) {
    if (index >= children_.size()) {
        throw std::out_of_range("Index out of range in get_child method");
    }
    return std::get<SharedPtr<Node>>(children_[index]);
}

template <typename Key, typename RecordId, size_t Order>
RecordId BPlusTree<Key, RecordId, Order>::Node::get_record(size_t index) {
    if (index >= children_.size()) {
        throw std::out_of_range("Index out of range in get_record method");
    }
    return std::get<RecordId>(children_[index]);
}


template <typename Key, typename RecordId, size_t Order>
bool BPlusTree<Key, RecordId, Order>::Node::is_full() const {
    return keys_.size() >= Order - 1;
}


template <typename Key, typename RecordId, size_t Order>
bool BPlusTree<Key, RecordId, Order>::Node::has_min_keys() const noexcept {
    if (is_root_) {
        return !is_leaf_ ? children_.size() >= 2 : keys_.size() >= 1;
    }
    return keys_.size() >= (Order + 1) / 2 - 1;
}


template <typename Key, typename RecordId, size_t Order>
void BPlusTree<Key, RecordId, Order>::Node::insert_key_at(size_t index, const Key& key) {
    keys_.insert(keys_.begin() + index, key);
}


template <typename Key, typename RecordId, size_t Order>
void BPlusTree<Key, RecordId, Order>::Node::insert_key_at(size_t index, Key&& key) {
    keys_.insert(keys_.begin() + index, std::move(key));
}


template <typename Key, typename RecordId, size_t Order>
void BPlusTree<Key, RecordId, Order>::Node::remove_key_at(size_t index) {
    keys_.erase(keys_.begin() + index);
}


template <typename Key, typename RecordId, size_t Order>
void BPlusTree<Key, RecordId, Order>::Node::remove_child_at(size_t index) {
    children_.erase(children_.begin() + index);    
}


template <typename Key, typename RecordId, size_t Order>
void BPlusTree<Key, RecordId, Order>::Node::insert_child_at(size_t index, const std::variant<SharedPtr<Node>, RecordId>& child) {
    children_.insert(children_.begin() + index, child);
}


// ---------------- ITERATOR METHODS IMPLEMENTATION ----------------

template <typename Key, typename RecordId, size_t Order>
typename BPlusTree<Key, RecordId, Order>::Iterator& 
BPlusTree<Key, RecordId, Order>::Iterator::operator++() {
    if (!current_node_) {
        return *this;
    }

    current_index_++;
    
    if (current_index_ >= current_node_->size()) {
        current_node_ = current_node_->next_leaf_;
        current_index_ = 0;
    }
    
    return *this;
}

template <typename Key, typename RecordId, size_t Order>
Pair<const Key&, RecordId> 
BPlusTree<Key, RecordId, Order>::Iterator::operator*() const {
    if (!current_node_ || current_index_ >= current_node_->size()) {
        throw std::runtime_error("Invalid iterator dereference");
    }

    return Pair<const Key&, RecordId>(
        current_node_->keys_[current_index_],
        current_node_->get_record(current_index_)
    );
}

template <typename Key, typename RecordId, size_t Order>
bool BPlusTree<Key, RecordId, Order>::Iterator::operator==(const Iterator& other) const {
    return current_node_ == other.current_node_ && current_index_ == other.current_index_;
}

template <typename Key, typename RecordId, size_t Order>
bool BPlusTree<Key, RecordId, Order>::Iterator::operator!=(const Iterator& other) const {
    return !(*this == other);
}

// ---------------- CONST ITERATOR METHODS IMPLEMENTATION ----------------

template <typename Key, typename RecordId, size_t Order>
typename BPlusTree<Key, RecordId, Order>::ConstIterator& 
BPlusTree<Key, RecordId, Order>::ConstIterator::operator++() {
    if (!current_node_) {
        return *this;
    }

    current_index_++;
    
    if (current_index_ >= current_node_->size()) {
        current_node_ = current_node_->next_leaf_;
        current_index_ = 0;
    }
    
    return *this;
}

template <typename Key, typename RecordId, size_t Order>
const Pair<const Key&, const RecordId&> 
BPlusTree<Key, RecordId, Order>::ConstIterator::operator*() const {
    if (!current_node_ || current_index_ >= current_node_->size()) {
        throw std::runtime_error("Invalid iterator dereference");
    }
    
    return Pair<const Key&, const RecordId&>(
        current_node_->keys_[current_index_],
        current_node_->get_record(current_index_)
    );
}

template <typename Key, typename RecordId, size_t Order>
bool BPlusTree<Key, RecordId, Order>::ConstIterator::operator==(const ConstIterator& other) const {
    return current_node_ == other.current_node_ && current_index_ == other.current_index_;
}

template <typename Key, typename RecordId, size_t Order>
bool BPlusTree<Key, RecordId, Order>::ConstIterator::operator!=(const ConstIterator& other) const {
    return !(*this == other);
}

// ---------------- B+TREE METHODS IMPLEMENTATION ----------------

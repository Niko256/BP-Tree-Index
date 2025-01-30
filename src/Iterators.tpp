#include "BP-Tree.hpp"



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


// ---------------- FILTER ITERATOR IMPLEMENTATION ----------------

template <typename Key, typename RecordId, size_t Order, typename compare>
template <typename Predicate>
void BPlusTree<Key, RecordId, Order, compare>::FilterIterator<Predicate>::find_next_valid() {
    while (current_ != end_ && !pred_(*current_)) {
        ++current_;
    }
}

template <typename Key, typename RecordId, size_t Order, typename compare>
template <typename Predicate>
BPlusTree<Key, RecordId, Order, compare>::FilterIterator<Predicate>::FilterIterator(
    Iterator begin, Iterator end, Predicate pred)
    : current_(begin), end_(end), pred_(pred) {
    find_next_valid();
}

template <typename Key, typename RecordId, size_t Order, typename compare>
template <typename Predicate>
typename BPlusTree<Key, RecordId, Order, compare>::template FilterIterator<Predicate>& 
BPlusTree<Key, RecordId, Order, compare>::FilterIterator<Predicate>::operator++() {
    if (current_ != end_) {
        ++current_;
        find_next_valid();
    }
    return *this;
}

template <typename Key, typename RecordId, size_t Order, typename compare>
template <typename Predicate>
auto BPlusTree<Key, RecordId, Order, compare>::FilterIterator<Predicate>::operator*() {
    return *current_;
}

template <typename Key, typename RecordId, size_t Order, typename compare>
template <typename Predicate>
bool BPlusTree<Key, RecordId, Order, compare>::FilterIterator<Predicate>::operator==(
    const FilterIterator& other) const {
    return current_ == other.current_;
}

template <typename Key, typename RecordId, size_t Order, typename compare>
template <typename Predicate>
bool BPlusTree<Key, RecordId, Order, compare>::FilterIterator<Predicate>::operator!=(
    const FilterIterator& other) const {
    return !(*this == other);
}

// TreeRange implementation
template <typename Key, typename RecordId, size_t Order, typename compare>
typename BPlusTree<Key, RecordId, Order, compare>::Iterator 
BPlusTree<Key, RecordId, Order, compare>::TreeRange::begin() {
    return tree_.begin();
}

template <typename Key, typename RecordId, size_t Order, typename compare>
typename BPlusTree<Key, RecordId, Order, compare>::Iterator 
BPlusTree<Key, RecordId, Order, compare>::TreeRange::end() {
    return tree_.end();
}

// FilterRange implementation
template <typename Key, typename RecordId, size_t Order, typename compare>
template <typename Predicate>
BPlusTree<Key, RecordId, Order, compare>::FilterRange<Predicate>::FilterRange(
    BPlusTree& tree, Predicate pred)
    : tree_(tree), pred_(pred) {}

template <typename Key, typename RecordId, size_t Order, typename compare>
template <typename Predicate>
typename BPlusTree<Key, RecordId, Order, compare>::template FilterIterator<Predicate>
BPlusTree<Key, RecordId, Order, compare>::FilterRange<Predicate>::begin() {
    return FilterIterator<Predicate>(tree_.begin(), tree_.end(), pred_);
}

template <typename Key, typename RecordId, size_t Order, typename compare>
template <typename Predicate>
typename BPlusTree<Key, RecordId, Order, compare>::template FilterIterator<Predicate>
BPlusTree<Key, RecordId, Order, compare>::FilterRange<Predicate>::end() {
    return FilterIterator<Predicate>(tree_.end(), tree_.end(), pred_);
}

// Range-based for support methods
template <typename Key, typename RecordId, size_t Order, typename compare>
typename BPlusTree<Key, RecordId, Order, compare>::TreeRange 
BPlusTree<Key, RecordId, Order, compare>::range() {
    return TreeRange(*this);
}

template <typename Key, typename RecordId, size_t Order, typename compare>
BPlusTree<Key, RecordId, Order, compare>::operator TreeRange() {
    return range();
}

template <typename Key, typename RecordId, size_t Order, typename compare>
template <typename Predicate>
typename BPlusTree<Key, RecordId, Order, compare>::template FilterRange<Predicate>
BPlusTree<Key, RecordId, Order, compare>::filter(Predicate pred) {
    return FilterRange<Predicate>(*this, pred);
}

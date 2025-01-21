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




template <typename Key, typename RecordId, size_t Order, typename compare>
typename BPlusTree<Key, RecordId, Order, compare>::LeafNodePtr 
BPlusTree<Key, RecordId, Order, compare>::find_leaf(const Key& key) {
    if (std::holds_alternative<std::monostate>(root_)) {
        return nullptr;
    }
    
    if (std::holds_alternative<LeafNodePtr>(root_)) {
        return std::get<LeafNodePtr>(root_);
    }

    auto current = std::get<InternalNodePtr>(root_);
    while (true) {
        auto it = std::lower_bound(current->keys_.begin(),
                                 current->keys_.end(),
                                 key,
                                 comparator_);
        size_t index = it - current->keys_.begin();
        
        if (index == current->keys_.size()) {
            index--;
        }
        
        auto& child = current->children_[index];
        
        if (std::holds_alternative<LeafNodePtr>(child)) {
            return std::get<LeafNodePtr>(child);
        }
        current = std::get<InternalNodePtr>(child);
    }
}


template <typename Key, typename RecordId, size_t Order, typename compare>
bool BPlusTree<Key, RecordId, Order, compare>::is_less_or_eq(const Key& key1, const Key& key2) const {
    return !comparator_(key2, key1);
} 



template <typename Key, typename RecordId, size_t Order, typename compare>
template <typename T>
void BPlusTree<Key, RecordId, Order, compare>::insert(const Key& key, T&& id) {
    std::unique_lock<std::shared_mutex> write_lock(root_mutex_);

    if (std::holds_alternative<std::monostate>(root_)) {
        auto new_leaf = std::make_shared<LeafNode<Key, RecordId, Order>>();
        new_leaf->keys_.push_back(key);
        new_leaf->values_.push_back(std::forward<T>(id));
        root_ = new_leaf;
        size_++;
        return;
    }

    LeafNodePtr leaf = find_leaf(key);
    if (!leaf) {
        throw std::runtime_error("Failed to find leaf node");
    }

    std::unique_lock<std::shared_mutex> leaf_lock(leaf->mutex_);

    auto it = std::lower_bound(leaf->keys_.begin(), 
                             leaf->keys_.end(), 
                             key, 
                             comparator_);
    size_t insert_pos = it - leaf->keys_.begin();

    if (it != leaf->keys_.end() && !comparator_(key, *it) && !comparator_(*it, key)) {
        throw std::runtime_error("Duplicate key");
    }

    leaf->keys_.insert(leaf->keys_.begin() + insert_pos, key);
    leaf->values_.insert(leaf->values_.begin() + insert_pos, std::forward<T>(id));
    size_++;

    if (leaf->size() >= Order) {
        split_leaf(leaf);
    }
}


template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::split_leaf(LeafNodePtr leaf) {

    auto new_leaf = std::make_shared<LeafNode<Key, RecordId, Order>>();
    
    size_t mid = leaf->keys_.size() / 2;
    
    new_leaf->keys_.assign(leaf->keys_.begin() + mid, leaf->keys_.end());
    new_leaf->values_.assign(leaf->values_.begin() + mid, leaf->values_.end());
    
    leaf->keys_.resize(mid);
    leaf->values_.resize(mid);
    
    new_leaf->next_ = leaf->next_;
    leaf->next_ = new_leaf;

    if (std::holds_alternative<LeafNodePtr>(root_) && 
        std::get<LeafNodePtr>(root_) == leaf) {
        auto new_root = std::make_shared<InternalNode<Key, RecordId, Order>>();
        new_root->keys_.push_back(new_leaf->keys_.front());
        new_root->children_.push_back(leaf);
        new_root->children_.push_back(new_leaf);
        root_ = new_root;
    } else {
        auto parent = find_parent(leaf);
        if (parent) {
            size_t insert_pos = std::lower_bound(parent->keys_.begin(), 
                                               parent->keys_.end(), 
                                               new_leaf->keys_.front(), 
                                               comparator_) - parent->keys_.begin();
            
            parent->keys_.insert(parent->keys_.begin() + insert_pos, 
                               new_leaf->keys_.front());
            parent->children_.insert(parent->children_.begin() + insert_pos + 1, 
                                   new_leaf);

            if (parent->is_full()) {
                split_internal(parent);
            }
        }
    }
}

template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::split_internal(InternalNodePtr node) {
    auto new_node = std::make_shared<InternalNode<Key, RecordId, Order>>();
    
    size_t mid = node->keys_.size() / 2;
    Key mid_key = node->keys_[mid];
    
    new_node->keys_.assign(node->keys_.begin() + mid + 1, node->keys_.end());
    new_node->children_.assign(node->children_.begin() + mid + 1, node->children_.end());
    
    node->keys_.resize(mid);
    node->children_.resize(mid + 1);

    if (std::holds_alternative<InternalNodePtr>(root_) && 
        std::get<InternalNodePtr>(root_) == node) {
        auto new_root = std::make_shared<InternalNode<Key, RecordId, Order>>();
        new_root->keys_.push_back(mid_key);
        new_root->children_.push_back(node);
        new_root->children_.push_back(new_node);
        root_ = new_root;
    } else {
        auto parent = find_parent(node);
        if (parent) {
            size_t insert_pos = std::lower_bound(parent->keys_.begin(), 
                                               parent->keys_.end(), 
                                               mid_key, 
                                               comparator_) - parent->keys_.begin();
            
            parent->keys_.insert(parent->keys_.begin() + insert_pos, mid_key);
            parent->children_.insert(parent->children_.begin() + insert_pos + 1, 
                                   new_node);

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


template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::remove(const Key& key) {
    std::unique_lock write_lock(root_mutex_);

    if (std::holds_alternative<std::monostate>(root_)) {
        return;
    }

    LeafNodePtr leaf = find_leaf(key);
    if (!leaf) {
        return;
    }

    std::unique_lock leaf_lock(leaf->mutex_);

    auto it = std::lower_bound(leaf->keys_.begin(), leaf->keys_.end(), key, comparator_);
    if (it == leaf->keys_.end() || comparator_(key, *it) || comparator_(*it, key)) {
        return; 
    }

    size_t remove_pos = it - leaf->keys_.begin();

    leaf->keys_.erase(leaf->keys_.begin() + remove_pos);
    leaf->values_.erase(leaf->values_.begin() + remove_pos);
    size_--;

    if (std::holds_alternative<LeafNodePtr>(root_) && leaf->keys_.empty()) {
        root_ = std::monostate{};
        return;
    }

    const size_t min_size = (Order - 1) / 2;
    if (leaf->keys_.size() < min_size) {
        balance_after_remove(leaf);
    }
}

template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::balance_after_remove(VariantNode<Key, RecordId, Order> node) {
    if (std::holds_alternative<std::monostate>(node)) {
        return;
    }

    auto parent = find_parent(node);
    if (!parent) {
        return;
    }

    size_t node_idx = 0;
    for (; node_idx < parent->children_.size(); ++node_idx) {
        if (parent->children_[node_idx] == node) {
            break;
        }
    }

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

    if (node_idx > 0) {
        merge_nodes(parent->children_[node_idx - 1], node);
    } else {
        merge_nodes(node, parent->children_[node_idx + 1]);
    }

    if (parent->keys_.size() < (Order - 1) / 2) {
        balance_after_remove(parent);
    }
}


template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::redistribute_nodes(

    VariantNode<Key, RecordId, Order>& lhs, 
    VariantNode<Key, RecordId, Order>& rhs) {
    
    if (std::holds_alternative<LeafNodePtr>(lhs) && std::holds_alternative<LeafNodePtr>(rhs)) {
        auto left = std::get<LeafNodePtr>(lhs);
        auto right = std::get<LeafNodePtr>(rhs);

        right->keys_.insert(right->keys_.begin(), left->keys_.back());
        right->values_.insert(right->values_.begin(), left->values_.back());
        left->keys_.pop_back();
        left->values_.pop_back();

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


template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::merge_nodes(
    
    VariantNode<Key, RecordId, Order>& left, 
    VariantNode<Key, RecordId, Order>& right) {
    
    if (std::holds_alternative<LeafNodePtr>(left) && std::holds_alternative<LeafNodePtr>(right)) {
        auto left_leaf = std::get<LeafNodePtr>(left);
        auto right_leaf = std::get<LeafNodePtr>(right);

        left_leaf->keys_.insert(left_leaf->keys_.end(), 
                              right_leaf->keys_.begin(), 
                              right_leaf->keys_.end());
        left_leaf->values_.insert(left_leaf->values_.end(), 
                                right_leaf->values_.begin(), 
                                right_leaf->values_.end());

        left_leaf->next_ = right_leaf->next_;

        auto parent = find_parent(left);
        if (parent) {
            size_t idx = std::find(parent->children_.begin(), 
                                 parent->children_.end(), 
                                 right) - parent->children_.begin();
            parent->children_.erase(parent->children_.begin() + idx);
            parent->keys_.erase(parent->keys_.begin() + idx - 1);
        }
    }
}




template <typename Key, typename RecordId, size_t Order, typename compare>
bool BPlusTree<Key, RecordId, Order, compare>::empty() const {
    if (std::holds_alternative<std::monostate>(root_)) {
        return true;    
    }
    return false;
}



template <typename Key, typename RecordId, size_t Order, typename compare>
std::vector<RecordId> BPlusTree<Key, RecordId, Order, compare>::find(const Key& key) {
    std::shared_lock read_lock(root_mutex_);

    if (std::holds_alternative<std::monostate>(root_)) {
        return {};
    }

    LeafNodePtr leaf = find_leaf(key);
    if (!leaf) {
        return {};
    }

    std::shared_lock leaf_lock(leaf->mutex_);

    auto it = std::lower_bound(leaf->keys_.begin(), leaf->keys_.end(), key, comparator_);
    if (it != leaf->keys_.end() && !comparator_(key, *it) && !comparator_(*it, key)) {
        size_t index = it - leaf->keys_.begin();
        return {leaf->values_[index]};
    }

    return {};
}



template <typename Key, typename RecordId, size_t Order, typename compare>
std::vector<RecordId> BPlusTree<Key, RecordId, Order, compare>::range_search(
    const Key& from, const Key& to) {
    std::shared_lock read_lock(root_mutex_);
    std::vector<RecordId> result;

    if (std::holds_alternative<std::monostate>(root_)) {
        return result;
    }

    LeafNodePtr current = find_leaf(from);
    if (!current) {
        return result;
    }

    while (current) {
        std::shared_lock leaf_lock(current->mutex_);

        auto start_it = std::lower_bound(current->keys_.begin(), 
                                       current->keys_.end(), 
                                       from, 
                                       comparator_);
        
        for (auto it = start_it; it != current->keys_.end(); ++it) {
            if (comparator_(to, *it)) {
                return result;
            }
            
            size_t index = it - current->keys_.begin();
            result.push_back(current->values_[index]);
        }

        current = current->next_;
    }

    return result;
}

template <typename Key, typename RecordId, size_t Order, typename compare>
template<typename Predicate>
std::vector<RecordId> BPlusTree<Key, RecordId, Order, compare>::find_if(Predicate pred) {
    std::shared_lock read_lock(root_mutex_);
    std::vector<RecordId> result;
    
    if (std::holds_alternative<std::monostate>(root_)) {
        return result;
    }

    LeafNodePtr leaf = std::holds_alternative<LeafNodePtr>(root_) ?
                      std::get<LeafNodePtr>(root_) :
                      find_leaf(std::get<InternalNodePtr>(root_)->keys_.front());
                      
    while (leaf) {
        std::shared_lock leaf_lock(leaf->mutex_);
        
        for (size_t i = 0; i < leaf->size(); ++i) {
            if (pred(leaf->keys_[i])) {
                result.push_back(leaf->values_[i]);
            }
        }
        
        leaf = leaf->next_;
    }
    
    return result;
}


template <typename Key, typename RecordId, size_t Order, typename compare>
std::vector<RecordId> BPlusTree<Key, RecordId, Order, compare>::prefix_search(const std::string& prefix) {

    std::shared_lock read_lock(root_mutex_);
    std::vector<RecordId> result;
    
    if (std::holds_alternative<std::monostate>(root_)) {
        return result;
    }

    LeafNodePtr leaf = find_leaf(prefix);
    
    while (leaf) {
        std::shared_lock leaf_lock(leaf->mutex_);
        
        for (size_t i = 0; i < leaf->keys_.size(); ++i) {
            const auto& key = leaf->keys_[i];
            if (key.substr(0, prefix.size()) == prefix) {
                result.push_back(leaf->values_[i]);
            } else if (key.substr(0, prefix.size()) > prefix) {
                return result;
            }
        }
        leaf = leaf->next_;
    }
    return result;
}



template <typename Key, typename RecordId, size_t Order, typename compare>
typename BPlusTree<Key, RecordId, Order, compare>::Iterator 
BPlusTree<Key, RecordId, Order, compare>::begin() {
    std::shared_lock read_lock(root_mutex_);

    if (std::holds_alternative<std::monostate>(root_)) {
        return Iterator();
    }

    LeafNodePtr current;
    if (std::holds_alternative<LeafNodePtr>(root_)) {
        current = std::get<LeafNodePtr>(root_);
    } else {
        auto node = std::get<InternalNodePtr>(root_);
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



template <typename Key, typename RecordId, size_t Order, typename compare>
size_t BPlusTree<Key, RecordId, Order, compare>::height() const {
    std::shared_lock read_lock(root_mutex_);
    
    if (std::holds_alternative<std::monostate>(root_)) {
        return 0;  
    }
    
    size_t h = 1; 
    
    if (std::holds_alternative<InternalNodePtr>(root_)) {
        auto current = std::get<InternalNodePtr>(root_);
        
        while (current) {
            h++;
            if (current->children_.empty()) {
                break;
            }
            
            const auto& first_child = current->children_[0];
            if (std::holds_alternative<LeafNodePtr>(first_child)) {
                break;
            }
            
            current = std::get<InternalNodePtr>(first_child);
        }
    }
    
    return h;
}

template <typename Key, typename RecordId, size_t Order, typename compare>
double BPlusTree<Key, RecordId, Order, compare>::fill_factor() const {
    std::shared_lock read_lock(root_mutex_);
    
    if (std::holds_alternative<std::monostate>(root_)) {
        return 0.0;  
    }
    
    size_t total_capacity = 0;
    size_t total_used = 0;
    
    std::function<void(const VariantNode<Key, RecordId, Order>&)> calculate_fill =
        [&](const VariantNode<Key, RecordId, Order>& node) {
            if (std::holds_alternative<LeafNodePtr>(node)) {
                auto leaf = std::get<LeafNodePtr>(node);
                total_capacity += Order - 1;  
                total_used += leaf->keys_.size();
            }
            else if (std::holds_alternative<InternalNodePtr>(node)) {
                auto internal = std::get<InternalNodePtr>(node);
                total_capacity += Order - 1; 
                total_used += internal->keys_.size();
                
                for (const auto& child : internal->children_) {
                    calculate_fill(child);
                }
            }
        };
    
    calculate_fill(root_);
    
    return total_capacity > 0 ? static_cast<double>(total_used) / total_capacity : 0.0;
}



template <typename Key, typename RecordId, size_t Order, typename compare>
BPlusTree<Key, RecordId, Order, compare>::BPlusTree(const BPlusTree& other) 
    : size_(other.size_), comparator_(other.comparator_) {

    std::shared_lock read_lock(other.root_mutex_);
    
    if (std::holds_alternative<std::monostate>(other.root_)) {
        root_ = std::monostate{};
        return;
    }

    if (std::holds_alternative<LeafNodePtr>(other.root_)) {
        auto other_leaf = std::get<LeafNodePtr>(other.root_);
        auto new_leaf = std::make_shared<LeafNode<Key, RecordId, Order>>();
        
        new_leaf->keys_ = other_leaf->keys_;
        new_leaf->values_ = other_leaf->values_;
        new_leaf->next_ = nullptr;  
        
        root_ = new_leaf;
    } else {
        root_ = deep_copy_node(std::get<InternalNodePtr>(other.root_));
    }

    rebuild_leaf_links();
}



template <typename Key, typename RecordId, size_t Order, typename compare>
BPlusTree<Key, RecordId, Order, compare>::BPlusTree(BPlusTree&& other) noexcept
    : root_(std::monostate{}), size_(0), comparator_() {
    std::unique_lock write_lock(other.root_mutex_);
    
    root_ = std::move(other.root_);
    size_ = other.size_;
    comparator_ = std::move(other.comparator_);
    
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


template <typename Key, typename RecordId, size_t Order, typename compare>
typename BPlusTree<Key, RecordId, Order, compare>::InternalNodePtr
BPlusTree<Key, RecordId, Order, compare>::deep_copy_node(const InternalNodePtr& node) {
    if (!node) return nullptr;

    auto new_node = std::make_shared<InternalNode<Key, RecordId, Order>>();
    new_node->keys_ = node->keys_;

    for (const auto& child : node->children_) {
        if (std::holds_alternative<InternalNodePtr>(child)) {
            new_node->children_.push_back(deep_copy_node(std::get<InternalNodePtr>(child)));

        } else if (std::holds_alternative<LeafNodePtr>(child)) {
        
            auto leaf = std::get<LeafNodePtr>(child);
            auto new_leaf = std::make_shared<LeafNode<Key, RecordId, Order>>();
            
            new_leaf->keys_ = leaf->keys_;
            new_leaf->values_ = leaf->values_;
            new_leaf->next_ = nullptr;  
            
            new_node->children_.push_back(new_leaf);
        }
    }

    return new_node;
}

template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::rebuild_leaf_links() {
    if (std::holds_alternative<std::monostate>(root_)) return;

    std::vector<LeafNodePtr> leaves;
    collect_leaves(root_, leaves);

    for (size_t i = 0; i < leaves.size() - 1; ++i) {
        leaves[i]->next_ = leaves[i + 1];
    }
}

template <typename Key, typename RecordId, size_t Order, typename compare>
void BPlusTree<Key, RecordId, Order, compare>::collect_leaves(
    const VariantNode<Key, RecordId, Order>& node,
    std::vector<LeafNodePtr>& leaves) {
    
    if (std::holds_alternative<LeafNodePtr>(node)) {
        leaves.push_back(std::get<LeafNodePtr>(node));

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

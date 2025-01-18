#pragma once

#include "../../Data_Structures/Containers/Dynamic_Array.hpp"
#include "../../Data_Structures/Containers/Pair.hpp"
#include "../../Data_Structures/SmartPtrs/include/SharedPtr.hpp"
#include <functional>
#include <iterator>
#include <shared_mutex>
#include <tuple>
#include <variant>


template <typename... Keys>
struct CompositeKey {
    std::tuple<Keys...> parameters_;

    CompositeKey(Keys... keys) : parameters_(std::make_tuple(keys...)) {}

    template <size_t I>
    const auto& get() const; 

    template <size_t N>
    bool matches_prefix(const CompositeKey& other) const;

    bool operator<(const CompositeKey& other) const;

    bool operator==(const CompositeKey& other) const; 

    // Example:
    //      using EmployeeKey = CompositeKey<std::string, int>; // name, age
    //      BPlusTree<EmployeeKey, RecordId> tree;
};

// forward declaration
template <typename Key, typename RecordId, size_t Order>
struct InternalNode;

template <typename Key, typename RecordId, size_t Order>
struct LeafNode;


// CRTP
template <typename Derived> struct BaseNode {
    bool is_leaf() const {
        return static_cast<const Derived*>(this)->is_leaf_impl();
    }
}; 


template <typename Key, typename RecordId, size_t Order>
using VariantNode = std::variant<
    SharedPtr<InternalNode<Key, RecordId, Order>>, 
    SharedPtr<LeafNode<Key, RecordId, Order>>, 
    std::monostate>;


template <typename Key, typename RecordId, size_t Order>
struct InternalNode : public BaseNode<InternalNode<Key, RecordId, Order>> {

    mutable std::shared_mutex mutex_;

    DynamicArray<Key> keys_;
    DynamicArray<VariantNode<Key, RecordId, Order>> children_;
       
    bool is_leaf_impl() const noexcept { return false; }

    size_t size() const; 
    bool is_full() const;

    void insert_key_at(size_t index, const Key& key);
    void insert_key_at(size_t index, Key&& key);

};

template <typename Key, typename RecordId, size_t Order>
struct LeafNode : public BaseNode<LeafNode<Key, RecordId, Order>> {
    
    mutable std::shared_mutex mutex_;

    DynamicArray<Key> keys_;
    DynamicArray<RecordId> values_;
        
    SharedPtr<LeafNode> next_;

    bool is_leaf_impl() const noexcept { return true; }

    size_t size() const; 
    bool is_full() const;

    RecordId get_record(size_t index) const;

};



template <typename Key, typename RecordId, size_t Order = 128, typename compare = std::less<Key>>
class BPlusTree {
  
  private:

// ------------------------------------
    
    using InternalNodePtr = SharedPtr<InternalNode<Key, RecordId, Order>>;
    using LeafNodePtr = SharedPtr<LeafNode<Key, RecordId, Order>>;

    mutable std::shared_mutex root_mutex_;
    VariantNode<Key, RecordId, Order> root_;
    
    size_t size_ = 0;

    compare comparator_;

// -------------------------------------

    void split_child(VariantNode<Key, RecordId, Order>& parent, size_t child_index);

    LeafNodePtr find_leaf(const Key& key);

    bool is_less_or_eq(const Key& key1, const Key& key2) const;

    void redistrebute_nodes(VariantNode<Key, RecordId, Order>& lhs, VariantNode<Key, RecordId, Order>& rhs);

    void merge_nodes(VariantNode<Key, RecordId, Order>& left, VariantNode<Key, RecordId, Order>& right);

    void balance_after_remove(LeafNodePtr node);

    SharedPtr<InternalNode<Key, RecordId, Order>> find_parent(LeafNodePtr node);

    SharedPtr<InternalNode<Key, RecordId, Order>> copy_nodes(const SharedPtr<InternalNode<Key, RecordId, Order>>& node);

    SharedPtr<InternalNode<Key, RecordId, Order>> copy_nodes(const SharedPtr<LeafNode<Key, RecordId, Order>>& node);

  public:
    
    class Iterator {
      private:
      
        LeafNodePtr current_node_;
        
        size_t current_index_;

        using value_type = Pair<const Key&, RecordId>;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::forward_iterator_tag;

      public:

        Iterator(LeafNodePtr node = nullptr, size_t index = 0);
        
        Iterator& operator++();
        
        Pair<const Key&, RecordId&> operator*() const;

        bool operator==(const Iterator& other) const;
        bool operator!=(const Iterator& other) const;
    };

    class ConstIterator {
      private:
        
        LeafNodePtr current_node_;

        size_t current_index_;

        using value_type = Pair<const Key&, const RecordId&>;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::forward_iterator_tag;

      public:


        ConstIterator(LeafNodePtr node = nullptr, size_t index = 0)
            : current_node_(node), current_index_(index) {}

        ConstIterator& operator++();

        const Pair<const Key&, const RecordId&> operator*() const;

        bool operator==(const ConstIterator& other) const;
        bool operator!=(const ConstIterator& other) const;

    };

    BPlusTree() : root_(LeafNodePtr{}) {}

    BPlusTree(const BPlusTree& other);

    BPlusTree(BPlusTree&& other) noexcept;

    BPlusTree& operator=(const BPlusTree& other);

    BPlusTree& operator=(BPlusTree&& other) noexcept;

    ~BPlusTree() = default;

    void insert(const Key& key, RecordId& id);

    void remove(const Key& key);

    template <typename InputIt>
    void bulk_load(InputIt first, InputIt last);

    DynamicArray<RecordId> find(const Key& key);
    
    DynamicArray<RecordId> range_search(const Key& from, const Key& to);

    Iterator begin();
    Iterator end();

    bool empty() const;

    void clear();
};

#include "Composite-Key.tpp"
#include "BP-Tree.tpp"

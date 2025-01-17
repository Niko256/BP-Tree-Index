#pragma once

#include "../../Data_Structures/Containers/Dynamic_Array.hpp"
#include "../../Data_Structures/Containers/Pair.hpp"
#include "../../Data_Structures/SmartPtrs/include/SharedPtr.hpp"
#include <functional>
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


// CRTP
template <typename Derived> struct BaseNode {
    bool is_leaf() const {
        return static_cast<const Derived*>(this)->is_leaf_impl();
    }
}; 


template <typename Key, typename RecordId>
struct InternalNode : public BaseNode<InternalNode<Key, RecordId>> {

    mutable std::shared_mutex mutex_;

    DynamicArray<Key> keys_;
    DynamicArray<SharedPtr<BaseNode<InternalNode<Key, RecordId>>>> children_;
       
    bool is_leaf_impl() const noexcept { return false; }

    size_t size() const; 
    bool is_full() const;

    void insert_key_at(size_t index, const Key& key);
    void insert_key_at(size_t index, Key&& key);

};

template <typename Key, typename RecordId>
struct LeafNode : public BaseNode<LeafNode<Key, RecordId>> {
    
    mutable std::shared_mutex mutex_;

    DynamicArray<Key> keys_;
    DynamicArray<RecordId> values_;
        
    SharedPtr<LeafNode> next_;

    bool is_leaf_impl() const noexcept { return true; }

    size_t size() const; 
    bool is_full() const;

};



template <typename Key, typename RecordId, size_t Order = 128, typename compare = std::less<Key>>
class BPlusTree {
  
  private:

// ------------------------------------
    
    using InternalNodePtr = SharedPtr<InternalNode<Key, RecordId>>;
    using LeafNodePtr = SharedPtr<LeafNode<Key, RecordId>>;

    using VariantNode = std::variant<InternalNodePtr, LeafNodePtr>;

    mutable std::shared_mutex root_mutex_;
    VariantNode root_;
    
    size_t size_ = 0;

    compare comparator_;

// -------------------------------------

    void split_child(VariantNode& parent, size_t child_index);

    LeafNodePtr find_leaf(const Key& key);

    bool is_less_or_eq(const Key& key1, const Key& key2) const;

    void redistrebute_nodes(VariantNode& left, VariantNode& right);

    void merge_nodes(VariantNode& left, VariantNode& right);

  public:
    
    class Iterator {
      private:
      
        LeafNodePtr current_node_;
        
        size_t current_index_;

      public:

        Iterator(LeafNodePtr node = nullptr, size_t index = 0);
        
        Iterator& operator++();
        
        Pair<const Key&, RecordId> operator*() const;

        bool operator==(const Iterator& other) const;
    };

    BPlusTree() : root_(LeafNodePtr{}) {}

    BPlusTree(const BPlusTree& other);

    BPlusTree(BPlusTree&& other) noexcept;

    BPlusTree& operator=(const BPlusTree& other);

    BPlusTree& operator=(BPlusTree&& other) noexcept;

    ~BPlusTree();

    void insert(const Key& key, RecordId id);

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

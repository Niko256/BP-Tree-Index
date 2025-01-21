#pragma once

#include "../external/Data_Structures/Containers/Dynamic_Array.hpp"
#include "../external/Data_Structures/Containers/Pair.hpp"
#include "../external/Data_Structures/SmartPtrs/include/SharedPtr.hpp"
#include <functional>
#include <shared_mutex>
#include <tuple>
#include <variant>


template <typename... Keys>
struct CompositeKey {
    std::tuple<Keys...> parameters_; ///< Tuple storing the individual keys.

    CompositeKey(Keys... keys) : parameters_(std::make_tuple(keys...)) {}
    CompositeKey() : parameters_() {}
    CompositeKey(const CompositeKey& other) = default;
    CompositeKey(CompositeKey&& other) noexcept = default;

    CompositeKey& operator=(const CompositeKey& other) = default;
    CompositeKey& operator=(CompositeKey&& other) noexcept = default;

    template <size_t I>
    const auto& get() const;

    bool operator<(const CompositeKey& other) const;
    bool operator>(const CompositeKey& other) const;
    bool operator==(const CompositeKey& other) const;
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
    std::shared_ptr<InternalNode<Key, RecordId, Order>>, 
    std::shared_ptr<LeafNode<Key, RecordId, Order>>, 
    std::monostate>;



template <typename Key, typename RecordId, size_t Order>
struct InternalNode : public BaseNode<InternalNode<Key, RecordId, Order>> {

    mutable std::shared_mutex mutex_; ///< Mutex for thread-safe access.
    
    std::vector<Key> keys_; ///< Array of keys stored in the node.
    
    std::vector<VariantNode<Key, RecordId, Order>> children_; ///< Array of child nodes.

    bool is_leaf_impl() const noexcept { return false; }

    InternalNode() : keys_(), children_() {}
    ~InternalNode() {}

    size_t size() const;
    bool is_full() const;

    void insert_key_at(size_t index, const Key& key);
    void insert_key_at(size_t index, Key&& key);
};



template <typename Key, typename RecordId, size_t Order>
struct LeafNode : public BaseNode<LeafNode<Key, RecordId, Order>> {

    mutable std::shared_mutex mutex_; 
    
    std::vector<Key> keys_;
    
    std::vector<RecordId> values_;
    
    std::shared_ptr<LeafNode> next_;


    bool is_leaf_impl() const noexcept { return true; }

    LeafNode() : keys_(), values_(), next_(nullptr) {}

    size_t size() const;
    bool is_full() const;
    RecordId get_record(size_t index) const;
};




template <typename Key, typename RecordId, size_t Order = 128, typename compare = std::less<Key>>
class BPlusTree {

  private:

    using InternalNodePtr = std::shared_ptr<InternalNode<Key, RecordId, Order>>;
    using LeafNodePtr = std::shared_ptr<LeafNode<Key, RecordId, Order>>;

    mutable std::shared_mutex root_mutex_; ///< Mutex for thread-safe access to the root.
    
    VariantNode<Key, RecordId, Order> root_; ///< The root node of the tree.
    
    size_t size_ = 0; ///< The number of key-value pairs in the tree.
    
    compare comparator_; 


    void split_leaf(LeafNodePtr node);
    void split_internal(InternalNodePtr node);

    bool is_less_or_eq(const Key& key1, const Key& key2) const;

    void redistribute_nodes(VariantNode<Key, RecordId, Order>& lhs, VariantNode<Key, RecordId, Order>& rhs);
    void merge_nodes(VariantNode<Key, RecordId, Order>& left, VariantNode<Key, RecordId, Order>& right);
    void balance_after_remove(VariantNode<Key, RecordId, Order> node);

    LeafNodePtr find_leaf(const Key& key);
    std::shared_ptr<InternalNode<Key, RecordId, Order>> find_parent(const VariantNode<Key, RecordId, Order>& target);

    InternalNodePtr deep_copy_node(const InternalNodePtr& node);
    void rebuild_leaf_links();
    void collect_leaves(
            const VariantNode<Key, RecordId, Order>& node,
            std::vector<LeafNodePtr>& leaves);

  public:
    class Iterator {
      private:

        LeafNodePtr current_node_;
        size_t current_index_; 

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

      public:
        ConstIterator(LeafNodePtr node = nullptr, size_t index = 0);

        ConstIterator& operator++();
        const Pair<const Key&, const RecordId&> operator*() const;
        bool operator==(const ConstIterator& other) const;
        bool operator!=(const ConstIterator& other) const;
    };

    BPlusTree() : root_(std::monostate{}), size_(0), comparator_() {}
    BPlusTree(const BPlusTree& other);
    BPlusTree(BPlusTree&& other) noexcept;

    BPlusTree& operator=(const BPlusTree& other);
    BPlusTree& operator=(BPlusTree&& other) noexcept;

    ~BPlusTree() = default;

    template <typename T>
    void insert(const Key& key, T&& id);
    void remove(const Key& key);


    std::vector<RecordId> find(const Key& key);
    std::vector<RecordId> range_search(const Key& from, const Key& to);
    std::vector<RecordId> prefix_search(const std::string& prefix);
    
    template <typename Predicate>
    std::vector<RecordId> find_if(Predicate pred);

    Iterator begin();
    Iterator end();

    bool empty() const;
    size_t height() const;
    double fill_factor() const;

    void clear();
};

#include "Composite-Key.tpp"
#include "BP-Tree.tpp"

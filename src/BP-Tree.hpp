#pragma once

#include "../external/Data_Structures/Containers/Dynamic_Array.hpp"
#include "../external/Data_Structures/Containers/Pair.hpp"
#include "../external/Data_Structures/SmartPtrs/include/SharedPtr.hpp"
#include <functional>
#include <shared_mutex>
#include <tuple>
#include <variant>


/**
 * @brief Represents a composite key consisting of multiple keys.
 * 
 * A composite key is a combination of multiple keys (e.g., name and age).
 * It is used as a key in the B+ Tree for more complex indexing.
 * 
 * @tparam Keys The types of the keys that make up the composite key.
 */
template <typename... Keys>
struct CompositeKey {
    std::tuple<Keys...> parameters_; ///< Tuple storing the individual keys.

    /**
     * @brief Constructs a CompositeKey with the given keys.
     * 
     * @param keys The keys to be combined into a composite key.
     */
    CompositeKey(Keys... keys) : parameters_(std::make_tuple(keys...)) {}

    /**
     * @brief Gets the I-th key from the composite key.
     * 
     * @tparam I The index of the key to retrieve.
     * @return const auto& A reference to the I-th key.
     */
    template <size_t I>
    const auto& get() const;

    bool operator<(const CompositeKey& other) const; 

    bool operator==(const CompositeKey& other) const;
};


// forward declaration
template <typename Key, typename RecordId, size_t Order>
struct InternalNode;

template <typename Key, typename RecordId, size_t Order>
struct LeafNode;


/**
 * @brief Base class for nodes in the B+ Tree.
 * 
 * This class uses the CRTP (Curiously Recurring Template Pattern) to provide
 * a common interface for checking if a node is a leaf.
 * 
 * @tparam Derived The derived class (either InternalNode or LeafNode).
 */

template <typename Derived> struct BaseNode {
    /**
     * @brief Checks if the node is a leaf node.
     */
    bool is_leaf() const {
        return static_cast<const Derived*>(this)->is_leaf_impl();
    }
};


template <typename Key, typename RecordId, size_t Order>
using VariantNode = std::variant<
    SharedPtr<InternalNode<Key, RecordId, Order>>, 
    SharedPtr<LeafNode<Key, RecordId, Order>>, 
    std::monostate>;


/**
 * @brief Represents an internal (non-leaf) node in the B+ Tree.
 * 
 * Internal nodes store keys and pointers to child nodes. They are used to
 * navigate the tree during search, insertion, and deletion operations.
 * 
 * @tparam Key The type of the keys stored in the node.
 * @tparam RecordId The type of the record IDs associated with the keys.
 * @tparam Order The maximum number of children per node.
 */

template <typename Key, typename RecordId, size_t Order>
struct InternalNode : public BaseNode<InternalNode<Key, RecordId, Order>> {

    mutable std::shared_mutex mutex_; ///< Mutex for thread-safe access.
    
    DynamicArray<Key> keys_; ///< Array of keys stored in the node.
    
    DynamicArray<VariantNode<Key, RecordId, Order>> children_; ///< Array of child nodes.

    /**
     * @brief Checks if the node is a leaf node.
     */
    bool is_leaf_impl() const noexcept { return false; }

    /**
     * @brief Returns the number of keys in the node.
     */
    size_t size() const;

    /**
     * @brief Checks if the node is full.
     */
    bool is_full() const;

    /**
     * @brief Inserts a key at the specified index.
     */
    void insert_key_at(size_t index, const Key& key);

    /**
     * @brief Inserts a key at the specified index using move semantics.
     */
    void insert_key_at(size_t index, Key&& key);
};


/**
 * @brief Represents a leaf node in the B+ Tree.
 * 
 * Leaf nodes store keys and their associated record IDs. They are linked
 * together to support efficient range queries.
 * 
 * @tparam Key The type of the keys stored in the node.
 * @tparam RecordId The type of the record IDs associated with the keys.
 * @tparam Order The maximum number of keys per node.
 */

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



/**
 * @brief A B+ Tree implementation for efficient storage and retrieval of key-value pairs.
 * 
 * The B+ Tree is a self-balancing tree that supports efficient insertion, deletion,
 * and search operations. It is particularly useful for databases and file systems.
 * 
 * @tparam Key The type of the keys stored in the tree.
 * @tparam RecordId The type of the record IDs associated with the keys.
 * @tparam Order The maximum number of children per node (default is 128).
 * @tparam compare The comparison function for keys (default is std::less<Key>).
 */

template <typename Key, typename RecordId, size_t Order = 128, typename compare = std::less<Key>>
class BPlusTree {

  private:

    using InternalNodePtr = SharedPtr<InternalNode<Key, RecordId, Order>>;
    using LeafNodePtr = SharedPtr<LeafNode<Key, RecordId, Order>>;

    mutable std::shared_mutex root_mutex_; ///< Mutex for thread-safe access to the root.
    
    VariantNode<Key, RecordId, Order> root_; ///< The root node of the tree.
    
    size_t size_ = 0; ///< The number of key-value pairs in the tree.
    
    compare comparator_; 

    /**
     * @brief Splits a child node when it becomes full.
     * 
     * This method is called during insertion to split a full child node into two nodes
     * and promote the middle key to the parent node.
     * 
     * @param parent The parent node containing the child to be split.
     * @param child_index The index of the child node in the parent's children array.
     */
    void split_child(VariantNode<Key, RecordId, Order>& parent, size_t child_index);

    /**
     * @brief Finds the leaf node that should contain the specified key.
     * 
     * This method traverses the tree from the root to the appropriate leaf node.
     * 
     * @param key The key to search for.
     * @return LeafNodePtr A pointer to the leaf node containing the key.
     */
    LeafNodePtr find_leaf(const Key& key);

    /**
     * @brief Checks if the first key is less than or equal to the second key.
     * 
     * This method uses the comparator function to compare two keys.
     */
    bool is_less_or_eq(const Key& key1, const Key& key2) const;

    /**
     * @brief Redistributes keys and children between two nodes to balance them.
     * 
     * This method is called during deletion to balance underflowed nodes.
     */
    void redistribute_nodes(VariantNode<Key, RecordId, Order>& lhs, VariantNode<Key, RecordId, Order>& rhs);

    /**
     * @brief Merges two underflowed nodes into one.
     * 
     * This method is called during deletion to merge two nodes when redistribution
     * is not possible.
     */
    void merge_nodes(VariantNode<Key, RecordId, Order>& left, VariantNode<Key, RecordId, Order>& right);

    /**
     * @brief Balances the tree after a key is removed from a leaf node.
     * 
     * This method ensures that the tree remains balanced after a deletion operation.
     * 
     * @param node The leaf node from which a key was removed.
     */
    void balance_after_remove(VariantNode<Key, RecordId, Order> node);

    /**
     * @brief Finds the parent of a given leaf node.
     * 
     * This method traverses the tree to locate the parent of the specified leaf node.
     * 
     * @return a pointer to the parent node.
     */
    SharedPtr<InternalNode<Key, RecordId, Order>> find_parent(VariantNode<Key, RecordId, Order> node);

    /**
     * @brief Creates a deep copy of an internal node and its children.
     * 
     * This method is used during tree copying to recursively clone nodes.
     */
    SharedPtr<InternalNode<Key, RecordId, Order>> copy_nodes(const SharedPtr<InternalNode<Key, RecordId, Order>>& node);

    /**
     * @brief Creates a deep copy of a leaf node.
     */
    SharedPtr<LeafNode<Key, RecordId, Order>> copy_nodes(const SharedPtr<LeafNode<Key, RecordId, Order>>& node);

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

    BPlusTree() : root_(LeafNodePtr{}) {}

    BPlusTree(const BPlusTree& other);

    BPlusTree(BPlusTree&& other) noexcept;

    BPlusTree& operator=(const BPlusTree& other);

    BPlusTree& operator=(BPlusTree&& other) noexcept;

    ~BPlusTree() = default;

    /**
     * @brief Inserts a key-value pair into the tree.
     * 
     */
    template <typename T>
    void insert(const Key& key, T&& id);

    /**
     * @brief Removes a key-value pair from the tree.
     * 
     */
    void remove(const Key& key);

    /**
     * @brief Bulk loads key-value pairs into the tree.
     * 
     * This method efficiently builds the tree from a sorted range of key-value pairs.
     * 
     * @tparam InputIt The type of the input iterator.
     * @param first The beginning of the range.
     * @param last The end of the range.
     */
    template <typename InputIt>
    void bulk_load(InputIt first, InputIt last);

    /**
     * @brief Finds all record IDs associated with the specified key.
     * 
     * @param key The key to search for.
     * @return DynamicArray<RecordId> An array of record IDs.
     */
    DynamicArray<RecordId> find(const Key& key);

    /**
     * @brief Performs a range search for keys between `from` and `to`.
     * 
     * @param from The lower bound of the range.
     * @param to The upper bound of the range.
     * @return DynamicArray<RecordId> An array of record IDs within the range.
     */
    DynamicArray<RecordId> range_search(const Key& from, const Key& to);

    /**
     * @brief Returns an iterator to the beginning of the tree.
     */
    Iterator begin();

    /**
     * @brief Returns an iterator to the end of the tree.
     */
    Iterator end();

    /**
     * @brief Checks if the tree is empty.
     */
    bool empty() const;

    void clear();
};

#include "Composite-Key.tpp"
#include "BP-Tree.tpp"

#pragma once

#include "../../Data_Structures/Containers/Dynamic_Array.hpp"
#include "../../Data_Structures/Containers/List.hpp"
#include "../../Data_Structures/Containers/Pair.hpp"
#include "../../Data_Structures/SmartPtrs/include/SharedPtr.hpp"
#include <iterator>
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
    
    std::string to_string() const;

    // using EmployeeKey = CompositeKey<std::string, int>; // name, age
    // BPlusTree<EmployeeKey, RecordId> tree;
};


template <typename Key, typename RecordId, size_t Order = 128>
class BPlusTree {
  private:

    struct Node {

        bool is_leaf_ = true;

        bool is_root_ = false;
        
        SharedPtr<Node> next_leaf_;
        SharedPtr<Node> prev_leaf_;

        DynamicArray<Key> keys_;
        
        DynamicArray<std::variant<SharedPtr<Node>, RecordId>> children_;


        Node(bool leaf) : is_leaf_(leaf) {}

        size_t size() const;

        SharedPtr<Node> get_child(size_t index);

        RecordId get_record(size_t index);

        bool is_full() const;
        bool has_min_keys() const noexcept;

        void insert_key_at(size_t index, const Key& key);
        void insert_key_at(size_t index, Key&& key);

        void remove_key_at(size_t index);
        void remove_child_at(size_t index);

        void insert_child_at(size_t index, const std::variant<SharedPtr<Node>, RecordId>& child);
    };

// ----------------------------------
    
    SharedPtr<Node> root_;
    SharedPtr<Node> last_accessed_leaf_;
    size_t size_ = 0;
    
    using NodePtr = SharedPtr<Node>;
    using ChildVariant = std::variant<SharedPtr<Node>, RecordId>;
    
// ----------------------------------

  private:

    size_t split_child(SharedPtr<Node> parent, size_t child_index);

    size_t find_position(const Key& key, const DynamicArray<Key>& keys) const;

    SharedPtr<Node> find_leaf(const Key& key);

    DynamicArray<SharedPtr<Node>> find_path(const Key& key);

    void balance_after_delete(SharedPtr<Node> node);

    bool try_borrow_from_sibling(SharedPtr<Node> node, bool try_left);

    bool is_less_or_eq(const Key& key1, const Key& key2) const;

    void redistrebute_nodes(SharedPtr<Node> left, SharedPtr<Node> right);

    void merge_nodes(SharedPtr<Node> left, SharedPtr<Node> right);

  public:
    
    class Iterator {
      private:
      
        SharedPtr<Node> current_node_;
        
        size_t current_index_;

        using value_type = Pair<const Key&, RecordId>;
        using reference = value_type&;
        using pointer = value_type*;
        using iterator_category = std::forward_iterator_tag;

      public:
       
        Iterator(SharedPtr<Node> node = nullptr, size_t index = 0) : current_node_(node), current_index_(index) {}

        Iterator& operator++();
        
        Pair<const Key&, RecordId> operator*() const;

        bool operator==(const Iterator& other) const;

        bool operator!=(const Iterator& other) const;
    };

    class ConstIterator {
      private:
      
        SharedPtr<const Node> current_node_;  
        size_t current_index_;

        using value_type = Pair<const Key&, const RecordId&>;
        using reference = const value_type&;
        using pointer = const value_type*;
        using iterator_category = std::forward_iterator_tag;

      public:

        ConstIterator(SharedPtr<const Node> node = nullptr, size_t index = 0)
            : current_node_(node), current_index_(index) {}

        ConstIterator& operator++();

        const Pair<const Key&, const RecordId&> operator*() const;

        bool operator==(const ConstIterator& other) const;
        bool operator!=(const ConstIterator& other) const;
    };


    BPlusTree() : root_(nullptr), size_(0) {}

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
    ConstIterator cbegin() const;
    ConstIterator cend() const;

    bool empty();

    void clear();

    bool is_underflow() const;

    size_t get_size() const noexcept;

    size_t get_height() const;
};

#include "BP-Tree.tpp"
#include "CompositeKey.tpp"

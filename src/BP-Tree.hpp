#include "../../Data_Structures/Containers/Dynamic_Array.hpp"
#include "../../Data_Structures/Containers/Pair.hpp"
#include "../../Data_Structures/SmartPtrs/include/SharedPtr.hpp"
#include <tuple>
#include <variant>


template <typename... Keys>
struct CompositeKey {
    std::tuple<Keys...> parameters_;

    CompositeKey(Keys... keys) : parameters_(std::make_tuple(keys...)) {}

    template <size_t I>
    const auto& get() const {
        return std::get<I>(parameters_);
    }

    template <size_t N>
    bool matches_prefix(const CompositeKey& other) const;

    bool operator<(const CompositeKey& other) const {
        return parameters_ < other.parameters_;
    }

    bool operator==(const CompositeKey& other) const {
        return parameters_ == other.parameters_;
    };

    // using EmployeeKey = CompositeKey<std::string, int>; // name, age
    // BPlusTree<EmployeeKey, RecordId> tree;
};



template <typename Key, typename RecordId, size_t Order = 128>
class BPlusTree {
  private:

    struct Node {

        bool is_leaf_ = true;
        
        SharedPtr<Node> next_leaf_;

        DynamicArray<Key> keys_;
        
        DynamicArray<std::variant<SharedPtr<Node>, RecordId>> children_;

        Node(bool leaf) : is_leaf_(leaf) {}

        size_t size() const { return keys_.size(); }

        SharedPtr<Node> get_child(size_t index);

        RecordId get_record(size_t index);

        bool is_full() const;
        bool has_min_keys() const noexcept { return size() >= Order - 1; }

        void insert_key_at(size_t index, const Key& key);
        void insert_key_at(size_t index, Key&& key);

        void insert_child_at(size_t index, const std::variant<SharedPtr<Node>, RecordId>& child);
    };

    SharedPtr<Node> root_;

    size_t size = 0;

  private:

    void split_child(SharedPtr<Node> parent, size_t child_index);

    SharedPtr<Node> find_leaf(const Key& key);

    bool is_less_or_eq(const Key& key1, const Key& key2) const;

    void redistrebute_nodes(SharedPtr<Node> left, SharedPtr<Node> right);

    void merge_nodes(SharedPtr<Node> left, SharedPtr<Node> right);

  public:
    
    class Iterator {
      private:
      
        SharedPtr<Node> current_node_;
        
        size_t current_index_;

      public:
        
        Iterator& operator++();
        
        Pair<const Key&, RecordId> operator*() const;

        bool operator==(const Iterator& other) const;
    };

    BPlusTree() : root_(nullptr) {}

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

    bool empty();

    void clear();
};



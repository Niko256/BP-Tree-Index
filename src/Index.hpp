#pragma once
#include "BP-Tree.hpp"
#include "Composite-Key.hpp"

template<typename... Fields>
class Record {
  private:
    std::tuple<Fields...> fields_; // Tuple storing the fields of the record.
    size_t id_; // Unique identifier for the record.

  public:

    Record(size_t id, Fields... fields) 
        : fields_(std::make_tuple(fields...)), id_(id) {}

    /**
     * @brief Gets the value of a specific field by index.
     *
     */
    template<size_t I>
    const auto& get() const { return std::get<I>(fields_); }

    /**
     * @brief Gets the value of a specific field by index.
     *
     */
    template<size_t I>
    auto& get() { return std::get<I>(fields_); }

    /**
     * @brief Gets the unique ID of the record.
     */
    size_t get_id() const { return id_; }
};


/**
 * @class Index
 * @brief A template class representing an index for records.
 *
 * This class provides an index for records using a B+ tree. It supports insertion,
 * removal, updating, and querying of records based on a key.
 *
 * @tparam RecordType The type of the record.
 * @tparam KeyType The type of the key used for indexing.
 * @tparam Compare The comparison function for keys (default is std::less<KeyType>).
 */
template<typename RecordType, typename KeyType, typename Compare = std::less<KeyType>> 
class Index {
  protected:
    mutable BPlusTree<KeyType, size_t, 128, Compare> tree_; // B+ tree for indexing.
    
    mutable DynamicArray<RecordType> records_; // Dynamic array storing all records.
    
    std::function<KeyType(const RecordType&)> key_extractor_; // Function to extract keys from records.

  public:
    
    Index(std::function<KeyType(const RecordType&)> key_extractor) 
        : key_extractor_(key_extractor) {}

    /**
     * @brief Inserts a record into the index.
     */
    void insert(const RecordType& record) {
        records_.push_back(record);
        tree_.insert(key_extractor_(record), record.get_id());
    }

    /**
     * @brief Removes a record from the index by key.
     */
    void remove(const KeyType& key) {
        tree_.remove(key);
    }

    /**
     * @brief Updates an existing record in the index.
     *
     */
    void update(const RecordType& old_record, const RecordType& new_record) {
        auto old_key = key_extractor_(old_record);
        for (size_t i = 0; i < records_.size(); ++i) {
            if (key_extractor_(records_[i]) == old_key && 
                records_[i].get_id() == old_record.get_id()) {
                records_[i] = new_record;
                tree_.remove(old_key);
                tree_.insert(key_extractor_(new_record), new_record.get_id());
                return;
            }
        }
    }

    /**
     * @brief Checks if the index contains a record with the given key.
     *
     */
    bool contains(const KeyType& key) const {
        return !tree_.find(key).empty();
    }

    /**
     * @brief Finds all records with the given key.
     *
     */
    DynamicArray<RecordType> find(const KeyType& key) const {
        DynamicArray<RecordType> result;
        auto record_ids = tree_.find(key);
        for (const auto& id : record_ids) {
            result.push_back(records_[id]);
        }
        return result;
    }

    /**
     * @brief Performs a range search for records within a given key range.
     *
     * @param from The lower bound of the key range.
     * @param to The upper bound of the key range.
     * @return A DynamicArray of records within the key range.
     */
    DynamicArray<RecordType> range_search(const KeyType& from, const KeyType& to) const {
        DynamicArray<RecordType> result;
        auto record_ids = tree_.range_search(from, to);
        for (const auto& id : record_ids) {
            result.push_back(records_[id]);
        }
        return result;
    }

    /**
     * @brief Finds all records that satisfy a given predicate.
     *
     */
    template<typename Predicate>
    DynamicArray<RecordType> find_if(Predicate pred) const {
        DynamicArray<RecordType> result;
        for (size_t i = 0; i < records_.size(); ++i) {
            if (pred(records_[i])) {
                result.push_back(records_[i]);
            }
        }
        return result;
    }

    /**
     * @brief Gets the number of records in the index.
     */
    size_t size() const {
        return records_.size();
    }

    /**
     * @brief Gets the fill factor of the B+ tree.
     */
    double fill_factor() const {
        return tree_.fill_factor();
    }

    /**
     * @brief Gets a record by its ID.
     *
     */
    const RecordType& get_record(size_t id) const {
        return records_[id];
    }
};



/**
 * @class CompositeIndex
 * @brief A template class representing a composite index for records.
 */
template<typename RecordType, typename... Keys>
class CompositeIndex {
  private:
    mutable BPlusTree<CompositeKey<Keys...>, size_t> tree_; ///< B+ tree for indexing with composite keys.

    mutable DynamicArray<RecordType> records_; 
    
    std::tuple<std::function<Keys(const RecordType&)>...> key_extractors_; 

    /**
     * @brief Creates a composite key from a record.
     *
     * @tparam Is Index sequence for unpacking the tuple.
     * @param record The record to create the key from.
     */
    template<size_t... Is>
    CompositeKey<Keys...> make_key(const RecordType& record, std::index_sequence<Is...>) const {
        return CompositeKey<Keys...>(std::get<Is>(key_extractors_)(record)...);
    }

  public:
    /**
     * @brief Constructs a CompositeIndex object.
     *
     * @param extractors Functions to extract individual keys from records.
     */
    CompositeIndex(std::function<Keys(const RecordType&)>... extractors)
        : key_extractors_(extractors...) {}

    /**
     * @brief Inserts a record into the composite index.
     *
     * @param record The record to insert.
     */
    void insert(const RecordType& record) {
        records_.push_back(record);
        auto key = make_key(record, std::index_sequence_for<Keys...>{});
        tree_.insert(key, record.get_id());
    }

    /**
     * @brief Finds all records with the given composite key.
     *
     * @param key The composite key to search for.
     * @return A DynamicArray of records matching the composite key.
     */
    DynamicArray<RecordType> find(const CompositeKey<Keys...>& key) const {
        DynamicArray<RecordType> result;
        auto record_ids = tree_.find(key);
        for (const auto& id : record_ids) {
            result.push_back(records_[id]);
        }
        return result;
    }

    /**
     * @brief Finds all records with a specific component of the composite key.
     *
     * @tparam I The index of the component to search for.
     * @param value The value of the component to match.
     * @return A DynamicArray of records matching the component value.
     */
    template<size_t I>
    DynamicArray<RecordType> find_by_component(const std::tuple_element_t<I, std::tuple<Keys...>>& value) const {
        DynamicArray<RecordType> result;
        for (size_t i = 0; i < records_.size(); ++i) {
            if (std::get<I>(key_extractors_)(records_[i]) == value) {
                result.push_back(records_[i]);
            }
        }
        return result;
    }

    /**
     * @brief Updates an existing record in the composite index.
     *
     * @param old_record The old record to replace.
     * @param new_record The new record to insert.
     */
    void update(const RecordType& old_record, const RecordType& new_record) {
        auto old_key = make_key(old_record, std::index_sequence_for<Keys...>{});
        for (size_t i = 0; i < records_.size(); ++i) {
            if (records_[i].get_id() == old_record.get_id()) {
                records_[i] = new_record;
                tree_.remove(old_key);
                tree_.insert(make_key(new_record, std::index_sequence_for<Keys...>{}), new_record.get_id());
                return;
            }
        }
    }
};

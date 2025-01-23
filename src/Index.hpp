#pragma once
#include "BP-Tree.hpp"

template<typename... Fields> class Record {
  private:
    std::tuple<Fields...> fields_;
    size_t id_;

  public:
    Record(size_t id, Fields... fields) 
        : fields_(std::make_tuple(fields...)), id_(id) {}

    template<size_t I>
    const auto& get() const { return std::get<I>(fields_); }

    template<size_t I>
    auto& get() { return std::get<I>(fields_); }

    size_t get_id() const { return id_; }
};


template<typename RecordType, typename KeyType, typename Compare = std::less<KeyType>> 
class Index {
  protected:
    mutable BPlusTree<KeyType, size_t, 128, Compare> tree_;
    
    mutable DynamicArray<RecordType> records_;
    
    std::function<KeyType(const RecordType&)> key_extractor_;

  public:
    Index(std::function<KeyType(const RecordType&)> key_extractor) 
        : key_extractor_(key_extractor) {}

    void insert(const RecordType& record) {
        records_.push_back(record);
        tree_.insert(key_extractor_(record), record.get_id());
    }

    void remove(const KeyType& key) {
        tree_.remove(key);
    }

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

    bool contains(const KeyType& key) const {
        return !tree_.find(key).empty();
    }

    DynamicArray<RecordType> find(const KeyType& key) const {
        DynamicArray<RecordType> result;
        auto record_ids = tree_.find(key);
        for (const auto& id : record_ids) {
            result.push_back(records_[id]);
        }
        return result;
    }

    DynamicArray<RecordType> range_search(const KeyType& from, const KeyType& to) const {
        DynamicArray<RecordType> result;
        auto record_ids = tree_.range_search(from, to);
        for (const auto& id : record_ids) {
            result.push_back(records_[id]);
        }
        return result;
    }

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

    size_t size() const {
        return records_.size();
    }

    double fill_factor() const {
        return tree_.fill_factor();
    }

    const RecordType& get_record(size_t id) const {
        return records_[id];
    }
};

template<typename RecordType, typename... Keys>
class CompositeIndex {
  private:
    mutable BPlusTree<CompositeKey<Keys...>, size_t> tree_;
    
    mutable DynamicArray<RecordType> records_;
    
    std::tuple<std::function<Keys(const RecordType&)>...> key_extractors_;

    template<size_t... Is>
    CompositeKey<Keys...> make_key(const RecordType& record, std::index_sequence<Is...>) const {
        return CompositeKey<Keys...>(std::get<Is>(key_extractors_)(record)...);
    }

  public:
    CompositeIndex(std::function<Keys(const RecordType&)>... extractors)
        : key_extractors_(extractors...) {}

    void insert(const RecordType& record) {
        
        records_.push_back(record);
        auto key = make_key(record, std::index_sequence_for<Keys...>{});
        tree_.insert(key, record.get_id());
    }

    DynamicArray<RecordType> find(const CompositeKey<Keys...>& key) const {
        DynamicArray<RecordType> result;
        auto record_ids = tree_.find(key);
        
        for (const auto& id : record_ids) {
            result.push_back(records_[id]);
        }
        return result;
    }

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

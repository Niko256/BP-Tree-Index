#include "BP-Tree.hpp"

// ---------------------------------------
// COMPOSITE KEY IMPL

template <typename... Keys>
template <size_t I>
const auto& CompositeKey<Keys...>::get() const {
    return std::get<I>(parameters_);
}

template <typename... Keys>
template <size_t  N>
bool CompositeKey<Keys...>::matches_prefix(const CompositeKey& other) const {
    return std::get<N>(parameters_) == std::get<N>(other.parameters_);
}


template <typename... Keys>
bool CompositeKey<Keys...>::operator<(const CompositeKey& other) const {
    return parameters_ < other.parameters_;
}


template <typename... Keys>
bool CompositeKey<Keys...>::operator==(const CompositeKey& other) const {
    return parameters_ == other.parameters_;
}



#include "BP-Tree.hpp"

// ------------------------- COMPOSITE KEY IMPLEMENTATION --------------------------


/**
 * @brief Gets the I-th element of the composite key.
 * @tparam I The index of the element to retrieve.
 * @return A const reference to the I-th element.
 */
template <typename... Keys>
template <size_t I>
const auto& CompositeKey<Keys...>::get() const {
    return std::get<I>(parameters_);
}

/**
 * @brief Compares two composite keys using the less-than operator.
 */
template <typename... Keys>
bool CompositeKey<Keys...>::operator<(const CompositeKey& other) const {
    return parameters_ < other.parameters_;
}

/**
 * @brief Compares two composite keys for equality.
 */
template <typename... Keys>
bool CompositeKey<Keys...>::operator==(const CompositeKey& other) const {
    return parameters_ == other.parameters_;
}

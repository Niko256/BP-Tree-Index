#pragma once

#include <cstddef>
#include <tuple>

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

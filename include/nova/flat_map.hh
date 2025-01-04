/**
 * Part of Nova C++ Library.
 *
 * A cache friendly associative container.
 *
 * Because C++ standard is slow and it's not `constexpr` capable anyway.
 * Only partly C++ standard compliant, but it should provide a drop-in-replacement.
 */

#pragma once

#include "type_traits.hh"

#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace nova {
namespace detail {

template <typename KeyIterator, typename MappedIterator>
class flat_map_iterator {
public:
    using value_type = std::pair<typename KeyIterator::value_type, typename MappedIterator::value_type>;
    using reference = std::pair<typename KeyIterator::reference, typename MappedIterator::reference>;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;

private:
    class proxy {
    public:
        proxy(const reference& ref)
            : m_ref(ref)
        {}

        const reference* operator->() const {
            return std::addressof(m_ref);
        }

    private:
        reference m_ref;
    };

public:
    using pointer = proxy;

    constexpr flat_map_iterator(KeyIterator key_iter, MappedIterator mapped_iter)
        : m_key_iter(key_iter)
        , m_mapped_iter(mapped_iter)
    {}

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return reference { *m_key_iter, *m_mapped_iter };
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return { **this };
    }

    constexpr flat_map_iterator& operator++() noexcept {
        ++m_key_iter;
        ++m_mapped_iter;
        return *this;
    }

    [[nodiscard]] constexpr flat_map_iterator& operator++(int) noexcept {
        flat_map_iterator iter = *this;
        ++(*this);
        return iter;
    }

    constexpr flat_map_iterator& operator--() noexcept {
        --m_key_iter;
        --m_mapped_iter;
        return *this;
    }

    [[nodiscard]] constexpr flat_map_iterator& operator--(int) noexcept {
        flat_map_iterator iter = *this;
        --(*this);
        return iter;
    }

    [[nodiscard]] constexpr flat_map_iterator operator+(difference_type n) const noexcept {
        flat_map_iterator iter = *this;
        iter += n;
        return iter;
    }

    [[nodiscard]] constexpr flat_map_iterator operator-(difference_type n) const noexcept {
        flat_map_iterator iter = *this;
        iter -= n;
        return iter;
    }

    [[nodiscard]] constexpr flat_map_iterator operator+=(difference_type n) const noexcept {
        flat_map_iterator iter = *this;
        iter = iter + n;
        return iter;
    }

    [[nodiscard]] constexpr difference_type operator-(flat_map_iterator other) const noexcept {
        return m_key_iter - other.m_key_iter;
    }

    [[nodiscard]] constexpr bool operator==(flat_map_iterator other) const noexcept {
        return m_key_iter == other.m_key_iter;
    }

    [[nodiscard]] constexpr auto operator<=>(flat_map_iterator other) const noexcept {
        return m_key_iter <=> other.m_key_iter;
    }

    [[nodiscard]] constexpr auto key() const noexcept {
        return m_key_iter;
    }

    [[nodiscard]] constexpr auto value() const noexcept {
        return m_mapped_iter;
    }

private:
    KeyIterator m_key_iter;
    MappedIterator m_mapped_iter;
};

} // namespace detail

template <typename Key,
          typename T,
          typename Compare = std::less<Key>,
          typename KeyContainer = std::vector<Key>,
          typename MappedContainer = std::vector<T>
>
class flat_map {
public:
    using self = flat_map<Key, T, Compare, KeyContainer, MappedContainer>;

    using key_type    = Key;
    using mapped_type = T;
    using key_compare = Compare;

    using value_type      = std::pair<const Key, T>;
    using pointer         = std::pair<const Key*, T*>;
    using reference       = std::pair<const Key&, T&>;
    using const_reference = std::pair<const Key&, const T&>;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    using iterator               = detail::flat_map_iterator<typename KeyContainer::iterator, typename MappedContainer::iterator>;
    using const_iterator         = detail::flat_map_iterator<typename KeyContainer::const_iterator, typename MappedContainer::const_iterator>;
    using reverse_iterator       = detail::flat_map_iterator<typename KeyContainer::reverse_iterator, typename MappedContainer::reverse_iterator>;
    using const_reverse_iterator = detail::flat_map_iterator<typename KeyContainer::const_reverse_iterator, typename MappedContainer::const_reverse_iterator>;

    using key_container_type    = KeyContainer;
    using mapped_container_type = MappedContainer;

private:
    class get_fn {
    public:
        constexpr get_fn(const Key& key)
            : m_key(key)
        {}

        [[nodiscard]] constexpr
        const Key& operator()(const value_type& value) const {
            return value.first;
        }

    private:
        const Key& m_key;
    };

public:

    constexpr flat_map() = default;

    constexpr flat_map(std::initializer_list<value_type> ilist) {
        range_initialize(std::begin(ilist), std::end(ilist));
    }

    [[nodiscard]] constexpr bool empty()                          const noexcept { return m_keys.empty(); }
    [[nodiscard]] constexpr size_type size()                      const noexcept { return m_keys.size(); }
    [[nodiscard]] constexpr const key_container_type& keys()      const noexcept { return m_keys; }
    [[nodiscard]] constexpr const mapped_container_type& values() const noexcept { return m_values; }

private:

    /**
     * @brief   Find the key/value pair possibly in `log(n)` time.
     *
     * @return  The iterator and whether it matches find the requested key.
     *
     * Note: if the result is unsuccessful it is undefined behaviour to
     * dereference the returned iterator. It might, or might not be pointing to
     * the end iterator.
     */
    [[nodiscard]] constexpr auto at_impl(const key_type& key)
            -> std::pair<iterator, bool>
    {
        const auto iter_key = std::lower_bound(
            std::begin(m_keys),
            std::end(m_keys),
            key
        );

        const auto iter_value = std::next(
            std::begin(m_values),
            std::distance(std::begin(m_keys), iter_key)
        );

        if (iter_key == std::end(m_keys) || *iter_key != key) {
            return { iterator{ iter_key, iter_value }, false };
        }

        return { iterator{ iter_key, iter_value }, true };
    }

    /**
     * @brief   Find the key/value pair possibly in `log(n)` time.
     *
     * @return  The iterator and whether it matches find the requested key.
     *
     * Note: if the result is unsuccessful it is undefined behaviour to
     * dereference the returned iterator. It might, or might not be pointing to
     * the end iterator.
     */
    [[nodiscard]] constexpr auto at_impl(const key_type& key) const
            -> std::pair<const_iterator, bool>
    {
        // TODO(refact): deducing this
        const auto iter_key = std::lower_bound(
            std::begin(m_keys),
            std::end(m_keys),
            key
        );

        const auto iter_value = std::next(
            std::begin(m_values),
            std::distance(std::begin(m_keys), iter_key)
        );

        if (iter_key == std::end(m_keys) || *iter_key != key) {
            return { const_iterator{ iter_key, iter_value }, false };
        }

        return { const_iterator{ iter_key, iter_value }, true };
    }

    /**
     * @brief   Return the iterator if it is valid.
     */
    [[nodiscard]] constexpr auto checked_at(const key_type& key) -> iterator {
        const auto [it, success] = at_impl(key);
        if (not success) {
            throw std::out_of_range("flat_map out of range");
        }
        return it;
    }

    /**
     * @brief   Return the iterator if it is valid.
     */
    [[nodiscard]] constexpr auto checked_at(const key_type& key) const -> const_iterator {
        const auto [it, success] = at_impl(key);
        if (not success) {
            throw std::out_of_range("flat_map out of range");
        }
        return it;
    }

public:

    /**
     * @brief   Return the associated value for the `key`.
     */
    [[nodiscard]] constexpr const mapped_type& at(const key_type& key) {
        return checked_at(key).value().operator*();
    }

    /**
     * @brief   Return the associated value for the `key`.
     */
    [[nodiscard]] constexpr const mapped_type& at(const key_type& key) const {
        return checked_at(key).value().operator*();
    }

    /**
     * @brief   Insert a value. Does not overwrite associated values.
     *
     * @return  Iterator pointing to the inserted value and if its a freshly
     *          inserted value.
     */
    constexpr std::pair<iterator, bool> insert(const std::pair<Key, T>& value) {
        auto [it, success] = at_impl(value.first);
        if (success) {
            return { it, false };
        }

        const auto iter_key = m_keys.insert(it.key(), value.first);
        const auto iter_value = m_values.insert(it.value(), value.second);

        return { iterator{ iter_key, iter_value }, true };
    }

    [[nodiscard]] constexpr mapped_type& operator[](const key_type& key) {
        return insert({ key, T{} }).first.value().operator*();
    }

    [[nodiscard]] constexpr iterator begin() noexcept {
        return { std::begin(m_keys), std::begin(m_values) };
    }

    [[nodiscard]] constexpr iterator end() noexcept {
        return { std::end(m_keys), std::end(m_values) };
    }

    [[nodiscard]] constexpr const_iterator begin() const noexcept {
        return { std::begin(m_keys), std::begin(m_values) };
    }

    [[nodiscard]] constexpr const_iterator end() const noexcept {
        return { std::end(m_keys), std::end(m_values) };
    }

    [[nodiscard]] constexpr const_iterator cbegin() const noexcept {
        return { std::cbegin(m_keys), std::cbegin(m_values) };
    }

    [[nodiscard]] constexpr const_iterator cend() const noexcept {
        return { std::cend(m_keys), std::cend(m_values) };
    }

    [[nodiscard]] constexpr reverse_iterator rbegin() noexcept {
        return { std::rbegin(m_keys), std::rbegin(m_values) };
    }

    [[nodiscard]] constexpr reverse_iterator rend() noexcept {
        return { std::rend(m_keys), std::rend(m_values) };
    }

    [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept {
        return { std::rbegin(m_keys), std::rbegin(m_values) };
    }

    [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept {
        return { std::rend(m_keys), std::rend(m_values) };
    }

    [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept {
        return { std::crbegin(m_keys), std::crbegin(m_values) };
    }

    [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept {
        return { std::crend(m_keys), std::crend(m_values) };
    }

private:
    KeyContainer m_keys;
    MappedContainer m_values;

    template <class Iter>
    constexpr void range_initialize(Iter first, Iter last) {
        if constexpr (is_std_array_v<key_container_type>) {
            std::size_t index = 0;
            for (; first != last; ++first) {
                m_keys[index] = first->first;
                m_values[index] = first->second;
                ++index;
            }
        }
        else {
            for (; first != last; ++first) {
                m_keys.push_back(first->first);
                m_values.push_back(first->second);
            }
        }
    }
};

template <typename Key,
          typename T,
          std::size_t Capacity,
          typename Compare = std::less<Key>
>
using static_map = flat_map<Key, T, Compare, std::array<Key, Capacity>, std::array<T, Capacity>>;

} // namespace nova

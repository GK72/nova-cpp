#pragma once

#include <libnova/error.hpp>

#include <cstddef>
#include <type_traits>

namespace nova {

namespace detail {

    template <typename T>
    struct is_pointer_like : std::false_type {};

    template <typename T>
        requires std::is_convertible_v<decltype(std::declval<T>() != nullptr), bool>
    struct is_pointer_like<T> : std::true_type {};

    template <typename T>
    constexpr inline bool is_pointer_like_v = is_pointer_like<T>::value;

    template <typename T> concept pointer_like = is_pointer_like_v<T>;


    /**
     * @brief   Optimized Return Type.
     *
     */
    template <typename T>
    using ort = std::conditional_t<
        sizeof(T) < 2 * sizeof(void*)
            and std::is_trivially_copy_constructible_v<T>,
        const T,
        const T&
    >;


} // namepsace detail

/**
 * @brief   A wrapper around pointers that checks/enforces that the underlying
 *          pointer is not null.
 *
 */
template <detail::pointer_like T>
class not_null {
public:
    /**
     * @brief   Construct from a pointer-like object with asserting non-null.
     *
     * Define `NOVA_RUNTIME_ASSERTIONS` macro for hard assert that throws exception.
     * Else it is a native `assert` call.
    */
    constexpr not_null(T ptr)
        : m_ptr(std::move(ptr))
    {
        nova_assert(m_ptr != nullptr);
    }

    constexpr not_null(std::nullptr_t)             = delete;
    constexpr not_null& operator=(std::nullptr_t)  = delete;

    constexpr not_null(const not_null&)            = default;
    constexpr not_null& operator=(const not_null&) = default;
    constexpr not_null(not_null&&)                 = default;
    constexpr not_null& operator=(not_null&&)      = default;
    constexpr ~not_null()                          = default;

    [[nodiscard]] constexpr auto get()        const -> detail::ort<T> { return m_ptr; }
    [[nodiscard]] constexpr operator T()      const                   { return get(); }
    [[nodiscard]] constexpr auto operator->() const -> decltype(auto) { return get(); }
    [[nodiscard]] constexpr auto operator*()  const -> decltype(auto) { return *get(); }

private:
    T m_ptr;
};

} // namespace nova

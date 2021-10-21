#pragma once

#include <type_traits>

namespace utl {

namespace detail {

template <class Default
         ,class AlwaysVoid
         ,template <class...> class Op
         ,class... Args>
struct detector {
    using value_t = std::false_type;
    using type    = Default;
};

template <class Default
         ,template <class...> class MetaFunc
         ,class... Args>
struct detector<Default, std::void_t<MetaFunc<Args...>>, MetaFunc, Args...> {
    using value_t = std::true_type;
    using type = MetaFunc<Args...>;
};

// Not A Type
struct NAT {
    NAT()                      = delete;
    ~NAT()                     = delete;
    NAT(const NAT&)            = delete;
    NAT(NAT&&)                 = delete;
    void operator=(const NAT&) = delete;
    void operator=(NAT&&)      = delete;
};

} // namespace detail

// ------------------------------==[ Detector meta functions ]==------------------------------------

template <template <class...> class MetaFunc
         ,class... Args>
using is_detected = typename detail::detector<detail::NAT, void, MetaFunc, Args...>::value_t;

template <template <class...> class MetaFunc
         ,class... Args>
using detected_t = typename detail::detector<detail::NAT, void, MetaFunc, Args...>::type;

template <class Default
         ,template <class...> class MetaFunc
         ,class... Args>
using detected_or = detail::detector<Default, void, MetaFunc, Args...>;

template <class Default
         ,template <class...> class MetaFunc
         ,class... Args>
using detected_or_t = typename detected_or<Default, MetaFunc, Args...>::type;

template <class Expected
         ,template <class...> class MetaFunc
         ,class... Args>
using is_detected_exact = std::is_same<Expected, detected_t<MetaFunc, Args...>>;

// ----------------------------------==[ Helper variables ]==---------------------------------------

template <template <class...> class MetaFunc
         ,class... Args>
inline constexpr bool is_detected_v =
    is_detected<MetaFunc, Args...>::value;

template <class Expected
         ,template <class...> class MetaFunc
         ,class... Args>
inline constexpr bool is_detected_exact_v =
    std::is_same<Expected, detected_t<MetaFunc, Args...>>::value;


} // namespace utl

#pragma once

namespace nova {

template <typename T>
struct range {
    T low;
    T high;
};

template <typename T1, typename T2 = T1>
struct extent {
    T1 pos;
    T2 len;
};

template <typename T1, typename T2 = T1>
extent(T1 pos, T2 len) -> extent<T1, T2>;

} // namespace nova

#pragma once

// User-defined literals to express various time units in milliseconds easily
constexpr unsigned long long operator "" _ms(unsigned long long milliseconds) {
    return milliseconds;
}

constexpr unsigned long long operator "" _s(unsigned long long seconds) {
    return seconds * 1000_ms;
}

constexpr unsigned long long operator "" _m(unsigned long long minutes) {
    return minutes * 60_s;
}

constexpr unsigned long long operator "" _h(unsigned long long hours) {
    return hours * 60_m;
}

// Type-safe array size helper
template<typename T, size_t N>
constexpr size_t array_size(T (&)[N]) {
    return N;
}

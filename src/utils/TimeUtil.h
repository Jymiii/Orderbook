#pragma once

#include <ctime>

inline bool safe_localtime(const std::time_t *t, std::tm *out) {
#if defined(_WIN32)
    return localtime_s(out, t) == 0;
#else
    return localtime_r(t, out) != nullptr;
#endif
}

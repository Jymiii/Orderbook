//
// Created by jimiv on 3-3-2026.
//

#ifndef TIMEUTIL_H
#define TIMEUTIL_H
#include <ctime>

inline bool safe_localtime(const std::time_t* t, std::tm* out) {
#if defined(_WIN32)
    // Windows/MSVC-style safe function
    return localtime_s(out, t) == 0;
#else
    // POSIX
    return localtime_r(t, out) != nullptr;
#endif
}
#endif //TIMEUTIL_H

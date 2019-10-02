#ifndef MATHFUNC_H
#define MATHFUNC_H

#include "pg_config.h"

#include <cstdint>

namespace pg_math {
    inline std::uint32_t round_positive(real n) {
        return static_cast<std::uint32_t>(n + 0.5f);
    }

    inline std::int32_t round(real n) {
        return n > 0 ? static_cast<std::int32_t>(n + 0.5f) : -static_cast<std::int32_t>(0.5f - n);
    }
}

#endif // MATHFUNC_H

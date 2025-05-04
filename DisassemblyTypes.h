#pragma once

#include "SidblasterUtils.h"

namespace sidblaster {

    /**
     * @struct RelocEntry
     * @brief Information about a relocated byte
     */
    struct RelocEntry {
        u16 effectiveAddr;
        enum class Type { Low, High } type;
    };

} // namespace sidblaster
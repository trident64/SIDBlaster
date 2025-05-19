// ==================================
//             SIDBlaster
//
//  Raistlin / Genesis Project (G*P)
// ==================================
#pragma once

#include "SIDBlasterUtils.h"

/**
 * @file DisassemblyTypes.h
 * @brief Common types used in the disassembly process
 *
 * Defines structures and types needed for tracking
 * relocation information and other disassembly-related data.
 */

namespace sidblaster {

    /**
     * @struct RelocEntry
     * @brief Information about a relocated byte in memory
     *
     * Used to track low and high bytes of addresses when
     * generating relocatable code in the disassembly.
     */
    struct RelocEntry {
        u16 targetAddr;  // The actual address being pointed to
        enum class Type { Low, High } type;  // Whether this is a low or high byte
    };

} // namespace sidblaster
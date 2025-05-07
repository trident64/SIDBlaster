#pragma once

#include "Common.h"
#include "app/TraceLogger.h"
#include <functional>
#include <memory>

class CPU6510;
class SIDLoader;

namespace sidblaster {

    /**
     * @class SIDEmulator
     * @brief Unified SID emulation functionality
     *
     * Provides a consistent interface for SID emulation across the application,
     * eliminating duplicated code and providing a single point of control.
     */
    class SIDEmulator {
    public:
        /**
         * @struct EmulationOptions
         * @brief Configuration options for SID emulation
         */
        struct EmulationOptions {
            int frames = DEFAULT_SID_EMULATION_FRAMES;   ///< Number of frames to emulate
            bool backupAndRestore = true;                ///< Whether to backup/restore memory
            bool traceEnabled = false;                   ///< Whether to generate trace logs
            TraceFormat traceFormat = TraceFormat::Binary; ///< Format for trace logs
            std::string traceLogPath;                    ///< Path for trace log (if enabled)
            int callsPerFrame = 1;                       ///< Calls to play routine per frame
        };

        /**
         * @brief Constructor
         * @param cpu Pointer to CPU instance
         * @param sid Pointer to SID loader
         */
        SIDEmulator(CPU6510* cpu, SIDLoader* sid);

        /**
         * @brief Run SID emulation
         * @param options Emulation options
         * @return True if emulation completed successfully
         */
        bool runEmulation(const EmulationOptions& options);

        /**
         * @brief Get cycle count per frame statistics
         * @return Pair of average and maximum cycles per frame
         */
        std::pair<u64, u64> getCycleStats() const;

    private:
        CPU6510* cpu_;                 ///< CPU instance
        SIDLoader* sid_;               ///< SID loader
        std::unique_ptr<TraceLogger> traceLogger_; ///< Trace logger (if enabled)
        u64 totalCycles_ = 0;          ///< Total cycles used
        u64 maxCyclesPerFrame_ = 0;    ///< Maximum cycles used in a frame
        int framesExecuted_ = 0;       ///< Number of frames executed
    };

} // namespace sidblaster
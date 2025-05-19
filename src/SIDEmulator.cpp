#include "SIDEmulator.h"
#include "cpu6510.h"
#include "SIDLoader.h"
#include "SIDBlasterUtils.h"

namespace sidblaster {

    SIDEmulator::SIDEmulator(CPU6510* cpu, SIDLoader* sid)
        : cpu_(cpu), sid_(sid) {
    }

    bool SIDEmulator::runEmulation(const EmulationOptions& options) {
        if (!cpu_ || !sid_) {
            util::Logger::error("Invalid CPU or SID loader for emulation");
            return false;
        }

        // Set up trace logger if enabled
        if (options.traceEnabled && !options.traceLogPath.empty()) {
            traceLogger_ = std::make_unique<TraceLogger>(options.traceLogPath, options.traceFormat);

            // Set up callback for SID writes
            cpu_->setOnSIDWriteCallback([this](u16 addr, u8 value) {
                traceLogger_->logSIDWrite(addr, value);
                });

            // Set up callback for CIA writes
            cpu_->setOnCIAWriteCallback([this](u16 addr, u8 value) {
                traceLogger_->logCIAWrite(addr, value);
                });

            util::Logger::debug("Trace logging enabled to: " + options.traceLogPath);
        }
        else
        {
            traceLogger_.reset();
            cpu_->reset();
        }

        // Create a backup of memory
        sid_->backupMemory();

        // Initialize the SID
        const u16 initAddr = sid_->getInitAddress();
        const u16 playAddr = sid_->getPlayAddress();

        util::Logger::debug("Running SID emulation - Init: $" + util::wordToHex(initAddr) +
            ", Play: $" + util::wordToHex(playAddr) +
            ", Frames: " + std::to_string(options.frames));

        // Execute the init routine once
        cpu_->resetRegistersAndFlags();
        cpu_->executeFunction(initAddr);

        // Run a short playback period to identify initial memory patterns
        // This helps with memory copies performed during initialization
        const int preAnalysisFrames = 100;  // A small number of frames for initial analysis
        for (int frame = 0; frame < preAnalysisFrames; ++frame) {
            for (int call = 0; call < options.callsPerFrame; ++call) {
                cpu_->resetRegistersAndFlags();
                if (!cpu_->executeFunction(playAddr)) {
                    return false;
                }
            }

            // Mark end of frame in trace log
            if (traceLogger_) {
                traceLogger_->logFrameMarker();
            }
        }

        // Re-run the init routine to reset the player state
        cpu_->resetRegistersAndFlags();
        cpu_->executeFunction(initAddr);

        // Mark end of initialization in trace log
        if (traceLogger_) {
            traceLogger_->logFrameMarker();
        }

        // Reset counters
        totalCycles_ = 0;
        maxCyclesPerFrame_ = 0;
        framesExecuted_ = 0;

        // Get initial cycle count
        u64 lastCycles = cpu_->getCycles();

        // Call play routine for the specified number of frames
        bool bGood = true;
        for (int frame = 0; frame < options.frames; ++frame) {
            // Execute play routine (multiple times per frame if requested)
            for (int call = 0; call < options.callsPerFrame; ++call) {
                cpu_->resetRegistersAndFlags();
                bGood = cpu_->executeFunction(playAddr);
                if (!bGood)
                {
                    break;
                }
            }
            if (!bGood)
            {
                break;
            }

            // Calculate cycles used in this frame
            const u64 curCycles = cpu_->getCycles();
            const u64 frameCycles = curCycles - lastCycles;

            // Update statistics
            maxCyclesPerFrame_ = std::max(maxCyclesPerFrame_, frameCycles);
            totalCycles_ += frameCycles;
            lastCycles = curCycles;

            // Mark end of frame in trace log
            if (traceLogger_) {
                traceLogger_->logFrameMarker();
            }
            framesExecuted_++;
        }

        // Log cycle stats
        const u64 avgCycles = options.frames > 0 ? totalCycles_ / options.frames : 0;
        util::Logger::debug("SID emulation complete - Average cycles per frame: " +
            std::to_string(avgCycles) + ", Maximum: " + std::to_string(maxCyclesPerFrame_));

        // Restore original memory
        sid_->restoreMemory();

        return true;
    }

    std::pair<u64, u64> SIDEmulator::getCycleStats() const {
        const u64 avgCycles = framesExecuted_ > 0 ? totalCycles_ / framesExecuted_ : 0;
        return { avgCycles, maxCyclesPerFrame_ };
    }

} // namespace sidblaster
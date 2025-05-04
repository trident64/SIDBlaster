#pragma once

#include "SidblasterUtils.h"

#include <functional>
#include <memory>
#include <string>

// Forward declarations
class CPU6510;
class SIDLoader;

namespace sidblaster {

    // Forward declarations
    class MemoryAnalyzer;
    class LabelGenerator;
    class CodeFormatter;
    class DisassemblyWriter;

    /**
     * @class Disassembler
     * @brief High-level class for disassembling SID files
     */
    class Disassembler {
    public:
        /**
         * @brief Constructor
         * @param cpu Reference to the CPU
         * @param sid Reference to the SID loader
         */
        Disassembler(const CPU6510& cpu, const SIDLoader& sid);

        /**
         * @brief Destructor
         */
        ~Disassembler();

        /**
         * @brief Generate an assembly file from the loaded SID
         * @param outputPath Path to write the assembly file
         * @param sidLoad New SID load address
         * @param sidInit New SID init address (relative to load)
         * @param sidPlay New SID play address (relative to load)
         * @return Number of unused bytes removed, or -1 on error
         */
        int generateAsmFile(
            const std::string& outputPath,
            u16 sidLoad,
            u16 sidInit,
            u16 sidPlay);

        /**
         * @brief Set the callback for indirect memory reads
         * @param callback Function to call on indirect reads
         */
        void setIndirectReadCallback(
            std::function<void(u16 pc, u8 zpAddr, u16 effectiveAddr)> callback);

    private:
        const CPU6510& cpu_;
        const SIDLoader& sid_;

        std::unique_ptr<MemoryAnalyzer> analyzer_;
        std::unique_ptr<LabelGenerator> labelGenerator_;
        std::unique_ptr<CodeFormatter> formatter_;
        std::unique_ptr<DisassemblyWriter> writer_;

        std::function<void(u16 pc, u8 zpAddr, u16 effectiveAddr)> indirectReadCallback_;

        /**
         * @brief Initialize the disassembler components
         */
        void initialize();
    };

} // namespace sidblaster
// ==================================
//             SIDBlaster
//
//  Raistlin / Genesis Project (G*P)
// ==================================
#pragma once

#include "CodeFormatter.h"
#include "DisassemblyTypes.h"
#include "LabelGenerator.h"
#include "MemoryAnalyzer.h"
#include "SIDBlasterUtils.h"

#include <fstream>
#include <map>
#include <string>
#include <vector>

/**
 * @file DisassemblyWriter.h
 * @brief Writes formatted disassembly to output files
 *
 * This module handles the high-level writing of disassembled code to
 * assembly files, managing the overall structure and organization of
 * the output.
 */

 // Forward declarations
class SIDLoader;
class CPU6510;

namespace sidblaster {

    /**
     * @struct RelocationInfo
     * @brief Information about a relocated byte
     *
     * Used to track address relocations during the disassembly process.
     */
    struct RelocationInfo {
        u16 effectiveAddr;                  // Target address being referenced
        enum class Type { Low, High } type; // Whether this is a low or high byte
    };

    /**
     * @class DisassemblyWriter
     * @brief Writes disassembled code to an output file
     *
     * Coordinates the entire process of writing a disassembly to an
     * assembly language file, including header comments, constants,
     * and the structured output of code and data sections.
     */
    class DisassemblyWriter {
    public:
        /**
         * @brief Constructor
         * @param cpu Reference to the CPU
         * @param sid Reference to the SID loader
         * @param analyzer Reference to the memory analyzer
         * @param labelGenerator Reference to the label generator
         * @param formatter Reference to the code formatter
         */
        DisassemblyWriter(
            const CPU6510& cpu,
            const SIDLoader& sid,
            const MemoryAnalyzer& analyzer,
            const LabelGenerator& labelGenerator,
            const CodeFormatter& formatter);

        /**
         * @brief Generate an assembly file
         * @param filename Output filename
         * @param sidLoad New SID load address
         * @param sidInit New SID init address
         * @param sidPlay New SID play address
         * @return Number of unused bytes removed
         *
         * Creates a complete assembly language file for the disassembled SID.
         */
        int generateAsmFile(
            const std::string& filename,
            u16 sidLoad,
            u16 sidInit,
            u16 sidPlay);

        /**
         * @brief Add a relocation byte
         * @param address Address of the byte
         * @param info Relocation information
         *
         * Registers a byte as a relocation point (address reference).
         */
        void addRelocationByte(u16 address, const RelocationInfo& info);

        /**
         * @brief Add an indirect memory access
         * @param pc Program counter
         * @param zpAddr Zero page address
         * @param effectiveAddr Effective address
         *
         * Tracks indirect memory accesses for later analysis.
         */
        void addIndirectAccess(u16 pc, u8 zpAddr, u16 effectiveAddr);

        /**
         * @brief Process all recorded indirect accesses to identify relocation bytes
         *
         * Analyzes indirect access patterns to identify address references.
         */
        void processIndirectAccesses();

    private:
        const CPU6510& cpu_;                      // Reference to CPU
        const SIDLoader& sid_;                    // Reference to SID loader
        const MemoryAnalyzer& analyzer_;          // Reference to memory analyzer
        const LabelGenerator& labelGenerator_;    // Reference to label generator
        const CodeFormatter& formatter_;          // Reference to code formatter

        std::map<u16, RelocationInfo> relocationBytes_;  // Map of bytes that need relocation

        /**
         * @brief Struct for tracking indirect memory accesses
         *
         * Records detailed information about indirect memory access patterns
         * to identify address references and pointer tables.
         */
        struct IndirectAccessInfo {
            u16 instructionAddress;   // Address of the instruction
            u8 zpAddr;                // Zero page pointer address (low byte)
            u8 zpPairAddr;            // Zero page pointer address (high byte)
            u16 lastWriteLow;         // Address of last write to low byte
            u16 lastWriteHigh;        // Address of last write to high byte
            u16 sourceLowAddress;     // Source of the low byte value
            u16 sourceHighAddress;    // Source of the high byte value
            u16 effectiveAddress;     // Effective address accessed

            // Track if these pointer sources have been marked for relocation
            bool sourceLowMarked = false;
            bool sourceHighMarked = false;
        };

        std::vector<IndirectAccessInfo> indirectAccesses_;  // List of indirect accesses

        /**
         * @brief Output hardware constants to the assembly file
         * @param file Output stream
         *
         * Writes hardware-related constant definitions.
         */
        void outputHardwareConstants(std::ofstream& file);

        /**
         * @brief Output zero page definitions to the assembly file
         * @param file Output stream
         *
         * Writes zero page variable definitions.
         */
        void emitZPDefines(std::ofstream& file);

        /**
         * @brief Disassemble to the output file
         * @param file Output stream
         * @return Number of unused bytes removed
         *
         * Performs the actual disassembly writing to the file.
         */
        int disassembleToFile(std::ofstream& file);

        /**
         * @brief Propagate relocation sources
         *
         * Analyzes and propagates relocation information across
         * the disassembly to ensure consistent address references.
         */
        void propagateRelocationSources();

        /**
         * @brief Trace memory dependencies backward to find pointer table sources
         *
         * This method performs a multi-pass analysis to trace the origins of
         * data used in indirect addressing, even when it's copied through
         * multiple memory locations before being used.
         */
        void tracePointerSources();
    };

} // namespace sidblaster
// ==================================
//             SIDBlaster
//
//  Raistlin / Genesis Project (G*P)
// ==================================
#pragma once

#include "cpu6510.h"
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
     * @struct RelocationEntry
     * @brief Information about a memory location that needs relocation
     */
    struct RelocationEntry {
        u16 targetAddress;          // The effective address being pointed to
        enum class Type {
            Low,                    // Low byte of address
            High                    // High byte of address
        } type;

        std::string toString() const {
            return std::string(type == Type::Low ? "LOW" : "HIGH") +
                " byte of $" + util::wordToHex(targetAddress);
        }
    };

    /**
     * @class RelocationTable
     * @brief Central registry of all memory locations that need relocation
     */
    class RelocationTable {
    public:
        /**
         * @brief Add a relocation entry
         * @param addr Memory address to mark for relocation
         * @param targetAddr The target address it points to
         * @param type Whether this is a low or high byte
         */
        void addEntry(u16 addr, u16 targetAddr, RelocationEntry::Type type) {
            entries_[addr] = { targetAddr, type };
        }

        /**
         * @brief Check if an address needs relocation
         * @param addr Memory address to check
         * @return True if address is marked for relocation
         */
        bool hasEntry(u16 addr) const {
            return entries_.find(addr) != entries_.end();
        }

        /**
         * @brief Get the relocation entry for an address
         * @param addr Memory address
         * @return The relocation entry or nullptr if not found
         */
        const RelocationEntry* getEntry(u16 addr) const {
            auto it = entries_.find(addr);
            if (it != entries_.end()) {
                return &it->second;
            }
            return nullptr;
        }

        /**
         * @brief Get all relocation entries
         * @return The map of addresses to relocation entries
         */
        const std::map<u16, RelocationEntry>& getAllEntries() const {
            return entries_;
        }

        /**
         * @brief Clear all entries
         */
        void clear() {
            entries_.clear();
        }

        /**
         * @brief Dump the relocation table to a file
         * @param filename Output file path
         */
        void dumpToFile(const std::string& filename) const {
            std::ofstream file(filename);
            if (!file) {
                util::Logger::error("Failed to open relocation table dump file: " + filename);
                return;
            }

            file << "===== RELOCATION TABLE =====\n\n";
            file << "Format: address -> target (type)\n\n";

            for (const auto& [addr, entry] : entries_) {
                file << "$" << util::wordToHex(addr) << " -> $"
                    << util::wordToHex(entry.targetAddress) << " ("
                    << (entry.type == RelocationEntry::Type::Low ? "LOW" : "HIGH") << ")\n";
            }

            file.close();
            util::Logger::info("Relocation table written to: " + filename);
        }

    private:
        std::map<u16, RelocationEntry> entries_;
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
            u16 instructionAddress = 0;   // Address of the instruction
            u8 zpAddr = 0;                // Zero page pointer address (low byte)
            u16 lastWriteLow = 0;         // Address of last write to low byte
            u16 lastWriteHigh = 0;        // Address of last write to high byte
            u16 sourceLowAddress = 0;     // Source of the low byte value
            u16 sourceHighAddress = 0;    // Source of the high byte value
            u16 targetAddress = 0;        // Effective address targetted
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
         * @brief Build the relocation table from indirect accesses and data flow
         *
         * Analyzes all indirect memory accesses and traces data flow chains
         * to build a consolidated table of all addresses needing relocation.
         */
        void buildRelocationTable();

        void processRelocationChain(const MemoryDataFlow& dataFlow, RelocationTable& relocTable, u16 addr, u16 targetAddr, RelocationEntry::Type relocType);

    };

} // namespace sidblaster
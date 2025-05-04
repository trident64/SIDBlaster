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

// Forward declarations
class SIDLoader;
class CPU6510;

namespace sidblaster {

    /**
     * @struct RelocationInfo
     * @brief Information about a relocated byte
     */
    struct RelocationInfo {
        u16 effectiveAddr;
        enum class Type { Low, High } type;
    };

    /**
     * @class DisassemblyWriter
     * @brief Writes disassembled code to an output file
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
         */
        void addRelocationByte(u16 address, const RelocationInfo& info);

        /**
         * @brief Add an indirect memory access
         * @param pc Program counter
         * @param zpAddr Zero page address
         * @param effectiveAddr Effective address
         */
        void addIndirectAccess(u16 pc, u8 zpAddr, u16 effectiveAddr);

        /**
         * @brief Process all recorded indirect accesses to identify relocation bytes
         */
        void processIndirectAccesses();

    private:
        const CPU6510& cpu_;
        const SIDLoader& sid_;
        const MemoryAnalyzer& analyzer_;
        const LabelGenerator& labelGenerator_;
        const CodeFormatter& formatter_;

        std::map<u16, RelocationInfo> relocationBytes_;

        /**
         * @brief Struct for tracking indirect memory accesses
         */
        struct IndirectAccessInfo {
            u16 instructionAddress;
            u8 zpAddr;
            u8 zpPairAddr;
            u16 lastWriteLow;
            u16 lastWriteHigh;
            u16 sourceLowAddress;
            u16 sourceHighAddress;
            u16 effectiveAddress;
        };

        std::vector<IndirectAccessInfo> indirectAccesses_;

        /**
         * @brief Output hardware constants to the assembly file
         * @param file Output stream
         */
        void outputHardwareConstants(std::ofstream& file);

        /**
         * @brief Output zero page definitions to the assembly file
         * @param file Output stream
         */
        void emitZPDefines(std::ofstream& file);

        /**
         * @brief Disassemble to the output file
         * @param file Output stream
         * @return Number of unused bytes removed
         */
        int disassembleToFile(std::ofstream& file);

        /**
         * @brief Propagate relocation sources
         */
        void propagateRelocationSources();
    };

} // namespace sidblaster
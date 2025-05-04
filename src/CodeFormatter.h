#pragma once

#include "DisassemblyTypes.h"
#include "LabelGenerator.h"
#include "SidblasterUtils.h"

#include <memory>
#include <span>
#include <string>
#include <vector>

// Forward declarations
class CPU6510;

namespace sidblaster {

    /**
     * @class CodeFormatter
     * @brief Formats disassembled instructions and data
     */
    class CodeFormatter {
    public:
        /**
         * @brief Constructor
         * @param cpu Reference to the CPU
         * @param labelGenerator Reference to the label generator
         * @param memory Span of memory data
         */
        CodeFormatter(
            const CPU6510& cpu,
            const LabelGenerator& labelGenerator,
            std::span<const u8> memory);

        /**
         * @brief Format a disassembled instruction
         * @param pc Program counter (will be updated)
         * @return Formatted instruction string
         */
        std::string formatInstruction(u16& pc) const;

        /**
         * @brief Format data bytes
         * @param file Output stream
         * @param pc Program counter (will be updated)
         * @param originalMemory Original memory data
         * @param originalBase Base address of original memory
         * @param endAddress End address
         * @param relocationBytes Map of relocation bytes
         * @param memoryTags Memory type tags
         * @return Number of unused bytes zeroed out
         */
        int formatDataBytes(
            std::ostream& file,
            u16& pc,
            std::span<const u8> originalMemory,
            u16 originalBase,
            u16 endAddress,
            const std::map<u16, RelocEntry>& relocationBytes,
            std::span<const MemoryType> memoryTags) const;

        /**
         * @brief Check if a store instruction is a CIA timer patch
         * @param opcode Instruction opcode
         * @param mode Addressing mode
         * @param operand Operand address
         * @param mnemonic Instruction mnemonic
         * @return True if this is a CIA timer patch
         */
        bool isCIAStorePatch(
            u8 opcode,
            int mode,
            u16 operand,
            std::string_view mnemonic) const;

        /**
         * @brief Format an instruction operand
         * @param pc Program counter
         * @param mode Addressing mode
         * @return Formatted operand string
         */
        std::string formatOperand(u16 pc, int mode) const;

        /**
         * @brief Format an indexed address with minimum offset
         * @param baseAddr Base address
         * @param minOffset Minimum offset
         * @param indexReg Index register ('X' or 'Y')
         * @return Formatted address string
         */
        std::string formatIndexedAddressWithMinOffset(
            u16 baseAddr,
            u8 minOffset,
            char indexReg) const;

        /**
         * @brief Format a SID register with its name
         * @param addr SID register address
         * @param usedBases Vector of used SID base addresses
         * @return Formatted register name
         */
        std::string formatSIDRegister(
            u16 addr,
            const std::vector<u16>& usedBases) const;

        /**
         * @brief Get the name of a SID register
         * @param offset Register offset
         * @return Register name
         */
        std::string getSIDRegisterName(u8 offset) const;

    private:
        const CPU6510& cpu_;
        const LabelGenerator& labelGenerator_;
        std::span<const u8> memory_;
        std::vector<u16> usedSidBases_;
    };

} // namespace sidblaster
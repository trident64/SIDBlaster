#include "CodeFormatter.h"
#include "cpu6510.h"

#include <algorithm>
#include <sstream>

namespace sidblaster {

    CodeFormatter::CodeFormatter(
        const CPU6510& cpu,
        const LabelGenerator& labelGenerator,
        std::span<const u8> memory)
        : cpu_(cpu),
        labelGenerator_(labelGenerator),
        memory_(memory) {
        // No configuration loading needed anymore
    }

    std::string CodeFormatter::formatInstruction(u16& pc) const {
        std::ostringstream line;

        const u8 opcode = memory_[pc];
        const std::string mnemonic = std::string(cpu_.getMnemonic(opcode));
        const auto mode = cpu_.getAddressingMode(opcode);
        const int size = cpu_.getInstructionSize(opcode);

        // Check for CIA timer patch
        if (static_cast<int>(mode) == static_cast<int>(AddressingMode::Absolute)) {
            const u16 absAddr = memory_[pc + 1] | (memory_[pc + 2] << 8);
            if (isCIAStorePatch(opcode, static_cast<int>(mode), absAddr, mnemonic)) {
                std::ostringstream patched;
                patched << "    bit $abcd   //; disabled " << mnemonic << " $"
                    << util::wordToHex(absAddr) << " (CIA Timer)";
                pc += size;
                return patched.str();
            }
        }

        // Format instruction with mnemonic
        line << "    " << mnemonic;

        // Add operand if needed
        if (size > 1) {
            line << " " << formatOperand(pc, static_cast<int>(mode));
        }

        // Update PC
        pc += size;

        return line.str();
    }

    int CodeFormatter::formatDataBytes(
        std::ostream& file,
        u16& pc,
        std::span<const u8> originalMemory,
        u16 originalBase,
        u16 endAddress,
        const std::map<u16, RelocEntry>& relocationBytes,
        std::span<const MemoryType> memoryTags) const {

        int unusedByteCount = 0;

        while (pc < endAddress && (memoryTags[pc] & MemoryType::Data)) {
            // Emit label if present
            const std::string label = labelGenerator_.getLabel(pc);
            if (!label.empty()) {
                file << label << ":\n";
            }

            // Check for relocation byte
            auto relocIt = relocationBytes.find(pc);
            if (relocIt != relocationBytes.end()) {
                const u16 target = relocIt->second.effectiveAddr;
                const std::string targetLabel = labelGenerator_.formatAddress(target);

                file << "    .byte ";
                if (relocIt->second.type == RelocEntry::Type::Low) {
                    file << "<(" << targetLabel << ")";
                }
                else {
                    file << ">(" << targetLabel << ")";
                }
                file << "\n";
                ++pc;
                continue;
            }

            // Emit normal .byte data
            file << "    .byte ";
            int count = 0;

            while (pc < endAddress && (memoryTags[pc] & MemoryType::Data)) {
                // Stop at relocation bytes
                if (relocationBytes.count(pc)) {
                    break;
                }

                // Add comma if not the first byte
                if (count > 0) {
                    file << ", ";
                }

                // Get the byte from original memory if possible
                u8 byte;
                if (pc - originalBase < originalMemory.size()) {
                    byte = originalMemory[pc - originalBase];
                }
                else {
                    byte = memory_[pc];
                }

                // Check if unused (not accessed)
                bool isUnused = false; //; !(memoryTags[pc] & (MemoryType::Accessed | MemoryType::LabelTarget));

                if (isUnused) {
                    byte = 0;  // Always zero out unused bytes to help with compression
                    unusedByteCount++;
                }

                file << "$" << util::byteToHex(byte);

                ++pc;
                ++count;

                // Stop at code or label
                if ((memoryTags[pc] & MemoryType::Code) || !labelGenerator_.getLabel(pc).empty()) {
                    break;
                }

                // Line break after 16 bytes
                if (count == 16) {
                    file << "\n";
                    if (pc < endAddress && (memoryTags[pc] & MemoryType::Data)) {
                        file << "    .byte ";
                    }
                    count = 0;
                }
            }

            file << "\n";
        }

        return unusedByteCount;
    }

    bool CodeFormatter::isCIAStorePatch(
        u8 opcode,
        int mode,
        u16 operand,
        std::string_view mnemonic) const {

        if (mode != static_cast<int>(AddressingMode::Absolute)) {
            return false;
        }

        if (operand != 0xDC04 && operand != 0xDC05) {
            return false;
        }

        return mnemonic == "sta" || mnemonic == "stx" || mnemonic == "sty";
    }

    std::string CodeFormatter::formatOperand(u16 pc, int mode) const {
        const auto addressingMode = static_cast<AddressingMode>(mode);

        switch (addressingMode) {
        case AddressingMode::Immediate: {
            return "#$" + util::byteToHex(memory_[pc + 1]);
        }

        case AddressingMode::ZeroPage: {
            const u8 zp = memory_[pc + 1];
            return labelGenerator_.formatZeroPage(zp);
        }

        case AddressingMode::ZeroPageX: {
            const u8 zp = memory_[pc + 1];
            const std::string baseAddr = labelGenerator_.formatZeroPage(zp);
            return baseAddr + ",X";
        }

        case AddressingMode::ZeroPageY: {
            const u8 zp = memory_[pc + 1];
            const std::string baseAddr = labelGenerator_.formatZeroPage(zp);
            return baseAddr + ",Y";
        }

        case AddressingMode::IndirectX: {
            const u8 zp = memory_[pc + 1];
            const std::string baseAddr = labelGenerator_.formatZeroPage(zp);
            return "(" + baseAddr + ",X)";
        }

        case AddressingMode::IndirectY: {
            const u8 zp = memory_[pc + 1];
            const std::string baseAddr = labelGenerator_.formatZeroPage(zp);
            return "(" + baseAddr + "),Y";
        }

        case AddressingMode::Absolute: {
            const u16 accessAddr = memory_[pc + 1] | (memory_[pc + 2] << 8);
            return labelGenerator_.formatAddress(accessAddr);
        }

        case AddressingMode::AbsoluteX: {
            const u16 baseAddr = memory_[pc + 1] | (memory_[pc + 2] << 8);
            const auto [minIndex, maxIndex] = cpu_.getIndexRange(pc + 1);
            return formatIndexedAddressWithMinOffset(baseAddr, minIndex, 'X');
        }

        case AddressingMode::AbsoluteY: {
            const u16 baseAddr = memory_[pc + 1] | (memory_[pc + 2] << 8);
            const auto [minIndex, maxIndex] = cpu_.getIndexRange(pc + 1);
            return formatIndexedAddressWithMinOffset(baseAddr, minIndex, 'Y');
        }

        case AddressingMode::Indirect: {
            const u16 accessAddr = memory_[pc + 1] | (memory_[pc + 2] << 8);
            return "($" + util::wordToHex(accessAddr) + ")";
        }

        case AddressingMode::Relative: {
            const i8 offset = static_cast<i8>(memory_[pc + 1]);
            const u16 dest = pc + 2 + offset;
            return labelGenerator_.formatAddress(dest);
        }

        default:
            return "";
        }
    }

    std::string CodeFormatter::formatIndexedAddressWithMinOffset(
        u16 baseAddr,
        u8 minOffset,
        char indexReg) const {

        const u16 effectiveAddr = baseAddr + minOffset;
        const std::string label = labelGenerator_.getLabel(effectiveAddr);

        if (!label.empty()) {
            if (minOffset == 0) {
                return label + "," + indexReg;
            }
            else {
                return label + "-" + std::to_string(minOffset) + "," + indexReg;
            }
        }

        return labelGenerator_.formatAddress(baseAddr) + "," + indexReg;
    }

    std::string CodeFormatter::formatSIDRegister(
        u16 addr,
        const std::vector<u16>& usedBases) const {

        const u16 base = addr & 0xffe0; // Align to 32 bytes
        const u8 offset = addr & 0x1f;

        // Find the SID index
        for (size_t i = 0; i < usedBases.size(); ++i) {
            if (usedBases[i] == base) {
                const std::string regName = getSIDRegisterName(offset);
                if (!regName.empty()) {
                    return "SID" + std::to_string(i) + "." + regName;
                }
                else {
                    return "SID" + std::to_string(i) + "+" + std::to_string(offset);
                }
            }
        }

        // Fall back to hex
        return "$" + util::wordToHex(addr);
    }

    std::string CodeFormatter::getSIDRegisterName(u8 offset) const {
        static const std::array<std::string_view, 25> sidRegs = {
            "Voice1FreqLo", "Voice1FreqHi", "Voice1PulseLo", "Voice1PulseHi",
            "Voice1Control", "Voice1AttackDecay", "Voice1SustainRelease",
            "Voice2FreqLo", "Voice2FreqHi", "Voice2PulseLo", "Voice2PulseHi",
            "Voice2Control", "Voice2AttackDecay", "Voice2SustainRelease",
            "Voice3FreqLo", "Voice3FreqHi", "Voice3PulseLo", "Voice3PulseHi",
            "Voice3Control", "Voice3AttackDecay", "Voice3SustainRelease",
            "FilterCutoffLo", "FilterCutoffHi", "FilterResonanceRouting",
            "FilterModeVolume"
        };

        if (offset < sidRegs.size()) {
            return std::string(sidRegs[offset]);
        }

        return "";
    }

} // namespace sidblaster
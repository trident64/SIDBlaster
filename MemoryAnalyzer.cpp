#include "MemoryAnalyzer.h"
#include "SidblasterUtils.h"

namespace sidblaster {

    // Inside a namespace to avoid conflicts - will be used with MemoryAccessFlag in the refactored code
    namespace {
        constexpr u8 MemoryAccess_Execute = 1 << 0;
        constexpr u8 MemoryAccess_Read = 1 << 1;
        constexpr u8 MemoryAccess_Write = 1 << 2;
        constexpr u8 MemoryAccess_JumpTarget = 1 << 3;
        constexpr u8 MemoryAccess_OpCode = 1 << 4;
    }

    MemoryAnalyzer::MemoryAnalyzer(
        std::span<const u8> memory,
        std::span<const u8> memoryAccess,
        u16 startAddress,
        u16 endAddress)
        : memory_(memory),
        memoryAccess_(memoryAccess),
        startAddress_(startAddress),
        endAddress_(endAddress) {

        // Initialize memory types to Unknown
        memoryTypes_.resize(65536, MemoryType::Unknown);
    }

    void MemoryAnalyzer::analyzeExecution() {
        util::Logger::debug("Analyzing execution patterns...");

        int codeCount = 0;
        int jumpCount = 0;

        // For each address in the entire memory
        for (u32 addr = 0; addr < 0x10000; ++addr) {
            // Check if the address has been executed
            if (memoryAccess_[addr] & MemoryAccess_Execute) {
                memoryTypes_[addr] |= MemoryType::Code;
                codeCount++;
            }

            // Check if the address is a jump target
            if (memoryAccess_[addr] & MemoryAccess_JumpTarget) {
                memoryTypes_[addr] |= MemoryType::LabelTarget;
                jumpCount++;
            }
        }

        util::Logger::debug("Execution analysis complete: " +
            std::to_string(codeCount) + " code bytes, " +
            std::to_string(jumpCount) + " jump targets");
    }

    void MemoryAnalyzer::analyzeAccesses() {
        util::Logger::debug("Analyzing memory accesses...");

        // For each address in the memory
        u16 addr = 0;
        bool finished = false;

        while (!finished) {
            // Check if the address has been read or written and is also code
            if ((memoryAccess_[addr] & (MemoryAccess_Read | MemoryAccess_Write)) &&
                (memoryTypes_[addr] & MemoryType::Code)) {

                // Find the instruction that covers this address
                u16 instrStart = findInstructionStartCovering(addr);

                // Mark the instruction start as a label target
                memoryTypes_[instrStart] |= MemoryType::LabelTarget;
            }

            // Check if we've processed all memory
            if (addr == 0xFFFF) {
                finished = true;
            }

            addr++;
        }

        util::Logger::debug("Memory access analysis complete");
    }

    void MemoryAnalyzer::analyzeData() {
        util::Logger::debug("Analyzing data regions...");

        // For each address in the memory
        u16 addr = 0;
        bool finished = false;

        while (!finished) {
            // If this address is not code, mark it as data
            if (!(memoryTypes_[addr] & MemoryType::Code)) {
                memoryTypes_[addr] |= MemoryType::Data;
            }

            // Check if we've processed all memory
            if (addr == 0xFFFF) {
                finished = true;
            }

            addr++;
        }

        util::Logger::debug("Data region analysis complete");
    }

    u16 MemoryAnalyzer::findInstructionStartCovering(u16 addr) const {
        // Look back up to 3 bytes to find an opcode
        for (int i = 0; i < 3; i++) {
            if (addr < i) {
                break; // Avoid underflow
            }

            u16 search = addr - i;
            if (memoryAccess_[search] & MemoryAccess_OpCode) {
                return search;
            }
        }

        // If no opcode found, return the original address
        return addr;
    }

    MemoryType MemoryAnalyzer::getMemoryType(u16 addr) const {
        if (addr < memoryTypes_.size()) {
            return memoryTypes_[addr];
        }
        return MemoryType::Unknown;
    }

    std::span<const MemoryType> MemoryAnalyzer::getMemoryTypes() const {
        return std::span<const MemoryType>(memoryTypes_.data(), memoryTypes_.size());
    }

    std::vector<std::pair<u16, u16>> MemoryAnalyzer::findDataRanges() const {
        std::vector<std::pair<u16, u16>> ranges;

        bool inDataRange = false;
        u16 rangeStart = 0;

        // Only look at the SID range
        for (u16 addr = startAddress_; addr < endAddress_; ++addr) {
            const bool isData = memoryTypes_[addr] & MemoryType::Data;

            if (isData && !inDataRange) {
                // Start of a new data range
                rangeStart = addr;
                inDataRange = true;
            }
            else if (!isData && inDataRange) {
                // End of a data range
                ranges.emplace_back(rangeStart, addr - 1);
                inDataRange = false;
            }
        }

        // Handle the case where the last range extends to the end
        if (inDataRange) {
            ranges.emplace_back(rangeStart, endAddress_ - 1);
        }

        return ranges;
    }

    std::vector<std::pair<u16, u16>> MemoryAnalyzer::findCodeRanges() const {
        std::vector<std::pair<u16, u16>> ranges;

        bool inCodeRange = false;
        u16 rangeStart = 0;

        // Only look at the SID range
        for (u16 addr = startAddress_; addr < endAddress_; ++addr) {
            const bool isCode = memoryTypes_[addr] & MemoryType::Code;

            if (isCode && !inCodeRange) {
                // Start of a new code range
                rangeStart = addr;
                inCodeRange = true;
            }
            else if (!isCode && inCodeRange) {
                // End of a code range
                ranges.emplace_back(rangeStart, addr - 1);
                inCodeRange = false;
            }
        }

        // Handle the case where the last range extends to the end
        if (inCodeRange) {
            ranges.emplace_back(rangeStart, endAddress_ - 1);
        }

        return ranges;
    }

    std::vector<u16> MemoryAnalyzer::findLabelTargets() const {
        std::vector<u16> targets;

        // Only look at the SID range
        for (u16 addr = startAddress_; addr < endAddress_; ++addr) {
            if (memoryTypes_[addr] & MemoryType::LabelTarget) {
                targets.push_back(addr);
            }
        }

        return targets;
    }

    bool MemoryAnalyzer::isValidAddress(u16 addr) const {
        return addr >= startAddress_ && addr < endAddress_;
    }

} // namespace sidblaster
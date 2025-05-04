#pragma once

#include "SIDBlasterUtils.h"

#include <memory>
#include <span>
#include <vector>

namespace sidblaster {

    /**
     * @brief Types of memory regions
     */
    enum class MemoryType : u8 {
        Unknown = 0,
        Code = 1 << 0,
        Data = 1 << 1,
        LabelTarget = 1 << 2,
        Accessed = 1 << 3
    };

    // Operator overloads for MemoryType
    inline MemoryType operator|(MemoryType a, MemoryType b) {
        return static_cast<MemoryType>(static_cast<u8>(a) | static_cast<u8>(b));
    }

    inline bool operator&(MemoryType a, MemoryType b) {
        return (static_cast<u8>(a) & static_cast<u8>(b)) != 0;
    }

    inline MemoryType& operator|=(MemoryType& a, MemoryType b) {
        a = static_cast<MemoryType>(static_cast<u8>(a) | static_cast<u8>(b));
        return a;
    }

    /**
     * @class MemoryAnalyzer
     * @brief Analyzes CPU memory to identify code, data, and label targets
     */
    class MemoryAnalyzer {
    public:
        /**
         * @brief Constructor
         * @param memory CPU memory
         * @param memoryAccess Memory access tracking data
         * @param startAddress Start address of the region to analyze
         * @param endAddress End address of the region to analyze
         */
        MemoryAnalyzer(
            std::span<const u8> memory,
            std::span<const u8> memoryAccess,
            u16 startAddress,
            u16 endAddress);

        /**
         * @brief Analyze execution patterns
         *
         * Marks memory regions that have been executed as Code,
         * and jump targets as LabelTarget.
         */
        void analyzeExecution();

        /**
         * @brief Analyze memory access patterns
         *
         * Identifies accessed memory regions and marks them accordingly.
         */
        void analyzeAccesses();

        /**
         * @brief Analyze data regions
         *
         * Identifies memory regions that are not code and marks them as Data.
         */
        void analyzeData();

        /**
         * @brief Find the start of an instruction that covers a specific address
         * @param addr Address to find the covering instruction for
         * @return Start address of the instruction
         */
        u16 findInstructionStartCovering(u16 addr) const;

        /**
         * @brief Get the memory type for a specific address
         * @param addr Address to check
         * @return Memory type
         */
        MemoryType getMemoryType(u16 addr) const;

        /**
         * @brief Get the memory type map
         * @return Span of memory types for all addresses
         */
        std::span<const MemoryType> getMemoryTypes() const;

        /**
         * @brief Find all data ranges in the analyzed memory
         * @return Vector of pairs representing start and end addresses of data blocks
         */
        std::vector<std::pair<u16, u16>> findDataRanges() const;

        /**
         * @brief Find all code ranges in the analyzed memory
         * @return Vector of pairs representing start and end addresses of code blocks
         */
        std::vector<std::pair<u16, u16>> findCodeRanges() const;

        /**
         * @brief Find all addresses that should have labels
         * @return Vector of addresses that should have labels
         */
        std::vector<u16> findLabelTargets() const;

    private:
        std::span<const u8> memory_;
        std::span<const u8> memoryAccess_;
        u16 startAddress_;
        u16 endAddress_;
        std::vector<MemoryType> memoryTypes_;

        /**
         * @brief Check if an address is valid for analysis
         * @param addr Address to check
         * @return True if the address is within the analysis range
         */
        bool isValidAddress(u16 addr) const;
    };

} // namespace sidblaster
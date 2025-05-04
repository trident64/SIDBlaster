#pragma once

#include "MemoryAnalyzer.h"
#include "SIDBlasterUtils.h"

#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace sidblaster {

    /**
     * @brief Represents a data block in memory
     */
    struct DataBlock {
        std::string label;
        u16 start;
        u16 end;
    };

    /**
     * @brief Enumeration of hardware component types
     */
    enum class HardwareType {
        SID,
        VIC,
        CIA1,
        CIA2,
        Other
    };

    /**
     * @brief Information about a hardware component base address
     */
    struct HardwareBase {
        HardwareType type;
        u16 address;
        int index;  // For multiple instances (e.g., SID0, SID1)
        std::string name;  // Name used in assembly output
    };

    /**
     * @class LabelGenerator
     * @brief Generates and manages labels for disassembled code
     */
    class LabelGenerator {
    public:
        /**
         * @brief Constructor
         * @param analyzer Memory analyzer to use for code/data analysis
         * @param loadAddress Load address of the SID file
         * @param endAddress End address of the SID file
         */
        LabelGenerator(
            const MemoryAnalyzer& analyzer,
            u16 loadAddress,
            u16 endAddress);

        /**
         * @brief Generate labels for code and data regions
         */
        void generateLabels();

        /**
         * @brief Get the label for a given address
         * @param addr Address to look up
         * @return Label for the address, or empty string if no label
         */
        std::string getLabel(u16 addr) const;

        /**
         * @brief Get all identified data blocks
         * @return Vector of data blocks
         */
        const std::vector<DataBlock>& getDataBlocks() const;

        /**
         * @brief Format an address with its label and offset
         * @param addr Address to format
         * @return Formatted string
         */
        std::string formatAddress(u16 addr) const;

        /**
         * @brief Format a zero page address with its label
         * @param addr Zero page address to format
         * @return Formatted string
         */
        std::string formatZeroPage(u8 addr) const;

        /**
         * @brief Add a zero page variable definition
         * @param addr Zero page address
         * @param label Label for the variable
         */
        void addZeroPageVar(u8 addr, const std::string& label);

        /**
         * @brief Get all zero page variables
         * @return Map of zero page addresses to labels
         */
        const std::map<u8, std::string>& getZeroPageVars() const;

        /**
         * @brief Add a hardware component base
         * @param type Hardware type
         * @param address Base address
         * @param index Component index
         * @param name Component name
         */
        void addHardwareBase(HardwareType type, u16 address, int index, const std::string& name);

        /**
         * @brief Get all hardware component bases
         * @return Vector of hardware bases
         */
        const std::vector<HardwareBase>& getHardwareBases() const;

        /**
         * @brief Add a subdivision to a data block
         * @param blockLabel Label of the data block
         * @param startOffset Start offset within the block
         * @param endOffset End offset within the block
         */
        void addDataBlockSubdivision(
            const std::string& blockLabel,
            u16 startOffset,
            u16 endOffset);

        /**
         * @brief Add an address that should be considered for subdivision
         * @param addr Address to mark for subdivision
         */
        void addPendingSubdivisionAddress(u16 addr);

        /**
         * @brief Apply subdivisions to all data blocks
         */
        void applySubdivisions();

        /**
         * @brief Get the label map (address to label)
         * @return Map of addresses to labels
         */
        const std::map<u16, std::string>& getLabelMap() const;

    private:
        const MemoryAnalyzer& analyzer_;
        u16 loadAddress_;
        u16 endAddress_;

        int codeLabelCounter_ = 0;
        int dataLabelCounter_ = 0;

        std::map<u16, std::string> labelMap_;
        std::vector<DataBlock> dataBlocks_;
        std::map<u8, std::string> zeroPageVars_;
        std::vector<HardwareBase> usedHardwareBases_;

        // Data block subdivision structures
        struct AccessInfo {
            u16 offset;
            u16 absAddr;
            u16 pc;
            bool isWrite;
        };

        std::unordered_map<std::string, std::vector<AccessInfo>> dataBlockAccessMap_;
        std::unordered_map<std::string, std::vector<std::pair<u16, u16>>> dataBlockSubdivisions_;
        std::set<u16> pendingSubdivisionAddresses_;

        /**
         * @brief Build subdivisions for data blocks
         */
        void buildDataBlockSubdivisions();
    };

} // namespace sidblaster
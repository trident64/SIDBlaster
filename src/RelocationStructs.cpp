#include "RelocationStructs.h"
#include <fstream>

namespace sidblaster {

    /**
     * @brief Dump the relocation table to a file
     * @param filename Output file path
     */
    void RelocationTable::dumpToFile(const std::string& filename) const {
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

} // namesp
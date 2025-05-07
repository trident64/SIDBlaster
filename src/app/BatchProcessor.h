// ==================================
//             SIDBlaster
//
//  Raistlin / Genesis Project (G*P)
// ==================================
#pragma once

#include "Common.h"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <regex>

class CPU6510;
class SIDLoader;

namespace sidblaster {

    /**
     * @class BatchProcessor
     * @brief Processes batches of SID files with wildcard support
     *
     * Provides functionality for batch relocating and verifying SID files,
     * with support for wildcards and preserving directory structures.
     */
    class BatchProcessor {
    public:
        /**
         * @struct BatchOptions
         * @brief Options for batch processing
         */
        struct BatchOptions {
            fs::path inputPattern;         ///< Input path with potential wildcards (e.g., "SID/*.sid")
            fs::path outputPattern;        ///< Output path with potential wildcards (e.g., "Relocated/*.sid")
            fs::path reportPath;           ///< Path for summary report
            fs::path tempDir;              ///< Directory for temporary files
            u16 relocationAddress;         ///< Target relocation address
            bool verify = true;            ///< Whether to verify relocation
            bool preserveSubfolders = true; ///< Whether to maintain subfolder structure
        };

        /**
         * @struct BatchResult
         * @brief Results of batch processing
         */
        struct BatchResult {
            int totalFiles = 0;             ///< Total number of files processed
            int successfulRelocations = 0;  ///< Number of successful relocations
            int verifiedMatches = 0;        ///< Number of relocations with matching outputs
        };

        /**
         * @brief Constructor
         */
        BatchProcessor();

        /**
         * @brief Process all files matching the input pattern
         * @param options Batch processing options
         * @return Processing results
         */
        BatchResult processBatch(const BatchOptions& options);

    private:
        // Utility methods
        std::vector<fs::path> expandWildcards(const fs::path& pattern);
        fs::path generateOutputPath(const fs::path& inputPath, const fs::path& outputPattern, bool preserveSubfolders);
        void ensureDirectoryExists(const fs::path& path);
    };

} // namespace sidblaster
// ==================================
//             SIDBlaster
//
//  Raistlin / Genesis Project (G*P)
// ==================================
#pragma once

#include "Common.h"
#include "CommandProcessor.h"
#include <filesystem>
#include <map>
#include <string>
#include <vector>

class CPU6510;
class SIDLoader;

namespace sidblaster {

    /**
     * @class BatchConverter
     * @brief Processes batch files with multiple tasks
     *
     * Supports wildcards in input/output paths for batch processing multiple files
     * with a single batch configuration file. Maintains file mapping across tasks
     * to ensure consistent file processing flow.
     */
    class BatchConverter {
    public:
        /**
         * @brief Constructor
         * @param configFile Path to batch configuration file
         * @param cpu Pointer to CPU instance
         * @param sid Pointer to SID loader instance
         */
        BatchConverter(const std::string& configFile, CPU6510* cpu, SIDLoader* sid, const std::string& reportPath = "batchreport.txt");

        /**
         * @brief Execute the batch processing
         * @return True if all tasks completed successfully
         */
        bool execute();

    private:
        /**
         * @struct BatchTask
         * @brief A single task in a batch file
         */
        struct BatchTask {
            std::map<std::string, std::string> params;
        };

        /**
         * @struct FileMapping
         * @brief Maps files across different stages of processing
         */
        struct FileMapping {
            fs::path originalFile;       ///< Original input file
            fs::path relocatedFile;      ///< Relocated version
            fs::path originalTraceFile;  ///< Trace of original file
            fs::path relocatedTraceFile; ///< Trace of relocated file
        };

        std::string configPath_;              ///< Path to the configuration file
        std::map<std::string, BatchTask> tasks_; ///< Map of task name to task parameters
        CPU6510* cpu_;                        ///< Pointer to CPU instance
        SIDLoader* sid_;                      ///< Pointer to SID loader instance
        std::map<std::string, FileMapping> fileMapping_; ///< Map of base names to file paths
        std::string reportPath_;

        /**
         * @brief Load the batch configuration file
         * @return True if configuration was loaded successfully
         */
        bool loadConfiguration();

        /**
         * @brief Process all input files through all tasks
         * @return True if all files were processed successfully
         */
        bool processFiles();

        /**
         * @brief Process a Verify task for a single file
         * @param taskName Name of the task
         * @param task Task parameters
         * @return True if the task completed successfully
         */
        bool processVerifySingleFile(const std::string& taskName, const BatchTask& task);

        /**
         * @brief Expand wildcards in a file pattern
         * @param pattern File pattern with potential wildcards
         * @return Vector of paths matching the pattern
         */
        std::vector<fs::path> expandWildcards(const std::string& pattern);

        /**
         * @brief Generate output path based on input path and pattern
         * @param inputPath Input file path
         * @param outputPattern Output pattern, possibly with wildcards
         * @return Generated output path
         */
        fs::path generateOutputPath(const fs::path& inputPath, const std::string& outputPattern);

        /**
         * @brief Ensure directory exists, creating it if necessary
         * @param path Path for which to ensure directory exists
         * @throws std::runtime_error if directory cannot be created
         */
        void ensureDirectoryExists(const fs::path& path);

        /**
         * @brief Register file mapping for consistent processing
         * @param originalFile Original file path
         * @param processedFile Processed file path
         * @param taskType Type of task ("Relocate", "Trace", etc.)
         */
        void registerFileMapping(const fs::path& originalFile, const fs::path& processedFile, const std::string& taskType);

        /**
         * @brief Process a Convert task with possible wildcards
         * @param taskName Name of the task
         * @param task Task parameters
         * @return True if the task completed successfully
         */
        bool processConvertTask(const std::string& taskName, const BatchTask& task);

        /**
         * @brief Process a Convert task for a single file
         * @param taskName Name of the task
         * @param task Task parameters
         * @return True if the task completed successfully
         */
        bool processConvertSingleFile(const std::string& taskName, const BatchTask& task);

        /**
         * @brief Process a Relocate task with possible wildcards
         * @param taskName Name of the task
         * @param task Task parameters
         * @return True if the task completed successfully
         */
        bool processRelocateTask(const std::string& taskName, const BatchTask& task);

        /**
         * @brief Process a Relocate task for a single file
         * @param taskName Name of the task
         * @param task Task parameters
         * @return True if the task completed successfully
         */
        bool processRelocateSingleFile(const std::string& taskName, const BatchTask& task);

        /**
         * @brief Process a Trace task with possible wildcards
         * @param taskName Name of the task
         * @param task Task parameters
         * @return True if the task completed successfully
         */
        bool processTraceTask(const std::string& taskName, const BatchTask& task);

        /**
         * @brief Process a Trace task for a single file
         * @param taskName Name of the task
         * @param task Task parameters
         * @return True if the task completed successfully
         */
        bool processTraceSingleFile(const std::string& taskName, const BatchTask& task);

        /**
         * @brief Process a Verify task
         * @param taskName Name of the task
         * @param task Task parameters
         * @return True if the task completed successfully
         */
        bool processVerifyTask(const std::string& taskName, const BatchTask& task);
    };

} // namespace sidblaster
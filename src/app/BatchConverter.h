// ==================================
//             SIDBlaster
//
//  Raistlin / Genesis Project (G*P)
// ==================================
#pragma once

#include "../Common.h"
#include "TraceLogger.h"
#include <memory>
#include <string>
#include <vector>
#include <map>

class CPU6510;
class SIDLoader;

namespace sidblaster {

    /**
     * @class BatchConverter
     * @brief Processes batch conversions of SID files
     *
     * Handles batch operations for converting, relocating, tracing, and
     * verifying SID music files based on a configuration file.
     */
    class BatchConverter {
    public:
        /**
         * @enum TaskType
         * @brief Types of tasks that can be performed in batch mode
         */
        enum class TaskType {
            Convert,    ///< Convert between formats
            Relocate,   ///< Relocate SID to a new address
            Verify,     ///< Verify relocation succeeded
            Trace       ///< Create trace log
        };

        /**
         * @struct BatchTask
         * @brief A single task in a batch operation
         */
        struct BatchTask {
            TaskType type;                           ///< Type of task
            std::string input;                       ///< Input file
            std::string output;                      ///< Output file
            std::map<std::string, std::string> parameters;  ///< Task parameters
        };

        /**
         * @brief Constructor
         * @param configFile Path to batch configuration file
         * @param cpu Pointer to CPU instance
         * @param sid Pointer to SID loader instance
         */
        BatchConverter(
            const std::string& configFile,
            CPU6510* cpu,
            SIDLoader* sid);

        /**
         * @brief Execute all tasks in the batch
         * @return True if all tasks executed successfully
         */
        bool execute();

    private:
        std::string configFile_;       ///< Configuration file path
        CPU6510* cpu_;                 ///< CPU instance
        SIDLoader* sid_;               ///< SID loader instance
        std::vector<BatchTask> tasks_; ///< List of tasks
        fs::path tempDir_;             ///< Temporary directory

        /**
         * @brief Parse the configuration file
         * @return True if parsing was successful
         */
        bool parseConfig();

        /**
         * @brief Execute a single task
         * @param task Task to execute
         * @return True if task executed successfully
         */
        bool executeTask(const BatchTask& task);

        /**
         * @brief Execute a conversion task
         * @param task Conversion task
         * @return True if conversion succeeded
         */
        bool executeConversion(const BatchTask& task);

        /**
         * @brief Execute a relocation task
         * @param task Relocation task
         * @return True if relocation succeeded
         */
        bool executeRelocation(const BatchTask& task);

        /**
         * @brief Execute a tracing task
         * @param task Tracing task
         * @return True if tracing succeeded
         */
        bool executeTracing(const BatchTask& task);

        /**
         * @brief Execute a verification task
         * @param task Verification task
         * @return True if verification succeeded (logs match)
         */
        bool executeVerification(const BatchTask& task);

        /**
         * @brief Get a string parameter from a task
         * @param task Task to get parameter from
         * @param name Parameter name
         * @param defaultValue Default value if parameter not found
         * @return Parameter value or default
         */
        std::string getParameterValue(
            const BatchTask& task,
            const std::string& name,
            const std::string& defaultValue = "") const;

        /**
         * @brief Get a boolean parameter from a task
         * @param task Task to get parameter from
         * @param name Parameter name
         * @param defaultValue Default value if parameter not found
         * @return Parameter value or default
         */
        bool getParameterBool(
            const BatchTask& task,
            const std::string& name,
            bool defaultValue = false) const;

        /**
         * @brief Get an integer parameter from a task
         * @param task Task to get parameter from
         * @param name Parameter name
         * @param defaultValue Default value if parameter not found
         * @return Parameter value or default
         */
        int getParameterInt(
            const BatchTask& task,
            const std::string& name,
            int defaultValue = 0) const;

        /**
         * @brief Get an address parameter from a task
         * @param task Task to get parameter from
         * @param name Parameter name
         * @param defaultValue Default value if parameter not found
         * @return Parameter value or default
         */
        u16 getParameterAddress(
            const BatchTask& task,
            const std::string& name,
            u16 defaultValue = 0) const;
    };

} // namespace sidblaster
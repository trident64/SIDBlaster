// ==================================
//             SIDBlaster
//
//  Raistlin / Genesis Project (G*P)
// ==================================
#pragma once

#include "../CommandLineParser.h"
#include "CommandProcessor.h"
#include "BatchConverter.h"
#include <memory>
#include <string>


namespace sidblaster {

    /**
     * @class SIDBlasterApp
     * @brief Main application class for SIDBlaster
     *
     * This is a slimmed-down version of the original SIDBlasterApp that delegates
     * most functionality to specialized components.
     */
    class SIDBlasterApp {
    public:
        /**
         * @brief Constructor
         * @param argc Number of command line arguments
         * @param argv Command line arguments
         */
        SIDBlasterApp(int argc, char** argv);

        /**
         * @brief Run the application
         * @return Exit code (0 on success, non-zero on failure)
         */
        int run();

    private:
        CommandLineParser cmdParser_;  ///< Command line parser
        fs::path logFile_;             ///< Log file path
        bool verbose_ = false;         ///< Verbose logging flag

        /**
         * @brief Set up command line options
         */
        void setupCommandLine();

        /**
         * @brief Initialize logging
         */
        void initializeLogging();

        /**
         * @brief Parse command line arguments
         * @return True if parsing succeeded
         */
        bool parseCommandLine();

        /**
         * @brief Create command processor options from command line
         * @return Processing options
         */
        CommandProcessor::ProcessingOptions createProcessingOptions();

        /**
         * @brief Run batch mode
         * @param batchFile Batch configuration file
         * @return Exit code (0 on success, non-zero on failure)
         */
        int runBatchMode(const std::string& batchFile);

        /**
         * @brief Run single file processing
         *
         * Processes a single input file according to the command line options.
         * This is the default mode when no batch options are specified.
         *
         * @return Exit code (0 on success, non-zero on failure)
         */
        int runSingleFileMode();

        /**
         * @brief Run batch processing with wildcard support
         *
         * Processes multiple files matching wildcard patterns in the input/output paths.
         * Supports relocation, verification, and detailed reporting capabilities.
         * Preserves subfolder structure when requested.
         *
         * @return Exit code (0 on success, non-zero on failure)
         */
        int runBatchWildcardMode();

    };

} // namespace sidblaster
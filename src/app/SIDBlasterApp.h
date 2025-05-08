// SIDBlasterApp.h
#pragma once

#include "CommandLineParser.h"
#include "CommandProcessor.h"
#include "CommandClass.h"
#include <memory>
#include <string>

namespace sidblaster {

    /**
     * @class SIDBlasterApp
     * @brief Main application class for SIDBlaster
     *
     * Handles processing of single commands based on command line arguments.
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
        CommandClass command_;              ///< Current command to execute
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
         * @brief Execute the current command
         * @return Exit code (0 on success, non-zero on failure)
         */
        int executeCommand();

        /**
         * @brief Create a CommandProcessor options object from the current command
         * @return CommandProcessor options
         */
        CommandProcessor::ProcessingOptions createProcessingOptions();

        /**
         * @brief Display help information
         * @return Exit code (0 on success)
         */
        int showHelp();

        /**
         * @brief Process a conversion command
         * @return Exit code (0 on success, non-zero on failure)
         */
        int processConversion();

        /**
         * @brief Process a relocation command
         * @return Exit code (0 on success, non-zero on failure)
         */
        int processRelocation();

        /**
         * @brief Process a disassembly command
         * @return Exit code (0 on success, non-zero on failure)
         */
        int processDisassembly();
    };

} // namespace sidblaster
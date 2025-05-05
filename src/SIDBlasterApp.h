// ==================================
//             SIDBlaster
//
//  Raistlin / Genesis Project (G*P)
// ==================================
#pragma once

#include "CommandLineParser.h"
#include "SIDBlasterUtils.h"
#include "SIDLoader.h"

#include <filesystem>
#include <string>

/**
 * @file SIDBlasterApp.h
 * @brief Main application class for SIDBlaster
 *
 * Provides the main application logic for the SIDBlaster tool,
 * including command line parsing, file processing, and output generation.
 */

 // Forward declarations
class CPU6510;

namespace fs = std::filesystem;

namespace sidblaster {

    class Disassembler;

    /**
     * @class SIDBlasterApp
     * @brief Main application class for SIDBlaster
     *
     * Coordinates the overall process of loading, analyzing, disassembling,
     * and outputting SID music files in various formats.
     */
    class SIDBlasterApp {
    public:
        /**
         * @brief Constructor
         * @param argc Number of command line arguments
         * @param argv Command line arguments
         *
         * Initializes the application with command line arguments.
         */
        SIDBlasterApp(int argc, char** argv);

        /**
         * @brief Run the application
         * @return Exit code
         *
         * Executes the main application logic and returns an exit code.
         */
        int run();

    private:
        CommandLineParser cmdParser_;     // Command line parser
        fs::path outputFile_;             // Single output file path
        fs::path inputFile_;              // Input file path
        fs::path tempDir_;                // Directory for temporary files
        fs::path logFile_;                // Log file path
        bool verbose_ = false;            // Verbose logging flag

        /**
         * @brief Output formats supported by the application
         */
        enum class OutputFormat {
            PRG,   // PRG file (with or without player)
            SID,   // SID file
            ASM    // Assembly file
        };

        OutputFormat outputFormat_ = OutputFormat::PRG;  // Default format
        bool includePlayer_ = true;                      // Default to include player

        // For relocation command-line option
        u16 relocAddress_ = 0;             // Relocation address
        bool hasRelocation_ = false;       // Whether relocation is requested

        // SID addresses
        u16 overrideInitAddress_ = 0;      // Override init address
        u16 overridePlayAddress_ = 0;      // Override play address
        u16 overrideLoadAddress_ = 0;      // Override load address
        bool hasOverrideInit_ = false;     // Whether init address is overridden
        bool hasOverridePlay_ = false;     // Whether play address is overridden
        bool hasOverrideLoad_ = false;     // Whether load address is overridden

        // SID metadata overrides
        std::string overrideTitle_;        // Override SID title
        std::string overrideAuthor_;       // Override SID author
        std::string overrideCopyright_;    // Override SID copyright

        SIDLoader* sid_ = nullptr;         // Pointer to current SID loader

        // For player settings
        std::string playerName_;           // Player name
        u16 playerAddress_ = 0;            // Player address

        /**
         * @brief Set up command line options
         *
         * Configures the command line parser with supported options.
         */
        void setupCommandLine();

        /**
         * @brief Initialize logging
         *
         * Sets up the logging system based on command line options.
         */
        void initializeLogging();

        /**
         * @brief Parse command line arguments
         * @return True if parsing succeeded
         *
         * Parses and validates command line arguments.
         */
        bool parseCommandLine();

        /**
         * @brief Check if output file is valid (not same as input, etc.)
         * @return True if output file is valid
         *
         * Validates that the output file path is valid and can be written.
         */
        bool checkOutputFileValid();

        /**
         * @brief Load player definitions from a file
         * @param filename Definitions file name
         * @param defs Output map of definitions
         * @return True if loading was successful
         *
         * Loads player configuration from a definitions file.
         */
        bool loadPlayerDefs(const std::string& filename, std::map<std::string, std::string>& defs);

        /**
         * @brief Apply SID metadata overrides from command line or defs file
         * @param sid SIDLoader instance to update
         *
         * Applies any metadata overrides specified in the command line or definitions file.
         */
        void applySIDMetadataOverrides(SIDLoader& sid);

        /**
         * @brief Process the input file
         * @return Exit code (0 on success)
         *
         * Processes the input file according to the command line options.
         */
        int processFile();

        /**
         * @brief Load a SID file
         * @param sid SID loader
         * @param extractedPrgPath Path to save extracted PRG
         * @return True if loading succeeded
         *
         * Loads a SID file and extracts the PRG data.
         */
        bool loadSidFile(SIDLoader& sid, const fs::path& extractedPrgPath);

        /**
         * @brief Load a PRG file
         * @param sid SID loader
         * @param extractedPrgPath Path to save extracted PRG
         * @return True if loading succeeded
         *
         * Loads a PRG file into the SID loader.
         */
        bool loadPrgFile(SIDLoader& sid, const fs::path& extractedPrgPath);

        /**
         * @brief Load a BIN file
         * @param sid SID loader
         * @param extractedPrgPath Path to save extracted PRG
         * @return True if loading succeeded
         *
         * Loads a BIN file into the SID loader.
         */
        bool loadBinFile(SIDLoader& sid, const fs::path& extractedPrgPath);

        /**
         * @brief Extract PRG data from a SID file
         * @param sidFile SID file path
         * @param outputPrg Output PRG file path
         *
         * Extracts the PRG data portion of a SID file.
         */
        void extractPrgFromSid(const fs::path& sidFile, const fs::path& outputPrg);

        /**
         * @brief Calculate the number of play calls per frame
         * @param sid SID loader with file information
         * @param CIATimerLo CIA timer low byte
         * @param CIATimerHi CIA timer high byte
         * @return Number of play calls per frame
         *
         * Determines how many times the play routine should be called per frame.
         */
        int calculatePlayCallsPerFrame(
            const SIDLoader& sid,
            u8 CIATimerLo,
            u8 CIATimerHi);

        /**
         * @brief Build the pure music file (no player)
         * @param basename Base name of the file
         * @param asmFile Path to the assembly file
         * @param outputPrg Path for output PRG
         * @return True if successful
         *
         * Assembles a pure music file without a player.
         */
        bool buildPureMusic(
            const std::string& basename,
            const fs::path& asmFile,
            const fs::path& outputPrg);

        /**
         * @brief Build with player linked
         * @param sid Pointer to the SID loader
         * @param basename Base name of the file
         * @param musicFile Path to the music file (ASM or PRG)
         * @param linkerFile Path for linker file
         * @param outputPrg Path for output PRG
         * @param playCallsPerFrame Number of play calls per frame
         * @param isPAL True if using PAL timing
         * @param sidInit SID init address
         * @param sidPlay SID play address
         * @param isPrgInput True if music file is PRG (not ASM)
         * @return True if successful
         *
         * Builds a complete player+music PRG file.
         */
        bool buildWithPlayer(
            SIDLoader* sid,
            const std::string& basename,
            const fs::path& musicFile,
            const fs::path& linkerFile,
            const fs::path& outputPrg,
            int playCallsPerFrame,
            bool isPAL,
            u16 sidInit,
            u16 sidPlay,
            bool isPrgInput = false);

        /**
         * @brief Create a player linker file
         * @param linkerFile Path to the linker file to create
         * @param musicFile Path to the music file (ASM or PRG)
         * @param playerAsmFile Path to the player assembly file
         * @param playCallsPerFrame Number of play calls per frame
         * @param isPAL True if using PAL timing
         * @param sidInit SID init address
         * @param sidPlay SID play address
         * @param isPrgInput True if music file is PRG (not ASM)
         * @return True if successful
         *
         * Creates a linker file for assembling the player with the music.
         */
        bool createPlayerLinkerFile(
            const fs::path& linkerFile,
            const fs::path& musicFile,
            const fs::path& playerAsmFile,
            int playCallsPerFrame,
            bool isPAL,
            u16 sidInit,
            u16 sidPlay,
            bool isPrgInput = false);

        /**
         * @brief Compress a PRG file using the configured compressor tool
         * @param inputPrg Input PRG file path
         * @param outputPrg Output compressed PRG file path
         * @param loadAddress Load address for the compressed file
         * @return True if compression succeeded
         *
         * Compresses a PRG file using an external compression tool.
         */
        bool compressPrg(
            const fs::path& inputPrg,
            const fs::path& outputPrg,
            u16 loadAddress);

        /**
         * @brief Generate a new SID file
         * @param sourcePrgFile Source PRG file
         * @param outputSidFile Output SID file
         * @param loadAddr Load address
         * @param initAddr Init address
         * @param playAddr Play address
         * @return True if successful
         *
         * Creates a SID file from a PRG file with specified addresses.
         */
        bool generateSIDFile(
            const fs::path& sourcePrgFile,
            const fs::path& outputSidFile,
            u16 loadAddr,
            u16 initAddr,
            u16 playAddr);

        /**
         * @brief Fix SID header endianness for writing
         * @param header Header to fix
         *
         * Swaps endianness of multi-byte values in the SID header.
         */
        void fixHeaderEndianness(SIDHeader& header);
    };

} // namespace sidblaster
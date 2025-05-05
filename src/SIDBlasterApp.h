#pragma once

#include "CommandLineParser.h"
#include "SIDBlasterUtils.h"
#include "SIDLoader.h"

#include <filesystem>
#include <string>

// Forward declarations
class CPU6510;

namespace fs = std::filesystem;

namespace sidblaster {

    class Disassembler;

    /**
     * @class SIDBlasterApp
     * @brief Main application class for SIDBlaster
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
         * @return Exit code
         */
        int run();

    private:
        CommandLineParser cmdParser_;
        fs::path outputFile_;  // Single output file path
        fs::path inputFile_;
        fs::path tempDir_;     // Directory for temporary files
        fs::path logFile_;
        bool verbose_ = false;

        enum class OutputFormat {
            PRG,   // PRG file (with or without player)
            SID,   // SID file
            ASM    // Assembly file
        };

        OutputFormat outputFormat_ = OutputFormat::PRG;  // Default format
        bool includePlayer_ = true;  // Default to include player

        // For relocation command-line option
        u16 relocAddress_ = 0;
        bool hasRelocation_ = false;

        // SID addresses
        u16 overrideInitAddress_ = 0;
        u16 overridePlayAddress_ = 0;
        u16 overrideLoadAddress_ = 0;
        bool hasOverrideInit_ = false;
        bool hasOverridePlay_ = false;
        bool hasOverrideLoad_ = false;

        // SID metadata overrides
        std::string overrideTitle_;
        std::string overrideAuthor_;
        std::string overrideCopyright_;

        SIDLoader* sid_ = nullptr;

        // For player settings
        std::string playerName_;
        u16 playerAddress_ = 0;

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
         * Check if output file is valid (not same as input, etc.)
         *
         * @return True if output file is valid
         */
        bool checkOutputFileValid();

        /**
         * Load player definitions from a file
         *
         * @param filename Definitions file name
         * @param defs Output map of definitions
         * @return True if loading was successful
         */
        bool loadPlayerDefs(const std::string& filename, std::map<std::string, std::string>& defs);

        /**
         * Apply SID metadata overrides from command line or defs file
         *
         * @param sid SIDLoader instance to update
         */
        void applySIDMetadataOverrides(SIDLoader& sid);

        /**
         * @brief Process the input file
         * @return Exit code (0 on success)
         */
        int processFile();

        /**
         * @brief Load a SID file
         * @param sid SID loader
         * @param extractedPrgPath Path to save extracted PRG
         * @return True if loading succeeded
         */
        bool loadSidFile(SIDLoader& sid, const fs::path& extractedPrgPath);

        /**
         * @brief Load a PRG file
         * @param sid SID loader
         * @param extractedPrgPath Path to save extracted PRG
         * @return True if loading succeeded
         */
        bool loadPrgFile(SIDLoader& sid, const fs::path& extractedPrgPath);

        /**
         * @brief Load a BIN file
         * @param sid SID loader
         * @param extractedPrgPath Path to save extracted PRG
         * @return True if loading succeeded
         */
        bool loadBinFile(SIDLoader& sid, const fs::path& extractedPrgPath);

        /**
         * @brief Extract PRG data from a SID file
         * @param sidFile SID file path
         * @param outputPrg Output PRG file path
         */
        void extractPrgFromSid(const fs::path& sidFile, const fs::path& outputPrg);

        /**
         * @brief Calculate the number of play calls per frame
         * @param sid SID loader with file information
         * @param CIATimerLo CIA timer low byte
         * @param CIATimerHi CIA timer high byte
         * @return Number of play calls per frame
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
         */
        bool buildPureMusic(
            const std::string& basename,
            const fs::path& asmFile,
            const fs::path& outputPrg);

        /**
         * @brief Build with player linked
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
         */
        void fixHeaderEndianness(SIDHeader& header);
    };

} // namespace sidblaster
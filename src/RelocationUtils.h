#pragma once

#include "Common.h"
#include <filesystem>
#include <string>

namespace fs = std::filesystem;
class CPU6510;
class SIDLoader;
namespace sidblaster {
    class Disassembler;
}

namespace sidblaster {
    namespace util {

        /**
         * @struct RelocationParams
         * @brief Parameters for SID relocation
         */
        struct RelocationParams {
            fs::path inputFile;           ///< Input file
            fs::path outputFile;          ///< Output file
            fs::path tempDir;             ///< Temp directory
            u16 relocationAddress;        ///< Target address for relocation
            std::string kickAssPath;      ///< Path to KickAss.jar
            bool verbose;                 ///< Verbose logging
        };

        /**
         * @struct RelocationResult
         * @brief Result of a relocation operation
         */
        struct RelocationResult {
            bool success;                 ///< Operation success
            u16 originalLoad;             ///< Original load address
            u16 originalInit;             ///< Original init address
            u16 originalPlay;             ///< Original play address
            u16 newLoad;                  ///< New load address
            u16 newInit;                  ///< New init address
            u16 newPlay;                  ///< New play address
            int unusedBytesRemoved;       ///< Number of unused bytes removed
            std::string message;          ///< Additional info/error message
        };

        /**
         * @brief Relocate a SID file
         * @param cpu CPU instance for disassembly
         * @param sid SID loader for file handling
         * @param params Relocation parameters
         * @return Result of the relocation
         */
        RelocationResult relocateSID(
            CPU6510* cpu,
            SIDLoader* sid,
            const RelocationParams& params);

        /**
         * @brief Assemble an ASM file to PRG
         * @param asmFile Input assembly file
         * @param prgFile Output PRG file
         * @param kickAssPath Path to KickAss.jar
         * @return True if assembly succeeded
         */
        bool assembleAsmToPrg(
            const fs::path& asmFile,
            const fs::path& prgFile,
            const std::string& kickAssPath);

        /**
         * @brief Create a SID file from a PRG file
         * @param prgFile Input PRG file
         * @param sidFile Output SID file
         * @param loadAddr SID load address
         * @param initAddr SID init address
         * @param playAddr SID play address
         * @param title SID title
         * @param author SID author
         * @param copyright SID copyright
         * @return True if SID file creation succeeded
         */
        bool createSIDFromPRG(
            const fs::path& prgFile,
            const fs::path& sidFile,
            u16 loadAddr,
            u16 initAddr,
            u16 playAddr,
            const std::string& title = "",
            const std::string& author = "",
            const std::string& copyright = "");

        /**
         * @brief Run SID emulation to analyze memory patterns
         *
         * Initializes the SID and executes the play routine for a specified number of frames,
         * allowing memory access patterns to be analyzed for relocation or tracing.
         *
         * @param cpu Pointer to CPU instance
         * @param sid Pointer to SID loader instance
         * @param frames Number of frames to emulate
         * @param backupAndRestore Whether to backup and restore memory (default: true)
         * @return True if emulation completed successfully
         */
        bool runSIDEmulation(
            CPU6510* cpu,
            SIDLoader* sid,
            int frames,
            bool backupAndRestore = true);

    }
}

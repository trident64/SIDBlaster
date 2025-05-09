#include "RelocationUtils.h"
#include "SIDBlasterUtils.h"
#include "cpu6510.h"
#include "SIDEmulator.h"
#include "SIDLoader.h"
#include "Disassembler.h"

#include <fstream>

namespace sidblaster {
    namespace util {

        RelocationResult relocateSID(
            CPU6510* cpu,
            SIDLoader* sid,
            const RelocationParams& params) {

            RelocationResult result;
            result.success = false;

            // Validate that both input and output are SID files
            const std::string inExt = getFileExtension(params.inputFile);
            if (inExt != ".sid") {
                result.message = "Input file must be a SID file (.sid): " + params.inputFile.string();
                Logger::error(result.message);
                return result;
            }

            const std::string outExt = getFileExtension(params.outputFile);
            if (outExt != ".sid") {
                result.message = "Output file must be a SID file (.sid): " + params.outputFile.string();
                Logger::error(result.message);
                return result;
            }

            // Create temp directory if it doesn't exist
            try {
                fs::create_directories(params.tempDir);
            }
            catch (const std::exception& e) {
                result.message = std::string("Failed to create temp directory: ") + e.what();
                Logger::error(result.message);
                return result;
            }

            // Load the input file
            if (!sid->loadSID(params.inputFile.string())) {
                result.message = "Failed to load file for relocation: " + params.inputFile.string();
                Logger::error(result.message);
                return result;
            }

            // Get original addresses
            result.originalLoad = sid->getLoadAddress();
            result.originalInit = sid->getInitAddress();
            result.originalPlay = sid->getPlayAddress();

            // Calculate relocated addresses
            result.newLoad = params.relocationAddress;
            result.newInit = result.newLoad + (result.originalInit - result.originalLoad);
            result.newPlay = result.newLoad + (result.originalPlay - result.originalLoad);

            Logger::info("Original addresses - Load: $" + wordToHex(result.originalLoad) +
                ", Init: $" + wordToHex(result.originalInit) +
                ", Play: $" + wordToHex(result.originalPlay));

            Logger::info("Relocated addresses - Load: $" + wordToHex(result.newLoad) +
                ", Init: $" + wordToHex(result.newInit) +
                ", Play: $" + wordToHex(result.newPlay));

            // Create a Disassembler
            sidblaster::Disassembler disassembler(*cpu, *sid);

            // Run emulation to analyze memory access patterns
            const int numFrames = 30000; // TODO, this should be coming from (1) a global constant (in common.h), then overriden by the CFG file if there is one, then overridden by batch script or command-line, etc
            if (!runSIDEmulation(cpu, sid, numFrames)) {
                result.message = "Failed to run SID emulation for memory analysis";
                Logger::error(result.message);
                return result;
            }

            // For SID output, we need to:
            // 1. Generate assembly with relocation
            // 2. Assemble to PRG
            // 3. Create a SID file with proper header

            // Setup temp files
            const std::string basename = params.inputFile.stem().string();
            const fs::path tempAsmFile = params.tempDir / (basename + "-relocated.asm");
            const fs::path tempPrgFile = params.tempDir / (basename + "-relocated.prg");

            // Generate ASM with relocated addresses
            result.unusedBytesRemoved = disassembler.generateAsmFile(
                tempAsmFile.string(),
                result.newLoad,
                result.newInit,
                result.newPlay);

            // Assemble to PRG
            if (!assembleAsmToPrg(tempAsmFile, tempPrgFile, params.kickAssPath)) {
                result.message = "Failed to assemble relocated code: " + tempAsmFile.string();
                Logger::error(result.message);
                return result;
            }

            // Create SID file from PRG
            const std::string title = sid->getHeader().name;
            const std::string author = sid->getHeader().author;
            const std::string copyright = sid->getHeader().copyright;

            if (!createSIDFromPRG(
                tempPrgFile,
                params.outputFile,
                result.newLoad,
                result.newInit,
                result.newPlay,
                title,
                author,
                copyright)) {

                // If SID creation fails, fall back to PRG
                Logger::warning("SID file generation failed. Saving as PRG instead.");

                try {
                    fs::copy_file(tempPrgFile, params.outputFile, fs::copy_options::overwrite_existing);

                    result.success = true;
                    result.message = "Relocation complete (saved as PRG). " +
                        std::to_string(result.unusedBytesRemoved) + " unused bytes removed.";
                    Logger::info(result.message);
                }
                catch (const std::exception& e) {
                    result.message = std::string("Failed to copy output file: ") + e.what();
                    Logger::error(result.message);
                    return result;
                }
            }
            else {
                result.success = true;
                result.message = "Relocation to SID complete. " +
                    std::to_string(result.unusedBytesRemoved) + " unused bytes removed.";
                Logger::info(result.message);
            }

            return result;
        }

        RelocationVerificationResult relocateAndVerifySID(
            CPU6510* cpu,
            SIDLoader* sid,
            const fs::path& inputFile,
            const fs::path& outputFile,
            u16 relocationAddress,
            const fs::path& tempDir) {

            RelocationVerificationResult result;
            result.success = false;
            result.verified = false;
            result.outputsMatch = false;

            // Prepare paths for verification
            fs::path originalTrace = tempDir / (inputFile.stem().string() + "-original.trace");
            fs::path relocatedTrace = tempDir / (inputFile.stem().string() + "-relocated.trace");
            fs::path diffReport = tempDir / (inputFile.stem().string() + "-diff.txt");

            result.originalTrace = originalTrace.string();
            result.relocatedTrace = relocatedTrace.string();
            result.diffReport = diffReport.string();

            try {
                // Step 1: Create trace of original SID
                if (!sid->loadSID(inputFile.string())) {
                    result.message = "Failed to load original SID file";
                    return result;
                }

                // Create an emulator for the original SID
                SIDEmulator originalEmulator(cpu, sid);
                SIDEmulator::EmulationOptions options;
                options.frames = DEFAULT_SID_EMULATION_FRAMES;
                options.traceEnabled = true;
                options.traceLogPath = originalTrace.string();

                if (!originalEmulator.runEmulation(options)) {
                    result.message = "Failed to emulate original SID file";
                    return result;
                }

                // Step 2: Relocate the SID file
                util::RelocationParams relocParams;
                relocParams.inputFile = inputFile;
                relocParams.outputFile = outputFile;
                relocParams.tempDir = tempDir;
                relocParams.relocationAddress = relocationAddress;

                util::RelocationResult relocResult = util::relocateSID(cpu, sid, relocParams);

                if (!relocResult.success) {
                    result.message = "Relocation failed: " + relocResult.message;
                    return result;
                }

                result.success = true;

                // Step 3: Verify the relocated SID
                if (!sid->loadSID(outputFile.string())) {
                    result.message = "Relocation succeeded but failed to load relocated SID file";
                    return result;
                }

                // Create an emulator for the relocated SID
                SIDEmulator relocatedEmulator(cpu, sid);
                options.traceLogPath = relocatedTrace.string();

                if (!relocatedEmulator.runEmulation(options)) {
                    result.message = "Relocation succeeded but failed to emulate relocated SID file";
                    return result;
                }

                result.verified = true;

                // Step 4: Compare trace files
                result.outputsMatch = TraceLogger::compareTraceLogs(
                    originalTrace.string(),
                    relocatedTrace.string(),
                    diffReport.string());

                if (result.outputsMatch) {
                    result.message = "Relocation and verification successful";
                }
                else {
                    result.message = "Relocation succeeded but verification failed - outputs differ";
                }

                return result;
            }
            catch (const std::exception& e) {
                result.message = std::string("Exception during relocation/verification: ") + e.what();
                return result;
            }
        }

        bool assembleAsmToPrg(
            const fs::path& asmFile,
            const fs::path& prgFile,
            const std::string& kickAssPath) {

            // Prepare the command line
            std::string kickCommand = kickAssPath + " \"" + asmFile.string() + "\" -o \"" +
                prgFile.string() + "\"";

            Logger::debug("Assembling: " + kickCommand);
            const int result = std::system(kickCommand.c_str());

            if (result != 0) {
                Logger::error("Assembly failed with error code: " + std::to_string(result));
                return false;
            }

            return true;
        }

        bool createSIDFromPRG(
            const fs::path& prgFile,
            const fs::path& sidFile,
            u16 loadAddr,
            u16 initAddr,
            u16 playAddr,
            const std::string& title,
            const std::string& author,
            const std::string& copyright) {

            // Read the PRG file
            std::ifstream prg(prgFile, std::ios::binary | std::ios::ate);
            if (!prg) {
                Logger::error("Failed to open PRG file: " + prgFile.string());
                return false;
            }

            // Get file size
            const auto filePos = prg.tellg();
            const size_t fileSize = static_cast<size_t>(filePos);
            prg.seekg(0, std::ios::beg);

            if (fileSize < 2) {
                Logger::error("PRG file too small: " + prgFile.string());
                return false;
            }

            // PRG files start with a 2-byte load address, verify it matches our expected loadAddr
            u8 lo, hi;
            prg.read(reinterpret_cast<char*>(&lo), 1);
            prg.read(reinterpret_cast<char*>(&hi), 1);

            // The load address from the PRG file
            const u16 prgLoadAddr = (hi << 8) | lo;

            // If the specified load address doesn't match the one in the PRG, log a warning
            if (prgLoadAddr != loadAddr) {
                Logger::warning("PRG file load address ($" + wordToHex(prgLoadAddr) +
                    ") doesn't match specified address ($" + wordToHex(loadAddr) + ")");
                // We'll use the load address from the PRG if it doesn't match
                loadAddr = prgLoadAddr;
            }

            // Create a SID header
            SIDHeader header;

            // Fill in the header fields (initially in little-endian format)
            std::memcpy(header.magicID, "PSID", 4);  // Magic ID for SID format
            header.version = 2;              // Version 2
            header.dataOffset = 0x7C;        // Standard offset
            header.loadAddress = 0;          // 0 means load address is in the data
            header.initAddress = initAddr;   // Init address
            header.playAddress = playAddr;   // Play address
            header.songs = 1;                // 1 song
            header.startSong = 1;            // Start song 1
            header.speed = 0;                // No CIA timers specified

            // Fill in metadata fields with null-termination
            std::memset(header.name, 0, sizeof(header.name));
            std::memset(header.author, 0, sizeof(header.author));
            std::memset(header.copyright, 0, sizeof(header.copyright));

            // Copy metadata if provided, ensuring null-termination
            if (!title.empty()) {
                std::strncpy(header.name, title.c_str(), sizeof(header.name) - 1);
            }
            if (!author.empty()) {
                std::strncpy(header.author, author.c_str(), sizeof(header.author) - 1);
            }
            if (!copyright.empty()) {
                std::strncpy(header.copyright, copyright.c_str(), sizeof(header.copyright) - 1);
            }

            // Set remaining fields
            header.flags = 0;                // Default flags (PAL)
            header.startPage = 0;            // Not used in this context
            header.pageLength = 0;           // Not used in this context
            header.secondSIDAddress = 0;     // Not used in this context
            header.thirdSIDAddress = 0;      // Not used in this context

            // Fix endianness (SID files are big-endian)
            // We need to reverse the byte order for all multi-byte fields
            auto swapEndian = [](u16 value) -> u16 {
                return (value >> 8) | (value << 8);
                };

            header.version = swapEndian(header.version);
            header.dataOffset = swapEndian(header.dataOffset);
            header.loadAddress = swapEndian(header.loadAddress);
            header.initAddress = swapEndian(header.initAddress);
            header.playAddress = swapEndian(header.playAddress);
            header.songs = swapEndian(header.songs);
            header.startSong = swapEndian(header.startSong);
            header.flags = swapEndian(header.flags);

            // Open the output SID file
            std::ofstream sid_file(sidFile, std::ios::binary);
            if (!sid_file) {
                Logger::error("Failed to create SID file: " + sidFile.string());
                return false;
            }

            // Write the header
            sid_file.write(reinterpret_cast<const char*>(&header), sizeof(header));

            // Write the load address (little-endian) at the beginning of the data
            sid_file.write(reinterpret_cast<const char*>(&lo), 1);
            sid_file.write(reinterpret_cast<const char*>(&hi), 1);

            // Write the actual PRG data, skipping the 2-byte load address that was already in the PRG
            const size_t dataSize = fileSize - 2;
            std::vector<char> buffer(dataSize);
            prg.read(buffer.data(), dataSize);
            sid_file.write(buffer.data(), dataSize);

            sid_file.close();

            Logger::info("Created SID file: " + sidFile.string() +
                " (Load: $" + wordToHex(loadAddr) +
                ", Init: $" + wordToHex(initAddr) +
                ", Play: $" + wordToHex(playAddr) + ")");

            return true;
        }

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
            bool backupAndRestore) {

            SIDEmulator emulator(cpu, sid);
            SIDEmulator::EmulationOptions options;
            options.frames = frames;
            options.backupAndRestore = backupAndRestore;
            options.traceEnabled = false;

            return emulator.runEmulation(options);
        }

    }
}
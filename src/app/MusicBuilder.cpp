// ==================================
//             SIDBlaster
//
//  Raistlin / Genesis Project (G*P)
// ==================================
#include "MusicBuilder.h"
#include "../SIDBlasterUtils.h"
#include "../cpu6510.h"
#include "../SIDLoader.h"

#include <algorithm>
#include <fstream>
#include <cctype>

namespace sidblaster {

    MusicBuilder::MusicBuilder(const CPU6510* cpu, const SIDLoader* sid)
        : cpu_(cpu), sid_(sid) {
    }

    bool MusicBuilder::buildMusic(
        const std::string& basename,
        const fs::path& inputFile,
        const fs::path& outputFile,
        const BuildOptions& options) {

        // Create temp directory if it doesn't exist
        try {
            fs::create_directories(options.tempDir);
        }
        catch (const std::exception& e) {
            util::Logger::error(std::string("Failed to create temp directory: ") + e.what());
            return false;
        }

        // Setup temporary file paths
        fs::path tempPrgFile = options.tempDir / (basename + ".prg");
        fs::path tempPlayerPrgFile = options.tempDir / (basename + "-player.prg");
        fs::path tempCompressedPrgFile = options.tempDir / (basename + "-compressed.prg");
        fs::path tempLinkerFile = options.tempDir / (basename + "-linker.asm");
        fs::path tempExtractedPrg = options.tempDir / (basename + "-extracted.prg");

        // Determine input file type
        InputType inputType = determineInputType(inputFile);

        // Handle based on whether to include player or not
        if (!options.playerName.empty()) {
            // Set default player name if none specified
            std::string playerToUse = options.playerName;
            if (playerToUse == "default") {
                playerToUse = "SimpleRaster";  // Default player name
            }

            // Get player assembly file path
            fs::path playerAsmFile = fs::path("SIDPlayers") / playerToUse / (playerToUse + ".asm");

            // Create the player directory if it doesn't exist
            fs::create_directories(playerAsmFile.parent_path());

            // Create linker file
            if (!createLinkerFile(tempLinkerFile, inputFile, playerAsmFile, options)) {
                return false;
            }

            // Run assembler to build player+music
            if (!runAssembler(tempLinkerFile, tempPlayerPrgFile, options.kickAssPath)) {
                return false;
            }

            // Apply compression if requested
            if (options.compress) {
                if (!compressPrg(tempPlayerPrgFile, outputFile, options.playerAddress, options)) {
                    // Fallback to uncompressed if compression fails
                    util::Logger::warning(std::string("Compression failed on ") + tempPlayerPrgFile.string());
                }
            }
            else {
                // Copy uncompressed file to output
                try {
                    fs::copy_file(tempPlayerPrgFile, outputFile,
                        fs::copy_options::overwrite_existing);
                    return true;
                }
                catch (const std::exception& e) {
                    util::Logger::error(std::string("Failed to copy uncompressed PRG: ") + e.what());
                    return false;
                }
            }
        }
        else {
            // Pure music without player

            // If input is ASM, just assemble it
            if (inputType == InputType::ASM) {
                // Run assembler to build pure music
                if (!runAssembler(inputFile, outputFile, options.kickAssPath)) {
                    return false;
                }
                return true;
            }
            else if (inputType == InputType::PRG) {
                // For PRG input, just copy the file
                try {
                    fs::copy_file(inputFile, outputFile, fs::copy_options::overwrite_existing);
                    return true;
                }
                catch (const std::exception& e) {
                    util::Logger::error(std::string("Failed to copy PRG file: ") + e.what());
                    return false;
                }
            }
            else if (inputType == InputType::SID) {
                // For SID input, extract the PRG data
                return extractPrgFromSid(inputFile, outputFile);
            }
            else {
                util::Logger::error("Unsupported input file type for pure music output");
                return false;
            }
        }

        return true;
    }

    MusicBuilder::InputType MusicBuilder::determineInputType(const fs::path& inputFile) const {
        std::string ext = inputFile.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return std::tolower(c); });

        if (ext == ".sid") return InputType::SID;
        if (ext == ".prg") return InputType::PRG;
        if (ext == ".asm") return InputType::ASM;
        if (ext == ".bin") return InputType::BIN;

        // Default to PRG if unknown
        return InputType::PRG;
    }

    bool MusicBuilder::createLinkerFile(
        const fs::path& linkerFile,
        const fs::path& musicFile,
        const fs::path& playerAsmFile,
        const BuildOptions& options) {

        std::string ext = musicFile.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return std::tolower(c); });

        bool bIsSID = (ext == ".sid");
        bool bIsASM = (ext == ".asm");

        if ((!bIsSID) && (!bIsASM))
        {
            util::Logger::error(std::string("Only SID and ASM files can be linked - '" + musicFile.string() + "' rejected."));
            return false;
        }

        std::ofstream file(linkerFile);
        if (!file) {
            util::Logger::error("Failed to create linker file: " + linkerFile.string());
            return false;
        }

        // Write the linker file header
        file << "//; ------------------------------------------\n";
        file << "//; SIDBlaster Player Linker\n";
        file << "//; ------------------------------------------\n";
        file << "\n";

        if (bIsSID)
        {
            file << ".var music_prg = LoadSid(\"" << musicFile.string() << "\")\n";
            file << "* = music_prg.location \"SID\"\n";
            file << ".fill music_prg.size, music_prg.getData(i)\n";
            file << "\n";
            file << ".var SIDInit = music_prg.init\n";
            file << ".var SIDPlay = music_prg.play\n";
        }
        else
        {
            u16 sidInit = options.sidInitAddr;
            u16 sidPlay = options.sidPlayAddr;
            file << ".var SIDInit = $" << util::wordToHex(sidInit) << "\n";
            file << ".var SIDPlay = $" << util::wordToHex(sidPlay) << "\n";
        }

        // Define player variables
        file << ".var NumCallsPerFrame = " << options.playCallsPerFrame << "\n";
        file << ".var PlayerADDR = $" << util::wordToHex(options.playerAddress) << "\n";
        file << "\n";

        // Add SID metadata if available
        if (sid_) {
            const auto& header = sid_->getHeader();

            // Clean up strings for embedding in the linker file
            auto cleanString = [](const std::string& str) {
                std::string result;
                for (char c : str) {
                    // Keep alphanumeric and basic punctuation, replace others with _
                    if (std::isalnum(c) || c == ' ' || c == '-' || c == '_' || c == '!') {
                        result.push_back(c);
                    }
                    else {
                        result.push_back('_');
                    }
                }
                return result;
                };

            // Add SID metadata
            file << "// SID Metadata\n";
            file << ".var SIDName = \"" << cleanString(std::string(header.name)) << "\"\n";
            file << ".var SIDAuthor = \"" << cleanString(std::string(header.author)) << "\"\n";
            file << ".var SIDCopyright = \"" << cleanString(std::string(header.copyright)) << "\"\n\n";
            file << "\n";
        }

        // Add player code
        file << "* = PlayerADDR\n";
        file << ".import source \"" << playerAsmFile.string() << "\"\n";
        file << "\n";

        if (bIsASM)
        {
            u16 sidLoad = options.sidLoadAddr;
            file << "* = $" << util::wordToHex(sidLoad) << "\n";
            file << ".import source \"" << musicFile.string() << "\"\n";
            file << "\n";
        }

        file.close();

        util::Logger::debug("Created player linker file: " + linkerFile.string());

        return true;
    }

    bool MusicBuilder::runAssembler(
        const fs::path& sourceFile,
        const fs::path& outputFile,
        const std::string& kickAssPath) {

        // Build the command line
        const std::string kickCommand = kickAssPath + " " +
            sourceFile.string() + " -o " +
            outputFile.string();

        util::Logger::debug("Assembling: " + kickCommand);
        const int result = std::system(kickCommand.c_str());

        if (result != 0) {
            util::Logger::error("Failed to assemble: " + sourceFile.string());
            return false;
        }

        util::Logger::info("Assembly successful: " + outputFile.string());
        return true;
    }

    bool MusicBuilder::compressPrg(
        const fs::path& inputPrg,
        const fs::path& outputPrg,
        u16 loadAddress,
        const BuildOptions& options) {

        // Build compression command based on compressor type
        std::string compressCommand;

        if (options.compressorType == "exomizer") {
            compressCommand = options.exomizerPath + " sfx " + std::to_string(loadAddress) +
                " -x 3 -q \"" + inputPrg.string() + "\" -o \"" + outputPrg.string() + "\"";
        }
        else if (options.compressorType == "pucrunch") {
            std::string pucrunchPath = "pucrunch"; // Default, should be configurable
            compressCommand = pucrunchPath + " -x " + std::to_string(loadAddress) +
                " \"" + inputPrg.string() + "\" \"" + outputPrg.string() + "\"";
        }
        else {
            util::Logger::error("Unsupported compressor type: " + options.compressorType);
            return false;
        }

        // Execute the compression command
        util::Logger::debug("Compressing with command: " + compressCommand);
        const int result = std::system(compressCommand.c_str());

        if (result != 0) {
            util::Logger::error("Compression failed: " + compressCommand);
            return false;
        }

        util::Logger::info("Compressed PRG created: " + outputPrg.string());
        return true;
    }

    bool MusicBuilder::extractPrgFromSid(const fs::path& sidFile, const fs::path& outputPrg) {
        // Read the SID file
        std::ifstream input(sidFile, std::ios::binary);
        if (!input) {
            util::Logger::error("Failed to open SID file for extraction: " + sidFile.string());
            return false;
        }

        // Read header to determine data offset
        SIDHeader header;
        input.read(reinterpret_cast<char*>(&header), sizeof(header));

        // Fix endianness (SID files are big-endian)
        u16 dataOffset = (header.dataOffset >> 8) | (header.dataOffset << 8);
        u16 loadAddress = (header.loadAddress >> 8) | (header.loadAddress << 8);

        // Handle embedded load address if present
        if (loadAddress == 0) {
            // If load address is 0, it's embedded in the file
            if (input.seekg(dataOffset, std::ios::beg)) {
                u8 lo, hi;
                input.read(reinterpret_cast<char*>(&lo), 1);
                input.read(reinterpret_cast<char*>(&hi), 1);
                loadAddress = (hi << 8) | lo;
                dataOffset += 2; // Skip these two bytes in subsequent copy
            }
            else {
                util::Logger::error("Error seeking to data in SID file");
                return false;
            }
        }

        // Log what we're doing
        util::Logger::debug("Extracting PRG from SID: " + sidFile.string() +
            " (load address: $" + util::wordToHex(loadAddress) +
            ", data offset: $" + util::wordToHex(dataOffset) + ")");

        // Create PRG file (first 2 bytes are load address in little-endian)
        std::ofstream output(outputPrg, std::ios::binary);
        if (!output) {
            util::Logger::error("Failed to create PRG file: " + outputPrg.string());
            return false;
        }

        // Write load address (little-endian)
        const u8 lo = loadAddress & 0xFF;
        const u8 hi = (loadAddress >> 8) & 0xFF;
        output.write(reinterpret_cast<const char*>(&lo), 1);
        output.write(reinterpret_cast<const char*>(&hi), 1);

        // Position to start of data
        if (!input.seekg(dataOffset, std::ios::beg)) {
            util::Logger::error("Error seeking to data in SID file");
            return false;
        }

        // Copy the data
        char buffer[4096];
        while (input) {
            input.read(buffer, sizeof(buffer));
            std::streamsize bytesRead = input.gcount();
            if (bytesRead > 0) {
                output.write(buffer, bytesRead);
            }
            else {
                break;
            }
        }

        util::Logger::debug("Extracted PRG data to: " + outputPrg.string());
        return true;
    }

} // namespace sidblaster
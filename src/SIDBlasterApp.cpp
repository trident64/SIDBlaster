#define _CRT_SECURE_NO_WARNINGS  // Add this to handle strncpy/localtime warnings

#include "SIDBlasterApp.h"
#include "Disassembler.h"
#include "SIDLoader.h"
#include "cpu6510.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <cctype>

namespace sidblaster {

    SIDBlasterApp::SIDBlasterApp(int argc, char** argv)
        : cmdParser_(argc, argv) {
        setupCommandLine();
    }

    int SIDBlasterApp::run() {
        // Initialize logging
        initializeLogging();

        // Parse command line
        if (!parseCommandLine()) {
            return 1;
        }

        // Process input file
        return processFile();
    }

    void SIDBlasterApp::setupCommandLine() {
        // General options
        cmdParser_.addOptionDefinition("output", "file", "Output file path", "General", "");
        cmdParser_.addOptionDefinition("logfile", "file", "Log file path", "Logging", "SIDBlaster.log");
        cmdParser_.addOptionDefinition("relocate", "address", "Relocation address for the SID", "SID", "");

        // SID address options
        cmdParser_.addOptionDefinition("sidloadaddr", "address", "Override SID load address", "SID",
            util::wordToHex(util::Configuration::getDefaultSidLoadAddress()));
        cmdParser_.addOptionDefinition("sidinitaddr", "address", "Override SID init address", "SID",
            util::wordToHex(util::Configuration::getDefaultSidInitAddress()));
        cmdParser_.addOptionDefinition("sidplayaddr", "address", "Override SID play address", "SID",
            util::wordToHex(util::Configuration::getDefaultSidPlayAddress()));

        // Player options
        cmdParser_.addOptionDefinition("player", "name", "Player name", "Player", util::Configuration::getPlayerName());
        cmdParser_.addOptionDefinition("playeraddr", "address", "Player load address", "Player",
            util::wordToHex(util::Configuration::getPlayerAddress()));

        // Tool path options
        cmdParser_.addOptionDefinition("kickass", "path", "Path to KickAss.jar", "Tools", util::Configuration::getKickAssPath());
        cmdParser_.addOptionDefinition("exomizer", "path", "Path to Exomizer", "Tools", util::Configuration::getExomizerPath());
        cmdParser_.addOptionDefinition("compressor", "type", "Compression tool to use", "Tools", util::Configuration::getCompressorType());

        // Flags
        cmdParser_.addFlagDefinition("verbose", "Enable verbose logging", "Logging");
        cmdParser_.addFlagDefinition("help", "Display this help message", "General");
        cmdParser_.addFlagDefinition("noplayer", "Don't link player code", "Player");
        cmdParser_.addFlagDefinition("nocompress", "Don't compress output PRG files", "Output");

        // Add example usages
        cmdParser_.addExample(
            "SIDBlaster music.sid",
            "Processes music.sid with default settings (creates player-linked PRG)");

        cmdParser_.addExample(
            "SIDBlaster -output music.asm music.sid",
            "Disassembles music.sid to an assembly file");

        cmdParser_.addExample(
            "SIDBlaster -output music.prg -noplayer music.sid",
            "Creates a PRG file without player code");

        cmdParser_.addExample(
            "SIDBlaster -relocate $2000 -output relocated.sid music.sid",
            "Relocates music.sid to $2000 and creates a new SID file");

        cmdParser_.addExample(
            "SIDBlaster -player BassoonTracker -playeraddr $0800 music.sid",
            "Uses the BassoonTracker player at address $0800");
    }

    void SIDBlasterApp::initializeLogging() {
        // Get log file path from command line
        logFile_ = fs::path(cmdParser_.getOption("logfile", "SIDBlaster.log"));

        // Set log level based on verbose flag
        verbose_ = cmdParser_.hasFlag("verbose");
        const auto logLevel = verbose_ ?
            util::Logger::Level::Debug :
            util::Logger::Level::Info;

        // Initialize logger
        util::Logger::initialize(logFile_);
        util::Logger::setLogLevel(logLevel);

        util::Logger::info(SIDBLASTER_VERSION " started");
    }

    bool SIDBlasterApp::parseCommandLine() {
        // Check for help
        if (cmdParser_.hasFlag("help")) {
            cmdParser_.printUsage(SIDBLASTER_VERSION);
            return false;
        }

        // Get input file
        inputFile_ = cmdParser_.getInputFile();
        if (inputFile_.empty()) {
            cmdParser_.printUsage("No input file specified");
            return false;
        }

        // Check if input file exists
        if (!fs::exists(inputFile_)) {
            util::Logger::error("Input file not found: " + inputFile_.string());
            return false;
        }

        // Get output file
        outputFile_ = fs::path(cmdParser_.getOption("output", ""));

        // If no output file specified, create default based on input filename
        if (outputFile_.empty()) {
            outputFile_ = fs::path(inputFile_).stem().string() + ".prg";
            util::Logger::info("No output file specified, defaulting to: " + outputFile_.string());
        }

        // Determine output format from extension
        std::string ext = outputFile_.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return std::tolower(c); });

        if (ext == ".prg") {
            outputFormat_ = OutputFormat::PRG;
        }
        else if (ext == ".sid") {
            outputFormat_ = OutputFormat::SID;
        }
        else if (ext == ".asm") {
            outputFormat_ = OutputFormat::ASM;
        }
        else {
            util::Logger::warning("Unknown output extension: " + ext + ", defaulting to PRG");
            outputFormat_ = OutputFormat::PRG;
            // Append .prg extension if none provided
            if (outputFile_.extension().empty()) {
                outputFile_ = fs::path(outputFile_.string() + ".prg");  // Fix: Use string() instead of += with path
            }
        }

        // Check player options
        includePlayer_ = !cmdParser_.hasFlag("noplayer");

        // Create temp directory
        tempDir_ = fs::path("temp");
        try {
            fs::create_directories(tempDir_);
        }
        catch (const std::exception& e) {
            util::Logger::error(std::string("Failed to create temp directory: ") + e.what());
            return false;
        }

        // Parse relocation address
        std::string relocAddressStr = cmdParser_.getOption("relocate", "");
        if (!relocAddressStr.empty()) {
            auto relocAddr = util::parseHex(relocAddressStr);
            if (relocAddr) {
                relocAddress_ = *relocAddr;
                hasRelocation_ = true;
                util::Logger::debug("Relocation address set to $" + util::wordToHex(relocAddress_));
            }
            else {
                util::Logger::error("Invalid relocation address: " + relocAddressStr);
                return false;
            }
        }

        // Parse player settings
        playerName_ = cmdParser_.getOption("player", util::Configuration::getPlayerName());

        std::string playerAddrStr = cmdParser_.getOption("playeraddr",
            util::wordToHex(util::Configuration::getPlayerAddress()));

        // Add $ prefix if not present for proper hex parsing
        if (!playerAddrStr.empty() && playerAddrStr[0] != '$' &&
            (playerAddrStr.size() < 2 || playerAddrStr.substr(0, 2) != "0x")) {
            playerAddrStr = "$" + playerAddrStr;
        }

        auto playerAddr = util::parseHex(playerAddrStr);
        if (playerAddr) {
            playerAddress_ = *playerAddr;
            util::Logger::debug("Player address set to $" + util::wordToHex(playerAddress_) +
                " (decimal: " + std::to_string(playerAddress_) + ")");
        }
        else {
            util::Logger::error("Invalid player address: " + playerAddrStr);
            return false;
        }

        // Parse override addresses
        std::string initAddrStr = cmdParser_.getOption("sidinitaddr", "");
        if (!initAddrStr.empty()) {
            // Add $ prefix if not present
            if (initAddrStr[0] != '$' && (initAddrStr.size() < 2 || initAddrStr.substr(0, 2) != "0x")) {
                initAddrStr = "$" + initAddrStr;
            }

            auto initAddr = util::parseHex(initAddrStr);
            if (initAddr) {
                overrideInitAddress_ = *initAddr;
                hasOverrideInit_ = true;
                util::Logger::debug("Override init address: $" + util::wordToHex(overrideInitAddress_));
            }
            else {
                util::Logger::error("Invalid init address: " + initAddrStr);
                return false;
            }
        }

        std::string playAddrStr = cmdParser_.getOption("sidplayaddr", "");
        if (!playAddrStr.empty()) {
            // Add $ prefix if not present
            if (playAddrStr[0] != '$' && (playAddrStr.size() < 2 || playAddrStr.substr(0, 2) != "0x")) {
                playAddrStr = "$" + playAddrStr;
            }

            auto playAddr = util::parseHex(playAddrStr);
            if (playAddr) {
                overridePlayAddress_ = *playAddr;
                hasOverridePlay_ = true;
                util::Logger::debug("Override play address: $" + util::wordToHex(overridePlayAddress_));
            }
            else {
                util::Logger::error("Invalid play address: " + playAddrStr);
                return false;
            }
        }

        std::string loadAddrStr = cmdParser_.getOption("sidloadaddr", "");
        if (!loadAddrStr.empty()) {
            // Add $ prefix if not present
            if (loadAddrStr[0] != '$' && (loadAddrStr.size() < 2 || loadAddrStr.substr(0, 2) != "0x")) {
                loadAddrStr = "$" + loadAddrStr;
            }

            auto loadAddr = util::parseHex(loadAddrStr);
            if (loadAddr) {
                overrideLoadAddress_ = *loadAddr;
                hasOverrideLoad_ = true;
                util::Logger::debug("Override load address: $" + util::wordToHex(overrideLoadAddress_));
            }
            else {
                util::Logger::error("Invalid load address: " + loadAddrStr);
                return false;
            }
        }

        return true;
    }

    int SIDBlasterApp::processFile() {
        util::Logger::info("Processing file: " + inputFile_.string());

        // Base name for files
        std::string basename = fs::path(inputFile_).stem().string();

        // Setup temporary file paths
        fs::path tempExtractedPrg = tempDir_ / (basename + "-original.prg");
        fs::path tempAsmFile = tempDir_ / (basename + ".asm");
        fs::path tempPrgFile = tempDir_ / (basename + ".prg");
        fs::path tempPlayerPrgFile = tempDir_ / (basename + "-player.prg");
        fs::path tempCompressedPrgFile = tempDir_ / (basename + "-compressed.prg");
        fs::path tempSidFile = tempDir_ / (basename + ".sid");
        fs::path tempLinkerFile = tempDir_ / (basename + "-linker.asm");

        // Initialize CPU and SID loader
        CPU6510 cpu;
        cpu.reset();

        SIDLoader sid;
        sid.setCPU(&cpu);

        // Load the input file
        bool loaded = false;
        const fs::path extension = inputFile_.extension();

        // Handle different input file types
        if (extension == ".prg") {
            loaded = loadPrgFile(sid, tempExtractedPrg);
        }
        else if (extension == ".bin") {
            loaded = loadBinFile(sid, tempExtractedPrg);
        }
        else if (extension == ".sid") {
            loaded = loadSidFile(sid, tempExtractedPrg);
        }
        else {
            util::Logger::error("Unsupported file type: " + inputFile_.string());
            return 1;
        }

        if (!loaded) {
            util::Logger::error("Failed to load file: " + inputFile_.string());
            return 1;
        }

        // Get SID info
        const u16 sidLoad = sid.getLoadAddress();
        const u16 sidInit = sid.getInitAddress();
        const u16 sidPlay = sid.getPlayAddress();

        util::Logger::info("SID info - Load: $" + util::wordToHex(sidLoad) +
            ", Init: $" + util::wordToHex(sidInit) +
            ", Play: $" + util::wordToHex(sidPlay));

        // Initialize the SID and calculate play calls per frame
        // Track CIA timer writes
        u8 CIATimerLo = 0;
        u8 CIATimerHi = 0;
        cpu.setOnCIAWriteCallback([&](u16 addr, u8 value) {
            if (addr == 0xDC04) CIATimerLo = value;
            if (addr == 0xDC05) CIATimerHi = value;
            });

        Disassembler disasm(cpu, sid);

        // Create a backup of memory before execution
        sid.backupMemory();

        // Initialize the SID
        cpu.executeFunction(sidInit);

        // Calculate play calls per frame
        int playCallsPerFrame = calculatePlayCallsPerFrame(sid, CIATimerLo, CIATimerHi);
        sid.setNumPlayCallsPerFrame(playCallsPerFrame);

        util::Logger::info("Play calls per frame: " + std::to_string(playCallsPerFrame) +
            " (" + (sid.isPAL() ? "PAL" : "NTSC") + ")");

        // Call play routine for analysis
        const int numFrames = 30000;
        util::Logger::debug("Calling SID play routine " + std::to_string(numFrames) + " times");

        u64 cycles = cpu.getCycles();
        u64 lastCycles = cycles;
        u64 maxCycles = 0;

        for (int frame = 0; frame < numFrames; ++frame) {
            cpu.executeFunction(sidPlay);
            const u64 cur = cpu.getCycles();
            const u64 frameCycles = cur - lastCycles;
            maxCycles = std::max(maxCycles, frameCycles);
            lastCycles = cur;
        }

        util::Logger::debug("Maximum cycles per frame: " + std::to_string(maxCycles));

        // Calculate new addresses if relocating
        u16 newSidLoad;
        if (hasRelocation_) {
            newSidLoad = relocAddress_;
        }
        else {
            newSidLoad = sidLoad; // Use original load address if not relocating
        }

        u16 newSidInit = newSidLoad + (sidInit - sidLoad);
        u16 newSidPlay = newSidLoad + (sidPlay - sidLoad);

        if (hasRelocation_) {
            util::Logger::info("Relocated addresses - Load: $" + util::wordToHex(newSidLoad) +
                ", Init: $" + util::wordToHex(newSidInit) +
                ", Play: $" + util::wordToHex(newSidPlay));
        }

        // Process based on output format and relocation needs
        fs::path finalOutputPath;

        // If we need ASM output or relocation, generate ASM
        bool needAsm = (outputFormat_ == OutputFormat::ASM || hasRelocation_);

        if (needAsm) {
            // Restore original memory for clean disassembly
            sid.restoreMemory();

            // Generate assembly file
            disasm.generateAsmFile(tempAsmFile.string(), newSidLoad, newSidInit, newSidPlay);
            util::Logger::info("Generated assembly file: " + tempAsmFile.string());

            // If ASM is the final output, use it
            if (outputFormat_ == OutputFormat::ASM) {
                finalOutputPath = tempAsmFile;
            }
            else {
                // Otherwise, assemble it to PRG
                buildPureMusic(basename, tempAsmFile, tempPrgFile);

                // If SID output with relocation, generate it
                if (outputFormat_ == OutputFormat::SID) {
                    generateSIDFile(tempPrgFile, tempSidFile, newSidLoad, newSidInit, newSidPlay);
                    finalOutputPath = tempSidFile;
                }
                // For PRG output with player
                else if (outputFormat_ == OutputFormat::PRG && includePlayer_) {
                    buildWithPlayer(basename, tempAsmFile, tempLinkerFile, tempPlayerPrgFile,
                        playCallsPerFrame, sid.isPAL(), newSidInit, newSidPlay);

                    // Apply compression if not disabled
                    if (!cmdParser_.hasFlag("nocompress")) {
                        if (compressPrg(tempPlayerPrgFile, tempCompressedPrgFile, playerAddress_)) {
                            finalOutputPath = tempCompressedPrgFile;
                        }
                        else {
                            // Fallback to uncompressed if compression fails
                            finalOutputPath = tempPlayerPrgFile;
                            util::Logger::warning("Using uncompressed PRG due to compression failure");
                        }
                    }
                    else {
                        finalOutputPath = tempPlayerPrgFile;
                    }
                }
                // For PRG output without player
                else {
                    // Apply compression if not disabled
                    if (!cmdParser_.hasFlag("nocompress")) {
                        if (compressPrg(tempPrgFile, tempCompressedPrgFile, newSidLoad)) {
                            finalOutputPath = tempCompressedPrgFile;
                        }
                        else {
                            // Fallback to uncompressed if compression fails
                            finalOutputPath = tempPrgFile;
                            util::Logger::warning("Using uncompressed PRG due to compression failure");
                        }
                    }
                    else {
                        finalOutputPath = tempPrgFile;
                    }
                }
            }
        }
        // If we don't need ASM generation (no relocation, direct output)
        else {
            // For PRG output with player
            if (outputFormat_ == OutputFormat::PRG && includePlayer_) {
                // Use the original SID file directly when possible
                fs::path musicFile;
                if (extension == ".sid") {
                    // Use the original SID file directly
                    musicFile = inputFile_; // Use original SID directly
                    util::Logger::debug("Using original SID file directly: " + musicFile.string());
                }
                else {
                    // For other input types, use the extracted PRG
                    musicFile = tempExtractedPrg;
                }

                // Create a linker file using the original address
                buildWithPlayer(basename, musicFile, tempLinkerFile, tempPlayerPrgFile,
                    playCallsPerFrame, sid.isPAL(), sidInit, sidPlay, true);

                // Apply compression if not disabled
                if (!cmdParser_.hasFlag("nocompress")) {
                    if (compressPrg(tempPlayerPrgFile, tempCompressedPrgFile, playerAddress_)) {
                        finalOutputPath = tempCompressedPrgFile;
                    }
                    else {
                        // Fallback to uncompressed if compression fails
                        finalOutputPath = tempPlayerPrgFile;
                        util::Logger::warning("Using uncompressed PRG due to compression failure");
                    }
                }
                else {
                    finalOutputPath = tempPlayerPrgFile;
                }
            }
            // For PRG output without player, use extracted PRG
            else if (outputFormat_ == OutputFormat::PRG) {
                // Apply compression if not disabled
                if (!cmdParser_.hasFlag("nocompress")) {
                    if (compressPrg(tempExtractedPrg, tempCompressedPrgFile, sidLoad)) {
                        finalOutputPath = tempCompressedPrgFile;
                    }
                    else {
                        // Fallback to uncompressed if compression fails
                        finalOutputPath = tempExtractedPrg;
                        util::Logger::warning("Using uncompressed PRG due to compression failure");
                    }
                }
                else {
                    finalOutputPath = tempExtractedPrg;
                }
            }
            // For SID output without relocation, use original SID or create new
            else if (outputFormat_ == OutputFormat::SID) {
                if (extension == ".sid" && !hasOverrideInit_ && !hasOverridePlay_ && !hasOverrideLoad_) {
                    // Copy original SID file since no relocation or overrides needed
                    fs::copy_file(inputFile_, tempSidFile, fs::copy_options::overwrite_existing);
                }
                else {
                    // Generate SID from PRG using original or overridden addresses
                    generateSIDFile(tempExtractedPrg, tempSidFile, sidLoad, sidInit, sidPlay);
                }
                finalOutputPath = tempSidFile;
            }
        }

        // Copy the final output file to the requested location
        try {
            fs::copy_file(finalOutputPath, outputFile_, fs::copy_options::overwrite_existing);
            util::Logger::info("Final output generated: " + outputFile_.string());
        }
        catch (const std::exception& e) {
            util::Logger::error(std::string("Failed to copy final output: ") + e.what());
            return 1;
        }

        util::Logger::info("Processing complete: " + inputFile_.string());
        return 0;
    }

    bool SIDBlasterApp::loadSidFile(SIDLoader& sid, const fs::path& extractedPrgPath) {
        bool loaded = sid.loadSID(inputFile_.string());

        if (loaded) {
            // Apply overrides after loading
            if (hasOverrideInit_) {
                util::Logger::debug("Overriding SID init address: $" +
                    util::wordToHex(overrideInitAddress_));
                sid.setInitAddress(overrideInitAddress_);
            }

            if (hasOverridePlay_) {
                util::Logger::debug("Overriding SID play address: $" +
                    util::wordToHex(overridePlayAddress_));
                sid.setPlayAddress(overridePlayAddress_);
            }

            if (hasOverrideLoad_) {
                util::Logger::debug("Overriding SID load address: $" +
                    util::wordToHex(overrideLoadAddress_));
                sid.setLoadAddress(overrideLoadAddress_);
            }

            // Extract PRG data from SID to temp file
            extractPrgFromSid(inputFile_, extractedPrgPath);
        }

        return loaded;
    }

    bool SIDBlasterApp::loadPrgFile(SIDLoader& sid, const fs::path& extractedPrgPath) {
        // For PRG files, we need init and play addresses
        u16 init = hasOverrideInit_ ? overrideInitAddress_ : util::Configuration::getDefaultSidInitAddress();
        u16 play = hasOverridePlay_ ? overridePlayAddress_ : util::Configuration::getDefaultSidPlayAddress();

        bool loaded = sid.loadPRG(inputFile_.string(), init, play);

        if (loaded) {
            // Copy the PRG to extracted path
            try {
                fs::copy_file(inputFile_, extractedPrgPath, fs::copy_options::overwrite_existing);
            }
            catch (const std::exception& e) {
                util::Logger::warning(std::string("Failed to copy PRG file: ") + e.what());
                // Not critical, continue anyway
            }
        }

        return loaded;
    }

    bool SIDBlasterApp::loadBinFile(SIDLoader& sid, const fs::path& extractedPrgPath) {
        // For BIN files, we need load, init, and play addresses
        u16 load = hasOverrideLoad_ ? overrideLoadAddress_ : util::Configuration::getDefaultSidLoadAddress();
        u16 init = hasOverrideInit_ ? overrideInitAddress_ : util::Configuration::getDefaultSidInitAddress();
        u16 play = hasOverridePlay_ ? overridePlayAddress_ : util::Configuration::getDefaultSidPlayAddress();

        bool loaded = sid.loadBIN(inputFile_.string(), load, init, play);

        if (loaded) {
            // Create a PRG file from the BIN with load address header
            std::ifstream binFile(inputFile_, std::ios::binary | std::ios::ate);
            if (binFile) {
                size_t size = static_cast<size_t>(binFile.tellg());
                binFile.seekg(0, std::ios::beg);

                // Read BIN data
                std::vector<char> buffer(size);
                binFile.read(buffer.data(), size);
                binFile.close();

                // Write as PRG with load address header
                std::ofstream prgFile(extractedPrgPath, std::ios::binary);
                if (prgFile) {
                    // Write load address (little-endian)
                    u8 lo = load & 0xFF;
                    u8 hi = (load >> 8) & 0xFF;
                    prgFile.write(reinterpret_cast<const char*>(&lo), 1);
                    prgFile.write(reinterpret_cast<const char*>(&hi), 1);

                    // Write data
                    prgFile.write(buffer.data(), size);
                    prgFile.close();
                }
                else {
                    util::Logger::warning("Failed to create PRG from BIN");
                }
            }
        }

        return loaded;
    }

    void SIDBlasterApp::extractPrgFromSid(const fs::path& sidFile, const fs::path& outputPrg) {
        // Read the SID file
        std::ifstream input(sidFile, std::ios::binary);
        if (!input) {
            util::Logger::error("Failed to open SID file for extraction: " + sidFile.string());
            return;
        }

        // Read header to determine data offset
        SIDHeader header;
        input.read(reinterpret_cast<char*>(&header), sizeof(header));

        // Fix endianness (SID files use big-endian)
        header.dataOffset = (header.dataOffset >> 8) | (header.dataOffset << 8);
        header.loadAddress = (header.loadAddress >> 8) | (header.loadAddress << 8);

        // Skip to data portion
        input.seekg(header.dataOffset, std::ios::beg);

        // Create PRG file (first 2 bytes are load address in little-endian)
        std::ofstream output(outputPrg, std::ios::binary);
        if (!output) {
            util::Logger::error("Failed to create PRG file: " + outputPrg.string());
            return;
        }

        // Write load address (little-endian)
        const u8 lo = header.loadAddress & 0xFF;
        const u8 hi = (header.loadAddress >> 8) & 0xFF;
        output.write(reinterpret_cast<const char*>(&lo), 1);
        output.write(reinterpret_cast<const char*>(&hi), 1);

        // Copy the rest of the data
        output << input.rdbuf();

        util::Logger::debug("Extracted PRG data to: " + outputPrg.string());
    }

    int SIDBlasterApp::calculatePlayCallsPerFrame(
        const SIDLoader& sid,
        u8 CIATimerLo,
        u8 CIATimerHi) {

        const uint32_t speedBits = sid.getHeader().speed;
        int count = 0;

        // Count bits in speed field
        for (int i = 0; i < 32; ++i) {
            if (speedBits & (1u << i)) {
                ++count;
            }
        }

        int numPlayCallsPerFrame = std::clamp(count == 0 ? 1 : count, 1, 16);

        // Check for CIA timer
        if ((CIATimerLo != 0) || (CIATimerHi != 0)) {
            const u16 timerValue = CIATimerLo | (CIATimerHi << 8);
            const double NumCyclesPerFrame = sid.isPAL() ? (63.0 * 312.0) : (65.0 * 263.0);
            const double freq = NumCyclesPerFrame / std::max(1, static_cast<int>(timerValue));
            const int numCalls = static_cast<int>(freq + 0.5);
            numPlayCallsPerFrame = std::clamp(numCalls, 1, 16);
        }

        return numPlayCallsPerFrame;
    }

    bool SIDBlasterApp::buildPureMusic(
        const std::string& basename,
        const fs::path& asmFile,
        const fs::path& outputPrg) {

        // Get KickAss path from configuration or command line
        const std::string kickAssPath = cmdParser_.getOption("kickass", util::Configuration::getKickAssPath());

        // Build the command line
        const std::string kickCommand = kickAssPath + " " +
            asmFile.string() + " -o " +
            outputPrg.string();

        util::Logger::debug("Assembling pure music: " + kickCommand);
        const int result = std::system(kickCommand.c_str());

        if (result != 0) {
            util::Logger::error("Failed to assemble pure music: " + asmFile.string());
            return false;
        }

        util::Logger::info("Pure music PRG generated: " + outputPrg.string());
        return true;
    }

    bool SIDBlasterApp::buildWithPlayer(
        const std::string& basename,
        const fs::path& musicFile,  // Can be either ASM, PRG or SID
        const fs::path& linkerFile,
        const fs::path& outputPrg,
        int playCallsPerFrame,
        bool isPAL,
        u16 sidInit,
        u16 sidPlay,
        bool isPrgInput) {  // Flag to indicate if music file is PRG

        // Get KickAss path
        const std::string kickAssPath = cmdParser_.getOption("kickass", util::Configuration::getKickAssPath());
        const fs::path playerAsmFile = fs::path("SIDPlayers") / playerName_ / (playerName_ + ".asm");

        // Create the player directory if it doesn't exist
        fs::create_directories(playerAsmFile.parent_path());

        // Create the linker file with appropriate content
        if (!createPlayerLinkerFile(linkerFile, musicFile, playerAsmFile,
            playCallsPerFrame, isPAL, sidInit, sidPlay, isPrgInput)) {
            return false;
        }

        // Build the command line for KickAss
        const std::string kickCommand = kickAssPath + " " +
            linkerFile.string() + " -o " +
            outputPrg.string();

        util::Logger::debug("Assembling with player: " + kickCommand);
        const int result = std::system(kickCommand.c_str());

        if (result != 0) {
            util::Logger::error("Failed to assemble with player: " + linkerFile.string());
            return false;
        }

        util::Logger::info("Player-linked PRG generated: " + outputPrg.string());
        return true;
    }

    bool SIDBlasterApp::createPlayerLinkerFile(
        const fs::path& linkerFile,
        const fs::path& musicFile,
        const fs::path& playerAsmFile,
        int playCallsPerFrame,
        bool isPAL,
        u16 sidInit,
        u16 sidPlay,
        bool isPrgInput) {

        std::ofstream file(linkerFile);
        if (!file) {
            util::Logger::error("Failed to create linker file: " + linkerFile.string());
            return false;
        }

        // Write the linker file header
        file << "//; ------------------------------------------\n";
        file << "//; SIDBlaster Player Linker\n";
        file << "//; ------------------------------------------\n\n";

        // Define player variables
        file << ".var NumCallsPerFrame = " << playCallsPerFrame << "\n";
        file << ".var IsPAL = " << (isPAL ? "true" : "false") << "\n";
        file << ".var PlayerADDR = $" << util::wordToHex(playerAddress_) << "\n";
        file << ".var SIDInit = $" << util::wordToHex(sidInit) << "\n";
        file << ".var SIDPlay = $" << util::wordToHex(sidPlay) << "\n\n";

        // Add player code
        file << "* = PlayerADDR\n";
        file << ".import source \"" << playerAsmFile.string() << "\"\n\n";

        if (isPrgInput) {
            // Determine if it's a SID or PRG file based on extension
            std::string ext = musicFile.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(),
                [](unsigned char c) { return std::tolower(c); });

            if (ext == ".sid") {
                file << ".var music_prg = LoadSid(\"" << musicFile.string() << "\")\n";
                file << "* = music_prg.location \"SID\"\n";
                file << ".fill music_prg.size, music_prg.getData(i)\n";
            }
            else {
                file << ".import c64 \"" << musicFile.string() << "\"\n";
            }
        }
        else {
            // For ASM input, use source include
            file << "// Including ASM music source\n";
            file << ".import source \"" << musicFile.string() << "\"\n";
        }
        file.close();

        // Log the actual values for debugging
        util::Logger::debug("Created player linker file: " + linkerFile.string());
        util::Logger::debug("  PlayerADDR = $" + util::wordToHex(playerAddress_));
        util::Logger::debug("  SIDInit = $" + util::wordToHex(sidInit));
        util::Logger::debug("  SIDPlay = $" + util::wordToHex(sidPlay));

        return true;
    }

    bool SIDBlasterApp::compressPrg(
        const fs::path& inputPrg,
        const fs::path& outputPrg,
        u16 loadAddress) {

        // Get compressor settings
        const std::string compressorType = cmdParser_.getOption("compressor", util::Configuration::getCompressorType());

        // Build compression command based on compressor type
        std::string compressCommand;

        if (compressorType == "exomizer") {
            const std::string exomizerPath = cmdParser_.getOption("exomizer", util::Configuration::getExomizerPath());
            compressCommand = exomizerPath + " sfx " + std::to_string(loadAddress) + " -x 3 -q \"" +
                inputPrg.string() + "\" -o \"" + outputPrg.string() + "\"";
        }
        else if (compressorType == "pucrunch") {
            const std::string pucrunchPath = cmdParser_.getOption("pucrunch", "pucrunch");
            compressCommand = pucrunchPath + " -x " + std::to_string(loadAddress) + " \"" +
                inputPrg.string() + "\" \"" + outputPrg.string() + "\"";
        }
        else {
            util::Logger::error("Unsupported compressor type: " + compressorType);
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

    bool SIDBlasterApp::generateSIDFile(
        const fs::path& sourcePrgFile,
        const fs::path& outputSidFile,
        u16 loadAddr,
        u16 initAddr,
        u16 playAddr) {

        // Check if source PRG file exists
        if (!fs::exists(sourcePrgFile)) {
            util::Logger::error("Source PRG file not found: " + sourcePrgFile.string());
            return false;
        }

        // Create a new SID header with the specified addresses
        SIDHeader header = {};

        // Set magic ID
        memcpy(header.magicID, "PSID", 4);

        // Set version and standard values
        header.version = 2;
        header.dataOffset = 0x7C; // Standard offset for v2 header
        header.loadAddress = loadAddr;
        header.initAddress = initAddr;
        header.playAddress = playAddr;
        header.songs = 1;
        header.startSong = 1;

        // If the input file is a SID, try to copy original metadata
        if (inputFile_.extension() == ".sid") {
            std::ifstream originalSid(inputFile_, std::ios::binary);
            if (originalSid) {
                SIDHeader originalHeader;
                originalSid.read(reinterpret_cast<char*>(&originalHeader), sizeof(originalHeader));

                // Copy metadata fields (name, author, copyright)
                memcpy(header.name, originalHeader.name, 32);
                memcpy(header.author, originalHeader.author, 32);
                memcpy(header.copyright, originalHeader.copyright, 32);

                // Copy other fields that should persist
                header.flags = (originalHeader.flags >> 8) | (originalHeader.flags << 8); // Fix endianness
                header.speed = originalHeader.speed;
                header.startPage = originalHeader.startPage;
                header.pageLength = originalHeader.pageLength;
            }
        }
        else {
            // Set default metadata for non-SID input
            std::string basename = fs::path(inputFile_).stem().string();
            std::time_t now = std::time(nullptr);

            // Use safe string copy
#ifdef _WIN32
            strncpy_s(header.name, sizeof(header.name), basename.c_str(), _TRUNCATE);
            strncpy_s(header.author, sizeof(header.author), "SIDBlaster", _TRUNCATE);

            // Create timestamp
            char timestamp[32];
            struct tm timeInfo;
            localtime_s(&timeInfo, &now);
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeInfo);
            strncpy_s(header.copyright, sizeof(header.copyright), timestamp, _TRUNCATE);
#else
    // Use standard strncpy with explicit null termination
            strncpy(header.name, basename.c_str(), sizeof(header.name) - 1);
            header.name[sizeof(header.name) - 1] = '\0';

            strncpy(header.author, "SIDBlaster", sizeof(header.author) - 1);
            header.author[sizeof(header.author) - 1] = '\0';

            // Create timestamp
            char timestamp[32];
            struct tm timeInfo;
            localtime_r(&now, &timeInfo);
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeInfo);
            strncpy(header.copyright, timestamp, sizeof(header.copyright) - 1);
            header.copyright[sizeof(header.copyright) - 1] = '\0';
#endif
        }

        // Read the PRG file
        std::ifstream prgFile(sourcePrgFile, std::ios::binary | std::ios::ate);
        if (!prgFile) {
            util::Logger::error("Failed to open PRG file: " + sourcePrgFile.string());
            return false;
        }

        // Get PRG file size
        std::streamsize prgSize = prgFile.tellg();
        prgFile.seekg(0, std::ios::beg);

        // Read PRG content
        std::vector<u8> prgData(prgSize);
        prgFile.read(reinterpret_cast<char*>(prgData.data()), prgSize);
        prgFile.close();

        // Create the new SID file
        std::ofstream newSidFile(outputSidFile, std::ios::binary);
        if (!newSidFile) {
            util::Logger::error("Failed to create SID file: " + outputSidFile.string());
            return false;
        }

        // Fix endianness for SID file (big-endian)
        SIDHeader outHeader = header;
        fixHeaderEndianness(outHeader);

        // Write the header
        newSidFile.write(reinterpret_cast<const char*>(&outHeader), sizeof(outHeader));

        // Write the PRG data (skip the first 2 bytes which are the load address)
        if (prgSize > 2) {
            newSidFile.write(reinterpret_cast<const char*>(prgData.data() + 2), prgSize - 2);
        }
        else {
            util::Logger::warning("PRG file is too small, may not contain valid data");
        }

        newSidFile.close();

        util::Logger::info("SID file generated: " + outputSidFile.string());
        return true;
    }

    void SIDBlasterApp::fixHeaderEndianness(SIDHeader& header) {
        // SID files store multi-byte values in big-endian format
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
    }

} // namespace sidblaster
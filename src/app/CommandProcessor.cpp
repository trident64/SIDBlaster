// ==================================
//             SIDBlaster
//
//  Raistlin / Genesis Project (G*P)
// ==================================
#include "CommandProcessor.h"
#include "../SIDBlasterUtils.h"
#include "../cpu6510.h"
#include "../SIDEmulator.h"
#include "../SIDLoader.h"
#include "../Disassembler.h"
#include "../RelocationUtils.h"
#include "MusicBuilder.h"

#include <algorithm>
#include <fstream>
#include <cctype>

namespace sidblaster {

    CommandProcessor::CommandProcessor() {
        // Initialize CPU
        cpu_ = std::make_unique<CPU6510>();
        cpu_->reset();

        // Initialize SID Loader
        sid_ = std::make_unique<SIDLoader>();
        sid_->setCPU(cpu_.get());
    }

    CommandProcessor::~CommandProcessor() {
        // Ensure trace logger is closed properly
        traceLogger_.reset();
    }

    bool CommandProcessor::processFile(const ProcessingOptions& options) {
        try {
            util::Logger::info("Processing file: " + options.inputFile.string());

            // Create temp directory if it doesn't exist
            fs::create_directories(options.tempDir);

            // Set up tracing if enabled
            if (options.enableTracing && !options.traceLogPath.empty()) {
                traceLogger_ = std::make_unique<TraceLogger>(options.traceLogPath, options.traceFormat);

                // Set up callback for SID writes
                cpu_->setOnSIDWriteCallback([this](u16 addr, u8 value) {
                    traceLogger_->logSIDWrite(addr, value);
                    });

                // Set up callback for CIA writes
                cpu_->setOnCIAWriteCallback([this](u16 addr, u8 value) {
                    traceLogger_->logCIAWrite(addr, value);
                    });

                util::Logger::info("Trace logging enabled to: " + options.traceLogPath);
            }

            // Load the input file
            if (!loadInputFile(options)) {
                return false;
            }

            // Apply any metadata overrides
            applySIDMetadataOverrides(options);

            // Analyze the music
            if (!analyzeMusic(options)) {
                return false;
            }

            // Generate the output file
            if (!generateOutput(options)) {
                return false;
            }

            util::Logger::info("Processing complete: " + options.inputFile.string());
            return true;
        }
        catch (const std::exception& e) {
            util::Logger::error(std::string("Error processing file: ") + e.what());
            return false;
        }
    }

    bool CommandProcessor::loadInputFile(const ProcessingOptions& options) {
        // Base name for temporary files
        std::string basename = options.inputFile.stem().string();

        // Setup temporary file paths
        fs::path tempExtractedPrg = options.tempDir / (basename + "-original.prg");

        if (getFileExtension(options.inputFile) != ".sid")
        {
            util::Logger::error("Unsupported file type: " + options.inputFile.string() + " - only SID files accepted.");
            return false;
        }
        if (!loadSidFile(options, tempExtractedPrg)) {
            util::Logger::error("Failed to load file: " + options.inputFile.string());
            return false;
        }

        return true;
    }


    bool CommandProcessor::loadSidFile(const ProcessingOptions& options, const fs::path& tempExtractedPrg) {
        bool loaded = sid_->loadSID(options.inputFile.string());

        if (loaded) {
            // Apply overrides if specified
            if (options.hasOverrideInit) {
                util::Logger::debug("Overriding SID init address: $" +
                    util::wordToHex(options.overrideInitAddress));
                sid_->setInitAddress(options.overrideInitAddress);
            }

            if (options.hasOverridePlay) {
                util::Logger::debug("Overriding SID play address: $" +
                    util::wordToHex(options.overridePlayAddress));
                sid_->setPlayAddress(options.overridePlayAddress);
            }

            if (options.hasOverrideLoad) {
                util::Logger::debug("Overriding SID load address: $" +
                    util::wordToHex(options.overrideLoadAddress));
                sid_->setLoadAddress(options.overrideLoadAddress);
            }

            // Extract PRG data from SID to temp file
            // Create a temporary MusicBuilder to use its extractPrgFromSid method
            MusicBuilder builder(cpu_.get(), sid_.get());
            builder.extractPrgFromSid(options.inputFile, tempExtractedPrg);
        }

        return loaded;
    }

    void CommandProcessor::applySIDMetadataOverrides(const ProcessingOptions& options) {
        // Apply overrides from command line
        if (!options.overrideTitle.empty()) {
            sid_->setTitle(options.overrideTitle);
            util::Logger::debug("Overriding SID title: " + options.overrideTitle);
        }

        if (!options.overrideAuthor.empty()) {
            sid_->setAuthor(options.overrideAuthor);
            util::Logger::debug("Overriding SID author: " + options.overrideAuthor);
        }

        if (!options.overrideCopyright.empty()) {
            sid_->setCopyright(options.overrideCopyright);
            util::Logger::debug("Overriding SID copyright: " + options.overrideCopyright);
        }
    }

    bool CommandProcessor::analyzeMusic(const ProcessingOptions& options) {
        // Track CIA timer writes
        u8 CIATimerLo = 0;
        u8 CIATimerHi = 0;
        cpu_->setOnCIAWriteCallback([&](u16 addr, u8 value) {
            if (addr == 0xDC04) CIATimerLo = value;
            if (addr == 0xDC05) CIATimerHi = value;
            });

        // Set up Disassembler if needed
//        if (options.outputFormat == OutputFormat::ASM || options.hasRelocation) {
            disassembler_ = std::make_unique<Disassembler>(*cpu_, *sid_);
//        }

        // Set up emulation options
        SIDEmulator emulator(cpu_.get(), sid_.get());
        SIDEmulator::EmulationOptions emulationOptions;
        emulationOptions.frames = DEFAULT_SID_EMULATION_FRAMES;
        emulationOptions.backupAndRestore = true;
        emulationOptions.traceEnabled = options.enableTracing;
        emulationOptions.traceFormat = options.traceFormat;
        emulationOptions.traceLogPath = options.traceLogPath;

        // Run the emulation
        if (!emulator.runEmulation(emulationOptions)) {
            util::Logger::error("SID emulation failed");
            return false;
        }

        // Get SID info
        const u16 sidLoad = sid_->getLoadAddress();
        const u16 sidInit = sid_->getInitAddress();
        const u16 sidPlay = sid_->getPlayAddress();

        util::Logger::info("SID info - Load: $" + util::wordToHex(sidLoad) +
            ", Init: $" + util::wordToHex(sidInit) +
            ", Play: $" + util::wordToHex(sidPlay));

        // Calculate play calls per frame
        int playCallsPerFrame = calculatePlayCallsPerFrame(CIATimerLo, CIATimerHi);
        sid_->setNumPlayCallsPerFrame(playCallsPerFrame);

        util::Logger::info("Play calls per frame: " + std::to_string(playCallsPerFrame));

        // Get cycle statistics
        auto [avgCycles, maxCycles] = emulator.getCycleStats();
        util::Logger::debug("Maximum cycles per frame: " + std::to_string(maxCycles));

        return true;
    }

    int CommandProcessor::calculatePlayCallsPerFrame(u8 CIATimerLo, u8 CIATimerHi) {
        const uint32_t speedBits = sid_->getHeader().speed;
        int count = 0;

        // Count bits in speed field
        for (int i = 0; i < 32; ++i) {
            if (speedBits & (1u << i)) {
                ++count;
            }
        }

        // Default to 1 call per frame if no speed bits set, or clamp to 1-16 range
        int numPlayCallsPerFrame = std::clamp(count == 0 ? 1 : count, 1, 16);

        // Check for CIA timer
        if ((CIATimerLo != 0) || (CIATimerHi != 0)) {
            const u16 timerValue = CIATimerLo | (CIATimerHi << 8);
            const double NumCyclesPerFrame = (63.0 * 312.0); //; TODO: NTSC?
            const double freq = NumCyclesPerFrame / std::max(1, static_cast<int>(timerValue));
            const int numCalls = static_cast<int>(freq + 0.5);
            numPlayCallsPerFrame = std::clamp(numCalls, 1, 16);
        }

        return numPlayCallsPerFrame;
    }

    bool CommandProcessor::generateOutput(const ProcessingOptions& options) {

        // Determine new addresses for relocation
        u16 newSidLoad;
        u16 newSidInit;
        u16 newSidPlay;

        const u16 sidLoad = sid_->getLoadAddress();
        const u16 sidInit = sid_->getInitAddress();
        const u16 sidPlay = sid_->getPlayAddress();

        if (options.hasRelocation) {
            newSidLoad = options.relocationAddress;
            // Calculate offset for init and play addresses
            newSidInit = newSidLoad + (sidInit - sidLoad);
            newSidPlay = newSidLoad + (sidPlay - sidLoad);

            util::Logger::info("Relocated addresses - Load: $" + util::wordToHex(newSidLoad) +
                ", Init: $" + util::wordToHex(newSidInit) +
                ", Play: $" + util::wordToHex(newSidPlay));
        }
        else {
            // Use original addresses if not relocating
            newSidLoad = sidLoad;
            newSidInit = sidInit;
            newSidPlay = sidPlay;
        }

        // Generate the appropriate output format
        std::string ext = getFileExtension(options.outputFile);
        if (ext == ".prg") {
            return generatePRGOutput(options);
        }
        else if (ext == ".sid") {
            return generateSIDOutput(options);
        }
        else if (ext == ".asm") {
            return generateASMOutput(options);
        }
        util::Logger::error("Unsupported output format");
        return false;
    }

    bool CommandProcessor::generatePRGOutput(const ProcessingOptions& options) {

        // Base name for files
        std::string basename = options.inputFile.stem().string();

        // Setup temporary file paths
        fs::path tempDir = options.tempDir;
        fs::path tempExtractedPrg = tempDir / (basename + "-original.prg");
        fs::path tempAsmFile = tempDir / (basename + ".asm");
        fs::path tempPrgFile = tempDir / (basename + ".prg");

        bool bRelocation = options.hasRelocation;

        bool bIsSID = (getFileExtension(options.inputFile) == ".sid");

        u16 newSidLoad = options.relocationAddress;

        // If we're using a player and have a SID file as input
        if (options.includePlayer && bIsSID)
        {
            if (!bRelocation)
            {
                newSidLoad = sid_->getLoadAddress();
            }
            bRelocation = true;
        }

        // If the input file is a SID and we haven't extracted it yet, do so now
        if ((!bRelocation) && (bIsSID) && (!fs::exists(tempExtractedPrg))) {
            util::Logger::debug("Extracting PRG from SID file: " + options.inputFile.string());
            MusicBuilder builder(cpu_.get(), sid_.get());
            builder.extractPrgFromSid(options.inputFile, tempExtractedPrg);
        }

        // Determine if we need to use the disassembler for relocation
        if (bRelocation) {
            // Restore original memory for clean disassembly
            sid_->restoreMemory();

            // Generate assembly file with relocated addresses
            const u16 sidLoad = sid_->getLoadAddress();
            const u16 newSidInit = newSidLoad + (sid_->getInitAddress() - sidLoad);
            const u16 newSidPlay = newSidLoad + (sid_->getPlayAddress() - sidLoad);

            disassembler_->generateAsmFile(tempAsmFile.string(), newSidLoad, newSidInit, newSidPlay);
            util::Logger::info("Generated relocated assembly: " + tempAsmFile.string());

            // Run assembler to build pure music
            MusicBuilder builder(cpu_.get(), sid_.get());
            MusicBuilder::BuildOptions buildOptions;
            buildOptions.includePlayer = options.includePlayer;
            buildOptions.playerName = options.playerName;
            buildOptions.playerAddress = options.playerAddress;
            buildOptions.compress = options.compress;
            buildOptions.tempDir = tempDir;
            buildOptions.sidLoadAddr = newSidLoad;
            buildOptions.sidInitAddr = newSidInit;
            buildOptions.sidPlayAddr = newSidPlay;

            return builder.buildMusic(basename, tempAsmFile, options.outputFile, buildOptions);
        }
        else {
            // No relocation, use original extracted PRG
            MusicBuilder builder(cpu_.get(), sid_.get());
            MusicBuilder::BuildOptions buildOptions;
            buildOptions.includePlayer = options.includePlayer;
            buildOptions.playerName = options.playerName;
            buildOptions.playerAddress = options.playerAddress;
            buildOptions.compress = options.compress;
            buildOptions.tempDir = tempDir;

            return builder.buildMusic(basename, tempExtractedPrg, options.outputFile, buildOptions);
        }
    }

    bool CommandProcessor::generateSIDOutput(const ProcessingOptions& options) {
        // Check if relocation is requested
        if (options.hasRelocation) {
            // Setup parameters for relocation
            util::RelocationParams params;
            params.inputFile = options.inputFile;
            params.outputFile = options.outputFile;
            params.tempDir = options.tempDir;
            params.relocationAddress = options.relocationAddress;
            params.kickAssPath = options.kickAssPath;
//;            params.verbose = options.verbose;

            util::Logger::info("Relocating " + options.inputFile.string() + " to $" +
                util::wordToHex(options.relocationAddress) + " -> " + options.outputFile.string());

            // Perform the relocation
            util::RelocationResult result = util::relocateSID(cpu_.get(), sid_.get(), params);

            // Return success/failure
            return result.success;
        }
        else {
            // No relocation requested - just create a SID file

            // Different handling based on input type

            std::string ext = getFileExtension(options.inputFile);

            if (ext == ".sid") {
                // Input is already a SID - just copy it (or extract/modify if needed)
                try {
                    fs::copy_file(options.inputFile, options.outputFile, fs::copy_options::overwrite_existing);
                    return true;
                }
                catch (const std::exception& e) {
                    util::Logger::error(std::string("Failed to copy SID file: ") + e.what());
                    return false;
                }
            }
            else if (ext == ".prg") {
                // Input is PRG - need to create a SID file

                // We need load, init, and play addresses
                u16 loadAddr = options.hasOverrideLoad ?
                    options.overrideLoadAddress : util::Configuration::getDefaultSidLoadAddress();
                u16 initAddr = options.hasOverrideInit ?
                    options.overrideInitAddress : util::Configuration::getDefaultSidInitAddress();
                u16 playAddr = options.hasOverridePlay ?
                    options.overridePlayAddress : util::Configuration::getDefaultSidPlayAddress();

                // Create SID from PRG
                bool success = util::createSIDFromPRG(
                    options.inputFile,
                    options.outputFile,
                    loadAddr,
                    initAddr,
                    playAddr,
                    options.overrideTitle,
                    options.overrideAuthor,
                    options.overrideCopyright);

                if (!success) {
                    util::Logger::warning("SID file creation not yet implemented. Copying PRG instead.");

                    try {
                        fs::copy_file(options.inputFile, options.outputFile, fs::copy_options::overwrite_existing);
                        return true;
                    }
                    catch (const std::exception& e) {
                        util::Logger::error(std::string("Failed to copy PRG file: ") + e.what());
                        return false;
                    }
                }

                return success;
            }
            else {
                util::Logger::error("Unsupported input file type for SID output");
                return false;
            }
        }
    }

    bool CommandProcessor::generateASMOutput(const ProcessingOptions& options) {
        // Base name for files
        std::string basename = options.inputFile.stem().string();

        // Setup temporary file paths
        fs::path tempDir = options.tempDir;

        // Restore original memory for clean disassembly
        sid_->restoreMemory();

        // Determine if we're relocating
        u16 outputSidLoad = options.hasRelocation ?
            options.relocationAddress : sid_->getLoadAddress();

        // Generate disassembly with adjusted addresses as needed
        const u16 sidLoad = sid_->getLoadAddress();
        const u16 newSidInit = outputSidLoad + (sid_->getInitAddress() - sidLoad);
        const u16 newSidPlay = outputSidLoad + (sid_->getPlayAddress() - sidLoad);

        int unusedBytes = disassembler_->generateAsmFile(
            options.outputFile.string(), outputSidLoad, newSidInit, newSidPlay);

        util::Logger::info("Generated assembly file: " + options.outputFile.string() +
            " (" + std::to_string(unusedBytes) + " unused bytes removed)");

        return true;
    }

} // namespace sidblaster
// SIDBlasterApp.cpp
#include "SIDBlasterApp.h"
#include "CommandProcessor.h"
#include "RelocationUtils.h"
#include "SIDBlasterUtils.h"
#include "cpu6510.h"
#include "SIDLoader.h"
#include <iostream>
#include <filesystem>

namespace sidblaster {

    SIDBlasterApp::SIDBlasterApp(int argc, char** argv)
        : cmdParser_(argc, argv) {
        // Setup command line options
        setupCommandLine();
    }

    int SIDBlasterApp::run() {
        // Initialize logging
        initializeLogging();

        // Parse command line into command object
        command_ = cmdParser_.parse();

        // Execute the command
        return executeCommand();
    }

    void SIDBlasterApp::setupCommandLine() {
        // SID metadata options
        cmdParser_.addOptionDefinition("title", "text", "Override SID title", "SID", "");
        cmdParser_.addOptionDefinition("author", "text", "Override SID author", "SID", "");
        cmdParser_.addOptionDefinition("copyright", "text", "Override SID copyright", "SID", "");

        // SID address options
        cmdParser_.addOptionDefinition("relocate", "address", "Relocation address for the SID", "SID", "");
        cmdParser_.addOptionDefinition("sidloadaddr", "address", "Override SID load address", "SID",
            "$" + util::wordToHex(util::Configuration::getDefaultSidLoadAddress()));
        cmdParser_.addOptionDefinition("sidinitaddr", "address", "Override SID init address", "SID",
            "$" + util::wordToHex(util::Configuration::getDefaultSidInitAddress()));
        cmdParser_.addOptionDefinition("sidplayaddr", "address", "Override SID play address", "SID",
            "$" + util::wordToHex(util::Configuration::getDefaultSidPlayAddress()));

        // Player options
        cmdParser_.addOptionDefinition("player", "name", "Player name (omit for no player, specify without name for default player)", "Player", "");
        cmdParser_.addOptionDefinition("playeraddr", "address", "Player load address", "Player",
            "$" + util::wordToHex(util::Configuration::getPlayerAddress()));
        cmdParser_.addOptionDefinition("playerdefs", "file", "Player definitions file", "Player", "");
        cmdParser_.addOptionDefinition("defs", "file", "General definitions file", "SID", "");

        // Tracelog options
        cmdParser_.addOptionDefinition("tracelog", "file", "Log file for SID register writes", "Logging", "");
        cmdParser_.addOptionDefinition("traceformat", "format", "Format for trace log (text/binary)", "Logging", "binary");

        // Other options
        cmdParser_.addOptionDefinition("logfile", "file", "Log file path", "Logging", "SIDBlaster.log");
        cmdParser_.addOptionDefinition("kickass", "path", "Path to KickAss.jar", "Tools", util::Configuration::getKickAssPath());
        cmdParser_.addOptionDefinition("exomizer", "path", "Path to Exomizer", "Tools", util::Configuration::getExomizerPath());
        cmdParser_.addOptionDefinition("compressor", "type", "Compression tool to use", "Tools", util::Configuration::getCompressorType());

        // Flags
        cmdParser_.addFlagDefinition("verbose", "Enable verbose logging", "Logging");
        cmdParser_.addFlagDefinition("help", "Display this help message", "General");
        cmdParser_.addFlagDefinition("nocompress", "Don't compress output PRG files", "Output");
        cmdParser_.addFlagDefinition("force", "Force overwrite output file", "Output");

        // Add example usages with new format
        cmdParser_.addExample(
            "SIDBlaster music.sid music.prg",
            "Processes music.sid with default settings (creates player-linked PRG)");

        cmdParser_.addExample(
            "SIDBlaster music.sid music.asm",
            "Disassembles music.sid to an assembly file");

        cmdParser_.addExample(
            "SIDBlaster -noplayer music.sid music.prg",
            "Creates a PRG file without player code");

        cmdParser_.addExample(
            "SIDBlaster -relocate=$2000 music.sid relocated.sid",
            "Relocates music.sid to $2000 and creates a new SID file");

        cmdParser_.addExample(
            "SIDBlaster -player=SimpleBitmap -playeraddr=$0800 music.sid player.prg",
            "Uses the SimpleBitmap player at address $0800");
    }

    void SIDBlasterApp::initializeLogging() {
        // Get log file path from command parameters
        logFile_ = fs::path(command_.getParameter("logfile", "SIDBlaster.log"));

        // Set log level based on verbose flag
        verbose_ = command_.hasFlag("verbose");
        const auto logLevel = verbose_ ?
            util::Logger::Level::Debug :
            util::Logger::Level::Info;

        // Initialize logger
        util::Logger::initialize(logFile_);
        util::Logger::setLogLevel(logLevel);

        util::Logger::info(SIDBLASTER_VERSION " started");
    }

    int SIDBlasterApp::executeCommand() {
        // Execute based on command type
        switch (command_.getType()) {
        case CommandClass::Type::Help:
            return showHelp();
        case CommandClass::Type::Convert:
            return processConversion();
        case CommandClass::Type::Relocate:
            return processRelocation();
        case CommandClass::Type::Disassemble:
            return processDisassembly();
        default:
            std::cout << "Unknown command type" << std::endl;
            return 1;
        }
    }

    CommandProcessor::ProcessingOptions SIDBlasterApp::createProcessingOptions() {
        CommandProcessor::ProcessingOptions options;

        // Get input and output files
        options.inputFile = fs::path(command_.getInputFile());
        options.outputFile = fs::path(command_.getOutputFile());

        // Determine output format from extension
        std::string ext = options.outputFile.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return std::tolower(c); });

        if (ext == ".prg") {
            options.outputFormat = CommandProcessor::OutputFormat::PRG;
        }
        else if (ext == ".sid") {
            options.outputFormat = CommandProcessor::OutputFormat::SID;
        }
        else if (ext == ".asm") {
            options.outputFormat = CommandProcessor::OutputFormat::ASM;
        }
        else {
            util::Logger::warning("Unknown output extension: " + ext + ", defaulting to PRG");
            options.outputFormat = CommandProcessor::OutputFormat::PRG;
            // Append .prg extension if none provided
            if (options.outputFile.extension().empty()) {
                options.outputFile = fs::path(options.outputFile.string() + ".prg");
            }
        }

        // Create temp directory
        options.tempDir = fs::path("temp");
        try {
            fs::create_directories(options.tempDir);
        }
        catch (const std::exception& e) {
            util::Logger::error(std::string("Failed to create temp directory: ") + e.what());
        }

        // Player options
        std::string playerOption = command_.getParameter("player", "");
        options.includePlayer = !playerOption.empty();
        options.playerName = playerOption.empty() ? "" : (playerOption == "player") ? "Default" : playerOption;

        // Player address
        options.playerAddress = command_.getHexParameter("playeraddr", util::Configuration::getPlayerAddress());

        // Build options
        options.compress = !command_.hasFlag("nocompress");
        options.compressorType = command_.getParameter("compressor", util::Configuration::getCompressorType());
        options.exomizerPath = command_.getParameter("exomizer", util::Configuration::getExomizerPath());
        options.kickAssPath = command_.getParameter("kickass", util::Configuration::getKickAssPath());

        // Parse relocation address
        std::string relocAddressStr = command_.getParameter("relocate", "");
        if (!relocAddressStr.empty()) {
            options.relocationAddress = command_.getHexParameter("relocate", 0);
            options.hasRelocation = true;
            util::Logger::debug("Relocation address set to $" + util::wordToHex(options.relocationAddress));
        }

        // Parse override addresses
        std::string initAddrStr = command_.getParameter("sidinitaddr", "");
        if (!initAddrStr.empty()) {
            options.overrideInitAddress = command_.getHexParameter("sidinitaddr", 0);
            options.hasOverrideInit = true;
            util::Logger::debug("Override init address: $" + util::wordToHex(options.overrideInitAddress));
        }

        std::string playAddrStr = command_.getParameter("sidplayaddr", "");
        if (!playAddrStr.empty()) {
            options.overridePlayAddress = command_.getHexParameter("sidplayaddr", 0);
            options.hasOverridePlay = true;
            util::Logger::debug("Override play address: $" + util::wordToHex(options.overridePlayAddress));
        }

        std::string loadAddrStr = command_.getParameter("sidloadaddr", "");
        if (!loadAddrStr.empty()) {
            options.overrideLoadAddress = command_.getHexParameter("sidloadaddr", 0);
            options.hasOverrideLoad = true;
            util::Logger::debug("Override load address: $" + util::wordToHex(options.overrideLoadAddress));
        }

        // SID metadata overrides
        options.overrideTitle = command_.getParameter("title", "");
        options.overrideAuthor = command_.getParameter("author", "");
        options.overrideCopyright = command_.getParameter("copyright", "");

        // Trace options
        options.traceLogPath = command_.getParameter("tracelog", "");
        options.enableTracing = !options.traceLogPath.empty();
        std::string traceFormat = command_.getParameter("traceformat", "binary");
        options.traceFormat = (traceFormat == "text") ?
            TraceFormat::Text : TraceFormat::Binary;

        return options;
    }

    int SIDBlasterApp::showHelp() {
        cmdParser_.printUsage(SIDBLASTER_VERSION);
        return 0;
    }

    int SIDBlasterApp::processConversion() {
        // Validate input file
        fs::path inputFile = fs::path(command_.getInputFile());
        fs::path outputFile = fs::path(command_.getOutputFile());

        if (inputFile.empty()) {
            std::cout << "Error: No input file specified" << std::endl;
            return 1;
        }

        if (outputFile.empty()) {
            std::cout << "Error: No output file specified" << std::endl;
            return 1;
        }

        if (!fs::exists(inputFile)) {
            std::cout << "Error: Input file not found: " << inputFile.string() << std::endl;
            return 1;
        }

        // Check for input/output file conflicts
        try {
            // Only check equivalence if output file already exists
            if (fs::exists(outputFile) && fs::equivalent(inputFile, outputFile)) {
                std::cout << "Error: Input and output files cannot be the same: " << inputFile.string() << std::endl;
                return 1;
            }
        }
        catch (const std::exception& e) {
            // Log the exception for debugging
            util::Logger::error("Error checking file equivalence: " + std::string(e.what()));

            // Alternative check: compare the canonicalized paths
            if (fs::absolute(inputFile) == fs::absolute(outputFile)) {
                std::cout << "Error: Input and output files cannot be the same" << std::endl;
                return 1;
            }
        }

        // Create processing options
        CommandProcessor::ProcessingOptions options = createProcessingOptions();

        // Create and run command processor
        CommandProcessor processor;
        bool success = processor.processFile(options);

        return success ? 0 : 1;
    }

    int SIDBlasterApp::processRelocation() {
        // Validate input file
        fs::path inputFile = fs::path(command_.getInputFile());
        fs::path outputFile = fs::path(command_.getOutputFile());

        if (inputFile.empty()) {
            std::cout << "Error: No input file specified" << std::endl;
            return 1;
        }

        if (outputFile.empty()) {
            std::cout << "Error: No output file specified" << std::endl;
            return 1;
        }

        if (!fs::exists(inputFile)) {
            std::cout << "Error: Input file not found: " << inputFile.string() << std::endl;
            return 1;
        }

        // Check if relocation address is specified
        std::string relocAddressStr = command_.getParameter("relocate", "");
        if (relocAddressStr.empty()) {
            std::cout << "Error: Relocation address (-relocate) must be specified for relocation" << std::endl;
            return 1;
        }

        // Create CPU and SID Loader
        auto cpu = std::make_unique<CPU6510>();
        cpu->reset();

        auto sid = std::make_unique<SIDLoader>();
        sid->setCPU(cpu.get());

        // Set up relocation parameters
        util::RelocationParams params;
        params.inputFile = inputFile;
        params.outputFile = outputFile;
        params.tempDir = fs::path("temp");
        params.relocationAddress = command_.getHexParameter("relocate", 0);
        params.kickAssPath = command_.getParameter("kickass", util::Configuration::getKickAssPath());
        params.verbose = command_.hasFlag("verbose");

        // Ensure temp directory exists
        try {
            fs::create_directories(params.tempDir);
        }
        catch (const std::exception& e) {
            util::Logger::error(std::string("Failed to create temp directory: ") + e.what());
            return 1;
        }

        // Relocate the SID file
        util::RelocationResult result = util::relocateSID(cpu.get(), sid.get(), params);

        if (result.success) {
            util::Logger::info("Successfully relocated " + inputFile.string() + " to " + outputFile.string() +
                " (Load: $" + util::wordToHex(result.newLoad) +
                ", Init: $" + util::wordToHex(result.newInit) +
                ", Play: $" + util::wordToHex(result.newPlay) + ")");
            return 0;
        }
        else {
            util::Logger::error("Failed to relocate " + inputFile.string() + ": " + result.message);
            return 1;
        }
    }

    int SIDBlasterApp::processDisassembly() {
        // This is similar to conversion but specifically for disassembly
        CommandProcessor::ProcessingOptions options = createProcessingOptions();
        options.outputFormat = CommandProcessor::OutputFormat::ASM;

        // Create and run command processor
        CommandProcessor processor;
        bool success = processor.processFile(options);

        return success ? 0 : 1;
    }

} // namespace sidblaster
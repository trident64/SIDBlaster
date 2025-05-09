// SIDBlasterApp.cpp
#include "SIDBlasterApp.h"
#include "CommandProcessor.h"
#include "RelocationUtils.h"
#include "SIDBlasterUtils.h"
#include "cpu6510.h"
#include "SIDLoader.h"
#include "SIDEmulator.h"
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
        // Command type flags
        cmdParser_.addFlagDefinition("linkplayer", "Link SID music with a player (convert .sid to playable .prg)", "Commands");
        cmdParser_.addFlagDefinition("relocate", "Relocate a SID file to a new address", "Commands");
        cmdParser_.addFlagDefinition("disassemble", "Disassemble a SID file to assembly code", "Commands");
        cmdParser_.addFlagDefinition("trace", "Trace SID register writes during emulation", "Commands");

        // LinkPlayer options
        cmdParser_.addOptionDefinition("linkplayertype", "name", "Player type to use (default: SimpleRaster)", "LinkPlayer", "SimpleRaster");
        cmdParser_.addOptionDefinition("linkplayeraddr", "address", "Player load address", "LinkPlayer",
            "$" + util::wordToHex(util::Configuration::getPlayerAddress()));
        cmdParser_.addOptionDefinition("linkplayerdefs", "file", "Player definitions file", "LinkPlayer", "");

        // Relocate options
        cmdParser_.addOptionDefinition("relocateaddr", "address", "Target address for SID relocation", "Relocate", "");

        // Trace options
        cmdParser_.addOptionDefinition("tracelog", "file", "Output file for SID register trace log", "Trace", "");
        cmdParser_.addOptionDefinition("traceformat", "format", "Format for trace log (text/binary)", "Trace", "binary");

        // General options
        cmdParser_.addOptionDefinition("logfile", "file", "Log file path", "General", "SIDBlaster.log");
        cmdParser_.addOptionDefinition("kickass", "path", "Path to KickAss.jar", "General", util::Configuration::getKickAssPath());

        // Flags
        cmdParser_.addFlagDefinition("verbose", "Enable verbose logging", "General");
        cmdParser_.addFlagDefinition("help", "Display this help message", "General");
        cmdParser_.addFlagDefinition("force", "Force overwrite of output file", "General");

        // Add example usages
        cmdParser_.addExample(
            "SIDBlaster -linkplayer music.sid music.prg",
            "Links music.sid with player code to create an executable music.prg");

        cmdParser_.addExample(
            "SIDBlaster -linkplayer -linkplayertype=SimpleBitmap -linkplayeraddr=$0800 music.sid player.prg",
            "Links music.sid with SimpleBitmap player at address $0800");

        cmdParser_.addExample(
            "SIDBlaster -relocate -relocateaddr=$2000 music.sid relocated.sid",
            "Relocates music.sid to $2000 and saves as relocated.sid");

        cmdParser_.addExample(
            "SIDBlaster -disassemble music.sid music.asm",
            "Disassembles music.sid to assembly code in music.asm");

        cmdParser_.addExample(
            "SIDBlaster -trace -tracelog=music.trace music.sid",
            "Traces SID register writes while executing music.sid");
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
        case CommandClass::Type::LinkPlayer:
            return processLinkPlayer();
        case CommandClass::Type::Relocate:
            return processRelocation();
        case CommandClass::Type::Disassemble:
            return processDisassembly();
        case CommandClass::Type::Trace:
            return processTrace();
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

        // Create temp directory
        options.tempDir = fs::path("temp");
        try {
            fs::create_directories(options.tempDir);
        }
        catch (const std::exception& e) {
            util::Logger::error(std::string("Failed to create temp directory: ") + e.what());
        }

        // Player options for LinkPlayer command
        if (command_.getType() == CommandClass::Type::LinkPlayer) {
            options.includePlayer = true;
            options.playerName = command_.getParameter("linkplayertype", "SimpleRaster");
            options.playerAddress = command_.getHexParameter("linkplayeraddr", util::Configuration::getPlayerAddress());
        }
        else {
            options.includePlayer = false;
        }

        // Build options
        options.kickAssPath = command_.getParameter("kickass", util::Configuration::getKickAssPath());

        // Parse relocation address for Relocate command
        if (command_.getType() == CommandClass::Type::Relocate) {
            options.relocationAddress = command_.getHexParameter("relocateaddr", 0);
            options.hasRelocation = true;
            util::Logger::debug("Relocation address set to $" + util::wordToHex(options.relocationAddress));
        }

        // Trace options
        options.traceLogPath = command_.getParameter("tracelog", "");
        options.enableTracing = !options.traceLogPath.empty() || (command_.getType() == CommandClass::Type::Trace);
        std::string traceFormat = command_.getParameter("traceformat", "binary");
        options.traceFormat = (traceFormat == "text") ?
            TraceFormat::Text : TraceFormat::Binary;

        return options;
    }

    int SIDBlasterApp::showHelp() {
        cmdParser_.printUsage(SIDBLASTER_VERSION);
        return 0;
    }

    int SIDBlasterApp::processLinkPlayer() {
        // Validate input file
        fs::path inputFile = fs::path(command_.getInputFile());
        fs::path outputFile = fs::path(command_.getOutputFile());

        if (inputFile.empty()) {
            std::cout << "Error: No input file specified for linkplayer command" << std::endl;
            return 1;
        }

        if (outputFile.empty()) {
            std::cout << "Error: No output file specified for linkplayer command" << std::endl;
            return 1;
        }

        if (!fs::exists(inputFile)) {
            std::cout << "Error: Input file not found: " << inputFile.string() << std::endl;
            return 1;
        }

        // Strictly enforce .sid input and .prg output
        std::string inExt = getFileExtension(inputFile);
        if (inExt != ".sid") {
            std::cout << "Error: LinkPlayer command requires a .sid input file, got: " << inExt << std::endl;
            return 1;
        }

        std::string outExt = getFileExtension(outputFile);
        if (outExt != ".prg") {
            std::cout << "Error: LinkPlayer command requires a .prg output file, got: " << outExt << std::endl;
            return 1;
        }

        // Create processing options
        CommandProcessor::ProcessingOptions options = createProcessingOptions();

        // Set LinkPlayer specific options
        options.includePlayer = true;
        options.playerName = command_.getParameter("linkplayertype", "SimpleRaster");
        options.playerAddress = command_.getHexParameter("linkplayeraddr", util::Configuration::getPlayerAddress());

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
            std::cout << "Error: No input file specified for relocate command" << std::endl;
            return 1;
        }

        if (outputFile.empty()) {
            std::cout << "Error: No output file specified for relocate command" << std::endl;
            return 1;
        }

        // Strictly enforce .sid extension for both input and output
        std::string inExt = getFileExtension(inputFile);
        if (inExt != ".sid") {
            std::cout << "Error: Relocate command requires a .sid input file, got: " << inExt << std::endl;
            return 1;
        }

        std::string outExt = getFileExtension(inputFile);
        if (outExt != ".sid") {
            std::cout << "Error: Relocate command requires a .sid output file, got: " << outExt << std::endl;
            return 1;
        }

        if (!fs::exists(inputFile)) {
            std::cout << "Error: Input file not found: " << inputFile.string() << std::endl;
            return 1;
        }

        // Check if relocation address is specified
        std::string relocAddressStr = command_.getParameter("relocateaddr", "");
        if (relocAddressStr.empty()) {
            std::cout << "Error: Relocation address (-relocateaddr) must be specified for relocate command" << std::endl;
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
        params.relocationAddress = command_.getHexParameter("relocateaddr", 0);
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
        // Validate input file
        fs::path inputFile = fs::path(command_.getInputFile());
        fs::path outputFile = fs::path(command_.getOutputFile());

        if (inputFile.empty()) {
            std::cout << "Error: No input file specified for disassemble command" << std::endl;
            return 1;
        }

        if (outputFile.empty()) {
            std::cout << "Error: No output file specified for disassemble command" << std::endl;
            return 1;
        }

        if (!fs::exists(inputFile)) {
            std::cout << "Error: Input file not found: " << inputFile.string() << std::endl;
            return 1;
        }

        // Strictly enforce .sid input and .asm output
        std::string inExt = getFileExtension(inputFile);
        if (inExt != ".sid") {
            std::cout << "Error: Disassemble command requires a .sid input file, got: " << inExt << std::endl;
            return 1;
        }

        std::string outExt = getFileExtension(inputFile);
        if (outExt != ".asm") {
            std::cout << "Error: Disassemble command requires an .asm output file, got: " << outExt << std::endl;
            return 1;
        }

        // Create processing options
        CommandProcessor::ProcessingOptions options = createProcessingOptions();

        // Create and run command processor
        CommandProcessor processor;
        bool success = processor.processFile(options);

        if (success) {
            util::Logger::info("Successfully disassembled " + inputFile.string() + " to " + outputFile.string());
        }
        else {
            util::Logger::error("Failed to disassemble " + inputFile.string());
        }

        return success ? 0 : 1;
    }

    int SIDBlasterApp::processTrace() {
        // Validate input file
        fs::path inputFile = fs::path(command_.getInputFile());

        if (inputFile.empty()) {
            std::cout << "Error: No input file specified for trace command" << std::endl;
            return 1;
        }

        if (!fs::exists(inputFile)) {
            std::cout << "Error: Input file not found: " << inputFile.string() << std::endl;
            return 1;
        }

        // Strictly enforce .sid extension for input
        std::string inExt = getFileExtension(inputFile);
        if (inExt != ".sid") {
            std::cout << "Error: Trace command requires a .sid input file, got: " << inExt << std::endl;
            return 1;
        }

        // Check for trace log path
        std::string traceLogPath = command_.getParameter("tracelog");
        if (traceLogPath.empty()) {
            // If not specified, create a default based on input filename
            traceLogPath = inputFile.stem().string() + ".trace";
        }

        // Determine trace format
        std::string traceFormatStr = command_.getParameter("traceformat", "binary");
        TraceFormat traceFormat = TraceFormat::Binary;
        if (traceFormatStr == "text") {
            traceFormat = TraceFormat::Text;
        }

        util::Logger::info("Tracing SID register writes for " + inputFile.string() +
            " to " + traceLogPath + " in " + traceFormatStr + " format");

        // Create CPU and SID Loader
        auto cpu = std::make_unique<CPU6510>();
        cpu->reset();

        auto sid = std::make_unique<SIDLoader>();
        sid->setCPU(cpu.get());

        // Load the SID file
        if (!sid->loadSID(inputFile.string())) {
            std::cout << "Error: Failed to load SID file: " << inputFile.string() << std::endl;
            return 1;
        }

        // Create trace logger
        auto traceLogger = std::make_unique<TraceLogger>(traceLogPath, traceFormat);

        // Set up callback for SID writes
        cpu->setOnSIDWriteCallback([&traceLogger](u16 addr, u8 value) {
            traceLogger->logSIDWrite(addr, value);
            });

        // Create emulator
        SIDEmulator emulator(cpu.get(), sid.get());
        SIDEmulator::EmulationOptions options;
        options.frames = DEFAULT_SID_EMULATION_FRAMES;
        options.backupAndRestore = false;
        options.traceEnabled = true;
        options.traceFormat = traceFormat;
        options.traceLogPath = traceLogPath;

        // Run the emulation
        bool success = emulator.runEmulation(options);

        if (success) {
            std::cout << "Successfully traced SID register writes to: " << traceLogPath << std::endl;
            return 0;
        }
        else {
            std::cout << "Error occurred during SID emulation" << std::endl;
            return 1;
        }
    }

} // namespace sidblaster
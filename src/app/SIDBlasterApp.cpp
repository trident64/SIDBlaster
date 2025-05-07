// ==================================
//             SIDBlaster
//
//  Raistlin / Genesis Project (G*P)
// ==================================
#include "SIDBlasterApp.h"
#include "BatchProcessor.h"
#include "../SIDBlasterUtils.h"
#include "../cpu6510.h"
#include "../SIDLoader.h"
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

        // Parse command line arguments
        if (!parseCommandLine()) {
            return 1;
        }

        // Check if we're in batch mode
        std::string batchFile = cmdParser_.getOption("batch");
        if (!batchFile.empty()) {
            return runBatchMode(batchFile);
        }

        // Check for wildcard mode (batch with wildcards)
        std::string inputFile = cmdParser_.getInputFile();
        std::string outputFile = cmdParser_.getOutputFile();
        bool hasWildcards = (inputFile.find('*') != std::string::npos ||
            inputFile.find('?') != std::string::npos ||
            outputFile.find('*') != std::string::npos ||
            outputFile.find('?') != std::string::npos);

        if (hasWildcards) {
            return runBatchWildcardMode();
        }

        // Otherwise, process a single file
        return runSingleFileMode();
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

        // Batch processing
        cmdParser_.addOptionDefinition("batch", "file", "Batch configuration file", "Batch", "");
        cmdParser_.addOptionDefinition("batchreport", "file", "Path for batch processing report", "Batch", "SIDBlaster-report.txt");
        cmdParser_.addFlagDefinition("preserve-subfolders", "Maintain subfolder structure in output", "Batch");
        cmdParser_.addFlagDefinition("verify", "Verify relocated SIDs by comparing outputs", "Batch");

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

        cmdParser_.addExample(
            "SIDBlaster -batch=jobs.txt",
            "Executes batch processing from configuration file");

        cmdParser_.addExample(
            "SIDBlaster -relocate=$2000 -verify SID/*.sid RelocatedSID/*.sid",
            "Process all SID files in SID folder, relocate to $2000, verify and output to RelocatedSID folder"
        );

        cmdParser_.addExample(
            "SIDBlaster -relocate=$1000 -preserve-subfolders \"Collections/C64/**/*.sid\" \"Relocated/*.sid\"",
            "Process all SID files in Collections/C64 and its subfolders, preserve folder structure in output"
        );

        cmdParser_.addExample(
            "SIDBlaster -relocate=$2000 -verify -batchreport=results.txt SID/*.sid RelocatedSID/*.sid",
            "Process all SIDs and save the verification report to results.txt"
        );
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

        // Check for batch mode
        std::string batchFile = cmdParser_.getOption("batch");
        if (!batchFile.empty()) {
            // In batch mode, we don't need input/output files
            return true;
        }

        // Get input and output files
        fs::path inputFile = fs::path(cmdParser_.getInputFile());
        fs::path outputFile = fs::path(cmdParser_.getOutputFile());

        // Check if the input/output files include wildcards
        std::string inputFileStr = cmdParser_.getInputFile();
        std::string outputFileStr = cmdParser_.getOutputFile();
        bool hasWildcards = inputFileStr.find('*') != std::string::npos ||
            inputFileStr.find('?') != std::string::npos ||
            outputFileStr.find('*') != std::string::npos ||
            outputFileStr.find('?') != std::string::npos;

        // Validate input and output files
        if (inputFile.empty()) {
            std::cout << "Error: No input file specified\n";
            std::cout << "Use -help for command line options\n";
            return false;
        }

        if (outputFile.empty()) {
            std::cout << "Error: No output file specified\n";
            std::cout << "Use -help for command line options\n";
            return false;
        }

        // For wildcard mode, verify relocation address is specified
        if (hasWildcards) {
            std::string relocateOption = cmdParser_.getOption("relocate");
            if (relocateOption.empty()) {
                std::cout << "Error: Relocation address (-relocate) must be specified when using wildcards\n";
                std::cout << "Use -help for command line options\n";
                return false;
            }

            // Validate the wildcard patterns
            if (inputFileStr.find('*') == std::string::npos && inputFileStr.find('?') == std::string::npos) {
                std::cout << "Warning: Output path contains wildcards but input path doesn't.\n";
                std::cout << "Did you mean to use wildcards in the input path?\n";
            }

            // For wildcard mode, we don't need to check file existence
            util::Logger::info("Wildcard mode detected - will process multiple files");
            return true;
        }

        // For non-wildcard mode, check if input file exists
        if (!fs::exists(inputFile)) {
            std::cout << "Error: Input file not found: " << inputFile.string() << "\n";
            std::cout << "Use -help for command line options\n";
            return false;
        }

        // Check for input/output file conflicts (only for non-wildcard mode)
        try {
            // Only check equivalence if output file already exists
            if (fs::exists(outputFile) && fs::equivalent(inputFile, outputFile)) {
                std::cout << "Error: Input and output files cannot be the same: " << inputFile.string() << "\n";
                std::cout << "Use -help for command line options\n";
                return false;
            }
        }
        catch (const std::exception& e) {
            // Log the exception for debugging
            util::Logger::error("Error checking file equivalence: " + std::string(e.what()));

            // Alternative check: compare the canonicalized paths
            if (fs::absolute(inputFile) == fs::absolute(outputFile)) {
                std::cout << "Error: Input and output files cannot be the same\n";
                std::cout << "Use -help for command line options\n";
                return false;
            }
        }

        return true;
    }

    CommandProcessor::ProcessingOptions SIDBlasterApp::createProcessingOptions() {
        CommandProcessor::ProcessingOptions options;

        // Get input and output files
        options.inputFile = fs::path(cmdParser_.getInputFile());
        options.outputFile = fs::path(cmdParser_.getOutputFile());

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
        std::string playerOption = cmdParser_.getOption("player", "");
        options.includePlayer = !playerOption.empty();
        options.playerName = playerOption.empty() ? "" : (playerOption == "player") ? "Default" : playerOption;

        // Parse player address
        std::string playerAddrStr = cmdParser_.getOption("playeraddr",
            "$" + util::wordToHex(util::Configuration::getPlayerAddress()));
        auto playerAddr = util::parseHex(playerAddrStr);
        if (playerAddr) {
            options.playerAddress = *playerAddr;
        }

        // Build options
        options.compress = !cmdParser_.hasFlag("nocompress");
        options.compressorType = cmdParser_.getOption("compressor", util::Configuration::getCompressorType());
        options.exomizerPath = cmdParser_.getOption("exomizer", util::Configuration::getExomizerPath());
        options.kickAssPath = cmdParser_.getOption("kickass", util::Configuration::getKickAssPath());

        // Parse relocation address
        std::string relocAddressStr = cmdParser_.getOption("relocate", "");
        if (!relocAddressStr.empty()) {
            auto relocAddr = util::parseHex(relocAddressStr);
            if (relocAddr) {
                options.relocationAddress = *relocAddr;
                options.hasRelocation = true;
                util::Logger::debug("Relocation address set to $" + util::wordToHex(options.relocationAddress));
            }
            else {
                util::Logger::error("Invalid relocation address: " + relocAddressStr);
            }
        }

        // Parse override addresses
        std::string initAddrStr = cmdParser_.getOption("sidinitaddr", "");
        if (!initAddrStr.empty()) {
            auto initAddr = util::parseHex(initAddrStr);
            if (initAddr) {
                options.overrideInitAddress = *initAddr;
                options.hasOverrideInit = true;
                util::Logger::debug("Override init address: $" + util::wordToHex(options.overrideInitAddress));
            }
            else {
                util::Logger::error("Invalid init address: " + initAddrStr);
            }
        }

        std::string playAddrStr = cmdParser_.getOption("sidplayaddr", "");
        if (!playAddrStr.empty()) {
            auto playAddr = util::parseHex(playAddrStr);
            if (playAddr) {
                options.overridePlayAddress = *playAddr;
                options.hasOverridePlay = true;
                util::Logger::debug("Override play address: $" + util::wordToHex(options.overridePlayAddress));
            }
            else {
                util::Logger::error("Invalid play address: " + playAddrStr);
            }
        }

        std::string loadAddrStr = cmdParser_.getOption("sidloadaddr", "");
        if (!loadAddrStr.empty()) {
            auto loadAddr = util::parseHex(loadAddrStr);
            if (loadAddr) {
                options.overrideLoadAddress = *loadAddr;
                options.hasOverrideLoad = true;
                util::Logger::debug("Override load address: $" + util::wordToHex(options.overrideLoadAddress));
            }
            else {
                util::Logger::error("Invalid load address: " + loadAddrStr);
            }
        }

        // SID metadata overrides
        options.overrideTitle = cmdParser_.getOption("title", "");
        options.overrideAuthor = cmdParser_.getOption("author", "");
        options.overrideCopyright = cmdParser_.getOption("copyright", "");

        // Trace options
        options.traceLogPath = cmdParser_.getOption("tracelog", "");
        options.enableTracing = !options.traceLogPath.empty();
        std::string traceFormat = cmdParser_.getOption("traceformat", "binary");
        options.traceFormat = (traceFormat == "text") ?
            TraceFormat::Text : TraceFormat::Binary;

        return options;
    }

    int SIDBlasterApp::runBatchMode(const std::string& batchFile) {
        util::Logger::info("Running in batch mode with config: " + batchFile);

        // Create CPU and SID Loader for the batch processor
        auto cpu = std::make_unique<CPU6510>();
        cpu->reset();

        auto sid = std::make_unique<SIDLoader>();
        sid->setCPU(cpu.get());

        // Create and run batch converter
        std::string reportPath = cmdParser_.getOption("batchreport");
        BatchConverter batchConverter(batchFile, cpu.get(), sid.get(), reportPath);
        bool success = batchConverter.execute();

        return success ? 0 : 1;
    }

    int SIDBlasterApp::runSingleFileMode() {
        // Create processing options from command line
        CommandProcessor::ProcessingOptions options = createProcessingOptions();

        // Create and run command processor
        CommandProcessor processor;
        bool success = processor.processFile(options);

        return success ? 0 : 1;
    }

    int SIDBlasterApp::runBatchWildcardMode() {
        // Check if we have wildcard patterns
        std::string inputFile = cmdParser_.getInputFile();
        std::string outputFile = cmdParser_.getOutputFile();

        // Check if either contains wildcards
        bool hasWildcards = (inputFile.find('*') != std::string::npos ||
            inputFile.find('?') != std::string::npos ||
            outputFile.find('*') != std::string::npos ||
            outputFile.find('?') != std::string::npos);

        if (!hasWildcards) {
            return runSingleFileMode(); // Handle as normal file
        }

        // Set up batch processor options
        BatchProcessor processor;
        BatchProcessor::BatchOptions options;

        options.inputPattern = fs::path(inputFile);
        options.outputPattern = fs::path(outputFile);
        options.reportPath = fs::path(cmdParser_.getOption("batchreport", "batchreport.txt"));
        options.tempDir = fs::path("temp");
        options.preserveSubfolders = cmdParser_.hasFlag("preserve-subfolders");

        // Get relocation address
        std::string relocAddressStr = cmdParser_.getOption("relocate", "");
        if (!relocAddressStr.empty()) {
            auto relocAddr = util::parseHex(relocAddressStr);
            if (relocAddr) {
                options.relocationAddress = *relocAddr;
            }
            else {
                std::cout << "Error: Invalid relocation address: " << relocAddressStr << std::endl;
                return 1;
            }
        }
        else {
            std::cout << "Error: Relocation address must be specified for batch wildcard mode" << std::endl;
            return 1;
        }

        // Set verification option
        options.verify = cmdParser_.hasFlag("verify");

        // Process the batch
        try {
            BatchProcessor::BatchResult result = processor.processBatch(options);
            return 0; // Success
        }
        catch (const std::exception& e) {
            std::cout << "Error during batch processing: " << e.what() << std::endl;
            return 1;
        }
    }

} // namespace sidblaster
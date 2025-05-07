// ==================================
//             SIDBlaster
//
//  Raistlin / Genesis Project (G*P)
// ==================================
#include "BatchConverter.h"
#include "cpu6510.h"
#include "SIDBlasterUtils.h"
#include "SIDEmulator.h"
#include "SIDLoader.h"
#include "RelocationUtils.h"
#include "TraceLogger.h"
#include <fstream>
#include <regex>

namespace sidblaster {

    BatchConverter::BatchConverter(const std::string& configFile, CPU6510* cpu, SIDLoader* sid, const std::string& reportPath)
        : configPath_(configFile), cpu_(cpu), sid_(sid), reportPath_(reportPath) {
    }

    bool BatchConverter::execute() {
        try {
            if (!loadConfiguration()) {
                util::Logger::error("Failed to load batch configuration file: " + configPath_);
                return false;
            }

            return processFiles();
        }
        catch (const std::exception& e) {
            util::Logger::error("Exception during batch processing: " + std::string(e.what()));
            return false;
        }
    }

    bool BatchConverter::loadConfiguration() {
        std::ifstream config(configPath_);
        if (!config.is_open()) {
            util::Logger::error("Could not open batch file: " + configPath_);
            return false;
        }

        std::string line;
        std::string currentTask;
        BatchTask task;

        while (std::getline(config, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#' || line[0] == ';') {
                continue;
            }

            // Check for task header
            if (line[0] == '[' && line.back() == ']') {
                // If we already have a task, add it to the list
                if (!currentTask.empty()) {
                    tasks_[currentTask] = task;
                    task = BatchTask(); // Reset for new task
                }

                // Extract task name
                currentTask = line.substr(1, line.size() - 2);
                continue;
            }

            // Parse task parameters
            size_t equalPos = line.find('=');
            if (equalPos != std::string::npos) {
                std::string key = line.substr(0, equalPos);
                std::string value = line.substr(equalPos + 1);

                // Trim whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                task.params[key] = value;
            }
        }

        // Add the last task
        if (!currentTask.empty()) {
            tasks_[currentTask] = task;
        }

        config.close();
        return !tasks_.empty();
    }

    bool BatchConverter::processFiles() {
        // Find all SID files to process
        std::string inputPattern;

        // Find the first task with an input parameter
        for (const auto& [taskName, task] : tasks_) {
            if (task.params.count("input")) {
                inputPattern = task.params.at("input");
                break;
            }
        }

        if (inputPattern.empty()) {
            util::Logger::error("No input pattern found in any task");
            return false;
        }

        // Expand wildcards to get all input files
        std::vector<fs::path> inputFiles = expandWildcards(inputPattern);
        if (inputFiles.empty()) {
            util::Logger::error("No input files found matching pattern: " + inputPattern);
            return false;
        }

        util::Logger::info("Found " + std::to_string(inputFiles.size()) + " files to process");

        // Process each file through all tasks
        int fileIndex = 0;
        int totalFiles = inputFiles.size();
        bool allSuccessful = true;

        for (const auto& inputFile : inputFiles) {
            fileIndex++;
            std::string baseName = inputFile.stem().string();
            util::Logger::info("Processing file " + std::to_string(fileIndex) + "/" +
                std::to_string(totalFiles) + ": " + inputFile.string());

            // Clear file mapping for this file
            FileMapping mapping;
            mapping.originalFile = inputFile;
            fileMapping_[baseName] = mapping;

            // Process all tasks for this file
            for (const auto& [taskName, task] : tasks_) {
                util::Logger::info("  Task: " + taskName);

                // Reset CPU before each task
                cpu_->reset();

                if (!task.params.count("type")) {
                    util::Logger::error("Task " + taskName + " has no type specified");
                    allSuccessful = false;
                    continue;
                }

                std::string taskType = task.params.at("type");

                // Create a modified task with file-specific parameters
                BatchTask fileTask = task;

                // Modify input parameter if needed
                if (fileTask.params.count("input")) {
                    std::string input = fileTask.params.at("input");

                    // If input contains wildcard, replace with appropriate file
                    if (input.find('*') != std::string::npos || input.find('?') != std::string::npos) {
                        // Use the appropriate file based on task type and input path
                        if (taskType == "Relocate") {
                            // For relocate, use original file
                            fileTask.params["input"] = mapping.originalFile.string();
                        }
                        else if (taskType == "Convert") {
                            // For convert, check the input pattern
                            if (input.find("-rel") != std::string::npos) {
                                // If input contains "-rel", use relocated file
                                if (!mapping.relocatedFile.empty()) {
                                    fileTask.params["input"] = mapping.relocatedFile.string();
                                }
                                else {
                                    util::Logger::warning("Skipping convert for " + baseName +
                                        " - no relocated file available");
                                    continue;
                                }
                            }
                            else {
                                // Otherwise use original file
                                fileTask.params["input"] = mapping.originalFile.string();
                            }
                        }
                        else if (taskType == "Trace") {
                            // For trace, check if we're tracing original or relocated
                            if (input.find("-rel") != std::string::npos) {
                                // Tracing relocated file
                                if (mapping.relocatedFile.empty()) {
                                    util::Logger::warning("Skipping trace for " + baseName +
                                        " - no relocated file available yet");
                                    continue;
                                }
                                fileTask.params["input"] = mapping.relocatedFile.string();
                            }
                            else {
                                // Tracing original file
                                fileTask.params["input"] = mapping.originalFile.string();
                            }
                        }
                    }
                }

                // Modify output parameter if needed
                if (fileTask.params.count("output")) {
                    std::string output = fileTask.params.at("output");

                    // If output contains wildcard, replace with current file's base name
                    if (output.find('*') != std::string::npos) {
                        size_t wildcardPos = output.find('*');
                        std::string outputPattern = output;
                        outputPattern.replace(wildcardPos, 1, baseName);
                        fileTask.params["output"] = outputPattern;
                    }
                }

                // Process the specific task for this file
                bool success = false;

                if (taskType == "Convert") {
                    success = processConvertSingleFile(taskName, fileTask);
                }
                else if (taskType == "Relocate") {
                    success = processRelocateSingleFile(taskName, fileTask);

                    // Update file mapping if successful
                    if (success && fileTask.params.count("output")) {
                        mapping.relocatedFile = fs::path(fileTask.params.at("output"));
                        fileMapping_[baseName] = mapping;
                    }
                }
                else if (taskType == "Trace") {
                    success = processTraceSingleFile(taskName, fileTask);

                    // Update file mapping if successful
                    if (success && fileTask.params.count("output")) {
                        std::string input = fileTask.params.at("input");
                        if (input.find("-rel") != std::string::npos) {
                            // Tracing relocated file
                            mapping.relocatedTraceFile = fs::path(fileTask.params.at("output"));
                        }
                        else {
                            // Tracing original file
                            mapping.originalTraceFile = fs::path(fileTask.params.at("output"));
                        }
                        fileMapping_[baseName] = mapping;
                    }
                }
                else if (taskType == "Verify") {
                    // For verify, we need both trace files
                    if (mapping.originalTraceFile.empty() || mapping.relocatedTraceFile.empty()) {
                        util::Logger::warning("Skipping verification for " + baseName +
                            " - missing trace files");
                        continue;
                    }

                    // Create a task with the specific file paths
                    BatchTask verifyTask = fileTask;
                    verifyTask.params["original"] = mapping.originalTraceFile.string();
                    verifyTask.params["relocated"] = mapping.relocatedTraceFile.string();

                    // Generate a file-specific report name if needed
                    if (verifyTask.params.count("reportFile")) {
                        std::string reportFile = verifyTask.params.at("reportFile");
                        if (reportFile.find('*') != std::string::npos) {
                            size_t wildcardPos = reportFile.find('*');
                            reportFile.replace(wildcardPos, 1, baseName);
                            verifyTask.params["reportFile"] = reportFile;
                        }
                    }

                    success = processVerifySingleFile(taskName, verifyTask);
                }
                else {
                    util::Logger::error("Unknown task type: " + taskType);
                    allSuccessful = false;
                    continue;
                }

                if (!success) {
                    util::Logger::error("Failed to execute " + taskType + " task for " +
                        inputFile.string());
                    allSuccessful = false;
                }
            }
        }

        return allSuccessful;
    }

    std::vector<fs::path> BatchConverter::expandWildcards(const std::string& pattern) {
        std::vector<fs::path> result;

        // Extract directory and filename parts
        fs::path fullPath(pattern);
        fs::path dir = fullPath.parent_path();
        std::string filePattern = fullPath.filename().string();

        // If directory is empty, use current directory
        if (dir.empty()) dir = ".";

        try {
            // Check if directory exists
            if (!fs::exists(dir)) {
                util::Logger::warning("Directory does not exist: " + dir.string());
                return result;
            }

            // Convert file pattern to regex
            std::string regexPattern = "";
            for (char c : filePattern) {
                if (c == '*') regexPattern += ".*";
                else if (c == '?') regexPattern += ".";
                else if (c == '.') regexPattern += "\\.";  // Escape dots
                else regexPattern += c;
            }

            std::regex regex(regexPattern);

            // Traverse the directory and match files
            for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                if (!fs::is_directory(entry.path())) {
                    std::string filename = entry.path().filename().string();
                    if (std::regex_match(filename, regex)) {
                        result.push_back(entry.path());
                    }
                }
            }

            // Sort the results for consistency across tasks
            std::sort(result.begin(), result.end());

            if (result.empty()) {
                util::Logger::warning("No files matched pattern: " + pattern);
            }
        }
        catch (const std::exception& e) {
            util::Logger::error(std::string("Error expanding wildcards: ") + e.what());
        }

        return result;
    }

    fs::path BatchConverter::generateOutputPath(const fs::path& inputPath, const std::string& outputPattern) {
        // Extract the base directory from the output pattern
        fs::path outputPatternPath(outputPattern);
        fs::path outputDir = outputPatternPath.parent_path();

        // Extract the filename pattern from the output pattern
        fs::path filenamePattern = outputPatternPath.filename();

        // Handle wildcard replacement in filename
        std::string newFilename = filenamePattern.string();
        if (newFilename.find('*') != std::string::npos) {
            // Replace * with the input filename
            std::string inputFilename = inputPath.filename().string();
            // Strip extension if output pattern includes a different extension
            if (filenamePattern.has_extension() &&
                filenamePattern.extension() != inputPath.extension()) {
                inputFilename = inputPath.stem().string();
            }

            // Replace wildcard with input filename
            size_t pos = newFilename.find('*');
            newFilename.replace(pos, 1, inputFilename);
        }
        else {
            // If no wildcard, use the pattern as is
            newFilename = filenamePattern.string();
        }

        // Combine output dir and new filename
        return outputDir / newFilename;
    }

    void BatchConverter::ensureDirectoryExists(const fs::path& path) {
        fs::path dir = path.parent_path();
        if (!dir.empty() && !fs::exists(dir)) {
            try {
                fs::create_directories(dir);
                util::Logger::debug("Created directory: " + dir.string());
            }
            catch (const std::exception& e) {
                throw std::runtime_error("Failed to create directory: " + dir.string() +
                    " Error: " + e.what());
            }
        }
    }

    void BatchConverter::registerFileMapping(const fs::path& originalFile, const fs::path& processedFile, const std::string& taskType) {
        // Extract base name (without extension) for mapping key
        std::string baseName = originalFile.stem().string();

        // If this is the first time we're seeing this file, create a new mapping entry
        if (fileMapping_.find(baseName) == fileMapping_.end()) {
            FileMapping newMapping;
            fileMapping_[baseName] = newMapping;
        }

        // Update the mapping based on task type
        if (taskType == "Original") {
            fileMapping_[baseName].originalFile = originalFile;
        }
        else if (taskType == "Relocate") {
            fileMapping_[baseName].relocatedFile = processedFile;
        }
        else if (taskType == "TraceOriginal") {
            fileMapping_[baseName].originalTraceFile = processedFile;
        }
        else if (taskType == "TraceRelocated") {
            fileMapping_[baseName].relocatedTraceFile = processedFile;
        }

        util::Logger::debug("Registered file mapping: " + baseName + " " + taskType + " -> " + processedFile.string());
    }

    bool BatchConverter::processConvertTask(const std::string& taskName, const BatchTask& task) {
        if (!task.params.count("input") || !task.params.count("output")) {
            util::Logger::error("Convert task requires 'input' and 'output' parameters");
            return false;
        }

        std::string inputPattern = task.params.at("input");
        std::string outputPattern = task.params.at("output");

        // Check for wildcards in input pattern
        bool hasWildcards = (inputPattern.find('*') != std::string::npos || inputPattern.find('?') != std::string::npos);

        if (hasWildcards) {
            // Expand wildcards in input pattern
            std::vector<fs::path> inputFiles = expandWildcards(inputPattern);

            if (inputFiles.empty()) {
                util::Logger::error("No input files found matching pattern: " + inputPattern);
                return false;
            }

            // Process each input file
            bool allSuccessful = true;
            for (const auto& inputFile : inputFiles) {
                // Reset CPU for each file
                cpu_->reset();

                // Generate output path for this input file
                fs::path outputFile = generateOutputPath(inputFile, outputPattern);

                // Ensure output directory exists
                ensureDirectoryExists(outputFile);

                // Create task parameters for this file
                BatchTask fileTask = task;
                fileTask.params["input"] = inputFile.string();
                fileTask.params["output"] = outputFile.string();

                // Process this specific file
                if (!processConvertSingleFile(taskName, fileTask)) {
                    util::Logger::error("Failed to convert file: " + inputFile.string());
                    allSuccessful = false;
                }
                else {
                    // Register the file mapping for original file
                    registerFileMapping(inputFile, outputFile, "Original");
                }
            }

            return allSuccessful;
        }
        else {
            // Process single file
            bool success = processConvertSingleFile(taskName, task);
            if (success) {
                // Register the file mapping for original file
                registerFileMapping(fs::path(task.params.at("input")), fs::path(task.params.at("output")), "Original");
            }
            return success;
        }
    }

    bool BatchConverter::processConvertSingleFile(const std::string& taskName, const BatchTask& task) {
        std::string inputFile = task.params.at("input");
        std::string outputFile = task.params.at("output");

        // Get player options
        std::string playerType = task.params.count("player") ? task.params.at("player") : "";
        std::string playerAddrStr = task.params.count("playerAddr") ? task.params.at("playerAddr") : "";

        // Parse player address if specified
        u16 playerAddr = 0x0900; // Default
        if (!playerAddrStr.empty()) {
            auto addr = util::parseHex(playerAddrStr);
            if (addr) {
                playerAddr = *addr;
            }
            else {
                util::Logger::error("Invalid player address: " + playerAddrStr);
                return false;
            }
        }

        // Get format (optional, default based on extension)
        std::string format = task.params.count("format") ? task.params.at("format") : "";

        // Load the SID file
        if (!sid_->loadSID(inputFile)) {
            util::Logger::error("Failed to load SID file: " + inputFile);
            return false;
        }

        // Set up processing options
        CommandProcessor::ProcessingOptions options;
        options.inputFile = inputFile;
        options.outputFile = outputFile;

        // Set output format
        if (format == "PRG") {
            options.outputFormat = CommandProcessor::OutputFormat::PRG;
        }
        else if (format == "ASM") {
            options.outputFormat = CommandProcessor::OutputFormat::ASM;
        }
        else if (format == "SID") {
            options.outputFormat = CommandProcessor::OutputFormat::SID;
        }
        else {
            // Determine from output extension
            fs::path outputPath(outputFile);
            std::string ext = outputPath.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(),
                [](unsigned char c) { return std::tolower(c); });

            if (ext == ".prg") {
                options.outputFormat = CommandProcessor::OutputFormat::PRG;
            }
            else if (ext == ".asm") {
                options.outputFormat = CommandProcessor::OutputFormat::ASM;
            }
            else if (ext == ".sid") {
                options.outputFormat = CommandProcessor::OutputFormat::SID;
            }
            else {
                util::Logger::warning("Unknown output extension: " + ext + ", defaulting to PRG");
                options.outputFormat = CommandProcessor::OutputFormat::PRG;
            }
        }

        // Set player options
        options.includePlayer = !playerType.empty();
        options.playerName = playerType;
        options.playerAddress = playerAddr;

        // Create temp directory
        options.tempDir = fs::path("temp");
        try {
            fs::create_directories(options.tempDir);
        }
        catch (const std::exception& e) {
            util::Logger::error(std::string("Failed to create temp directory: ") + e.what());
            return false;
        }

        // Set tool paths from configuration
        options.kickAssPath = util::Configuration::getKickAssPath();
        options.exomizerPath = util::Configuration::getExomizerPath();

        // Process the file
        CommandProcessor processor;
        bool success = processor.processFile(options);

        return success;
    }

    bool BatchConverter::processRelocateTask(const std::string& taskName, const BatchTask& task) {
        if (!task.params.count("input") || !task.params.count("output") || !task.params.count("address")) {
            util::Logger::error("Relocate task requires 'input', 'output', and 'address' parameters");
            return false;
        }

        std::string inputPattern = task.params.at("input");
        std::string outputPattern = task.params.at("output");
        std::string addressStr = task.params.at("address");

        // Parse relocation address
        auto relocationAddr = util::parseHex(addressStr);
        if (!relocationAddr) {
            util::Logger::error("Invalid relocation address: " + addressStr);
            return false;
        }

        // Check for wildcards in input pattern
        bool hasWildcards = (inputPattern.find('*') != std::string::npos || inputPattern.find('?') != std::string::npos);

        if (hasWildcards) {
            // Expand wildcards in input pattern
            std::vector<fs::path> inputFiles = expandWildcards(inputPattern);

            if (inputFiles.empty()) {
                util::Logger::error("No input files found matching pattern: " + inputPattern);
                return false;
            }

            // Process each input file
            bool allSuccessful = true;
            for (const auto& inputFile : inputFiles) {
                // Reset CPU for each file
                cpu_->reset();

                // Generate output path for this input file
                fs::path outputFile = generateOutputPath(inputFile, outputPattern);

                // Ensure output directory exists
                ensureDirectoryExists(outputFile);

                // Create task parameters for this file
                BatchTask fileTask = task;
                fileTask.params["input"] = inputFile.string();
                fileTask.params["output"] = outputFile.string();

                // Process this specific file
                if (!processRelocateSingleFile(taskName, fileTask)) {
                    util::Logger::error("Failed to relocate file: " + inputFile.string());
                    allSuccessful = false;
                }
                else {
                    // Register the file mapping for relocated file
                    registerFileMapping(inputFile, outputFile, "Relocate");
                }
            }

            return allSuccessful;
        }
        else {
            // Process single file
            bool success = processRelocateSingleFile(taskName, task);
            if (success) {
                // Register the file mapping for relocated file
                registerFileMapping(fs::path(task.params.at("input")), fs::path(task.params.at("output")), "Relocate");
            }
            return success;
        }
    }

    bool BatchConverter::processRelocateSingleFile(const std::string& taskName, const BatchTask& task) {
        std::string inputFile = task.params.at("input");
        std::string outputFile = task.params.at("output");
        std::string addressStr = task.params.at("address");

        // Parse relocation address
        auto relocationAddr = util::parseHex(addressStr);
        if (!relocationAddr) {
            util::Logger::error("Invalid relocation address: " + addressStr);
            return false;
        }

        // Set up relocation parameters
        util::RelocationParams params;
        params.inputFile = inputFile;
        params.outputFile = outputFile;
        params.tempDir = fs::path("temp");
        params.relocationAddress = *relocationAddr;
        params.kickAssPath = util::Configuration::getKickAssPath(); // Get KickAss path from configuration

        // Ensure temp directory exists
        try {
            fs::create_directories(params.tempDir);
        }
        catch (const std::exception& e) {
            util::Logger::error(std::string("Failed to create temp directory: ") + e.what());
            return false;
        }

        // Relocate the SID file
        util::RelocationResult result = util::relocateSID(cpu_, sid_, params);

        if (result.success) {
            util::Logger::info("Successfully relocated " + inputFile + " to " + outputFile +
                " (Load: $" + util::wordToHex(result.newLoad) +
                ", Init: $" + util::wordToHex(result.newInit) +
                ", Play: $" + util::wordToHex(result.newPlay) + ")");
            return true;
        }
        else {
            util::Logger::error("Failed to relocate " + inputFile + ": " + result.message);
            return false;
        }
    }

    bool BatchConverter::processTraceTask(const std::string& taskName, const BatchTask& task) {
        if (!task.params.count("input") || !task.params.count("output")) {
            util::Logger::error("Trace task requires 'input' and 'output' parameters");
            return false;
        }

        std::string inputPattern = task.params.at("input");
        std::string outputPattern = task.params.at("output");

        // Parse frames (optional)
        int frames = DEFAULT_SID_EMULATION_FRAMES;
        if (task.params.count("frames")) {
            try {
                frames = std::stoi(task.params.at("frames"));
            }
            catch (const std::exception&) {
                util::Logger::warning("Invalid frames value, using default: " + task.params.at("frames"));
            }
        }

        // Parse format (optional)
        std::string format = task.params.count("format") ? task.params.at("format") : "binary";
        TraceFormat traceFormat = (format == "text") ? TraceFormat::Text : TraceFormat::Binary;

        // Check for wildcards in input pattern
        bool hasWildcards = (inputPattern.find('*') != std::string::npos || inputPattern.find('?') != std::string::npos);

        if (hasWildcards) {
            // Expand wildcards in input pattern
            std::vector<fs::path> inputFiles = expandWildcards(inputPattern);

            if (inputFiles.empty()) {
                util::Logger::error("No input files found matching pattern: " + inputPattern);
                return false;
            }

            // Process each input file
            bool allSuccessful = true;
            for (const auto& inputFile : inputFiles) {
                // Reset CPU for each file
                cpu_->reset();

                // Generate output path for this input file
                fs::path outputFile = generateOutputPath(inputFile, outputPattern);

                // Ensure output directory exists
                ensureDirectoryExists(outputFile);

                // Create task parameters for this file
                BatchTask fileTask = task;
                fileTask.params["input"] = inputFile.string();
                fileTask.params["output"] = outputFile.string();

                // Process this specific file
                if (!processTraceSingleFile(taskName, fileTask)) {
                    util::Logger::error("Failed to trace file: " + inputFile.string());
                    allSuccessful = false;
                }
                else {
                    // Determine if this is original or relocated trace
                    std::string traceType;
                    std::string baseName = inputFile.stem().string();

                    // Check if this is a trace of an original or relocated file
                    if (baseName.find("-rel") != std::string::npos) {
                        traceType = "TraceRelocated";
                    }
                    else {
                        traceType = "TraceOriginal";
                    }

                    // Register the file mapping
                    registerFileMapping(inputFile, outputFile, traceType);
                }
            }

            return allSuccessful;
        }
        else {
            // Process single file
            bool success = processTraceSingleFile(taskName, task);
            if (success) {
                // Determine if this is original or relocated trace
                fs::path inputFile = fs::path(task.params.at("input"));
                fs::path outputFile = fs::path(task.params.at("output"));
                std::string baseName = inputFile.stem().string();

                // Check if this is a trace of an original or relocated file
                std::string traceType;
                if (baseName.find("-rel") != std::string::npos) {
                    traceType = "TraceRelocated";
                }
                else {
                    traceType = "TraceOriginal";
                }

                // Register the file mapping
                registerFileMapping(inputFile, outputFile, traceType);
            }
            return success;
        }
    }

    bool BatchConverter::processTraceSingleFile(const std::string& taskName, const BatchTask& task) {
        std::string inputFile = task.params.at("input");
        std::string outputFile = task.params.at("output");

        // Parse frames (optional)
        int frames = DEFAULT_SID_EMULATION_FRAMES;
        if (task.params.count("frames")) {
            try {
                frames = std::stoi(task.params.at("frames"));
            }
            catch (const std::exception&) {
                util::Logger::warning("Invalid frames value, using default: " + task.params.at("frames"));
            }
        }

        // Parse format (optional)
        std::string format = task.params.count("format") ? task.params.at("format") : "binary";
        TraceFormat traceFormat = (format == "text") ? TraceFormat::Text : TraceFormat::Binary;

        // Load the SID file
        if (!sid_->loadSID(inputFile)) {
            util::Logger::error("Failed to load SID file: " + inputFile);
            return false;
        }

        // Create emulator and run trace
        SIDEmulator emulator(cpu_, sid_);
        SIDEmulator::EmulationOptions options;
        options.frames = frames;
        options.traceEnabled = true;
        options.traceLogPath = outputFile;
        options.traceFormat = traceFormat;

        // Run emulation to generate trace
        bool success = emulator.runEmulation(options);

        if (success) {
            util::Logger::info("Successfully created trace for " + inputFile + " to " + outputFile);
            return true;
        }
        else {
            util::Logger::error("Failed to create trace for " + inputFile);
            return false;
        }
    }

    bool BatchConverter::processVerifySingleFile(const std::string& taskName, const BatchTask& task) {
        if (!task.params.count("original") || !task.params.count("relocated")) {
            util::Logger::error("Verify task requires 'original' and 'relocated' parameters");
            return false;
        }

        std::string originalFile = task.params.at("original");
        std::string relocatedFile = task.params.at("relocated");
        std::string reportFile = task.params.count("reportFile") ? task.params.at("reportFile") : "verification.log";

        // Verify the trace logs
        bool match = TraceLogger::compareTraceLogs(originalFile, relocatedFile, reportFile);

        if (match) {
            util::Logger::info("Verification successful - trace logs match");
        }
        else {
            util::Logger::warning("Verification failed - trace logs differ");
        }

        return true; // Return true even if logs don't match, as the task itself completed
    }

} // namespace sidblaster
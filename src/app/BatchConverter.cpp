// ==================================
//             SIDBlaster
//
//  Raistlin / Genesis Project (G*P)
// ==================================
#include "BatchConverter.h"
#include "../SIDBlasterUtils.h"
#include "../cpu6510.h"
#include "../SIDLoader.h"
#include "../RelocationUtils.h"
#include "../SIDEmulator.h"
#include "MusicBuilder.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace sidblaster {

    BatchConverter::BatchConverter(
        const std::string& configFile,
        CPU6510* cpu,
        SIDLoader* sid)
        : configFile_(configFile), cpu_(cpu), sid_(sid), tempDir_("temp") {

        // Create temp directory if it doesn't exist
        try {
            fs::create_directories(tempDir_);
        }
        catch (const std::exception& e) {
            util::Logger::error(std::string("Failed to create temp directory: ") + e.what());
        }
    }

    bool BatchConverter::execute() {
        // Parse the configuration file
        if (!parseConfig()) {
            util::Logger::error("Failed to parse batch configuration file");
            return false;
        }

        // Execute each task in order
        bool success = true;
        for (size_t i = 0; i < tasks_.size(); ++i) {
            const auto& task = tasks_[i];
            util::Logger::info("Executing batch task " + std::to_string(i + 1) +
                " of " + std::to_string(tasks_.size()) +
                ": " + task.input + " -> " + task.output);

            if (!executeTask(task)) {
                util::Logger::error("Failed to execute task: " + task.input + " -> " + task.output);
                success = false;
                //; -- continue with remaining tasks, even if this one failed.. otherwise, do a break here
                //; TODO: we should make it an option of the task system that we stop at a failure?
            }
        }

        return success;
    }

    bool BatchConverter::parseConfig() {
         std::ifstream file(configFile_);
        if (!file) {
            util::Logger::error("Failed to open batch config file: " + configFile_);
            return false;
        }

        std::string line;
        BatchTask* currentTask = nullptr;

        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }

            // Trim whitespace
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);

            if (line.empty()) {
                continue;
            }

            // Check for section header
            if (line[0] == '[' && line[line.length() - 1] == ']') {
                // Start a new task
                tasks_.push_back(BatchTask());
                currentTask = &tasks_.back();
                // Section name is used as a comment, not stored
                continue;
            }

            // Parse key-value pair
            if (!currentTask) {
                // Must have a task section before parameters
                util::Logger::warning("Configuration line outside of task section: " + line);
                continue;
            }

            auto pos = line.find('=');
            if (pos == std::string::npos) {
                util::Logger::warning("Invalid configuration line: " + line);
                continue;
            }

            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            // Handle special fields
            if (key == "type") {
                if (value == "Convert") {
                    currentTask->type = TaskType::Convert;
                }
                else if (value == "Relocate") {
                    currentTask->type = TaskType::Relocate;
                }
                else if (value == "Verify") {
                    currentTask->type = TaskType::Verify;
                }
                else if (value == "Trace") {
                    currentTask->type = TaskType::Trace;
                }
                else {
                    util::Logger::warning("Unknown task type: " + value);
                }
            }
            else if (key == "input") {
                currentTask->input = value;
            }
            else if (key == "output") {
                currentTask->output = value;
            }
            else {
                // Store all other parameters in the map
                currentTask->parameters[key] = value;
            }
        }

        util::Logger::info("Parsed " + std::to_string(tasks_.size()) + " tasks from batch config");
        return !tasks_.empty();
    }

    bool BatchConverter::executeTask(const BatchTask& task) {
        cpu_->reset();
        switch (task.type) {
        case TaskType::Convert:
            return executeConversion(task);
        case TaskType::Relocate:
            return executeRelocation(task);
        case TaskType::Trace:
            return executeTracing(task);
        case TaskType::Verify:
            return executeVerification(task);
        default:
            util::Logger::error("Unknown task type");
            return false;
        }
    }

    bool BatchConverter::executeConversion(const BatchTask& task) {
        // Get parameters
        const std::string format = getParameterValue(task, "format", "PRG");
        const std::string playerName = getParameterValue(task, "player", "");
        const bool includePlayer = !playerName.empty();
        const u16 playerAddr = getParameterAddress(task, "playerAddr", 0x0900);
        const bool compress = getParameterBool(task, "compress", true);

        // Setup MusicBuilder with options
        MusicBuilder::BuildOptions options;
        options.includePlayer = includePlayer;
        options.playerName = playerName.empty() ? "" :
            (playerName == "player") ? "default" : playerName;
        options.playerAddress = playerAddr;
        options.compress = compress;
        options.tempDir = tempDir_;

        // Get SID-specific options if available
        options.sidLoadAddr = getParameterAddress(task, "loadAddr", 0x1000);
        options.sidInitAddr = getParameterAddress(task, "initAddr", 0x1000);
        options.sidPlayAddr = getParameterAddress(task, "playAddr", 0x1003);
        options.playCallsPerFrame = getParameterInt(task, "callsPerFrame", 1);

        // Create builder
        MusicBuilder builder(cpu_, sid_);

        // Extract basename from input file
        fs::path inputPath(task.input);
        std::string basename = inputPath.stem().string();

        // Execute build
        return builder.buildMusic(basename, task.input, task.output, options);
    }

    bool BatchConverter::executeRelocation(const BatchTask& task) {
        // Get relocation address
        const u16 relocAddr = getParameterAddress(task, "address", 0x2000);

        // Setup parameters for relocation
        util::RelocationParams params;
        params.inputFile = fs::path(task.input);
        params.outputFile = fs::path(task.output);
        params.tempDir = tempDir_;
        params.relocationAddress = relocAddr;
        params.kickAssPath = getParameterValue(task, "kickAssPath", "java -jar KickAss.jar");
        params.verbose = false; // Could be configurable

        util::Logger::info("Relocating " + task.input + " to $" +
            util::wordToHex(relocAddr) + " -> " + task.output);

        // Perform the relocation
        util::RelocationResult result = util::relocateSID(cpu_, sid_, params);

        // Log result
        if (result.success) {
            util::Logger::info(result.message);
        }
        else {
            util::Logger::error(result.message);
        }

        return result.success;
    }

    bool BatchConverter::executeTracing(const BatchTask& task) {
        // Get trace format
        const std::string formatStr = getParameterValue(task, "format", "binary");
        const TraceFormat format = (formatStr == "text") ?
            TraceFormat::Text : TraceFormat::Binary;

        // Load the input file
        bool loaded = false;
        const fs::path inputPath(task.input);
        const std::string ext = inputPath.extension().string();

        if (ext == ".sid") {
            loaded = sid_->loadSID(task.input);
        }
        else if (ext == ".prg") {
            // For PRG, we need init and play addresses
            const u16 init = getParameterAddress(task, "initAddr", 0x1000);
            const u16 play = getParameterAddress(task, "playAddr", 0x1003);
            loaded = sid_->loadPRG(task.input, init, play);
        }
        else {
            util::Logger::error("Unsupported input file type for tracing: " + ext);
            return false;
        }

        if (!loaded) {
            util::Logger::error("Failed to load file for tracing: " + task.input);
            return false;
        }

        // Set up emulation options
        SIDEmulator emulator(cpu_, sid_);
        SIDEmulator::EmulationOptions options;
        options.frames = getParameterInt(task, "frames", DEFAULT_SID_EMULATION_FRAMES);
        options.backupAndRestore = true;
        options.traceEnabled = true;
        options.traceFormat = format;
        options.traceLogPath = task.output;
        options.callsPerFrame = getParameterInt(task, "callsPerFrame", 1);

        // Run emulation
        bool success = emulator.runEmulation(options);

        util::Logger::info("Trace completed: " + task.output);
        return success;
    }

    bool BatchConverter::executeVerification(const BatchTask& task) {
        // Get parameters
        const std::string originalLog = getParameterValue(task, "original");
        const std::string relocatedLog = getParameterValue(task, "relocated");
        const std::string reportFile = getParameterValue(task, "reportFile", "verification.log");

        if (originalLog.empty() || relocatedLog.empty()) {
            util::Logger::error("Missing trace log files for verification");
            return false;
        }

        // Compare the trace logs
        std::string report;
        bool identical = TraceLogger::compareTraceLogs(originalLog, relocatedLog, reportFile);

        if (identical) {
            util::Logger::info("Verification PASSED: Trace logs are identical");
        }
        else {
            util::Logger::warning("Verification FAILED: Differences found. See " + reportFile);
            std::cout << "\nVERIFICATION FAILED: Differences found between trace logs. Check " << reportFile << " for detailed comparison.\n\n";
        }

        return true;
    }

    std::string BatchConverter::getParameterValue(
        const BatchTask& task,
        const std::string& name,
        const std::string& defaultValue) const {

        auto it = task.parameters.find(name);
        return (it != task.parameters.end()) ? it->second : defaultValue;
    }

    bool BatchConverter::getParameterBool(
        const BatchTask& task,
        const std::string& name,
        bool defaultValue) const {

        auto it = task.parameters.find(name);
        if (it == task.parameters.end()) {
            return defaultValue;
        }

        const std::string& value = it->second;
        return (value == "true" || value == "yes" || value == "1" ||
            value == "on" || value == "enable" || value == "enabled");
    }

    int BatchConverter::getParameterInt(
        const BatchTask& task,
        const std::string& name,
        int defaultValue) const {

        auto it = task.parameters.find(name);
        if (it == task.parameters.end()) {
            return defaultValue;
        }

        try {
            return std::stoi(it->second);
        }
        catch (...) {
            return defaultValue;
        }
    }

    u16 BatchConverter::getParameterAddress(
        const BatchTask& task,
        const std::string& name,
        u16 defaultValue) const {

        auto it = task.parameters.find(name);
        if (it == task.parameters.end()) {
            return defaultValue;
        }

        std::string value = it->second;

        // Handle hex format
        if (value.size() > 2 && value.substr(0, 2) == "0x") {
            try {
                return static_cast<u16>(std::stoul(value.substr(2), nullptr, 16));
            }
            catch (...) {
                return defaultValue;
            }
        }
        else if (!value.empty() && value[0] == '$') {
            try {
                return static_cast<u16>(std::stoul(value.substr(1), nullptr, 16));
            }
            catch (...) {
                return defaultValue;
            }
        }
        else {
            // Try as decimal
            try {
                return static_cast<u16>(std::stoul(value));
            }
            catch (...) {
                return defaultValue;
            }
        }
    }

} // namespace sidblaster
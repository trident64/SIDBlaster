// ==================================
//             SIDBlaster
//
//  Raistlin / Genesis Project (G*P)
// ==================================
#include "BatchProcessor.h"
#include "SIDBlasterUtils.h"
#include "SIDEmulator.h"
#include "RelocationUtils.h"
#include "TraceLogger.h"
#include "cpu6510.h"
#include "SIDLoader.h"

namespace sidblaster {

    /**
     * @class ReportGenerator
     * @brief Helper class for generating batch processing reports
     */
    class ReportGenerator {
    public:
        /**
         * @struct ReportEntry
         * @brief Entry for a single file in the report
         */
        struct ReportEntry {
            fs::path inputFile;
            fs::path outputFile;
            u16 relocationAddress;
            bool success;
            bool verified;
            bool outputsMatch;
            std::string message;
        };

        /**
         * @brief Add an entry to the report
         * @param entry Report entry to add
         */
        void addEntry(const ReportEntry& entry) {
            entries_.push_back(entry);
        }

        /**
         * @brief Generate the summary report
         * @param reportPath Path for the report file
         */
        void generateReport(const fs::path& reportPath) {
            std::ofstream report(reportPath);
            if (!report) {
                throw std::runtime_error("Failed to create report file: " + reportPath.string());
            }

            report << "SIDBlaster Relocation Batch Report\n";
            report << "================================\n\n";

            size_t totalFiles = entries_.size();
            int successfulRelocations = 0;
            int verifiedMatches = 0;

            for (const auto& entry : entries_) {
                if (entry.success) successfulRelocations++;
                if (entry.verified && entry.outputsMatch) verifiedMatches++;

                std::string status = entry.success ?
                    (entry.verified ?
                        (entry.outputsMatch ? "PASS" : "FAIL")
                        : "RELOCATED")
                    : "ERROR";

                report << status << ": " << entry.inputFile.string()
                    << " - relocated to $" << util::wordToHex(entry.relocationAddress);

                if (entry.verified) {
                    report << " (SID output " << (entry.outputsMatch ? "matches" : "differs") << ")";
                }

                report << std::endl;
            }

            report << "\n\nSummary:\n";
            report << "--------\n";
            report << successfulRelocations << " out of " << totalFiles
                << " SIDs were successfully relocated\n";

            if (verifiedMatches > 0) {
                report << verifiedMatches << " were verified to produce identical output\n";
            }

            report.close();

            // Also log to console
            util::Logger::info("Batch processing complete. " +
                std::to_string(successfulRelocations) + " out of " +
                std::to_string(totalFiles) + " SIDs were successfully relocated.");

            if (verifiedMatches > 0) {
                util::Logger::info(std::to_string(verifiedMatches) +
                    " were verified to produce identical output.");
            }
        }

    private:
        std::vector<ReportEntry> entries_;
    };

    /**
     * @struct RelocationVerificationResult
     * @brief Result of relocating and verifying a SID file
     */
    struct RelocationVerificationResult {
        bool success;                // Whether relocation was successful
        bool verified;               // Whether verification was attempted
        bool outputsMatch;           // Whether original and relocated outputs match
        std::string originalTrace;   // Path to original trace file
        std::string relocatedTrace;  // Path to relocated trace file
        std::string diffReport;      // Path to difference report file
        std::string message;         // Detailed message
    };

    /**
     * @brief Relocate and verify a SID file
     * @param cpu CPU instance
     * @param sid SID loader instance
     * @param inputFile Input SID file
     * @param outputFile Output relocated file
     * @param relocationAddress Target relocation address
     * @param tempDir Directory for temporary files
     * @param verify Whether to verify the relocation
     * @return Relocation and verification result
     */
    RelocationVerificationResult relocateAndVerifySID(
        CPU6510* cpu,
        SIDLoader* sid,
        const fs::path& inputFile,
        const fs::path& outputFile,
        u16 relocationAddress,
        const fs::path& tempDir,
        bool verify) {

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
            // Step 1: Create trace of original SID if verification is needed
            if (verify) {
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

            // Step 3: Verify the relocated SID if requested
            if (verify) {
                if (!sid->loadSID(outputFile.string())) {
                    result.message = "Relocation succeeded but failed to load relocated SID file";
                    return result;
                }

                // Create an emulator for the relocated SID
                SIDEmulator relocatedEmulator(cpu, sid);
                SIDEmulator::EmulationOptions options;
                options.frames = DEFAULT_SID_EMULATION_FRAMES;
                options.traceEnabled = true;
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
            }
            else {
                result.message = "Relocation successful (verification skipped)";
            }

            return result;
        }
        catch (const std::exception& e) {
            result.message = std::string("Exception during relocation/verification: ") + e.what();
            return result;
        }
    }

    // BatchProcessor implementation
    BatchProcessor::BatchProcessor() {
    }

    BatchProcessor::BatchResult BatchProcessor::processBatch(const BatchOptions& options) {
        BatchResult result;
        result.totalFiles = 0;
        result.successfulRelocations = 0;
        result.verifiedMatches = 0;

        // Create report generator
        ReportGenerator reporter;

        // Expand input pattern to get all matching files
        std::vector<fs::path> inputFiles = expandWildcards(options.inputPattern);

        // Create CPU and SID loader
        std::unique_ptr<CPU6510> cpu = std::make_unique<CPU6510>();
        std::unique_ptr<SIDLoader> sid = std::make_unique<SIDLoader>();
        sid->setCPU(cpu.get());

        // Process each file
        for (const auto& inputFile : inputFiles) {
            cpu->reset();

            result.totalFiles++;

            // Generate output path
            fs::path outputFile = generateOutputPath(inputFile, options.outputPattern, options.preserveSubfolders);

            // Ensure output directory exists
            ensureDirectoryExists(outputFile);

            // Relocate and verify
            auto relocResult = relocateAndVerifySID(
                cpu.get(), sid.get(), inputFile, outputFile,
                options.relocationAddress, options.tempDir, options.verify);

            // Add to report
            ReportGenerator::ReportEntry entry;
            entry.inputFile = inputFile;
            entry.outputFile = outputFile;
            entry.relocationAddress = options.relocationAddress;
            entry.success = relocResult.success;
            entry.verified = relocResult.verified;
            entry.outputsMatch = relocResult.outputsMatch;
            entry.message = relocResult.message;

            reporter.addEntry(entry);

            // Update statistics
            if (relocResult.success) result.successfulRelocations++;
            if (relocResult.verified && relocResult.outputsMatch) result.verifiedMatches++;

            // Log progress
            util::Logger::info("Processed " + std::to_string(result.totalFiles) + " of " +
                std::to_string(inputFiles.size()) + ": " + inputFile.string());
        }

        // Generate summary report
        reporter.generateReport(options.reportPath);

        return result;
    }

    std::vector<fs::path> BatchProcessor::expandWildcards(const fs::path& pattern) {
        std::vector<fs::path> result;

        // Extract directory and filename parts
        fs::path dir = pattern.parent_path();
        std::string filePattern = pattern.filename().string();

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
        }
        catch (const std::exception& e) {
            util::Logger::error(std::string("Error expanding wildcards: ") + e.what());
        }

        return result;
    }

    fs::path BatchProcessor::generateOutputPath(
        const fs::path& inputPath,
        const fs::path& outputPattern,
        bool preserveSubfolders) {

        // Extract the base directory from the output pattern
        fs::path outputDir = outputPattern.parent_path();

        // Extract the filename pattern from the output pattern
        fs::path filenamePattern = outputPattern.filename();

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
            // If no wildcard, use the pattern as is (potentially replacing extension)
            if (filenamePattern.has_extension() && !inputPath.extension().empty()) {
                newFilename = inputPath.stem().string() + filenamePattern.extension().string();
            }
            else {
                newFilename = inputPath.filename().string();
            }
        }

        // Handle subfolder preservation if requested
        if (preserveSubfolders) {
            // Extract input directory
            fs::path inputDir = outputPattern.parent_path();

            // Extract relative path to input file
            fs::path relPath;
            try {
                fs::path inputParent = inputPath.parent_path();
                // Find common part of paths (this is a simplified approach)
                std::string inputStr = inputParent.string();
                if (inputStr.find(inputDir.string()) == 0) {
                    relPath = inputStr.substr(inputDir.string().length());
                }
                else {
                    // Just use the parent path name as a subfolder
                    relPath = inputParent.filename();
                }
            }
            catch (const std::exception&) {
                // Fall back to just using the parent folder name
                relPath = inputPath.parent_path().filename();
            }

            // Combine output dir, relative path, and new filename
            if (!relPath.empty()) {
                return outputDir / relPath / newFilename;
            }
        }

        // Just combine output dir and new filename
        return outputDir / newFilename;
    }

    void BatchProcessor::ensureDirectoryExists(const fs::path& path) {
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

} // namespace sidblaster
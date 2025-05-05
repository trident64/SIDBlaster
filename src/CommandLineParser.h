// ==================================
//             SIDBlaster
//
//  Raistlin / Genesis Project (G*P)
// ==================================
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <map>

/**
 * @file CommandLineParser.h
 * @brief Parser for command line arguments
 *
 * Provides facilities for parsing and accessing command line arguments
 * with support for flags, options, and commands with multiple parameters.
 */

namespace sidblaster {

    /**
     * @struct CommandOption
     * @brief Represents a command line option with multiple parameters
     */
    struct CommandOption {
        std::string name;                  // Option name
        std::vector<std::string> parameters;  // Option parameters
    };

    /**
     * @class CommandLineParser
     * @brief Enhanced parser for command line arguments with support for commands with multiple parameters
     *
     * Provides a flexible interface for parsing and accessing command line arguments,
     * with support for flags, options with values, and commands with multiple parameters.
     * Also includes facilities for generating usage information.
     */
    class CommandLineParser {
    public:
        /**
         * @brief Constructor that parses command line arguments
         * @param argc Number of arguments
         * @param argv Array of argument strings
         *
         * Parses command line arguments into flags, options, commands, and input/output files.
         */
        CommandLineParser(int argc, char** argv);

        /**
         * @brief Get the output file
         * @return The output file path
         */
        const std::string& getOutputFile() const;

        /**
         * @brief Check if a flag is present
         * @param flag Flag name (without leading dashes)
         * @return True if flag is present
         *
         * Checks if a simple flag (without value) is present in the arguments.
         */
        bool hasFlag(const std::string& flag) const;

        /**
         * @brief Get the value of an option (first parameter only)
         * @param option Option name (without leading dashes)
         * @param defaultValue Value to return if option not present
         * @return First parameter value or default
         *
         * Retrieves the value of an option, or the default if not present.
         */
        std::string getOption(const std::string& option, const std::string& defaultValue = {}) const;

        /**
         * @brief Get all parameters for a command
         * @param command Command name (without leading dashes)
         * @return Vector of parameter values or empty vector if command not present
         *
         * Retrieves all parameters for a multi-parameter command.
         */
        std::vector<std::string> getCommandParameters(const std::string& command) const;

        /**
         * @brief Get the integer value of an option (first parameter only)
         * @param option Option name (without leading dashes)
         * @param defaultValue Value to return if option not present or invalid
         * @return Option value or default
         *
         * Parses the option value as an integer, returning the default if not present or invalid.
         */
        int getIntOption(const std::string& option, int defaultValue = 0) const;

        /**
         * @brief Get the boolean value of an option (first parameter only)
         * @param option Option name (without leading dashes)
         * @param defaultValue Value to return if option not present
         * @return Option value or default
         *
         * Parses the option value as a boolean, supporting various formats (yes/no, true/false, etc.).
         */
        bool getBoolOption(const std::string& option, bool defaultValue = false) const;

        /**
         * @brief Get a path option and ensure it exists
         * @param option Option name (without leading dashes)
         * @param defaultValue Value to return if option not present
         * @return Path value or nullopt if path doesn't exist
         *
         * Retrieves a path option and verifies that it exists.
         */
        std::optional<std::filesystem::path> getExistingPath(
            const std::string& option,
            const std::filesystem::path& defaultValue = {}) const;

        /**
         * @brief Get the input file (SID, PRG, or BIN)
         * @return Input file path or empty string if not specified
         */
        const std::string& getInputFile() const;

        /**
         * @brief Get the program name
         * @return Program name (from argv[0])
         */
        const std::string& getProgramName() const;

        /**
         * @brief Print usage information
         * @param message Optional message to display before usage
         *
         * Generates and displays usage information, including flags, options,
         * and example usage.
         */
        void printUsage(const std::string& message = {}) const;

        /**
         * @brief Add a flag definition for usage help
         * @param flag Flag name (without leading dashes)
         * @param description Flag description
         * @param category Category for grouping in help text
         * @return Reference to this parser (for chaining)
         *
         * Defines a flag for usage information.
         */
        CommandLineParser& addFlagDefinition(
            const std::string& flag,
            const std::string& description,
            const std::string& category = "General");

        /**
         * @brief Add an option definition for usage help
         * @param option Option name (without leading dashes)
         * @param argName Name of the option's argument
         * @param description Option description
         * @param category Category for grouping in help text
         * @param defaultValue Default value (empty for no default)
         * @return Reference to this parser (for chaining)
         *
         * Defines an option for usage information.
         */
        CommandLineParser& addOptionDefinition(
            const std::string& option,
            const std::string& argName,
            const std::string& description,
            const std::string& category = "General",
            const std::string& defaultValue = {});

        /**
         * @brief Add example usage for help text
         * @param example Example command line
         * @param description Description of what the example does
         * @return Reference to this parser (for chaining)
         *
         * Adds an example usage to the help text.
         */
        CommandLineParser& addExample(
            const std::string& example,
            const std::string& description);

    private:
        std::string programName_;                                 // Program name from argv[0]
        std::string inputFile_;                                   // Input file path
        std::unordered_set<std::string> flags_;                   // Set of flags
        std::unordered_map<std::string, CommandOption> commands_; // Map of commands

        // For usage help
        struct OptionDefinition {
            std::string argName;       // Name of the option's argument
            std::string description;   // Description of the option
            std::string category;      // Category for grouping
            std::string defaultValue;  // Default value
        };

        struct FlagDefinition {
            std::string description;   // Description of the flag
            std::string category;      // Category for grouping
        };

        struct ExampleUsage {
            std::string example;       // Example command line
            std::string description;   // Description of the example
        };

        std::map<std::string, OptionDefinition> optionDefs_;  // Option definitions
        std::map<std::string, FlagDefinition> flagDefs_;      // Flag definitions
        std::vector<ExampleUsage> examples_;                  // Example usages
        std::string outputFile_;                              // Output file path
    };

} // namespace sidblaster
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <map>

namespace sidblaster {

    /**
     * @struct CommandOption
     * @brief Represents a command line option with multiple parameters
     */
    struct CommandOption {
        std::string name;
        std::vector<std::string> parameters;
    };

    /**
     * @class CommandLineParser
     * @brief Enhanced parser for command line arguments with support for commands with multiple parameters
     */
    class CommandLineParser {
    public:
        /**
         * @brief Constructor that parses command line arguments
         * @param argc Number of arguments
         * @param argv Array of argument strings
         */
        CommandLineParser(int argc, char** argv);

        /**
         * Get the output file
         *
         * @return The output file path
         */
        const std::string& getOutputFile() const;

        /**
         * @brief Check if a flag is present
         * @param flag Flag name (without leading dashes)
         * @return True if flag is present
         */
        bool hasFlag(const std::string& flag) const;

        /**
         * @brief Get the value of an option (first parameter only)
         * @param option Option name (without leading dashes)
         * @param defaultValue Value to return if option not present
         * @return First parameter value or default
         */
        std::string getOption(const std::string& option, const std::string& defaultValue = {}) const;

        /**
         * @brief Get all parameters for a command
         * @param command Command name (without leading dashes)
         * @return Vector of parameter values or empty vector if command not present
         */
        std::vector<std::string> getCommandParameters(const std::string& command) const;

        /**
         * @brief Get the integer value of an option (first parameter only)
         * @param option Option name (without leading dashes)
         * @param defaultValue Value to return if option not present or invalid
         * @return Option value or default
         */
        int getIntOption(const std::string& option, int defaultValue = 0) const;

        /**
         * @brief Get the boolean value of an option (first parameter only)
         * @param option Option name (without leading dashes)
         * @param defaultValue Value to return if option not present
         * @return Option value or default
         */
        bool getBoolOption(const std::string& option, bool defaultValue = false) const;

        /**
         * @brief Get a path option and ensure it exists
         * @param option Option name (without leading dashes)
         * @param defaultValue Value to return if option not present
         * @return Path value or nullopt if path doesn't exist
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
         */
        void printUsage(const std::string& message = {}) const;

        /**
         * @brief Add a flag definition for usage help
         * @param flag Flag name (without leading dashes)
         * @param description Flag description
         * @param category Category for grouping in help text
         * @return Reference to this parser (for chaining)
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
         */
        CommandLineParser& addExample(
            const std::string& example,
            const std::string& description);

    private:
        std::string programName_;
        std::string inputFile_;
        std::unordered_set<std::string> flags_;
        std::unordered_map<std::string, CommandOption> commands_;

        // For usage help
        struct OptionDefinition {
            std::string argName;
            std::string description;
            std::string category;
            std::string defaultValue;
        };

        struct FlagDefinition {
            std::string description;
            std::string category;
        };

        struct ExampleUsage {
            std::string example;
            std::string description;
        };

        std::map<std::string, OptionDefinition> optionDefs_;
        std::map<std::string, FlagDefinition> flagDefs_;
        std::vector<ExampleUsage> examples_;
        std::string outputFile_;
    };

} // namespace sidblaster
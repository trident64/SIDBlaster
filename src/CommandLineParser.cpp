#include "CommandLineParser.h"
#include "SIDBlasterUtils.h"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace sidblaster {

    CommandLineParser::CommandLineParser(int argc, char** argv) {
        if (argc > 0) {
            programName_ = std::filesystem::path(argv[0]).filename().string();
        }

        // Process command line options first
        std::vector<std::string> nonOptionArgs;
        int i = 1;

        while (i < argc) {
            std::string arg = argv[i++];

            // Skip empty arguments
            if (arg.empty()) {
                continue;
            }

            // Check if it's a command (starts with '-')
            if (arg[0] == '-') {
                // Process option with potential parameter
                std::string option = arg.substr(1);  // Remove leading dash
                size_t equalPos = option.find('=');

                if (equalPos != std::string::npos) {
                    // Option with value using equals sign: -option=value
                    std::string name = option.substr(0, equalPos);
                    std::string value = option.substr(equalPos + 1);

                    // Create command option
                    CommandOption command;
                    command.name = name;
                    command.parameters.push_back(value);
                    commands_[name] = command;
                }
                else {
                    // Flag without value
                    flags_.insert(option);
                }
            }
            else {
                // This is a non-option argument (input or output file)
                nonOptionArgs.push_back(arg);
            }
        }

        // Handle input and output files
        if (nonOptionArgs.size() >= 2) {
            // Last two arguments are input and output files
            inputFile_ = nonOptionArgs[nonOptionArgs.size() - 2];
            outputFile_ = nonOptionArgs[nonOptionArgs.size() - 1];
        }
        else if (nonOptionArgs.size() == 1) {
            // Only input file provided
            inputFile_ = nonOptionArgs[0];
        }
    }

    const std::string& CommandLineParser::getOutputFile() const {
        return outputFile_;
    }

    bool CommandLineParser::hasFlag(const std::string& flag) const {
        return flags_.find(flag) != flags_.end();
    }

    std::string CommandLineParser::getOption(const std::string& option, const std::string& defaultValue) const {
        auto it = commands_.find(option);
        if (it != commands_.end() && !it->second.parameters.empty()) {
            return it->second.parameters[0];
        }
        return defaultValue;
    }

    std::vector<std::string> CommandLineParser::getCommandParameters(const std::string& command) const {
        auto it = commands_.find(command);
        if (it != commands_.end()) {
            return it->second.parameters;
        }
        return {};
    }

    int CommandLineParser::getIntOption(const std::string& option, int defaultValue) const {
        auto optionValue = getOption(option);
        if (optionValue.empty()) {
            return defaultValue;
        }

        try {
            return std::stoi(optionValue);
        }
        catch (const std::exception&) {
            return defaultValue;
        }
    }

    bool CommandLineParser::getBoolOption(const std::string& option, bool defaultValue) const {
        auto optionValue = getOption(option);
        if (optionValue.empty()) {
            return defaultValue;
        }

        if (optionValue == "true" || optionValue == "yes" || optionValue == "1" ||
            optionValue == "on" || optionValue == "enable" || optionValue == "enabled") {
            return true;
        }
        else if (optionValue == "false" || optionValue == "no" || optionValue == "0" ||
            optionValue == "off" || optionValue == "disable" || optionValue == "disabled") {
            return false;
        }

        return defaultValue;
    }

    std::optional<std::filesystem::path> CommandLineParser::getExistingPath(
        const std::string& option,
        const std::filesystem::path& defaultValue) const {

        auto optionValue = getOption(option);
        if (optionValue.empty()) {
            if (defaultValue.empty()) {
                return std::nullopt;
            }

            if (!std::filesystem::exists(defaultValue)) {
                return std::nullopt;
            }

            return defaultValue;
        }

        std::filesystem::path path = optionValue;
        if (!std::filesystem::exists(path)) {
            return std::nullopt;
        }

        return path;
    }

    const std::string& CommandLineParser::getInputFile() const {
        return inputFile_;
    }

    const std::string& CommandLineParser::getProgramName() const {
        return programName_;
    }

    void CommandLineParser::printUsage(const std::string& message) const {
        if (!message.empty()) {
            std::cout << message << std::endl << std::endl;
        }

        std::cout << "Developed by: Robert Troughton (Raistlin of Genesis Project)" << std::endl;
        std::cout << std::endl;

        std::cout << "Usage: " << programName_ << " [options] inputfile outputfile" << std::endl;
        std::cout << std::endl;
        std::cout << "  inputfile             Path to input file (.sid, .prg, or .bin)" << std::endl;
        std::cout << "  outputfile            Path to output file (.sid, .prg, or .asm)" << std::endl;
        std::cout << std::endl;

        // Group flags and options by category
        std::map<std::string, std::vector<std::string>> flagsByCategory;
        std::map<std::string, std::vector<std::string>> optionsByCategory;

        // Collect flags by category
        for (const auto& [flag, def] : flagDefs_) {
            flagsByCategory[def.category].push_back(flag);
        }

        // Collect options by category
        for (const auto& [option, def] : optionDefs_) {
            optionsByCategory[def.category].push_back(option);
        }

        // Display options by category
        for (const auto& [category, options] : optionsByCategory) {
            std::cout << category << " Options:" << std::endl;

            for (const auto& option : options) {
                const auto& def = optionDefs_.find(option)->second;

                std::cout << "  -" << option;

                // Format option name and argument name with equals sign
                std::string optionDesc = "=<" + def.argName + ">";

                // Calculate padding for alignment
                const int padding = std::max(0, 20 - static_cast<int>(option.length() + optionDesc.length()));
                std::cout << optionDesc << std::string(padding, ' ');

                // Print description and default value
                std::cout << def.description;
                if (!def.defaultValue.empty()) {
                    std::cout << " (default: " << def.defaultValue << ")";
                }

                std::cout << std::endl;
            }
            std::cout << std::endl;
        }

        // Display flags by category
        for (const auto& [category, flags] : flagsByCategory) {
            std::cout << category << " Flags:" << std::endl;

            for (const auto& flag : flags) {
                const auto& def = flagDefs_.find(flag)->second;

                std::cout << "  -" << flag;

                // Calculate padding for alignment
                const int padding = std::max(0, 20 - static_cast<int>(flag.length()));
                std::cout << std::string(padding, ' ');

                // Print description
                std::cout << def.description << std::endl;
            }
            std::cout << std::endl;
        }

        // Display examples if any
        if (!examples_.empty()) {
            std::cout << "Examples:" << std::endl;

            for (const auto& example : examples_) {
                std::cout << "  " << example.example << std::endl;
                std::cout << "      " << example.description << std::endl;
                std::cout << std::endl;
            }
        }
    }

    CommandLineParser& CommandLineParser::addFlagDefinition(
        const std::string& flag,
        const std::string& description,
        const std::string& category) {

        flagDefs_[flag] = { description, category };
        return *this;
    }

    CommandLineParser& CommandLineParser::addOptionDefinition(
        const std::string& option,
        const std::string& argName,
        const std::string& description,
        const std::string& category,
        const std::string& defaultValue) {

        optionDefs_[option] = { argName, description, category, defaultValue };
        return *this;
    }

    CommandLineParser& CommandLineParser::addExample(
        const std::string& example,
        const std::string& description) {

        examples_.push_back({ example, description });
        return *this;
    }

} // namespace sidblaster
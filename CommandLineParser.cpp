#include "CommandLineParser.h"
#include "SidblasterUtils.h"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace sidblaster {

    CommandLineParser::CommandLineParser(int argc, char** argv) {
        if (argc > 0) {
            programName_ = std::filesystem::path(argv[0]).filename().string();
        }

        // First, extract the input file which should be the last argument
        if (argc > 1) {
            std::string lastArg = argv[argc - 1];

            // Check if the last argument is a command (starts with '-')
            if (lastArg.empty() || lastArg[0] != '-') {
                inputFile_ = lastArg;
                // Reduce argc by 1 to ignore the input file in subsequent processing
                argc--;
            }
        }

        // Process command line options
        int i = 1;
        while (i < argc) {
            std::string arg = argv[i++];

            // Skip empty arguments
            if (arg.empty()) {
                continue;
            }

            // Check if it's a command (starts with '-')
            if (arg[0] == '-') {
                // Remove leading dashes
                size_t startPos = 0;
                while (startPos < arg.length() && arg[startPos] == '-') {
                    startPos++;
                }

                if (startPos == arg.length()) {
                    // Just dashes, ignore
                    continue;
                }

                std::string commandName = arg.substr(startPos);

                // Create a command option structure
                CommandOption command;
                command.name = commandName;

                // Collect all parameters for this command (until the next command or end)
                while (i < argc) {
                    std::string param = argv[i];

                    // If the parameter starts with '-', it's a new command
                    if (!param.empty() && param[0] == '-') {
                        break;
                    }

                    // Add parameter to the command
                    command.parameters.push_back(param);
                    i++;
                }

                // If it has no parameters, treat it as a flag
                if (command.parameters.empty()) {
                    flags_.insert(commandName);
                }
                else {
                    // Otherwise, store the command with its parameters
                    commands_[commandName] = command;
                }
            }
        }
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

        std::cout << "Usage: " << programName_ << " [options] inputfile.sid" << std::endl;
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

                // Format option name and argument name
                std::string optionDesc = " <" + def.argName + ">";

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
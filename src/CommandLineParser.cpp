// CommandLineParser.cpp
#include "CommandLineParser.h"
#include "SIDBlasterUtils.h"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace sidblaster {

    CommandLineParser::CommandLineParser(int argc, char** argv) {
        // Save program name
        if (argc > 0) {
            programName_ = std::filesystem::path(argv[0]).filename().string();
        }

        // Save all arguments
        for (int i = 1; i < argc; ++i) {
            args_.push_back(argv[i]);
        }
    }

    CommandClass CommandLineParser::parse() const {
        CommandClass cmd;

        // Process arguments
        std::vector<std::string> positionalArgs;

        size_t i = 0;
        while (i < args_.size()) {
            std::string arg = args_[i++];

            // Skip empty arguments
            if (arg.empty()) {
                continue;
            }

            // Check if it's an option (starts with '-')
            if (arg[0] == '-') {
                // Process option with potential parameter
                std::string option = arg.substr(1);  // Remove leading dash
                size_t equalPos = option.find('=');

                if (equalPos != std::string::npos) {
                    // Option with value using equals sign: -option=value
                    std::string name = option.substr(0, equalPos);
                    std::string value = option.substr(equalPos + 1);
                    cmd.setParameter(name, value);
                }
                else {
                    // Flag without value
                    cmd.setFlag(option);
                }
            }
            else {
                // This is a positional argument (input or output file)
                positionalArgs.push_back(arg);
            }
        }

        // Handle positional arguments (input and output files)
        if (!positionalArgs.empty()) {
            cmd.setInputFile(positionalArgs[0]);
        }

        if (positionalArgs.size() >= 2) {
            cmd.setOutputFile(positionalArgs[1]);
        }

        // Determine command type based on flags and options
        if (cmd.hasFlag("help")) {
            cmd.setType(CommandClass::Type::Help);
        }
        else if (cmd.hasParameter("relocate")) {
            cmd.setType(CommandClass::Type::Relocate);
        }
        else if (cmd.getOutputFile().empty()) {
            // If no output file, default to help
            cmd.setType(CommandClass::Type::Help);
        }
        else {
            // Determine by output file extension
            std::string outputFile = cmd.getOutputFile();
            std::string ext = std::filesystem::path(outputFile).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(),
                [](unsigned char c) { return std::tolower(c); });

            if (ext == ".asm") {
                cmd.setType(CommandClass::Type::Disassemble);
            }
            else {
                cmd.setType(CommandClass::Type::Convert);
            }
        }

        return cmd;
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

        std::cout << "Usage: " << programName_ << " [options] inputfile [outputfile]" << std::endl;
        std::cout << std::endl;
        std::cout << "  inputfile             Path to input file (.sid, .prg, or .bin)" << std::endl;
        std::cout << "  outputfile            Path to output file (.sid, .prg, or .asm)" << std::endl;
        std::cout << "                        If not specified, shows information about input file" << std::endl;
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
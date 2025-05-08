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

        // Determine command type based on flags
        if (cmd.hasFlag("help")) {
            cmd.setType(CommandClass::Type::Help);
        }
        else if (cmd.hasFlag("linkplayer")) {
            cmd.setType(CommandClass::Type::LinkPlayer);
        }
        else if (cmd.hasFlag("relocate") || cmd.hasParameter("relocateaddr")) {
            cmd.setType(CommandClass::Type::Relocate);
        }
        else if (cmd.hasFlag("disassemble")) {
            cmd.setType(CommandClass::Type::Disassemble);
        }
        else if (cmd.hasFlag("trace") || !cmd.getParameter("tracelog").empty()) {
            cmd.setType(CommandClass::Type::Trace);
        }
        else {
            // default to help
            cmd.setType(CommandClass::Type::Help);
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

        std::cout << "SIDBlaster - C64 SID Music Utility" << std::endl;
        std::cout << "Developed by: Robert Troughton (Raistlin of Genesis Project)" << std::endl;
        std::cout << std::endl;

        // Main usage patterns
        std::cout << "USAGE:" << std::endl;
        std::cout << "  " << programName_ << " inputfile.sid outputfile.sid -relocate -relocateaddr=<address>" << std::endl;
        std::cout << "  " << programName_ << " inputfile.sid -trace [-tracelog=<file>] [-traceformat=<format>]" << std::endl;
        std::cout << "  " << programName_ << " inputfile.sid outputfile.prg -linkplayer [-linkplayertype=<name>] [-linkplayeraddr=<address>]" << std::endl;
        std::cout << "  " << programName_ << " inputfile.sid outputfile.asm -disassemble" << std::endl;
        std::cout << "  " << programName_ << " -help" << std::endl;
        std::cout << std::endl;

        // Command descriptions
        std::cout << "COMMANDS:" << std::endl;
        std::cout << "  -relocate        Relocate a SID file to a new memory address" << std::endl;
        std::cout << "  -trace           Trace SID register writes during emulation" << std::endl;
        std::cout << "  -linkplayer      Link SID music with a player to create executable PRG" << std::endl;
        std::cout << "  -disassemble     Disassemble a SID file to assembly code" << std::endl;
        std::cout << "  -help            Display this help information" << std::endl;
        std::cout << std::endl;

        // Relocate command options
        std::cout << "RELOCATE OPTIONS:" << std::endl;
        std::cout << "  -relocateaddr=<address>  Target memory address for relocation (required)" << std::endl;
        std::cout << "                           Example: -relocateaddr=$2000" << std::endl;
        std::cout << std::endl;

        // Trace command options
        std::cout << "TRACE OPTIONS:" << std::endl;
        std::cout << "  -tracelog=<file>         Output file for SID register trace log" << std::endl;
        std::cout << "                           Default: <inputfile>.trace" << std::endl;
        std::cout << "  -traceformat=<format>    Format for trace log: 'text' or 'binary'" << std::endl;
        std::cout << "                           Default: binary" << std::endl;
        std::cout << std::endl;

        // LinkPlayer command options
        std::cout << "LINKPLAYER OPTIONS:" << std::endl;
        std::cout << "  -linkplayertype=<name>   Player type to use" << std::endl;
        std::cout << "                           Default: SimpleRaster" << std::endl;
        std::cout << "  -linkplayeraddr=<address> Player load address" << std::endl;
        std::cout << "                           Default: $0900" << std::endl;
        std::cout << "  -linkplayerdefs=<file>   Player definitions file (optional)" << std::endl;
        std::cout << std::endl;

        // General options
        std::cout << "GENERAL OPTIONS:" << std::endl;
        std::cout << "  -verbose                 Enable verbose logging" << std::endl;
        std::cout << "  -force                   Force overwrite of output file" << std::endl;
        std::cout << "  -logfile=<file>          Log file path (default: SIDBlaster.log)" << std::endl;
        std::cout << "  -kickass=<path>          Path to KickAss.jar assembler" << std::endl;
        std::cout << std::endl;

        // Examples
        std::cout << "EXAMPLES:" << std::endl;
        std::cout << "  " << programName_ << " music.sid relocated.sid -relocate -relocateaddr=$2000" << std::endl;
        std::cout << "    Relocates music.sid to address $2000 and saves as relocated.sid" << std::endl;
        std::cout << std::endl;

        std::cout << "  " << programName_ << " music.sid -trace -tracelog=music.trace -traceformat=text" << std::endl;
        std::cout << "    Traces SID register writes for music.sid in text format" << std::endl;
        std::cout << std::endl;

        std::cout << "  " << programName_ << " music.sid player.prg -linkplayer -linkplayertype=SimpleBitmap -linkplayeraddr=$0800" << std::endl;
        std::cout << "    Links music.sid with SimpleBitmap player at address $0800" << std::endl;
        std::cout << std::endl;

        std::cout << "  " << programName_ << " music.sid music.asm -disassemble" << std::endl;
        std::cout << "    Disassembles music.sid to assembly code in music.asm" << std::endl;
        std::cout << std::endl;
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
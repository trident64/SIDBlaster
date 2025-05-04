#include "SidblasterUtils.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <ctime>

namespace sidblaster {
    namespace util {

        // Initialize static members
        Logger::Level Logger::minLevel_ = Logger::Level::Info;
        std::optional<std::filesystem::path> Logger::logFile_ = std::nullopt;
        bool Logger::consoleOutput_ = true;
        std::unordered_map<std::string, std::string> Configuration::configValues_ = {
            {"kickassPath", DEFAULT_KICKASS_PATH},
            {"exomizerPath", DEFAULT_EXOMIZER_PATH},
            {"compressorType", DEFAULT_COMPRESSOR_TYPE},
            {"playerName", DEFAULT_PLAYER_NAME},
            {"playerPath", DEFAULT_PLAYER_PATH},
            {"playerAddress", DEFAULT_PLAYER_ADDRESS},
            {"defaultSidLoadAddress", DEFAULT_SID_LOAD_ADDRESS},
            {"defaultSidInitAddress", DEFAULT_SID_INIT_ADDRESS},
            {"defaultSidPlayAddress", DEFAULT_SID_PLAY_ADDRESS}
        };

        std::string byteToHex(u8 value, bool upperCase) {
            std::ostringstream ss;
            ss << (upperCase ? std::uppercase : std::nouppercase)
                << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(value);
            return ss.str();
        }

        std::string wordToHex(u16 value, bool upperCase) {
            std::ostringstream ss;
            ss << (upperCase ? std::uppercase : std::nouppercase)
                << std::hex << std::setw(4) << std::setfill('0')
                << value;
            return ss.str();
        }

        std::optional<u16> parseHex(std::string_view str) {
            // Trim whitespace
            const auto start = str.find_first_not_of(" \t\r\n");
            if (start == std::string_view::npos) {
                return std::nullopt;
            }
            const auto end = str.find_last_not_of(" \t\r\n");
            const auto trimmed = str.substr(start, end - start + 1);

            try {
                // Check for hex prefix
                if (!trimmed.empty() && trimmed[0] == '$') {
                    // Convert from "$XXXX" format
                    return static_cast<u16>(std::stoul(std::string(trimmed.substr(1)), nullptr, 16));
                }
                else if (trimmed.size() > 2 && trimmed.substr(0, 2) == "0x") {
                    // Convert from "0xXXXX" format
                    return static_cast<u16>(std::stoul(std::string(trimmed), nullptr, 16));
                }
                else {
                    // Try to parse as decimal
                    return static_cast<u16>(std::stoul(std::string(trimmed), nullptr, 10));
                }
            }
            catch (const std::exception&) {
                // Catch any exceptions from stoul
                return std::nullopt;
            }
        }

        std::string padToColumn(std::string_view str, size_t width) {
            if (str.length() >= width) {
                return std::string(str);
            }

            return std::string(str) + std::string(width - str.length(), ' ');
        }

        // IndexRange implementation
        void IndexRange::update(int offset) {
            min_ = std::min(min_, offset);
            max_ = std::max(max_, offset);
        }

        std::pair<int, int> IndexRange::getRange() const {
            if (min_ > max_) {
                return { 0, 0 };  // No valid data
            }
            return { min_, max_ };
        }

        // Helper function to normalize various address formats to a numeric value
        uint32_t normalizeAddress(const std::string& addrStr) {
            std::string trimmed = addrStr;

            // Handle hex format with 0x prefix
            if (trimmed.size() >= 2 && trimmed.substr(0, 2) == "0x") {
                return std::stoul(trimmed.substr(2), nullptr, 16);
            }
            // Handle hex format with $ prefix
            else if (!trimmed.empty() && trimmed[0] == '$') {
                return std::stoul(trimmed.substr(1), nullptr, 16);
            }
            // Try to determine format based on content
            else {
                // Check if it looks like a hex number (contains A-F)
                bool containsHexChars = false;
                for (char c : trimmed) {
                    if ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
                        containsHexChars = true;
                        break;
                    }
                }

                if (containsHexChars) {
                    // It's a hex number without prefix
                    return std::stoul(trimmed, nullptr, 16);
                }
                else {
                    // Assume decimal
                    return std::stoul(trimmed, nullptr, 10);
                }
            }
        }

        // Helper function for safe localtime
        std::tm getLocalTime(const std::time_t& time) {
            std::tm timeInfo = {};
#ifdef _WIN32
            // Windows-specific version
            localtime_s(&timeInfo, &time);
#else
            // POSIX version
            localtime_r(&time, &timeInfo);
#endif
            return timeInfo;
        }

        void Logger::initialize(const std::filesystem::path& logFile) {
            logFile_ = logFile;
            consoleOutput_ = !logFile_.has_value();
            if (logFile_) {
                // Create directories if needed
                const auto parent = logFile_->parent_path();
                if (!parent.empty()) {
                    std::filesystem::create_directories(parent);
                }
                // Test if we can write to the log file
                std::ofstream file(logFile_.value(), std::ios::trunc);
                if (!file) {
                    std::cerr << "Warning: Could not open log file: " << logFile_.value().string() << std::endl;
                    logFile_ = std::nullopt;
                    consoleOutput_ = true;
                }
                else {
                    // Write header
                    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                    std::tm timeInfo = getLocalTime(now);
                    file << "===== SIDBlaster Log Started at "
                        << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S")
                        << " =====\n";
                }
            }
        }

        void Logger::setLogLevel(Level level) {
            minLevel_ = level;
        }

        void Logger::log(Level level, const std::string& message) {
            if (level < minLevel_) {
                return;
            }

            // Format timestamp
            const auto now = std::chrono::system_clock::now();
            const auto nowTime = std::chrono::system_clock::to_time_t(now);
            std::tm timeInfo = getLocalTime(nowTime);

            std::stringstream timestampStr;
            timestampStr << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");

            // Format level
            std::string levelStr;
            switch (level) {
            case Level::Debug:   levelStr = "DEBUG"; break;
            case Level::Info:    levelStr = "INFO"; break;
            case Level::Warning: levelStr = "WARNING"; break;
            case Level::Error:   levelStr = "ERROR"; break;
            }

            // Format full message
            std::stringstream fullMessage;
            fullMessage << "[" << timestampStr.str() << "] [" << levelStr << "] " << message;

            // Write to console if enabled
            if (consoleOutput_) {
                if (level == Level::Error) {
                    std::cerr << fullMessage.str() << std::endl;
                }
                else {
                    std::cout << fullMessage.str() << std::endl;
                }
            }

            // Write to file if enabled
            if (logFile_) {
                std::ofstream file(logFile_.value(), std::ios::app);
                if (file) {
                    file << fullMessage.str() << std::endl;
                }
            }
        }

        void Logger::debug(const std::string& message) {
            log(Level::Debug, message);
        }

        void Logger::info(const std::string& message) {
            log(Level::Info, message);
        }

        void Logger::warning(const std::string& message) {
            log(Level::Warning, message);
        }

        void Logger::error(const std::string& message) {
            log(Level::Error, message);
        }

        bool Configuration::loadFromFile(const std::filesystem::path& configFile) {
            std::ifstream file(configFile);
            if (!file) {
                Logger::warning("Could not open configuration file: " + configFile.string());
                return false;
            }

            std::string line;
            while (std::getline(file, line)) {
                // Skip empty lines and comments
                if (line.empty() || line[0] == '#' || line[0] == ';') {
                    continue;
                }

                // Find key-value separator
                const auto pos = line.find('=');
                if (pos == std::string::npos) {
                    continue;
                }

                // Extract key and value
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);

                // Trim whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                // Store in config map
                if (!key.empty()) {
                    configValues_[key] = value;
                }
            }

            Logger::info("Configuration loaded from: " + configFile.string());
            return true;
        }

        void Configuration::setValue(const std::string& key, const std::string& value) {
            configValues_[key] = value;
        }

        std::string Configuration::getString(const std::string& key, const std::string& defaultValue) {
            const auto it = configValues_.find(key);
            return (it != configValues_.end()) ? it->second : defaultValue;
        }

        int Configuration::getInt(const std::string& key, int defaultValue) {
            const auto it = configValues_.find(key);
            if (it == configValues_.end()) {
                return defaultValue;
            }

            try {
                return std::stoi(it->second);
            }
            catch (const std::exception&) {
                return defaultValue;
            }
        }

        bool Configuration::getBool(const std::string& key, bool defaultValue) {
            const auto it = configValues_.find(key);
            if (it == configValues_.end()) {
                return defaultValue;
            }

            const auto& value = it->second;
            if (value == "true" || value == "yes" || value == "1" ||
                value == "on" || value == "enable" || value == "enabled") {
                return true;
            }
            else if (value == "false" || value == "no" || value == "0" ||
                value == "off" || value == "disable" || value == "disabled") {
                return false;
            }

            return defaultValue;
        }

        // Tool path accessors
        std::string Configuration::getKickAssPath() {
            return getString("kickassPath", DEFAULT_KICKASS_PATH);
        }

        std::string Configuration::getExomizerPath() {
            return getString("exomizerPath", DEFAULT_EXOMIZER_PATH);
        }

        std::string Configuration::getCompressorType() {
            return getString("compressorType", DEFAULT_COMPRESSOR_TYPE);
        }

        // Player settings accessors
        std::string Configuration::getPlayerName() {
            return getString("playerName", DEFAULT_PLAYER_NAME);
        }

        std::string Configuration::getPlayerPath() {
            return getString("playerPath", DEFAULT_PLAYER_PATH);
        }

        u16 Configuration::getPlayerAddress() {
            std::string addrStr = getString("playerAddress", DEFAULT_PLAYER_ADDRESS);
            auto addr = parseHex(addrStr);
            return addr.value_or(0x0400); // Default to $0400 if parsing fails
        }

        // Default SID address accessors
        u16 Configuration::getDefaultSidLoadAddress() {
            std::string addrStr = getString("defaultSidLoadAddress", DEFAULT_SID_LOAD_ADDRESS);
            auto addr = parseHex(addrStr);
            return addr.value_or(0x1000); // Default to $1000 if parsing fails
        }

        u16 Configuration::getDefaultSidInitAddress() {
            std::string addrStr = getString("defaultSidInitAddress", DEFAULT_SID_INIT_ADDRESS);
            auto addr = parseHex(addrStr);
            return addr.value_or(0x1000); // Default to $1000 if parsing fails
        }

        u16 Configuration::getDefaultSidPlayAddress() {
            std::string addrStr = getString("defaultSidPlayAddress", DEFAULT_SID_PLAY_ADDRESS);
            auto addr = parseHex(addrStr);
            return addr.value_or(0x1003); // Default to $1003 if parsing fails
        }

    } // namespace util
} // namespace sidblaster
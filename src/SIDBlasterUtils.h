#pragma once

#include "Common.h"

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace sidblaster {

    namespace util {

        /**
         * @brief Convert a byte to a hexadecimal string
         *
         * @param value Byte value to convert
         * @param upperCase Whether to use uppercase letters (default: true)
         * @return Formatted hex string (always 2 characters)
         */
        std::string byteToHex(u8 value, bool upperCase = true);

        /**
         * @brief Convert a word to a hexadecimal string
         *
         * @param value Word value to convert
         * @param upperCase Whether to use uppercase letters (default: true)
         * @return Formatted hex string (always 4 characters)
         */
        std::string wordToHex(u16 value, bool upperCase = true);

        /**
         * @brief Parse a hexadecimal string into a numeric value
         *
         * Supports various formats including:
         * - "1234" (decimal)
         * - "$1234" (hex with $ prefix)
         * - "0x1234" (hex with 0x prefix)
         *
         * @param str String to parse
         * @return Parsed value, or std::nullopt if parsing failed
         */
        std::optional<u16> parseHex(std::string_view str);

        /**
         * @brief Pad a string to a specific width with spaces
         *
         * @param str String to pad
         * @param width Target width
         * @return Padded string
         */
        std::string padToColumn(std::string_view str, size_t width);

        /**
         * @brief A range of indices (min to max)
         */
        class IndexRange {
        public:
            /**
             * @brief Update the range to include a new offset
             * @param offset Value to include in the range
             */
            void update(int offset);

            /**
             * @brief Get the current min,max range
             * @return Pair containing min and max values
             */
            std::pair<int, int> getRange() const;

        private:
            int min_ = std::numeric_limits<int>::max();
            int max_ = std::numeric_limits<int>::min();
        };

        /**
         * @brief Normalize various address formats to a single numeric representation
         *
         * This function converts different address format notations into a standard
         * numeric value, supporting multiple common formats used in assembly and
         * low-level programming:
         *
         * Supported formats:
         * - Decimal: "1024" (interpreted as decimal)
         * - Hex with 0x prefix: "0x400" (interpreted as hex)
         * - Hex with $ prefix: "$400" (interpreted as hex)
         * - Unprefixed hex: "400" (interpreted as hex if it contains A-F chars)
         *
         * Examples:
         * - "1024", "$0400", "0x400", "$400" all normalize to 1024 decimal
         *
         * @param addrStr The address string to normalize
         * @return The normalized address as a numeric value
         * @throws std::exception if the address cannot be parsed
         */
        uint32_t normalizeAddress(const std::string& addrStr);

        /**
         * @brief Logging utility for the SIDBlaster project
         */
        class Logger {
        public:
            /**
             * @brief Log severity levels
             */
            enum class Level {
                Debug,
                Info,
                Warning,
                Error
            };

            /**
             * @brief Initialize the logger
             * @param logFile Path to log file (optional, uses console if not provided)
             */
            static void initialize(const std::filesystem::path& logFile = {});

            /**
             * @brief Set minimum log level to show
             * @param level Minimum level
             */
            static void setLogLevel(Level level);

            /**
             * @brief Log a message
             * @param level Message severity
             * @param message Text to log
             */
            static void log(Level level, const std::string& message);

            /**
             * @brief Log a debug message
             * @param message Text to log
             */
            static void debug(const std::string& message);

            /**
             * @brief Log an info message
             * @param message Text to log
             */
            static void info(const std::string& message);

            /**
             * @brief Log a warning message
             * @param message Text to log
             */
            static void warning(const std::string& message);

            /**
             * @brief Log an error message
             * @param message Text to log
             */
            static void error(const std::string& message);

        private:
            static Level minLevel_;
            static std::optional<std::filesystem::path> logFile_;
            static bool consoleOutput_;
        };

        /**
         * @brief Configuration management for the SIDBlaster project
         */
        class Configuration {
        public:
            /**
             * @brief Load configuration from a file
             * @param configFile Path to configuration file
             * @return true if loading succeeded
             */
            static bool loadFromFile(const std::filesystem::path& configFile);

            /**
             * @brief Set a configuration value
             * @param key Configuration key
             * @param value Value to set
             */
            static void setValue(const std::string& key, const std::string& value);

            /**
             * @brief Get a string configuration value
             * @param key Configuration key
             * @param defaultValue Default value if key not found
             * @return Configuration value
             */
            static std::string getString(const std::string& key, const std::string& defaultValue = {});

            /**
             * @brief Get an integer configuration value
             * @param key Configuration key
             * @param defaultValue Default value if key not found
             * @return Configuration value
             */
            static int getInt(const std::string& key, int defaultValue = 0);

            /**
             * @brief Get a boolean configuration value
             * @param key Configuration key
             * @param defaultValue Default value if key not found
             * @return Configuration value
             */
            static bool getBool(const std::string& key, bool defaultValue = false);

            // Tool paths
            static std::string getKickAssPath();
            static std::string getExomizerPath();
            static std::string getCompressorType();

            // Player settings
            static std::string getPlayerName();
            static std::string getPlayerPath();
            static u16 getPlayerAddress();

            // Default SID addresses
            static u16 getDefaultSidLoadAddress();
            static u16 getDefaultSidInitAddress();
            static u16 getDefaultSidPlayAddress();

        private:
            static std::unordered_map<std::string, std::string> configValues_;

            // Default configuration constants
            static constexpr const char* DEFAULT_KICKASS_PATH = "java -jar KickAss.jar";
            static constexpr const char* DEFAULT_EXOMIZER_PATH = "Exomizer.exe";
            static constexpr const char* DEFAULT_COMPRESSOR_TYPE = "exomizer";
            static constexpr const char* DEFAULT_PLAYER_NAME = "SimpleRaster";
            static constexpr const char* DEFAULT_PLAYER_PATH = "player/SimpleRaster/SimpleRaster.asm";
            static constexpr const char* DEFAULT_PLAYER_ADDRESS = "$0900";
            static constexpr const char* DEFAULT_SID_LOAD_ADDRESS = "$1000";
            static constexpr const char* DEFAULT_SID_INIT_ADDRESS = "$1000";
            static constexpr const char* DEFAULT_SID_PLAY_ADDRESS = "$1003";
        };

    } // namespace util
} // namespace sidblaster
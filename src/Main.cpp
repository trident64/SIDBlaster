// Main.cpp
#include "app/SIDBlasterApp.h"
#include "SIDBlasterUtils.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * @brief Main entry point for the SIDBlaster application
 *
 * Creates and runs an instance of the SIDBlasterApp, handling any exceptions
 * that might be thrown during execution.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * @return Exit code (0 on success, 1 on error)
 */
int main(int argc, char** argv) {
    try {
        // Look for configuration file in current directory, executable directory, 
        // and common config locations
        std::vector<fs::path> configPaths = {
            "SIDBlaster.cfg",                                // Current directory
            fs::path(argv[0]).parent_path() / "SIDBlaster.cfg", // Executable directory

            #ifdef _WIN32
            fs::path(getenv("APPDATA") ? getenv("APPDATA") : "") / "SIDBlaster" / "SIDBlaster.cfg", // Windows AppData
            #else
            fs::path(getenv("HOME") ? getenv("HOME") : "") / ".config" / "sidblaster" / "SIDBlaster.cfg", // Unix/Linux ~/.config
            fs::path(getenv("HOME") ? getenv("HOME") : "") / ".sidblaster" / "SIDBlaster.cfg",       // Unix/Linux ~/.sidblaster
            "/etc/sidblaster/SIDBlaster.cfg",                // System-wide config
            #endif
        };

        // Create and run the application
        sidblaster::SIDBlasterApp app(argc, argv);
        return app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
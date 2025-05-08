// Main.cpp
#include "app/SIDBlasterApp.h"
#include <iostream>

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
        sidblaster::SIDBlasterApp app(argc, argv);
        return app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
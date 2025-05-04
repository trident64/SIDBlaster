#include "SIDBlasterApp.h"
#include <iostream>

/**
 * Main entry point
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
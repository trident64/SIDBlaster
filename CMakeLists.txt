cmake_minimum_required(VERSION 3.14)

# Read version from Common.h
file(STRINGS "src/Common.h" VERSION_DEFINE REGEX "^#define SIDBLASTER_VERSION")
string(REGEX REPLACE ".*\"(.*)\".*" "\\1" PROJECT_VERSION "${VERSION_DEFINE}")

project(SIDBlaster VERSION 0.5 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define sources
set(SOURCES
    src/CodeFormatter.cpp
    src/CommandLineParser.cpp
    
    src/cpu6510.cpp
    src/Disassembler.cpp
    src/DisassemblyWriter.cpp
    src/LabelGenerator.cpp
    src/Main.cpp
    src/MemoryAnalyzer.cpp
    src/SIDBlasterApp.cpp
    src/SIDBlasterUtils.cpp
    src/SIDLoader.cpp
)

# Add executable
add_executable(SIDBlaster ${SOURCES})

# Include directories
target_include_directories(SIDBlaster PRIVATE src)

# Define platform-specific settings
if(WIN32)
    target_compile_definitions(SIDBlaster PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Set optimization flags for Release build
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(SIDBlaster PRIVATE /O2)
    else()
        target_compile_options(SIDBlaster PRIVATE -O3)
    endif()
endif()

# Install rule
install(TARGETS SIDBlaster DESTINATION bin)
install(DIRECTORY SIDPlayers DESTINATION share/SIDBlaster)

# Package with CPack (optional)
include(CPack)
set(CPACK_PACKAGE_NAME "SIDBlaster")
set(CPACK_PACKAGE_VENDOR "Genesis Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "C64 SID music manipulator extraordinaire!")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})

# Post-build command to copy executable to root directory
add_custom_command(TARGET SIDBlaster POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
    $<TARGET_FILE:SIDBlaster> 
    ${CMAKE_SOURCE_DIR}/SIDBlaster.exe
    COMMENT "Copying executable to root directory"
)

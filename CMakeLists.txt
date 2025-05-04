cmake_minimum_required(VERSION 3.14)
project(SIDBlaster VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard to C++20 (matches your Visual Studio settings)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define source files
set(SOURCES
    CodeFormatter.cpp
    CommandLineParser.cpp
    Common.cpp
    cpu6510.cpp
    Disassembler.cpp
    DisassemblyWriter.cpp
    LabelGenerator.cpp
    Main.cpp
    MemoryAnalyzer.cpp
    SIDBlasterApp.cpp
    SIDBlasterUtils.cpp
    SIDLoader.cpp
)

# Define header files
set(HEADERS
    CodeFormatter.h
    CommandLineParser.h
    Common.h
    Disassembler.h
    cpu6510.h
    DisassemblyTypes.h
    DisassemblyWriter.h
    LabelGenerator.h
    MemoryAnalyzer.h
    SIDBlasterApp.h
    SIDBlasterUtils.h
    SIDLoader.h
)

# Create executable
add_executable(SIDBlaster ${SOURCES} ${HEADERS})

# Add include directories if needed
# target_include_directories(SIDBlaster PRIVATE include)

# Platform-specific configurations
if(WIN32)
    # Windows-specific settings
    target_compile_definitions(SIDBlaster PRIVATE WIN32)
    if(MSVC)
        # MSVC-specific compiler flags
        target_compile_options(SIDBlaster PRIVATE /W3)
    endif()
elseif(APPLE)
    # macOS-specific settings
    target_compile_options(SIDBlaster PRIVATE -Wall)
elseif(UNIX AND NOT APPLE)
    # Linux-specific settings
    target_compile_options(SIDBlaster PRIVATE -Wall)
endif()

# Debug configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(SIDBlaster PRIVATE _DEBUG)
else()
    target_compile_definitions(SIDBlaster PRIVATE NDEBUG)
endif()

# Install target
install(TARGETS SIDBlaster
    RUNTIME DESTINATION bin
)

# Copy additional files (similar to your post-build steps)
add_custom_command(TARGET SIDBlaster POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
            $<TARGET_FILE:SIDBlaster>
            ${CMAKE_SOURCE_DIR}/$<TARGET_FILE_NAME:SIDBlaster>
    COMMENT "Copying executable to solution directory"
)
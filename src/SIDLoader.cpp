// ==================================
//             SIDBlaster
//
//  Raistlin / Genesis Project (G*P)
// ==================================
#include "cpu6510.h"
#include "SIDLoader.h"
#include "SIDBlasterUtils.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace sidblaster::util;

/**
 * @brief Constructor for SIDLoader
 *
 * Initializes a SID loader with default settings and empty header.
 */
SIDLoader::SIDLoader() {
    std::memset(&header_, 0, sizeof(header_));
}

/**
 * @brief Set the CPU instance to use for loading music data
 *
 * @param cpuPtr Pointer to a CPU6510 instance
 */
void SIDLoader::setCPU(CPU6510* cpuPtr) {
    cpu_ = cpuPtr;
}

/**
 * @brief Override the init address in the SID header
 *
 * @param address New init address
 */
void SIDLoader::setInitAddress(u16 address) {
    header_.initAddress = address;
    Logger::debug("SID init address overridden: $" + wordToHex(address));
}

/**
 * @brief Override the play address in the SID header
 *
 * @param address New play address
 */
void SIDLoader::setPlayAddress(u16 address) {
    header_.playAddress = address;
    Logger::debug("SID play address overridden: $" + wordToHex(address));
}

/**
 * @brief Override the load address in the SID header
 *
 * @param address New load address
 */
void SIDLoader::setLoadAddress(u16 address) {
    header_.loadAddress = address;
    Logger::debug("SID load address overridden: $" + wordToHex(address));
}

/**
 * @brief Load a SID file
 *
 * Reads a SID file, parses its header, and loads the music data into memory.
 *
 * @param filename Path to the SID file
 * @return true if loading succeeded, false otherwise
 */
bool SIDLoader::loadSID(const std::string& filename) {
    if (!cpu_) {
        std::cerr << "CPU not set!\n";
        return false;
    }

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << filename << "\n";
        return false;
    }

    // Get file size
    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize <= 0) {
        std::cerr << "File is empty: " << filename << "\n";
        return false;
    }

    // Read the entire file into a buffer
    std::vector<u8> buffer(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
        std::cerr << "Failed to read file: " << filename << "\n";
        return false;
    }

    // Check if file is large enough to contain a header
    if (fileSize < sizeof(SIDHeader)) {
        std::cerr << "SID file too small!\n";
        return false;
    }

    // Copy and validate header
    std::memcpy(&header_, buffer.data(), sizeof(header_));

    if (std::string(header_.magicID, 4) != "PSID") {
        std::cerr << "Not a valid SID file!\n";
        return false;
    }

    // Fix endianness (SID files are big-endian)
    fixHeaderEndianness(header_);

    // Handle embedded load address if needed
    if (header_.loadAddress == 0) {
        if (fileSize < header_.dataOffset + 2) {
            std::cerr << "SID file corrupt (missing embedded load address)!\n";
            return false;
        }
        const u8 lo = buffer[header_.dataOffset];
        const u8 hi = buffer[header_.dataOffset + 1];
        header_.loadAddress = static_cast<u16>(lo | (hi << 8));
        header_.dataOffset += 2;
    }

    // Calculate data size
    dataSize_ = static_cast<u16>(fileSize - header_.dataOffset);

    if (dataSize_ <= 0 || header_.loadAddress + dataSize_ > 65536) {
        std::cerr << "SID file corrupt or too large!\n";
        return false;
    }

    // Copy music data to CPU memory
    const u8* musicData = &buffer[header_.dataOffset];
    if (!copyMusicToMemory(musicData, dataSize_, header_.loadAddress)) {
        std::cerr << "Failed to copy music data to memory!\n";
        return false;
    }

    return true;
}

/**
 * @brief Load a raw binary file
 *
 * Loads a raw binary file with explicitly specified addresses.
 *
 * @param filename Path to the BIN file
 * @param loadAddr Memory address where the data should be loaded
 * @param initAddr Address of initialization routine
 * @param playAddr Address of play routine
 * @return true if loading succeeded, false otherwise
 */
bool SIDLoader::loadBIN(const std::string& filename, u16 loadAddr, u16 initAddr, u16 playAddr) {
    if (!cpu_) {
        std::cerr << "CPU not set!\n";
        return false;
    }

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << filename << "\n";
        return false;
    }

    // Get file size
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size <= 0) {
        std::cerr << "File is empty: " << filename << "\n";
        return false;
    }

    // Read the entire file into a buffer
    std::vector<u8> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        std::cerr << "Failed to read file: " << filename << "\n";
        return false;
    }

    // Create a SID header for this binary file
    if (!createSIDHeader(loadAddr, initAddr, playAddr)) {
        std::cerr << "Failed to create SID header for binary file!\n";
        return false;
    }

    // Copy music data to CPU memory
    if (!copyMusicToMemory(buffer.data(), static_cast<u16>(size), loadAddr)) {
        std::cerr << "Failed to copy music data to memory!\n";
        return false;
    }

    return true;
}

/**
 * @brief Load a PRG file
 *
 * Loads a C64 program file with a built-in load address.
 *
 * @param filename Path to the PRG file
 * @param initAddr Address of initialization routine
 * @param playAddr Address of play routine
 * @return true if loading succeeded, false otherwise
 */
bool SIDLoader::loadPRG(const std::string& filename, u16 initAddr, u16 playAddr) {
    if (!cpu_) {
        std::cerr << "CPU not set!\n";
        return false;
    }

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file: " << filename << "\n";
        return false;
    }

    // Get file size
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size < 2) {
        std::cerr << "PRG file too small (needs at least load address)!\n";
        return false;
    }

    // Read the entire file into a buffer
    std::vector<u8> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        std::cerr << "Failed to read file: " << filename << "\n";
        return false;
    }

    // First two bytes are the load address in little-endian
    const u16 loadAddr = static_cast<u16>(buffer[0] | (buffer[1] << 8));
    const u16 dataLen = static_cast<u16>(size - 2);

    // Create a SID header for this PRG file
    if (!createSIDHeader(loadAddr, initAddr, playAddr)) {
        std::cerr << "Failed to create SID header for PRG file!\n";
        return false;
    }

    // Copy music data to CPU memory (skip the 2-byte load address)
    if (!copyMusicToMemory(buffer.data() + 2, dataLen, loadAddr)) {
        std::cerr << "Failed to copy music data to memory!\n";
        return false;
    }

    return true;
}

/**
 * @brief Create a SID header for non-SID format files
 *
 * Creates a synthetic SID header with appropriate fields for
 * BIN or PRG files that don't have their own headers.
 *
 * @param loadAddr Memory load address
 * @param initAddr Initialization routine address
 * @param playAddr Play routine address
 * @return true if header creation succeeded
 */
bool SIDLoader::createSIDHeader(u16 loadAddr, u16 initAddr, u16 playAddr) {
    // Create a minimal SID header for non-SID files
    std::memset(&header_, 0, sizeof(header_));
    std::memcpy(header_.magicID, "PSID", 4);
    header_.version = 2;
    header_.dataOffset = 0;  // Not used for our purposes
    header_.loadAddress = loadAddr;
    header_.initAddress = initAddr;
    header_.playAddress = playAddr;
    header_.songs = 1;
    header_.startSong = 1;
    header_.flags = 0;

    return true;
}

/**
 * @brief Copy music data to CPU memory
 *
 * Loads music data into the CPU's memory at the specified address
 * and stores a copy for later reference.
 *
 * @param data Pointer to the music data
 * @param size Size of the data in bytes
 * @param loadAddr Memory address to load the data
 * @return true if copying succeeded
 */
bool SIDLoader::copyMusicToMemory(const u8* data, u16 size, u16 loadAddr) {
    if (!cpu_) {
        std::cerr << "CPU not set!\n";
        return false;
    }

    if (size == 0 || loadAddr + size > 65536) {
        std::cerr << "Invalid data size or load address!\n";
        return false;
    }

    // Load data into CPU memory
    for (u16 i = 0; i < size; ++i) {
        cpu_->writeByte(loadAddr + i, data[i]);
    }

    dataSize_ = size;

    // Save original copy for later reference
    originalMemory_.assign(data, data + size);
    originalMemoryBase_ = loadAddr;

    return true;
}

/**
 * @brief Fix SID header endianness
 *
 * SID files store multi-byte values in big-endian format, but the CPU
 * uses little-endian. This function swaps the byte order as needed.
 *
 * @param header Header to fix
 */
void SIDLoader::fixHeaderEndianness(SIDHeader& header) {
    // SID files store multi-byte values in big-endian format
    auto swapEndian = [](u16 value) -> u16 {
        return (value >> 8) | (value << 8);
        };

    header.version = swapEndian(header.version);
    header.dataOffset = swapEndian(header.dataOffset);
    header.loadAddress = swapEndian(header.loadAddress);
    header.initAddress = swapEndian(header.initAddress);
    header.playAddress = swapEndian(header.playAddress);
    header.songs = swapEndian(header.songs);
    header.startSong = swapEndian(header.startSong);
    header.flags = swapEndian(header.flags);
}

/**
 * @brief Check if the SID file is for PAL or NTSC
 *
 * Determines the intended video standard from the SID header flags.
 *
 * @return true for PAL, false for NTSC
 */
bool SIDLoader::isPAL() const {
    // Default to PAL if no flags or unknown
    if (header_.version < 2 || header_.dataOffset < 0x76) {
        return true;
    }

    // PSID v2+: flags at 0x76
    const u8 video = header_.flags & 0xF;
    return (video == 0 || video == 3);  // 0 = PAL, 3 = Both -> assume PAL
}

/**
 * @brief Backup the current memory to allow restoration later
 *
 * Creates a snapshot of the CPU memory for later restoration.
 *
 * @return True if backup succeeded
 */
bool SIDLoader::backupMemory() {
    if (!cpu_) {
        sidblaster::util::Logger::error("CPU not set for memory backup!");
        return false;
    }

    // Get the entire CPU memory
    auto cpuMemory = cpu_->getMemory();

    // Make a copy
    memoryBackup_.assign(cpuMemory.begin(), cpuMemory.end());

    sidblaster::util::Logger::debug("Memory backup created: " + std::to_string(memoryBackup_.size()) + " bytes");
    return true;
}

/**
 * @brief Restore memory from backup
 *
 * Restores the CPU memory from a previously created backup.
 *
 * @return True if restoration succeeded
 */
bool SIDLoader::restoreMemory() {
    if (!cpu_ || memoryBackup_.empty()) {
        sidblaster::util::Logger::error("Cannot restore memory: CPU not set or backup empty!");
        return false;
    }

    // Copy the backup back to CPU memory
    // Use size_t to avoid overflow with large memory sizes
    for (size_t addr = 0; addr < memoryBackup_.size(); ++addr) {
        // Cast to u16 is safe for addressing CPU memory (which is limited to 64K)
        cpu_->writeByte(static_cast<u16>(addr), memoryBackup_[addr]);
    }

    sidblaster::util::Logger::debug("Memory restored from backup");
    return true;
}
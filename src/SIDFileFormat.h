// ==================================
//             SIDBlaster
//
//  Raistlin / Genesis Project (G*P)
// ==================================
#pragma once

/**
 * @file SIDFileFormat.h
 * @brief Defines the SID file format header structure
 *
 * This file contains the definition of the SIDHeader structure which matches
 * the binary layout of SID music files for the Commodore 64. The structure is
 * packed to ensure correct binary compatibility with the file format.
 */

 // SID header structure (packed to match file format)
#pragma pack(push, 1)
/**
 * @struct SIDHeader
 * @brief Represents the header of a SID music file
 *
 * The SID file format stores multi-byte values in big-endian format.
 * This structure needs to be byte-swapped when loading or saving.
 */
struct SIDHeader {
    char magicID[4];        // 'PSID' or 'RSID'
    u16 version;            // Version number (1-3)
    u16 dataOffset;         // Offset to binary data
    u16 loadAddress;        // Load address
    u16 initAddress;        // Init address
    u16 playAddress;        // Play address
    u16 songs;              // Number of songs
    u16 startSong;          // Default song
    u32 speed;              // Speed flags
    char name[32];          // Song name
    char author[32];        // Author name
    char copyright[32];     // Copyright info
    u16 flags;              // Flags
    u8 startPage;           // Start page (PSID v2)
    u8 pageLength;          // Page length (PSID v2)
    u8 secondSIDAddress;    // Second SID address (PSID v3)
    u8 thirdSIDAddress;     // Third SID address (PSID v4)
};
#pragma pack(pop)
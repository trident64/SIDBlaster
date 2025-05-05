#pragma once

// SID header structure (packed to match file format)
#pragma pack(push, 1)
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

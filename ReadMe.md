# SIDBlaster

A utility for releasing and relocating Commodore 64 SID music files.

Developed by Raistlin/Genesis Project

## Key Features

### Player/Visualizer Integration
- Add players and visualizers to SID files
COMINGSOON: - Customizable player integration with various visualization options

### Relocation Capabilities
- Relocate SID files to different memory addresses
- Support for adjusting memory locations of existing SID files
- Preservation of proper playback after relocation
COMINGSOON: - A "sanity check" to ensure that every single SID write matches the pre-relocated version

NOTE: Work in progress. The relocation code works pretty well with many SIDs but it's not perfect. It also doesn't work for relocating many SIDs by byte offsets - better to move in who Kb for now (so if it was at $1000, keep to $xx00 range, if at $0ff6, keep to $xxf6).

## Usage

### Adding a Player to a SID File

  SIDBlaster -player SimpleBitmap -output lundiaplayer.prg SID/Flex-Lundia.sid
   - links Flex-Lundia.sid to the SimpleBitmap player

  SIDBlaster -player SimpleBitmap -playeraddr $0800 SID/Drax-RockingAround.sid
   - relocates Drax-RockingAround.sid to $8000 and adds the SimpleBitmap player at $0800

Available players:
- SimpleBitmap - just a static bitmap displayed while the music plays
- SimpleRaster - the simplest player, turns the screen off and you have a single raster showing the timing of the SID play
- Many more players will be added as we flesh out the system - of course including full visualisers etc

### Relocating a SID File

  SIDBlaster -relocate $8000 -output relocated.sid SID/music.sid
   - relocates music.sid to $8000 and saves as "relocated.sid"

## File Format Support

This tool supports the following input file formats:
- SID (load/init/play values will be picked up from the SID file - but you can of course override these if you know what you're doing)
- PRG (nb. if init/play are not at $1000/1003, you'll need to use the -sidinitaddr and -sidplayaddr options to specify the right values)
- BIN (nb. if not at $1000 with init/play at $1000/1003, you'll need to use the -sidloadaddr, -sidinitaddr and -sidplayaddr options to specify the right values)

## Full Help (dumped from the "-help: option)

SIDBlaster 0.6.1

Usage: SIDBlaster.exe [options] inputfile.sid

General Options:
  -output <file>       Output file path

Logging Options:
  -logfile <file>      Log file path (default: SIDBlaster.log)

Player Options:
  -player <name>       Player name (default: SimpleRaster)
  -playeraddr <address>Player load address (default: 0900)

SID Options:
  -relocate <address>  Relocation address for the SID
  -sidinitaddr <address>Override SID init address (default: 1000)
  -sidloadaddr <address>Override SID load address (default: 1000)
  -sidplayaddr <address>Override SID play address (default: 1003)

Tools Options:
  -compressor <type>   Compression tool to use (default: exomizer)
  -exomizer <path>     Path to Exomizer (default: Exomizer.exe)
  -kickass <path>      Path to KickAss.jar (default: java -jar KickAss.jar)

General Flags:
  -help                Display this help message

Logging Flags:
  -verbose             Enable verbose logging

Output Flags:
  -nocompress          Don't compress output PRG files

Player Flags:
  -noplayer            Don't link player code

Examples:
  SIDBlaster SID/music.sid
      Processes music.sid with default settings (creates player-linked PRG)

  SIDBlaster -output music.asm SID/music.sid
      Disassembles music.sid to an assembly file

  SIDBlaster -output music.prg -noplayer SID/music.sid
      Creates a PRG file without player code

  SIDBlaster -relocate $2000 -output relocated.sid SID/music.sid
      Relocates music.sid to $2000 and creates a new SID file

  SIDBlaster -player SimpleBitmap -playeraddr $0800 SID/music.sid
      Uses the SimpleBitmap player at address $0800

  SIDBlaster -player SimpleRaster -playeraddr $9000 -relocate $8000 SID/music.sid
      Uses the SimpleRaster player at address $9000 with the music relocated to $8000

## Acknowledgements

- Zagon for Exomizer
- Mads Nielsen for KickAss

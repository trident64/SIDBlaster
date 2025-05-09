# SIDBlaster

*SIDBlaster 0.7.0 - C64 SID Music File Processor*

Developed by Robert Troughton (Raistlin of Genesis Project)

## Overview

SIDBlaster is a versatile tool for processing C64 SID music files. It provides several key functions:

- **Link SID with player**: Convert SID files to executable PRG files with various player routines
- **Relocate**: Move SID music to different memory addresses while preserving functionality
- **Disassemble**: Convert SID files to human-readable assembly language
- **Trace**: Analyze SID register access patterns for debugging and verification

## Basic Usage

```
SIDBlaster [command] [options] inputfile outputfile
```

## Commands

SIDBlaster supports the following main commands:

### `-linkplayer` 
Links a SID file with a player routine to create an executable PRG file.

```
SIDBlaster -linkplayer music.sid music.prg
```

Options:
- `-linkplayertype=<name>`: Player type to use (default: SimpleRaster)
- `-linkplayeraddr=<address>`: Player load address (default: $0900)
- `-linkplayerdefs=<file>`: Player definitions file (optional)

### `-relocate`
Relocates a SID file to a different memory address.

```
SIDBlaster -relocate -relocateaddr=<address> music.sid relocated.sid
```

Options:
- `-relocateaddr=<address>`: Target address for relocation (required, e.g. $2000)

### `-disassemble`
Disassembles a SID file to assembly code.

```
SIDBlaster -disassemble music.sid music.asm
```

### `-trace`
Traces SID register writes during emulation.

```
SIDBlaster -trace -tracelog=<file> music.sid
```

Options:
- `-tracelog=<file>`: Output file for SID register trace log
- `-traceformat=<format>`: Format for trace log: 'text' or 'binary' (default: binary)

### `-help`
Displays help information.

```
SIDBlaster -help
```

## General Options

These options can be used with any command:

- `-verbose`: Enable verbose logging
- `-force`: Force overwrite of output file
- `-logfile=<file>`: Log file path (default: SIDBlaster.log)
- `-kickass=<path>`: Path to KickAss.jar assembler

## SID Metadata Options

- `-title=<text>`: Override SID title
- `-author=<text>`: Override SID author
- `-copyright=<text>`: Override SID copyright
- `-sidloadaddr=<address>`: Override SID load address
- `-sidinitaddr=<address>`: Override SID init address
- `-sidplayaddr=<address>`: Override SID play address

## Player Types

SIDBlaster includes several player routines:

- **SimpleRaster**: Basic raster-based player (default)
- **SimpleBitmap**: Player with bitmap display capabilities

## Examples

### Convert SID to PRG with default player:

```
SIDBlaster -linkplayer music.sid music.prg
```

### Convert SID with specific player at custom address:

```
SIDBlaster -linkplayer -linkplayertype=SimpleBitmap -linkplayeraddr=$0800 music.sid player.prg
```

### Relocate SID to address $2000:

```
SIDBlaster -relocate -relocateaddr=$2000 music.sid relocated.sid
```

### Disassemble SID to assembly:

```
SIDBlaster -disassemble music.sid music.asm
```

### Trace SID register writes:

```
SIDBlaster -trace -tracelog=music.trace -traceformat=text music.sid
```

## Supported File Formats

- **Input**: SID files (.sid)
- **Output**: 
  - PRG files (.prg) - Executable Commodore 64 programs
  - SID files (.sid) - Commodore 64 music format
  - ASM files (.asm) - Assembly language source code

## Requirements

- Java Runtime Environment (for KickAss assembler)
- KickAss Assembler (for assembling code)
- Exomizer (for compression, optional)

## Technical Details

SIDBlaster includes a complete 6510 CPU emulator to analyze SID files and ensure accurate relocation and disassembly. It tracks memory access patterns to identify code, data, and jump targets, producing high-quality disassembly output with meaningful labels.

## Acknowledgements

- Zagon for Exomizer
- Mads Nielsen for KickAss assembler
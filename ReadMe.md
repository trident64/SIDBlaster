# SIDBlaster

*SIDBlaster 0.7.0 - C64 SID Music File Processor*

Developed by Robert Troughton (Raistlin of Genesis Project)

## Overview

SIDBlaster is a versatile tool for processing C64 SID music files. It provides several key functions:

- **Player**: Convert SID files to executable PRG files with various player routines
- **Relocate**: Move SID music to different memory addresses while preserving functionality
- **Disassemble**: Convert SID files to human-readable assembly language
- **Trace**: Analyze SID register access patterns for debugging and verification

## Basic Usage

```
SIDBlaster [command] [options] inputfile [outputfile]
```

## Commands

SIDBlaster supports the following main commands:

### `-player[=<type>]` 
Links a SID file with a player routine to create an executable PRG file.

```
SIDBlaster -player music.sid music.prg
```

Options:
- Use `-player` for the default player (SimpleRaster)
- Use `-player=<type>` to specify a different player (e.g., `-player=SimpleBitmap`)
- `-playeraddr=<address>`: Player load address (default: $0900)

### `-relocate=<address>`
Relocates a SID file to a different memory address.

```
SIDBlaster -relocate=$2000 music.sid relocated.sid
```

The address parameter is required and specifies the target memory location (e.g., $2000).

### `-disassemble`
Disassembles a SID file to assembly code.

```
SIDBlaster -disassemble music.sid music.asm
```

### `-trace[=<file>]`
Traces SID register writes during emulation.

```
SIDBlaster -trace music.sid
```

Options:
- Use `-trace` to output to default file (trace.bin in binary format)
- Use `-trace=<file>` to specify output file
  - Files with .txt or .log extension use text format
  - Files with other extensions use binary format

### `-help`
Displays help information.

```
SIDBlaster -help
```

## General Options

These options can be used with any command:

- `-verbose`: Enable verbose logging
- `-force`: Force overwrite of output file
- `-log=<file>`: Log file path (default: SIDBlaster.log)
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
SIDBlaster -player music.sid music.prg
```

### Convert SID with specific player:

```
SIDBlaster -player=SimpleBitmap music.sid player.prg
```

### Relocate SID to address $2000:

```
SIDBlaster -relocate=$2000 music.sid relocated.sid
```

### Disassemble SID to assembly:

```
SIDBlaster -disassemble music.sid music.asm
```

### Trace SID register writes to default output:

```
SIDBlaster -trace music.sid
```

### Trace SID register writes to custom file:

```
SIDBlaster -trace=music.log music.sid
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

# SIDBlaster

*SIDBlaster 0.7.0 - C64 SID Music File Processor!*

Developed by Robert Troughton (Raistlin of Genesis Project)

## Overview

SIDBlaster is a versatile tool for processing C64 SID music files. It can:

- Convert SID files to PRG (with or without a player)
- Relocate SID music to different memory addresses
- Disassemble SID files to human-readable assembly
- Trace SID register access for analysis and verification
- Process multiple files in batch mode

*- note that we're not at v1.0 status so not all of these will work just yet

## Usage

```
SIDBlaster [options] inputfile outputfile
```

Or for batch mode:

```
SIDBlaster [options] -batch=configfile
```

## Command Line Options

### General Options
- `-help`: Display help message

### SID Options
- `-title=<text>`: Override SID title
- `-author=<text>`: Override SID author
- `-copyright=<text>`: Override SID copyright
- `-relocate=<address>`: Relocate SID to specified address (e.g., $2000)
- `-sidloadaddr=<address>`: Override SID load address
- `-sidinitaddr=<address>`: Override SID init address
- `-sidplayaddr=<address>`: Override SID play address

### Player Options
- `-player=<name>`: Player name to use (omit for no player)
- `-playeraddr=<address>`: Player load address
- `-playerdefs=<file>`: Player definitions file

### Processing Options
- `-batch=<file>`: Batch configuration file for processing multiple files
- `-defs=<file>`: General definitions file

### Logging Options
- `-logfile=<file>`: Log file path (default: SIDBlaster.log)
- `-tracelog=<file>`: Log file for SID register writes
- `-traceformat=<format>`: Format for trace log (text/binary)
- `-verbose`: Enable verbose logging

### Output Options
- `-nocompress`: Don't compress output PRG files
- `-force`: Force overwrite output file

### Tool Paths
- `-kickass=<path>`: Path to KickAss.jar
- `-exomizer=<path>`: Path to Exomizer
- `-compressor=<type>`: Compression tool to use (exomizer, pucrunch)

## Batch Mode

The batch mode allows processing multiple files with different settings. Create a batch configuration file with sections for each task:

```
[Task1]
type=Convert
input=music.sid
output=music.prg
player=SimpleBitmap
playerAddr=$0900

[Task2]
type=Relocate
input=music.sid
output=relocated.sid
address=$2000

[Task3]
type=Trace
input=music.sid
output=music_trace.bin
format=binary
frames=30000
```

### Batch Task Types

#### Convert

Converts between file formats, optionally adding a player:

```
[Task]
type=Convert
input=music.sid
output=music.prg
format=PRG
player=SimpleRaster
playerAddr=0x0900
compress=true
```

**Parameters:**
- `input`: Source file path (SID, PRG)
- `output`: Output file path
- `format`: Output format (PRG, SID, ASM)
- `player`: Player name (omit for no player)
- `playerAddr`: Memory address to load player at (hexadecimal)
- `compress`: Whether to compress output (true/false)

#### Relocate

Relocates a SID file to a different memory address:

```
[Task]
type=Relocate
input=music.sid
output=relocated.sid
address=0x2000
```

**Parameters:**
- `input`: Source SID file
- `output`: Output SID file
- `address`: Target relocation address (hexadecimal)

#### Trace

Creates a trace log of SID register writes during playback:

```
[Task]
type=Trace
input=music.sid
output=music-trace.bin
format=binary
frames=30000
```

**Parameters:**
- `input`: SID file to trace
- `output`: Output trace log file
- `format`: Trace format (binary)
- `frames`: Number of frames to trace
- `initAddr`: Optional init address override (hexadecimal)
- `playAddr`: Optional play address override (hexadecimal)

#### Verify

Compares two trace logs to verify if they match:

```
[Task]
type=Verify
original=original-trace.bin
relocated=relocated-trace.bin
reportFile=verification.log
```

**Parameters:**
- `original`: Path to first trace log
- `relocated`: Path to second trace log
- `reportFile`: Path for comparison report

### Example Batch File

Here's a complete example showing a workflow that:
1. Converts a SID to PRG with player
2. Creates a trace log of the original SID
3. Relocates the SID to a new address
4. Creates a trace log of the relocated SID
5. Verifies that both trace logs match

```
[Task1]
type=Convert
input=Test\Music.sid
output=Test\Music.prg
format=PRG
player=SimpleRaster
playerAddr=0x2000
compress=true

[Task2]
type=Trace
input=Test\Music.sid
output=Test\Music-trace.bin
format=binary
frames=30000

[Task3]
type=Relocate
input=Test\Music.sid
output=Test\Music-relocated.sid
address=0x2000

[Task4]
type=Trace
input=Test\Music-relocated.sid
output=Test\Music-relocated-trace.bin
format=binary
frames=30000

[Task5]
type=Verify
original=Test\Music-trace.bin
relocated=Test\Music-relocated-trace.bin
reportFile=verification.log
```

## Examples

Convert a SID file to a PRG with the default player:
```
SIDBlaster music.sid music.prg
```

Disassemble a SID file to assembly:
```
SIDBlaster music.sid music.asm
```

Create a PRG without a player:
```
SIDBlaster -player= music.sid music.prg
```

Relocate a SID file:
```
SIDBlaster -relocate=$2000 music.sid relocated.sid
```

Use a specific player:
```
SIDBlaster -player=SimpleBitmap -playeraddr=$0800 music.sid music.prg
```

Run batch processing:
```
SIDBlaster -batch=jobs.txt
```

## Player Libraries

SIDBlaster supports various player routines:

- **Default**: The simplest .. turns the screen off and plays the music (default)
- **SimpleRaster**: Basic raster player
- **SimpleBitmap**: Shows a bitmap - drop your own in or wait for us to provide configuration options (TODO!)

Place custom players in the `SIDPlayers` directory.

## Building

### Windows
```
build.bat
```

### Linux/macOS
```
./build.sh
```

## Dependencies

SIDBlaster requires:
- KickAss Assembler (for ASM compilation)
- Exomizer or Pucrunch (for PRG compression)

## Acknowledgements

- Zagon for Exomizer
- Mads Nielsen for KickAss


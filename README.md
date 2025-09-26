# Space Invaders Emulator (Intel 8080)

An Intel 8080 CPU emulator running the classic Space Invaders arcade game from 1978.

## Features

- Full Intel 8080 CPU emulation
- Original Space Invaders ROM support
- SDL3 for graphics and input handling
- CPU diagnostic test support
- Cycle-accurate timing

## Controls

- `Z` - Shoot
- `Q` - Move Left 
- `D` - Move Right
- `C` - Insert Coin
- `R` - 1 Player Start
- `T` - 2 Player Start
- `ESC` - Quit

## Building

### Prerequisites

- GCC compiler
- SDL3 development libraries
- Make

### Build Steps

```bash
make clean   # Clean previous build
make        # Build the emulator
```

## Running

```bash
# Run Space Invaders
./bin/emu rom/invaders.rom

# Run CPU diagnostics
./bin/emu rom/test_rom/cpudiag.bin
```

For use test rom u need to make some modification and compile with i8080_test.c not main.c

## Project Structure

```
.
├── src/                # Source files
│   ├── main.c         # Main entry point
│   ├── cpu8080.c      # CPU emulation
│   ├── memory.c       # Memory management
│   ├── io.c          # I/O port handling
│   └── video.c       # Video/Display handling
├── includes/          # Header files
└── rom/              # ROM files
```

## License

This project is open source and available under the MIT License.

## Credits

- Original Space Invaders game by Taito/Midway
- SDL3 library by Simple DirectMedia Layer team
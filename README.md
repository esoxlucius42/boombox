# Boombox

Audio player for Raspberry Pi 5 built with C++ and Qt6.

## Requirements

- C++17 compiler
- Qt6 (Core, Gui, Widgets)
- libmpv
- CMake 3.16+

## Build Instructions

```bash
mkdir build
cd build
cmake ..
make
```

The executable will be located in `build/bin/boombox`.

## Project Structure

```
boombox/
├── CMakeLists.txt      # Main build configuration
├── src/                # Source files
│   └── main.cpp
├── include/            # Header files
├── resources/          # Assets and styling
└── README.md
```

## Features (Planned)

- Audio playback with libmpv
- Qt-based GUI
- Optimized for Raspberry Pi 5

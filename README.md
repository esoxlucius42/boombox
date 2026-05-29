# Boombox

![Boombox app screenshot showing the current touchscreen playback UI](resources/appshot.png)

Boombox is a Qt6 + C++ audio player designed for Raspberry Pi, especially 800x480 touchscreen setups.

## Features

- Plays audio files from a selected folder with recursive folder scanning
- Uses random track selection for playback progression
- Displays track title, artist, album, and queue position
- Extracts and shows embedded album art when available
- Includes a live seek bar with elapsed and total time
- Shows a stereo spectrum visualizer during playback
- Provides large touchscreen-friendly controls for **Play/Pause**, **Next**, **Browse**, and in-app **Fullscreen**
- Remembers the last selected folder in `~/.boombox/config.json` and auto-loads it on startup
- Writes logs to `~/boombox.log`

## Supported audio file extensions

`mp3`, `flac`, `wav`, `ogg`, `m4a`, `aac`, `wma`, `ape`

## Prerequisites

### Runtime dependencies

- Raspberry Pi OS (64-bit recommended)
- Qt6 runtime libraries
- GStreamer runtime/plugins for audio playback

### Build dependencies

- CMake 3.16+
- C++17 compiler toolchain (`g++`, `make`, etc.)
- Qt6 development packages (`qt6-base-dev`)
- `pkg-config`
- `libgstreamer1.0-dev`

### Install GStreamer if missing

On Raspberry Pi OS / Debian:

```bash
sudo apt update
sudo apt install -y \
  libgstreamer1.0-dev \
  gstreamer1.0-tools \
  gstreamer1.0-plugins-base \
  gstreamer1.0-plugins-good \
  gstreamer1.0-libav
```

Then rebuild:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Quick start (local build)

```bash
git clone <repository-url> boombox
cd boombox
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
```

Run:

```bash
./build/bin/boombox
```

Fullscreen mode:

```bash
./build/bin/boombox --fullscreen
```

## CLI commands for users

Boombox currently supports one runtime flag:

- `--fullscreen` — starts the window in fullscreen mode

Common user shell commands:

```bash
# Start app in windowed mode
./build/bin/boombox

# Start app in fullscreen mode
./build/bin/boombox --fullscreen

# Follow logs
tail -f ~/boombox.log
```

## Raspberry Pi OS installation (full guide)

For full installation, deployment, service setup, and troubleshooting on Raspberry Pi OS, see:

- [`DEPLOYMENT.md`](./DEPLOYMENT.md)

Quick auto-install on Raspberry Pi OS:

```bash
sudo ./scripts/install_raspberry_pi_os.sh
```

Quick update on Raspberry Pi OS:

```bash
sudo ./scripts/update_raspberry_pi_os.sh
```

## Useful developer commands

```bash
# Build
cmake --build build

# Run logger test binary
./build/bin/test_logger_exe

# Run error handling test binary
./build/bin/test_error_handling
```

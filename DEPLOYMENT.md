# Boombox Deployment Guide (Raspberry Pi OS)

This guide documents prerequisites, installation, and day-to-day commands for running Boombox on Raspberry Pi OS.

## 1. Deployment target

- **Board**: Raspberry Pi 5 (ARM64)
- **OS**: Raspberry Pi OS 64-bit
- **Recommended**: active internet connection during setup, external audio output configured

## 2. Prerequisites

### 2.1 System packages

Install build and runtime dependencies:

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  pkg-config \
  qt6-base-dev \
  libmpv-dev
```

Optional but useful diagnostics packages:

```bash
sudo apt install -y \
  git \
  alsa-utils \
  pulseaudio-utils \
  bluetooth
```

### 2.2 Verify prerequisites

```bash
cmake --version
pkg-config --modversion Qt6Core
pkg-config --modversion mpv
```

If `pkg-config --modversion mpv` fails, the project can still try direct `libmpv` lookup during CMake configure. If no linkable libmpv is found, Boombox builds with a stub backend (UI works, playback unavailable).

## 3. Get source code

```bash
git clone <repository-url> boombox
cd boombox
```

## 4. Build on Raspberry Pi OS

Configure:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

Build:

```bash
cmake --build build -j4
```

Validate output:

```bash
ls -l build/bin/boombox
```

## 5. Install options

### 5.1 User-local install (no sudo at runtime)

```bash
mkdir -p "$HOME/apps/boombox"
cp build/bin/boombox "$HOME/apps/boombox/boombox"
chmod +x "$HOME/apps/boombox/boombox"
```

Run:

```bash
"$HOME/apps/boombox/boombox"
```

### 5.2 System-wide install

```bash
sudo install -d /opt/boombox
sudo install -m 0755 build/bin/boombox /opt/boombox/boombox
sudo ln -sf /opt/boombox/boombox /usr/local/bin/boombox
```

Run:

```bash
boombox
```

## 6. First run

1. Start Boombox.
2. Use **Browse** to select your music folder.
3. Playback starts from a random track if backend is available.
4. App state is saved in `~/.boombox/config.json`.

## 7. CLI commands for users

Boombox runtime flag support:

- `--fullscreen`

Common user command set:

```bash
# Windowed
boombox

# Fullscreen
boombox --fullscreen

# Follow logs (adjust path based on your install location)
tail -f /opt/boombox/boombox.log

# Check linked libraries
ldd /opt/boombox/boombox | grep "not found"

# Check config state
cat ~/.boombox/config.json
```

If running from the build directory instead:

```bash
./build/bin/boombox
./build/bin/boombox --fullscreen
tail -f ./build/bin/boombox.log
```

## 8. Runtime behavior and files

- **Config path**: `~/.boombox/config.json`
- **Log path**: `boombox.log` in the same directory as the executable
- **Audio formats scanned**: `mp3`, `flac`, `wav`, `ogg`, `m4a`, `aac`, `wma`, `ape`
- **Primary UI controls**: Play/Pause, Next, Browse, seek bar

## 9. Troubleshooting

### 9.1 App starts but no sound

```bash
bluetoothctl devices
pactl list short sinks
amixer sget Master
```

If built without linkable `libmpv`, playback will not start; rebuild after installing/repairing `libmpv`.

### 9.2 App fails to start

```bash
ldd /opt/boombox/boombox | grep "not found"
tail -n 200 /opt/boombox/boombox.log
```

### 9.3 Folder loads but no tracks found

Check readable supported file types:

```bash
find /path/to/music -type f | grep -Ei '\.(mp3|flac|wav|ogg|m4a|aac|wma|ape)$' | head
```

### 9.4 Reset app state

```bash
rm -f ~/.boombox/config.json
```

## 10. Optional: run as a systemd service

Create `/etc/systemd/system/boombox.service`:

```ini
[Unit]
Description=Boombox Audio Player
After=network.target sound.target bluetooth.target

[Service]
Type=simple
User=pi
WorkingDirectory=/opt/boombox
ExecStart=/opt/boombox/boombox --fullscreen
Restart=on-failure
RestartSec=3

[Install]
WantedBy=multi-user.target
```

Enable/start:

```bash
sudo systemctl daemon-reload
sudo systemctl enable boombox
sudo systemctl start boombox
sudo systemctl status boombox --no-pager
```

## 11. Update procedure

```bash
cd /path/to/boombox
git pull
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
sudo install -m 0755 build/bin/boombox /opt/boombox/boombox
sudo systemctl restart boombox
```

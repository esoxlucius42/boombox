# Boombox - Raspberry Pi Deployment Guide

## System Requirements

- **Hardware**: Raspberry Pi 5 with ARM64 processor
- **OS**: Raspberry Pi OS (64-bit)
- **Display**: 7" touchscreen (800x480 resolution)
- **Audio**: Bluetooth speaker (must be paired before app starts)
- **Storage**: 500MB free storage for app and config files

## Building for Raspberry Pi

### Prerequisites

Install the required dependencies on the Raspberry Pi:

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  qt6-base-dev \
  qt6-base-private-dev \
  qt6-declarative-dev \
  libmpv-dev
```

### Build Steps

1. **Clone the repository**:
   ```bash
   git clone <repository-url> boombox
   cd boombox
   ```

2. **Create build directory**:
   ```bash
   mkdir build
   cd build
   ```

3. **Configure with CMake (Release build for optimized performance)**:
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

4. **Build the application**:
   ```bash
   make -j4
   ```
   The `-j4` flag uses 4 cores (adjust based on RPi resources if needed)

5. **Verify the binary**:
   ```bash
   ls -la bin/boombox
   ```
   The executable should be at `build/bin/boombox`

### Cross-Compilation (Building on Desktop Linux for RPi)

If building on a desktop Linux system:

1. Install the ARM64 cross-compiler:
   ```bash
   sudo apt install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
   ```

2. Create a CMake toolchain file (`toolchain-arm64.cmake`):
   ```cmake
   set(CMAKE_SYSTEM_NAME Linux)
   set(CMAKE_SYSTEM_PROCESSOR aarch64)
   set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
   set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
   set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)
   set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
   set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
   set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
   ```

3. Configure and build:
   ```bash
   cmake .. -DCMAKE_TOOLCHAIN_FILE=toolchain-arm64.cmake -DCMAKE_BUILD_TYPE=Release
   make -j4
   ```

## Installation on Raspberry Pi

### Option 1: Install to /opt/boombox (System-wide)

```bash
# Create installation directory
sudo mkdir -p /opt/boombox

# Copy the binary
sudo cp build/bin/boombox /opt/boombox/

# Make it executable
sudo chmod +x /opt/boombox/boombox

# Create a symlink for easy access
sudo ln -s /opt/boombox/boombox /usr/local/bin/boombox
```

### Option 2: Install to Home Directory (User)

```bash
# Create installation directory
mkdir -p ~/boombox

# Copy the binary
cp build/bin/boombox ~/boombox/

# Make it executable
chmod +x ~/boombox/boombox

# Add to PATH (optional, add to ~/.bashrc)
export PATH="$PATH:$HOME/boombox"
```

### Create Config Directory

```bash
mkdir -p ~/.boombox
```

## First Time Setup

1. **Start the application**:
   ```bash
   # If installed system-wide
   /opt/boombox/boombox

   # OR if in home directory
   ~/boombox/boombox

   # OR with fullscreen mode enabled
   /opt/boombox/boombox --fullscreen
   ```

2. **Initial configuration**:
   - Application will create `~/.boombox/config.json` automatically
   - Click the "Browse" button to select a folder with audio files
   - The first track will start playing automatically (random selection)
   - Settings are saved automatically on exit

3. **Verify audio output**:
   - Check that the Bluetooth speaker is paired and connected
   - Verify volume is not muted using system controls
   - Test with a track to ensure audio plays correctly

## Usage

### On-Screen Controls

The interface is optimized for 7" touchscreen (800x480):

- **Left Panel (320x480)**:
  - Track Info: Current track name, artist, album, track count
  - Seek Bar: Tap to seek within current track, progress display
  - Control Buttons:
    - **Previous**: Skip to previous track (regress in queue)
    - **Play/Pause**: Toggle playback (button text changes)
    - **Next**: Skip to next track
    - **Random ON/OFF**: Toggle random mode (true random selection)
    - **Browse**: Select audio folder
    - **Vol-**: Decrease volume (controlled by libmpv)
    - **Vol+**: Increase volume (controlled by libmpv)

- **Right Panel (480x480)**:
  - Album Art: Displays album artwork extracted from track metadata

### Keyboard Shortcuts (if keyboard attached)

- Press `Escape` key to exit application
- Alt+Enter to toggle fullscreen (if not started with --fullscreen)

### Fullscreen Mode

Run with fullscreen flag to hide window chrome:

```bash
/opt/boombox/boombox --fullscreen
```

This is recommended for RPi deployment to maximize usable screen space.

## Audio Configuration

### Bluetooth Speaker Setup

1. **Before starting the app**:
   ```bash
   # Pair Bluetooth speaker using system controls
   # Use: Settings > Bluetooth (on RPi desktop)
   # OR use bluetoothctl:
   bluetoothctl
   # Then: scan on, pair <MAC>, connect <MAC>, trust <MAC>
   ```

2. **Audio routing**:
   - Boombox uses system default audio output
   - libmpv automatically routes to paired Bluetooth device
   - Verify device is "Connected" before launching app

3. **Volume control**:
   - Use on-screen Vol+/Vol- buttons
   - Boombox controls playback volume (0-100%)
   - Bluetooth speaker may have independent volume

### Troubleshooting Audio

**No sound output**:
1. Check Bluetooth speaker is paired and connected:
   ```bash
   bluetoothctl info
   ```

2. Verify audio device configuration:
   ```bash
   alsamixer
   # OR
   amixer sset Master unmute
   ```

3. Check system audio routing:
   ```bash
   pactl list sinks
   ```

4. Review application logs for errors (see Logging section)

## Logging and Debugging

### Log File Location

Logs are written to `./boombox.log` in the same directory as the executable:

```bash
# If installed in /opt/boombox/
tail -f /opt/boombox/boombox.log

# If in home directory
tail -f ~/boombox/boombox.log
```

### Log Levels

- **INFO**: Standard operational messages
- **DEBUG**: Detailed diagnostic information
- **WARN**: Non-critical issues
- **ERROR**: Critical failures

### Common Diagnostics

**Check if app is running**:
```bash
ps aux | grep boombox
```

**Monitor in real-time**:
```bash
# Terminal 1: Start app and watch logs
tail -f /opt/boombox/boombox.log

# Terminal 2: Run boombox
/opt/boombox/boombox --fullscreen
```

**Review startup sequence**:
```bash
grep "initialized\|error\|Error" /opt/boombox/boombox.log
```

## Performance Optimization

### Memory Usage

Expected memory footprint:
- Boombox binary: ~80 MB
- Runtime (idle): ~150 MB
- Runtime (playing): ~200 MB
- Total system: Acceptable for RPi 5 (4GB+ recommended)

### Optimization Tips

1. **Run headless** (without desktop environment):
   ```bash
   # Reduces system memory usage
   # Use: Preferences > Boot > Desktop vs. Console
   ```

2. **Disable unnecessary services**:
   ```bash
   # Disable desktop if not needed
   sudo systemctl disable lightdm
   ```

3. **Monitor performance**:
   ```bash
   top
   # Watch CPU %, MEM %, temperature
   ```

### Temperature Management

RPi 5 has thermal throttling. If experiencing performance issues:

1. Check system temperature:
   ```bash
   vcgencmd measure_temp
   ```

2. Ensure adequate cooling (heatsink + case ventilation)

3. Monitor throttling status:
   ```bash
   vcgencmd get_throttled
   ```

## Troubleshooting

### UI is Not Responsive

1. Verify 800x480 resolution is set:
   ```bash
   xrandr
   ```

2. Check touch screen calibration:
   ```bash
   TSLIB_CALIBRATE=1 ts_calibrate
   ```

3. Review logs for errors:
   ```bash
   grep -i "touch\|event\|error" boombox.log
   ```

### Application Crashes on Startup

1. Check for missing dependencies:
   ```bash
   ldd /opt/boombox/boombox | grep "not found"
   ```

2. Review log file for startup errors:
   ```bash
   cat boombox.log | grep -i error
   ```

3. Try running in verbose mode with terminal output:
   ```bash
   /opt/boombox/boombox 2>&1 | tee debug.log
   ```

### File Browser Doesn't Open

1. Verify folder permissions:
   ```bash
   # Config directory writable
   ls -la ~/.boombox/
   # Audio folders readable
   ls -la /path/to/audio/folder/
   ```

2. Check filesystem for issues:
   ```bash
   df -h
   fsck -n /  # Check (read-only)
   ```

### No Supported Audio Files Found

1. Verify supported formats are present:
   - MP3, FLAC, WAV, OGG (via libmpv)

2. Check file extensions:
   ```bash
   ls -la /path/to/folder/*.{mp3,flac,wav,ogg}
   ```

3. Verify file permissions:
   ```bash
   chmod 644 /path/to/audio/*.mp3
   ```

## Deployment Architecture

### Technology Stack

- **GUI Framework**: Qt6 (C++)
- **Audio Engine**: libmpv with C binding
- **State Management**: JSON config file (Qt serialization)
- **Logging**: File-based with timestamp and level tracking
- **Build System**: CMake 3.16+

### Data Flow

```
User Input (Touch) 
    ↓
Qt6 Event System → ControlsWidget
    ↓
PlaybackController
    ↓
AudioEngine (libmpv)
    ↓
System Audio → Bluetooth Speaker

StateManager
    ↓
~/.boombox/config.json
```

### Hardware Interface

- **Display**: Qt6 renders to framebuffer (800x480)
- **Touch**: Qt6 touch events from kernel input drivers
- **Audio**: libmpv routes to system ALSA/PulseAudio
- **Storage**: Config file in home directory

## Advanced Configuration

### Custom Build Options

```bash
# Debug build (with symbols)
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Install to specific prefix
cmake .. -DCMAKE_INSTALL_PREFIX=/custom/path

# Parallel build (adjust to CPU count)
make -j8
```

### Environment Variables

```bash
# Qt platform plugin selection
export QT_QPA_PLATFORM=linuxfb  # Framebuffer (for headless)

# libmpv logging
export MPV_LOG=all

# Application-specific
export QT_LOGGING_RULES="*=true"
```

### Systemd Service Setup (Optional)

Create `/etc/systemd/system/boombox.service`:

```ini
[Unit]
Description=Boombox Audio Player
After=bluetooth.service

[Service]
Type=simple
ExecStart=/opt/boombox/boombox --fullscreen
Restart=always
RestartSec=5
StandardOutput=journal
User=pi

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl daemon-reload
sudo systemctl enable boombox
sudo systemctl start boombox
```

## Maintenance

### Update Application

```bash
# Pull latest changes
cd ~/boombox-source
git pull origin main

# Rebuild
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4

# Install updated binary
sudo cp bin/boombox /opt/boombox/
```

### Backup Configuration

```bash
# Backup state and preferences
cp ~/.boombox/config.json ~/.boombox/config.json.backup
```

### Log Rotation

Configure logrotate for long-term deployments:

Create `/etc/logrotate.d/boombox`:
```
/opt/boombox/boombox.log {
    daily
    rotate 7
    compress
    delaycompress
    notifempty
    create 0644 pi pi
}
```

## Known Limitations

1. **No Shuffle Mode**: Only true random track selection available
2. **Single Folder**: Cannot browse multiple folders simultaneously
3. **No Metadata Editing**: Read-only metadata extraction
4. **Qt6 Requirement**: No Qt5 compatibility
5. **Touch Only**: Designed for touch input (mouse supported but not primary)

## Support and Debugging Checklist

Before reporting issues, verify:

- [ ] Raspberry Pi OS is fully updated: `sudo apt update && sudo apt upgrade`
- [ ] All dependencies installed: `dpkg -l | grep qt6` and `dpkg -l | grep libmpv`
- [ ] Boombox binary is executable: `ls -la /opt/boombox/boombox`
- [ ] Config directory exists and is writable: `ls -la ~/.boombox`
- [ ] Bluetooth speaker is paired and connected: `bluetoothctl show`
- [ ] Audio device is working: `aplay -l` shows speakers
- [ ] Touch screen is responsive: `TSLIB_CONFFILE=/etc/ts.conf ts_test`
- [ ] Logs show no startup errors: `grep -i error boombox.log`

## Technical Details

### Audio Playback Pipeline

```
File → libmpv → ALSA/PulseAudio → Bluetooth Device
```

Volume control is applied at the libmpv layer (0-100% range).

### State Persistence

- **Config File**: `~/.boombox/config.json`
- **Format**: JSON (Qt serialized)
- **Contents**: Current folder path, last played track
- **Auto-save**: On application exit

### Resource Cleanup

- All resources released on application exit
- Config directory automatically created on first run
- Log file appended to with timestamp and level

---

**Last Updated**: 2025
**For Questions**: See logs at `/opt/boombox/boombox.log`

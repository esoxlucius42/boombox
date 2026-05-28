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
  libmpv2 \
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

If `pkg-config --modversion mpv` fails, the project can still use a direct `libmpv` runtime-library lookup during CMake configure. If no usable libmpv runtime is found, Boombox builds with a stub backend (UI works, playback unavailable).

## 3. Get source code

```bash
git clone <repository-url> boombox
cd boombox
```

## 3.1 One-command auto install (Raspberry Pi OS)

This installs dependencies, builds Boombox, installs it to `/opt/boombox`, and registers it in the desktop app menu under **Sound & Video**.

```bash
sudo ./scripts/install_raspberry_pi_os.sh
```

For day-to-day updates on an existing Pi install, use:

```bash
sudo ./scripts/update_raspberry_pi_os.sh
```

## 4. Build on Raspberry Pi OS (manual)

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

# Follow logs
tail -f ~/boombox.log

# Check linked libraries
ldd /opt/boombox/boombox | grep "not found"

# Check config state
cat ~/.boombox/config.json
```

If running from the build directory instead:

```bash
./build/bin/boombox
./build/bin/boombox --fullscreen
tail -f ~/boombox.log
```

## 8. Runtime behavior and files

- **Config path**: `~/.boombox/config.json`
- **Log path**: `~/boombox.log`
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
tail -n 200 ~/boombox.log
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

### 9.5 Playback stutters mid-track for a few seconds

Recent builds log extra playback diagnostics to help distinguish:

- mpv cache starvation / buffering pauses
- audio-device reconfiguration
- Boombox detecting that `time-pos` stopped advancing while playback was expected

Start by following the app log:

```bash
tail -f ~/boombox.log
```

The log file is always written to:

```bash
tail -f ~/boombox.log
```

Look for lines containing:

- `Playback stall suspected`
- `paused-for-cache=yes`
- `audio reconfigured`
- `Playback diagnostic snapshot`

Then compare the same file with plain mpv on the same Pi and output path:

```bash
mpv --no-video --audio-display=no "/path/to/the/same/test-track.flac"
```

Then check the Pi for USB or audio stack issues around the same time:

```bash
sudo dmesg | grep -Ei 'usb|uas|reset|i/o|ext4|xfs|snd|alsa' | tail -n 100
journalctl --since '-10 min' --no-pager | grep -Ei 'boombox|mpv|usb|alsa|pipewire|pulseaudio'
lsblk -o NAME,MODEL,TRAN,ROTA,MOUNTPOINT
```

Interpretation guide:

- If the log shows `paused-for-cache=yes` or very low `demuxer-cache-duration`, suspect media-drive latency, USB autosuspend, cable/power issues, or filesystem stalls.
- If the log shows `audio reconfigured` or the system log reports ALSA/PipeWire/PulseAudio device resets, suspect the audio sink rather than decoding.
- If system load and memory remain low while the app reports stalled `time-pos`, decoding performance is less likely than storage or audio-output interruption.
- If plain `mpv` also stutters with the same file and sink, focus on the Raspberry Pi audio/backend stack before spending more time on Qt/UI code.
- If plain `mpv` is clean while Boombox still stutters, the remaining app-side playback workload is the better place to investigate.
- If the pause happens only at track changes, focus next on metadata and album-art extraction latency instead.

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
sudo ./scripts/update_raspberry_pi_os.sh
```

The update script performs a fast-forward `git pull`, reruns the Raspberry Pi installer, and restarts `boombox.service` automatically when that service is active. Set `UPDATE_SOURCE=0` to rebuild the current checkout without pulling, or `RESTART_SERVICE=0` to skip the service restart.

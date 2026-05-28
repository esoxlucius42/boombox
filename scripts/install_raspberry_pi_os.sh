#!/usr/bin/env bash
set -euo pipefail

# Resolve repo root from the script location, works regardless of cwd.
# Fall back to the parent of the directory containing this script.
if [[ -n "${BASH_SOURCE[0]:-}" ]]; then
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
else
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
fi
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Install prefix can be overridden: INSTALL_PREFIX=/usr/local sudo ./install_raspberry_pi_os.sh
INSTALL_PREFIX="${INSTALL_PREFIX:-/opt/boombox}"
DESKTOP_DIR="${DESKTOP_DIR:-/usr/share/applications}"
SYMLINK_DIR="${SYMLINK_DIR:-/usr/local/bin}"

if [[ "${EUID}" -ne 0 ]]; then
    echo "Please run as root: sudo ./scripts/install_raspberry_pi_os.sh"
    echo "  Optional env overrides:"
    echo "    INSTALL_PREFIX=<dir>  (default: /opt/boombox)"
    echo "    SYMLINK_DIR=<dir>     (default: /usr/local/bin)"
    echo "    DESKTOP_DIR=<dir>     (default: /usr/share/applications)"
    exit 1
fi

if ! command -v apt >/dev/null 2>&1; then
    echo "This installer supports Debian/Raspberry Pi OS systems with apt."
    exit 1
fi

# Determine which user to build as (avoid building as root inside sudo).
BUILD_USER="${SUDO_USER:-}"
if [[ -z "${BUILD_USER}" ]] || ! id -u "${BUILD_USER}" >/dev/null 2>&1; then
    BUILD_USER="$(id -un)"
fi

run_as_build_user() {
    if [[ "${BUILD_USER}" == "$(id -un)" ]]; then
        "$@"
    elif command -v runuser >/dev/null 2>&1; then
        runuser -u "${BUILD_USER}" -- "$@"
    else
        sudo -u "${BUILD_USER}" "$@"
    fi
}

echo "[1/6] Installing system dependencies..."
apt update
apt install -y \
    build-essential \
    cmake \
    pkg-config \
    qt6-base-dev \
    qt6-base-dev-tools \
    libgstreamer1.0-dev \
    gstreamer1.0-tools \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-libav \
    desktop-file-utils

# Warn if the Qt6 MOC tool cannot be found anywhere; it is required for AUTOMOC.
if ! find /usr/lib /usr/libexec /usr/bin -name 'moc' -path '*qt6*' \
        -o -name 'moc6' 2>/dev/null | grep -q .; then
    echo "WARNING: Qt6 moc not found after package install — the build may fail."
    echo "  Try installing 'qt6-tools-dev-tools' manually."
fi

echo "[2/6] Configuring build (repo: ${REPO_ROOT})..."
# Remove any stale build directory that may have been copied from another machine.
# Its CMakeCache.txt would contain the original machine's absolute paths and
# cause CMake to abort with a path-mismatch error.
if [[ -f "${REPO_ROOT}/build/CMakeCache.txt" ]]; then
    echo "  Removing stale build directory..."
    rm -rf "${REPO_ROOT}/build"
fi
run_as_build_user cmake -S "${REPO_ROOT}" -B "${REPO_ROOT}/build" \
    -DCMAKE_BUILD_TYPE=Release

echo "[3/6] Building boombox..."
# Limit parallel jobs to avoid out-of-memory failures on Raspberry Pi.
# Override with BUILD_JOBS=N if you have more RAM available.
BUILD_JOBS="${BUILD_JOBS:-2}"
if ! run_as_build_user cmake --build "${REPO_ROOT}/build" -j"${BUILD_JOBS}"; then
    echo ""
    echo "Build failed. Re-running with verbose output to show the root cause..."
    run_as_build_user cmake --build "${REPO_ROOT}/build" -j1 -- VERBOSE=1 || true
    exit 1
fi

echo "[4/6] Installing binary and assets to ${INSTALL_PREFIX}..."
install -d "${INSTALL_PREFIX}"
install -m 0755 "${REPO_ROOT}/build/bin/boombox" "${INSTALL_PREFIX}/boombox"
install -d "${INSTALL_PREFIX}/resources"
install -m 0644 "${REPO_ROOT}/resources/icon.jpg" "${INSTALL_PREFIX}/resources/icon.jpg"
ln -sf "${INSTALL_PREFIX}/boombox" "${SYMLINK_DIR}/boombox"

echo "[5/6] Installing desktop entry in Sound & Video (${DESKTOP_DIR})..."
install -d "${DESKTOP_DIR}"

# Generate the desktop file using the actual install paths so they are never
# hardcoded in the source file.
cat > "${DESKTOP_DIR}/boombox.desktop" <<EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=Boombox
Comment=Boombox audio player
Exec=${INSTALL_PREFIX}/boombox
Icon=${INSTALL_PREFIX}/resources/icon.jpg
Terminal=false
Categories=AudioVideo;Audio;Player;
Keywords=music;audio;player;boombox;
StartupNotify=true
EOF
chmod 0644 "${DESKTOP_DIR}/boombox.desktop"

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "${DESKTOP_DIR}" || true
fi

echo "[6/6] Install complete."
echo "  Binary : ${INSTALL_PREFIX}/boombox"
echo "  Symlink: ${SYMLINK_DIR}/boombox"
echo "  Desktop: ${DESKTOP_DIR}/boombox.desktop"
echo "Boombox is available from the desktop menu under Sound & Video and via 'boombox'."

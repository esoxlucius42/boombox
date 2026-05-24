#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

if [[ "${EUID}" -ne 0 ]]; then
    echo "Please run as root: sudo ./scripts/install_raspberry_pi_os.sh"
    exit 1
fi

if ! command -v apt >/dev/null 2>&1; then
    echo "This installer supports Debian/Raspberry Pi OS systems with apt."
    exit 1
fi

BUILD_USER="${SUDO_USER:-${USER:-root}}"
if ! id -u "${BUILD_USER}" >/dev/null 2>&1; then
    echo "Unable to determine a valid build user."
    exit 1
fi

echo "[1/6] Installing system dependencies..."
apt update
apt install -y \
    build-essential \
    cmake \
    pkg-config \
    qt6-base-dev \
    libmpv-dev \
    desktop-file-utils

echo "[2/6] Configuring build..."
runuser -u "${BUILD_USER}" -- cmake -S "${REPO_ROOT}" -B "${REPO_ROOT}/build" -DCMAKE_BUILD_TYPE=Release

echo "[3/6] Building boombox..."
runuser -u "${BUILD_USER}" -- cmake --build "${REPO_ROOT}/build" -j"$(nproc)"

echo "[4/6] Installing binary and assets..."
install -d /opt/boombox
install -m 0755 "${REPO_ROOT}/build/bin/boombox" /opt/boombox/boombox
install -d /opt/boombox/resources
install -m 0644 "${REPO_ROOT}/resources/icon.jpg" /opt/boombox/resources/icon.jpg
ln -sf /opt/boombox/boombox /usr/local/bin/boombox

echo "[5/6] Installing desktop entry in Sound & Video..."
install -d /usr/share/applications
install -m 0644 "${REPO_ROOT}/packaging/boombox.desktop" /usr/share/applications/boombox.desktop

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database /usr/share/applications || true
fi

echo "[6/6] Install complete."
echo "Boombox is available from the desktop menu under Sound & Video and via 'boombox'."

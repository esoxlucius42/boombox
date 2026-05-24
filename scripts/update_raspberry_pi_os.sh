#!/usr/bin/env bash
set -euo pipefail

# Resolve repo root from the script location, works regardless of cwd.
if [[ -n "${BASH_SOURCE[0]:-}" ]]; then
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
else
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
fi
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
INSTALL_SCRIPT="${SCRIPT_DIR}/install_raspberry_pi_os.sh"

INSTALL_PREFIX="${INSTALL_PREFIX:-/opt/boombox}"
DESKTOP_DIR="${DESKTOP_DIR:-/usr/share/applications}"
SYMLINK_DIR="${SYMLINK_DIR:-/usr/local/bin}"
UPDATE_SOURCE="${UPDATE_SOURCE:-1}"
RESTART_SERVICE="${RESTART_SERVICE:-auto}"

if [[ "${EUID}" -ne 0 ]]; then
    echo "Please run as root: sudo ./scripts/update_raspberry_pi_os.sh"
    echo "  Optional env overrides:"
    echo "    UPDATE_SOURCE=0        (skip git fetch/pull)"
    echo "    RESTART_SERVICE=0      (do not restart boombox.service)"
    echo "    RESTART_SERVICE=1      (restart boombox.service, fail if unavailable)"
    echo "    INSTALL_PREFIX=<dir>   (default: /opt/boombox)"
    echo "    SYMLINK_DIR=<dir>      (default: /usr/local/bin)"
    echo "    DESKTOP_DIR=<dir>      (default: /usr/share/applications)"
    exit 1
fi

if [[ ! -x "${INSTALL_SCRIPT}" ]]; then
    echo "Install script not found or not executable: ${INSTALL_SCRIPT}"
    exit 1
fi

if ! command -v git >/dev/null 2>&1; then
    echo "git is required to update the source checkout."
    exit 1
fi

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

if [[ "${UPDATE_SOURCE}" == "1" ]]; then
    if ! run_as_build_user git -C "${REPO_ROOT}" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
        echo "Repository checkout not found at ${REPO_ROOT}."
        exit 1
    fi

    if ! run_as_build_user git -C "${REPO_ROOT}" diff --quiet --ignore-submodules --; then
        echo "Working tree has uncommitted changes; refusing to pull over local modifications."
        echo "Commit or stash your changes, or rerun with UPDATE_SOURCE=0 to rebuild the current checkout."
        exit 1
    fi

    if ! run_as_build_user git -C "${REPO_ROOT}" diff --cached --quiet --ignore-submodules --; then
        echo "Index has staged changes; refusing to pull over local modifications."
        echo "Commit or stash your changes, or rerun with UPDATE_SOURCE=0 to rebuild the current checkout."
        exit 1
    fi

    echo "[1/3] Updating source checkout..."
    run_as_build_user git -C "${REPO_ROOT}" pull --ff-only
else
    echo "[1/3] Skipping source update (UPDATE_SOURCE=0)."
fi

echo "[2/3] Reinstalling Boombox..."
"${INSTALL_SCRIPT}"

restart_service() {
    if ! command -v systemctl >/dev/null 2>&1; then
        return 1
    fi

    systemctl status boombox.service >/dev/null 2>&1 || return 1
    systemctl restart boombox.service
}

case "${RESTART_SERVICE}" in
    0)
        echo "[3/3] Skipping boombox.service restart (RESTART_SERVICE=0)."
        ;;
    1)
        echo "[3/3] Restarting boombox.service..."
        if ! restart_service; then
            echo "Failed to restart boombox.service."
            exit 1
        fi
        ;;
    auto)
        if restart_service; then
            echo "[3/3] Restarted boombox.service."
        else
            echo "[3/3] boombox.service not active; no restart needed."
        fi
        ;;
    *)
        echo "Invalid RESTART_SERVICE value: ${RESTART_SERVICE}"
        echo "Use 0, 1, or auto."
        exit 1
        ;;
esac

echo "Update complete."
echo "  Binary : ${INSTALL_PREFIX}/boombox"
echo "  Symlink: ${SYMLINK_DIR}/boombox"
echo "  Desktop: ${DESKTOP_DIR}/boombox.desktop"

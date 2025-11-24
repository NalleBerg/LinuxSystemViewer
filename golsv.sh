#!/usr/bin/env bash
set -euo pipefail

# Friendly installer for the LSV .deb produced by ./makeit.sh
# - builds the package as the invoking user (even when the script is run via sudo)
# - removes any old LSV package
# - inspects the new .deb
# - copies the .deb to /tmp (avoids _apt sandbox warnings when the file lives in a user's home)
# - installs via apt (preferred) with a dpkg fallback
# - updates desktop/icon caches quietly

GREEN="\033[32m"
YELLOW="\033[33m"
RED="\033[31m"
BLUE="\033[34m"
RESET="\033[0m"
BOLD="\033[1m"
ICON_OK="✅"
ICON_WARN="⚠️"
ICON_INFO="🔧"

log() { echo -e "${BLUE}${ICON_INFO} $*${RESET}"; }
ok()  { echo -e "${GREEN}${ICON_OK} $*${RESET}"; }
warn(){ echo -e "${YELLOW}${ICON_WARN} $*${RESET}"; }
err() { echo -e "${RED}✖ $*${RESET}" >&2; }

# Determine the unprivileged user who invoked sudo (if any)
BUILD_USER="${SUDO_USER:-${USER:-$(id -un)}}"

log "Building package (./makeit.sh) as user: ${BUILD_USER}"
# Ensure we obtain fresh sudo credentials up-front (do not rely on an existing sudo grace period)
# If the script is not running as root, force a password prompt and re-exec under sudo.
if [[ $EUID -ne 0 ]]; then
	log "Requesting sudo privileges (you will be prompted for your password)..."
	sudo -k
	if ! sudo -v; then
		err "Unable to obtain sudo credentials. Aborting."
		exit 1
	fi
	# Re-exec the script under sudo so subsequent privileged operations run as root
	exec sudo -E bash "$0" "$@"
else
	# If we're already running as root via sudo, force the original user to re-auth
	# so we don't accidentally reuse a previous grace period. This uses su to run
	# sudo -v as the original user which will prompt for their password.
	if [[ -n "${SUDO_USER:-}" ]]; then
		log "Forcing re-authentication for ${SUDO_USER} to avoid sudo grace reuse..."
		su - "${SUDO_USER}" -c 'sudo -k; sudo -v' || warn "Could not force re-auth for ${SUDO_USER}; continuing as root."
	fi
fi
# Purge any existing system-installed lsv package first (user requested)
log "Purging any system-installed LSV package (apt purge)..."
sudo apt-get purge -y lsv 2>/dev/null || true
# If we're running under sudo, ensure the project build output is writable by the real user.
# This often happens when previous runs were done as root and created root-owned files
# which then prevent the normal (non-root) build step from cleaning the tree.
if [[ -n "${SUDO_USER:-}" ]]; then
	# Project directories we may need to adjust so the unprivileged build can write/clean them.
	PROJECT_DIR="$(pwd)"
	for d in build LSV; do
		if [[ -e "$PROJECT_DIR/$d" ]]; then
			log "Fixing ownership of $PROJECT_DIR/$d -> ${BUILD_USER}:${BUILD_USER}"
			sudo chown -R "${BUILD_USER}:${BUILD_USER}" "$PROJECT_DIR/$d" || true
		fi
	done

	# Run the build as the original non-root user to avoid root-owned artifacts.
	sudo -u "${BUILD_USER}" bash -lc './makeit.sh'
else
	./makeit.sh
fi

# Remove old install (if any)
log "Removing any existing LSV package (apt remove --purge)..."
sudo apt-get remove --purge -y lsv 2>/dev/null || true

# Find the produced .deb (newest under LSV/)
deb=$(ls -1t LSV/*.deb 2>/dev/null | head -n1 || true)
if [[ -z "$deb" ]]; then
	err "No .deb found under ./LSV. Did makeit.sh produce one?"
	exit 2
fi
ok "Found package: $deb"

log "Inspecting package control data (dpkg-deb -I)"
dpkg-deb -I "$deb" || true

# Copy to /tmp with safe permissions so _apt can access it during apt install.
# Using /tmp avoids the common warning about _apt not being able to read files inside home dirs
tmpdeb=$(mktemp --suffix=.deb /tmp/lsvXXXXXX)
log "Copying package to temporary location: $tmpdeb"
sudo cp -- "$deb" "$tmpdeb"
sudo chmod 644 "$tmpdeb"
sudo chown root:root "$tmpdeb"

log "Installing package via apt (preferred; resolves dependencies)"
if sudo apt install -y "$tmpdeb"; then
	ok "Installed successfully via apt."
else
	warn "apt install failed; falling back to dpkg -i + apt-get -f"
	sudo dpkg -i "$tmpdeb" || true
	sudo apt-get install -f -y
fi

log "Cleaning up temporary package file"
sudo rm -f "$tmpdeb" || true

log "Updating desktop/menu/icon caches (quiet)"
sudo update-desktop-database --quiet /usr/share/applications 2>/dev/null || true
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
	sudo gtk-update-icon-cache -f -t /usr/share/icons/hicolor >/dev/null 2>&1 || true
fi

ok "Done — LSV is installed. You can launch it from your desktop menu or run: ${BOLD}lsv${RESET}"

# Play notification sound (try multiple methods)
paplay /usr/share/sounds/LinuxMint/stereo/dialog-question.wav 2>/dev/null || \
aplay /usr/share/sounds/LinuxMint/stereo/dialog-question.wav 2>/dev/null || \
canberra-gtk-play -i complete 2>/dev/null || \
true

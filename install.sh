#!/usr/bin/env bash
set -euo pipefail


usage() {
  cat <<'USAGE'
Usage: sudo bash ./install.sh [--help]

Installs the generated .deb (preferred) or, if none exists, copies the built
binary and desktop assets into system locations. Must be run as root.

Options:
  -h, --help   Show this help message and exit
USAGE
}

echo "LSV installer helper"

for arg in "$@"; do
  case "$arg" in
    -h|--help) usage; exit 0 ;;
  esac
done

if [ "${EUID:-$(id -u)}" -ne 0 ]; then
  echo "This script must be run as root. Use: sudo bash ./install.sh" >&2
  exit 1
fi

ROOTDIR=$(pwd)
BUILD_DIR="$ROOTDIR/build"

# Don't enable shell debug tracing in normal installs — it produces noisy '+' lines
# when the script runs under sudo. Keep output minimal for GUI/CI use.

# 1) Try to install a .deb if present
DEB_FILE=$(find "$BUILD_DIR" -maxdepth 1 -type f -name 'lsv-*.deb' -print -quit 2>/dev/null || true)
if [ -n "$DEB_FILE" ]; then
  echo "Found DEB: $DEB_FILE. Installing with dpkg..."
  dpkg -i "$DEB_FILE" || true
  # try to fix deps if apt is available
  if command -v apt-get >/dev/null 2>&1; then
    apt-get -f install -y || true
  fi
  echo "DEB install finished."
  exit 0
fi

# 2) If no DEB, try to install the built binary
BIN_FILE="$BUILD_DIR/LSV"
if [ ! -f "$BIN_FILE" ]; then
  echo "No built binary found at $BIN_FILE. Trying to build with ./makeit.sh"
  if [ -x ./makeit.sh ]; then
    bash ./makeit.sh || { echo "makeit.sh failed"; exit 2; }
  else
    bash ./makeit.sh || { echo "makeit.sh failed or is not executable"; exit 2; }
  fi
fi

if [ -f "$BIN_FILE" ]; then
  echo "Installing binary to /usr/bin/LSV"
  install -Dm755 "$BIN_FILE" /usr/bin/LSV
else
  echo "Build did not produce $BIN_FILE. Aborting." >&2
  exit 3
fi

# Install desktop file if present

# Install desktop file if present
if [ -f "$ROOTDIR/lsv.desktop" ]; then
  echo "Installing desktop file to /usr/share/applications/lsv.desktop"
  install -Dm644 "$ROOTDIR/lsv.desktop" /usr/share/applications/lsv.desktop
fi

# Install icons: try AppDir hicolor layout first, then AppDir icons, then
# repo-root icon files (lsv-512.png, lsv.png). Print concise status messages
# so users see what's happening.
echo "Installing icons (if available)..."
ICON_INSTALLED=0

# Prefer AppDir hicolor layout if present
if [ -d "$ROOTDIR/AppDir/usr/share/icons/hicolor" ]; then
  echo "  - Copying AppDir hicolor icons into /usr/share/icons/hicolor/"
  rsync -a "$ROOTDIR/AppDir/usr/share/icons/hicolor/" /usr/share/icons/hicolor/ || true
  ICON_INSTALLED=1
fi

# Fallback: any AppDir icons
if [ -d "$ROOTDIR/AppDir/usr/share/icons" ]; then
  echo "  - Copying AppDir icons into /usr/share/icons/"
  rsync -a "$ROOTDIR/AppDir/usr/share/icons/" /usr/share/icons/ || true
  ICON_INSTALLED=1
fi

# Repo-root icon fallbacks (common sizes)
if [ -f "$ROOTDIR/lsv-512.png" ]; then
  echo "  - Found repo icon: lsv-512.png"
  mkdir -p /usr/share/icons/hicolor/512x512/apps
  if install -Dm644 "$ROOTDIR/lsv-512.png" /usr/share/icons/hicolor/512x512/apps/lsv.png; then
    echo "    -> installed to /usr/share/icons/hicolor/512x512/apps/lsv.png"
    chmod 644 /usr/share/icons/hicolor/512x512/apps/lsv.png || true
    ICON_INSTALLED=1
  else
    echo "    -> failed to install lsv-512.png" >&2
  fi
fi
if [ -f "$ROOTDIR/lsv.png" ]; then
  echo "  - Found repo icon: lsv.png"
  # Install to several common sizes as fallbacks
  for size in 128 64 48; do
    dst="/usr/share/icons/hicolor/${size}x${size}/apps/lsv.png"
    mkdir -p "$(dirname "$dst")"
    if install -Dm644 "$ROOTDIR/lsv.png" "$dst"; then
      echo "    -> installed to ${dst}"
      chmod 644 "$dst" || true
      ICON_INSTALLED=1
    else
      echo "    -> failed to install lsv.png -> ${dst}" >&2
    fi
  done
fi

if [ "$ICON_INSTALLED" -eq 0 ]; then
  echo "  - No icons found in AppDir or repo root; skipping icon install"
fi

# If we installed an icon under hicolor, update the system desktop file to
# point at the absolute icon path so desktop environments (Cinnamon) show the
# icon immediately without waiting for theme cache propagation.
if [ -f /usr/share/applications/lsv.desktop ]; then
  # prefer large icon if present
  if [ -f /usr/share/icons/hicolor/512x512/apps/lsv.png ]; then
    ICON_ABS=/usr/share/icons/hicolor/512x512/apps/lsv.png
  elif [ -f /usr/share/icons/hicolor/128x128/apps/lsv.png ]; then
    ICON_ABS=/usr/share/icons/hicolor/128x128/apps/lsv.png
  elif [ -f /usr/share/icons/hicolor/64x64/apps/lsv.png ]; then
    ICON_ABS=/usr/share/icons/hicolor/64x64/apps/lsv.png
  else
    ICON_ABS=""
  fi

  if [ -n "$ICON_ABS" ]; then
    echo "Updating /usr/share/applications/lsv.desktop to use absolute Icon path"
    if grep -q '^Icon=' /usr/share/applications/lsv.desktop; then
      sed -i "s|^Icon=.*|Icon=${ICON_ABS}|" /usr/share/applications/lsv.desktop || true
    else
      echo "Icon=${ICON_ABS}" >> /usr/share/applications/lsv.desktop || true
    fi
    chmod 644 /usr/share/applications/lsv.desktop || true
  fi
fi
# Refresh icon cache and report briefly
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
  if gtk-update-icon-cache -t -f /usr/share/icons/hicolor >/dev/null 2>&1; then
    echo "Icons: cache updated"
  else
    echo "Icons: cache update failed (non-fatal)" >&2
  fi
fi

# Install AppStream metadata (appdata) and screenshots if present
if [ -d "$ROOTDIR/AppDir/usr/share/metainfo" ]; then
  echo "Installing AppStream metadata (if any) to /usr/share/metainfo/"
  mkdir -p /usr/share/metainfo
  rsync -a "$ROOTDIR/AppDir/usr/share/metainfo/" /usr/share/metainfo/ || true
fi
if [ -f "$ROOTDIR/lsv.appdata.xml" ]; then
  echo "Installing $ROOTDIR/lsv.appdata.xml to /usr/share/metainfo/"
  install -Dm644 "$ROOTDIR/lsv.appdata.xml" /usr/share/metainfo/lsv.appdata.xml || true
fi

# refresh caches where possible
if command -v update-desktop-database >/dev/null 2>&1; then
  update-desktop-database /usr/share/applications >/dev/null 2>&1 || true
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
  gtk-update-icon-cache -t -f /usr/share/icons/hicolor >/dev/null 2>&1 || true
fi

# If AppStream tooling is available, request a refresh so GUI stores pick up the metadata
if command -v appstreamcli >/dev/null 2>&1; then
  echo "Refreshing AppStream metadata cache"
  # appstreamcli can be very verbose when run with debug flags. Silence output
  # to avoid overwhelming the installer logs and GUI package managers.
  appstreamcli refresh >/dev/null 2>&1 || true
fi

# SELinux: if enabled, attempt to relabel installed files so they can be used immediately
if [ -d /sys/fs/selinux ] && command -v restorecon >/dev/null 2>&1; then
  if [ "$(cat /sys/fs/selinux/enforce 2>/dev/null || echo 0)" = "1" ]; then
    echo "SELinux is enforcing: attempting restorecon on installed files"
    restorecon -Rv /usr/bin/LSV /usr/share/applications/lsv.desktop /usr/share/icons/hicolor /usr/share/metainfo || true
  fi
fi

echo "Installation finished. You can launch the app from the desktop menu or by running: /usr/bin/LSV"

exit 0

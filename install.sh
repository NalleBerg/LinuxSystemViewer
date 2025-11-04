#!/usr/bin/env bash
set -euo pipefail

echo "LSV installer helper"

if [ "${EUID:-$(id -u)}" -ne 0 ]; then
  echo "This script must be run as root. Use: sudo bash ./install.sh" >&2
  exit 1
fi

ROOTDIR=$(pwd)
BUILD_DIR="$ROOTDIR/build"

set -x

# 1) Try to install a .deb if present
DEB_FILE=$(ls "$BUILD_DIR"/lsv-*.deb 2>/dev/null | head -n1 || true)
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

# Copy icons from AppDir layout if present
if [ -d "$ROOTDIR/AppDir/usr/share/icons" ]; then
  echo "Copying AppDir icons into /usr/share/icons/hicolor/"
  rsync -a "$ROOTDIR/AppDir/usr/share/icons/" /usr/share/icons/ || true
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
  update-desktop-database /usr/share/applications || true
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
  gtk-update-icon-cache -t -f /usr/share/icons/hicolor || true
fi

# If AppStream tooling is available, request a refresh so GUI stores pick up the metadata
if command -v appstreamcli >/dev/null 2>&1; then
  echo "Refreshing AppStream metadata cache"
  appstreamcli refresh --verbose || true
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

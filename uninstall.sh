#!/usr/bin/env bash
set -euo pipefail

if [ "${EUID:-$(id -u)}" -ne 0 ]; then
  echo "This script must be run as root. Use: sudo bash ./uninstall.sh" >&2
  exit 1
fi

ROOTDIR=$(pwd)

FORCE=no
if [ "${1:-}" = "-y" ] || [ "${1:-}" = "--yes" ]; then
  FORCE=yes
fi

echo "This will remove files that were installed by install.sh (binary, desktop file, icons, and AppStream metadata)."
if [ "$FORCE" != "yes" ]; then
  read -p "Proceed? (y/N): " ans
  case "$ans" in
    [Yy]*) ;;
    *) echo "Aborted."; exit 0;;
  esac
fi

echo "Removing /usr/bin/LSV"
rm -f /usr/bin/LSV || true

echo "Removing desktop file /usr/share/applications/lsv.desktop"
rm -f /usr/share/applications/lsv.desktop || true

echo "Removing hicolor icons for lsv (if present)"
find /usr/share/icons/hicolor -type f -iname "*lsv*.png" -exec rm -f {} \; || true

echo "Removing AppStream metadata for lsv (if present)"
rm -f /usr/share/metainfo/lsv.appdata.xml || true
rm -rf /usr/share/metainfo/screenshots/lsv* || true

# refresh caches where possible
if command -v update-desktop-database >/dev/null 2>&1; then
  update-desktop-database /usr/share/applications || true
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
  gtk-update-icon-cache -t -f /usr/share/icons/hicolor || true
fi

# SELinux: attempt to relabel the parent dirs so permissions are correct
if [ -d /sys/fs/selinux ] && command -v restorecon >/dev/null 2>&1; then
  if [ "$(cat /sys/fs/selinux/enforce 2>/dev/null || echo 0)" = "1" ]; then
    echo "SELinux is enforcing: running restorecon on /usr/share/applications and /usr/share/icons/hicolor"
    restorecon -Rv /usr/share/applications /usr/share/icons/hicolor /usr/share/metainfo || true
  fi
fi

echo "Uninstall finished. If any files remain, they may have been installed by a package manager (dpkg/apt)."

exit 0

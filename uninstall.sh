#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage: sudo bash ./uninstall.sh [-y] [--help]

Removes files that `install.sh` copied into system locations. Use -y or
--yes to skip the confirmation prompt.

Options:
  -y, --yes    Do not prompt; proceed with uninstall
  -h, --help   Show this help message and exit
USAGE
}

for arg in "$@"; do
  case "$arg" in
    -h|--help) usage; exit 0 ;;
  esac
done

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

# Also remove any per-user desktop/icon copies that the package post-install
# step may have created in ~/.local/share for regular users.
getent passwd | while IFS=: read -r username _ uid _ _ homedir shell; do
  if [ -z "$uid" ] || [ "$uid" -lt 1000 ]; then
    continue
  fi
  if [ -z "$homedir" ] || [ ! -d "$homedir" ]; then
    continue
  fi
  case "$shell" in
    */nologin|*/false)
      continue
      ;;
  esac

  user_apps="$homedir/.local/share/applications"
  user_icons="$homedir/.local/share/icons/hicolor"

  if [ -f "$user_apps/lsv.desktop" ]; then
    echo "Removing per-user desktop file $user_apps/lsv.desktop"
    rm -f "$user_apps/lsv.desktop" || true
  fi

  if [ -d "$user_icons" ]; then
    echo "Removing per-user hicolor icons for $username (if present)"
    find "$user_icons" -type f -iname "*lsv*" -exec rm -f {} \; 2>/dev/null || true
  fi

  # Attempt to update the user's icon cache (best-effort, run as that user)
  if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    su -s /bin/sh -c "gtk-update-icon-cache -f -t '${user_icons}' >/dev/null 2>&1 || true" "$username" || true
  fi
done

# refresh caches where possible
if command -v update-desktop-database >/dev/null 2>&1; then
  update-desktop-database /usr/share/applications >/dev/null 2>&1 || true
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
  gtk-update-icon-cache -t -f /usr/share/icons/hicolor >/dev/null 2>&1 || true
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

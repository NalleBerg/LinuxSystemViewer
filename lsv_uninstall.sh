#!/bin/bash
set -euo pipefail

# lsv_uninstall.sh - verbose uninstaller helper for Linux System Viewer
# Usage: ./lsv_uninstall.sh [--yes] [--dry-run]
#   --yes    : do not prompt, remove everything found
#   --dry-run: show what would be removed but do not delete

DRY_RUN=0
ASSUME_YES=0
for arg in "$@"; do
    case "$arg" in
        --dry-run) DRY_RUN=1 ;;
        --yes) ASSUME_YES=1 ;;
        -h|--help) echo "Usage: $0 [--yes] [--dry-run]"; exit 0 ;;
    esac
done

logfile="/tmp/lsv-uninstall-$(date +%Y%m%d-%H%M%S).log"
exec > >(tee -a "$logfile") 2>&1

echo "LSV Uninstall helper"
echo "Log: $logfile"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
HOME_DIR="${HOME:-/home/$(whoami)}"

# candidate paths to inspect/remove
candidates=(
    "$SCRIPT_DIR/LSV"
    "$SCRIPT_DIR/lsv-x86_64.AppImage"
    "$SCRIPT_DIR/lsv.desktop"
    "$SCRIPT_DIR/AppDir"
    "$SCRIPT_DIR/priv"
    "/usr/bin/LSV"
    "/usr/local/bin/LSV"
    "/opt/LSV"
    "/usr/share/applications/lsv.desktop"
    "/usr/share/applications/LSV.desktop"
    "$HOME_DIR/.local/share/applications/lsv.desktop"
    "$HOME_DIR/.local/share/applications/LSV.desktop"
    # don't remove whole icon-theme directories; we'll search for LSV-specific icon files instead
    "/usr/share/pixmaps/lsv.png"
    "/usr/share/icons/lsv.png"
    "$HOME_DIR/.config/LSV/lsv_lang.rc"
    "/etc/LSV"
    "/usr/share/LSV"
)

# Also search for files that match LSV or lsv.desktop in common locations
echo "Scanning for existing LSV-related files and directories..."
found=()
for p in "${candidates[@]}"; do
    if [ -e "$p" ] || [ -d "$p" ]; then
        echo "  FOUND: $p"
        found+=("$p")
    fi
done

# Additional heuristic searches
# Find executables named 'LSV' in common bin dirs
while IFS= read -r f; do
    if [ -n "$f" ]; then
        echo "  FOUND: $f"
        found+=("$f")
    fi
done < <(find /usr /usr/local /opt "$HOME_DIR" -maxdepth 3 -type f -name 'LSV' -print 2>/dev/null || true)

# Find .desktop files mentioning LSV
while IFS= read -r f; do
    if [ -n "$f" ]; then
        echo "  FOUND: $f"
        found+=("$f")
    fi
done < <(grep -IlR "Linux System Viewer\|LSV" /usr/share/applications "$HOME_DIR/.local/share/applications" 2>/dev/null || true)

# Find LSV-specific icon files under common icon locations (user and system)
echo "Searching for LSV icon files in icon theme directories..."
for icon_dir in "$HOME_DIR/.local/share/icons/hicolor" "/usr/share/icons/hicolor" "/usr/share/pixmaps" "/usr/share/icons"; do
    if [ -d "$icon_dir" ]; then
        while IFS= read -r f; do
            if [ -n "$f" ]; then
                echo "  FOUND ICON: $f"
                found+=("$f")
            fi
        done < <(find "$icon_dir" -maxdepth 4 -type f -iname 'lsv*' -print 2>/dev/null || true)
    fi
done

if [ ${#found[@]} -eq 0 ]; then
    echo "No LSV files found in standard locations. Nothing to remove."
    echo "If you installed via a package manager, consider running:"
    echo "  sudo dpkg -l | grep -i lsv"
    echo "  sudo rpm -qa | grep -i lsv"
    exit 0
fi

# Confirm removal
if [ "$ASSUME_YES" -eq 0 ] && [ "$DRY_RUN" -eq 0 ]; then
    echo
    echo "The following items were found and will be removed:" 
    for p in "${found[@]}"; do
        echo "  - $p"
    done
    echo
    read -p "Proceed and remove these files/directories? [y/N]: " ans
    case "$ans" in
        y|Y|yes|Yes) ;;
        *) echo "Aborted by user."; exit 0 ;;
    esac
fi

# Remove items
for p in "${found[@]}"; do
    if [ "$DRY_RUN" -eq 1 ]; then
        echo "DRY-RUN: Would remove: $p"
        continue
    fi
    if [ -e "$p" ] || [ -d "$p" ]; then
        # safety: never remove root or common system dirs accidentally
        case "$p" in
            "/"|"/usr"|"/usr/share"|"/usr/share/icons"|"/usr/share/icons/hicolor")
                echo "Skipping dangerous path (will not remove): $p"
                continue
                ;;
        esac

        # If path is outside the user's home or the script dir, confirm per-file unless --yes
        if [[ "$p" != "$HOME_DIR"* && "$p" != "$SCRIPT_DIR"* ]]; then
            if [ "$ASSUME_YES" -eq 0 ]; then
                echo
                read -p "Path '$p' appears system-wide. Remove it? [y/N]: " resp
                case "$resp" in
                    y|Y|yes|Yes) ;;
                    *) echo "Skipping: $p"; continue ;;
                esac
            fi
        fi

        echo "Removing: $p"
        if rm -rf -- "$p"; then
            echo "  OK: removed $p"
        else
            echo "  FAILED: could not remove $p (insufficient permissions?)"
        fi
    else
        echo "Skipping (not found): $p"
    fi
done

# Extra cleanup: try to remove leftover desktop caches
if [ "$DRY_RUN" -eq 0 ]; then
    echo "Updating desktop icon caches (where applicable)..."
    if command -v update-desktop-database >/dev/null 2>&1; then
        sudo update-desktop-database || true
    fi
    if command -v gtk-update-icon-cache >/dev/null 2>&1; then
        sudo gtk-update-icon-cache -f /usr/share/icons/hicolor || true
    fi
fi

echo "Uninstall completed. Log: $logfile"

echo "If you still see the application in menus, try logging out and back in, or run:"
echo "  update-desktop-database ~/.local/share/applications"
echo "  gtk-update-icon-cache -f ~/.local/share/icons/hicolor"

exit 0

#!/bin/bash
set -euo pipefail
UDF="$HOME/.local/share/applications/lsv.desktop"
if [ ! -f "$UDF" ]; then
  echo "No user desktop file at $UDF"
  exit 1
fi
# choose best icon
ICON=""
for s in 512 256 128 64 48 32 24 22 16; do
  p="$HOME/.local/share/icons/hicolor/${s}x${s}/apps/lsv.png"
  if [ -f "$p" ]; then
    ICON="$p"
    break
  fi
done
if [ -z "$ICON" ]; then
  echo "No icon file found in ~/.local/share/icons/hicolor"
  exit 1
fi
cp -a "$UDF" "$UDF.bak.$(date +%s)"
if grep -q '^Icon=' "$UDF"; then
  sed -i "s|^Icon=.*|Icon=$ICON|" "$UDF"
else
  # insert after TryExec if present
  if grep -q '^TryExec=' "$UDF"; then
    sed -i "/^TryExec=/a Icon=$ICON" "$UDF"
  else
    echo "Icon=$ICON" >> "$UDF"
  fi
fi
chmod 644 "$UDF"
sed -n '1,200p' "$UDF"
if command -v update-desktop-database >/dev/null 2>&1; then
  update-desktop-database "$HOME/.local/share/applications" || true
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
  gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor" || true
fi
echo "Done."

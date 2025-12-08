#!/bin/bash
set -eu

echo "Step C: validate desktop file and refresh user caches (best-effort)"
if command -v desktop-file-validate >/dev/null 2>&1; then
  desktop-file-validate "$HOME/.local/share/applications/lsv.desktop" || true
else
  echo "desktop-file-validate not available"
fi

if command -v update-desktop-database >/dev/null 2>&1; then
  update-desktop-database "$HOME/.local/share/applications" || true
else
  echo "update-desktop-database not available"
fi

if command -v gtk-update-icon-cache >/dev/null 2>&1; then
  gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor" || true
else
  echo "gtk-update-icon-cache not available"
fi

echo "Installed files:"
ls -l "$HOME/.local/share/applications/lsv.desktop" || true
ls -l "$HOME/.local/share/icons/hicolor"/*/apps/lsv.* 2>/dev/null || ls -l "$HOME/.local/share/icons/hicolor/512x512/apps/lsv.png" 2>/dev/null || true

echo "User install steps complete." 

#!/bin/bash
set -eu

echo "Step A: create user dirs and copy icons (from /tmp/lsv_pkg if present, otherwise fallback to repo png)"
mkdir -p "$HOME/.local/share/applications" "$HOME/.local/share/icons/hicolor" "$HOME/.local/bin"

if [ -d /tmp/lsv_pkg/share/icons/hicolor ]; then
  echo "Copying icons from /tmp/lsv_pkg"
  cp -a /tmp/lsv_pkg/share/icons/hicolor/* "$HOME/.local/share/icons/hicolor/" || true
else
  echo "No /tmp/lsv_pkg icons, searching repo for lsv*.png"
  icon=$(find . -maxdepth 3 -type f -iname 'lsv*.png' | head -n1 || true)
  if [ -n "$icon" ]; then
    echo "Found $icon; copying to 512x512/apps"
    mkdir -p "$HOME/.local/share/icons/hicolor/512x512/apps"
    cp -f "$icon" "$HOME/.local/share/icons/hicolor/512x512/apps/lsv.png"
  else
    echo "No icon found"
  fi
fi

ls -la "$HOME/.local/share/icons/hicolor" || true

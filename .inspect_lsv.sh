#!/bin/bash
set -eu

target="$HOME/.local/share/applications/lsv.desktop"

echo "--- USER DESKTOP FILE ---"
if [ -f "$target" ]; then
  ls -l "$target"
  echo
  echo "--- CONTENTS ---"
  sed -n '1,200p' "$target" || true
else
  echo "No user desktop file found at $target"
  exit 0
fi

echo
echo "--- HIDING FLAGS ---"
grep -En 'NoDisplay|Hidden|OnlyShowIn|NotShowIn' "$target" || true

echo
echo "--- VALIDATE ---"
if command -v desktop-file-validate >/dev/null 2>&1; then
  desktop-file-validate "$target" || true
else
  echo "desktop-file-validate not installed"
fi

echo
echo "--- EXEC CHECK ---"
execline=$(awk -F= '/^Exec=/{print $2; exit}' "$target" || true)
execpath=""
if [ -n "$execline" ]; then
  # take the program part before any space/arguments
  execpath=$(printf "%s" "$execline" | awk '{print $1}')
fi
printf "Exec: <%s>\n" "$execpath"
if [ -n "$execpath" ]; then
  if [ -x "$execpath" ]; then
    echo "Exec exists and is executable"
  else
    echo "Exec missing or not executable"
  fi
else
  echo "No Exec line found"
fi

echo
echo "--- ICON INFO ---"
iconval=$(awk -F= '/^Icon=/{print $2; exit}' "$target" || true)
printf "Icon: <%s>\n" "$iconval"
ls -l "$HOME/.local/share/icons/hicolor"/*/apps/lsv.* 2>/dev/null || true

echo
echo "--- DESKTOP DB MATCHES ---"
grep -RIl "lsv" "$HOME/.local/share/applications" || true

echo
 echo "Inspection complete." 

#!/bin/bash
set -eu

echo "Step B: choose exec and install desktop file"
if [ -x "$HOME/.local/bin/LSV" ]; then
  execpath="$HOME/.local/bin/LSV"
elif [ -x "/usr/bin/LSV" ]; then
  execpath="/usr/bin/LSV"
elif [ -x ./build/LSV ]; then
  echo "Copying ./build/LSV to ~/.local/bin/LSV"
  cp ./build/LSV "$HOME/.local/bin/LSV"
  chmod 755 "$HOME/.local/bin/LSV"
  execpath="$HOME/.local/bin/LSV"
else
  execpath="/usr/bin/LSV"
fi

echo "Chosen exec: $execpath"

cat > "$HOME/.local/share/applications/lsv.desktop" <<EOF
[Desktop Entry]
Name=Linux System Viewer
Comment=Simple GUI for system & hardware info
Exec=$execpath
TryExec=$execpath
Icon=lsv
Type=Application
Categories=Utility;
Terminal=false
StartupNotify=true
EOF

chmod 644 "$HOME/.local/share/applications/lsv.desktop"
ls -l "$HOME/.local/share/applications/lsv.desktop"

#!/bin/bash
# Simple package check script for CI/local runs.
# - configures and builds the project
# - runs CPack to create .deb/.rpm
# - inspects the generated .deb to ensure desktop file and icons are packaged
# - extracts DEBIAN control scripts for manual inspection

set -euo pipefail

ROOT_DIR=$(cd "$(dirname "$0")/../.." && pwd)
BUILD_DIR="$ROOT_DIR/build"

echo "Build dir: $BUILD_DIR"
mkdir -p "$BUILD_DIR"

echo "Configuring..."
cmake -S "$ROOT_DIR" -B "$BUILD_DIR"

echo "Building and packaging..."
cmake --build "$BUILD_DIR" --target package -- -j2

DEB="$BUILD_DIR/lsv-0.7.2.deb"
if [ ! -f "$DEB" ]; then
    echo "Error: .deb not found at $DEB" >&2
    exit 2
fi

echo "Listing .deb contents (top-level)..."
dpkg-deb -c "$DEB" | sed -n '1,200p'

echo "Checking for desktop file and icons in package..."
dpkg-deb -c "$DEB" | awk '{print $6}' | grep -E '^./share/(applications|icons)/' || true

TMP_CTRL=$(mktemp -d)
dpkg-deb -e "$DEB" "$TMP_CTRL"
echo "Extracted DEBIAN control files to $TMP_CTRL"
ls -l "$TMP_CTRL"
echo "Postinst summary (first 120 lines):"
sed -n '1,120p' "$TMP_CTRL/postinst" || true

echo "Package check complete. Review output for expected files and scripts."

exit 0

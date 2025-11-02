#!/usr/bin/env bash
# Helper: build .qm files from .ts files using lrelease
# Run from project root.
set -euo pipefail
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
cd "$ROOT_DIR"

mkdir -p i18n

for TS in i18n/*.ts; do
    [ -e "$TS" ] || continue
    QM=${TS%.ts}.qm
    echo "Compiling $TS -> $QM"
    lrelease "$TS" -qm "$QM"
done

echo "Compiled .qm files are in i18n/ — re-run CMake and build to include them in the packaged artifacts (DEB/RPM) when building."

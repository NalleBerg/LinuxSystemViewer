#!/usr/bin/env bash
# Helper: generate .ts files for translators using lupdate
# Run from project root.
set -euo pipefail
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
cd "$ROOT_DIR"

# Ensure i18n directory exists
mkdir -p i18n

# Languages we ship / want translators for
LANGS=(en fr da de sv nb fi)

# Create .ts files using lupdate (skip English since it's the source)
for L in "${LANGS[@]}"; do
    if [ "$L" = "en" ]; then
        echo "Skipping English (source strings)"
        continue
    fi
    TSFILE="i18n/lsv_${L}.ts"
    echo "Generating $TSFILE"
    /usr/lib/qt6/bin/lupdate . -ts "$TSFILE"
done

echo "Created .ts files in i18n/ — open them in Qt Linguist and translate, then run scripts/build_translations.sh"

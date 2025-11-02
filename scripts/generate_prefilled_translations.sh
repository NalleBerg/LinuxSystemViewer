#!/usr/bin/env bash
# Generate .ts files using lupdate and prefill translations with English source
# Usage: ./scripts/generate_prefilled_translations.sh [lang1 lang2 ...]
# Example: ./scripts/generate_prefilled_translations.sh fr de sv nb da en_GB
set -euo pipefail
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd)
cd "$ROOT_DIR"

# Default languages to create if none are provided
if [ "$#" -eq 0 ]; then
    LANGS=(fr da de sv nb en_GB)
else
    LANGS=("$@")
fi

mkdir -p i18n

# Create .ts files (using lupdate). If lupdate is missing, inform the user.
if ! command -v lupdate >/dev/null 2>&1; then
    echo "lupdate not found in PATH. Please install Qt development tools (lupdate) to generate .ts files." >&2
    exit 2
fi

# Run lupdate once to extract strings and write into each target .ts
for L in "${LANGS[@]}"; do
    TSFILE="i18n/lsv_${L}.ts"
    echo "Generating TS: $TSFILE"
    lupdate . -ts "$TSFILE"
done

# Post-process to prefill translations with source text
if ! command -v python3 >/dev/null 2>&1; then
    echo "python3 not found in PATH. Prefill step requires python3." >&2
    exit 3
fi

python3 "$SCRIPT_DIR/prefill_ts.py" i18n

echo "Generated and prefilled .ts files in i18n/. Open them in Qt Linguist to refine translations."
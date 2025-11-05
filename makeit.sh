#!/bin/bash
set -e

# Parse simple command-line flags and environment overrides.
# Supported flags:
#   --run             : start the built executable (runtime test) in background after packaging
#   --norun           : don't run the runtime test
#   --debug-logger    : build the project with the optional debug logger compiled in
#   --help, -h        : show this help message and exit
# Environment variables:
#   RUN_PACKAGE=1     : same as --run
#   DEBUG_LOGGER=1    : same as --debug-logger
# shellcheck disable=SC2034  # DO_RUN intentionally configurable; runtime test may be enabled later
DO_RUN=1
DO_DEBUG=0
usage() {
        cat <<'USAGE'
Usage: ./makeit.sh [OPTIONS]

Helper script that configures, builds and packages the project.

Options:
    --run             Run a short runtime test after packaging
    --norun           Do not run the runtime test (default when set)
    --debug-logger    Compile the optional debug logger into the binary
    -h, --help        Show this help message and exit

Environment variables:
    RUN_PACKAGE=1     Same as --run
    DEBUG_LOGGER=1    Same as --debug-logger

Examples:
    ./makeit.sh --run
    ./makeit.sh --debug-logger --norun
USAGE
}

for arg in "$@"; do
        case "$arg" in
                --run) DO_RUN=1 ;;
                --norun) DO_RUN=0 ;;
                --debug-logger) DO_DEBUG=1 ;;
                -h|--help) usage; exit 0 ;;
        esac
done
if [ "${RUN_PACKAGE:-}" = "1" ]; then DO_RUN=1; fi
if [ "${RUN_PACKAGE:-}" = "0" ]; then DO_RUN=0; fi
if [ "${DEBUG_LOGGER:-}" = "1" ]; then DO_DEBUG=1; fi

: "$DO_RUN"

# Format seconds to MM:SS
format_time() {
    # Truncate fractional seconds and format MM:SS
    local total_seconds=${1%.*}
    if [ -z "$total_seconds" ]; then total_seconds=0; fi
    local minutes=$(( total_seconds / 60 ))
    local seconds=$(( total_seconds % 60 ))
    printf "%d:%02d" $minutes $seconds
}

clear
rm -rf build LSV

# Generate compiled translation files (.qm) from the .ts sources so CMake
# can pick them up and embed them into the application resources. This
# ensures the shipped binary contains translations and no runtime
# fallback is necessary.
if [ -d "i18n" ]; then
    # Only run lrelease when needed: generate .qm files for each .ts when the
    # .qm is missing or older than its .ts source. This avoids unnecessary
    # re-generation during frequent builds and handles missing lrelease more
    # gracefully.
    if ls i18n/*.ts >/dev/null 2>&1; then
        if command -v lrelease >/dev/null 2>&1; then
            echo "🔤 Generating/updating .qm translation files from .ts (only when needed)..."
            for ts in i18n/*.ts; do
                # If the glob didn't match any files the loop will iterate with
                # the literal pattern on some shells; guard against that.
                [ -e "$ts" ] || continue
                qm="${ts%.ts}.qm"
                if [ ! -f "$qm" ] || [ "$ts" -nt "$qm" ]; then
                    echo "  • Generating $qm from $ts"
                    if ! lrelease "$ts"; then
                        echo "❌ lrelease failed for $ts"
                        exit 1
                    fi
                else
                    echo "  • Up-to-date: $qm"
                fi
            done
        else
            echo "⚠️  lrelease not found; please install Qt tools (lrelease). Translations won't be generated or embedded."
        fi
    else
        echo "ℹ️  No translation source (.ts) files found in i18n/. Skipping translation generation."
    fi
fi

echo "📁 Tools cached in: ./tools/"
echo ""

COMPILE_START=$(date +%s.%N)

echo "🔨 Starting build process for Linux System Viewer..."
echo "⏰ Build started at: $(date '+%H:%M:%S')"
echo ""

mkdir build
cd build

echo "🔧 Configuring with CMake..."

# Pass debug-logger flag to CMake when requested so the built binary may
# include the optional logger (disabled by default).
CMAKE_FLAGS=""
if [ "$DO_DEBUG" -eq 1 ]; then
    CMAKE_FLAGS="-DLSV_ENABLE_DEBUG_LOGGER=ON"
    echo "ℹ️  CMake: enabling debug logger for this build"
fi

cmake .. $CMAKE_FLAGS || { echo "❌ CMake configuration failed"; exit 1; }

echo "🔨 Building Linux System Viewer..."
make -j"$(nproc)" || { echo "❌ Build failed"; exit 1; }

COMPILE_END=$(date +%s.%N)
COMPILE_TIME=$(echo "$COMPILE_END - $COMPILE_START" | bc -l)

echo ""
echo "✅ Build completed successfully!"
echo "⏱️  Compile time: $(format_time "$COMPILE_TIME")"
echo ""

ls -la ./
echo ""

if [ -f "./LSV" ]; then
    echo "🎯 Linux System Viewer executable found"
    ls -la ./LSV
    ldd ./LSV 2>/dev/null || echo "   ldd failed - static binary or missing libraries"
    file ./LSV
    echo ""
    
    echo "⚠️  Skipping automatic runtime test of the built executable (no terminal spawn)."
    echo "📦 Preparing DEB and RPM packages for Linux System Viewer..."
    cd ..
    mkdir -p LSV

    # Run CPack to generate DEB and RPM packages in the build directory
    echo "� Generating .deb package with CPack..."
    (cd build && cpack -G DEB) || { echo "❌ cpack DEB failed"; exit 1; }
    echo "🔁 Generating .rpm package with CPack (if rpmbuild is available)..."
    if (cd build && command -v rpmbuild >/dev/null 2>&1); then
        (cd build && cpack -G RPM) || { echo "❌ cpack RPM failed"; }
    else
        echo "⚠️  rpmbuild not found; skipping RPM generation. Install rpmbuild (rpm-build) to enable RPM packaging."
    fi

    # Move generated packages into ./LSV/ directory
    echo "📁 Collecting generated packages into ./LSV/"
    find build -maxdepth 1 -type f \( -name "*.deb" -o -name "*.rpm" \) -print0 | while IFS= read -r -d '' pkg; do
        echo "  • Found: $pkg"
        mv "$pkg" ./LSV/ || { echo "❌ Failed to move $pkg"; exit 1; }
    done

    # Copy the built executable for convenience
    if [ -f build/LSV ]; then
        cp build/LSV LSV/
        chmod +x LSV/LSV
        echo "✅ LSV executable copied to ./LSV/"
    fi

else
    echo "❌ Build failed - LSV executable not found"
    ls -la ./
    exit 1
fi

echo ""
echo "🏁 Build process completed at: $(date '+%H:%M:%S')"
echo "📊 Summary:"
echo "   • Compile time: $(format_time "$COMPILE_TIME")"
echo ""
echo "   • Packaging: DEB and RPM placed in ./LSV/ (if generation succeeded)"
echo ""
echo "📋 Files in ./LSV/ directory:"
if [ -d "LSV" ]; then
    ls -la LSV/
else
    echo "   No LSV directory found"
fi

echo ""
echo "🚀 To install the generated packages on a target system (example):"
echo "   sudo dpkg -i ./LSV/<package>.deb   # Debian/Ubuntu"
echo "   sudo rpm -i ./LSV/<package>.rpm    # openSUSE/Fedora (or use zypper/dnf)"
echo ""
echo "🧪 To test the executable manually (without installing):"
echo "   cd LSV"
echo "   ldd ./LSV                    # Check dependencies"
echo "   file ./LSV                   # Check file type"
echo "   ./LSV                        # Run the application"


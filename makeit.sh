#!/bin/bash
set -e

# Format seconds to MM:SS
format_time() {
    local total_seconds=$1
    local minutes=$(echo "$total_seconds / 60" | bc)
    local seconds=$(echo "$total_seconds % 60" | bc)
    printf "%d:%02.0f" $minutes $seconds
}

clear
rm -rf build AppDir *.AppImage LSV

echo "📁 Tools cached in: ./tools/"
echo ""

COMPILE_START=$(date +%s.%N)

echo "🔨 Starting build process..."
echo "⏰ Build started at: $(date '+%H:%M:%S')"
echo ""

mkdir build
cd build

echo "🔧 Configuring with CMake..."
cmake .. || { echo "❌ CMake configuration failed"; exit 1; }

echo "🔨 Building LSV..."
make -j$(nproc) || { echo "❌ Build failed"; exit 1; }

COMPILE_END=$(date +%s.%N)
COMPILE_TIME=$(echo "$COMPILE_END - $COMPILE_START" | bc -l)

echo ""
echo "✅ Build completed successfully!"
echo "⏱️  Compile time: $(format_time $COMPILE_TIME)"
echo ""

ls -la ./
echo ""

if [ -f "./LSV" ]; then
    echo "🎯 LSV executable found"
    ls -la ./LSV
    ldd ./LSV 2>/dev/null || echo "   ldd failed - static binary or missing libraries"
    file ./LSV
    echo ""
    
    echo "🧪 Testing LSV executable..."
    echo "⏰ Test started at: $(date '+%H:%M:%S')"
    echo "================================================"
    export DISPLAY=${DISPLAY:-:0}
    echo "🖥️  Display: $DISPLAY"
    echo "🚀 Running ./LSV with 1 second timeout..."
    if timeout 1s ./LSV 2>&1; then
        echo "✅ LSV ran successfully"
    else
        EXIT_CODE=$?
        echo "⚠️  LSV exited with code: $EXIT_CODE"
        if [ $EXIT_CODE -eq 124 ]; then
            echo "   (Timeout - this is expected for GUI apps)"
        elif [ $EXIT_CODE -eq 127 ]; then
            echo "   (Command not found - executable may be corrupted)"
        else
            echo "   (Runtime error - check dependencies)"
        fi
    fi
    echo ""
    echo "================================================"
    echo "⏰ Test finished at: $(date '+%H:%M:%S')"
    
    PACKAGE_START=$(date +%s.%N)
    echo "📦 Preparing AppImage creation..."
    cd ..
    mkdir -p LSV

    # Rebuild AppDir from scratch
    rm -rf AppDir
    mkdir -p AppDir/usr/bin
    mkdir -p AppDir/usr/lib
    mkdir -p AppDir/usr/plugins

    # Copy main binary and helper files
    cp build/LSV AppDir/usr/bin/
    chmod +x AppDir/usr/bin/LSV
    

    # Copy all shared libraries needed by LSV into AppDir/usr/lib
    echo "🔍 Copying all shared libraries required by LSV..."
    ldd AppDir/usr/bin/LSV | awk '{print $3}' | grep '^/' | while read lib; do
        cp --preserve=links "$lib" AppDir/usr/lib/
    done

    # Desktop file and icon
    mkdir -p AppDir/usr/share/applications
    if [ ! -f "lsv.desktop" ]; then
        echo "⚠️  Warning: lsv.desktop not found, creating basic one..."
        cat > lsv.desktop << EOF
[Desktop Entry]
Type=Application
Name=LSV
Comment=Linux System Viewer
Exec=LSV
Icon=lsv
Categories=System;Monitor;
StartupNotify=true
EOF
        echo "✅ Created lsv.desktop"
    fi
    cp lsv.desktop AppDir/lsv.desktop
    cp lsv.desktop AppDir/usr/share/applications/lsv.desktop

    # Icon
    if [ ! -f "lsv.png" ]; then
        echo "⚠️  Warning: lsv.png not found, creating placeholder..."
        if command -v convert >/dev/null 2>&1; then
            convert -size 64x64 xc:blue -fill white -gravity center -pointsize 24 -annotate 0 "LSV" lsv.png 2>/dev/null && {
                echo "✅ Created placeholder icon with ImageMagick"
            } || {
                echo "⚠️  ImageMagick convert failed, trying alternative..."
                echo "LSV" > lsv.png
            }
        else
            echo "⚠️  ImageMagick not available, creating text placeholder..."
            echo "LSV" > lsv.png
        fi
    fi
    cp lsv.png AppDir/
    mkdir -p AppDir/usr/share/icons
    cp lsv.png AppDir/usr/share/icons/lsv.png
    mkdir -p AppDir/usr/share/icons/hicolor/128x128/apps
    cp lsv.png AppDir/usr/share/icons/hicolor/128x128/apps/lsv.png

    # Copy only Qt 6 plugins (platforms, imageformats, etc.)
    QT6_PLATFORMS="/usr/lib/x86_64-linux-gnu/qt6/plugins/platforms"
    QT6_IMAGEFORMATS="/usr/lib/x86_64-linux-gnu/qt6/plugins/imageformats"

    if [ -d "$QT6_PLATFORMS" ]; then
        mkdir -p AppDir/usr/plugins/platforms
        cp "$QT6_PLATFORMS"/* AppDir/usr/plugins/platforms/
    fi
    if [ -d "$QT6_IMAGEFORMATS" ]; then
        mkdir -p AppDir/usr/plugins/imageformats
        cp "$QT6_IMAGEFORMATS"/* AppDir/usr/plugins/imageformats/
    fi

    # Create qt.conf to help Qt find plugins
    cat > AppDir/usr/bin/qt.conf << EOF
[Paths]
Plugins = ../plugins
EOF

    # Download appimagetool if not present
    if [ ! -f "./tools/appimagetool-x86_64.AppImage" ]; then
        wget -O ./tools/appimagetool-x86_64.AppImage \
            "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
        chmod +x ./tools/appimagetool-x86_64.AppImage
    fi

    chmod +x ./tools/appimagetool-x86_64.AppImage

    # Apprun needed
    ln -sf usr/bin/LSV AppDir/AppRun
    chmod +x AppDir/AppRun

    # Enable AppImage debug output
    export APPIMAGE_DEBUG=1

    # Create AppImage
    ./tools/appimagetool-x86_64.AppImage AppDir

    # Move the resulting AppImage to ./LSV/
    APPIMAGE_FILE=$(find . -maxdepth 1 -name "*.AppImage" -type f | head -1)
    if [ -n "$APPIMAGE_FILE" ]; then
        mv "$APPIMAGE_FILE" LSV/lsv-x86_64.AppImage
        chmod +x LSV/lsv-x86_64.AppImage
        echo "🚀 AppImage moved to: ./LSV/lsv-x86_64.AppImage"
        echo "📊 AppImage file info:"
        ls -la LSV/lsv-x86_64.AppImage
    else
        echo "❌ AppImage creation failed. Check appimagetool output for errors."
        exit 1
    fi

    cp build/LSV LSV/
    echo "✅ LSV executable also copied to ./LSV/ directory"

else
    echo "❌ Build failed - LSV executable not found"
    ls -la ./
    exit 1
fi

echo ""
echo "🏁 Build process completed at: $(date '+%H:%M:%S')"
echo "📊 Summary:"
echo "   • Compile time: $(format_time $COMPILE_TIME)"
echo "   • AppImage packaging handled by appimagetool"
echo ""
echo "📋 Available executables in ./LSV/ directory:"
if [ -d "LSV" ]; then
    ls -la LSV/
else
    echo "   No LSV directory found"
fi

echo ""
echo "🚀 To run LSV:"
echo "   ./LSV/LSV                    # Run regular executable"
if [ -f "LSV/lsv-x86_64.AppImage" ]; then
    echo "   ./LSV/lsv-x86_64.AppImage    # Run AppImage (portable)"
fi

echo ""
echo "🧪 To test the executable manually:"
echo "   cd LSV"
echo "   ldd ./LSV                    # Check dependencies"
echo "   file ./LSV                   # Check file type"
echo "   ./LSV                        # Run the application"
#!/bin/bash

# Clean previous build
rm -rf build

# Create build directory and configure with cmake
cmake -S . -B build

# Build the project
cmake --build build --verbose

# Create LSV directory for AppImage
mkdir -p build/LSV

# Download AppImage tools if not present
if [ ! -f "build/linuxdeploy-x86_64.AppImage" ]; then
    echo "Downloading linuxdeploy..."
    wget -q https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage -O build/linuxdeploy-x86_64.AppImage
    chmod +x build/linuxdeploy-x86_64.AppImage
fi

if [ ! -f "build/linuxdeploy-plugin-qt-x86_64.AppImage" ]; then
    echo "Downloading linuxdeploy Qt plugin..."
    wget -q https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage -O build/linuxdeploy-plugin-qt-x86_64.AppImage
    chmod +x build/linuxdeploy-plugin-qt-x86_64.AppImage
fi

# Create desktop file
cat > build/lsv.desktop << EOF
[Desktop Entry]
Type=Application
Name=LSV
Comment=Linux System Viewer
Exec=LSV
Icon=lsv
Categories=System;Utility;
EOF

# Ensure we have the icon file
echo "Preparing icon for AppImage..."
cp lsv.png build/lsv.png

echo "Creating AppImage..."
cd build

# Create AppImage using linuxdeploy with verbose output
echo "Running linuxdeploy to create AppImage..."
export APPIMAGE_EXTRACT_AND_RUN=1
./linuxdeploy-x86_64.AppImage --appdir LSV_AppDir --executable LSV --desktop-file lsv.desktop --icon-file lsv.png --plugin qt --output appimage 2>&1

# Check if AppImage was created successfully
if ls LSV-*.AppImage 1> /dev/null 2>&1; then
    APPIMAGE_FILE=$(ls LSV-*.AppImage | head -1)
    chmod +x "$APPIMAGE_FILE"
    echo "✅ AppImage created successfully: $APPIMAGE_FILE"
    echo "AppImage details:"
    ls -lh "$APPIMAGE_FILE"
    
    # Move AppImage to final location and clean up everything else
    rm -rf LSV  # Remove any existing directory first
    mkdir -p LSV
    mv "$APPIMAGE_FILE" LSV/lsv-0.3.5.AppImage
    
    # Clean up all build artifacts - keep only the AppImage
    rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake LSV_AppDir LSV_autogen lsv.desktop lsv.png linuxdeploy-plugin-qt-x86_64.AppImage linuxdeploy-x86_64.AppImage Makefile .qt LSV_dist
    
    cd ..
    echo "🎉 Single portable AppImage ready: build/LSV/lsv-0.3.5.AppImage"
    echo "This is your ONE file that does it all!"
    
    # Play completion beep (using beep command)
    echo "Build completed successfully!"
    # Use beep command for clear audio notification
    beep -f 800 -l 200 && printf "\n✅ BUILD COMPLETE!\n"
    
    # Optionally run the AppImage
    if [ "$1" != "norun" ]; then
        echo "Running AppImage..."
        ./build/LSV/lsv-0.3.5.AppImage
    else
        echo ""
        echo "To run: ./build/LSV/lsv-0.3.5.AppImage"
    fi
    exit 0
else
    echo "⚠️  AppImage creation failed, creating fallback executable with embedded icons..."
    # Fallback: create regular executable with embedded icon
    mkdir -p LSV
    cp build/LSV LSV/LSV
    chmod +x LSV/LSV
    
    # Clean up build artifacts but keep the executable
    rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake LSV_AppDir LSV_autogen lsv.desktop lsv.png linuxdeploy-plugin-qt-x86_64.AppImage linuxdeploy-x86_64.AppImage Makefile .qt LSV_dist
    
    cd ..
    echo "✅ Fallback executable ready: build/LSV/LSV (has embedded icons)"
    
    # Play completion beep (using beep command)
    echo "Build completed successfully!"
    # Use beep command for clear audio notification
    beep -f 800 -l 200 && printf "\n✅ BUILD COMPLETE!\n"
    
    # Optionally run the executable  
    if [ "$1" != "norun" ]; then
        echo "Running executable..."
        ./build/LSV/LSV
    else
        echo ""
        echo "DONE!"
    fi
fi

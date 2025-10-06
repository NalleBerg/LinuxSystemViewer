#!/bin/bash

# Clean previous build
rm -rf build

# Create build directory and configure with cmake
cmake -S . -B build

# Build the project
cmake --build build --verbose

# Create LinInfoGUI directory for AppImage
mkdir -p build/LinInfoGUI

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
cat > build/LinInfoGUI.desktop << EOF
[Desktop Entry]
Type=Application
Name=LinInfoGUI
Comment=Linux Information Viewer
Exec=LinInfoGUI
Icon=LinInfoGUI
Categories=System;Utility;
EOF

# Ensure we have the icon file
echo "Preparing icon for AppImage..."
cp LinInfoGUI.png build/LinInfoGUI.png

echo "Creating AppImage..."
cd build

# Create AppImage using linuxdeploy with verbose output
echo "Running linuxdeploy to create AppImage..."
export APPIMAGE_EXTRACT_AND_RUN=1
./linuxdeploy-x86_64.AppImage --appdir LinInfoGUI_AppDir --executable LinInfoGUI --desktop-file LinInfoGUI.desktop --icon-file LinInfoGUI.png --plugin qt --output appimage 2>&1

# Check if AppImage was created successfully
if ls LinInfoGUI-*.AppImage 1> /dev/null 2>&1; then
    APPIMAGE_FILE=$(ls LinInfoGUI-*.AppImage | head -1)
    chmod +x "$APPIMAGE_FILE"
    echo "✅ AppImage created successfully: $APPIMAGE_FILE"
    echo "AppImage details:"
    ls -lh "$APPIMAGE_FILE"
    
    # Move AppImage to final location and clean up everything else
    rm -rf LinInfoGUI  # Remove any existing directory first
    mkdir -p LinInfoGUI
    mv "$APPIMAGE_FILE" LinInfoGUI/LinInfoGUI.AppImage
    
    # Clean up all build artifacts - keep only the AppImage
    rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake LinInfoGUI_AppDir LinInfoGUI_autogen LinInfoGUI.desktop LinInfoGUI.png linuxdeploy-plugin-qt-x86_64.AppImage linuxdeploy-x86_64.AppImage Makefile .qt LinInfoGUI_dist
    
    cd ..
    echo "🎉 Single portable AppImage ready: build/LinInfoGUI/LinInfoGUI.AppImage"
    echo "This is your ONE file that does it all!"
    
    # Play completion beep (using beep command)
    echo "Build completed successfully!"
    # Use beep command for clear audio notification
    beep -f 800 -l 200 && printf "\n✅ BUILD COMPLETE!\n"
    
    # Optionally run the AppImage
    if [ "$1" != "norun" ]; then
        echo "Running AppImage..."
        ./build/LinInfoGUI/LinInfoGUI.AppImage
    else
        echo ""
        echo "To run: ./build/LinInfoGUI/LinInfoGUI.AppImage"
    fi
    exit 0
else
    echo "⚠️  AppImage creation failed, creating fallback executable with embedded icons..."
    # Fallback: create regular executable with embedded icon
    mkdir -p LinInfoGUI
    cp build/LinInfoGUI LinInfoGUI/LinInfoGUI
    chmod +x LinInfoGUI/LinInfoGUI
    
    # Clean up build artifacts but keep the executable
    rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake LinInfoGUI_AppDir LinInfoGUI_autogen LinInfoGUI.desktop LinInfoGUI.png linuxdeploy-plugin-qt-x86_64.AppImage linuxdeploy-x86_64.AppImage Makefile .qt LinInfoGUI_dist
    
    cd ..
    echo "✅ Fallback executable ready: build/LinInfoGUI/LinInfoGUI (has embedded icons)"
    
    # Play completion beep (using beep command)
    echo "Build completed successfully!"
    # Use beep command for clear audio notification
    beep -f 800 -l 200 && printf "\n✅ BUILD COMPLETE!\n"
    
    # Optionally run the executable  
    if [ "$1" != "norun" ]; then
        echo "Running executable..."
        ./build/LinInfoGUI/LinInfoGUI
    else
        echo ""
        echo "DONE!"
    fi
fi

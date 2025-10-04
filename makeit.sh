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

# Create a simple icon (you can replace this with a real icon later)
echo "Using LinInfoGUI icon..."
# Convert SVG to PNG if needed
if [ ! -f "LinInfoGUI.png" ]; then
    convert LinInfoGUI.svg -resize 128x128 LinInfoGUI.png
fi
cp LinInfoGUI.png build/LinInfoGUI.png

echo "Creating AppImage..."
cd build

# Create AppImage using linuxdeploy
echo "Running linuxdeploy to create AppImage..."
./linuxdeploy-x86_64.AppImage --appdir LinInfoGUI_AppDir --executable LinInfoGUI --desktop-file LinInfoGUI.desktop --icon-file LinInfoGUI.png --plugin qt --output appimage

# Check if AppImage was created successfully
if ls LinInfoGUI-*.AppImage 1> /dev/null 2>&1; then
    echo "AppImage created successfully!"
    APPIMAGE_FILE=$(ls LinInfoGUI-*.AppImage | head -1)
    chmod +x "$APPIMAGE_FILE"
    echo "AppImage details:"
    ls -lh "$APPIMAGE_FILE"
    
    # Create a convenient symlink
    rm -rf LinInfoGUI
    mkdir -p LinInfoGUI
    ln -sf "../$APPIMAGE_FILE" LinInfoGUI/LinInfoGUI.AppImage
    echo "Portable AppImage ready: build/$APPIMAGE_FILE"
else
    echo "Warning: AppImage creation may have failed, creating fallback executable"
    mkdir -p LinInfoGUI_dist
    cp LinInfoGUI LinInfoGUI_dist/LinInfoGUI
fi

cd ..

echo "Build completed successfully!"

# Check what was created
APPIMAGE_CREATED=""
REGULAR_EXECUTABLE=""

if ls build/LinInfoGUI-*.AppImage 1> /dev/null 2>&1; then
    APPIMAGE_CREATED=$(ls build/LinInfoGUI-*.AppImage | head -1)
    echo "✅ AppImage created: $APPIMAGE_CREATED"
    
    # Clean up and organize the LinInfoGUI directory to only contain the AppImage
    rm -rf build/LinInfoGUI
    mkdir -p build/LinInfoGUI
    cp "$APPIMAGE_CREATED" build/LinInfoGUI/
    # Copy icon to final directory for AppImage version too
    if [ -f "build/LinInfoGUI.png" ]; then
        cp build/LinInfoGUI.png build/LinInfoGUI/LinInfoGUI.png
        echo "✅ Icon copied to AppImage directory"
    fi
    APPIMAGE_NAME=$(basename "$APPIMAGE_CREATED")
    ORGANIZED_APPIMAGE="build/LinInfoGUI/$APPIMAGE_NAME"
    
    # Clean up all build artifacts except the LinInfoGUI directory
    echo "Cleaning up build artifacts..."
    cd build
    rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake LinInfoGUI_AppDir LinInfoGUI_autogen LinInfoGUI.desktop LinInfoGUI.png LinInfoGUI-*.AppImage linuxdeploy-plugin-qt-x86_64.AppImage linuxdeploy-x86_64.AppImage Makefile .qt
    cd ..
fi

if [ -f "build/LinInfoGUI_dist/LinInfoGUI" ]; then
    REGULAR_EXECUTABLE="build/LinInfoGUI_dist/LinInfoGUI"
    echo "✅ Regular executable: $REGULAR_EXECUTABLE"
fi

# Always clean up build artifacts and organize the LinInfoGUI directory
echo "Cleaning up build artifacts..."
cd build

# If we have an AppImage, organize it properly
if [ -n "$APPIMAGE_CREATED" ]; then
    # AppImage already organized above, just clean up artifacts
    rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake LinInfoGUI_AppDir LinInfoGUI_autogen LinInfoGUI.desktop LinInfoGUI.png LinInfoGUI-*.AppImage linuxdeploy-plugin-qt-x86_64.AppImage linuxdeploy-x86_64.AppImage Makefile .qt
else
    # No AppImage created, organize the regular executable instead
    if [ -f "LinInfoGUI" ]; then
        TEMP_EXEC=$(mktemp)
        cp LinInfoGUI "$TEMP_EXEC"  # Backup the executable
        rm -rf LinInfoGUI  # Remove any existing LinInfoGUI file/directory
        mkdir -p LinInfoGUI
        mv "$TEMP_EXEC" LinInfoGUI/LinInfoGUI
        chmod +x LinInfoGUI/LinInfoGUI
        # Copy icon to final directory
        if [ -f "LinInfoGUI.png" ]; then
            cp LinInfoGUI.png LinInfoGUI/LinInfoGUI.png
            echo "✅ Icon copied to final directory"
        fi
        echo "✅ Organized executable: build/LinInfoGUI/LinInfoGUI"
    fi
    # Clean up all build artifacts (but preserve icon in LinInfoGUI directory)
    rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake LinInfoGUI_AppDir LinInfoGUI_autogen LinInfoGUI.desktop LinInfoGUI.png linuxdeploy-plugin-qt-x86_64.AppImage linuxdeploy-x86_64.AppImage Makefile .qt LinInfoGUI_dist
fi

cd ..

# Optionally run the application
if [ "$1" != "norun" ]; then
    echo "Running LinInfoGUI..."
    if [ -n "$ORGANIZED_APPIMAGE" ]; then
        ./"$ORGANIZED_APPIMAGE"
    elif [ -n "$APPIMAGE_CREATED" ]; then
        ./"$APPIMAGE_CREATED"
    elif [ -f "build/LinInfoGUI/LinInfoGUI" ]; then
        ./build/LinInfoGUI/LinInfoGUI
    elif [ -n "$REGULAR_EXECUTABLE" ]; then
        ./"$REGULAR_EXECUTABLE"
    else
        echo "Error: No executable found!"
    fi
else
    echo ""
    echo "Build completed. Use one of the following to run:"
    if [ -n "$ORGANIZED_APPIMAGE" ]; then
        echo "  ./$ORGANIZED_APPIMAGE (Portable AppImage - works on any Linux!)"
    elif [ -n "$APPIMAGE_CREATED" ]; then
        echo "  ./$APPIMAGE_CREATED (Portable AppImage - works on any Linux!)"
    elif [ -f "build/LinInfoGUI/LinInfoGUI" ]; then
        echo "  ./build/LinInfoGUI/LinInfoGUI (Regular executable)"
    fi
    [ -n "$REGULAR_EXECUTABLE" ] && echo "  ./$REGULAR_EXECUTABLE (Regular executable)"
fi

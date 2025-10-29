# Copy all shared libraries needed by Linux System Viewer into AppDir/usr/lib
echo "🔍 Copying all shared libraries required by Linux System Viewer..."
mkdir -p AppDir/usr/lib
ldd AppDir/usr/bin/LSV | awk '{print $3}' | grep '^/' | while read lib; do
    cp --preserve=links "$lib" AppDir/usr/lib/
done
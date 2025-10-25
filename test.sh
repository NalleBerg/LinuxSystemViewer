# Copy all shared libraries needed by LSV into AppDir/usr/lib
echo "🔍 Copying all shared libraries required by LSV..."
mkdir -p AppDir/usr/lib
ldd AppDir/usr/bin/LSV | awk '{print $3}' | grep '^/' | while read lib; do
    cp --preserve=links "$lib" AppDir/usr/lib/
done
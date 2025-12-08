# Version Management for LSV

## How to Update the Version

**To change the version number, edit ONLY ONE file:**

### version.cpp
```cpp
const char* LSV_VERSION = "0.17.0";  // <-- Change this line
```

This single change will automatically update the version in all locations throughout the application.

## Files That Use the Version

### Source Files (automatically updated)
- **lsv.cpp** - Uses `LSV_VERSION` and `LSVVersionQString()`
- **about_tab.cpp** - Uses `LSV_VERSION` for the About tab title
- **version.h** - Header file with extern declaration

### Files That Are Automatically Synchronized
- **CMakeLists.txt** - Line 2: `project(LSV VERSION 0.7.2)`
  - This is automatically updated by `makeit.sh` when you build
  - The version is extracted from `version.cpp` and synced via sed command

- **tools/ci/package_check.sh** - Line 22: `DEB="$BUILD_DIR/lsv-0.7.2.deb"`
  - Update if the deb filename needs to match exactly

- **README.md** - Line 4: Screenshot filename reference
  - Update screenshot filename when creating new release screenshots

- **CHANGELOG.md** / **CHANGELOG.html** - Version history
  - Add new version entries when releasing

## Build-Generated Files (auto-updated by CMake)
These files are automatically generated during build and don't need manual editing:
- build/CPackConfig.cmake
- build/CPackSourceConfig.cmake

## Version System Architecture

```
version.cpp (definition)
    ↓
version.h (extern declaration)
    ↓
lsv.cpp, about_tab.cpp (usage)
```

The version is defined once in `version.cpp` as a C-string constant, declared as extern in `version.h`, and used throughout the application. A helper function `LSVVersionQString()` provides a Qt QString version when needed.

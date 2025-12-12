# Linux System Viewer

Official project home & binaries: https://lsv.nalle.no/

**Current version: 1.3.0**

Linux System Viewer (LSV) is a lightweight, focused Qt6-based GUI tool that presents
detailed system hardware and software information on Linux. It provides both a
user-friendly overview and technical ("geek") views of components such as CPU,
memory, storage, network, graphics, audio, and more.

**Key features:**
- Clean, read-only design: gathers information using direct system file access (no external dependencies like lshw, dmidecode, etc.)
- Two presentation modes: user-friendly summaries and detailed technical views
- Universal search functionality: comprehensive search across all geek mode tabs with regex support
- Instant loading: no external process spawning for data collection
- Packaging: DEB and RPM packages via CPack for easy distribution
- Internationalization: supports 13 languages with built-in translations
- Custom elevation dialog: PAM-based authentication without terminal dependency
- Sound testing: built-in audio hardware testing using ALSA with immediate cancellation
- Developer-friendly: optional debug logger for safe troubleshooting (disabled by default)

**Recent changes (v1.3.0):**
- Fedora/RPM logging fix: root/elevated logs go to <code>/tmp/lsv_debug.log</code>
- Unified language config: always uses <code>~/.config/LinuxSystemViewer/LSV.conf</code>
- Language selector only appears if config file is missing
- Debug logger disabled by default for release
- RPM/DEB packaging confirmed on Fedora, Ubuntu, Mint, openSUSE
- Bugfixes for elevation, config, and language persistence

**Configuration:**
- Language selection is stored in `~/.config/LSV/LSV.conf`
- "Change language" dropdown in the app title bar
- "Reset language" button reverts UI to English
- Config persists correctly even when running with elevated privileges

**Downloads:**
- Official releases and packages: https://lsv.nalle.no/
- DEB packages for Debian, Ubuntu, and Linux Mint
- RPM packages for Fedora, RHEL, and openSUSE

Quickstart — build & run (developer)
----------------------------------

> **VeryQuick guide.**
>
> While being in the root of the source files:
>
> Build and install: `./makeit.sh` -> `sudo ./install.sh`
>
> Uninstall: `sudo ./uninstall.sh`

You only need `makeit.sh` to build and package the project, and `install.sh` /
`uninstall.sh` to install or remove the built artifacts. `makeit.sh` handles
configure/build/package for both release and development (debug logger) modes.

makeit.sh options
------------------

`makeit.sh` is the canonical build-and-package helper for this repository. You don't need to run `cmake` or `make` manually — `makeit.sh` performs configure, build and packaging steps for you.

Supported command-line flags:

- `--run`       : after packaging, start the built executable as a runtime test (this is a background convenience step).
- `--norun`     : disable the runtime test (opposite of `--run`).
- `--debug-logger` : compile the optional debug logger into the binary (useful for development). When this flag is not present the logger is disabled (recommended for release builds).

Environment variable equivalents (alternate ways to set the same options):

- `RUN_PACKAGE=1` or `RUN_PACKAGE=0` — same as `--run` / `--norun`.
- `DEBUG_LOGGER=1` — same as `--debug-logger`.

Examples:

```bash
# Build+package and run a brief runtime test
./makeit.sh --run

# Build+package without running tests and enable the debug logger
./makeit.sh --debug-logger --norun

# Or set via environment
DEBUG_LOGGER=1 ./makeit.sh
```

Packaging note — where to find the built artifacts and how to install
------------------------------------------------------------------

The packaging script `./makeit.sh` builds the project and creates distribution
artifacts (DEB and a small `LSV` runtime bundle) inside the `./LSV`
directory at the repository root. After `./makeit.sh` completes you should see
one or more files under `./LSV/` such as `*.deb` and the runtime
executable/bundle named `LSV`.

To install the package you built (example for Debian/Ubuntu):

```bash
# Install the generated .deb (adjust path if your build puts files elsewhere)
sudo apt install ./LSV/*.deb

# Verify the installed executable is available
which LSV
# Should print: /usr/bin/LSV

# Run a quick check (list supported languages or RC path):
/usr/bin/LSV --list-langs
/usr/bin/LSV --rc-path
```

Manual installation (without package manager):
If you prefer not to use the package manager, you can install the built binary manually:

```bash
# After running ./makeit.sh, copy the binary and assets
sudo install -Dm755 ./LSV/LSV /usr/bin/LSV
sudo install -Dm644 ./lsv.desktop /usr/share/applications/lsv.desktop

# Copy icon (if available)
sudo install -Dm644 ./lsv-512.png /usr/share/icons/hicolor/512x512/apps/lsv.png

# Update desktop database
sudo update-desktop-database /usr/share/applications
sudo gtk-update-icon-cache -f -t /usr/share/icons/hicolor
```

Installation helpers
--------------------

The repository includes `install.sh` and `uninstall.sh` scripts that handle package installation and system integration:

**Installing:**
```bash
# Build and package (creates ./LSV/lsv-0.17.0.deb and ./LSV/lsv-0.17.0.rpm)
./makeit.sh

# Install the DEB package (Debian/Ubuntu)
sudo ./install.sh
```

**What install.sh does:**
- Installs the generated DEB package if found in `build/` directory (preferred method)
- Falls back to manual binary installation if no package exists
- Installs desktop entry to `/usr/share/applications/lsv.desktop`
- Installs hicolor icons in multiple sizes to `/usr/share/icons/hicolor/`
- Installs AppStream metadata to `/usr/share/metainfo/`
- Updates desktop and icon caches
- Handles SELinux relabeling if needed

**Uninstalling:**
```bash
# Remove all installed files
sudo ./uninstall.sh

# Or use -y to skip confirmation prompt
sudo ./uninstall.sh -y
```

**What uninstall.sh does:**
- Removes `/usr/bin/LSV` binary
- Removes desktop file and icons
- Removes AppStream metadata
- Cleans up per-user desktop/icon copies in `~/.local/share/`
- Updates desktop and icon caches

Build examples
--------------

**Default build** (creates DEB and RPM packages):
```bash
./makeit.sh
```

**Build with debug logger** (for development):
```bash
./makeit.sh --debug-logger
```

**Build without running tests:**
```bash
./makeit.sh --norun
```

**Build and run:**
```bash
./makeit.sh --run
```

The logger is disabled by default in release builds. When compiled with `--debug-logger`, 
you still need to set `LSV_DEBUG=1` environment variable at runtime to actually write logs.

Development and debugging
-------------------------

**Logging policy:**
- Release builds: NO logging, no files written by the app
- Debug builds: logging compiled in only when `LSV_ENABLE_DEBUG_LOGGER` is enabled
- Runtime logging requires environment variable `LSV_DEBUG=1` (or `true`)
- Logs written to system temp directory to avoid persistent files

**Why this design:**
The application is read-only by design and respects users' machines. It gathers 
system information without modifying anything or leaving traces unless explicitly 
requested for debugging.

**Sound files location:**
Audio test files are installed to:
- System: `/usr/share/lsv/sounds/` (package install)
- Local: `/usr/local/share/lsv/sounds/` (make install)
- Development: `../sounds/` (relative to binary)

Build requirements
------------------

**Required:**
- Qt6 (Core, Widgets, Network, Multimedia)
- CMake 3.16 or later
- C++17 compatible compiler (GCC 7+, Clang 5+)
- ALSA development libraries (`libasound2-dev` on Debian/Ubuntu)

**Optional:**
- Qt Linguist tools for translation work
- CPack for package generation (included with CMake)
- `rpmbuild` for RPM package generation
- AppStream tools for metadata validation

**Install dependencies on Debian/Ubuntu:**
```bash
sudo apt install build-essential cmake \
                 qt6-base-dev qt6-multimedia-dev \
                 libasound2-dev \
                 qt6-tools-dev qt6-tools-dev-tools
```

Contributing
------------

Bug reports, feature requests, and patches are welcome! 

**Guidelines:**
- Keep the app read-only - no persistent data writes unless for debug purposes
- Follow existing code style and architecture patterns
- Test in both normal and geek mode views
- Update translations if you modify UI strings
- Submit PRs with clear descriptions of changes

**Code structure:**
- Each tab inherits from `QWidget` (simplified from `TabWidgetBase`)
- Direct system file access (no external tool dependencies)
- Two-mode display: normal view and geek mode (detailed technical info)

Help translate LSV
------------------

We welcome translations! Current status:
- **Norwegian (Bokmål)**: 69% complete (311/452 strings)
- **British English**: Available as reference

**To contribute a translation:**

1. **Install Qt Linguist tools:**
   ```bash
   # Debian/Ubuntu (Qt6)
   sudo apt install qt6-tools-dev qt6-tools-dev-tools
   
   # Or Qt5 tools work too
   sudo apt install qttools5-dev-tools
   ```

2. **Create or update translation file:**
   ```bash
   # Extract strings to .ts file
   lupdate . -ts i18n/lsv_<langcode>.ts
   
   # Open in Qt Linguist GUI
   linguist i18n/lsv_<langcode>.ts
   ```

3. **Translate and compile:**
   - Mark translated strings as "finished" in Qt Linguist
   - Save the .ts file
   - Compile to .qm: `lrelease i18n/lsv_<langcode>.ts`

4. **Test your translation:**
   ```bash
   ./makeit.sh
   ./build/LSV
   # Select your language from the dropdown
   ```

5. **Submit:**
   - Commit both `.ts` and `.qm` files
   - Open a pull request
   - Example message: `i18n(is): add Icelandic translation`

**Translation notes:**
- Keep HTML/markup unchanged (e.g. `<br>`, `%1` placeholders)
- Technical labels should be short and consistent
- Test in both normal and geek mode views

License
-------

This project is distributed under the **GNU General Public License v2 (GPLv2)**.

Contact
-------

- **Project homepage:** https://lsv.nalle.no/
- **Source repository:** https://github.com/NalleBerg/LinuxSystemViewer
- **Bug reports:** Open an issue on GitHub
- **Email:** Contact address available on project website

---

**Version 0.17.0** — November 2024

Enjoy LSV, and thank you for respecting users' machines and privacy!


# Linux System Viewer

Official project home & binaries: https://lsv.nalle.no/

NOTE: At present we provide official DEB binaries (Debian/Ubuntu). If you need other formats we can add them later; meanwhile the repository includes helper scripts (`makeit.sh`, `install.sh`, `uninstall.sh`) to build from source and install locally.

Linux System Viewer (LSV) is a small, focused Qt6-based GUI tool that presents
detailed system hardware and software information on Linux. It provides both a
user-friendly overview and technical ("geek") views of components such as CPU,
memory, storage, network, and more.

- Clean, read-only design: the app gathers information using system utilities and
  presents it without modifying the system.
- Two presentation modes: user-friendly summaries and detailed technical views.
- Packaging: DEB via CPack for easy distribution on Debian/Ubuntu.
- Developer-friendly build options and an opt-in debug logger for safe
  troubleshooting (disabled by default in release builds).

Configuration
- Persistent language selection is stored in `~/.config/LSV/lsv_lang.rc`. The
  application will consult only this file for the saved language choice — no
  other locations are used. A "Change language" dropdown is available in the
  app title bar and a "Reset language" button removes the saved file and
  reverts the UI to English.

Screenshots and binary downloads
- Releases and packaging builds are published at: https://lsv.nalle.no/

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

If you prefer not to install a package you can also move the runtime `LSV`
bundle into place manually. After running `./makeit.sh` the `LSV` runtime can
be copied to `/usr/bin` so all users can run it from the menu or command line:

```bash
# Copy runtime to a system-wide location and make it executable
sudo cp ./LSV/LSV /usr/bin/LSV
sudo chown root:root /usr/bin/LSV
sudo chmod 0755 /usr/bin/LSV

# Now you can run it as a normal system command
/usr/bin/LSV
```

Icons & desktop installer
-------------------------

The repository now includes a small installer helper (`install.sh`) and a matching uninstaller (`uninstall.sh`) that will install the runtime, desktop entry, icons and AppStream metadata for you.

Usage (from project root):

```bash
# Build and package (creates ./LSV/*.deb and the runtime bundle)
./makeit.sh

# Install the built package and helper-installed files (run as root)
sudo ./install.sh

# Remove installed files
sudo ./uninstall.sh
```

What `install.sh` does:
- Installs the generated DEB if present (preferred), or copies the runtime bundle to `/usr/bin/LSV`.
- Installs `/usr/share/applications/lsv.desktop` and the hicolor icons under `/usr/share/icons/hicolor/` (multiple sizes when available).
- Installs the AppStream metadata to `/usr/share/metainfo/` and refreshes the system caches (best-effort).

If you prefer manual installation you can still copy `lsv.desktop` to `/usr/share/applications/` and `lsv.png` (or the sizes in `./LSV`) to `/usr/share/icons/hicolor/` and update the icon/desktop caches (requires sudo).

Developer note: to build a developer package that includes the optional logger, pass `-DLSV_ENABLE_DEBUG_LOGGER=ON` to CMake or run `./makeit.sh --debug-logger` (the logger is disabled in normal release builds).

One-liner examples
- Default (build and package for DEB):

```bash
./makeit.sh
```

- Build/package without running any runtime tests (opt-out):

```bash
./makeit.sh --norun
```

- Create a developer package that includes the debug logger (runtime still needs LSV_DEBUG=1 to write logs):

```bash
./makeit.sh --debug-logger
```

Logging policy and design
- Default (release): NO logging and no files written by the app.
- Developer/debug builds: logging is compiled in only when the CMake option
	`LSV_ENABLE_DEBUG_LOGGER` is enabled. Runtime logging still requires the
	environment variable `LSV_DEBUG=1` (or `true`) to actually write logs.
- Logs are deliberately written to the system temp dir to avoid persistent
	files in user folders or inside packaged artifacts.

Why this model
- Respect for users' machines: the application is intended to be read-only for
	normal users and will not leave traces or write files unless explicitly
	requested by developers for debugging.

Contributing
- Bug reports, feature requests and patches are welcome. If you plan to add
	functionality that writes persistent data, please discuss it first.

Help translate LSV
------------------

We welcome help translating Linux System Viewer into more languages. If you'd like to contribute translations (for example Icelandic), follow these steps:

1. Install Qt Linguist tools (lupdate / linguist / lrelease). On Debian/Ubuntu: `sudo apt install qttools5-dev-tools qttools5-dev`.
2. Update or open the `.ts` file for the language in `i18n/` (e.g. `i18n/lsv_is.ts`) using Qt Linguist, translate any unfinished entries and mark them as "finished".
3. Run `lrelease i18n/lsv_<code>.ts` to generate the binary `i18n/lsv_<code>.qm` file used at runtime.
4. Commit both the `.ts` and the generated `.qm` to a branch and open a pull request. Example commit message: `i18n(is): complete Icelandic translation`.

Notes for translators
- Keep HTML/markup inside translations unchanged (e.g. `&lt;br&gt;`, links, `%1` placeholders).
- Try to keep technical labels short and consistent with tab names (e.g. "Summary" -> "Yfirlit").
- If you want me to help with an initial draft I can provide a first-pass translation and you can refine it with Qt Linguist.

Contact
- Open a PR on GitHub or email me at the project contact address shown on the project website.

License
- This project is distributed under the GNU General Public License v2 (GPLv2).

Contact / Project home
- Homepage and downloadable binaries: https://lsv.nalle.no/

Enjoy — and thanks for keeping users' machines respected and secure.



This document explains how logging works, how to enable it for development, and how to produce release builds / packages without logging.


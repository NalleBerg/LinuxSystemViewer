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
- Live demo, releases and packaging builds are published at: https://lsv.nalle.no/

Quickstart — build & run (developer)

You only need `makeit.sh` to build and package the project, and `install.sh` /
`uninstall.sh` to install or remove the built artifacts. `makeit.sh` handles
configure/build/package for both release and development (debug logger) modes.

Examples:

```bash
# Build & package (release — recommended for releases)
./makeit.sh

# Build & package and enable the optional debug logger (development)
./makeit.sh --debug-logger

# Install the generated DEB (run as root)
sudo bash ./install.sh

# Uninstall helper-installed files
sudo bash ./uninstall.sh -y
```

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

Icons & desktop installer (coming soon)
--------------------------------------

For convenience we will provide a small shell helper that installs
icons and the desktop entry (so the application appears in the menus
for GNOME/Cinnamon/MATE/KDE). That helper script will live inside
the `./LSV` directory next to the built artifacts. For now packaging
only places the runtime bundle and packages in `./LSV`; the icon and
desktop installer script will be added in a follow-up change.

If you want to install the desktop file and icon manually right now,
copy `lsv.desktop` to `/usr/share/applications/` and `lsv.png` to a
matching location under `/usr/share/icons/hicolor/` and update the
desktop database (requires sudo). See the repository `lsv.desktop` for
the packaged desktop file we use.


If you need a developer package that includes the logger, build with
`-DLSV_ENABLE_DEBUG_LOGGER=ON` before running `./makeit.sh`, or add the
`--debug-logger` option to the packaging script (not enabled by default).

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


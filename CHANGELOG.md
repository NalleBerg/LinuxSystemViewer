# Changelog

All notable changes to this project are documented in this file.

## [0.7.0] - 2025-11-02

### Added
- User language chooser: a left-side "Change language" drop-down in the app title area that persists the chosen code to `~/.config/LSV/lsv_lang.rc`.
- Reset language button: removes the saved selection so the chooser will be shown again and the application reverts to English.
- Runtime language switching: selecting a language loads the appropriate `.qm` translator and updates the UI immediately by recreating the main tab widget so constructor-time `tr()` calls run under the new translator.
- Norwegian Bokmål translations: updated `i18n/lsv_nb.ts` (build date and translations); the project now ships only English (UK) and Norwegian Bokmål translation assets.
- Translation generation in build script: `makeit.sh` runs `lrelease i18n/*.ts` prior to configuring CMake so compiled `.qm` files are available to be embedded into resources.

### Changed
- Strict per-user language config: the application treats `~/.config/LSV/lsv_lang.rc` as the single authoritative persistent language setting (no more multiple fallbacks).
- No developer i18n fallback: runtime fallback that probed `../i18n` from the binary was removed to avoid confusing end users; translations must be embedded or installed to the application i18n directory.

### Fixed
- Immediate UI retranslation: switching languages at runtime correctly updates tab labels and tab-internal text by recreating the tab widget so translations are applied without requiring an application restart.

### Packaging / Build
- The build flow now generates compiled translation files before CMake config and embeds available `.qm` files into the Qt resources. To produce packages locally:

```bash
# from repository root
lrelease i18n/*.ts
rm -rf build
mkdir build && cd build
cmake ..
cmake --build . -j
```

### Notes & Future work
- State preservation: the current approach recreates the tab widget to ensure constructor-time strings are translated. This correctly re-translates UI but discards transient tab state (selected subviews, scroll positions, etc.). A future improvement would be to implement explicit `retranslateUi()` methods for each tab so UI strings can be updated in-place while preserving state.
- Translator availability: ensure `lrelease` (Qt l10n tool) is installed on build hosts/CI.

### Files touched (high level)
- `lsv.cpp` — language chooser UI, per-user RC helpers, translator reload and tab recreation logic
- `makeit.sh` — runs `lrelease` before CMake configure; packaging helpers
- `i18n/lsv_nb.ts` — updated Norwegian translations (build date updated)
- `CMakeLists.txt` — ensured resource embedding behavior for `i18n/*.qm`

---

## [0.6.5] - 2025-10-29

### Added
- Robust URL opening for About-tab links: added a multi-stage fallback that
  detects the default browser, uses QDesktopServices, falls back to common
  helpers (xdg-open/gio/...), and finally tries common browser binaries.
  The new `openUrlRobust()` helper logs attempts to `/tmp/lsv-about-links.log`.
- Background window raise attempts after launching a browser: `startRaiseAsync`
  and `attemptRaiseWindow` try xdotool/wmctrl searches to activate newly
  created browser windows so they become visible when AppImage is launched
  from a file manager.
- Embedded GPLv2 text and GNU icon into Qt resources (`:/gpl2.txt`,
  `:/gnu_icon.png`) so the license is available offline in the AppImage.

### Changed
- Centralized application version: added `version.h` and updated code to use
  `LSV_VERSION` / `LSVVersionQString()`; bumped the project version to
  `0.6.5` and synchronized `CMakeLists.txt` project VERSION.
- About dialog polishing: larger logo area, tightened layout, and improved
  license viewer (HTML rendering with mini-headlines and monospace body).
- Resource handling in `CMakeLists.txt`: always embed license resources and
  register optional workspace images only when present to avoid rcc failures.

### Fixed
- AppImage/browser interaction: made About links open the default browser more
  reliably when the AppImage is launched from a file manager (covers many
  environments where QDesktopServices alone failed).
- rcc parsing/build error: fixed an rcc parse failure caused by generator
  expressions being written into the .qrc by switching to a safe CMake list
  for optional resources.
- Ctrl+W and close behavior: implemented `CtrlWHandler` event filter that
  intercepts Ctrl+W / quit shortcuts and shows a minimal Quit confirmation.
  Fixes included:
  - Ctrl+W now closes the app when the user confirms (sets `confirmedQuit` and
    calls `close()`).
  - Added a heuristic (cursor-position check) to skip confirmation for
    taskbar/panel-initiated window closes.

### Packaging / Build
- The AppImage packaging will now include `:/gpl2.txt` and `:/gnu_icon.png`.
  Re-run the packaging script (e.g. `./makeit.sh --debug-logger`) to produce
  the updated AppImage containing the license.

### Notes & Future work
- Window raising currently relies on X11 helper tools (xdotool/wmctrl). Wayland
  users may not see consistent raise behavior; a desktop-portal (xdg-desktop-portal)
  OpenURI DBus fallback would be a future improvement.
- Optionally convert `gpl2.txt` rendering to Markdown (`QTextDocument::setMarkdown`)
  for nicer formatting across platforms.

### Files touched (high level)
- about_tab.cpp — `openUrlRobust()`, license viewer and GNU icon use
- ctrlw.h / ctrlw.cpp — Ctrl+W handler and close/confirmation logic
- lsv.cpp — cleanup integration, `CleaningMainWindow` close flow, CtrlWHandler instantiation
- version.h — new centralized version header
- CMakeLists.txt — bumped project version and improved resource registration

---

Suggested release steps
1. Commit changes and tag the repo: `git tag -a v0.6.5 -m "Release v0.6.5"`
2. Push and create a GitHub release using this changelog entry as the release
   notes.
3. Re-run the packaging script locally to create the AppImage and test
   About → License and link behavior.

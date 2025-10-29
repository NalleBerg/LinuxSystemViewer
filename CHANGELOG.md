# Changelog

All notable changes to this project are documented in this file.

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

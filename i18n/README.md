Translation workflow for Linux System Viewer (LSV)

Overview

This project uses Qt translations (.ts/.qm). The source strings in the code are the canonical English. Translators should edit the .ts files (Qt Linguist) and developers compile the .qm files which are then bundled with the packaged application (DEB/RPM) or included in builds.

Prerequisites

- Qt translation tools on PATH: `lupdate` and `lrelease` (usually provided by Qt development packages).

Quick workflow (developer)

1) Generate .ts skeletons

   - Minimal: ./scripts/generate_translations.sh
     (runs lupdate and creates/updates language .ts files)

   - Recommended (seeds English UK automatically):

     ./scripts/generate_prefilled_translations.sh

     This runs `lupdate` then a small prefill helper (scripts/prefill_ts.py) to copy source text into the `en_GB` TS so translators get a usable English reference immediately.

2) Translate

   - Open each `i18n/lsv_XX.ts` in Qt Linguist and translate.
   - Mark translations as 'Done' and save the .ts file.

3) Compile .qm files (developer)

   ./scripts/build_translations.sh

   This runs `lrelease` to produce binary `.qm` files into `i18n/` which the build will include.

4) Build the project and packages

   mkdir -p build && cd build
   cmake ..
   cmake --build . -- -j4

Language codes included by default

- en_GB  English (UK) — default shipped UI language
- nb     Norsk (Bokmål)

Notes

- Do not edit generated `.qm` files by hand. Edit the `.ts` files using Qt Linguist.
- Packaging includes any `i18n/*.qm` files automatically when packaging.
- Runtime language selection:
   - The application consults exactly one persistent file: `~/.config/LSV/lsv_lang.rc`.
      This file, when present, must contain the chosen language code (for
      example `en_GB` or `nb`). The application will NOT consult any other
      locations; there are no fallbacks. If the file is missing the app runs
      in English.
   - A UI language chooser is available in the app ("Change language") and
      it writes the selection to `~/.config/LSV/lsv_lang.rc`.
   - A "Reset language" button is provided in the UI which removes the
      `lsv_lang.rc` file so the chooser will be shown again and the app will
      revert to English.
   - Runtime CLI options supported: `--list-langs`, `--lang=<code>`, `--choose-lang`, and `--NO-nb`.

If you update translations, re-run `./scripts/build_translations.sh` and then rebuild the project so the new `.qm` files are picked up by the resource compiler (rcc).

# Chinese Translation Complete

## Summary
Complete Simplified Chinese (简体中文) translation has been added to LSV.

## Translation Status
- **Total strings**: 460
- **Translated**: 460 (100%)
- **Unfinished**: 0

## Key Features

### Product Name Protection
**CRITICAL**: The application name is NEVER translated:
- "Linux System Viewer" → stays "Linux System Viewer" (NOT "Linux 系统查看器")
- "LSV" → stays "LSV"

This follows standard software industry practice where product names remain in their original language (e.g., Microsoft Office is not translated to "Veslemyk Kontor" in Norwegian).

### Translation Quality
1. **Main descriptions**: All descriptive text translated to Chinese while preserving product names
2. **UI elements**: All buttons, labels, tabs, menus translated
3. **Technical terms**: Hardware/software terminology properly translated
4. **Mixed language fixed**: Previous issue where About tab had 50% Chinese / 50% English is now fully translated

### Example Translations
- About tab description: "Linux System Viewer 是一个全面的系统信息工具..." (Chinese text with English product name)
- Developer info: "开发者：Nalle Berg..." (Chinese with proper formatting)
- Security notice: "这是一个只读应用程序..." (fully translated)

## Files Modified

### Translation Files
- `i18n/lsv_zh_CN.ts` - Source translation file (460 strings)
- `i18n/lsv_zh_CN.qm` - Compiled binary translation (embedded in LSV)

### Translation Tool
- `translate_chinese.py` - Automated translation script
  - Includes Chinese translation dictionary
  - **Protects product names** from being translated
  - Fixes existing incorrect translations
  - Adds new translations for unfinished strings

### Source Files
- `lsv.cpp` - Added "zh_CN" → "简体中文" to language menu
  - Line 149-167: shippedLanguageDisplayNames()
  - Line 1529: Language chooser codes list

## Testing
To test the Chinese translation:
1. Run `./build/LSV`
2. Click "Language" in menu
3. Select "简体中文"
4. Application UI should now be in Chinese
5. Verify "Linux System Viewer" appears in English everywhere

## Build Process
```bash
# Extract translatable strings
lupdate lsv.cpp multitabs.cpp *_tab.cpp tab_widget_base.cpp gui_helpers.h -ts i18n/lsv_zh_CN.ts

# Translate strings (automated)
python3 translate_chinese.py

# Compile translation to binary
/usr/lib/qt6/bin/lrelease i18n/lsv_zh_CN.ts

# Rebuild application
cd build
cmake ..
make -j$(nproc)
```

## Translation Quality Checks
✅ All 460 strings translated
✅ Product name "Linux System Viewer" preserved in English
✅ No mixed language in UI elements
✅ Technical terms properly localized
✅ Formatted strings (%1, %2) handled correctly
✅ HTML formatting preserved in translations
✅ Special characters (Chinese) display correctly

## Language Support Summary
LSV now supports 21 languages including:
- English (default)
- Simplified Chinese (zh_CN) - **NEW!**
- Danish, German, Greek, Spanish, Basque
- Finnish, French, Icelandic, Italian, Japanese
- Korean, Norwegian, Polish, Portuguese, Russian
- Swedish, Tamil, Turkish, Ukrainian

## Notes
- Translation file automatically embedded during build via CMakeLists.txt
- Qt's translation system loads appropriate .qm file based on user selection
- Language setting persists via QSettings between application runs

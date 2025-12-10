# Simplified Chinese Translation Added to LSV

## What Was Done

### 1. Code Changes (lsv.cpp)
- Added `zh_CN` → `简体中文` to the `shippedLanguageDisplayNames()` map
- Added `zh_CN` to the GUI language chooser hardcoded list
- Chinese now appears as "简体中文" in the Language menu dropdown

### 2. Translation Files Created
- **i18n/lsv_zh_CN.ts** (89 KB) - Translation source file with 460 strings
- **i18n/lsv_zh_CN.qm** (32 KB) - Compiled translation file embedded in the app

### 3. Translation Status
- **318 strings translated** (69% complete)
- **142 strings untranslated** (31% remaining)

The 318 translated strings cover:
- All main menu items (Language, About, tabs)
- Common UI elements (Close, Copy, Save, Cancel, OK, Search, etc.)
- Hardware terms (Device, Name, Type, Model, Vendor, etc.)
- Storage terms (Size, Used, Available, Filesystem, Mount Point, etc.)
- Network terms (Interface, IP Address, MAC Address, Gateway, etc.)
- Memory, CPU, Graphics terms
- Common messages and buttons

### 4. Application Rebuilt
- LSV successfully compiled with Chinese translation support
- Chinese (简体中文) now available in Language menu
- Translation loads automatically when selected

## How to Use

1. **Run LSV**:
   ```bash
   sudo ./build/LSV
   ```

2. **Change to Chinese**:
   - Click the "Language" button (or "语言" if already in Chinese)
   - Select "简体中文" from the dropdown
   - Click OK
   - Application will restart in Simplified Chinese

3. **Verify**:
   - Tabs should show: 概览, 计算机信息, 处理器, 内存, etc.
   - Buttons should show: 关闭, 复制, 保存, etc.
   - 69% of the interface will be in Chinese

## What Remains

142 strings still need translation (mostly long descriptive text):
- Detailed hardware descriptions
- Error messages
- Multi-line help text
- Some geek mode technical details

These can be added later by:
1. Editing `i18n/lsv_zh_CN.ts` manually
2. Adding translations between `<translation></translation>` tags
3. Running: `/usr/lib/qt6/bin/lrelease i18n/lsv_zh_CN.ts`
4. Rebuilding: `cd build && make`

## Translation Quality

The translations use:
- **Standard computer terminology** used in Chinese Linux distributions
- **Simplified Chinese characters** (简体中文) - read by 1.4+ billion people
- **Professional technical terms** common in Chinese tech documentation

Examples:
- Linux System Viewer → Linux 系统查看器
- Processor → 处理器
- Memory → 内存
- Storage → 存储
- Network → 网络
- Geek Mode → 专家模式

## Files Modified

1. **lsv.cpp** - Added Chinese to language maps
2. **i18n/lsv_zh_CN.ts** - Translation source (NEW)
3. **i18n/lsv_zh_CN.qm** - Compiled translation (NEW)
4. **translate_chinese.py** - Translation helper script (NEW)

## Next Steps (Optional)

To complete the remaining 142 strings:
1. Get help from a native Chinese speaker
2. Edit `i18n/lsv_zh_CN.ts` with Qt Linguist or text editor
3. Translate strings marked with `<translation type="unfinished"></translation>`
4. Recompile translations and rebuild

The app is fully functional with 69% translation coverage!

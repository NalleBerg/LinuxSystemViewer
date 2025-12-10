#!/usr/bin/env python3
"""
Complete Chinese translation for LSV
- Keeps application name "Linux System Viewer" untranslated
- Adds remaining translations
"""

import re
import xml.etree.ElementTree as ET

# Comprehensive translation dictionary
translations = {
    # DO NOT translate application names!
    # "Linux System Viewer" stays as is
    # "LSV" stays as is
    
    # Menu & Buttons
    "About": "关于",
    "Language": "语言",
    "Choose language": "选择语言",
    "Change language...": "更改语言...",
    "Reset language": "重置语言",
    "Language:": "语言：",
    "Application": "应用程序",
    "Version": "版本",
    "Authors": "作者",
    "License": "许可证",
    "Open Web Page": "打开网页",
    "Copy URL": "复制网址",
    "Copied": "已复制",
    "The URL was copied to the clipboard!": "网址已复制到剪贴板！",
    "(Embedded license not found)": "（未找到嵌入的许可证）",
    "GNU GPL v2": "GNU GPL v2",
    
    # Language change messages
    "Language selection": "语言选择",
    "Failed to write language selection to configuration directory": "无法将语言选择写入配置目录",
    "Language changed": "语言已更改",
    "Language saved. UI updated to the selected language.": "语言已保存。界面已更新为所选语言。",
    
    # Tabs
    "Summary": "概览",
    "PC Info": "计算机信息",
    "CPU": "处理器",
    "Memory": "内存",
    "Motherboard": "主板",
    "Graphics": "显卡",
    "Screen": "显示器",
    "Storage": "存储",
    "Network": "网络",
    "Ports": "端口",
    "Peripherals": "外设",
    "Audio": "音频",
    "OS": "操作系统",
    "Desktop": "桌面环境",
    "Windowing": "窗口系统",
    
    # Common UI
    "Close": "关闭",
    "Copy": "复制",
    "Save": "保存",
    "Save to file": "保存到文件",
    "Cancel": "取消",
    "OK": "确定",
    "Yes": "是",
    "No": "否",
    "Search": "搜索",
    "Search...": "搜索...",
    "Geek Mode": "专家模式",
    "Refresh": "刷新",
    "Loading...": "加载中...",
    "Error": "错误",
    "Warning": "警告",
    "Information": "信息",
    "Success": "成功",
    "Property": "属性",
    "Value": "值",
    
    # Hardware terms
    "Computer": "计算机",
    "Device": "设备",
    "Name": "名称",
    "Type": "类型",
    "Model": "型号",
    "Vendor": "制造商",
    "Manufacturer": "制造商",
    "Product": "产品",
    "Serial": "序列号",
    "Status": "状态",
    "Active": "活动",
    "Inactive": "未活动",
    "Connected": "已连接",
    "Disconnected": "未连接",
    "Enabled": "已启用",
    "Disabled": "已禁用",
    
    # Storage
    "Size": "大小",
    "Used": "已用",
    "Available": "可用",
    "Use%": "使用率",
    "Mount Point": "挂载点",
    "Filesystem": "文件系统",
    "(unmounted)": "（未挂载）",
    "Disk": "磁盘",
    "Partition": "分区",
    "Total": "总计",
    "Storage Devices": "存储设备",
    "Total Storage": "总存储",
    
    # Memory
    "Total Memory": "总内存",
    "Free": "空闲",
    "Cached": "缓存",
    "Buffers": "缓冲区",
    "Swap": "交换空间",
    "System Uptime": "系统运行时间",
    "Kernel Version": "内核版本",
    
    # Processor
    "Cores": "核心",
    "Threads": "线程",
    "Frequency": "频率",
    "Cache": "缓存",
    "Temperature": "温度",
    "Architecture": "架构",
    "Processor": "处理器",
    
    # Graphics
    "Resolution": "分辨率",
    "Refresh Rate": "刷新率",
    "Driver": "驱动程序",
    "OpenGL": "OpenGL",
    "Graphics Cards": "显卡",
    "Display Resolution": "显示分辨率",
    "Display": "显示器",
    "Screens": "显示器",
    
    # Network
    "Interface": "接口",
    "IP Address": "IP 地址",
    "MAC Address": "MAC 地址",
    "Gateway": "网关",
    "Netmask": "子网掩码",
    "Broadcast": "广播",
    "Speed": "速度",
    "Link": "链路",
    "Up": "启动",
    "Down": "关闭",
    "Ethernet Interfaces": "以太网接口",
    "Wireless Interfaces": "无线接口",
    "USB Devices": "USB 设备",
    "Operating System": "操作系统",
    
    # Common messages
    "Loading system information...": "正在加载系统信息...",
    "Failed to load": "加载失败",
    "No data available": "无可用数据",
    "Not available": "不可用",
    "Unknown": "未知",
    "None": "无",
    "N/A": "不适用",
    
    # Buttons & Actions
    "Test": "测试",
    "Stop": "停止",
    "Start": "开始",
    "Pause": "暂停",
    "Resume": "继续",
    "Clear": "清除",
    "Export": "导出",
    "Import": "导入",
    "Settings": "设置",
    "Help": "帮助",
    "Quit": "退出",
    
    # File operations
    "Choose file": "选择文件",
    "File saved": "文件已保存",
    "Save failed": "保存失败",
    "CSV Files": "CSV 文件",
    "Text Files": "文本文件",
    "All Files": "所有文件",
    "Saving...": "保存中...",
    "Saved": "已保存",
    
    # Search
    "Search in Results": "在结果中搜索",
    "Case Sensitive": "区分大小写",
    "Exact Match": "精确匹配",
    "Contains": "包含",
    "Regular Expression": "正则表达式",
    "No matches found": "未找到匹配项",
    "matches found": "找到匹配项",
    "Match Case": "匹配大小写",
    "Use Regular Expressions": "使用正则表达式",
    
    # System info
    "System Information": "系统信息",
    "Hardware Information": "硬件信息",
    "Software Information": "软件信息",
    "Click to copy": "点击复制",
    "Right-click to copy": "右键点击复制",
    "No hardware detected": "未检测到硬件",
    "Detection failed": "检测失败",
    
    # Descriptions - keep technical but translate
    "Build Date": "构建日期",
    "Qt Version": "Qt 版本",
    "Platform": "平台",
    "Developer": "开发者",
    "Web page": "网页",
    "Built with": "构建工具",
    "for optimal performance and cross-platform compatibility": "以实现最佳性能和跨平台兼容性",
    "Special thanks to": "特别感谢",
    "the open-source community": "开源社区",
    
    # Messages
    "This is a read only application": "这是一个只读应用程序",
    "For security reasons this app will not do anything to your disk nor start any applications": "出于安全原因，此应用不会对您的磁盘进行任何操作，也不会启动任何应用程序",
    "However click below to copy the URL": "但是您可以点击下方复制网址",
    "to the clipboard": "到剪贴板",
    
    # Time units
    "days": "天",
    "hours": "小时",
    "minutes": "分钟",
    "seconds": "秒",
    
    # Common computer terms that should be translated
    "audio": "音频",
    "video": "视频",
    "input": "输入",
    "output": "输出",
    "controller": "控制器",
    "adapter": "适配器",
    "card": "卡",
    "port": "端口",
    "hub": "集线器",
    "bluetooth": "蓝牙",
}

def translate_text(text):
    """Translate English text to Simplified Chinese, preserving app name"""
    if not text or not text.strip():
        return None
    
    # NEVER translate the application name
    if text == "Linux System Viewer":
        return None  # Keep original
    if text == "LSV":
        return None  # Keep original
    
    # Check if text contains app name - don't translate those strings
    if "Linux System Viewer" in text:
        # Translate the parts around it but keep the app name
        # This is complex, so for now skip these strings
        return None
    
    # Direct match
    if text in translations:
        return translations[text]
    
    # Handle formatted strings with %1, %2, etc
    if "%" in text and any(c.isdigit() for c in text):
        result = text
        for eng, chi in translations.items():
            if eng in text and eng != text:
                result = result.replace(eng, chi)
        if result != text:
            return result
    
    # Check for partial matches (case insensitive)
    text_lower = text.lower()
    for eng, chi in translations.items():
        if eng.lower() in text_lower and eng != text:
            # Do case-preserving replacement
            pattern = re.compile(re.escape(eng), re.IGNORECASE)
            result = pattern.sub(chi, text)
            if result != text:
                return result
    
    return None

def process_ts_file(filepath):
    """Process the .ts file and add Chinese translations"""
    tree = ET.parse(filepath)
    root = tree.getroot()
    
    translated_count = 0
    skipped_count = 0
    total_count = 0
    
    for message in root.findall('.//message'):
        total_count += 1
        source = message.find('source')
        translation = message.find('translation')
        
        if source is not None and translation is not None:
            source_text = source.text
            if source_text:
                # Skip if already translated
                if translation.get('type') != 'unfinished':
                    if translation.text:
                        translated_count += 1
                    continue
                
                # Try to translate
                chinese = translate_text(source_text)
                if chinese:
                    translation.text = chinese
                    if 'type' in translation.attrib:
                        del translation.attrib['type']  # Remove "unfinished"
                    translated_count += 1
                else:
                    # Keep app name and complex strings unfinished
                    skipped_count += 1
    
    # Write back to file with proper formatting
    ET.indent(tree, space='    ')
    tree.write(filepath, encoding='utf-8', xml_declaration=True)
    
    print(f"Translation complete:")
    print(f"  Translated: {translated_count} strings")
    print(f"  Remaining: {total_count - translated_count} strings")
    print(f"  (Intentionally kept {skipped_count} app names/complex strings in English)")
    
    return translated_count, total_count

if __name__ == '__main__':
    filepath = 'i18n/lsv_zh_CN.ts'
    print(f"Processing {filepath}...")
    print("Note: 'Linux System Viewer' and 'LSV' will NOT be translated")
    print()
    process_ts_file(filepath)
    print("\nDone! Recompile with: /usr/lib/qt6/bin/lrelease i18n/lsv_zh_CN.ts")

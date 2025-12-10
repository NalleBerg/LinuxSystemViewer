#!/usr/bin/env python3
"""
Translate LSV strings to Simplified Chinese
This script adds quality technical translations to lsv_zh_CN.ts

IMPORTANT: "Linux System Viewer" and "LSV" are NEVER translated - they stay in English!
"""

import re
import xml.etree.ElementTree as ET

# Translation dictionary for common LSV terms
translations = {
    # Application & Menu
    "About": "关于",
    "Language": "语言",
    "Choose language": "选择语言",
    "Change language...": "更改语言...",
    "Reset language": "重置语言",
    # NOTE: "Linux System Viewer" is NEVER translated - product name stays in English!
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
    
    # Long descriptions - translate around "Linux System Viewer"
    "Linux System Viewer is a comprehensive system information tool designed to provide detailed insights into your Linux system hardware and software configuration.\n\nLinux System Viewer presents system information in an intuitive, easy-to-read format with both user-friendly and technical (geek mode) views for different levels of detail.": 
    "Linux System Viewer 是一个全面的系统信息工具，旨在提供有关您的 Linux 系统硬件和软件配置的详细信息。\n\nLinux System Viewer 以直观、易读的格式呈现系统信息，提供用户友好和技术性（专家模式）视图以满足不同的详细程度需求。",
    
    "This is a read only application. For security reasons this app will not do anything to your disk nor start any applications.\n\nHowever click below to copy the URL https://lsv.nalle.no/ to the clipboard.":
    "这是一个只读应用程序。出于安全原因，此应用不会对您的磁盘执行任何操作，也不会启动任何应用程序。\n\n但是，您可以点击下方按钮将 URL https://lsv.nalle.no/ 复制到剪贴板。",
    
    "Developer: Nalle Berg<br><a href=\"https://lsv.nalle.no/\">Web page</a><br><br>Built with Qt6 and modern C++ for optimal performance and cross-platform compatibility.<br><br>Special thanks to the open-source community and the developers of lshw, lscpu, and other system utilities that inspired me to create this application.":
    "开发者：Nalle Berg<br><a href=\"https://lsv.nalle.no/\">网页</a><br><br>使用 Qt6 和现代 C++ 构建，以实现最佳性能和跨平台兼容性。<br><br>特别感谢开源社区以及 lshw、lscpu 和其他系统实用程序的开发者，他们激励我创建了这个应用程序。",
    
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
    "Cancel": "取消",
    "OK": "确定",
    "Yes": "是",
    "No": "否",
    "Search": "搜索",
    "Geek Mode": "专家模式",
    "Refresh": "刷新",
    "Loading...": "加载中...",
    "Error": "错误",
    "Warning": "警告",
    "Information": "信息",
    "Success": "成功",
    
    # Hardware terms
    "Device": "设备",
    "Name": "名称",
    "Type": "类型",
    "Model": "型号",
    "Vendor": "制造商",
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
    
    # Memory
    "Total Memory": "总内存",
    "Free": "空闲",
    "Cached": "缓存",
    "Buffers": "缓冲区",
    "Swap": "交换空间",
    
    # Processor
    "Cores": "核心",
    "Threads": "线程",
    "Frequency": "频率",
    "Cache": "缓存",
    "Temperature": "温度",
    "Architecture": "架构",
    
    # Graphics
    "Resolution": "分辨率",
    "Refresh Rate": "刷新率",
    "Driver": "驱动程序",
    "OpenGL": "OpenGL",
    "Vendor": "制造商",
    
    # Common messages
    "Loading system information...": "正在加载系统信息...",
    "Failed to load": "加载失败",
    "No data available": "无可用数据",
    "Not available": "不可用",
    "Unknown": "未知",
    "None": "无",
    
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
    "Save to file": "保存到文件",
    "Choose file": "选择文件",
    "File saved": "文件已保存",
    "Save failed": "保存失败",
    "CSV Files": "CSV 文件",
    "Text Files": "文本文件",
    "All Files": "所有文件",
    
    # Search
    "Search...": "搜索...",
    "Search in Results": "在结果中搜索",
    "Case Sensitive": "区分大小写",
    "Exact Match": "精确匹配",
    "Contains": "包含",
    "Regular Expression": "正则表达式",
    "No matches found": "未找到匹配项",
    "matches found": "找到匹配项",
    
    # Common phrases
    "System Information": "系统信息",
    "Hardware Information": "硬件信息",
    "Software Information": "软件信息",
    "Click to copy": "点击复制",
    "Right-click to copy": "右键点击复制",
    "No hardware detected": "未检测到硬件",
    "Detection failed": "检测失败",
    
    # Additional unfinished strings
    "Rescan": "重新扫描",
    "Rescanning, please wait...": "正在重新扫描，请稍候...",
    "Loading, please wait...": "正在加载，请稍候...",
    "Current freq (GHz)": "当前频率（GHz）",
    "Physical packages": "物理封装",
    "Unique core ids seen (per-logical sample)": "唯一核心 ID（每逻辑采样）",
    "Per-core current frequencies (kHz)": "每核心当前频率（kHz）",
    "uevent": "uevent",
    "DRI Debug Dir": "DRI 调试目录",
    "DRI Clients": "DRI 客户端",
    "Connector": "连接器",
    "EDID (hex)": "EDID（十六进制）",
    "RAM Technical Details": "内存技术细节",
    "/proc/meminfo": "/proc/meminfo",
    "Raw DMI data too short": "原始 DMI 数据过短",
    "PC Technical Details": "计算机技术细节",
    "Manufacture Date": "制造日期",
    
    # Version strings with placeholders
    "Version: %1\nBuild Date: November 2025\nQt Version: %2\nPlatform: Linux":
    "版本：%1\n构建日期：2025 年 11 月\nQt 版本：%2\n平台：Linux",
    
    # Additional technical terms
    "AMDGPU Clock Levels (%1)": "AMDGPU 时钟级别（%1）",
    "%1 (%2)": "%1（%2）",
    "NVIDIA Power State (%1)": "NVIDIA 电源状态（%1）",
    "Slot %1 (DMI entry %2)": "插槽 %1（DMI 条目 %2）",
    "Slot %1 - String %2": "插槽 %1 - 字符串 %2",
    "Slot %1 - formatted bytes (hex)": "插槽 %1 - 格式化字节（十六进制）",
    "=== CHIPSET & BRIDGES ===": "=== 芯片组和桥接器 ===",
    "=== MACHINE ID ===": "=== 机器 ID ===",
    "%1/dpms": "%1/dpms",
    "%1/modes": "%1/modes",
    "%1/EDID (first 128 bytes hex)": "%1/EDID（前 128 字节十六进制）",
    "RAM Usage": "内存使用",
    "Distribution ID": "发行版 ID",
    "Home Page": "主页",
    "Privacy Policy": "隐私政策",
    "Build ID": "构建 ID",
    "Variant": "变体",
    "Variant ID": "变体 ID",
    "SKU Number": "SKU 编号",
    "UUID": "UUID",
    "Mouse/Pointing": "鼠标/指针",
    "Keyboard": "键盘",
    "Camera": "摄像头",
    "Printer (USB)": "打印机（USB）",
    "Scanner": "扫描仪",
    "Drawing Tablet": "绘图板",
    "Wireless Receiver": "无线接收器",
    "Authentication failed (%1/3)": "身份验证失败（%1/3）",
    "Elevation failed": "提权失败",
    "MultiRowTabWidget: Initialized": "MultiRowTabWidget：已初始化",
    "MultiRowTabWidget: Added tab": "MultiRowTabWidget：已添加标签页",
    "MultiRowTabWidget: Arranging": "MultiRowTabWidget：正在排列",
    "tabs in": "标签页在",
    "rows with": "行中，每行有",
    "tabs per row": "个标签页",
    "MultiRowTabWidget: Tab area height set to:": "MultiRowTabWidget：标签区域高度设置为：",
    "Zoom Level": "缩放级别",
    "Orientation": "方向",
    "Session": "会话",
    "Window Manager": "窗口管理器",
    
    # Complete compound terms - NO PARTIAL MATCHING!
    "Computer Name": "计算机名称",
    "PC Type": "计算机类型",
    "Product Name": "产品名称",
    "Product Family": "产品系列",
    "Serial Number": "序列号",
    "Manufacturer": "制造商",
    "Full Name": "全名",
    "Property": "属性",
    "Value": "值",
    
    # Network terms
    "Interface Name": "接口名称",
    "MAC Address": "MAC 地址",
    "IPv4 Address": "IPv4 地址",
    "IPv6 Address": "IPv6 地址",
    "Default Gateway": "默认网关",
    "Ethernet Interfaces": "以太网接口",
    "Ethernet Ports": "以太网端口",
    "Wireless Interfaces": "无线接口",
    
    # Audio terms
    "Audio Device": "音频设备",
    "Audio Server": "音频服务器",
    "Audio System Technical Details": "音频系统技术细节",
    "Audio Test": "音频测试",
    "Default Input": "默认输入",
    "Default Output": "默认输出",
    "Playback Device %1": "播放设备 %1",
    "Sound Card %1": "声卡 %1",
    "Save Audio Info": "保存音频信息",
    
    # CPU terms
    "CPU Cores": "处理器核心",
    "CPU Technical Details": "处理器技术细节",
    "Logical CPUs": "逻辑处理器",
    "Save CPU Info": "保存处理器信息",
    
    # Graphics terms
    "Graphics Cards": "显卡",
    "Graphics Technical Details": "显卡技术细节",
    "Device Class": "设备类别",
    "Current Driver": "当前驱动程序",
    "Framebuffer Device": "帧缓冲设备",
    "Backlight Device": "背光设备",
    "Save Graphics Info": "保存显卡信息",
    "External Display": "外接显示器",
    "Current Resolution": "当前分辨率",
    "Display Resolution": "显示分辨率",
    "Display Scaling": "显示缩放",
    "Display Server": "显示服务器",
    
    # Memory terms
    "Memory Resources": "内存资源",
    "Save Memory Info": "保存内存信息",
    
    # Motherboard terms
    "Motherboard Technical Details": "主板技术细节",
    "Save Motherboard Info": "保存主板信息",
    
    # OS terms
    "OS Technical Details": "操作系统技术细节",
    "Save OS Info": "保存操作系统信息",
    "Distribution Name": "发行版名称",
    "Distribution Version": "发行版版本",
    "Desktop Environment": "桌面环境",
    "Desktop Environment Technical Details": "桌面环境技术细节",
    "Save Desktop Info": "保存桌面信息",
    
    # PC terms
    "Save PC Info": "保存计算机信息",
    
    # Peripherals terms
    "Peripherals Technical Details": "外设技术细节",
    "Save Peripherals Info": "保存外设信息",
    "Device Name": "设备名称",
    "Device Type": "设备类型",
    "Bluetooth Adapter": "蓝牙适配器",
    "Bluetooth Adapters": "蓝牙适配器",
    "Game Controller": "游戏控制器",
    "Card Reader": "读卡器",
    
    # Ports terms
    "Ports Technical Details": "端口技术细节",
    "Save Ports Info": "保存端口信息",
    
    # Screen terms
    "Screen Technical Details": "显示器技术细节",
    "Save Screen Info": "保存显示器信息",
    "Screen %1 Serial Number": "显示器 %1 序列号",
    
    # Storage terms
    "Storage Technical Details": "存储技术细节",
    "Save Storage Info": "保存存储信息",
    "Disk Technical Details": "磁盘技术细节",
    
    # Summary terms
    "Save Summary Info": "保存概览信息",
    
    # Windowing terms
    "Windowing System Technical Details": "窗口系统技术细节",
    "Save Windowing Info": "保存窗口系统信息",
    
    # Network geek terms
    "Network Technical Details": "网络技术细节",
    "Save Network Info": "保存网络信息",
    
    # About terms
    "About Linux System Viewer": "关于 Linux System Viewer",
    "Bug Report Page": "错误报告页面",
    
    # Placeholder terms
    "%1/Display Name": "%1/显示名称",
    "%1/Display String": "%1/显示字符串",
    "%1/Product Code": "%1/产品代码",
    "%1/Serial Number": "%1/序列号",
}

def translate_text(text):
    """Translate English text to Simplified Chinese
    
    CRITICAL: Never translate product names!
    - "Linux System Viewer" stays in English
    - "LSV" stays in English
    """
    if not text:
        return None
    
    # Direct match
    if text in translations:
        return translations[text]
    
    # NEVER translate these product names
    protected_terms = ["Linux System Viewer", "LSV"]
    
    # Check if this is just a product name - don't translate it!
    for term in protected_terms:
        if text.strip() == term:
            return None  # Keep original
    
    # Handle formatted strings
    if "%1" in text or "%2" in text or "%3" in text:
        # Try to translate the base text
        for eng, chi in translations.items():
            if eng in text:
                result = text.replace(eng, chi)
                return result
    
    # DO NOT do partial matches - they create mixed language!
    # Only exact matches from the dictionary
    
    # Return original if no translation found
    return None

def process_ts_file(filepath):
    """Process the .ts file and add Chinese translations"""
    tree = ET.parse(filepath)
    root = tree.getroot()
    
    translated_count = 0
    fixed_count = 0
    total_count = 0
    
    for message in root.findall('.//message'):
        total_count += 1
        source = message.find('source')
        translation = message.find('translation')
        
        if source is not None and translation is not None:
            source_text = source.text
            
            # Fix incorrect translations of product names
            if source_text and translation.text:
                # If source is exactly "Linux System Viewer", translation should be same
                if source_text.strip() == "Linux System Viewer":
                    if translation.text != source_text:
                        translation.text = source_text
                        if 'type' in translation.attrib:
                            del translation.attrib['type']
                        fixed_count += 1
                        print(f"Fixed: Removed translation of product name 'Linux System Viewer'")
                
                # Fix mixed language - replace "Linux 系统查看器" back to "Linux System Viewer"
                elif "Linux 系统查看器" in translation.text:
                    translation.text = translation.text.replace("Linux 系统查看器", "Linux System Viewer")
                    if 'type' in translation.attrib:
                        del translation.attrib['type']
                    fixed_count += 1
                    print(f"Fixed: Corrected mixed language - restored 'Linux System Viewer' in English")
            
            # Translate unfinished strings
            if source_text and translation.get('type') == 'unfinished':
                chinese = translate_text(source_text)
                if chinese:
                    translation.text = chinese
                    del translation.attrib['type']  # Remove "unfinished" attribute
                    translated_count += 1
    
    # Write back to file
    tree.write(filepath, encoding='utf-8', xml_declaration=True)
    
    print(f"\nTranslated {translated_count} new strings")
    print(f"Fixed {fixed_count} incorrect product name translations")
    print(f"Total processed: {total_count} strings")
    print(f"Remaining unfinished: {total_count - translated_count - fixed_count} strings")
    
    return translated_count, total_count

if __name__ == '__main__':
    filepath = 'i18n/lsv_zh_CN.ts'
    print(f"Processing {filepath}...")
    process_ts_file(filepath)
    print("Done!")

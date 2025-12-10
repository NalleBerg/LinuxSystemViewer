#!/usr/bin/env python3
"""
Complete all remaining Chinese translations
"""

import xml.etree.ElementTree as ET

# Complete translation dictionary
translations = {
    # Dialog buttons
    "Quit?": "退出？",
    "No": "否",
    "Yes": "是",
    "Close": "关闭",
    "Copy": "复制",
    "Save...": "保存...",
    "Copied": "已复制",
    "Error": "错误",
    
    # Geek Search
    "Geek Mode Search": "专家模式搜索",
    "Enter search term...": "输入搜索词...",
    "Search": "搜索",
    "Search Type": "搜索类型",
    "Contains": "包含",
    "Exact match": "精确匹配",
    "Case Sensitivity": "区分大小写",
    "Case insensitive": "不区分大小写",
    "Case sensitive": "区分大小写",
    "Search Scope": "搜索范围",
    "New search": "新搜索",
    "Search in results": "在结果中搜索",
    "Advanced": "高级",
    "Regular Expression (Regex)": "正则表达式",
    "Tab": "标签页",
    "Section": "部分",
    "Property": "属性",
    "Value": "值",
    "Context": "上下文",
    "Enter regular expression... (e.g., 'Intel.*WiFi')": "输入正则表达式...（例如：'Intel.*WiFi'）",
    "Result Details": "结果详情",
    "Search Geek mode data": "搜索专家模式数据",
    
    # Network
    "Network - Geek Mode": "网络 - 专家模式",
    "Network Technical Details": "网络技术细节",
    "The information has been copied": "信息已复制",
    "Save network info": "保存网络信息",
    "CSV files (*.csv);;All files (*)": "CSV 文件 (*.csv);;所有文件 (*)",
    "Interface Name": "接口名称",
    "MAC Address": "MAC 地址",
    "IPv4 Address": "IPv4 地址",
    "IPv6 Address": "IPv6 地址",
    "MTU": "MTU",
    "%1 bytes": "%1 字节",
    "Default Gateway": "默认网关",
    
    # CPU
    "Could not read /proc/cpuinfo": "无法读取 /proc/cpuinfo",
    "Total number of processors": "处理器总数",
    "Number of processor (Physical)": "物理处理器数量",
    "Vendor": "制造商",
    "Unknown": "未知",
    "Model": "型号",
    "Temperature": "温度",
    "Cache size": "缓存大小",
    "Bogomips": "Bogomips",
    "Current freq (GHz)": "当前频率（GHz）",
    "Max freq (GHz)": "最大频率（GHz）",
    "Min Freq (GHz)": "最小频率（GHz）",
    "Error reading CPU information": "读取处理器信息时出错",
    "Unknown CPU": "未知处理器",
    
    # Graphics
    "Graphics Card": "显卡",
    "Device ID": "设备 ID",
    "Driver": "驱动程序",
    "DRI Device": "DRI 设备",
    "Framebuffer Mode": "帧缓冲模式",
    "Virtual Resolution": "虚拟分辨率",
    "Video Memory": "显存",
    "Status": "状态",
    "No graphics card detected": "未检测到显卡",
    
    # Motherboard
    "Manufacturer": "制造商",
    "Name": "名称",
    "Type": "类型",
    "Version": "版本",
    "Serial Number": "序列号",
    "BIOS Vendor": "BIOS 制造商",
    "BIOS Version": "BIOS 版本",
    "BIOS Date": "BIOS 日期",
    "Chipset": "芯片组",
}

def translate_file(filepath):
    """Add all missing translations"""
    tree = ET.parse(filepath)
    root = tree.getroot()
    
    translated_count = 0
    
    for message in root.findall('.//message'):
        source = message.find('source')
        translation = message.find('translation')
        
        if source is not None and translation is not None:
            source_text = source.text
            if source_text and translation.get('type') == 'unfinished':
                if source_text in translations:
                    translation.text = translations[source_text]
                    del translation.attrib['type']
                    translated_count += 1
                    print(f"Translated: '{source_text}' → '{translations[source_text]}'")
    
    tree.write(filepath, encoding='utf-8', xml_declaration=True)
    
    print(f"\n✅ Translated {translated_count} new strings")
    return translated_count

if __name__ == '__main__':
    filepath = 'i18n/lsv_zh_CN.ts'
    print(f"Translating remaining strings in {filepath}...\n")
    translate_file(filepath)
    print("\nDone!")

#!/usr/bin/env python3
"""
Fix mixed English/Chinese translations in lsv_zh_CN.ts
Replace all mixed translations with proper complete Chinese translations
"""

import re
import xml.etree.ElementTree as ET

# Complete translations to fix mixed language
fix_map = {
    # PC Info tab
    "Computer 名称": "计算机名称",
    "PC 类型": "计算机类型",
    "Product 名称": "产品名称",
    "产品 Family": "产品系列",
    "序列号 Number": "序列号",
    "SKU 编号": "SKU 编号",
    
    # Audio
    "音频 System Technical Details": "音频系统技术细节",
    "Save 音频 Info": "保存音频信息",
    "音频 Test": "音频测试",
    "音频 Server": "音频服务器",
    "Default 输出": "默认输出",
    "Default 输入": "默认输入",
    "Sound 卡 %1": "声卡 %1",
    "Playback 设备 %1": "播放设备 %1",
    
    # CPU
    "处理器 Technical Details": "处理器技术细节",
    "Save 处理器 Info": "保存处理器信息",
    "处理器 Cores": "处理器核心",
    "Logical 处理器s": "逻辑处理器",
    
    # Graphics
    "显卡 Technical Details": "显卡技术细节",
    "Save 显卡 Info": "保存显卡信息",
    "设备 Class": "设备类别",
    "Current 驱动程序": "当前驱动程序",
    "内存 Resources": "内存资源",
    "Framebuffer 设备": "帧缓冲设备",
    "Backlight 设备": "背光设备",
    "External 显示器": "外接显示器",
    "Current 分辨率": "当前分辨率",
    "显示器 分辨率": "显示分辨率",
    "显示器 Scaling": "显示缩放",
    "显示器 服务器": "显示服务器",
    
    # Memory
    "Save 内存 Info": "保存内存信息",
    
    # Motherboard
    "主板 Technical Details": "主板技术细节",
    "Save 主板 Info": "保存主板信息",
    
    # OS
    "操作系统 Technical Details": "操作系统技术细节",
    "Save 操作系统 Info": "保存操作系统信息",
    "Distribution 名称": "发行版名称",
    "Distribution 版本": "发行版版本",
    "桌面环境 Technical Details": "桌面环境技术细节",
    "Save 桌面环境 Info": "保存桌面信息",
    
    # PC
    "Save 计算机信息": "保存计算机信息",
    
    # Peripherals
    "外设 Technical Details": "外设技术细节",
    "Save 外设 Info": "保存外设信息",
    "设备 名称": "设备名称",
    "设备 类型": "设备类型",
    "Bluetooth 适配器": "蓝牙适配器",
    "Game 控制器": "游戏控制器",
    "Card 读卡器": "读卡器",
    
    # Ports
    "端口 Technical Details": "端口技术细节",
    "Save 端口 Info": "保存端口信息",
    
    # Screen
    "显示器 Technical Details": "显示器技术细节",
    "Save 显示器 Info": "保存显示器信息",
    "显示器 %1 序列号 Number": "显示器 %1 序列号",
    "Screen %1 序列号 Number": "显示器 %1 序列号",
    
    # Storage
    "存储 Technical Details": "存储技术细节",
    "Save 存储 Info": "保存存储信息",
    "Disk Technical Details": "磁盘技术细节",
    
    # Summary
    "Save 概览 Info": "保存概览信息",
    
    # Windowing
    "窗口系统 Technical Details": "窗口系统技术细节",
    "Save 窗口系统 Info": "保存窗口系统信息",
    
    # Network
    "网络 Technical Details": "网络技术细节",
    "Save 网络 Info": "保存网络信息",
    "以太网 接口": "以太网接口",
    "以太网 端口": "以太网端口",
    "Wireless 接口": "无线接口",
    
    # About
    "About Linux System Viewer": "关于 Linux System Viewer",
    "Bug Report Page": "错误报告页面",
    "Bug Re端口 Page": "错误报告页面",
    
    # More mixed translations
    "序列号 Port": "串口",
    "%1/产品 Code": "%1/产品代码",
    "Full 名称": "全名",
    "版本 Codename": "版本代号",
    "设备 Type": "设备类型",
    "设备 Name": "设备名称",
    "Mobile 设备": "移动设备",
    "网络 Adapter": "网络适配器",
    "存储 Device": "存储设备",
    "音频 Device": "音频设备",
    "卡 Reader": "读卡器",
    "序列号 Adapter": "串口适配器",
    "Serial 设备": "串口设备",
    "Bluetooth 适配器s": "蓝牙适配器",
    "Ethernet 端口": "以太网端口",
    "Wireless 适配器s": "无线适配器",
    "Physical 大小": "物理大小",
    "System 启动time": "系统启动时间",
    "存储 Devices": "存储设备",
    "Total 存储": "总存储",
    "显卡 Cards": "显卡",
    "Display 分辨率": "显示分辨率",
    "Ethernet 接口s": "以太网接口",
    "Wireless 接口s": "无线接口",
    "Kernel 版本": "内核版本",
    "桌面环境 Environment Technical Details": "桌面环境技术细节",
    "桌面环境 Environment": "桌面环境",
    "显示器 Server": "显示服务器",
    "Wayland 显示器": "Wayland 显示",
    
    # Placeholders
    "%1/显示器 名称": "%1/显示名称",
    "%1/显示器 String": "%1/显示字符串",
    "%1/Product Code": "%1/产品代码",
    "%1/序列号 Number": "%1/序列号",
    "%1/Display 名称": "%1/显示名称",
    "%1/Display String": "%1/显示字符串",
}

def fix_ts_file(filepath):
    """Fix all mixed translations in the .ts file"""
    tree = ET.parse(filepath)
    root = tree.getroot()
    
    fixed_count = 0
    
    for message in root.findall('.//message'):
        translation = message.find('translation')
        
        if translation is not None and translation.text:
            original = translation.text
            
            # Check if this translation needs fixing
            if original in fix_map:
                translation.text = fix_map[original]
                if 'type' in translation.attrib:
                    del translation.attrib['type']
                fixed_count += 1
                print(f"Fixed: '{original}' → '{fix_map[original]}'")
    
    # Write back to file
    tree.write(filepath, encoding='utf-8', xml_declaration=True)
    
    print(f"\nTotal fixed: {fixed_count} mixed translations")
    
    return fixed_count

if __name__ == '__main__':
    filepath = 'i18n/lsv_zh_CN.ts'
    print(f"Fixing mixed translations in {filepath}...")
    fix_ts_file(filepath)
    print("Done!")

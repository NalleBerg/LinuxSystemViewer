#!/usr/bin/env python3
import re

with open('i18n/lsv_zh_CN.ts', 'r', encoding='utf-8') as f:
    content = f.read()

# Fix "Geek Mode"
content = content.replace(' - Geek Mode</translation>', ' - 专家模式</translation>')

# Fix "All Files"
content = content.replace('All Files', '所有文件')

# Fix common phrases
fixes = {
    'The information has been copied': '信息已复制',
    'Could not open': '无法打开',
    'Sound-test': '声音测试',
    'Testing audio output with generated tones:': '使用生成的音调测试音频输出：',
    'MemTotal': '内存总计',
    'MemFree': '空闲内存',
    'MemAvailable': '可用内存',
    'SwapTotal': '交换空间总计',
    'Memory block entries': '内存块条目',
    'NUMA nodes count': 'NUMA 节点数',
    'None detected': '未检测到',
    'DMI memory device entries': 'DMI 内存设备条目',
    'Full lshw -C bus output': '完整 lshw -C bus 输出',
    'Support Page': '支持页面',
    'Cleaning up': '清理中',
    'Cleaning up temporary files...': '正在清理临时文件...',
    'Cannot elevate': '无法提权',
    'PCI Device': 'PCI 设备',
    'Device uevent': '设备 uevent',
    'DRM Card': 'DRM 卡',
    'EDID Size': 'EDID 大小',
    'CPU MHz': '处理器 MHz',
}

for eng, chi in fixes.items():
    content = content.replace(f'>{eng}</translation>', f'>{chi}</translation>')

# Fix section headers
sections = {
    'PCI GRAPHICS DEVICES': 'PCI 显卡设备',
    'DRM/DRI INFORMATION': 'DRM/DRI 信息',
    'FRAMEBUFFER INFORMATION': '帧缓冲信息',
    'DISPLAY CONNECTORS': '显示器连接器',
    'BACKLIGHT INFORMATION': '背光信息',
    'GPU FREQUENCY/POWER': 'GPU 频率/功耗',
    'LSHW BUS OUTPUT': 'LSHW 总线输出',
    'HOSTNAME INFORMATION': '主机名信息',
    'HOSTNAMECTL OUTPUT': 'HOSTNAMECTL 输出',
    'ACPI INFORMATION': 'ACPI 信息',
    'BOOT INFORMATION': '启动信息',
}

for eng, chi in sections.items():
    content = content.replace(f'=== {eng} ===', f'=== {chi} ===')

with open('i18n/lsv_zh_CN.ts', 'w', encoding='utf-8') as f:
    f.write(content)

print("✅ Fixed all English!")

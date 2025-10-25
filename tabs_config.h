#ifndef TABS_CONFIG_H
#define TABS_CONFIG_H

#include <QString>
#include <QStringList>

struct TabConfig {
    QString name;
    QString description;
    QString command;
    bool hasGeekMode;
    QString geekCommand;
};

// Tab configuration for LSV
static const QList<TabConfig> TAB_CONFIGS = {
    {
        "Summary",
        "System overview and key information",
        "lshw -short",
        true,
        "lshw -xml"
    },
    {
        "OS",
        "Operating system and kernel information", 
        "uname -a && lsb_release -a 2>/dev/null || cat /etc/os-release",
        true,
        "uname -a && cat /proc/version && cat /etc/os-release && cat /proc/cmdline"
    },
    {
        "Desktop",
        "Windowing environment and desktop session",
        "echo $XDG_CURRENT_DESKTOP && echo $DESKTOP_SESSION && echo $XDG_SESSION_TYPE",
        true,
        "env | grep -E '(DESKTOP|XDG|WAYLAND|X11)' | sort"
    },
    {
        "Audio", 
        "Audio devices and sound configuration",
        "lshw -C multimedia -short",
        true,
        "lshw -C multimedia && aplay -l 2>/dev/null && pactl info 2>/dev/null"
    },
    {
        "Graphics gard",
        "Graphics cards and display information",
        "lshw -C display -short",
        true,
        "lshw -C display && lspci | grep VGA && glxinfo | head -20 2>/dev/null"
    },
    {
        "Screen",
        "Display resolution and monitor information", 
        "xrandr --query 2>/dev/null || echo 'Display info not available'",
        true,
        "xrandr --verbose 2>/dev/null && xdpyinfo 2>/dev/null"
    },
    {
        "Ports",
        "USB, serial and other port information",
        "lsusb -t && lspci | grep -i 'serial\\|usb'",
        true,
        "lsusb -v && lspci -v | grep -A5 -i 'serial\\|usb' && dmesg | grep -i usb | tail -10"
    },
    {
        "Peripherals",
        "Connected peripherals and devices",
        "lsusb && lspci -nn | head -10",
        true,
        "lsusb -v && lspci -vv && lsblk && cat /proc/bus/input/devices"
    },
    {
        "Memory",
        "RAM slots, capacity and memory information",
        "free -h && lshw -C memory -short",
        true,
        "free -h && cat /proc/meminfo && lshw -C memory && dmidecode -t memory 2>/dev/null"
    },
    {
        "CPU",
        "Processor cores, cache, temperature and performance",
        "lscpu && cat /proc/cpuinfo | grep -E '(model name|cpu cores|siblings|cache)' | head -8",
        true,
        "lscpu -e && cat /proc/cpuinfo && lshw -C processor && sensors 2>/dev/null"
    },
    {
        "Motherboard", 
        "Mainboard and system board information",
        "lshw -C bus -short",
        true,
        "lshw -C bus && dmidecode -t baseboard 2>/dev/null && dmidecode -t system 2>/dev/null"
    },
    {
        "Disk",
        "Disk drives, partitions and storage devices",
        "lsblk && df -h",
        true,
        "lsblk -f && df -h && lshw -C disk && fdisk -l 2>/dev/null && smartctl --scan 2>/dev/null"
    },
    {
    "PC Info",
    "PC type, name, manufacturer and related system info",
    "hostnamectl && cat /sys/class/dmi/id/* 2>/dev/null",
    true,
    "hostnamectl && cat /sys/class/dmi/id/* 2>/dev/null"
    },
    {
        "About",
        "About this application",
    "echo 'Linux System Viewer (LSV) v0.6.0'",
        false,
        ""
    }
};

#endif // TABS_CONFIG_H
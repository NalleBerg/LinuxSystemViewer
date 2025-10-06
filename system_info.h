#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <QString>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QFile>
#include <QDir>
#include <QIODevice>
#include <QRegularExpression>
#include <QProcess>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <QtGlobal>
#include "memory.h"

// Forward declarations
class QTableWidget;

// System information functions
void loadOSInformation(QTableWidget* osTable, const QJsonObject &systemData);
void loadCPUInformation(QTableWidget* cpuTable, const QJsonObject &cpuData);
void loadSystemInformation(QTableWidget* systemTable, const QJsonObject &systemData);
void loadSummaryInformation(QTableWidget* summaryTable);

// Helper functions for system info
QString getCPUInfo();
QString getOSInfo();
QString getSystemUptime();
QString getKernelInfo();
QString getSystemLoad();

// Table utility functions
void addRowToTable(QTableWidget* table, const QStringList& data);

// Inline implementations

inline QString getCPUInfo()
{
    QFile cpuInfoFile("/proc/cpuinfo");
    if (!cpuInfoFile.open(QIODevice::ReadOnly)) {
        return "Unknown";
    }
    
    QString content = cpuInfoFile.readAll();
    cpuInfoFile.close();
    
    // Extract model name
    QRegularExpression modelRegex(R"(model name\s*:\s*(.+))");
    QRegularExpressionMatch match = modelRegex.match(content);
    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }
    
    return "Unknown";
}

inline QString getOSInfo()
{
    QFile osReleaseFile("/etc/os-release");
    if (!osReleaseFile.open(QIODevice::ReadOnly)) {
        // Try alternative location
        osReleaseFile.setFileName("/usr/lib/os-release");
        if (!osReleaseFile.open(QIODevice::ReadOnly)) {
            return "Unknown Linux Distribution";
        }
    }
    
    QString content = osReleaseFile.readAll();
    osReleaseFile.close();
    
    // Extract PRETTY_NAME
    QRegularExpression nameRegex("PRETTY_NAME=\"?([^\"\\n]+)\"?");
    QRegularExpressionMatch match = nameRegex.match(content);
    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }
    
    return "Unknown Linux Distribution";
}

inline QString getSystemUptime()
{
    QFile uptimeFile("/proc/uptime");
    if (!uptimeFile.open(QIODevice::ReadOnly)) {
        return "Unknown";
    }
    
    QString content = uptimeFile.readAll().trimmed();
    uptimeFile.close();
    
    QStringList parts = content.split(' ');
    if (parts.size() >= 1) {
        double uptimeSeconds = parts[0].toDouble();
        int days = int(uptimeSeconds) / (24 * 3600);
        int hours = (int(uptimeSeconds) % (24 * 3600)) / 3600;
        int minutes = (int(uptimeSeconds) % 3600) / 60;
        
        if (days > 0) {
            return QString("%1 days, %2:%3").arg(days).arg(hours, 2, 10, QChar('0')).arg(minutes, 2, 10, QChar('0'));
        } else {
            return QString("%1:%2").arg(hours, 2, 10, QChar('0')).arg(minutes, 2, 10, QChar('0'));
        }
    }
    
    return "Unknown";
}

inline QString getKernelInfo()
{
    QProcess unameProcess;
    unameProcess.start("uname", QStringList() << "-r");
    unameProcess.waitForFinished(2000);
    
    if (unameProcess.exitCode() == 0) {
        return unameProcess.readAllStandardOutput().trimmed();
    }
    
    return "Unknown";
}

inline QString getSystemLoad()
{
    QFile loadavgFile("/proc/loadavg");
    if (!loadavgFile.open(QIODevice::ReadOnly)) {
        return "Unknown";
    }
    
    QString content = loadavgFile.readAll().trimmed();
    loadavgFile.close();
    
    QStringList parts = content.split(' ');
    if (parts.size() >= 3) {
        return QString("%1, %2, %3").arg(parts[0]).arg(parts[1]).arg(parts[2]);
    }
    
    return "Unknown";
}

inline void loadOSInformation(QTableWidget* osTable, const QJsonObject &systemData)
{
    if (!osTable) return;
    
    osTable->setRowCount(0);
    
    // Get OS information from multiple sources
    addRowToTable(osTable, {"Distribution", getOSInfo()});
    addRowToTable(osTable, {"Kernel", getKernelInfo()});
    addRowToTable(osTable, {"Uptime", getSystemUptime()});
    addRowToTable(osTable, {"Load Average", getSystemLoad()});
    
    // Add lshw data if available
    if (systemData.contains("product")) {
        addRowToTable(osTable, {"System", systemData["product"].toString()});
    }
    if (systemData.contains("vendor")) {
        addRowToTable(osTable, {"Vendor", systemData["vendor"].toString()});
    }
    if (systemData.contains("version")) {
        addRowToTable(osTable, {"Version", systemData["version"].toString()});
    }
}

inline void loadCPUInformation(QTableWidget* cpuTable, const QJsonObject &cpuData)
{
    if (!cpuTable) return;
    
    cpuTable->setRowCount(0);
    
    // Get CPU information from /proc/cpuinfo
    addRowToTable(cpuTable, {"Model", getCPUInfo()});
    
    // Count CPU cores
    QFile cpuInfoFile("/proc/cpuinfo");
    if (cpuInfoFile.open(QIODevice::ReadOnly)) {
        QString content = cpuInfoFile.readAll();
        int coreCount = content.count("processor\t:");
        addRowToTable(cpuTable, {"Cores", QString::number(coreCount)});
        cpuInfoFile.close();
    }
    
    // Add lshw CPU data if available
    if (cpuData.contains("product")) {
        addRowToTable(cpuTable, {"Product", cpuData["product"].toString()});
    }
    if (cpuData.contains("vendor")) {
        addRowToTable(cpuTable, {"Vendor", cpuData["vendor"].toString()});
    }
    if (cpuData.contains("size")) {
        double mhz = cpuData["size"].toDouble() / 1000000.0; // Convert Hz to MHz
        addRowToTable(cpuTable, {"Frequency", QString::number(mhz, 'f', 0) + " MHz"});
    }
    if (cpuData.contains("width")) {
        addRowToTable(cpuTable, {"Architecture", QString::number(cpuData["width"].toInt()) + "-bit"});
    }
}

inline void loadSystemInformation(QTableWidget* systemTable, const QJsonObject &systemData)
{
    if (!systemTable) return;
    
    systemTable->setRowCount(0);
    
    // Add system information from lshw
    if (systemData.contains("product")) {
        addRowToTable(systemTable, {"Product", systemData["product"].toString()});
    }
    if (systemData.contains("vendor")) {
        addRowToTable(systemTable, {"Vendor", systemData["vendor"].toString()});
    }
    if (systemData.contains("version")) {
        addRowToTable(systemTable, {"Version", systemData["version"].toString()});
    }
    if (systemData.contains("serial")) {
        addRowToTable(systemTable, {"Serial", systemData["serial"].toString()});
    }
    if (systemData.contains("width")) {
        addRowToTable(systemTable, {"Architecture", QString::number(systemData["width"].toInt()) + "-bit"});
    }
    
    // Add additional system information
    addRowToTable(systemTable, {"Hostname", QProcess::systemEnvironment().filter("HOSTNAME=").value(0).split("=").value(1, "Unknown")});
    
    // Get desktop environment
    QString desktop = QProcess::systemEnvironment().filter("XDG_CURRENT_DESKTOP=").value(0).split("=").value(1, "Unknown");
    if (desktop == "Unknown") {
        desktop = QProcess::systemEnvironment().filter("DESKTOP_SESSION=").value(0).split("=").value(1, "Unknown");
    }
    addRowToTable(systemTable, {"Desktop Environment", desktop});
}

inline void loadSummaryInformation(QTableWidget* summaryTable)
{
    if (!summaryTable) return;
    
    summaryTable->setRowCount(0);
    
    // Add key system information to summary
    addRowToTable(summaryTable, {"OS", getOSInfo()});
    addRowToTable(summaryTable, {"Kernel", getKernelInfo()});
    addRowToTable(summaryTable, {"CPU", getCPUInfo()});
    addRowToTable(summaryTable, {"Memory", getMemoryInfo()});
    addRowToTable(summaryTable, {"Uptime", getSystemUptime()});
    addRowToTable(summaryTable, {"Load", getSystemLoad()});
}

#endif // SYSTEM_INFO_H
#ifndef CPU_H
#define CPU_H

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QJsonObject>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QStringList>
#include <QDebug>
#include <QDir>
#include <QProcess>
#include "gui_helpers.h"

// CPU information functions
void loadCpuInformation(QTableWidget* table, const QJsonObject& data);
QStringList getCpuHeaders();
void styleCpuTable(QTableWidget* table);
QString getCpuInfo();

// CPU Headers
QStringList getCpuHeaders()
{
    return QStringList() << "Property" << "Value" << "Unit";
}

// CPU Table Styling
void styleCpuTable(QTableWidget* table)
{
    // Set column widths
    table->setColumnWidth(0, 220);  // Property
    table->setColumnWidth(1, 300);  // Value
    table->setColumnWidth(2, 80);   // Unit
    
    // Style headers
    table->horizontalHeader()->setStyleSheet(
        "QHeaderView::section { "
        "background-color: #2c3e50; "
        "color: white; "
        "padding: 8px; "
        "border: none; "
        "font-weight: bold; "
        "}"
    );
}

// Load CPU Information
void loadCpuInformation(QTableWidget* table, const QJsonObject& data)
{
    Q_UNUSED(data); // Use direct system calls instead of JSON data
    
    // Only populate the limited, user-friendly CPU fields requested by the UI:
    // Vendor, Model, Number of processor (Physical), Total number of processors,
    // Max freq (GHz), Current freq (GHz), Min Freq (GHz), Cache size, Bogomips
    table->setRowCount(0);

    QFile file("/proc/cpuinfo");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addRowToTable(table, QStringList() << "Error" << "Could not read /proc/cpuinfo" << "");
        return;
    }
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    // Extract fields
    QString vendor;
    QString model;
    QString cacheSize;
    QString bogomips;
    QString currentFreqGHz;

    QSet<QString> physicalIds;
    int logicalCount = 0;

    for (const QString& line : content.split('\n')) {
        if (line.trimmed().isEmpty()) continue;
        if (line.startsWith("vendor_id")) {
            if (vendor.isEmpty()) vendor = line.section(':',1).trimmed();
        } else if (line.startsWith("model name")) {
            if (model.isEmpty()) model = line.section(':',1).trimmed();
        } else if (line.startsWith("cache size")) {
            if (cacheSize.isEmpty()) cacheSize = line.section(':',1).trimmed();
        } else if (line.startsWith("bogomips")) {
            if (bogomips.isEmpty()) bogomips = line.section(':',1).trimmed();
        } else if (line.startsWith("cpu MHz")) {
            if (currentFreqGHz.isEmpty()) {
                double mhz = line.section(':',1).trimmed().toDouble();
                currentFreqGHz = QString::number(mhz / 1000.0, 'f', 2);
            }
        } else if (line.startsWith("physical id")) {
            physicalIds.insert(line.section(':',1).trimmed());
        } else if (line.startsWith("processor")) {
            logicalCount++;
        }
    }

    // Total number of processors (logical)
    addRowToTable(table, QStringList() << "Total number of processors" << QString::number(logicalCount) << "");

    // Number of physical processors (sockets)
    int physicalCount = physicalIds.size() > 0 ? physicalIds.size() : 1;
    addRowToTable(table, QStringList() << "Number of processor (Physical)" << QString::number(physicalCount) << "");

    // Vendor and Model
    addRowToTable(table, QStringList() << "Vendor" << (vendor.isEmpty() ? "Unknown" : vendor) << "");
    addRowToTable(table, QStringList() << "Model" << (model.isEmpty() ? "Unknown" : model) << "");

    // Cache size and BogoMIPS
    addRowToTable(table, QStringList() << "Cache size" << (cacheSize.isEmpty() ? "Unknown" : cacheSize) << "");
    addRowToTable(table, QStringList() << "Bogomips" << (bogomips.isEmpty() ? "Unknown" : bogomips) << "");

    // Current frequency (GHz)
    addRowToTable(table, QStringList() << "Current freq (GHz)" << (currentFreqGHz.isEmpty() ? "Unknown" : currentFreqGHz) << "GHz");

    // Min / Max freq from sysfs (if available)
    QFile maxFreqFile("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
    if (maxFreqFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString maxFreq = QTextStream(&maxFreqFile).readLine().trimmed();
        maxFreqFile.close();
        double maxGHz = maxFreq.toLongLong() / 1000000.0;
        addRowToTable(table, QStringList() << "Max freq (GHz)" << QString::number(maxGHz, 'f', 2) << "GHz");
    } else {
        addRowToTable(table, QStringList() << "Max freq (GHz)" << "Unknown" << "GHz");
    }

    QFile minFreqFile("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq");
    if (minFreqFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString minFreq = QTextStream(&minFreqFile).readLine().trimmed();
        minFreqFile.close();
        double minGHz = minFreq.toLongLong() / 1000000.0;
        addRowToTable(table, QStringList() << "Min Freq (GHz)" << QString::number(minGHz, 'f', 2) << "GHz");
    } else {
        addRowToTable(table, QStringList() << "Min Freq (GHz)" << "Unknown" << "GHz");
    }
}

// Get basic CPU info string
QString getCpuInfo()
{
    QFile file("/proc/cpuinfo");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return "Error reading CPU information";
    }
    
    QTextStream in(&file);
    QString content = in.readAll();
    file.close();
    
    QStringList lines = content.split('\n');
    QString result;
    
    for (const QString& line : lines) {
        if (line.startsWith("model name")) {
            QStringList parts = line.split(':');
            if (parts.size() >= 2) {
                result = parts[1].trimmed();
                break;
            }
        }
    }
    
    return result.isEmpty() ? "Unknown CPU" : result;
}

#endif // CPU_H
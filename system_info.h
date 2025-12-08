#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

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
#include <QHostInfo>
#include <QSysInfo>
#include <QDateTime>
#include "gui_helpers.h"

// System information functions
void loadSystemInformation(QTableWidget* table, const QJsonObject& data);
void styleSystemTable(QTableWidget* table);
QString getSystemInfo();

// System Table Styling
void styleSystemTable(QTableWidget* table)
{
    // Set column widths
    table->setColumnWidth(0, 200);  // Property
    table->setColumnWidth(1, 300);  // Value
    table->setColumnWidth(2, 80);   // Unit
    table->setColumnWidth(3, 120);  // Type
    
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

// Load System Information
void loadSystemInformation(QTableWidget* table, const QJsonObject& data)
{
    Q_UNUSED(data); // Use direct system calls instead of JSON data
    
    table->setRowCount(0);
    
    // Basic system information
    addRowToTable(table, QStringList() << "Hostname" << QHostInfo::localHostName() << "" << "System");
    addRowToTable(table, QStringList() << "Kernel Name" << QSysInfo::kernelType() << "" << "System");
    addRowToTable(table, QStringList() << "Kernel Version" << QSysInfo::kernelVersion() << "" << "System");
    addRowToTable(table, QStringList() << "Architecture" << QSysInfo::currentCpuArchitecture() << "" << "System");
    addRowToTable(table, QStringList() << "Product Name" << QSysInfo::prettyProductName() << "" << "System");
    addRowToTable(table, QStringList() << "Product Type" << QSysInfo::productType() << "" << "System");
    addRowToTable(table, QStringList() << "Product Version" << QSysInfo::productVersion() << "" << "System");
    
    // Read additional system information from /proc/version
    QFile versionFile("/proc/version");
    if (versionFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&versionFile);
        QString version = stream.readLine();
        versionFile.close();
        addRowToTable(table, QStringList() << "Kernel Info" << version << "" << "System");
    }
    
    // Read uptime
    QFile uptimeFile("/proc/uptime");
    if (uptimeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&uptimeFile);
        QString uptimeStr = stream.readLine();
        uptimeFile.close();
        
        QStringList parts = uptimeStr.split(' ');
        if (!parts.isEmpty()) {
            double uptimeSeconds = parts[0].toDouble();
            int days = static_cast<int>(uptimeSeconds / 86400);
            int hours = static_cast<int>((uptimeSeconds - days * 86400) / 3600);
            int minutes = static_cast<int>((uptimeSeconds - days * 86400 - hours * 3600) / 60);
            
            QString uptimeFormatted = QString("%1 days, %2 hours, %3 minutes")
                .arg(days).arg(hours).arg(minutes);
            addRowToTable(table, QStringList() << "Uptime" << uptimeFormatted << "" << "System");
        }
    }
    
    // Read load average
    QFile loadAvgFile("/proc/loadavg");
    if (loadAvgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&loadAvgFile);
        QString loadLine = stream.readLine();
        loadAvgFile.close();
        
        QStringList loadParts = loadLine.split(' ');
        if (loadParts.size() >= 3) {
            addRowToTable(table, QStringList() << "Load Average" << QString("%1, %2, %3").arg(loadParts[0], loadParts[1], loadParts[2]) << "" << "System");
        }
    }
    
    // Read number of processes
    QDir procDir("/proc");
    QStringList procEntries = procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    int processCount = 0;
    for (const QString& entry : procEntries) {
        bool ok;
        entry.toInt(&ok);
        if (ok) processCount++;
    }
    addRowToTable(table, QStringList() << "Running Processes" << QString::number(processCount) << "" << "System");
    
    // Read distribution information
    QFile osReleaseFile("/etc/os-release");
    if (osReleaseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&osReleaseFile);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            if (line.startsWith("NAME=")) {
                QString distroName = line.mid(5).replace("\"", "");
                addRowToTable(table, QStringList() << "Distribution" << distroName << "" << "System");
            } else if (line.startsWith("VERSION=")) {
                QString version = line.mid(8).replace("\"", "");
                addRowToTable(table, QStringList() << "Distribution Version" << version << "" << "System");
            } else if (line.startsWith("ID=")) {
                QString id = line.mid(3).replace("\"", "");
                addRowToTable(table, QStringList() << "Distribution ID" << id << "" << "System");
            }
        }
        osReleaseFile.close();
    }
    
    // Read timezone information
    QFile timezoneFile("/etc/timezone");
    if (timezoneFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&timezoneFile);
        QString timezone = stream.readLine().trimmed();
        timezoneFile.close();
        addRowToTable(table, QStringList() << "Timezone" << timezone << "" << "System");
    }
    
    // Get current date and time
    QDateTime currentDateTime = QDateTime::currentDateTime();
    addRowToTable(table, QStringList() << "Current Time" << currentDateTime.toString("yyyy-MM-dd hh:mm:ss") << "" << "System");
    
    // Read locale information
    QString locale = qgetenv("LANG");
    if (!locale.isEmpty()) {
        addRowToTable(table, QStringList() << "Locale" << locale << "" << "System");
    }
}

// Get basic system info string
QString getSystemInfo()
{
    return QSysInfo::prettyProductName() + " - " + QSysInfo::kernelVersion();
}

#endif // SYSTEM_INFO_H
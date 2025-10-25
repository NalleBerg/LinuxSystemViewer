#ifndef OS_H
#define OS_H

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QJsonObject>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>
#include "gui_helpers.h"

// OS information functions
void loadOSInformation(QTableWidget* table, const QJsonObject& data);
QStringList getOSHeaders();
void styleOSTable(QTableWidget* table);
QString getOSInfo();

// OS Headers
QStringList getOSHeaders()
{
    return QStringList() << "Property" << "Value" << "Unit" << "Type";
}

// OS Table Styling
void styleOSTable(QTableWidget* table)
{
    // Set column widths
    table->setColumnWidth(0, 200);  // Property
    table->setColumnWidth(1, 300);  // Value
    table->setColumnWidth(2, 80);   // Unit
    table->setColumnWidth(3, 120);  // Type
    
    // Style headers
    table->horizontalHeader()->setStyleSheet(
        "QHeaderView::section { "
        "background-color: #8e44ad; "
        "color: white; "
        "padding: 8px; "
        "border: none; "
        "font-weight: bold; "
        "}"
    );
}

// Load OS Information
void loadOSInformation(QTableWidget* table, const QJsonObject& data)
{
    Q_UNUSED(data);
    
    table->setRowCount(0);
    
    // Get OS Release information
    QFile osReleaseFile("/etc/os-release");
    if (osReleaseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&osReleaseFile);
        QStringList lines = stream.readAll().split('\n');
        osReleaseFile.close();
        
        QString osName, osVersion, osId, versionId, prettyName;
        
        for (const QString& line : lines) {
            if (line.startsWith("NAME=")) {
                osName = line.mid(5).remove('"');
            } else if (line.startsWith("VERSION=")) {
                osVersion = line.mid(8).remove('"');
            } else if (line.startsWith("ID=")) {
                osId = line.mid(3).remove('"');
            } else if (line.startsWith("VERSION_ID=")) {
                versionId = line.mid(11).remove('"');
            } else if (line.startsWith("PRETTY_NAME=")) {
                prettyName = line.mid(12).remove('"');
            }
        }
        
        if (!prettyName.isEmpty()) {
            addRowToTable(table, QStringList() << "OS Name" << prettyName << "" << "OS");
        }
        if (!osName.isEmpty()) {
            addRowToTable(table, QStringList() << "Distribution" << osName << "" << "OS");
        }
        if (!osVersion.isEmpty()) {
            addRowToTable(table, QStringList() << "Version" << osVersion << "" << "OS");
        }
        if (!versionId.isEmpty()) {
            addRowToTable(table, QStringList() << "Version ID" << versionId << "" << "OS");
        }
    }
    
    // Get kernel information
    QFile versionFile("/proc/version");
    if (versionFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&versionFile);
        QString versionInfo = stream.readLine();
        versionFile.close();
        
        // Parse kernel version
        QRegularExpression kernelRegex("Linux version ([^ ]+)");
        QRegularExpressionMatch match = kernelRegex.match(versionInfo);
        if (match.hasMatch()) {
            addRowToTable(table, QStringList() << "Kernel Version" << match.captured(1) << "" << "OS");
        }
        
        // Parse compiler info
        QRegularExpression gccRegex("\\(gcc version ([^)]+)\\)");
        match = gccRegex.match(versionInfo);
        if (match.hasMatch()) {
            addRowToTable(table, QStringList() << "Compiled with" << "GCC " + match.captured(1) << "" << "OS");
        }
    }
    
    // Get uptime
    QFile uptimeFile("/proc/uptime");
    if (uptimeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&uptimeFile);
        QString uptimeStr = stream.readLine();
        uptimeFile.close();
        
        double uptimeSeconds = uptimeStr.split(' ')[0].toDouble();
        int days = uptimeSeconds / 86400;
        int hours = (uptimeSeconds - days * 86400) / 3600;
        int minutes = (uptimeSeconds - days * 86400 - hours * 3600) / 60;
        
        QString uptimeFormatted = QString("%1 days, %2 hours, %3 minutes").arg(days).arg(hours).arg(minutes);
        addRowToTable(table, QStringList() << "Uptime" << uptimeFormatted << "" << "OS");
    }
    
    // Get architecture
    QProcess archProcess;
    archProcess.start("uname", QStringList() << "-m");
    archProcess.waitForFinished();
    QString architecture = archProcess.readAllStandardOutput().trimmed();
    if (!architecture.isEmpty()) {
        addRowToTable(table, QStringList() << "Architecture" << architecture << "" << "OS");
    }
    
    // Get hostname
    QFile hostnameFile("/proc/sys/kernel/hostname");
    if (hostnameFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&hostnameFile);
        QString hostname = stream.readLine().trimmed();
        hostnameFile.close();
        addRowToTable(table, QStringList() << "Hostname" << hostname << "" << "OS");
    }
    
    // Get timezone
    QProcess timezoneProcess;
    timezoneProcess.start("timedatectl", QStringList() << "show" << "--property=Timezone" << "--value");
    timezoneProcess.waitForFinished();
    QString timezone = timezoneProcess.readAllStandardOutput().trimmed();
    if (!timezone.isEmpty()) {
        addRowToTable(table, QStringList() << "Timezone" << timezone << "" << "OS");
    }
}

QString getOSInfo()
{
    QFile osReleaseFile("/etc/os-release");
    if (osReleaseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&osReleaseFile);
        QStringList lines = stream.readAll().split('\n');
        osReleaseFile.close();
        
        for (const QString& line : lines) {
            if (line.startsWith("PRETTY_NAME=")) {
                return line.mid(12).remove('"');
            }
        }
    }
    return "Unknown OS";
}

#endif // OS_H
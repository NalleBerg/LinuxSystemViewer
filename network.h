#ifndef NETWORK_H
#define NETWORK_H

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
#include <QNetworkInterface>
#include <QHostAddress>
#include <QtEndian>  // For qFromBigEndian
#include "gui_helpers.h"

// Forward declaration of formatBytes
QString formatBytes(long long bytes);

// Network information functions
void loadNetworkInformation(QTableWidget* table, const QJsonObject& data);
void styleNetworkTable(QTableWidget* table);
QString getNetworkInfo();

// Network Table Styling
inline void styleNetworkTable(QTableWidget* table)
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

// Helper function to format bytes - moved to top
inline QString formatBytes(long long bytes)
{
    const QStringList units = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = bytes;
    
    while (size >= 1024.0 && unitIndex < units.size() - 1) {
        size /= 1024.0;
        unitIndex++;
    }
    
    return QString("%1 %2").arg(QString::number(size, 'f', 2)).arg(units[unitIndex]);
}

// Load Network Information - Storage-style with separate tables per device

// Get basic network info string  
inline QString getNetworkInfo()
{
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    
    for (const QNetworkInterface& interface : interfaces) {
        if (interface.flags() & QNetworkInterface::IsUp && 
            interface.flags() & QNetworkInterface::IsRunning &&
            !(interface.flags() & QNetworkInterface::IsLoopBack)) {
            
            QList<QNetworkAddressEntry> addresses = interface.addressEntries();
            for (const QNetworkAddressEntry& entry : addresses) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    return QString("%1: %2").arg(interface.name(), entry.ip().toString());
                }
            }
        }
    }
    
    return "No active network interfaces";
}

#endif // NETWORK_H
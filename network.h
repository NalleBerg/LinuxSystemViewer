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

// Load Network Information
inline void loadNetworkInformation(QTableWidget* table, const QJsonObject& data)
{
    Q_UNUSED(data); // Use direct system calls instead of JSON data
    
    table->setRowCount(0);
    
    // Get network interfaces using Qt
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    
    for (const QNetworkInterface& interface : interfaces) {
        QString interfaceName = interface.name();
        QString displayName = interface.humanReadableName();
        
        // Skip loopback if it's just "lo"
        if (interfaceName == "lo") continue;
        
    addRowToTable(table, QStringList() << QString("Interface: %1").arg(interfaceName) << displayName);
        
        // Interface flags
        QNetworkInterface::InterfaceFlags flags = interface.flags();
        QStringList flagsList;
        if (flags & QNetworkInterface::IsUp) flagsList << "UP";
        if (flags & QNetworkInterface::IsRunning) flagsList << "RUNNING";
        if (flags & QNetworkInterface::CanBroadcast) flagsList << "BROADCAST";
        if (flags & QNetworkInterface::IsLoopBack) flagsList << "LOOPBACK";
        if (flags & QNetworkInterface::IsPointToPoint) flagsList << "POINTOPOINT";
        if (flags & QNetworkInterface::CanMulticast) flagsList << "MULTICAST";
        
        if (!flagsList.isEmpty()) {
            addRowToTable(table, QStringList() << QString("  %1 Flags").arg(interfaceName) << flagsList.join(", "));
        }
        
        // MAC Address
        QString macAddress = interface.hardwareAddress();
        if (!macAddress.isEmpty()) {
            addRowToTable(table, QStringList() << QString("  %1 MAC").arg(interfaceName) << macAddress);
        }
        
        // IP Addresses
        QList<QNetworkAddressEntry> addresses = interface.addressEntries();
        for (const QNetworkAddressEntry& entry : addresses) {
            QHostAddress ip = entry.ip();
                if (ip.protocol() == QAbstractSocket::IPv4Protocol) {
                addRowToTable(table, QStringList() << QString("  %1 IPv4").arg(interfaceName) << ip.toString());
                
                QHostAddress netmask = entry.netmask();
                if (!netmask.isNull()) {
                    addRowToTable(table, QStringList() << QString("  %1 Netmask").arg(interfaceName) << netmask.toString());
                }
                
                QHostAddress broadcast = entry.broadcast();
                if (!broadcast.isNull()) {
                    addRowToTable(table, QStringList() << QString("  %1 Broadcast").arg(interfaceName) << broadcast.toString());
                }
            } else if (ip.protocol() == QAbstractSocket::IPv6Protocol) {
                addRowToTable(table, QStringList() << QString("  %1 IPv6").arg(interfaceName) << ip.toString() << "" << "Network");
            }
        }
        
        // MTU
        int mtu = interface.maximumTransmissionUnit();
        if (mtu > 0) {
            addRowToTable(table, QStringList() << QString("  %1 MTU").arg(interfaceName) << QString("%1 bytes").arg(QString::number(mtu)));
        }
    }
    
    // Read network statistics from /proc/net/dev
    QFile netDevFile("/proc/net/dev");
    if (netDevFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&netDevFile);
        QStringList lines = stream.readAll().split('\n');
        netDevFile.close();
        
        // Skip header lines
        for (int i = 2; i < lines.size(); ++i) {
            QString line = lines[i].trimmed();
            if (line.isEmpty()) continue;
            
            QStringList parts = line.split(QRegularExpression("\\s+"));
            if (parts.size() >= 17) {
                QString interfaceName = parts[0];
                interfaceName.chop(1); // Remove trailing ':'
                
                if (interfaceName == "lo") continue; // Skip loopback
                
                // Received bytes, packets, errors, dropped
                long long rxBytes = parts[1].toLongLong();
                long long rxPackets = parts[2].toLongLong();
                long long rxErrors = parts[3].toLongLong();
                long long rxDropped = parts[4].toLongLong();
                
                // Transmitted bytes, packets, errors, dropped  
                long long txBytes = parts[9].toLongLong();
                long long txPackets = parts[10].toLongLong();
                long long txErrors = parts[11].toLongLong();
                long long txDropped = parts[12].toLongLong();
                
                // Convert bytes to human readable format
                QString rxBytesStr = formatBytes(rxBytes);
                QString txBytesStr = formatBytes(txBytes);
                
                addRowToTable(table, QStringList() << QString("  %1 RX Bytes").arg(interfaceName) << rxBytesStr);
                addRowToTable(table, QStringList() << QString("  %1 RX Packets").arg(interfaceName) << QString::number(rxPackets));
                if (rxErrors > 0) {
                    addRowToTable(table, QStringList() << QString("  %1 RX Errors").arg(interfaceName) << QString::number(rxErrors));
                }
                if (rxDropped > 0) {
                    addRowToTable(table, QStringList() << QString("  %1 RX Dropped").arg(interfaceName) << QString::number(rxDropped));
                }

                addRowToTable(table, QStringList() << QString("  %1 TX Bytes").arg(interfaceName) << txBytesStr);
                addRowToTable(table, QStringList() << QString("  %1 TX Packets").arg(interfaceName) << QString::number(txPackets));
                if (txErrors > 0) {
                    addRowToTable(table, QStringList() << QString("  %1 TX Errors").arg(interfaceName) << QString::number(txErrors));
                }
                if (txDropped > 0) {
                    addRowToTable(table, QStringList() << QString("  %1 TX Dropped").arg(interfaceName) << QString::number(txDropped));
                }
            }
        }
    }
    
    // Get default route information
    QFile routeFile("/proc/net/route");
    if (routeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&routeFile);
        QStringList lines = stream.readAll().split('\n');
        routeFile.close();
        
        for (int i = 1; i < lines.size(); ++i) { // Skip header
            QString line = lines[i].trimmed();
            if (line.isEmpty()) continue;
            
            QStringList parts = line.split('\t');
            if (parts.size() >= 8) {
                QString iface = parts[0];
                QString destination = parts[1];
                QString gateway = parts[2];
                QString flags = parts[3];
                
                // Check if this is the default route (destination 00000000)
                if (destination == "00000000") {
                    // Convert gateway from hex to IP
                    bool ok;
                    quint32 gwHex = gateway.toUInt(&ok, 16);
                    if (ok) {
                        QHostAddress gwAddr(qFromLittleEndian(gwHex));
                        addRowToTable(table, QStringList() << "Default Gateway" << gwAddr.toString());
                        addRowToTable(table, QStringList() << "Default Interface" << iface);
                    }
                    break;
                }
            }
        }
    }
    
    // Get DNS servers from /etc/resolv.conf
    QFile resolvFile("/etc/resolv.conf");
    if (resolvFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&resolvFile);
        QStringList lines = stream.readAll().split('\n');
        resolvFile.close();
        
        QStringList dnsServers;
        for (const QString& line : lines) {
            if (line.startsWith("nameserver ")) {
                QString dns = line.mid(11).trimmed();
                dnsServers << dns;
            }
        }
        
        if (!dnsServers.isEmpty()) {
            addRowToTable(table, QStringList() << "DNS Servers" << dnsServers.join(", ") << "" << "Network");
        }
    }
    
    // Get hostname
    QFile hostnameFile("/proc/sys/kernel/hostname");
    if (hostnameFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&hostnameFile);
        QString hostname = stream.readLine().trimmed();
        hostnameFile.close();
        
        if (!hostname.isEmpty()) {
            addRowToTable(table, QStringList() << "Hostname" << hostname << "" << "Network");
        }
    }
}

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
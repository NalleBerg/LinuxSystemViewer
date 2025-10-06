#ifndef NETWORK_H
#define NETWORK_H

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
#include <QMap>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QDebug>
#include <QtGlobal>
#include <QFileInfo>
#include <QScrollBar>

// Network interface information structure
struct NetworkInfo {
    QString ipv4Address;
    QString ipv6Address;
    QString subnet;
    QString product;
    QString vendor;
    QString driver;
    bool isActive;
};

// Network utility functions
QMap<QString, NetworkInfo> getNetworkInterfaces();
QString prefixLengthToSubnetMask(int prefixLength);
QString getExternalIPv4();
QString getExternalIPv6();

// Network information loading functions
void loadNetworkInformation(QTableWidget* networkTable, const QJsonObject &networkData);
void loadLiveNetworkInformation(QTableWidget* networkTable);
void addLiveNetworkToSummary(QTableWidget* summaryTable);
void refreshNetworkInfo(QTableWidget* networkTable);

// Helper functions
void addRowToTable(QTableWidget* table, const QStringList& data);

// Inline implementations

inline QString prefixLengthToSubnetMask(int prefixLength)
{
    if (prefixLength < 0 || prefixLength > 32) return "Invalid";
    
    quint32 mask = (0xFFFFFFFF << (32 - prefixLength)) & 0xFFFFFFFF;
    
    return QString("%1.%2.%3.%4")
        .arg((mask >> 24) & 0xFF)
        .arg((mask >> 16) & 0xFF)
        .arg((mask >> 8) & 0xFF)
        .arg(mask & 0xFF);
}

inline QString getExternalIPv4()
{
    // Try multiple methods and services for better reliability
    QStringList methods = {
        // Method 1: Try wget if curl fails
        "wget -qO- --timeout=5 --tries=1 ifconfig.me 2>/dev/null",
        "wget -qO- --timeout=5 --tries=1 ipecho.net/plain 2>/dev/null",
        "wget -qO- --timeout=5 --tries=1 icanhazip.com 2>/dev/null",
        
        // Method 2: Try curl with different options
        "curl -s --max-time 5 --connect-timeout 3 ifconfig.me 2>/dev/null",
        "curl -s --max-time 5 --connect-timeout 3 ipecho.net/plain 2>/dev/null", 
        "curl -s --max-time 5 --connect-timeout 3 icanhazip.com 2>/dev/null",
        "curl -s --max-time 5 --connect-timeout 3 checkip.amazonaws.com 2>/dev/null",
        
        // Method 3: Try dig method (DNS-based)
        "dig +short myip.opendns.com @resolver1.opendns.com 2>/dev/null",
        "dig +short txt ch whoami.cloudflare @1.0.0.1 2>/dev/null | tr -d '\"'",
        
        // Method 4: Use ip route to find default gateway and try local detection
        "ip route get 8.8.8.8 2>/dev/null | grep -oP 'src \\K[0-9.]+'"
    };
    
    for (const QString &method : methods) {
        QProcess process;
        process.start("bash", QStringList() << "-c" << method);
        process.waitForFinished(6000);
        
        if (process.exitCode() == 0) {
            QString result = process.readAllStandardOutput().trimmed();
            // Basic IPv4 validation - more precise regex
            QRegularExpression ipv4Regex(R"(^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
            if (ipv4Regex.match(result).hasMatch()) {
                // Additional validation - exclude private/local ranges
                QStringList parts = result.split('.');
                if (parts.size() == 4) {
                    int first = parts[0].toInt();
                    int second = parts[1].toInt();
                    
                    // Exclude private ranges: 10.x.x.x, 172.16-31.x.x, 192.168.x.x, 127.x.x.x
                    if (!(first == 10 || 
                          (first == 172 && second >= 16 && second <= 31) ||
                          (first == 192 && second == 168) ||
                          first == 127 ||
                          first == 0 ||
                          first >= 224)) { // Exclude multicast and reserved
                        return result;
                    }
                }
            }
        }
    }
    
    return "Not available";
}

inline QString getExternalIPv6()
{
    QStringList services = {
        "curl -s --max-time 5 -6 ifconfig.co",
        "curl -s --max-time 5 -6 icanhazip.com"
    };
    
    for (const QString &service : services) {
        QProcess process;
        process.start("bash", QStringList() << "-c" << service);
        process.waitForFinished(6000);
        
        if (process.exitCode() == 0) {
            QString result = process.readAllStandardOutput().trimmed();
            // Basic IPv6 validation (simplified)
            if (result.contains(":") && result.length() > 7) {
                return result;
            }
        }
    }
    
    return "Unknown";
}

inline QMap<QString, NetworkInfo> getNetworkInterfaces()
{
    QMap<QString, NetworkInfo> interfaces;
    
    // Get all network interfaces from /sys/class/net
    QDir netDir("/sys/class/net");
    QStringList interfaceNames = netDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QString &interfaceName : interfaceNames) {
        if (interfaceName == "lo") continue; // Skip loopback
        
        NetworkInfo info;
        
        // Check if interface is up
        QFile operState(QString("/sys/class/net/%1/operstate").arg(interfaceName));
        if (operState.open(QIODevice::ReadOnly)) {
            QString state = operState.readAll().trimmed();
            info.isActive = (state == "up");
        }
        
        // Get IP addresses using ip command
        QProcess ipProcess;
        ipProcess.start("ip", QStringList() << "addr" << "show" << interfaceName);
        ipProcess.waitForFinished(2000);
        
        if (ipProcess.exitCode() == 0) {
            QString output = ipProcess.readAllStandardOutput();
            
            // IPv4 pattern
            QRegularExpression ipv4Regex(R"(inet (\d+\.\d+\.\d+\.\d+)/(\d+))");
            QRegularExpressionMatch ipv4Match = ipv4Regex.match(output);
            
            if (ipv4Match.hasMatch()) {
                info.ipv4Address = ipv4Match.captured(1);
                int prefixLength = ipv4Match.captured(2).toInt();
                info.subnet = prefixLengthToSubnetMask(prefixLength);
            }
            
            // IPv6 pattern (global unicast addresses)
            QRegularExpression ipv6Regex(R"(inet6 ([0-9a-fA-F:]+)/(\d+) scope global)");
            QRegularExpressionMatch ipv6Match = ipv6Regex.match(output);
            
            if (ipv6Match.hasMatch()) {
                info.ipv6Address = ipv6Match.captured(1);
            }
        }
        
        // Get driver information
        QString driverPath = QString("/sys/class/net/%1/device/driver").arg(interfaceName);
        QFile driverFile(driverPath);
        if (driverFile.exists()) {
            QFileInfo driverInfo(driverFile.symLinkTarget());
            info.driver = driverInfo.baseName();
        }
        
        // Get vendor and product info from uevent
        QFile ueventFile(QString("/sys/class/net/%1/device/uevent").arg(interfaceName));
        if (ueventFile.open(QIODevice::ReadOnly)) {
            QString ueventContent = ueventFile.readAll();
            
            QRegularExpression vendorRegex(R"(PCI_ID=([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4}))");
            QRegularExpressionMatch vendorMatch = vendorRegex.match(ueventContent);
            
            if (vendorMatch.hasMatch()) {
                info.vendor = vendorMatch.captured(1);
                info.product = vendorMatch.captured(2);
            }
        }
        
        interfaces[interfaceName] = info;
    }
    
    return interfaces;
}

inline void loadNetworkInformation(QTableWidget* networkTable, const QJsonObject &networkData)
{
    if (!networkTable) return;
    
    QString type = networkData["class"].toString();
    if (type != "network") return;
    
    QString logicalName = networkData["logicalname"].toString();
    QString product = networkData["product"].toString();
    QString vendor = networkData["vendor"].toString();
    QString busInfo = networkData["businfo"].toString();
    
    // Get live network information for this interface
    QMap<QString, NetworkInfo> liveInterfaces = getNetworkInterfaces();
    
    QString ipv4 = "Not assigned";
    QString status = "Down";
    
    if (liveInterfaces.contains(logicalName)) {
        const NetworkInfo &info = liveInterfaces[logicalName];
        if (!info.ipv4Address.isEmpty()) {
            ipv4 = info.ipv4Address;
        }
        status = info.isActive ? "Up" : "Down";
    }
    
    addRowToTable(networkTable, {
        logicalName,
        product,
        vendor,
        ipv4,
        status,
        busInfo
    });
}

inline void loadLiveNetworkInformation(QTableWidget* networkTable)
{
    if (!networkTable) return;
    
    networkTable->setRowCount(0);
    
    QMap<QString, NetworkInfo> interfaces = getNetworkInterfaces();
    
    for (auto it = interfaces.begin(); it != interfaces.end(); ++it) {
        const QString &interfaceName = it.key();
        const NetworkInfo &info = it.value();
        
        QString status = info.isActive ? "Up" : "Down";
        QString ipv4 = info.ipv4Address.isEmpty() ? "Not assigned" : info.ipv4Address;
        QString ipv6 = info.ipv6Address.isEmpty() ? "Not assigned" : info.ipv6Address;
        
        addRowToTable(networkTable, {
            interfaceName,
            info.product.isEmpty() ? "Unknown" : info.product,
            info.vendor.isEmpty() ? "Unknown" : info.vendor,
            ipv4,
            status,
            info.driver.isEmpty() ? "Unknown" : info.driver
        });
    }
    
    // Add external IP information
    if (!interfaces.isEmpty()) {
        // Add separator
        addRowToTable(networkTable, {"", "", "", "", "", ""});
        
        // Add external IPs
        QString extIPv4 = getExternalIPv4();
        QString extIPv6 = getExternalIPv6();
        
        addRowToTable(networkTable, {
            "External IPv4",
            "",
            "",
            extIPv4,
            extIPv4 != "Not available" ? "Available" : "Not available",
            ""
        });
        
        addRowToTable(networkTable, {
            "External IPv6",
            "",
            "",
            extIPv6,
            extIPv6 != "Unknown" ? "Available" : "Not available",
            ""
        });
    }
}

inline void addLiveNetworkToSummary(QTableWidget* summaryTable)
{
    if (!summaryTable) return;
    
    QMap<QString, NetworkInfo> interfaces = getNetworkInterfaces();
    
    // Count active interfaces
    int activeInterfaces = 0;
    QString primaryIP = "None";
    
    for (auto it = interfaces.begin(); it != interfaces.end(); ++it) {
        const NetworkInfo &info = it.value();
        if (info.isActive) {
            activeInterfaces++;
            if (primaryIP == "None" && !info.ipv4Address.isEmpty()) {
                primaryIP = info.ipv4Address;
            }
        }
    }
    
    QString networkSummary;
    if (activeInterfaces > 0) {
        networkSummary = QString("%1 active interface%2, Primary IP: %3")
                        .arg(activeInterfaces)
                        .arg(activeInterfaces > 1 ? "s" : "")
                        .arg(primaryIP);
    } else {
        networkSummary = "No active network interfaces";
    }
    
    addRowToTable(summaryTable, {"Network", networkSummary});
}

inline void refreshNetworkInfo(QTableWidget* networkTable)
{
    if (!networkTable) return;
    
    // Only refresh if table is visible and has been populated
    if (!networkTable->isVisible() || networkTable->rowCount() == 0) {
        return;
    }
    
    // Store current scroll position to maintain user experience
    int currentRow = networkTable->currentRow();
    int scrollValue = networkTable->verticalScrollBar() ? networkTable->verticalScrollBar()->value() : 0;
    
    // Get live network information to detect new USB WiFi adapters, etc.
    loadLiveNetworkInformation(networkTable);
    
    // Restore scroll position
    if (networkTable->verticalScrollBar()) {
        networkTable->verticalScrollBar()->setValue(scrollValue);
    }
    if (currentRow >= 0 && currentRow < networkTable->rowCount()) {
        networkTable->setCurrentCell(currentRow, 0);
    }
}

#endif // NETWORK_H
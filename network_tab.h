#ifndef NETWORK_TAB_H
#define NETWORK_TAB_H

#include "tab_widget_base.h"
#include "network.h"
#include "network_geek.h"
#include "gui_helpers.h"
#include <QWidget>
#include <QString>
#include <QTableWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QTimer>
#include <QNetworkInterface>
#include <QNetworkAddressEntry>
#include <QFile>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QVector>
#include <QPushButton>
#include <QProcess>
#include <QRegularExpression>
#include <QScrollArea>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QTimer>
#include <QShowEvent>
#include <QHideEvent>

class NetworkTab : public TabWidgetBase
{
    Q_OBJECT
public:
    explicit NetworkTab(QWidget* parent = nullptr)
        : TabWidgetBase(tr("Network"), "", false, "", parent), refreshTimer(nullptr)
    {
        initializeTab();
    }

protected:
    void showEvent(QShowEvent* ev) override
    {
        TabWidgetBase::showEvent(ev);
        if (refreshTimer) refreshTimer->start();
    }
    
    void hideEvent(QHideEvent* ev) override
    {
        if (refreshTimer) refreshTimer->stop();
        TabWidgetBase::hideEvent(ev);
    }

private slots:

private:
    QTimer* refreshTimer;
    
    // Storage-style layout components
    QWidget* networkInfoContainer;
    QVBoxLayout* networkInfoLayout;
    QVector<QTableWidget*> networkTables;
    QVector<QLabel*> networkLabels;
    QWidget* createUserFriendlyView() override
    {
        QWidget* w = new QWidget;
        QVBoxLayout* mainLayout = new QVBoxLayout(w);
        applyMainLayoutDefaults(mainLayout);

        // Headline + Geek button (use strict UI helper to ensure exact placement)
        QPushButton* geekButton = nullptr;
        QHBoxLayout* headlineLayout = createHeadlineWithGeek(w, tr("Network"), &geekButton);
        mainLayout->addLayout(headlineLayout);

        // Create container for network info with Storage-style scroll area
        networkInfoContainer = new QWidget();
        networkInfoLayout = new QVBoxLayout(networkInfoContainer);
        networkInfoLayout->setContentsMargins(10, 10, 10, 10);
        networkInfoLayout->setSpacing(15);

        QScrollArea* scrollArea = new QScrollArea();
        scrollArea->setWidget(networkInfoContainer);
        scrollArea->setWidgetResizable(true);
        scrollArea->setMinimumHeight(220);
        mainLayout->addWidget(scrollArea);

        // Populate immediately with Storage-style layout
        refreshNetworkData();
        
        // Set up auto-refresh timer
        refreshTimer = new QTimer(this);
        refreshTimer->setInterval(3000); // Refresh every 3 seconds
        connect(refreshTimer, &QTimer::timeout, this, &NetworkTab::refreshNetworkData);
        
        // Geek dialog: opens the richer NetworkGeekDialog
        connect(geekButton, &QPushButton::clicked, this, [this, w]() {
            NetworkGeekDialog dlg(w);
            dlg.exec();
        });

        return w;
    }
    
    void refreshNetworkData()
    {
        // Clear existing content
        QLayoutItem* child;
        while ((child = networkInfoLayout->takeAt(0)) != nullptr) {
            if (child->widget()) {
                delete child->widget();
            }
            delete child;
        }
        
        // Create Storage-style layout for each network device
        createNetworkDeviceSections();
    }
    
    void createNetworkDeviceSections()
    {
        QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
        
        for (const QNetworkInterface& interface : interfaces) {
            if (interface.name() == "lo") continue;
            
            // Get device name using enhanced detection
            QString deviceName = getEnhancedDeviceName(interface);
            QString status = getDeviceStatus(interface);
            
            // Create blue header like Storage tab
            QLabel* headerLabel = new QLabel(QString("%1 %2").arg(deviceName, status));
            headerLabel->setStyleSheet(
                "QLabel {"
                "  background-color: #3498db;"
                "  color: white;"
                "  font-weight: bold;"
                "  font-size: 14px;"
                "  padding: 8px;"
                "  border-radius: 4px;"
                "  text-align: center;"
                "}"
            );
            headerLabel->setAlignment(Qt::AlignCenter);
            
            // Create table for this device
            QTableWidget* deviceTable = new QTableWidget();
            deviceTable->setColumnCount(2);
            deviceTable->setHorizontalHeaderLabels(QStringList() << tr("Property") << tr("Value"));
            deviceTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
            deviceTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
            deviceTable->setColumnWidth(0, 200);
            deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
            deviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
            deviceTable->setAlternatingRowColors(true);
            deviceTable->verticalHeader()->setVisible(false);
            
            // Remove table scrollbars and fix size to content
            deviceTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            deviceTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            deviceTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            
            // Style the header to match Storage tab
            deviceTable->horizontalHeader()->setStyleSheet(
                "QHeaderView::section { "
                "background-color: #34495e; "
                "color: white; "
                "padding: 8px; "
                "border: 1px solid #2c3e50; "
                "font-weight: bold; "
                "}"
            );
            
            // Populate device table
            populateDeviceTable(deviceTable, interface);
            
            // Add to layout with spacing
            networkInfoLayout->addWidget(headerLabel);
            networkInfoLayout->addWidget(deviceTable);
            networkInfoLayout->addSpacing(15); // More spacing like Storage tab
        }
    }
    
    QString getEnhancedDeviceName(const QNetworkInterface& interface)
    {
        QString ifName = interface.name();
        QString deviceName = ifName;
        
        // Get hardware information from PCI subsystem
        QString sysPath = QString("/sys/class/net/%1/device").arg(ifName);
        QString hwName = getHardwareDeviceName(sysPath);
        if (!hwName.isEmpty()) {
            return hwName;
        }
        
        // Enhanced USB device detection (fallback for USB adapters)
        if (ifName.startsWith("wlx")) {
            QString usbSysPath = QString("/sys/class/net/%1").arg(ifName);
            QFile usbVendor(usbSysPath + "/device/../idVendor");
            QFile usbProduct(usbSysPath + "/device/../idProduct");
            QFile usbManufacturer(usbSysPath + "/device/../manufacturer");
            QFile usbProductName(usbSysPath + "/device/../product");
            
            if (usbVendor.open(QIODevice::ReadOnly) && usbProduct.open(QIODevice::ReadOnly)) {
                QString vendor = QString::fromLatin1(usbVendor.readAll()).trimmed();
                QString product = QString::fromLatin1(usbProduct.readAll()).trimmed();
                
                // Try to get manufacturer and product strings
                QString manufacturerStr, productStr;
                if (usbManufacturer.open(QIODevice::ReadOnly)) {
                    manufacturerStr = QString::fromLatin1(usbManufacturer.readAll()).trimmed();
                }
                if (usbProductName.open(QIODevice::ReadOnly)) {
                    productStr = QString::fromLatin1(usbProductName.readAll()).trimmed();
                }
                
                // Map specific vendor:product combinations
                if (vendor == "0bda" && product == "c811") {
                    return "Realtek RTL8811CU 802.11ac USB WiFi Adapter";
                } else if (vendor == "0bda") {
                    if (!manufacturerStr.isEmpty() && !productStr.isEmpty()) {
                        return QString("%1 %2 USB WiFi Adapter").arg(manufacturerStr, productStr);
                    } else {
                        return "Realtek 802.11ac USB WiFi Adapter";
                    }
                } else if (vendor == "148f") {
                    return "Ralink USB WiFi Adapter";
                } else if (vendor == "2357") {
                    return "TP-Link USB WiFi Adapter";
                } else {
                    // Use manufacturer and product strings if available
                    if (!manufacturerStr.isEmpty() && !productStr.isEmpty()) {
                        return QString("%1 %2 USB WiFi Adapter").arg(manufacturerStr, productStr);
                    } else {
                        return QString("USB WiFi Adapter (ID %1:%2)").arg(vendor, product);
                    }
                }
            }
            return "USB WiFi Adapter";
        }
        
        // Legacy fallback patterns (should rarely be used now)
        if (ifName.startsWith("wlp") || ifName.startsWith("wlo")) {
            return "Wireless Network Adapter";
        } else if (ifName.startsWith("en") || ifName.startsWith("eth")) {
            return "Ethernet Network Adapter";
        }
        
        return deviceName;
    }

    QString getHardwareDeviceName(const QString& devicePath)
    {
        // Read PCI vendor and device IDs
        QFile vendorFile(devicePath + "/vendor");
        QFile deviceFile(devicePath + "/device");
        QFile subsysVendorFile(devicePath + "/subsystem_vendor");
        QFile subsysDeviceFile(devicePath + "/subsystem_device");
        
        if (!vendorFile.open(QIODevice::ReadOnly) || !deviceFile.open(QIODevice::ReadOnly)) {
            return QString();
        }
        
        QString vendorId = QString::fromLatin1(vendorFile.readAll()).trimmed().toLower();
        QString deviceId = QString::fromLatin1(deviceFile.readAll()).trimmed().toLower();
        
        // Remove 0x prefix if present
        if (vendorId.startsWith("0x")) vendorId = vendorId.mid(2);
        if (deviceId.startsWith("0x")) deviceId = deviceId.mid(2);
        
        // Get subsystem IDs for more specific identification
        QString subsysVendorId, subsysDeviceId;
        if (subsysVendorFile.open(QIODevice::ReadOnly) && subsysDeviceFile.open(QIODevice::ReadOnly)) {
            subsysVendorId = QString::fromLatin1(subsysVendorFile.readAll()).trimmed().toLower();
            subsysDeviceId = QString::fromLatin1(subsysDeviceFile.readAll()).trimmed().toLower();
            if (subsysVendorId.startsWith("0x")) subsysVendorId = subsysVendorId.mid(2);
            if (subsysDeviceId.startsWith("0x")) subsysDeviceId = subsysDeviceId.mid(2);
        }
        
        // Map vendor:device combinations to human-readable names
        if (vendorId == "8086") { // Intel
            if (deviceId == "51f0") return "Intel Wi-Fi 6E AX210 160MHz";
            if (deviceId == "51f1") return "Intel Wi-Fi 6E AX211 160MHz";
            if (deviceId == "2725") return "Intel Wi-Fi 6 AX210 160MHz";
            if (deviceId == "2720") return "Intel Wi-Fi 6 AX201 160MHz";
            if (deviceId == "02f0") return "Intel Comet Lake PCH CNVi WiFi";
            if (deviceId == "a0f0") return "Intel Wi-Fi 6 AX201";
            if (deviceId == "34f0") return "Intel Ice Lake-LP PCH CNVi WiFi";
            if (deviceId == "06f0") return "Intel Comet Lake PCH CNVi WiFi";
            if (deviceId == "4df0") return "Intel Tiger Lake PCH CNVi WiFi 6E AX210";
            if (deviceId == "4238") return "Intel Centrino Ultimate-N 6300";
            if (deviceId == "4239") return "Intel Centrino Advanced-N 6200";
            if (deviceId == "08b1") return "Intel Wireless 7260";
            if (deviceId == "08b2") return "Intel Wireless 7260";
            if (deviceId == "24f3") return "Intel Wireless 8260";
            if (deviceId == "24f4") return "Intel Wireless 8260";
            if (deviceId == "9df0") return "Intel Cannon Point-LP CNVi [Wireless-AC]";
            if (deviceId == "2526") return "Intel Wireless-AC 9260";
            if (deviceId == "271b") return "Intel Dual Band Wireless-AC 9560";
            if (deviceId == "271c") return "Intel Dual Band Wireless-AC 9560";
            // Ethernet controllers
            if (deviceId == "15b7") return "Intel Ethernet Connection (2) I219-LM";
            if (deviceId == "15b8") return "Intel Ethernet Connection (2) I219-V";
            if (deviceId == "15d7") return "Intel Ethernet Connection (4) I219-LM";
            if (deviceId == "15d8") return "Intel Ethernet Connection (4) I219-V";
            if (deviceId == "15e3") return "Intel Ethernet Connection (5) I219-LM";
            if (deviceId == "1570") return "Intel Ethernet Connection (12) I219-V";
            // Generic Intel WiFi/Ethernet fallback
            return QString("Intel Network Adapter (ID %1:%2)").arg(vendorId, deviceId);
        }
        else if (vendorId == "10ec") { // Realtek
            if (deviceId == "8168") return "Realtek RTL8111/8168/8211/8411 PCI Express Gigabit Ethernet Controller";
            if (deviceId == "8125") return "Realtek RTL8125 2.5GbE Controller";
            if (deviceId == "8169") return "Realtek RTL8169 PCI Gigabit Ethernet Controller";
            if (deviceId == "8139") return "Realtek RTL-8100/8101L/8139 PCI Fast Ethernet Adapter";
            if (deviceId == "8821") return "Realtek RTL8821CE 802.11ac PCIe Wireless Network Adapter";
            if (deviceId == "8822") return "Realtek RTL8822CE 802.11ac PCIe Wireless Network Adapter";
            if (deviceId == "c821") return "Realtek RTL8821CE 802.11ac PCIe Wireless Network Adapter";
            if (deviceId == "c822") return "Realtek RTL8822CE 802.11ac PCIe Wireless Network Adapter";
            return QString("Realtek Network Adapter (ID %1:%2)").arg(vendorId, deviceId);
        }
        else if (vendorId == "14e4") { // Broadcom
            if (deviceId == "43a0") return "Broadcom BCM4360 802.11ac Wireless Network Adapter";
            if (deviceId == "43a3") return "Broadcom BCM4350 802.11ac Wireless Network Adapter";
            if (deviceId == "4331") return "Broadcom BCM4331 802.11a/b/g/n";
            if (deviceId == "4353") return "Broadcom BCM4353 802.11ac Wireless Network Adapter";
            return QString("Broadcom Network Adapter (ID %1:%2)").arg(vendorId, deviceId);
        }
        else if (vendorId == "1969") { // Atheros/Qualcomm
            if (deviceId == "1063") return "Atheros AR8131 Gigabit Ethernet";
            if (deviceId == "1062") return "Atheros AR8132 Fast Ethernet";
            if (deviceId == "2062") return "Atheros AR8152 v2.0 Fast Ethernet";
            if (deviceId == "1073") return "Atheros AR8151 v1.0 Gigabit Ethernet";
            return QString("Atheros Network Adapter (ID %1:%2)").arg(vendorId, deviceId);
        }
        else if (vendorId == "168c") { // Qualcomm Atheros WiFi
            if (deviceId == "0042") return "Qualcomm Atheros QCA9377 802.11ac Wireless Network Adapter";
            if (deviceId == "0032") return "Qualcomm Atheros AR9485 Wireless Network Adapter";
            if (deviceId == "0034") return "Qualcomm Atheros AR9462 Wireless Network Adapter";
            if (deviceId == "003e") return "Qualcomm Atheros QCA6174 802.11ac Wireless Network Adapter";
            return QString("Qualcomm Atheros WiFi Adapter (ID %1:%2)").arg(vendorId, deviceId);
        }
        else if (vendorId == "11ab") { // Marvell
            if (deviceId == "4362") return "Marvell 88W8363 [TopDog] 802.11n Wireless";
            if (deviceId == "4320") return "Marvell 88W8363 [TopDog] 802.11n Wireless";
            return QString("Marvell Network Adapter (ID %1:%2)").arg(vendorId, deviceId);
        }
        
        // Generic fallback with vendor name
        QString vendorName = getVendorName(vendorId);
        return QString("%1 Network Adapter (ID %2:%3)").arg(vendorName, vendorId, deviceId);
    }

    QString getVendorName(const QString& vendorId)
    {
        if (vendorId == "8086") return "Intel";
        if (vendorId == "10ec") return "Realtek";
        if (vendorId == "14e4") return "Broadcom";
        if (vendorId == "1969") return "Atheros";
        if (vendorId == "168c") return "Qualcomm Atheros";
        if (vendorId == "11ab") return "Marvell";
        if (vendorId == "1022") return "AMD";
        if (vendorId == "10de") return "NVIDIA";
        if (vendorId == "1106") return "VIA";
        if (vendorId == "1039") return "SiS";
        return "Unknown Vendor";
    }
    
    QString getDeviceStatus(const QNetworkInterface& interface)
    {
        if (interface.flags() & QNetworkInterface::IsUp && interface.flags() & QNetworkInterface::IsRunning) {
            return "(Active)";
        } else if (interface.flags() & QNetworkInterface::IsUp) {
            return "(Up but not connected)";
        } else {
            return "(Inactive)";
        }
    }
    
    void populateDeviceTable(QTableWidget* table, const QNetworkInterface& interface)
    {
        int row = 0;
        
        auto addRow = [&](const QString& property, const QString& value) {
            table->insertRow(row);
            QTableWidgetItem* propItem = new QTableWidgetItem(property);
            QFont boldFont;
            boldFont.setBold(true);
            propItem->setFont(boldFont);
            table->setItem(row, 0, propItem);
            table->setItem(row, 1, new QTableWidgetItem(value));
            row++;
        };
        
        // Interface details
        addRow(tr("Interface Name"), interface.name());
        
        // MAC Address
        QString macAddress = interface.hardwareAddress();
        if (!macAddress.isEmpty()) {
            addRow(tr("MAC Address"), macAddress);
        }
        
        // IP Addresses
        QList<QNetworkAddressEntry> addresses = interface.addressEntries();
        for (const QNetworkAddressEntry& entry : addresses) {
            QHostAddress ip = entry.ip();
            if (ip.protocol() == QAbstractSocket::IPv4Protocol) {
                addRow(tr("IPv4 Address"), ip.toString());
            } else if (ip.protocol() == QAbstractSocket::IPv6Protocol) {
                addRow(tr("IPv6 Address"), ip.toString());
            }
        }
        
        // MTU
        int mtu = interface.maximumTransmissionUnit();
        if (mtu > 0) {
            addRow(tr("MTU"), tr("%1 bytes").arg(mtu));
        }
        
        // Get default gateway for active interfaces
        if (interface.flags() & QNetworkInterface::IsUp && interface.flags() & QNetworkInterface::IsRunning) {
            QProcess routeProcess;
            routeProcess.start("ip", QStringList() << "route" << "show" << "default" << "dev" << interface.name());
            routeProcess.waitForFinished(2000);
            QString routeOutput = routeProcess.readAllStandardOutput();
            
            QRegularExpression gwRx("default via ([0-9.]+)");
            QRegularExpressionMatch gwMatch = gwRx.match(routeOutput);
            if (gwMatch.hasMatch()) {
                addRow(tr("Default Gateway"), gwMatch.captured(1));
            }
        }
        
        // Resize table to fit content exactly
        table->resizeRowsToContents();
        table->setFixedHeight(table->verticalHeader()->length() + table->horizontalHeader()->height() + 2);
    }

    void parseOutput(const QString& output) override { Q_UNUSED(output); }
};

#endif // NETWORK_TAB_H

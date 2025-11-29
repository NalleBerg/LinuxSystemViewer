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
            deviceTable->setHorizontalHeaderLabels(QStringList() << "Property" << "Value");
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
        
        // Enhanced USB device detection
        if (ifName.startsWith("wlx")) {
            QString sysPath = QString("/sys/class/net/%1").arg(ifName);
            QFile usbVendor(sysPath + "/device/../idVendor");
            QFile usbProduct(sysPath + "/device/../idProduct");
            QFile usbManufacturer(sysPath + "/device/../manufacturer");
            QFile usbProductName(sysPath + "/device/../product");
            
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
        } else if (ifName.startsWith("wlp")) {
            return "Broadcom 802.11ac Wireless Network Adapter";
        } else if (ifName.startsWith("en") || ifName.startsWith("eth")) {
            return "Ethernet Network Controller";
        }
        
        return deviceName;
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
        addRow("Interface Name", interface.name());
        
        // MAC Address
        QString macAddress = interface.hardwareAddress();
        if (!macAddress.isEmpty()) {
            addRow("MAC Address", macAddress);
        }
        
        // IP Addresses
        QList<QNetworkAddressEntry> addresses = interface.addressEntries();
        for (const QNetworkAddressEntry& entry : addresses) {
            QHostAddress ip = entry.ip();
            if (ip.protocol() == QAbstractSocket::IPv4Protocol) {
                addRow("IPv4 Address", ip.toString());
            } else if (ip.protocol() == QAbstractSocket::IPv6Protocol) {
                addRow("IPv6 Address", ip.toString());
            }
        }
        
        // MTU
        int mtu = interface.maximumTransmissionUnit();
        if (mtu > 0) {
            addRow("MTU", QString("%1 bytes").arg(mtu));
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
                addRow("Default Gateway", gwMatch.captured(1));
            }
        }
        
        // Resize table to fit content exactly
        table->resizeRowsToContents();
        table->setFixedHeight(table->verticalHeader()->length() + table->horizontalHeader()->height() + 2);
    }

    void parseOutput(const QString& output) override { Q_UNUSED(output); }
};

#endif // NETWORK_TAB_H

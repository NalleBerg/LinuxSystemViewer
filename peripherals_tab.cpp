#include "peripherals_tab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QGroupBox>
#include <QFont>
#include <QRegularExpression>
#include <QDebug>

PeripheralsTab::PeripheralsTab(QWidget* parent)
    : TabWidgetBase("Peripherals", "lsusb && lspci -nn | head -10", true, 
                    "lsusb -v && lspci -vv && lsblk && cat /proc/bus/input/devices", parent)
{
    qDebug() << "PeripheralsTab: Constructor called - base constructor done";
    initializeTab();
    qDebug() << "PeripheralsTab: Constructor finished";
}

QWidget* PeripheralsTab::createUserFriendlyView()
{
    qDebug() << "PeripheralsTab: createUserFriendlyView called";
    
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    QWidget* contentWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    QLabel* titleLabel = new QLabel("Connected Peripherals and Devices");
    titleLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  color: #2c3e50;"
        "  margin-bottom: 10px;"
        "}"
    );
    mainLayout->addWidget(titleLabel);
    
    createInfoSection("USB Devices", &m_usbDevicesSection, &m_usbDevicesContent, mainLayout);
    createInfoSection("Input Devices", &m_inputDevicesSection, &m_inputDevicesContent, mainLayout);
    createInfoSection("Storage Devices", &m_storageDevicesSection, &m_storageDevicesContent, mainLayout);
    createInfoSection("Network Devices", &m_networkDevicesSection, &m_networkDevicesContent, mainLayout);
    
    mainLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);
    
    qDebug() << "PeripheralsTab: createUserFriendlyView completed";
    return scrollArea;
}

void PeripheralsTab::createInfoSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout)
{
    *groupBox = new QGroupBox(title);
    (*groupBox)->setStyleSheet(
        "QGroupBox {"
        "  font-weight: bold;"
        "  border: 2px solid #bdc3c7;"
        "  border-radius: 8px;"
        "  margin-top: 10px;"
        "  padding-top: 10px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 10px;"
        "  padding: 0 10px 0 10px;"
        "}"
    );
    
    QVBoxLayout* sectionLayout = new QVBoxLayout(*groupBox);
    *contentLabel = new QLabel("Loading " + title.toLower() + " information...");
    (*contentLabel)->setWordWrap(true);
    (*contentLabel)->setStyleSheet("QLabel { padding: 10px; background-color: #f8f9fa; border-radius: 4px; }");
    sectionLayout->addWidget(*contentLabel);
    
    parentLayout->addWidget(*groupBox);
}

void PeripheralsTab::parseOutput(const QString& output)
{
    qDebug() << "PeripheralsTab: parseOutput called";
    
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    QString usbDevicesInfo = "USB Devices: Not detected";
    QString inputDevicesInfo = "Input Devices: Not detected";
    QString storageDevicesInfo = "Storage Devices: Not detected";
    QString networkDevicesInfo = "Network Devices: Not detected";
    
    QStringList usbDevices;
    QStringList inputDevices;
    QStringList storageDevices;
    QStringList networkDevices;
    
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        
        // Parse lsusb output
        if (trimmed.startsWith("Bus ") && trimmed.contains("Device ")) {
            QRegularExpression usbRegex("Bus\\s+\\d+\\s+Device\\s+\\d+:\\s+ID\\s+[0-9a-fA-F:]+\\s+(.+)");
            QRegularExpressionMatch usbMatch = usbRegex.match(trimmed);
            if (usbMatch.hasMatch()) {
                QString deviceName = usbMatch.captured(1);
                usbDevices.append(deviceName);
                
                // Categorize USB devices
                if (deviceName.contains("keyboard", Qt::CaseInsensitive) ||
                    deviceName.contains("mouse", Qt::CaseInsensitive) ||
                    deviceName.contains("trackpad", Qt::CaseInsensitive) ||
                    deviceName.contains("touchscreen", Qt::CaseInsensitive)) {
                    inputDevices.append(deviceName + " (USB)");
                }
                
                if (deviceName.contains("storage", Qt::CaseInsensitive) ||
                    deviceName.contains("drive", Qt::CaseInsensitive) ||
                    deviceName.contains("disk", Qt::CaseInsensitive) ||
                    deviceName.contains("flash", Qt::CaseInsensitive)) {
                    storageDevices.append(deviceName + " (USB)");
                }
                
                if (deviceName.contains("ethernet", Qt::CaseInsensitive) ||
                    deviceName.contains("wireless", Qt::CaseInsensitive) ||
                    deviceName.contains("wifi", Qt::CaseInsensitive) ||
                    deviceName.contains("bluetooth", Qt::CaseInsensitive)) {
                    networkDevices.append(deviceName + " (USB)");
                }
            }
        }
        
        // Parse lspci output
        if (trimmed.contains(":") && !trimmed.startsWith("Bus")) {
            QStringList parts = trimmed.split(':', Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                QString deviceDesc = parts[1].trimmed();
                
                if (deviceDesc.contains("Ethernet", Qt::CaseInsensitive) ||
                    deviceDesc.contains("Wireless", Qt::CaseInsensitive) ||
                    deviceDesc.contains("Network", Qt::CaseInsensitive) ||
                    deviceDesc.contains("WiFi", Qt::CaseInsensitive)) {
                    networkDevices.append(deviceDesc + " (PCI)");
                }
                
                if (deviceDesc.contains("Storage", Qt::CaseInsensitive) ||
                    deviceDesc.contains("SATA", Qt::CaseInsensitive) ||
                    deviceDesc.contains("RAID", Qt::CaseInsensitive) ||
                    deviceDesc.contains("IDE", Qt::CaseInsensitive)) {
                    storageDevices.append(deviceDesc + " (PCI)");
                }
            }
        }
        
        // Parse lsblk output for storage devices
        if ((trimmed.startsWith("sd") || trimmed.startsWith("nvme") || 
             trimmed.startsWith("hd") || trimmed.startsWith("mmcblk")) &&
            trimmed.contains("disk")) {
            QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 4) {
                QString device = parts[0];
                QString size = parts[3];
                storageDevices.append(device + " (" + size + ")");
            }
        }
        
        // Parse /proc/bus/input/devices
        if (trimmed.startsWith("N: Name=")) {
            QString deviceName = trimmed.split("Name=")[1].trimmed().remove('"');
            if (!deviceName.isEmpty()) {
                inputDevices.append(deviceName);
            }
        }
    }
    
    // Remove duplicates
    usbDevices.removeDuplicates();
    inputDevices.removeDuplicates();
    storageDevices.removeDuplicates();
    networkDevices.removeDuplicates();
    
    // Format the information
    if (!usbDevices.isEmpty()) {
        usbDevicesInfo = "USB Devices:\n" + usbDevices.join("\n");
    }
    
    if (!inputDevices.isEmpty()) {
        inputDevicesInfo = "Input Devices:\n" + inputDevices.join("\n");
    }
    
    if (!storageDevices.isEmpty()) {
        storageDevicesInfo = "Storage Devices:\n" + storageDevices.join("\n");
    }
    
    if (!networkDevices.isEmpty()) {
        networkDevicesInfo = "Network Devices:\n" + networkDevices.join("\n");
    }
    
    // Update the UI with parsed information
    if (m_usbDevicesContent) {
        m_usbDevicesContent->setText(usbDevicesInfo);
    }
    if (m_inputDevicesContent) {
        m_inputDevicesContent->setText(inputDevicesInfo);
    }
    if (m_storageDevicesContent) {
        m_storageDevicesContent->setText(storageDevicesInfo);
    }
    if (m_networkDevicesContent) {
        m_networkDevicesContent->setText(networkDevicesInfo);
    }
    
    qDebug() << "PeripheralsTab: parseOutput completed";
}
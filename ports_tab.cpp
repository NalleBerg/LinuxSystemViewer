#include "ports_tab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QGroupBox>
#include <QFont>
#include <QRegularExpression>
#include <QDebug>
#include <QCoreApplication>

PortsTab::PortsTab(QWidget* parent)
    : TabWidgetBase(QCoreApplication::translate("PortsTab", "Ports"), "lsusb -t && lspci | grep -i 'serial\\|usb'", true, 
                    "lsusb -v && lspci -v | grep -A5 -i 'serial\\|usb' && dmesg | grep -i usb | tail -10", parent)
{
    qDebug() << "PortsTab: Constructor called - base constructor done";
    initializeTab();
    qDebug() << "PortsTab: Constructor finished";
}

QWidget* PortsTab::createUserFriendlyView()
{
    qDebug() << "PortsTab: createUserFriendlyView called";
    
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    QWidget* contentWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    QLabel* titleLabel = new QLabel(QCoreApplication::translate("PortsTab", "System Ports Information"));
    titleLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  color: #2c3e50;"
        "  margin-bottom: 10px;"
        "}"
    );
    mainLayout->addWidget(titleLabel);
    
    createInfoSection(QCoreApplication::translate("PortsTab", "USB Ports"), &m_usbPortsSection, &m_usbPortsContent, mainLayout);
    createInfoSection(QCoreApplication::translate("PortsTab", "Serial Ports"), &m_serialPortsSection, &m_serialPortsContent, mainLayout);
    createInfoSection(QCoreApplication::translate("PortsTab", "PCI Ports"), &m_pciPortsSection, &m_pciPortsContent, mainLayout);
    createInfoSection(QCoreApplication::translate("PortsTab", "Port Status"), &m_portStatusSection, &m_portStatusContent, mainLayout);
    
    mainLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);
    
    qDebug() << "PortsTab: createUserFriendlyView completed";
    return scrollArea;
}

void PortsTab::createInfoSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout)
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
    *contentLabel = new QLabel(QCoreApplication::translate("PortsTab", "Loading %1 information...").arg(title.toLower()));
    (*contentLabel)->setWordWrap(true);
    (*contentLabel)->setStyleSheet("QLabel { padding: 10px; background-color: #f8f9fa; border-radius: 4px; }");
    sectionLayout->addWidget(*contentLabel);
    
    parentLayout->addWidget(*groupBox);
}

void PortsTab::parseOutput(const QString& output)
{
    qDebug() << "PortsTab: parseOutput called";
    
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    QString usbPortsInfo = QCoreApplication::translate("PortsTab", "USB Ports: Not detected");
    QString serialPortsInfo = QCoreApplication::translate("PortsTab", "Serial Ports: Not detected");
    QString pciPortsInfo = QCoreApplication::translate("PortsTab", "PCI Ports: Not detected");
    QString portStatusInfo = QCoreApplication::translate("PortsTab", "Port Status: Unknown");
    
    QStringList usbPorts;
    QStringList serialPorts;
    QStringList pciPorts;
    QStringList portStatus;
    
    bool inUsbTree = false;
    int usbHubCount = 0;
    int usbDeviceCount = 0;
    
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        
        // Parse lsusb -t output (USB tree)
        if (trimmed.startsWith("/:") || trimmed.startsWith("|__") || trimmed.startsWith("`__")) {
            inUsbTree = true;
            
            if (trimmed.startsWith("/:")) {
                // USB hub/root
                QRegularExpression hubRegex("Bus\\s+(\\d+)");
                QRegularExpressionMatch hubMatch = hubRegex.match(trimmed);
                if (hubMatch.hasMatch()) {
                    QString busNum = hubMatch.captured(1);
                    usbPorts.append("USB Bus " + busNum + " (Root Hub)");
                    usbHubCount++;
                }
            } else if (trimmed.contains("Class=")) {
                // USB device
                QRegularExpression devRegex("Dev\\s+(\\d+).*?Class=([^,]+)");
                QRegularExpressionMatch devMatch = devRegex.match(trimmed);
                if (devMatch.hasMatch()) {
                    QString devNum = devMatch.captured(1);
                    QString devClass = devMatch.captured(2);
                    usbPorts.append("  Device " + devNum + " (" + devClass.trimmed() + ")");
                    usbDeviceCount++;
                }
            }
        }
        
        // Parse lspci output for serial and USB controllers
        if (trimmed.contains("Serial controller") || trimmed.contains("serial", Qt::CaseInsensitive)) {
            serialPorts.append(trimmed);
        }
        
        if (trimmed.contains("USB controller") || trimmed.contains("USB", Qt::CaseInsensitive)) {
            pciPorts.append(trimmed);
        }
        
        // Parse other PCI devices that might be relevant
        if (trimmed.contains("Communication controller") || 
            trimmed.contains("Bridge") ||
            trimmed.contains("Host bridge")) {
            pciPorts.append(trimmed);
        }
    }
    
    // Generate port status summary
    portStatus.append("USB Hubs: " + QString::number(usbHubCount));
    portStatus.append("USB Devices: " + QString::number(usbDeviceCount));
    portStatus.append("Serial Controllers: " + QString::number(serialPorts.size()));
    portStatus.append("USB Controllers: " + QString::number(pciPorts.count("USB")));
    
    // Format the information
    if (!usbPorts.isEmpty()) {
        usbPortsInfo = QCoreApplication::translate("PortsTab", "USB Ports:\n") + usbPorts.join("\n");
    }
    
    if (!serialPorts.isEmpty()) {
        serialPortsInfo = QCoreApplication::translate("PortsTab", "Serial Ports:\n") + serialPorts.join("\n");
    } else {
        serialPortsInfo = QCoreApplication::translate("PortsTab", "Serial Ports:\nNo serial controllers detected");
    }
    
    if (!pciPorts.isEmpty()) {
        pciPortsInfo = QCoreApplication::translate("PortsTab", "PCI Ports:\n") + pciPorts.join("\n");
    }
    
    if (!portStatus.isEmpty()) {
        portStatusInfo = QCoreApplication::translate("PortsTab", "Port Status:\n") + portStatus.join("\n");
    }
    
    // Update the UI with parsed information
    if (m_usbPortsContent) {
        m_usbPortsContent->setText(usbPortsInfo);
    }
    if (m_serialPortsContent) {
        m_serialPortsContent->setText(serialPortsInfo);
    }
    if (m_pciPortsContent) {
        m_pciPortsContent->setText(pciPortsInfo);
    }
    if (m_portStatusContent) {
        m_portStatusContent->setText(portStatusInfo);
    }
    
    qDebug() << "PortsTab: parseOutput completed";
}
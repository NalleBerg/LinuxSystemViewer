#include "summary_tab.h"
#include "gui_helpers.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QScrollArea>
#include <QFont>
#include <QStorageInfo>
#include <QScreen>
#include <QGuiApplication>

SummaryTab::SummaryTab(QWidget* parent)
    : QWidget(parent)
{
    // Headline (no Geek button for Summary)
    QHBoxLayout* headlineLayout = new QHBoxLayout();
    QLabel* headline = new QLabel(tr("Summary"));
    headline->setStyleSheet("font-size:18px; font-weight:bold; color:#2c3e50;");
    headlineLayout->addWidget(headline);
    headlineLayout->addStretch();

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    applyMainLayoutDefaults(mainLayout);
    mainLayout->addLayout(headlineLayout);

    // Table
    tableWidget = new QTableWidget();
    tableWidget->setColumnCount(2);
    tableWidget->setHorizontalHeaderLabels({tr("Property"), tr("Value")});
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->horizontalHeader()->setStyleSheet(
        "QHeaderView::section { "
        "background-color: #2c3e50; "
        "color: white; "
        "padding: 8px; "
        "border: none; "
        "font-weight: bold; "
        "}"
    );
    tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    tableWidget->setColumnWidth(0, 220);  // Match CPU tab width
    tableWidget->setWordWrap(true);
    tableWidget->setAlternatingRowColors(true);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Scroll area
    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidget(tableWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(220);
    mainLayout->addWidget(scrollArea);

    // Enable copy
    enableTableCopy(tableWidget);

    // Load information
    loadSummaryInformation();
}

void SummaryTab::loadSummaryInformation()
{
    tableWidget->setRowCount(0);
    
    auto addRow = [this](const QString& property, const QString& value) {
        if (value.isEmpty()) return;
        int row = tableWidget->rowCount();
        tableWidget->insertRow(row);
        
        QTableWidgetItem* propItem = new QTableWidgetItem(property);
        QFont boldFont;
        boldFont.setBold(true);
        propItem->setFont(boldFont);
        tableWidget->setItem(row, 0, propItem);
        
        QString displayVal = value;
        displayVal.replace("\n", " ↳ ");
        
        QTableWidgetItem* valItem = new QTableWidgetItem(displayVal);
        tableWidget->setItem(row, 1, valItem);
        tableWidget->resizeRowToContents(row);
    };
    
    // === SYSTEM / PC INFO ===
    auto readFile = [](const QString& path) -> QString {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QTextStream(&f).readLine().trimmed();
            f.close();
            return content;
        }
        return QString();
    };
    
    QString computerName = readFile("/sys/class/dmi/id/product_name");
    if (!computerName.isEmpty() && computerName != "System Product Name" && computerName != "To be filled by O.E.M.") {
        addRow(tr("Computer"), computerName);
    }
    
    QString manufacturer = readFile("/sys/class/dmi/id/sys_vendor");
    if (!manufacturer.isEmpty() && manufacturer != "System manufacturer" && manufacturer != "To be filled by O.E.M.") {
        addRow(tr("Manufacturer"), manufacturer);
    }
    
    // === CPU INFO ===
    QFile cpuInfo("/proc/cpuinfo");
    if (cpuInfo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&cpuInfo);
        QString cpuModel;
        int coreCount = 0;
        
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            if (line.startsWith("model name") && cpuModel.isEmpty()) {
                cpuModel = line.section(':', 1).trimmed();
            }
            if (line.startsWith("processor")) {
                coreCount++;
            }
        }
        cpuInfo.close();
        
        if (!cpuModel.isEmpty()) {
            addRow(tr("Processor"), cpuModel);
            addRow(tr("CPU Cores"), QString::number(coreCount));
        }
    }
    
    // === MEMORY INFO ===
    QFile memInfo("/proc/meminfo");
    if (memInfo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&memInfo);
        QString line = stream.readLine();
        if (line.startsWith("MemTotal:")) {
            QString memStr = line.section(':', 1).trimmed();
            long long memKB = memStr.section(' ', 0, 0).toLongLong();
            double memGB = memKB / (1024.0 * 1024.0);
            addRow(tr("Total Memory"), QString("%1 GB").arg(QString::number(memGB, 'f', 1)));
        }
        memInfo.close();
    }
    
    // === UPTIME ===
    QFile uptimeFile("/proc/uptime");
    if (uptimeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString uptimeStr = QTextStream(&uptimeFile).readLine().section(' ', 0, 0);
        uptimeFile.close();
        
        double seconds = uptimeStr.toDouble();
        int days = static_cast<int>(seconds) / 86400;
        int hours = (static_cast<int>(seconds) % 86400) / 3600;
        int minutes = (static_cast<int>(seconds) % 3600) / 60;
        
        QString formattedUptime;
        if (days > 0) formattedUptime += QString("%1 d, ").arg(days);
        if (hours > 0) formattedUptime += QString("%1 h, ").arg(hours);
        formattedUptime += QString("%1 m").arg(minutes);
        
        addRow(tr("System Uptime"), formattedUptime);
    }
    
    // === STORAGE INFO ===
    QList<QStorageInfo> volumes = QStorageInfo::mountedVolumes();
    long long totalStorage = 0;
    int storageCount = 0;
    
    for (const QStorageInfo& storage : volumes) {
        if (storage.isValid() && !storage.isReadOnly()) {
            QString device = storage.device();
            QString fsType = storage.fileSystemType();
            
            if (!device.startsWith("/dev/loop") && 
                !device.startsWith("/dev/snap") && 
                fsType != "tmpfs" && 
                fsType != "devtmpfs") {
                totalStorage += storage.bytesTotal();
                storageCount++;
            }
        }
    }
    
    if (storageCount > 0) {
        addRow(tr("Storage Devices"), QString::number(storageCount));
        addRow(tr("Total Storage"), QString("%1 GB").arg(QString::number(totalStorage / (1024.0 * 1024.0 * 1024.0), 'f', 1)));
    }
    
    // === GRAPHICS INFO ===
    QDir drmDir("/sys/class/drm");
    if (drmDir.exists()) {
        QStringList cards = drmDir.entryList(QStringList() << "card?", QDir::Dirs);
        QStringList gpuNames;
        
        for (const QString& card : cards) {
            QDir deviceDir(QString("/sys/class/drm/%1/device").arg(card));
            if (deviceDir.exists()) {
                QFile vendorFile(deviceDir.filePath("vendor"));
                QFile deviceFile(deviceDir.filePath("device"));
                
                if (vendorFile.open(QIODevice::ReadOnly) && deviceFile.open(QIODevice::ReadOnly)) {
                    QString vendor = QTextStream(&vendorFile).readLine().trimmed();
                    QString device = QTextStream(&deviceFile).readLine().trimmed();
                    vendorFile.close();
                    deviceFile.close();
                    
                    // Try to get friendly name from uevent
                    QFile ueventFile(deviceDir.filePath("uevent"));
                    QString gpuName = QString("%1:%2").arg(vendor, device);
                    
                    if (ueventFile.open(QIODevice::ReadOnly)) {
                        QString uevent = QTextStream(&ueventFile).readAll();
                        ueventFile.close();
                        
                        if (uevent.contains("PCI_SLOT_NAME")) {
                            QString slot = uevent.section("PCI_SLOT_NAME=", 1).section('\n', 0, 0);
                            if (!slot.isEmpty()) {
                                gpuName = slot;
                            }
                        }
                    }
                    
                    gpuNames.append(gpuName);
                }
            }
        }
        
        if (!gpuNames.isEmpty()) {
            addRow(tr("Graphics Cards"), QString::number(gpuNames.count()));
        }
    }
    
    // === SCREEN INFO ===
    QList<QScreen*> screens = QGuiApplication::screens();
    if (!screens.isEmpty()) {
        QScreen* primaryScreen = screens.first();
        QSize resolution = primaryScreen->size();
        qreal refreshRate = primaryScreen->refreshRate();
        
        addRow(tr("Display Resolution"), QString("%1 x %2").arg(resolution.width()).arg(resolution.height()));
        addRow(tr("Refresh Rate"), QString("%1 Hz").arg(QString::number(refreshRate, 'f', 1)));
        
        if (screens.count() > 1) {
            addRow(tr("Number of Screens"), QString::number(screens.count()));
        }
    }
    
    // === NETWORK INFO ===
    QDir netDir("/sys/class/net");
    if (netDir.exists()) {
        QStringList interfaces = netDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        int ethCount = 0, wlanCount = 0;
        
        for (const QString& iface : interfaces) {
            if (iface == "lo") continue;
            if (iface.startsWith("eth") || iface.startsWith("enp")) ethCount++;
            else if (iface.startsWith("wlan") || iface.startsWith("wlp")) wlanCount++;
        }
        
        if (ethCount > 0) addRow(tr("Ethernet Interfaces"), QString::number(ethCount));
        if (wlanCount > 0) addRow(tr("Wireless Interfaces"), QString::number(wlanCount));
    }
    
    // === USB INFO ===
    QDir usbDir("/sys/bus/usb/devices");
    if (usbDir.exists()) {
        QStringList usbDevices = usbDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        int deviceCount = 0;
        
        for (const QString& dev : usbDevices) {
            QFile prodFile(QString("/sys/bus/usb/devices/%1/product").arg(dev));
            if (prodFile.exists() && dev.contains(':')) {
                deviceCount++;
            }
        }
        
        if (deviceCount > 0) {
            addRow(tr("USB Devices Connected"), QString::number(deviceCount));
        }
    }
    
    // === OS INFO ===
    QFile osRelease("/etc/os-release");
    if (osRelease.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&osRelease);
        QString osName, osVersion;
        
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            if (line.startsWith("PRETTY_NAME=")) {
                osName = line.section('=', 1).trimmed();
                osName.remove('"');
            } else if (line.startsWith("VERSION=")) {
                osVersion = line.section('=', 1).trimmed();
                osVersion.remove('"');
            }
        }
        osRelease.close();
        
        if (!osName.isEmpty()) {
            addRow(tr("Operating System"), osName);
        }
    }
    
    // === KERNEL INFO ===
    QFile kernelVersion("/proc/version");
    if (kernelVersion.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString version = QTextStream(&kernelVersion).readLine();
        kernelVersion.close();
        
        if (version.contains("Linux version")) {
            QString kernel = version.section("Linux version ", 1).section(' ', 0, 0);
            addRow(tr("Kernel Version"), kernel);
        }
    }
}
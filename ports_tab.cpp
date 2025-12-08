#include "ports_tab.h"
#include "gui_helpers.h"
#include "geek_search_integration.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QScrollArea>
#include <QFont>
#include <QClipboard>
#include <QGuiApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QShowEvent>

PortsTab::PortsTab(QWidget* parent)
    : QWidget(parent)
{
    // Headline and Geek button
    QPushButton* gb = nullptr;
    QHBoxLayout* headlineLayout = createHeadlineWithGeek(this, tr("Ports"), &gb);
    geekButton = gb;
    connect(geekButton, &QPushButton::clicked, this, &PortsTab::showGeekMode);

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
    loadPortsInformation();
}

void PortsTab::loadPortsInformation()
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
    
    // Count USB devices
    int usbDeviceCount = 0;
    int usbHubCount = 0;
    QDir usbDir("/sys/bus/usb/devices");
    if (usbDir.exists()) {
        QStringList usbDevices = usbDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& dev : usbDevices) {
            QFile devFile(QString("/sys/bus/usb/devices/%1/product").arg(dev));
            if (devFile.exists()) {
                if (dev.contains(':')) {
                    usbDeviceCount++;
                } else {
                    usbHubCount++;
                }
            }
        }
    }
    
    addRow(tr("USB Buses/Hubs"), QString::number(usbHubCount));
    addRow(tr("USB Devices Connected"), QString::number(usbDeviceCount));
    
    // Enhanced TTY/Serial device detection (only show connected hardware)
    QStringList connectedSerialDevices;
    QDir serialDir("/sys/class/tty");
    if (serialDir.exists()) {
        QStringList serials = serialDir.entryList(QStringList() << "ttyUSB*" << "ttyACM*", QDir::Dirs);
        
        for (const QString& ttyDev : serials) {
            QString devicePath = QString("/sys/class/tty/%1/device").arg(ttyDev);
            QDir deviceDir(devicePath);
            
            if (deviceDir.exists()) {
                QString realDeviceName = ttyDev;
                
                // Check for USB serial device info
                QString vendorPath = devicePath + "/../idVendor";
                QString productPath = devicePath + "/../idProduct";
                QString manufacturerPath = devicePath + "/../manufacturer";
                QString productNamePath = devicePath + "/../product";
                
                QFile vendorFile(vendorPath);
                QFile productFile(productPath);
                QFile manufacturerFile(manufacturerPath);
                QFile productNameFile(productNamePath);
                
                QString vendor, product, manufacturer, productName;
                
                if (vendorFile.open(QIODevice::ReadOnly)) {
                    vendor = QString::fromLatin1(vendorFile.readAll()).trimmed();
                    vendorFile.close();
                }
                if (productFile.open(QIODevice::ReadOnly)) {
                    product = QString::fromLatin1(productFile.readAll()).trimmed();
                    productFile.close();
                }
                if (manufacturerFile.open(QIODevice::ReadOnly)) {
                    manufacturer = QString::fromLatin1(manufacturerFile.readAll()).trimmed();
                    manufacturerFile.close();
                }
                if (productNameFile.open(QIODevice::ReadOnly)) {
                    productName = QString::fromLatin1(productNameFile.readAll()).trimmed();
                    productNameFile.close();
                }
                
                // Map known vendor/product combinations to friendly names
                if (vendor == "2341") { // Arduino
                    if (product == "0043" || product == "0001") {
                        realDeviceName = "Arduino Uno R3 (USB Serial)";
                    } else if (product == "8036") {
                        realDeviceName = "Arduino Leonardo (USB Serial)";
                    } else if (product == "0042") {
                        realDeviceName = "Arduino Mega 2560 (USB Serial)";
                    } else {
                        realDeviceName = "Arduino Board (USB Serial)";
                    }
                } else if (vendor == "1a86") { // CH340/CH341 (common on Arduino clones)
                    realDeviceName = "Arduino Compatible (CH340 USB Serial)";
                } else if (vendor == "0403") { // FTDI
                    if (product == "6001") {
                        realDeviceName = "FTDI USB Serial Adapter";
                    } else {
                        realDeviceName = "FTDI USB to Serial";
                    }
                } else if (vendor == "10c4") { // Silicon Labs
                    realDeviceName = "Silicon Labs USB to UART Bridge";
                } else if (vendor == "067b") { // Prolific
                    realDeviceName = "Prolific USB to Serial Adapter";
                } else if (vendor == "2e8a") { // Raspberry Pi Foundation
                    realDeviceName = "Raspberry Pi Pico (MicroPython)";
                } else if (!manufacturer.isEmpty() && !productName.isEmpty()) {
                    realDeviceName = QString("%1 %2").arg(manufacturer, productName);
                } else if (!productName.isEmpty()) {
                    realDeviceName = productName;
                }
                
                connectedSerialDevices.append(QString("%1 (/dev/%2)").arg(realDeviceName, ttyDev));
            }
        }
    }
    
    // Add serial devices to the table
    if (!connectedSerialDevices.isEmpty()) {
        for (const QString& device : connectedSerialDevices) {
            addRow(tr("Serial Device"), device);
        }
    }
    
    // Count PCI USB controllers
    int usbControllers = 0;
    QDir pciDir("/sys/bus/pci/devices");
    if (pciDir.exists()) {
        QStringList pciDevices = pciDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& dev : pciDevices) {
            QFile classFile(QString("/sys/bus/pci/devices/%1/class").arg(dev));
            if (classFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString classId = QTextStream(&classFile).readLine().trimmed();
                classFile.close();
                // 0x0c03xx = USB controller
                if (classId.startsWith("0x0c03")) {
                    usbControllers++;
                }
            }
        }
    }
    
    addRow(tr("USB Controllers (PCI)"), QString::number(usbControllers));
    
    // Check for Bluetooth
    QDir bluetoothDir("/sys/class/bluetooth");
    if (bluetoothDir.exists()) {
        QStringList btDevices = bluetoothDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        if (!btDevices.isEmpty()) {
            addRow(tr("Bluetooth Adapters"), QString::number(btDevices.count()));
        }
    }
    
    // Network interfaces (can be seen as network ports)
    int ethCount = 0, wlanCount = 0;
    QDir netDir("/sys/class/net");
    if (netDir.exists()) {
        QStringList interfaces = netDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& iface : interfaces) {
            if (iface.startsWith("eth") || iface.startsWith("enp")) ethCount++;
            else if (iface.startsWith("wlan") || iface.startsWith("wlp")) wlanCount++;
        }
        if (ethCount > 0) addRow(tr("Ethernet Ports"), QString::number(ethCount));
        if (wlanCount > 0) addRow(tr("Wireless Adapters"), QString::number(wlanCount));
    }
}

void PortsTab::showGeekMode()
{
    GeekPortsDialog dlg(this);
    dlg.exec();
}

// ===== Geek Mode Dialog =====

GeekPortsDialog::GeekPortsDialog(QWidget* parent)
    : QDialog(parent)
    , refreshTimer(new QTimer(this))
{
    setWindowTitle(tr("Ports - Geek Mode"));
    setModal(true);
    resize(700, 500);

    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* titleLabel = new QLabel(tr("Ports Technical Details"));
    titleLabel->setStyleSheet("font-size:16px; font-weight:bold; color:#2c3e50; margin-bottom:10px;");
    layout->addWidget(titleLabel);

    table = new QTableWidget();
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList() << tr("Property") << tr("Value"));
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #34495e; color: white; font-weight: bold; padding: 8px; border: 1px solid #2c3e50; }");
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->setWordWrap(true);

    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidget(table);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(350);
    layout->addWidget(scrollArea);

    // Buttons: Search (left) - stretch - Copy, Save, Close (right)
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    GeekSearchIntegration::addSearchButtonToGeekDialog(buttonLayout, this, table);
    buttonLayout->addStretch();
    
    QPushButton* copyBtn = new QPushButton(tr("Copy"));
    QPushButton* saveBtn = new QPushButton(tr("Save..."));
    QPushButton* closeBtn = new QPushButton(tr("Close"));
    
    buttonLayout->addWidget(copyBtn);
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(closeBtn);
    
    layout->addLayout(buttonLayout);

    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);

    // Enable copy
    enableTableCopy(table, refreshTimer);

    connect(copyBtn, &QPushButton::clicked, [this]() {
        QString all;
        for (int r = 0; r < table->rowCount(); ++r) {
            QString prop = table->item(r,0) ? table->item(r,0)->text() : QString();
            QString val = table->item(r,1) ? table->item(r,1)->text() : QString();
            all += prop + ": " + val + "\n";
        }
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(all, QClipboard::Clipboard);
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("Copied"));
        msgBox.setText(tr("The information has been copied\nto the clipboard."));
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    });

    connect(saveBtn, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save Ports Info"), "ports-info.csv", tr("CSV Files (*.csv);;All Files (*)"));
        if (fileName.isEmpty()) return;

        auto esc = [](const QString &s)->QString {
            QString out = s;
            out.replace('"', "\"\"");
            if (out.contains(',') || out.contains('\n') || out.contains('"')) {
                out = '"' + out + '"';
            }
            return out;
        };

        QFile out(fileName);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) return;
        QTextStream ts(&out);
        ts << "Property,Value\n";
        for (int r = 0; r < table->rowCount(); ++r) {
            QString prop = table->item(r,0) ? table->item(r,0)->text() : QString();
            QString val = table->item(r,1) ? table->item(r,1)->text() : QString();
            ts << esc(prop) << ',' << esc(val) << '\n';
        }
        out.close();
    });

    fillTable();

    // Auto-refresh every second
    refreshTimer->setInterval(1000);
    connect(refreshTimer, &QTimer::timeout, this, &GeekPortsDialog::fillTable);
}

void GeekPortsDialog::showEvent(QShowEvent* ev)
{
    QDialog::showEvent(ev);
    if (refreshTimer) refreshTimer->start();
}

void GeekPortsDialog::hideEvent(QHideEvent* ev)
{
    if (refreshTimer) refreshTimer->stop();
    QDialog::hideEvent(ev);
}

void GeekPortsDialog::fillTable()
{
    table->setRowCount(0);
    int row = 0;
    
    auto addRow = [&](const QString& prop, const QString& val) {
        table->insertRow(row);
        QTableWidgetItem* p = new QTableWidgetItem(prop);
        QFont bold;
        bold.setBold(true);
        p->setFont(bold);
        table->setItem(row, 0, p);
        
        QString displayVal = val;
        displayVal.replace("\n", " ↳ ");
        
        QTableWidgetItem* v = new QTableWidgetItem(displayVal);
        v->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(row, 1, v);
        table->resizeRowToContents(row);
        ++row;
    };
    
    // === USB DEVICES ===
    addRow(tr("=== USB DEVICES ==="), "");
    
    QDir usbDir("/sys/bus/usb/devices");
    if (usbDir.exists()) {
        QStringList usbDevices = usbDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QString& dev : usbDevices) {
            QDir devDir(usbDir.filePath(dev));
            
            addRow(tr("Device"), dev);
            
            // Read device properties
            QStringList props;
            props << "manufacturer" << "product" << "serial" << "idVendor" << "idProduct" 
                  << "bDeviceClass" << "bDeviceSubClass" << "bDeviceProtocol" 
                  << "bMaxPacketSize0" << "bMaxPower" << "speed" << "version";
            
            for (const QString& prop : props) {
                QFile propFile(devDir.filePath(prop));
                if (propFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString value = QTextStream(&propFile).readLine().trimmed();
                    propFile.close();
                    if (!value.isEmpty()) {
                        addRow(QString("  %1/%2").arg(dev, prop), value);
                    }
                }
            }
            
            addRow("", ""); // Spacer
        }
    }
    
    // === SERIAL PORTS ===
    addRow(tr("=== SERIAL PORTS ==="), "");
    
    QDir ttyDir("/sys/class/tty");
    if (ttyDir.exists()) {
        QStringList ttys = ttyDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QString& tty : ttys) {
            if (tty.startsWith("ttyS") || tty.startsWith("ttyUSB") || tty.startsWith("ttyACM")) {
                QDir ttyDevDir(ttyDir.filePath(tty));
                
                addRow(tr("Serial Port"), QString("/dev/%1").arg(tty));
                
                // Check if device exists
                QFile devFile(QString("/dev/%1").arg(tty));
                if (devFile.exists()) {
                    addRow(QString("  %1/exists").arg(tty), "yes");
                    
                    // Try to read uevent
                    QFile ueventFile(ttyDevDir.filePath("uevent"));
                    if (ueventFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        QString uevent = QTextStream(&ueventFile).readAll();
                        ueventFile.close();
                        addRow(QString("  %1/uevent").arg(tty), uevent);
                    }
                }
                
                addRow("", "");
            }
        }
    }
    
    // === PCI USB CONTROLLERS ===
    addRow(tr("=== PCI USB CONTROLLERS ==="), "");
    
    QDir pciDir("/sys/bus/pci/devices");
    if (pciDir.exists()) {
        QStringList pciDevices = pciDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& dev : pciDevices) {
            QFile classFile(QString("/sys/bus/pci/devices/%1/class").arg(dev));
            if (classFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString classId = QTextStream(&classFile).readLine().trimmed();
                classFile.close();
                
                // 0x0c03xx = USB controller, 0x0c00xx = Firewire, 0x0c05xx = SMBus
                if (classId.startsWith("0x0c03") || classId.startsWith("0x0c00") || classId.startsWith("0x0c05")) {
                    addRow(tr("PCI Device"), dev);
                    addRow(QString("  %1/class").arg(dev), classId);
                    
                    QStringList pciProps;
                    pciProps << "vendor" << "device" << "subsystem_vendor" << "subsystem_device" 
                             << "irq" << "enable";
                    
                    for (const QString& prop : pciProps) {
                        QFile propFile(QString("/sys/bus/pci/devices/%1/%2").arg(dev, prop));
                        if (propFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                            QString value = QTextStream(&propFile).readLine().trimmed();
                            propFile.close();
                            if (!value.isEmpty()) {
                                addRow(QString("  %1/%2").arg(dev, prop), value);
                            }
                        }
                    }
                    
                    addRow("", "");
                }
            }
        }
    }
    
    // === BLUETOOTH ADAPTERS ===
    QDir bluetoothDir("/sys/class/bluetooth");
    if (bluetoothDir.exists()) {
        QStringList btDevices = bluetoothDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        if (!btDevices.isEmpty()) {
            addRow("", "");
            addRow(tr("=== BLUETOOTH ADAPTERS ==="), "");
            
            for (const QString& bt : btDevices) {
                addRow(tr("Bluetooth Adapter"), bt);
                
                QDir btDir(bluetoothDir.filePath(bt));
                QStringList btProps;
                btProps << "address" << "name" << "type";
                
                for (const QString& prop : btProps) {
                    QFile propFile(btDir.filePath(prop));
                    if (propFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        QString value = QTextStream(&propFile).readLine().trimmed();
                        propFile.close();
                        if (!value.isEmpty()) {
                            addRow(QString("  %1/%2").arg(bt, prop), value);
                        }
                    }
                }
                
                addRow("", "");
            }
        }
    }
    
    // === NETWORK INTERFACES ===
    addRow(tr("=== NETWORK INTERFACES ==="), "");
    
    QDir netDir("/sys/class/net");
    if (netDir.exists()) {
        QStringList interfaces = netDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QString& iface : interfaces) {
            if (iface == "lo") continue; // Skip loopback
            
            QDir ifaceDir(netDir.filePath(iface));
            
            addRow(tr("Interface"), iface);
            
            QStringList netProps;
            netProps << "address" << "operstate" << "speed" << "duplex" << "mtu" 
                     << "type" << "tx_queue_len";
            
            for (const QString& prop : netProps) {
                QFile propFile(ifaceDir.filePath(prop));
                if (propFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString value = QTextStream(&propFile).readLine().trimmed();
                    propFile.close();
                    if (!value.isEmpty() && value != "-1") {
                        addRow(QString("  %1/%2").arg(iface, prop), value);
                    }
                }
            }
            
            addRow("", "");
        }
    }
}
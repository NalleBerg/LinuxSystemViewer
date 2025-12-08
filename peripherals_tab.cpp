#include "peripherals_tab.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QScrollArea>
#include <QTableWidgetItem>
#include <QTableWidget>
#include <QAbstractItemView>
#include <QStringList>
#include <QFont>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QThread>
#include <QTimer>
#include <QPainter>
#include <QTimer>
#include <QShowEvent>
#include <QHideEvent>
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QClipboard>
#include <QGuiApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QApplication>
#include <QEventLoop>
#include <QDir>
#include "gui_helpers.h"

#include <QDir>

PeripheralsTab::PeripheralsTab(QWidget* parent)
    : QWidget(parent)
{
    // Headline and Geek button (use helper to guarantee identical placement)
    QPushButton* gb = nullptr;
    QHBoxLayout* headlineLayout = createHeadlineWithGeek(this, tr("Peripherals"), &gb);
    geekButton = gb;
    connect(geekButton, &QPushButton::clicked, this, &PeripheralsTab::showGeekMode);
    
    // Add loading indicator next to Geek button
    loadingLabel = new QLabel(tr("Loading, please wait..."), this);
    loadingLabel->setStyleSheet("QLabel { font-weight: bold; color: #333; margin-right: 10px; }");
    loadingLabel->hide();
    
    // Add loading elements to headline layout
    headlineLayout->insertWidget(headlineLayout->count() - 1, loadingLabel);  // Before geek button

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    applyMainLayoutDefaults(mainLayout);
    mainLayout->addLayout(headlineLayout);

    // Table
    tableWidget = new QTableWidget();
    tableWidget->setColumnCount(2);
    tableWidget->setHorizontalHeaderLabels(QStringList() << tr("Device Type") << tr("Device Name"));
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "  background-color: #2c3e50;"
        "  color: white;"
        "  font-weight: bold;"
        "  padding: 8px;"
        "  border: none;"
        "}"
    );
    tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    tableWidget->setColumnWidth(0, 220);  // Match CPU tab width
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
    tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->setAlternatingRowColors(true);

    // Scroll area
    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidget(tableWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(220);
    mainLayout->addWidget(scrollArea);

    // Auto-refresh every 3 seconds
    refreshTimer = new QTimer(this);
    refreshTimer->setInterval(3000);
    connect(refreshTimer, &QTimer::timeout, this, &PeripheralsTab::refreshPeripherals);
    
    // Enable copy functionality
    enableTableCopy(tableWidget, refreshTimer);

    // Initial population
    refreshPeripherals();
    refreshTimer->start();
}

// Test if a camera device is actually accessible
bool PeripheralsTab::isCameraAccessible(const QString& deviceName)
{
    Q_UNUSED(deviceName);
    
    // Check if any video devices exist and are accessible
    QDir videoDir("/dev");
    QStringList videoDevices = videoDir.entryList(QStringList() << "video*", QDir::System);
    
    for (const QString& device : videoDevices) {
        QString devicePath = "/dev/" + device;
        
        // Try to access the camera device with a quick test
        QProcess testProcess;
        testProcess.start("timeout", QStringList() << "1s" << "cat" << devicePath);
        testProcess.waitForFinished(1500); // Wait max 1.5 seconds
        
        // If the process exits successfully (not permission denied), camera is accessible
        if (testProcess.exitCode() == 0 || testProcess.exitCode() == 124) {
            return true;
        }
    }
    
    return false;
}

void PeripheralsTab::refreshPeripherals()
{
    tableWidget->setRowCount(0);
    int row = 0;

    auto addRow = [&](const QString& type, const QString& device){
        tableWidget->insertRow(row);
        QTableWidgetItem* typeItem = new QTableWidgetItem(type);
        QFont boldFont;
        boldFont.setBold(true);
        typeItem->setFont(boldFont);
        tableWidget->setItem(row, 0, typeItem);
        QTableWidgetItem* deviceItem = new QTableWidgetItem(device);
        tableWidget->setItem(row, 1, deviceItem);
        tableWidget->resizeRowToContents(row);
        ++row;
    };

    // Get USB devices
    QProcess lsusb;
    lsusb.start("lsusb");
    lsusb.waitForFinished(3000);
    QString usbOutput = lsusb.readAllStandardOutput();
    
    if (!usbOutput.isEmpty()) {
        QStringList lines = usbOutput.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            // Parse: Bus 001 Device 003: ID 046d:c52b Logitech, Inc. Unifying Receiver
            QRegularExpression rx("Bus\\s+\\d+\\s+Device\\s+\\d+:\\s+ID\\s+([0-9a-fA-F:]+)\\s+(.+)");
            QRegularExpressionMatch match = rx.match(line);
            if (match.hasMatch()) {
                QString deviceName = match.captured(2).trimmed();
                QString deviceId = match.captured(1);
                
                // Filter: Skip internal hubs, root hubs, host controllers
                if (deviceName.contains("Linux Foundation", Qt::CaseInsensitive) ||
                    deviceName.contains("root hub", Qt::CaseInsensitive) ||
                    deviceName.contains("Host Controller", Qt::CaseInsensitive) ||
                    deviceName.contains("Hub", Qt::CaseInsensitive) ||
                    (deviceName.contains("Intel Corp", Qt::CaseInsensitive) && deviceName.contains("Hub", Qt::CaseInsensitive)) ||
                    (deviceName.contains("AMD", Qt::CaseInsensitive) && deviceName.contains("Hub", Qt::CaseInsensitive))) {
                    continue;
                }
                
                // Categorize the device
                QString type;
                if (deviceName.contains("mouse", Qt::CaseInsensitive) ||
                    deviceName.contains("trackpad", Qt::CaseInsensitive) ||
                    deviceName.contains("trackpoint", Qt::CaseInsensitive)) {
                    type = tr("Mouse/Pointing");
                } else if (deviceName.contains("keyboard", Qt::CaseInsensitive)) {
                    type = tr("Keyboard");
                } else if (deviceName.contains("camera", Qt::CaseInsensitive) ||
                           deviceName.contains("webcam", Qt::CaseInsensitive)) {
                    // Test if camera is actually accessible
                    if (isCameraAccessible(deviceName)) {
                        type = tr("Camera");
                    } else {
                        type = tr("Camera (Disabled)");
                    }
                } else if (deviceName.contains("printer", Qt::CaseInsensitive) ||
                           deviceName.contains("print", Qt::CaseInsensitive)) {
                    type = tr("Printer (USB)");
                } else if (deviceName.contains("phone", Qt::CaseInsensitive) ||
                           deviceName.contains("android", Qt::CaseInsensitive) ||
                           deviceName.contains("iphone", Qt::CaseInsensitive) ||
                           deviceName.contains("mobile", Qt::CaseInsensitive) ||
                           deviceName.contains("samsung", Qt::CaseInsensitive) ||
                           deviceName.contains("apple", Qt::CaseInsensitive)) {
                    type = tr("Mobile Device");
                } else if (deviceName.contains("ethernet", Qt::CaseInsensitive) ||
                           deviceName.contains("network", Qt::CaseInsensitive) ||
                           deviceName.contains("wireless", Qt::CaseInsensitive) ||
                           deviceName.contains("wifi", Qt::CaseInsensitive) ||
                           deviceName.contains("wlan", Qt::CaseInsensitive)) {
                    type = tr("Network Adapter");
                } else if (deviceName.contains("bluetooth", Qt::CaseInsensitive)) {
                    // Skip Bluetooth adapters - they are typically built-in
                    continue;
                } else if (deviceName.contains("storage", Qt::CaseInsensitive) ||
                           deviceName.contains("mass storage", Qt::CaseInsensitive) ||
                           deviceName.contains("flash", Qt::CaseInsensitive) ||
                           deviceName.contains("disk", Qt::CaseInsensitive)) {
                    type = tr("Storage Device");
                } else if (deviceName.contains("audio", Qt::CaseInsensitive) ||
                           deviceName.contains("sound", Qt::CaseInsensitive) ||
                           deviceName.contains("speaker", Qt::CaseInsensitive) ||
                           deviceName.contains("headset", Qt::CaseInsensitive) ||
                           deviceName.contains("headphone", Qt::CaseInsensitive)) {
                    type = tr("Audio Device");
                } else if (deviceName.contains("gamepad", Qt::CaseInsensitive) ||
                           deviceName.contains("joystick", Qt::CaseInsensitive) ||
                           deviceName.contains("controller", Qt::CaseInsensitive) ||
                           deviceName.contains("xbox", Qt::CaseInsensitive) ||
                           deviceName.contains("playstation", Qt::CaseInsensitive)) {
                    type = tr("Game Controller");
                } else if (deviceName.contains("scanner", Qt::CaseInsensitive)) {
                    type = tr("Scanner");
                } else if (deviceName.contains("tablet", Qt::CaseInsensitive) ||
                           deviceName.contains("wacom", Qt::CaseInsensitive) ||
                           deviceName.contains("digitizer", Qt::CaseInsensitive)) {
                    type = tr("Drawing Tablet");
                } else if (deviceName.contains("receiver", Qt::CaseInsensitive) ||
                           deviceName.contains("unifying", Qt::CaseInsensitive)) {
                    type = tr("Wireless Receiver");
                } else if (deviceName.contains("hub", Qt::CaseInsensitive)) {
                    type = tr("USB Hub");
                } else if (deviceName.contains("card reader", Qt::CaseInsensitive) ||
                           deviceName.contains("card", Qt::CaseInsensitive)) {
                    type = tr("Card Reader");
                } else if (deviceName.contains("serial", Qt::CaseInsensitive) ||
                           deviceName.contains("ftdi", Qt::CaseInsensitive) ||
                           deviceName.contains("ch340", Qt::CaseInsensitive) ||
                           deviceName.contains("cp210", Qt::CaseInsensitive)) {
                    type = tr("Serial Adapter");
                } else {
                    // Show other USB devices
                    type = tr("USB Device");
                }
                
                addRow(type, deviceName + " [" + deviceId + "]");
            }
        }
    }

    // Check for CUPS printers (network and USB printers)
    QProcess lpstat;
    lpstat.start("lpstat", QStringList() << "-p" << "-d");
    lpstat.waitForFinished(3000);
    QString printerOutput = lpstat.readAllStandardOutput();
    
    if (!printerOutput.isEmpty() && !printerOutput.contains("No destinations added")) {
        QStringList printerLines = printerOutput.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : printerLines) {
            if (line.startsWith("printer ")) {
                // Parse: printer HP_LaserJet is idle
                QRegularExpression printerRx("printer\\s+(\\S+)\\s+(.*)");
                QRegularExpressionMatch printerMatch = printerRx.match(line);
                if (printerMatch.hasMatch()) {
                    QString printerName = printerMatch.captured(1);
                    QString status = printerMatch.captured(2);
                    addRow(tr("Printer (Network/CUPS)"), printerName + " - " + status);
                }
            }
        }
    }

    // Check for external displays (not laptop internal screens)
    QProcess xrandr;
    xrandr.start("xrandr", QStringList() << "--query");
    xrandr.waitForFinished(3000);
    QString displayOutput = xrandr.readAllStandardOutput();
    
    if (!displayOutput.isEmpty()) {
        QStringList displayLines = displayOutput.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : displayLines) {
            // Look for connected displays
            if (line.contains(" connected")) {
                QStringList parts = line.split(' ', Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    QString displayName = parts[0];
                    
                    // Skip internal laptop displays (eDP, LVDS, DSI)
                    if (displayName.startsWith("eDP", Qt::CaseInsensitive) ||
                        displayName.startsWith("LVDS", Qt::CaseInsensitive) ||
                        displayName.startsWith("DSI", Qt::CaseInsensitive)) {
                        continue;
                    }
                    
                    // Only show actual external displays (HDMI, DP, VGA, DVI)
                    if (displayName.contains("HDMI", Qt::CaseInsensitive) ||
                        displayName.startsWith("DP-", Qt::CaseInsensitive) ||
                        displayName.contains("DisplayPort", Qt::CaseInsensitive) ||
                        displayName.contains("VGA", Qt::CaseInsensitive) ||
                        displayName.contains("DVI", Qt::CaseInsensitive)) {
                        
                        QString resolution;
                        // Try to find resolution in the line
                        for (int i = 2; i < parts.size(); ++i) {
                            if (parts[i].contains('x') && parts[i].contains('+')) {
                                resolution = parts[i].split('+')[0];
                                break;
                            }
                        }
                        QString displayInfo = displayName;
                        if (!resolution.isEmpty()) {
                            displayInfo += " (" + resolution + ")";
                        }
                        addRow(tr("External Display"), displayInfo);
                    }
                }
            }
        }
    }
    
    // Check for Miracast/Wireless Display (casting to TV)
    QProcess miracast;
    miracast.start("bash", QStringList() << "-c" << "nmcli device status | grep -E 'wifi-p2p|p2p-dev' || ip link show | grep 'p2p-dev'");
    miracast.waitForFinished(3000);
    QString miracastOutput = miracast.readAllStandardOutput();
    
    if (!miracastOutput.isEmpty()) {
        QStringList miracastLines = miracastOutput.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : miracastLines) {
            if (line.contains("p2p", Qt::CaseInsensitive) && line.contains("connected", Qt::CaseInsensitive)) {
                addRow("Wireless Display (Casting)", "Active Miracast/Cast connection");
                break;
            }
        }
    }

    // Check for MTP devices (Android phones, tablets via USB)
    QProcess mtpDevices;
    mtpDevices.start("mtp-detect");
    mtpDevices.waitForFinished(3000);
    QString mtpOutput = mtpDevices.readAllStandardOutput();
    
    if (!mtpOutput.isEmpty() && mtpOutput.contains("Device ")) {
        QStringList mtpLines = mtpOutput.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : mtpLines) {
            if (line.contains("Manufacturer:") || line.contains("Model:")) {
                QString info = line.split(':').last().trimmed();
                if (!info.isEmpty()) {
                    addRow("Mobile Device (MTP)", info);
                }
            }
        }
    }

    if (row == 0) {
        addRow("Info", "No external peripherals detected");
    }
}

void PeripheralsTab::showGeekMode()
{
    showMainTabSpinner();
    
    // Create dialog and load data while spinner runs
    GeekPeripheralsDialog dlg(this);
    dlg.loadData();  // This calls fillTable() which now has processEvents
    
    hideMainTabSpinner();
    
    // Show the populated dialog
    dlg.exec();
}

void PeripheralsTab::showMainTabSpinner()
{
    loadingLabel->show();
}

void PeripheralsTab::hideMainTabSpinner()
{
    loadingLabel->hide();
}

// --- GeekPeripheralsDialog ---

GeekPeripheralsDialog::GeekPeripheralsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Peripherals - Geek Mode"));
    setModal(true);
    resize(700, 500);

    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* titleLabel = new QLabel(tr("Peripherals Technical Details"));
    titleLabel->setStyleSheet("font-size:16px; font-weight:bold; color:#2c3e50; margin-bottom:10px;");
    layout->addWidget(titleLabel);

    table = new QTableWidget();
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList() << tr("Property") << tr("Value"));
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #34495e; color: white; font-weight: bold; padding: 8px; border: 1px solid #2c3e50; }");
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidget(table);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(350);
    layout->addWidget(scrollArea);

    // Buttons: Rescan on left, Copy/Save/Close on right
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    // Left side: Rescan button
    QPushButton* rescanBtn = new QPushButton(tr("Rescan"));
    buttonLayout->addWidget(rescanBtn);
    
    // Add rescanning text label
    QLabel* rescanningLabel = new QLabel(tr("Rescanning, please wait..."), this);
    rescanningLabel->setStyleSheet("QLabel { font-weight: bold; color: #333; }");
    rescanningLabel->hide();
    buttonLayout->addWidget(rescanningLabel);
    
    // Store rescanning label for show/hide
    this->setProperty("rescanningLabel", QVariant::fromValue(rescanningLabel));
    
    buttonLayout->addStretch();
    
    // Right side: Copy, Save, Close buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    QPushButton* copyBtn = new QPushButton(tr("Copy"));
    QPushButton* saveBtn = new QPushButton(tr("Save..."));
    buttonBox->addButton(copyBtn, QDialogButtonBox::ActionRole);
    buttonBox->addButton(saveBtn, QDialogButtonBox::ActionRole);
    // Fix Close button translation
    buttonBox->button(QDialogButtonBox::Close)->setText(tr("Close"));
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    buttonLayout->addWidget(buttonBox);
    layout->addLayout(buttonLayout);
    
    // Connect Rescan button to new rescan method
    connect(rescanBtn, &QPushButton::clicked, this, &GeekPeripheralsDialog::rescan);

    // Enable copy on main geek table (right-click + Ctrl+C)
    enableTableCopy(table, nullptr);

    connect(copyBtn, &QPushButton::clicked, [this]() {
        // Human-readable UTF-8 copy: Property: Value per line
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
        // Save as CSV (UTF-8)
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save Peripherals Info"), "peripherals-info.csv", tr("CSV Files (*.csv);;All Files (*)"));
        if (fileName.isEmpty()) return;

        auto esc = [](const QString &s)->QString {
            QString out = s;
            // double quotes -> two double quotes
            out.replace('"', "\"\"");
            // wrap in quotes if contains comma, quote or newline
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
}

void GeekPeripheralsDialog::loadData()
{
    fillTable();
}

void GeekPeripheralsDialog::showEvent(QShowEvent* ev)
{
    QDialog::showEvent(ev);
}

void GeekPeripheralsDialog::hideEvent(QHideEvent* ev)
{
    QDialog::hideEvent(ev);
}

void GeekPeripheralsDialog::fillTable()
{
    table->setRowCount(0);
    int row = 0;

    auto addRow = [&](const QString& prop, const QString& val){
        table->insertRow(row);
        QTableWidgetItem* p = new QTableWidgetItem(prop);
        QFont bold; bold.setBold(true); p->setFont(bold);
        table->setItem(row, 0, p);
        QTableWidgetItem* v = new QTableWidgetItem(val);
        table->setItem(row, 1, v);
        table->resizeRowToContents(row);
        ++row;
    };

    // === USB Devices (lsusb -v) ===
    QProcess lsusb;
    lsusb.start("lsusb", QStringList() << "-v");
    lsusb.waitForFinished(2000);
    QString usbOutput = lsusb.readAllStandardOutput();
    QApplication::processEvents(); // Allow spinner to continue

    if (!usbOutput.isEmpty()) {
        addRow("=== USB Devices (lsusb -v) ===", "");
        addRow("Full USB Information", usbOutput.left(30000)); // Limit to 30KB
    }

    // === PCI Devices (lspci -vv) ===
    QProcess lspci;
    lspci.start("lspci", QStringList() << "-vv");
    lspci.waitForFinished(2000);
    QString pciOutput = lspci.readAllStandardOutput();
    QApplication::processEvents(); // Allow spinner to continue

    if (!pciOutput.isEmpty()) {
        addRow("", "");
        addRow("=== PCI Devices (lspci -vv) ===", "");
        addRow("Full PCI Information", pciOutput.left(30000));
    }

    // === Input Devices (/proc/bus/input/devices) ===
    QFile inputFile("/proc/bus/input/devices");
    if (inputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addRow("", "");
        addRow("=== Input Devices (/proc/bus/input/devices) ===", "");
        QString inputContent = QTextStream(&inputFile).readAll();
        addRow("Input Devices", inputContent.left(30000));
        inputFile.close();
    }

    // === USB Topology (usb-devices) ===
    QProcess usbDevices;
    usbDevices.start("usb-devices");
    usbDevices.waitForFinished(2000);
    QString usbDevOutput = usbDevices.readAllStandardOutput();

    if (!usbDevOutput.isEmpty()) {
        addRow("", "");
        addRow("=== USB Topology (usb-devices) ===", "");
        addRow("USB Device Tree", usbDevOutput.left(30000));
    }

    // === HID Devices ===
    QFile hidrawDir("/sys/class/hidraw");
    if (hidrawDir.exists()) {
        addRow("", "");
        addRow("=== HID Devices ===", "");
        QProcess lsHid;
        lsHid.start("bash", QStringList() << "-c" << "ls -la /sys/class/hidraw/");
        lsHid.waitForFinished(1500);
        QString hidOutput = lsHid.readAllStandardOutput();
        QApplication::processEvents(); // Allow spinner to continue
        if (!hidOutput.isEmpty()) {
            addRow("HID Raw Devices", hidOutput);
        }
    }

    // === Bluetooth Devices (if bluetoothctl available) ===
    QProcess btctl;
    btctl.start("bluetoothctl", QStringList() << "devices");
    btctl.waitForFinished(2000);
    QString btOutput = btctl.readAllStandardOutput();
    QApplication::processEvents(); // Allow spinner to continue

    if (!btOutput.isEmpty() && !btOutput.contains("command not found")) {
        addRow("", "");
        addRow("=== Bluetooth Devices ===", "");
        addRow("Bluetooth Devices", btOutput);
    }

    // === Network Interfaces (ip link) ===
    QProcess ipLink;
    ipLink.start("ip", QStringList() << "link" << "show");
    ipLink.waitForFinished(1000);
    QString ipOutput = ipLink.readAllStandardOutput();

    if (!ipOutput.isEmpty()) {
        addRow("", "");
        addRow("=== Network Interfaces (ip link) ===", "");
        addRow("Network Interfaces", ipOutput);
    }

    // === /sys/class/net/ Network Devices ===
    QProcess netDevices;
    netDevices.start("bash", QStringList() << "-c" << "ls -la /sys/class/net/");
    netDevices.waitForFinished(1000);
    QString netOutput = netDevices.readAllStandardOutput();

    if (!netOutput.isEmpty()) {
        addRow("", "");
        addRow("=== Network Device Classes ===", "");
        addRow("/sys/class/net/", netOutput);
    }

    // === Sound Devices ===
    QFile soundDevices("/proc/asound/cards");
    if (soundDevices.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addRow("", "");
        addRow("=== Sound Cards (/proc/asound/cards) ===", "");
        QString soundContent = QTextStream(&soundDevices).readAll();
        addRow("Sound Cards", soundContent);
        soundDevices.close();
    }

    // === Video Devices (/dev/video*) ===
    QProcess videoDevs;
    videoDevs.start("bash", QStringList() << "-c" << "ls -la /dev/video* 2>/dev/null");
    videoDevs.waitForFinished(1000);
    QString videoOutput = videoDevs.readAllStandardOutput();
    QApplication::processEvents(); // Allow spinner to continue

    if (!videoOutput.isEmpty()) {
        addRow("", "");
        addRow("=== Video Devices ===", "");
        addRow("Video Devices", videoOutput);
    }

    // === DRM Devices ===
    QProcess drmDevs;
    drmDevs.start("bash", QStringList() << "-c" << "ls -la /sys/class/drm/");
    drmDevs.waitForFinished(1000);
    QString drmOutput = drmDevs.readAllStandardOutput();

    if (!drmOutput.isEmpty()) {
        addRow("", "");
        addRow("=== DRM (Graphics) Devices ===", "");
        addRow("DRM Devices", drmOutput);
    }

    // === CUPS Printers (lpstat -v) ===
    QProcess lpstatV;
    lpstatV.start("lpstat", QStringList() << "-v");
    lpstatV.waitForFinished(1500);
    QString lpstatOutput = lpstatV.readAllStandardOutput();

    if (!lpstatOutput.isEmpty()) {
        addRow("", "");
        addRow("=== CUPS Printers (lpstat -v) ===", "");
        addRow("Printers", lpstatOutput);
    }

    // === Printer Status (lpstat -p -d) ===
    QProcess lpstatP;
    lpstatP.start("lpstat", QStringList() << "-p" << "-d");
    lpstatP.waitForFinished(1500);
    QString lpstatPOutput = lpstatP.readAllStandardOutput();

    if (!lpstatPOutput.isEmpty()) {
        addRow("", "");
        addRow("=== Printer Status ===", "");
        addRow("Status", lpstatPOutput);
    }

    // === Display Information (xrandr) ===
    QProcess xrandr;
    xrandr.start("xrandr", QStringList() << "--verbose");
    xrandr.waitForFinished(1500);
    QString xrandrOutput = xrandr.readAllStandardOutput();
    QApplication::processEvents(); // Allow spinner to continue

    if (!xrandrOutput.isEmpty()) {
        addRow("", "");
        addRow("=== Display Information (xrandr --verbose) ===", "");
        addRow("Displays", xrandrOutput.left(30000));
    }

    // === EDID Information (for external displays) ===
    QProcess edid;
    edid.start("bash", QStringList() << "-c" << "find /sys/devices -name edid -exec cat {} \\; 2>/dev/null | strings");
    edid.waitForFinished(2000);
    QString edidOutput = edid.readAllStandardOutput();

    if (!edidOutput.isEmpty()) {
        addRow("", "");
        addRow("=== EDID Display Data ===", "");
        addRow("EDID Info", edidOutput);
    }

    // === MTP Devices (mtp-detect) ===
    QProcess mtpDetect;
    mtpDetect.start("mtp-detect");
    mtpDetect.waitForFinished(2500);
    QString mtpOutput = mtpDetect.readAllStandardOutput();
    QApplication::processEvents(); // Allow spinner to continue

    if (!mtpOutput.isEmpty() && !mtpOutput.contains("Unable to find")) {
        addRow("", "");
        addRow("=== MTP Devices (Android/Mobile) ===", "");
        addRow("MTP Devices", mtpOutput.left(30000));
    }

    // === ADB Devices (if adb available) ===
    QProcess adb;
    adb.start("adb", QStringList() << "devices" << "-l");
    adb.waitForFinished(2500);
    QString adbOutput = adb.readAllStandardOutput();
    QApplication::processEvents(); // Allow spinner to continue

    if (!adbOutput.isEmpty() && !adbOutput.contains("command not found")) {
        addRow("", "");
        addRow("=== ADB Devices (Android Debug Bridge) ===", "");
        addRow("ADB Devices", adbOutput);
    }

    // === Scanners (scanimage -L) ===
    QProcess scanners;
    scanners.start("scanimage", QStringList() << "-L");
    scanners.waitForFinished(2000);
    QString scanOutput = scanners.readAllStandardOutput();

    if (!scanOutput.isEmpty() && !scanOutput.contains("No scanners")) {
        addRow("", "");
        addRow("=== Scanners (scanimage -L) ===", "");
        addRow("Scanners", scanOutput);
    }

    // === Power Devices (UPS, Battery) ===
    QProcess powerDevs;
    powerDevs.start("bash", QStringList() << "-c" << "ls -la /sys/class/power_supply/");
    powerDevs.waitForFinished(1000);
    QString powerOutput = powerDevs.readAllStandardOutput();

    if (!powerOutput.isEmpty()) {
        addRow("", "");
        addRow("=== Power Devices ===", "");
        addRow("/sys/class/power_supply/", powerOutput);
    }

    // === Thunderbolt Devices ===
    QProcess thunderbolt;
    thunderbolt.start("bash", QStringList() << "-c" << "ls -la /sys/bus/thunderbolt/devices/ 2>/dev/null");
    thunderbolt.waitForFinished(1000);
    QString tbOutput = thunderbolt.readAllStandardOutput();

    if (!tbOutput.isEmpty() && !tbOutput.contains("No such file")) {
        addRow("", "");
        addRow("=== Thunderbolt Devices ===", "");
        addRow("Thunderbolt", tbOutput);
    }

    // === Serial Ports ===
    QProcess serial;
    serial.start("bash", QStringList() << "-c" << "ls -la /dev/ttyUSB* /dev/ttyACM* /dev/ttyS* 2>/dev/null");
    serial.waitForFinished(1000);
    QString serialOutput = serial.readAllStandardOutput();

    if (!serialOutput.isEmpty()) {
        addRow("", "");
        addRow("=== Serial Ports ===", "");
        addRow("Serial Devices", serialOutput);
    }

    // === Gamepad/Joystick Devices ===
    QProcess gamepad;
    gamepad.start("bash", QStringList() << "-c" << "ls -la /dev/input/js* 2>/dev/null");
    gamepad.waitForFinished(1000);
    QString gamepadOutput = gamepad.readAllStandardOutput();

    if (!gamepadOutput.isEmpty()) {
        addRow("", "");
        addRow("=== Gamepad/Joystick Devices ===", "");
        addRow("Game Controllers", gamepadOutput);
    }
}

void GeekPeripheralsDialog::rescan()
{
    showSpinner();
    
    // Refresh the data - fillTable now has processEvents calls to keep spinner running
    fillTable();
    
    hideSpinner();
}

void GeekPeripheralsDialog::showSpinner()
{
    // Show rescanning label
    if (auto rescanningLabel = this->property("rescanningLabel").value<QLabel*>()) {
        rescanningLabel->show();
    }
}

void GeekPeripheralsDialog::hideSpinner()
{
    // Hide rescanning label
    if (auto rescanningLabel = this->property("rescanningLabel").value<QLabel*>()) {
        rescanningLabel->hide();
    }
}
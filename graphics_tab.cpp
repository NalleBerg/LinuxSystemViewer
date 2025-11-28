#include "graphics_tab.h"
#include "graphics.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QScrollArea>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QClipboard>
#include <QGuiApplication>
#include <QFileDialog>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QTableWidgetItem>
#include <QFont>
#include <QJsonObject>
#include <QTableWidget>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include "gui_helpers.h"

GraphicsTab::GraphicsTab(QWidget* parent)
    : QWidget(parent)
{
    // Headline and Geek button
    QPushButton* gb = nullptr;
    QHBoxLayout* headlineLayout = createHeadlineWithGeek(this, tr("Graphics"), &gb);
    geekButton = gb;
    connect(geekButton, &QPushButton::clicked, this, &GraphicsTab::showGeekMode);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    applyMainLayoutDefaults(mainLayout);
    mainLayout->addLayout(headlineLayout);

    // Table
    tableWidget = new QTableWidget();
    tableWidget->setColumnCount(2);
    tableWidget->setHorizontalHeaderLabels(QStringList() << tr("Property") << tr("Value"));
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
    tableWidget->setColumnWidth(0, 200);
    tableWidget->setAlternatingRowColors(true);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Enable copy (Ctrl+C and right-click Copy)
    enableTableCopy(tableWidget, nullptr);

    // Scroll area
    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidget(tableWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(220);
    mainLayout->addWidget(scrollArea);

    // Populate table
    loadGraphicsInformation(tableWidget, QJsonObject());
}

void GraphicsTab::showGeekMode()
{
    GeekGraphicsDialog dlg(this);
    dlg.exec();
}

// --- GeekGraphicsDialog ---

GeekGraphicsDialog::GeekGraphicsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Graphics - Geek Mode"));
    setModal(true);
    resize(700, 500);

    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* titleLabel = new QLabel(tr("Graphics Technical Details"));
    titleLabel->setStyleSheet("font-size:16px; font-weight:bold; color:#2c3e50; margin-bottom:10px;");
    layout->addWidget(titleLabel);

    table = new QTableWidget();
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList() << tr("Property") << tr("Value"));
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #34495e; color: white; font-weight: bold; padding: 8px; border: 1px solid #2c3e50; }");
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->setColumnWidth(0, 250);

    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidget(table);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(350);
    layout->addWidget(scrollArea);

    // Buttons: Copy, Save, Close
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    QPushButton* copyBtn = new QPushButton(tr("Copy"));
    QPushButton* saveBtn = new QPushButton(tr("Save..."));
    buttonBox->addButton(copyBtn, QDialogButtonBox::ActionRole);
    buttonBox->addButton(saveBtn, QDialogButtonBox::ActionRole);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);
    
    // Fix Close button translation - apply after dialog is shown
    QTimer::singleShot(0, [buttonBox, this]() {
        if (auto closeBtn = buttonBox->button(QDialogButtonBox::Close)) {
            closeBtn->setText(tr("Close"));
        }
    });

    // Enable copy on geek table
    enableTableCopy(table, nullptr);

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
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save Graphics Info"), "graphics-info.csv", tr("CSV Files (*.csv);;All Files (*)"));
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
}

void GeekGraphicsDialog::fillTable()
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

    // === PCI GRAPHICS DEVICES (from /sys) ===
    addRow(tr("=== PCI GRAPHICS DEVICES ==="), "");
    
    QDir pciDir("/sys/bus/pci/devices");
    QStringList pciDevices = pciDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QString& pciDev : pciDevices) {
        QString classPath = QString("/sys/bus/pci/devices/%1/class").arg(pciDev);
        QFile classFile(classPath);
        
        if (classFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString deviceClass = QTextStream(&classFile).readLine().trimmed();
            classFile.close();
            
            // Check if it's a display controller (class 0x03xxxx)
            if (deviceClass.startsWith("0x03")) {
                addRow(tr("PCI Device"), pciDev);
                addRow(tr("Device Class"), deviceClass);
                
                // Read all properties
                QDir devDir(QString("/sys/bus/pci/devices/%1").arg(pciDev));
                QStringList props;
                props << "vendor" << "device" << "subsystem_vendor" << "subsystem_device" 
                      << "revision" << "class" << "irq" << "enable" << "numa_node";
                
                for (const QString& prop : props) {
                    QFile propFile(devDir.filePath(prop));
                    if (propFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        QString value = QTextStream(&propFile).readLine().trimmed();
                        propFile.close();
                        if (!value.isEmpty()) {
                            addRow(prop, value);
                        }
                    }
                }
                
                // Check driver
                QString driverPath = QString("/sys/bus/pci/devices/%1/driver").arg(pciDev);
                QFileInfo driverLink(driverPath);
                if (driverLink.isSymLink()) {
                    QString driverTarget = driverLink.symLinkTarget();
                    addRow(tr("Current Driver"), QFileInfo(driverTarget).fileName());
                }
                
                // Read uevent
                QFile ueventFile(devDir.filePath("uevent"));
                if (ueventFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString uevent = QTextStream(&ueventFile).readAll();
                    ueventFile.close();
                    addRow(tr("uevent"), uevent);
                }
                
                // Read resource info
                QFile resourceFile(devDir.filePath("resource"));
                if (resourceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString resources = QTextStream(&resourceFile).readAll();
                    resourceFile.close();
                    addRow(tr("Memory Resources"), resources.left(1000));
                }
            }
        }
    }
    
    // === DRM/DRI INFORMATION ===
    addRow(tr("=== DRM/DRI INFORMATION ==="), "");
    
    QDir drmDir("/sys/class/drm");
    QStringList drmCards = drmDir.entryList(QStringList() << "card*", QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QString& card : drmCards) {
        if (card.contains("-")) continue; // Skip connectors, only want cards
        
        addRow(tr("DRM Card"), card);
        
        QDir cardDir(drmDir.filePath(card));
        QFile deviceFile(cardDir.filePath("device/uevent"));
        if (deviceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            addRow(tr("Device uevent"), QTextStream(&deviceFile).readAll().left(500));
            deviceFile.close();
        }
    }
    
    // DRI debug info (may require root)
    QDir driDebug("/sys/kernel/debug/dri");
    if (driDebug.exists()) {
        QStringList driDirs = driDebug.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& driDir : driDirs) {
            addRow(tr("DRI Debug Dir"), driDir);
            
            QFile nameFile(QString("/sys/kernel/debug/dri/%1/name").arg(driDir));
            if (nameFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                addRow(tr("DRI Name"), QTextStream(&nameFile).readLine().trimmed());
                nameFile.close();
            }
            
            QFile clientsFile(QString("/sys/kernel/debug/dri/%1/clients").arg(driDir));
            if (clientsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                addRow(tr("DRI Clients"), QTextStream(&clientsFile).readAll().left(1000));
                clientsFile.close();
            }
        }
    }
    
    // === FRAMEBUFFER INFORMATION ===
    addRow(tr("=== FRAMEBUFFER INFORMATION ==="), "");
    
    QDir fbDir("/sys/class/graphics");
    QStringList fbDevices = fbDir.entryList(QStringList() << "fb*", QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QString& fb : fbDevices) {
        addRow(tr("Framebuffer Device"), fb);
        
        QDir fbDevDir(fbDir.filePath(fb));
        QStringList fbProps;
        fbProps << "bits_per_pixel" << "modes" << "virtual_size" << "stride" << "state" << "rotate" << "name";
        
        for (const QString& prop : fbProps) {
            QFile propFile(fbDevDir.filePath(prop));
            if (propFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString value = QTextStream(&propFile).readLine().trimmed();
                propFile.close();
                if (!value.isEmpty()) {
                    addRow(QString("fb/%1").arg(prop), value);
                }
            }
        }
    }
    
    // === DISPLAY CONNECTORS ===
    addRow(tr("=== DISPLAY CONNECTORS ==="), "");
    
    QStringList connectors = drmDir.entryList(QStringList() << "card*-*", QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& connector : connectors) {
        addRow(tr("Connector"), connector);
        
        QDir connDir(drmDir.filePath(connector));
        QStringList connProps;
        connProps << "status" << "enabled" << "dpms" << "modes";
        
        for (const QString& prop : connProps) {
            QFile propFile(connDir.filePath(prop));
            if (propFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString value = QTextStream(&propFile).readAll().trimmed();
                propFile.close();
                if (!value.isEmpty()) {
                    addRow(QString("%1/%2").arg(connector, prop), value.left(500));
                }
            }
        }
        
        // EDID data
        QFile edidFile(connDir.filePath("edid"));
        if (edidFile.open(QIODevice::ReadOnly)) {
            QByteArray edidData = edidFile.readAll();
            edidFile.close();
            if (!edidData.isEmpty()) {
                addRow(tr("EDID Size"), QString::number(edidData.size()) + " bytes");
                addRow(tr("EDID (hex)"), edidData.toHex().left(200));
            }
        }
    }
    
    // === BACKLIGHT INFORMATION ===
    QDir backlightDir("/sys/class/backlight");
    if (backlightDir.exists()) {
        addRow(tr("=== BACKLIGHT INFORMATION ==="), "");
        QStringList backlights = backlightDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        
        for (const QString& backlight : backlights) {
            addRow(tr("Backlight Device"), backlight);
            
            QDir blDir(backlightDir.filePath(backlight));
            QStringList blProps;
            blProps << "brightness" << "max_brightness" << "actual_brightness" << "type";
            
            for (const QString& prop : blProps) {
                QFile propFile(blDir.filePath(prop));
                if (propFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString value = QTextStream(&propFile).readLine().trimmed();
                    propFile.close();
                    addRow(QString("backlight/%1").arg(prop), value);
                }
            }
        }
    }
    
    // === GPU FREQUENCY/POWER ===
    addRow(tr("=== GPU FREQUENCY/POWER ==="), "");
    
    // Scan all DRM cards for frequency/power information
    for (const QString& card : drmCards) {
        if (card.contains("-")) continue; // Skip connectors
        
        QDir cardDevDir(QString("/sys/class/drm/%1").arg(card));
        
        // Check for AMDGPU frequency
        QFile amdgpuFreq(cardDevDir.filePath("device/pp_dpm_sclk"));
        if (amdgpuFreq.open(QIODevice::ReadOnly | QIODevice::Text)) {
            addRow(tr("AMDGPU Clock Levels (%1)").arg(card), QTextStream(&amdgpuFreq).readAll().left(500));
            amdgpuFreq.close();
        }
        
        // Check for Intel GPU frequencies
        QStringList intelFreqFiles;
        intelFreqFiles << "gt_cur_freq_mhz" << "gt_act_freq_mhz" << "gt_max_freq_mhz" 
                       << "gt_min_freq_mhz" << "gt_boost_freq_mhz" 
                       << "gt_RP0_freq_mhz" << "gt_RP1_freq_mhz" << "gt_RPn_freq_mhz";
        
        for (const QString& freqFile : intelFreqFiles) {
            QFile intelFreq(cardDevDir.filePath(freqFile));
            if (intelFreq.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString label = freqFile;
                label.replace("gt_", "Intel GPU ").replace("_", " ");
                addRow(tr("%1 (%2)").arg(label, card), QTextStream(&intelFreq).readLine().trimmed() + " MHz");
                intelFreq.close();
            }
        }
        
        // Check for NVIDIA GPU power/frequency
        QFile nvidiaPower(cardDevDir.filePath("device/power_state"));
        if (nvidiaPower.open(QIODevice::ReadOnly | QIODevice::Text)) {
            addRow(tr("NVIDIA Power State (%1)").arg(card), QTextStream(&nvidiaPower).readLine().trimmed());
            nvidiaPower.close();
        }
    }
}
#include "pc_tab.h"
#include "gui_helpers.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QFile>
#include <QIODevice>
#include <QTableWidget>
#include <QScrollArea>
#include <QFont>
#include <QClipboard>
#include <QGuiApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QTextStream>
#include <QShowEvent>
#include <QDir>
#include <QProcess>

PCTab::PCTab(QWidget* parent)
    : QWidget(parent)
{
    // Headline and Geek button (use helper to guarantee identical placement)
    QPushButton* gb = nullptr;
    QHBoxLayout* headlineLayout = createHeadlineWithGeek(this, tr("PC Info"), &gb);
    geekButton = gb;
    connect(geekButton, &QPushButton::clicked, this, &PCTab::showGeekMode);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    applyMainLayoutDefaults(mainLayout);
    mainLayout->addLayout(headlineLayout);

    // Table
    tableWidget = new QTableWidget();
    tableWidget->setColumnCount(2);
    tableWidget->setHorizontalHeaderLabels({tr("Property"), tr("Value")});
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->horizontalHeader()->setStyleSheet(
        "QHeaderView::section { background-color: #2c3e50; color: white; "
        "padding: 5px; font-weight: bold; }"
    );
    tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    tableWidget->setAlternatingRowColors(true);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Scroll area
    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidget(tableWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(220);
    mainLayout->addWidget(scrollArea);

    // Enable copy (Ctrl+C and right-click Copy)
    enableTableCopy(tableWidget);

    // Load information
    loadPCInformation();
}

void PCTab::loadPCInformation()
{
    tableWidget->setRowCount(0);
    
    auto addRow = [this](const QString& property, const QString& value) {
        if (value.isEmpty()) return; // Skip empty values
        int row = tableWidget->rowCount();
        tableWidget->insertRow(row);
        
        QTableWidgetItem* propItem = new QTableWidgetItem(property);
        QFont boldFont;
        boldFont.setBold(true);
        propItem->setFont(boldFont);
        tableWidget->setItem(row, 0, propItem);
        
        tableWidget->setItem(row, 1, new QTableWidgetItem(value));
    };
    
    // Read DMI information
    auto readDMI = [](const QString& file) -> QString {
        QFile f(QString("/sys/class/dmi/id/%1").arg(file));
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QString::fromLocal8Bit(f.readAll()).trimmed();
            f.close();
            // Filter out placeholder values
            if (content != "To Be Filled By O.E.M." && 
                content != "Not Specified" && 
                content != "Default string" &&
                content != "None" &&
                !content.isEmpty()) {
                return content;
            }
        }
        return QString();
    };
    
    // Get hostname
    QFile hostnameFile("/etc/hostname");
    QString hostname;
    if (hostnameFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        hostname = QString::fromLocal8Bit(hostnameFile.readAll()).trimmed();
        hostnameFile.close();
    }
    
    // PC Type based on chassis
    QString chassisType = readDMI("chassis_type");
    QString pcType;
    if (!chassisType.isEmpty()) {
        bool ok = false;
        int chassisNum = chassisType.toInt(&ok);
        if (ok) {
            switch (chassisNum) {
                case 3: pcType = "Desktop"; break;
                case 4: pcType = "Low Profile Desktop"; break;
                case 6: pcType = "Mini Tower"; break;
                case 7: pcType = "Tower"; break;
                case 8: case 9: pcType = "Laptop"; break;
                case 10: case 14: pcType = "Notebook"; break;
                case 11: pcType = "Hand Held"; break;
                case 30: case 31: case 32: pcType = "Tablet"; break;
                case 13: pcType = "All In One"; break;
                default: pcType = QString("Type %1").arg(chassisNum);
            }
        }
    }
    
    // Display normal mode information
    addRow(tr("Computer Name"), hostname);
    if (!pcType.isEmpty()) addRow(tr("PC Type"), pcType);
    addRow(tr("Manufacturer"), readDMI("sys_vendor"));
    addRow(tr("Product Name"), readDMI("product_name"));
    addRow(tr("Product Family"), readDMI("product_family"));
    addRow(tr("Version"), readDMI("product_version"));
    addRow(tr("Serial Number"), readDMI("product_serial"));
    addRow(tr("SKU Number"), readDMI("product_sku"));
    addRow(tr("UUID"), readDMI("product_uuid"));
}

void PCTab::showGeekMode()
{
    GeekPCDialog dlg(this);
    dlg.exec();
}

// ===== Geek Mode Dialog =====

GeekPCDialog::GeekPCDialog(QWidget* parent)
    : QDialog(parent)
    , refreshTimer(new QTimer(this))
{
    setWindowTitle(tr("PC Info - Geek Mode"));
    setModal(true);
    resize(700, 500);

    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* titleLabel = new QLabel(tr("PC Technical Details"));
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

    // Buttons: Copy, Save, Close
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    QPushButton* copyBtn = new QPushButton(tr("Copy"));
    QPushButton* saveBtn = new QPushButton(tr("Save..."));
    buttonBox->addButton(copyBtn, QDialogButtonBox::ActionRole);
    buttonBox->addButton(saveBtn, QDialogButtonBox::ActionRole);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);

    // Enable copy on main geek table (right-click + Ctrl+C)
    enableTableCopy(table, refreshTimer);

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
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save PC Info"), "pc-info.csv", tr("CSV Files (*.csv);;All Files (*)"));
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

    // Auto-refresh every second while visible
    refreshTimer->setInterval(1000);
    connect(refreshTimer, &QTimer::timeout, this, &GeekPCDialog::fillTable);
}

void GeekPCDialog::showEvent(QShowEvent* ev)
{
    QDialog::showEvent(ev);
    if (refreshTimer) refreshTimer->start();
}

void GeekPCDialog::hideEvent(QHideEvent* ev)
{
    if (refreshTimer) refreshTimer->stop();
    QDialog::hideEvent(ev);
}

void GeekPCDialog::fillTable()
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
        
        // Replace newlines with arrow for better display
        QString displayVal = val;
        displayVal.replace("\n", " ↳ ");
        
        QTableWidgetItem* v = new QTableWidgetItem(displayVal);
        v->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(row, 1, v);
        table->resizeRowToContents(row);
        ++row;
    };
    
    // === DMI/SMBIOS INFORMATION ===
    addRow(tr("=== DMI/SMBIOS INFORMATION ==="), "");
    
    QDir dmiDir("/sys/class/dmi/id");
    if (dmiDir.exists()) {
        QStringList dmiFiles = dmiDir.entryList(QDir::Files, QDir::Name);
        for (const QString& file : dmiFiles) {
            QFile f(dmiDir.filePath(file));
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString content = QString::fromLocal8Bit(f.readAll()).trimmed();
                f.close();
                if (!content.isEmpty()) {
                    addRow(file, content);
                }
            }
        }
    }
    
    // === HOSTNAME INFORMATION ===
    addRow("", "");
    addRow(tr("=== HOSTNAME INFORMATION ==="), "");
    
    QFile hostnameFile("/etc/hostname");
    if (hostnameFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addRow("/etc/hostname", QString::fromLocal8Bit(hostnameFile.readAll()).trimmed());
        hostnameFile.close();
    }
    
    QFile hostsFile("/etc/hosts");
    if (hostsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString hosts = QString::fromLocal8Bit(hostsFile.readAll());
        hostsFile.close();
        addRow("/etc/hosts", hosts.left(500));
    }
    
    // === MACHINE ID ===
    addRow("", "");
    addRow(tr("=== MACHINE ID ==="), "");
    
    QFile machineIdFile("/etc/machine-id");
    if (machineIdFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addRow("/etc/machine-id", QString::fromLocal8Bit(machineIdFile.readAll()).trimmed());
        machineIdFile.close();
    }
    
    QFile dbusIdFile("/var/lib/dbus/machine-id");
    if (dbusIdFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addRow("/var/lib/dbus/machine-id", QString::fromLocal8Bit(dbusIdFile.readAll()).trimmed());
        dbusIdFile.close();
    }
    
    // === HOSTNAMECTL OUTPUT ===
    addRow("", "");
    addRow(tr("=== HOSTNAMECTL OUTPUT ==="), "");
    
    QProcess hostnamectl;
    hostnamectl.start("hostnamectl", QStringList());
    hostnamectl.waitForFinished(3000);
    QString hostnameOutput = hostnamectl.readAllStandardOutput();
    if (!hostnameOutput.isEmpty()) {
        QStringList lines = hostnameOutput.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            if (line.contains(':')) {
                QString key = line.section(':', 0, 0).trimmed();
                QString value = line.section(':', 1).trimmed();
                addRow(key, value);
            }
        }
    }
    
    // === FIRMWARE/BIOS INFORMATION ===
    addRow("", "");
    addRow(tr("=== FIRMWARE/BIOS INFORMATION ==="), "");
    
    QDir biosDir("/sys/firmware");
    if (biosDir.exists("efi")) {
        addRow("Firmware Type", "UEFI");
        
        QFile efiVars("/sys/firmware/efi/fw_platform_size");
        if (efiVars.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString size = QString::fromLocal8Bit(efiVars.readAll()).trimmed();
            addRow("EFI Platform Size", size + " bit");
            efiVars.close();
        }
    } else {
        addRow("Firmware Type", "Legacy BIOS");
    }
    
    // === ACPI INFORMATION ===
    addRow("", "");
    addRow(tr("=== ACPI INFORMATION ==="), "");
    
    QDir acpiDir("/sys/firmware/acpi");
    if (acpiDir.exists()) {
        QDir tablesDir("/sys/firmware/acpi/tables");
        if (tablesDir.exists()) {
            QStringList tables = tablesDir.entryList(QDir::Files, QDir::Name);
            addRow("ACPI Tables", tables.join(", "));
        }
    }
    
    // === BOOT INFORMATION ===
    addRow("", "");
    addRow(tr("=== BOOT INFORMATION ==="), "");
    
    QFile cmdlineFile("/proc/cmdline");
    if (cmdlineFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addRow("/proc/cmdline", QString::fromLocal8Bit(cmdlineFile.readAll()).trimmed());
        cmdlineFile.close();
    }
    
    // === SYSTEM UPTIME ===
    addRow("", "");
    addRow(tr("=== SYSTEM UPTIME ==="), "");
    
    QFile uptimeFile("/proc/uptime");
    if (uptimeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString uptimeStr = QString::fromLocal8Bit(uptimeFile.readAll());
        double uptime = uptimeStr.section(' ', 0, 0).toDouble();
        int days = static_cast<int>(uptime / 86400);
        int hours = static_cast<int>((uptime - days * 86400) / 3600);
        int minutes = static_cast<int>((uptime - days * 86400 - hours * 3600) / 60);
        addRow("Uptime", QString("%1 days, %2 hours, %3 minutes").arg(days).arg(hours).arg(minutes));
        addRow("Uptime (seconds)", QString::number(uptime, 'f', 2));
        uptimeFile.close();
    }
}
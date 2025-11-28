#include "motherboard_tab.h"
#include "mainboard.h"
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
#include <QProgressDialog>
#include "gui_helpers.h"

MotherboardTab::MotherboardTab(QWidget* parent)
    : QWidget(parent)
{
    // Headline and Geek button (use helper to guarantee identical placement)
    QPushButton* gb = nullptr;
    QHBoxLayout* headlineLayout = createHeadlineWithGeek(this, tr("Motherboard"), &gb);
    geekButton = gb;
    connect(geekButton, &QPushButton::clicked, this, &MotherboardTab::showGeekMode);

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

    // Populate table (no progress dialog - motherboard info doesn't change)
    loadMainboardInformation(tableWidget, QJsonObject());
}

void MotherboardTab::showGeekMode()
{
    GeekMotherboardDialog dlg(this);
    dlg.exec();
}

// --- GeekMotherboardDialog ---

GeekMotherboardDialog::GeekMotherboardDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Motherboard - Geek Mode"));
    setModal(true);
    resize(700, 500);

    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* titleLabel = new QLabel(tr("Motherboard Technical Details"));
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
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save Motherboard Info"), "motherboard-info.csv", tr("CSV Files (*.csv);;All Files (*)"));
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

    // Fill table (no progress dialog - motherboard info doesn't change)
    fillTable();
}

void GeekMotherboardDialog::fillTable()
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

    // === DMI BASEBOARD ===
    QProcess dmiBaseboard;
    dmiBaseboard.start("dmidecode", QStringList() << "-t" << "baseboard");
    dmiBaseboard.waitForFinished(3000);
    QString baseboardOutput = dmiBaseboard.readAllStandardOutput();
    
    if (!baseboardOutput.isEmpty()) {
        addRow(tr("=== BASEBOARD (DMI Type 2) ==="), "");
        QStringList lines = baseboardOutput.split('\n');
        for (const QString& line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.isEmpty() || trimmed.startsWith("Base Board") || 
                trimmed.startsWith("Handle") || trimmed.startsWith("DMI type")) continue;
            
            if (trimmed.contains(':')) {
                QStringList parts = trimmed.split(':', Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    QString key = parts[0].trimmed();
                    QString value = parts.mid(1).join(':').trimmed();
                    if (!value.isEmpty() && value != "Not Specified" && value != "To Be Filled By O.E.M.") {
                        addRow(key, value);
                    }
                }
            } else if (trimmed.startsWith("Features:") || trimmed.startsWith("Chassis Handle:")) {
                // Skip subheadings
            } else {
                // Potentially multi-line values or bullet points
                if (row > 0 && !trimmed.isEmpty()) {
                    QTableWidgetItem* lastVal = table->item(row - 1, 1);
                    if (lastVal) {
                        QString current = lastVal->text();
                        if (!current.isEmpty()) {
                            lastVal->setText(current + "\n" + trimmed);
                        }
                    }
                }
            }
        }
    }

    // === DMI SYSTEM ===
    QProcess dmiSystem;
    dmiSystem.start("dmidecode", QStringList() << "-t" << "system");
    dmiSystem.waitForFinished(3000);
    QString systemOutput = dmiSystem.readAllStandardOutput();
    
    if (!systemOutput.isEmpty()) {
        addRow(tr("=== SYSTEM (DMI Type 1) ==="), "");
        QStringList lines = systemOutput.split('\n');
        for (const QString& line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.isEmpty() || trimmed.startsWith("System Information") || 
                trimmed.startsWith("Handle") || trimmed.startsWith("DMI type")) continue;
            
            if (trimmed.contains(':')) {
                QStringList parts = trimmed.split(':', Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    QString key = parts[0].trimmed();
                    QString value = parts.mid(1).join(':').trimmed();
                    if (!value.isEmpty() && value != "Not Specified" && value != "To Be Filled By O.E.M.") {
                        addRow(key, value);
                    }
                }
            }
        }
    }

    // === DMI BIOS ===
    QProcess dmiBios;
    dmiBios.start("dmidecode", QStringList() << "-t" << "bios");
    dmiBios.waitForFinished(3000);
    QString biosOutput = dmiBios.readAllStandardOutput();
    
    if (!biosOutput.isEmpty()) {
        addRow(tr("=== BIOS (DMI Type 0) ==="), "");
        QStringList lines = biosOutput.split('\n');
        for (const QString& line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.isEmpty() || trimmed.startsWith("BIOS Information") || 
                trimmed.startsWith("Handle") || trimmed.startsWith("DMI type")) continue;
            
            if (trimmed.contains(':')) {
                QStringList parts = trimmed.split(':', Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    QString key = parts[0].trimmed();
                    QString value = parts.mid(1).join(':').trimmed();
                    if (!value.isEmpty()) {
                        addRow(key, value);
                    }
                }
            } else if (trimmed.startsWith("Characteristics:") || trimmed.startsWith("BIOS Revision:")) {
                // Handle these specially
            } else {
                // Multi-line characteristics
                if (row > 0 && !trimmed.isEmpty() && trimmed != "Characteristics:") {
                    QTableWidgetItem* lastVal = table->item(row - 1, 1);
                    if (lastVal) {
                        QString current = lastVal->text();
                        if (!current.isEmpty()) {
                            lastVal->setText(current + "\n" + trimmed);
                        }
                    }
                }
            }
        }
    }

    // === DMI CHASSIS ===
    QProcess dmiChassis;
    dmiChassis.start("dmidecode", QStringList() << "-t" << "chassis");
    dmiChassis.waitForFinished(3000);
    QString chassisOutput = dmiChassis.readAllStandardOutput();
    
    if (!chassisOutput.isEmpty()) {
        addRow(tr("=== CHASSIS (DMI Type 3) ==="), "");
        QStringList lines = chassisOutput.split('\n');
        for (const QString& line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.isEmpty() || trimmed.startsWith("Chassis Information") || 
                trimmed.startsWith("Handle") || trimmed.startsWith("DMI type")) continue;
            
            if (trimmed.contains(':')) {
                QStringList parts = trimmed.split(':', Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    QString key = parts[0].trimmed();
                    QString value = parts.mid(1).join(':').trimmed();
                    if (!value.isEmpty() && value != "Not Specified" && value != "To Be Filled By O.E.M.") {
                        addRow(key, value);
                    }
                }
            }
        }
    }

    // === LSHW BUS ===
    QProcess lshwBus;
    lshwBus.start("lshw", QStringList() << "-C" << "bus");
    lshwBus.waitForFinished(3000);
    QString lshwOutput = lshwBus.readAllStandardOutput();
    
    if (!lshwOutput.isEmpty()) {
        addRow(tr("=== LSHW BUS OUTPUT ==="), "");
        addRow(tr("Full lshw -C bus output"), lshwOutput.trimmed().left(10000));
    }

    // === LSPCI CHIPSET INFO ===
    QProcess lspci;
    lspci.start("lspci", QStringList() << "-v");
    lspci.waitForFinished(3000);
    QString lspciOutput = lspci.readAllStandardOutput();
    
    if (!lspciOutput.isEmpty()) {
        addRow(tr("=== CHIPSET & BRIDGES ==="), "");
        QStringList lines = lspciOutput.split('\n');
        for (const QString& line : lines) {
            if (line.contains("Host bridge:", Qt::CaseInsensitive) || 
                line.contains("ISA bridge:", Qt::CaseInsensitive) ||
                line.contains("PCI bridge:", Qt::CaseInsensitive)) {
                QString cleaned = line.trimmed();
                if (!cleaned.isEmpty()) {
                    QStringList parts = cleaned.split(':');
                    if (parts.size() >= 2) {
                        QString device = parts[0].trimmed();
                        QString description = parts.mid(1).join(':').trimmed();
                        addRow(device, description);
                    }
                }
            }
        }
    }

    // === USB CONTROLLERS ===
    addRow(tr("=== USB CONTROLLERS ==="), "");
    QStringList usbLines = lspciOutput.split('\n');
    int usbCount = 1;
    for (const QString& line : usbLines) {
        if (line.contains("USB controller:", Qt::CaseInsensitive)) {
            QString cleaned = line.trimmed();
            QStringList parts = cleaned.split(':');
            if (parts.size() >= 2) {
                QString description = parts.mid(1).join(':').trimmed();
                addRow(tr("USB Controller %1").arg(usbCount++), description);
            }
        }
    }
}


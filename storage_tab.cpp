#include "storage_tab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QGroupBox>
#include <QFont>
#include <QRegularExpression>
#include <QDebug>
#include <QtConcurrent/QtConcurrentRun>
#include <QFuture>
#include <QFutureWatcher>
#include <QCoreApplication>
#include "gui_helpers.h"
#include "log_helper.h"
#include <QTableWidget>
#include <QFileInfo>
#include <QFrame>
#include <QTimer>
#include <QShowEvent>
#include <QHideEvent>
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QClipboard>
#include <QGuiApplication>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QScrollBar>
#include <QMessageBox>

StorageTab::StorageTab(QWidget* parent)
    : TabWidgetBase(QCoreApplication::translate("StorageTab", "Storage"),
                    "lsblk -P -o NAME,PKNAME,SIZE,MOUNTPOINT,FSTYPE,TYPE,MODEL,VENDOR,TRAN && df -P -h",
                    true,
                    "lsblk -P -o NAME,PKNAME,SIZE,MOUNTPOINT,FSTYPE,TYPE,MODEL,VENDOR,TRAN && df -P -h",
                    parent)
{
    qDebug() << "StorageTab: Constructor called";
    
    refreshTimer = new QTimer(this);
    refreshTimer->setInterval(3000);
    connect(refreshTimer, &QTimer::timeout, this, &StorageTab::refreshData);
    
    parseWatcher = new QFutureWatcher<QVariantMap>(this);
    connect(parseWatcher, &QFutureWatcher<QVariantMap>::finished, this, &StorageTab::onParseFinished);

    initializeTab();
    qDebug() << "StorageTab: Constructor finished";
}

QWidget* StorageTab::createUserFriendlyView() {
    QWidget* contentWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);

    // Headline and Geek button (use helper to guarantee identical placement)
    QPushButton* gb = nullptr;
    QHBoxLayout* headlineLayout = createHeadlineWithGeek(contentWidget, tr("Storage"), &gb);
    geekButton = gb;
    connect(geekButton, &QPushButton::clicked, this, &StorageTab::showGeekMode);
    
    applyMainLayoutDefaults(mainLayout);
    mainLayout->addLayout(headlineLayout);

    // Create container for disk info with CPU-style scroll area
    diskInfoContainer = new QWidget();
    diskInfoLayout = new QVBoxLayout(diskInfoContainer);
    diskInfoLayout->setContentsMargins(10, 10, 10, 10);
    diskInfoLayout->setSpacing(15);

    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidget(diskInfoContainer);
    scrollArea->setWidgetResizable(true);
    
    mainLayout->addWidget(scrollArea);

    // Start initial data parsing
    parseOutput("");

    return contentWidget;
}

QTableWidget* StorageTab::createDiskTable() {
    QTableWidget* table = new QTableWidget();
    table->setColumnCount(7);
    QStringList headers = {"Device", "Size", "Used", "Available", "Use%", "Mount Point", "Filesystem"};
    table->setHorizontalHeaderLabels(headers);
    
    // Apply CPU-style header formatting
    table->horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "    background-color: #2c3e50;"
        "    color: white;"
        "    padding: 8px;"
        "    border: none;"
        "    font-weight: bold;"
        "}"
    );
    
    // Set all columns to equal width
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    
    // Calculate equal column width
    int totalWidth = 800; // Default table width
    int columnWidth = totalWidth / 7;
    for (int i = 0; i < 7; ++i) {
        table->setColumnWidth(i, columnWidth);
    }
    
    table->setAlternatingRowColors(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->verticalHeader()->setVisible(false);
    
    // Enable word wrapping for multiline cells
    table->setWordWrap(true);
    
    // Enable rich text support for HTML formatting
    table->setTextElideMode(Qt::ElideNone);
    
    // Custom stylesheet for green arrows
    table->setStyleSheet(
        "QTableWidget {"
        "    color: black;"
        "}"
        "QTableWidget::item {"
        "    padding: 4px;"
        "}"
    );
    
    // Add custom stylesheet for green arrows
    table->setStyleSheet(
        "QTableWidget {"
        "    border: none;"
        "}"
        "QTableWidget::item {"
        "    color: black;"  // Ensure text is black
        "}"
    );
    
    // Remove scrollbars and adapt to content
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // Size table to fit content
    table->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    table->setMaximumHeight(16777215); // Remove height constraint
    
    return table;
}

void StorageTab::parseOutput(const QString& output) {
    // Run parsing in background
    QFuture<QVariantMap> future = QtConcurrent::run([this, output]() {
        return parseStorageData(output);
    });
    parseWatcher->setFuture(future);
}

void StorageTab::onParseFinished() {
    QVariantMap data = parseWatcher->result();
    applyParsedPartitions(data);
}

QVariantMap StorageTab::parseStorageData(const QString& output) {
    QVariantMap result;
    QVariantMap disks;
    QVariantList partitions;
    
    QString command = m_command;
    QProcess process;
    process.start("bash", QStringList() << "-c" << command);
    process.waitForFinished();
    QString actualOutput = process.readAllStandardOutput();

    if (actualOutput.isEmpty()) {
        return result;
    }

    QStringList lines = actualOutput.split('\n', Qt::SkipEmptyParts);
    
    // Parse lsblk output (first part before df)
    QRegularExpression rx("(\\w+)=\"([^\"]*)\"");
    QMap<QString, QVariantMap> deviceMap;
    
    for (const QString& line : lines) {
        if (line.startsWith("NAME=")) {
            QVariantMap device;
            QRegularExpressionMatchIterator matches = rx.globalMatch(line);
            
            while (matches.hasNext()) {
                QRegularExpressionMatch match = matches.next();
                QString key = match.captured(1);
                QString value = match.captured(2);
                device[key.toLower()] = value;
            }
            
            if (!device.isEmpty()) {
                QString name = device["name"].toString();
                deviceMap[name] = device;
                
                // Determine if this is a disk or partition
                QString type = device["type"].toString();
                if (type == "disk") {
                    QString diskName = name;
                    QVariantMap diskInfo;
                    
                    // Get transport type (USB, ATA, etc.)
                    QString transport = device["tran"].toString();
                    if (transport.isEmpty()) {
                        transport = "Unknown";
                    }
                    
                    // Build device name from model and vendor
                    QString model = device["model"].toString();
                    QString vendor = device["vendor"].toString();
                    QString deviceName = "Unknown";
                    
                    if (!model.isEmpty() && !vendor.isEmpty()) {
                        deviceName = vendor + " " + model;
                    } else if (!model.isEmpty()) {
                        deviceName = model;
                    } else if (!vendor.isEmpty()) {
                        deviceName = vendor;
                    }
                    
                    diskInfo["transport"] = transport;
                    diskInfo["device_name"] = deviceName;
                    diskInfo["total_size"] = device["size"].toString();
                    disks[diskName] = diskInfo;
                }
            }
        }
        else if (line.startsWith("/dev/")) {
            // Parse df output
            QStringList fields = line.split(QRegularExpression("\\s+"));
            if (fields.size() >= 6) {
                QString device = fields[0];
                QString size = fields[1];
                QString used = fields[2];
                QString available = fields[3];
                QString usePercent = fields[4];
                QString mountPoint = fields[5];
                
                // Find filesystem from lsblk data
                QString deviceName = device;
                if (deviceName.startsWith("/dev/")) {
                    deviceName = deviceName.mid(5);
                }
                
                QString filesystem = "Unknown";
                if (deviceMap.contains(deviceName)) {
                    filesystem = deviceMap[deviceName]["fstype"].toString();
                    if (filesystem.isEmpty()) {
                        filesystem = "Unknown";
                    }
                }
                
                QVariantMap partition;
                partition["device"] = device;
                partition["size"] = formatSizeLocale(size);
                partition["used"] = formatSizeLocale(used);
                partition["available"] = formatSizeLocale(available);
                partition["use_percent"] = usePercent;
                partition["mount_point"] = mountPoint;
                partition["filesystem"] = filesystem;
                
                partitions.append(partition);
            }
        }
    }
    
    result["disks"] = disks;
    result["partitions"] = partitions;
    return result;
}

void StorageTab::applyParsedPartitions(const QVariantMap& data) {
    if (data.isEmpty()) {
        return;
    }

    // Clear existing widgets and vectors
    QLayoutItem* item;
    while ((item = diskInfoLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    diskTables.clear();
    diskLabels.clear();

    // Get disk data
    QVariantMap disks = data["disks"].toMap();
    QVariantList allPartitions = data["partitions"].toList();
    
    for (auto it = disks.constBegin(); it != disks.constEnd(); ++it) {
        QString diskName = it.key();
        QVariantMap diskData = it.value().toMap();
        
        // Format size with locale-aware formatting
        QString sizeText = diskData["total_size"].toString();
        QString formattedSize = formatSizeLocale(sizeText);
        
        // Create first line: Disk: sdb - Type: USB - Total size: 29.82 GB
        QString firstLine = QString("Disk: %1 - Type: %2 - Total size: %3")
                           .arg(diskName)
                           .arg(diskData["transport"].toString())
                           .arg(formattedSize);
        
        // Create second line: Name: Corsair Survivor 3.0
        QString secondLine = QString("Name: %1")
                           .arg(diskData["device_name"].toString());
        
        // Create label with two centered lines
        QLabel* diskLabel = new QLabel(firstLine + "\n" + secondLine);
        QFont font = diskLabel->font();
        font.setBold(true);
        diskLabel->setFont(font);
        diskLabel->setAlignment(Qt::AlignCenter);
        diskLabel->setWordWrap(true);
        diskInfoLayout->addWidget(diskLabel);
        diskLabels.append(diskLabel);
        
        // Create table for this disk
        QTableWidget* diskTable = createDiskTable();
        
        // Filter partitions for this disk
        QVariantList diskPartitions;
        for (const QVariant& partVar : allPartitions) {
            QVariantMap partition = partVar.toMap();
            QString device = partition["device"].toString();
            if (device.startsWith("/dev/" + diskName)) {
                diskPartitions.append(partition);
            }
        }
        
        // Only create and show table if there are partitions
        if (!diskPartitions.isEmpty()) {
            // Populate table with this disk's partitions
            diskTable->setRowCount(diskPartitions.size());
            
            // Calculate column width for truncation
            int columnWidth = diskTable->columnWidth(0);
            QFont tableFont = diskTable->font();
            
            for (int i = 0; i < diskPartitions.size(); ++i) {
                QVariantMap partition = diskPartitions[i].toMap();
                
                // Create items with truncation detection and HTML formatting
                QString deviceText = formatTextWithTruncation(partition["device"].toString(), columnWidth, tableFont);
                QTableWidgetItem* deviceItem = createColoredTextItem(deviceText);
                diskTable->setItem(i, 0, deviceItem);
                
                QString sizeText = formatTextWithTruncation(partition["size"].toString(), columnWidth, tableFont);
                QTableWidgetItem* sizeItem = createColoredTextItem(sizeText);
                diskTable->setItem(i, 1, sizeItem);
                
                QString usedText = formatTextWithTruncation(partition["used"].toString(), columnWidth, tableFont);
                QTableWidgetItem* usedItem = createColoredTextItem(usedText);
                diskTable->setItem(i, 2, usedItem);
                
                QString availableText = formatTextWithTruncation(partition["available"].toString(), columnWidth, tableFont);
                QTableWidgetItem* availableItem = createColoredTextItem(availableText);
                diskTable->setItem(i, 3, availableItem);
                
                QString usePercentText = formatTextWithTruncation(partition["use_percent"].toString(), columnWidth, tableFont);
                QTableWidgetItem* usePercentItem = createColoredTextItem(usePercentText);
                diskTable->setItem(i, 4, usePercentItem);
                
                QString mountText = formatTextWithTruncation(partition["mount_point"].toString(), columnWidth, tableFont);
                QTableWidgetItem* mountItem = createColoredTextItem(mountText);
                diskTable->setItem(i, 5, mountItem);
                
                QString filesystemText = formatTextWithTruncation(partition["filesystem"].toString(), columnWidth, tableFont);
                QTableWidgetItem* filesystemItem = createColoredTextItem(filesystemText);
                diskTable->setItem(i, 6, filesystemItem);
            }
            
            // Resize table to fit content with proper row heights for multiline text
            diskTable->resizeRowsToContents();
            int totalHeight = 0;
            for (int i = 0; i < diskTable->rowCount(); ++i) {
                totalHeight += diskTable->rowHeight(i);
            }
            totalHeight += diskTable->horizontalHeader()->height() + 2; // Add header height and border
            diskTable->setFixedHeight(totalHeight);
            
            diskInfoLayout->addWidget(diskTable);
            diskTables.append(diskTable);
            
            // Enable copy functionality for this table
            enableTableCopy(diskTable, refreshTimer);
        } else {
            // No partitions, just show a message or skip the table entirely
            QLabel* noPartitionsLabel = new QLabel("No mounted partitions found for this disk");
            noPartitionsLabel->setAlignment(Qt::AlignCenter);
            noPartitionsLabel->setStyleSheet("color: #666; font-style: italic; padding: 10px;");
            diskInfoLayout->addWidget(noPartitionsLabel);
            
            // Delete the unused table
            delete diskTable;
        }
        
        // Add spacing between disk sections
        diskInfoLayout->addSpacing(10);
    }
    
    // Add stretch at the end to eliminate extra space at bottom
    diskInfoLayout->addStretch();
}

QString StorageTab::formatSizeLocale(const QString& sizeStr) {
    // Convert sizes like "238.52G" to "238,52 GB" (European locale style)
    if (sizeStr.isEmpty() || sizeStr == "-") {
        return sizeStr;
    }
    
    QRegularExpression sizeRegex("^([0-9]+(?:\\.[0-9]+)?)([KMGTPE]?)$");
    QRegularExpressionMatch match = sizeRegex.match(sizeStr);
    
    if (match.hasMatch()) {
        QString number = match.captured(1);
        QString unit = match.captured(2);
        
        // Replace decimal point with comma
        number.replace('.', ',');
        
        // Add "B" to unit if not empty
        if (!unit.isEmpty()) {
            unit += "B";
        }
        
        return number + " " + unit;
    }
    
    return sizeStr;
}

QString StorageTab::formatTextWithTruncation(const QString& text, int maxWidth, const QFont& font) {
    QFontMetrics fm(font);
    
    // Check if text would wrap by measuring its width against column width
    int textWidth = fm.horizontalAdvance(text);
    
    // If text is longer than column width, it will wrap
    if (textWidth > maxWidth) {
        QString arrow = " ↵";  // Plain black arrow
        int arrowWidth = fm.horizontalAdvance(arrow);
        int availableWidth = maxWidth - arrowWidth;
        
        // Find the best break point - preferably before a "/"
        int bestBreakPoint = -1;
        
        // Try to find the last "/" that fits within available width
        for (int i = text.length() - 1; i >= 0; --i) {
            if (text[i] == '/') {
                QString beforeSlash = text.left(i);
                if (fm.horizontalAdvance(beforeSlash) <= availableWidth) {
                    bestBreakPoint = i;
                    break;
                }
            }
        }
        
        // If no suitable "/" found, break at character level
        if (bestBreakPoint == -1) {
            for (int i = 1; i <= text.length(); ++i) {
                QString substring = text.left(i);
                if (fm.horizontalAdvance(substring) > availableWidth) {
                    bestBreakPoint = i - 1;
                    break;
                }
            }
        }
        
        // Create result with plain arrow at end of first line
        if (bestBreakPoint > 0) {
            QString result = text.left(bestBreakPoint) + arrow + "\n" + text.mid(bestBreakPoint);
            return result;
        }
    }
    
    return text;
}

QTableWidgetItem* StorageTab::createColoredTextItem(const QString& text) {
    QTableWidgetItem* item = new QTableWidgetItem(text);
    item->setTextAlignment(Qt::AlignTop | Qt::AlignLeft);
    return item;
}

void StorageTab::refreshData() {
    // Override to use custom refresh without loading overlay
    parseOutput("");
}

void StorageTab::showEvent(QShowEvent* ev) {
    TabWidgetBase::showEvent(ev);
    if (refreshTimer) {
        refreshTimer->start();
    }
}

void StorageTab::hideEvent(QHideEvent* ev) {
    TabWidgetBase::hideEvent(ev);
    if (refreshTimer) {
        refreshTimer->stop();
    }
}

void StorageTab::populateStorageTable() {
    // Placeholder for compatibility
}

void StorageTab::showGeekMode()
{
    GeekStorageDialog dlg(this);
    dlg.exec();
}

// --- GeekStorageDialog ---

GeekStorageDialog::GeekStorageDialog(QWidget* parent)
    : QDialog(parent)
    , refreshTimer(new QTimer(this))
{
    setWindowTitle(tr("Storage - Geek Mode"));
    setModal(true);
    resize(700, 500);

    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* titleLabel = new QLabel(tr("Storage Technical Details"));
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

    // Buttons: Copy, Save, Close
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    QPushButton* copyBtn = new QPushButton(tr("Copy"));
    QPushButton* saveBtn = new QPushButton(tr("Save..."));
    buttonBox->addButton(copyBtn, QDialogButtonBox::ActionRole);
    buttonBox->addButton(saveBtn, QDialogButtonBox::ActionRole);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);

    // Enable copy on main geek table (right-click + Ctrl+C) and pause the geek dialog's refresh while copying
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
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save Storage Info"), "storage-info.csv", tr("CSV Files (*.csv);;All Files (*)"));
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

    fillTable();

    // Auto-refresh every second while visible
    refreshTimer->setInterval(1000);
    connect(refreshTimer, &QTimer::timeout, this, &GeekStorageDialog::fillTable);
}

void GeekStorageDialog::showEvent(QShowEvent* ev)
{
    QDialog::showEvent(ev);
    if (refreshTimer) refreshTimer->start();
}

void GeekStorageDialog::hideEvent(QHideEvent* ev)
{
    if (refreshTimer) refreshTimer->stop();
    QDialog::hideEvent(ev);
}

void GeekStorageDialog::fillTable()
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

    // Run lsblk with ALL available fields for maximum information
    QProcess lsblk;
    lsblk.start("lsblk", QStringList() << "-P" << "-b" << "-o" 
        << "NAME,KNAME,PATH,MAJ:MIN,FSAVAIL,FSSIZE,FSTYPE,FSUSED,FSUSE%"
        << ",MOUNTPOINT,MOUNTPOINTS,LABEL,UUID,PTUUID,PTTYPE,PARTTYPE"
        << ",PARTLABEL,PARTUUID,PARTFLAGS,RA,RO,RM,HOTPLUG,MODEL,SERIAL"
        << ",SIZE,STATE,OWNER,GROUP,MODE,ALIGNMENT,MIN-IO,OPT-IO,PHY-SEC"
        << ",LOG-SEC,ROTA,SCHED,RQ-SIZE,TYPE,DISC-ALN,DISC-GRAN,DISC-MAX"
        << ",DISC-ZERO,WSAME,WWN,RAND,PKNAME,HCTL,TRAN,SUBSYSTEMS,REV"
        << ",VENDOR,ZONED,DAX,FSROOTS,FSVER");
    lsblk.waitForFinished(5000);
    QString output = lsblk.readAllStandardOutput();

    if (!output.isEmpty()) {
        addRow("=== LSBLK Full Output ===", "");
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        
        for (const QString& line : lines) {
            if (line.startsWith("NAME=")) {
                addRow("", ""); // Blank line between devices
                
                // Parse the line into key-value pairs
                QRegularExpression rx("(\\w+)=\"([^\"]*)\"");
                QRegularExpressionMatchIterator matches = rx.globalMatch(line);
                
                while (matches.hasNext()) {
                    QRegularExpressionMatch match = matches.next();
                    QString key = match.captured(1);
                    QString value = match.captured(2);
                    
                    if (!value.isEmpty()) {
                        addRow(key, value);
                    }
                }
            }
        }
    }

    // Get df output for mounted filesystems
    QProcess df;
    df.start("df", QStringList() << "-h" << "-T");
    df.waitForFinished(5000);
    QString dfOutput = df.readAllStandardOutput();

    if (!dfOutput.isEmpty()) {
        addRow("", "");
        addRow("=== DF Output (Mounted Filesystems) ===", "");
        QStringList dfLines = dfOutput.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : dfLines) {
            addRow("", line);
        }
    }

    // Get mount information
    QFile mountFile("/proc/mounts");
    if (mountFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addRow("", "");
        addRow("=== /proc/mounts ===", "");
        QTextStream in(&mountFile);
        while (!in.atEnd()) {
            QString line = in.readLine();
            addRow("", line);
        }
        mountFile.close();
    }

    // Get blkid information (requires root typically, but try anyway)
    QProcess blkid;
    blkid.start("blkid", QStringList());
    blkid.waitForFinished(5000);
    QString blkidOutput = blkid.readAllStandardOutput();

    if (!blkidOutput.isEmpty()) {
        addRow("", "");
        addRow("=== BLKID Output ===", "");
        QStringList blkidLines = blkidOutput.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : blkidLines) {
            addRow("", line);
        }
    }

    // Get /proc/partitions
    QFile partFile("/proc/partitions");
    if (partFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addRow("", "");
        addRow("=== /proc/partitions ===", "");
        QTextStream in(&partFile);
        while (!in.atEnd()) {
            QString line = in.readLine();
            addRow("", line);
        }
        partFile.close();
    }

    // Get disk statistics from /proc/diskstats
    QFile diskstatsFile("/proc/diskstats");
    if (diskstatsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addRow("", "");
        addRow("=== /proc/diskstats ===", "");
        QTextStream in(&diskstatsFile);
        while (!in.atEnd()) {
            QString line = in.readLine();
            addRow("", line);
        }
        diskstatsFile.close();
    }

    // Get smart information if smartctl is available
    QProcess smart;
    smart.start("smartctl", QStringList() << "--scan");
    smart.waitForFinished(5000);
    QString smartScan = smart.readAllStandardOutput();

    if (!smartScan.isEmpty()) {
        addRow("", "");
        addRow("=== SMART Devices ===", "");
        QStringList devices = smartScan.split('\n', Qt::SkipEmptyParts);
        for (const QString& device : devices) {
            QString devName = device.section(' ', 0, 0);
            addRow("Device", devName);
        }
    }
}
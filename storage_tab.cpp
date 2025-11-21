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

StorageTab::StorageTab(QWidget* parent)
    : TabWidgetBase(QCoreApplication::translate("StorageTab", "Storage"),
                    "lsblk -P -o NAME,PKNAME,SIZE,MOUNTPOINT,FSTYPE,TYPE,MODEL,VENDOR && df -P -h",
                    true,
                    "lsblk -P -o NAME,PKNAME,SIZE,MOUNTPOINT,FSTYPE,TYPE,MODEL,VENDOR && df -P -h",
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
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

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
                    
                    // Build disk type from model and vendor
                    QString model = device["model"].toString();
                    QString vendor = device["vendor"].toString();
                    QString diskType = "Unknown";
                    
                    if (!model.isEmpty() && !vendor.isEmpty()) {
                        diskType = vendor + " " + model;
                    } else if (!model.isEmpty()) {
                        diskType = model;
                    } else if (!vendor.isEmpty()) {
                        diskType = vendor;
                    }
                    
                    diskInfo["type"] = diskType;
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
        
        // Create disk label
        QString labelText = QString("Disk: %1").arg(diskName);
        if (diskData.contains("type")) {
            labelText += QString(" - Type: %1").arg(diskData["type"].toString());
        }
        if (diskData.contains("total_size")) {
            labelText += QString(" - Total size: %1").arg(diskData["total_size"].toString());
        }
        
        QLabel* diskLabel = new QLabel(labelText);
        QFont font = diskLabel->font();
        font.setBold(true);
        diskLabel->setFont(font);
        diskLabel->setAlignment(Qt::AlignCenter);
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
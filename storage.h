#ifndef STORAGE_H
#define STORAGE_H

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QFont>
#include <QColor>
#include <QJsonObject>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>
#include <QLabel>
#include <QDir>
#include <QStorageInfo>
#include <QProcess>
#include "gui_helpers.h"

// Storage information functions
void loadStorageInformation(QTableWidget* table, const QJsonObject& data);
void loadLiveStorageInformation(QTableWidget* table);
void styleStorageTable(QTableWidget* table);
QString getStorageInfo();

// Enhanced progress bar creation for storage
QWidget* createStorageProgressBar(const QString& device, const QString& mountPoint, double percentage, long long used, long long total, const QString& unit, const QString& fsType);

// Storage Table Styling
void styleStorageTable(QTableWidget* table)
{
    // Set column widths
    table->setColumnWidth(0, 200);  // Property
    table->setColumnWidth(1, 400);  // Value (wider for progress bars)
    table->setColumnWidth(2, 80);   // Unit
    table->setColumnWidth(3, 120);  // Type
    
    // Style headers
    table->horizontalHeader()->setStyleSheet(
        "QHeaderView::section { "
        "background-color: #2c3e50; "
        "color: white; "
        "padding: 8px; "
        "border: none; "
        "font-weight: bold; "
        "}"
    );
}

// Enhanced progress bar widget creation for storage
QWidget* createStorageProgressBar(const QString& device, const QString& mountPoint, double percentage, long long used, long long total, const QString& unit, const QString& fsType)
{
    QWidget* container = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(5, 5, 5, 8); // More bottom margin
    mainLayout->setSpacing(4);
    
    // Top layout with device name and percentage
    QHBoxLayout* topLayout = new QHBoxLayout();
    
    // Create title with device and total size
    QString titleText = QString("%1 - Tot: %2 %3").arg(device).arg(QString::number(total / (1024.0 * 1024.0 * 1024.0), 'f', 1)).arg(unit);
    QLabel* nameLabel = new QLabel(titleText);
    nameLabel->setFont(QFont(nameLabel->font().family(), nameLabel->font().pointSize(), QFont::Bold));
    nameLabel->setStyleSheet("color: #2c3e50;");
    
    QLabel* percentLabel = new QLabel(QString("%1%").arg(QString::number(percentage, 'f', 1)));
    percentLabel->setFont(QFont(percentLabel->font().family(), percentLabel->font().pointSize(), QFont::Bold));
    percentLabel->setAlignment(Qt::AlignRight);
    
    // Color code the percentage for storage (different thresholds than memory)
    if (percentage < 80) {
        percentLabel->setStyleSheet("color: #27ae60;"); // Green
    } else if (percentage < 95) {
        percentLabel->setStyleSheet("color: #f39c12;"); // Orange
    } else {
        percentLabel->setStyleSheet("color: #e74c3c;"); // Red
    }
    
    topLayout->addWidget(nameLabel);
    topLayout->addStretch();
    topLayout->addWidget(percentLabel);
    
    // Progress bar
    QProgressBar* progressBar = new QProgressBar();
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
    progressBar->setValue(static_cast<int>(percentage));
    progressBar->setFixedHeight(24); // Slightly taller progress bar
    progressBar->setTextVisible(false); // We'll show custom text below
    
    // Style progress bar based on usage (storage-specific colors)
    QString progressStyle;
    if (percentage < 80) {
        progressStyle = 
            "QProgressBar { "
            "border: 2px solid #bdc3c7; "
            "border-radius: 12px; "
            "background-color: #ecf0f1; "
            "} "
            "QProgressBar::chunk { "
            "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3498db, stop:1 #2980b9); "
            "border-radius: 10px; "
            "}";
    } else if (percentage < 95) {
        progressStyle = 
            "QProgressBar { "
            "border: 2px solid #bdc3c7; "
            "border-radius: 12px; "
            "background-color: #ecf0f1; "
            "} "
            "QProgressBar::chunk { "
            "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #f39c12, stop:1 #e67e22); "
            "border-radius: 10px; "
            "}";
    } else {
        progressStyle = 
            "QProgressBar { "
            "border: 2px solid #bdc3c7; "
            "border-radius: 12px; "
            "background-color: #ecf0f1; "
            "} "
            "QProgressBar::chunk { "
            "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #e74c3c, stop:1 #c0392b); "
            "border-radius: 10px; "
            "}";
    }
    progressBar->setStyleSheet(progressStyle);
    
    // Middle layout with mount point and filesystem type
    QHBoxLayout* middleLayout = new QHBoxLayout();
    
    QString mountText = mountPoint.isEmpty() ? "Not mounted" : QString("Mounted: %1").arg(mountPoint);
    QLabel* mountLabel = new QLabel(mountText);
    mountLabel->setFont(QFont(mountLabel->font().family(), mountLabel->font().pointSize() - 1));
    mountLabel->setStyleSheet("color: #7f8c8d;");
    
    QLabel* fsLabel = new QLabel(QString("FS: %1").arg(fsType.isEmpty() ? "Unknown" : fsType));
    fsLabel->setFont(QFont(fsLabel->font().family(), fsLabel->font().pointSize() - 1));
    fsLabel->setStyleSheet("color: #7f8c8d;");
    fsLabel->setAlignment(Qt::AlignRight);
    
    middleLayout->addWidget(mountLabel);
    middleLayout->addStretch();
    middleLayout->addWidget(fsLabel);
    
    // Bottom layout with usage details
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    
    QLabel* usageLabel = new QLabel(QString("Used: %1 %2").arg(QString::number(used / (1024.0 * 1024.0 * 1024.0), 'f', 2)).arg(unit));
    usageLabel->setFont(QFont(usageLabel->font().family(), usageLabel->font().pointSize() - 1));
    usageLabel->setStyleSheet("color: #7f8c8d;");
    
    QLabel* availableLabel = new QLabel(QString("Free: %1 %2").arg(QString::number((total - used) / (1024.0 * 1024.0 * 1024.0), 'f', 2)).arg(unit));
    availableLabel->setFont(QFont(availableLabel->font().family(), availableLabel->font().pointSize() - 1));
    availableLabel->setStyleSheet("color: #7f8c8d;");
    availableLabel->setAlignment(Qt::AlignRight);
    
    bottomLayout->addWidget(usageLabel);
    bottomLayout->addStretch();
    bottomLayout->addWidget(availableLabel);
    
    // Add all layouts to main container
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(progressBar);
    mainLayout->addLayout(middleLayout);
    mainLayout->addLayout(bottomLayout);
    
    container->setFixedHeight(95); // Slightly taller than memory for extra info line
    return container;
}

void loadLiveStorageInformation(QTableWidget* table)
{
    table->setRowCount(0);
    
    // Get mounted filesystems using QStorageInfo
    QList<QStorageInfo> mountedVolumes = QStorageInfo::mountedVolumes();
    
    for (const QStorageInfo& storage : mountedVolumes) {
        if (!storage.isValid() || storage.isReadOnly()) {
            continue;
        }
        
        QString device = storage.device();
        QString mountPoint = storage.rootPath();
        QString fsType = storage.fileSystemType();
        
        // Skip special filesystems
        if (device.startsWith("/dev/loop") || 
            device.startsWith("/dev/snap") || 
            fsType == "tmpfs" || 
            fsType == "devtmpfs" || 
            fsType == "sysfs" || 
            fsType == "proc" ||
            mountPoint.startsWith("/snap/") ||
            mountPoint.startsWith("/sys/") ||
            mountPoint.startsWith("/proc/") ||
            mountPoint.startsWith("/dev/") ||
            mountPoint.startsWith("/run/")) {
            continue;
        }
        
        long long totalBytes = storage.bytesTotal();
        long long availableBytes = storage.bytesAvailable();
        long long usedBytes = totalBytes - availableBytes;
        
        if (totalBytes > 0) {
            double usagePercent = (static_cast<double>(usedBytes) / totalBytes) * 100.0;
            
            int row = table->rowCount();
            table->insertRow(row);
            
            // Property name (device)
            QString propertyName = QString("Disk: %1").arg(device);
            QTableWidgetItem* propertyItem = new QTableWidgetItem(propertyName);
            propertyItem->setForeground(QColor(44, 62, 80));
            propertyItem->setFont(QFont(propertyItem->font().family(), propertyItem->font().pointSize(), QFont::Bold));
            table->setItem(row, 0, propertyItem);
            
            // Progress bar widget
            QWidget* progressWidget = createStorageProgressBar(device, mountPoint, usagePercent, usedBytes, totalBytes, "GB", fsType);
            table->setCellWidget(row, 1, progressWidget);
            table->setRowHeight(row, 95); // Taller for storage info
            
            // Empty unit column
            table->setItem(row, 2, new QTableWidgetItem(""));
            
            // Type column
            QTableWidgetItem* typeItem = new QTableWidgetItem("Storage");
            typeItem->setForeground(QColor(52, 152, 219));
            table->setItem(row, 3, typeItem);
        }
    }
    
    // If no storage devices found, add a message
    if (table->rowCount() == 0) {
        addRowToTable(table, QStringList() << "No Storage" << "No mounted storage devices found" << "" << "Storage");
    }
    
    // Add additional disk information from /proc/partitions
    QFile partitionsFile("/proc/partitions");
    if (partitionsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&partitionsFile);
        QStringList lines = stream.readAll().split('\n');
        partitionsFile.close();
        
        // Skip first two header lines
        for (int i = 2; i < lines.size(); ++i) {
            QString line = lines[i].trimmed();
            if (line.isEmpty()) continue;
            
            QStringList parts = line.split(QRegularExpression("\\s+"));
            if (parts.size() >= 4) {
                QString deviceName = parts[3];
                long long sizeKB = parts[2].toLongLong();
                
                // Only show actual disk devices, not partitions for summary
                if (deviceName.length() >= 3 && 
                    (deviceName.startsWith("sd") || deviceName.startsWith("nvme") || deviceName.startsWith("hd")) &&
                    !deviceName.contains(QRegularExpression("\\d$"))) { // No numbers at end = main device
                    
                    long long sizeBytes = sizeKB * 1024;
                    addRowToTable(table, QStringList() 
                        << QString("Total Capacity: %1").arg(deviceName) 
                        << QString::number(sizeBytes / (1024.0 * 1024.0 * 1024.0), 'f', 1) 
                        << "GB" 
                        << "Storage");
                }
            }
        }
    }
}

void loadStorageInformation(QTableWidget* table, const QJsonObject& data)
{
    // Use the live storage information instead of JSON data
    loadLiveStorageInformation(table);
}

QString getStorageInfo()
{
    QList<QStorageInfo> mountedVolumes = QStorageInfo::mountedVolumes();
    
    long long totalStorage = 0;
    int deviceCount = 0;
    
    for (const QStorageInfo& storage : mountedVolumes) {
        if (storage.isValid() && !storage.isReadOnly()) {
            QString device = storage.device();
            QString fsType = storage.fileSystemType();
            
            // Skip special filesystems
            if (!device.startsWith("/dev/loop") && 
                !device.startsWith("/dev/snap") && 
                fsType != "tmpfs" && 
                fsType != "devtmpfs") {
                totalStorage += storage.bytesTotal();
                deviceCount++;
            }
        }
    }
    
    if (deviceCount > 0) {
        return QString("Storage: %1 devices, %2 GB total")
            .arg(deviceCount)
            .arg(QString::number(totalStorage / (1024.0 * 1024.0 * 1024.0), 'f', 1));
    }
    
    return "No storage devices found";
}

#endif // STORAGE_H
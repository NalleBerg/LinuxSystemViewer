#include "storage_tab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QGroupBox>
#include <QFont>
#include <QRegularExpression>
#include <QDebug>
#include <QCoreApplication>

StorageTab::StorageTab(QWidget* parent)
    : TabWidgetBase(QCoreApplication::translate("StorageTab", "Storage"), "lsblk && df -h", true, 
                    "lsblk -f && df -h && lshw -C disk && fdisk -l 2>/dev/null && smartctl --scan 2>/dev/null", parent)
{
    qDebug() << "StorageTab: Constructor called - base constructor done";
    initializeTab();
    qDebug() << "StorageTab: Constructor finished";
}

QWidget* StorageTab::createUserFriendlyView()
{
    qDebug() << "StorageTab: createUserFriendlyView called";
    
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    QWidget* contentWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    QLabel* titleLabel = new QLabel(QCoreApplication::translate("StorageTab", "Storage Devices and Disk Information"));
    titleLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  color: #2c3e50;"
        "  margin-bottom: 10px;"
        "}"
    );
    mainLayout->addWidget(titleLabel);
    
    createInfoSection(QCoreApplication::translate("StorageTab", "Disk Drives"), &m_diskDrivesSection, &m_diskDrivesContent, mainLayout);
    createInfoSection(QCoreApplication::translate("StorageTab", "Partitions"), &m_partitionsSection, &m_partitionsContent, mainLayout);
    createInfoSection(QCoreApplication::translate("StorageTab", "Mount Points"), &m_mountPointsSection, &m_mountPointsContent, mainLayout);
    createInfoSection(QCoreApplication::translate("StorageTab", "Disk Usage"), &m_diskUsageSection, &m_diskUsageContent, mainLayout);
    
    mainLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);
    
    qDebug() << "StorageTab: createUserFriendlyView completed";
    return scrollArea;
}

void StorageTab::createInfoSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout)
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
    *contentLabel = new QLabel(QCoreApplication::translate("StorageTab", "Loading %1 information...").arg(title.toLower()));
    (*contentLabel)->setWordWrap(true);
    (*contentLabel)->setStyleSheet("QLabel { padding: 10px; background-color: #f8f9fa; border-radius: 4px; }");
    sectionLayout->addWidget(*contentLabel);
    
    parentLayout->addWidget(*groupBox);
}

void StorageTab::parseOutput(const QString& output)
{
    qDebug() << "StorageTab: parseOutput called";
    
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    QString diskDrivesInfo = QCoreApplication::translate("StorageTab", "Disk Drives: Not detected");
    QString partitionsInfo = QCoreApplication::translate("StorageTab", "Partitions: Not detected");
    QString mountPointsInfo = QCoreApplication::translate("StorageTab", "Mount Points: Not detected");
    QString diskUsageInfo = QCoreApplication::translate("StorageTab", "Disk Usage: Not detected");
    
    QStringList diskDrives;
    QStringList partitions;
    QStringList mountPoints;
    QStringList diskUsage;
    
    bool inLsblkSection = false;
    bool inDfSection = false;
    
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        
        // Parse lsblk output
        if (trimmed.startsWith("NAME") && trimmed.contains("SIZE") && trimmed.contains("TYPE")) {
            inLsblkSection = true;
            continue;
        }
        
        if (inLsblkSection && !trimmed.isEmpty()) {
            // Check if we've moved to df output
            if (trimmed.startsWith("Filesystem") && trimmed.contains("Use%")) {
                inLsblkSection = false;
                inDfSection = true;
                continue;
            }
            
            QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 3) {
                QString name = parts[0];
                QString size = parts[3]; // SIZE column
                QString type = parts[5]; // TYPE column
                
                if (type == "disk") {
                    diskDrives.append(name + " (" + size + ")");
                } else if (type == "part") {
                    QString mountpoint = (parts.size() > 6) ? parts[6] : "Not mounted";
                    partitions.append(name + " (" + size + ") - " + mountpoint);
                    if (!mountpoint.isEmpty() && mountpoint != "Not mounted") {
                        mountPoints.append(name + " -> " + mountpoint);
                    }
                }
            }
        }
        
        // Parse df output for disk usage
        if (inDfSection && !trimmed.isEmpty()) {
            QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 6 && !trimmed.startsWith("tmpfs") && !trimmed.startsWith("udev")) {
                QString filesystem = parts[0];
                QString size = parts[1];
                QString used = parts[2];
                QString available = parts[3];
                QString usePercent = parts[4];
                QString mountpoint = parts[5];
                
                diskUsage.append(mountpoint + ": " + used + "/" + size + " (" + usePercent + " used)");
            }
        }
        
        // Look for disk device info from other commands
        if (trimmed.contains("/dev/sd") || trimmed.contains("/dev/nvme") || trimmed.contains("/dev/hd")) {
            QRegularExpression diskRegex("(/dev/[a-zA-Z0-9]+)");
            QRegularExpressionMatch match = diskRegex.match(trimmed);
            if (match.hasMatch()) {
                QString device = match.captured(1);
                if (!diskDrives.contains(device)) {
                    diskDrives.append(device);
                }
            }
        }
    }
    
    // Remove duplicates
    diskDrives.removeDuplicates();
    partitions.removeDuplicates();
    mountPoints.removeDuplicates();
    diskUsage.removeDuplicates();
    
    // Format the information
    if (!diskDrives.isEmpty()) {
        diskDrivesInfo = QCoreApplication::translate("StorageTab", "Disk Drives:\n") + diskDrives.join("\n");
    }
    
    if (!partitions.isEmpty()) {
        partitionsInfo = QCoreApplication::translate("StorageTab", "Partitions:\n") + partitions.join("\n");
    }
    
    if (!mountPoints.isEmpty()) {
        mountPointsInfo = QCoreApplication::translate("StorageTab", "Mount Points:\n") + mountPoints.join("\n");
    }
    
    if (!diskUsage.isEmpty()) {
        diskUsageInfo = QCoreApplication::translate("StorageTab", "Disk Usage:\n") + diskUsage.join("\n");
    }
    
    // Update the UI with parsed information
    if (m_diskDrivesContent) {
        m_diskDrivesContent->setText(diskDrivesInfo);
    }
    if (m_partitionsContent) {
        m_partitionsContent->setText(partitionsInfo);
    }
    if (m_mountPointsContent) {
        m_mountPointsContent->setText(mountPointsInfo);
    }
    if (m_diskUsageContent) {
        m_diskUsageContent->setText(diskUsageInfo);
    }
    
    qDebug() << "StorageTab: parseOutput completed";
}
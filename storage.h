#ifndef STORAGE_H
#define STORAGE_H

#include <QString>
#include <QApplication>
#include <QStorageInfo>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QRegularExpression>
#include <QProcess>
#include <QJsonObject>
#include <QStringList>
#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QWidget>
#include <QHeaderView>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QAbstractItemView>
#include <QObject>
#include <QMap>
#include <QList>
#include <QDebug>
#include <QtGlobal>
#include <QScrollBar>

// Forward declarations
class QTableWidget;

// Storage device information structure
struct PhysicalDisk {
    QString device;       // e.g., /dev/sda
    QString model;        // Disk model
    QString vendor;       // Manufacturer
    QString type;         // SSD, HDD, NVMe SSD
    qint64 totalSize;     // Total size in bytes
    QString serial;       // Serial number
    QString interface;    // SATA, NVMe, USB, etc.
    QString health;       // Health status
    int temperature;      // Temperature in Celsius (-1 if unknown)
    QString firmware;     // Firmware version
};

// Partition information structure
struct PartitionInfo {
    QString device;           // e.g., /dev/sda1
    QString parentDisk;       // e.g., /dev/sda
    QString filesystem;       // ext4, ntfs, swap, etc.
    QString mountPoint;       // Where it's mounted
    QString label;            // Partition label
    QString uuid;             // Partition UUID
    qint64 totalSize;         // Total partition size in bytes
    qint64 usedSize;          // Used space in bytes
    qint64 availableSize;     // Available space in bytes
    bool isMounted;           // Whether partition is mounted
    QString partitionType;    // Primary, Extended, Logical
};

// Enhanced disk information gathering
void getDiskInformation(const QString &device, PhysicalDisk &disk);

// Storage utility functions
QString formatSize(double bytes);
QMap<QString, QString> getBlockDeviceInfo();
QList<PhysicalDisk> getPhysicalDisks();
QList<PartitionInfo> getPartitionsForDisk(const QString& diskDevice);

// Storage information loading functions
void loadStorageInformation(QTableWidget* storageTable, const QJsonObject &item);
void loadLiveStorageInformation(QTableWidget* storageTable);
void addLiveStorageToSummary(QTableWidget* summaryTable);
void showDiskDetails(QWidget* parent, const PhysicalDisk& disk);
void showPartitionDetails(QWidget* parent, const PartitionInfo& partition);
void setupStorageTableWithButtons(QTableWidget* storageTable);

// Enhanced storage display functions
void displayFilesystemInformation(QTableWidget* storageTable);
void displayPhysicalDiskInformation(QTableWidget* storageTable);
void displayIntegratedStorageInformation(QTableWidget* storageTable);
void refreshStorageInfo(QTableWidget* storageTable);

// Helper functions for disk information
QString getDiskModel(const QString &device);
QString getDiskVendor(const QString &device);
QString getDiskSerial(const QString &device);
QString getDiskInterface(const QString &device);
QString getDiskHealth(const QString &device);
int getDiskTemperature(const QString &device);
QString getDiskFirmware(const QString &device);
qint64 getDiskSize(const QString &device);

// Inline implementations

inline QString formatSize(double bytes)
{
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024*1024) return QString::number(bytes/1024, 'f', 1) + " KB";
    if (bytes < 1024*1024*1024) return QString::number(bytes/(1024*1024), 'f', 1) + " MB";
    return QString::number(bytes/(1024*1024*1024), 'f', 1) + " GB";
}

inline QMap<QString, QString> getBlockDeviceInfo()
{
    QMap<QString, QString> deviceInfo;
    
    // Read block device information from /sys/block/
    QDir sysBlockDir("/sys/block");
    QStringList devices = sysBlockDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QString &device : devices) {
        QString devicePath = "/sys/block/" + device;
        
        // Read device type
        QFile rotationalFile(devicePath + "/queue/rotational");
        if (rotationalFile.open(QIODevice::ReadOnly)) {
            QString rotational = rotationalFile.readAll().trimmed();
            deviceInfo[device + "_type"] = (rotational == "0") ? "SSD" : "HDD";
            rotationalFile.close();
        }
        
        // Try to read model information
        QFile modelFile(devicePath + "/device/model");
        if (modelFile.open(QIODevice::ReadOnly)) {
            QString model = modelFile.readAll().trimmed();
            if (!model.isEmpty()) {
                deviceInfo[device + "_model"] = model;
            }
            modelFile.close();
        }
        
        // For NVMe devices, check different path
        if (device.startsWith("nvme")) {
            QFile nvmeModelFile(devicePath + "/device/model");
            if (nvmeModelFile.open(QIODevice::ReadOnly)) {
                QString model = nvmeModelFile.readAll().trimmed();
                if (!model.isEmpty()) {
                    deviceInfo[device + "_model"] = model;
                }
                nvmeModelFile.close();
            }
            deviceInfo[device + "_type"] = "NVMe SSD";
        }
    }
    
    return deviceInfo;
}

inline QList<PhysicalDisk> getPhysicalDisks()
{
    QList<PhysicalDisk> disks;
    
    // Get all block devices
    QDir sysBlockDir("/sys/block");
    QStringList devices = sysBlockDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QString &device : devices) {
        // Skip loop devices, ram disks, and other virtual devices
        if (device.startsWith("loop") || device.startsWith("ram") || 
            device.startsWith("dm-") || device.startsWith("md")) {
            continue;
        }
        
        PhysicalDisk disk;
        disk.device = "/dev/" + device;
        
        // Get comprehensive disk information
        getDiskInformation(device, disk);
        
        // Only add if we got useful information (must have size or model)
        if (!disk.model.isEmpty() || disk.totalSize > 0) {
            disks.append(disk);
        }
    }
    
    return disks;
}

inline QList<PartitionInfo> getPartitionsForDisk(const QString& diskDevice)
{
    QList<PartitionInfo> partitions;
    
    // Extract device name without /dev/ prefix
    QString deviceName = diskDevice;
    if (deviceName.startsWith("/dev/")) {
        deviceName = deviceName.mid(5);
    }
    
    // Look for partitions in /sys/block/devicename/
    QDir sysBlock("/sys/block/" + deviceName);
    QStringList partList = sysBlock.entryList(QStringList() << deviceName + "*", QDir::Dirs);
    
    // Get storage info for mounted filesystems
    QList<QStorageInfo> storageList = QStorageInfo::mountedVolumes();
    QMap<QString, QStorageInfo> mountedDevices;
    for (const QStorageInfo &storage : storageList) {
        mountedDevices[storage.device()] = storage;
    }
    
    for (const QString& part : partList) {
        if (part == deviceName) continue; // Skip the main disk device
        
        PartitionInfo partition;
        partition.device = "/dev/" + part;
        partition.parentDisk = diskDevice;
        
        // Get partition size
        QFile sizeFile("/sys/block/" + deviceName + "/" + part + "/size");
        if (sizeFile.open(QIODevice::ReadOnly)) {
            QString sizeStr = sizeFile.readAll().trimmed();
            partition.totalSize = sizeStr.toLongLong() * 512; // Size is in 512-byte sectors
        }
        
        // Check if partition is mounted and get filesystem info
        if (mountedDevices.contains(partition.device)) {
            QStorageInfo storage = mountedDevices[partition.device];
            partition.isMounted = true;
            partition.mountPoint = storage.rootPath();
            partition.filesystem = storage.fileSystemType();
            partition.usedSize = storage.bytesTotal() - storage.bytesAvailable();
            partition.availableSize = storage.bytesAvailable();
        } else {
            partition.isMounted = false;
        }
        
        // Get filesystem type from blkid if not mounted
        if (partition.filesystem.isEmpty()) {
            QProcess blkidProcess;
            blkidProcess.start("blkid", QStringList() << "-o" << "value" << "-s" << "TYPE" << partition.device);
            blkidProcess.waitForFinished(2000);
            if (blkidProcess.exitCode() == 0) {
                partition.filesystem = blkidProcess.readAllStandardOutput().trimmed();
            }
        }
        
        // Get partition label
        QProcess labelProcess;
        labelProcess.start("blkid", QStringList() << "-o" << "value" << "-s" << "LABEL" << partition.device);
        labelProcess.waitForFinished(2000);
        if (labelProcess.exitCode() == 0) {
            partition.label = labelProcess.readAllStandardOutput().trimmed();
        }
        
        // Get partition UUID
        QProcess uuidProcess;
        uuidProcess.start("blkid", QStringList() << "-o" << "value" << "-s" << "UUID" << partition.device);
        uuidProcess.waitForFinished(2000);
        if (uuidProcess.exitCode() == 0) {
            partition.uuid = uuidProcess.readAllStandardOutput().trimmed();
        }
        
        // Determine partition type (simplified)
        partition.partitionType = "Primary"; // Could be enhanced with more detailed detection
        
        partitions.append(partition);
    }
    
    return partitions;
}

// === Enhanced Model Detection Functions ===
inline QString getDiskModelFromSys(const QString &device)
{
    QString devicePath = "/sys/block/" + device;
    
    // Try different model file locations
    QStringList modelPaths = {
        devicePath + "/device/model",
        devicePath + "/device/name",
        devicePath + "/queue/product"
    };
    
    for (const QString &path : modelPaths) {
        QFile modelFile(path);
        if (modelFile.open(QIODevice::ReadOnly)) {
            QString model = modelFile.readAll().trimmed();
            if (!model.isEmpty() && model != "Unknown") {
                return model;
            }
        }
    }
    
    return "Unknown";
}

inline QString getDiskModelFromSmart(const QString &device)
{
    QProcess smartProcess;
    smartProcess.start("smartctl", QStringList() << "-i" << "/dev/" + device);
    smartProcess.waitForFinished(3000);
    
    if (smartProcess.exitCode() == 0) {
        QString output = smartProcess.readAllStandardOutput();
        QStringList lines = output.split("\\n");
        
        for (const QString &line : lines) {
            if (line.contains("Device Model:", Qt::CaseInsensitive)) {
                QString model = line.split(":").last().trimmed();
                if (!model.isEmpty()) {
                    return model;
                }
            } else if (line.contains("Model Family:", Qt::CaseInsensitive)) {
                QString model = line.split(":").last().trimmed();
                if (!model.isEmpty()) {
                    return model;
                }
            }
        }
    }
    
    return "Unknown";
}

inline QString getDiskModelFromLsblk(const QString &device)
{
    QProcess lsblkProcess;
    lsblkProcess.start("lsblk", QStringList() << "-o" << "MODEL" << "-n" << "/dev/" + device);
    lsblkProcess.waitForFinished(3000);
    
    if (lsblkProcess.exitCode() == 0) {
        QString output = lsblkProcess.readAllStandardOutput().trimmed();
        QStringList lines = output.split("\\n");
        if (!lines.isEmpty()) {
            QString model = lines.first().trimmed();
            if (!model.isEmpty() && model != "Unknown") {
                return model;
            }
        }
    }
    
    return "Unknown";
}

// === Enhanced Vendor Detection Functions ===
inline QString getDiskVendorFromSys(const QString &device)
{
    QString devicePath = "/sys/block/" + device;
    
    QFile vendorFile(devicePath + "/device/vendor");
    if (vendorFile.open(QIODevice::ReadOnly)) {
        QString vendor = vendorFile.readAll().trimmed();
        if (!vendor.isEmpty() && vendor != "Unknown") {
            return vendor;
        }
    }
    
    return "Unknown";
}

inline QString getDiskVendorFromSmart(const QString &device)
{
    QProcess smartProcess;
    smartProcess.start("smartctl", QStringList() << "-i" << "/dev/" + device);
    smartProcess.waitForFinished(3000);
    
    if (smartProcess.exitCode() == 0) {
        QString output = smartProcess.readAllStandardOutput();
        QStringList lines = output.split("\n");
        
        for (const QString &line : lines) {
            if (line.contains("Vendor:", Qt::CaseInsensitive)) {
                QString vendor = line.split(":").last().trimmed();
                if (!vendor.isEmpty()) {
                    return vendor;
                }
            }
        }
    }
    
    return "Unknown";
}

// === Enhanced Serial Number Detection Functions ===
inline QString getDiskSerialFromSmart(const QString &device)
{
    QProcess smartProcess;
    smartProcess.start("smartctl", QStringList() << "-i" << "/dev/" + device);
    smartProcess.waitForFinished(3000);
    
    if (smartProcess.exitCode() == 0) {
        QString output = smartProcess.readAllStandardOutput();
        QStringList lines = output.split("\n");
        
        for (const QString &line : lines) {
            if (line.contains("Serial Number:", Qt::CaseInsensitive)) {
                QString serial = line.split(":").last().trimmed();
                if (!serial.isEmpty() && serial != "Unknown") {
                    return serial;
                }
            }
        }
    }
    
    return "Unknown";
}

inline QString getDiskSerialFromHdparm(const QString &device)
{
    QProcess hdparmProcess;
    hdparmProcess.start("hdparm", QStringList() << "-i" << "/dev/" + device);
    hdparmProcess.waitForFinished(3000);
    
    if (hdparmProcess.exitCode() == 0) {
        QString output = hdparmProcess.readAllStandardOutput();
        QRegularExpression serialRegex(R"(SerialNo=(\S+))");
        QRegularExpressionMatch match = serialRegex.match(output);
        if (match.hasMatch()) {
            return match.captured(1);
        }
    }
    
    return "Unknown";
}

inline QString getDiskSerialFromSys(const QString &device)
{
    QString devicePath = "/sys/block/" + device;
    
    QFile serialFile(devicePath + "/device/serial");
    if (serialFile.open(QIODevice::ReadOnly)) {
        QString serial = serialFile.readAll().trimmed();
        if (!serial.isEmpty() && serial != "Unknown") {
            return serial;
        }
    }
    
    return "Unknown";
}

inline QString getDiskSerialFromLsblk(const QString &device)
{
    QProcess lsblkProcess;
    lsblkProcess.start("lsblk", QStringList() << "-o" << "SERIAL" << "-n" << "/dev/" + device);
    lsblkProcess.waitForFinished(3000);
    
    if (lsblkProcess.exitCode() == 0) {
        QString output = lsblkProcess.readAllStandardOutput().trimmed();
        QStringList lines = output.split("\\n");
        if (!lines.isEmpty()) {
            QString serial = lines.first().trimmed();
            if (!serial.isEmpty() && serial != "Unknown") {
                return serial;
            }
        }
    }
    
    return "Unknown";
}

// === Enhanced Interface Detection ===
inline QString getDiskInterfaceFromSys(const QString &device)
{
    QString devicePath = "/sys/block/" + device;
    
    // Check for NVMe devices
    if (device.startsWith("nvme")) {
        return "NVMe";
    }
    
    // Check for SATA/ATA devices
    if (device.startsWith("sd")) {
        // Try to determine if it's SATA by checking the subsystem
        QFile subsystemFile(devicePath + "/device/subsystem");
        if (subsystemFile.exists()) {
            QString target = subsystemFile.symLinkTarget();
            if (target.contains("scsi")) {
                return "SATA";
            }
        }
        return "SATA";
    }
    
    // Check for IDE/PATA devices
    if (device.startsWith("hd")) {
        return "IDE/PATA";
    }
    
    // Check for eMMC/SD devices
    if (device.startsWith("mmcblk")) {
        return "eMMC/SD";
    }
    
    return "Unknown";
}

// === Enhanced Disk Type Detection ===
inline QString getDiskTypeFromSys(const QString &device)
{
    QString devicePath = "/sys/block/" + device;
    
    // Check if it's an NVMe SSD
    if (device.startsWith("nvme")) {
        return "NVMe SSD";
    }
    
    // Check for rotational flag (0 = SSD, 1 = HDD)
    QFile rotationalFile(devicePath + "/queue/rotational");
    if (rotationalFile.open(QIODevice::ReadOnly)) {
        QString rotational = rotationalFile.readAll().trimmed();
        if (rotational == "0") {
            return "SSD";
        } else if (rotational == "1") {
            return "HDD";
        }
    }
    
    // Fallback: try to guess from device name
    if (device.startsWith("sd")) {
        return "Disk";  // Could be either SSD or HDD
    }
    
    return "Unknown";
}

// === Enhanced Health Detection ===
inline QString getDiskHealthFromSmart(const QString &device)
{
    QProcess smartProcess;
    smartProcess.start("smartctl", QStringList() << "-H" << "/dev/" + device);
    smartProcess.waitForFinished(3000);
    
    if (smartProcess.exitCode() == 0) {
        QString output = smartProcess.readAllStandardOutput();
        if (output.contains("PASSED", Qt::CaseInsensitive)) {
            return "Healthy";
        } else if (output.contains("FAILED", Qt::CaseInsensitive)) {
            return "Warning";
        }
    }
    
    return "Unknown";
}

// === Enhanced Temperature Detection ===
inline int getDiskTemperatureFromSmart(const QString &device)
{
    QProcess smartProcess;
    smartProcess.start("smartctl", QStringList() << "-A" << "/dev/" + device);
    smartProcess.waitForFinished(3000);
    
    if (smartProcess.exitCode() == 0) {
        QString output = smartProcess.readAllStandardOutput();
        QStringList lines = output.split("\\n");
        
        for (const QString &line : lines) {
            if (line.contains("Temperature", Qt::CaseInsensitive)) {
                // Try to extract temperature value
                QRegularExpression tempRegex(R"(\\b(\\d+)\\s*°?C)");
                QRegularExpressionMatch match = tempRegex.match(line);
                if (match.hasMatch()) {
                    bool ok;
                    int temp = match.captured(1).toInt(&ok);
                    if (ok && temp > 0 && temp < 100) {
                        return temp;
                    }
                }
            }
        }
    }
    
    return -1;
}

// === Enhanced Firmware Detection ===
inline QString getDiskFirmwareFromSmart(const QString &device)
{
    QProcess smartProcess;
    smartProcess.start("smartctl", QStringList() << "-i" << "/dev/" + device);
    smartProcess.waitForFinished(3000);
    
    if (smartProcess.exitCode() == 0) {
        QString output = smartProcess.readAllStandardOutput();
        QStringList lines = output.split("\\n");
        
        for (const QString &line : lines) {
            if (line.contains("Firmware Version:", Qt::CaseInsensitive)) {
                QString firmware = line.split(":").last().trimmed();
                if (!firmware.isEmpty()) {
                    return firmware;
                }
            }
        }
    }
    
    return "Unknown";
}

inline QString getDiskFirmwareFromSys(const QString &device)
{
    QString devicePath = "/sys/block/" + device;
    
    QFile firmwareFile(devicePath + "/device/rev");
    if (firmwareFile.open(QIODevice::ReadOnly)) {
        QString firmware = firmwareFile.readAll().trimmed();
        if (!firmware.isEmpty()) {
            return firmware;
        }
    }
    
    return "Unknown";
}

inline QString getDiskInterface(const QString &device)
{
    if (device.startsWith("nvme")) {
        return "NVMe";
    } else if (device.startsWith("sd")) {
        return "SATA";
    } else if (device.startsWith("hd")) {
        return "IDE/PATA";
    } else if (device.startsWith("mmcblk")) {
        return "eMMC/SD";
    }
    
    return "Unknown";
}

inline QString getDiskHealth(const QString &device)
{
    // Try to get SMART status using smartctl
    QProcess smartProcess;
    smartProcess.start("smartctl", QStringList() << "-H" << "/dev/" + device);
    smartProcess.waitForFinished(3000);
    
    if (smartProcess.exitCode() == 0) {
        QString output = smartProcess.readAllStandardOutput();
        if (output.contains("PASSED", Qt::CaseInsensitive)) {
            return "Healthy";
        } else if (output.contains("FAILED", Qt::CaseInsensitive)) {
            return "Warning";
        }
    }
    
    return "Unknown";
}

inline int getDiskTemperature(const QString &device)
{
    // Try to get temperature using smartctl
    QProcess smartProcess;
    smartProcess.start("smartctl", QStringList() << "-A" << "/dev/" + device);
    smartProcess.waitForFinished(3000);
    
    if (smartProcess.exitCode() == 0) {
        QString output = smartProcess.readAllStandardOutput();
        QRegularExpression tempRegex(R"(Temperature_Celsius.*?(\d+))");
        QRegularExpressionMatch match = tempRegex.match(output);
        if (match.hasMatch()) {
            return match.captured(1).toInt();
        }
    }
    
    return -1; // Unknown temperature
}

inline QString getDiskFirmware(const QString &device)
{
    // Try to get firmware version using hdparm
    QProcess hdparmProcess;
    hdparmProcess.start("hdparm", QStringList() << "-i" << "/dev/" + device);
    hdparmProcess.waitForFinished(3000);
    
    if (hdparmProcess.exitCode() == 0) {
        QString output = hdparmProcess.readAllStandardOutput();
        QRegularExpression fwRegex(R"(FwRev=(\S+))");
        QRegularExpressionMatch match = fwRegex.match(output);
        if (match.hasMatch()) {
            return match.captured(1);
        }
    }
    
    return "Unknown";
}

inline qint64 getDiskSizeFromSys(const QString &device)
{
    QString sizePath = "/sys/block/" + device + "/size";
    QFile sizeFile(sizePath);
    
    if (sizeFile.open(QIODevice::ReadOnly)) {
        QString sizeStr = sizeFile.readAll().trimmed();
        bool ok;
        qint64 sectors = sizeStr.toLongLong(&ok);
        if (ok) {
            // Each sector is 512 bytes
            return sectors * 512;
        }
    }
    
    return 0;
}

inline void loadStorageInformation(QTableWidget* storageTable, const QJsonObject &item)
{
    if (!storageTable) return;
    
    QString type = item["class"].toString();
    if (type != "disk" && type != "volume") return;
    
    QString size = formatSize(item["size"].toDouble());
    
    addRowToTable(storageTable, {
        item["logicalname"].toString(),
        size,
        type,
        item["product"].toString(),
        item["vendor"].toString()
    });
}

// Function to add a progress bar row for storage usage
inline void addStorageProgressBarRow(QTableWidget* storageTable, double usagePercent, const QString& devicePath)
{
    if (!storageTable) return;
    
    qDebug() << "Adding progress bar for" << devicePath << "with" << usagePercent << "% usage";
    
    int row = storageTable->rowCount();
    storageTable->insertRow(row);
    
    // Create a progress bar widget
    QProgressBar* progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(static_cast<int>(usagePercent));
    progressBar->setTextVisible(true);
    progressBar->setFormat(QString("%1% used").arg(QString::number(usagePercent, 'f', 1)));
    
    // Style the progress bar with colors based on usage
    QString styleSheet;
    if (usagePercent >= 90) {
        styleSheet = "QProgressBar::chunk { background-color: #e74c3c; }";
    } else if (usagePercent >= 75) {
        styleSheet = "QProgressBar::chunk { background-color: #f39c12; }";
    } else {
        styleSheet = "QProgressBar::chunk { background-color: #27ae60; }";
    }
    progressBar->setStyleSheet(styleSheet);
    
    // Span the progress bar across all columns
    storageTable->setCellWidget(row, 0, progressBar);
    storageTable->setSpan(row, 0, 1, storageTable->columnCount());
    
    // Set row height for better visibility
    storageTable->setRowHeight(row, 25);
}

inline void loadLiveStorageInformation(QTableWidget* storageTable)
{
    if (!storageTable) return;
    
    // Clear existing data
    storageTable->setRowCount(0);
    
    qDebug() << "Loading live storage information with progress bars...";
    
    // Display integrated physical disk and filesystem information
    displayIntegratedStorageInformation(storageTable);
}

inline void displayIntegratedStorageInformation(QTableWidget* storageTable)
{
    // Get physical disks
    QList<PhysicalDisk> disks = getPhysicalDisks();
    
    // Get all mounted filesystems with live usage data
    QList<QStorageInfo> storageList = QStorageInfo::mountedVolumes();
    
    // Get block device information from /proc/mounts and /sys/block
    QMap<QString, QString> deviceInfo = getBlockDeviceInfo();
    
    for (const PhysicalDisk &disk : disks) {
        // Add physical disk row
        QString sizeStr = disk.totalSize > 0 ? formatSize(disk.totalSize) : "";
        QString typeInfo = "";
        if (!disk.type.isEmpty() && disk.type != "Unknown") {
            typeInfo = disk.type;
        }
        QString modelInfo = "";
        if (!disk.model.isEmpty() && disk.model != "Unknown") {
            modelInfo = disk.model;
            if (!disk.vendor.isEmpty() && disk.vendor != "Unknown" && !disk.model.contains(disk.vendor)) {
                modelInfo += " (" + disk.vendor + ")";
            }
        }
        
        QStringList diskRowData = {
            disk.device,        // Device
            "",                 // Mount Point (empty for physical disk)
            sizeStr,            // Size
            "",                 // Used (empty for physical disk)
            "",                 // Available (empty for physical disk) 
            "",                 // Use% (empty for physical disk)
            "",                 // Filesystem (empty for physical disk)
            typeInfo,           // Type
            modelInfo           // Model
        };
        
        addRowToTable(storageTable, diskRowData);
        int diskRow = storageTable->rowCount() - 1;
        
        // Make disk row bold and slightly larger
        for (int col = 0; col < storageTable->columnCount(); col++) {
            QTableWidgetItem* item = storageTable->item(diskRow, col);
            if (item) {
                QFont boldFont = item->font();
                boldFont.setBold(true);
                boldFont.setPointSize(boldFont.pointSize() + 1);  // 1px larger
                item->setFont(boldFont);
            }
        }
        
        // Add Details button for physical disk
        QPushButton* diskDetailsButton = new QPushButton("Details");
        diskDetailsButton->setMaximumWidth(80);
        PhysicalDisk diskCopy = disk; // Create a copy for the lambda
        
        QObject::connect(diskDetailsButton, &QPushButton::clicked, [storageTable, diskCopy]() {
            showDiskDetails(storageTable->window(), diskCopy);
        });
        
        storageTable->setCellWidget(diskRow, 8, diskDetailsButton);
        
        // Add empty separator line after physical disk
        QStringList separatorRowData = {"", "", "", "", "", "", "", "", ""};
        addRowToTable(storageTable, separatorRowData);
        int separatorRow = storageTable->rowCount() - 1;
        storageTable->setRowHeight(separatorRow, 10); // Thin separator
        
        // Store first partition row for border styling
        int firstPartitionRow = -1;
        int lastPartitionRow = -1;
        
        // Now add partitions for this disk
        for (const QStorageInfo &storage : storageList) {
            if (!storage.isValid() || storage.isReadOnly()) {
                continue; // Skip invalid or special filesystems
            }
            
            QString devicePath = storage.device();
            QString mountPoint = storage.rootPath();
            QString fileSystemType = storage.fileSystemType();
            
            // Skip special filesystems that aren't real storage
            if (fileSystemType == "tmpfs" || fileSystemType == "devtmpfs" || 
                fileSystemType == "sysfs" || fileSystemType == "proc" ||
                fileSystemType == "devpts" || fileSystemType == "cgroup" ||
                mountPoint.startsWith("/snap/") || mountPoint.startsWith("/run/") ||
                mountPoint.startsWith("/sys/") || mountPoint.startsWith("/dev/")) {
                continue;
            }
            
            // Check if this partition belongs to the current physical disk
            if (!devicePath.startsWith(disk.device)) {
                continue;
            }
            
            qint64 totalBytes = storage.bytesTotal();
            qint64 availableBytes = storage.bytesAvailable();
            qint64 usedBytes = totalBytes - availableBytes;
            
            QString totalSize = formatSize(totalBytes);
            QString usedSize = formatSize(usedBytes);
            QString availableSize = formatSize(availableBytes);
            
            // Calculate usage percentage
            double usagePercent = totalBytes > 0 ? (double(usedBytes) / totalBytes) * 100.0 : 0.0;
            QString usagePercentStr = QString::number(usagePercent, 'f', 1) + "%";
            
            // Get device type and model from our device info map
            QString deviceType = "";
            QString deviceModel = "";
            
            // Add partition row (indented)
            QStringList partitionRowData = {
                "    " + devicePath,       // Device (indented)
                mountPoint,                // Mount Point  
                totalSize,                 // Size
                usedSize,                  // Used
                availableSize,             // Available
                usagePercentStr,           // Use%
                fileSystemType,            // Filesystem
                deviceType,                // Type (empty for partition)
                deviceModel                // Model (empty for partition)
            };
            
            addRowToTable(storageTable, partitionRowData);
            int partitionRow = storageTable->rowCount() - 1;
            
            // Track first and last partition rows for border styling
            if (firstPartitionRow == -1) {
                firstPartitionRow = partitionRow;
            }
            lastPartitionRow = partitionRow;
            
            // Add Details button for partition
            QPushButton* partDetailsButton = new QPushButton("Details");
            partDetailsButton->setMaximumWidth(80);
            PartitionInfo partitionCopy; // Create partition info from storage data
            partitionCopy.device = devicePath;
            partitionCopy.mountPoint = mountPoint;
            partitionCopy.filesystem = fileSystemType;
            partitionCopy.totalSize = totalBytes;
            partitionCopy.availableSize = availableBytes;
            partitionCopy.usedSize = usedBytes;
            
            QObject::connect(partDetailsButton, &QPushButton::clicked, [storageTable, partitionCopy]() {
                showPartitionDetails(storageTable->window(), partitionCopy);
            });
            
            storageTable->setCellWidget(partitionRow, 8, partDetailsButton);
            
            // Add progress bar row for this partition
            storageTable->insertRow(storageTable->rowCount());
            int progressRow = storageTable->rowCount() - 1;
            
            // Create a container widget for centering the progress bar
            QWidget* container = new QWidget();
            QHBoxLayout* hLayout = new QHBoxLayout(container);
            hLayout->setContentsMargins(5, 5, 5, 5);  // Small margins on all sides
            
            // Create progress bar with proper size and styling
            QProgressBar* progressBar = new QProgressBar();
            progressBar->setRange(0, 100);
            progressBar->setValue(static_cast<int>(usagePercent));
            progressBar->setTextVisible(true);
            progressBar->setFormat(QString("%1% used (%2 of %3)").arg(
                QString::number(usagePercent, 'f', 1), usedSize, totalSize));
            progressBar->setFixedHeight(18);  // Fixed height to fit properly
            
            // Style the progress bar with colors, rounded corners, and centering
            QString styleSheet = "QProgressBar { "
                "text-align: center; "
                "border: 1px solid #ccc; "
                "border-radius: 8px; "
                "background-color: #f0f0f0; "
                "font-weight: bold; "
                "font-size: 11px; "
                "} "
                "QProgressBar::chunk { "
                "border-radius: 7px; ";
            
            if (usagePercent >= 90) {
                styleSheet += "background-color: #e74c3c; ";
            } else if (usagePercent >= 75) {
                styleSheet += "background-color: #f39c12; ";
            } else {
                styleSheet += "background-color: #27ae60; ";
            }
            styleSheet += "}";
            progressBar->setStyleSheet(styleSheet);
            
            // Add to horizontal layout with 75% width and centering
            hLayout->addStretch(1);  // 12.5% left margin
            hLayout->addWidget(progressBar, 3);  // 75% width
            hLayout->addStretch(1);  // 12.5% right margin
            
            // Set the container to span all columns (merged row)
            storageTable->setCellWidget(progressRow, 0, container);
            storageTable->setSpan(progressRow, 0, 1, storageTable->columnCount());
            
            // Set row height to accommodate progress bar properly (increased to prevent truncation)
            storageTable->setRowHeight(progressRow, 35);
        }
        
        // Add visual divider between disk groups (like <hr width="75%">)
        if (firstPartitionRow != -1 && lastPartitionRow != -1) {
            // Create a horizontal divider row
            QStringList dividerRowData = {"", "", "", "", "", "", "", "", ""};
            addRowToTable(storageTable, dividerRowData);
            int dividerRow = storageTable->rowCount() - 1;
            
            // Create a custom divider widget that looks like <hr width="75%">
            QWidget* dividerWidget = new QWidget();
            dividerWidget->setFixedHeight(20);
            dividerWidget->setStyleSheet("background-color: transparent;");
            
            QHBoxLayout* dividerLayout = new QHBoxLayout(dividerWidget);
            dividerLayout->setContentsMargins(0, 8, 0, 8);
            
            // Add spacing before the line (12.5%)
            dividerLayout->addStretch(1);
            
            // Create the actual divider line (75% width)
            QFrame* line = new QFrame();
            line->setFrameShape(QFrame::HLine);
            line->setFrameShadow(QFrame::Sunken);
            line->setStyleSheet("QFrame { color: #999999; background-color: #999999; height: 2px; }");
            dividerLayout->addWidget(line, 3);
            
            // Add spacing after the line (12.5%)
            dividerLayout->addStretch(1);
            
            // Set the divider to span all columns
            storageTable->setCellWidget(dividerRow, 0, dividerWidget);
            storageTable->setSpan(dividerRow, 0, 1, storageTable->columnCount());
            storageTable->setRowHeight(dividerRow, 20);
        }
    }
}

inline void displayFilesystemInformation(QTableWidget* storageTable)
{
    // Get all mounted filesystems with live usage data
    QList<QStorageInfo> storageList = QStorageInfo::mountedVolumes();
    
    // Get block device information from /proc/mounts and /sys/block
    QMap<QString, QString> deviceInfo = getBlockDeviceInfo();
    
    for (const QStorageInfo &storage : storageList) {
        if (!storage.isValid() || storage.isReadOnly()) {
            continue; // Skip invalid or special filesystems
        }
        
        QString devicePath = storage.device();
        QString mountPoint = storage.rootPath();
        QString fileSystemType = storage.fileSystemType();
        
        // Skip special filesystems that aren't real storage
        if (fileSystemType == "tmpfs" || fileSystemType == "devtmpfs" || 
            fileSystemType == "sysfs" || fileSystemType == "proc" ||
            fileSystemType == "devpts" || fileSystemType == "cgroup" ||
            mountPoint.startsWith("/snap/") || mountPoint.startsWith("/run/") ||
            mountPoint.startsWith("/sys/") || mountPoint.startsWith("/dev/")) {
            continue;
        }
        
        qint64 totalBytes = storage.bytesTotal();
        qint64 availableBytes = storage.bytesAvailable();
        qint64 usedBytes = totalBytes - availableBytes;
        
        QString totalSize = formatSize(totalBytes);
        QString usedSize = formatSize(usedBytes);
        QString availableSize = formatSize(availableBytes);
        
        // Calculate usage percentage
        double usagePercent = totalBytes > 0 ? (double(usedBytes) / totalBytes) * 100.0 : 0.0;
        QString usagePercentStr = QString::number(usagePercent, 'f', 1) + "%";
        
        // Get device type and model from our device info map
        QString deviceType = "Unknown";
        QString deviceModel = "Unknown";
        
        // Extract the base device name (e.g., /dev/sda1 -> sda)
        QString baseDev = devicePath;
        if (baseDev.startsWith("/dev/")) {
            baseDev = baseDev.mid(5); // Remove "/dev/"
            // Remove partition numbers (e.g., sda1 -> sda, nvme0n1p1 -> nvme0n1)
            baseDev = baseDev.replace(QRegularExpression("[0-9]+$"), "");
            if (baseDev.contains("nvme") && baseDev.endsWith("n1")) {
                // Keep nvme0n1 as is
            } else if (baseDev.endsWith("p")) {
                baseDev.chop(1); // Remove trailing 'p' from nvme0n1p
            }
        }
        
        if (deviceInfo.contains(baseDev + "_type")) {
            deviceType = deviceInfo[baseDev + "_type"];
        }
        if (deviceInfo.contains(baseDev + "_model")) {
            deviceModel = deviceInfo[baseDev + "_model"];
        }
        
        // Add row to storage table with comprehensive information
        QStringList rowData = {
            devicePath,                    // Device
            mountPoint,                    // Mount Point  
            totalSize,                     // Size
            usedSize,                      // Used
            availableSize,                 // Available
            usagePercentStr,               // Use%
            fileSystemType,                // Filesystem
            deviceType,                    // Type
            deviceModel                    // Model
        };
        
        addRowToTable(storageTable, rowData);
        
        // Replace the Use% cell (column 5) with a progress bar
        int currentRow = storageTable->rowCount() - 1;
        
        // Create a container widget for centering the progress bar in the cell
        QWidget* container = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout(container);
        layout->setContentsMargins(2, 2, 2, 2);  // Small margins
        
        // Create progress bar with proper size and styling
        QProgressBar* progressBar = new QProgressBar();
        progressBar->setRange(0, 100);
        progressBar->setValue(static_cast<int>(usagePercent));
        progressBar->setTextVisible(true);
        progressBar->setFormat(QString("%1%").arg(QString::number(usagePercent, 'f', 1)));
        progressBar->setMinimumHeight(18);  // Set height to fit in cell
        progressBar->setMaximumHeight(18);  // Fixed height
        
        // Style the progress bar with colors, rounded corners, and centering
        QString styleSheet = "QProgressBar { "
            "text-align: center; "
            "border: 1px solid #ccc; "
            "border-radius: 9px; "
            "background-color: #f0f0f0; "
            "font-weight: bold; "
            "font-size: 11px; "
            "} "
            "QProgressBar::chunk { "
            "border-radius: 8px; ";
        
        if (usagePercent >= 90) {
            styleSheet += "background-color: #e74c3c; ";
        } else if (usagePercent >= 75) {
            styleSheet += "background-color: #f39c12; ";
        } else {
            styleSheet += "background-color: #27ae60; ";
        }
        styleSheet += "}";
        progressBar->setStyleSheet(styleSheet);
        
        // Add to layout with 75% width and centering
        layout->addStretch(1);  // 12.5% left margin
        layout->addWidget(progressBar, 3);  // 75% width
        layout->addStretch(1);  // 12.5% right margin
        
        // Replace the Use% cell with our progress bar container
        storageTable->setCellWidget(currentRow, 5, container);
    }
}

inline void displayPhysicalDiskInformation(QTableWidget* storageTable)
{
    QList<PhysicalDisk> disks = getPhysicalDisks();
    
    for (const PhysicalDisk &disk : disks) {
        QString sizeStr = disk.totalSize > 0 ? formatSize(disk.totalSize) : "";
        QString typeInfo = "";
        if (!disk.type.isEmpty() && disk.type != "Unknown") {
            typeInfo = disk.type;
        }
        QString modelInfo = "";
        if (!disk.model.isEmpty() && disk.model != "Unknown") {
            modelInfo = disk.model;
            if (!disk.vendor.isEmpty() && disk.vendor != "Unknown" && !disk.model.contains(disk.vendor)) {
                modelInfo += " (" + disk.vendor + ")";
            }
        }
        
        int row = storageTable->rowCount();
        storageTable->setRowCount(row + 1);
        
        // Add physical disk information with Details button
        QTableWidgetItem* diskDeviceItem = new QTableWidgetItem(disk.device);
        QFont boldFont = diskDeviceItem->font();
        boldFont.setBold(true);
        diskDeviceItem->setFont(boldFont);
        storageTable->setItem(row, 0, diskDeviceItem);
        
        // Set dark blue color for data columns (excluding device name)
        QColor darkBlue(0, 100, 200); // Vibrant blue color
        
        QTableWidgetItem* typeItem = new QTableWidgetItem("Physical Disk");
        typeItem->setForeground(QBrush(darkBlue));
        storageTable->setItem(row, 1, typeItem);
        
        QTableWidgetItem* sizeItem = new QTableWidgetItem(sizeStr);
        sizeItem->setForeground(QBrush(darkBlue));
        storageTable->setItem(row, 2, sizeItem);
        
        QTableWidgetItem* usedItem = new QTableWidgetItem("-");
        usedItem->setForeground(QBrush(darkBlue));
        storageTable->setItem(row, 3, usedItem);
        
        QTableWidgetItem* availItem = new QTableWidgetItem("-");
        availItem->setForeground(QBrush(darkBlue));
        storageTable->setItem(row, 4, availItem);
        
        QTableWidgetItem* usageItem = new QTableWidgetItem("-");
        usageItem->setForeground(QBrush(darkBlue));
        storageTable->setItem(row, 5, usageItem);
        
        QTableWidgetItem* interfaceItem = new QTableWidgetItem(disk.interface.isEmpty() ? "" : disk.interface);
        interfaceItem->setForeground(QBrush(darkBlue));
        storageTable->setItem(row, 6, interfaceItem);
        
        QTableWidgetItem* typeInfoItem = new QTableWidgetItem(typeInfo);
        typeInfoItem->setForeground(QBrush(darkBlue));
        storageTable->setItem(row, 7, typeInfoItem);
        
        // Add Details button for physical disk
        QPushButton* diskDetailsButton = new QPushButton("Details");
        diskDetailsButton->setMaximumWidth(80);
        
        // Create a copy of disk data for the lambda
        PhysicalDisk diskCopy = disk;
        QObject::connect(diskDetailsButton, &QPushButton::clicked, [storageTable, diskCopy]() {
            showDiskDetails(storageTable->window(), diskCopy);
        });
        
        storageTable->setCellWidget(row, 8, diskDetailsButton);
        
        // Now add all partitions for this disk
        QList<PartitionInfo> partitions = getPartitionsForDisk(disk.device);
        for (const PartitionInfo &partition : partitions) {
            row = storageTable->rowCount();
            storageTable->setRowCount(row + 1);
            
            QString partSizeStr = partition.totalSize > 0 ? formatSize(partition.totalSize) : "";
            QString partUsedStr = partition.usedSize > 0 ? formatSize(partition.usedSize) : "";
            QString partAvailStr = partition.availableSize > 0 ? formatSize(partition.availableSize) : "";
            QString partUsageStr = "";
            if (partition.totalSize > 0 && partition.usedSize > 0) {
                double usedPercent = (double)partition.usedSize / partition.totalSize * 100.0;
                partUsageStr = QString::number(usedPercent, 'f', 1) + "%";
            }
            
            // Format partition device name (indent to show hierarchy)
            QString partDevice = "  " + partition.device; // Indent with spaces
            
            // Add partition information
            storageTable->setItem(row, 0, new QTableWidgetItem(partDevice));
            
            // Set dark blue color for partition data columns (excluding device name)
            QColor darkBlue(0, 100, 200); // Vibrant blue color
            
            QTableWidgetItem* mountItem = new QTableWidgetItem(partition.mountPoint.isEmpty() ? "Unmounted" : partition.mountPoint);
            mountItem->setForeground(QBrush(darkBlue));
            storageTable->setItem(row, 1, mountItem);
            
            QTableWidgetItem* partSizeItem = new QTableWidgetItem(partSizeStr);
            partSizeItem->setForeground(QBrush(darkBlue));
            storageTable->setItem(row, 2, partSizeItem);
            
            QTableWidgetItem* partUsedItem = new QTableWidgetItem(partUsedStr);
            partUsedItem->setForeground(QBrush(darkBlue));
            storageTable->setItem(row, 3, partUsedItem);
            
            QTableWidgetItem* partAvailItem = new QTableWidgetItem(partAvailStr);
            partAvailItem->setForeground(QBrush(darkBlue));
            storageTable->setItem(row, 4, partAvailItem);
            
            QTableWidgetItem* partUsageItem = new QTableWidgetItem(partUsageStr);
            partUsageItem->setForeground(QBrush(darkBlue));
            storageTable->setItem(row, 5, partUsageItem);
            
            QTableWidgetItem* filesystemItem = new QTableWidgetItem(partition.filesystem);
            filesystemItem->setForeground(QBrush(darkBlue));
            storageTable->setItem(row, 6, filesystemItem);
            
            QTableWidgetItem* partTypeItem = new QTableWidgetItem("Partition");
            partTypeItem->setForeground(QBrush(darkBlue));
            storageTable->setItem(row, 7, partTypeItem);
            
            // Add Details button for partition
            QPushButton* partDetailsButton = new QPushButton("Details");
            partDetailsButton->setMaximumWidth(80);
            
            // Create a copy of partition data for the lambda
            PartitionInfo partitionCopy = partition;
            QObject::connect(partDetailsButton, &QPushButton::clicked, [storageTable, partitionCopy]() {
                showPartitionDetails(storageTable->window(), partitionCopy);
            });
            
            storageTable->setCellWidget(row, 8, partDetailsButton);
        }
        
        // Add empty separator row after each physical disk group
        if (!partitions.isEmpty()) {
            row = storageTable->rowCount();
            storageTable->setRowCount(row + 1);
            // Leave all cells empty for visual separation
        }
    }
}

inline void refreshStorageInfo(QTableWidget* storageTable)
{
    if (!storageTable) return;
    
    // Only refresh if table is visible and has been populated
    if (!storageTable->isVisible() || storageTable->rowCount() == 0) {
        return;
    }
    
    // Store current scroll position to maintain user experience
    int currentRow = storageTable->currentRow();
    int scrollValue = storageTable->verticalScrollBar() ? storageTable->verticalScrollBar()->value() : 0;
    
    // Get live storage information combining lshw data with filesystem data
    loadLiveStorageInformation(storageTable);
    
    // Restore scroll position
    if (storageTable->verticalScrollBar()) {
        storageTable->verticalScrollBar()->setValue(scrollValue);
    }
    if (currentRow >= 0 && currentRow < storageTable->rowCount()) {
        storageTable->setCurrentCell(currentRow, 0);
    }
}

inline void addLiveStorageToSummary(QTableWidget* summaryTable)
{
    if (!summaryTable) return;
    
    // Calculate total storage usage across all mounted filesystems
    QList<QStorageInfo> storageList = QStorageInfo::mountedVolumes();
    qint64 totalSystemStorage = 0;
    qint64 totalSystemUsed = 0;
    
    for (const QStorageInfo &storage : storageList) {
        if (!storage.isValid() || storage.isReadOnly()) {
            continue;
        }
        
        QString mountPoint = storage.rootPath();
        QString fileSystemType = storage.fileSystemType();
        
        // Skip special filesystems
        if (fileSystemType == "tmpfs" || fileSystemType == "devtmpfs" || 
            fileSystemType == "sysfs" || fileSystemType == "proc" ||
            fileSystemType == "devpts" || fileSystemType == "cgroup" ||
            mountPoint.startsWith("/snap/") || mountPoint.startsWith("/run/") ||
            mountPoint.startsWith("/sys/") || mountPoint.startsWith("/dev/")) {
            continue;
        }
        
        qint64 totalBytes = storage.bytesTotal();
        qint64 availableBytes = storage.bytesAvailable();
        qint64 usedBytes = totalBytes - availableBytes;
        
        totalSystemStorage += totalBytes;
        totalSystemUsed += usedBytes;
    }
    
    if (totalSystemStorage > 0) {
        double usagePercent = (double(totalSystemUsed) / totalSystemStorage) * 100.0;
        QString storageInfo = QString("Total: %1 Used: %2 (%3)")
                                     .arg(formatSize(totalSystemStorage))
                                     .arg(formatSize(totalSystemUsed))
                                     .arg(QString::number(usagePercent, 'f', 1) + "%");
        
        addRowToTable(summaryTable, {"Storage", storageInfo});
    }
}

// === Comprehensive Disk Information Function ===
inline void getDiskInformation(const QString &device, PhysicalDisk &disk)
{
    QString devicePath = "/sys/block/" + device;
    
    // === Get Model Information (prioritize lsblk for full names) ===
    disk.model = getDiskModelFromLsblk(device);
    if (disk.model.isEmpty() || disk.model == "Unknown") {
        disk.model = getDiskModelFromSys(device);
    }
    if (disk.model.isEmpty() || disk.model == "Unknown") {
        disk.model = getDiskModelFromSmart(device);
    }
    
    // === Get Vendor/Manufacturer ===
    disk.vendor = getDiskVendorFromSys(device);
    if (disk.vendor.isEmpty() || disk.vendor == "Unknown") {
        disk.vendor = getDiskVendorFromSmart(device);
    }
    
    // === Get Serial Number (prioritize lsblk) ===
    disk.serial = getDiskSerialFromLsblk(device);
    if (disk.serial.isEmpty() || disk.serial == "Unknown") {
        disk.serial = getDiskSerialFromSmart(device);
    }
    if (disk.serial.isEmpty() || disk.serial == "Unknown") {
        disk.serial = getDiskSerialFromHdparm(device);
    }
    if (disk.serial.isEmpty() || disk.serial == "Unknown") {
        disk.serial = getDiskSerialFromSys(device);
    }
    
    // === Get Size ===
    disk.totalSize = getDiskSizeFromSys(device);
    
    // === Get Interface Type ===
    disk.interface = getDiskInterfaceFromSys(device);
    
    // === Determine Disk Type (SSD/HDD/NVMe) ===
    disk.type = getDiskTypeFromSys(device);
    
    // === Get Health Status ===
    disk.health = getDiskHealthFromSmart(device);
    
    // === Get Temperature ===
    disk.temperature = getDiskTemperatureFromSmart(device);
    
    // === Get Firmware Version ===
    disk.firmware = getDiskFirmwareFromSmart(device);
    if (disk.firmware.isEmpty() || disk.firmware == "Unknown") {
        disk.firmware = getDiskFirmwareFromSys(device);
    }
}

inline void showDiskDetails(QWidget* parent, const PhysicalDisk& disk)
{
    QDialog* dialog = new QDialog(parent);
    dialog->setWindowTitle("Disk Details - " + disk.device);
    dialog->setMinimumSize(400, 300);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    
    QTableWidget* detailsTable = new QTableWidget(dialog);
    detailsTable->setColumnCount(2);
    detailsTable->setHorizontalHeaderLabels({"Property", "Value"});
    detailsTable->horizontalHeader()->setStretchLastSection(true);
    detailsTable->verticalHeader()->setVisible(false);
    detailsTable->setAlternatingRowColors(true);
    detailsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    detailsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    int row = 0;
    
    // Only add rows if the information is available (not empty/unknown)
    auto addDetailRow = [&](const QString& property, const QString& value) {
        if (!value.isEmpty() && value != "Unknown" && value != "N/A" && value != "-") {
            detailsTable->setRowCount(row + 1);
            detailsTable->setItem(row, 0, new QTableWidgetItem(property));
            
            // Set dark blue color for the value column
            QTableWidgetItem* valueItem = new QTableWidgetItem(value);
            valueItem->setForeground(QBrush(QColor(0, 100, 200))); // Vibrant blue color
            detailsTable->setItem(row, 1, valueItem);
            row++;
        }
    };
    
    addDetailRow("Device", disk.device);
    addDetailRow("Model", disk.model);
    addDetailRow("Vendor", disk.vendor);
    addDetailRow("Type", disk.type);
    if (disk.totalSize > 0) {
        addDetailRow("Total Size", formatSize(disk.totalSize));
    }
    addDetailRow("Serial Number", disk.serial);
    addDetailRow("Interface", disk.interface);
    addDetailRow("Health Status", disk.health);
    if (disk.temperature > 0) {
        addDetailRow("Temperature", QString::number(disk.temperature) + "°C");
    }
    addDetailRow("Firmware Version", disk.firmware);
    
    // Add additional technical information
    QFile deviceInfo("/sys/block/" + disk.device.split('/').last() + "/queue/rotational");
    if (deviceInfo.open(QIODevice::ReadOnly)) {
        QString rotational = deviceInfo.readAll().trimmed();
        addDetailRow("Rotational Media", rotational == "1" ? "Yes (HDD)" : "No (SSD)");
        deviceInfo.close();
    }
    
    layout->addWidget(detailsTable);
    
    // Add close button
    QPushButton* closeButton = new QPushButton("Close", dialog);
    QObject::connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    layout->addWidget(closeButton);
    
    dialog->exec();
}

inline void showPartitionDetails(QWidget* parent, const PartitionInfo& partition)
{
    QDialog* dialog = new QDialog(parent);
    dialog->setWindowTitle("Partition Details - " + partition.device);
    dialog->setMinimumSize(400, 300);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    
    QTableWidget* detailsTable = new QTableWidget(dialog);
    detailsTable->setColumnCount(2);
    detailsTable->setHorizontalHeaderLabels({"Property", "Value"});
    detailsTable->horizontalHeader()->setStretchLastSection(true);
    detailsTable->verticalHeader()->setVisible(false);
    detailsTable->setAlternatingRowColors(true);
    detailsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    detailsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    int row = 0;
    
    // Only add rows if the information is available (not empty/unknown)
    auto addDetailRow = [&](const QString& property, const QString& value) {
        if (!value.isEmpty() && value != "Unknown" && value != "N/A" && value != "-") {
            detailsTable->setRowCount(row + 1);
            detailsTable->setItem(row, 0, new QTableWidgetItem(property));
            
            // Set dark blue color for the value column
            QTableWidgetItem* valueItem = new QTableWidgetItem(value);
            valueItem->setForeground(QBrush(QColor(0, 100, 200))); // Vibrant blue color
            detailsTable->setItem(row, 1, valueItem);
            row++;
        }
    };
    
    addDetailRow("Device", partition.device);
    addDetailRow("Parent Disk", partition.parentDisk);
    addDetailRow("Filesystem", partition.filesystem);
    addDetailRow("Mount Point", partition.mountPoint);
    addDetailRow("Label", partition.label);
    addDetailRow("UUID", partition.uuid);
    addDetailRow("Partition Type", partition.partitionType);
    addDetailRow("Mounted", partition.isMounted ? "Yes" : "No");
    
    if (partition.totalSize > 0) {
        addDetailRow("Total Size", formatSize(partition.totalSize));
    }
    if (partition.usedSize > 0) {
        addDetailRow("Used Space", formatSize(partition.usedSize));
        double usedPercent = (double)partition.usedSize / partition.totalSize * 100.0;
        addDetailRow("Used Percentage", QString::number(usedPercent, 'f', 1) + "%");
    }
    if (partition.availableSize > 0) {
        addDetailRow("Available Space", formatSize(partition.availableSize));
    }
    
    layout->addWidget(detailsTable);
    
    // Add close button
    QPushButton* closeButton = new QPushButton("Close", dialog);
    QObject::connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    layout->addWidget(closeButton);
    
    dialog->exec();
}

#endif // STORAGE_H
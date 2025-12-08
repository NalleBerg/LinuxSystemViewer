#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <QTableWidget>
#include <QObject>
#include <QTableWidgetItem>
#include <QJsonObject>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QRegularExpression>
#include <QStringList>
#include <QDebug>
#include <QFont>
#include <QDir>
#include <QFileInfo>
#include "gui_helpers.h"

// Graphics information functions
void loadGraphicsInformation(QTableWidget* table, const QJsonObject& data);
QStringList getGraphicsHeaders();
void styleGraphicsTable(QTableWidget* table);

// Graphics Headers
QStringList getGraphicsHeaders()
{
    return QStringList() << QObject::tr("Property") << QObject::tr("Value");
}

// Graphics Table Styling
void styleGraphicsTable(QTableWidget* table)
{
    // Set column widths
    table->setColumnWidth(0, 200);  // Property
    table->setColumnWidth(1, 400);  // Value
    
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

// Helper to add a simple 2-column row
inline void addGraphicsRow(QTableWidget* table, const QString& property, const QString& value)
{
    int row = table->rowCount();
    table->insertRow(row);
    
    QTableWidgetItem* propItem = new QTableWidgetItem(property);
    QFont boldFont;
    boldFont.setBold(true);
    propItem->setFont(boldFont);
    table->setItem(row, 0, propItem);
    
    QTableWidgetItem* valItem = new QTableWidgetItem(value);
    table->setItem(row, 1, valItem);
    
    table->resizeRowToContents(row);
}

// Load Graphics Information - User-friendly view (reads directly from system)
void loadGraphicsInformation(QTableWidget* table, const QJsonObject& data)
{
    Q_UNUSED(data);
    
    table->setRowCount(0);
    
    QString cardName, vendor, driver, deviceId, subsystemVendor, subsystemDevice;
    
    // Scan /sys/class/drm for graphics cards
    QDir drmDir("/sys/class/drm");
    QStringList drmCards = drmDir.entryList(QStringList() << "card*", QDir::Dirs | QDir::NoDotAndDotDot);
    
    // Also check PCI devices directly
    QDir pciDir("/sys/bus/pci/devices");
    QStringList pciDevices = pciDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QString& pciDev : pciDevices) {
        QString classPath = QString("/sys/bus/pci/devices/%1/class").arg(pciDev);
        QFile classFile(classPath);
        
        if (classFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString deviceClass = QTextStream(&classFile).readLine().trimmed();
            classFile.close();
            
            // Check if it's a VGA/Display controller (class 0x03xxxx)
            if (deviceClass.startsWith("0x03")) {
                // Read vendor ID
                QFile vendorFile(QString("/sys/bus/pci/devices/%1/vendor").arg(pciDev));
                if (vendorFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString vendorId = QTextStream(&vendorFile).readLine().trimmed();
                    vendorFile.close();
                    
                    if (vendorId == "0x10de") {
                        vendor = "NVIDIA";
                    } else if (vendorId == "0x1002") {
                        vendor = "AMD";
                    } else if (vendorId == "0x8086") {
                        vendor = "Intel";
                    } else {
                        vendor = vendorId;
                    }
                }
                
                // Read device ID
                QFile deviceFile(QString("/sys/bus/pci/devices/%1/device").arg(pciDev));
                if (deviceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    deviceId = QTextStream(&deviceFile).readLine().trimmed();
                    deviceFile.close();
                }
                
                // Read current driver
                QString driverPath = QString("/sys/bus/pci/devices/%1/driver").arg(pciDev);
                QFileInfo driverLink(driverPath);
                if (driverLink.isSymLink()) {
                    QString driverTarget = driverLink.symLinkTarget();
                    driver = QFileInfo(driverTarget).fileName();
                }
                
                // Read subsystem info for card name
                QFile subsysVendorFile(QString("/sys/bus/pci/devices/%1/subsystem_vendor").arg(pciDev));
                if (subsysVendorFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    subsystemVendor = QTextStream(&subsysVendorFile).readLine().trimmed();
                    subsysVendorFile.close();
                }
                
                QFile subsysDeviceFile(QString("/sys/bus/pci/devices/%1/subsystem_device").arg(pciDev));
                if (subsysDeviceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    subsystemDevice = QTextStream(&subsysDeviceFile).readLine().trimmed();
                    subsysDeviceFile.close();
                }
                
                // Try to read a friendly name from uevent
                QFile ueventFile(QString("/sys/bus/pci/devices/%1/uevent").arg(pciDev));
                if (ueventFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString ueventContent = QTextStream(&ueventFile).readAll();
                    ueventFile.close();
                    
                    QRegularExpression modalias("PCI_ID=([0-9A-F]{4}):([0-9A-F]{4})");
                    QRegularExpressionMatch match = modalias.match(ueventContent);
                    if (match.hasMatch()) {
                        cardName = QString("%1 [%2:%3]").arg(vendor, match.captured(1), match.captured(2));
                    }
                }
                
                break; // Use first graphics card found
            }
        }
    }
    
    // Add basic info
    if (!vendor.isEmpty()) {
        addGraphicsRow(table, QObject::tr("Vendor"), vendor);
    }
    
    if (!cardName.isEmpty()) {
        addGraphicsRow(table, QObject::tr("Graphics Card"), cardName);
    } else if (!deviceId.isEmpty()) {
        addGraphicsRow(table, QObject::tr("Device ID"), deviceId);
    }
    
    if (!driver.isEmpty()) {
        addGraphicsRow(table, QObject::tr("Driver"), driver);
    }
    
    // Try to get OpenGL info from DRI
    QFile glVersionFile("/sys/kernel/debug/dri/0/name");
    if (glVersionFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString driName = QTextStream(&glVersionFile).readLine().trimmed();
        glVersionFile.close();
        if (!driName.isEmpty()) {
            addGraphicsRow(table, QObject::tr("DRI Device"), driName);
        }
    }
    
    // Read framebuffer info
    QDir fbDir("/sys/class/graphics");
    QStringList fbDevices = fbDir.entryList(QStringList() << "fb*", QDir::Dirs | QDir::NoDotAndDotDot);
    
    if (!fbDevices.isEmpty()) {
        QString fbDev = fbDevices.first();
        
        // Read resolution
        QFile modesFile(QString("/sys/class/graphics/%1/modes").arg(fbDev));
        if (modesFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString modes = QTextStream(&modesFile).readLine().trimmed();
            modesFile.close();
            if (!modes.isEmpty()) {
                addGraphicsRow(table, QObject::tr("Framebuffer Mode"), modes);
            }
        }
        
        // Read virtual resolution
        QFile virtualFile(QString("/sys/class/graphics/%1/virtual_size").arg(fbDev));
        if (virtualFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString virtualSize = QTextStream(&virtualFile).readLine().trimmed();
            virtualFile.close();
            if (!virtualSize.isEmpty()) {
                addGraphicsRow(table, QObject::tr("Virtual Resolution"), virtualSize.replace(',', 'x'));
            }
        }
    }
    
    // Try to read VRAM from debugfs (may require root)
    QFile vramFile("/sys/kernel/debug/dri/0/amdgpu_vram_mm");
    if (!vramFile.exists()) {
        vramFile.setFileName("/sys/kernel/debug/dri/0/i915_gem_objects");
    }
    
    if (vramFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString vramContent = QTextStream(&vramFile).readAll();
        vramFile.close();
        
        // Try to parse VRAM size
        QRegularExpression vramRegex("(\\d+)\\s*bytes");
        QRegularExpressionMatch match = vramRegex.match(vramContent);
        if (match.hasMatch()) {
            qint64 bytes = match.captured(1).toLongLong();
            double mb = bytes / (1024.0 * 1024.0);
            if (mb > 1024) {
                addGraphicsRow(table, QObject::tr("Video Memory"), QString::number(mb / 1024.0, 'f', 2) + " GB");
            } else {
                addGraphicsRow(table, QObject::tr("Video Memory"), QString::number(mb, 'f', 0) + " MB");
            }
        }
    }
    
    if (table->rowCount() == 0) {
        addGraphicsRow(table, QObject::tr("Status"), QObject::tr("No graphics card detected"));
    }
}

#endif // GRAPHICS_H

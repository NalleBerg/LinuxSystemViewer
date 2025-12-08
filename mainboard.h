#ifndef MAINBOARD_H
#define MAINBOARD_H

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QJsonObject>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>
#include "gui_helpers.h"

// Mainboard information functions
void loadMainboardInformation(QTableWidget* table, const QJsonObject& data);
QStringList getMainboardHeaders();
void styleMainboardTable(QTableWidget* table);
QString getMainboardInfo();

// Mainboard Headers
QStringList getMainboardHeaders()
{
    return QStringList() << "Property" << "Value";
}

// Mainboard Table Styling
void styleMainboardTable(QTableWidget* table)
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
inline void addMainboardRow(QTableWidget* table, const QString& property, const QString& value)
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

// Load Mainboard Information - User-friendly view
void loadMainboardInformation(QTableWidget* table, const QJsonObject& data)
{
    Q_UNUSED(data);
    
    table->setRowCount(0);
    
    // Get DMI baseboard information
    QProcess dmiProcess;
    dmiProcess.start("dmidecode", QStringList() << "-t" << "baseboard");
    dmiProcess.waitForFinished(3000);
    QString dmiOutput = dmiProcess.readAllStandardOutput();
    
    QString manufacturer, product, version, serialNumber;
    
    if (!dmiOutput.isEmpty()) {
        QStringList lines = dmiOutput.split('\n');
        for (const QString& line : lines) {
            QString trimmedLine = line.trimmed();
            if (trimmedLine.startsWith("Manufacturer:")) {
                manufacturer = trimmedLine.mid(13).trimmed();
            } else if (trimmedLine.startsWith("Product Name:")) {
                product = trimmedLine.mid(13).trimmed();
            } else if (trimmedLine.startsWith("Version:")) {
                version = trimmedLine.mid(8).trimmed();
            } else if (trimmedLine.startsWith("Serial Number:")) {
                serialNumber = trimmedLine.mid(14).trimmed();
            }
        }
    }
    
    // Get system information for additional context
    dmiProcess.start("dmidecode", QStringList() << "-t" << "system");
    dmiProcess.waitForFinished(3000);
    QString systemOutput = dmiProcess.readAllStandardOutput();
    
    QString systemManufacturer, systemProduct, systemFamily;
    
    if (!systemOutput.isEmpty()) {
        QStringList lines = systemOutput.split('\n');
        for (const QString& line : lines) {
            QString trimmedLine = line.trimmed();
            if (trimmedLine.startsWith("Manufacturer:")) {
                systemManufacturer = trimmedLine.mid(13).trimmed();
            } else if (trimmedLine.startsWith("Product Name:")) {
                systemProduct = trimmedLine.mid(13).trimmed();
            } else if (trimmedLine.startsWith("Family:")) {
                systemFamily = trimmedLine.mid(7).trimmed();
            }
        }
    }
    
    // Add user-friendly rows
    if (!manufacturer.isEmpty() && manufacturer != "Not Specified" && manufacturer != "To Be Filled By O.E.M.") {
        addMainboardRow(table, QObject::tr("Manufacturer"), manufacturer);
    } else if (!systemManufacturer.isEmpty() && systemManufacturer != "Not Specified") {
        addMainboardRow(table, QObject::tr("Manufacturer"), systemManufacturer);
    }
    
    if (!product.isEmpty() && product != "Not Specified" && product != "To Be Filled By O.E.M.") {
        addMainboardRow(table, QObject::tr("Name"), product);
    } else if (!systemProduct.isEmpty() && systemProduct != "Not Specified") {
        addMainboardRow(table, QObject::tr("Name"), systemProduct);
    }
    
    if (!systemFamily.isEmpty() && systemFamily != "Not Specified" && systemFamily != "To Be Filled By O.E.M.") {
        addMainboardRow(table, QObject::tr("Type"), systemFamily);
    }
    
    if (!version.isEmpty() && version != "Not Specified" && version != "To Be Filled By O.E.M.") {
        addMainboardRow(table, QObject::tr("Version"), version);
    }
    
    if (!serialNumber.isEmpty() && serialNumber != "Not Specified" && serialNumber != "To Be Filled By O.E.M.") {
        addMainboardRow(table, QObject::tr("Serial Number"), serialNumber);
    }
    
    // Get BIOS information
    dmiProcess.start("dmidecode", QStringList() << "-t" << "bios");
    dmiProcess.waitForFinished(3000);
    QString biosOutput = dmiProcess.readAllStandardOutput();
    
    if (!biosOutput.isEmpty()) {
        QStringList lines = biosOutput.split('\n');
        QString biosVendor, biosVersion, biosDate;
        
        for (const QString& line : lines) {
            QString trimmedLine = line.trimmed();
            if (trimmedLine.startsWith("Vendor:")) {
                biosVendor = trimmedLine.mid(7).trimmed();
            } else if (trimmedLine.startsWith("Version:")) {
                biosVersion = trimmedLine.mid(8).trimmed();
            } else if (trimmedLine.startsWith("Release Date:")) {
                biosDate = trimmedLine.mid(13).trimmed();
            }
        }
        
        if (!biosVendor.isEmpty()) {
            addMainboardRow(table, QObject::tr("BIOS Vendor"), biosVendor);
        }
        if (!biosVersion.isEmpty()) {
            addMainboardRow(table, QObject::tr("BIOS Version"), biosVersion);
        }
        if (!biosDate.isEmpty()) {
            addMainboardRow(table, QObject::tr("BIOS Date"), biosDate);
        }
    }
    
    // Get chipset information from lspci
    QProcess lspciProcess;
    lspciProcess.start("lspci");
    lspciProcess.waitForFinished(3000);
    QString lspciOutput = lspciProcess.readAllStandardOutput();
    
    QStringList lspciLines = lspciOutput.split('\n');
    bool foundChipset = false;
    for (const QString& line : lspciLines) {
        if (line.contains("Host bridge:", Qt::CaseInsensitive)) {
            QString chipset = line.section(':', 2).trimmed();
            if (!chipset.isEmpty() && !foundChipset) {
                addMainboardRow(table, QObject::tr("Chipset"), chipset);
                foundChipset = true;
            }
        }
    }
}

QString getMainboardInfo()
{
    QProcess dmiProcess;
    dmiProcess.start("sudo", QStringList() << "dmidecode" << "-t" << "baseboard");
    dmiProcess.waitForFinished();
    QString dmiOutput = dmiProcess.readAllStandardOutput();
    
    QStringList lines = dmiOutput.split('\n');
    QString manufacturer, product;
    
    for (const QString& line : lines) {
        QString trimmedLine = line.trimmed();
        if (trimmedLine.startsWith("Manufacturer:")) {
            manufacturer = trimmedLine.mid(13).trimmed();
        } else if (trimmedLine.startsWith("Product Name:")) {
            product = trimmedLine.mid(13).trimmed();
        }
    }
    
    if (!manufacturer.isEmpty() && !product.isEmpty()) {
        return QString("%1 %2").arg(manufacturer, product);
    }
    
    return "Unknown Mainboard";
}

#endif // MAINBOARD_H
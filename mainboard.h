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
    return QStringList() << "Property" << "Value" << "Unit" << "Type";
}

// Mainboard Table Styling
void styleMainboardTable(QTableWidget* table)
{
    // Set column widths
    table->setColumnWidth(0, 200);  // Property
    table->setColumnWidth(1, 300);  // Value
    table->setColumnWidth(2, 80);   // Unit
    table->setColumnWidth(3, 120);  // Type
    
    // Style headers
    table->horizontalHeader()->setStyleSheet(
        "QHeaderView::section { "
        "background-color: #16a085; "
        "color: white; "
        "padding: 8px; "
        "border: none; "
        "font-weight: bold; "
        "}"
    );
}

// Load Mainboard Information
void loadMainboardInformation(QTableWidget* table, const QJsonObject& data)
{
    Q_UNUSED(data);
    
    table->setRowCount(0);
    
    // Get DMI information
    QProcess dmiProcess;
    dmiProcess.start("sudo", QStringList() << "dmidecode" << "-t" << "baseboard");
    dmiProcess.waitForFinished();
    QString dmiOutput = dmiProcess.readAllStandardOutput();
    
    if (!dmiOutput.isEmpty()) {
        QStringList lines = dmiOutput.split('\n');
        QString manufacturer, product, version, serialNumber;
        
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
        
        if (!manufacturer.isEmpty() && manufacturer != "Not Specified") {
            addRowToTable(table, QStringList() << "Manufacturer" << manufacturer << "" << "Mainboard");
        }
        if (!product.isEmpty() && product != "Not Specified") {
            addRowToTable(table, QStringList() << "Model" << product << "" << "Mainboard");
        }
        if (!version.isEmpty() && version != "Not Specified") {
            addRowToTable(table, QStringList() << "Version" << version << "" << "Mainboard");
        }
        if (!serialNumber.isEmpty() && serialNumber != "Not Specified") {
            addRowToTable(table, QStringList() << "Serial Number" << serialNumber << "" << "Mainboard");
        }
    }
    
    // Get BIOS information
    dmiProcess.start("sudo", QStringList() << "dmidecode" << "-t" << "bios");
    dmiProcess.waitForFinished();
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
            addRowToTable(table, QStringList() << "BIOS Vendor" << biosVendor << "" << "Mainboard");
        }
        if (!biosVersion.isEmpty()) {
            addRowToTable(table, QStringList() << "BIOS Version" << biosVersion << "" << "Mainboard");
        }
        if (!biosDate.isEmpty()) {
            addRowToTable(table, QStringList() << "BIOS Date" << biosDate << "" << "Mainboard");
        }
    }
    
    // Get chipset information from lspci
    QProcess lspciProcess;
    lspciProcess.start("lspci", QStringList() << "-v");
    lspciProcess.waitForFinished();
    QString lspciOutput = lspciProcess.readAllStandardOutput();
    
    QStringList lspciLines = lspciOutput.split('\n');
    for (const QString& line : lspciLines) {
        if (line.contains("Host bridge:") || line.contains("ISA bridge:")) {
            QString chipset = line.split(':').last().trimmed();
            if (line.contains("Host bridge:")) {
                addRowToTable(table, QStringList() << "Chipset" << chipset << "" << "Mainboard");
            } else if (line.contains("ISA bridge:")) {
                addRowToTable(table, QStringList() << "South Bridge" << chipset << "" << "Mainboard");
            }
        }
    }
    
    // Get USB controllers
    lspciProcess.start("lspci", QStringList() << "-v" << "-d" << "*:*");
    lspciProcess.waitForFinished();
    QString usbOutput = lspciProcess.readAllStandardOutput();
    
    QStringList usbLines = usbOutput.split('\n');
    QStringList usbControllers;
    for (const QString& line : usbLines) {
        if (line.contains("USB controller:")) {
            QString controller = line.split(':').last().trimmed();
            if (!usbControllers.contains(controller)) {
                usbControllers.append(controller);
            }
        }
    }
    
    for (int i = 0; i < usbControllers.size(); ++i) {
        addRowToTable(table, QStringList() << QString("USB Controller %1").arg(i + 1) << usbControllers[i] << "" << "Mainboard");
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
#include "motherboard_tab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QGroupBox>
#include <QFont>
#include <QRegularExpression>
#include <QDebug>

MotherboardTab::MotherboardTab(QWidget* parent)
    : TabWidgetBase("Motherboard", "lshw -C bus -short", true, 
                    "lshw -C bus && dmidecode -t baseboard 2>/dev/null && dmidecode -t system 2>/dev/null", parent)
{
    qDebug() << "MotherboardTab: Constructor called - base constructor done";
    initializeTab();
    qDebug() << "MotherboardTab: Constructor finished";
}

QWidget* MotherboardTab::createUserFriendlyView()
{
    qDebug() << "MotherboardTab: createUserFriendlyView called";
    
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    QWidget* contentWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    QLabel* titleLabel = new QLabel("Motherboard and System Information");
    titleLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  color: #2c3e50;"
        "  margin-bottom: 10px;"
        "}"
    );
    mainLayout->addWidget(titleLabel);
    
    createInfoSection("System Board", &m_systemBoardSection, &m_systemBoardContent, mainLayout);
    createInfoSection("Chipset", &m_chipsetSection, &m_chipsetContent, mainLayout);
    createInfoSection("BIOS/UEFI", &m_biosSection, &m_biosContent, mainLayout);
    createInfoSection("Expansion Slots", &m_expansionSlotsSection, &m_expansionSlotsContent, mainLayout);
    
    mainLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);
    
    qDebug() << "MotherboardTab: createUserFriendlyView completed";
    return scrollArea;
}

void MotherboardTab::createInfoSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout)
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
    *contentLabel = new QLabel("Loading " + title.toLower() + " information...");
    (*contentLabel)->setWordWrap(true);
    (*contentLabel)->setStyleSheet("QLabel { padding: 10px; background-color: #f8f9fa; border-radius: 4px; }");
    sectionLayout->addWidget(*contentLabel);
    
    parentLayout->addWidget(*groupBox);
}

void MotherboardTab::parseOutput(const QString& output)
{
    qDebug() << "MotherboardTab: parseOutput called";
    
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    QString systemBoardInfo = "System Board: Not detected";
    QString chipsetInfo = "Chipset: Not detected";
    QString biosInfo = "BIOS/UEFI: Not detected";
    QString expansionSlotsInfo = "Expansion Slots: Not detected";
    
    QStringList systemBoard;
    QStringList chipset;
    QStringList bios;
    QStringList expansionSlots;
    
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        
        // Parse lshw bus output
        if (trimmed.contains("bus") && !trimmed.startsWith("H/W path")) {
            QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 3) {
                QString description = parts.mid(2).join(" ");
                if (!description.isEmpty()) {
                    if (description.contains("PCI", Qt::CaseInsensitive) ||
                        description.contains("Host bridge", Qt::CaseInsensitive)) {
                        chipset.append(description);
                    } else {
                        expansionSlots.append(description);
                    }
                }
            }
        }
        
        // Parse dmidecode baseboard output
        if (trimmed.startsWith("Manufacturer:")) {
            QString manufacturer = trimmed.split(":")[1].trimmed();
            if (!manufacturer.isEmpty() && manufacturer != "Not Specified") {
                systemBoard.append("Manufacturer: " + manufacturer);
            }
        }
        
        if (trimmed.startsWith("Product Name:")) {
            QString product = trimmed.split(":")[1].trimmed();
            if (!product.isEmpty() && product != "Not Specified") {
                systemBoard.append("Product: " + product);
            }
        }
        
        if (trimmed.startsWith("Version:")) {
            QString version = trimmed.split(":")[1].trimmed();
            if (!version.isEmpty() && version != "Not Specified") {
                systemBoard.append("Version: " + version);
            }
        }
        
        if (trimmed.startsWith("Serial Number:")) {
            QString serial = trimmed.split(":")[1].trimmed();
            if (!serial.isEmpty() && serial != "Not Specified") {
                systemBoard.append("Serial: " + serial);
            }
        }
        
        // Parse BIOS information
        if (trimmed.startsWith("BIOS Information") || trimmed.startsWith("Vendor:")) {
            if (trimmed.startsWith("Vendor:")) {
                QString vendor = trimmed.split(":")[1].trimmed();
                if (!vendor.isEmpty()) {
                    bios.append("BIOS Vendor: " + vendor);
                }
            }
        }
        
        if (trimmed.startsWith("BIOS Revision:") || trimmed.startsWith("Firmware Revision:")) {
            QString revision = trimmed.split(":")[1].trimmed();
            if (!revision.isEmpty()) {
                bios.append("BIOS Revision: " + revision);
            }
        }
        
        if (trimmed.startsWith("Release Date:")) {
            QString date = trimmed.split(":")[1].trimmed();
            if (!date.isEmpty()) {
                bios.append("Release Date: " + date);
            }
        }
        
        // Look for system information
        if (trimmed.startsWith("System Information")) {
            // This indicates the start of system info section
        }
        
        if (trimmed.startsWith("Family:")) {
            QString family = trimmed.split(":")[1].trimmed();
            if (!family.isEmpty() && family != "Not Specified") {
                systemBoard.append("Family: " + family);
            }
        }
    }
    
    // Format the information
    if (!systemBoard.isEmpty()) {
        systemBoardInfo = "System Board:\n" + systemBoard.join("\n");
    }
    
    if (!chipset.isEmpty()) {
        chipsetInfo = "Chipset:\n" + chipset.join("\n");
    }
    
    if (!bios.isEmpty()) {
        biosInfo = "BIOS/UEFI:\n" + bios.join("\n");
    }
    
    if (!expansionSlots.isEmpty()) {
        expansionSlotsInfo = "Expansion Slots:\n" + expansionSlots.join("\n");
    }
    
    // Update the UI with parsed information
    if (m_systemBoardContent) {
        m_systemBoardContent->setText(systemBoardInfo);
    }
    if (m_chipsetContent) {
        m_chipsetContent->setText(chipsetInfo);
    }
    if (m_biosContent) {
        m_biosContent->setText(biosInfo);
    }
    if (m_expansionSlotsContent) {
        m_expansionSlotsContent->setText(expansionSlotsInfo);
    }
    
    qDebug() << "MotherboardTab: parseOutput completed";
}
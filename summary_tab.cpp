#include "summary_tab.h"
#include "tabs_config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QScrollArea>
#include <QGroupBox>
#include <QGridLayout>
#include <QFont>
#include <QRegularExpression>
#include <QDebug>

SummaryTab::SummaryTab(QWidget* parent)
    : TabWidgetBase("Summary", "lshw -short", true, "lshw", parent)
{
    qDebug() << "SummaryTab: Constructor called - base constructor done";
    initializeTab();
    qDebug() << "SummaryTab: Constructor finished";
}

QWidget* SummaryTab::createUserFriendlyView()
{
    qDebug() << "SummaryTab: createUserFriendlyView called";
    
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    QWidget* contentWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    QLabel* titleLabel = new QLabel("System Hardware Summary");
    titleLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  color: #2c3e50;"
        "  margin-bottom: 10px;"
        "}"
    );
    mainLayout->addWidget(titleLabel);
    
    m_systemOverview = new QGroupBox("System Overview");
    m_systemOverview->setStyleSheet(
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
    
    QVBoxLayout* overviewLayout = new QVBoxLayout(m_systemOverview);
    m_overviewContent = new QLabel("Loading system information...");
    m_overviewContent->setWordWrap(true);
    m_overviewContent->setStyleSheet("QLabel { padding: 10px; background-color: #f8f9fa; border-radius: 4px; }");
    overviewLayout->addWidget(m_overviewContent);
    
    mainLayout->addWidget(m_systemOverview);
    
    createHardwareSection("Processor", &m_cpuSection, &m_cpuContent, mainLayout);
    createHardwareSection("Memory", &m_memorySection, &m_memoryContent, mainLayout);
    createHardwareSection("Storage", &m_storageSection, &m_storageContent, mainLayout);
    createHardwareSection("Network", &m_networkSection, &m_networkContent, mainLayout);
    createHardwareSection("Graphics", &m_graphicsSection, &m_graphicsContent, mainLayout);
    
    mainLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);
    
    qDebug() << "SummaryTab: createUserFriendlyView completed";
    return scrollArea;
}

void SummaryTab::createHardwareSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout)
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

void SummaryTab::parseOutput(const QString& output)
{
    qDebug() << "SummaryTab: parseOutput called";
    
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    QString systemInfo = "System: Unknown\n";
    QString cpuInfo = "CPU: Not detected\n";
    QString memoryInfo = "Memory: Not detected\n";
    QString storageInfo = "Storage: Not detected\n";
    QString networkInfo = "Network: Not detected\n";
    QString graphicsInfo = "Graphics: Not detected\n";
    
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith("H/W path")) {
            continue;
        }
        
        QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() < 3) continue;
        
        QString path = parts[0];
        QString device = parts[1];
        QString description = parts.mid(2).join(" ");
        
        if (path.contains("/cpu") || device == "processor") {
            if (cpuInfo.contains("Not detected")) {
                cpuInfo = "CPU: " + description + "\n";
            } else {
                cpuInfo += "     " + description + "\n";
            }
        }
        else if (path.contains("/memory") || device == "memory") {
            if (memoryInfo.contains("Not detected")) {
                memoryInfo = "Memory: " + description + "\n";
            } else {
                memoryInfo += "        " + description + "\n";
            }
        }
        else if (path.contains("/disk") || path.contains("/storage") || device.contains("disk")) {
            if (storageInfo.contains("Not detected")) {
                storageInfo = "Storage: " + description + "\n";
            } else {
                storageInfo += "         " + description + "\n";
            }
        }
        else if (path.contains("/network") || device == "network") {
            if (networkInfo.contains("Not detected")) {
                networkInfo = "Network: " + description + "\n";
            } else {
                networkInfo += "         " + description + "\n";
            }
        }
        else if (path.contains("/display") || device == "display") {
            if (graphicsInfo.contains("Not detected")) {
                graphicsInfo = "Graphics: " + description + "\n";
            } else {
                graphicsInfo += "          " + description + "\n";
            }
        }
        else if (path == "/0" || device == "system") {
            systemInfo = "System: " + description + "\n";
        }
    }
    
    if (m_overviewContent) {
        m_overviewContent->setText(systemInfo.trimmed());
    }
    if (m_cpuContent) {
        m_cpuContent->setText(cpuInfo.trimmed());
    }
    if (m_memoryContent) {
        m_memoryContent->setText(memoryInfo.trimmed());
    }
    if (m_storageContent) {
        m_storageContent->setText(storageInfo.trimmed());
    }
    if (m_networkContent) {
        m_networkContent->setText(networkInfo.trimmed());
    }
    if (m_graphicsContent) {
        m_graphicsContent->setText(graphicsInfo.trimmed());
    }
    
    qDebug() << "SummaryTab: parseOutput completed";
}
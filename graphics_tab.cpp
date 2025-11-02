#include "graphics_tab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QGroupBox>
#include <QFont>
#include <QRegularExpression>
#include <QDebug>
#include <QObject>

GraphicsTab::GraphicsTab(QWidget* parent)
    : TabWidgetBase(QObject::tr("Graphics card"), "lshw -C display -short", true, 
                    "lshw -C display && lspci | grep VGA && glxinfo | head -20 2>/dev/null", parent)
{
    qDebug() << "GraphicsTab: Constructor called - base constructor done";
    initializeTab();
    qDebug() << "GraphicsTab: Constructor finished";
}

QWidget* GraphicsTab::createUserFriendlyView()
{
    qDebug() << "GraphicsTab: createUserFriendlyView called";
    
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    QWidget* contentWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    QLabel* titleLabel = new QLabel(tr("Graphics Card Information"));
    titleLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  color: #2c3e50;"
        "  margin-bottom: 10px;"
        "}"
    );
    mainLayout->addWidget(titleLabel);
    
    createInfoSection(tr("Graphics Cards"), &m_graphicsCardSection, &m_graphicsCardContent, mainLayout);
    createInfoSection(tr("Graphics Drivers"), &m_driverSection, &m_driverContent, mainLayout);
    createInfoSection(tr("OpenGL Information"), &m_openglSection, &m_openglContent, mainLayout);
    createInfoSection(tr("Video Memory"), &m_memorySection, &m_memoryContent, mainLayout);
    
    mainLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);
    
    qDebug() << "GraphicsTab: createUserFriendlyView completed";
    return scrollArea;
}

void GraphicsTab::createInfoSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout)
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
    *contentLabel = new QLabel(tr("Loading %1 information...").arg(title.toLower()));
    (*contentLabel)->setWordWrap(true);
    (*contentLabel)->setStyleSheet("QLabel { padding: 10px; background-color: #f8f9fa; border-radius: 4px; }");
    sectionLayout->addWidget(*contentLabel);
    
    parentLayout->addWidget(*groupBox);
}

void GraphicsTab::parseOutput(const QString& output)
{
    qDebug() << "GraphicsTab: parseOutput called";
    
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    QString graphicsCardInfo = tr("Graphics Cards: Not detected");
    QString driverInfo = tr("Graphics Drivers: Not detected");
    QString openglInfo = tr("OpenGL: Not detected");
    QString memoryInfo = tr("Video Memory: Not detected");
    
    QStringList graphicsCards;
    QStringList drivers;
    QStringList openglDetails;
    
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        
        // Parse lshw display output
        if (trimmed.contains("display") && !trimmed.startsWith("H/W path")) {
            QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 3) {
                QString description = parts.mid(2).join(" ");
                if (!description.isEmpty()) {
                    graphicsCards.append(description);
                }
            }
        }
        
        // Parse lspci VGA output
        if (trimmed.contains("VGA compatible controller:")) {
            QString cardName = trimmed.split("VGA compatible controller:")[1].trimmed();
            if (!cardName.isEmpty()) {
                graphicsCards.append(cardName);
            }
        }
        
        // Parse driver information
        if (trimmed.contains("driver=") || trimmed.contains("configuration: driver=")) {
            QRegularExpression driverRegex("driver=([^\\s]+)");
            QRegularExpressionMatch match = driverRegex.match(trimmed);
            if (match.hasMatch()) {
                QString driver = match.captured(1);
                drivers.append(driver);
            }
        }
        
        // Parse OpenGL information
        if (trimmed.startsWith("OpenGL vendor string:") || 
            trimmed.startsWith("OpenGL renderer string:") ||
            trimmed.startsWith("OpenGL version string:")) {
            openglDetails.append(trimmed);
        }
        
        // Parse memory information
        if (trimmed.contains("memory") && (trimmed.contains("MB") || trimmed.contains("GB"))) {
            QRegularExpression memRegex("(\\d+\\s*[MG]B)");
            QRegularExpressionMatch match = memRegex.match(trimmed);
            if (match.hasMatch()) {
                memoryInfo = tr("Video Memory: %1").arg(match.captured(1));
            }
        }
        
        // Look for NVIDIA/AMD specific information
        if (trimmed.contains("NVIDIA") || trimmed.contains("GeForce") || trimmed.contains("Quadro")) {
            drivers.append(tr("NVIDIA proprietary driver (likely)"));
        } else if (trimmed.contains("AMD") || trimmed.contains("Radeon") || trimmed.contains("ATI")) {
            drivers.append(tr("AMD/ATI driver (AMDGPU or Radeon)"));
        } else if (trimmed.contains("Intel")) {
            drivers.append(tr("Intel integrated graphics driver"));
        }
    }
    
    // Remove duplicates and format information
    graphicsCards.removeDuplicates();
    drivers.removeDuplicates();
    
    if (!graphicsCards.isEmpty()) {
        graphicsCardInfo = tr("Graphics Cards:\n%1").arg(graphicsCards.join("\n"));
    }

    if (!drivers.isEmpty()) {
        driverInfo = tr("Graphics Drivers:\n%1").arg(drivers.join("\n"));
    }

    if (!openglDetails.isEmpty()) {
        openglInfo = tr("OpenGL Information:\n%1").arg(openglDetails.join("\n"));
    }
    
    // Update the UI with parsed information
    if (m_graphicsCardContent) {
        m_graphicsCardContent->setText(graphicsCardInfo);
    }
    if (m_driverContent) {
        m_driverContent->setText(driverInfo);
    }
    if (m_openglContent) {
        m_openglContent->setText(openglInfo);
    }
    if (m_memoryContent) {
        m_memoryContent->setText(memoryInfo);
    }
    
    qDebug() << "GraphicsTab: parseOutput completed";
}
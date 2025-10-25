#include "screen_tab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QGroupBox>
#include <QFont>
#include <QRegularExpression>
#include <QDebug>

ScreenTab::ScreenTab(QWidget* parent)
    : TabWidgetBase("Screen", "xrandr --query 2>/dev/null || echo 'Display info not available'", true, 
                    "xrandr --verbose 2>/dev/null && xdpyinfo 2>/dev/null", parent)
{
    qDebug() << "ScreenTab: Constructor called - base constructor done";
    initializeTab();
    qDebug() << "ScreenTab: Constructor finished";
}

QWidget* ScreenTab::createUserFriendlyView()
{
    qDebug() << "ScreenTab: createUserFriendlyView called";
    
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    QWidget* contentWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    QLabel* titleLabel = new QLabel("Display and Monitor Information");
    titleLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  color: #2c3e50;"
        "  margin-bottom: 10px;"
        "}"
    );
    mainLayout->addWidget(titleLabel);
    
    createInfoSection("Connected Displays", &m_displaysSection, &m_displaysContent, mainLayout);
    createInfoSection("Screen Resolution", &m_resolutionSection, &m_resolutionContent, mainLayout);
    createInfoSection("Refresh Rates", &m_refreshRateSection, &m_refreshRateContent, mainLayout);
    createInfoSection("Display Orientation", &m_orientationSection, &m_orientationContent, mainLayout);
    
    mainLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);
    
    qDebug() << "ScreenTab: createUserFriendlyView completed";
    return scrollArea;
}

void ScreenTab::createInfoSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout)
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

void ScreenTab::parseOutput(const QString& output)
{
    qDebug() << "ScreenTab: parseOutput called";
    
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    QString displaysInfo = "Connected Displays: Not detected";
    QString resolutionInfo = "Screen Resolution: Not detected";
    QString refreshRateInfo = "Refresh Rates: Not detected";
    QString orientationInfo = "Display Orientation: Not detected";
    
    QStringList displays;
    QStringList resolutions;
    QStringList refreshRates;
    QStringList orientations;
    
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        
        // Parse xrandr output for connected displays
        if (trimmed.contains(" connected") && !trimmed.startsWith("Screen")) {
            QStringList parts = trimmed.split(' ');
            if (!parts.isEmpty()) {
                QString displayName = parts[0];
                displays.append(displayName);
                
                // Extract resolution from the connected line
                QRegularExpression resRegex("(\\d+x\\d+)");
                QRegularExpressionMatch resMatch = resRegex.match(trimmed);
                if (resMatch.hasMatch()) {
                    QString resolution = resMatch.captured(1);
                    resolutions.append(displayName + ": " + resolution);
                }
                
                // Extract refresh rate
                QRegularExpression refreshRegex("(\\d+\\.\\d+)\\*");
                QRegularExpressionMatch refreshMatch = refreshRegex.match(trimmed);
                if (refreshMatch.hasMatch()) {
                    QString refresh = refreshMatch.captured(1);
                    refreshRates.append(displayName + ": " + refresh + " Hz");
                }
                
                // Extract orientation
                if (trimmed.contains("normal")) {
                    orientations.append(displayName + ": Normal");
                } else if (trimmed.contains("left")) {
                    orientations.append(displayName + ": Rotated Left");
                } else if (trimmed.contains("right")) {
                    orientations.append(displayName + ": Rotated Right");
                } else if (trimmed.contains("inverted")) {
                    orientations.append(displayName + ": Inverted");
                }
            }
        }
        
        // Parse Screen information from xrandr
        if (trimmed.startsWith("Screen") && trimmed.contains("current")) {
            QRegularExpression screenRegex("current (\\d+ x \\d+)");
            QRegularExpressionMatch screenMatch = screenRegex.match(trimmed);
            if (screenMatch.hasMatch()) {
                QString screenRes = screenMatch.captured(1);
                resolutions.append("Total Screen: " + screenRes);
            }
        }
        
        // Parse mode lines for additional resolutions
        if (trimmed.contains("x") && trimmed.contains(".") && !trimmed.contains("connected")) {
            QRegularExpression modeRegex("^\\s*(\\d+x\\d+)\\s+([\\d\\.]+)");
            QRegularExpressionMatch modeMatch = modeRegex.match(trimmed);
            if (modeMatch.hasMatch()) {
                QString mode = modeMatch.captured(1);
                QString rate = modeMatch.captured(2);
                
                // Check if this is the current mode (marked with *)
                if (trimmed.contains("*")) {
                    refreshRates.append("Current: " + mode + " @ " + rate + " Hz");
                }
            }
        }
        
        // Parse xdpyinfo output
        if (trimmed.startsWith("dimensions:")) {
            QString dimensions = trimmed.split(":")[1].trimmed();
            resolutions.append("Physical: " + dimensions);
        }
        
        if (trimmed.startsWith("resolution:")) {
            QString dpi = trimmed.split(":")[1].trimmed();
            resolutions.append("DPI: " + dpi);
        }
    }
    
    // Remove duplicates and format information
    displays.removeDuplicates();
    resolutions.removeDuplicates();
    refreshRates.removeDuplicates();
    orientations.removeDuplicates();
    
    if (!displays.isEmpty()) {
        displaysInfo = "Connected Displays:\n" + displays.join("\n");
    }
    
    if (!resolutions.isEmpty()) {
        resolutionInfo = "Screen Resolution:\n" + resolutions.join("\n");
    }
    
    if (!refreshRates.isEmpty()) {
        refreshRateInfo = "Refresh Rates:\n" + refreshRates.join("\n");
    }
    
    if (!orientations.isEmpty()) {
        orientationInfo = "Display Orientation:\n" + orientations.join("\n");
    } else {
        orientationInfo = "Display Orientation:\nNormal (default)";
    }
    
    // Update the UI with parsed information
    if (m_displaysContent) {
        m_displaysContent->setText(displaysInfo);
    }
    if (m_resolutionContent) {
        m_resolutionContent->setText(resolutionInfo);
    }
    if (m_refreshRateContent) {
        m_refreshRateContent->setText(refreshRateInfo);
    }
    if (m_orientationContent) {
        m_orientationContent->setText(orientationInfo);
    }
    
    qDebug() << "ScreenTab: parseOutput completed";
}
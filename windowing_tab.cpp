#include "windowing_tab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QGroupBox>
#include <QFont>
#include <QRegularExpression>
#include <QDebug>

WindowingTab::WindowingTab(QWidget* parent)
    : TabWidgetBase("Windowing environment", "echo $XDG_CURRENT_DESKTOP && echo $DESKTOP_SESSION && echo $XDG_SESSION_TYPE", true, 
                    "env | grep -E '(DESKTOP|XDG|WAYLAND|X11)' | sort", parent)
{
    qDebug() << "WindowingTab: Constructor called - base constructor done";
    initializeTab();
    qDebug() << "WindowingTab: Constructor finished";
}

QWidget* WindowingTab::createUserFriendlyView()
{
    qDebug() << "WindowingTab: createUserFriendlyView called";
    
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    QWidget* contentWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    QLabel* titleLabel = new QLabel("Windowing Environment Information");
    titleLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  color: #2c3e50;"
        "  margin-bottom: 10px;"
        "}"
    );
    mainLayout->addWidget(titleLabel);
    
    createInfoSection("Desktop Environment", &m_desktopSection, &m_desktopContent, mainLayout);
    createInfoSection("Session Type", &m_sessionSection, &m_sessionContent, mainLayout);
    createInfoSection("Display Server", &m_displayServerSection, &m_displayServerContent, mainLayout);
    createInfoSection("Window Manager", &m_windowManagerSection, &m_windowManagerContent, mainLayout);
    
    mainLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);
    
    qDebug() << "WindowingTab: createUserFriendlyView completed";
    return scrollArea;
}

void WindowingTab::createInfoSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout)
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

void WindowingTab::parseOutput(const QString& output)
{
    qDebug() << "WindowingTab: parseOutput called";
    
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    QString desktopInfo = "Desktop Environment: Not detected";
    QString sessionInfo = "Session: Not detected";
    QString displayServerInfo = "Display Server: Not detected";
    QString windowManagerInfo = "Window Manager: Not detected";
    
    // Simple parsing - the output should contain desktop, session, and session type
    for (int i = 0; i < lines.size(); ++i) {
        QString trimmed = lines[i].trimmed();
        
        if (i == 0 && !trimmed.isEmpty()) {
            // First line should be XDG_CURRENT_DESKTOP
            desktopInfo = "Desktop Environment: " + trimmed;
        } else if (i == 1 && !trimmed.isEmpty()) {
            // Second line should be DESKTOP_SESSION
            sessionInfo = "Session: " + trimmed;
        } else if (i == 2 && !trimmed.isEmpty()) {
            // Third line should be XDG_SESSION_TYPE
            if (trimmed.contains("wayland")) {
                displayServerInfo = "Display Server: Wayland";
            } else if (trimmed.contains("x11")) {
                displayServerInfo = "Display Server: X11";
            } else {
                displayServerInfo = "Display Server: " + trimmed;
            }
        }
        
        // Look for window manager info in environment variables
        if (trimmed.contains("WINDOW_MANAGER=") || trimmed.contains("WM_NAME=")) {
            QString wm = trimmed.split('=')[1];
            windowManagerInfo = "Window Manager: " + wm;
        }
    }
    
    // If we can detect common desktop environments, also set likely window manager
    if (desktopInfo.contains("GNOME")) {
        windowManagerInfo = "Window Manager: Mutter (GNOME)";
    } else if (desktopInfo.contains("KDE")) {
        windowManagerInfo = "Window Manager: KWin (KDE)";
    } else if (desktopInfo.contains("XFCE")) {
        windowManagerInfo = "Window Manager: Xfwm4 (XFCE)";
    } else if (desktopInfo.contains("MATE")) {
        windowManagerInfo = "Window Manager: Marco (MATE)";
    } else if (desktopInfo.contains("Cinnamon")) {
        windowManagerInfo = "Window Manager: Muffin (Cinnamon)";
    } else if (desktopInfo.contains("LXDE")) {
        windowManagerInfo = "Window Manager: Openbox (LXDE)";
    }
    
    // Update the UI with parsed information
    if (m_desktopContent) {
        m_desktopContent->setText(desktopInfo);
    }
    if (m_sessionContent) {
        m_sessionContent->setText(sessionInfo);
    }
    if (m_displayServerContent) {
        m_displayServerContent->setText(displayServerInfo);
    }
    if (m_windowManagerContent) {
        m_windowManagerContent->setText(windowManagerInfo);
    }
    
    qDebug() << "WindowingTab: parseOutput completed";
}
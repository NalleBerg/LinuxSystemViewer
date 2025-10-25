#include "about_tab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QGroupBox>
#include <QFont>
#include <QTextEdit>
#include <QPixmap>
#include <QDebug>

AboutTab::AboutTab(QWidget* parent)
    : TabWidgetBase("About", "echo 'Linux System Viewer (LSV) v0.6.0'", false, "", parent)
{
    qDebug() << "AboutTab: Constructor called - base constructor done";
    initializeTab();
    qDebug() << "AboutTab: Constructor finished";
}

QWidget* AboutTab::createUserFriendlyView()
{
    qDebug() << "AboutTab: createUserFriendlyView called";
    
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    QWidget* contentWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    
    // Application logo and title
    QLabel* logoLabel = new QLabel();
    QPixmap logo(":/lsv-512.png");
    if (!logo.isNull()) {
        logoLabel->setPixmap(logo.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        logoLabel->setText("LSV");
        logoLabel->setStyleSheet(
            "QLabel {"
            "  font-size: 48px;"
            "  font-weight: bold;"
            "  color: #2c3e50;"
            "  background-color: #ecf0f1;"
            "  border-radius: 64px;"
            "  min-width: 128px;"
            "  min-height: 128px;"
            "}"
        );
    }
    logoLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(logoLabel, 0, Qt::AlignCenter);
    
    QLabel* titleLabel = new QLabel("Linux System Viewer (LSV)");
    titleLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 24px;"
        "  font-weight: bold;"
        "  color: #2c3e50;"
        "  margin: 10px 0;"
        "}"
    );
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    createInfoSection("Application", &m_applicationSection, &m_applicationContent, mainLayout);
    createInfoSection("Version", &m_versionSection, &m_versionContent, mainLayout);
    createInfoSection("Authors", &m_authorsSection, &m_authorsContent, mainLayout);
    createInfoSection("License", &m_licenseSection, &m_licenseContent, mainLayout);
    
    mainLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);
    
    qDebug() << "AboutTab: createUserFriendlyView completed";
    return scrollArea;
}

void AboutTab::createInfoSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout)
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
    *contentLabel = new QLabel();
    (*contentLabel)->setWordWrap(true);
    (*contentLabel)->setStyleSheet("QLabel { padding: 15px; background-color: #f8f9fa; border-radius: 4px; line-height: 1.4; }");
    sectionLayout->addWidget(*contentLabel);
    
    parentLayout->addWidget(*groupBox);
}

void AboutTab::parseOutput(const QString& output)
{
    qDebug() << "AboutTab: parseOutput called";
    
    // Static information for About tab
    QString applicationInfo = 
        "Linux System Viewer (LSV) is a comprehensive system information tool "
        "designed to provide detailed insights into your Linux system hardware "
        "and software configuration.\n\n"
        "LSV presents system information in an intuitive, easy-to-read format "
        "with both user-friendly and technical (geek mode) views for different "
        "levels of detail.";
    
    QString versionInfo = 
        "Version: 0.6.0\n"
        "Build Date: October 2025\n"
        "Qt Version: " + QString(QT_VERSION_STR) + "\n"
        "Platform: Linux";
    
    QString authorsInfo = 
        "Developer:  Berg\n"
        "Project: Linux System Viewer\n\n"
        "Built with Qt6 and modern C++ for optimal performance "
        "and cross-platform compatibility.\n\n"
        "Special thanks to the open-source community and the "
        "developers of lshw, lscpu, and other system utilities "
        "that make this application possible.";
    
    // Show a simple clickable license link (rendered as HTML)
    QString licenseInfo =
        "<a href=\"https://www.gnu.org/licenses/old-licenses/gpl-2.0.html\">GPL V2</a>";
    
    // Update the UI with static information
    if (m_applicationContent) {
        m_applicationContent->setText(applicationInfo);
    }
    if (m_versionContent) {
        m_versionContent->setText(versionInfo);
    }
    if (m_authorsContent) {
        m_authorsContent->setText(authorsInfo);
    }
    if (m_licenseContent) {
        // Use rich text and allow the user to open the link in an external browser
        m_licenseContent->setTextFormat(Qt::RichText);
        m_licenseContent->setTextInteractionFlags(Qt::TextBrowserInteraction);
        m_licenseContent->setOpenExternalLinks(true);
        m_licenseContent->setText(licenseInfo);
    }
    
    qDebug() << "AboutTab: parseOutput completed";
}
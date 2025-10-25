#include "os_tab.h"
#include <QDebug>
#include <QVBoxLayout>
#include <QLabel>
#include "log_helper.h"
#include <QTableWidget>
#include <QHeaderView>
#include <QScrollArea>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QDir>
#include <sys/utsname.h>

OSTab::OSTab(const QString& tabName, const QString& command, bool showHeader, const QString& headerText, QWidget* parent)
    : TabWidgetBase(tabName, QString(), showHeader, headerText, parent)
{
    appendLog("OSTab: constructor start");
    // Build the content in a separate widget instead of modifying the TabWidgetBase's
    // main layout. This avoids adding 'this' (the TabWidgetBase) into its own stacked widget
    // which can cause reparenting issues during construction.
    QWidget* contentWidget = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0,0,0,0);

    QLabel* headline = new QLabel("Operating System");
    headline->setStyleSheet("font-size: 15px; font-weight: bold; color: #222; margin-bottom: 2px;");
    contentLayout->addWidget(headline);

    tableWidget = new QTableWidget();
    tableWidget->setColumnCount(2);
    tableWidget->setHorizontalHeaderLabels(QStringList() << "Property" << "Value");
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "  background-color: #34495e;"
        "  color: white;"
        "  font-weight: bold;"
        "  padding: 8px;"
        "  border: 1px solid #2c3e50;"
        "}"
    );
    tableWidget->setStyleSheet(
        "QTableWidget {"
        "  gridline-color: #bdc3c7;"
        "  selection-background-color: #3498db;"
        "  alternate-background-color: #f8f9fa;"
        "}"
        "QTableWidget::item {"
        "  padding: 8px;"
        "  border-bottom: 1px solid #ecf0f1;"
        "}"
    );
    tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    tableWidget->setColumnWidth(0, 250);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
    tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->setAlternatingRowColors(true);

    // Make table scrollable
    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidget(tableWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(250);
    contentLayout->addWidget(scrollArea);

    // Initialize the UI widgets (but avoid starting TabWidgetBase's executeCommand which
    // launches a shell process). Add the user-friendly widget directly to the stacked widget.
    appendLog("OSTab: adding content widget to stacked widget");
    if (m_stackedWidget) {
        m_stackedWidget->addWidget(contentWidget);
        appendLog("OSTab: content widget added to stacked widget");
    } else {
        appendLog("OSTab: m_stackedWidget is null");
    }

    // Populate OS info directly from system files to avoid relying on external binaries
    QString osOutput;
    QFile osReleaseFile("/etc/os-release");
    appendLog("OSTab: reading /etc/os-release");
    if (osReleaseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&osReleaseFile);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.contains('=')) {
                QString k = line.section('=', 0, 0).trimmed();
                QString v = line.section('=', 1).trimmed();
                if (v.startsWith('"') && v.endsWith('"')) v = v.mid(1, v.size()-2);
                osOutput += QString("%1: %2\n").arg(k, v);
            }
        }
        osReleaseFile.close();
        appendLog("OSTab: Read /etc/os-release for OS info");
    } else {
        appendLog("OSTab: /etc/os-release not available");
    }

    // Add uname info using uname(2) syscall (avoid launching external 'uname')
    auto getUnameString = []() -> QString {
        struct utsname u;
        if (uname(&u) == 0) {
            // Format similar to `uname -a`: sysname nodename release version machine
            return QString("%1 %2 %3 %4 %5")
                .arg(QString::fromLocal8Bit(u.sysname))
                .arg(QString::fromLocal8Bit(u.nodename))
                .arg(QString::fromLocal8Bit(u.release))
                .arg(QString::fromLocal8Bit(u.version))
                .arg(QString::fromLocal8Bit(u.machine));
        }
        // Fallback: try /proc/version
        QFile f("/proc/version");
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray data = f.readAll().trimmed();
            return QString::fromLocal8Bit(data);
        }
        return QString();
    };

    appendLog("OSTab: calling getUnameString");
    QString unameOut = getUnameString();
    appendLog(QString("OSTab: getUnameString returned length %1").arg(unameOut.size()));
    if (!unameOut.isEmpty()) {
        osOutput += QString("uname: %1\n").arg(unameOut);
        appendLog("OSTab: Added uname() output");
    } else {
        appendLog("OSTab: uname() and /proc/version both unavailable");
    }

    if (!osOutput.isEmpty()) {
        parseOutput(osOutput);
    }

    // Show the populated user-friendly view (set stacked widget to the content widget)
    if (m_stackedWidget) {
        m_stackedWidget->setCurrentWidget(contentWidget);
    }
}

QWidget* OSTab::createUserFriendlyView()
{
    return this;
}

void OSTab::parseOutput(const QString& output)
{
    appendLog(QString("OSTab: parseOutput called, output length: %1").arg(output.size()));
    if (output.trimmed().isEmpty()) {
        appendLog("OSTab: output empty; nothing to display");
    } else {
        appendLog("OSTab: filling table with output");
        fillTableWithOutput(output);
    }
}

void OSTab::fillTableWithOutput(const QString& output)
{
    tableWidget->setRowCount(0);

    QStringList lines = output.split('\n');
    int row = 0;
    for (const QString& line : lines) {
        if (line.contains(":")) {
            QStringList parts = line.split(':');
            if (parts.size() == 2) {
                tableWidget->insertRow(row);
                QTableWidgetItem* propItem = new QTableWidgetItem(parts[0].trimmed());
                QFont boldFont;
                boldFont.setBold(true);
                propItem->setFont(boldFont);
                propItem->setForeground(QColor("#000000"));
                tableWidget->setItem(row, 0, propItem);
                QTableWidgetItem* valItem = new QTableWidgetItem(parts[1].trimmed());
                valItem->setForeground(QColor("#1f1971"));
                tableWidget->setItem(row, 1, valItem);
                tableWidget->resizeRowToContents(row);
                row++;
            }
        }
    }
}
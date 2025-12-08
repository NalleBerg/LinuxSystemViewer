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
#include <QCoreApplication>
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QClipboard>
#include <QGuiApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QShowEvent>
#include <QHideEvent>
#include "gui_helpers.h"
#include "geek_search_integration.h"

OSTab::OSTab(const QString& tabName, const QString& command, bool showHeader, const QString& headerText, QWidget* parent)
    : TabWidgetBase(tabName, QString(), showHeader, headerText, parent)
{
    appendLog("OSTab: constructor start");
    // Build the content in a separate widget instead of modifying the TabWidgetBase's
    // main layout. This avoids adding 'this' (the TabWidgetBase) into its own stacked widget
    // which can cause reparenting issues during construction.
    QWidget* contentWidget = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    applyMainLayoutDefaults(contentLayout);  // Apply exact CPU tab layout defaults

    // Headline and Geek button (use helper to guarantee identical placement)
    QPushButton* gb = nullptr;
    QHBoxLayout* headlineLayout = createHeadlineWithGeek(contentWidget, QCoreApplication::translate("OSTab", "Operating System"), &gb);
    geekButton = gb;
    connect(geekButton, &QPushButton::clicked, this, &OSTab::showGeekMode);
    contentLayout->addLayout(headlineLayout);

    tableWidget = new QTableWidget();
    tableWidget->setColumnCount(2);
    tableWidget->setHorizontalHeaderLabels(QStringList() << QCoreApplication::translate("OSTab", "Property") << QCoreApplication::translate("OSTab", "Value"));
    tableWidget->verticalHeader()->setVisible(false);
    
    // Apply exact CPU tab styling
    tableWidget->setColumnWidth(0, 220);  // Property
    tableWidget->setColumnWidth(1, 300);  // Value
    
    // Style headers exactly like CPU tab
    tableWidget->horizontalHeader()->setStyleSheet(
        "QHeaderView::section { "
        "background-color: #2c3e50; "
        "color: white; "
        "padding: 8px; "
        "border: none; "
        "font-weight: bold; "
        "}"
    );
    
    tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
    tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Scroll area with exact CPU tab settings
    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidget(tableWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(220);  // Same as CPU tab
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
                
                // Translate standard os-release property names for normal users
                QString translatedKey = k;
                if (k == "NAME") {
                    translatedKey = tr("Distribution Name");
                } else if (k == "VERSION") {
                    translatedKey = tr("Distribution Version");
                } else if (k == "ID") {
                    translatedKey = tr("Distribution ID");
                } else if (k == "VERSION_ID") {
                    translatedKey = tr("Version ID");
                } else if (k == "PRETTY_NAME") {
                    translatedKey = tr("Full Name");
                } else if (k == "VERSION_CODENAME") {
                    translatedKey = tr("Version Codename");
                } else if (k == "UBUNTU_CODENAME") {
                    translatedKey = tr("Ubuntu Codename");
                } else if (k == "HOME_URL") {
                    translatedKey = tr("Home Page");
                } else if (k == "SUPPORT_URL") {
                    translatedKey = tr("Support Page");
                } else if (k == "BUG_REPORT_URL") {
                    translatedKey = tr("Bug Report Page");
                } else if (k == "PRIVACY_POLICY_URL") {
                    translatedKey = tr("Privacy Policy");
                } else if (k == "BUILD_ID") {
                    translatedKey = tr("Build ID");
                } else if (k == "VARIANT") {
                    translatedKey = tr("Variant");
                } else if (k == "VARIANT_ID") {
                    translatedKey = tr("Variant ID");
                }
                
                osOutput += QString("%1: %2\n").arg(translatedKey, v);
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
    osOutput += QCoreApplication::translate("OSTab", "uname: %1\n").arg(unameOut);
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

void OSTab::showGeekMode()
{
    GeekOsDialog dlg(this);
    dlg.exec();
}

// --- GeekOsDialog ---

GeekOsDialog::GeekOsDialog(QWidget* parent)
    : QDialog(parent)
    , refreshTimer(new QTimer(this))
{
    setWindowTitle(tr("OS - Geek Mode"));
    setModal(true);
    resize(700, 500);

    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* titleLabel = new QLabel(tr("OS Technical Details"));
    titleLabel->setStyleSheet("font-size:16px; font-weight:bold; color:#2c3e50; margin-bottom:10px;");
    layout->addWidget(titleLabel);

    table = new QTableWidget();
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList() << tr("Property") << tr("Value"));
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #34495e; color: white; font-weight: bold; padding: 8px; border: 1px solid #2c3e50; }");
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidget(table);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(400);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setContentsMargins(0, 0, 0, 5);  // Add bottom margin
    layout->addWidget(scrollArea);

    // Button layout: Search on left, Copy/Save/Close on right
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    // Add search button using the integration helper
    GeekSearchIntegration::addSearchButtonToGeekDialog(buttonLayout, this, table);
    
    buttonLayout->addStretch();
    
    // Buttons: Copy, Save, Close
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    QPushButton* copyBtn = new QPushButton(tr("Copy"));
    QPushButton* saveBtn = new QPushButton(tr("Save..."));
    buttonBox->addButton(copyBtn, QDialogButtonBox::ActionRole);
    buttonBox->addButton(saveBtn, QDialogButtonBox::ActionRole);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    buttonLayout->addWidget(buttonBox);
    layout->addLayout(buttonLayout);
    
    // Fix Close button translation - apply after dialog is shown
    QTimer::singleShot(0, [buttonBox, this]() {
        if (auto closeBtn = buttonBox->button(QDialogButtonBox::Close)) {
            closeBtn->setText(tr("Close"));
        }
    });

    // Enable copy on main geek table (right-click + Ctrl+C)
    enableTableCopy(table, refreshTimer);

    connect(copyBtn, &QPushButton::clicked, [this]() {
        // Human-readable UTF-8 copy: Property: Value per line
        QString all;
        for (int r = 0; r < table->rowCount(); ++r) {
            QString prop = table->item(r,0) ? table->item(r,0)->text() : QString();
            QString val = table->item(r,1) ? table->item(r,1)->text() : QString();
            all += prop + ": " + val + "\n";
        }
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(all, QClipboard::Clipboard);
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("Copied"));
        msgBox.setText(tr("The information has been copied\nto the clipboard."));
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    });

    connect(saveBtn, &QPushButton::clicked, [this]() {
        // Save as CSV (UTF-8)
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save OS Info"), "os-info.csv", tr("CSV Files (*.csv);;All Files (*)"));
        if (fileName.isEmpty()) return;

        auto esc = [](const QString &s)->QString {
            QString out = s;
            out.replace('"', "\"\"");
            if (out.contains(',') || out.contains('\n') || out.contains('"')) {
                out = '"' + out + '"';
            }
            return out;
        };

        QFile out(fileName);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) return;
        QTextStream ts(&out);
        ts << "Property,Value\n";
        for (int r = 0; r < table->rowCount(); ++r) {
            QString prop = table->item(r,0) ? table->item(r,0)->text() : QString();
            QString val = table->item(r,1) ? table->item(r,1)->text() : QString();
            ts << esc(prop) << ',' << esc(val) << '\n';
        }
        out.close();
    });

    fillTable();

    // Auto-refresh every second while visible
    refreshTimer->setInterval(1000);
    connect(refreshTimer, &QTimer::timeout, this, &GeekOsDialog::fillTable);
}

void GeekOsDialog::showEvent(QShowEvent* ev)
{
    QDialog::showEvent(ev);
    if (refreshTimer) refreshTimer->start();
}

void GeekOsDialog::hideEvent(QHideEvent* ev)
{
    if (refreshTimer) refreshTimer->stop();
    QDialog::hideEvent(ev);
}

void GeekOsDialog::fillTable()
{
    table->setRowCount(0);

    auto addRow = [&](const QString& prop, const QString& val) {
        int row = table->rowCount();
        table->insertRow(row);
        QTableWidgetItem* propItem = new QTableWidgetItem(prop);
        QFont boldFont;
        boldFont.setBold(true);
        propItem->setFont(boldFont);
        table->setItem(row, 0, propItem);
        table->setItem(row, 1, new QTableWidgetItem(val));
    };
    
    auto addSection = [&](const QString& title) {
        int row = table->rowCount();
        table->insertRow(row);
        QTableWidgetItem* sectionItem = new QTableWidgetItem(title);
        sectionItem->setBackground(QBrush(QColor("#ecf0f1")));
        QFont boldFont = sectionItem->font();
        boldFont.setBold(true);
        sectionItem->setFont(boldFont);
        table->setItem(row, 0, sectionItem);
        table->setItem(row, 1, new QTableWidgetItem(""));
    };

    // Read /etc/os-release
    addSection("=== OS Release ===");
    QFile osRelease("/etc/os-release");
    if (osRelease.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&osRelease);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.contains('=')) {
                QString k = line.section('=', 0, 0).trimmed();
                QString v = line.section('=', 1).trimmed();
                if (v.startsWith('"') && v.endsWith('"')) v = v.mid(1, v.size()-2);
                addRow(k, v);
            }
        }
        osRelease.close();
    }

    // Get uname info
    addSection("=== System Information ===");
    struct utsname unameData;
    if (uname(&unameData) == 0) {
        addRow("System Name", unameData.sysname);
        addRow("Node Name", unameData.nodename);
        addRow("Release", unameData.release);
        addRow("Version", unameData.version);
        addRow("Machine", unameData.machine);
    }

    // Read /proc/version
    addSection("=== Kernel Version ===");
    QFile procVersion("/proc/version");
    if (procVersion.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString version = procVersion.readAll().trimmed();
        addRow("Kernel Version", version);
        procVersion.close();
    }

    // Read /proc/cmdline
    addSection("=== Kernel Command Line ===");
    QFile cmdline("/proc/cmdline");
    if (cmdline.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString cmd = cmdline.readAll().trimmed();
        // Split command line into individual parameters for better searching
        QStringList params = cmd.split(' ', Qt::SkipEmptyParts);
        for (const QString& param : params) {
            if (param.contains('=')) {
                QString key = param.section('=', 0, 0);
                QString value = param.section('=', 1);
                addRow(key, value);
            } else {
                addRow("Parameter", param);
            }
        }
        cmdline.close();
    }

    // Get environment variables
    addSection("=== Environment Variables ===");
    QStringList env = QProcess::systemEnvironment();
    for (const QString& e : env) {
        if (e.contains('=')) {
            QString key = e.section('=', 0, 0);
            QString value = e.section('=', 1);
            addRow(key, value);
        }
    }

    table->resizeColumnToContents(0);
}
#include "network_geek.h"
#include "network.h"
#include "geek_search_integration.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QProcess>
#include <QGuiApplication>
#include <QCoreApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QLabel>
#include <QFont>
#include <QScrollArea>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QMessageBox>
#include "gui_helpers.h"
#include <QShowEvent>
#include <QHideEvent>

NetworkGeekDialog::NetworkGeekDialog(QWidget* parent)
    : QDialog(parent), table(new QTableWidget(this)), timer(new QTimer(this))
{
    setWindowTitle(QCoreApplication::translate("NetworkGeekDialog", "Network - Geek Mode"));
    setModal(true);
    resize(700, 500);

    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* titleLabel = new QLabel(QCoreApplication::translate("NetworkGeekDialog", "Network Technical Details"));
    titleLabel->setStyleSheet("font-size:16px; font-weight:bold; color:#2c3e50; margin-bottom:10px;");
    layout->addWidget(titleLabel);

    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList() << QCoreApplication::translate("NetworkGeekDialog", "Property") << QCoreApplication::translate("NetworkGeekDialog", "Value"));
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #34495e; color: white; font-weight: bold; padding: 8px; border: 1px solid #2c3e50; }");
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidget(table);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(350);
    layout->addWidget(scrollArea);

    // Buttons: Search (left) - stretch - Copy, Save, Close (right)
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    GeekSearchIntegration::addSearchButtonToGeekDialog(buttonLayout, this, table);
    buttonLayout->addStretch();
    
    QPushButton* copyBtn = new QPushButton(QCoreApplication::translate("NetworkGeekDialog", "Copy"));
    QPushButton* saveBtn = new QPushButton(QCoreApplication::translate("NetworkGeekDialog", "Save..."));
    QPushButton* closeBtn = new QPushButton(QCoreApplication::translate("NetworkGeekDialog", "Close"));
    
    buttonLayout->addWidget(copyBtn);
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(closeBtn);
    
    layout->addLayout(buttonLayout);

    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);

    // Enable copy on main geek table (right-click + Ctrl+C) and pause the geek dialog's refresh while copying
    enableTableCopy(table, timer);

    connect(copyBtn, &QPushButton::clicked, this, &NetworkGeekDialog::copyToClipboard);
    connect(saveBtn, &QPushButton::clicked, this, &NetworkGeekDialog::saveToFile);

    connect(timer, &QTimer::timeout, this, &NetworkGeekDialog::refresh);
    timer->start(3000);

    fillTable();
}

void NetworkGeekDialog::fillTable()
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
    
    // Get network info from ip command
    QProcess ipAddr;
    ipAddr.start("ip", QStringList() << "addr");
    ipAddr.waitForFinished(3000);
    QString ipAddrOutput = QString::fromLocal8Bit(ipAddr.readAllStandardOutput());
    
    if (!ipAddrOutput.isEmpty()) {
        addSection("=== Network Interfaces (ip addr) ===");
        QStringList lines = ipAddrOutput.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.isEmpty()) continue;
            
            // Interface line: "2: wlp2s0: <BROADCAST,MULTICAST,UP,LOWER_UP>"
            if (QRegularExpression("^\\d+:").match(trimmed).hasMatch()) {
                QStringList parts = trimmed.split(':', Qt::SkipEmptyParts);
                if (parts.size() >= 2) {
                    addRow("Interface", parts[1].trimmed() + (parts.size() > 2 ? " " + parts[2].trimmed() : ""));
                }
            }
            // inet line: "inet 192.168.1.105/24"
            else if (trimmed.startsWith("inet ")) {
                addRow("IPv4 Address", trimmed.mid(5));
            }
            // inet6 line
            else if (trimmed.startsWith("inet6 ")) {
                addRow("IPv6 Address", trimmed.mid(6));
            }
            // link/ether line
            else if (trimmed.startsWith("link/")) {
                addRow("Link Info", trimmed.mid(5));
            }
            // Other interface properties
            else if (trimmed.startsWith("valid_lft")) {
                addRow("Validity", trimmed);
            }
        }
    }
    
    // Get routing info
    QProcess ipRoute;
    ipRoute.start("ip", QStringList() << "route");
    ipRoute.waitForFinished(3000);
    QString routeOutput = QString::fromLocal8Bit(ipRoute.readAllStandardOutput());
    
    if (!routeOutput.isEmpty()) {
        addSection("=== Routing Table (ip route) ===");
        QStringList lines = routeOutput.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            if (line.contains("default")) {
                addRow("Default Route", line);
            } else {
                addRow("Route", line);
            }
        }
    }
    
    // Get network statistics
    QFile netDev("/proc/net/dev");
    if (netDev.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addSection("=== Network Statistics (/proc/net/dev) ===");
        QTextStream in(&netDev);
        QString header1 = in.readLine();
        QString header2 = in.readLine();
        
        // Add header
        if (!header2.isEmpty()) {
            addRow("Header", header2.trimmed());
        }
        
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty()) continue;
            
            QStringList parts = line.split(':', Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                QString iface = parts[0].trimmed();
                addRow(iface + " Stats", parts[1].trimmed());
            }
        }
        netDev.close();
    }
    
    // Get routing table from /proc
    QFile netRoute("/proc/net/route");
    if (netRoute.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addSection("=== Routing Table (/proc/net/route) ===");
        QTextStream in(&netRoute);
        QString header = in.readLine();
        
        if (!header.isEmpty()) {
            addRow("Columns", header.trimmed());
        }
        
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty()) continue;
            
            QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (!parts.isEmpty()) {
                addRow(parts[0] + " Route", line);
            }
        }
        netRoute.close();
    }
}

void NetworkGeekDialog::refresh()
{
    fillTable();
}

void NetworkGeekDialog::copyToClipboard()
{
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
    msgBox.setWindowTitle(QCoreApplication::translate("NetworkGeekDialog", "Copied"));
    msgBox.setText(QCoreApplication::translate("NetworkGeekDialog", "The information has been copied\nto the clipboard."));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

void NetworkGeekDialog::saveToFile()
{
    QString fn = QFileDialog::getSaveFileName(this, QCoreApplication::translate("NetworkGeekDialog", "Save network info"), "network-info.csv", QCoreApplication::translate("NetworkGeekDialog", "CSV files (*.csv);;All files (*)"));
    if (fn.isEmpty()) return;
    QFile f(fn);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    QTextStream ts(&f);

    auto esc = [](const QString &s)->QString{
        QString out = s;
        out.replace('"', "\"\"");
        if (out.contains(',') || out.contains('\n') || out.contains('"')) out = '"' + out + '"';
        return out;
    };

    ts << "Property,Value\n";
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem* propItem = table->item(row, 0);
        QTableWidgetItem* valItem = table->item(row, 1);
        if (propItem) {
            ts << esc(propItem->text()) << ',';
            if (valItem) {
                ts << esc(valItem->text());
            }
            ts << '\n';
        }
    }
    f.close();
}

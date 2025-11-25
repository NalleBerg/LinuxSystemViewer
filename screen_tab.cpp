#include "screen_tab.h"
#include "screen.h"
#include "gui_helpers.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QGuiApplication>
#include <QScreen>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QScrollArea>
#include <QFont>
#include <QClipboard>
#include <QMessageBox>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QShowEvent>

ScreenTab::ScreenTab(QWidget* parent)
    : QWidget(parent)
{
    // Headline and Geek button (use helper to guarantee identical placement)
    QPushButton* gb = nullptr;
    QHBoxLayout* headlineLayout = createHeadlineWithGeek(this, tr("Screen"), &gb);
    geekButton = gb;
    connect(geekButton, &QPushButton::clicked, this, &ScreenTab::showGeekMode);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    applyMainLayoutDefaults(mainLayout);
    mainLayout->addLayout(headlineLayout);

    // Table
    tableWidget = new QTableWidget();
    tableWidget->setColumnCount(2);
    tableWidget->setHorizontalHeaderLabels({tr("Property"), tr("Value")});
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->horizontalHeader()->setStyleSheet(
        "QHeaderView::section { background-color: #2c3e50; color: white; "
        "padding: 5px; font-weight: bold; }"
    );
    tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    tableWidget->setAlternatingRowColors(true);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Scroll area
    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidget(tableWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(220);
    mainLayout->addWidget(scrollArea);

    // Enable copy (Ctrl+C and right-click Copy)
    enableTableCopy(tableWidget);

    // Load information
    loadScreenInformation();
}

void ScreenTab::loadScreenInformation()
{
    tableWidget->setRowCount(0);
    
    auto addRow = [this](const QString& property, const QString& value) {
        int row = tableWidget->rowCount();
        tableWidget->insertRow(row);
        
        QTableWidgetItem* propItem = new QTableWidgetItem(property);
        QFont boldFont;
        boldFont.setBold(true);
        propItem->setFont(boldFont);
        tableWidget->setItem(row, 0, propItem);
        
        tableWidget->setItem(row, 1, new QTableWidgetItem(value));
    };
    
    ScreenInfo info = ::loadScreenInformation();
    
    addRow(tr("Resolution"), info.resolution);
    addRow(tr("Refresh Rate"), info.refreshRate);
    addRow(tr("Zoom Level"), QString::number(info.scaleFactor * 100, 'f', 0) + "%");
    addRow(tr("Physical Size"), info.sizeInMm);
    
    // Only show manufacturer if we found one
    if (!info.manufacturer.isEmpty()) {
        addRow(tr("Manufacturer"), info.manufacturer);
    }
    
    addRow(tr("Orientation"), info.orientation);
}

void ScreenTab::showGeekMode()
{
    GeekScreenDialog dlg(this);
    dlg.exec();
}

// ===== Geek Mode Dialog =====

GeekScreenDialog::GeekScreenDialog(QWidget* parent)
    : QDialog(parent)
    , refreshTimer(new QTimer(this))
{
    setWindowTitle(tr("Screen - Geek Mode"));
    setModal(true);
    resize(700, 500);

    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* titleLabel = new QLabel(tr("Screen Technical Details"));
    titleLabel->setStyleSheet("font-size:16px; font-weight:bold; color:#2c3e50; margin-bottom:10px;");
    layout->addWidget(titleLabel);

    table = new QTableWidget();
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList() << tr("Property") << tr("Value"));
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #34495e; color: white; font-weight: bold; padding: 8px; border: 1px solid #2c3e50; }");
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->setWordWrap(true);

    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidget(table);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(350);
    layout->addWidget(scrollArea);

    // Buttons: Copy, Save, Close
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    QPushButton* copyBtn = new QPushButton(tr("Copy"));
    QPushButton* saveBtn = new QPushButton(tr("Save..."));
    buttonBox->addButton(copyBtn, QDialogButtonBox::ActionRole);
    buttonBox->addButton(saveBtn, QDialogButtonBox::ActionRole);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);

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
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save Screen Info"), "screen-info.csv", tr("CSV Files (*.csv);;All Files (*)"));
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
    connect(refreshTimer, &QTimer::timeout, this, &GeekScreenDialog::fillTable);
}

void GeekScreenDialog::showEvent(QShowEvent* ev)
{
    QDialog::showEvent(ev);
    if (refreshTimer) refreshTimer->start();
}

void GeekScreenDialog::hideEvent(QHideEvent* ev)
{
    if (refreshTimer) refreshTimer->stop();
    QDialog::hideEvent(ev);
}

void GeekScreenDialog::fillTable()
{
    table->setRowCount(0);
    int row = 0;
    
    auto addRow = [&](const QString& prop, const QString& val) {
        table->insertRow(row);
        QTableWidgetItem* p = new QTableWidgetItem(prop);
        QFont bold;
        bold.setBold(true);
        p->setFont(bold);
        table->setItem(row, 0, p);
        
        // Replace newlines with arrow for better display
        QString displayVal = val;
        displayVal.replace("\n", " ↳ ");
        
        QTableWidgetItem* v = new QTableWidgetItem(displayVal);
        v->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->setItem(row, 1, v);
        table->resizeRowToContents(row);
        ++row;
    };
    
    // === ALL SCREENS FROM QT ===
    addRow(tr("=== ALL SCREENS ==="), "");
    
    QList<QScreen*> screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); ++i) {
        QScreen* screen = screens[i];
        
        addRow(tr("Screen %1 Name").arg(i), screen->name());
        addRow(tr("Screen %1 Manufacturer").arg(i), screen->manufacturer());
        addRow(tr("Screen %1 Model").arg(i), screen->model());
        addRow(tr("Screen %1 Serial Number").arg(i), screen->serialNumber());
        
        QSize res = screen->size();
        addRow(tr("Screen %1 Resolution").arg(i), QString("%1 x %2").arg(res.width()).arg(res.height()));
        
        QSize virtualRes = screen->virtualSize();
        addRow(tr("Screen %1 Virtual Resolution").arg(i), QString("%1 x %2").arg(virtualRes.width()).arg(virtualRes.height()));
        
        addRow(tr("Screen %1 Refresh Rate").arg(i), QString::number(screen->refreshRate(), 'f', 2) + " Hz");
        
        QSizeF physSize = screen->physicalSize();
        addRow(tr("Screen %1 Physical Size").arg(i), QString("%1 x %2 mm").arg(physSize.width(), 0, 'f', 1).arg(physSize.height(), 0, 'f', 1));
        
        addRow(tr("Screen %1 DPI").arg(i), QString::number(screen->logicalDotsPerInch(), 'f', 1));
        addRow(tr("Screen %1 Physical DPI X").arg(i), QString::number(screen->physicalDotsPerInchX(), 'f', 1));
        addRow(tr("Screen %1 Physical DPI Y").arg(i), QString::number(screen->physicalDotsPerInchY(), 'f', 1));
        
        addRow(tr("Screen %1 Device Pixel Ratio").arg(i), QString::number(screen->devicePixelRatio(), 'f', 2));
        
        Qt::ScreenOrientation orientation = screen->orientation();
        QString orientStr;
        switch (orientation) {
            case Qt::PortraitOrientation: orientStr = "Portrait"; break;
            case Qt::LandscapeOrientation: orientStr = "Landscape"; break;
            case Qt::InvertedPortraitOrientation: orientStr = "Inverted Portrait"; break;
            case Qt::InvertedLandscapeOrientation: orientStr = "Inverted Landscape"; break;
            default: orientStr = "Primary"; break;
        }
        addRow(tr("Screen %1 Orientation").arg(i), orientStr);
        
        QRect geom = screen->geometry();
        addRow(tr("Screen %1 Geometry").arg(i), QString("x=%1, y=%2, w=%3, h=%4").arg(geom.x()).arg(geom.y()).arg(geom.width()).arg(geom.height()));
        
        QRect availGeom = screen->availableGeometry();
        addRow(tr("Screen %1 Available Geometry").arg(i), QString("x=%1, y=%2, w=%3, h=%4").arg(availGeom.x()).arg(availGeom.y()).arg(availGeom.width()).arg(availGeom.height()));
        
        addRow(tr("Screen %1 Depth").arg(i), QString::number(screen->depth()) + " bits");
        
        addRow("", ""); // Spacer
    }
    
    // === DRM CONNECTOR INFORMATION ===
    addRow(tr("=== DRM CONNECTOR INFORMATION ==="), "");
    
    QDir drmDir("/sys/class/drm");
    QStringList connectors = drmDir.entryList(QStringList() << "card*-*", QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QString& connector : connectors) {
        QDir connDir(drmDir.filePath(connector));
        
        addRow(tr("Connector"), connector);
        
        QFile statusFile(connDir.filePath("status"));
        if (statusFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            addRow(tr("%1/status").arg(connector), QTextStream(&statusFile).readLine().trimmed());
            statusFile.close();
        }
        
        QFile enabledFile(connDir.filePath("enabled"));
        if (enabledFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            addRow(tr("%1/enabled").arg(connector), QTextStream(&enabledFile).readLine().trimmed());
            enabledFile.close();
        }
        
        QFile dpmsFile(connDir.filePath("dpms"));
        if (dpmsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            addRow(tr("%1/dpms").arg(connector), QTextStream(&dpmsFile).readLine().trimmed());
            dpmsFile.close();
        }
        
        QFile modesFile(connDir.filePath("modes"));
        if (modesFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString modes = QTextStream(&modesFile).readAll().trimmed();
            modesFile.close();
            addRow(tr("%1/modes").arg(connector), modes.replace("\n", ", "));
        }
        
        // EDID information
        QFile edidFile(connDir.filePath("edid"));
        if (edidFile.open(QIODevice::ReadOnly)) {
            QByteArray edidData = edidFile.readAll();
            edidFile.close();
            
            if (!edidData.isEmpty()) {
                addRow(tr("%1/EDID Size").arg(connector), QString::number(edidData.size()) + " bytes");
                
                // Parse manufacturer ID (bytes 8-9)
                if (edidData.size() >= 18) {
                    unsigned char byte1 = edidData[8];
                    unsigned char byte2 = edidData[9];
                    
                    char letter1 = ((byte1 >> 2) & 0x1F) + 'A' - 1;
                    char letter2 = (((byte1 & 0x03) << 3) | ((byte2 >> 5) & 0x07)) + 'A' - 1;
                    char letter3 = (byte2 & 0x1F) + 'A' - 1;
                    
                    addRow(tr("%1/Manufacturer ID").arg(connector), QString("%1%2%3").arg(letter1).arg(letter2).arg(letter3));
                }
                
                // Parse product code (bytes 10-11, little-endian)
                if (edidData.size() >= 18) {
                    unsigned short productCode = (unsigned char)edidData[10] | ((unsigned char)edidData[11] << 8);
                    addRow(tr("%1/Product Code").arg(connector), QString("0x%1").arg(productCode, 4, 16, QChar('0')));
                }
                
                // Parse serial number (bytes 12-15, little-endian)
                if (edidData.size() >= 18) {
                    unsigned int serial = (unsigned char)edidData[12] | 
                                        ((unsigned char)edidData[13] << 8) |
                                        ((unsigned char)edidData[14] << 16) |
                                        ((unsigned char)edidData[15] << 24);
                    if (serial != 0) {
                        addRow(tr("%1/Serial Number").arg(connector), QString::number(serial));
                    }
                }
                
                // Parse week/year (bytes 16-17)
                if (edidData.size() >= 18) {
                    unsigned char week = edidData[16];
                    unsigned char year = edidData[17];
                    if (year != 0) {
                        addRow(tr("%1/Manufacture Date").arg(connector), 
                              QString("Week %1, Year %2").arg(week).arg(1990 + year));
                    }
                }
                
                // Try to find display name in descriptor blocks
                for (int offset : {54, 72, 90, 108}) {
                    if (edidData.size() >= offset + 18) {
                        if (edidData[offset + 3] == (char)0xFC) { // Display name descriptor
                            QString name = QString::fromLatin1(edidData.mid(offset + 5, 13)).trimmed();
                            addRow(tr("%1/Display Name").arg(connector), name);
                        }
                        if (edidData[offset + 3] == (char)0xFE) { // Display string descriptor
                            QString str = QString::fromLatin1(edidData.mid(offset + 5, 13)).trimmed();
                            addRow(tr("%1/Display String").arg(connector), str);
                        }
                    }
                }
                
                // Format EDID hex with spaces every 2 bytes for better wrapping
                QString edidHex = edidData.left(128).toHex();
                QString formattedEdid;
                for (int i = 0; i < edidHex.length(); i += 2) {
                    if (i > 0) formattedEdid += " ";
                    formattedEdid += edidHex.mid(i, 2);
                }
                addRow(tr("%1/EDID (first 128 bytes hex)").arg(connector), formattedEdid);
            }
        }
        
        addRow("", ""); // Spacer
    }
}
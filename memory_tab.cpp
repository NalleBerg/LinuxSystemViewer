#include "memory_tab.h"
#include <sys/sysinfo.h>
#include <QFrame>
#include <QProcess>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QScrollArea>
// Needed for sysfs/proc reads in geek dialog
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QGuiApplication>
#include <QClipboard>
#include <QFileDialog>

MemoryTab::MemoryTab(QWidget* parent) : QWidget(parent)
{
    // Headline and Geek button on same line
    QHBoxLayout* headlineLayout = new QHBoxLayout();
    QLabel* headline = new QLabel(tr("Memory"));
    headline->setStyleSheet("font-size: 15px; font-weight: bold; color: #222; margin-bottom: 0px;");
    geekButton = new QPushButton(tr("Geek Mode"), this);
    geekButton->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; border: none; padding: 4px 10px; border-radius: 4px; font-weight: bold; font-size: 11px; min-width: 80px; max-height: 22px;}"
        "QPushButton:hover { background-color: #2980b9; }"
    );
    connect(geekButton, &QPushButton::clicked, this, &MemoryTab::showGeekMode);
    headlineLayout->addWidget(headline);
    headlineLayout->addStretch();
    headlineLayout->addWidget(geekButton);

    // RAM widgets
    ramTotalLabel = new QLabel(this);
    ramTotalLabel->setStyleSheet("font-weight: bold; font-size: 11px; color: #222; margin-bottom: 0px;");
    ramUsageBar = new QProgressBar(this);
    ramUsedLabel = new QLabel(this);
    ramFreeLabel = new QLabel(this);

    ramUsageBar->setMinimum(0);
    ramUsageBar->setMaximum(100);
    ramUsageBar->setTextVisible(false);
    ramUsageBar->setFixedHeight(10);

    QFont smallBoldFont = ramUsedLabel->font();
    smallBoldFont.setPointSize(9);
    smallBoldFont.setBold(true);

    ramUsedLabel->setFont(smallBoldFont);
    ramFreeLabel->setFont(smallBoldFont);

    QPalette darkGray;
    darkGray.setColor(QPalette::WindowText, QColor("#333"));
    ramUsedLabel->setPalette(darkGray);
    ramFreeLabel->setPalette(darkGray);

    // SWAP widgets
    swapTotalLabel = new QLabel(this);
    swapTotalLabel->setStyleSheet("font-weight: bold; font-size: 11px; color: #222; margin-bottom: 0px;");
    swapUsageBar = new QProgressBar(this);
    swapUsedLabel = new QLabel(this);
    swapFreeLabel = new QLabel(this);

    swapUsageBar->setMinimum(0);
    swapUsageBar->setMaximum(100);
    swapUsageBar->setTextVisible(false);
    swapUsageBar->setFixedHeight(10);

    swapUsedLabel->setFont(smallBoldFont);
    swapFreeLabel->setFont(smallBoldFont);
    swapUsedLabel->setPalette(darkGray);
    swapFreeLabel->setPalette(darkGray);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(1);
    mainLayout->setContentsMargins(20, 8, 20, 8);

    mainLayout->addLayout(headlineLayout);

    // RAM section
    mainLayout->addWidget(ramTotalLabel);

    QHBoxLayout* ramBarLayout = new QHBoxLayout();
    ramBarLayout->setSpacing(1);
    ramBarLayout->addWidget(ramUsageBar, 1);
    mainLayout->addLayout(ramBarLayout);

    QHBoxLayout* ramLabelsLayout = new QHBoxLayout();
    ramLabelsLayout->setSpacing(1);
    ramLabelsLayout->addWidget(ramUsedLabel, 0, Qt::AlignLeft);
    ramLabelsLayout->addStretch(1);
    ramLabelsLayout->addWidget(ramFreeLabel, 0, Qt::AlignRight);
    mainLayout->addLayout(ramLabelsLayout);

    // Separator
    QFrame* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(sep);

    // SWAP section
    mainLayout->addWidget(swapTotalLabel);

    QHBoxLayout* swapBarLayout = new QHBoxLayout();
    swapBarLayout->setSpacing(1);
    swapBarLayout->addWidget(swapUsageBar, 1);
    mainLayout->addLayout(swapBarLayout);

    QHBoxLayout* swapLabelsLayout = new QHBoxLayout();
    swapLabelsLayout->setSpacing(1);
    swapLabelsLayout->addWidget(swapUsedLabel, 0, Qt::AlignLeft);
    swapLabelsLayout->addStretch(1);
    swapLabelsLayout->addWidget(swapFreeLabel, 0, Qt::AlignRight);
    mainLayout->addLayout(swapLabelsLayout);

    setStyleSheet(
        "QLabel { font-size: 11px; color: #2c3e50; }"
        "QProgressBar {"
        " border: 1px solid #34495e;"
        " border-radius: 5px;"
        " background: #eee;"
        " min-height: 6px;"
        " max-height: 12px;"
        "}"
        "QProgressBar::chunk {"
        " border-radius: 5px;"
        "}"
    );

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MemoryTab::updateMemoryInfo);
    timer->start(1000);

    updateMemoryInfo();
}

void MemoryTab::updateMemoryInfo()
{
    struct sysinfo info;
    sysinfo(&info);

    // RAM
    double ramTotalGB = info.totalram * info.mem_unit / (1024.0 * 1024 * 1024);
    double ramFreeGB = info.freeram * info.mem_unit / (1024.0 * 1024 * 1024);
    double ramUsedGB = ramTotalGB - ramFreeGB;
    int ramPercent = ramTotalGB > 0 ? (int)((ramUsedGB / ramTotalGB) * 100) : 0;

    ramTotalLabel->setText(tr("RAM Total: %1 GB").arg(QString::number(ramTotalGB, 'f', 3)));
    ramUsageBar->setValue(ramPercent);
    setBarColor(ramUsageBar, ramPercent);

    ramUsedLabel->setText(tr("Used: %1 GB").arg(QString::number(ramUsedGB, 'f', 3)));
    ramFreeLabel->setText(tr("Free: %1 GB (%2%)").arg(QString::number(ramFreeGB, 'f', 3)).arg(100 - ramPercent));

    // SWAP
    double swapTotalGB = info.totalswap * info.mem_unit / (1024.0 * 1024 * 1024);
    double swapFreeGB = info.freeswap * info.mem_unit / (1024.0 * 1024 * 1024);
    double swapUsedGB = swapTotalGB - swapFreeGB;
    int swapPercent = swapTotalGB > 0 ? (int)((swapUsedGB / swapTotalGB) * 100) : 0;

    swapTotalLabel->setText(tr("SWAP Total: %1 GB").arg(QString::number(swapTotalGB, 'f', 3)));
    swapUsageBar->setValue(swapPercent);
    setBarColor(swapUsageBar, swapPercent);

    swapUsedLabel->setText(tr("Used: %1 GB").arg(QString::number(swapUsedGB, 'f', 3)));
    swapFreeLabel->setText(tr("Free: %1 GB (%2%)").arg(QString::number(swapFreeGB, 'f', 3)).arg(100 - swapPercent));
}

void MemoryTab::setBarColor(QProgressBar* bar, int percent)
{
    QString color;
    if (percent < 75)
        color = "#4caf50"; // green
    else if (percent < 90)
        color = "#ffeb3b"; // yellow
    else
        color = "#f44336"; // red

    bar->setStyleSheet(QString(
        "QProgressBar {"
        " border: 1px solid #34495e;"
        " border-radius: 5px;"
        " background: #eee;"
        " min-height: 6px;"
        " max-height: 12px;"
        "}"
        "QProgressBar::chunk {"
        " background-color: %1;"
        " border-radius: 5px;"
        "}"
    ).arg(color));
}

void MemoryTab::showGeekMode()
{
    GeekMemoryDialog dlg(this);
    dlg.exec();
}

// --- GeekMemoryDialog ---

GeekMemoryDialog::GeekMemoryDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Memory - Geek Mode"));
    setModal(true);
    resize(600, 400);

    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* titleLabel = new QLabel(tr("RAM Technical Details"));
    titleLabel->setStyleSheet(
        "QLabel {"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "  color: #2c3e50;"
        "  margin-bottom: 10px;"
        "}"
    );
    layout->addWidget(titleLabel);

    table = new QTableWidget();
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList() << tr("Property") << tr("Value"));
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "  background-color: #34495e;"
        "  color: white;"
        "  font-weight: bold;"
        "  padding: 8px;"
        "  border: 1px solid #2c3e50;"
        "}"
    );
    table->setStyleSheet(
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
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->setColumnWidth(0, 250);
    table->setSelectionBehavior(QAbstractItemView::SelectItems);
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);

    // Make table scrollable
    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidget(table);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(250);
    layout->addWidget(scrollArea);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    // Copy and Save buttons (Copy: readable UTF-8; Save: CSV)
    QPushButton* copyBtn = new QPushButton(tr("Copy"));
    QPushButton* saveBtn = new QPushButton(tr("Save..."));
    buttonBox->addButton(copyBtn, QDialogButtonBox::ActionRole);
    buttonBox->addButton(saveBtn, QDialogButtonBox::ActionRole);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);

    connect(copyBtn, &QPushButton::clicked, [this]() {
        QString all;
        for (int r = 0; r < table->rowCount(); ++r) {
            QString prop = table->item(r,0) ? table->item(r,0)->text() : QString();
            QString val = table->item(r,1) ? table->item(r,1)->text() : QString();
            all += prop + ": " + val + "\n";
        }
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(all, QClipboard::Clipboard);
    });

    connect(saveBtn, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save Memory Info"), "memory-info.csv", tr("CSV Files (*.csv);;All Files (*)"));
        if (fileName.isEmpty()) return;
        auto esc = [](const QString &s)->QString {
            QString out = s;
            out.replace('"', "\"");
            if (out.contains(',') || out.contains('\n') || out.contains('"')) out = '"' + out + '"';
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
}

void GeekMemoryDialog::fillTable()
{
    table->setRowCount(0);

    // Helper to add rows
    int row = 0;
    auto addRow = [&](const QString& prop, const QString& val) {
        table->insertRow(row);
        QTableWidgetItem* propItem = new QTableWidgetItem(prop);
        QFont boldFont;
        boldFont.setBold(true);
        propItem->setFont(boldFont);
        propItem->setForeground(QColor("#000000"));
        table->setItem(row, 0, propItem);
        QTableWidgetItem* valItem = new QTableWidgetItem(val);
        valItem->setForeground(QColor("#1f1971"));
        table->setItem(row, 1, valItem);
        table->resizeRowToContents(row);
        row++;
    };

    // 1) Basic memory totals from /proc/meminfo
    QFile meminfo("/proc/meminfo");
    QString memContent;
    if (meminfo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&meminfo);
        memContent = in.readAll();
        meminfo.close();

        // Extract some useful fields
        auto extractField = [&](const QString& name)->QString{
            for (const QString& line : memContent.split('\n')) {
                if (line.startsWith(name)) {
                    return line.section(':',1).trimmed();
                }
            }
            return QString();
        };

        QString total = extractField("MemTotal");
        QString free = extractField("MemFree");
        QString available = extractField("MemAvailable");
        QString swapTotal = extractField("SwapTotal");

        addRow(tr("MemTotal (/proc/meminfo)"), total.isEmpty() ? tr("Unknown") : total);
        addRow(tr("MemFree (/proc/meminfo)"), free.isEmpty() ? tr("Unknown") : free);
        addRow(tr("MemAvailable (/proc/meminfo)"), available.isEmpty() ? tr("Unknown") : available);
        addRow(tr("SwapTotal (/proc/meminfo)"), swapTotal.isEmpty() ? tr("Unknown") : swapTotal);
    } else {
        addRow(tr("/proc/meminfo"), tr("Could not open /proc/meminfo"));
    }

    // 2) Memory blocks (hotplug) from sysfs
    QDir memDir("/sys/devices/system/memory");
    int memBlocks = 0;
    if (memDir.exists()) {
        QStringList entries = memDir.entryList(QStringList() << "memory*", QDir::Dirs | QDir::NoDotAndDotDot);
        memBlocks = entries.size();
        addRow(tr("Memory block entries (/sys/devices/system/memory)"), QString::number(memBlocks));
    } else {
        addRow(tr("Memory block entries"), tr("Not available"));
    }

    // 3) NUMA nodes
    QDir nodeDir("/sys/devices/system/node");
    if (nodeDir.exists()) {
        QStringList nodes = nodeDir.entryList(QStringList() << "node*", QDir::Dirs | QDir::NoDotAndDotDot);
        addRow(tr("NUMA nodes (count)"), QString::number(nodes.size()));
    } else {
        addRow(tr("NUMA nodes (count)"), tr("Not available"));
    }

    // 4) Try to detect DMI memory device entries (type 17) via sysfs if present
    QDir dmiDir("/sys/firmware/dmi/entries");
    int dmiMemDevices = 0;
    if (dmiDir.exists()) {
        QStringList entries = dmiDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        // First pass: count type 17 entries
        for (const QString& e : entries) {
            QString typePath = QString("/sys/firmware/dmi/entries/%1/type").arg(e);
            QFile typeF(typePath);
            if (typeF.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString t = QTextStream(&typeF).readLine().trimmed();
                typeF.close();
                if (t == "17") dmiMemDevices++;
            }
        }
        addRow(tr("DMI memory device entries (type 17)"), dmiMemDevices > 0 ? QString::number(dmiMemDevices) : tr("None detected"));

        // If entries exist, parse each type 17 entry's raw data and extract strings
        if (dmiMemDevices > 0) {
            int slotIndex = 1;
            for (const QString& e : entries) {
                QString typePath = QString("/sys/firmware/dmi/entries/%1/type").arg(e);
                QFile typeF(typePath);
                if (!typeF.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
                QString t = QTextStream(&typeF).readLine().trimmed();
                typeF.close();
                if (t != "17") continue;

                // Read raw blob
                QString rawPath = QString("/sys/firmware/dmi/entries/%1/raw").arg(e);
                QFile rawF(rawPath);
                if (!rawF.open(QIODevice::ReadOnly)) {
                    addRow(tr("Slot %1 (DMI entry %2)" ).arg(slotIndex).arg(e), tr("Could not open raw DMI data"));
                    slotIndex++;
                    continue;
                }
                QByteArray raw = rawF.readAll();
                rawF.close();

                if (raw.size() < 4) {
                    addRow(tr("Slot %1 (DMI entry %2)").arg(slotIndex).arg(e), tr("Raw DMI data too short"));
                    slotIndex++;
                    continue;
                }

                int length = static_cast<unsigned char>(raw[1]);
                if (length > raw.size()) length = raw.size();
                QByteArray formatted = raw.left(length);

                // Extract trailing NUL-separated strings
                QList<QString> strings;
                int i = length;
                while (i < raw.size()) {
                    int j = raw.indexOf('\0', i);
                    if (j == -1) break;
                    if (j == i) { // double NUL terminator
                        break;
                    }
                    QByteArray s = raw.mid(i, j - i);
                    strings.append(QString::fromLocal8Bit(s));
                    i = j + 1;
                }

                addRow(tr("Slot %1 (DMI entry %2)").arg(slotIndex).arg(e), tr(""));
                for (int si = 0; si < strings.size(); ++si) {
                    addRow(tr("Slot %1 - String %2").arg(slotIndex).arg(si+1), strings[si]);
                }

                // Also show formatted bytes as hex for low-level inspection
                QString hex;
                for (int k = 0; k < formatted.size(); ++k) {
                    hex += QString::asprintf("%02x ", static_cast<unsigned char>(formatted[k]));
                }
                addRow(tr("Slot %1 - formatted bytes (hex)").arg(slotIndex), hex.trimmed());

                slotIndex++;
            }
        }
    } else {
        addRow(tr("DMI memory device entries"), tr("Not available"));
    }
}
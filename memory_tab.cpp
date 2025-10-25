#include "memory_tab.h"
#include <sys/sysinfo.h>
#include <QFrame>
#include <QProcess>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QScrollArea>

MemoryTab::MemoryTab(QWidget* parent) : QWidget(parent)
{
    // Headline and Geek button on same line
    QHBoxLayout* headlineLayout = new QHBoxLayout();
    QLabel* headline = new QLabel("Memory");
    headline->setStyleSheet("font-size: 15px; font-weight: bold; color: #222; margin-bottom: 0px;");
    geekButton = new QPushButton("Geek Mode", this);
    geekButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #3498db;"
        "  color: white;"
        "  border: none;"
        "  padding: 4px 10px;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "  font-size: 11px;"
        "  min-width: 80px;"
        "  max-height: 22px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #2980b9;"
        "}"
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

    ramTotalLabel->setText(QString("RAM Total: %1 GB").arg(QString::number(ramTotalGB, 'f', 3)));
    ramUsageBar->setValue(ramPercent);
    setBarColor(ramUsageBar, ramPercent);

    ramUsedLabel->setText(QString("Used: %1 GB").arg(QString::number(ramUsedGB, 'f', 3)));
    ramFreeLabel->setText(QString("Free: %1 GB (%2%)").arg(QString::number(ramFreeGB, 'f', 3)).arg(100 - ramPercent));

    // SWAP
    double swapTotalGB = info.totalswap * info.mem_unit / (1024.0 * 1024 * 1024);
    double swapFreeGB = info.freeswap * info.mem_unit / (1024.0 * 1024 * 1024);
    double swapUsedGB = swapTotalGB - swapFreeGB;
    int swapPercent = swapTotalGB > 0 ? (int)((swapUsedGB / swapTotalGB) * 100) : 0;

    swapTotalLabel->setText(QString("SWAP Total: %1 GB").arg(QString::number(swapTotalGB, 'f', 3)));
    swapUsageBar->setValue(swapPercent);
    setBarColor(swapUsageBar, swapPercent);

    swapUsedLabel->setText(QString("Used: %1 GB").arg(QString::number(swapUsedGB, 'f', 3)));
    swapFreeLabel->setText(QString("Free: %1 GB (%2%)").arg(QString::number(swapFreeGB, 'f', 3)).arg(100 - swapPercent));
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
    setWindowTitle("Memory - Geek Mode");
    setModal(true);
    resize(600, 400);

    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* titleLabel = new QLabel("RAM Technical Details");
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
    table->setHorizontalHeaderLabels(QStringList() << "Property" << "Value");
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
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);

    fillTable();
}

void GeekMemoryDialog::fillTable()
{
    table->setRowCount(0);

    QProcess proc;
    proc.start("dmidecode", QStringList() << "-t" << "memory");
    proc.waitForFinished(2000);
    QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());

    int slotCount = 0, freeSlots = 0, maxModuleSize = 0, totalMaxRam = 0;
    QString ramType, ramSpeed;
    QList<QMap<QString, QString>> devices;

    QStringList lines = output.split('\n');
    QMap<QString, QString> currentDevice;
    bool inDevice = false;
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.startsWith("Memory Device")) {
            if (!currentDevice.isEmpty()) devices.append(currentDevice);
            currentDevice.clear();
            inDevice = true;
        } else if (inDevice && !trimmed.isEmpty() && trimmed.contains(':')) {
            QStringList parts = trimmed.split(':');
            if (parts.size() == 2)
                currentDevice[parts[0].trimmed()] = parts[1].trimmed();
        }
    }
    if (!currentDevice.isEmpty()) devices.append(currentDevice);

    slotCount = devices.size();
    for (const auto& dev : devices) {
        QString sizeStr = dev.value("Size");
        if (sizeStr == "No Module Installed") {
            freeSlots++;
        } else if (sizeStr.endsWith("MB")) {
            totalMaxRam += sizeStr.remove("MB").trimmed().toInt();
        } else if (sizeStr.endsWith("GB")) {
            totalMaxRam += sizeStr.remove("GB").trimmed().toInt() * 1024;
        }
        if (ramType.isEmpty()) ramType = dev.value("Type");
        if (ramSpeed.isEmpty()) ramSpeed = dev.value("Configured Clock Speed");
        if (maxModuleSize == 0 && dev.contains("Maximum Capacity")) {
            QString maxStr = dev.value("Maximum Capacity");
            if (maxStr.endsWith("MB")) maxModuleSize = maxStr.remove("MB").trimmed().toInt();
            else if (maxStr.endsWith("GB")) maxModuleSize = maxStr.remove("GB").trimmed().toInt() * 1024;
        }
    }

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

    addRow("RAM Slots", QString::number(slotCount));
    addRow("Free Slots", QString::number(freeSlots));
    addRow("Max Module Size", maxModuleSize > 0 ? QString("%1 MB").arg(maxModuleSize) : "Unknown");
    addRow("Total Installed RAM", totalMaxRam > 0 ? QString("%1 MB").arg(totalMaxRam) : "Unknown");
    addRow("RAM Type", ramType.isEmpty() ? "Unknown" : ramType);
    addRow("RAM Speed", ramSpeed.isEmpty() ? "Unknown" : ramSpeed);

    // Add per-slot info
    for (int i = 0; i < devices.size(); ++i) {
        const auto& dev = devices[i];
        QString slotInfo = QString("Slot %1: %2, %3, %4")
            .arg(i + 1)
            .arg(dev.value("Size", "No Module"))
            .arg(dev.value("Type", "Unknown"))
            .arg(dev.value("Configured Clock Speed", dev.value("Speed", "Unknown")));
        addRow(QString("Slot %1 Info").arg(i + 1), slotInfo);
    }
}
#include "cpu_tab.h"
#include "cpu.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QScrollArea>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QSet>
#include <QClipboard>
#include <QGuiApplication>
#include <QFileDialog>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QTableWidgetItem>
#include <QFont>
#include <QJsonObject>
#include <QTableWidget>

CPUTab::CPUTab(QWidget* parent)
    : QWidget(parent)
{
    // Headline and Geek button
    QHBoxLayout* headlineLayout = new QHBoxLayout();
    QLabel* headline = new QLabel("CPU");
    headline->setStyleSheet("font-size: 15px; font-weight: bold; color: #222; margin-bottom: 0px;");
    geekButton = new QPushButton("Geek Mode", this);
    geekButton->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; border: none; padding: 4px 10px; border-radius: 4px; font-weight: bold; font-size: 11px; min-width: 80px; max-height: 22px;}"
        "QPushButton:hover { background-color: #2980b9; }"
    );
    connect(geekButton, &QPushButton::clicked, this, &CPUTab::showGeekMode);
    headlineLayout->addWidget(headline);
    headlineLayout->addStretch();
    headlineLayout->addWidget(geekButton);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(headlineLayout);

    // Table
    tableWidget = new QTableWidget();
    tableWidget->setColumnCount(4);
    tableWidget->setHorizontalHeaderLabels(getCpuHeaders());
    tableWidget->verticalHeader()->setVisible(false);
    styleCpuTable(tableWidget);
    tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    // Scroll area
    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidget(tableWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(220);
    mainLayout->addWidget(scrollArea);

    // Populate table using cpu helper
    loadCpuInformation(tableWidget, QJsonObject());
}

void CPUTab::showGeekMode()
{
    GeekCpuDialog dlg(this);
    dlg.exec();
}

// --- GeekCpuDialog ---

GeekCpuDialog::GeekCpuDialog(QWidget* parent)
    : QDialog(parent)
    , refreshTimer(new QTimer(this))
{
    setWindowTitle("CPU - Geek Mode");
    setModal(true);
    resize(700, 500);

    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* titleLabel = new QLabel("CPU Technical Details");
    titleLabel->setStyleSheet("font-size:16px; font-weight:bold; color:#2c3e50; margin-bottom:10px;");
    layout->addWidget(titleLabel);

    table = new QTableWidget();
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList() << "Property" << "Value");
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: #34495e; color: white; font-weight: bold; padding: 8px; border: 1px solid #2c3e50; }");
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidget(table);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(350);
    layout->addWidget(scrollArea);

    // Buttons: Copy, Save, Close
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    QPushButton* copyBtn = new QPushButton("Copy");
    QPushButton* saveBtn = new QPushButton("Save...");
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
        clipboard->setText(all);
    });

    // Note: run-as-root functionality removed to avoid accidental termination when
    // the user is not running as root. Elevation is handled separately via the
    // `lsv-elevate` helper in `priv/` if desired.

    connect(saveBtn, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, "Save CPU Info", "cpu-info.txt", "Text Files (*.txt);;All Files (*)");
        if (!fileName.isEmpty()) {
            QFile out(fileName);
            if (out.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream ts(&out);
                for (int r = 0; r < table->rowCount(); ++r) {
                    QString prop = table->item(r,0) ? table->item(r,0)->text() : QString();
                    QString val = table->item(r,1) ? table->item(r,1)->text() : QString();
                    ts << prop << ": " << val << "\n";
                }
                out.close();
            }
        }
    });

    fillTable();

    // Auto-refresh every second while visible
    refreshTimer->setInterval(1000);
    connect(refreshTimer, &QTimer::timeout, this, &GeekCpuDialog::fillTable);
}

void GeekCpuDialog::showEvent(QShowEvent* ev)
{
    QDialog::showEvent(ev);
    if (refreshTimer) refreshTimer->start();
}

void GeekCpuDialog::hideEvent(QHideEvent* ev)
{
    if (refreshTimer) refreshTimer->stop();
    QDialog::hideEvent(ev);
}

void GeekCpuDialog::fillTable()
{
    table->setRowCount(0);
    int row = 0;

    auto addRow = [&](const QString& prop, const QString& val){
        table->insertRow(row);
        QTableWidgetItem* p = new QTableWidgetItem(prop);
        QFont bold; bold.setBold(true); p->setFont(bold);
        table->setItem(row, 0, p);
        QTableWidgetItem* v = new QTableWidgetItem(val);
        table->setItem(row, 1, v);
        table->resizeRowToContents(row);
        ++row;
    };

    // Read /proc/cpuinfo
    QFile file("/proc/cpuinfo");
    QString content;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        content = in.readAll();
        file.close();

        // Add some key values
        QString model;
        QString vendor;
        QString cores;
        QString cpuMHz;

        for (const QString& line : content.split('\n')) {
            if (line.startsWith("model name")) {
                model = line.section(':', 1).trimmed();
            } else if (line.startsWith("vendor_id")) {
                vendor = line.section(':', 1).trimmed();
            } else if (line.startsWith("cpu cores")) {
                cores = line.section(':', 1).trimmed();
            } else if (line.startsWith("cpu MHz")) {
                cpuMHz = line.section(':', 1).trimmed();
            }
        }

        addRow("Model", model.isEmpty() ? "Unknown" : model);
        addRow("Vendor", vendor.isEmpty() ? "Unknown" : vendor);
        addRow("CPU Cores", cores.isEmpty() ? "Unknown" : cores);
        addRow("CPU MHz", cpuMHz.isEmpty() ? "Unknown" : cpuMHz);

        // Add full /proc/cpuinfo as one cell
        addRow("/proc/cpuinfo", content.trimmed().left(20000));
    } else {
        addRow("/proc/cpuinfo", "Could not open /proc/cpuinfo");
    }

    // Parse topology and per-core frequencies from sysfs
    // Count CPUs
    int logicalCount = 0;
    for (const QString& line : content.split('\n')) {
        if (line.startsWith("processor")) logicalCount++;
    }
    addRow("Logical processors", QString::number(logicalCount));

    // Physical packages and cores
    // Collect physical ids and core ids
    QSet<QString> physIds;
    QSet<QString> coreIds;
    for (const QString& line : content.split('\n')) {
        if (line.startsWith("physical id")) physIds.insert(line.section(':',1).trimmed());
        if (line.startsWith("core id")) coreIds.insert(line.section(':',1).trimmed());
    }
    addRow("Physical packages", QString::number(physIds.size()));
    addRow("Unique core ids seen (per-logical sample)", QString::number(coreIds.size()));

    // Per-core current frequency (if available)
    QString freqSummary;
    int maxCpu = 0;
    // find max cpuN directory
    QDir sysCpuDir("/sys/devices/system/cpu");
    QStringList entries = sysCpuDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &e : entries) {
        if (e.startsWith("cpu")) {
            bool ok = false;
            int idx = e.mid(3).toInt(&ok);
            if (ok && idx > maxCpu) maxCpu = idx;
        }
    }
    if (maxCpu >= 0) {
        for (int cpu = 0; cpu <= maxCpu; ++cpu) {
            QString curFreqPath = QString("/sys/devices/system/cpu/cpu%1/cpufreq/scaling_cur_freq").arg(cpu);
            QString cpuinfoFreqPath = QString("/sys/devices/system/cpu/cpu%1/cpufreq/cpuinfo_cur_freq").arg(cpu);
            QString val;
            QFile f(curFreqPath);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                val = QTextStream(&f).readLine().trimmed();
                f.close();
            } else {
                QFile f2(cpuinfoFreqPath);
                if (f2.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    val = QTextStream(&f2).readLine().trimmed();
                    f2.close();
                }
            }
            if (!val.isEmpty()) {
                freqSummary += QString("cpu%1: %2 kHz\n").arg(cpu).arg(val);
            }
        }
        if (!freqSummary.isEmpty()) addRow("Per-core current frequencies (kHz)", freqSummary.trimmed());
    }

    // sysfs cpuinfo_max_freq/min_freq as additional info
    QFile maxf("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
    if (maxf.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addRow("cpuinfo_max_freq", QTextStream(&maxf).readLine().trimmed());
        maxf.close();
    }
    QFile minf("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq");
    if (minf.open(QIODevice::ReadOnly | QIODevice::Text)) {
        addRow("cpuinfo_min_freq", QTextStream(&minf).readLine().trimmed());
        minf.close();
    }
}

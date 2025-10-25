#include "pc_tab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QFile>
#include <QIODevice>
#include <QTableWidget>
#include <QDebug>

PCTab::PCTab(QWidget* parent)
    : TabWidgetBase("PC Info", "hostnamectl && cat /sys/class/dmi/id/* 2>/dev/null", true, "", parent)
{
    initializeTab();
}

QWidget* PCTab::createUserFriendlyView()
{
    QWidget* mainWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);

    QLabel* titleLabel = new QLabel("PC Information");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    mainLayout->addWidget(titleLabel);

    m_pcTable = new QTableWidget();
    setupTable(m_pcTable);
    mainLayout->addWidget(m_pcTable);

    return mainWidget;
}

void PCTab::setupTable(QTableWidget* table)
{
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList() << "Property" << "Value");
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSelectionBehavior(QAbstractItemView::SelectItems);
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setMinimumHeight(200);
}

void PCTab::addTableRow(QTableWidget* table, const QString& property, const QString& value)
{
    int row = table->rowCount();
    table->setRowCount(row + 1);
    table->setItem(row, 0, new QTableWidgetItem(property));
    table->setItem(row, 1, new QTableWidgetItem(value));
    table->resizeRowToContents(row);
}

void PCTab::parseOutput(const QString& output)
{
    m_pcTable->setRowCount(0);

    // Try to parse hostnamectl output
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        if (line.startsWith("Static hostname:"))
            m_pcName = line.section(':', 1).trimmed();
        else if (line.startsWith("Chassis:"))
            m_chassis = line.section(':', 1).trimmed();
        else if (line.startsWith("Machine ID:"))
            m_serial = line.section(':', 1).trimmed();
        else if (line.startsWith("Product Name:"))
            m_product = line.section(':', 1).trimmed();
        else if (line.startsWith("System Family:"))
            m_family = line.section(':', 1).trimmed();
        else if (line.startsWith("Manufacturer:"))
            m_manufacturer = line.section(':', 1).trimmed();
        // Other lines are skipped
    }

    // Fallbacks using DMI
    QFile file;
    file.setFileName("/sys/class/dmi/id/product_name");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        m_product = QString::fromLocal8Bit(file.readAll()).trimmed();

    file.setFileName("/sys/class/dmi/id/sys_vendor");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        m_manufacturer = QString::fromLocal8Bit(file.readAll()).trimmed();

    file.setFileName("/sys/class/dmi/id/chassis_type");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        m_chassis = QString::fromLocal8Bit(file.readAll()).trimmed();

    file.setFileName("/sys/class/dmi/id/product_family");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        m_family = QString::fromLocal8Bit(file.readAll()).trimmed();

    file.setFileName("/sys/class/dmi/id/product_serial");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        m_serial = QString::fromLocal8Bit(file.readAll()).trimmed();

    // PC Type (simple guess)
    m_pcType.clear();
    if (!m_chassis.isEmpty()) {
        bool ok = false;
        int chassisNum = m_chassis.toInt(&ok);
        if (ok) {
            switch (chassisNum) {
                case 3: m_pcType = "Desktop"; break;
                case 8: m_pcType = "Laptop"; break;
                case 10: m_pcType = "Notebook"; break;
                case 30: m_pcType = "Tablet"; break;
                default: m_pcType = QString("Type %1").arg(chassisNum);
            }
        } else {
            m_pcType = m_chassis;
        }
    }

    addTableRow(m_pcTable, "PC Type", m_pcType);
    addTableRow(m_pcTable, "PC Name", m_pcName);
    addTableRow(m_pcTable, "Manufacturer", m_manufacturer);
    addTableRow(m_pcTable, "Product", m_product);
    addTableRow(m_pcTable, "Serial", m_serial);
    addTableRow(m_pcTable, "Family", m_family);
    addTableRow(m_pcTable, "Chassis", m_chassis);
}
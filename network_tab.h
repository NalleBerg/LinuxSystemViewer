#ifndef NETWORK_TAB_H
#define NETWORK_TAB_H

#include "tab_widget_base.h"
#include "network.h"
#include "network_geek.h"
#include "gui_helpers.h"
#include <QTableWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProcess>

class NetworkTab : public TabWidgetBase
{
    Q_OBJECT
public:
    explicit NetworkTab(QWidget* parent = nullptr)
        : TabWidgetBase(tr("Network"), "", false, "", parent)
    {
        initializeTab();
    }

protected:
    QWidget* createUserFriendlyView() override
    {
        QWidget* w = new QWidget;
        QVBoxLayout* mainLayout = new QVBoxLayout(w);
        applyMainLayoutDefaults(mainLayout);

    // Headline + Geek button (use strict UI helper to ensure exact placement)
    QPushButton* geekButton = nullptr;
    QHBoxLayout* headlineLayout = createHeadlineWithGeek(w, tr("Network"), &geekButton);
    mainLayout->addLayout(headlineLayout);

        // Table (Property / Value)
        QTableWidget* table = new QTableWidget();
        table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList() << tr("Property") << tr("Value"));
        table->verticalHeader()->setVisible(false);
        styleNetworkTable(table);
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        mainLayout->addWidget(table);

    // Populate immediately
    loadNetworkInformation(table, QJsonObject());
    // Enable copy support on the main table for user pages
    enableTableCopy(table);

        // Geek dialog: opens the richer NetworkGeekDialog
        connect(geekButton, &QPushButton::clicked, this, [this, w]() {
            NetworkGeekDialog dlg(w);
            dlg.exec();
        });

        return w;
    }

    void parseOutput(const QString& output) override { Q_UNUSED(output); }
};

#endif // NETWORK_TAB_H

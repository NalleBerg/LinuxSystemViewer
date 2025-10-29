#ifndef NETWORK_TAB_H
#define NETWORK_TAB_H

#include "tab_widget_base.h"
#include "network.h"
#include "network_geek.h"
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
        : TabWidgetBase("Network", "", false, "", parent)
    {
        initializeTab();
    }

protected:
    QWidget* createUserFriendlyView() override
    {
        QWidget* w = new QWidget;
        QVBoxLayout* mainLayout = new QVBoxLayout(w);

        // Headline + Geek button
        QHBoxLayout* headlineLayout = new QHBoxLayout();
        QLabel* headline = new QLabel("Network");
        headline->setStyleSheet("font-size: 15px; font-weight: bold; color: #222; margin-bottom: 0px;");
        QPushButton* geekButton = new QPushButton("Geek Mode", w);
        geekButton->setStyleSheet(
            "QPushButton { background-color: #3498db; color: white; border: none; padding: 4px 10px; border-radius: 4px; font-weight: bold; font-size: 11px; min-width: 80px; max-height: 22px;}"
            "QPushButton:hover { background-color: #2980b9; }"
        );
        headlineLayout->addWidget(headline);
        headlineLayout->addStretch();
        headlineLayout->addWidget(geekButton);
        mainLayout->addLayout(headlineLayout);

        // Table (Property / Value)
        QTableWidget* table = new QTableWidget();
        table->setColumnCount(2);
        table->setHorizontalHeaderLabels(QStringList() << "Property" << "Value");
        table->verticalHeader()->setVisible(false);
        styleNetworkTable(table);
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        mainLayout->addWidget(table);

        // Populate immediately
        loadNetworkInformation(table, QJsonObject());

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

#ifndef PC_TAB_H
#define PC_TAB_H

#include "tab_widget_base.h"
#include <QTableWidget>
#include <QPushButton>

class PCTab : public TabWidgetBase
{
    Q_OBJECT

public:
    explicit PCTab(QWidget* parent = nullptr);

protected:
    QWidget* createUserFriendlyView() override;
    void parseOutput(const QString& output) override;

private:
    void setupTable(QTableWidget* table);
    void addTableRow(QTableWidget* table, const QString& property, const QString& value);

    QTableWidget* m_pcTable;
    QPushButton* m_refreshButton;

    // Info fields
    QString m_pcType;
    QString m_pcName;
    QString m_manufacturer;
    QString m_product;
    QString m_serial;
    QString m_chassis;
    QString m_family;
};

#endif // PC_TAB_H
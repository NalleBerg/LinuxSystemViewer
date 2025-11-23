#ifndef OS_TAB_H
#define OS_TAB_H

#include "tab_widget_base.h"
#include <QTableWidget>
#include <QPushButton>
#include <QDialog>
#include <QTimer>

class GeekOsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GeekOsDialog(QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* ev) override;
    void hideEvent(QHideEvent* ev) override;

private:
    QTableWidget* table;
    QTimer* refreshTimer;
    void fillTable();
};

class OSTab : public TabWidgetBase
{
    Q_OBJECT
public:
    explicit OSTab(const QString& tabName = "OS",
                   const QString& command = "",
                   bool showHeader = true,
                   const QString& headerText = "",
                   QWidget* parent = nullptr);

    QWidget* createUserFriendlyView() override;
    void parseOutput(const QString& output) override;

private:
    QTableWidget* tableWidget;
    QPushButton* geekButton = nullptr;
    void startLsbReleaseProcess();
    void fillTableWithOutput(const QString& output);

private slots:
    void showGeekMode();
};

#endif // OS_TAB_H
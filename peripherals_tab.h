#ifndef PERIPHERALS_TAB_H
#define PERIPHERALS_TAB_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QDialog>
#include <QTimer>

class GeekPeripheralsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GeekPeripheralsDialog(QWidget* parent = nullptr);
    void loadData();
    void fillTable();

protected:
    void showEvent(QShowEvent* ev) override;
    void hideEvent(QHideEvent* ev) override;

private:
    QTableWidget* table;
};

class PeripheralsTab : public QWidget
{
    Q_OBJECT
public:
    explicit PeripheralsTab(QWidget* parent = nullptr);

private:
    QTableWidget* tableWidget;
    QPushButton* geekButton;
    QTimer* refreshTimer;

    void showGeekMode();
    void refreshPeripherals();
};

#endif // PERIPHERALS_TAB_H
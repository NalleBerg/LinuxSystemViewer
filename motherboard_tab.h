#ifndef MOTHERBOARD_TAB_H
#define MOTHERBOARD_TAB_H

#include <QWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QDialog>

class GeekMotherboardDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GeekMotherboardDialog(QWidget* parent = nullptr);
    void fillTable();

private:
    QTableWidget* table;
};

class MotherboardTab : public QWidget
{
    Q_OBJECT
public:
    explicit MotherboardTab(QWidget* parent = nullptr);

private slots:
    void showGeekMode();

private:
    QTableWidget* tableWidget;
    QPushButton* geekButton;
};

#endif // MOTHERBOARD_TAB_H
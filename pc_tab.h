#ifndef PC_TAB_H
#define PC_TAB_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QDialog>
#include <QTimer>

class PCTab : public QWidget
{
    Q_OBJECT

public:
    explicit PCTab(QWidget* parent = nullptr);

private slots:
    void showGeekMode();

private:
    void loadPCInformation();
    QTableWidget* tableWidget;
    QPushButton* geekButton;
};

// Geek Mode Dialog
class GeekPCDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GeekPCDialog(QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* ev) override;
    void hideEvent(QHideEvent* ev) override;

private:
    void fillTable();
    QTableWidget* table;
    QTimer* refreshTimer;
};

#endif // PC_TAB_H
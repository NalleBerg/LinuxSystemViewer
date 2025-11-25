#ifndef PORTS_TAB_H
#define PORTS_TAB_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QDialog>
#include <QTimer>

class PortsTab : public QWidget
{
    Q_OBJECT

public:
    explicit PortsTab(QWidget* parent = nullptr);

private slots:
    void showGeekMode();

private:
    void loadPortsInformation();
    QTableWidget* tableWidget;
    QPushButton* geekButton;
};

// Geek Mode Dialog
class GeekPortsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GeekPortsDialog(QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* ev) override;
    void hideEvent(QHideEvent* ev) override;

private:
    void fillTable();
    QTableWidget* table;
    QTimer* refreshTimer;
};

#endif // PORTS_TAB_H
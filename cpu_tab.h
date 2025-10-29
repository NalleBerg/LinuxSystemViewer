#ifndef CPU_TAB_H
#define CPU_TAB_H

#include <QWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QDialog>

#include <QTimer>
class QShowEvent;
class QHideEvent;
class GeekCpuDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GeekCpuDialog(QWidget* parent = nullptr);
    void fillTable();


protected:
    void showEvent(QShowEvent* ev) override;
    void hideEvent(QHideEvent* ev) override;

private:
    QTableWidget* table;
    QTimer* refreshTimer;
};

class CPUTab : public QWidget
{
    Q_OBJECT
public:
    explicit CPUTab(QWidget* parent = nullptr);

private slots:
    void showGeekMode();
    void refreshCpuValues();

private:
    QTableWidget* tableWidget;
    QPushButton* geekButton;
    QTimer* refreshTimer;
protected:
    void showEvent(QShowEvent* ev) override;
    void hideEvent(QHideEvent* ev) override;
};

#endif // CPU_TAB_H

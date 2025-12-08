#ifndef MEMORY_TAB_H
#define MEMORY_TAB_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QDialog>
#include <QTimer>

class GeekMemoryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GeekMemoryDialog(QWidget* parent = nullptr);
    void fillTable();

protected:
    void showEvent(QShowEvent* ev) override;
    void hideEvent(QHideEvent* ev) override;

private:
    QTableWidget* table;
    QTimer* refreshTimer;
};

class MemoryTab : public QWidget
{
    Q_OBJECT
public:
    explicit MemoryTab(QWidget* parent = nullptr);

    void updateMemoryInfo();

private:
    QTableWidget* tableWidget;
    QPushButton* geekButton;
    QTimer* refreshTimer;

    void setBarColor(QProgressBar* bar, int percent);
    void showGeekMode();

private slots:
    void refreshMemoryValues();
};

#endif // MEMORY_TAB_H
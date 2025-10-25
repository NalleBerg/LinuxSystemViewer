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

private:
    QTableWidget* table;
};

class MemoryTab : public QWidget
{
    Q_OBJECT
public:
    explicit MemoryTab(QWidget* parent = nullptr);

    void updateMemoryInfo();

private:
    // RAM widgets
    QLabel* ramTotalLabel;
    QProgressBar* ramUsageBar;
    QLabel* ramUsedLabel;
    QLabel* ramFreeLabel;

    // SWAP widgets
    QLabel* swapTotalLabel;
    QProgressBar* swapUsageBar;
    QLabel* swapUsedLabel;
    QLabel* swapFreeLabel;

    QPushButton* geekButton;

    void setBarColor(QProgressBar* bar, int percent);
    void showGeekMode();
    QTimer* timer;
};

#endif // MEMORY_TAB_H
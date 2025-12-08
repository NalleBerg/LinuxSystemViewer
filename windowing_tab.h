#ifndef WINDOWING_TAB_H
#define WINDOWING_TAB_H

#include <QWidget>
#include <QTableWidget>
#include <QTimer>
#include <QPushButton>
#include <QDialog>

class WindowingGeekDialog;

class WindowingTab : public QWidget
{
    Q_OBJECT

public:
    explicit WindowingTab(QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* ev) override;
    void hideEvent(QHideEvent* ev) override;

private slots:
    void showGeekMode();
    void refreshValues();

private:
    void loadWindowingInfo();
    QTableWidget* tableWidget;
    QTimer* refreshTimer;
    QPushButton* geekButton;
};

// --- Geek Mode Dialog ---
class WindowingGeekDialog : public QDialog
{
    Q_OBJECT
public:
    explicit WindowingGeekDialog(QWidget* parent = nullptr);

private slots:
    void copyToClipboard();
    void saveToFile();

private:
    void fillTable();
    QTableWidget* table;
};

#endif // WINDOWING_TAB_H

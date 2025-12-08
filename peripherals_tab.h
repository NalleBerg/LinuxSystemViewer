#ifndef PERIPHERALS_TAB_H
#define PERIPHERALS_TAB_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QDialog>
#include <QTimer>
#include <QLabel>
#include <QStringList>

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
    void showSpinner();
    void hideSpinner();

private slots:
    void rescan();
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
    
    // Loading indicator for main tab
    QLabel* loadingLabel;
    
    void showGeekMode();
    void refreshPeripherals();
    void showMainTabSpinner();
    void hideMainTabSpinner();
    
    // Helper function to test camera accessibility
    bool isCameraAccessible(const QString& deviceName);
};

#endif // PERIPHERALS_TAB_H
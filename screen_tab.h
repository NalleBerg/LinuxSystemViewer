#ifndef SCREEN_TAB_H
#define SCREEN_TAB_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QDialog>
#include <QTimer>

class ScreenTab : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenTab(QWidget* parent = nullptr);

private slots:
    void showGeekMode();

private:
    void loadScreenInformation();
    QTableWidget* tableWidget;
    QPushButton* geekButton;
};

// Geek Mode Dialog
class GeekScreenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GeekScreenDialog(QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* ev) override;
    void hideEvent(QHideEvent* ev) override;

private:
    void fillTable();
    QTableWidget* table;
    QTimer* refreshTimer;
};

#endif // SCREEN_TAB_H
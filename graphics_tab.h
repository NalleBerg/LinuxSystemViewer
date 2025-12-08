#ifndef GRAPHICS_TAB_H
#define GRAPHICS_TAB_H

#include <QWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QDialog>

class GeekGraphicsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GeekGraphicsDialog(QWidget* parent = nullptr);
    void fillTable();

private:
    QTableWidget* table;
};

class GraphicsTab : public QWidget
{
    Q_OBJECT
public:
    explicit GraphicsTab(QWidget* parent = nullptr);

private slots:
    void showGeekMode();

private:
    QTableWidget* tableWidget;
    QPushButton* geekButton;
};

#endif // GRAPHICS_TAB_H
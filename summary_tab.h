#ifndef SUMMARY_TAB_H
#define SUMMARY_TAB_H

#include <QWidget>
#include <QTableWidget>

class SummaryTab : public QWidget
{
    Q_OBJECT

public:
    explicit SummaryTab(QWidget* parent = nullptr);

private:
    void loadSummaryInformation();
    
    QTableWidget* tableWidget;
};

#endif // SUMMARY_TAB_H
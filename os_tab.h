#ifndef OS_TAB_H
#define OS_TAB_H

#include "tab_widget_base.h"
#include <QTableWidget>

class OSTab : public TabWidgetBase
{
    Q_OBJECT
public:
    explicit OSTab(const QString& tabName = "OS",
                   const QString& command = "",
                   bool showHeader = true,
                   const QString& headerText = "",
                   QWidget* parent = nullptr);

    QWidget* createUserFriendlyView() override;
    void parseOutput(const QString& output) override;

private:
    QTableWidget* tableWidget;
    void startLsbReleaseProcess();
    void fillTableWithOutput(const QString& output);
};

#endif // OS_TAB_H
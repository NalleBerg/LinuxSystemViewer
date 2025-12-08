#ifndef GENERIC_TAB_H
#define GENERIC_TAB_H

#include "tab_widget_base.h"
#include <QTextEdit>

class GenericTab : public TabWidgetBase
{
    Q_OBJECT

public:
    explicit GenericTab(const QString& tabName, const QString& command, 
                       bool hasGeekMode = false, const QString& geekCommand = "", 
                       QWidget* parent = nullptr);

protected:
    QWidget* createUserFriendlyView() override;
    void parseOutput(const QString& output) override;

private:
    QTextEdit* m_outputDisplay;
};

#endif // GENERIC_TAB_H
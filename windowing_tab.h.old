#ifndef WINDOWING_TAB_H
#define WINDOWING_TAB_H

#include "tab_widget_base.h"
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

class WindowingTab : public TabWidgetBase
{
    Q_OBJECT

public:
    explicit WindowingTab(QWidget* parent = nullptr);

protected:
    QWidget* createUserFriendlyView() override;
    void parseOutput(const QString& output) override;

private:
    void createInfoSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout);
    
    QGroupBox* m_desktopSection;
    QLabel* m_desktopContent;
    
    QGroupBox* m_sessionSection;
    QLabel* m_sessionContent;
    
    QGroupBox* m_displayServerSection;
    QLabel* m_displayServerContent;
    
    QGroupBox* m_windowManagerSection;
    QLabel* m_windowManagerContent;
};

#endif // WINDOWING_TAB_H
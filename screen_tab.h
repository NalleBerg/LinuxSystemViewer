#ifndef SCREEN_TAB_H
#define SCREEN_TAB_H

#include "tab_widget_base.h"
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

class ScreenTab : public TabWidgetBase
{
    Q_OBJECT

public:
    explicit ScreenTab(QWidget* parent = nullptr);

protected:
    QWidget* createUserFriendlyView() override;
    void parseOutput(const QString& output) override;

private:
    void createInfoSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout);
    
    QGroupBox* m_displaysSection;
    QLabel* m_displaysContent;
    
    QGroupBox* m_resolutionSection;
    QLabel* m_resolutionContent;
    
    QGroupBox* m_refreshRateSection;
    QLabel* m_refreshRateContent;
    
    QGroupBox* m_orientationSection;
    QLabel* m_orientationContent;
};

#endif // SCREEN_TAB_H
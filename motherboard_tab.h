#ifndef MOTHERBOARD_TAB_H
#define MOTHERBOARD_TAB_H

#include "tab_widget_base.h"
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

class MotherboardTab : public TabWidgetBase
{
    Q_OBJECT

public:
    explicit MotherboardTab(QWidget* parent = nullptr);

protected:
    QWidget* createUserFriendlyView() override;
    void parseOutput(const QString& output) override;

private:
    void createInfoSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout);
    
    QGroupBox* m_systemBoardSection;
    QLabel* m_systemBoardContent;
    
    QGroupBox* m_chipsetSection;
    QLabel* m_chipsetContent;
    
    QGroupBox* m_biosSection;
    QLabel* m_biosContent;
    
    QGroupBox* m_expansionSlotsSection;
    QLabel* m_expansionSlotsContent;
};

#endif // MOTHERBOARD_TAB_H
#ifndef SUMMARY_TAB_H
#define SUMMARY_TAB_H

#include "tab_widget_base.h"
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

class SummaryTab : public TabWidgetBase
{
    Q_OBJECT

public:
    explicit SummaryTab(QWidget* parent = nullptr);

protected:
    QWidget* createUserFriendlyView() override;
    void parseOutput(const QString& output) override;

private:
    void createHardwareSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout);
    
    QGroupBox* m_systemOverview;
    QLabel* m_overviewContent;
    
    QGroupBox* m_cpuSection;
    QLabel* m_cpuContent;
    
    QGroupBox* m_memorySection;
    QLabel* m_memoryContent;
    
    QGroupBox* m_storageSection;
    QLabel* m_storageContent;
    
    QGroupBox* m_networkSection;
    QLabel* m_networkContent;
    
    QGroupBox* m_graphicsSection;
    QLabel* m_graphicsContent;
};

#endif // SUMMARY_TAB_H
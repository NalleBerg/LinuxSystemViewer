#ifndef STORAGE_TAB_H
#define STORAGE_TAB_H

#include "tab_widget_base.h"
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

class StorageTab : public TabWidgetBase
{
    Q_OBJECT

public:
    explicit StorageTab(QWidget* parent = nullptr);

protected:
    QWidget* createUserFriendlyView() override;
    void parseOutput(const QString& output) override;

private:
    void createInfoSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout);
    
    QGroupBox* m_diskDrivesSection;
    QLabel* m_diskDrivesContent;
    
    QGroupBox* m_partitionsSection;
    QLabel* m_partitionsContent;
    
    QGroupBox* m_mountPointsSection;
    QLabel* m_mountPointsContent;
    
    QGroupBox* m_diskUsageSection;
    QLabel* m_diskUsageContent;
};

#endif // STORAGE_TAB_H
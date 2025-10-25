#ifndef PERIPHERALS_TAB_H
#define PERIPHERALS_TAB_H

#include "tab_widget_base.h"
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

class PeripheralsTab : public TabWidgetBase
{
    Q_OBJECT

public:
    explicit PeripheralsTab(QWidget* parent = nullptr);

protected:
    QWidget* createUserFriendlyView() override;
    void parseOutput(const QString& output) override;

private:
    void createInfoSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout);
    
    QGroupBox* m_usbDevicesSection;
    QLabel* m_usbDevicesContent;
    
    QGroupBox* m_inputDevicesSection;
    QLabel* m_inputDevicesContent;
    
    QGroupBox* m_storageDevicesSection;
    QLabel* m_storageDevicesContent;
    
    QGroupBox* m_networkDevicesSection;
    QLabel* m_networkDevicesContent;
};

#endif // PERIPHERALS_TAB_H
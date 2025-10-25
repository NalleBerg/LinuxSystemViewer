#ifndef PORTS_TAB_H
#define PORTS_TAB_H

#include "tab_widget_base.h"
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

class PortsTab : public TabWidgetBase
{
    Q_OBJECT

public:
    explicit PortsTab(QWidget* parent = nullptr);

protected:
    QWidget* createUserFriendlyView() override;
    void parseOutput(const QString& output) override;

private:
    void createInfoSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout);
    
    QGroupBox* m_usbPortsSection;
    QLabel* m_usbPortsContent;
    
    QGroupBox* m_serialPortsSection;
    QLabel* m_serialPortsContent;
    
    QGroupBox* m_pciPortsSection;
    QLabel* m_pciPortsContent;
    
    QGroupBox* m_portStatusSection;
    QLabel* m_portStatusContent;
};

#endif // PORTS_TAB_H
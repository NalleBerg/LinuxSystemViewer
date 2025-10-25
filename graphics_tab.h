#ifndef GRAPHICS_TAB_H
#define GRAPHICS_TAB_H

#include "tab_widget_base.h"
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

class GraphicsTab : public TabWidgetBase
{
    Q_OBJECT

public:
    explicit GraphicsTab(QWidget* parent = nullptr);

protected:
    QWidget* createUserFriendlyView() override;
    void parseOutput(const QString& output) override;

private:
    void createInfoSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout);
    
    QGroupBox* m_graphicsCardSection;
    QLabel* m_graphicsCardContent;
    
    QGroupBox* m_driverSection;
    QLabel* m_driverContent;
    
    QGroupBox* m_openglSection;
    QLabel* m_openglContent;
    
    QGroupBox* m_memorySection;
    QLabel* m_memoryContent;
};

#endif // GRAPHICS_TAB_H
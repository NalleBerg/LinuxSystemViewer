#ifndef ABOUT_TAB_H
#define ABOUT_TAB_H

#include "tab_widget_base.h"
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

class AboutTab : public TabWidgetBase
{
    Q_OBJECT

public:
    explicit AboutTab(QWidget* parent = nullptr);

protected:
    QWidget* createUserFriendlyView() override;
    void parseOutput(const QString& output) override;

private:
    void createInfoSection(const QString& title, QGroupBox** groupBox, QLabel** contentLabel, QVBoxLayout* parentLayout);
    
    QGroupBox* m_applicationSection;
    QLabel* m_applicationContent;
    
    QGroupBox* m_versionSection;
    QLabel* m_versionContent;
    
    QGroupBox* m_authorsSection;
    QLabel* m_authorsContent;
    
    QGroupBox* m_licenseSection;
    QLabel* m_licenseContent;
};

#endif // ABOUT_TAB_H